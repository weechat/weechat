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

/* logger-config.c: logger configuration options */


#include <stdlib.h>
#include <limits.h>

#include "../weechat-plugin.h"
#include "logger.h"
#include "logger-config.h"


struct t_config_file *logger_config_file = NULL;

/* logger config, look section */

struct t_config_option *logger_config_look_backlog;

/* logger config, file section */

struct t_config_option *logger_config_file_auto_log;
struct t_config_option *logger_config_file_name_lower_case;
struct t_config_option *logger_config_file_path;
struct t_config_option *logger_config_file_info_lines;
struct t_config_option *logger_config_file_time_format;


/*
 * logger_config_init: init logger configuration file
 *                     return: 1 if ok, 0 if error
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
                                              NULL, NULL, NULL, NULL);
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
        NULL, 0, INT_MAX, "20", NULL, NULL, NULL, NULL, NULL, NULL);
    
    /* file */
    ptr_section = weechat_config_new_section (logger_config_file, "file",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL);
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
        NULL, 0, 0, "on", NULL, NULL, NULL, NULL, NULL, NULL);
    logger_config_file_name_lower_case = weechat_config_new_option (
        logger_config_file, ptr_section,
        "name_lower_case", "boolean",
        N_("use only lower case for log filenames"),
        NULL, 0, 0, "on", NULL, NULL, NULL, NULL, NULL, NULL);
    logger_config_file_path = weechat_config_new_option (
        logger_config_file, ptr_section,
        "path", "string",
        N_("path for WeeChat log files ('%h' will be replaced by WeeChat "
           "home, ~/.weechat by default)"),
        NULL, 0, 0, "%h/logs/", NULL, NULL, NULL, NULL, NULL, NULL);
    logger_config_file_info_lines = weechat_config_new_option (
        logger_config_file, ptr_section,
        "info_lines", "boolean",
        N_("write information line in log file when log starts or ends for a "
           "buffer"),
        NULL, 0, 0, "off", NULL, NULL, NULL, NULL, NULL, NULL);
    logger_config_file_time_format = weechat_config_new_option (
        logger_config_file, ptr_section,
        "time_format", "string",
        N_("timestamp used in log files (see man strftime for date/time "
           "specifiers)"),
        NULL, 0, 0, "%Y-%m-%d %H:%M:%S", NULL, NULL, NULL, NULL, NULL, NULL);
    
    return 1;
}

/*
 * logger_config_read: read logger configuration file
 */

int
logger_config_read ()
{
    return weechat_config_read (logger_config_file);
}

/*
 * logger_config_write: write logger configuration file
 */

int
logger_config_write ()
{
    return weechat_config_write (logger_config_file);
}

/*
 * logger_config_free: free logger configuration
 */

void
logger_config_free ()
{
    weechat_config_free (logger_config_file);
}
