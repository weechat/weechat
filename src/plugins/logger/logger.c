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

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-buffer.h"


static struct t_weechat_plugin *weechat_plugin = NULL;
static char *logger_path = NULL;
static char *logger_time_format = NULL;


/*
 * logger_config_read: read config options for logger plugin
 */

static void
logger_config_read ()
{
    if (logger_path)
        free (logger_path);
    logger_path = weechat_plugin_config_get ("path");
    if (!logger_path)
    {
        weechat_plugin_config_set ("path", "%h/logs/");
        logger_path = weechat_plugin_config_get ("path");
    }

    if (logger_time_format)
        free (logger_time_format);
    logger_time_format = weechat_plugin_config_get ("time_format");
    if (!logger_time_format)
    {
        weechat_plugin_config_set ("time_format", "%Y %b %d %H:%M:%S");
        logger_time_format = weechat_plugin_config_get ("time_format");
    }
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
    
    dir1 = weechat_string_replace (logger_path, "~", getenv ("HOME"));
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
logger_get_filename (void *buffer)
{
    struct t_plugin_list *ptr_list;
    char *res;
    char *dir_separator, *weechat_dir, *log_path, *log_path2;
    char *category, *category2, *name, *name2;
    int length;
    
    res = NULL;
    
    dir_separator = weechat_info_get ("dir_separator");
    weechat_dir = weechat_info_get ("weechat_dir");
    log_path = weechat_string_replace (logger_path, "~", getenv ("HOME"));
    log_path2 = weechat_string_replace (log_path, "%h", weechat_dir);
    
    if (dir_separator && weechat_dir && log_path && log_path2)
    {
        ptr_list = weechat_list_get ("buffer", buffer);
        if (ptr_list)
        {
            category2 = NULL;
            name2 = NULL;
            if (weechat_list_next (ptr_list))
            {
                category = weechat_list_string (ptr_list, "category");
                category2 = (category) ?
                    weechat_string_replace (category, dir_separator, "_") : NULL;
                name = weechat_list_string (ptr_list, "name");
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
            weechat_list_free (ptr_list);
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
            
            seconds = time (NULL);
            date_tmp = localtime (&seconds);
            buf_time[0] = '\0';
            if (date_tmp)
                strftime (buf_time, sizeof (buf_time) - 1,
                          logger_time_format, date_tmp);
            snprintf (buf, sizeof (buf) - 1,
                      _("****  Beginning of log  %s  ****"),
                      buf_time);
            message = (charset) ?
                weechat_iconv_from_internal (charset, buf) : NULL;
            fprintf (logger_buffer->log_file,
                     "%s\n", (message) ? message : buf);
            if (message)
                free (message);
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
logger_start_buffer (void *buffer)
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
    struct t_plugin_list *ptr_list;
    
    ptr_list = weechat_list_get ("buffer", NULL);
    while (weechat_list_next (ptr_list))
    {
        logger_start_buffer (weechat_list_pointer (ptr_list, "pointer"));
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
        seconds = time (NULL);
        date_tmp = localtime (&seconds);
        buf_time[0] = '\0';
        if (date_tmp)
            strftime (buf_time, sizeof (buf_time) - 1,
                      logger_time_format, date_tmp);
        logger_write_line (logger_buffer,
                           _("****  End of log  %s  ****"),
                           buf_time);
        fclose (logger_buffer->log_file);
        logger_buffer->log_file = NULL;
        logger_buffer_remove (logger_buffer);
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
 * logger_event_cb: callback for event hook
 */

static int
logger_event_cb (void *data, char *event, void *pointer)
{
    /* make C compiler happy */
    (void) data;
    (void) pointer;
    
    if (weechat_strcasecmp (event, "buffer_open") == 0)
    {
        logger_start_buffer (pointer);
    }
    else if (weechat_strcasecmp (event, "buffer_close") == 0)
    {
        logger_end (logger_buffer_search (pointer));
    }
    
    return PLUGIN_RC_SUCCESS;
}

/*
 * logger_print_cb: callback for print hook
 */

static int
logger_print_cb (void *data, void *buffer, time_t date, char *prefix,
                 char *message)
{
    struct t_logger_buffer *ptr_logger_buffer;
    struct tm *date_tmp;
    char buf_time[256];
    
    /* make C compiler happy */
    (void) data;

    ptr_logger_buffer = logger_buffer_search (buffer);
    if (ptr_logger_buffer && ptr_logger_buffer->log_filename)
    {
        date_tmp = localtime (&date);
        buf_time[0] = '\0';
        if (date_tmp)
            strftime (buf_time, sizeof (buf_time) - 1,
                      logger_time_format, date_tmp);
        
        logger_write_line (ptr_logger_buffer,
                           "%s%s%s%s%s",
                           buf_time,
                           (buf_time[0]) ? "  " : "",
                           (prefix) ? prefix : "",
                           (prefix && prefix[0]) ? " " : "",
                           message);
    }
    
    return PLUGIN_RC_SUCCESS;
}

/*
 * weechat_plugin_init: initialize logger plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    weechat_plugin = plugin;
    
    logger_config_read ();
    if (!logger_path || !logger_time_format)
        return PLUGIN_RC_FAILED;
    if (!logger_create_directory ())
        return PLUGIN_RC_FAILED;
    
    logger_start_buffer_all ();
    
    weechat_hook_event ("buffer_open", logger_event_cb, NULL);
    weechat_hook_event ("buffer_close", logger_event_cb, NULL);
    
    weechat_hook_print (NULL, NULL, 1, logger_print_cb, NULL);
    
    return PLUGIN_RC_SUCCESS;
}

/*
 * weechat_plugin_end: end logger plugin
 */

int
weechat_plugin_end ()
{
    logger_end_all ();
    
    return PLUGIN_RC_SUCCESS;
}
