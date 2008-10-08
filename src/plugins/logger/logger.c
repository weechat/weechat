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

/* logger.c: Logger plugin for WeeChat */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
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

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-buffer.h"
#include "logger-config.h"
#include "logger-info.h"
#include "logger-tail.h"


WEECHAT_PLUGIN_NAME(LOGGER_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("Logger plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_WEECHAT_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_logger_plugin = NULL;

char *logger_buf_write = NULL;         /* buffer for writing a line         */


/*
 * logger_create_directory: create logger directory
 *                          return 1 if success (directory created or already
 *                          exists), 0 if failed
 */

int
logger_create_directory ()
{
    int rc;
    char *dir1, *dir2, *weechat_dir;
    
    rc = 1;
    
    dir1 = weechat_string_replace (weechat_config_string (logger_config_file_path),
                                   "~", getenv ("HOME"));
    if (dir1)
    {
        weechat_dir = weechat_info_get ("weechat_dir", "");
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
    struct t_infolist *ptr_infolist;
    char *res;
    char *dir_separator, *weechat_dir, *log_path, *log_path2;
    char *plugin_name, *plugin_name2, *name, *name2;
    int length;
    
    res = NULL;
    
    dir_separator = weechat_info_get ("dir_separator", "");
    weechat_dir = weechat_info_get ("weechat_dir", "");
    log_path = weechat_string_replace (weechat_config_string (logger_config_file_path),
                                       "~", getenv ("HOME"));
    log_path2 = weechat_string_replace (log_path, "%h", weechat_dir);
    
    if (dir_separator && weechat_dir && log_path && log_path2)
    {
        ptr_infolist = weechat_infolist_get ("buffer", buffer, NULL);
        if (ptr_infolist)
        {
            name2 = NULL;
            if (weechat_infolist_next (ptr_infolist))
            {
                plugin_name = weechat_infolist_string (ptr_infolist, "plugin_name");
                plugin_name2 = (plugin_name) ?
                    weechat_string_replace (plugin_name, dir_separator, "_") : strdup ("core");
                name = weechat_infolist_string (ptr_infolist, "name");
                name2 = (name) ?
                    weechat_string_replace (name, dir_separator, "_") : NULL;
            }
            length = strlen (log_path2);
            if (plugin_name2)
                length += strlen (plugin_name2) + 1;
            if (name2)
                length += strlen (name2) + 1;
            length += 16;
            res = malloc (length);
            if (res)
            {
                strcpy (res, log_path2);
                if (plugin_name2)
                {
                    if (weechat_config_boolean (logger_config_file_name_lower_case))
                        weechat_string_tolower (plugin_name2);
                    strcat (res, plugin_name2);
                    strcat (res, ".");
                }
                if (name2)
                {
                    if (weechat_config_boolean (logger_config_file_name_lower_case))
                        weechat_string_tolower (name2);
                    strcat (res, name2);
                    strcat (res, ".");
                }
                strcat (res, "weechatlog");
            }
            if (name2)
                free (name2);
            weechat_infolist_free (ptr_infolist);
        }
    }
    
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
logger_write_line (struct t_logger_buffer *logger_buffer,
                   const char *format, ...)
{
    va_list argptr;
    char *charset, *message;
    time_t seconds;
    struct tm *date_tmp;
    char buf_time[256];

    if (!logger_buf_write)
        logger_buf_write = malloc (LOGGER_BUF_WRITE_SIZE);
    if (!logger_buf_write)
        return;
    
    if (logger_buffer->log_filename)
    {
        charset = weechat_info_get ("charset_terminal", "");
        
        if (!logger_buffer->log_file)
        {
            if (!logger_create_directory ())
            {
                weechat_printf (NULL,
                                _("%s%s: unable to create directory for logs "
                                  "(\"%s\")"),
                                weechat_prefix ("error"), LOGGER_PLUGIN_NAME,
                                weechat_config_string (logger_config_file_path));
                free (logger_buffer->log_filename);
                logger_buffer->log_filename = NULL;
                return;
            }
            logger_buffer->log_file =
                fopen (logger_buffer->log_filename, "a");
            if (!logger_buffer->log_file)
            {
                weechat_printf (NULL,
                                _("%s%s: unable to write log file \"%s\""),
                                weechat_prefix ("error"), LOGGER_PLUGIN_NAME,
                                logger_buffer->log_filename);
                free (logger_buffer->log_filename);
                logger_buffer->log_filename = NULL;
                return;
            }
            
            if (weechat_config_boolean (logger_config_file_info_lines))
            {
                seconds = time (NULL);
                date_tmp = localtime (&seconds);
                buf_time[0] = '\0';
                if (date_tmp)
                    strftime (buf_time, sizeof (buf_time) - 1,
                              weechat_config_string (logger_config_file_time_format),
                              date_tmp);
                snprintf (logger_buf_write, LOGGER_BUF_WRITE_SIZE,
                          _("%s\t****  Beginning of log  ****"),
                          buf_time);
                message = (charset) ?
                    weechat_iconv_from_internal (charset, logger_buf_write) : NULL;
                fprintf (logger_buffer->log_file,
                         "%s\n", (message) ? message : logger_buf_write);
                if (message)
                    free (message);
            }
        }

        va_start (argptr, format);
        vsnprintf (logger_buf_write, LOGGER_BUF_WRITE_SIZE, format, argptr);
        va_end (argptr);
        
        message = (charset) ?
            weechat_iconv_from_internal (charset, logger_buf_write) : NULL;
        
        fprintf (logger_buffer->log_file,
                 "%s\n", (message) ? message : logger_buf_write);
        fflush (logger_buffer->log_file);
        if (message)
            free (message);
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
    
    if (!buffer || !weechat_config_boolean (logger_config_file_auto_log))
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
    struct t_infolist *ptr_infolist;
    
    ptr_infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (ptr_infolist)
    {
        while (weechat_infolist_next (ptr_infolist))
        {
            logger_start_buffer (weechat_infolist_pointer (ptr_infolist,
                                                           "pointer"));
        }
        weechat_infolist_free (ptr_infolist);
    }
}

/*
 * logger_stop: stop log for a logger buffer
 */

void
logger_stop (struct t_logger_buffer *logger_buffer, int write_info_line)
{
    time_t seconds;
    struct tm *date_tmp;
    char buf_time[256];
    
    if (!logger_buffer)
        return;
    
    if (logger_buffer->log_file)
    {
        if (write_info_line && weechat_config_boolean (logger_config_file_info_lines))
        {
            seconds = time (NULL);
            date_tmp = localtime (&seconds);
            buf_time[0] = '\0';
            if (date_tmp)
                strftime (buf_time, sizeof (buf_time) - 1,
                          weechat_config_string (logger_config_file_time_format),
                          date_tmp);
            logger_write_line (logger_buffer,
                               _("%s\t****  End of log  ****"),
                               buf_time);
        }
        fclose (logger_buffer->log_file);
        logger_buffer->log_file = NULL;
    }
    logger_buffer_free (logger_buffer);
}

/*
 * logger_stop_all: end log for all buffers
 */

void
logger_stop_all ()
{
    struct t_logger_buffer *ptr_logger_buffer;
    
    for (ptr_logger_buffer = logger_buffers; ptr_logger_buffer;
         ptr_logger_buffer = ptr_logger_buffer->next_buffer)
    {
        logger_stop (ptr_logger_buffer, 1);
    }
}

/*
 * logger_buffer_open_signal_cb: callback for "buffer_open" signal
 */

int
logger_buffer_open_signal_cb (void *data, const char *signal,
                              const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    logger_start_buffer (signal_data);
    
    return WEECHAT_RC_OK;
}

/*
 * logger_buffer_closing_signal_cb: callback for "buffer_closing" signal
 */

int
logger_buffer_closing_signal_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    logger_stop (logger_buffer_search (signal_data), 1);
    
    return WEECHAT_RC_OK;
}

/*
 * logger_backlog: display backlog for a buffer (by reading end of log file)
 */

void
logger_backlog (struct t_gui_buffer *buffer, const char *filename, int lines)
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
            error = strptime (ptr_lines->data,
                              weechat_config_string (logger_config_file_time_format),
                              &tm_line);
            if (error && !error[0])
                datetime = mktime (&tm_line);
            pos_message[0] = '\t';
        }
        if (pos_message)
        {
            if (datetime != 0)
                weechat_printf_date (buffer, datetime, "%s", pos_message + 1);
            else
                weechat_printf (buffer, "%s", ptr_lines->data);
        }
        else
        {
            weechat_printf (buffer, "%s", ptr_lines->data);
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
logger_backlog_signal_cb (void *data, const char *signal,
                          const char *type_data, void *signal_data)
{
    struct t_logger_buffer *ptr_logger_buffer;
    
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (weechat_config_integer (logger_config_look_backlog) >= 0)
    {
        ptr_logger_buffer = logger_buffer_search (signal_data);
        if (ptr_logger_buffer && ptr_logger_buffer->log_filename
            && ptr_logger_buffer->log_enabled)
        {
            ptr_logger_buffer->log_enabled = 0;
            
            logger_backlog (signal_data,
                            ptr_logger_buffer->log_filename,
                            weechat_config_integer (logger_config_look_backlog));
            
            ptr_logger_buffer->log_enabled = 1;
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * logger_start_signal_cb: callback for "logger_start" signal
 */

int
logger_start_signal_cb (void *data, const char *signal, const char *type_data,
                        void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    logger_start_buffer (signal_data);
    
    return WEECHAT_RC_OK;
}

/*
 * logger_stop_signal_cb: callback for "logger_stop" signal
 */

int
logger_stop_signal_cb (void *data, const char *signal, const char *type_data,
                       void *signal_data)
{
    struct t_logger_buffer *ptr_logger_buffer;
    
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    ptr_logger_buffer = logger_buffer_search (signal_data);
    if (ptr_logger_buffer)
        logger_stop (ptr_logger_buffer, 0);
    
    return WEECHAT_RC_OK;
}

/*
 * logger_print_cb: callback for print hook
 */

int
logger_print_cb (void *data, struct t_gui_buffer *buffer, time_t date,
                 int tags_count, char **tags,
                 const char *prefix, const char *message)
{
    struct t_logger_buffer *ptr_logger_buffer;
    struct tm *date_tmp;
    char buf_time[256];
    
    /* make C compiler happy */
    (void) data;
    (void) tags_count;
    (void) tags;
    
    ptr_logger_buffer = logger_buffer_search (buffer);
    if (ptr_logger_buffer && ptr_logger_buffer->log_filename
        && ptr_logger_buffer->log_enabled
        && (date > 0))
    {
        date_tmp = localtime (&date);
        buf_time[0] = '\0';
        if (date_tmp)
        {
            strftime (buf_time, sizeof (buf_time) - 1,
                      weechat_config_string (logger_config_file_time_format),
                      date_tmp);
        }
        
        logger_write_line (ptr_logger_buffer,
                           "%s\t%s\t%s",
                           buf_time,
                           (prefix) ? prefix : "",
                           message);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize logger plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    weechat_plugin = plugin;
    
    if (!logger_config_init ())
        return WEECHAT_RC_ERROR;
    
    if (logger_config_read () < 0)
        return WEECHAT_RC_ERROR;
    
    logger_start_buffer_all ();
    
    weechat_hook_signal ("buffer_open", &logger_buffer_open_signal_cb, NULL);
    weechat_hook_signal ("buffer_closing", &logger_buffer_closing_signal_cb, NULL);
    weechat_hook_signal ("logger_backlog", &logger_backlog_signal_cb, NULL);
    weechat_hook_signal ("logger_start", &logger_start_signal_cb, NULL);
    weechat_hook_signal ("logger_stop", &logger_stop_signal_cb, NULL);
    
    weechat_hook_print (NULL, NULL, NULL, 1, &logger_print_cb, NULL);
    
    logger_info_init ();
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end logger plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    logger_config_write ();
    
    logger_stop_all ();
    
    logger_config_free ();
    
    if (logger_buf_write)
        free (logger_buf_write);
    
    return WEECHAT_RC_OK;
}
