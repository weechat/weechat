/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* gui-chat.c: chat functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "gui-chat.h"
#include "gui-hotlist.h"
#include "gui-main.h"
#include "gui-status.h"
#include "gui-window.h"


char *gui_chat_prefix[GUI_CHAT_PREFIX_NUMBER]; /* prefixes                  */
int gui_chat_time_length = 0;    /* length of time for each line (in chars) */


/*
 * gui_chat_prefix_build: build prefix with colors
 */

void
gui_chat_prefix_build ()
{
    char prefix[128];
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_INFO),
              cfg_look_prefix[GUI_CHAT_PREFIX_INFO]);
    gui_chat_prefix[GUI_CHAT_PREFIX_INFO] = strdup (prefix);
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_ERROR),
              cfg_look_prefix[GUI_CHAT_PREFIX_ERROR]);
    gui_chat_prefix[GUI_CHAT_PREFIX_ERROR] = strdup (prefix);
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_NETWORK),
              cfg_look_prefix[GUI_CHAT_PREFIX_NETWORK]);
    gui_chat_prefix[GUI_CHAT_PREFIX_NETWORK] = strdup (prefix);
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_ACTION),
              cfg_look_prefix[GUI_CHAT_PREFIX_ACTION]);
    gui_chat_prefix[GUI_CHAT_PREFIX_ACTION] = strdup (prefix);
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_JOIN),
              cfg_look_prefix[GUI_CHAT_PREFIX_JOIN]);
    gui_chat_prefix[GUI_CHAT_PREFIX_JOIN] = strdup (prefix);
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_QUIT),
              cfg_look_prefix[GUI_CHAT_PREFIX_QUIT]);
    gui_chat_prefix[GUI_CHAT_PREFIX_QUIT] = strdup (prefix);
}

/*
 * gui_chat_strlen_screen: returns number of char needed on sreen to display a
 *                         word special chars like color, bold, .. are ignored
 */

int
gui_chat_strlen_screen (char *string)
{
    int length;
    
    length = 0;
    while (string && string[0])
    {
        string = gui_chat_string_next_char (NULL, (unsigned char *)string, 0);
        if (string)
        {
            length += utf8_char_size_screen (string);
            string = utf8_next_char (string);
        }
    }
    return length;
}

/*
 * gui_chat_string_real_pos: get real position in string
 *                           (ignoring color/bold/.. chars)
 */

int
gui_chat_string_real_pos (char *string, int pos)
{
    char *ptr_string, *real_pos;
    
    if (pos <= 0)
        return 0;
    
    real_pos = string;
    ptr_string = string;
    while (ptr_string && ptr_string[0] && (pos > 0))
    {
        ptr_string = gui_chat_string_next_char (NULL,
                                                (unsigned char *)ptr_string,
                                                0);
        if (ptr_string)
        {
            pos -= utf8_char_size_screen (ptr_string);
            ptr_string = utf8_next_char (ptr_string);
            real_pos = ptr_string;
        }
    }
    return 0 + (real_pos - string);
}

/*
 * gui_chat_get_word_info: returns info about next word: beginning, end, length
 */

void
gui_chat_get_word_info (struct t_gui_window *window,
                        char *data,
                        int *word_start_offset, int *word_end_offset,
                        int *word_length_with_spaces, int *word_length)
{
    char *start_data, *next_char, *next_char2;
    int leading_spaces, char_size;
    
    *word_start_offset = 0;
    *word_end_offset = 0;
    *word_length_with_spaces = 0;
    *word_length = 0;
    
    start_data = data;
    
    leading_spaces = 1;
    while (data && data[0])
    {
        next_char = gui_chat_string_next_char (window, (unsigned char *)data,
                                               0);
        if (next_char)
        {
            next_char2 = utf8_next_char (next_char);
            if (next_char2)
            {
                if (next_char[0] != ' ')
                {
                    if (leading_spaces)
                        *word_start_offset = next_char - start_data;
                    leading_spaces = 0;
                    char_size = next_char2 - next_char;
                    *word_end_offset = next_char2 - start_data - 1;
                    (*word_length_with_spaces) += char_size;
                    (*word_length) += char_size;
                }
                else
                {
                    if (leading_spaces)
                        (*word_length_with_spaces)++;
                    else
                    {
                        *word_end_offset = next_char - start_data - 1;
                        return;
                    }
                }
                data = next_char2;
            }
        }
        else
        {
            *word_end_offset = data + strlen (data) - start_data - 1;
            return;
        }
    }
}

/*
 * gu_chat_get_time_string: get time string, for display (with colors)
 */

char *
gui_chat_get_time_string (time_t date)
{
    char text_time[128], text_time2[(128*3)+16], text_time_char[2];
    int i, time_first_digit, time_last_digit, last_color;
    struct tm *local_time;
    
    if (!cfg_look_buffer_time_format
        || !cfg_look_buffer_time_format[0])
        return NULL;
    
    local_time = localtime (&date);
    if (strftime (text_time, sizeof (text_time),
                  cfg_look_buffer_time_format, local_time) == 0)
        return NULL;
    
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
    
    text_time2[0] = '\0';
    text_time_char[1] = '\0';
    last_color = -1;
    i = 0;
    while (text_time[i])
    {
        text_time_char[0] = text_time[i];
        if (time_first_digit < 0)
        {
            if (last_color != GUI_COLOR_CHAT_TIME)
            {
                strcat (text_time2, GUI_COLOR(GUI_COLOR_CHAT_TIME));
                last_color = GUI_COLOR_CHAT_TIME;
            }
            strcat (text_time2, text_time_char);
        }
        else
        {
            if ((i < time_first_digit) || (i > time_last_digit))
            {
                if (last_color != GUI_COLOR_CHAT_DELIMITERS)
                {
                    strcat (text_time2, GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
                    last_color = GUI_COLOR_CHAT_DELIMITERS;
                }
                strcat (text_time2, text_time_char);
            }
            else
            {
                if (isdigit (text_time[i]))
                {
                    if (last_color != GUI_COLOR_CHAT_TIME)
                    {
                        strcat (text_time2, GUI_COLOR(GUI_COLOR_CHAT_TIME));
                        last_color = GUI_COLOR_CHAT_TIME;
                    }
                    strcat (text_time2, text_time_char);
                }
                else
                {
                    if (last_color != GUI_COLOR_CHAT_TIME_DELIMITERS)
                    {
                        strcat (text_time2,
                                GUI_COLOR(GUI_COLOR_CHAT_TIME_DELIMITERS));
                        last_color = GUI_COLOR_CHAT_TIME_DELIMITERS;
                    }
                    strcat (text_time2, text_time_char);
                }
            }
        }
        i++;
    }
    
    return strdup (text_time2);
}

/*
 * gui_chat_change_time_format: change time format for all lines of all buffers
 */

void
gui_chat_change_time_format ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *ptr_line;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        for (ptr_line = ptr_buffer->lines; ptr_line;
             ptr_line = ptr_line->next_line)
        {
            if (ptr_line->date != 0)
            {
                if (ptr_line->str_time)
                    free (ptr_line->str_time);
                ptr_line->str_time = gui_chat_get_time_string (ptr_line->date);
            }
        }
    }
}

/*
 * gui_chat_get_line_align: get alignment for a line
 */

int
gui_chat_get_line_align (struct t_gui_buffer *buffer, struct t_gui_line *line,
                         int with_suffix)
{
    int length_suffix;
    
    if (cfg_look_prefix_align == CFG_LOOK_PREFIX_ALIGN_NONE)
        return gui_chat_time_length + 1 + line->prefix_length + 2;

    length_suffix = 0;
    if (with_suffix)
    {
        if (cfg_look_prefix_suffix && cfg_look_prefix_suffix[0])
            length_suffix = gui_chat_strlen_screen (cfg_look_prefix_suffix) + 1;
    }
    if (cfg_look_prefix_align_max > 0)
        return gui_chat_time_length + 1 + cfg_look_prefix_align_max +
            length_suffix + 1;
    else
        return gui_chat_time_length + 1 + buffer->prefix_max_length +
            length_suffix + 1;
}

/*
 * gui_chat_line_search: search for text in a line
 */

int
gui_chat_line_search (struct t_gui_line *line, char *text, int case_sensitive)
{
    char *message;
    int rc;
    
    if (!line || !line->message || !text || !text[0])
        return 0;
    
    rc = 0;
    message = (char *)gui_color_decode ((unsigned char *)line->message);
    if (message)
    {
        if ((case_sensitive && (strstr (message, text)))
            || (!case_sensitive && (string_strcasestr (message, text))))
            rc = 1;
        free (message);
    }
    return rc;
}

/*
 * gui_chat_line_free: delete a line from a buffer
 */

void
gui_chat_line_free (struct t_gui_line *line)
{
    struct t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->start_line == line)
        {
            ptr_win->start_line = ptr_win->start_line->next_line;
            ptr_win->start_line_pos = 0;
            gui_chat_draw (ptr_win->buffer, 0);
            gui_status_draw (ptr_win->buffer, 0);
        }
    }
    if (line->str_time)
        free (line->str_time);
    if (line->prefix)
        free (line->prefix);
    if (line->message)
        free (line->message);
    free (line);
}

/*
 * gui_chat_line_add: add a new line for a buffer
 */

void
gui_chat_line_add (struct t_gui_buffer *buffer, time_t date, char *prefix,
                   char *message)
{
    struct t_gui_line *new_line, *ptr_line;
    
    new_line = (struct t_gui_line *) malloc (sizeof (struct t_gui_line));
    if (!new_line)
    {
        weechat_log_printf (_("Not enough memory for new line\n"));
        return;
    }
    
    /* add new line */
    new_line->date = date;
    new_line->str_time = (date == 0) ?
        NULL : gui_chat_get_time_string (date);
    new_line->prefix = (prefix) ?
        strdup (prefix) : ((date != 0) ? strdup ("") : NULL);
    new_line->prefix_length = (prefix) ?
        gui_chat_strlen_screen (prefix) : 0;
    if (new_line->prefix_length > buffer->prefix_max_length)
        buffer->prefix_max_length = new_line->prefix_length;
    new_line->message = (message) ? strdup (message) : strdup ("");
    if (!buffer->lines)
        buffer->lines = new_line;
    else
        buffer->last_line->next_line = new_line;
    new_line->prev_line = buffer->last_line;
    new_line->next_line = NULL;
    buffer->last_line = new_line;
    buffer->lines_count++;
    
    /* remove one line if necessary */
    if ((cfg_history_max_lines > 0)
        && (buffer->lines_count > cfg_history_max_lines))
    {
        if (buffer->last_line == buffer->lines)
            buffer->last_line = NULL;
        ptr_line = buffer->lines->next_line;
        gui_chat_line_free (buffer->lines);
        buffer->lines = ptr_line;
        ptr_line->prev_line = NULL;
        buffer->lines_count--;
    }
}

/*
 * gui_chat_printf: display a message in a buffer
 */

void
gui_chat_printf (struct t_gui_buffer *buffer, char *message, ...)
{
    static char buf[8192];
    time_t date;
    int display_time;
    char *pos, *pos_prefix, *pos_tab, *pos_end;
    va_list argptr;
    
    if (gui_init_ok)
    {
        if (buffer == NULL)
            buffer = gui_buffers;
        
        if (buffer->type == GUI_BUFFER_TYPE_FREE)
            buffer = gui_buffers;
        
        if (buffer->type == GUI_BUFFER_TYPE_FREE)
            return;
    }
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    
    utf8_normalize (buf, '?');
    
    date = time (NULL);
    
    pos = buf;
    while (pos)
    {
        pos_prefix = NULL;
        display_time = 1;
        
        /* if two first chars are tab, then do not display time */
        if ((buf[0] == '\t') && (buf[1] == '\t'))
        {
            display_time = 0;
            pos += 2;
        }
        else
        {
            /* if tab found, use prefix (before tab) */
            pos_tab = strchr (buf, '\t');
            if (pos_tab)
            {
                pos_tab[0] = '\0';
                pos_prefix = buf;
                pos = pos_tab + 1;
            }
        }

        /* display until next end of line */
        pos_end = strchr (pos, '\n');
        if (pos_end)
            pos_end[0] = '\0';

        if (gui_init_ok)
            gui_chat_line_add (buffer, (display_time) ? date : 0,
                               pos_prefix, pos);
        else
        {
            if (pos_prefix)
                string_iconv_fprintf (stdout, "%s ", pos_prefix);
            string_iconv_fprintf (stdout, "%s\n", pos);
        }
        
        pos = (pos_end && pos_end[1]) ? pos_end + 1 : NULL;
    }
    
    if (gui_init_ok)
    {
        buffer->chat_refresh_needed = 1;
        if (gui_add_hotlist
            && ((buffer->num_displayed == 0)
                || (gui_buffer_is_scrolled (buffer))))
        {
            gui_hotlist_add (buffer, 0, NULL, 1);
            gui_status_draw (buffer, 0);
        }
    }
}

/*
 * gui_chat_printf_raw_data: display raw IRC data (only if raw IRC data buffer
 *                           exists)
 *                           type: 0 = recv, 1 = send, -1 = recv, modified by
 *                                 a modifier (plugin)
 */

void
gui_chat_printf_raw_data (void *server, int send, int modified, char *message)
{
    (void) server;
    (void) send;
    (void) modified;
    (void) message;
    
/*    char *pos;
    
    if (gui_buffer_raw_data)
    {
        while (message && message[0])
        {
            pos = strstr (message, "\r\n");
            if (pos)
                pos[0] = '\0';
            gui_printf_nolog (gui_buffer_raw_data,
                              "%s[%s%s%s] %s%s%s %s\n",
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                              GUI_COLOR(GUI_COLOR_WIN_CHAT_SERVER),
                              ((t_irc_server *)server)->name,
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                              GUI_COLOR((send) ? GUI_COLOR_WIN_CHAT_PART : GUI_COLOR_WIN_CHAT_JOIN),
                              (send) ? ((modified) ? GUI_PREFIX_SEND_MOD : GUI_PREFIX_PART) :
                              ((modified) ? GUI_PREFIX_RECV_MOD : GUI_PREFIX_JOIN),
                              GUI_COLOR(GUI_COLOR_CHAT),
                              message);
            if (pos)
            {
                pos[0] = '\r';
                message = pos + 2;
            }
            else
                message = NULL;
        }
    }*/
}
