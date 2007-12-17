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

/* logger.c: Logger plugin for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "logger.h"
#include "logger-buffer.h"
#include "logger-tail.h"


char plugin_name[] = "logger";
char plugin_version[]     = "0.1";
char plugin_description[] = "Logger plugin for WeeChat";

struct t_weechat_plugin *weechat_logger_plugin = NULL;

#define LOGGER_OPTION_PATH        "path"
#define LOGGER_OPTION_TIME_FORMAT "time_format"
#define LOGGER_OPTION_INFO_LINES  "info_lines"
#define LOGGER_OPTION_BACKLOG     "backlog"

char *logger_option_path = NULL;
char *logger_option_time_format = NULL;
int logger_option_info_lines;
int logger_option_backlog;


/*
 * logger_config_read: read config options for logger plugin
 *                     return: 1 if ok
 *                             0 if error
 */

int
logger_config_read ()
{
    long number;
    char *string, *error;
    
    logger_option_path = weechat_config_get_plugin (LOGGER_OPTION_PATH);
    if (!logger_option_path)
    {
        weechat_config_set_plugin (LOGGER_OPTION_PATH, "%h/logs/");
        logger_option_path = weechat_config_get_plugin ("path");
    }
    
    logger_option_time_format = weechat_config_get_plugin (LOGGER_OPTION_TIME_FORMAT);
    if (!logger_option_time_format)
    {
        weechat_config_set_plugin (LOGGER_OPTION_TIME_FORMAT,
                                   "%Y %b %d %H:%M:%S");
        logger_option_time_format = weechat_config_get_plugin (LOGGER_OPTION_TIME_FORMAT);
    }
    
    string = weechat_config_get_plugin (LOGGER_OPTION_INFO_LINES);
    if (!string)
    {
        weechat_config_set_plugin (LOGGER_OPTION_INFO_LINES, "off");
        string = weechat_config_get_plugin (LOGGER_OPTION_INFO_LINES);
    }
    if (string && (weechat_config_string_to_boolean (string) > 0))
        logger_option_info_lines = 1;
    else
        logger_option_info_lines = 0;
    
    string = weechat_config_get_plugin (LOGGER_OPTION_BACKLOG);
    if (!string)
    {
        weechat_config_set_plugin (LOGGER_OPTION_BACKLOG, "20");
        string = weechat_config_get_plugin (LOGGER_OPTION_BACKLOG);
    }
    logger_option_backlog = 20;
    if (string)
    {
        error = NULL;
        number = strtol (string, &error, 10);
        if (error && (error[0] == '\0'))
            logger_option_backlog = number;
    }
    if (logger_option_path && logger_option_time_format && logger_option_backlog)
        return 1;
    else
        return 0;
}

/*
 * logger_create_directory: create logger directory
 *                          return 1 if success, 0 if failed
 */

int
logger_create_directory ()
{
    int rc;
    char *dir1, *dir2, *weechat_dir;
    
    rc = 1;
    
    dir1 = weechat_string_replace (logger_option_path, "~", getenv ("HOME"));
    if (dir1)
    {
        weechat_dir = weechat_info_get ("weechat_dir");
        if (weechat_dir)
        {
            dir2 = weechat_string_replace (dir1, "%h", weechat_dir);
            if (dir2)
            {
                if (mkdir (dir2, 0755) < 0)
                {
                    if (errno != EEXIST)
                        rc = 0;
                }
                else
                    chmod (dir2, 0700);
                free (dir2);
            }
            else
                rc = 0;
            free (weechat_dir);
        }
        else
            rc = 0;
        free (dir1);
    }
    else
        rc = 0;
    
    return rc;
}

/*
 * logger_get_filename: build log filename for a buffer
 */

char *
logger_get_filename (struct t_gui_buffer *buffer)
{
    struct t_plugin_infolist *ptr_infolist;
    char *res;
    char *dir_separator, *weechat_dir, *log_path, *log_path2;
    char *category, *category2, *name, *name2;
    int length;
    
    res = NULL;
    
    dir_separator = weechat_info_get ("dir_separator");
    weechat_dir = weechat_info_get ("weechat_dir");
    log_path = weechat_string_replace (logger_option_path, "~",
                                       getenv ("HOME"));
    log_path2 = weechat_string_replace (log_path, "%h", weechat_dir);
    
    if (dir_separator && weechat_dir && log_path && log_path2)
    {
        ptr_infolist = weechat_infolist_get ("buffer", buffer);
        if (ptr_infolist)
        {
            category2 = NULL;
            name2 = NULL;
            if (weechat_infolist_next (ptr_infolist))
            {
                category = weechat_infolist_string (ptr_infolist, "category");
                category2 = (category) ?
                    weechat_string_replace (category, dir_separator, "_") : NULL;
                name = weechat_infolist_string (ptr_infolist, "name");
                name2 = (name) ?
                    weechat_string_replace (name, dir_separator, "_") : NULL;
            }
            length = strlen (log_path2);
            if (category2)
                length += strlen (category2);
            if (name2)
                length += strlen (name2);
            length += 16;
            res = (char *)malloc (length);
            if (res)
            {
                strcpy (res, log_path2);
                if (category2)
                {
                    strcat (res, category2);
                    strcat (res, ".");
                }
                if (name2)
                {
                    strcat (res, name2);
                    strcat (res, ".");
                }
                strcat (res, "weechatlog");
            }
            if (category2)
                free (category2);
            if (name2)
                free (name2);
            weechat_infolist_free (ptr_infolist);
        }
    }
    
    if (dir_separator)
        free (dir_separator);
    if (weechat_dir)
        free (weechat_dir);
    if (log_path)
        free (log_path);
    if (log_path2)
        free (log_path2);
    
    return res;
}

/*
 * logger_write_line: write a line to log file
 */

void
logger_write_line (struct t_logger_buffer *logger_buffer, char *format, ...)
{
    va_list argptr;
    char buf[4096], *charset, *message;
    time_t seconds;
    struct tm *date_tmp;
    char buf_time[256];
    
    if (logger_buffer->log_filename)
    {
        charset = weechat_info_get ("charset_terminal");
        
        if (!logger_buffer->log_file)
        {
            logger_buffer->log_file =
                fopen (logger_buffer->log_filename, "a");
            if (!logger_buffer->log_file)
            {
                weechat_printf (NULL,
                                _("%sLogger: unable to write log file \"%s\"\n"),
                                weechat_prefix ("error"),
                                logger_buffer->log_filename);
                free (logger_buffer->log_filename);
                logger_buffer->log_filename = NULL;
                if (charset)
                    free (charset);
                return;
            }

            if (logger_option_info_lines)
            {
                seconds = time (NULL);
                date_tmp = localtime (&seconds);
                buf_time[0] = '\0';
                if (date_tmp)
                    strftime (buf_time, sizeof (buf_time) - 1,
                              logger_option_time_format, date_tmp);
                snprintf (buf, sizeof (buf) - 1,
                          _("%s\t****  Beginning of log  ****"),
                          buf_time);
                message = (charset) ?
                    weechat_iconv_from_internal (charset, buf) : NULL;
                fprintf (logger_buffer->log_file,
                         "%s\n", (message) ? message : buf);
                if (message)
                    free (message);
            }
        }
        
        va_start (argptr, format);
        vsnprintf (buf, sizeof (buf) - 1, format, argptr);
        va_end (argptr);
        
        message = (charset) ?
            weechat_iconv_from_internal (charset, buf) : NULL;
        
        fprintf (logger_buffer->log_file,
                 "%s\n", (message) ? message : buf);
        fflush (logger_buffer->log_file);
        if (message)
            free (message);
        if (charset)
            free (charset);
    }
}

/*
 * logger_start_buffer: start a log for a buffer
 */

void
logger_start_buffer (struct t_gui_buffer *buffer)
{
    struct t_logger_buffer *ptr_logger_buffer;
    char *log_filename;
    
    if (!buffer)
        return;
    
    ptr_logger_buffer = logger_buffer_search (buffer);
    if (!ptr_logger_buffer)
    {
        log_filename = logger_get_filename (buffer);
        if (!log_filename)
            return;
        ptr_logger_buffer = logger_buffer_add (buffer, log_filename);
        free (log_filename);
    }
    if (ptr_logger_buffer)
    {
        if (ptr_logger_buffer->log_filename)
        {
            if (ptr_logger_buffer->log_file)
            {
                fclose (ptr_logger_buffer->log_file);
                ptr_logger_buffer->log_file = NULL;
            }
        }
    }
}

/*
 * logger_start_buffer_all: start log buffer for all buffers
 */

void
logger_start_buffer_all ()
{
    struct t_plugin_infolist *ptr_infolist;
    
    ptr_infolist = weechat_infolist_get ("buffer", NULL);
    while (weechat_infolist_next (ptr_infolist))
    {
        logger_start_buffer (weechat_infolist_pointer (ptr_infolist,
                                                       "pointer"));
    }
}

/*
 * logger_end: end log for a logger buffer
 */

void
logger_end (struct t_logger_buffer *logger_buffer)
{
    time_t seconds;
    struct tm *date_tmp;
    char buf_time[256];
    
    if (!logger_buffer)
        return;
    
    if (logger_buffer->log_file)
    {
        if (logger_option_info_lines)
        {
            seconds = time (NULL);
            date_tmp = localtime (&seconds);
            buf_time[0] = '\0';
            if (date_tmp)
                strftime (buf_time, sizeof (buf_time) - 1,
                          logger_option_time_format, date_tmp);
            logger_write_line (logger_buffer,
                               _("%s\t****  End of log  ****"),
                               buf_time);
        }
        fclose (logger_buffer->log_file);
        logger_buffer->log_file = NULL;
        logger_buffer_free (logger_buffer);
    }
}

/*
 * logger_end_all: end log for all buffers
 */

void
logger_end_all ()
{
    struct t_logger_buffer *ptr_logger_buffer;
    
    for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
         ptr_logger_buffer = ptr_logger_buffer->next_buffer)
    {
        logger_end (ptr_logger_buffer);
    }
}

/*
 * logger_buffer_open_signal_cb: callback for "buffer_open" signal
 */

int
logger_buffer_open_signal_cb (void *data, char *signal, void *pointer)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) pointer;
    
    logger_start_buffer (pointer);
    
    return WEECHAT_RC_OK;
}

/*
 * logger_buffer_close_signal_cb: callback for "buffer_close" signal
 */

int
logger_buffer_close_signal_cb (void *data, char *signal, void *pointer)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) pointer;
    
    logger_end (logger_buffer_search (pointer));
    
    return WEECHAT_RC_OK;
}

/*
 * logger_backlog: display backlog for a buffer (by reading end of log file)
 */

void
logger_backlog (struct t_gui_buffer *buffer, char *filename, int lines)
{
    struct t_logger_line *last_lines, *ptr_lines;
    char *pos_message, *error;
    time_t datetime;
    struct tm tm_line;
    int num_lines;
    
    num_lines = 0;
    last_lines = logger_tail_file (filename, lines);
    ptr_lines = last_lines;
    while (ptr_lines)
    {
        datetime = 0;
        pos_message = strchr (ptr_lines->data, '\t');
        if (pos_message)
        {
            pos_message[0] = '\0';
            error = strptime (ptr_lines->data, logger_option_time_format,
                              &tm_line);
            if (error && !error[0])
                datetime = mktime (&tm_line);
            pos_message[0] = '\t';
        }
        if (pos_message)
        {
            if (datetime != 0)
                weechat_printf_date (buffer, datetime, pos_message + 1);
            else
                weechat_printf (buffer, ptr_lines->data);
        }
        else
        {
            weechat_printf (buffer, ptr_lines->data);
        }
        num_lines++;
        ptr_lines = ptr_lines->next_line;
    }
    if (last_lines)
        logger_tail_free (last_lines);
    if (num_lines > 0)
        weechat_printf (buffer,
                        _("===\t========== End of backlog (%d lines) =========="),
                        num_lines);
}

/*
 * logger_backlog_signal_cb: callback for "logger_backlog" signal
 */

int
logger_backlog_signal_cb (void *data, char *signal, void *pointer)
{
    struct t_logger_buffer *ptr_logger_buffer;
    
    /* make C compiler happy */
    (void) data;
    (void) signal;

    if (logger_option_backlog >= 0)
    {
        ptr_logger_buffer = logger_buffer_search (pointer);
        if (ptr_logger_buffer && ptr_logger_buffer->log_filename
            && ptr_logger_buffer->log_enabled)
        {
            ptr_logger_buffer->log_enabled = 0;
            
            logger_backlog (pointer,
                            ptr_logger_buffer->log_filename,
                            logger_option_backlog);
            
            ptr_logger_buffer->log_enabled = 1;
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * logger_print_cb: callback for print hook
 */

int
logger_print_cb (void *data, struct t_gui_buffer *buffer, time_t date,
                 char *prefix, char *message)
{
    struct t_logger_buffer *ptr_logger_buffer;
    struct tm *date_tmp;
    char buf_time[256];
    
    /* make C compiler happy */
    (void) data;

    ptr_logger_buffer = logger_buffer_search (buffer);
    if (ptr_logger_buffer && ptr_logger_buffer->log_filename
        && ptr_logger_buffer->log_enabled)
    {
        date_tmp = localtime (&date);
        buf_time[0] = '\0';
        if (date_tmp)
            strftime (buf_time, sizeof (buf_time) - 1,
                      logger_option_time_format, date_tmp);
        
        logger_write_line (ptr_logger_buffer,
                           "%s\t%s\t%s",
                           buf_time,
                           (prefix) ? prefix : "",
                           message);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * logger_config_cb: callback for config hook
 */

int
logger_config_cb (void *data, char *type, char *option, char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) type;
    (void) option;
    (void) value;
    
    logger_config_read ();
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize logger plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    weechat_plugin = plugin;
    
    if (!logger_config_read ())
        return WEECHAT_RC_ERROR;
    if (!logger_create_directory ())
        return WEECHAT_RC_ERROR;
    
    logger_start_buffer_all ();
    
    weechat_hook_signal ("buffer_open", &logger_buffer_open_signal_cb, NULL);
    weechat_hook_signal ("buffer_close", &logger_buffer_close_signal_cb, NULL);
    weechat_hook_signal ("logger_backlog", &logger_backlog_signal_cb, NULL);
    
    weechat_hook_print (NULL, NULL, 1, &logger_print_cb, NULL);
    
    weechat_hook_config ("plugin", "logger." LOGGER_OPTION_PATH,
                         &logger_config_cb, NULL);
    weechat_hook_config ("plugin", "logger." LOGGER_OPTION_TIME_FORMAT,
                         &logger_config_cb, NULL);
    weechat_hook_config ("plugin", "logger." LOGGER_OPTION_INFO_LINES,
                         &logger_config_cb, NULL);
    weechat_hook_config ("plugin", "logger." LOGGER_OPTION_BACKLOG,
                         &logger_config_cb, NULL);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end logger plugin
 */

int
weechat_plugin_end ()
{
    logger_end_all ();
    
    return WEECHAT_RC_OK;
}
