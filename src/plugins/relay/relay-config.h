/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_RELAY_CONFIG_H
#define __WEECHAT_RELAY_CONFIG_H 1

#define RELAY_CONFIG_NAME "relay"

extern struct t_config_file *relay_config;

extern struct t_config_option *relay_config_look_auto_open_buffer;

extern struct t_config_option *relay_config_color_text;
extern struct t_config_option *relay_config_color_text_bg;
extern struct t_config_option *relay_config_color_text_selected;
extern struct t_config_option *relay_config_color_status[];

extern struct t_config_option *relay_config_network_enabled;
extern struct t_config_option *relay_config_network_listen_port_range;

extern int relay_config_init ();
extern int relay_config_read ();
extern int relay_config_write ();

#endif /* relay-config.h */
