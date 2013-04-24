/*
 * gui-input.c - input functions (used by all GUI)
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-input.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-input.h"
#include "gui-buffer.h"
#include "gui-completion.h"
#include "gui-cursor.h"
#include "gui-history.h"
#include "gui-hotlist.h"
#include "gui-key.h"
#include "gui-line.h"
#include "gui-mouse.h"
#include "gui-window.h"


char *gui_input_clipboard = NULL;      /* clipboard content                 */


/*
 * Optimizes input buffer size by adding or deleting data block (predefined
 * size).
 */

void
gui_input_optimize_size (struct t_gui_buffer *buffer)
{
    int optimal_size;
    char *input_buffer2;

    if (buffer->input)
    {
        optimal_size = ((buffer->input_buffer_size / GUI_BUFFER_INPUT_BLOCK_SIZE) *
                        GUI_BUFFER_INPUT_BLOCK_SIZE) + GUI_BUFFER_INPUT_BLOCK_SIZE;
        if (buffer->input_buffer_alloc != optimal_size)
        {
            buffer->input_buffer_alloc = optimal_size;
            input_buffer2 = realloc (buffer->input_buffer, optimal_size);
            if (!input_buffer2)
            {
                if (buffer->input_buffer)
                {
                    free (buffer->input_buffer);
                    buffer->input_buffer = NULL;
                }
                return;
            }
            buffer->input_buffer = input_buffer2;
        }
    }
}

/*
 * Replaces full input by another string, trying to keep cursor position if new
 * string is long enough.
 */

void
gui_input_replace_input (struct t_gui_buffer *buffer, const char *new_input)
{
    int size, length;
    char *input_utf8;

    input_utf8 = strdup (new_input);
    if (input_utf8)
    {
        utf8_normalize (input_utf8, '?');

        size = strlen (input_utf8);
        length = utf8_strlen (input_utf8);

        /* compute new buffer size */
        buffer->input_buffer_size = size;
        buffer->input_buffer_length = length;
        gui_input_optimize_size (buffer);

        /* copy new string to input */
        strcpy (buffer->input_buffer, input_utf8);

        /* move cursor to the end of new input if it is now after the end */
        if (buffer->input_buffer_pos > buffer->input_buffer_length)
            buffer->input_buffer_pos = buffer->input_buffer_length;

        free (input_utf8);
    }
}

/*
 * Sends signal "input_paste_pending".
 */

void
gui_input_paste_pending_signal ()
{
    hook_signal_send ("input_paste_pending", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * Sends modifier and signal "input_text_changed".
 */

void
gui_input_text_changed_modifier_and_signal (struct t_gui_buffer *buffer,
                                            int save_undo,
                                            int stop_completion)
{
    char str_buffer[128], *new_input;

    if (!gui_cursor_mode)
    {
        if (save_undo)
            gui_buffer_undo_add (buffer);

        /* send modifier, and change input if needed */
        snprintf (str_buffer, sizeof (str_buffer),
                  "0x%lx", (long unsigned int)buffer);
        new_input = hook_modifier_exec (NULL,
                                        "input_text_content",
                                        str_buffer,
                                        (buffer->input_buffer) ?
                                        buffer->input_buffer : "");
        if (new_input)
        {
            if (!buffer->input_buffer
                || strcmp (new_input, buffer->input_buffer) != 0)
            {
                /* input has been changed by modifier, use it */
                gui_input_replace_input (buffer, new_input);
            }
            free (new_input);
        }
    }

    if (stop_completion && !gui_completion_freeze)
        gui_completion_stop (buffer->completion);

    /* send signal */
    hook_signal_send ("input_text_changed", WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Sends signal "input_text_cursor_moved".
 */

void
gui_input_text_cursor_moved_signal (struct t_gui_buffer *buffer)
{
    hook_signal_send ("input_text_cursor_moved", WEECHAT_HOOK_SIGNAL_POINTER,
                      buffer);
}

/*
 * Sends signal "input_search".
 */

void
gui_input_search_signal (struct t_gui_buffer *buffer)
{
    hook_signal_send ("input_search", WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Sets position in input line.
 */

void
gui_input_set_pos (struct t_gui_buffer *buffer, int pos)
{
    if ((pos >= 0) && (buffer->input_buffer_pos != pos))
    {
        buffer->input_buffer_pos = pos;
        if (buffer->input_buffer_pos > buffer->input_buffer_length)
            buffer->input_buffer_pos = buffer->input_buffer_length;
        gui_input_text_cursor_moved_signal (buffer);
    }
}

/*
 * Inserts a string into the input buffer.
 *
 * If pos == -1, string is inserted at cursor position.
 *
 * Returns number of chars inserted (may be different of strlen if UTF-8
 * string).
 */

int
gui_input_insert_string (struct t_gui_buffer *buffer, const char *string,
                         int pos)
{
    int size, length;
    char *string_utf8, *ptr_start;

    if (buffer->input)
    {
        string_utf8 = strdup (string);
        if (!string_utf8)
            return 0;

        if (pos == -1)
            pos = buffer->input_buffer_pos;

        utf8_normalize (string_utf8, '?');

        size = strlen (string_utf8);
        length = utf8_strlen (string_utf8);

        /* increase buffer size */
        buffer->input_buffer_size += size;
        buffer->input_buffer_length += length;
        gui_input_optimize_size (buffer);
        buffer->input_buffer[buffer->input_buffer_size] = '\0';

        /* move end of string to the right */
        ptr_start = utf8_add_offset (buffer->input_buffer, pos);
        memmove (ptr_start + size, ptr_start, strlen (ptr_start));

        /* insert new string */
        ptr_start = utf8_add_offset (buffer->input_buffer, pos);
        strncpy (ptr_start, string_utf8, size);

        buffer->input_buffer_pos += length;

        free (string_utf8);

        return length;
    }
    return 0;
}

/*
 * Moves input content and undo data from a buffer to another buffer.
 */

void
gui_input_move_to_buffer (struct t_gui_buffer *from_buffer,
                          struct t_gui_buffer *to_buffer)
{
    int is_command;

    /*
     * move of input is allowed if:
     * - 2 buffers are different
     * - input_share is not set to "none"
     * - input buffer in first buffer is not empty
     */
    if (!from_buffer || !to_buffer || (from_buffer == to_buffer)
        || (CONFIG_INTEGER(config_look_input_share) == CONFIG_LOOK_INPUT_SHARE_NONE)
        || !from_buffer->input_buffer || !from_buffer->input_buffer[0])
        return;

    /*
     * if input is command and that only text is allowed,
     * or if input is text and that only command is allowed,
     * then do nothing
     */
    is_command = (string_input_for_buffer (from_buffer->input_buffer) == NULL) ? 1 : 0;
    if ((is_command && (CONFIG_INTEGER(config_look_input_share) == CONFIG_LOOK_INPUT_SHARE_TEXT))
        || (!is_command && (CONFIG_INTEGER(config_look_input_share) == CONFIG_LOOK_INPUT_SHARE_COMMANDS)))
        return;

    /*
     * if overwrite is off and that input of target buffer is not empty,
     * then do nothing
     */
    if ((!CONFIG_BOOLEAN(config_look_input_share_overwrite))
        && to_buffer->input_buffer && to_buffer->input_buffer[0])
        return;

    /* move input_buffer */
    if (to_buffer->input_buffer)
        free (to_buffer->input_buffer);
    to_buffer->input_buffer = from_buffer->input_buffer;
    to_buffer->input_buffer_alloc = from_buffer->input_buffer_alloc;
    to_buffer->input_buffer_size = from_buffer->input_buffer_size;
    to_buffer->input_buffer_length = from_buffer->input_buffer_length;
    to_buffer->input_buffer_pos = from_buffer->input_buffer_pos;
    to_buffer->input_buffer_1st_display = from_buffer->input_buffer_1st_display;
    gui_buffer_input_buffer_init (from_buffer);

    /* move undo data */
    gui_buffer_undo_free_all (to_buffer);
    (to_buffer->input_undo_snap)->data = (from_buffer->input_undo_snap)->data;
    (to_buffer->input_undo_snap)->pos = (from_buffer->input_undo_snap)->pos;
    to_buffer->input_undo = from_buffer->input_undo;
    to_buffer->last_input_undo = from_buffer->last_input_undo;
    to_buffer->ptr_input_undo = from_buffer->ptr_input_undo;
    to_buffer->input_undo_count = from_buffer->input_undo_count;
    (from_buffer->input_undo_snap)->data = NULL;
    (from_buffer->input_undo_snap)->pos = 0;
    from_buffer->input_undo = NULL;
    from_buffer->last_input_undo = NULL;
    from_buffer->ptr_input_undo = NULL;
    from_buffer->input_undo_count = 0;

    gui_completion_stop (from_buffer->completion);
}

/*
 * Copies string into clipboard.
 */

void
gui_input_clipboard_copy (const char *buffer, int size)
{
    if (size <= 0)
        return;

    if (gui_input_clipboard != NULL)
        free (gui_input_clipboard);

    gui_input_clipboard = malloc((size + 1) * sizeof(*gui_input_clipboard));

    if (gui_input_clipboard)
    {
        memcpy (gui_input_clipboard, buffer, size);
        gui_input_clipboard[size] = '\0';
    }
}

/*
 * Pastes clipboard at cursor pos in input line (default key: ctrl-Y).
 */

void
gui_input_clipboard_paste (struct t_gui_buffer *buffer)
{
    if (buffer->input && gui_input_clipboard)
    {
        gui_buffer_undo_snap (buffer);
        gui_input_insert_string (buffer,
                                 gui_input_clipboard, -1);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }
}

/*
 * Terminates line:
 *   - saves text in history
 *   - stops completion
 *   - frees all undos
 *   - sends modifier and signal
 *   - sends data to buffer.
 */

void
gui_input_return (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;
    char *command;

    window = gui_window_search_with_buffer (buffer);
    if (window && window->buffer->input
        && (window->buffer->input_buffer_size > 0))
    {
        window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
        command = strdup (window->buffer->input_buffer);
        if (command)
        {
            gui_history_add (window->buffer,
                             window->buffer->input_buffer);
            window->buffer->input_buffer[0] = '\0';
            window->buffer->input_buffer_size = 0;
            window->buffer->input_buffer_length = 0;
            window->buffer->input_buffer_pos = 0;
            window->buffer->input_buffer_1st_display = 0;
            gui_buffer_undo_free_all (window->buffer);
            window->buffer->ptr_history = NULL;
            gui_history_ptr = NULL;
            gui_input_optimize_size (window->buffer);
            gui_input_text_changed_modifier_and_signal (window->buffer,
                                                        0, /* save undo */
                                                        1); /* stop completion */
            input_data (window->buffer, command);
            free (command);
        }
    }
}

/*
 * Completes a word in input buffer.
 */

void
gui_input_complete (struct t_gui_buffer *buffer)
{
    int i;

    if (!buffer->completion)
        return;

    if (buffer->completion->word_found)
    {
        /* replace word with new completed word into input buffer */
        if (buffer->completion->diff_size > 0)
        {
            buffer->input_buffer_size +=
                buffer->completion->diff_size;
            buffer->input_buffer_length +=
                buffer->completion->diff_length;
            gui_input_optimize_size (buffer);
            buffer->input_buffer[buffer->input_buffer_size] = '\0';
            for (i = buffer->input_buffer_size - 1;
                 i >= buffer->completion->position_replace +
                     (int)strlen (buffer->completion->word_found); i--)
            {
                buffer->input_buffer[i] =
                    buffer->input_buffer[i - buffer->completion->diff_size];
            }
        }
        else
        {
            for (i = buffer->completion->position_replace +
                     strlen (buffer->completion->word_found);
                 i < buffer->input_buffer_size; i++)
            {
                buffer->input_buffer[i] =
                    buffer->input_buffer[i - buffer->completion->diff_size];
            }
            buffer->input_buffer_size += buffer->completion->diff_size;
            buffer->input_buffer_length += buffer->completion->diff_length;
            gui_input_optimize_size (buffer);
            buffer->input_buffer[buffer->input_buffer_size] = '\0';
        }

        strncpy (buffer->input_buffer + buffer->completion->position_replace,
                 buffer->completion->word_found,
                 strlen (buffer->completion->word_found));
        buffer->input_buffer_pos =
            utf8_pos (buffer->input_buffer,
                      buffer->completion->position_replace) +
            utf8_strlen (buffer->completion->word_found);

        /*
         * position is < 0 this means only one word was found to complete,
         * so reinit to stop completion
         */
        if (buffer->completion->position >= 0)
            buffer->completion->position = utf8_real_pos (buffer->input_buffer,
                                                          buffer->input_buffer_pos);

        /* add space if needed after completion */
        if (buffer->completion->add_space)
        {
            if (buffer->input_buffer[utf8_real_pos (buffer->input_buffer,
                                                    buffer->input_buffer_pos)] != ' ')
            {
                gui_input_insert_string (buffer, " ",
                                         buffer->input_buffer_pos);
            }
            else
                buffer->input_buffer_pos++;
            if (buffer->completion->position >= 0)
                buffer->completion->position++;
        }
    }
}

/*
 * Completes with next word (default key: tab).
 */

void
gui_input_complete_next (struct t_gui_buffer *buffer)
{
    if (buffer->input
        && (buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        gui_buffer_undo_snap (buffer);
        gui_completion_search (buffer->completion,
                               1,
                               buffer->input_buffer,
                               buffer->input_buffer_size,
                               utf8_real_pos (buffer->input_buffer,
                                              buffer->input_buffer_pos));
        gui_input_complete (buffer);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    0); /* stop completion */
    }
}

/*
 * Completes with previous word (default key: shift-tab).
 */

void
gui_input_complete_previous (struct t_gui_buffer *buffer)
{
    if (buffer->input
        && (buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        gui_buffer_undo_snap (buffer);
        gui_completion_search (buffer->completion,
                               -1,
                               buffer->input_buffer,
                               buffer->input_buffer_size,
                               utf8_real_pos (buffer->input_buffer,
                                              buffer->input_buffer_pos));
        gui_input_complete (buffer);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    0); /* stop completion */
    }
}

/*
 * Searches for text in buffer (default key: ctrl-R).
 */

void
gui_input_search_text (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window && (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
        && (window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        gui_window_search_start (window);
        gui_input_search_signal (buffer);
    }
}

/*
 * Searches backward in buffer (default key: up during search).
 */

void
gui_input_search_previous (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window && (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
        && (window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED))
    {
        window->buffer->text_search = GUI_TEXT_SEARCH_BACKWARD;
        (void) gui_window_search_text (window);
    }
}

/*
 * Searches forward in buffer (default key: down during search).
 */

void
gui_input_search_next (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window && (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
        && (window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED))
    {
        window->buffer->text_search = GUI_TEXT_SEARCH_FORWARD;
        (void) gui_window_search_text (window);
    }
}

/*
 * Switches case for search in buffer (default key: ctrl-R during search).
 */

void
gui_input_search_switch_case (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window && (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
        && (window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED))
    {
        window->buffer->text_search_exact ^= 1;
        gui_window_search_restart (window);
        gui_input_search_signal (buffer);
    }
}

/*
 * Stops text search (default key: return during search).
 */

void
gui_input_search_stop (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window && (window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
        && (window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED))
    {
        gui_window_search_stop (window);
        gui_input_search_signal (buffer);
    }
}

/*
 * Deletes previous char (default key: backspace).
 */

void
gui_input_delete_previous_char (struct t_gui_buffer *buffer)
{
    char *pos, *pos_last;
    int char_size, size_to_move;

    if (buffer->input && (buffer->input_buffer_pos > 0))
    {
        gui_buffer_undo_snap (buffer);
        pos = utf8_add_offset (buffer->input_buffer,
                               buffer->input_buffer_pos);
        pos_last = utf8_prev_char (buffer->input_buffer, pos);
        char_size = pos - pos_last;
        size_to_move = strlen (pos);
        memmove (pos_last, pos, size_to_move);
        buffer->input_buffer_size -= char_size;
        buffer->input_buffer_length--;
        buffer->input_buffer_pos--;
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        gui_input_optimize_size (buffer);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }
}

/*
 * Deletes next char (default key: del).
 */

void
gui_input_delete_next_char (struct t_gui_buffer *buffer)
{
    char *pos, *pos_next;
    int char_size, size_to_move;

    if (buffer->input
        && (buffer->input_buffer_pos < buffer->input_buffer_length))
    {
        gui_buffer_undo_snap (buffer);
        pos = utf8_add_offset (buffer->input_buffer,
                               buffer->input_buffer_pos);
        pos_next = utf8_next_char (pos);
        char_size = pos_next - pos;
        size_to_move = strlen (pos_next);
        memmove (pos, pos_next, size_to_move);
        buffer->input_buffer_size -= char_size;
        buffer->input_buffer_length--;
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        gui_input_optimize_size (buffer);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }
}

/*
 * Deletes previous word (default key: ctrl-W).
 */

void
gui_input_delete_previous_word (struct t_gui_buffer *buffer)
{
    int length_deleted, size_deleted;
    char *start, *string;

    if (buffer->input && (buffer->input_buffer_pos > 0))
    {
        gui_buffer_undo_snap (buffer);
        start = utf8_add_offset (buffer->input_buffer,
                                 buffer->input_buffer_pos - 1);
        string = start;
        while (string && (string[0] == ' '))
        {
            string = utf8_prev_char (buffer->input_buffer, string);
        }
        if (string)
        {
            while (string && (string[0] != ' '))
            {
                string = utf8_prev_char (buffer->input_buffer, string);
            }
            if (string)
            {
                while (string && (string[0] == ' '))
                {
                    string = utf8_prev_char (buffer->input_buffer, string);
                }
            }
        }

        if (string)
            string = utf8_next_char (utf8_next_char (string));
        else
            string = buffer->input_buffer;

        size_deleted = utf8_next_char (start) - string;
        length_deleted = utf8_strnlen (string, size_deleted);

        gui_input_clipboard_copy (string, size_deleted);

        memmove (string, string + size_deleted, strlen (string + size_deleted));

        buffer->input_buffer_size -= size_deleted;
        buffer->input_buffer_length -= length_deleted;
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        buffer->input_buffer_pos -= length_deleted;
        gui_input_optimize_size (buffer);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }
}

/*
 * Deletes next word (default key: meta-d).
 */

void
gui_input_delete_next_word (struct t_gui_buffer *buffer)
{
    int size_deleted, length_deleted;
    char *start, *string;

    if (buffer->input)
    {
        gui_buffer_undo_snap (buffer);
        start = utf8_add_offset (buffer->input_buffer,
                                 buffer->input_buffer_pos);
        string = start;
        length_deleted = 0;
        while (string[0])
        {
            if ((string[0] == ' ') && (string > start))
                break;
            string = utf8_next_char (string);
            length_deleted++;
        }
        size_deleted = string - start;

        gui_input_clipboard_copy (start, size_deleted);

        memmove (start, string, strlen (string));

        buffer->input_buffer_size -= size_deleted;
        buffer->input_buffer_length -= length_deleted;
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        gui_input_optimize_size (buffer);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }
}


/*
 * Deletes all from cursor pos to beginning of line (default key: ctrl-U).
 */

void
gui_input_delete_beginning_of_line (struct t_gui_buffer *buffer)
{
    int length_deleted, size_deleted;
    char *start;

    if (buffer->input && (buffer->input_buffer_pos > 0))
    {
        gui_buffer_undo_snap (buffer);
        start = utf8_add_offset (buffer->input_buffer,
                                 buffer->input_buffer_pos);
        size_deleted = start - buffer->input_buffer;
        length_deleted = utf8_strnlen (buffer->input_buffer, size_deleted);
        gui_input_clipboard_copy (buffer->input_buffer,
                                  start - buffer->input_buffer);

        memmove (buffer->input_buffer, start, strlen (start));

        buffer->input_buffer_size -= size_deleted;
        buffer->input_buffer_length -= length_deleted;
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        buffer->input_buffer_pos = 0;
        gui_input_optimize_size (buffer);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }
}

/*
 * Deletes all from cursor pos to end of line (default key: ctrl-K).
 */

void
gui_input_delete_end_of_line (struct t_gui_buffer *buffer)
{
    char *start;
    int size_deleted;

    if (buffer->input)
    {
        gui_buffer_undo_snap (buffer);
        start = utf8_add_offset (buffer->input_buffer,
                                 buffer->input_buffer_pos);
        size_deleted = strlen (start);
        gui_input_clipboard_copy (start, size_deleted);
        start[0] = '\0';
        buffer->input_buffer_size = strlen (buffer->input_buffer);
        buffer->input_buffer_length = utf8_strlen (buffer->input_buffer);
        gui_input_optimize_size (buffer);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }
}

/*
 * Deletes entire line (default key: meta-r).
 */

void
gui_input_delete_line (struct t_gui_buffer *buffer)
{
    if (buffer->input)
    {
        gui_buffer_undo_snap (buffer);
        buffer->input_buffer[0] = '\0';
        buffer->input_buffer_size = 0;
        buffer->input_buffer_length = 0;
        buffer->input_buffer_pos = 0;
        gui_input_optimize_size (buffer);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }
}

/*
 * Transposes chars at cursor pos (default key: ctrl-T).
 */

void
gui_input_transpose_chars (struct t_gui_buffer *buffer)
{
    char *start, *prev_char, saved_char[5];
    int size_prev_char, size_start_char;

    if (buffer->input && (buffer->input_buffer_pos > 0)
        && (buffer->input_buffer_length > 1))
    {
        gui_buffer_undo_snap (buffer);

        if (buffer->input_buffer_pos == buffer->input_buffer_length)
            buffer->input_buffer_pos--;

        start = utf8_add_offset (buffer->input_buffer,
                                 buffer->input_buffer_pos);
        prev_char = utf8_prev_char (buffer->input_buffer, start);
        size_prev_char = start - prev_char;
        size_start_char = utf8_char_size (start);

        memcpy (saved_char, prev_char, size_prev_char);
        memcpy (prev_char, start, size_start_char);
        memcpy (prev_char + size_start_char, saved_char, size_prev_char);

        buffer->input_buffer_pos++;

        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }
}

/*
 * Moves cursor to beginning of line (default key: home).
 */

void
gui_input_move_beginning_of_line (struct t_gui_buffer *buffer)
{
    if (buffer->input && (buffer->input_buffer_pos > 0))
    {
        buffer->input_buffer_pos = 0;
        gui_input_text_cursor_moved_signal (buffer);
    }
}

/*
 * Moves cursor to end of line (default key: end).
 */

void
gui_input_move_end_of_line (struct t_gui_buffer *buffer)
{
    if (buffer->input
        && (buffer->input_buffer_pos < buffer->input_buffer_length))
    {
        buffer->input_buffer_pos = buffer->input_buffer_length;
        gui_input_text_cursor_moved_signal (buffer);
    }
}

/*
 * Moves cursor to previous char (default key: left).
 */

void
gui_input_move_previous_char (struct t_gui_buffer *buffer)
{
    if (buffer->input && (buffer->input_buffer_pos > 0))
    {
        buffer->input_buffer_pos--;
        gui_input_text_cursor_moved_signal (buffer);
    }
}

/*
 * Moves cursor to next char (default key: right).
 */

void
gui_input_move_next_char (struct t_gui_buffer *buffer)
{
    if (buffer->input
        && (buffer->input_buffer_pos < buffer->input_buffer_length))
    {
        buffer->input_buffer_pos++;
        gui_input_text_cursor_moved_signal (buffer);
    }
}

/*
 * Moves cursor to beginning of previous word (default key: meta-b or
 * ctrl-left).
 */

void
gui_input_move_previous_word (struct t_gui_buffer *buffer)
{
    char *pos;

    if (buffer->input
        && (buffer->input_buffer_pos > 0))
    {
        pos = utf8_add_offset (buffer->input_buffer,
                               buffer->input_buffer_pos - 1);
        while (pos && (pos[0] == ' '))
        {
            pos = utf8_prev_char (buffer->input_buffer, pos);
        }
        if (pos)
        {
            while (pos && (pos[0] != ' '))
            {
                pos = utf8_prev_char (buffer->input_buffer, pos);
            }
            if (pos)
                pos = utf8_next_char (pos);
            else
                pos = buffer->input_buffer;
            buffer->input_buffer_pos = utf8_pos (buffer->input_buffer,
                                                 pos - buffer->input_buffer);
        }
        else
            buffer->input_buffer_pos = 0;

        gui_input_text_cursor_moved_signal (buffer);
    }
}

/*
 * Moves cursor to the beginning of next word (default key: meta-f or
 * ctrl-right).
 */

void
gui_input_move_next_word (struct t_gui_buffer *buffer)
{
    char *pos;

    if (buffer->input
        && (buffer->input_buffer_pos < buffer->input_buffer_length))
    {
        pos = utf8_add_offset (buffer->input_buffer,
                               buffer->input_buffer_pos);
        while (pos[0] && (pos[0] == ' '))
        {
            pos = utf8_next_char (pos);
        }
        if (pos[0])
        {
            while (pos[0] && (pos[0] != ' '))
            {
                pos = utf8_next_char (pos);
            }
            if (pos[0])
            {
                buffer->input_buffer_pos =
                    utf8_pos (buffer->input_buffer,
                              pos - buffer->input_buffer);
            }
            else
                buffer->input_buffer_pos = buffer->input_buffer_length;
        }
        else
        {
            buffer->input_buffer_pos =
                utf8_pos (buffer->input_buffer,
                          utf8_prev_char (buffer->input_buffer, pos) - buffer->input_buffer);
        }

        gui_input_text_cursor_moved_signal (buffer);
    }
}

/*
 * Recalls previous command from local or global history.
 */

void
gui_input_history_previous (struct t_gui_window *window,
                            struct t_gui_history *history,
                            struct t_gui_history **ptr_history)
{
    if (!window->buffer->input)
        return;

    if (*ptr_history)
    {
        if (!(*ptr_history)->next_history)
            return;
        *ptr_history = (*ptr_history)->next_history;
    }
    if (!(*ptr_history))
        *ptr_history = history;

    if (!(*ptr_history))
        return;

    /* bash/readline like use of history */
    if (window->buffer->input_buffer_size > 0)
    {
        if ((*ptr_history)->prev_history)
        {
            /* replace text in history with current input */
            window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            if ((*ptr_history)->prev_history->text)
                free ((*ptr_history)->prev_history->text);
            (*ptr_history)->prev_history->text =
                strdup (window->buffer->input_buffer);
        }
        else
        {
            /* add current input in history */
            window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            gui_history_add (window->buffer,
                             window->buffer->input_buffer);
        }
    }
    window->buffer->input_buffer_size =
        strlen ((*ptr_history)->text);
    window->buffer->input_buffer_length =
        utf8_strlen ((*ptr_history)->text);
    gui_input_optimize_size (window->buffer);
    window->buffer->input_buffer_pos = window->buffer->input_buffer_length;
    window->buffer->input_buffer_1st_display = 0;
    strcpy (window->buffer->input_buffer, (*ptr_history)->text);
    gui_input_text_changed_modifier_and_signal (window->buffer,
                                                0, /* save undo */
                                                1); /* stop completion */
    gui_buffer_undo_free_all (window->buffer);
}

/*
 * Recalls next command from local or global history.
 */

void
gui_input_history_next (struct t_gui_window *window,
                        struct t_gui_history *history,
                        struct t_gui_history **ptr_history)
{
    int input_changed;

    /* make C compiler happy */
    (void) history;

    input_changed = 0;

    if (!window->buffer->input)
        return;

    if (*ptr_history)
    {
        /* replace text in history with current input */
        window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
        if ((*ptr_history)->text)
            free ((*ptr_history)->text);
        (*ptr_history)->text = strdup (window->buffer->input_buffer);

        *ptr_history = (*ptr_history)->prev_history;
        if (*ptr_history)
        {
            window->buffer->input_buffer_size =
                strlen ((*ptr_history)->text);
            window->buffer->input_buffer_length =
                utf8_strlen ((*ptr_history)->text);
        }
        else
        {
            window->buffer->input_buffer[0] = '\0';
            window->buffer->input_buffer_size = 0;
            window->buffer->input_buffer_length = 0;
        }
        gui_input_optimize_size (window->buffer);
        window->buffer->input_buffer_pos =
            window->buffer->input_buffer_length;
        window->buffer->input_buffer_1st_display = 0;
        if (*ptr_history)
        {
            strcpy (window->buffer->input_buffer, (*ptr_history)->text);
        }
        input_changed = 1;
    }
    else
    {
        /* add line to history then clear input */
        if (window->buffer->input_buffer_size > 0)
        {
            window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            gui_history_add (window->buffer,
                             window->buffer->input_buffer);
            window->buffer->input_buffer[0] = '\0';
            window->buffer->input_buffer_size = 0;
            window->buffer->input_buffer_length = 0;
            window->buffer->input_buffer_pos = 0;
            window->buffer->input_buffer_1st_display = 0;
            gui_input_optimize_size (window->buffer);
            input_changed = 1;
        }
    }
    if (input_changed)
    {
        gui_input_text_changed_modifier_and_signal (window->buffer,
                                                    0, /* save undo */
                                                    1); /* stop completion */
        gui_buffer_undo_free_all (window->buffer);
    }
}

/*
 * Recalls previous command from local history (default key: up).
 */

void
gui_input_history_local_previous (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window)
    {
        gui_input_history_previous (window,
                                    window->buffer->history,
                                    &(window->buffer->ptr_history));
    }
}

/*
 * Recalls next command from local history (default key: down).
 */

void
gui_input_history_local_next (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window)
    {
        gui_input_history_next (window,
                                window->buffer->history,
                                &(window->buffer->ptr_history));
    }
}

/*
 * Recalls previous command from global history (default key: ctrl-up).
 */

void
gui_input_history_global_previous (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window)
    {
        gui_input_history_previous (window,
                                    gui_history,
                                    &gui_history_ptr);
    }
}

/*
 * Recalls next command from global history (default key: ctrl-down).
 */

void
gui_input_history_global_next (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window)
    {
        gui_input_history_next (window,
                                gui_history,
                                &gui_history_ptr);
    }
}

/*
 * Jumps to buffer with activity (default key: alt-a).
 */

void
gui_input_jump_smart (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window
        && (window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        if (gui_hotlist)
        {
            if (!gui_hotlist_initial_buffer)
                gui_hotlist_initial_buffer = window->buffer;
            gui_window_switch_to_buffer (window, gui_hotlist->buffer, 1);
            gui_hotlist_remove_buffer (window->buffer);
            gui_window_scroll_bottom (window);
        }
        else
        {
            if (gui_hotlist_initial_buffer)
            {
                if (CONFIG_BOOLEAN(config_look_jump_smart_back_to_buffer))
                {
                    gui_window_switch_to_buffer (window,
                                                 gui_hotlist_initial_buffer, 1);
                    gui_window_scroll_bottom (window);
                }
                gui_hotlist_initial_buffer = NULL;
            }
            else
            {
                gui_hotlist_initial_buffer = NULL;
            }
        }
    }
}

/*
 * Jumps to last buffer (default key: meta-j, meta-l).
 */

void
gui_input_jump_last_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window
        && (window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
        && last_gui_buffer)
    {
        gui_buffer_switch_by_number (window, last_gui_buffer->number);
    }
}

/*
 * Jumps to last buffer displayed (before last jump to a buffer) (default key:
 * meta-/).
 */

void
gui_input_jump_last_buffer_displayed (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window
        && (window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        if (gui_buffer_last_displayed)
            gui_buffer_switch_by_number (window,
                                         gui_buffer_last_displayed->number);
    }
}

/*
 * Jumps to previously visited buffer (buffer displayed before current one)
 * (default key: meta-<).
 */

void
gui_input_jump_previously_visited_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;
    int index;
    struct t_gui_buffer_visited *ptr_buffer_visited;

    window = gui_window_search_with_buffer (buffer);
    if (window
        && (window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        index = gui_buffer_visited_get_index_previous ();
        if (index >= 0)
        {
            gui_buffers_visited_index = index;

            ptr_buffer_visited =
                gui_buffer_visited_search_by_number (gui_buffers_visited_index);
            if (ptr_buffer_visited)
            {
                gui_buffers_visited_frozen = 1;
                gui_buffer_switch_by_number (window,
                                             ptr_buffer_visited->buffer->number);
                gui_buffers_visited_frozen = 0;
            }
        }
    }
}

/*
 * Jumps to next visited buffer (buffer displayed after current one) (default
 * key: meta->).
 */

void
gui_input_jump_next_visited_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;
    int index;
    struct t_gui_buffer_visited *ptr_buffer_visited;

    window = gui_window_search_with_buffer (buffer);
    if (window
        && (window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        index = gui_buffer_visited_get_index_next ();
        if (index >= 0)
        {
            gui_buffers_visited_index = index;

            ptr_buffer_visited = gui_buffer_visited_search_by_number (gui_buffers_visited_index);
            if (ptr_buffer_visited)
            {
                gui_buffers_visited_frozen = 1;
                gui_buffer_switch_by_number (window,
                                             ptr_buffer_visited->buffer->number);
                gui_buffers_visited_frozen = 0;
            }
        }
    }
}

/*
 * Clears hotlist (default key: meta-h).
 */

void
gui_input_hotlist_clear (struct t_gui_buffer *buffer)
{
    gui_hotlist_clear ();
    gui_hotlist_initial_buffer = buffer;
}

/*
 * Initializes "grab key mode" (next key will be inserted into input buffer)
 * (default key: meta-k).
 */

void
gui_input_grab_key (struct t_gui_buffer *buffer, int command, const char *delay)
{
    if (buffer->input)
        gui_key_grab_init (command, delay);
}

/*
 * Initializes "grab mouse mode" (next mouse event will be inserted into input
 * buffer) (default key: button2 of mouse in input bar).
 */

void
gui_input_grab_mouse (struct t_gui_buffer *buffer, int area)
{
    if (buffer->input)
        gui_mouse_grab_init (area);
}

/*
 * Sets unread marker for all buffers (default key: ctrl-S, ctrl-U).
 */

void
gui_input_set_unread ()
{
    struct t_gui_buffer *ptr_buffer;

    /* set read marker for all standard buffers */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_buffer_set_unread (ptr_buffer);
    }
}

/*
 * Sets unread marker for a buffer.
 */

void
gui_input_set_unread_current (struct t_gui_buffer *buffer)
{
    gui_buffer_set_unread (buffer);
}

/*
 * Switches active buffer to next buffer (when many buffers are merged) (default
 * key: ctrl-X).
 */

void
gui_input_switch_active_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_window *window;

    ptr_buffer = gui_buffer_get_next_active_buffer (buffer);
    if (ptr_buffer)
    {
        gui_buffer_set_active_buffer (ptr_buffer);
        window = gui_window_search_with_buffer (buffer);
        if (window)
            gui_window_switch_to_buffer (window, ptr_buffer, 1);
    }
}

/*
 * Switches active buffer to previous buffer (when many buffers are merged).
 */

void
gui_input_switch_active_buffer_previous (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_window *window;

    ptr_buffer = gui_buffer_get_previous_active_buffer (buffer);
    if (ptr_buffer)
    {
        gui_buffer_set_active_buffer (ptr_buffer);
        window = gui_window_search_with_buffer (buffer);
        if (window)
            gui_window_switch_to_buffer (window, ptr_buffer, 1);
    }
}

/*
 * Zooms on current active merged buffer, or display all merged buffers if zoom
 * was active (default key: alt-x).
 */

void
gui_input_zoom_merged_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_window;

    /* do nothing if current buffer is not merged with another buffer */
    if (gui_buffer_count_merged_buffers (buffer->number) < 2)
        return;

    /* reset scroll in all windows displaying this buffer number */
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if ((ptr_window->buffer->number == buffer->number)
            && ptr_window->scroll && ptr_window->scroll->start_line)
        {
            gui_window_scroll_bottom (ptr_window);
        }
    }

    /* first make buffer active if it is not */
    if (!buffer->active)
    {
        gui_buffer_set_active_buffer (buffer);
        ptr_window = gui_window_search_with_buffer (buffer);
        if (ptr_window)
            gui_window_switch_to_buffer (ptr_window, buffer, 1);
    }

    /*
     * toggle active flag between 1 and 2
     * (1 = active with other merged buffers displayed, 2 = the only active)
     */
    if (buffer->active == 1)
    {
        buffer->active = 2;
        buffer->lines = buffer->own_lines;
    }
    else if (buffer->active == 2)
    {
        buffer->active = 1;
        buffer->lines = buffer->mixed_lines;
    }

    gui_buffer_ask_chat_refresh (buffer, 2);
}

/*
 * Inserts a string in command line.
 */

void
gui_input_insert (struct t_gui_buffer *buffer, const char *args)
{
    char *args2;

    if (args)
    {
        gui_buffer_undo_snap (buffer);
        args2 = string_convert_hex_chars (args);
        gui_input_insert_string (buffer, (args2) ? args2 : args, -1);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
        if (args2)
            free (args2);
    }
}

/*
 * Uses a undo: replace input with undo content.
 */

void
gui_input_undo_use (struct t_gui_buffer *buffer, struct t_gui_input_undo *undo)
{
    if ((undo->data) && (strcmp (undo->data, buffer->input_buffer) != 0))
    {
        gui_input_replace_input (buffer, undo->data);
        gui_input_set_pos (buffer, undo->pos);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    0, /* save undo */
                                                    1); /* stop completion */
    }
}

/*
 * Undoes last action on input buffer (default key: ctrl-_).
 */

void
gui_input_undo (struct t_gui_buffer *buffer)
{
    if (buffer->ptr_input_undo)
    {
        /*
         * if we are doing undo and that undo pointer is to the end of list
         * (for example first time undo is used), then save current input
         * content in undo list
         */
        if ((buffer->ptr_input_undo == buffer->last_input_undo)
            && (buffer->ptr_input_undo)->data
            && (strcmp (buffer->input_buffer, (buffer->ptr_input_undo)->data) != 0))
        {
            gui_buffer_undo_snap_free (buffer);
            gui_buffer_undo_add (buffer);
        }

        if (buffer->ptr_input_undo
            && (buffer->ptr_input_undo)->prev_undo)
        {
            buffer->ptr_input_undo = (buffer->ptr_input_undo)->prev_undo;
            gui_input_undo_use (buffer, buffer->ptr_input_undo);
        }
    }
}

/*
 * Redoes last action on input buffer (default key: alt-_).
 */

void
gui_input_redo (struct t_gui_buffer *buffer)
{
    if (buffer->ptr_input_undo
        && (buffer->ptr_input_undo)->next_undo)
    {
        buffer->ptr_input_undo = (buffer->ptr_input_undo)->next_undo;
        gui_input_undo_use (buffer, buffer->ptr_input_undo);
    }
}
