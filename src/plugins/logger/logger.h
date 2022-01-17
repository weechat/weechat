/*
 * Copyright (C) 2003-2022 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_LOGGER_H
#define WEECHAT_PLUGIN_LOGGER_H

#define weechat_plugin weechat_logger_plugin
#define LOGGER_PLUGIN_NAME "logger"

#define LOGGER_LEVEL_DEFAULT 9

struct t_gui_buffer;
struct t_logger_buffer;

extern struct t_weechat_plugin *weechat_logger_plugin;

extern struct t_hook *logger_hook_timer;
extern struct t_hook *logger_hook_print;

extern char *logger_build_option_name (struct t_gui_buffer *buffer);
extern void logger_set_log_filename (struct t_logger_buffer *logger_buffer);
extern void logger_start_buffer_all (int write_info_line);
extern void logger_flush ();
extern void logger_stop_all (int write_info_line);
extern void logger_adjust_log_filenames ();
extern int logger_print_cb (const void *pointer, void *data,
                            struct t_gui_buffer *buffer, time_t date,
                            int tags_count, const char **tags,
                            int displayed, int highlight,
                            const char *prefix, const char *message);
extern int logger_timer_cb (const void *pointer, void *data,
                            int remaining_calls);

#endif /* WEECHAT_PLUGIN_LOGGER_H */
