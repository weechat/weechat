/*
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

#ifndef WEECHAT_PLUGIN_LOGGER_CONFIG_H
#define WEECHAT_PLUGIN_LOGGER_CONFIG_H

#define LOGGER_CONFIG_NAME "logger"
#define LOGGER_CONFIG_PRIO_NAME (TO_STR(LOGGER_PLUGIN_PRIORITY) "|" LOGGER_CONFIG_NAME)

extern struct t_config_option *logger_config_look_backlog;
extern struct t_config_option *logger_config_look_backlog_conditions;

extern struct t_config_option *logger_config_color_backlog_end;
extern struct t_config_option *logger_config_color_backlog_line;

extern struct t_config_option *logger_config_file_auto_log;
extern struct t_config_option *logger_config_file_color_lines;
extern struct t_config_option *logger_config_file_flush_delay;
extern struct t_config_option *logger_config_file_fsync;
extern struct t_config_option *logger_config_file_info_lines;
extern struct t_config_option *logger_config_file_log_conditions;
extern struct t_config_option *logger_config_file_mask;
extern struct t_config_option *logger_config_file_name_lower_case;
extern struct t_config_option *logger_config_file_nick_prefix;
extern struct t_config_option *logger_config_file_nick_suffix;
extern struct t_config_option *logger_config_file_path;
extern struct t_config_option *logger_config_file_replacement_char;
extern struct t_config_option *logger_config_file_rotation_compression_level;
extern struct t_config_option *logger_config_file_rotation_compression_type;
extern struct t_config_option *logger_config_file_rotation_size_max;
extern struct t_config_option *logger_config_file_time_format;

extern unsigned long long logger_config_rotation_size_max;

extern struct t_config_option *logger_config_get_level (const char *name);
extern int logger_config_set_level (const char *name, const char *value);
extern struct t_config_option *logger_config_get_mask (const char *name);
extern void logger_config_color_lines_change (const void *pointer, void *data,
                                              struct t_config_option *option);
extern int logger_config_init (void);
extern int logger_config_read (void);
extern int logger_config_write (void);
extern void logger_config_free (void);

#endif /* WEECHAT_PLUGIN_LOGGER_CONFIG_H */
