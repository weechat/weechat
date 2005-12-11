/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* gui-common.c: display functions, used by all GUI */


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

#include "../common/weechat.h"
#include "gui.h"
#include "../common/command.h"
#include "../common/weeconfig.h"
#include "../common/history.h"
#include "../common/hotlist.h"
#include "../common/log.h"
#include "../common/utf8.h"
#include "../irc/irc.h"


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
    
    gui_input_clipboard = (char *) malloc( (size + 1) * sizeof(*gui_input_clipboard));
    
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
gui_action_clipboard_paste (t_gui_window *window)
{
    if (window->buffer->has_input && gui_input_clipboard) 
    {
        gui_insert_string_input (window, gui_input_clipboard, -1);
        window->buffer->input_buffer_pos += utf8_strlen (gui_input_clipboard);
        gui_draw_buffer_input (window->buffer, 0);
        window->buffer->completion.position = -1;
    }
}

/*
 * gui_action_return: terminate line (return pressed)
 */

void
gui_action_return (t_gui_window *window)
{
    char *command;
    
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_size > 0)
        {
            window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            command = strdup (window->buffer->input_buffer);
            if (!command)
                return;
            history_buffer_add (window->buffer, window->buffer->input_buffer);
            history_global_add (window->buffer->input_buffer);
            window->buffer->input_buffer[0] = '\0';
            window->buffer->input_buffer_size = 0;
            window->buffer->input_buffer_length = 0;
            window->buffer->input_buffer_pos = 0;
            window->buffer->input_buffer_1st_display = 0;
            window->buffer->completion.position = -1;
            window->buffer->ptr_history = NULL;
            gui_draw_buffer_input (window->buffer, 0);
            user_command (window, SERVER(window->buffer), command);
            free (command);
        }
    }
}

/*
 * gui_action_tab: tab key => completion
 */

void
gui_action_tab (t_gui_window *window)
{
    int i;

    if (window->buffer->has_input)
    {
        completion_search (&(window->buffer->completion),
                           SERVER(window->buffer),
                           CHANNEL(window->buffer),
                           window->buffer->input_buffer,
                           window->buffer->input_buffer_size,
                           utf8_real_pos (window->buffer->input_buffer,
                                          window->buffer->input_buffer_pos));
        
        if (window->buffer->completion.word_found)
        {
            /* replace word with new completed word into input buffer */
            if (window->buffer->completion.diff_size > 0)
            {
                window->buffer->input_buffer_size +=
                    window->buffer->completion.diff_size;
                window->buffer->input_buffer_length +=
                    window->buffer->completion.diff_length;
                gui_optimize_input_buffer_size (window->buffer);
                window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
                for (i = window->buffer->input_buffer_size - 1;
                     i >=  window->buffer->completion.position_replace +
                         (int)strlen (window->buffer->completion.word_found); i--)
                    window->buffer->input_buffer[i] =
                        window->buffer->input_buffer[i - window->buffer->completion.diff_size];
            }
            else
            {
                for (i = window->buffer->completion.position_replace +
                         strlen (window->buffer->completion.word_found);
                     i < window->buffer->input_buffer_size; i++)
                    window->buffer->input_buffer[i] =
                        window->buffer->input_buffer[i - window->buffer->completion.diff_size];
                window->buffer->input_buffer_size +=
                    window->buffer->completion.diff_size;
                window->buffer->input_buffer_length +=
                    window->buffer->completion.diff_length;
                gui_optimize_input_buffer_size (window->buffer);
                window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            }
            
            strncpy (window->buffer->input_buffer + window->buffer->completion.position_replace,
                     window->buffer->completion.word_found,
                     strlen (window->buffer->completion.word_found));
            window->buffer->input_buffer_pos =
                utf8_pos (window->buffer->input_buffer,
                          window->buffer->completion.position_replace) +
                utf8_strlen (window->buffer->completion.word_found);
                        
            /* position is < 0 this means only one word was found to complete,
               so reinit to stop completion */
            if (window->buffer->completion.position >= 0)
                window->buffer->completion.position =
                    utf8_real_pos (window->buffer->input_buffer,
                                   window->buffer->input_buffer_pos);
                        
            /* add space or completor to the end of completion, if needed */
            if ((window->buffer->completion.context == COMPLETION_COMMAND)
                || (window->buffer->completion.context == COMPLETION_COMMAND_ARG))
            {
                if (window->buffer->input_buffer[utf8_real_pos (window->buffer->input_buffer,
                                                                window->buffer->input_buffer_pos)] != ' ')
                    gui_insert_string_input (window, " ",
                                             window->buffer->input_buffer_pos);
                if (window->buffer->completion.position >= 0)
                    window->buffer->completion.position++;
                window->buffer->input_buffer_pos++;
            }
            else
            {
                /* add nick completor if position 0 and completing nick */
                if ((window->buffer->completion.base_word_pos == 0)
                    && (window->buffer->completion.context == COMPLETION_NICK))
                {
                    if (strncmp (utf8_add_offset (window->buffer->input_buffer,
                                                  window->buffer->input_buffer_pos),
                                 cfg_look_completor, strlen (cfg_look_completor)) != 0)
                        gui_insert_string_input (window, cfg_look_completor,
                                                 window->buffer->input_buffer_pos);
                    if (window->buffer->completion.position >= 0)
                        window->buffer->completion.position += strlen (cfg_look_completor);
                    window->buffer->input_buffer_pos += utf8_strlen (cfg_look_completor);
                    if (window->buffer->input_buffer[utf8_real_pos (window->buffer->input_buffer,
                                                                    window->buffer->input_buffer_pos)] != ' ')
                        gui_insert_string_input (window, " ",
                                                 window->buffer->input_buffer_pos);
                    if (window->buffer->completion.position >= 0)
                        window->buffer->completion.position++;
                    window->buffer->input_buffer_pos++;
                }
            }
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_backspace: backspace key
 */

void
gui_action_backspace (t_gui_window *window)
{
    char *pos, *pos_last;
    int char_size, size_to_move;

    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos > 0)
        {
            pos = utf8_add_offset (window->buffer->input_buffer,
                                   window->buffer->input_buffer_pos);
            pos_last = utf8_prev_char (window->buffer->input_buffer, pos);
            char_size = pos - pos_last;
            size_to_move = strlen (pos);
            memmove (pos_last, pos, size_to_move);
            window->buffer->input_buffer_size -= char_size;
            window->buffer->input_buffer_length--;
            window->buffer->input_buffer_pos--;
            window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            gui_optimize_input_buffer_size (window->buffer);
            gui_draw_buffer_input (window->buffer, 0);
            window->buffer->completion.position = -1;
        }
    }
}

/*
 * gui_action_delete: delete key
 */

void
gui_action_delete (t_gui_window *window)
{
    char *pos, *pos_next;
    int char_size, size_to_move;
    
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos <
            window->buffer->input_buffer_length)
        {
            pos = utf8_add_offset (window->buffer->input_buffer,
                                   window->buffer->input_buffer_pos);
            pos_next = utf8_next_char (pos);
            char_size = pos_next - pos;
            size_to_move = strlen (pos_next);
            memmove (pos, pos_next, size_to_move);
            window->buffer->input_buffer_size -= char_size;
            window->buffer->input_buffer_length--;
            window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            gui_optimize_input_buffer_size (window->buffer);
            gui_draw_buffer_input (window->buffer, 0);
            window->buffer->completion.position = -1;
        }
    }
}

/*
 * gui_action_delete_previous_word: delete previous word 
 */

void
gui_action_delete_previous_word (t_gui_window *window)
{
    int length_deleted, size_deleted;
    char *start, *string;
    
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos > 0)
        {
            start = utf8_add_offset (window->buffer->input_buffer,
                                      window->buffer->input_buffer_pos - 1);
            string = start;
            while (string && (string[0] == ' '))
            {
                string = utf8_prev_char (window->buffer->input_buffer, string);
            }
            if (string)
            {
                while (string && (string[0] != ' '))
                {
                    string = utf8_prev_char (window->buffer->input_buffer, string);
                }
                if (string)
                {
                    while (string && (string[0] == ' '))
                    {
                        string = utf8_prev_char (window->buffer->input_buffer, string);
                    }
                }
            }
            
            if (string)
                string = utf8_next_char (utf8_next_char (string));
            else
                string = window->buffer->input_buffer;
            
            size_deleted = utf8_next_char (start) - string;
            length_deleted = utf8_strnlen (string, size_deleted);
            
            gui_action_clipboard_copy (string, size_deleted);
            
            memmove (string, string + size_deleted, strlen (string + size_deleted));
            
            window->buffer->input_buffer_size -= size_deleted;
            window->buffer->input_buffer_length -= length_deleted;
            window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            window->buffer->input_buffer_pos -= length_deleted;
            gui_optimize_input_buffer_size (window->buffer);
            gui_draw_buffer_input (window->buffer, 0);
            window->buffer->completion.position = -1;
        }
    }
}

/*
 * gui_action_delete_next_word: delete next word 
 */

void
gui_action_delete_next_word (t_gui_window *window)
{
    int size_deleted, length_deleted;
    char *start, *string;
    
    if (window->buffer->has_input)
    {
        start = utf8_add_offset (window->buffer->input_buffer,
                                 window->buffer->input_buffer_pos);
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
        
        memmove (start, string, strlen (string));
        
        window->buffer->input_buffer_size -= size_deleted;
        window->buffer->input_buffer_length -= length_deleted;
        window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
        gui_optimize_input_buffer_size (window->buffer);
        gui_draw_buffer_input (window->buffer, 0);
        window->buffer->completion.position = -1;
    }
}

/*
 * gui_action_delete_begin_of_line: delete all from cursor pos to beginning of line
 */

void
gui_action_delete_begin_of_line (t_gui_window *window)
{
    int length_deleted, size_deleted;
    char *start;
    
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos > 0)
        {
            start = utf8_add_offset (window->buffer->input_buffer,
                                     window->buffer->input_buffer_pos);
            size_deleted = start - window->buffer->input_buffer;
            length_deleted = utf8_strnlen (window->buffer->input_buffer, size_deleted);
            gui_action_clipboard_copy (window->buffer->input_buffer,
                                       start - window->buffer->input_buffer);
            
            memmove (window->buffer->input_buffer, start, strlen (start));
            
            window->buffer->input_buffer_size -= size_deleted;
            window->buffer->input_buffer_length -= length_deleted;
            window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            window->buffer->input_buffer_pos = 0;
            gui_optimize_input_buffer_size (window->buffer);
            gui_draw_buffer_input (window->buffer, 0);
            window->buffer->completion.position = -1;
        }
    }
}

/*
 * gui_action_delete_end_of_line: delete all from cursor pos to end of line
 */

void
gui_action_delete_end_of_line (t_gui_window *window)
{
    char *start;
    int size_deleted, length_deleted;
    
    if (window->buffer->has_input)
    {
        start = utf8_add_offset (window->buffer->input_buffer,
                                 window->buffer->input_buffer_pos);
        size_deleted = strlen (start);
        length_deleted = utf8_strlen (start);
        gui_action_clipboard_copy (start, size_deleted);
        start[0] = '\0';
        window->buffer->input_buffer_size = strlen (window->buffer->input_buffer);
        window->buffer->input_buffer_length = utf8_strlen (window->buffer->input_buffer);
        gui_optimize_input_buffer_size (window->buffer);
        gui_draw_buffer_input (window->buffer, 0);
        window->buffer->completion.position = -1;
    }
}

/*
 * gui_action_delete_line: delete entire line
 */

void
gui_action_delete_line (t_gui_window *window)
{
    if (window->buffer->has_input)
    {
        window->buffer->input_buffer[0] = '\0';
        window->buffer->input_buffer_size = 0;
        window->buffer->input_buffer_length = 0;
        window->buffer->input_buffer_pos = 0;
        gui_optimize_input_buffer_size (window->buffer);
        gui_draw_buffer_input (window->buffer, 0);
        window->buffer->completion.position = -1;
    }
}

/*
 * gui_action_transpose_chars: transpose chars (on lth left) at cursor pos
 */

void
gui_action_transpose_chars (t_gui_window *window)
{
    char *start, *prev_char, saved_char[4];
    int size_current_char;
    
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos > 0)
        {
            if (window->buffer->input_buffer_pos == window->buffer->input_buffer_length)
                window->buffer->input_buffer_pos--;
            
            start = utf8_add_offset (window->buffer->input_buffer,
                                     window->buffer->input_buffer_pos);
            prev_char = utf8_prev_char (window->buffer->input_buffer,
                                        start);
            size_current_char = start - prev_char;
            memcpy (saved_char, prev_char, size_current_char);
            memcpy (prev_char, start, utf8_char_size (start));
            start = utf8_next_char (prev_char);
            memcpy (start, saved_char, size_current_char);
            
            window->buffer->input_buffer_pos++;
            
            gui_draw_buffer_input (window->buffer, 0);	
            window->buffer->completion.position = -1;
        }
    }
}

/*
 * gui_action_home: home key
 */

void
gui_action_home (t_gui_window *window)
{
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos > 0)
        {
            window->buffer->input_buffer_pos = 0;
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_end: end key
 */

void
gui_action_end (t_gui_window *window)
{
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos <
            window->buffer->input_buffer_length)
        {
            window->buffer->input_buffer_pos =
                window->buffer->input_buffer_length;
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_left: move to previous char
 */

void
gui_action_left (t_gui_window *window)
{
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos > 0)
        {
            window->buffer->input_buffer_pos--;
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_previous_word: move to beginning of previous word
 */

void
gui_action_previous_word (t_gui_window *window)
{
    char *pos;
    
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos > 0)
        {
            pos = utf8_add_offset (window->buffer->input_buffer,
                                   window->buffer->input_buffer_pos - 1);
            while (pos && (pos[0] == ' '))
            {
                pos = utf8_prev_char (window->buffer->input_buffer, pos);
            }
            if (pos)
            {
                while (pos && (pos[0] != ' '))
                {
                    pos = utf8_prev_char (window->buffer->input_buffer, pos);
                }
                if (pos)
                    pos = utf8_next_char (pos);
                else
                    pos = window->buffer->input_buffer;
                window->buffer->input_buffer_pos = utf8_pos (window->buffer->input_buffer,
                                                             pos - window->buffer->input_buffer);
            }
            else
                window->buffer->input_buffer_pos = 0;
            
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_right: move to previous char
 */

void
gui_action_right (t_gui_window *window)
{
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos <
            window->buffer->input_buffer_length)
        {
            window->buffer->input_buffer_pos++;
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_next_word: move to the end of next
 */

void
gui_action_next_word (t_gui_window *window)
{
    char *pos;
    
    if (window->buffer->has_input)
    {
        if (window->buffer->input_buffer_pos <
            window->buffer->input_buffer_length)
        {
            pos = utf8_add_offset (window->buffer->input_buffer,
                                   window->buffer->input_buffer_pos);
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
                    window->buffer->input_buffer_pos =
                        utf8_pos (window->buffer->input_buffer,
                                  pos - window->buffer->input_buffer);
                else
                    window->buffer->input_buffer_pos = 
                        window->buffer->input_buffer_length;
            }
            else
                window->buffer->input_buffer_pos =
                    utf8_pos (window->buffer->input_buffer,
                              utf8_prev_char (window->buffer->input_buffer, pos) - window->buffer->input_buffer);
            
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_up: recall last command or move to previous DCC in list
 */

void
gui_action_up (t_gui_window *window)
{
    if (window->buffer->dcc)
    {
        if (dcc_list)
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
                gui_draw_buffer_chat (window->buffer, 1);
                gui_draw_buffer_input (window->buffer, 1);
            }
        }
    }
    else if (window->buffer->has_input)
    {
        if (window->buffer->ptr_history)
        {
            window->buffer->ptr_history =
                window->buffer->ptr_history->next_history;
            if (!window->buffer->ptr_history)
                window->buffer->ptr_history =
                    window->buffer->history;
        }
        else
            window->buffer->ptr_history =
                window->buffer->history;
        if (window->buffer->ptr_history)
        {
	    /* bash/readline like use of history */
            if (window->buffer->ptr_history->prev_history == NULL)
            {
                if (window->buffer->input_buffer_size > 0)
                {
                    window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
                    history_buffer_add (window->buffer, window->buffer->input_buffer);
                    history_global_add (window->buffer->input_buffer);
                }
            }
            else
            {
                if (window->buffer->input_buffer_size > 0)
                {
                    window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
                    if (window->buffer->ptr_history->prev_history->text)
			free(window->buffer->ptr_history->prev_history->text);
                    window->buffer->ptr_history->prev_history->text = strdup (window->buffer->input_buffer);
                }
            }
            window->buffer->input_buffer_size =
                strlen (window->buffer->ptr_history->text);
            window->buffer->input_buffer_length =
                utf8_strlen (window->buffer->ptr_history->text);
            gui_optimize_input_buffer_size (window->buffer);
            window->buffer->input_buffer_pos =
                window->buffer->input_buffer_length;
            window->buffer->input_buffer_1st_display = 0;
            strcpy (window->buffer->input_buffer,
                    window->buffer->ptr_history->text);
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_up_global: recall last command in global history
 */

void
gui_action_up_global (t_gui_window *window)
{
    if (window->buffer->has_input)
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
            window->buffer->input_buffer_size =
                strlen (history_global_ptr->text);
            window->buffer->input_buffer_length =
                utf8_strlen (history_global_ptr->text);
            gui_optimize_input_buffer_size (window->buffer);
            window->buffer->input_buffer_pos =
                window->buffer->input_buffer_length;
            window->buffer->input_buffer_1st_display = 0;
            strcpy (window->buffer->input_buffer,
                    history_global_ptr->text);
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_down: recall next command or move to next DCC in list
 */

void
gui_action_down (t_gui_window *window)
{
    if (window->buffer->dcc)
    {
        if (dcc_list)
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
                            dcc_list->next_dcc;
                }
                if (window->dcc_selected)
                    window->dcc_selected =
                        ((t_irc_dcc *)(window->dcc_selected))->next_dcc;
                else
                    window->dcc_selected =
                        dcc_list->next_dcc;
                gui_draw_buffer_chat (window->buffer, 1);
                gui_draw_buffer_input (window->buffer, 1);
            }
        }
    }
    else if (window->buffer->has_input)
    {
        if (window->buffer->ptr_history)
        {
            window->buffer->ptr_history =
                window->buffer->ptr_history->prev_history;
            if (window->buffer->ptr_history)
            {
                window->buffer->input_buffer_size =
                    strlen (window->buffer->ptr_history->text);
                window->buffer->input_buffer_length =
                    utf8_strlen (window->buffer->ptr_history->text);
            }
            else
            {
                window->buffer->input_buffer_size = 0;
                window->buffer->input_buffer_length = 0;
            }
            gui_optimize_input_buffer_size (window->buffer);
            window->buffer->input_buffer_pos =
                window->buffer->input_buffer_length;
            window->buffer->input_buffer_1st_display = 0;
            if (window->buffer->ptr_history)
                strcpy (window->buffer->input_buffer,
                        window->buffer->ptr_history->text);
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_down_global: recall next command in global history
 */

void
gui_action_down_global (t_gui_window *window)
{
    if (window->buffer->has_input)
    {
        if (history_global_ptr)
        {
            history_global_ptr = history_global_ptr->prev_history;
            if (history_global_ptr)
            {
                window->buffer->input_buffer_size =
                    strlen (history_global_ptr->text);
                window->buffer->input_buffer_length =
                    utf8_strlen (history_global_ptr->text);
            }
            else
            {
                window->buffer->input_buffer_size = 0;
                window->buffer->input_buffer_length = 0;
            }
            gui_optimize_input_buffer_size (window->buffer);
            window->buffer->input_buffer_pos =
                window->buffer->input_buffer_length;
            window->buffer->input_buffer_1st_display = 0;
            if (history_global_ptr)
                strcpy (window->buffer->input_buffer,
                        history_global_ptr->text);
            gui_draw_buffer_input (window->buffer, 0);
        }
    }
}

/*
 * gui_action_page_up: display previous page on buffer
 */

void
gui_action_page_up (t_gui_window *window)
{
    gui_window_page_up (window);
}

/*
 * gui_action_page_down: display next page on buffer
 */

void
gui_action_page_down (t_gui_window *window)
{
    gui_window_page_down (window);
}

/*
 * gui_action_nick_beginning: go to beginning of nicklist
 */

void
gui_action_nick_beginning (t_gui_window *window)
{
    gui_window_nick_beginning (window);
}

/*
 * gui_action_nick_end: go to the end of nicklist
 */

void
gui_action_nick_end (t_gui_window *window)
{
    gui_window_nick_end (window);
}

/*
 * gui_action_nick_page_up: scroll one page up in nicklist
 */

void
gui_action_nick_page_up (t_gui_window *window)
{
    gui_window_nick_page_up (window);
}

/*
 * gui_action_nick_page_down: scroll one page down in nicklist
 */

void
gui_action_nick_page_down (t_gui_window *window)
{
    gui_window_nick_page_down (window);
}

/*
 * gui_action_jump_smart: jump to buffer with activity (alt-A by default)
 */

void
gui_action_jump_smart (t_gui_window *window)
{
    if (hotlist)
    {
        if (!hotlist_initial_buffer)
            hotlist_initial_buffer = window->buffer;
        gui_switch_to_buffer (window, hotlist->buffer);
        gui_redraw_buffer (window->buffer);
    }
    else
    {
        if (hotlist_initial_buffer)
        {
            gui_switch_to_buffer (window, hotlist_initial_buffer);
            gui_redraw_buffer (window->buffer);
            hotlist_initial_buffer = NULL;
        }
    }
}

/*
 * gui_action_jump_dcc: jump to DCC buffer
 */

void
gui_action_jump_dcc (t_gui_window *window)
{
    if (window->buffer->dcc)
    {
        if (buffer_before_dcc)
        {
            gui_switch_to_buffer (window,
                                  buffer_before_dcc);
            gui_redraw_buffer (window->buffer);
        }
    }
    else
    {
        buffer_before_dcc = window->buffer;
        gui_buffer_switch_dcc (window);
    }
}

/*
 * gui_action_jump_last_buffer: jump to last buffer
 */

void
gui_action_jump_last_buffer (t_gui_window *window)
{
    if (last_gui_buffer)
        gui_buffer_switch_by_number (window, last_gui_buffer->number);
}

/*
 * gui_action_jump_server: jump to server buffer
 */

void
gui_action_jump_server (t_gui_window *window)
{
    if (SERVER(window->buffer))
    {
        if (SERVER(window->buffer)->buffer !=
            window->buffer)
        {
            gui_switch_to_buffer (window,
                                  SERVER(window->buffer)->buffer);
            gui_redraw_buffer (window->buffer);
        }
    }
}

/*
 * gui_action_jump_next_server: jump to next server
 */

void
gui_action_jump_next_server (t_gui_window *window)
{
    t_irc_server *ptr_server;
    t_gui_buffer *ptr_buffer;
    
    if (SERVER(window->buffer))
    {
        ptr_server = SERVER(window->buffer)->next_server;
        if (!ptr_server)
            ptr_server = irc_servers;
        while (ptr_server != SERVER(window->buffer))
        {
            if (ptr_server->buffer)
                break;
            ptr_server = (ptr_server->next_server) ?
                ptr_server->next_server : irc_servers;
        }
        if (ptr_server != SERVER(window->buffer))
        {
            /* save current buffer */
            SERVER(window->buffer)->saved_buffer = window->buffer;
            
            /* come back to memorized chan if found */
            if (ptr_server->saved_buffer)
                ptr_buffer = ptr_server->saved_buffer;
            else
                ptr_buffer = (ptr_server->channels) ?
                    ptr_server->channels->buffer : ptr_server->buffer;
            if ((ptr_server->buffer == ptr_buffer)
                && (ptr_buffer->all_servers))
                ptr_buffer->server = ptr_server;
            gui_switch_to_buffer (window, ptr_buffer);
            gui_redraw_buffer (window->buffer);
        }
    }
}

/*
 * gui_action_switch_server: switch server on servers buffer
 *                          (if same buffer is used for all buffers)
 */

void
gui_action_switch_server (t_gui_window *window)
{
    gui_window_switch_server (window);
}

/*
 * gui_action_scroll_previous_highlight: scroll to previous highlight
 */

void
gui_action_scroll_previous_highlight (t_gui_window *window)
{
    t_gui_line *ptr_line;
    
    if (!window->buffer->dcc)
    {
        if (window->buffer->lines)
        {
            ptr_line = (window->start_line) ?
                window->start_line->prev_line : window->buffer->last_line->prev_line;
            while (ptr_line)
            {
                if (ptr_line->line_with_highlight)
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == window->buffer->lines);
                    gui_draw_buffer_chat (window->buffer, 1);
                    gui_draw_buffer_status (window->buffer, 0);
                    return;
                }
                ptr_line = ptr_line->prev_line;
            }
        }
    }
}

/*
 * gui_action_scroll_next_highlight: scroll to next highlight
 */

void
gui_action_scroll_next_highlight (t_gui_window *window)
{
    t_gui_line *ptr_line;
    
    if (!window->buffer->dcc)
    {
        if (window->buffer->lines)
        {
            ptr_line = (window->start_line) ?
                window->start_line->next_line : window->buffer->lines->next_line;
            while (ptr_line)
            {
                if (ptr_line->line_with_highlight)
                {
                    window->start_line = ptr_line;
                    window->start_line_pos = 0;
                    window->first_line_displayed =
                        (window->start_line == window->buffer->lines);
                    gui_draw_buffer_chat (window->buffer, 1);
                    gui_draw_buffer_status (window->buffer, 0);
                    return;
                }
                ptr_line = ptr_line->next_line;
            }
        }
    }
}

/*
 * gui_action_scroll_unread: scroll to first unread line of buffer
 */

void
gui_action_scroll_unread (t_gui_window *window)
{
    if (cfg_look_read_marker &&
        cfg_look_read_marker[0] &&
        !window->buffer->dcc &&
        window->buffer->last_read_line &&
        window->buffer->last_read_line != window->buffer->last_line)
    {
        window->start_line = window->buffer->last_read_line->next_line;
        window->start_line_pos = 0;
        window->first_line_displayed =
            (window->start_line == window->buffer->lines);
        gui_draw_buffer_chat (window->buffer, 1);
        gui_draw_buffer_status (window->buffer, 0);
    }
}

/*
 * gui_action_hotlist_clear: clear hotlist
 */

void
gui_action_hotlist_clear (t_gui_window *window)
{
    if (hotlist)
    {
        hotlist_free_all ();
        gui_redraw_buffer (window->buffer);
    }
    hotlist_initial_buffer = window->buffer;
}

/*
 * gui_action_infobar_clear: clear infobar
 */

void
gui_action_infobar_clear (t_gui_window *window)
{
    gui_infobar_remove ();
    gui_draw_buffer_infobar (window->buffer, 1);
}

/*
 * gui_action_refresh: refresh screen
 */

void
gui_action_refresh_screen (t_gui_window *window)
{
    /* make gcc happy */
    (void) window;
    
    gui_refresh_screen ();
}

/*
 * gui_action_grab_key: init "grab key mode" (next key will be inserted into input buffer)
 */

void
gui_action_grab_key (t_gui_window *window)
{
    if (window->buffer->has_input)
        gui_key_init_grab ();
}
