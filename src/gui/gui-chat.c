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

/* gui-chat.c: chat functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <regex.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-chat.h"
#include "gui-buffer.h"
#include "gui-color.h"
#include "gui-filter.h"
#include "gui-line.h"
#include "gui-main.h"
#include "gui-window.h"


char *gui_chat_buffer = NULL;                 /* buffer for printf          */
char *gui_chat_prefix[GUI_CHAT_NUM_PREFIXES]; /* prefixes                   */
char gui_chat_prefix_empty[] = "";            /* empty prefix               */
int gui_chat_time_length = 0;    /* length of time for each line (in chars) */


/*
 * gui_chat_prefix_build_empty: build empty prefixes
 *                              (called before reading WeeChat config file)
 */

void
gui_chat_prefix_build_empty ()
{
    gui_chat_prefix[GUI_CHAT_PREFIX_ERROR] = strdup (gui_chat_prefix_empty);
    gui_chat_prefix[GUI_CHAT_PREFIX_NETWORK] = strdup (gui_chat_prefix_empty);
    gui_chat_prefix[GUI_CHAT_PREFIX_ACTION] = strdup (gui_chat_prefix_empty);
    gui_chat_prefix[GUI_CHAT_PREFIX_JOIN] = strdup (gui_chat_prefix_empty);
    gui_chat_prefix[GUI_CHAT_PREFIX_QUIT] = strdup (gui_chat_prefix_empty);
}

/*
 * gui_chat_prefix_build: build prefix with colors
 *                        (called after reading WeeChat config file)
 */

void
gui_chat_prefix_build ()
{
    char prefix[128];
    int i;
    
    for (i = 0; i < GUI_CHAT_NUM_PREFIXES; i++)
    {
        if (gui_chat_prefix[i])
        {
            free (gui_chat_prefix[i]);
            gui_chat_prefix[i] = NULL;
        }
    }
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_ERROR),
              CONFIG_STRING(config_look_prefix[GUI_CHAT_PREFIX_ERROR]));
    gui_chat_prefix[GUI_CHAT_PREFIX_ERROR] = strdup (prefix);
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_NETWORK),
              CONFIG_STRING(config_look_prefix[GUI_CHAT_PREFIX_NETWORK]));
    gui_chat_prefix[GUI_CHAT_PREFIX_NETWORK] = strdup (prefix);
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_ACTION),
              CONFIG_STRING(config_look_prefix[GUI_CHAT_PREFIX_ACTION]));
    gui_chat_prefix[GUI_CHAT_PREFIX_ACTION] = strdup (prefix);
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_JOIN),
              CONFIG_STRING(config_look_prefix[GUI_CHAT_PREFIX_JOIN]));
    gui_chat_prefix[GUI_CHAT_PREFIX_JOIN] = strdup (prefix);
    
    snprintf (prefix, sizeof (prefix), "%s%s\t",
              GUI_COLOR(GUI_COLOR_CHAT_PREFIX_QUIT),
              CONFIG_STRING(config_look_prefix[GUI_CHAT_PREFIX_QUIT]));
    gui_chat_prefix[GUI_CHAT_PREFIX_QUIT] = strdup (prefix);
}

/*
 * gui_chat_strlen_screen: returns number of char needed on sreen to display a
 *                         word special chars like color, bold, .. are ignored
 */

int
gui_chat_strlen_screen (const char *string)
{
    int length, size_on_screen;
    
    length = 0;
    while (string && string[0])
    {
        string = gui_chat_string_next_char (NULL, (unsigned char *)string, 0);
        if (string)
        {
            size_on_screen = (((unsigned char)string[0]) < 32) ? 1 : utf8_char_size_screen (string);
            if (size_on_screen > 0)
                length += size_on_screen;
            string = utf8_next_char (string);
        }
    }
    return length;
}

/*
 * gui_chat_string_add_offset: move forward N chars in a string, skipping all
 *                             formatting chars (like colors,..)
 */

char *
gui_chat_string_add_offset (const char *string, int offset)
{
    while (string && string[0] && (offset > 0))
    {
        string = gui_chat_string_next_char (NULL,
                                            (unsigned char *)string,
                                            0);
        if (string)
        {
            string = utf8_next_char (string);
            offset--;
        }
    }
    return (char *)string;
}

/*
 * gui_chat_string_real_pos: get real position in string
 *                           (ignoring color/bold/.. chars)
 */

int
gui_chat_string_real_pos (const char *string, int pos)
{
    const char *real_pos, *ptr_string;
    int size_on_screen;
    
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
            size_on_screen = (((unsigned char)ptr_string[0]) < 32) ? 1 : utf8_char_size_screen (ptr_string);
            if (size_on_screen > 0)
                pos -= size_on_screen;
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
                        const char *data,
                        int *word_start_offset, int *word_end_offset,
                        int *word_length_with_spaces, int *word_length)
{
    const char *start_data;
    char *next_char, *next_char2;
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
    
    if (!CONFIG_STRING(config_look_buffer_time_format)
        || !CONFIG_STRING(config_look_buffer_time_format)[0])
        return NULL;
    
    local_time = localtime (&date);
    if (strftime (text_time, sizeof (text_time),
                  CONFIG_STRING(config_look_buffer_time_format),
                  local_time) == 0)
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
        for (ptr_line = ptr_buffer->lines->first_line; ptr_line;
             ptr_line = ptr_line->next_line)
        {
            if (ptr_line->data->date != 0)
            {
                if (ptr_line->data->str_time)
                    free (ptr_line->data->str_time);
                ptr_line->data->str_time = gui_chat_get_time_string (ptr_line->data->date);
            }
        }
    }
}

/*
 * gui_chat_build_string_prefix_message: build a string with prefix and message
 */

char *
gui_chat_build_string_prefix_message (struct t_gui_line *line)
{
    char *string, *string_without_colors;
    int length;
    
    length = 0;
    if (line->data->prefix)
        length += strlen (line->data->prefix);
    length++;
    if (line->data->message)
        length += strlen (line->data->message);
    length++;
    
    string = malloc (length);
    if (string)
    {
        string[0] = '\0';
        if (line->data->prefix)
            strcat (string, line->data->prefix);
        strcat (string, "\t");
        if (line->data->message)
            strcat (string, line->data->message);
    }

    if (string)
    {
        string_without_colors = gui_color_decode (string, NULL);
        if (string_without_colors)
        {
            free (string);
            string = string_without_colors;
        }
    }
    
    return string;
}

/*
 * gui_chat_printf_date_tags: display a message in a buffer with optional
 *                            date and tags
 *                            Info: this function works only with formatted
 *                            buffers (not buffers with free content)
 */

void
gui_chat_printf_date_tags (struct t_gui_buffer *buffer, time_t date,
                           const char *tags, const char *message, ...)
{
    va_list argptr;
    time_t date_printed;
    int display_time, length, at_least_one_message_printed;
    char *pos, *pos_prefix, *pos_tab, *pos_end;
    char *modifier_data, *new_msg, *ptr_msg;
    struct t_gui_line *ptr_line;
    
    if (!gui_buffer_valid (buffer))
        return;
    
    if (!message)
        return;
    
    if (gui_init_ok)
    {
        if (!buffer)
            buffer = gui_buffer_search_main ();

        if (!buffer)
            return;
        
        if (buffer->type != GUI_BUFFER_TYPE_FORMATTED)
            buffer = gui_buffers;
        
        if (buffer->type != GUI_BUFFER_TYPE_FORMATTED)
            return;
    }
    
    if (!gui_chat_buffer)
        gui_chat_buffer = malloc (GUI_CHAT_BUFFER_PRINTF_SIZE);
    if (!gui_chat_buffer)
        return;
    
    va_start (argptr, message);
    vsnprintf (gui_chat_buffer, GUI_CHAT_BUFFER_PRINTF_SIZE, message, argptr);
    va_end (argptr);
    
    utf8_normalize (gui_chat_buffer, '?');
    
    date_printed = time (NULL);
    if (date <= 0)
        date = date_printed;
    
    at_least_one_message_printed = 0;
    
    pos = gui_chat_buffer;
    while (pos)
    {
        /* display until next end of line */
        pos_end = strchr (pos, '\n');
        if (pos_end)
            pos_end[0] = '\0';
        
        /* call modifier for message printed ("weechat_print") */
        new_msg = NULL;
        if (buffer)
        {
            length = strlen (plugin_get_name (buffer->plugin)) + 1 +
                strlen (buffer->name) + 1 + ((tags) ? strlen (tags) : 0) + 1;
            modifier_data = malloc (length);
            if (modifier_data)
            {
                snprintf (modifier_data, length, "%s;%s;%s",
                          plugin_get_name (buffer->plugin),
                          buffer->name,
                          (tags) ? tags : "");
                new_msg = hook_modifier_exec (NULL,
                                              "weechat_print",
                                              modifier_data,
                                              pos);
                /* no changes in new message */
                if (new_msg && (strcmp (message, new_msg) == 0))
                {
                    free (new_msg);
                    new_msg = NULL;
                }
                free (modifier_data);
            }
        }
        
        pos_prefix = NULL;
        display_time = 1;
        ptr_msg = (new_msg) ? new_msg : pos;
        
        /* space followed by tab => prefix ignored */
        if ((ptr_msg[0] == ' ') && (ptr_msg[1] == '\t'))
        {
            ptr_msg += 2;
        }
        else
        {
            /* if two first chars are tab, then do not display time */
            if ((ptr_msg[0] == '\t') && (ptr_msg[1] == '\t'))
            {
                display_time = 0;
                ptr_msg += 2;
            }
            else
            {
                /* if tab found, use prefix (before tab) */
                pos_tab = strchr (ptr_msg, '\t');
                if (pos_tab)
                {
                    pos_tab[0] = '\0';
                    pos_prefix = ptr_msg;
                    ptr_msg = pos_tab + 1;
                }
            }
        }
        
        if (gui_init_ok)
        {
            ptr_line = gui_line_add (buffer, (display_time) ? date : 0,
                                     (display_time) ? date_printed : 0,
                                     tags, pos_prefix, ptr_msg);
            if (ptr_line)
            {
                if (buffer->print_hooks_enabled)
                    hook_print_exec (buffer, ptr_line);
                if (ptr_line->data->displayed)
                    at_least_one_message_printed = 1;
            }
        }
        else
        {
            if (pos_prefix)
                string_iconv_fprintf (stdout, "%s ", pos_prefix);
            string_iconv_fprintf (stdout, "%s\n", ptr_msg);
        }
        
        if (new_msg)
            free (new_msg);
        
        pos = (pos_end && pos_end[1]) ? pos_end + 1 : NULL;
    }
    
    if (gui_init_ok && at_least_one_message_printed)
        gui_buffer_ask_chat_refresh (buffer, 1);
}

/*
 * gui_chat_printf_y: display a message on a line in a buffer with free content
 *                    Info: this function works only with free content
 *                    buffers (not formatted buffers)
 */

void
gui_chat_printf_y (struct t_gui_buffer *buffer, int y, const char *message, ...)
{
    va_list argptr;
    struct t_gui_line *ptr_line;
    
    if (gui_init_ok)
    {
        if (!buffer)
            buffer = gui_buffer_search_main ();
        
        if (buffer->type != GUI_BUFFER_TYPE_FREE)
            buffer = gui_buffers;
        
        if (buffer->type != GUI_BUFFER_TYPE_FREE)
            return;
    }
    
    if (!gui_chat_buffer)
        gui_chat_buffer = malloc (GUI_CHAT_BUFFER_PRINTF_SIZE);
    if (!gui_chat_buffer)
        return;
    
    va_start (argptr, message);
    vsnprintf (gui_chat_buffer, GUI_CHAT_BUFFER_PRINTF_SIZE, message, argptr);
    va_end (argptr);
    
    utf8_normalize (gui_chat_buffer, '?');

    /* no message: delete line */
    if (!gui_chat_buffer[0])
    {
        if (gui_init_ok)
        {
            for (ptr_line = buffer->lines->first_line; ptr_line;
                 ptr_line = ptr_line->next_line)
            {
                if (ptr_line->data->y >= y)
                    break;
            }
            if (ptr_line && (ptr_line->data->y == y))
            {
                gui_line_free (buffer, ptr_line);
                gui_buffer_ask_chat_refresh (buffer, 2);
            }
        }
    }
    else
    {
        if (gui_init_ok)
        {
            gui_line_add_y (buffer, y, gui_chat_buffer);
            gui_buffer_ask_chat_refresh (buffer, 1);
        }
        else
            string_iconv_fprintf (stdout, "%s\n", gui_chat_buffer);
    }
}

/*
 * gui_chat_free_buffer: free buffer used by chat functions
 */

void
gui_chat_free_buffer ()
{
    if (gui_chat_buffer)
    {
        free (gui_chat_buffer);
        gui_chat_buffer = NULL;
    }
}
