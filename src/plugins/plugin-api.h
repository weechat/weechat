/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_PLUGIN_API_H
#define __WEECHAT_PLUGIN_API_H 1

struct t_plugin_api_hdata
{
    char *name;                              /* hdata name                  */
    struct t_hdata *(*callback_get_hdata)(); /* callback to get hdata       */
};

/* strings */
extern void plugin_api_charset_set (struct t_weechat_plugin *plugin,
                                    const char *charset);
extern const char *plugin_api_gettext (const char *string);
extern const char *plugin_api_ngettext (const char *single, const char *plural,
                                        int count);

/* config */
extern struct t_config_option *plugin_api_config_get (const char *option_name);
extern const char *plugin_api_config_get_plugin (struct t_weechat_plugin *plugin,
                                           const char *option_name);
extern int plugin_api_config_is_set_plugin (struct t_weechat_plugin *plugin,
                                            const char *option_name);
extern int plugin_api_config_set_plugin (struct t_weechat_plugin *plugin,
                                         const char *option_name,
                                         const char *value);
extern void plugin_api_config_set_desc_plugin (struct t_weechat_plugin *plugin,
                                               const char *option_name,
                                               const char *description);
extern int plugin_api_config_unset_plugin (struct t_weechat_plugin *plugin,
                                           const char *option_name);

/* display */
extern const char *plugin_api_prefix (const char *prefix);
extern const char *plugin_api_color (const char *color_name);

/* command */
extern void plugin_api_command (struct t_weechat_plugin *plugin,
                                struct t_gui_buffer *buffer, const char *command);

/* infolist */
extern int plugin_api_infolist_next (struct t_infolist *infolist);
extern int plugin_api_infolist_prev (struct t_infolist *infolist);
extern void plugin_api_infolist_reset_item_cursor (struct t_infolist *infolist);
extern const char *plugin_api_infolist_fields (struct t_infolist *infolist);
extern int plugin_api_infolist_integer (struct t_infolist *infolist,
                                        const char *var);
extern const char *plugin_api_infolist_string (struct t_infolist *infolist,
                                               const char *var);
extern void *plugin_api_infolist_pointer (struct t_infolist *infolist,
                                          const char *var);
extern void *plugin_api_infolist_buffer (struct t_infolist *infolist,
                                         const char *var, int *size);
extern time_t plugin_api_infolist_time (struct t_infolist *infolist,
                                        const char *var);
extern void plugin_api_infolist_free (struct t_infolist *infolist);

extern void plugin_api_init ();

#endif /* __WEECHAT_PLUGIN_API_H */
