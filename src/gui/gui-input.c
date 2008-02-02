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

/* gui-input.c: input functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-input.h"
#include "gui-buffer.h"
#include "gui-completion.h"
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
            buffer->input_buffer = realloc (buffer->input_buffer, optimal_size * sizeof (char));
            buffer->input_buffer_color_mask = realloc (buffer->input_buffer_color_mask,
                                                       optimal_size * sizeof (char));
        }
    }
}

/*
 * gui_input_init_color_mask: initialize color mask for input buffer
 */

void
gui_input_init_color_mask (struct t_gui_buffer *buffer)
{
    int i;
    
    if (buffer->input)
    {
        for (i = 0; i < buffer->input_buffer_size; i++)
            buffer->input_buffer_color_mask[i] = ' ';
        buffer->input_buffer_color_mask[buffer->input_buffer_size] = '\0';
    }
}

/*
 * gui_input_move: move data in input buffer
 */

void
gui_input_move (struct t_gui_buffer *buffer, char *target, char *source,
                int size)
{
    int pos_source, pos_target;
    
    pos_target = target - buffer->input_buffer;
    pos_source = source - buffer->input_buffer;
    
    memmove (target, source, size);
    memmove (buffer->input_buffer_color_mask + pos_target,
             buffer->input_buffer_color_mask + pos_source, size);
}

/*
 * gui_input_insert_string: insert a string into the input buffer
 *                          if pos == -1, string is inserted at cursor position
 *                          return: number of chars inserted
 *                          (may be different of strlen if UTF-8 string)
 */

int
gui_input_insert_string (struct t_gui_buffer *buffer, char *string, int pos)
{
    int i, pos_start, size, length;
    char *ptr_start;
    char *buffer_before_insert, *string2;
    
    if (buffer->input)
    {
        buffer_before_insert =
            (buffer->input_buffer) ?
            strdup (buffer->input_buffer) : strdup ("");
        
        if (pos == -1)
            pos = buffer->input_buffer_pos;
        
        size = strlen (string);
        length = utf8_strlen (string);
        
        /* increase buffer size */
        buffer->input_buffer_size += size;
        buffer->input_buffer_length += length;
        gui_input_optimize_size (buffer);
        buffer->input_buffer[buffer->input_buffer_size] = '\0';
        buffer->input_buffer_color_mask[buffer->input_buffer_size] = '\0';
        
        /* move end of string to the right */
        ptr_start = utf8_add_offset (buffer->input_buffer, pos);
        pos_start = ptr_start - buffer->input_buffer;
        memmove (ptr_start + size, ptr_start, strlen (ptr_start));
        memmove (buffer->input_buffer_color_mask + pos_start + size,
                 buffer->input_buffer_color_mask + pos_start,
                 strlen (buffer->input_buffer_color_mask + pos_start));
        
        /* insert new string */
        ptr_start = utf8_add_offset (buffer->input_buffer, pos);
        pos_start = ptr_start - buffer->input_buffer;
        strncpy (ptr_start, string, size);
        for (i = 0; i < size; i++)
        {
            buffer->input_buffer_color_mask[pos_start + i] = ' ';
        }
        
        buffer->input_buffer_pos += length;
        
        string2 = (char *)malloc ((size + 2) * sizeof (char));
        if (string2)
        {
            snprintf (string2, size + 2, "*%s", string);
            /* TODO: execute keyboard hooks */
            /*(void) plugin_keyboard_handler_exec (string2,
                                                 buffer_before_insert,
                                                 buffer->input_buffer);*/
            free (string2);
        }
        if (buffer_before_insert)
            free (buffer_before_insert);
        
        return length;
    }
    return 0;
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
            buffer->input_buffer_color_mask[buffer->input_buffer_size] = '\0';
            for (i = buffer->input_buffer_size - 1;
                 i >=  buffer->completion->position_replace +
                     (int)strlen (buffer->completion->word_found); i--)
            {
                buffer->input_buffer[i] =
                    buffer->input_buffer[i - buffer->completion->diff_size];
                buffer->input_buffer_color_mask[i] =
                    buffer->input_buffer_color_mask[i - buffer->completion->diff_size];
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
                buffer->input_buffer_color_mask[i] =
                    buffer->input_buffer_color_mask[i - buffer->completion->diff_size];
            }
            buffer->input_buffer_size += buffer->completion->diff_size;
            buffer->input_buffer_length += buffer->completion->diff_length;
            gui_input_optimize_size (buffer);
            buffer->input_buffer[buffer->input_buffer_size] = '\0';
            buffer->input_buffer_color_mask[buffer->input_buffer_size] = '\0';
        }
            
        strncpy (buffer->input_buffer + buffer->completion->position_replace,
                 buffer->completion->word_found,
                 strlen (buffer->completion->word_found));
        for (i = 0; i < (int)strlen (buffer->completion->word_found); i++)
        {
            buffer->input_buffer_color_mask[buffer->completion->position_replace + i] = ' ';
        }
        buffer->input_buffer_pos =
            utf8_pos (buffer->input_buffer,
                      buffer->completion->position_replace) +
            utf8_strlen (buffer->completion->word_found);
        
        /* position is < 0 this means only one word was found to complete,
           so reinit to stop completion */
        if (buffer->completion->position >= 0)
            buffer->completion->position = utf8_real_pos (buffer->input_buffer,
                                                      buffer->input_buffer_pos);
        
        /* add nick completor if position 0 and completing nick */
        if ((buffer->completion->base_word_pos == 0)
            && (buffer->completion->context == GUI_COMPLETION_NICK))
        {
            if (strncmp (utf8_add_offset (buffer->input_buffer,
                                          buffer->input_buffer_pos),
                         CONFIG_STRING(config_look_nick_completor),
                         strlen (CONFIG_STRING(config_look_nick_completor))) != 0)
                gui_input_insert_string (buffer,
                                         CONFIG_STRING(config_look_nick_completor),
                                         buffer->input_buffer_pos);
            else
                buffer->input_buffer_pos += utf8_strlen (CONFIG_STRING(config_look_nick_completor));
            if (buffer->completion->position >= 0)
                buffer->completion->position += strlen (CONFIG_STRING(config_look_nick_completor));
            if (buffer->input_buffer[utf8_real_pos (buffer->input_buffer,
                                                    buffer->input_buffer_pos)] != ' ')
                gui_input_insert_string (buffer, " ",
                                         buffer->input_buffer_pos);
            else
                buffer->input_buffer_pos++;
            if (buffer->completion->position >= 0)
                buffer->completion->position++;
        }
        else
        {
            /* add space or completor to the end of completion, if needed */
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
        }
        gui_input_draw (buffer, 0);
    }
}

/*
 * gui_input_delete_line: delete entire input line
 */

void
gui_input_delete_line (struct t_gui_buffer *buffer)
{
    if (gui_current_window->buffer->input)
    {
        buffer->input_buffer[0] = '\0';
        buffer->input_buffer_color_mask[0] = '\0';
        buffer->input_buffer_size = 0;
        buffer->input_buffer_length = 0;
        buffer->input_buffer_pos = 0;
        gui_input_optimize_size (buffer);
        buffer->completion->position = -1;
    }
}

/*
 * gui_input_get_prompt_length: return input prompt length (displayed on screen)
 */

int
gui_input_get_prompt_length (struct t_gui_buffer *buffer)
{
    char *pos, saved_char;
    int char_size, length;
    
    if (buffer->text_search != GUI_TEXT_SEARCH_DISABLED)
    {
        if (buffer->text_search_exact)
            return utf8_strlen_screen (_("Text search (exact): "));
        else
            return utf8_strlen_screen (_("Text search: "));
    }
    
    length = 0;
    pos = CONFIG_STRING(config_look_input_format);
    while (pos && pos[0])
    {
        switch (pos[0])
        {
            case '%':
                pos++;
                switch (pos[0])
                {
                    case 'c': /* buffer name */
                        length += utf8_strlen_screen (buffer->name);
                        pos++;
                        break;
                    case 'm': // nick modes
                        /*if (GUI_SERVER(buffer) && GUI_SERVER(buffer)->is_connected)
                        {
                            if (GUI_SERVER(buffer)->nick_modes
                                && GUI_SERVER(buffer)->nick_modes[0])
                                length += strlen (GUI_SERVER(buffer)->nick_modes);
                        }*/
                        pos++;
                        break;
                    case 'n': /* nick */
                        if (buffer->input_nick)
                            length += utf8_strlen_screen (buffer->input_nick);
                        pos++;
                        break;
                    default:
                        length++;
                        if (pos[0])
                        {
                            if (pos[0] == '%')
                                pos++;
                            else
                            {
                                length++;
                                pos += utf8_char_size (pos);
                            }
                        }
                        break;
                }
                break;
            default:
                char_size = utf8_char_size (pos);
                saved_char = pos[char_size];
                pos[char_size] = '\0';
                length += utf8_strlen_screen (pos);
                pos[char_size] = saved_char;
                pos += char_size;
                break;
        }
    }
    return length;
}

/*
 * gui_exec_action_dcc: execute an action on a DCC after a user input
 *                      return -1 if DCC buffer was closed due to action,
 *                      0 otherwise
 */

void
gui_exec_action_dcc (struct t_gui_window *window, char *actions)
{
    (void) window;
    (void) actions;
    /*t_irc_dcc *dcc_selected, *ptr_dcc, *ptr_dcc_next;
    struct t_gui_window *ptr_win;
    struct t_gui_buffer *ptr_buffer;
    
    while (actions[0])
    {
        if (actions[0] >= 32)
        {
            dcc_selected = (window->dcc_selected) ?
                (t_irc_dcc *) window->dcc_selected : irc_dcc_list;
            
            switch (actions[0])
            {
                // accept DCC
                case 'a':
                case 'A':
                    if (dcc_selected
                        && (IRC_DCC_IS_RECV(dcc_selected->status))
                        && (dcc_selected->status == IRC_DCC_WAITING))
                    {
                        irc_dcc_accept (dcc_selected);
                    }
                    break;
                // cancel DCC
                case 'c':
                case 'C':
                    if (dcc_selected
                        && (!IRC_DCC_ENDED(dcc_selected->status)))
                    {
                        irc_dcc_close (dcc_selected, IRC_DCC_ABORTED);
                        gui_window_redraw_buffer (window->buffer);
                    }
                    break;
                // purge old DCC
                case 'p':
                case 'P':
                    window->dcc_first = NULL;
                    window->dcc_selected = NULL;
                    window->dcc_last_displayed = NULL;
                    ptr_dcc = irc_dcc_list;
                    while (ptr_dcc)
                    {
                        ptr_dcc_next = ptr_dcc->next_dcc;
                        if (IRC_DCC_ENDED(ptr_dcc->status))
                            irc_dcc_free (ptr_dcc);
                        ptr_dcc = ptr_dcc_next;
                    }
                    gui_window_redraw_buffer (window->buffer);
                    break;
                // close DCC window
                case 'q':
                case 'Q':
                    if (gui_buffer_before_dcc)
                    {
                        ptr_buffer = window->buffer;
                        for (ptr_win = gui_windows; ptr_win;
                             ptr_win = ptr_win->next_window)
                        {
                            if (ptr_win->buffer == ptr_buffer)
                                gui_window_switch_to_buffer (ptr_win,
                                                             gui_buffer_before_dcc);
                        }
                        gui_buffer_free (ptr_buffer, 0);
                    }
                    else
                        gui_buffer_free (window->buffer, 1);
                    gui_window_redraw_buffer (window->buffer);
                    return;
                    break;
                // remove from DCC list
                case 'r':
                case 'R':
                    if (dcc_selected
                        && (IRC_DCC_ENDED(dcc_selected->status)))
                    {
                        if (dcc_selected->next_dcc)
                            window->dcc_selected = dcc_selected->next_dcc;
                        else
                            window->dcc_selected = NULL;
                        irc_dcc_free (dcc_selected);
                        gui_window_redraw_buffer (window->buffer);
                    }
                    break;
            }
        }
        actions = utf8_next_char (actions);
        }*/
}

/*
 * gui_exec_action_raw_data: execute an action on raw IRC data
 *                           return -1 if raw IRC data was closed due to action,
 *                           0 otherwise
 */

void
gui_exec_action_raw_data (struct t_gui_window *window, char *actions)
{
    struct t_gui_window *ptr_win;
    struct t_gui_buffer *ptr_buffer;
    
    while (actions[0])
    {
        if (actions[0] >= 32)
        {
            switch (actions[0])
            {
                /* close raw IRC data */
                case 'c':
                case 'C':
                    gui_buffer_clear (window->buffer);
                    gui_window_redraw_buffer (window->buffer);
                    return;
                    break;
                /* close raw IRC data */
                case 'q':
                case 'Q':
                    if (gui_buffer_before_raw_data)
                    {
                        ptr_buffer = window->buffer;
                        for (ptr_win = gui_windows; ptr_win;
                             ptr_win = ptr_win->next_window)
                        {
                            if (ptr_win->buffer == ptr_buffer)
                                gui_window_switch_to_buffer (ptr_win,
                                                             gui_buffer_before_raw_data);
                        }
                        gui_buffer_close (ptr_buffer, 0);
                    }
                    else
                        gui_buffer_close (window->buffer, 1);
                    gui_window_redraw_buffer (window->buffer);
                    return;
                    break;
            }
        }
        actions = utf8_next_char (actions);
    }
}
