/*
 * logger-config.c - logger configuration options (file logger.conf)
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <limits.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-config.h"


struct t_config_file *logger_config_file = NULL;
struct t_config_section *logger_config_section_level = NULL;
struct t_config_section *logger_config_section_mask = NULL;

int logger_config_loading = 0;

/* logger config, look section */

struct t_config_option *logger_config_look_backlog;

/* logger config, color section */

struct t_config_option *logger_config_color_backlog_line;
struct t_config_option *logger_config_color_backlog_end;

/* logger config, file section */

struct t_config_option *logger_config_file_auto_log;
struct t_config_option *logger_config_file_flush_delay;
struct t_config_option *logger_config_file_name_lower_case;
struct t_config_option *logger_config_file_path;
struct t_config_option *logger_config_file_mask;
struct t_config_option *logger_config_file_replacement_char;
struct t_config_option *logger_config_file_info_lines;
struct t_config_option *logger_config_file_time_format;


/*
 * Callback for changes on option that require a restart of logging for all
 * buffers.
 */

void
logger_config_change_file_option_restart_log (void *data,
                                              struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (!logger_config_loading)
        logger_adjust_log_filenames ();
}

/*
 * Callback for changes on option "logger.file.flush_delay".
 */

void
logger_config_flush_delay_change (void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (logger_config_loading)
        return;

    if (logger_timer)
    {
        if (weechat_logger_plugin->debug)
        {
            weechat_printf_tags (NULL,
                                 "no_log",
                                 "%s: stopping timer", LOGGER_PLUGIN_NAME);
        }
        weechat_unhook (logger_timer);
        logger_timer = NULL;
    }

    if (weechat_config_integer (logger_config_file_flush_delay) > 0)
    {
        if (weechat_logger_plugin->debug)
        {
            weechat_printf_tags (NULL,
                                 "no_log",
                                 "%s: starting timer (interval: %d seconds)",
                                 LOGGER_PLUGIN_NAME,
                                 weechat_config_integer (logger_config_file_flush_delay));
        }
        logger_timer = weechat_hook_timer (weechat_config_integer (logger_config_file_flush_delay) * 1000,
                                           0, 0,
                                           &logger_timer_cb, NULL);
    }
}

/*
 * Callback for changes on a level option.
 */

void
logger_config_level_change (void *data,
                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (!logger_config_loading)
        logger_start_buffer_all (1);
}

/*
 * Callback called when an option is deleted in section "level".
 */

int
logger_config_level_delete_option (void *data,
                                   struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    weechat_config_option_free (option);

    logger_start_buffer_all (1);

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Callback called when an option is created in section "level".
 */

int
logger_config_level_create_option (void *data,
                                   struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   const char *option_name,
                                   const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = weechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = weechat_config_option_set (ptr_option, value, 1);
            else
            {
                weechat_config_option_free (ptr_option);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = weechat_config_new_option (
                    config_file, section,
                    option_name, "integer",
                    _("logging level for this buffer (0 = logging disabled, "
                      "1 = a few messages (most important) .. 9 = all messages)"),
                      NULL, 0, 9, "9", value, 0, NULL, NULL,
                    &logger_config_level_change, NULL,
                    NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (!logger_config_loading)
        logger_start_buffer_all (1);

    return rc;
}

/*
 * Gets a level option.
 */

struct t_config_option *
logger_config_get_level (const char *name)
{
    return weechat_config_search_option (logger_config_file,
                                         logger_config_section_level,
                                         name);
}

/*
 * Sets a level option.
 */

int
logger_config_set_level (const char *name, const char *value)
{
    return logger_config_level_create_option (NULL,
                                              logger_config_file,
                                              logger_config_section_level,
                                              name,
                                              value);
}

/*
 * Callback for changes on a mask option.
 */

void
logger_config_mask_change (void *data,
                           struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (!logger_config_loading)
        logger_adjust_log_filenames ();
}

/*
 * Callback called when an option is deleted in section "mask".
 */

int
logger_config_mask_delete_option (void *data,
                                  struct t_config_file *config_file,
                                  struct t_config_section *section,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    weechat_config_option_free (option);

    logger_adjust_log_filenames ();

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Callback called when an option is created in section "mask".
 */

int
logger_config_mask_create_option (void *data,
                                  struct t_config_file *config_file,
                                  struct t_config_section *section,
                                  const char *option_name,
                                  const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = weechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = weechat_config_option_set (ptr_option, value, 1);
            else
            {
                weechat_config_option_free (ptr_option);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = weechat_config_new_option (
                    config_file, section,
                    option_name, "string",
                    _("file mask for log file; local buffer variables are "
                      "permitted"),
                    NULL, 0, 0, "", value, 0, NULL, NULL,
                    &logger_config_mask_change, NULL,
                    NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (!logger_config_loading)
        logger_adjust_log_filenames ();

    return rc;
}

/*
 * Gets a mask option.
 */

struct t_config_option *
logger_config_get_mask (const char *name)
{
    return weechat_config_search_option (logger_config_file,
                                         logger_config_section_mask,
                                         name);
}

/*
 * Initializes logger configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
logger_config_init ()
{
    struct t_config_section *ptr_section;

    logger_config_file = weechat_config_new (LOGGER_CONFIG_NAME,
                                             NULL, NULL);
    if (!logger_config_file)
        return 0;

    /* look */
    ptr_section = weechat_config_new_section (logger_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (logger_config_file);
        return 0;
    }

    logger_config_look_backlog = weechat_config_new_option (
        logger_config_file, ptr_section,
        "backlog", "integer",
        N_("maximum number of lines to display from log file when creating "
           "new buffer (0 = no backlog)"),
        NULL, 0, INT_MAX, "20", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* color */
    ptr_section = weechat_config_new_section (logger_config_file, "color",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (logger_config_file);
        return 0;
    }

    logger_config_color_backlog_line = weechat_config_new_option (
        logger_config_file, ptr_section,
        "backlog_line", "color",
        N_("color for backlog lines"),
        NULL, -1, 0, "darkgray", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    logger_config_color_backlog_end = weechat_config_new_option (
        logger_config_file, ptr_section,
        "backlog_end", "color",
        N_("color for line ending the backlog"),
        NULL, -1, 0, "darkgray", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* file */
    ptr_section = weechat_config_new_section (logger_config_file, "file",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (logger_config_file);
        return 0;
    }

    logger_config_file_auto_log = weechat_config_new_option (
        logger_config_file, ptr_section,
        "auto_log", "boolean",
        N_("automatically save content of buffers to files (unless a buffer "
           "disables log)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    logger_config_file_flush_delay = weechat_config_new_option (
        logger_config_file, ptr_section,
        "flush_delay", "integer",
        N_("number of seconds between flush of log files (0 = write in log "
           "files immediately for each line printed)"),
        NULL, 0, 3600, "120", NULL, 0, NULL, NULL,
        &logger_config_flush_delay_change, NULL, NULL, NULL);
    logger_config_file_name_lower_case = weechat_config_new_option (
        logger_config_file, ptr_section,
        "name_lower_case", "boolean",
        N_("use only lower case for log filenames"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        &logger_config_change_file_option_restart_log, NULL, NULL, NULL);
    logger_config_file_path = weechat_config_new_option (
        logger_config_file, ptr_section,
        "path", "string",
        N_("path for WeeChat log files; \"%h\" at beginning of string is "
           "replaced by WeeChat home (\"~/.weechat\" by default); date "
           "specifiers are permitted (see man strftime)"),
        NULL, 0, 0, "%h/logs/", NULL, 0, NULL, NULL,
        &logger_config_change_file_option_restart_log, NULL, NULL, NULL);
    logger_config_file_mask = weechat_config_new_option (
        logger_config_file, ptr_section,
        "mask", "string",
        N_("default file name mask for log files (format is "
           "\"directory/to/file\" or \"file\", without first \"/\" because "
           "\"path\" option is used to build complete path to file); local "
           "buffer variables are permitted; date specifiers are permitted "
           "(see man strftime)"),
        NULL, 0, 0, "$plugin.$name.weechatlog", NULL, 0, NULL, NULL,
        &logger_config_change_file_option_restart_log, NULL, NULL, NULL);
    logger_config_file_replacement_char = weechat_config_new_option (
        logger_config_file, ptr_section,
        "replacement_char", "string",
        N_("replacement char for special chars in filename built with mask "
           "(like directory delimiter)"),
        NULL, 0, 0, "_", NULL, 0, NULL, NULL,
        &logger_config_change_file_option_restart_log, NULL, NULL, NULL);
    logger_config_file_info_lines = weechat_config_new_option (
        logger_config_file, ptr_section,
        "info_lines", "boolean",
        N_("write information line in log file when log starts or ends for a "
           "buffer"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    logger_config_file_time_format = weechat_config_new_option (
        logger_config_file, ptr_section,
        "time_format", "string",
        N_("timestamp used in log files (see man strftime for date/time "
           "specifiers)"),
        NULL, 0, 0, "%Y-%m-%d %H:%M:%S", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* level */
    ptr_section = weechat_config_new_section (logger_config_file, "level",
                                              1, 1,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL,
                                              &logger_config_level_create_option, NULL,
                                              &logger_config_level_delete_option, NULL);
    if (!ptr_section)
    {
        weechat_config_free (logger_config_file);
        return 0;
    }

    logger_config_section_level = ptr_section;

    /* mask */
    ptr_section = weechat_config_new_section (logger_config_file, "mask",
                                              1, 1,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL,
                                              &logger_config_mask_create_option, NULL,
                                              &logger_config_mask_delete_option, NULL);
    if (!ptr_section)
    {
        weechat_config_free (logger_config_file);
        return 0;
    }

    logger_config_section_mask = ptr_section;

    return 1;
}

/*
 * Reads logger configuration file.
 */

int
logger_config_read ()
{
    int rc;

    logger_config_loading = 1;
    rc = weechat_config_read (logger_config_file);
    logger_config_loading = 0;

    logger_config_flush_delay_change (NULL, NULL);

    return rc;
}

/*
 * Writes logger configuration file.
 */

int
logger_config_write ()
{
    return weechat_config_write (logger_config_file);
}

/*
 * Frees logger configuration.
 */

void
logger_config_free ()
{
    weechat_config_free (logger_config_file);
}
