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
#include <ctype.h>
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
    char *dir1, *dir2;
    const char *weechat_dir;
    
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
                if (!weechat_mkdir_parents (dir2, 0700))
                    rc = 0;
                
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
 * logger_build_option_name: build option name with a buffer
 */

char *
logger_build_option_name (struct t_gui_buffer *buffer)
{
    const char *plugin_name, *name;
    char *option_name;
    int length;
    
    if (!buffer)
        return NULL;
    
    plugin_name = weechat_buffer_get_string (buffer, "plugin");
    name = weechat_buffer_get_string (buffer, "name");
    
    length = strlen (plugin_name) + 1 + strlen (name) + 1;
    option_name = malloc (length);
    if (!option_name)
        return NULL;
    
    snprintf (option_name, length, "%s.%s", plugin_name, name);
    
    return option_name;
}

/*
 * logger_get_level_for_buffer: get logging level for buffer (0 = disabled, 1..9)
 */

int
logger_get_level_for_buffer (struct t_gui_buffer *buffer)
{
    const char *no_log;
    char *name, *option_name, *ptr_end;
    struct t_config_option *ptr_option;
    
    /* no log for buffer if local variable "no_log" is defined for buffer */
    no_log = weechat_buffer_get_string (buffer, "localvar_no_log");
    if (no_log && no_log[0])
        return 0;
    
    name = logger_build_option_name (buffer);
    if (!name)
        return LOGGER_LEVEL_DEFAULT;
    
    option_name = strdup (name);
    if (option_name)
    {
        ptr_end = option_name + strlen (option_name);
        while (ptr_end >= option_name)
        {
            ptr_option = logger_config_get_level (option_name);
            if (ptr_option)
            {
                free (option_name);
                free (name);
                return weechat_config_integer (ptr_option);
            }
            ptr_end--;
            while ((ptr_end >= option_name) && (ptr_end[0] != '.'))
            {
                ptr_end--;
            }
            if ((ptr_end >= option_name) && (ptr_end[0] == '.'))
                ptr_end[0] = '\0';
        }
        ptr_option = logger_config_get_level (option_name);
        
        free (option_name);
        free (name);
        
        if (ptr_option)
            return weechat_config_integer (ptr_option);
    }
    else
        free (name);
    
    /* nothing found => return default level */
    return LOGGER_LEVEL_DEFAULT;
}

/*
 * logger_get_mask_for_buffer: get filename mask for a buffer
 *                             we first try with all arguments, then remove one by
 *                             one to find mask (from specific to general mask)
 */

const char *
logger_get_mask_for_buffer (struct t_gui_buffer *buffer)
{
    char *name, *option_name, *ptr_end;
    struct t_config_option *ptr_option;
    
    name = logger_build_option_name (buffer);
    if (!name)
        return NULL;
    
    option_name = strdup (name);
    if (option_name)
    {
        ptr_end = option_name + strlen (option_name);
        while (ptr_end >= option_name)
        {
            ptr_option = logger_config_get_mask (option_name);
            if (ptr_option)
            {
                free (option_name);
                free (name);
                return weechat_config_string (ptr_option);
            }
            ptr_end--;
            while ((ptr_end >= option_name) && (ptr_end[0] != '.'))
            {
                ptr_end--;
            }
            if ((ptr_end >= option_name) && (ptr_end[0] == '.'))
                ptr_end[0] = '\0';
        }
        ptr_option = logger_config_get_mask (option_name);
        
        free (option_name);
        free (name);
        
        if (ptr_option)
            return weechat_config_string (ptr_option);
    }
    else
        free (name);
    
    /* nothing found => return default mask (if set) */
    if (weechat_config_string (logger_config_file_mask)
        && weechat_config_string (logger_config_file_mask)[0])
        return weechat_config_string (logger_config_file_mask);
    
    /* no default mask set */
    return NULL;
}

/*
 * logger_get_filename: build log filename for a buffer
 */

char *
logger_get_filename (struct t_gui_buffer *buffer)
{
    char *res, *mask_decoded;
    const char *mask;
    const char *dir_separator, *weechat_dir;
    char *log_path, *log_path2, *pos_last_sep;
    int length;
    
    res = NULL;
    
    /* get filename mask for buffer */
    mask = logger_get_mask_for_buffer (buffer);
    if (!mask)
    {
        weechat_printf (NULL,
                        _("%s%s: unable to find filename mask for buffer "
                          "\"%s\", logging is disabled for this buffer"),
                        weechat_prefix ("error"), LOGGER_PLUGIN_NAME,
                        weechat_buffer_get_string (buffer, "name"));
        return NULL;
    }
    mask_decoded = weechat_buffer_string_replace_local_var (buffer, mask);
    if (!mask_decoded)
        return NULL;
    
    if (weechat_logger_plugin->debug)
    {
        weechat_printf (NULL,
                        "%s: buffer = \"%s\", mask = \"%s\", "
                        "decoded mask = \"%s\"",
                        LOGGER_PLUGIN_NAME,
                        weechat_buffer_get_string (buffer, "name"),
                        mask, mask_decoded);
    }
    
    if (weechat_config_boolean (logger_config_file_name_lower_case))
        weechat_string_tolower (mask_decoded);
    
    dir_separator = weechat_info_get ("dir_separator", "");
    weechat_dir = weechat_info_get ("weechat_dir", "");
    log_path = weechat_string_replace (weechat_config_string (logger_config_file_path),
                                       "~", getenv ("HOME"));
    log_path2 = weechat_string_replace (log_path, "%h", weechat_dir);
    
    if (dir_separator && weechat_dir && log_path && log_path2)
    {
        length = strlen (log_path2) + strlen (mask_decoded) + 1;
        res = malloc (length);
        if (res)
        {
            snprintf (res, length, "%s%s", log_path2, mask_decoded);
            pos_last_sep = strrchr (res, dir_separator[0]);
            if (pos_last_sep)
                pos_last_sep[0] = '\0';
            weechat_mkdir_parents (res, 0700);
            if (pos_last_sep)
                pos_last_sep[0] = dir_separator[0];
        }
    }
    
    if (log_path)
        free (log_path);
    if (log_path2)
        free (log_path2);
    
    return res;
}

/*
 * logger_set_log_filename: set log filename for a logger buffer
 */

void
logger_set_log_filename (struct t_logger_buffer *logger_buffer)
{
    char *log_filename;
    struct t_logger_buffer *ptr_logger_buffer;
    
    log_filename = logger_get_filename (logger_buffer->buffer);
    if (!log_filename)
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory"),
                        weechat_prefix ("error"),
                        LOGGER_PLUGIN_NAME);
        return;
    }
    
    ptr_logger_buffer = logger_buffer_search_log_filename (log_filename);
    if (ptr_logger_buffer)
    {
        weechat_printf (NULL,
                        _("%s%s: unable to start logging for buffer "
                          "\"%s\": filename \"%s\" is already user by "
                          "another buffer (check your log settings)"),
                        weechat_prefix ("error"),
                        LOGGER_PLUGIN_NAME,
                        weechat_buffer_get_string (logger_buffer->buffer, "name"),
                        log_filename);
        free (log_filename);
        return;
    }
    
    logger_buffer->log_filename = log_filename;
}

/*
 * logger_write_line: write a line to log file
 */

void
logger_write_line (struct t_logger_buffer *logger_buffer,
                   const char *format, ...)
{
    va_list argptr;
    char *message, buf_time[256];
    const char *charset;
    time_t seconds;
    struct tm *date_tmp;
    int log_level;
    
    if (!logger_buf_write)
        logger_buf_write = malloc (LOGGER_BUF_WRITE_SIZE);
    if (!logger_buf_write)
        return;
    
    charset = weechat_info_get ("charset_terminal", "");
    
    if (!logger_buffer->log_file)
    {
        log_level = logger_get_level_for_buffer (logger_buffer->buffer);
        if (log_level == 0)
        {
            logger_buffer_free (logger_buffer);
            return;
        }
        if (!logger_create_directory ())
        {
            weechat_printf (NULL,
                            _("%s%s: unable to create directory for logs "
                              "(\"%s\")"),
                            weechat_prefix ("error"), LOGGER_PLUGIN_NAME,
                            weechat_config_string (logger_config_file_path));
            logger_buffer_free (logger_buffer);
            return;
        }
        if (!logger_buffer->log_filename)
            logger_set_log_filename (logger_buffer);
        
        if (!logger_buffer->log_filename)
        {
            logger_buffer_free (logger_buffer);
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
            logger_buffer_free (logger_buffer);
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
    
    if (logger_buffer->log_enabled && logger_buffer->log_file)
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
 * logger_start_buffer: start a log for a buffer
 */

void
logger_start_buffer (struct t_gui_buffer *buffer)
{
    struct t_logger_buffer *ptr_logger_buffer;
    int log_level, log_enabled;
    
    if (!buffer)
        return;
    
    log_level = logger_get_level_for_buffer (buffer);
    log_enabled =  weechat_config_boolean (logger_config_file_auto_log)
        && (log_level > 0);
    
    ptr_logger_buffer = logger_buffer_search_buffer (buffer);
    
    /* logging is disabled for buffer */
    if (!log_enabled)
    {
        /* stop logger if it is active */
        if (ptr_logger_buffer)
            logger_stop (ptr_logger_buffer, 1);
    }
    else
    {
        /* logging is enabled for buffer */
        if (ptr_logger_buffer)
            ptr_logger_buffer->log_level = log_level;
        else
        {
            ptr_logger_buffer = logger_buffer_add (buffer, log_level);
            
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
 * logger_list: show logging status for buffers
 */

void
logger_list ()
{
    struct t_infolist *ptr_infolist;
    struct t_logger_buffer *ptr_logger_buffer;
    struct t_gui_buffer *ptr_buffer;
    char status[128];
    
    weechat_printf (NULL, "");
    weechat_printf (NULL, _("Logging on buffers:"));
    
    ptr_infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (ptr_infolist)
    {
        while (weechat_infolist_next (ptr_infolist))
        {
            ptr_buffer = weechat_infolist_pointer (ptr_infolist, "pointer");
            if (ptr_buffer)
            {
                ptr_logger_buffer = logger_buffer_search_buffer (ptr_buffer);
                if (ptr_logger_buffer)
                {
                    snprintf (status, sizeof (status),
                              _("logging (level: %d)"),
                              ptr_logger_buffer->log_level);
                }
                else
                {
                    snprintf (status, sizeof (status), _("not logging"));
                }
                weechat_printf (NULL,
                                "  %s[%s%d%s]%s (%s) %s%s%s: %s%s%s%s",
                                weechat_color("chat_delimiters"),
                                weechat_color("chat"),
                                weechat_infolist_integer (ptr_infolist, "number"),
                                weechat_color("chat_delimiters"),
                                weechat_color("chat"),
                                weechat_infolist_string (ptr_infolist, "plugin_name"),
                                weechat_color("chat_buffer"),
                                weechat_infolist_string (ptr_infolist, "name"),
                                weechat_color("chat"),
                                status,
                                (ptr_logger_buffer) ? " (" : "",
                                (ptr_logger_buffer) ?
                                ((ptr_logger_buffer->log_filename) ?
                                 ptr_logger_buffer->log_filename : _("log not started")) : "",
                                (ptr_logger_buffer) ? ")" : "");
            }
        }
        weechat_infolist_free (ptr_infolist);
    }
}

/*
 * logger_set_buffer: enable/disable log on a buffer
 */

void
logger_set_buffer (struct t_gui_buffer *buffer, const char *value)
{
    char *name;
    struct t_config_option *ptr_option;
    
    name = logger_build_option_name (buffer);
    if (!name)
        return;
    
    if (logger_config_set_level (name, value) != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        ptr_option = logger_config_get_level (name);
        if (ptr_option)
        {
            weechat_printf (NULL, _("%s: \"%s\" => level %d"),
                            LOGGER_PLUGIN_NAME, name,
                            weechat_config_integer (ptr_option));
        }
    }
    
    free (name);
}

/*
 * logger_command_cb: callback for /logger command
 */

int
logger_command_cb (void *data, struct t_gui_buffer *buffer,
                   int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if ((argc == 1)
        || ((argc == 2) && (weechat_strcasecmp (argv[1], "list") == 0)))
    {
        logger_list ();
        return WEECHAT_RC_OK;
    }
    
    if (argc > 1)
    {
        if (weechat_strcasecmp (argv[1], "set") == 0)
        {
            if (argc > 2)
                logger_set_buffer (buffer, argv[2]);
            return WEECHAT_RC_OK;
        }
        
        if (weechat_strcasecmp (argv[1], "disable") == 0)
        {
            logger_set_buffer (buffer, "0");
        }
    }
    
    return WEECHAT_RC_OK;
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
    
    logger_stop (logger_buffer_search_buffer (signal_data), 1);
    
    return WEECHAT_RC_OK;
}

/*
 * logger_buffer_renamed_signal_cb: callback for "buffer_closing" signal
 */

int
logger_buffer_renamed_signal_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    logger_stop (logger_buffer_search_buffer (signal_data), 1);
    logger_start_buffer (signal_data);
    
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
    {
        weechat_printf (buffer,
                        _("===\t========== End of backlog (%d lines) =========="),
                        num_lines);
        weechat_buffer_set (buffer, "unread", "");
    }
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
        ptr_logger_buffer = logger_buffer_search_buffer (signal_data);
        if (ptr_logger_buffer && ptr_logger_buffer->log_enabled)
        {
            if (!ptr_logger_buffer->log_filename)
                logger_set_log_filename (ptr_logger_buffer);
            
            if (ptr_logger_buffer->log_filename)
            {
                ptr_logger_buffer->log_enabled = 0;
                
                logger_backlog (signal_data,
                                ptr_logger_buffer->log_filename,
                                weechat_config_integer (logger_config_look_backlog));
                
                ptr_logger_buffer->log_enabled = 1;
            }
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
    
    ptr_logger_buffer = logger_buffer_search_buffer (signal_data);
    if (ptr_logger_buffer)
        logger_stop (ptr_logger_buffer, 0);
    
    return WEECHAT_RC_OK;
}

/*
 * logger_line_log_level: get log level for a line (with its tags)
 */

int
logger_line_log_level (int tags_count, const char **tags)
{
    int i;
    
    for (i = 0; i < tags_count; i++)
    {
        if (strncmp (tags[i], "log", 3) == 0)
        {
            if (isdigit (tags[i][3]))
            {
                return (tags[i][3] - '0');
            }
        }
    }
    
    return 9;
}

/*
 * logger_print_cb: callback for print hook
 */

int
logger_print_cb (void *data, struct t_gui_buffer *buffer, time_t date,
                 int tags_count, const char **tags,
                 int displayed, int highlight,
                 const char *prefix, const char *message)
{
    struct t_logger_buffer *ptr_logger_buffer;
    struct tm *date_tmp;
    char buf_time[256];
    int line_log_level;
    
    /* make C compiler happy */
    (void) data;
    (void) displayed;
    (void) highlight;
    
    line_log_level = logger_line_log_level (tags_count, tags);
    
    ptr_logger_buffer = logger_buffer_search_buffer (buffer);
    if (ptr_logger_buffer
        && ptr_logger_buffer->log_enabled
        && (date > 0)
        && (line_log_level <= ptr_logger_buffer->log_level))
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
    
    /* command /logger */
    weechat_hook_command ("logger",
                          N_("logger plugin configuration"),
                          N_("[list | set level | disable]"),
                          N_("   list: show logging status for open buffers\n"
                             "    set: set logging level on current buffer\n"
                             "  level: level for messages to be logged (0 = "
                             "logging disabled, 1 = a few messages (most "
                             "important) .. 9 = all messages)\n"
                             "disable: disable logging on current buffer (set "
                             "level to 0)\n\n"
                             "Options \"logger.level.*\" and \"logger.mask.*\" "
                             "can be used to set level or mask for a buffer, "
                             "or buffers beginning with name.\n\n"
                             "Examples:\n"
                             "  set level to 5 for current buffer:\n"
                             "    /logger set 5\n"
                             "  disable logging for current buffer:\n"
                             "    /logger disable\n\n"
                             "  set level to 3 for all IRC buffers:\n"
                             "    /set logger.level.irc 3\n"
                             "  disable logging for main WeeChat buffer:\n"
                             "    /set logger.level.core.weechat 0\n"
                             "  use a directory per IRC server and a file per channel inside:\n"
                             "    /set logger.mask.irc \"$server/$channel.weechatlog\"\n\n"
                             "Log levels used by IRC plugin:\n"
                             "  1: user message, notice, private\n"
                             "  2: nick change\n"
                             "  3: server message\n"
                             "  4: join/part/quit\n"
                             "  9: all other messages"),
                          "list|set|disable",
                          &logger_command_cb, NULL);
    
    logger_start_buffer_all ();
    
    weechat_hook_signal ("buffer_open", &logger_buffer_open_signal_cb, NULL);
    weechat_hook_signal ("buffer_closing", &logger_buffer_closing_signal_cb, NULL);
    weechat_hook_signal ("buffer_renamed", &logger_buffer_renamed_signal_cb, NULL);
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
