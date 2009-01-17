/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_JABBER_CONFIG_H
#define __WEECHAT_JABBER_CONFIG_H 1

#define JABBER_CONFIG_NAME "jabber"

#define JABBER_CONFIG_DISPLAY_AWAY_OFF   0
#define JABBER_CONFIG_DISPLAY_AWAY_LOCAL 1
#define JABBER_CONFIG_DISPLAY_AWAY_MUC   2


extern struct t_config_file *jabber_config_file;
extern struct t_config_section *jabber_config_section_server_default;
extern struct t_config_section *jabber_config_section_server;

extern struct t_config_option *jabber_config_look_color_nicks_in_server_messages;
extern struct t_config_option *jabber_config_look_one_server_buffer;
extern struct t_config_option *jabber_config_look_open_near_server;
extern struct t_config_option *jabber_config_look_nick_prefix;
extern struct t_config_option *jabber_config_look_nick_suffix;
extern struct t_config_option *jabber_config_look_nick_completion_smart;
extern struct t_config_option *jabber_config_look_display_away;
extern struct t_config_option *jabber_config_look_display_muc_modes;
extern struct t_config_option *jabber_config_look_highlight_tags;
extern struct t_config_option *jabber_config_look_show_away_once;
extern struct t_config_option *jabber_config_look_smart_filter;
extern struct t_config_option *jabber_config_look_smart_filter_delay;

extern struct t_config_option *jabber_config_color_message_join;
extern struct t_config_option *jabber_config_color_message_quit;
extern struct t_config_option *jabber_config_color_input_nick;

extern struct t_config_option *jabber_config_network_default_msg_part;
extern struct t_config_option *jabber_config_network_default_msg_quit;
extern struct t_config_option *jabber_config_network_lag_check;
extern struct t_config_option *jabber_config_network_lag_min_show;
extern struct t_config_option *jabber_config_network_lag_disconnect;
extern struct t_config_option *jabber_config_network_anti_flood;
extern struct t_config_option *jabber_config_network_colors_receive;
extern struct t_config_option *jabber_config_network_colors_send;

extern struct t_config_option *jabber_config_server_default[];

extern void jabber_config_server_change_cb (void *data,
                                            struct t_config_option *option);
struct t_config_option *jabber_config_server_new_option (struct t_config_file *config_file,
                                                         struct t_config_section *section,
                                                         int index_option,
                                                         const char *option_name,
                                                         const char *default_value,
                                                         const char *value,
                                                         int null_value_allowed,
                                                         void *callback_change,
                                                         void *callback_change_data);
extern int jabber_config_init ();
extern int jabber_config_read ();
extern int jabber_config_write (int write_temp_servers);
extern void jabber_config_free ();

#endif /* jabber-config.h */
