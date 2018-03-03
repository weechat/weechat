/*
 * gui-chat.c - chat functions (used by all GUI)
 *
 * Copyright (C) 2003-2018 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#ifdef HAVE_PCRE
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-eval.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-hook.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-chat.h"
#include "gui-buffer.h"
#include "gui-color.h"
#include "gui-filter.h"
#include "gui-input.h"
#include "gui-line.h"
#include "gui-main.h"
#include "gui-window.h"


char *gui_chat_prefix[GUI_CHAT_NUM_PREFIXES];   /* prefixes                 */
char gui_chat_prefix_empty[] = "";              /* empty prefix             */
int gui_chat_time_length = 0;    /* length of time for each line (in chars) */
int gui_chat_mute = GUI_CHAT_MUTE_DISABLED;     /* mute mode                */
struct t_gui_buffer *gui_chat_mute_buffer = NULL; /* mute buffer            */
int gui_chat_display_tags = 0;                  /* display tags?            */
char *gui_chat_lines_waiting_buffer = NULL;     /* lines waiting for core   */
                                                /* buffer                   */


/*
 * Initializes some variables for chat area (called before reading WeeChat
 * configuration file).
 */

void
gui_chat_init ()
{
    char *default_prefix[GUI_CHAT_NUM_PREFIXES] =
        { GUI_CHAT_PREFIX_ERROR_DEFAULT, GUI_CHAT_PREFIX_NETWORK_DEFAULT,
          GUI_CHAT_PREFIX_ACTION_DEFAULT, GUI_CHAT_PREFIX_JOIN_DEFAULT,
          GUI_CHAT_PREFIX_QUIT_DEFAULT };
    char str_prefix[64];
    int i;

    /* build default prefixes */
    for (i = 0; i < GUI_CHAT_NUM_PREFIXES; i++)
    {
        snprintf (str_prefix, sizeof (str_prefix), "%s\t", default_prefix[i]);
        gui_chat_prefix[i] = strdup (str_prefix);
    }

    /* some hsignals */
    hook_hsignal (NULL, "chat_quote_time_prefix_message",
                  &gui_chat_hsignal_quote_line_cb, NULL, NULL);
    hook_hsignal (NULL, "chat_quote_prefix_message",
                  &gui_chat_hsignal_quote_line_cb, NULL, NULL);
    hook_hsignal (NULL, "chat_quote_message",
                  &gui_chat_hsignal_quote_line_cb, NULL, NULL);
}

/*
 * Builds prefix with colors (called after reading WeeChat configuration file).
 */

void
gui_chat_prefix_build ()
{
    const char *ptr_prefix;
    char prefix[512], *pos_color;
    int prefix_color[GUI_CHAT_NUM_PREFIXES] =
        { GUI_COLOR_CHAT_PREFIX_ERROR, GUI_COLOR_CHAT_PREFIX_NETWORK,
          GUI_COLOR_CHAT_PREFIX_ACTION, GUI_COLOR_CHAT_PREFIX_JOIN,
          GUI_COLOR_CHAT_PREFIX_QUIT };
    int i;

    for (i = 0; i < GUI_CHAT_NUM_PREFIXES; i++)
    {
        if (gui_chat_prefix[i])
        {
            free (gui_chat_prefix[i]);
            gui_chat_prefix[i] = NULL;
        }

        ptr_prefix = CONFIG_STRING(config_look_prefix[i]);
        pos_color = strstr (ptr_prefix, "${");

        snprintf (prefix, sizeof (prefix), "%s%s\t",
                  (ptr_prefix[0] && (!pos_color || (pos_color > ptr_prefix))) ? GUI_COLOR(prefix_color[i]) : "",
                  ptr_prefix);

        if (pos_color)
            gui_chat_prefix[i] = eval_expression (prefix, NULL, NULL, NULL);
        else
            gui_chat_prefix[i] = strdup (prefix);
    }
}

/*
 * Checks if an UTF-8 char is valid for screen.
 *
 * Returns:
 *   1: char is valid
 *   0: char is not valid
 */

int
gui_chat_utf_char_valid (const char *utf_char)
{
    /* chars below 32 are not valid (except TAB) */
    if (((unsigned char)utf_char[0] < 32) && (utf_char[0] != '\t'))
        return 0;

    /* 146 or 0x7F are not valid */
    if ((((unsigned char)(utf_char[0]) == 146)
         || ((unsigned char)(utf_char[0]) == 0x7F))
        && (!utf_char[1]))
        return 0;

    /* any other char is valid */
    return 1;
}

/*
 * Returns number of char needed on screen to display a char.
 */

int
gui_chat_char_size_screen (const char *utf_char)
{
    /* if char is invalid, it will be displayed as one space on screen */
    if (!gui_chat_utf_char_valid (utf_char))
        return 1;

    return utf8_char_size_screen (utf_char);
}

/*
 * Returns number of char needed on screen to display a word (special chars like
 * colors/attributes are ignored).
 */

int
gui_chat_strlen_screen (const char *string)
{
    int length, size_on_screen;

    length = 0;
    while (string && string[0])
    {
        string = gui_chat_string_next_char (NULL, NULL,
                                            (unsigned char *)string, 0, 0, 0);
        if (string)
        {
            size_on_screen = gui_chat_char_size_screen (string);
            if (size_on_screen > 0)
                length += size_on_screen;
            string = utf8_next_char (string);
        }
    }
    return length;
}

/*
 * Moves forward N chars in a string, skipping all formatting chars (like
 * colors/attributes).
 */

const char *
gui_chat_string_add_offset (const char *string, int offset)
{
    while (string && string[0] && (offset > 0))
    {
        string = gui_chat_string_next_char (NULL, NULL,
                                            (unsigned char *)string,
                                            0, 0, 0);
        if (string)
        {
            string = utf8_next_char (string);
            offset--;
        }
    }
    return string;
}

/*
 * Moves forward N chars (using size on screen) in a string, skipping all
 * formatting chars (like colors/attributes).
 */

const char *
gui_chat_string_add_offset_screen (const char *string, int offset_screen)
{
    int size_on_screen;

    while (string && string[0] && (offset_screen >= 0))
    {
        string = gui_chat_string_next_char (NULL, NULL,
                                            (unsigned char *)string,
                                            0, 0, 0);
        if (string)
        {
            size_on_screen = gui_chat_char_size_screen (string);
            offset_screen -= size_on_screen;
            if (offset_screen < 0)
                return string;
            string = utf8_next_char (string);
        }
    }
    return string;
}

/*
 * Gets real position in string (ignoring formatting chars like
 * colors/attributes).
 *
 * If argument "use_screen_size" is 0, the "pos" argument is a number of UTF-8
 * chars.
 * If argument "use_screen_size" is 1, the "pos" argument is the width of UTF-8
 * chars on screen.
 *
 * Returns real position, in bytes.
 */

int
gui_chat_string_real_pos (const char *string, int pos, int use_screen_size)
{
    const char *real_pos, *real_pos_prev, *ptr_string;
    int size_on_screen;

    if (pos <= 0)
        return 0;

    real_pos = string;
    real_pos_prev = string;
    ptr_string = string;
    while (ptr_string && ptr_string[0] && (pos >= 0))
    {
        ptr_string = gui_chat_string_next_char (NULL, NULL,
                                                (unsigned char *)ptr_string,
                                                0, 0, 0);
        if (ptr_string)
        {
            size_on_screen = gui_chat_char_size_screen (ptr_string);
            if (size_on_screen > 0)
                pos -= (use_screen_size) ? size_on_screen : 1;
            ptr_string = utf8_next_char (ptr_string);
            real_pos_prev = real_pos;
            real_pos = ptr_string;
        }
    }
    if (pos < 0)
        real_pos = real_pos_prev;
    return 0 + (real_pos - string);
}

/*
 * Gets real position in string (ignoring formatting chars like
 * colors/attributes).
 *
 * Returns position, in number of UTF-8 chars.
 */

int
gui_chat_string_pos (const char *string, int real_pos)
{
    const char *ptr_string, *limit;
    int count;

    count = 0;
    ptr_string = string;
    limit = ptr_string + real_pos;
    while (ptr_string && ptr_string[0] && (ptr_string < limit))
    {
        ptr_string = gui_chat_string_next_char (NULL, NULL,
                                                (unsigned char *)ptr_string,
                                                0, 0, 0);
        if (ptr_string)
        {
            ptr_string = utf8_next_char (ptr_string);
            count++;
        }
    }
    return count;
}

/*
 * Returns info about next word: beginning, end, length.
 *
 * Note: the word_{start|end}_offset are in bytes, but word_length(_with_spaces)
 * are in number of chars on screen.
 */

void
gui_chat_get_word_info (struct t_gui_window *window,
                        const char *data,
                        int *word_start_offset, int *word_end_offset,
                        int *word_length_with_spaces, int *word_length)
{
    const char *start_data, *next_char, *next_char2;
    int leading_spaces, char_size_screen;

    *word_start_offset = 0;
    *word_end_offset = 0;
    *word_length_with_spaces = 0;
    *word_length = -1;

    start_data = data;

    leading_spaces = 1;
    while (data && data[0])
    {
        next_char = gui_chat_string_next_char (window, NULL,
                                               (unsigned char *)data, 0, 0, 0);
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
                    *word_end_offset = next_char2 - start_data - 1;
                    char_size_screen = gui_chat_char_size_screen (next_char);
                    (*word_length_with_spaces) += char_size_screen;
                    if (*word_length < 0)
                        *word_length = 0;
                    (*word_length) += char_size_screen;
                }
                else
                {
                    if (leading_spaces)
                    {
                        (*word_length_with_spaces)++;
                        *word_end_offset = next_char2 - start_data - 1;
                    }
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
 * Gets time string, for display (with colors).
 *
 * Note: result must be freed after use.
 */

char *
gui_chat_get_time_string (time_t date)
{
    char text_time[128], text_time2[(128*3)+16], text_time_char[2];
    char *text_with_color;
    int i, time_first_digit, time_last_digit, last_color;
    struct tm *local_time;

    if (date == 0)
        return NULL;

    if (!CONFIG_STRING(config_look_buffer_time_format)
        || !CONFIG_STRING(config_look_buffer_time_format)[0])
        return NULL;

    local_time = localtime (&date);
    if (!local_time)
        return NULL;
    if (strftime (text_time, sizeof (text_time),
                  CONFIG_STRING(config_look_buffer_time_format),
                  local_time) == 0)
        return NULL;

    if (strstr (text_time, "${"))
    {
        text_with_color = eval_expression (text_time, NULL, NULL, NULL);
        if (text_with_color)
        {
            if (strcmp (text_time, text_with_color) != 0)
                return text_with_color;
            free (text_with_color);
        }
    }

    time_first_digit = -1;
    time_last_digit = -1;
    i = 0;
    while (text_time[i])
    {
        if (isdigit ((unsigned char)text_time[i]))
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
                if (isdigit ((unsigned char)text_time[i]))
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
 * Calculates time length with a time format (format can include color codes
 * with format ${name}).
 */

int
gui_chat_get_time_length ()
{
    time_t date;
    char *text_time;
    int length;

    if (!CONFIG_STRING(config_look_buffer_time_format)
        || !CONFIG_STRING(config_look_buffer_time_format)[0])
        return 0;

    length = 0;
    date = time (NULL);
    text_time = gui_chat_get_time_string (date);

    if (text_time)
    {
        length = gui_chat_strlen_screen (text_time);
        free (text_time);
    }

    return length;
}

/*
 * Changes time format for all lines of all buffers.
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
 * Builds a string with prefix and message.
 *
 * Note: result must be freed after use.
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
 * Builds a string with message and tags.
 *
 * Note: result must be freed after use.
 */


char *
gui_chat_build_string_message_tags (struct t_gui_line *line)
{
    int i, length;
    char *buf;

    length = 64 + 2;
    if (line->data->message)
        length += strlen (line->data->message);
    for (i = 0; i < line->data->tags_count; i++)
    {
        length += strlen (line->data->tags_array[i]) + 1;
    }
    length += 2;

    buf = malloc (length);
    buf[0] = '\0';
    if (line->data->message)
        strcat (buf, line->data->message);
    strcat (buf, GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    strcat (buf, " [");
    strcat (buf, GUI_COLOR(GUI_COLOR_CHAT_TAGS));
    for (i = 0; i < line->data->tags_count; i++)
    {
        strcat (buf, line->data->tags_array[i]);
        if (i < line->data->tags_count - 1)
            strcat (buf, ",");
    }
    strcat (buf, GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    strcat (buf, "]");

    return buf;
}

/*
 * Displays a message in a buffer with optional date and tags.
 *
 * Note: this function works only with formatted buffers (not buffers with free
 * content).
 */

void
gui_chat_printf_date_tags (struct t_gui_buffer *buffer, time_t date,
                           const char *tags, const char *message, ...)
{
    time_t date_printed;
    int display_time, length, at_least_one_message_printed, msg_discarded;
    char *pos, *pos_prefix, *pos_tab, *pos_end, *pos_lines;
    char *modifier_data, *new_msg, *ptr_msg, *lines_waiting;
    struct t_gui_line *ptr_line;

    if (!message)
        return;

    if (!gui_buffer_valid (buffer))
        return;

    if (gui_init_ok)
    {
        if (!buffer)
            buffer = gui_buffer_search_main ();

        if (!buffer || buffer->closing)
            return;

        if (buffer->type != GUI_BUFFER_TYPE_FORMATTED)
            buffer = gui_buffers;

        if (buffer->type != GUI_BUFFER_TYPE_FORMATTED)
            return;
    }

    /* if mute is enabled for buffer (or all buffers), then just return */
    if ((gui_chat_mute == GUI_CHAT_MUTE_ALL_BUFFERS)
        || ((gui_chat_mute == GUI_CHAT_MUTE_BUFFER)
            && (gui_chat_mute_buffer == buffer)))
        return;

    weechat_va_format (message);
    if (!vbuffer)
        return;

    utf8_normalize (vbuffer, '?');

    date_printed = time (NULL);
    if (date <= 0)
        date = date_printed;

    at_least_one_message_printed = 0;

    pos = vbuffer;
    while (pos)
    {
        /* display until next end of line */
        pos_end = strchr (pos, '\n');
        if (pos_end)
            pos_end[0] = '\0';

        /* call modifier for message printed ("weechat_print") */
        new_msg = NULL;
        msg_discarded = 0;
        if (buffer)
        {
            length = strlen (gui_buffer_get_plugin_name (buffer)) + 1 +
                strlen (buffer->name) + 1 + ((tags) ? strlen (tags) : 0) + 1;
            modifier_data = malloc (length);
            if (modifier_data)
            {
                snprintf (modifier_data, length, "%s;%s;%s",
                          gui_buffer_get_plugin_name (buffer),
                          buffer->name,
                          (tags) ? tags : "");
                new_msg = hook_modifier_exec (NULL,
                                              "weechat_print",
                                              modifier_data,
                                              pos);
                free (modifier_data);
                if (new_msg)
                {
                    if (!new_msg[0] && pos[0])
                    {
                        /*
                         * modifier returned empty message, then we'll not
                         * print anything
                         */
                        free (new_msg);
                        new_msg = NULL;
                        msg_discarded = 1;
                    }
                    else if (strcmp (message, new_msg) == 0)
                    {
                        /* no changes in new message */
                        free (new_msg);
                        new_msg = NULL;
                    }
                }
            }
        }

        if (!msg_discarded)
        {
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
                                         date_printed, tags, pos_prefix, ptr_msg);
                if (ptr_line)
                {
                    if (buffer && buffer->print_hooks_enabled)
                        hook_print_exec (buffer, ptr_line);
                    if (ptr_line->data->displayed)
                        at_least_one_message_printed = 1;
                }
            }
            else
            {
                length = ((pos_prefix) ? strlen (pos_prefix) + 1 : 0) +
                    strlen (ptr_msg) + 1;
                if (gui_chat_lines_waiting_buffer)
                {
                    length += strlen (gui_chat_lines_waiting_buffer) + 1;
                    lines_waiting = realloc (gui_chat_lines_waiting_buffer, length);
                    if (lines_waiting)
                    {
                        gui_chat_lines_waiting_buffer = lines_waiting;
                    }
                    else
                    {
                        free (gui_chat_lines_waiting_buffer);
                        gui_chat_lines_waiting_buffer = NULL;
                    }
                }
                else
                {
                    gui_chat_lines_waiting_buffer = malloc (length);
                    if (gui_chat_lines_waiting_buffer)
                        gui_chat_lines_waiting_buffer[0] = '\0';
                }
                if (gui_chat_lines_waiting_buffer)
                {
                    pos_lines = gui_chat_lines_waiting_buffer +
                        strlen (gui_chat_lines_waiting_buffer);
                    if (pos_lines > gui_chat_lines_waiting_buffer)
                    {
                        pos_lines[0] = '\n';
                        pos_lines++;
                    }
                    if (pos_prefix)
                    {
                        memcpy (pos_lines, pos_prefix, strlen (pos_prefix));
                        pos_lines += strlen (pos_prefix);
                        pos_lines[0] = '\t';
                        pos_lines++;
                    }
                    memcpy (pos_lines, ptr_msg, strlen (ptr_msg) + 1);
                }
            }
        }

        if (new_msg)
            free (new_msg);

        pos = (pos_end && pos_end[1]) ? pos_end + 1 : NULL;
    }

    if (gui_init_ok && at_least_one_message_printed)
        gui_buffer_ask_chat_refresh (buffer, 1);

    free (vbuffer);
}

/*
 * Displays a message on a line in a buffer with free content.
 *
 * Note: this function works only with free content buffers (not formatted
 * buffers).
 */

void
gui_chat_printf_y (struct t_gui_buffer *buffer, int y, const char *message, ...)
{
    struct t_gui_line *ptr_line;
    int i, num_lines_to_add;

    if (!gui_buffer_valid (buffer))
        return;

    if (gui_init_ok)
    {
        if (!buffer)
            buffer = gui_buffer_search_main ();

        if (!buffer || buffer->closing)
            return;

        if (buffer->type != GUI_BUFFER_TYPE_FREE)
            buffer = gui_buffers;

        if (buffer->type != GUI_BUFFER_TYPE_FREE)
            return;
    }

    weechat_va_format (message);
    if (!vbuffer)
        return;

    utf8_normalize (vbuffer, '?');

    /* no message: delete line */
    if (!vbuffer[0])
    {
        if (gui_init_ok && (y >= 0))
        {
            for (ptr_line = buffer->own_lines->first_line; ptr_line;
                 ptr_line = ptr_line->next_line)
            {
                if (ptr_line->data->y >= y)
                    break;
            }
            if (ptr_line && (ptr_line->data->y == y))
            {
                if (ptr_line->next_line)
                    gui_line_clear (ptr_line);
                else
                    gui_line_free (buffer, ptr_line);
                gui_buffer_ask_chat_refresh (buffer, 2);
            }
        }
    }
    else
    {
        if (gui_init_ok)
        {
            /* if y is negative, add a line -N lines after the last line */
            if (y < 0)
            {
                y = (buffer->own_lines && buffer->own_lines->last_line) ?
                    buffer->own_lines->last_line->data->y - y : (-1 * y) - 1;
            }
            /* compute the number of lines to add before y */
            if (buffer->own_lines && buffer->own_lines->last_line)
                num_lines_to_add = y - buffer->own_lines->last_line->data->y - 1;
            else
                num_lines_to_add = y;
            if (num_lines_to_add > 0)
            {
                /*
                 * add empty line(s) before asked line, to ensure there is at
                 * least "y" lines in buffer, and then be able to scroll
                 * properly buffer page by page
                 */
                for (i = y - num_lines_to_add; i < y; i++)
                {
                    gui_line_add_y (buffer, i, "");
                }
            }
            gui_line_add_y (buffer, y, vbuffer);
            gui_buffer_ask_chat_refresh (buffer, 1);
        }
        else
            string_fprintf (stdout, "%s\n", vbuffer);
    }

    free (vbuffer);
}

/*
 * Displays lines that are waiting for buffer.
 *
 * If "f" is not NULL, the lines are written in this file (which is commonly
 * stdout or stderr).
 * If "f" is NULL, the lines are displayed in core buffer if the GUI is
 * initialized (gui_init_ok == 1), otherwise on stdout.
 */

void
gui_chat_print_lines_waiting_buffer (FILE *f)
{
    char **lines;
    int num_lines, i;

    if (gui_chat_lines_waiting_buffer)
    {
        lines = string_split (gui_chat_lines_waiting_buffer, "\n", 0, 0,
                              &num_lines);
        if (lines)
        {
            for (i = 0; i < num_lines; i++)
            {
                if (!f && gui_init_ok)
                    gui_chat_printf (NULL, "%s", lines[i]);
                else
                    string_fprintf ((f) ? f : stdout, "%s\n", lines[i]);
            }
            string_free_split (lines);
        }
        /*
         * gui_chat_lines_waiting_buffer may be NULL after call to
         * gui_chat_printf (if not enough memory)
         */
    }
    if (gui_chat_lines_waiting_buffer)
    {
        free (gui_chat_lines_waiting_buffer);
        gui_chat_lines_waiting_buffer = NULL;
    }
}

/*
 * Quotes a line.
 */

int
gui_chat_hsignal_quote_line_cb (const void *pointer, void *data,
                                const char *signal,
                                struct t_hashtable *hashtable)
{
    const char *date, *line, *prefix, *ptr_prefix, *message;
    long unsigned int value;
    long number;
    struct tm *local_time;
    struct t_gui_line *ptr_line;
    int is_nick, length_time, length_nick_prefix, length_prefix;
    int length_nick_suffix, length_message, length, rc;
    time_t line_date;
    char str_time[128], *str, *error;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!gui_current_window->buffer->input)
        return WEECHAT_RC_OK;

    /* get time */
    str_time[0] = '\0';
    date = (strstr (signal, "time")) ?
        hashtable_get (hashtable, "_chat_line_date") : NULL;
    if (date)
    {
        number = strtol (date, &error, 10);
        if (error && !error[0])
        {
            line_date = (time_t)number;
            local_time = localtime (&line_date);
            if (local_time)
            {
                if (strftime (str_time, sizeof (str_time),
                              CONFIG_STRING(config_look_quote_time_format),
                              local_time) == 0)
                    str_time[0] = '\0';
            }
        }
    }

    /* check if the prefix is a nick */
    is_nick = 0;
    line = hashtable_get (hashtable, "_chat_line");
    if (line && line[0])
    {
        rc = sscanf (line, "%lx", &value);
        if ((rc != EOF) && (rc != 0))
        {
            ptr_line = (struct t_gui_line *)value;
            if (gui_line_search_tag_starting_with (ptr_line, "prefix_nick"))
                is_nick = 1;
        }
    }

    /* get prefix + message */
    prefix = (strstr (signal, "prefix")) ?
        hashtable_get (hashtable, "_chat_line_prefix") : NULL;
    ptr_prefix = prefix;
    if (ptr_prefix)
    {
        while (ptr_prefix[0] == ' ')
        {
            ptr_prefix++;
        }
    }
    message = hashtable_get (hashtable, "_chat_line_message");

    if (!message)
        return WEECHAT_RC_OK;

    length_time = strlen (str_time);
    length_nick_prefix = strlen (CONFIG_STRING(config_look_quote_nick_prefix));
    length_prefix = (ptr_prefix) ? strlen (ptr_prefix) : 0;
    length_nick_suffix = strlen (CONFIG_STRING(config_look_quote_nick_suffix));
    length_message = strlen (message);

    length = length_time + 1 +
        length_nick_prefix + length_prefix + length_nick_suffix + 1 +
        length_message + 1 + 1;
    str = malloc (length);
    if (str)
    {
        snprintf (str, length, "%s%s%s%s%s%s%s ",
                  str_time,
                  (str_time[0]) ? " " : "",
                  (ptr_prefix && ptr_prefix[0] && is_nick) ? CONFIG_STRING(config_look_quote_nick_prefix) : "",
                  (ptr_prefix) ? ptr_prefix : "",
                  (ptr_prefix && ptr_prefix[0] && is_nick) ? CONFIG_STRING(config_look_quote_nick_suffix) : "",
                  (ptr_prefix && ptr_prefix[0]) ? " " : "",
                  message);
        gui_input_insert_string (gui_current_window->buffer, str, -1);
        gui_input_text_changed_modifier_and_signal (gui_current_window->buffer,
                                                    1, /* save undo */
                                                    1); /* stop completion */
        free (str);
    }

    return WEECHAT_RC_OK;
}

/*
 * Frees some variables allocated for chat area.
 */

void
gui_chat_end ()
{
    int i;

    /* free prefixes */
    for (i = 0; i < GUI_CHAT_NUM_PREFIXES; i++)
    {
        if (gui_chat_prefix[i])
        {
            free (gui_chat_prefix[i]);
            gui_chat_prefix[i] = NULL;
        }
    }

    /* free lines waiting for buffer (should always be NULL here) */
    if (gui_chat_lines_waiting_buffer)
    {
        free (gui_chat_lines_waiting_buffer);
        gui_chat_lines_waiting_buffer = NULL;
    }
}
