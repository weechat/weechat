/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_LOGGER_H
#define WEECHAT_PLUGIN_LOGGER_H

#define weechat_plugin weechat_logger_plugin
#define LOGGER_PLUGIN_NAME "logger"
#define LOGGER_PLUGIN_PRIORITY 15000

#define LOGGER_LEVEL_DEFAULT 9

struct t_gui_buffer;
struct t_logger_buffer;

extern struct t_weechat_plugin *weechat_logger_plugin;

extern struct t_hook *logger_hook_timer;
extern struct t_hook *logger_hook_print;

extern int logger_check_conditions (struct t_gui_buffer *buffer,
                                    const char *conditions);
extern int logger_create_directory (void);
extern char *logger_build_option_name (struct t_gui_buffer *buffer);
extern int logger_get_level_for_buffer (struct t_gui_buffer *buffer);
extern char *logger_get_filename (struct t_gui_buffer *buffer);
extern int logger_print_cb (const void *pointer, void *data,
                            struct t_gui_buffer *buffer,
                            time_t date, int date_usec,
                            int tags_count, const char **tags,
                            int displayed, int highlight,
                            const char *prefix, const char *message);
extern int logger_timer_cb (const void *pointer, void *data,
                            int remaining_calls);

#endif /* WEECHAT_PLUGIN_LOGGER_H */
