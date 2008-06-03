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


#ifndef __WEECHAT_PLUGIN_H
#define __WEECHAT_PLUGIN_H 1

#include "weechat-plugin.h"

typedef int (t_weechat_init_func) (struct t_weechat_plugin *plugin,
                                   int argc, char *argv[]);
typedef int (t_weechat_end_func) (struct t_weechat_plugin *plugin);

extern struct t_weechat_plugin *weechat_plugins;
extern struct t_weechat_plugin *last_weechat_plugin;

//extern t_plugin_irc_color plugins_irc_colors[GUI_NUM_IRC_COLORS];

extern struct t_weechat_plugin *plugin_search (const char *name);
extern struct t_weechat_plugin *plugin_load (const char *filename);
extern void plugin_auto_load ();
extern void plugin_remove (struct t_weechat_plugin *plugin);
extern void plugin_unload (struct t_weechat_plugin *plugin);
extern void plugin_unload_name (const char *name);
extern void plugin_unload_all ();
extern void plugin_reload_name (const char *name);
extern void plugin_init (int auto_load, int argc, char *argv[]);
extern void plugin_end ();
extern void plugin_print_log ();

#endif /* plugin.h */
