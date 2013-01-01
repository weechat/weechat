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

#ifndef __WEECHAT_PLUGIN_H
#define __WEECHAT_PLUGIN_H 1

#include "weechat-plugin.h"

#define PLUGIN_CORE "core"

typedef int (t_weechat_init_func) (struct t_weechat_plugin *plugin,
                                   int argc, char *argv[]);
typedef int (t_weechat_end_func) (struct t_weechat_plugin *plugin);

extern struct t_weechat_plugin *weechat_plugins;
extern struct t_weechat_plugin *last_weechat_plugin;

extern int plugin_valid (struct t_weechat_plugin *plugin);
extern struct t_weechat_plugin *plugin_search (const char *name);
extern const char *plugin_get_name (struct t_weechat_plugin *plugin);
extern struct t_weechat_plugin *plugin_load (const char *filename,
                                             int argc, char **argv);
extern void plugin_auto_load (int argc, char **argv);
extern void plugin_unload (struct t_weechat_plugin *plugin);
extern void plugin_unload_name (const char *name);
extern void plugin_unload_all ();
extern void plugin_reload_name (const char *name, int argc, char **argv);
extern void plugin_init (int auto_load, int argc, char *argv[]);
extern void plugin_end ();
extern struct t_hdata *plugin_hdata_plugin_cb (void *data,
                                               const char *hdata_name);
extern int plugin_add_to_infolist (struct t_infolist *infolist,
                                   struct t_weechat_plugin *plugin);
extern void plugin_print_log ();

#endif /* __WEECHAT_PLUGIN_H */
