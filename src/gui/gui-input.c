/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* gui-input.c: input functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-input.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-input.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-completion.h"
#include "gui-history.h"
#include "gui-hotlist.h"
#include "gui-keyboard.h"
#include "gui-window.h"


char *gui_input_clipboard = NULL;      /* clipboard content                 */


/*
 * gui_input_optimize_size: optimize input buffer size by adding
 *                          or deleting data block (predefined size)
 */

void
gui_input_optimize_size (struct t_gui_buffer *buffer)
{
    int optimal_size;
    
    if (buffer->input)
    {
        optimal_size = ((buffer->input_buffer_size / GUI_BUFFER_INPUT_BLOCK_SIZE) *
                        GUI_BUFFER_INPUT_BLOCK_SIZE) + GUI_BUFFER_INPUT_BLOCK_SIZE;
        if (buffer->input_buffer_alloc != optimal_size)
        {
            buffer->input_buffer_alloc = optimal_size;
            buffer->input_buffer = realloc (buffer->input_buffer, optimal_size);
        }
    }
}

/*
 * gui_input_replace_input: replace full input by another string, trying to
 *                          keep cursor position is new string is long enough
 */

void
gui_input_replace_input (struct t_gui_buffer *buffer, const char *new_input)
{
    int size, length;
    
    size = strlen (new_input);
    length = utf8_strlen (new_input);
    
    /* compute new buffer size */
    buffer->input_buffer_size = size;
    buffer->input_buffer_length = length;
    gui_input_optimize_size (buffer);
    
    /* copy new string to input */
    strcpy (buffer->input_buffer, new_input);
    
    /* move cursor to the end of new input if it is now after the end */
    if (buffer->input_buffer_pos > buffer->input_buffer_length)
        buffer->input_buffer_pos = buffer->input_buffer_length;
}

/*
 * gui_input_paste_pending_signal: send signal "input_paste_pending"
 */

void
gui_input_paste_pending_signal ()
{
    hook_signal_send ("input_paste_pending", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * gui_input_text_changed_modifier_and_signal: send modifier and signal
 *                                             "input_text_changed"
 */

void
gui_input_text_changed_modifier_and_signal (struct t_gui_buffer *buffer)
{
    char str_buffer[128], *new_input;
    
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
        if (strcmp (new_input, buffer->input_buffer) != 0)
        {
            /* input has been changed by modifier, use it */
            gui_input_replace_input (buffer, new_input);
        }
        free (new_input);
    }
    
    /* send signal */
    hook_signal_send ("input_text_changed", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * gui_input_text_cursor_moved_signal: send signal "input_text_cursor_moved"
 */

void
gui_input_text_cursor_moved_signal ()
{
    hook_signal_send ("input_text_cursor_moved", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * gui_input_search_signal: send signal "input_search"
 */

void
gui_input_search_signal ()
{
    hook_signal_send ("input_search", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * gui_input_move: move data in input buffer
 */

void
gui_input_move (struct t_gui_buffer *buffer, char *target, const char *source,
                int size)
{
    int pos_source, pos_target;
    
    pos_target = target - buffer->input_buffer;
    pos_source = source - buffer->input_buffer;
    
    memmove (target, source, size);
}

/*
 * gui_input_insert_string: insert a string into the input buffer
 *                          if pos == -1, string is inserted at cursor position
 *                          return: number of chars inserted
 *                          (may be different of strlen if UTF-8 string)
 */

int
gui_input_insert_string (struct t_gui_buffer *buffer, const char *string,
                         int pos)
{
    int pos_start, size, length;
    char *ptr_start;
    
    if (buffer->input)
    {
        if (pos == -1)
            pos = buffer->input_buffer_pos;
        
        size = strlen (string);
        length = utf8_strlen (string);
        
        /* increase buffer size */
        buffer->input_buffer_size += size;
        buffer->input_buffer_length += length;
        gui_input_optimize_size (buffer);
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        
        /* move end of string to the right */
        ptr_start = utf8_add_offset (buffer->input_buffer, pos);
        pos_start = ptr_start - buffer->input_buffer;
        memmove (ptr_start + size, ptr_start, strlen (ptr_start));
        
        /* insert new string */
        ptr_start = utf8_add_offset (buffer->input_buffer, pos);
        pos_start = ptr_start - buffer->input_buffer;
        strncpy (ptr_start, string, size);
        
        buffer->input_buffer_pos += length;
        
        return length;
    }
    return 0;
}

/*
 * gui_input_clipboard_copy: copy string into clipboard
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
 * gui_action_clipboard_paste: paste clipboard at cursor pos in input line
 */

void
gui_input_clipboard_paste ()
{
    if ((gui_current_window->buffer->input)
        && gui_input_clipboard) 
    {
        gui_input_insert_string (gui_current_window->buffer,
                                 gui_input_clipboard, -1);
        gui_completion_stop (gui_current_window->buffer->completion, 1);
        
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
    }
}

/*
 * gui_input_return: terminate line
 */

void
gui_input_return ()
{
    char *command;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
        {
            gui_window_search_stop (gui_current_window);
            gui_input_search_signal ();
        }
        else if (gui_current_window->buffer->input_buffer_size > 0)
        {
            gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
            command = strdup (gui_current_window->buffer->input_buffer);
            if (!command)
                return;
            gui_history_buffer_add (gui_current_window->buffer,
                                    gui_current_window->buffer->input_buffer);
            gui_history_global_add (gui_current_window->buffer->input_buffer);
            gui_current_window->buffer->input_buffer[0] = '\0';
            gui_current_window->buffer->input_buffer_size = 0;
            gui_current_window->buffer->input_buffer_length = 0;
            gui_current_window->buffer->input_buffer_pos = 0;
            gui_current_window->buffer->input_buffer_1st_display = 0;
            gui_completion_stop (gui_current_window->buffer->completion, 1);
            gui_current_window->buffer->ptr_history = NULL;
            gui_input_optimize_size (gui_current_window->buffer);
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
            input_data (gui_current_window->buffer, command);
            free (command);
        }
    }
}

/*
 * gui_input_complete: complete a word in input buffer
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
                 i >=  buffer->completion->position_replace +
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
        
        /* position is < 0 this means only one word was found to complete,
           so reinit to stop completion */
        if (buffer->completion->position >= 0)
            buffer->completion->position = utf8_real_pos (buffer->input_buffer,
                                                      buffer->input_buffer_pos);
        
        /* add space if needed after completion */
        if (buffer->completion->add_space)
        {
            if (buffer->input_buffer[utf8_real_pos (buffer->input_buffer,
                                                    buffer->input_buffer_pos)] != ' ')
                gui_input_insert_string (buffer, " ",
                                         buffer->input_buffer_pos);
            else
                buffer->input_buffer_pos++;
            if (buffer->completion->position >= 0)
                buffer->completion->position++;
        }
        gui_input_text_changed_modifier_and_signal (buffer);
    }
}

/*
 * gui_input_complete_next: complete with next word (default key: tab)
 */

void
gui_input_complete_next ()
{
    if ((gui_current_window->buffer->input)
        && (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        gui_completion_search (gui_current_window->buffer->completion,
                               1,
                               gui_current_window->buffer->input_buffer,
                               gui_current_window->buffer->input_buffer_size,
                               utf8_real_pos (gui_current_window->buffer->input_buffer,
                                              gui_current_window->buffer->input_buffer_pos));
        gui_input_complete (gui_current_window->buffer);
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
    }
}

/*
 * gui_complete_previous: complete with previous word (default key: shift-tab)
 */

void
gui_input_complete_previous ()
{
    if ((gui_current_window->buffer->input)
        && (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        gui_completion_search (gui_current_window->buffer->completion,
                               -1,
                               gui_current_window->buffer->input_buffer,
                               gui_current_window->buffer->input_buffer_size,
                               utf8_real_pos (gui_current_window->buffer->input_buffer,
                                              gui_current_window->buffer->input_buffer_pos));
        gui_input_complete (gui_current_window->buffer);
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
    }
}

/*
 * gui_input_search_text: search text in buffer history (default key: ctrl-r)
 */

void
gui_input_search_text ()
{
    if (gui_current_window->buffer->type == GUI_BUFFER_TYPE_FORMATTED)
    {
        /* toggle search */
        if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
            gui_window_search_start (gui_current_window);
        else
        {
            gui_current_window->buffer->text_search_exact ^= 1;
            gui_window_search_restart (gui_current_window);
        }
        gui_input_search_signal ();
    }
}


/*
 * gui_input_delete_previous_char: delete previous char (default key: backspace)
 */

void
gui_input_delete_previous_char ()
{
    char *pos, *pos_last;
    int char_size, size_to_move;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos > 0)
        {
            pos = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                   gui_current_window->buffer->input_buffer_pos);
            pos_last = utf8_prev_char (gui_current_window->buffer->input_buffer, pos);
            char_size = pos - pos_last;
            size_to_move = strlen (pos);
            gui_input_move (gui_current_window->buffer, pos_last, pos, size_to_move);
            gui_current_window->buffer->input_buffer_size -= char_size;
            gui_current_window->buffer->input_buffer_length--;
            gui_current_window->buffer->input_buffer_pos--;
            gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_input_optimize_size (gui_current_window->buffer);
            gui_completion_stop (gui_current_window->buffer->completion, 1);
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
        }
    }
}

/*
 * gui_input_delete_next_char: delete next char (default key: del)
 */

void
gui_input_delete_next_char ()
{
    char *pos, *pos_next;
    int char_size, size_to_move;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos <
            gui_current_window->buffer->input_buffer_length)
        {
            pos = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                   gui_current_window->buffer->input_buffer_pos);
            pos_next = utf8_next_char (pos);
            char_size = pos_next - pos;
            size_to_move = strlen (pos_next);
            gui_input_move (gui_current_window->buffer, pos, pos_next, size_to_move);
            gui_current_window->buffer->input_buffer_size -= char_size;
            gui_current_window->buffer->input_buffer_length--;
            gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_input_optimize_size (gui_current_window->buffer);
            gui_completion_stop (gui_current_window->buffer->completion, 1);
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
        }
    }
}

/*
 * gui_input_delete_previous_word: delete previous word (default key: ctrl-w)
 */

void
gui_input_delete_previous_word ()
{
    int length_deleted, size_deleted;
    char *start, *string;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos > 0)
        {
            start = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                      gui_current_window->buffer->input_buffer_pos - 1);
            string = start;
            while (string && (string[0] == ' '))
            {
                string = utf8_prev_char (gui_current_window->buffer->input_buffer, string);
            }
            if (string)
            {
                while (string && (string[0] != ' '))
                {
                    string = utf8_prev_char (gui_current_window->buffer->input_buffer, string);
                }
                if (string)
                {
                    while (string && (string[0] == ' '))
                    {
                        string = utf8_prev_char (gui_current_window->buffer->input_buffer, string);
                    }
                }
            }
            
            if (string)
                string = utf8_next_char (utf8_next_char (string));
            else
                string = gui_current_window->buffer->input_buffer;
            
            size_deleted = utf8_next_char (start) - string;
            length_deleted = utf8_strnlen (string, size_deleted);
            
            gui_input_clipboard_copy (string, size_deleted);
            
            gui_input_move (gui_current_window->buffer, string, string + size_deleted, strlen (string + size_deleted));
            
            gui_current_window->buffer->input_buffer_size -= size_deleted;
            gui_current_window->buffer->input_buffer_length -= length_deleted;
            gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_current_window->buffer->input_buffer_pos -= length_deleted;
            gui_input_optimize_size (gui_current_window->buffer);
            gui_completion_stop (gui_current_window->buffer->completion, 1);
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
        }
    }
}

/*
 * gui_input_delete_next_word: delete next word (default key: meta-d)
 */

void
gui_input_delete_next_word ()
{
    int size_deleted, length_deleted;
    char *start, *string;
    
    if (gui_current_window->buffer->input)
    {
        start = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                 gui_current_window->buffer->input_buffer_pos);
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
        
        gui_input_move (gui_current_window->buffer, start, string, strlen (string));
        
        gui_current_window->buffer->input_buffer_size -= size_deleted;
        gui_current_window->buffer->input_buffer_length -= length_deleted;
        gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
        gui_input_optimize_size (gui_current_window->buffer);
        gui_completion_stop (gui_current_window->buffer->completion, 1);
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
    }
}


/*
 * gui_input_delete_beginning_of_line: delete all from cursor pos to beginning of line
 *                                     (default key: ctrl-u)
 */

void
gui_input_delete_beginning_of_line ()
{
    int length_deleted, size_deleted, pos_start;
    char *start;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos > 0)
        {
            start = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                     gui_current_window->buffer->input_buffer_pos);
            pos_start = start - gui_current_window->buffer->input_buffer;
            size_deleted = start - gui_current_window->buffer->input_buffer;
            length_deleted = utf8_strnlen (gui_current_window->buffer->input_buffer, size_deleted);
            gui_input_clipboard_copy (gui_current_window->buffer->input_buffer,
                                      start - gui_current_window->buffer->input_buffer);
            
            gui_input_move (gui_current_window->buffer, gui_current_window->buffer->input_buffer, start, strlen (start));
            
            gui_current_window->buffer->input_buffer_size -= size_deleted;
            gui_current_window->buffer->input_buffer_length -= length_deleted;
            gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_current_window->buffer->input_buffer_pos = 0;
            gui_input_optimize_size (gui_current_window->buffer);
            gui_completion_stop (gui_current_window->buffer->completion, 1);
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
        }
    }
}

/*
 * gui_input_delete_end_of_line: delete all from cursor pos to end of line
 *                               (default key: ctrl-k)
 */

void
gui_input_delete_end_of_line (const char *args)
{
    char *start;
    int size_deleted, length_deleted, pos_start;

    /* make C compiler happy */
    (void) args;
    
    if (gui_current_window->buffer->input)
    {
        start = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                 gui_current_window->buffer->input_buffer_pos);
        pos_start = start - gui_current_window->buffer->input_buffer;
        size_deleted = strlen (start);
        length_deleted = utf8_strlen (start);
        gui_input_clipboard_copy (start, size_deleted);
        start[0] = '\0';
        gui_current_window->buffer->input_buffer_size = strlen (gui_current_window->buffer->input_buffer);
        gui_current_window->buffer->input_buffer_length = utf8_strlen (gui_current_window->buffer->input_buffer);
        gui_input_optimize_size (gui_current_window->buffer);
        gui_completion_stop (gui_current_window->buffer->completion, 1);
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
    }
}

/*
 * gui_input_delete_line: delete entire line (default key: meta-r)
 */

void
gui_input_delete_line ()
{
    if (gui_current_window->buffer->input)
    {
        gui_current_window->buffer->input_buffer[0] = '\0';
        gui_current_window->buffer->input_buffer_size = 0;
        gui_current_window->buffer->input_buffer_length = 0;
        gui_current_window->buffer->input_buffer_pos = 0;
        gui_input_optimize_size (gui_current_window->buffer);
        gui_completion_stop (gui_current_window->buffer->completion, 1);
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
    }
}

/*
 * gui_input_transpose_chars: transpose chars (on lth left) at cursor pos
 *                            (default key: ctrl-t)
 */

void
gui_input_transpose_chars ()
{
    char *start, *prev_char, saved_char[5];
    int size_prev_char, size_start_char;
    int pos_prev_char, pos_start;
    
    if (gui_current_window->buffer->input)
    {
        if ((gui_current_window->buffer->input_buffer_pos > 0)
            && (gui_current_window->buffer->input_buffer_length > 1))
        {
            if (gui_current_window->buffer->input_buffer_pos == gui_current_window->buffer->input_buffer_length)
                gui_current_window->buffer->input_buffer_pos--;
            
            start = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                     gui_current_window->buffer->input_buffer_pos);
            pos_start = start - gui_current_window->buffer->input_buffer;
            prev_char = utf8_prev_char (gui_current_window->buffer->input_buffer,
                                        start);
            pos_prev_char = prev_char - gui_current_window->buffer->input_buffer;
            size_prev_char = start - prev_char;
            size_start_char = utf8_char_size (start);
            
            memcpy (saved_char, prev_char, size_prev_char);
            memcpy (prev_char, start, size_start_char);
            memcpy (prev_char + size_start_char, saved_char, size_prev_char);
            
            gui_current_window->buffer->input_buffer_pos++;
            
            gui_completion_stop (gui_current_window->buffer->completion, 1);
            
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
        }
    }
}

/*
 * gui_input_move_beginning_of_line: move cursor to beginning of line (default key: home)
 */

void
gui_input_move_beginning_of_line ()
{
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos > 0)
        {
            gui_current_window->buffer->input_buffer_pos = 0;
            
            gui_input_text_cursor_moved_signal ();
        }
    }
}

/*
 * gui_input_move_end_of_line: move cursor to end of line (default key: end)
 */

void
gui_input_move_end_of_line ()
{
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos <
            gui_current_window->buffer->input_buffer_length)
        {
            gui_current_window->buffer->input_buffer_pos =
                gui_current_window->buffer->input_buffer_length;
            
            gui_input_text_cursor_moved_signal ();
        }
    }
}

/*
 * gui_input_move_previous_char: move cursor to previous char (default key: left)
 */

void
gui_input_move_previous_char ()
{
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos > 0)
        {
            gui_current_window->buffer->input_buffer_pos--;
            
            gui_input_text_cursor_moved_signal ();
        }
    }
}

/*
 * gui_input_move_next_char: move cursor to next char (default key: right)
 */

void
gui_input_move_next_char ()
{
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos <
            gui_current_window->buffer->input_buffer_length)
        {
            gui_current_window->buffer->input_buffer_pos++;
            
            gui_input_text_cursor_moved_signal ();
        }
    }
}

/*
 * gui_input_move_previous_word: move cursor to beginning of previous word
 *                               (default key: meta-b or ctrl-left)
 */

void
gui_input_move_previous_word ()
{
    char *pos;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos > 0)
        {
            pos = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                   gui_current_window->buffer->input_buffer_pos - 1);
            while (pos && (pos[0] == ' '))
            {
                pos = utf8_prev_char (gui_current_window->buffer->input_buffer, pos);
            }
            if (pos)
            {
                while (pos && (pos[0] != ' '))
                {
                    pos = utf8_prev_char (gui_current_window->buffer->input_buffer, pos);
                }
                if (pos)
                    pos = utf8_next_char (pos);
                else
                    pos = gui_current_window->buffer->input_buffer;
                gui_current_window->buffer->input_buffer_pos = utf8_pos (gui_current_window->buffer->input_buffer,
                                                             pos - gui_current_window->buffer->input_buffer);
            }
            else
                gui_current_window->buffer->input_buffer_pos = 0;
            
            gui_input_text_cursor_moved_signal ();
        }
    }
}

/*
 * gui_input_move_next_word: move cursor to the beginning of next word
 *                           (default key: meta-f or ctrl-right)
 */

void
gui_input_move_next_word ()
{
    char *pos;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos <
            gui_current_window->buffer->input_buffer_length)
        {
            pos = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                   gui_current_window->buffer->input_buffer_pos);
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
                    gui_current_window->buffer->input_buffer_pos =
                        utf8_pos (gui_current_window->buffer->input_buffer,
                                  pos - gui_current_window->buffer->input_buffer);
                else
                    gui_current_window->buffer->input_buffer_pos = 
                        gui_current_window->buffer->input_buffer_length;
            }
            else
                gui_current_window->buffer->input_buffer_pos =
                    utf8_pos (gui_current_window->buffer->input_buffer,
                              utf8_prev_char (gui_current_window->buffer->input_buffer, pos) - gui_current_window->buffer->input_buffer);
            
            gui_input_text_cursor_moved_signal ();
        }
    }
}

/*
 * gui_input_history_previous: recall previous command (default key: up)
 */

void
gui_input_history_previous ()
{
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
        {
            if (gui_current_window->buffer->ptr_history)
            {
                if (!gui_current_window->buffer->ptr_history->next_history)
                    return;
                gui_current_window->buffer->ptr_history =
                    gui_current_window->buffer->ptr_history->next_history;
                if (!gui_current_window->buffer->ptr_history)
                    gui_current_window->buffer->ptr_history =
                        gui_current_window->buffer->history;
            }
            else
                gui_current_window->buffer->ptr_history =
                    gui_current_window->buffer->history;
            if (gui_current_window->buffer->ptr_history)
            {
                /* bash/readline like use of history */
                if (gui_current_window->buffer->ptr_history->prev_history == NULL)
                {
                    if (gui_current_window->buffer->input_buffer_size > 0)
                    {
                        gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
                        gui_history_buffer_add (gui_current_window->buffer, gui_current_window->buffer->input_buffer);
                        gui_history_global_add (gui_current_window->buffer->input_buffer);
                    }
                }
                else
                {
                    if (gui_current_window->buffer->input_buffer_size > 0)
                    {
                        gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
                        if (gui_current_window->buffer->ptr_history->prev_history->text)
                            free(gui_current_window->buffer->ptr_history->prev_history->text);
                        gui_current_window->buffer->ptr_history->prev_history->text = strdup (gui_current_window->buffer->input_buffer);
                    }
                }
                gui_current_window->buffer->input_buffer_size =
                    strlen (gui_current_window->buffer->ptr_history->text);
                gui_current_window->buffer->input_buffer_length =
                    utf8_strlen (gui_current_window->buffer->ptr_history->text);
                gui_input_optimize_size (gui_current_window->buffer);
                gui_current_window->buffer->input_buffer_pos =
                    gui_current_window->buffer->input_buffer_length;
                gui_current_window->buffer->input_buffer_1st_display = 0;
                strcpy (gui_current_window->buffer->input_buffer,
                        gui_current_window->buffer->ptr_history->text);
            }
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
        }
        else
        {
            /* search backward in buffer history */
            gui_current_window->buffer->text_search = GUI_TEXT_SEARCH_BACKWARD;
            (void) gui_window_search_text (gui_current_window);
        }
    }
}

/*
 * gui_input_history_next: recall next command (default key: down)
 */

void
gui_input_history_next ()
{
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
        {
            if (gui_current_window->buffer->ptr_history)
            {
                gui_current_window->buffer->ptr_history =
                    gui_current_window->buffer->ptr_history->prev_history;
                if (gui_current_window->buffer->ptr_history)
                {
                    gui_current_window->buffer->input_buffer_size =
                        strlen (gui_current_window->buffer->ptr_history->text);
                    gui_current_window->buffer->input_buffer_length =
                        utf8_strlen (gui_current_window->buffer->ptr_history->text);
                }
                else
                {
                    gui_current_window->buffer->input_buffer[0] = '\0';
                    gui_current_window->buffer->input_buffer_size = 0;
                    gui_current_window->buffer->input_buffer_length = 0;
                }
                gui_input_optimize_size (gui_current_window->buffer);
                gui_current_window->buffer->input_buffer_pos =
                    gui_current_window->buffer->input_buffer_length;
                gui_current_window->buffer->input_buffer_1st_display = 0;
                if (gui_current_window->buffer->ptr_history)
                {
                    strcpy (gui_current_window->buffer->input_buffer,
                            gui_current_window->buffer->ptr_history->text);
                }
            }
            else
            {
                /* add line to history then clear input */
                if (gui_current_window->buffer->input_buffer_size > 0)
                {
                    gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
                    gui_history_buffer_add (gui_current_window->buffer, gui_current_window->buffer->input_buffer);
                    gui_history_global_add (gui_current_window->buffer->input_buffer);
                    gui_current_window->buffer->input_buffer[0] = '\0';
                    gui_current_window->buffer->input_buffer_size = 0;
                    gui_current_window->buffer->input_buffer_length = 0;
                    gui_current_window->buffer->input_buffer_pos = 0;
                    gui_current_window->buffer->input_buffer_1st_display = 0;
                    gui_input_optimize_size (gui_current_window->buffer);
                }
            }
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
        }
        else
        {
            /* search forward in buffer history */
            gui_current_window->buffer->text_search = GUI_TEXT_SEARCH_FORWARD;
            (void) gui_window_search_text (gui_current_window);
        }
    }
}

/*
 * gui_input_history_global_previous: recall previous command in global history
 *                                    (default key: ctrl-up)
 */

void
gui_input_history_global_previous ()
{
    if ((gui_current_window->buffer->input)
        && (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        if (history_global_ptr)
        {
            history_global_ptr = history_global_ptr->next_history;
            if (!history_global_ptr)
                history_global_ptr = history_global;
        }
        else
            history_global_ptr = history_global;
        if (history_global_ptr)
        {
            gui_current_window->buffer->input_buffer_size =
                strlen (history_global_ptr->text);
            gui_current_window->buffer->input_buffer_length =
                utf8_strlen (history_global_ptr->text);
            gui_input_optimize_size (gui_current_window->buffer);
            gui_current_window->buffer->input_buffer_pos =
                gui_current_window->buffer->input_buffer_length;
            gui_current_window->buffer->input_buffer_1st_display = 0;
            strcpy (gui_current_window->buffer->input_buffer,
                    history_global_ptr->text);
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
        }
    }
}

/*
 * gui_history_global_next: recall next command in global history
 *                          (default key: ctrl-down)
 */

void
gui_input_history_global_next ()
{
    if ((gui_current_window->buffer->input)
        && (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        if (history_global_ptr)
        {
            history_global_ptr = history_global_ptr->prev_history;
            if (history_global_ptr)
            {
                gui_current_window->buffer->input_buffer_size =
                    strlen (history_global_ptr->text);
                gui_current_window->buffer->input_buffer_length =
                    utf8_strlen (history_global_ptr->text);
            }
            else
            {
                gui_current_window->buffer->input_buffer_size = 0;
                gui_current_window->buffer->input_buffer_length = 0;
            }
            gui_input_optimize_size (gui_current_window->buffer);
            gui_current_window->buffer->input_buffer_pos =
                gui_current_window->buffer->input_buffer_length;
            gui_current_window->buffer->input_buffer_1st_display = 0;
            if (history_global_ptr)
            {
                strcpy (gui_current_window->buffer->input_buffer,
                        history_global_ptr->text);
            }
            gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
        }
    }
}

/*
 * gui_input_jump_smart: jump to buffer with activity (default key: alt-A)
 */

void
gui_input_jump_smart ()
{
    if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        if (gui_hotlist)
        {
            if (!gui_hotlist_initial_buffer)
                gui_hotlist_initial_buffer = gui_current_window->buffer;
            gui_window_switch_to_buffer (gui_current_window,
                                         gui_hotlist->buffer, 1);
            gui_window_scroll_bottom (gui_current_window);
        }
        else
        {
            if (gui_hotlist_initial_buffer)
            {
                gui_window_switch_to_buffer (gui_current_window,
                                             gui_hotlist_initial_buffer, 1);
                gui_window_scroll_bottom (gui_current_window);
                gui_hotlist_initial_buffer = NULL;
            }
        }
    }
}

/*
 * gui_input_jump_last_buffer: jump to last buffer
 *                             (default key: meta-j, meta-l)
 */

void
gui_input_jump_last_buffer ()
{
    if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        if (last_gui_buffer)
            gui_buffer_switch_by_number (gui_current_window,
                                         last_gui_buffer->number);
    }
}

/*
 * gui_input_jump_previously_visited_buffer: jump to previously visited buffer
 *                                           (buffer displayed before current one)
 *                                           (default key: meta-<)
 */

void
gui_input_jump_previously_visited_buffer ()
{
    int index;
    struct t_gui_buffer_visited *ptr_buffer_visited;
    
    if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        index = gui_buffer_visited_get_index_previous ();
        if (index >= 0)
        {
            gui_buffers_visited_index = index;
            log_printf ("prev: index = %d", index);
            
            ptr_buffer_visited = gui_buffer_visited_search_by_number (gui_buffers_visited_index);
            if (ptr_buffer_visited)
            {
                gui_buffers_visited_frozen = 1;
                gui_buffer_switch_by_number (gui_current_window,
                                             ptr_buffer_visited->buffer->number);
                gui_buffers_visited_frozen = 0;
            }
        }
    }
}

/*
 * gui_input_jump_next_visited_buffer: jump to next visited buffer
 *                                     (buffer displayed after current one)
 *                                     (default key: meta->)
 */

void
gui_input_jump_next_visited_buffer ()
{
    int index;
    struct t_gui_buffer_visited *ptr_buffer_visited;
    
    if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        index = gui_buffer_visited_get_index_next ();
        if (index >= 0)
        {
            gui_buffers_visited_index = index;
            
            ptr_buffer_visited = gui_buffer_visited_search_by_number (gui_buffers_visited_index);
            if (ptr_buffer_visited)
            {
                gui_buffers_visited_frozen = 1;
                gui_buffer_switch_by_number (gui_current_window,
                                             ptr_buffer_visited->buffer->number);
                gui_buffers_visited_frozen = 0;
            }
        }
    }
}

/*
 * gui_input_hotlist_clear: clear hotlist (default key: meta-h)
 */

void
gui_input_hotlist_clear ()
{
    gui_hotlist_clear ();
    gui_hotlist_initial_buffer = gui_current_window->buffer;
}

/*
 * gui_input_grab_key: init "grab key mode" (next key will be inserted into
 *                     input buffer) (default key: meta-k)
 */

void
gui_input_grab_key ()
{
    if (gui_current_window->buffer->input)
        gui_keyboard_grab_init ();
}

/*
 * gui_input_scroll_unread: scroll to first unread line of buffer
 *                          (default key: meta-u)
 */

void
gui_input_scroll_unread ()
{
    if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        if (CONFIG_STRING(config_look_read_marker) &&
            CONFIG_STRING(config_look_read_marker)[0] &&
            (gui_current_window->buffer->type == GUI_BUFFER_TYPE_FORMATTED) &&
            (gui_current_window->buffer->first_line_not_read ||
             (gui_current_window->buffer->last_read_line &&
              gui_current_window->buffer->last_read_line != gui_current_window->buffer->last_line)))
        {
            if (gui_current_window->buffer->first_line_not_read)
                gui_current_window->start_line = gui_current_window->buffer->lines;
            else
                gui_current_window->start_line = gui_current_window->buffer->last_read_line->next_line;
            gui_current_window->start_line_pos = 0;
            gui_current_window->first_line_displayed =
                (gui_current_window->start_line == gui_chat_get_first_line_displayed (gui_current_window->buffer));
            gui_buffer_ask_chat_refresh (gui_current_window->buffer, 2);
        }
    }
}

/*
 * gui_input_set_unread: set unread marker for all buffers
 *                       (default key: ctrl-s, ctrl-u)
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
 * gui_input_set_unread_current_buffer: set unread marker for current buffer
 */

void
gui_input_set_unread_current_buffer ()
{
    if (gui_current_window)
        gui_buffer_set_unread (gui_current_window->buffer);
}

/*
 * gui_input_insert: insert a string in command line
 *                   (many default keys are bound to this function)
 */

void
gui_input_insert (const char *args)
{
    char *args2;
    
    if (args)
    {
        args2 = string_convert_hex_chars (args);
        gui_input_insert_string (gui_current_window->buffer, (args2) ? args2 : args, -1);
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer);
        if (args2)
            free (args2);
    }
}
