/*
 * gui-input.c - input functions (used by all GUI)
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/core-config.h"
#include "../core/core-hook.h"
#include "../core/core-input.h"
#include "../core/core-string.h"
#include "../core/core-utf8.h"
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


char *gui_input_clipboard = NULL;      /* internal clipboard content        */


/*
 * Optimizes input buffer size by adding or deleting data block (predefined
 * size).
 *
 * Returns:
 *   1: input size optimized
 *   0: error (input and its size are not changed)
 */

int
gui_input_optimize_size (struct t_gui_buffer *buffer,
                         int new_size, int new_length)
{
    int optimal_size;
    char *input_buffer2;

    if (!buffer->input)
        return 0;

    optimal_size = ((new_size / GUI_BUFFER_INPUT_BLOCK_SIZE) *
                    GUI_BUFFER_INPUT_BLOCK_SIZE) + GUI_BUFFER_INPUT_BLOCK_SIZE;
    if (buffer->input_buffer_alloc != optimal_size)
    {
        input_buffer2 = realloc (buffer->input_buffer, optimal_size);
        if (!input_buffer2)
            return 0;
        buffer->input_buffer = input_buffer2;
        buffer->input_buffer_alloc = optimal_size;
    }

    buffer->input_buffer_size = new_size;
    buffer->input_buffer_length = new_length;

    return 1;
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

    input_utf8 = strdup ((new_input) ? new_input : "");
    if (input_utf8)
    {
        utf8_normalize (input_utf8, '?');

        size = strlen (input_utf8);
        length = utf8_strlen (input_utf8);

        /* compute new buffer size */
        if (gui_input_optimize_size (buffer, size, length))
        {
            /* copy new string to input */
            strcpy (buffer->input_buffer, input_utf8);

            /* move cursor to the end of new input if it is now after the end */
            if (buffer->input_buffer_pos > buffer->input_buffer_length)
                buffer->input_buffer_pos = buffer->input_buffer_length;
        }

        free (input_utf8);
    }
}

/*
 * Sends signal "input_paste_pending".
 */

void
gui_input_paste_pending_signal ()
{
    if (CONFIG_BOOLEAN(config_look_bare_display_exit_on_input)
        && gui_window_bare_display)
    {
        gui_window_bare_display_toggle (NULL);
    }

    (void) hook_signal_send ("input_paste_pending",
                             WEECHAT_HOOK_SIGNAL_STRING, NULL);
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

    if (CONFIG_BOOLEAN(config_look_bare_display_exit_on_input)
        && gui_window_bare_display)
    {
        gui_window_bare_display_toggle (NULL);
    }

    if (!gui_cursor_mode)
    {
        if (save_undo)
            gui_buffer_undo_add (buffer);

        /* send modifier, and change input if needed */
        snprintf (str_buffer, sizeof (str_buffer), "%p", buffer);
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
    (void) hook_signal_send ("input_text_changed",
                             WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Sends signal "input_text_cursor_moved".
 */

void
gui_input_text_cursor_moved_signal (struct t_gui_buffer *buffer)
{
    if (CONFIG_BOOLEAN(config_look_bare_display_exit_on_input)
        && gui_window_bare_display)
    {
        gui_window_bare_display_toggle (NULL);
    }

    (void) hook_signal_send ("input_text_cursor_moved",
                             WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Sends signal "input_search".
 */

void
gui_input_search_signal (struct t_gui_buffer *buffer)
{
    if (CONFIG_BOOLEAN(config_look_bare_display_exit_on_input)
        && gui_window_bare_display)
    {
        gui_window_bare_display_toggle (NULL);
    }
    (void) hook_signal_send ("input_search",
                             WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Sets position in input line.
 */

void
gui_input_set_pos (struct t_gui_buffer *buffer, int pos)
{
    if (!buffer || (pos < 0) || (buffer->input_buffer_pos == pos))
        return;

    buffer->input_buffer_pos = pos;
    if (buffer->input_buffer_pos > buffer->input_buffer_length)
        buffer->input_buffer_pos = buffer->input_buffer_length;
    gui_input_text_cursor_moved_signal (buffer);
}

/*
 * Inserts a string into the input buffer at cursor position.
 */

void
gui_input_insert_string (struct t_gui_buffer *buffer, const char *string)
{
    int size, length;
    char *string_utf8, *ptr_start;

    if (!buffer->input || !string)
        return;

    string_utf8 = strdup (string);
    if (!string_utf8)
        return;

    utf8_normalize (string_utf8, '?');

    size = strlen (string_utf8);
    length = utf8_strlen (string_utf8);

    if (gui_input_optimize_size (buffer,
                                 buffer->input_buffer_size + size,
                                 buffer->input_buffer_length + length))
    {
        buffer->input_buffer[buffer->input_buffer_size] = '\0';

        /* move end of string to the right */
        ptr_start = (char *)utf8_add_offset (buffer->input_buffer, buffer->input_buffer_pos);
        memmove (ptr_start + size, ptr_start, strlen (ptr_start));

        /* insert new string */
        memcpy (ptr_start, string_utf8, size);

        buffer->input_buffer_pos += length;
    }

    free (string_utf8);
}

/*
 * Copies string into the internal clipboard.
 */

void
gui_input_clipboard_copy (const char *buffer, int size)
{
    if (!buffer || (size <= 0))
        return;

    if (gui_input_clipboard != NULL)
        free (gui_input_clipboard);

    gui_input_clipboard = malloc ((size + 1) * sizeof (*gui_input_clipboard));

    if (gui_input_clipboard)
    {
        memcpy (gui_input_clipboard, buffer, size);
        gui_input_clipboard[size] = '\0';
    }
}

/*
 * Pastes the internal clipboard at cursor pos in input line
 * (default key: ctrl-y).
 */

void
gui_input_clipboard_paste (struct t_gui_buffer *buffer)
{
    if (buffer->input && gui_input_clipboard)
    {
        gui_buffer_undo_snap (buffer);
        gui_input_insert_string (buffer, gui_input_clipboard);
        gui_input_text_changed_modifier_and_signal (buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
    }
}

/*
 * Sends data to buffer:
 *   - saves text in history
 *   - stops completion
 *   - frees all undos
 *   - sends modifier and signal
 *   - sends data to buffer.
 */

void
gui_input_send_data_to_buffer (struct t_gui_buffer *buffer, char *data)
{
    gui_history_add (buffer, data);
    gui_buffer_undo_free_all (buffer);
    buffer->ptr_history = NULL;
    gui_history_ptr = NULL;
    gui_input_text_changed_modifier_and_signal (buffer,
                                                0, /* save undo */
                                                1); /* stop completion */
    (void) input_data (buffer, data, NULL, 1, 1);
}

/*
 * Sends current input to buffer.
 */

void
gui_input_return (struct t_gui_buffer *buffer)
{
    char *data;

    if (CONFIG_BOOLEAN(config_look_bare_display_exit_on_input)
        && gui_window_bare_display)
    {
        gui_window_bare_display_toggle (NULL);
    }

    if (buffer->input
        && (buffer->input_get_empty || (buffer->input_buffer_size > 0)))
    {
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        data = strdup (buffer->input_buffer);
        if (gui_input_optimize_size (buffer, 0, 0))
        {
            buffer->input_buffer[0] = '\0';
            buffer->input_buffer_pos = 0;
            buffer->input_buffer_1st_display = 0;
        }
        if (data)
        {
            gui_input_send_data_to_buffer (buffer, data);
            free (data);
        }
    }
}

/*
 * Splits input on newlines then sends each line to buffer.
 */

void
gui_input_split_return (struct t_gui_buffer *buffer)
{
    char **lines;
    int i, num_lines;

    if (buffer->input
        && (buffer->input_get_empty || (buffer->input_buffer_size > 0)))
    {
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        lines = string_split (buffer->input_buffer, "\n", NULL, 0, 0, &num_lines);
        if (gui_input_optimize_size (buffer, 0, 0))
        {
            buffer->input_buffer[0] = '\0';
            buffer->input_buffer_pos = 0;
            buffer->input_buffer_1st_display = 0;
        }
        if (lines)
        {
            for (i = 0; i < num_lines; i++)
            {
                gui_input_send_data_to_buffer (buffer, lines[i]);
            }
            string_free_split (lines);
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

    if (!buffer->completion || !buffer->completion->word_found)
        return;

    /*
     * in case the word found is empty, we keep the word in input as-is
     * (this can happen with partial completion when the common prefix found
     * is empty)
     */
    if (!buffer->completion->word_found[0])
        return;

    /* replace word with new completed word into input buffer */
    if (buffer->completion->diff_size > 0)
    {
        if (gui_input_optimize_size (
                buffer,
                buffer->input_buffer_size + buffer->completion->diff_size,
                buffer->input_buffer_length + buffer->completion->diff_length))
        {
            buffer->input_buffer[buffer->input_buffer_size] = '\0';
            for (i = buffer->input_buffer_size - 1;
                 i >= buffer->completion->position_replace +
                     (int)strlen (buffer->completion->word_found); i--)
            {
                buffer->input_buffer[i] =
                    buffer->input_buffer[i - buffer->completion->diff_size];
            }
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
        if (gui_input_optimize_size (
                buffer,
                buffer->input_buffer_size + buffer->completion->diff_size,
                buffer->input_buffer_length += buffer->completion->diff_length))
        {
            buffer->input_buffer[buffer->input_buffer_size] = '\0';
        }
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
            gui_input_insert_string (buffer, " ");
        }
        else
            buffer->input_buffer_pos++;
        if (buffer->completion->position >= 0)
            buffer->completion->position++;
    }
}

/*
 * Completes with next word (default key: tab).
 */

void
gui_input_complete_next (struct t_gui_buffer *buffer)
{
    if (!buffer->input || (buffer->text_search != GUI_BUFFER_SEARCH_DISABLED))
        return;

    gui_buffer_undo_snap (buffer);
    if (gui_completion_search (buffer->completion,
                               buffer->input_buffer,
                               buffer->input_buffer_pos,
                               1))
    {
        gui_input_complete (buffer);
        gui_input_text_changed_modifier_and_signal (
            buffer,
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
    if (!buffer->input || (buffer->text_search != GUI_BUFFER_SEARCH_DISABLED))
        return;

    gui_buffer_undo_snap (buffer);
    if (gui_completion_search (buffer->completion,
                               buffer->input_buffer,
                               buffer->input_buffer_pos,
                               -1))
    {
        gui_input_complete (buffer);
        gui_input_text_changed_modifier_and_signal (
            buffer,
            1, /* save undo */
            0); /* stop completion */
    }
}

/*
 * Searches for text in buffer at current position (default key: ctrl-r).
 */

void
gui_input_search_text_here (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window && (window->buffer->text_search == GUI_BUFFER_SEARCH_DISABLED))
    {
        gui_window_search_start (window, GUI_BUFFER_SEARCH_LINES,
                                 window->scroll->start_line);
        gui_input_search_signal (buffer);
    }
}

/*
 * Searches for text in buffer.
 */

void
gui_input_search_text (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window && (window->buffer->text_search == GUI_BUFFER_SEARCH_DISABLED))
    {
        gui_window_search_start (window, GUI_BUFFER_SEARCH_LINES, NULL);
        gui_input_search_signal (buffer);
    }
}

/*
 * Searches for text in buffer/global command line history.
 */

void
gui_input_search_history (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window && (window->buffer->text_search == GUI_BUFFER_SEARCH_DISABLED))
    {
        gui_window_search_start (window, GUI_BUFFER_SEARCH_HISTORY, NULL);
        gui_input_search_signal (buffer);
    }
}

/*
 * Compiles regex used to search text in buffer.
 */

void
gui_input_search_compile_regex (struct t_gui_buffer *buffer)
{
    int flags;

    /* remove the compiled regex */
    if (buffer->text_search_regex_compiled)
    {
        regfree (buffer->text_search_regex_compiled);
        free (buffer->text_search_regex_compiled);
        buffer->text_search_regex_compiled = NULL;
    }

    /* compile regex */
    if (buffer->text_search_regex)
    {
        buffer->text_search_regex_compiled = malloc (sizeof (*buffer->text_search_regex_compiled));
        if (buffer->text_search_regex_compiled)
        {
            flags = REG_EXTENDED;
            if (!buffer->text_search_exact)
                flags |= REG_ICASE;
            if (string_regcomp (buffer->text_search_regex_compiled,
                                buffer->input_buffer, flags) != 0)
            {
                free (buffer->text_search_regex_compiled);
                buffer->text_search_regex_compiled = NULL;
            }
        }
    }
}

/*
 * Switches case for search in buffer (default key: alt-c during search).
 */

void
gui_input_search_switch_case (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window && (window->buffer->text_search != GUI_BUFFER_SEARCH_DISABLED))
    {
        window->buffer->text_search_exact ^= 1;
        gui_window_search_restart (window);
        gui_input_search_signal (buffer);
    }
}

/*
 * Switches string/regex for search in buffer (default key: ctrl-r during
 * search).
 */

void
gui_input_search_switch_regex (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (window && (window->buffer->text_search != GUI_BUFFER_SEARCH_DISABLED))
    {
        window->buffer->text_search_regex ^= 1;
        gui_window_search_restart (window);
        gui_input_search_signal (buffer);
    }
}

/*
 * Switches search in messages/prefixes (default key: tab during search).
 */

void
gui_input_search_switch_where (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (!window)
        return;

    switch (window->buffer->text_search)
    {
        case GUI_BUFFER_SEARCH_DISABLED:
            break;
        case GUI_BUFFER_SEARCH_LINES:
            /* it's not possible to change that in a buffer not "formatted" */
            if (window->buffer->type != GUI_BUFFER_TYPE_FORMATTED)
                return;
            if (window->buffer->text_search_where == GUI_BUFFER_SEARCH_IN_MESSAGE)
                window->buffer->text_search_where = GUI_BUFFER_SEARCH_IN_PREFIX;
            else if (window->buffer->text_search_where == GUI_BUFFER_SEARCH_IN_PREFIX)
                window->buffer->text_search_where = GUI_BUFFER_SEARCH_IN_MESSAGE | GUI_BUFFER_SEARCH_IN_PREFIX;
            else
                window->buffer->text_search_where = GUI_BUFFER_SEARCH_IN_MESSAGE;
            gui_window_search_restart (window);
            gui_input_search_signal (buffer);
            break;
        case GUI_BUFFER_SEARCH_HISTORY:
            window->buffer->text_search_history =
                (window->buffer->text_search_history == GUI_BUFFER_SEARCH_HISTORY_LOCAL) ?
                GUI_BUFFER_SEARCH_HISTORY_GLOBAL : GUI_BUFFER_SEARCH_HISTORY_LOCAL;
            gui_window_search_restart (window);
            gui_input_search_signal (buffer);
            break;
        case GUI_BUFFER_NUM_SEARCH:
            break;
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
    if (!window)
        return;

    switch (window->buffer->text_search)
    {
        case GUI_BUFFER_SEARCH_DISABLED:
            break;
        case GUI_BUFFER_SEARCH_LINES:
            window->buffer->text_search_direction = GUI_BUFFER_SEARCH_DIR_BACKWARD;
            (void) gui_window_search_text (window);
            break;
        case GUI_BUFFER_SEARCH_HISTORY:
            window->buffer->text_search_direction = GUI_BUFFER_SEARCH_DIR_BACKWARD;
            if (gui_window_search_text (window))
                gui_input_search_signal (buffer);
            break;
        case GUI_BUFFER_NUM_SEARCH:
            break;
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
    if (!window)
        return;

    switch (window->buffer->text_search)
    {
        case GUI_BUFFER_SEARCH_DISABLED:
            break;
        case GUI_BUFFER_SEARCH_LINES:
            window->buffer->text_search_direction = GUI_BUFFER_SEARCH_DIR_FORWARD;
            (void) gui_window_search_text (window);
            break;
        case GUI_BUFFER_SEARCH_HISTORY:
            window->buffer->text_search_direction = GUI_BUFFER_SEARCH_DIR_FORWARD;
            if (gui_window_search_text (window))
                gui_input_search_signal (buffer);
            break;
        case GUI_BUFFER_NUM_SEARCH:
            break;
    }
}

/*
 * Stops text search at current position (default key: return during search).
 */

void
gui_input_search_stop_here (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (!window)
        return;

    switch (window->buffer->text_search)
    {
        case GUI_BUFFER_SEARCH_DISABLED:
            break;
        case GUI_BUFFER_SEARCH_LINES:
        case GUI_BUFFER_SEARCH_HISTORY:
            gui_window_search_stop (window, 1);
            gui_input_search_signal (buffer);
            break;
        case GUI_BUFFER_NUM_SEARCH:
            break;
    }
}

/*
 * Stops text search (default key: ctrl-q during search).
 */

void
gui_input_search_stop (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;

    window = gui_window_search_with_buffer (buffer);
    if (!window)
        return;

    switch (window->buffer->text_search)
    {
        case GUI_BUFFER_SEARCH_DISABLED:
            break;
        case GUI_BUFFER_SEARCH_LINES:
        case GUI_BUFFER_SEARCH_HISTORY:
            gui_window_search_stop (window, 0);
            gui_input_search_signal (buffer);
            break;
        case GUI_BUFFER_NUM_SEARCH:
            break;
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

    if (!buffer->input || (buffer->input_buffer_pos <= 0))
        return;

    gui_buffer_undo_snap (buffer);
    pos = (char *)utf8_add_offset (buffer->input_buffer,
                                   buffer->input_buffer_pos);
    pos_last = (char *)utf8_prev_char (buffer->input_buffer, pos);
    char_size = pos - pos_last;
    size_to_move = strlen (pos);
    memmove (pos_last, pos, size_to_move);
    if (gui_input_optimize_size (buffer,
                                 buffer->input_buffer_size - char_size,
                                 buffer->input_buffer_length - 1))
    {
        buffer->input_buffer_pos--;
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
    }
    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Deletes next char (default key: del).
 */

void
gui_input_delete_next_char (struct t_gui_buffer *buffer)
{
    char *pos, *pos_next;
    int char_size, size_to_move;

    if (!buffer->input
        || (buffer->input_buffer_pos >= buffer->input_buffer_length))
    {
        return;
    }

    gui_buffer_undo_snap (buffer);
    pos = (char *)utf8_add_offset (buffer->input_buffer,
                                   buffer->input_buffer_pos);
    pos_next = (char *)utf8_next_char (pos);
    char_size = pos_next - pos;
    size_to_move = strlen (pos_next);
    memmove (pos, pos_next, size_to_move);
    if (gui_input_optimize_size (buffer,
                                 buffer->input_buffer_size - char_size,
                                 buffer->input_buffer_length - 1))
    {
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
    }
    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Delete the range between two positions and copy the content to the
 * clipboard.
 */

void
gui_input_delete_range (struct t_gui_buffer *buffer,
                        char *start,
                        char *end)
{
    int size_deleted, length_deleted;

    size_deleted = utf8_next_char (end) - start;
    length_deleted = utf8_strnlen (start, size_deleted);

    gui_input_clipboard_copy (start, size_deleted);

    memmove (start, start + size_deleted, strlen (start + size_deleted));

    if (gui_input_optimize_size (
            buffer,
            buffer->input_buffer_size - size_deleted,
            buffer->input_buffer_length - length_deleted))
    {
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        buffer->input_buffer_pos -= length_deleted;
    }
    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Deletes previous word (default key: alt-backspace).
 */

void
gui_input_delete_previous_word (struct t_gui_buffer *buffer)
{
    char *start, *string;

    if (!buffer->input || (buffer->input_buffer_pos <= 0))
        return;

    gui_buffer_undo_snap (buffer);
    start = (char *)utf8_add_offset (buffer->input_buffer,
                                     buffer->input_buffer_pos - 1);
    string = start;
    /* move to the left until we reach a word char */
    while (string && !string_is_word_char_input (string))
    {
        string = (char *)utf8_prev_char (buffer->input_buffer, string);
    }
    /* move to the left to skip the whole word */
    if (string)
    {
        while (string && string_is_word_char_input (string))
        {
            string = (char *)utf8_prev_char (buffer->input_buffer, string);
        }
    }

    if (string)
        string = (char *)utf8_next_char (string);
    else
        string = buffer->input_buffer;

    gui_input_delete_range (buffer, string, start);
}

/*
 * Deletes previous word until whitespace (default key: ctrl-w).
 */

void
gui_input_delete_previous_word_whitespace (struct t_gui_buffer *buffer)
{
    char *start, *string;

    if (!buffer->input || (buffer->input_buffer_pos <= 0))
        return;

    gui_buffer_undo_snap (buffer);
    start = (char *)utf8_add_offset (buffer->input_buffer,
                                     buffer->input_buffer_pos - 1);
    string = start;
    /* move to the left, skipping whitespace */
    while (string && string_is_whitespace_char (string))
    {
        string = (char *)utf8_prev_char (buffer->input_buffer, string);
    }
    /* move to the left until we reach a char which is not whitespace */
    if (string)
    {
        while (string && !string_is_whitespace_char (string))
        {
            string = (char *)utf8_prev_char (buffer->input_buffer, string);
        }
    }

    if (string)
        string = (char *)utf8_next_char (string);
    else
        string = buffer->input_buffer;

    gui_input_delete_range (buffer, string, start);
}

/*
 * Deletes next word (default key: alt-d).
 */

void
gui_input_delete_next_word (struct t_gui_buffer *buffer)
{
    int size_deleted, length_deleted;
    char *start, *string;

    if (!buffer->input)
        return;

    gui_buffer_undo_snap (buffer);
    start = (char *)utf8_add_offset (buffer->input_buffer,
                                     buffer->input_buffer_pos);
    string = start;
    length_deleted = 0;
    /* move to the right until we reach a word char */
    while (string[0] && !string_is_word_char_input (string))
    {
        string = (char *)utf8_next_char (string);
        length_deleted++;
    }
    /* move to the right to skip the whole word */
    while (string[0] && string_is_word_char_input (string))
    {
        string = (char *)utf8_next_char (string);
        length_deleted++;
    }
    size_deleted = string - start;

    gui_input_clipboard_copy (start, size_deleted);

    memmove (start, string, strlen (string));

    if (gui_input_optimize_size (
            buffer,
            buffer->input_buffer_size - size_deleted,
            buffer->input_buffer_length - length_deleted))
    {
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
    }
    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Deletes all from cursor pos to beginning of line (default key: ctrl-u).
 *
 * If cursor is at beginning of line, deletes to beginning of previous line.
 */

void
gui_input_delete_beginning_of_line (struct t_gui_buffer *buffer)
{
    int length_deleted, size_deleted;
    char *start, *beginning_of_line;

    if (!buffer->input || (buffer->input_buffer_pos <= 0))
        return;

    gui_buffer_undo_snap (buffer);
    start = (char *)utf8_add_offset (buffer->input_buffer,
                                     buffer->input_buffer_pos);

    beginning_of_line = (char *)utf8_beginning_of_line (buffer->input_buffer,
                                                        start);
    if (beginning_of_line == start)
    {
        beginning_of_line = (char *)utf8_prev_char (buffer->input_buffer,
                                                    beginning_of_line);
        beginning_of_line = (char *)utf8_beginning_of_line (buffer->input_buffer,
                                                            beginning_of_line);
    }

    size_deleted = start - beginning_of_line;
    length_deleted = utf8_strnlen (beginning_of_line, size_deleted);
    gui_input_clipboard_copy (beginning_of_line, size_deleted);

    memmove (beginning_of_line, start, strlen (start));

    if (gui_input_optimize_size (buffer,
                                 buffer->input_buffer_size - size_deleted,
                                 buffer->input_buffer_length - length_deleted))
    {
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        buffer->input_buffer_pos = utf8_pos (
            buffer->input_buffer,
            beginning_of_line - buffer->input_buffer);
    }
    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Deletes all from cursor pos to end of line (default key: ctrl-k).
 *
 * If cursor is at end of line, deletes to end of next line.
 */

void
gui_input_delete_end_of_line (struct t_gui_buffer *buffer)
{
    int length_deleted, size_deleted;
    char *start, *end_of_line;

    if (!buffer->input)
        return;

    gui_buffer_undo_snap (buffer);
    start = (char *)utf8_add_offset (buffer->input_buffer,
                                     buffer->input_buffer_pos);

    if (start[0] && (start[0] == '\n'))
        end_of_line = (char *)utf8_next_char (start);
    else
        end_of_line = start;
    end_of_line = (char *)utf8_end_of_line (end_of_line);

    size_deleted = end_of_line - start;
    length_deleted = utf8_strnlen (start, size_deleted);
    gui_input_clipboard_copy (start, size_deleted);

    memmove (start, end_of_line, strlen (end_of_line));

    if (gui_input_optimize_size (buffer,
                                 buffer->input_buffer_size - size_deleted,
                                 buffer->input_buffer_length - length_deleted))
    {
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        buffer->input_buffer_pos = utf8_pos (buffer->input_buffer,
                                             start - buffer->input_buffer);
    }

    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Deletes all from cursor pos to beginning of input (default key: alt-ctrl-u).
 */

void
gui_input_delete_beginning_of_input (struct t_gui_buffer *buffer)
{
    int length_deleted, size_deleted;
    char *start;

    if (!buffer->input || (buffer->input_buffer_pos <= 0))
        return;

    gui_buffer_undo_snap (buffer);
    start = (char *)utf8_add_offset (buffer->input_buffer,
                                     buffer->input_buffer_pos);
    size_deleted = start - buffer->input_buffer;
    length_deleted = utf8_strnlen (buffer->input_buffer, size_deleted);
    gui_input_clipboard_copy (buffer->input_buffer,
                              start - buffer->input_buffer);

    memmove (buffer->input_buffer, start, strlen (start));

    if (gui_input_optimize_size (
            buffer,
            buffer->input_buffer_size - size_deleted,
            buffer->input_buffer_length - length_deleted))
    {
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        buffer->input_buffer_pos = 0;
    }
    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Deletes all from cursor pos to end of input (default key: alt-ctrl-k).
 */

void
gui_input_delete_end_of_input (struct t_gui_buffer *buffer)
{
    char *start;
    int size_deleted;

    if (!buffer->input)
        return;

    gui_buffer_undo_snap (buffer);
    start = (char *)utf8_add_offset (buffer->input_buffer,
                                     buffer->input_buffer_pos);
    size_deleted = strlen (start);
    gui_input_clipboard_copy (start, size_deleted);
    start[0] = '\0';
    (void) gui_input_optimize_size (buffer,
                                    strlen (buffer->input_buffer),
                                    utf8_strlen (buffer->input_buffer));
    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Deletes current line (default key: alt-r).
 */

void
gui_input_delete_line (struct t_gui_buffer *buffer)
{
    int length_deleted, size_deleted;
    char *start, *beginning_of_line, *end_of_line;

    if (!buffer->input)
        return;

    gui_buffer_undo_snap (buffer);
    start = (char *)utf8_add_offset (buffer->input_buffer,
                                     buffer->input_buffer_pos);

    beginning_of_line = (char *)utf8_beginning_of_line (buffer->input_buffer,
                                                        start);
    end_of_line = (char *)utf8_end_of_line (start);

    size_deleted = end_of_line - beginning_of_line;
    length_deleted = utf8_strnlen (beginning_of_line, size_deleted);

    memmove (beginning_of_line, end_of_line, strlen (end_of_line));

    if (gui_input_optimize_size (buffer,
                                 buffer->input_buffer_size - size_deleted,
                                 buffer->input_buffer_length - length_deleted))
    {
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        buffer->input_buffer_pos = utf8_pos (
            buffer->input_buffer,
            beginning_of_line - buffer->input_buffer);
    }

    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Deletes entire input (default key: alt-R).
 */

void
gui_input_delete_input (struct t_gui_buffer *buffer)
{
    if (!buffer->input)
        return;

    gui_buffer_undo_snap (buffer);
    if (gui_input_optimize_size (buffer, 0, 0))
    {
        buffer->input_buffer[0] = '\0';
        buffer->input_buffer_pos = 0;
    }
    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Transposes chars at cursor pos (default key: ctrl-t).
 */

void
gui_input_transpose_chars (struct t_gui_buffer *buffer)
{
    char *start, *prev_char, saved_char[5];
    int size_prev_char, size_start_char;

    if (!buffer->input
        || (buffer->input_buffer_pos <= 0)
        || (buffer->input_buffer_length <= 1))
    {
        return;
    }

    gui_buffer_undo_snap (buffer);

    if (buffer->input_buffer_pos == buffer->input_buffer_length)
        buffer->input_buffer_pos--;

    start = (char *)utf8_add_offset (buffer->input_buffer,
                                     buffer->input_buffer_pos);
    prev_char = (char *)utf8_prev_char (buffer->input_buffer, start);
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

/*
 * Moves cursor to beginning of line (default key: home).
 *
 * If cursor is at beginning of line, moves to beginning of previous line.
 */

void
gui_input_move_beginning_of_line (struct t_gui_buffer *buffer)
{
    char *pos, *original_pos;

    if (!buffer->input || (buffer->input_buffer_pos <= 0))
        return;

    pos = (char *)utf8_add_offset (buffer->input_buffer,
                                   buffer->input_buffer_pos);
    original_pos = pos;
    pos = (char *)utf8_beginning_of_line (buffer->input_buffer, pos);

    if (pos == original_pos)
    {
        pos = (char *)utf8_prev_char (buffer->input_buffer, pos);
        pos = (char *)utf8_beginning_of_line (buffer->input_buffer, pos);
    }

    buffer->input_buffer_pos = utf8_pos (buffer->input_buffer,
                                         pos - buffer->input_buffer);

    gui_input_text_cursor_moved_signal (buffer);
}

/*
 * Moves cursor to end of line (default key: end).
 *
 * If cursor is at end of line, moves to end of next line.
 */

void
gui_input_move_end_of_line (struct t_gui_buffer *buffer)
{
    char *pos;

    if (!buffer->input
        || (buffer->input_buffer_pos >= buffer->input_buffer_length))
    {
        return;
    }

    pos = (char *)utf8_add_offset (buffer->input_buffer,
                                   buffer->input_buffer_pos);
    if (pos[0] && pos[0] == '\n')
    {
        pos = (char *)utf8_next_char (pos);
    }
    pos = (char *)utf8_end_of_line (pos);
    if (pos[0])
    {
        buffer->input_buffer_pos = utf8_pos (buffer->input_buffer,
                                             pos - buffer->input_buffer);
    }
    else
    {
        buffer->input_buffer_pos = buffer->input_buffer_length;
    }

    gui_input_text_cursor_moved_signal (buffer);
}

/*
 * Moves cursor to beginning of input (default key: shift-home).
 */

void
gui_input_move_beginning_of_input (struct t_gui_buffer *buffer)
{
    if (!buffer->input || (buffer->input_buffer_pos <= 0))
        return;

    buffer->input_buffer_pos = 0;
    gui_input_text_cursor_moved_signal (buffer);
}

/*
 * Moves cursor to end of input (default key: shift-end).
 */

void
gui_input_move_end_of_input (struct t_gui_buffer *buffer)
{
    if (!buffer->input
        || (buffer->input_buffer_pos >= buffer->input_buffer_length))
    {
        return;
    }

    buffer->input_buffer_pos = buffer->input_buffer_length;
    gui_input_text_cursor_moved_signal (buffer);
}

/*
 * Moves cursor to previous char (default key: left).
 */

void
gui_input_move_previous_char (struct t_gui_buffer *buffer)
{
    if (!buffer->input || (buffer->input_buffer_pos <= 0))
        return;

    buffer->input_buffer_pos--;
    gui_input_text_cursor_moved_signal (buffer);
}

/*
 * Moves cursor to next char (default key: right).
 */

void
gui_input_move_next_char (struct t_gui_buffer *buffer)
{
    if (!buffer->input
        || (buffer->input_buffer_pos >= buffer->input_buffer_length))
    {
        return;
    }

    buffer->input_buffer_pos++;
    gui_input_text_cursor_moved_signal (buffer);
}

/*
 * Moves cursor to beginning of previous word (default key: alt-b or
 * ctrl-left).
 */

void
gui_input_move_previous_word (struct t_gui_buffer *buffer)
{
    char *pos;

    if (!buffer->input || (buffer->input_buffer_pos <= 0))
        return;

    pos = (char *)utf8_add_offset (buffer->input_buffer,
                                   buffer->input_buffer_pos - 1);
    while (pos && !string_is_word_char_input (pos))
    {
        pos = (char *)utf8_prev_char (buffer->input_buffer, pos);
    }
    if (pos)
    {
        while (pos && string_is_word_char_input (pos))
        {
            pos = (char *)utf8_prev_char (buffer->input_buffer, pos);
        }
        if (pos)
            pos = (char *)utf8_next_char (pos);
        else
            pos = buffer->input_buffer;
        buffer->input_buffer_pos = utf8_pos (buffer->input_buffer,
                                             pos - buffer->input_buffer);
    }
    else
        buffer->input_buffer_pos = 0;

    gui_input_text_cursor_moved_signal (buffer);
}

/*
 * Moves cursor to the beginning of next word (default key: alt-f or
 * ctrl-right).
 */

void
gui_input_move_next_word (struct t_gui_buffer *buffer)
{
    char *pos;

    if (!buffer->input
        || (buffer->input_buffer_pos >= buffer->input_buffer_length))
    {
        return;
    }

    pos = (char *)utf8_add_offset (buffer->input_buffer,
                                   buffer->input_buffer_pos);
    while (pos[0] && !string_is_word_char_input (pos))
    {
        pos = (char *)utf8_next_char (pos);
    }
    if (pos[0])
    {
        while (pos[0] && string_is_word_char_input (pos))
        {
            pos = (char *)utf8_next_char (pos);
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
        buffer->input_buffer_pos = buffer->input_buffer_length;

    gui_input_text_cursor_moved_signal (buffer);
}

/*
 * Moves cursor to previous line (default key: shift-up).
 */

void
gui_input_move_previous_line (struct t_gui_buffer *buffer)
{
    int beginning_of_line_pos, length_from_beginning, i;
    char *pos;

    if (!buffer->input || (buffer->input_buffer_pos <= 0))
        return;

    pos = (char *)utf8_add_offset (buffer->input_buffer,
                                   buffer->input_buffer_pos);
    pos = (char *)utf8_beginning_of_line (buffer->input_buffer, pos);

    if (pos != buffer->input_buffer)
    {
        beginning_of_line_pos = utf8_pos (buffer->input_buffer,
                                          pos - buffer->input_buffer);
        length_from_beginning = buffer->input_buffer_pos - beginning_of_line_pos;

        pos = (char *)utf8_prev_char (buffer->input_buffer, pos);
        pos = (char *)utf8_beginning_of_line (buffer->input_buffer, pos);

        for (i = 0;
             pos[0] && (pos[0] != '\n') && (i < length_from_beginning);
             i++)
        {
            pos = (char *)utf8_next_char (pos);
        }

        buffer->input_buffer_pos = utf8_pos (buffer->input_buffer,
                                             pos - buffer->input_buffer);

        gui_input_text_cursor_moved_signal (buffer);
    }
}

/*
 * Moves cursor to next line (default key: shift-down).
 */

void
gui_input_move_next_line (struct t_gui_buffer *buffer)
{
    int beginning_of_line_pos, length_from_beginning, i;
    char *pos;

    if (!buffer->input
        || (buffer->input_buffer_pos >= buffer->input_buffer_length))
    {
        return;
    }

    pos = (char *)utf8_add_offset (buffer->input_buffer,
                                   buffer->input_buffer_pos);
    pos = (char *)utf8_beginning_of_line (buffer->input_buffer, pos);

    beginning_of_line_pos = utf8_pos (buffer->input_buffer,
                                      pos - buffer->input_buffer);
    length_from_beginning = buffer->input_buffer_pos - beginning_of_line_pos;

    pos = (char *)utf8_end_of_line (pos);
    if (pos[0])
    {
        pos = (char *)utf8_next_char (pos);

        for (i = 0;
             pos[0] && (pos[0] != '\n') && (i < length_from_beginning);
             i++)
        {
            pos = (char *)utf8_next_char (pos);
        }

        buffer->input_buffer_pos = utf8_pos (buffer->input_buffer,
                                             pos - buffer->input_buffer);

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
    if (gui_input_optimize_size (window->buffer,
                                 strlen ((*ptr_history)->text),
                                 utf8_strlen ((*ptr_history)->text)))
    {
        window->buffer->input_buffer_pos = window->buffer->input_buffer_length;
        window->buffer->input_buffer_1st_display = 0;
        strcpy (window->buffer->input_buffer, (*ptr_history)->text);
    }
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
    int input_changed, rc;

    /* make C compiler happy */
    (void) history;

    input_changed = 0;

    if (!window->buffer->input)
        return;

    if (*ptr_history)
    {
        /* replace text in history with current input */
        window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
        free ((*ptr_history)->text);
        (*ptr_history)->text = strdup (window->buffer->input_buffer);

        *ptr_history = (*ptr_history)->prev_history;
        if (*ptr_history)
        {
            rc = gui_input_optimize_size (window->buffer,
                                          strlen ((*ptr_history)->text),
                                          utf8_strlen ((*ptr_history)->text));
        }
        else
        {
            rc = gui_input_optimize_size (window->buffer, 0, 0);
            if (rc)
                window->buffer->input_buffer[0] = '\0';
        }
        if (rc)
        {
            window->buffer->input_buffer_pos =
                window->buffer->input_buffer_length;
            window->buffer->input_buffer_1st_display = 0;
            if (*ptr_history)
            {
                strcpy (window->buffer->input_buffer, (*ptr_history)->text);
            }
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
            if (gui_input_optimize_size (window->buffer, 0, 0))
            {
                window->buffer->input_buffer[0] = '\0';
                window->buffer->input_buffer_pos = 0;
                window->buffer->input_buffer_1st_display = 0;
            }
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
 * Sends the current history entry (found with search or recalled with
 * "up" key) and inserts the next one in the command line without sending it
 * (default key: ctrl-o, in contexts "default" and "histsearch").
 */

void
gui_input_history_use_get_next (struct t_gui_buffer *buffer)
{
    struct t_gui_window *window;
    struct t_gui_history *ptr_history, **ptr_ptr_history;;

    window = gui_window_search_with_buffer (buffer);
    if (!window)
        return;

    ptr_history = NULL;
    ptr_ptr_history = NULL;
    if (window->buffer->text_search == GUI_BUFFER_SEARCH_HISTORY)
    {
        ptr_history = window->buffer->text_search_ptr_history;
        if (!ptr_history)
            return;
        ptr_ptr_history = (window->buffer->text_search_history == GUI_BUFFER_SEARCH_HISTORY_LOCAL) ?
            &(window->buffer->ptr_history) : &gui_history_ptr;
        gui_window_search_stop (window, 1);
    }
    else if (window->buffer->ptr_history)
    {
        ptr_history = window->buffer->ptr_history;
        ptr_ptr_history = &(window->buffer->ptr_history);
    }
    else if (gui_history_ptr)
    {
        ptr_history = gui_history_ptr;
        ptr_ptr_history = &gui_history_ptr;
    }

    gui_input_return (buffer);

    if (ptr_history && ptr_history->prev_history)
    {
        gui_input_insert_string (buffer, ptr_history->prev_history->text);
        if (ptr_ptr_history)
            *ptr_ptr_history = ptr_history->prev_history;
    }
}

/*
 * Initializes "grab key mode" (next key will be inserted into input buffer)
 * (default key: alt-k).
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
 * Inserts a string in command line.
 */

void
gui_input_insert (struct t_gui_buffer *buffer, const char *args)
{
    char *args2;

    if (!args)
        return;

    gui_buffer_undo_snap (buffer);
    args2 = string_convert_escaped_chars (args);
    gui_input_insert_string (buffer, (args2) ? args2 : args);
    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
    free (args2);
}

/*
 * Uses a undo: replace input with undo content.
 */

void
gui_input_undo_use (struct t_gui_buffer *buffer, struct t_gui_input_undo *undo)
{
    if ((!undo->data) || (strcmp (undo->data, buffer->input_buffer) == 0))
        return;

    gui_input_replace_input (buffer, undo->data);
    gui_input_set_pos (buffer, undo->pos);
    gui_input_text_changed_modifier_and_signal (buffer,
                                                0, /* save undo */
                                                1); /* stop completion */
}

/*
 * Undoes last action on input buffer (default key: ctrl-_).
 */

void
gui_input_undo (struct t_gui_buffer *buffer)
{
    if (!buffer->ptr_input_undo)
        return;

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

/*
 * Redoes last action on input buffer (default key: alt-_).
 */

void
gui_input_redo (struct t_gui_buffer *buffer)
{
    if (!buffer->ptr_input_undo || !(buffer->ptr_input_undo)->next_undo)
        return;

    buffer->ptr_input_undo = (buffer->ptr_input_undo)->next_undo;
    gui_input_undo_use (buffer, buffer->ptr_input_undo);
}
