/*
 * logger-backlog.c - display backlog of messages
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
 * Checks conditions to display the backlog.
 *
 * Returns:
 *   1: conditions OK (backlog is displayed)
 *   0: conditions not OK (backlog is NOT displayed)
 */

int
logger_backlog_check_conditions (struct t_gui_buffer *buffer)
{
    struct t_hashtable *pointers, *options;
    const char *ptr_condition;
    char *result;
    int condition_ok;

    ptr_condition = weechat_config_string (logger_config_look_backlog_conditions);

    /* empty condition displays the backlog everywhere */
    if (!ptr_condition || !ptr_condition[0])
        return 1;

    pointers = weechat_hashtable_new (32,
                                      WEECHAT_HASHTABLE_STRING,
                                      WEECHAT_HASHTABLE_POINTER,
                                      NULL,
                                      NULL);
    if (pointers)
    {
        weechat_hashtable_set (pointers, "window",
                               weechat_window_search_with_buffer (buffer));
        weechat_hashtable_set (pointers, "buffer", buffer);
    }

    options = weechat_hashtable_new (32,
                                     WEECHAT_HASHTABLE_STRING,
                                     WEECHAT_HASHTABLE_STRING,
                                     NULL,
                                     NULL);
    if (options)
        weechat_hashtable_set (options, "type", "condition");

    result = weechat_string_eval_expression (ptr_condition,
                                             pointers, NULL, options);
    condition_ok = (result && (strcmp (result, "1") == 0));
    if (result)
        free (result);

    if (pointers)
        weechat_hashtable_free (pointers);
    if (options)
        weechat_hashtable_free (options);

    return condition_ok;
}

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
        if (charset)
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
 * Displays backlog for a buffer by reading end of log file.
 */

void
logger_backlog_file (struct t_gui_buffer *buffer, const char *filename,
                     int lines)
{
    struct t_logger_line *last_lines, *ptr_lines;
    int num_lines;

    weechat_buffer_set (buffer, "print_hooks_enabled", "0");

    num_lines = 0;
    last_lines = logger_tail_file (filename, lines);
    ptr_lines = last_lines;
    while (ptr_lines)
    {
        logger_backlog_display_line (buffer, ptr_lines->data);
        num_lines++;
        ptr_lines = ptr_lines->next_line;
    }
    if (last_lines)
        logger_tail_free (last_lines);
    if (num_lines > 0)
    {
        weechat_printf_date_tags (buffer, 0,
                                  "no_highlight,notify_none,logger_backlog_end",
                                  _("%s===\t%s========== End of backlog (%d lines) =========="),
                                  weechat_color (weechat_config_string (logger_config_color_backlog_end)),
                                  weechat_color (weechat_config_string (logger_config_color_backlog_end)),
                                  num_lines);
        weechat_buffer_set (buffer, "unread", "");
    }
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

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (weechat_config_integer (logger_config_look_backlog) == 0)
        return WEECHAT_RC_OK;

    if (!logger_backlog_check_conditions (signal_data))
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
