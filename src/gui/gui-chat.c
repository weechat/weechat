/*
 * gui-chat.c - chat functions (used by all GUI)
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <regex.h>

#include "../core/weechat.h"
#include "../core/core-config.h"
#include "../core/core-eval.h"
#include "../core/core-hashtable.h"
#include "../core/core-hook.h"
#include "../core/core-input.h"
#include "../core/core-string.h"
#include "../core/core-utf8.h"
#include "../core/core-util.h"
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
char **gui_chat_lines_waiting_buffer = NULL;    /* lines waiting for core   */
                                                /* buffer                   */
int gui_chat_whitespace_mode = 0;               /* make whitespaces visible */

/* command /pipe */
int gui_chat_pipe = 0;                          /* pipe enabled             */
char *gui_chat_pipe_command = NULL;             /* piped command            */
struct t_gui_buffer *gui_chat_pipe_buffer = NULL;  /* pipe msgs to a buffer */
int gui_chat_pipe_send_to_buffer = 0;           /* send as input to buffer  */
FILE *gui_chat_pipe_file = NULL;                /* pipe msgs to a file      */
char *gui_chat_pipe_hsignal = NULL;             /* pipe msgs to a hsignal   */
char *gui_chat_pipe_concat_sep = NULL;          /* separator to concat lines*/
char **gui_chat_pipe_concat_lines = NULL;       /* concatenated lines       */
char **gui_chat_pipe_concat_tags = NULL;        /* concatenated tags        */
char *gui_chat_pipe_strip_chars = NULL;         /* chars to strip on lines  */
int gui_chat_pipe_skip_empty_lines = 0;         /* skip empty lines         */
                                                /* colors in pipe output    */
enum t_gui_chat_pipe_color gui_chat_pipe_color = GUI_CHAT_PIPE_COLOR_STRIP;
char *gui_chat_pipe_color_string[GUI_CHAT_PIPE_NUM_COLORS] =
{ "strip", "keep", "ansi" };


/*
 * Initializes some variables for chat area (called before reading WeeChat
 * configuration file).
 */

void
gui_chat_init (void)
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
    hook_hsignal (NULL,
                  "chat_quote_time_prefix_message;chat_quote_prefix_message;"
                  "chat_quote_message;chat_quote_focused_line",
                  &gui_chat_hsignal_quote_line_cb, NULL, NULL);
}

/*
 * Builds prefix with colors (called after reading WeeChat configuration file).
 */

void
gui_chat_prefix_build (void)
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
 * Returns number of char in a string (special chars like colors/attributes are
 * ignored).
 */

int
gui_chat_strlen (const char *string)
{
    int length;

    length = 0;
    while (string && string[0])
    {
        string = gui_chat_string_next_char (NULL, NULL,
                                            (unsigned char *)string, 0, 0, 0);
        if (string)
        {
            length++;
            string = utf8_next_char (string);
        }
    }
    return length;
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
            size_on_screen = utf8_char_size_screen (string);
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
            size_on_screen = utf8_char_size_screen (string);
            if (size_on_screen > 0)
            {
                offset_screen -= size_on_screen;
                if (offset_screen < 0)
                    return string;
            }
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
            size_on_screen = utf8_char_size_screen (ptr_string);
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
 * Stops before the first newline character, even if no characters or only spaces and color codes precede it.
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
                if (next_char[0] == '\n')
                {
                    *word_end_offset = next_char - start_data;
                    if (*word_length < 0)
                        *word_length = 0;
                    return;
                }
                else if (next_char[0] != ' ')
                {
                    if (leading_spaces)
                        *word_start_offset = next_char - start_data;
                    leading_spaces = 0;
                    *word_end_offset = next_char2 - start_data;
                    char_size_screen = utf8_char_size_screen (next_char);
                    if (char_size_screen > 0)
                        (*word_length_with_spaces) += char_size_screen;
                    if (*word_length < 0)
                        *word_length = 0;
                    if (char_size_screen > 0)
                        (*word_length) += char_size_screen;
                }
                else
                {
                    if (leading_spaces)
                    {
                        (*word_length_with_spaces)++;
                        *word_end_offset = next_char2 - start_data;
                    }
                    else
                    {
                        *word_end_offset = next_char - start_data;
                        return;
                    }
                }
                data = next_char2;
            }
        }
        else
        {
            *word_end_offset = data + strlen (data) - start_data;
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
gui_chat_get_time_string (time_t date, int date_usec, int highlight)
{
    char text_time[128], text_time2[(128*3)+16], text_time_char[2];
    char *text_with_color;
    int i, time_first_digit, time_last_digit, last_color;
    struct timeval tv;
    struct t_hashtable *extra_vars;

    if (date == 0)
        return NULL;

    if (!CONFIG_STRING(config_look_buffer_time_format)
        || !CONFIG_STRING(config_look_buffer_time_format)[0])
        return NULL;

    tv.tv_sec = date;
    tv.tv_usec = date_usec;
    if (util_strftimeval (text_time, sizeof (text_time),
                          CONFIG_STRING(config_look_buffer_time_format),
                          &tv) == 0)
        return NULL;

    if (strstr (text_time, "${"))
    {
        extra_vars = hashtable_new (32,
                                    WEECHAT_HASHTABLE_STRING,
                                    WEECHAT_HASHTABLE_STRING,
                                    NULL, NULL);
        if (extra_vars)
            hashtable_set (extra_vars, "highlight", (highlight) ? "1" : "0");
        text_with_color = eval_expression (text_time, NULL, extra_vars, NULL);
        hashtable_free (extra_vars);
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
gui_chat_get_time_length (void)
{
    struct timeval tv_now;
    char *text_time;
    int length;

    if (!CONFIG_STRING(config_look_buffer_time_format)
        || !CONFIG_STRING(config_look_buffer_time_format)[0])
        return 0;

    length = 0;
    gettimeofday (&tv_now, NULL);
    text_time = gui_chat_get_time_string (tv_now.tv_sec, tv_now.tv_usec, 0);

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
gui_chat_change_time_format (void)
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
                free (ptr_line->data->str_time);
                ptr_line->data->str_time = gui_chat_get_time_string (
                    ptr_line->data->date,
                    ptr_line->data->date_usec,
                    ptr_line->data->highlight);
            }
        }
    }
}

/*
 * Checks if the buffer pointer is valid for printing a chat message,
 * according to buffer type.
 *
 * Returns:
 *   1: buffer is valid
 *   0: buffer is not valid
 */

int
gui_chat_buffer_valid (struct t_gui_buffer *buffer,
                       int buffer_type)
{
    /* check buffer pointer, closing and type */
    if (!buffer || !gui_buffer_valid (buffer)
        || buffer->closing
        || ((int)(buffer->type) != buffer_type))
    {
        return 0;
    }

    /* check if mute is enabled */
    if ((buffer_type == GUI_BUFFER_TYPE_FORMATTED)
        && ((gui_chat_mute == GUI_CHAT_MUTE_ALL_BUFFERS)
            || ((gui_chat_mute == GUI_CHAT_MUTE_BUFFER)
                && (gui_chat_mute_buffer == buffer))))
    {
        return 0;
    }

    return 1;
}

/*
 * Searches for a pipe color name.
 *
 * Returns index of color in enum t_gui_chat_pipe_color, -1 if not found.
 */

int
gui_chat_pipe_search_color (const char *color)
{
    int i;

    if (!color)
        return -1;

    for (i = 0; i < GUI_CHAT_PIPE_NUM_COLORS; i++)
    {
        if (strcmp (gui_chat_pipe_color_string[i], color) == 0)
            return i;
    }

    /* color not found */
    return -1;
}

/*
 * Converts color in a message, according to variable gui_chat_pipe_color.
 *
 * Note: result must be freed after use.
 */

char *
gui_chat_pipe_convert_color (const char *data)
{
    if (!data)
        return NULL;

    switch (gui_chat_pipe_color)
    {
        case GUI_CHAT_PIPE_COLOR_STRIP:
            return gui_color_decode (data, NULL);
        case GUI_CHAT_PIPE_COLOR_KEEP:
            return strdup (data);
        case GUI_CHAT_PIPE_COLOR_ANSI:
            return gui_color_encode_ansi (data);
        case GUI_CHAT_PIPE_NUM_COLORS:
            break;
    }

    return strdup (data);
}

/*
 * Builds a message with a line: "<prefix> <message>" or "<message>" if there
 * is no prefix.
 *
 * Note: result must be freed after use.
 */

char *
gui_chat_pipe_build_message (struct t_gui_line *line)
{
    char *prefix, *message, *data;

    if (!line)
        return NULL;

    prefix = gui_chat_pipe_convert_color (line->data->prefix);
    message = gui_chat_pipe_convert_color (line->data->message);

    string_asprintf (&data,
                     "%s%s%s",
                     (prefix) ? prefix : "",
                     (prefix && prefix[0] && message && message[0]) ? " " : "",
                     (message) ? message : "");

    free (prefix);
    free (message);

    return data;
}

/*
 * Sends data to a buffer (command `/pipe -o`).
 */

void
gui_chat_pipe_send_buffer_input (struct t_gui_buffer *buffer, const char *data)
{
    struct t_gui_buffer *buffer_saved;
    int send_to_buffer_saved;
    char *data_color;

    if (!buffer || !data)
        return;

    data_color = gui_chat_pipe_convert_color (data);
    if (!data_color)
        return;

    buffer_saved = gui_chat_pipe_buffer;
    send_to_buffer_saved = gui_chat_pipe_send_to_buffer;

    /* temporarily disable the pipe redirection, to prevent infinite loop */
    gui_chat_pipe_buffer = NULL;
    gui_chat_pipe_send_to_buffer = 0;

    input_data (
        buffer,
        (data_color[0]) ? data_color : " ",
        "-",  /* commands_allowed */
        0,  /* split_newline */
        0);  /* user_data */

    /* restore pipe redirection */
    gui_chat_pipe_buffer = buffer_saved;
    gui_chat_pipe_send_to_buffer = send_to_buffer_saved;

    free (data_color);
}

/*
 * Handles a redirection with /pipe command.
 *
 * Returns:
 *   1: line has been handled and must NOT be displayed
 *   0: line has NOT been handled and must be displayed
 */

int
gui_chat_pipe_handle_line (struct t_gui_line *line)
{
    char *data, *data2, *tags;
    int rc;

    if (!line || !gui_chat_pipe)
        return 0;

    rc = 0;

    data = gui_chat_pipe_build_message (line);
    if (!data)
        return 1;

    if (gui_chat_pipe_concat_lines)
    {
        /*
         * concatenate line with previous ones, it will be displayed, sent to
         * buffer input or written to a file later
         */
        data2 = (gui_chat_pipe_strip_chars) ?
            string_strip (data, 1, 1, gui_chat_pipe_strip_chars) : strdup (data);
        if (data2 && (data2[0] || !gui_chat_pipe_skip_empty_lines))
        {
            if ((*gui_chat_pipe_concat_lines)[0])
            {
                string_dyn_concat (gui_chat_pipe_concat_lines,
                                   gui_chat_pipe_concat_sep,
                                   -1);
            }
            string_dyn_concat (gui_chat_pipe_concat_lines, data2, -1);
        }
        free (data2);
        /* concatenate tags */
        if (gui_chat_pipe_concat_tags)
        {
            tags = string_rebuild_split_string (
                (const char **)line->data->tags_array,
                ",", 0, -1);
            if ((*gui_chat_pipe_concat_tags)[0])
                string_dyn_concat (gui_chat_pipe_concat_tags, "\n", -1);
            string_dyn_concat (gui_chat_pipe_concat_tags, (tags) ? tags : "", -1);
            free (tags);
        }
        rc = 1;
    }
    else if (gui_chat_pipe_file)
    {
        /* pipe to a file */
        fprintf (gui_chat_pipe_file, "%s\n", data);
        rc = 1;
    }
    else if (gui_chat_pipe_buffer && gui_chat_pipe_send_to_buffer)
    {
        /* pipe to a buffer as input */
        gui_chat_pipe_send_buffer_input (gui_chat_pipe_buffer, data);
        rc = 1;
    }

    free (data);

    return rc;
}

/*
 * Ends pipe and flushes concatenated lines to a buffer or a file
 * (if command `/pipe -g` was used).
 */

void
gui_chat_pipe_end (void)
{
    struct t_gui_buffer *pipe_buffer;
    struct t_hashtable *hashtable;
    int pipe_send_to_buffer;
    FILE *pipe_file;

    pipe_buffer = gui_chat_pipe_buffer;
    pipe_send_to_buffer = gui_chat_pipe_send_to_buffer;
    pipe_file = gui_chat_pipe_file;

    gui_chat_pipe = 0;
    gui_chat_pipe_buffer = NULL;
    gui_chat_pipe_send_to_buffer = 0;
    gui_chat_pipe_file = NULL;
    free (gui_chat_pipe_concat_sep);
    gui_chat_pipe_concat_sep = NULL;
    free (gui_chat_pipe_strip_chars);
    gui_chat_pipe_strip_chars = NULL;
    gui_chat_pipe_skip_empty_lines = 0;

    if (gui_chat_pipe_concat_lines)
    {
        if (pipe_file)
        {
            fprintf (pipe_file, "%s\n", *gui_chat_pipe_concat_lines);
        }
        else if (pipe_buffer)
        {
            if (pipe_send_to_buffer)
            {
                gui_chat_pipe_send_buffer_input (
                    pipe_buffer,
                    *gui_chat_pipe_concat_lines);
            }
            else
            {
                gui_chat_printf (pipe_buffer,
                                 "%s", *gui_chat_pipe_concat_lines);
            }
        }
        else if (gui_chat_pipe_hsignal)
        {
            hashtable = hashtable_new (32,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL, NULL);
            if (hashtable)
            {
                hashtable_set (hashtable, "command", gui_chat_pipe_command);
                hashtable_set (hashtable, "output", *gui_chat_pipe_concat_lines);
                if (gui_chat_pipe_concat_tags)
                    hashtable_set (hashtable, "tags", *gui_chat_pipe_concat_tags);
                hook_hsignal_send (gui_chat_pipe_hsignal, hashtable);
                hashtable_free (hashtable);
            }
        }
        string_dyn_free (gui_chat_pipe_concat_lines, 1);
        gui_chat_pipe_concat_lines = NULL;
    }

    if (pipe_file)
        fclose (pipe_file);
    free (gui_chat_pipe_command);
    gui_chat_pipe_command = NULL;
    free (gui_chat_pipe_hsignal);
    gui_chat_pipe_hsignal = NULL;
    if (gui_chat_pipe_concat_tags)
    {
        string_dyn_free (gui_chat_pipe_concat_tags, 1);
        gui_chat_pipe_concat_tags = NULL;
    }
    gui_chat_pipe_color = GUI_CHAT_PIPE_COLOR_STRIP;
}

/*
 * Displays a message in a buffer with optional date and tags.
 * This function is called internally by the function
 * gui_chat_printf_date_tags.
 */

void
gui_chat_printf_datetime_tags_internal (struct t_gui_buffer *buffer,
                                        time_t date,
                                        int date_usec,
                                        time_t date_printed,
                                        int date_usec_printed,
                                        const char *tags,
                                        char *message)
{
    int display_time;
    char *ptr_msg, *pos_prefix, *pos_tab;
    char *modifier_data, *string, *new_string, *pos_newline;
    char *prefix_color, *message_color;
    struct t_gui_line *new_line;

    if (!buffer)
        return;

    new_line = NULL;
    string = NULL;
    modifier_data = NULL;
    new_string = NULL;

    display_time = 1;

    pos_prefix = NULL;
    ptr_msg = message;

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

    new_line = gui_line_new (buffer,
                             -1,
                             (display_time) ? date : 0,
                             (display_time) ? date_usec : 0,
                             date_printed,
                             date_usec_printed,
                             tags,
                             pos_prefix,
                             ptr_msg);
    if (!new_line)
        goto no_print;

    hook_line_exec (new_line);

    if (!new_line->data->buffer)
        goto no_print;

    /* call modifier for message printed ("weechat_print") */
    string_asprintf (&modifier_data,
                     "0x%lx;%s",
                     (unsigned long)buffer,
                     (tags) ? tags : "");
    if (display_time)
    {
        string_asprintf (
            &string,
            "%s\t%s",
            (new_line->data->prefix && new_line->data->prefix[0]) ?
            new_line->data->prefix : " ",
            (new_line->data->message) ? new_line->data->message : "");
    }
    else
    {
        string_asprintf (
            &string,
            "\t\t%s",
            (new_line->data->message) ? new_line->data->message : "");
    }
    new_string = hook_modifier_exec (NULL,
                                     "weechat_print",
                                     modifier_data,
                                     string);
    if (new_string)
    {
        if (!new_string[0] && string[0])
        {
            /*
             * modifier returned empty message, then we'll not
             * print anything
             */
            goto no_print;
        }
        else if (strcmp (string, new_string) != 0)
        {
            if (!buffer->input_multiline)
            {
                /* if input_multiline is not set, keep only first line */
                pos_newline = strchr (new_string, '\n');
                if (pos_newline)
                    pos_newline[0] = '\0';
            }

            /* use new message if there are changes */
            display_time = 1;
            pos_prefix = NULL;
            ptr_msg = new_string;
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
                    new_line->data->date = 0;
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
            if ((new_line->data->date == 0) && display_time)
            {
                new_line->data->date = new_line->data->date_printed;
                new_line->data->date_usec = new_line->data->date_usec_printed;
            }
            string_shared_free (new_line->data->prefix);
            if (pos_prefix)
            {
                new_line->data->prefix = (char *)string_shared_get (pos_prefix);
            }
            else
            {
                new_line->data->prefix = (new_line->data->date != 0) ?
                    (char *)string_shared_get ("") : NULL;
            }
            new_line->data->prefix_length = gui_chat_strlen_screen (
                new_line->data->prefix);
            free (new_line->data->message);
            new_line->data->message = strdup (ptr_msg);
        }
    }

    if (gui_chat_pipe_handle_line (new_line))
    {
        /* line was handled with /pipe command, do NOT display it */
        goto no_print;
    }
    else if (gui_chat_pipe_buffer)
    {
        prefix_color = gui_chat_pipe_convert_color (new_line->data->prefix);
        string_shared_free (new_line->data->prefix);
        new_line->data->prefix = (prefix_color) ?
            (char *)string_shared_get (prefix_color) : NULL;
        new_line->data->prefix_length = (prefix_color) ?
            gui_chat_strlen_screen (prefix_color) : 0;

        message_color = gui_chat_pipe_convert_color (new_line->data->message);
        free (new_line->data->message);
        new_line->data->message = message_color;
    }

    /* add line in the buffer */
    gui_line_add (new_line);

    /* run hook_print for the new line */
    if (new_line->data->buffer && new_line->data->buffer->print_hooks_enabled)
        hook_print_exec (new_line->data->buffer, new_line);

    gui_buffer_ask_chat_refresh (new_line->data->buffer, 1);

    free (string);
    free (modifier_data);
    free (new_string);

    return;

no_print:
    if (new_line)
    {
        gui_line_free_data (new_line);
        free (new_line);
    }
    free (string);
    free (modifier_data);
    free (new_string);
}

/*
 * Adds a line when WeeChat is waiting for the core buffer.
 *
 * The line is stored in an internal buffer area and will be displayed later
 * in the core buffer.
 */

void
gui_chat_add_line_waiting_buffer (const char *message)
{
    if (!gui_chat_lines_waiting_buffer)
    {
        gui_chat_lines_waiting_buffer = string_dyn_alloc (1024);
        if (!gui_chat_lines_waiting_buffer)
            return;
    }

    if ((*gui_chat_lines_waiting_buffer)[0])
        string_dyn_concat (gui_chat_lines_waiting_buffer, "\n", -1);

    string_dyn_concat (gui_chat_lines_waiting_buffer, message, -1);
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
        lines = string_split (*gui_chat_lines_waiting_buffer, "\n", NULL,
                              0, 0, &num_lines);
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
        string_dyn_free (gui_chat_lines_waiting_buffer, 1);
        gui_chat_lines_waiting_buffer = NULL;
    }
}

/*
 * Displays a message in a buffer with optional date and tags.
 *
 * Note: this function works only with formatted buffers (not buffers with free
 * content).
 */

void
gui_chat_printf_datetime_tags (struct t_gui_buffer *buffer,
                               time_t date, int date_usec,
                               const char *tags, const char *message, ...)
{
    struct timeval tv_date_printed;
    char *pos, *pos_end;
    int one_line;

    if (!message)
        return;

    if (gui_init_ok)
    {
        if (gui_chat_pipe_buffer)
            buffer = gui_chat_pipe_buffer;
        if (!buffer)
            buffer = gui_buffer_search_main ();
        if (!gui_chat_buffer_valid (buffer, GUI_BUFFER_TYPE_FORMATTED))
            return;
    }

    weechat_va_format (message);
    if (!vbuffer)
        return;

    utf8_normalize (vbuffer, '?');

    gettimeofday (&tv_date_printed, NULL);
    if (date <= 0)
    {
        date = tv_date_printed.tv_sec;
        date_usec = tv_date_printed.tv_usec;
    }

    one_line = 0;
    pos = vbuffer;
    while (pos)
    {
        pos_end = NULL;
        if (!buffer || !buffer->input_multiline)
        {
            /* display until next end of line */
            pos_end = strchr (pos, '\n');
            if (pos_end)
                pos_end[0] = '\0';
        }
        else
        {
            one_line = 1;
        }

        if (gui_init_ok)
        {
            gui_chat_printf_datetime_tags_internal (buffer,
                                                    date,
                                                    date_usec,
                                                    tv_date_printed.tv_sec,
                                                    tv_date_printed.tv_usec,
                                                    tags,
                                                    pos);
        }
        else
        {
            gui_chat_add_line_waiting_buffer (pos);
        }

        if (one_line)
            break;

        pos = (pos_end && pos_end[1]) ? pos_end + 1 : NULL;
    }

    free (vbuffer);
}

/*
 * Displays a message on a line in a buffer with free content.
 *
 * Note: this function works only with free content buffers (not formatted
 * buffers).
 */

void
gui_chat_printf_y_datetime_tags (struct t_gui_buffer *buffer, int y,
                                 time_t date, int date_usec,
                                 const char *tags, const char *message, ...)
{
    struct t_gui_line *ptr_line, *new_line, *new_line_empty;
    struct timeval tv_date_printed;
    int i, last_y, num_lines_to_add;

    if (!message)
        return;

    if (gui_init_ok && !gui_chat_buffer_valid (buffer, GUI_BUFFER_TYPE_FREE))
        return;

    /* if y is negative, add a line -N lines after the last line */
    if (y < 0)
    {
        y = (buffer->own_lines && buffer->own_lines->last_line) ?
            buffer->own_lines->last_line->data->y - y : (-1 * y) - 1;
    }

    weechat_va_format (message);
    if (!vbuffer)
        return;

    utf8_normalize (vbuffer, '?');

    gettimeofday (&tv_date_printed, NULL);
    if (date <= 0)
    {
        date = tv_date_printed.tv_sec;
        date_usec = tv_date_printed.tv_usec;
    }

    new_line = gui_line_new (buffer,
                             y,
                             date,
                             date_usec,
                             tv_date_printed.tv_sec,
                             tv_date_printed.tv_usec,
                             tags,
                             NULL,
                             vbuffer);
    if (!new_line)
        goto end;

    hook_line_exec (new_line);

    if (!new_line->data->buffer)
    {
        gui_line_free_data (new_line);
        free (new_line);
        goto end;
    }

    if (new_line->data->message && new_line->data->message[0])
    {
        if (gui_init_ok)
        {
            /* compute the number of lines to add before y */
            if (new_line->data->buffer->own_lines
                && new_line->data->buffer->own_lines->last_line)
            {
                num_lines_to_add = y - new_line->data->buffer->own_lines->last_line->data->y - 1;
            }
            else
            {
                num_lines_to_add = y;
            }
            if (num_lines_to_add > 0)
            {
                /*
                 * add empty line(s) before asked line, to ensure there is at
                 * least "y" lines in buffer, and then be able to scroll
                 * properly buffer page by page
                 */
                for (i = y - num_lines_to_add; i < y; i++)
                {
                    new_line_empty = gui_line_new (new_line->data->buffer,
                                                   i, 0, 0, 0, 0, NULL, NULL,
                                                   "");
                    if (new_line_empty)
                        gui_line_add_y (new_line_empty);
                }
            }
            gui_line_add_y (new_line);
        }
        else
        {
            string_fprintf (stdout, "%s\n", new_line->data->message);
            gui_line_free_data (new_line);
            free (new_line);
        }
    }
    else
    {
        if (gui_init_ok)
        {
            /* delete line */
            last_y = (new_line->data->buffer->own_lines->last_line) ?
                new_line->data->buffer->own_lines->last_line->data->y : 0;
            if (y <= last_y)
            {
                for (ptr_line = new_line->data->buffer->own_lines->first_line;
                     ptr_line; ptr_line = ptr_line->next_line)
                {
                    if (ptr_line->data->y >= y)
                        break;
                }
                if (ptr_line && (ptr_line->data->y == y))
                {
                    if (ptr_line->next_line)
                        gui_line_clear (ptr_line);
                    else
                        gui_line_free (new_line->data->buffer, ptr_line);
                    gui_buffer_ask_chat_refresh (new_line->data->buffer, 2);
                }
            }
        }
        gui_line_free_data (new_line);
        free (new_line);
    }

end:
    free (vbuffer);
}

/*
 * Quotes a line.
 */

int
gui_chat_hsignal_quote_line_cb (const void *pointer, void *data,
                                const char *signal,
                                struct t_hashtable *hashtable)
{
    const char *ptr_date, *ptr_date_usec, *line, *prefix, *ptr_prefix, *message;
    long long number;
    struct timeval tv;
    struct t_gui_line *ptr_line;
    int is_nick, rc;
    char str_time[128], *str, *error;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!gui_current_window->buffer->input)
        return WEECHAT_RC_OK;

    /* get time */
    str_time[0] = '\0';
    ptr_date = (strstr (signal, "time")) ?
        hashtable_get (hashtable, "_chat_line_date") : NULL;
    if (ptr_date)
    {
        error = NULL;
        number = strtoll (ptr_date, &error, 10);
        if (error && !error[0])
        {
            tv.tv_sec = (time_t)number;
            tv.tv_usec = 0;
            ptr_date_usec = (strstr (signal, "time")) ?
                hashtable_get (hashtable, "_chat_line_date_usec") : NULL;
            if (ptr_date_usec)
            {
                error = NULL;
                number = strtoll (ptr_date_usec, &error, 10);
                if (error && !error[0])
                    tv.tv_usec = (long)number;
            }
            util_strftimeval (str_time, sizeof (str_time),
                              CONFIG_STRING(config_look_quote_time_format),
                              &tv);
        }
    }

    /* check if the prefix is a nick */
    is_nick = 0;
    line = hashtable_get (hashtable, "_chat_line");
    if (line && line[0])
    {
        rc = sscanf (line, "%p", &ptr_line);
        if ((rc != EOF) && (rc != 0))
        {
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

    message = (strstr (signal, "focused_line")) ?
        hashtable_get (hashtable, "_chat_focused_line") : hashtable_get (hashtable, "_chat_line_message");

    if (!message)
        return WEECHAT_RC_OK;

    if (string_asprintf (
            &str,
            "%s%s%s%s%s%s%s ",
            str_time,
            (str_time[0]) ? " " : "",
            (ptr_prefix && ptr_prefix[0] && is_nick) ? CONFIG_STRING(config_look_quote_nick_prefix) : "",
            (ptr_prefix) ? ptr_prefix : "",
            (ptr_prefix && ptr_prefix[0] && is_nick) ? CONFIG_STRING(config_look_quote_nick_suffix) : "",
            (ptr_prefix && ptr_prefix[0]) ? " " : "",
            message) >= 0)
    {
        gui_input_insert_string (gui_current_window->buffer, str);
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
gui_chat_end (void)
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
        string_dyn_free (gui_chat_lines_waiting_buffer, 1);
        gui_chat_lines_waiting_buffer = NULL;
    }
}
