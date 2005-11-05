/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __WEECHAT_PLUGINS_H
#define __WEECHAT_PLUGINS_H 1

#include "weechat-plugin.h"
#include "../irc/irc.h"
#include "../gui/gui.h"

typedef int (t_weechat_init_func) (t_weechat_plugin *);
typedef void (t_weechat_end_func) (t_weechat_plugin *);

extern t_weechat_plugin *weechat_plugins;
extern t_weechat_plugin *last_weechat_plugin;

extern t_gui_buffer *plugin_find_buffer (char *, char *);
extern t_irc_server *plugin_find_server (char *, char *);
extern void plugin_exec_on_files (t_weechat_plugin *, char *,
                                  int (*)(t_weechat_plugin *, char *));
extern t_weechat_plugin *plugin_search (char *);
extern t_plugin_handler *plugin_msg_handler_add (t_weechat_plugin *, char *,
                                                 t_plugin_handler_func *,
                                                 char *, void *);
extern t_plugin_handler *plugin_cmd_handler_add (t_weechat_plugin *, char *,
                                                 char *, char *, char *,
                                                 t_plugin_handler_func *,
                                                 char *, void *);
extern int plugin_msg_handler_exec (char *, char *, char *);
extern int plugin_cmd_handler_exec (char *, char *, char *);
extern void plugin_handler_remove (t_weechat_plugin *,
                                   t_plugin_handler *);
extern void plugin_handler_remove_all (t_weechat_plugin *);
extern t_weechat_plugin *plugin_load (char *);
extern void plugin_auto_load ();
extern void plugin_remove (t_weechat_plugin *);
extern void plugin_unload (t_weechat_plugin *);
extern void plugin_unload_name (char *);
extern void plugin_unload_all ();
extern void plugin_init (int);
extern void plugin_end ();

#endif /* plugins.h */
