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


#ifndef __WEECHAT_PLUGIN_H
#define __WEECHAT_PLUGIN_H 1

#include "weechat-plugin.h"

typedef int (t_weechat_init_func) (struct t_weechat_plugin *);
typedef void (t_weechat_end_func) (struct t_weechat_plugin *);

extern struct t_weechat_plugin *weechat_plugins;
extern struct t_weechat_plugin *last_weechat_plugin;

//extern t_plugin_irc_color plugins_irc_colors[GUI_NUM_IRC_COLORS];

extern void plugin_exec_on_files (struct t_weechat_plugin *, char *,
                                  int (*)(struct t_weechat_plugin *, char *));
extern struct t_weechat_plugin *plugin_search (char *);
extern struct t_weechat_plugin *plugin_load (char *);
extern void plugin_auto_load ();
extern void plugin_remove (struct t_weechat_plugin *);
extern void plugin_unload (struct t_weechat_plugin *);
extern void plugin_unload_name (char *);
extern void plugin_unload_all ();
extern void plugin_reload_name (char *);
extern void plugin_init (int);
extern void plugin_end ();
extern void plugin_print_log ();

#endif /* plugin.h */
