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
extern char *plugin_api_strcasestr (struct t_weechat_plugin *,char *, char *);
extern char *plugin_api_string_replace (struct t_weechat_plugin *,char *,
                                        char *, char *);
extern char **plugin_api_string_explode (struct t_weechat_plugin *, char *,
                                         char *, int, int, int *);
extern void plugin_api_string_free_exploded (struct t_weechat_plugin *,
                                             char **);
extern char **plugin_api_string_split_command (struct t_weechat_plugin *,
                                               char *, char);
extern void plugin_api_string_free_splitted_command (struct t_weechat_plugin *,
                                                     char **);

/* UTF-8 strings */
extern int plugin_api_utf8_has_8bits (struct t_weechat_plugin *, char *);
extern int plugin_api_utf8_is_valid (struct t_weechat_plugin *, char *, char **);
extern void plugin_api_utf8_normalize (struct t_weechat_plugin *, char *, char);
extern char *plugin_api_utf8_prev_char (struct t_weechat_plugin *, char *, char *);
extern char *plugin_api_utf8_next_char (struct t_weechat_plugin *, char *);
extern int plugin_api_utf8_char_size (struct t_weechat_plugin *, char *);
extern int plugin_api_utf8_strlen (struct t_weechat_plugin *, char *);
extern int plugin_api_utf8_strnlen (struct t_weechat_plugin *, char *, int);
extern int plugin_api_utf8_strlen_screen (struct t_weechat_plugin *, char *);
extern int plugin_api_utf8_charcasecmp (struct t_weechat_plugin *, char *, char *);
extern int plugin_api_utf8_char_size_screen (struct t_weechat_plugin *, char *);
extern char *plugin_api_utf8_add_offset (struct t_weechat_plugin *, char *, int);
extern int plugin_api_utf8_real_pos (struct t_weechat_plugin *, char *, int);
extern int plugin_api_utf8_pos (struct t_weechat_plugin *, char *, int);

/* directories */
extern int plugin_api_mkdir_home (struct t_weechat_plugin *, char *, int);
extern int plugin_api_mkdir (struct t_weechat_plugin *, char *, int);
extern void plugin_api_exec_on_files (struct t_weechat_plugin *, char *,
                                          int (*)(char *));

/* util */
extern long plugin_api_timeval_diff (struct t_weechat_plugin *, void *, void *);;

/* lists */
extern struct t_weelist *plugin_api_list_new( struct t_weechat_plugin *);
extern char *plugin_api_list_add (struct t_weechat_plugin *, void *, char *,
                                  char *);
extern struct t_weelist_item *plugin_api_list_search (struct t_weechat_plugin *,
                                                      void *, char *);
extern struct t_weelist_item *plugin_api_list_casesearch (struct t_weechat_plugin *,
                                                          void *, char *);
extern struct t_weelist_item *plugin_api_list_get (struct t_weechat_plugin *,
                                                   void *, int);
extern struct t_weelist_item *plugin_api_list_next (struct t_weechat_plugin *,
                                                    void *);
extern struct t_weelist_item *plugin_api_list_prev (struct t_weechat_plugin *,
                                                    void *);
extern char *plugin_api_list_string (struct t_weechat_plugin *, void *);
extern int plugin_api_list_size (struct t_weechat_plugin *, void *);
extern void plugin_api_list_remove (struct t_weechat_plugin *, void *, void *);
extern void plugin_api_list_remove_all (struct t_weechat_plugin *, void *);
extern void plugin_api_list_free (struct t_weechat_plugin *, void *);

/* config */
extern struct t_config_file *plugin_api_config_new (struct t_weechat_plugin *,
                                                    char *);
extern struct t_config_section *plugin_api_config_new_section (struct t_weechat_plugin *,
                                                               void *, char *,
                                                               void (*)(void *, char *, char *),
                                                               void (*)(void *, char *),
                                                               void (*)(void *, char *));
extern struct t_config_section *plugin_api_config_search_section (struct t_weechat_plugin *,
                                                                  void *, char *);
extern struct t_config_option *plugin_api_config_new_option (struct t_weechat_plugin *,
                                                             void *, char *,
                                                             char *, char *,
                                                             char *, int, int,
                                                             char *,
                                                             void (*)());
extern struct t_config_option *plugin_api_config_search_option (struct t_weechat_plugin *,
                                                                void *, void *,
                                                                char *);
extern int plugin_api_config_option_set (struct t_weechat_plugin *, void *,
                                         char *);
extern char plugin_api_config_string_to_boolean (struct t_weechat_plugin *,
                                                 char *);
extern char plugin_api_config_boolean (struct t_weechat_plugin *, void *);
extern int plugin_api_config_integer (struct t_weechat_plugin *, void *);
extern char *plugin_api_config_string (struct t_weechat_plugin *, void *);
extern int plugin_api_config_color (struct t_weechat_plugin *, void *);
extern int plugin_api_config_read (struct t_weechat_plugin *, void *);
extern int plugin_api_config_reload (struct t_weechat_plugin *, void *);
extern int plugin_api_config_write (struct t_weechat_plugin *, void *);
extern void plugin_api_config_write_line (struct t_weechat_plugin *, void *,
                                          char *, char *, ...);
extern void plugin_api_config_free (struct t_weechat_plugin *, void *);
extern struct t_config_option *plugin_api_config_get (struct t_weechat_plugin *,
                                                      char *);
extern char *plugin_api_plugin_config_get (struct t_weechat_plugin *, char *);
extern int plugin_api_plugin_config_set (struct t_weechat_plugin *, char *,
                                         char *);

/* display */
extern char *plugin_api_prefix (struct t_weechat_plugin *, char *);
extern char *plugin_api_color (struct t_weechat_plugin *, char *);
extern void plugin_api_printf (struct t_weechat_plugin *, void *,
                               char *, ...);
extern void plugin_api_printf_date (struct t_weechat_plugin *, void *,
                                    time_t, char *, ...);
extern void plugin_api_log_printf (struct t_weechat_plugin *, char *, ...);
extern void plugin_api_infobar_printf (struct t_weechat_plugin *, int, char *,
                                       char *, ...);
extern void plugin_api_infobar_remove (struct t_weechat_plugin *, int);

/* hooks */
extern struct t_hook *plugin_api_hook_command (struct t_weechat_plugin *,
                                               char *, char *, char *, char *,
                                               char *,
                                               int (*)(void *, void *, int, char **, char **),
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
extern struct t_hook *plugin_api_hook_completion (struct t_weechat_plugin *,
                                                  char *,
                                                  int (*)(void *, char *, void *, void *),
                                                  void *);
extern void plugin_api_unhook (struct t_weechat_plugin *, void *);
extern void plugin_api_unhook_all (struct t_weechat_plugin *);

/* buffers */
extern struct t_gui_buffer *plugin_api_buffer_new (struct t_weechat_plugin *,
                                                   char *, char *,
                                                   void (*)(struct t_gui_buffer *, char *));
extern struct t_gui_buffer *plugin_api_buffer_search (struct t_weechat_plugin *,
                                                      char *, char *);
extern void plugin_api_buffer_close (struct t_weechat_plugin *, void *);
extern void *plugin_api_buffer_get (struct t_weechat_plugin *, void *, char *);
extern void plugin_api_buffer_set (struct t_weechat_plugin *, void *, char *,
                                   char *);
extern void plugin_api_buffer_nick_add (struct t_weechat_plugin *, void *,
                                        char *, int, char *, char, char *);
extern void plugin_api_buffer_nick_remove (struct t_weechat_plugin *, char *);

/* command */
extern void plugin_api_command (struct t_weechat_plugin *, void *, char *);

/* infos */
extern char *plugin_api_info_get (struct t_weechat_plugin *, char *);

/* infolists */
extern struct t_plugin_infolist *plugin_api_infolist_get (struct t_weechat_plugin *,
                                                          char *, void *);
extern int plugin_api_infolist_next (struct t_weechat_plugin *,
                                     void *);
extern int plugin_api_infolist_prev (struct t_weechat_plugin *,
                                     void *);
extern char *plugin_api_infolist_fields (struct t_weechat_plugin *, void *);
extern int plugin_api_infolist_integer (struct t_weechat_plugin *, void *, char *);
extern char *plugin_api_infolist_string (struct t_weechat_plugin *, void *, char *);
extern void *plugin_api_infolist_pointer (struct t_weechat_plugin *, void *, char *);
extern time_t plugin_api_infolist_time (struct t_weechat_plugin *, void *, char *);
extern void plugin_api_infolist_free (struct t_weechat_plugin *, void *);

/* log */
extern void plugin_api_log (struct t_weechat_plugin *, char *, char *,
                            char *, ...);

#endif /* plugin-api.h */
