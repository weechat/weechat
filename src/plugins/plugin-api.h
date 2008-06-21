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
                                    const char *charset);
extern char *plugin_api_gettext (const char *string);
extern char *plugin_api_ngettext (const char *single, const char *plural,
                                  int count);

/* directories */
extern int plugin_api_mkdir_home (const char *directory, int mode);
extern int plugin_api_mkdir (const char *directory, int mode);

/* config */
extern struct t_config_option *plugin_api_config_get (const char *option_name);
extern char *plugin_api_config_get_plugin (struct t_weechat_plugin *plugin,
                                           const char *option_name);
extern int plugin_api_config_set_plugin (struct t_weechat_plugin *plugin,
                                         const char *option_name, const char *value);

/* display */
extern char *plugin_api_prefix (const char *prefix);
extern char *plugin_api_color (const char *color_name);

/* command */
extern void plugin_api_command (struct t_weechat_plugin *plugin,
                                struct t_gui_buffer *buffer, const char *command);

/* infos */
extern char *plugin_api_info_get (struct t_weechat_plugin *plugin, const char *info);

/* infolists */
extern struct t_plugin_infolist *plugin_api_infolist_get (const char *name,
                                                          void *pointer,
                                                          const char *arguments);
extern int plugin_api_infolist_next (struct t_plugin_infolist *infolist);
extern int plugin_api_infolist_prev (struct t_plugin_infolist *infolist);
extern char *plugin_api_infolist_fields (struct t_plugin_infolist *infolist);
extern int plugin_api_infolist_integer (struct t_plugin_infolist *infolist,
                                        const char *var);
extern char *plugin_api_infolist_string (struct t_plugin_infolist *infolist,
                                         const char *var);
extern void *plugin_api_infolist_pointer (struct t_plugin_infolist *infolist,
                                          const char *var);
extern time_t plugin_api_infolist_time (struct t_plugin_infolist *infolist,
                                        const char *var);
extern void plugin_api_infolist_free (struct t_plugin_infolist *infolist);

#endif /* plugin-api.h */
