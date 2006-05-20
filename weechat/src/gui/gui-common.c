/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
#include <ctype.h>

#include "../common/weechat.h"
#include "gui.h"
#include "../common/command.h"
#include "../common/weeconfig.h"
#include "../common/history.h"
#include "../common/hotlist.h"
#include "../common/log.h"
#include "../common/utf8.h"
#include "../irc/irc.h"


int gui_init_ok = 0;                        /* = 1 if GUI is initialized    */
int gui_ok = 0;                             /* = 1 if GUI is ok             */
                                            /* (0 when term size too small) */
int gui_add_hotlist = 1;                    /* 0 is for temporarly disable  */
                                            /* hotlist add for all buffers  */

t_gui_infobar *gui_infobar;                 /* pointer to infobar content   */

char *gui_input_clipboard = NULL;           /* clipboard content            */

time_t gui_last_activity_time = 0;          /* last activity time           */
                                            /* (key pressed)                */


/*
 * gui_word_strlen: returns number of char needed on sreen to display a word
 *                  special chars like color, bold, .. are ignored
 */

int
gui_word_strlen (t_gui_window *window, char *string)
{
    int length, width_screen;
    
    length = 0;
    while (string && string[0])
    {
        string = gui_chat_word_get_next_char (window, (unsigned char *)string, 0, &width_screen);
        if (string)
            length += width_screen;
    }
    return length;
}

/*
 * gui_word_real_pos: get real position in string (ignoring color/bold/.. chars)
 */

int
gui_word_real_pos (t_gui_window *window, char *string, int pos)
{
    char *saved_pos;
    int real_pos, width_screen;
    
    if (pos <= 0)
        return 0;
    
    real_pos = 0;
    while (string && string[0] && (pos > 0))
    {
        saved_pos = string;
        string = gui_chat_word_get_next_char (window, (unsigned char *)string, 0, &width_screen);
        pos -= width_screen;
        if (string)
            real_pos += (string - saved_pos);
    }
    return real_pos;
}

/*
 * gui_add_to_line: add a message to last line of buffer
 */

void
gui_add_to_line (t_gui_buffer *buffer, int type, char *nick, char *message)
{
    char *pos;
    int length;
    
    /* create new line if previous was ending by '\n' (or if 1st line) */
    if (buffer->line_complete)
    {
        buffer->line_complete = 0;
        if (!gui_buffer_line_new (buffer))
            return;
    }
    
    pos = strchr (message, '\n');
    if (pos)
    {
        pos[0] = '\0';
        buffer->line_complete = 1;
    }
    
    if (nick && (!buffer->last_line->nick))
        buffer->last_line->nick = strdup (nick);
    
    if (buffer->last_line->data)
    {
        length = strlen (buffer->last_line->data);
        buffer->last_line->data = (char *) realloc (buffer->last_line->data,
                                                    length + strlen (message) + 1);
        if (((type & MSG_TYPE_TIME) == 0)
             && (buffer->last_line->ofs_after_date < 0))
            buffer->last_line->ofs_after_date = length;
        if (((type & (MSG_TYPE_TIME | MSG_TYPE_NICK)) == 0)
             && (buffer->last_line->ofs_start_message < 0))
            buffer->last_line->ofs_start_message = length;
        strcat (buffer->last_line->data, message);
    }
    else
    {
        if (((type & MSG_TYPE_TIME) == 0)
            && (buffer->last_line->ofs_after_date < 0))
            buffer->last_line->ofs_after_date = 0;
        if (((type & (MSG_TYPE_TIME | MSG_TYPE_NICK)) == 0)
            && (buffer->last_line->ofs_start_message < 0))
            buffer->last_line->ofs_start_message = 0;
        buffer->last_line->data = strdup (message);
    }
    
    length = gui_word_strlen (NULL, message);
    buffer->last_line->length += length;
    if (type & MSG_TYPE_MSG)
        buffer->last_line->line_with_message = 1;
    if (type & MSG_TYPE_HIGHLIGHT)
        buffer->last_line->line_with_highlight = 1;
    if ((type & MSG_TYPE_TIME) || (type & MSG_TYPE_NICK) || (type & MSG_TYPE_PREFIX))
        buffer->last_line->length_align += length;
    if (type & MSG_TYPE_NOLOG)
        buffer->last_line->log_write = 0;
    if (pos)
    {
        pos[0] = '\n';
        if (buffer->num_displayed > 0)
        {
            gui_chat_draw_line (buffer, buffer->last_line);
            gui_chat_draw (buffer, 0);
        }
        if (gui_add_hotlist && (buffer->num_displayed == 0))
        {
            if (3 - buffer->last_line->line_with_message -
                buffer->last_line->line_with_highlight <=
                buffer->notify_level)
            {
                if (buffer->last_line->line_with_highlight)
                    hotlist_add (HOTLIST_HIGHLIGHT, SERVER(buffer), buffer);
                else if (BUFFER_IS_PRIVATE(buffer) && (buffer->last_line->line_with_message))
                    hotlist_add (HOTLIST_PRIVATE, SERVER(buffer), buffer);
                else if (buffer->last_line->line_with_message)
                    hotlist_add (HOTLIST_MSG, SERVER(buffer), buffer);
                else
                    hotlist_add (HOTLIST_LOW, SERVER(buffer), buffer);
                gui_status_draw (gui_current_window->buffer, 1);
            }
        }
    }
    if (buffer->line_complete && buffer->log_file && buffer->last_line->log_write)
    {
        gui_log_write_date (buffer);
        if (buffer->last_line->nick)
        {
            gui_log_write (buffer, "<");
            gui_log_write (buffer, buffer->last_line->nick);
            gui_log_write (buffer, "> ");
        }
        if (buffer->last_line->ofs_start_message >= 0)
            gui_log_write_line (buffer,
                                buffer->last_line->data + buffer->last_line->ofs_start_message);
        else
            gui_log_write_line (buffer, buffer->last_line->data);
    }
}

/*
 * gui_printf_internal: display a message in a buffer
 *                      This function should NEVER be called directly.
 *                      You should use macros defined in gui.h
 */

void
gui_printf_internal (t_gui_buffer *buffer, int display_time, int type, char *nick, char *message, ...)
{
    static char buf[8192];
    char text_time[1024];
    char text_time_char[2];
    time_t time_seconds;
    struct tm *local_time;
    int time_first_digit, time_last_digit;
    char *buf2, *pos;
    int i;
    va_list argptr;
    static time_t seconds;
    struct tm *date_tmp;
    
    if (gui_init_ok)
    {
        if (buffer == NULL)
        {
            type |= MSG_TYPE_NOLOG;
            if (SERVER(gui_current_window->buffer))
                buffer = SERVER(gui_current_window->buffer)->buffer;
            else
                buffer = gui_current_window->buffer;
            
            if (!buffer || (buffer->type != BUFFER_TYPE_STANDARD))
                buffer = gui_buffers;
        }
    
        if (buffer == NULL)
        {
            weechat_log_printf ("WARNING: gui_printf_internal without buffer! This is a bug, "
                                "please send to developers - thanks\n");
            return;
        }
        
        if (buffer->type == BUFFER_TYPE_DCC)
            buffer = gui_buffers;
        
        if (buffer->type == BUFFER_TYPE_DCC)
            return;
    }
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    
    if (!buf[0])
        return;
    
    if (gui_init_ok)
        buf2 = channel_iconv_decode (SERVER(buffer), CHANNEL(buffer), buf);
    else
        buf2 = strdup (buf);
    
    if (gui_init_ok)
    {
        seconds = time (NULL);
        date_tmp = localtime (&seconds);
        
        pos = buf2;
        while (pos)
        {
            if ((!buffer->last_line) || (buffer->line_complete))
            {
                if (display_time && cfg_look_buffer_timestamp &&
                    cfg_look_buffer_timestamp[0])
                {
                    time_seconds = time (NULL);
                    local_time = localtime (&time_seconds);
                    strftime (text_time, sizeof (text_time), cfg_look_buffer_timestamp, local_time);
                    
                    time_first_digit = -1;
                    time_last_digit = -1;
                    i = 0;
                    while (text_time[i])
                    {
                        if (isdigit (text_time[i]))
                        {
                            if (time_first_digit == -1)
                                time_first_digit = i;
                            time_last_digit = i;
                        }
                        i++;
                    }
                    
                    text_time_char[1] = '\0';
                    i = 0;
                    while (text_time[i])
                    {
                        text_time_char[0] = text_time[i];
                        if (time_first_digit < 0)
                        {
                            gui_add_to_line (buffer, MSG_TYPE_TIME, NULL,
                                             GUI_COLOR(COLOR_WIN_CHAT_TIME));
                            gui_add_to_line (buffer, MSG_TYPE_TIME, NULL, text_time_char);
                        }
                        else
                        {
                            if ((i < time_first_digit) || (i > time_last_digit))
                            {
                                gui_add_to_line (buffer, MSG_TYPE_TIME, NULL,
                                                 GUI_COLOR(COLOR_WIN_CHAT_DARK));
                                gui_add_to_line (buffer, MSG_TYPE_TIME, NULL, text_time_char);
                            }
                            else
                            {
                                if (isdigit (text_time[i]))
                                {
                                    gui_add_to_line (buffer, MSG_TYPE_TIME, NULL,
                                                     GUI_COLOR(COLOR_WIN_CHAT_TIME));
                                    gui_add_to_line (buffer, MSG_TYPE_TIME, NULL, text_time_char);
                                }
                                else
                                {
                                    gui_add_to_line (buffer, MSG_TYPE_TIME, NULL,
                                                     GUI_COLOR(COLOR_WIN_CHAT_TIME_SEP));
                                    gui_add_to_line (buffer, MSG_TYPE_TIME, NULL, text_time_char);
                                }
                            }
                        }
                        i++;
                    }
                    gui_add_to_line (buffer, MSG_TYPE_TIME, NULL, GUI_COLOR(COLOR_WIN_CHAT));
                }
                gui_add_to_line (buffer, MSG_TYPE_TIME, NULL, " ");
            }
            gui_add_to_line (buffer, type, nick, pos);
            pos = strchr (pos, '\n');
            if (pos)
            {
                if (!pos[1])
                    pos = NULL;
                else
                    pos++;
            }
        }
    }
    else
        printf ("%s", buf2);
    
    free (buf2);
}

/*
 * gui_printf_raw_data: display raw IRC data (only if raw IRC data buffer exists)
 */

void
gui_printf_raw_data (void *server, int send, char *message)
{
    char *pos;
    
    if (gui_buffer_raw_data)
    {
        while (message && message[0])
        {
            pos = strstr (message, "\r\n");
            if (pos)
                pos[0] = '\0';
            gui_printf_nolog (gui_buffer_raw_data,
                              "%s[%s%s%s] %s%s%s %s\n",
                              GUI_COLOR(COLOR_WIN_CHAT_DARK),
                              GUI_COLOR(COLOR_WIN_CHAT_SERVER),
                              ((t_irc_server *)server)->name,
                              GUI_COLOR(COLOR_WIN_CHAT_DARK),
                              GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                              (send) ? "<<--" : "-->>",
                              GUI_COLOR(COLOR_WIN_CHAT),
                              message);
            if (pos)
            {
                pos[0] = '\r';
                message = pos + 2;
            }
            else
                message = NULL;
        }
    }
}

/* 
 * gui_infobar_printf: display message in infobar
 */

void
gui_infobar_printf (int time_displayed, int color, char *message, ...)
{
    static char buf[1024];
    va_list argptr;
    t_gui_infobar *ptr_infobar;
    char *pos;
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    
    ptr_infobar = (t_gui_infobar *)malloc (sizeof (t_gui_infobar));
    if (ptr_infobar)
    {
        ptr_infobar->color = color;
        ptr_infobar->text = strdup (buf);
        pos = strchr (ptr_infobar->text, '\n');
        if (pos)
            pos[0] = '\0';
        ptr_infobar->remaining_time = (time_displayed <= 0) ? -1 : time_displayed;
        ptr_infobar->next_infobar = gui_infobar;
        gui_infobar = ptr_infobar;
        gui_infobar_draw (gui_current_window->buffer, 1);
    }
    else
        weechat_log_printf (_("Not enough memory for infobar message\n"));
}

/*
 * gui_infobar_printf_from_buffer: remove color, convert charset, then
 *                                 display message in infobar
 */

void
gui_infobar_printf_from_buffer (t_gui_buffer *buffer, int time_displayed,
                                int color, char *message1, char *message, ...)
{
    static char buf[1024];
    va_list argptr;
    char *buf2, *buf3;
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    
    buf2 = (char *)gui_color_decode ((unsigned char *)buf, 0);
    
    if (buf2)
        buf3 = channel_iconv_decode (SERVER(buffer), CHANNEL(buffer), buf2);
    else
        buf3 = NULL;
    
    gui_infobar_printf (time_displayed, color,
                        "%s%s", message1,
                        (buf3) ? buf3 : ((buf2) ? buf2 : buf));
    
    if (buf2)
        free (buf2);
    if (buf3)
        free (buf3);
}

/*
 * gui_infobar_remove: remove last displayed message in infobar
 */

void
gui_infobar_remove ()
{
    t_gui_infobar *new_infobar;
    
    if (gui_infobar)
    {
        new_infobar = gui_infobar->next_infobar;
        if (gui_infobar->text)
            free (gui_infobar->text);
        free (gui_infobar);
        gui_infobar = new_infobar;
    }
}

/*
 * gui_infobar_remove_all: remove last displayed message in infobar
 */

void
gui_infobar_remove_all ()
{
    while (gui_infobar)
    {
        gui_infobar_remove ();
    }
}

/*
 * gui_input_optimize_size: optimize input buffer size by adding
 *                          or deleting data block (predefined size)
 */

void
gui_input_optimize_size (t_gui_buffer *buffer)
{
    int optimal_size;
    
    if (buffer->has_input)
    {
        optimal_size = ((buffer->input_buffer_size / INPUT_BUFFER_BLOCK_SIZE) *
                        INPUT_BUFFER_BLOCK_SIZE) + INPUT_BUFFER_BLOCK_SIZE;
        if (buffer->input_buffer_alloc != optimal_size)
        {
            buffer->input_buffer_alloc = optimal_size;
            buffer->input_buffer = realloc (buffer->input_buffer, optimal_size);
            buffer->input_buffer_color_mask = realloc (buffer->input_buffer_color_mask,
                                                       optimal_size);
        }
    }
}

/*
 * gui_input_init_color_mask: initialize color mask for input buffer
 */

void
gui_input_init_color_mask (t_gui_buffer *buffer)
{
    int i;
    
    if (buffer->has_input)
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
gui_input_move (t_gui_buffer *buffer, char *target, char *source, int size)
{
    int pos_source, pos_target;
    
    pos_target = target - buffer->input_buffer;
    pos_source = source - buffer->input_buffer;
    
    memmove (target, source, size);
    memmove (buffer->input_buffer_color_mask + pos_target,
             buffer->input_buffer_color_mask + pos_source, size);
}

/*
 * gui_input_complete: complete a word in input buffer
 */

void
gui_input_complete (t_gui_window *window)
{
    int i;
    
    if (window->buffer->completion.word_found)
    {
        /* replace word with new completed word into input buffer */
        if (window->buffer->completion.diff_size > 0)
        {
            window->buffer->input_buffer_size +=
                window->buffer->completion.diff_size;
            window->buffer->input_buffer_length +=
                window->buffer->completion.diff_length;
            gui_input_optimize_size (window->buffer);
            window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            window->buffer->input_buffer_color_mask[window->buffer->input_buffer_size] = '\0';
            for (i = window->buffer->input_buffer_size - 1;
                 i >=  window->buffer->completion.position_replace +
                     (int)strlen (window->buffer->completion.word_found); i--)
            {
                window->buffer->input_buffer[i] =
                    window->buffer->input_buffer[i - window->buffer->completion.diff_size];
                window->buffer->input_buffer_color_mask[i] =
                    window->buffer->input_buffer_color_mask[i - window->buffer->completion.diff_size];
            }
        }
        else
        {
            for (i = window->buffer->completion.position_replace +
                     strlen (window->buffer->completion.word_found);
                 i < window->buffer->input_buffer_size; i++)
            {
                window->buffer->input_buffer[i] =
                    window->buffer->input_buffer[i - window->buffer->completion.diff_size];
                window->buffer->input_buffer_color_mask[i] =
                    window->buffer->input_buffer_color_mask[i - window->buffer->completion.diff_size];
            }
            window->buffer->input_buffer_size +=
                window->buffer->completion.diff_size;
            window->buffer->input_buffer_length +=
                window->buffer->completion.diff_length;
            gui_input_optimize_size (window->buffer);
            window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
            window->buffer->input_buffer_color_mask[window->buffer->input_buffer_size] = '\0';
        }
            
        strncpy (window->buffer->input_buffer + window->buffer->completion.position_replace,
                 window->buffer->completion.word_found,
                 strlen (window->buffer->completion.word_found));
        for (i = 0; i < (int)strlen (window->buffer->completion.word_found); i++)
        {
            window->buffer->input_buffer_color_mask[window->buffer->completion.position_replace + i] = ' ';
        }
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
                             cfg_look_nick_completor, strlen (cfg_look_nick_completor)) != 0)
                    gui_insert_string_input (window, cfg_look_nick_completor,
                                             window->buffer->input_buffer_pos);
                if (window->buffer->completion.position >= 0)
                    window->buffer->completion.position += strlen (cfg_look_nick_completor);
                window->buffer->input_buffer_pos += utf8_strlen (cfg_look_nick_completor);
                if (window->buffer->input_buffer[utf8_real_pos (window->buffer->input_buffer,
                                                                window->buffer->input_buffer_pos)] != ' ')
                    gui_insert_string_input (window, " ",
                                             window->buffer->input_buffer_pos);
                if (window->buffer->completion.position >= 0)
                    window->buffer->completion.position++;
                window->buffer->input_buffer_pos++;
            }
        }
        gui_input_draw (window->buffer, 0);
    }
}

/*
 * gui_exec_action_dcc: execute an action on a DCC after a user input
 *                      return -1 if DCC buffer was closed due to action,
 *                      0 otherwise
 */

void
gui_exec_action_dcc (t_gui_window *window, char *actions)
{
    t_irc_dcc *dcc_selected, *ptr_dcc, *ptr_dcc_next;
    t_gui_buffer *ptr_buffer;
    
    while (actions[0])
    {
        if (actions[0] >= 32)
        {
            dcc_selected = (window->dcc_selected) ?
                (t_irc_dcc *) window->dcc_selected : dcc_list;
            
            switch (actions[0])
            {
                /* accept DCC */
                case 'a':
                case 'A':
                    if (dcc_selected
                        && (DCC_IS_RECV(dcc_selected->status))
                        && (dcc_selected->status == DCC_WAITING))
                    {
                        dcc_accept (dcc_selected);
                    }
                    break;
                /* cancel DCC */
                case 'c':
                case 'C':
                    if (dcc_selected
                        && (!DCC_ENDED(dcc_selected->status)))
                    {
                        dcc_close (dcc_selected, DCC_ABORTED);
                        gui_window_redraw_buffer (window->buffer);
                    }
                    break;
                /* purge old DCC */
                case 'p':
                case 'P':
                    window->dcc_selected = NULL;
                    ptr_dcc = dcc_list;
                    while (ptr_dcc)
                    {
                        ptr_dcc_next = ptr_dcc->next_dcc;
                        if (DCC_ENDED(ptr_dcc->status))
                            dcc_free (ptr_dcc);
                        ptr_dcc = ptr_dcc_next;
                    }
                    gui_window_redraw_buffer (window->buffer);
                    break;
                /* close DCC window */
                case 'q':
                case 'Q':
                    if (gui_buffer_before_dcc)
                    {
                        ptr_buffer = window->buffer;
                        gui_window_switch_to_buffer (window, gui_buffer_before_dcc);
                        gui_buffer_free (ptr_buffer, 0);
                    }
                    else
                        gui_buffer_free (window->buffer, 1);
                    gui_window_redraw_buffer (window->buffer);
                    return;
                    break;
                /* remove from DCC list */
                case 'r':
                case 'R':
                    if (dcc_selected
                        && (DCC_ENDED(dcc_selected->status)))
                    {
                        if (dcc_selected->next_dcc)
                            window->dcc_selected = dcc_selected->next_dcc;
                        else
                            window->dcc_selected = NULL;
                        dcc_free (dcc_selected);
                        gui_window_redraw_buffer (window->buffer);
                    }
                    break;
            }
        }
        actions = utf8_next_char (actions);
    }
}

/*
 * gui_exec_action_raw_data: execute an action on raw IRC data
 *                           return -1 if raw IRC data was closed due to action,
 *                           0 otherwise
 */

void
gui_exec_action_raw_data (t_gui_window *window, char *actions)
{
    t_gui_buffer *ptr_buffer;
    
    while (actions[0])
    {
        if (actions[0] >= 32)
        {
            switch (actions[0])
            {
                /* close raw IRC data */
                case 'q':
                case 'Q':
                    if (gui_buffer_before_raw_data)
                    {
                        ptr_buffer = window->buffer;
                        gui_window_switch_to_buffer (window,
                                                     gui_buffer_before_raw_data);
                        gui_buffer_free (ptr_buffer, 0);
                    }
                    else
                        gui_buffer_free (window->buffer, 1);
                    gui_window_redraw_buffer (window->buffer);
                    return;
                    break;
            }
        }
        actions = utf8_next_char (actions);
    }
}

/*
 * gui_insert_string_input: insert a string into the input buffer
 *                          if pos == -1, string is inserted at cursor position
 *                          return: number of chars inserted
 *                          (may be different of strlen if UTF-8 string)
 */

int
gui_insert_string_input (t_gui_window *window, char *string, int pos)
{
    int i, pos_start, size, length;
    char *ptr_start;
    
    if (window->buffer->has_input)
    {
        if (pos == -1)
            pos = window->buffer->input_buffer_pos;
        
        size = strlen (string);
        length = utf8_strlen (string);
        
        /* increase buffer size */
        window->buffer->input_buffer_size += size;
        window->buffer->input_buffer_length += length;
        gui_input_optimize_size (window->buffer);
        window->buffer->input_buffer[window->buffer->input_buffer_size] = '\0';
        window->buffer->input_buffer_color_mask[window->buffer->input_buffer_size] = '\0';
        
        /* move end of string to the right */
        ptr_start = utf8_add_offset (window->buffer->input_buffer, pos);
        pos_start = ptr_start - window->buffer->input_buffer;
        memmove (ptr_start + size, ptr_start, strlen (ptr_start));
        memmove (window->buffer->input_buffer_color_mask + pos_start + size,
                 window->buffer->input_buffer_color_mask + pos_start,
                 strlen (window->buffer->input_buffer_color_mask + pos_start));
        
        /* insert new string */
        ptr_start = utf8_add_offset (window->buffer->input_buffer, pos);
        pos_start = ptr_start - window->buffer->input_buffer;
        strncpy (ptr_start, string, size);
        for (i = 0; i < size; i++)
        {
            window->buffer->input_buffer_color_mask[pos_start + i] = ' ';
        }
        return length;
    }
    return 0;
}
