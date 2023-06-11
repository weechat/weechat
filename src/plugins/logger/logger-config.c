/*
 * logger-config.c - logger configuration options (file logger.conf)
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

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-config.h"
#include "logger-buffer.h"


struct t_config_file *logger_config_file = NULL;

/* sections */

struct t_config_section *logger_config_section_look = NULL;
struct t_config_section *logger_config_section_color = NULL;
struct t_config_section *logger_config_section_file = NULL;
struct t_config_section *logger_config_section_level = NULL;
struct t_config_section *logger_config_section_mask = NULL;

int logger_config_loading = 0;

/* logger config, look section */

struct t_config_option *logger_config_look_backlog = NULL;
struct t_config_option *logger_config_look_backlog_conditions = NULL;

/* logger config, color section */

struct t_config_option *logger_config_color_backlog_end = NULL;
struct t_config_option *logger_config_color_backlog_line = NULL;

/* logger config, file section */

struct t_config_option *logger_config_file_auto_log = NULL;
struct t_config_option *logger_config_file_color_lines = NULL;
struct t_config_option *logger_config_file_flush_delay = NULL;
struct t_config_option *logger_config_file_fsync = NULL;
struct t_config_option *logger_config_file_info_lines = NULL;
struct t_config_option *logger_config_file_log_conditions = NULL;
struct t_config_option *logger_config_file_mask = NULL;
struct t_config_option *logger_config_file_name_lower_case = NULL;
struct t_config_option *logger_config_file_nick_prefix = NULL;
struct t_config_option *logger_config_file_nick_suffix = NULL;
struct t_config_option *logger_config_file_path = NULL;
struct t_config_option *logger_config_file_replacement_char = NULL;
struct t_config_option *logger_config_file_rotation_compression_level = NULL;
struct t_config_option *logger_config_file_rotation_compression_type = NULL;
struct t_config_option *logger_config_file_rotation_size_max = NULL;
struct t_config_option *logger_config_file_time_format = NULL;

/* other */

unsigned long long logger_config_rotation_size_max = 0;


/*
 * Callback for changes on option that require a restart of logging for all
 * buffers.
 */

void
logger_config_change_file_option_restart_log (const void *pointer, void *data,
                                              struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (!logger_config_loading)
        logger_buffer_adjust_log_filenames ();
}

/*
 * Callback for changes on option "logger.file.color_lines".
 */

void
logger_config_color_lines_change (const void *pointer, void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (logger_config_loading)
        return;

    if (logger_hook_print)
        weechat_unhook (logger_hook_print);

    logger_hook_print = weechat_hook_print (
        NULL, NULL, NULL,
        (weechat_config_boolean (logger_config_file_color_lines)) ? 0 : 1,
        &logger_print_cb, NULL, NULL);
}

/*
 * Callback for changes on option "logger.file.flush_delay".
 */

void
logger_config_flush_delay_change (const void *pointer, void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (logger_config_loading)
        return;

    if (logger_hook_timer)
    {
        if (weechat_logger_plugin->debug)
        {
            weechat_printf_date_tags (
                NULL, 0, "no_log",
                "%s: stopping timer", LOGGER_PLUGIN_NAME);
        }
        weechat_unhook (logger_hook_timer);
        logger_hook_timer = NULL;
    }

    if (weechat_config_integer (logger_config_file_flush_delay) > 0)
    {
        if (weechat_logger_plugin->debug)
        {
            weechat_printf_date_tags (
                NULL, 0, "no_log",
                "%s: starting timer (interval: %d seconds)",
                LOGGER_PLUGIN_NAME,
                weechat_config_integer (logger_config_file_flush_delay));
        }
        logger_hook_timer = weechat_hook_timer (
            weechat_config_integer (logger_config_file_flush_delay) * 1000,
            0, 0,
            &logger_timer_cb, NULL, NULL);
    }
}

/*
 * Callback called when option "logger.file.rotation_size_max" is changed,
 * to check if value is valid.
 *
 * Returns:
 *   1: value is OK
 *   0: value is invalid
 */

int
logger_config_rotation_size_max_check (const void *pointer, void *data,
                                       struct t_config_option *option,
                                       const char *value)
{
    unsigned long long size;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (!value || !value[0])
        return 0;

    if (strcmp (value, "0") == 0)
        return 1;

    size = weechat_string_parse_size (value);

    return (size > 0) ? 1 : 0;
}

/*
 * Callback called when option "logger.file.rotation_size_max" is changed.
 *
 * Returns:
 *   1: value is OK
 *   0: value is invalid
 */

void
logger_config_rotation_size_max_change (const void *pointer, void *data,
                                        struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    logger_config_rotation_size_max = weechat_string_parse_size (
        weechat_config_string (logger_config_file_rotation_size_max));
}

/*
 * Callback for changes on a level option.
 */

void
logger_config_level_change (const void *pointer, void *data,
                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (!logger_config_loading)
        logger_buffer_start_all (1);
}

/*
 * Callback called when an option is deleted in section "level".
 */

int
logger_config_level_delete_option (const void *pointer, void *data,
                                   struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    weechat_config_option_free (option);

    logger_buffer_start_all (1);

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Callback called when an option is created in section "level".
 */

int
logger_config_level_create_option (const void *pointer, void *data,
                                   struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   const char *option_name,
                                   const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) pointer;
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
                      NULL, 0, 9, "9", value, 0,
                    NULL, NULL, NULL,
                    &logger_config_level_change, NULL,  NULL,
                    NULL, NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (!logger_config_loading)
        logger_buffer_start_all (1);

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
    return logger_config_level_create_option (NULL, NULL,
                                              logger_config_file,
                                              logger_config_section_level,
                                              name,
                                              value);
}

/*
 * Callback for changes on a mask option.
 */

void
logger_config_mask_change (const void *pointer, void *data,
                           struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (!logger_config_loading)
        logger_buffer_adjust_log_filenames ();
}

/*
 * Callback called when an option is deleted in section "mask".
 */

int
logger_config_mask_delete_option (const void *pointer, void *data,
                                  struct t_config_file *config_file,
                                  struct t_config_section *section,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    weechat_config_option_free (option);

    logger_buffer_adjust_log_filenames ();

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Callback called when an option is created in section "mask".
 */

int
logger_config_mask_create_option (const void *pointer, void *data,
                                  struct t_config_file *config_file,
                                  struct t_config_section *section,
                                  const char *option_name,
                                  const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) pointer;
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
                    NULL, 0, 0, "", value, 0,
                    NULL, NULL, NULL,
                    &logger_config_mask_change, NULL, NULL,
                    NULL, NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (!logger_config_loading)
        logger_buffer_adjust_log_filenames ();

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
    logger_config_file = weechat_config_new (LOGGER_CONFIG_PRIO_NAME,
                                             NULL, NULL, NULL);
    if (!logger_config_file)
        return 0;

    /* look */
    logger_config_section_look = weechat_config_new_section (
        logger_config_file, "look",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (logger_config_section_look)
    {
        logger_config_look_backlog = weechat_config_new_option (
            logger_config_file, logger_config_section_look,
            "backlog", "integer",
            N_("maximum number of lines to display from log file when creating "
               "new buffer (0 = no backlog)"),
            NULL, 0, INT_MAX, "20", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        logger_config_look_backlog_conditions = weechat_config_new_option (
            logger_config_file, logger_config_section_look,
            "backlog_conditions", "string",
            N_("conditions to display the backlog "
               "(note: content is evaluated, see /help eval); "
               "empty value displays the backlog on all buffers; "
               "for example to display backlog on private buffers only: "
               "\"${type} == private\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* color */
    logger_config_section_color = weechat_config_new_section (
        logger_config_file, "color",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (logger_config_section_color)
    {
        logger_config_color_backlog_end = weechat_config_new_option (
            logger_config_file, logger_config_section_color,
            "backlog_end", "color",
            N_("color for line ending the backlog"),
            NULL, -1, 0, "246", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        logger_config_color_backlog_line = weechat_config_new_option (
            logger_config_file, logger_config_section_color,
            "backlog_line", "color",
            N_("color for backlog lines, used only if the option "
               "logger.file.color_lines is off"),
            NULL, -1, 0, "246", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* file */
    logger_config_section_file = weechat_config_new_section (
        logger_config_file, "file",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (logger_config_section_file)
    {
        logger_config_file_auto_log = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "auto_log", "boolean",
            N_("automatically save content of buffers to files (unless a buffer "
               "disables log); if disabled, logging is disabled on all buffers"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        logger_config_file_color_lines = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "color_lines", "boolean",
            N_("use ANSI color codes in lines written in log files and display "
               "backlog lines with these colors"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &logger_config_color_lines_change, NULL, NULL,
            NULL, NULL, NULL);
        logger_config_file_flush_delay = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "flush_delay", "integer",
            N_("number of seconds between flush of log files (0 = write in log "
               "files immediately for each line printed)"),
            NULL, 0, 3600, "120", NULL, 0,
            NULL, NULL, NULL,
            &logger_config_flush_delay_change, NULL, NULL,
            NULL, NULL, NULL);
        logger_config_file_fsync = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "fsync", "boolean",
            N_("use fsync to synchronize the log file with the storage device "
               "after the flush (see man fsync); this is slower but should "
               "prevent any data loss in case of power failure during the save "
               "of log file"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        logger_config_file_info_lines = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "info_lines", "boolean",
            N_("write information line in log file when log starts or ends for "
               "a buffer"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        logger_config_file_log_conditions = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "log_conditions", "string",
            N_("conditions to save content of buffers to files "
               "(note: content is evaluated, see /help eval); "
               "empty value saves content on all buffers; "
               "for example to log only private buffers: "
               "\"${type} == private\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        logger_config_file_mask = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "mask", "string",
            N_("default file name mask for log files (format is "
               "\"directory/to/file\" or \"file\", without first \"/\" because "
               "\"path\" option is used to build complete path to file); local "
               "buffer variables are permitted (you should use only variables "
               "that are defined on all buffers, so for example you should NOT "
               "use $server nor $channel); date specifiers are permitted "
               "(see man strftime)"),
            NULL, 0, 0, "$plugin.$name.weechatlog", NULL, 0,
            NULL, NULL, NULL,
            &logger_config_change_file_option_restart_log, NULL, NULL,
            NULL, NULL, NULL);
        logger_config_file_name_lower_case = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "name_lower_case", "boolean",
            N_("use only lower case for log filenames"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &logger_config_change_file_option_restart_log, NULL, NULL,
            NULL, NULL, NULL);
        logger_config_file_nick_prefix = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "nick_prefix", "string",
            N_("text to write before nick in prefix of message, example: \"<\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        logger_config_file_nick_suffix = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "nick_suffix", "string",
            N_("text to write after nick in prefix of message, example: \">\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        logger_config_file_path = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "path", "string",
            N_("path for WeeChat log files; "
               "date specifiers are permitted (see man strftime) "
               "(path is evaluated, see function string_eval_path_home in "
               "plugin API reference)"),
            NULL, 0, 0, "${weechat_data_dir}/logs", NULL, 0,
            NULL, NULL, NULL,
            &logger_config_change_file_option_restart_log, NULL, NULL,
            NULL, NULL, NULL);
        logger_config_file_replacement_char = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "replacement_char", "string",
            N_("replacement char for special chars in filename built with mask "
               "(like directory delimiter)"),
            NULL, 0, 0, "_", NULL, 0,
            NULL, NULL, NULL,
            &logger_config_change_file_option_restart_log, NULL, NULL,
            NULL, NULL, NULL);
        logger_config_file_rotation_compression_level = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "rotation_compression_level", "integer",
            N_("compression level for rotated log files (with extension \".1\", "
               "\".2\", etc.), if option logger.file.rotation_compression_type "
               "is enabled: 1 = low compression / fast ... 100 = best "
               "compression / slow; the value is a percentage converted to "
               "1-9 for gzip and 1-19 for zstd; the default value is "
               "recommended, it offers a good compromise between compression "
               "and speed"),
            NULL, 1, 100, "20", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        logger_config_file_rotation_compression_type = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "rotation_compression_type", "integer",
            N_("compression type for rotated log files; if set to \"none\", "
               "rotated log files are not compressed; WARNING: if rotation was "
               "enabled with another type of compression (or no compression), "
               "you must first unload the logger plugin, compress files with the "
               "new type (or decompress files), then change the option in "
               "logger.conf, then load the logger plugin"),
            "none|gzip|zstd", 0, 0, "none", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        logger_config_file_rotation_size_max = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "rotation_size_max", "string",
            N_("when this size is reached, a rotation of log files is performed: "
               "the existing rotated log files are renamed (.1 becomes .2, .2 "
               "becomes .3, etc.) and the current file is renamed with extension "
               ".1; an integer number with a suffix is allowed: b = bytes "
               "(default if no unit given), k = kilobytes, m = megabytes, "
               "g = gigabytes, t = terabytes; example: \"2g\" causes a rotation "
               "if the file size is > 2,000,000,000 bytes; if set to \"0\", "
               "no rotation is performed (unlimited log size); WARNING: before "
               "changing this option, you should first set the compression type "
               "via option logger.file.rotation_compression_type"),
            NULL, 0, 0, "0", NULL, 0,
            &logger_config_rotation_size_max_check, NULL, NULL,
            &logger_config_rotation_size_max_change, NULL, NULL,
            NULL, NULL, NULL);
        logger_config_file_time_format = weechat_config_new_option (
            logger_config_file, logger_config_section_file,
            "time_format", "string",
            N_("timestamp used in log files (see man strftime for date/time "
               "specifiers)"),
            NULL, 0, 0, "%Y-%m-%d %H:%M:%S", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* level */
    logger_config_section_level = weechat_config_new_section (
        logger_config_file, "level",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &logger_config_level_create_option, NULL, NULL,
        &logger_config_level_delete_option, NULL, NULL);

    /* mask */
    logger_config_section_mask = weechat_config_new_section (
        logger_config_file, "mask",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &logger_config_mask_create_option, NULL, NULL,
        &logger_config_mask_delete_option, NULL, NULL);

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

    logger_config_flush_delay_change (NULL, NULL, NULL);

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
