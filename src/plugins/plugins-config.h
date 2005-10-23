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


#ifndef __WEECHAT_PLUGINS_CONFIG_H
#define __WEECHAT_PLUGINS_CONFIG_H 1

#define WEECHAT_PLUGINS_CONFIG_NAME "plugins.rc"

typedef struct t_plugin_option t_plugin_option;

struct t_plugin_option
{
    char *option_name;                  /* option name in config file          */
    char *value;                        /* value of option                     */
    t_plugin_option *prev_option;       /* link to previous option             */
    t_plugin_option *next_option;       /* link to next option                 */
};

extern t_plugin_option *plugin_config_search (t_weechat_plugin *, char *);
extern int plugin_config_set (t_weechat_plugin *, char *, char *);
extern void plugin_config_read ();
extern int plugin_config_write ();

#endif /* plugins-config.h */
