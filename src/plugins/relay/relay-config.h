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

#ifndef __WEECHAT_RELAY_CONFIG_H
#define __WEECHAT_RELAY_CONFIG_H 1

#include <regex.h>

#define RELAY_CONFIG_NAME "relay"

extern struct t_config_file *relay_config_file;
extern struct t_config_section *relay_config_section_port;

extern struct t_config_option *relay_config_look_auto_open_buffer;
extern struct t_config_option *relay_config_look_raw_messages;

extern struct t_config_option *relay_config_color_client;
extern struct t_config_option *relay_config_color_text;
extern struct t_config_option *relay_config_color_text_bg;
extern struct t_config_option *relay_config_color_text_selected;
extern struct t_config_option *relay_config_color_status[];

extern struct t_config_option *relay_config_network_allowed_ips;
extern struct t_config_option *relay_config_network_bind_address;
extern struct t_config_option *relay_config_network_compression_level;
extern struct t_config_option *relay_config_network_ipv6;
extern struct t_config_option *relay_config_network_max_clients;
extern struct t_config_option *relay_config_network_password;
extern struct t_config_option *relay_config_network_ssl_cert_key;

extern struct t_config_option *relay_config_irc_backlog_max_number;
extern struct t_config_option *relay_config_irc_backlog_max_minutes;
extern struct t_config_option *relay_config_irc_backlog_since_last_disconnect;
extern struct t_config_option *relay_config_irc_backlog_tags;
extern struct t_config_option *relay_config_irc_backlog_time_format;

extern regex_t *relay_config_regex_allowed_ips;
extern regex_t *relay_config_regex_websocket_allowed_origins;
extern struct t_hashtable *relay_config_hashtable_irc_backlog_tags;

extern int relay_config_create_option_port (void *data,
                                            struct t_config_file *config_file,
                                            struct t_config_section *section,
                                            const char *option_name,
                                            const char *value);
extern int relay_config_init ();
extern int relay_config_read ();
extern int relay_config_write ();
extern void relay_config_free ();

#endif /* __WEECHAT_RELAY_CONFIG_H */
