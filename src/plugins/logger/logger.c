/*
 * logger.c - logger plugin for WeeChat: save buffer lines to disk files
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-backlog.h"
#include "logger-buffer.h"
#include "logger-command.h"
#include "logger-config.h"
#include "logger-info.h"
#include "logger-tail.h"


WEECHAT_PLUGIN_NAME(LOGGER_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Log buffers to files"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(LOGGER_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_logger_plugin = NULL;

struct t_hook *logger_hook_timer = NULL;    /* timer to flush log files     */
struct t_hook *logger_hook_print = NULL;


/*
 * Checks conditions against a buffer.
 *
 * Returns:
 *   1: conditions OK
 *   0: conditions not OK
 */

int
logger_check_conditions (struct t_gui_buffer *buffer, const char *conditions)
{
    struct t_hashtable *pointers, *options;
    char *result;
    int condition_ok;

    if (!buffer)
        return 0;

    /* empty conditions = always true */
    if (!conditions || !conditions[0])
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

    result = weechat_string_eval_expression (conditions,
                                             pointers, NULL, options);
    condition_ok = (result && (strcmp (result, "1") == 0));
    free (result);

    weechat_hashtable_free (pointers);
    weechat_hashtable_free (options);

    return condition_ok;
}

/*
 * Gets logger file path option.
 *
 * Special vars are replaced:
 *   - with call to function string_eval_path_home
 *   - date/time specifiers (see man strftime)
 *
 * Note: result must be freed after use.
 */

char *
logger_get_file_path (void)
{
    char *path, *path2;
    int length;
    time_t seconds;
    struct tm *date_tmp;
    struct t_hashtable *options;

    path = NULL;
    path2 = NULL;

    /* evaluate path */
    options = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (options)
        weechat_hashtable_set (options, "directory", "data");
    path = weechat_string_eval_path_home (
        weechat_config_string (logger_config_file_path), NULL, NULL, options);
    weechat_hashtable_free (options);
    if (!path)
        goto end;

    /* replace date/time specifiers in path */
    length = strlen (path) + 256 + 1;
    path2 = malloc (length);
    if (!path2)
        goto end;
    seconds = time (NULL);
    date_tmp = localtime (&seconds);
    path2[0] = '\0';
    if (strftime (path2, length, path, date_tmp) == 0)
        path2[0] = '\0';

    if (weechat_logger_plugin->debug)
    {
        weechat_printf_date_tags (NULL, 0, "no_log",
                                  "%s: file path = \"%s\"",
                                  LOGGER_PLUGIN_NAME, path2);
    }

end:
    free (path);
    return path2;
}

/*
 * Creates logger directory.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
logger_create_directory (void)
{
    int rc;
    char *file_path;

    rc = 1;

    file_path = logger_get_file_path ();
    if (file_path)
    {
        if (!weechat_mkdir_parents (file_path, 0700))
        {
            weechat_printf_date_tags (
                NULL, 0, "no_log",
                _("%s%s: unable to create directory for logs "
                  "(\"%s\")"),
                weechat_prefix ("error"), LOGGER_PLUGIN_NAME,
                file_path);
            rc = 0;
        }
        free (file_path);
    }
    else
        rc = 0;

    return rc;
}

/*
 * Builds full name of buffer.
 *
 * Note: result must be freed after use.
 */

char *
logger_build_option_name (struct t_gui_buffer *buffer)
{
    const char *plugin_name, *name;
    char *option_name;

    if (!buffer)
        return NULL;

    plugin_name = weechat_buffer_get_string (buffer, "plugin");
    name = weechat_buffer_get_string (buffer, "name");

    weechat_asprintf (&option_name, "%s.%s", plugin_name, name);

    return option_name;
}

/*
 * Gets logging level for buffer.
 *
 * Returns level between 0 and 9 (0 = logging disabled).
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
 * Gets filename mask for a buffer.
 *
 * First tries with all arguments, then removes one by one to find mask (from
 * specific to general mask).
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
 * Gets expanded mask for a buffer.
 *
 * Special vars are replaced:
 *   - local variables of buffer ($plugin, $name, ..)
 *   - date/time specifiers (see man strftime)
 *
 * Note: result must be freed after use.
 */

char *
logger_get_mask_expanded (struct t_gui_buffer *buffer, const char *mask)
{
    char *mask2, *mask3, *mask4, *mask5, *mask6, *mask7, *mask8, *dir_separator;
    int length;
    time_t seconds;
    struct tm *date_tmp;

    mask2 = NULL;
    mask3 = NULL;
    mask4 = NULL;
    mask5 = NULL;
    mask6 = NULL;
    mask7 = NULL;
    mask8 = NULL;

    dir_separator = weechat_info_get ("dir_separator", "");
    if (!dir_separator)
        return NULL;

    /* replace date/time specifiers in mask */
    length = strlen (mask) + 256 + 1;
    mask2 = malloc (length);
    if (!mask2)
        goto end;
    seconds = time (NULL);
    date_tmp = localtime (&seconds);
    mask2[0] = '\0';
    if (strftime (mask2, length, mask, date_tmp) == 0)
        mask2[0] = '\0';

    /*
     * we first replace directory separator (commonly '/') by \001 because
     * buffer mask can contain this char, and will be replaced by replacement
     * char ('_' by default)
     */
    mask3 = weechat_string_replace (mask2, dir_separator, "\001");
    if (!mask3)
        goto end;

    mask4 = weechat_buffer_string_replace_local_var (buffer, mask3);
    if (!mask4)
        goto end;

    mask5 = weechat_string_replace (mask4,
                                    dir_separator,
                                    weechat_config_string (logger_config_file_replacement_char));
    if (!mask5)
        goto end;

#ifdef __CYGWIN__
    mask6 = weechat_string_replace (mask5, "\\",
                                    weechat_config_string (logger_config_file_replacement_char));
#else
    mask6 = strdup (mask5);
#endif /* __CYGWIN__ */
    if (!mask6)
        goto end;

    /* restore directory separator */
    mask7 = weechat_string_replace (mask6,
                                    "\001", dir_separator);
    if (!mask7)
        goto end;

    /* convert to lower case? */
    if (weechat_config_boolean (logger_config_file_name_lower_case))
        mask8 = weechat_string_tolower (mask7);
    else
        mask8 = strdup (mask7);

    if (weechat_logger_plugin->debug)
    {
        weechat_printf_date_tags (NULL, 0, "no_log",
                                  "%s: buffer = \"%s\", mask = \"%s\", "
                                  "decoded mask = \"%s\"",
                                  LOGGER_PLUGIN_NAME,
                                  weechat_buffer_get_string (buffer, "name"),
                                  mask, mask8);
    }

end:
    free (dir_separator);
    free (mask2);
    free (mask3);
    free (mask4);
    free (mask5);
    free (mask6);
    free (mask7);

    return mask8;
}

/*
 * Builds log filename for a buffer.
 *
 * Note: result must be freed after use.
 */

char *
logger_get_filename (struct t_gui_buffer *buffer)
{
    char *res, *mask_expanded, *file_path, *dir_separator;
    const char *mask;

    res = NULL;
    mask_expanded = NULL;
    file_path = NULL;

    dir_separator = weechat_info_get ("dir_separator", "");
    if (!dir_separator)
        return NULL;

    /* get filename mask for buffer */
    mask = logger_get_mask_for_buffer (buffer);
    if (!mask)
    {
        weechat_printf_date_tags (
            NULL, 0, "no_log",
            _("%s%s: unable to find filename mask for buffer "
              "\"%s\", logging is disabled for this buffer"),
            weechat_prefix ("error"), LOGGER_PLUGIN_NAME,
            weechat_buffer_get_string (buffer, "name"));
        free (dir_separator);
        return NULL;
    }

    mask_expanded = logger_get_mask_expanded (buffer, mask);
    if (!mask_expanded)
        goto end;

    file_path = logger_get_file_path ();
    if (!file_path)
        goto end;

    /* build string with path + mask */
    weechat_asprintf (
        &res,
        "%s%s%s",
        file_path,
        (file_path[strlen (file_path) - 1] == dir_separator[0]) ? "" : dir_separator,
        mask_expanded);

end:
    free (dir_separator);
    free (mask_expanded);
    free (file_path);

    return res;
}

/*
 * Callback for signal "buffer_opened".
 */

int
logger_buffer_opened_signal_cb (const void *pointer, void *data,
                                const char *signal,
                                const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    logger_buffer_start (signal_data, 1);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "buffer_closing".
 */

int
logger_buffer_closing_signal_cb (const void *pointer, void *data,
                                 const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    logger_buffer_stop (logger_buffer_search_buffer (signal_data), 1);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "buffer_renamed".
 */

int
logger_buffer_renamed_signal_cb (const void *pointer, void *data,
                                 const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    logger_buffer_stop (logger_buffer_search_buffer (signal_data), 1);
    logger_buffer_start (signal_data, 1);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "logger_start".
 */

int
logger_start_signal_cb (const void *pointer, void *data,
                        const char *signal, const char *type_data,
                        void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    logger_buffer_start (signal_data, 1);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "logger_stop".
 */

int
logger_stop_signal_cb (const void *pointer, void *data,
                       const char *signal, const char *type_data,
                       void *signal_data)
{
    struct t_logger_buffer *ptr_logger_buffer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    ptr_logger_buffer = logger_buffer_search_buffer (signal_data);
    if (ptr_logger_buffer)
        logger_buffer_stop (ptr_logger_buffer, 0);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "day_changed".
 */

int
logger_day_changed_signal_cb (const void *pointer, void *data,
                              const char *signal,
                              const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    logger_buffer_adjust_log_filenames ();

    return WEECHAT_RC_OK;
}

/*
 * Gets info with tags of line: log level and if prefix is a nick.
 */

void
logger_get_line_tag_info (int tags_count, const char **tags,
                          int *log_level, int *prefix_is_nick)
{
    int i, log_level_set, prefix_is_nick_set;

    if (log_level)
        *log_level = LOGGER_LEVEL_DEFAULT;
    if (prefix_is_nick)
        *prefix_is_nick = 0;

    log_level_set = 0;
    prefix_is_nick_set = 0;

    for (i = 0; i < tags_count; i++)
    {
        if (log_level && !log_level_set)
        {
            if (strcmp (tags[i], "no_log") == 0)
            {
                /* log disabled on line: set level to -1 */
                *log_level = -1;
                log_level_set = 1;
            }
            else if (strncmp (tags[i], "log", 3) == 0)
            {
                /* set log level for line */
                if (isdigit ((unsigned char)tags[i][3]))
                {
                    *log_level = (tags[i][3] - '0');
                    log_level_set = 1;
                }
            }
        }
        if (prefix_is_nick && !prefix_is_nick_set)
        {
            if (strncmp (tags[i], "prefix_nick", 11) == 0)
            {
                *prefix_is_nick = 1;
                prefix_is_nick_set = 1;
            }
        }
    }
}

/*
 * Callback for print hooked.
 */

int
logger_print_cb (const void *pointer, void *data,
                 struct t_gui_buffer *buffer, time_t date, int date_usec,
                 int tags_count, const char **tags,
                 int displayed, int highlight,
                 const char *prefix, const char *message)
{
    struct t_logger_buffer *ptr_logger_buffer;
    struct timeval tv;
    char buf_time[256], *prefix_ansi, *message_ansi;
    const char *ptr_prefix, *ptr_message;
    int line_log_level, prefix_is_nick, color_lines;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) displayed;
    (void) highlight;

    logger_get_line_tag_info (tags_count, tags, &line_log_level,
                              &prefix_is_nick);
    if (line_log_level < 0)
        return WEECHAT_RC_OK;

    ptr_logger_buffer = logger_buffer_search_buffer (buffer);
    if (ptr_logger_buffer
        && ptr_logger_buffer->log_enabled
        && (date > 0)
        && (line_log_level <= ptr_logger_buffer->log_level))
    {
        prefix_ansi = NULL;
        message_ansi = NULL;
        color_lines = weechat_config_boolean (logger_config_file_color_lines);
        if (color_lines)
        {
            prefix_ansi = weechat_hook_modifier_exec ("color_encode_ansi",
                                                      NULL, prefix);
            message_ansi = weechat_hook_modifier_exec ("color_encode_ansi",
                                                      NULL, message);
            ptr_prefix = prefix_ansi;
            ptr_message = message_ansi;
        }
        else
        {
            ptr_prefix = prefix;
            ptr_message = message;
        }
        tv.tv_sec = date;
        tv.tv_usec = date_usec;
        weechat_util_strftimeval (
            buf_time, sizeof (buf_time),
            weechat_config_string (logger_config_file_time_format),
            &tv);

        logger_buffer_write_line (
            ptr_logger_buffer,
            "%s\t%s%s%s\t%s%s",
            buf_time,
            (ptr_prefix && prefix_is_nick) ? weechat_config_string (logger_config_file_nick_prefix) : "",
            (ptr_prefix) ? ptr_prefix : "",
            (ptr_prefix && prefix_is_nick) ? weechat_config_string (logger_config_file_nick_suffix) : "",
            (color_lines) ? "\x1B[0m" : "",
            ptr_message);

        free (prefix_ansi);
        free (message_ansi);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for logger timer.
 */

int
logger_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    logger_buffer_flush ();

    return WEECHAT_RC_OK;
}

/*
 * Initializes logger plugin.
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

    logger_config_read ();

    logger_command_init ();

    logger_buffer_start_all (1);

    weechat_hook_signal ("buffer_opened",
                         &logger_buffer_opened_signal_cb, NULL, NULL);
    weechat_hook_signal ("buffer_closing",
                         &logger_buffer_closing_signal_cb, NULL, NULL);
    weechat_hook_signal ("buffer_renamed",
                         &logger_buffer_renamed_signal_cb, NULL, NULL);
    weechat_hook_signal ("logger_backlog",
                         &logger_backlog_signal_cb, NULL, NULL);
    weechat_hook_signal ("logger_start",
                         &logger_start_signal_cb, NULL, NULL);
    weechat_hook_signal ("logger_stop",
                         &logger_stop_signal_cb, NULL, NULL);
    weechat_hook_signal ("day_changed",
                         &logger_day_changed_signal_cb, NULL, NULL);

    logger_config_color_lines_change (NULL, NULL, NULL);

    logger_info_init ();

    return WEECHAT_RC_OK;
}

/*
 * Ends logger plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    if (logger_hook_print)
    {
        weechat_unhook (logger_hook_print);
        logger_hook_print = NULL;
    }

    if (logger_hook_timer)
    {
        weechat_unhook (logger_hook_timer);
        logger_hook_timer = NULL;
    }

    logger_config_write ();

    logger_buffer_stop_all (1);

    logger_config_free ();

    return WEECHAT_RC_OK;
}
