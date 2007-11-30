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


#ifndef __WEECHAT_PLUGIN_CONFIG_H
#define __WEECHAT_PLUGIN_CONFIG_H 1

#define PLUGIN_CONFIG_FILENAME "plugins.rc"

extern struct t_config_file *plugin_config;
extern struct t_config_option *plugin_options;

extern struct t_config_option *plugin_config_search_internal (char *);
extern struct t_config_option *plugin_config_search (char *, char *);
extern int plugin_config_set_internal (char *, char *);
extern int plugin_config_set (char *, char *, char *);
extern int plugin_config_read ();
extern int plugin_config_write ();

#endif /* plugin-config.h */
