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


#ifndef __WEECHAT_LOGGER_CONFIG_H
#define __WEECHAT_LOGGER_CONFIG_H 1

#define LOGGER_CONFIG_NAME "logger"


extern struct t_config_option *logger_config_look_backlog;

extern struct t_config_option *logger_config_file_auto_log;
extern struct t_config_option *logger_config_file_name_lower_case;
extern struct t_config_option *logger_config_file_path;
extern struct t_config_option *logger_config_file_info_lines;
extern struct t_config_option *logger_config_file_time_format;

extern int logger_config_init ();
extern int logger_config_read ();
extern int logger_config_write ();
extern void logger_config_free ();

#endif /* logger-config.h */
