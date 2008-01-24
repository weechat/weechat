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

#ifndef __WEECHAT_SCRIPT_API_H
#define __WEECHAT_SCRIPT_API_H 1

extern void script_api_charset_set (struct t_plugin_script *script,
                                    char *charset);
extern void script_api_printf (struct t_weechat_plugin *weechat_plugin,
                               struct t_plugin_script *script,
                               struct t_gui_buffer *buffer,
                               char *format, ...);
extern void script_api_infobar_printf (struct t_weechat_plugin *weechat_plugin,
                                       struct t_plugin_script *script,
                                       int delay, char *color_name,
                                       char *format, ...);
extern void script_api_log_printf (struct t_weechat_plugin *weechat_plugin,
                                   struct t_plugin_script *script,
                                   char *format, ...);
extern struct t_hook *script_api_hook_command (struct t_weechat_plugin *weechat_plugin,
                                               struct t_plugin_script *script,
                                               char *command, char *description,
                                               char *args, char *args_description,
                                               char *completion,
                                               int (*callback)(void *data,
                                                               struct t_gui_buffer *buffer,
                                                               int argc, char **argv,
                                                               char **argv_eol),
                                               char *function);
extern struct t_hook *script_api_hook_timer (struct t_weechat_plugin *weechat_plugin,
                                             struct t_plugin_script *script,
                                             int interval, int align_second,
                                             int max_calls,
                                             int (*callback)(void *data),
                                             char *function);
extern struct t_hook *script_api_hook_fd (struct t_weechat_plugin *weechat_plugin,
                                          struct t_plugin_script *script,
                                          int fd, int flag_read,
                                          int flag_write, int flag_exception,
                                          int (*callback)(void *data),
                                          char *function);
extern struct t_hook *script_api_hook_print (struct t_weechat_plugin *weechat_plugin,
                                             struct t_plugin_script *script,
                                             struct t_gui_buffer *buffer,
                                             char *message, int strip_colors,
                                             int (*callback)(void *data,
                                                             struct t_gui_buffer *buffer,
                                                             time_t date,
                                                             char *prefix,
                                                             char *message),
                                             char *function);
extern struct t_hook *script_api_hook_signal (struct t_weechat_plugin *weechat_plugin,
                                              struct t_plugin_script *script,
                                              char *signal,
                                              int (*callback)(void *data,
                                                              char *signal,
                                                              char *type_data,
                                                              void *signal_data),
                                              char *function);
extern struct t_hook *script_api_hook_config (struct t_weechat_plugin *weechat_plugin,
                                              struct t_plugin_script *script,
                                              char *type, char *option,
                                              int (*callback)(void *data, char *type,
                                                              char *option,
                                                              char *value),
                                              char *function);
extern struct t_hook *script_api_hook_completion (struct t_weechat_plugin *weechat_plugin,
                                                  struct t_plugin_script *script,
                                                  char *completion,
                                                  int (*callback)(void *data,
                                                                  char *completion,
                                                                  struct t_gui_buffer *buffer,
                                                                  struct t_weelist *list),
                                                  char *function);
extern struct t_hook *script_api_hook_modifier (struct t_weechat_plugin *weechat_plugin,
                                                struct t_plugin_script *script,
                                                char *modifier,
                                                char *(*callback)(void *data,
                                                                  char *modifier,
                                                                  char *modifier_data,
                                                                  char *string),
                                                char *function);
extern int script_api_unhook (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script,
                              struct t_hook *hook);
extern void script_api_unhook_all (struct t_weechat_plugin *weechat_plugin,
                                   struct t_plugin_script *script);
struct t_gui_buffer *script_api_buffer_new (struct t_weechat_plugin *weechat_plugin,
                                            struct t_plugin_script *script,
                                            char *category, char *name,
                                            int (*callback)(void *data,
                                                            struct t_gui_buffer *buffer,
                                                            char *input_data),
                                            char *function);
extern void script_api_buffer_close (struct t_weechat_plugin *weechat_plugin,
                                     struct t_plugin_script *script,
                                     struct t_gui_buffer *buffer,
                                     int switch_to_another);
extern void script_api_command (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                struct t_gui_buffer *buffer,
                                char *command);
extern char *script_api_config_get_plugin (struct t_weechat_plugin *weechat_plugin,
                                           struct t_plugin_script *script,
                                           char *option);
extern int script_api_config_set_plugin (struct t_weechat_plugin *weechat_plugin,
                                         struct t_plugin_script *script,
                                         char *option, char *value);

#endif /* script-api.h */
