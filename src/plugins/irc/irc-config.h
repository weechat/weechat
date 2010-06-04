/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_IRC_CONFIG_H
#define __WEECHAT_IRC_CONFIG_H 1

#define IRC_CONFIG_NAME "irc"

enum t_irc_config_look_server_buffer
{
    IRC_CONFIG_LOOK_SERVER_BUFFER_MERGE_WITH_CORE = 0,
    IRC_CONFIG_LOOK_SERVER_BUFFER_MERGE_WITHOUT_CORE,
    IRC_CONFIG_LOOK_SERVER_BUFFER_INDEPENDENT,
};

enum t_irc_config_look_item_display_server
{
    IRC_CONFIG_LOOK_ITEM_DISPLAY_SERVER_PLUGIN = 0,
    IRC_CONFIG_LOOK_ITEM_DISPLAY_SERVER_NAME,
};

enum t_irc_config_look_msgbuffer_fallback
{
    IRC_CONFIG_LOOK_MSGBUFFER_FALLBACK_CURRENT = 0,
    IRC_CONFIG_LOOK_MSGBUFFER_FALLBACK_SERVER,
};

enum t_irc_config_look_notice_as_pv
{
    IRC_CONFIG_LOOK_NOTICE_AS_PV_AUTO = 0,
    IRC_CONFIG_LOOK_NOTICE_AS_PV_NEVER,
    IRC_CONFIG_LOOK_NOTICE_AS_PV_ALWAYS,
};

enum t_irc_config_nick_completion
{
    IRC_CONFIG_NICK_COMPLETION_SMART_OFF = 0,
    IRC_CONFIG_NICK_COMPLETION_SMART_SPEAKERS,
    IRC_CONFIG_NICK_COMPLETION_SMART_SPEAKERS_HIGHLIGHTS,
};

enum t_irc_config_display_away
{
    IRC_CONFIG_DISPLAY_AWAY_OFF = 0,
    IRC_CONFIG_DISPLAY_AWAY_LOCAL,
    IRC_CONFIG_DISPLAY_AWAY_CHANNEL,
};

extern struct t_config_file *irc_config_file;
extern struct t_config_section *irc_config_section_msgbuffer;
extern struct t_config_section *irc_config_section_ctcp;
extern struct t_config_section *irc_config_section_server_default;
extern struct t_config_section *irc_config_section_server;

extern struct t_config_option *irc_config_look_color_nicks_in_server_messages;
extern struct t_config_option *irc_config_look_color_pv_nick_like_channel;
extern struct t_config_option *irc_config_look_server_buffer;
extern struct t_config_option *irc_config_look_open_channel_near_server;
extern struct t_config_option *irc_config_look_open_pv_near_server;
extern struct t_config_option *irc_config_look_nick_prefix;
extern struct t_config_option *irc_config_look_nick_suffix;
extern struct t_config_option *irc_config_look_nick_completion_smart;
extern struct t_config_option *irc_config_look_display_away;
extern struct t_config_option *irc_config_look_display_ctcp_blocked;
extern struct t_config_option *irc_config_look_display_ctcp_reply;
extern struct t_config_option *irc_config_look_display_ctcp_unknown;
extern struct t_config_option *irc_config_look_display_old_topic;
extern struct t_config_option *irc_config_look_display_pv_away_once;
extern struct t_config_option *irc_config_look_display_pv_back;
extern struct t_config_option *irc_config_look_item_channel_modes;
extern struct t_config_option *irc_config_look_item_channel_modes_hide_key;
extern struct t_config_option *irc_config_look_item_nick_modes;
extern struct t_config_option *irc_config_look_item_nick_prefix;
extern struct t_config_option *irc_config_look_hide_nickserv_pwd;
extern struct t_config_option *irc_config_look_highlight_tags;
extern struct t_config_option *irc_config_look_item_display_server;
extern struct t_config_option *irc_config_look_msgbuffer_fallback;
extern struct t_config_option *irc_config_look_notice_as_pv;
extern struct t_config_option *irc_config_look_part_closes_buffer;
extern struct t_config_option *irc_config_look_raw_messages;
extern struct t_config_option *irc_config_look_smart_filter;
extern struct t_config_option *irc_config_look_smart_filter_delay;
extern struct t_config_option *irc_config_look_smart_filter_join;
extern struct t_config_option *irc_config_look_smart_filter_quit;
extern struct t_config_option *irc_config_look_topic_strip_colors;

extern struct t_config_option *irc_config_color_message_join;
extern struct t_config_option *irc_config_color_message_quit;
extern struct t_config_option *irc_config_color_nick_prefix_op;
extern struct t_config_option *irc_config_color_nick_prefix_halfop;
extern struct t_config_option *irc_config_color_nick_prefix_voice;
extern struct t_config_option *irc_config_color_nick_prefix_user;
extern struct t_config_option *irc_config_color_notice;
extern struct t_config_option *irc_config_color_input_nick;
extern struct t_config_option *irc_config_color_item_away;
extern struct t_config_option *irc_config_color_item_channel_modes;

extern struct t_config_option *irc_config_network_autoreconnect_delay_growing;
extern struct t_config_option *irc_config_network_autoreconnect_delay_max;
extern struct t_config_option *irc_config_network_connection_timeout;
extern struct t_config_option *irc_config_network_default_msg_part;
extern struct t_config_option *irc_config_network_default_msg_quit;
extern struct t_config_option *irc_config_network_away_check;
extern struct t_config_option *irc_config_network_away_check_max_nicks;
extern struct t_config_option *irc_config_network_lag_check;
extern struct t_config_option *irc_config_network_lag_min_show;
extern struct t_config_option *irc_config_network_lag_disconnect;
extern struct t_config_option *irc_config_network_anti_flood[2];
extern struct t_config_option *irc_config_network_colors_receive;
extern struct t_config_option *irc_config_network_colors_send;
extern struct t_config_option *irc_config_network_send_unknown_commands;

extern struct t_config_option *irc_config_server_default[];

extern void irc_config_server_change_cb (void *data,
                                         struct t_config_option *option);
struct t_config_option *irc_config_server_new_option (struct t_config_file *config_file,
                                                      struct t_config_section *section,
                                                      int index_option,
                                                      const char *option_name,
                                                      const char *default_value,
                                                      const char *value,
                                                      int null_value_allowed,
                                                      void *callback_change,
                                                      void *callback_change_data);
extern int irc_config_init ();
extern int irc_config_read ();
extern int irc_config_write (int write_temp_servers);
extern void irc_config_free ();

#endif /* irc-config.h */
