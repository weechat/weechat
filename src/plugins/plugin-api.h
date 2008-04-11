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


#ifndef __WEECHAT_PLUGIN_API_H
#define __WEECHAT_PLUGIN_API_H 1

/* strings */
extern void plugin_api_charset_set (struct t_weechat_plugin *plugin,
                                    char *charset);
extern char *plugin_api_gettext (char *string);
extern char *plugin_api_ngettext (char *single, char *plural, int count);

/* directories */
extern int plugin_api_mkdir_home (char *directory, int mode);
extern int plugin_api_mkdir (char *directory, int mode);

/* config */
extern struct t_config_option *plugin_api_config_get (char *option_name);
extern char *plugin_api_config_get_plugin (struct t_weechat_plugin *plugin,
                                           char *option_name);
extern int plugin_api_config_set_plugin (struct t_weechat_plugin *plugin,
                                         char *option_name, char *value);

/* display */
extern char *plugin_api_prefix (char *prefix);
extern char *plugin_api_color (char *color_name);
extern void plugin_api_infobar_printf (struct t_weechat_plugin *plugin,
                                       int delay, char *color_name,
                                       char *format, ...);
extern void plugin_api_infobar_remove (int how_many);

/* command */
extern void plugin_api_command (struct t_weechat_plugin *plugin,
                                struct t_gui_buffer *buffer, char *command);

/* infos */
extern char *plugin_api_info_get (struct t_weechat_plugin *plugin, char *info);

/* infolists */
extern struct t_plugin_infolist *plugin_api_infolist_get (char *name,
                                                          void *pointer);
extern int plugin_api_infolist_next (struct t_plugin_infolist *infolist);
extern int plugin_api_infolist_prev (struct t_plugin_infolist *infolist);
extern char *plugin_api_infolist_fields (struct t_plugin_infolist *infolist);
extern int plugin_api_infolist_integer (struct t_plugin_infolist *infolist,
                                        char *var);
extern char *plugin_api_infolist_string (struct t_plugin_infolist *infolist,
                                         char *var);
extern void *plugin_api_infolist_pointer (struct t_plugin_infolist *infolist,
                                          char *var);
extern time_t plugin_api_infolist_time (struct t_plugin_infolist *infolist,
                                        char *var);
extern void plugin_api_infolist_free (struct t_plugin_infolist *infolist);

#endif /* plugin-api.h */
