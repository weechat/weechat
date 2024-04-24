/*
 * logger-backlog.c - display backlog of messages
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

/* this define is needed for strptime() (not on OpenBSD/Sun) */
#if !defined(__OpenBSD__) && !defined(__sun)
#define _XOPEN_SOURCE 700
#endif

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-buffer.h"
#include "logger-command.h"
#include "logger-config.h"
#include "logger-info.h"
#include "logger-tail.h"


/*
 * Displays a line read from log file.
 */

void
logger_backlog_display_line (struct t_gui_buffer *buffer, const char *line)
{
    const char *pos_message;
    char *str_date, *charset, *pos_tab, *error, *message, *message2;
    time_t datetime, time_now;
    struct tm tm_line;
    int color_lines;

    if (!line)
        return;

    color_lines = weechat_config_boolean (logger_config_file_color_lines);

    datetime = 0;
    pos_message = strchr (line, '\t');
    if (pos_message)
    {
        /* initialize structure, because strptime does not do it */
        memset (&tm_line, 0, sizeof (struct tm));
        /*
         * we get current time to initialize daylight saving time in
         * structure tm_line, otherwise printed time will be shifted
         * and will not use DST used on machine
         */
        time_now = time (NULL);
        localtime_r (&time_now, &tm_line);
        str_date = weechat_strndup (line, pos_message - line);
        if (str_date)
        {
            error = strptime (
                str_date,
                weechat_config_string (logger_config_file_time_format),
                &tm_line);
            if (error && !error[0] && (tm_line.tm_year > 0))
                datetime = mktime (&tm_line);
            free (str_date);
        }
    }
    pos_message = (pos_message && (datetime != 0)) ?
        pos_message + 1 : line;
    message = weechat_hook_modifier_exec (
        "color_decode_ansi",
        (color_lines) ? "1" : "0",
        pos_message);
    if (message)
    {
        charset = weechat_info_get ("charset_terminal", "");
        message2 = (charset) ?
            weechat_iconv_to_internal (charset, message) : strdup (message);
        free (charset);
        if (message2)
        {
            pos_tab = strchr (message2, '\t');
            if (pos_tab)
                pos_tab[0] = '\0';
            weechat_printf_date_tags (
                buffer, datetime,
                "no_highlight,notify_none,logger_backlog",
                "%s%s%s%s%s",
                (color_lines) ? "" : weechat_color (weechat_config_string (logger_config_color_backlog_line)),
                message2,
                (pos_tab) ? "\t" : "",
                (pos_tab && !color_lines) ? weechat_color (weechat_config_string (logger_config_color_backlog_line)) : "",
                (pos_tab) ? pos_tab + 1 : "");
            if (pos_tab)
                pos_tab[0] = '\t';
            free (message2);
        }
        free (message);
    }
}

/*
 * Compares two messages in arraylist.
 */

int
logger_backlog_msg_cmp_cb (void *data,
                           struct t_arraylist *arraylist,
                           void *pointer1,
                           void *pointer2)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    return weechat_strcmp ((const char *)pointer1, (const char *)pointer2);
}

/*
 * Frees a message in arraylist.
 */

void
logger_backlog_msg_free_cb (void *data, struct t_arraylist *arraylist,
                            void *pointer)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    free (pointer);
}

/*
 * Groups lines by messages: each line with a timestamp is considered the first
 * line of a message, and subsequent lines without timestamp are the rest of
 * the message.
 *
 * Note: result must be freed after use.
 */

struct t_arraylist *
logger_backlog_group_messages (struct t_arraylist *lines)
{
    int i, size, time_found;
    char *message, *new_message, *str_date, *error;
    const char *ptr_line, *pos_message;
    struct tm tm_line;
    struct t_arraylist *messages;

    if (!lines)
        return NULL;

    message = NULL;

    size = weechat_arraylist_size (lines);

    messages = weechat_arraylist_new (size, 0, 1,
                                      &logger_backlog_msg_cmp_cb, NULL,
                                      &logger_backlog_msg_free_cb, NULL);
    if (!messages)
        goto error;

    for (i = size - 1; i >= 0; i--)
    {
        ptr_line = (const char *)weechat_arraylist_get (lines, i);

        if (message)
        {
            new_message = malloc (strlen (ptr_line) + 1 + strlen (message) + 1);
            if (!new_message)
                goto error;
            strcpy (new_message, ptr_line);
            strcat (new_message, "\n");
            strcat (new_message, message);
            free (message);
            message = new_message;
        }
        else
        {
            message = malloc (strlen (ptr_line) + 1);
            if (!message)
                goto error;
            strcpy (message, ptr_line);
        }

        time_found = 0;
        pos_message = strchr (ptr_line, '\t');
        if (pos_message)
        {
            str_date = weechat_strndup (ptr_line, pos_message - ptr_line);
            if (str_date)
            {
                memset (&tm_line, 0, sizeof (struct tm));
                error = strptime (
                    str_date,
                    weechat_config_string (logger_config_file_time_format),
                    &tm_line);
                if (error && !error[0] && (tm_line.tm_year > 0))
                    time_found = 1;
                free (str_date);
            }
        }
        if (time_found)
        {
            /* add message (will be freed when arraylist is destroyed) */
            weechat_arraylist_insert (messages, 0, message);
            message = NULL;
        }
    }

    if (message)
    {
        /* add message (will be freed when arraylist is destroyed) */
        weechat_arraylist_insert (messages, 0, message);
    }

    return messages;

error:
    free (message);
    weechat_arraylist_free (messages);
    return NULL;
}

/*
 * Displays backlog for a buffer by reading end of log file.
 */

void
logger_backlog_file (struct t_gui_buffer *buffer, const char *filename,
                     int lines)
{
    struct t_arraylist *last_lines, *messages;
    int i, num_msgs, old_input_multiline;

    last_lines = logger_tail_file (filename, lines);
    if (!last_lines)
        return;

    messages = logger_backlog_group_messages (last_lines);
    if (!messages)
    {
        weechat_arraylist_free (last_lines);
        return;
    }

    weechat_arraylist_free (last_lines);

    /* disable any print hook during display of backlog */
    weechat_buffer_set (buffer, "print_hooks_enabled", "0");

    /* temporary enable multiline support so we can display multiline msgs */
    old_input_multiline = weechat_buffer_get_integer (buffer, "input_multiline");
    weechat_buffer_set (buffer, "input_multiline", "1");

    num_msgs = weechat_arraylist_size (messages);
    for (i = 0; i < num_msgs; i++)
    {
        logger_backlog_display_line (
            buffer,
            (const char *)weechat_arraylist_get (messages, i));
    }
    weechat_arraylist_free (messages);

    if (num_msgs > 0)
    {
        weechat_printf_date_tags (buffer, 0,
                                  "no_highlight,notify_none,logger_backlog_end",
                                  _("%s===\t%s========== End of backlog (%d lines) =========="),
                                  weechat_color (weechat_config_string (logger_config_color_backlog_end)),
                                  weechat_color (weechat_config_string (logger_config_color_backlog_end)),
                                  num_msgs);
        weechat_buffer_set (buffer, "unread", "");
    }

    weechat_buffer_set (buffer, "input_multiline",
                        (old_input_multiline) ? "1" : "0");
    weechat_buffer_set (buffer, "print_hooks_enabled", "1");
}

/*
 * Callback for signal "logger_backlog".
 */

int
logger_backlog_signal_cb (const void *pointer, void *data,
                          const char *signal,
                          const char *type_data, void *signal_data)
{
    struct t_logger_buffer *ptr_logger_buffer;
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (weechat_config_integer (logger_config_look_backlog) == 0)
        return WEECHAT_RC_OK;

    rc = logger_check_conditions (
        signal_data,
        weechat_config_string (logger_config_look_backlog_conditions));
    if (!rc)
        return WEECHAT_RC_OK;

    ptr_logger_buffer = logger_buffer_search_buffer (signal_data);
    if (ptr_logger_buffer && ptr_logger_buffer->log_enabled)
    {
        if (!ptr_logger_buffer->log_filename)
            logger_buffer_set_log_filename (ptr_logger_buffer);
        if (ptr_logger_buffer->log_filename)
        {
            ptr_logger_buffer->log_enabled = 0;
            logger_backlog_file (signal_data,
                                 ptr_logger_buffer->log_filename,
                                 weechat_config_integer (logger_config_look_backlog));
            ptr_logger_buffer->log_enabled = 1;
        }
    }

    return WEECHAT_RC_OK;
}
