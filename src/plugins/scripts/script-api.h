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
                                    const char *charset);
extern struct t_config_file *script_api_config_new (struct t_weechat_plugin *weechat_plugin,
                                                    struct t_plugin_script *script,
                                                    const char *name,
                                                    int (*callback_reload)(void *data,
                                                                           struct t_config_file *config_file),
                                                    const char *function);
extern struct t_config_section *script_api_config_new_section (struct t_weechat_plugin *weechat_plugin,
                                                               struct t_plugin_script *script,
                                                               struct t_config_file *config_file,
                                                               const char *name,
                                                               int user_can_add_options,
                                                               int user_can_delete_options,
                                                               void (*callback_read)(void *data,
                                                                                     struct t_config_file *config_file,
                                                                                     const char *option_name,
                                                                                     const char *value),
                                                               const char *function_read,
                                                               void (*callback_write)(void *data,
                                                                                      struct t_config_file *config_file,
                                                                                      const char *section_name),
                                                               const char *function_write,
                                                               void (*callback_write_default)(void *data,
                                                                                              struct t_config_file *config_file,
                                                                                              const char *section_name),
                                                               const char *function_write_default,
                                                               int (*callback_create_option)(void *data,
                                                                                             struct t_config_file *config_file,
                                                                                             struct t_config_section *section,
                                                                                             const char *option_name,
                                                                                             const char *value),
                                                               const char *function_create_option);
extern struct t_config_option *script_api_config_new_option (struct t_weechat_plugin *weechat_plugin,
                                                             struct t_plugin_script *script,
                                                             struct t_config_file *config_file,
                                                             struct t_config_section *section,
                                                             const char *name,
                                                             const char *type,
                                                             const char *description,
                                                             const char *string_values,
                                                             int min, int max,
                                                             const char *default_value,
                                                             void (*callback_change)(void *data),
                                                             const char *function);
extern void script_api_config_free (struct t_weechat_plugin *weechat_plugin,
                                    struct t_plugin_script *script,
                                    struct t_config_file *config_file);
extern void script_api_printf (struct t_weechat_plugin *weechat_plugin,
                               struct t_plugin_script *script,
                               struct t_gui_buffer *buffer,
                               const char *format, ...);
extern void script_api_printf_date_tags (struct t_weechat_plugin *weechat_plugin,
                                         struct t_plugin_script *script,
                                         struct t_gui_buffer *buffer,
                                         time_t date, const char *tags,
                                         const char *format, ...);
extern void script_api_printf_y (struct t_weechat_plugin *weechat_plugin,
                                 struct t_plugin_script *script,
                                 struct t_gui_buffer *buffer,
                                 int y, const char *format, ...);
extern void script_api_infobar_printf (struct t_weechat_plugin *weechat_plugin,
                                       struct t_plugin_script *script,
                                       int delay, const char *color_name,
                                       const char *format, ...);
extern void script_api_log_printf (struct t_weechat_plugin *weechat_plugin,
                                   struct t_plugin_script *script,
                                   const char *format, ...);
extern struct t_hook *script_api_hook_command (struct t_weechat_plugin *weechat_plugin,
                                               struct t_plugin_script *script,
                                               const char *command, const char *description,
                                               const char *args, const char *args_description,
                                               const char *completion,
                                               int (*callback)(void *data,
                                                               struct t_gui_buffer *buffer,
                                                               int argc, char **argv,
                                                               char **argv_eol),
                                               const char *function);
extern struct t_hook *script_api_hook_timer (struct t_weechat_plugin *weechat_plugin,
                                             struct t_plugin_script *script,
                                             int interval, int align_second,
                                             int max_calls,
                                             int (*callback)(void *data),
                                             const char *function);
extern struct t_hook *script_api_hook_fd (struct t_weechat_plugin *weechat_plugin,
                                          struct t_plugin_script *script,
                                          int fd, int flag_read,
                                          int flag_write, int flag_exception,
                                          int (*callback)(void *data),
                                          const char *function);
extern struct t_hook *script_api_hook_print (struct t_weechat_plugin *weechat_plugin,
                                             struct t_plugin_script *script,
                                             struct t_gui_buffer *buffer,
                                             const char *tags,
                                             const char *message,
                                             int strip_colors,
                                             int (*callback)(void *data,
                                                             struct t_gui_buffer *buffer,
                                                             time_t date,
                                                             int tags_count,
                                                             char **tags,
                                                             const char *prefix,
                                                             const char *message),
                                             const char *function);
extern struct t_hook *script_api_hook_signal (struct t_weechat_plugin *weechat_plugin,
                                              struct t_plugin_script *script,
                                              const char *signal,
                                              int (*callback)(void *data,
                                                              const char *signal,
                                                              const char *type_data,
                                                              void *signal_data),
                                              const char *function);
extern struct t_hook *script_api_hook_config (struct t_weechat_plugin *weechat_plugin,
                                              struct t_plugin_script *script,
                                              const char *option,
                                              int (*callback)(void *data,
                                                              const char *option,
                                                              const char *value),
                                              const char *function);
extern struct t_hook *script_api_hook_completion (struct t_weechat_plugin *weechat_plugin,
                                                  struct t_plugin_script *script,
                                                  const char *completion,
                                                  int (*callback)(void *data,
                                                                  const char *completion,
                                                                  struct t_gui_buffer *buffer,
                                                                  struct t_weelist *list),
                                                  const char *function);
extern struct t_hook *script_api_hook_modifier (struct t_weechat_plugin *weechat_plugin,
                                                struct t_plugin_script *script,
                                                const char *modifier,
                                                char *(*callback)(void *data,
                                                                  const char *modifier,
                                                                  const char *modifier_data,
                                                                  const char *string),
                                                const char *function);
extern void script_api_unhook (struct t_weechat_plugin *weechat_plugin,
                               struct t_plugin_script *script,
                               struct t_hook *hook);
extern void script_api_unhook_all (struct t_plugin_script *script);
extern struct t_gui_buffer *script_api_buffer_new (struct t_weechat_plugin *weechat_plugin,
                                                   struct t_plugin_script *script,
                                                   const char *category, const char *name,
                                                   int (*input_callback)(void *data,
                                                                         struct t_gui_buffer *buffer,
                                                                         const char *input_data),
                                                   const char *function_input,
                                                   int (*close_callback)(void *data,
                                                                         struct t_gui_buffer *buffer),
                                                   const char *function_close);
extern void script_api_buffer_close (struct t_weechat_plugin *weechat_plugin,
                                     struct t_plugin_script *script,
                                     struct t_gui_buffer *buffer,
                                     int switch_to_another);
extern struct t_gui_bar_item *script_api_bar_item_new (struct t_weechat_plugin *weechat_plugin,
                                                       struct t_plugin_script *script,
                                                       const char *name,
                                                       char *(*build_callback)(void *data,
                                                                               struct t_gui_bar_item *item,
                                                                               struct t_gui_window *window,
                                                                               int max_width,
                                                                               int max_height),
                                                       const char *function_build);
extern void script_api_bar_item_remove (struct t_weechat_plugin *weechat_plugin,
                                        struct t_plugin_script *script,
                                        struct t_gui_bar_item *item);
extern void script_api_command (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script,
                                struct t_gui_buffer *buffer,
                                const char *command);
extern char *script_api_config_get_plugin (struct t_weechat_plugin *weechat_plugin,
                                           struct t_plugin_script *script,
                                           const char *option);
extern int script_api_config_set_plugin (struct t_weechat_plugin *weechat_plugin,
                                         struct t_plugin_script *script,
                                         const char *option, const char *value);

#endif /* script-api.h */
