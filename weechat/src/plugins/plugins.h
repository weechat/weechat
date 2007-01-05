/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __WEECHAT_PLUGINS_H
#define __WEECHAT_PLUGINS_H 1

#include "weechat-plugin.h"
#include "plugins-config.h"
#include "../irc/irc.h"
#include "../gui/gui.h"

typedef struct t_plugin_irc_color t_plugin_irc_color;

struct t_plugin_irc_color
{
    int number;
    char *name;
};

typedef int (t_weechat_init_func) (t_weechat_plugin *);
typedef void (t_weechat_end_func) (t_weechat_plugin *);

extern t_weechat_plugin *weechat_plugins;
extern t_weechat_plugin *last_weechat_plugin;

extern t_plugin_irc_color plugins_irc_colors[GUI_NUM_IRC_COLORS];

extern int plugin_find_server_channel (char *, char *,
                                       t_irc_server **, t_irc_channel **);
extern void plugin_exec_on_files (t_weechat_plugin *, char *,
                                  int (*)(t_weechat_plugin *, char *));
extern t_weechat_plugin *plugin_search (char *);
extern t_plugin_handler *plugin_msg_handler_add (t_weechat_plugin *, char *,
                                                 t_plugin_handler_func *,
                                                 char *, void *);
extern t_plugin_handler *plugin_cmd_handler_add (t_weechat_plugin *, char *,
                                                 char *, char *, char *,
                                                 char *,
                                                 t_plugin_handler_func *,
                                                 char *, void *);
extern t_plugin_handler *plugin_timer_handler_add (t_weechat_plugin *, int,
                                                   t_plugin_handler_func *,
                                                   char *, void *);
extern t_plugin_handler *plugin_keyboard_handler_add (t_weechat_plugin *,
                                                      t_plugin_handler_func *,
                                                      char *, void *);
extern int plugin_msg_handler_exec (char *, char *, char *);
extern int plugin_cmd_handler_exec (char *, char *, char *);
extern int plugin_timer_handler_exec ();
extern int plugin_keyboard_handler_exec (char *, char *, char *);
extern void plugin_handler_remove (t_weechat_plugin *,
                                   t_plugin_handler *);
extern void plugin_handler_remove_all (t_weechat_plugin *);
extern t_plugin_modifier *plugin_modifier_add (t_weechat_plugin *,
                                               char *, char *,
                                               t_plugin_modifier_func *,
                                               char *, void *);
extern char *plugin_modifier_exec (t_plugin_modifier_type, char *, char *);
extern void plugin_modifier_remove (t_weechat_plugin *,
                                    t_plugin_modifier *);
extern void plugin_modifier_remove_all (t_weechat_plugin *);
extern t_weechat_plugin *plugin_load (char *);
extern void plugin_auto_load ();
extern void plugin_remove (t_weechat_plugin *);
extern void plugin_unload (t_weechat_plugin *);
extern void plugin_unload_name (char *);
extern void plugin_unload_all ();
extern void plugin_reload_name (char *);
extern void plugin_init (int);
extern void plugin_end ();

#endif /* plugins.h */
