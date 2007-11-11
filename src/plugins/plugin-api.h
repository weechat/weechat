/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_PLUGIN_API_H
#define __WEECHAT_PLUGIN_API_H 1

/* strings */
extern void plugin_api_charset_set (struct t_weechat_plugin *, char *);
extern char *plugin_api_iconv_to_internal (struct t_weechat_plugin *, char *,
                                           char *);
extern char *plugin_api_iconv_from_internal (struct t_weechat_plugin *, char *,
                                             char *);
extern char *plugin_api_gettext (struct t_weechat_plugin *, char *);
extern char *plugin_api_ngettext (struct t_weechat_plugin *, char *, char *,
                                  int);
extern int plugin_api_strcasecmp (struct t_weechat_plugin *,char *, char *);
extern int plugin_api_strncasecmp (struct t_weechat_plugin *,char *, char *,
                                   int);
extern char *plugin_api_string_replace (struct t_weechat_plugin *,char *,
                                        char *, char *);
extern char **plugin_api_string_explode (struct t_weechat_plugin *, char *,
                                         char *, int, int, int *);
extern void plugin_api_string_free_exploded (struct t_weechat_plugin *,
                                             char **);

/* directories */
extern int plugin_api_mkdir_home (struct t_weechat_plugin *, char *);
extern void plugin_api_exec_on_files (struct t_weechat_plugin *, char *,
                                          int (*)(char *));

/* display */
extern void plugin_api_printf (struct t_weechat_plugin *, void *,
                               char *, ...);
extern void plugin_api_printf_date (struct t_weechat_plugin *, void *,
                                    time_t, char *, ...);
extern char *plugin_api_prefix (struct t_weechat_plugin *, char *);
extern char *plugin_api_color (struct t_weechat_plugin *, char *);
extern void plugin_api_print_infobar (struct t_weechat_plugin *, int,
                                      char *, ...);
extern void plugin_api_infobar_remove (struct t_weechat_plugin *, int);

/* hooks */
extern struct t_hook *plugin_api_hook_command (struct t_weechat_plugin *,
                                               char *, char *, char *, char *,
                                               char *,
                                               int (*)(void *, int, char **, char **),
                                               void *);
extern struct t_hook *plugin_api_hook_timer (struct t_weechat_plugin *,
                                             long, int,
                                             int (*)(void *), void *);
extern struct t_hook *plugin_api_hook_fd (struct t_weechat_plugin *,
                                          int, int, int, int,
                                          int (*)(void *), void *);
extern struct t_hook *plugin_api_hook_print (struct t_weechat_plugin *,
                                             void *, char *, int,
                                             int (*)(void *, void *, time_t, char *, char *),
                                             void *);
extern struct t_hook *plugin_api_hook_event (struct t_weechat_plugin *, char *,
                                             int (*)(void *, char *, void *),
                                             void *);
extern struct t_hook *plugin_api_hook_config (struct t_weechat_plugin *,
                                              char *, char *,
                                              int (*)(void *, char *, char *, char *),
                                              void *);
extern void plugin_api_unhook (struct t_weechat_plugin *, void *);
extern void plugin_api_unhook_all (struct t_weechat_plugin *);

/* buffers */
extern struct t_gui_buffer *plugin_api_buffer_new (struct t_weechat_plugin *,
                                                   char *, char *);
extern struct t_gui_buffer *plugin_api_buffer_search (struct t_weechat_plugin *,
                                                      char *, char *);
extern void plugin_api_buffer_close (struct t_weechat_plugin *, void *);
extern void plugin_api_buffer_set (struct t_weechat_plugin *, void *, char *,
                                   char *);
extern void plugin_api_buffer_nick_add (struct t_weechat_plugin *, void *,
                                        char *, int, char *, char, char *);
extern void plugin_api_buffer_nick_remove (struct t_weechat_plugin *, char *);

/* command */
extern void plugin_api_command (struct t_weechat_plugin *, void *, char *);

/* infos */
extern char *plugin_api_info_get (struct t_weechat_plugin *, char *);

/* lists */
extern struct t_plugin_list *plugin_api_list_get (struct t_weechat_plugin *,
                                                  char *, void *);
extern int plugin_api_list_next (struct t_weechat_plugin *,
                                 void *);
extern int plugin_api_list_prev (struct t_weechat_plugin *,
                                 void *);
extern char *plugin_api_list_fields (struct t_weechat_plugin *, void *);
extern int plugin_api_list_int (struct t_weechat_plugin *, void *, char *);
extern char *plugin_api_list_string (struct t_weechat_plugin *, void *, char *);
extern void *plugin_api_list_pointer (struct t_weechat_plugin *, void *, char *);
extern time_t plugin_api_list_time (struct t_weechat_plugin *, void *, char *);
extern void plugin_api_list_free (struct t_weechat_plugin *, void *);

/* config */
extern char *plugin_api_config_get (struct t_weechat_plugin *, char *);
extern int plugin_api_config_set (struct t_weechat_plugin *, char *, char *);
extern char *plugin_api_plugin_config_get (struct t_weechat_plugin *, char *);
extern int plugin_api_plugin_config_set (struct t_weechat_plugin *, char *,
                                         char *);

/* log */
extern void plugin_api_log (struct t_weechat_plugin *, char *, char *,
                            char *, ...);

#endif /* plugin-api.h */
