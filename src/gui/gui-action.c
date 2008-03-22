/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* gui-action.c: GUI actions */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-input.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "gui-action.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-completion.h"
#include "gui-history.h"
#include "gui-hotlist.h"
#include "gui-infobar.h"
#include "gui-input.h"
#include "gui-keyboard.h"
#include "gui-status.h"
#include "gui-window.h"


/*
 * gui_action_clipboard_copy: copy string into clipboard
 */

void
gui_action_clipboard_copy (char *buffer, int size)
{
    if (size <= 0)
        return;
    
    if (gui_input_clipboard != NULL)
        free (gui_input_clipboard);
    
    gui_input_clipboard = (char *)malloc((size + 1) * sizeof(*gui_input_clipboard));
    
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
gui_action_clipboard_paste (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    if ((gui_current_window->buffer->input)
        && gui_input_clipboard) 
    {
        gui_input_insert_string (gui_current_window->buffer,
                                 gui_input_clipboard, -1);
        gui_current_window->buffer->completion->position = -1;
        gui_input_draw (gui_current_window->buffer, 0);
    }
}

/*
 * gui_action_return: terminate line (return pressed)
 */

void
gui_action_return (char *args)
{
    char *command;
    
    /* make C compiler happy */
    (void) args;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
            gui_window_search_stop (gui_current_window);
        else if (gui_current_window->buffer->input_buffer_size > 0)
        {
            gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_current_window->buffer->input_buffer_color_mask[gui_current_window->buffer->input_buffer_size] = '\0';
            command = strdup (gui_current_window->buffer->input_buffer);
            if (!command)
                return;
            gui_history_buffer_add (gui_current_window->buffer,
                                    gui_current_window->buffer->input_buffer);
            gui_history_global_add (gui_current_window->buffer->input_buffer);
            gui_current_window->buffer->input_buffer[0] = '\0';
            gui_current_window->buffer->input_buffer_color_mask[0] = '\0';
            gui_current_window->buffer->input_buffer_size = 0;
            gui_current_window->buffer->input_buffer_length = 0;
            gui_current_window->buffer->input_buffer_pos = 0;
            gui_current_window->buffer->input_buffer_1st_display = 0;
            gui_current_window->buffer->completion->position = -1;
            gui_current_window->buffer->ptr_history = NULL;
            gui_input_optimize_size (gui_current_window->buffer);
            gui_input_draw (gui_current_window->buffer, 0);
            input_data (gui_current_window->buffer, command, 0);
            free (command);
        }
    }
}

/*
 * gui_action_tab: tab key => completion
 */

void
gui_action_tab (char *args)
{
    /* make C compiler happy */
    (void) args;
    
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
    }
}

/*
 * gui_action_tab_previous: shift-tab key => find previous completion
 */

void
gui_action_tab_previous (char *args)
{
    /* make C compiler happy */
    (void) args;
    
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
    }
}

/*
 * gui_action_backspace: backspace key
 */

void
gui_action_backspace (char *args)
{
    char *pos, *pos_last;
    int char_size, size_to_move;
    
    /* make C compiler happy */
    (void) args;
    
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
            gui_current_window->buffer->input_buffer_color_mask[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_input_optimize_size (gui_current_window->buffer);
            gui_input_draw (gui_current_window->buffer, 0);
            gui_current_window->buffer->completion->position = -1;
        }
    }
}

/*
 * gui_action_delete: delete next char
 */

void
gui_action_delete (char *args)
{
    char *pos, *pos_next;
    int char_size, size_to_move;

    /* make C compiler happy */
    (void) args;
    
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
            gui_current_window->buffer->input_buffer_color_mask[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_input_optimize_size (gui_current_window->buffer);
            gui_input_draw (gui_current_window->buffer, 0);
            gui_current_window->buffer->completion->position = -1;
        }
    }
}

/*
 * gui_action_delete_previous_word: delete previous word 
 */

void
gui_action_delete_previous_word (char *args)
{
    int length_deleted, size_deleted;
    char *start, *string;
    
    /* make C compiler happy */
    (void) args;
    
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
            
            gui_action_clipboard_copy (string, size_deleted);
            
            gui_input_move (gui_current_window->buffer, string, string + size_deleted, strlen (string + size_deleted));
            
            gui_current_window->buffer->input_buffer_size -= size_deleted;
            gui_current_window->buffer->input_buffer_length -= length_deleted;
            gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_current_window->buffer->input_buffer_color_mask[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_current_window->buffer->input_buffer_pos -= length_deleted;
            gui_input_optimize_size (gui_current_window->buffer);
            gui_input_draw (gui_current_window->buffer, 0);
            gui_current_window->buffer->completion->position = -1;
        }
    }
}

/*
 * gui_action_delete_next_word: delete next word 
 */

void
gui_action_delete_next_word (char *args)
{
    int size_deleted, length_deleted;
    char *start, *string;
    
    /* make C compiler happy */
    (void) args;
    
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
        
        gui_action_clipboard_copy(start, size_deleted);
        
        gui_input_move (gui_current_window->buffer, start, string, strlen (string));
        
        gui_current_window->buffer->input_buffer_size -= size_deleted;
        gui_current_window->buffer->input_buffer_length -= length_deleted;
        gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
        gui_current_window->buffer->input_buffer_color_mask[gui_current_window->buffer->input_buffer_size] = '\0';
        gui_input_optimize_size (gui_current_window->buffer);
        gui_input_draw (gui_current_window->buffer, 0);
        gui_current_window->buffer->completion->position = -1;
    }
}

/*
 * gui_action_delete_begin_of_line: delete all from cursor pos to beginning of line
 */

void
gui_action_delete_begin_of_line (char *args)
{
    int length_deleted, size_deleted, pos_start;
    char *start;

    /* make C compiler happy */
    (void) args;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos > 0)
        {
            start = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                     gui_current_window->buffer->input_buffer_pos);
            pos_start = start - gui_current_window->buffer->input_buffer;
            size_deleted = start - gui_current_window->buffer->input_buffer;
            length_deleted = utf8_strnlen (gui_current_window->buffer->input_buffer, size_deleted);
            gui_action_clipboard_copy (gui_current_window->buffer->input_buffer,
                                       start - gui_current_window->buffer->input_buffer);
            
            gui_input_move (gui_current_window->buffer, gui_current_window->buffer->input_buffer, start, strlen (start));
            
            gui_current_window->buffer->input_buffer_size -= size_deleted;
            gui_current_window->buffer->input_buffer_length -= length_deleted;
            gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_current_window->buffer->input_buffer_color_mask[gui_current_window->buffer->input_buffer_size] = '\0';
            gui_current_window->buffer->input_buffer_pos = 0;
            gui_input_optimize_size (gui_current_window->buffer);
            gui_input_draw (gui_current_window->buffer, 0);
            gui_current_window->buffer->completion->position = -1;
        }
    }
}

/*
 * gui_action_delete_end_of_line: delete all from cursor pos to end of line
 */

void
gui_action_delete_end_of_line (char *args)
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
        gui_action_clipboard_copy (start, size_deleted);
        start[0] = '\0';
        gui_current_window->buffer->input_buffer_color_mask[pos_start] = '\0';
        gui_current_window->buffer->input_buffer_size = strlen (gui_current_window->buffer->input_buffer);
        gui_current_window->buffer->input_buffer_length = utf8_strlen (gui_current_window->buffer->input_buffer);
        gui_input_optimize_size (gui_current_window->buffer);
        gui_input_draw (gui_current_window->buffer, 0);
        gui_current_window->buffer->completion->position = -1;
    }
}

/*
 * gui_action_delete_line: delete entire line
 */

void
gui_action_delete_line (char *args)
{
    /* make C compiler happy */
    (void) args;

    gui_input_delete_line (gui_current_window->buffer);
    gui_input_draw (gui_current_window->buffer, 0);
}

/*
 * gui_action_transpose_chars: transpose chars (on lth left) at cursor pos
 */

void
gui_action_transpose_chars (char *args)
{
    /* make C compiler happy */
    (void) args;
    
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
            
            memcpy (saved_char, gui_current_window->buffer->input_buffer_color_mask + pos_prev_char, size_prev_char);
            memcpy (gui_current_window->buffer->input_buffer_color_mask + pos_prev_char,
                    gui_current_window->buffer->input_buffer_color_mask + pos_start, size_start_char);
            memcpy (gui_current_window->buffer->input_buffer_color_mask + pos_prev_char + size_start_char,
                    saved_char, size_prev_char);
            
            gui_current_window->buffer->input_buffer_pos++;
            
            gui_input_draw (gui_current_window->buffer, 0);	
            gui_current_window->buffer->completion->position = -1;
        }
    }
}

/*
 * gui_action_home: home key
 */

void
gui_action_home (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos > 0)
        {
            gui_current_window->buffer->input_buffer_pos = 0;
            gui_input_draw (gui_current_window->buffer, 0);
        }
    }
}

/*
 * gui_action_end: end key
 */

void
gui_action_end (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos <
            gui_current_window->buffer->input_buffer_length)
        {
            gui_current_window->buffer->input_buffer_pos =
                gui_current_window->buffer->input_buffer_length;
            gui_input_draw (gui_current_window->buffer, 0);
        }
    }
}

/*
 * gui_action_left: move to previous char
 */

void
gui_action_left (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos > 0)
        {
            gui_current_window->buffer->input_buffer_pos--;
            gui_input_draw (gui_current_window->buffer, 0);
        }
    }
}

/*
 * gui_action_previous_word: move to beginning of previous word
 */

void
gui_action_previous_word (char *args)
{
    char *pos;
    
    /* make C compiler happy */
    (void) args;
    
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
            
            gui_input_draw (gui_current_window->buffer, 0);
        }
    }
}

/*
 * gui_action_right: move to next char
 */

void
gui_action_right (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->input_buffer_pos <
            gui_current_window->buffer->input_buffer_length)
        {
            gui_current_window->buffer->input_buffer_pos++;
            gui_input_draw (gui_current_window->buffer, 0);
        }
    }
}

/*
 * gui_action_next_word: move to the end of next
 */

void
gui_action_next_word (char *args)
{
    char *pos;
    
    /* make C compiler happy */
    (void) args;
    
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
            
            gui_input_draw (gui_current_window->buffer, 0);
        }
    }
}

/*
 * gui_action_up: recall last command or move to previous DCC in list
 */

void
gui_action_up (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    /*if (gui_current_window->buffer->type == GUI_BUFFER_TYPE_FREE)
    {
        if (irc_dcc_list)
        {
            if (window->dcc_selected
                && ((t_irc_dcc *)(window->dcc_selected))->prev_dcc)
            {
                if (window->dcc_selected ==
                    window->dcc_first)
                    window->dcc_first =
                        ((t_irc_dcc *)(window->dcc_first))->prev_dcc;
                window->dcc_selected =
                    ((t_irc_dcc *)(window->dcc_selected))->prev_dcc;
                gui_current_window->buffer->chat_refresh_needed = 1;
                gui_current_window->buffer->input_refresh_needed = 1;
            }
        }
    }
    else*/ if (gui_current_window->buffer->input)
    {
        if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
        {
            if (gui_current_window->buffer->ptr_history)
            {
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
                        gui_current_window->buffer->input_buffer_color_mask[gui_current_window->buffer->input_buffer_size] = '\0';
                        gui_history_buffer_add (gui_current_window->buffer, gui_current_window->buffer->input_buffer);
                        gui_history_global_add (gui_current_window->buffer->input_buffer);
                    }
                }
                else
                {
                    if (gui_current_window->buffer->input_buffer_size > 0)
                    {
                        gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
                        gui_current_window->buffer->input_buffer_color_mask[gui_current_window->buffer->input_buffer_size] = '\0';
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
                gui_input_init_color_mask (gui_current_window->buffer);
                gui_input_draw (gui_current_window->buffer, 0);
            }
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
 * gui_action_up_global: recall last command in global history
 */

void
gui_action_up_global (char *args)
{
    /* make C compiler happy */
    (void) args;
    
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
            gui_input_init_color_mask (gui_current_window->buffer);
            gui_input_draw (gui_current_window->buffer, 0);
        }
    }
}

/*
 * gui_action_down: recall next command or move to next DCC in list
 */

void
gui_action_down (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    /*if (gui_current_window->buffer->type == GUI_BUFFER_TYPE_FREE)
    {
        if (irc_dcc_list)
        {
            if (!window->dcc_selected
                || ((t_irc_dcc *)(window->dcc_selected))->next_dcc)
            {
                if (window->dcc_last_displayed
                    && (window->dcc_selected ==
                        window->dcc_last_displayed))
                {
                    if (window->dcc_first)
                        window->dcc_first =
                            ((t_irc_dcc *)(window->dcc_first))->next_dcc;
                    else
                        window->dcc_first =
                            irc_dcc_list->next_dcc;
                }
                if (window->dcc_selected)
                    window->dcc_selected =
                        ((t_irc_dcc *)(window->dcc_selected))->next_dcc;
                else
                    window->dcc_selected =
                        irc_dcc_list->next_dcc;
                gui_current_window->buffer->chat_refresh_needed = 1;
                gui_current_window->buffer->input_refresh_needed = 1;
            }
        }
    }
    else */if (gui_current_window->buffer->input)
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
                    gui_input_init_color_mask (gui_current_window->buffer);
                }
                gui_input_draw (gui_current_window->buffer, 0);
            }
            else
            {
                /* add line to history then clear input */
                if (gui_current_window->buffer->input_buffer_size > 0)
                {
                    gui_current_window->buffer->input_buffer[gui_current_window->buffer->input_buffer_size] = '\0';
                    gui_current_window->buffer->input_buffer_color_mask[gui_current_window->buffer->input_buffer_size] = '\0';
                    gui_history_buffer_add (gui_current_window->buffer, gui_current_window->buffer->input_buffer);
                    gui_history_global_add (gui_current_window->buffer->input_buffer);
                    gui_current_window->buffer->input_buffer_size = 0;
                    gui_current_window->buffer->input_buffer_length = 0;
                    gui_input_optimize_size (gui_current_window->buffer);
                    gui_current_window->buffer->input_buffer_pos = 0;
                    gui_current_window->buffer->input_buffer_1st_display = 0;
                    gui_input_draw (gui_current_window->buffer, 0);
                }
            }
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
 * gui_action_down_global: recall next command in global history
 */

void
gui_action_down_global (char *args)
{
    /* make C compiler happy */
    (void) args;
    
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
                gui_input_init_color_mask (gui_current_window->buffer);
            }
            gui_input_draw (gui_current_window->buffer, 0);
        }
    }
}

/*
 * gui_action_page_up: display previous page on buffer
 */

void
gui_action_page_up (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_page_up (gui_current_window);
}

/*
 * gui_action_page_down: display next page on buffer
 */

void
gui_action_page_down (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_page_down (gui_current_window);
}

/*
 * gui_action_scroll_up: display previous few lines in buffer
 */

void
gui_action_scroll_up (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_scroll_up (gui_current_window);
}

/*
 * gui_action_scroll_down: display next few lines in buffer
 */

void
gui_action_scroll_down (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_scroll_down (gui_current_window);
}

/*
 * gui_action_scroll_top: scroll to top of buffer
 */

void
gui_action_scroll_top (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_scroll_top (gui_current_window);
}

/*
 * gui_action_scroll_bottom: scroll to bottom of buffer
 */

void
gui_action_scroll_bottom (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_scroll_bottom (gui_current_window);
}

/*
 * gui_action_scroll_topic_left: scroll left topic
 */

void
gui_action_scroll_topic_left (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_scroll_topic_left (gui_current_window);
}

/*
 * gui_action_scroll_topic_right: scroll right topic
 */

void
gui_action_scroll_topic_right (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_scroll_topic_right (gui_current_window);
}

/*
 * gui_action_nick_beginning: go to beginning of nicklist
 */

void
gui_action_nick_beginning (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_nick_beginning (gui_current_window);
}

/*
 * gui_action_nick_end: go to the end of nicklist
 */

void
gui_action_nick_end (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_nick_end (gui_current_window);
}

/*
 * gui_action_nick_page_up: scroll one page up in nicklist
 */

void
gui_action_nick_page_up (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_nick_page_up (gui_current_window);
}

/*
 * gui_action_nick_page_down: scroll one page down in nicklist
 */

void
gui_action_nick_page_down (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_nick_page_down (gui_current_window);
}

/*
 * gui_action_jump_smart: jump to buffer with activity (alt-A by default)
 */

void
gui_action_jump_smart (char *args)
{
    /* make C compiler happy */
    (void) args;

    if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        if (gui_hotlist)
        {
            if (!gui_hotlist_initial_buffer)
                gui_hotlist_initial_buffer = gui_current_window->buffer;
            gui_window_switch_to_buffer (gui_current_window,
                                         gui_hotlist->buffer);
            gui_window_redraw_buffer (gui_current_window->buffer);
        }
        else
        {
            if (gui_hotlist_initial_buffer)
            {
                gui_window_switch_to_buffer (gui_current_window,
                                             gui_hotlist_initial_buffer);
                gui_window_redraw_buffer (gui_current_window->buffer);
                gui_hotlist_initial_buffer = NULL;
            }
        }
    }
}

/*
 * gui_action_jump_dcc: jump to DCC buffer
 */

void
gui_action_jump_dcc (char *args)
{
    /* make C compiler happy */
    (void) args;

    /*if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        if (gui_current_window->buffer->type == GUI_BUFFER_TYPE_DCC)
        {
            if (gui_buffer_before_dcc)
            {
                gui_window_switch_to_buffer (window,
                                             gui_buffer_before_dcc);
                gui_window_redraw_buffer (gui_current_window->buffer);
            }
        }
        else
        {
            gui_buffer_before_dcc = buffer;
            gui_buffer_switch_dcc (window);
        }
        }*/
}

/*
 * gui_action_jump_last_buffer: jump to last buffer
 */

void
gui_action_jump_last_buffer (char *args)
{
    /* make C compiler happy */
    (void) args;

    if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        if (last_gui_buffer)
            gui_buffer_switch_by_number (gui_current_window,
                                         last_gui_buffer->number);
    }
}

/*
 * gui_action_jump_previous_buffer: jump to previous buffer (the one displayed
 *                                  before current one)
 */

void
gui_action_jump_previous_buffer (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        if (gui_previous_buffer)
            gui_buffer_switch_by_number (gui_current_window,
                                         gui_previous_buffer->number);
    }
}

/*
 * gui_action_jump_server: jump to server buffer
 */

void
gui_action_jump_server (char *args)
{
    /* make C compiler happy */
    (void) args;

    /*if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        if (GUI_SERVER(buffer))
        {
            if (GUI_SERVER(buffer)->buffer !=
                buffer)
            {
                gui_window_switch_to_buffer (window,
                                             GUI_SERVER(buffer)->buffer);
                gui_window_redraw_buffer (gui_current_window->buffer);
            }
        }
        }*/
}

/*
 * gui_action_jump_next_server: jump to next server
 */

void
gui_action_jump_next_server (char *args)
{
    //t_irc_server *ptr_server;
    //t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) args;

    /*if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        if (GUI_SERVER(buffer))
        {
            ptr_server = GUI_SERVER(buffer)->next_server;
            if (!ptr_server)
                ptr_server = irc_servers;
            while (ptr_server != GUI_SERVER(buffer))
            {
                if (ptr_server->buffer)
                    break;
                ptr_server = (ptr_server->next_server) ?
                    ptr_server->next_server : irc_servers;
            }
            if (ptr_server != GUI_SERVER(buffer))
            {
                // save current buffer
                GUI_SERVER(buffer)->saved_buffer = buffer;
                
                // come back to memorized chan if found
                if (ptr_server->saved_buffer)
                    ptr_buffer = ptr_server->saved_buffer;
                else
                    ptr_buffer = (ptr_server->channels) ?
                        ptr_server->channels->buffer : ptr_server->buffer;
                if ((ptr_server->buffer == ptr_buffer)
                    && (ptr_buffer->all_servers))
                    ptr_buffer->server = ptr_server;
                gui_window_switch_to_buffer (window, ptr_buffer);
                gui_window_redraw_buffer (gui_current_window->buffer);
            }
        }
        }*/
}

/*
 * gui_action_switch_server: switch server on servers buffer
 *                          (if same buffer is used for all buffers)
 */

void
gui_action_switch_server (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_switch_server (gui_current_window);
}

/*
 * gui_action_scroll_previous_highlight: scroll to previous highlight
 */

void
gui_action_scroll_previous_highlight (char *args)
{
    //t_gui_line *ptr_line;
    
    /* make C compiler happy */
    (void) args;
    
    /*if ((gui_current_window->buffer->type == GUI_BUFFER_TYPE_FORMATED)
        && (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        if (gui_current_window->buffer->lines)
        {
            ptr_line = (window->start_line) ?
                window->start_line->prev_line : gui_current_window->buffer->last_line;
            while (ptr_line)
            {
                if (ptr_line->line_with_highlight)
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == gui_current_window->buffer->lines);
                    gui_current_window->buffer->chat_refresh_needed = 1;
                    gui_current_window->buffer->input_refresh_needed = 1;
                    return;
                }
                ptr_line = ptr_line->prev_line;
            }
        }
    }*/
}

/*
 * gui_action_scroll_next_highlight: scroll to next highlight
 */

void
gui_action_scroll_next_highlight (char *args)
{
    //t_gui_line *ptr_line;
    
    /* make C compiler happy */
    (void) args;
    
    /*if ((gui_current_window->buffer->type == GUI_BUFFER_TYPE_FORMATED)
        && (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED))
    {
        if (gui_current_window->buffer->lines)
        {
            ptr_line = (window->start_line) ?
                window->start_line->next_line : gui_current_window->buffer->lines->next_line;
            while (ptr_line)
            {
                if (ptr_line->line_with_highlight)
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == gui_current_window->buffer->lines);
                    gui_current_window->buffer->chat_refresh_needed = 1;
                    gui_current_window->buffer->input_refresh_needed = 1;
                    return;
                }
                ptr_line = ptr_line->next_line;
            }
        }
    }*/
}

/*
 * gui_action_scroll_unread: scroll to first unread line of buffer
 */

void
gui_action_scroll_unread (char *args)
{
    /* make C compiler happy */
    (void) args;

    if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
    {
        if (CONFIG_STRING(config_look_read_marker) &&
            CONFIG_STRING(config_look_read_marker)[0] &&
            (gui_current_window->buffer->type == GUI_BUFFER_TYPE_FORMATED) &&
            gui_current_window->buffer->last_read_line &&
            gui_current_window->buffer->last_read_line != gui_current_window->buffer->last_line)
        {
            gui_current_window->start_line =
                gui_current_window->buffer->last_read_line->next_line;
            gui_current_window->start_line_pos = 0;
            gui_current_window->first_line_displayed =
                (gui_current_window->start_line == gui_chat_get_first_line_displayed (gui_current_window->buffer));
            gui_current_window->buffer->chat_refresh_needed = 1;
            gui_status_refresh_needed = 1;
        }
    }
}

/*
 * gui_action_set_unread: set unread marker for all buffers
 */

void
gui_action_set_unread (char *args)
{
    struct t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) args;

    /* set read marker for all standard buffers */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->type == GUI_BUFFER_TYPE_FORMATED)
            ptr_buffer->last_read_line = ptr_buffer->last_line;
    }
    
    /* refresh all windows */
    gui_window_redraw_all_buffers ();
}

/*
 * gui_action_hotlist_clear: clear hotlist
 */

void
gui_action_hotlist_clear (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    if (gui_hotlist)
    {
        gui_hotlist_free_all (&gui_hotlist, &last_gui_hotlist);
        gui_window_redraw_buffer (gui_current_window->buffer);
    }
    gui_hotlist_initial_buffer = gui_current_window->buffer;
}

/*
 * gui_action_infobar_clear: clear infobar
 */

void
gui_action_infobar_clear (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_infobar_remove ();
    gui_infobar_draw (gui_current_window->buffer, 1);
}

/*
 * gui_action_refresh: refresh screen
 */

void
gui_action_refresh_screen (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    gui_window_refresh_screen (1);
}

/*
 * gui_action_grab_key: init "grab key mode" (next key will be inserted into input buffer)
 */

void
gui_action_grab_key (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    if (gui_current_window->buffer->input)
        gui_keyboard_grab_init ();
}

/*
 * gui_action_insert_string: insert a string in command line
 */

void
gui_action_insert_string (char *args)
{
    char *args2;
    
    if (args)
    {
        args2 = string_convert_hex_chars (args);
        gui_input_insert_string (gui_current_window->buffer, (args2) ? args2 : args, -1);
        gui_input_draw (gui_current_window->buffer, 0);
        if (args2)
            free (args2);
    }
}

/*
 * gui_action_search_text: search text in buffer history
 */

void
gui_action_search_text (char *args)
{
    /* make C compiler happy */
    (void) args;
    
    if (gui_current_window->buffer->type == GUI_BUFFER_TYPE_FORMATED)
    {
        /* toggle search */
        if (gui_current_window->buffer->text_search == GUI_TEXT_SEARCH_DISABLED)
            gui_window_search_start (gui_current_window);
        else
        {
            gui_current_window->buffer->text_search_exact ^= 1;
            gui_window_search_restart (gui_current_window);
            gui_input_draw (gui_current_window->buffer, 1);
        }
    }
}
