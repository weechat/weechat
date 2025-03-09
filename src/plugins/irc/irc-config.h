/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEECHAT_PLUGIN_IRC_CONFIG_H
#define WEECHAT_PLUGIN_IRC_CONFIG_H

#define IRC_CONFIG_NAME "irc"
#define IRC_CONFIG_PRIO_NAME (TO_STR(IRC_PLUGIN_PRIORITY) "|" IRC_CONFIG_NAME)

#define IRC_CONFIG_VERSION 5

enum t_irc_config_look_server_buffer
{
    IRC_CONFIG_LOOK_SERVER_BUFFER_MERGE_WITH_CORE = 0,
    IRC_CONFIG_LOOK_SERVER_BUFFER_MERGE_WITHOUT_CORE,
    IRC_CONFIG_LOOK_SERVER_BUFFER_INDEPENDENT,
};

enum t_irc_config_look_pv_buffer
{
    IRC_CONFIG_LOOK_PV_BUFFER_INDEPENDENT = 0,
    IRC_CONFIG_LOOK_PV_BUFFER_MERGE_BY_SERVER,
    IRC_CONFIG_LOOK_PV_BUFFER_MERGE_ALL,
};

enum t_irc_config_look_buffer_position
{
    IRC_CONFIG_LOOK_BUFFER_POSITION_NONE = 0,
    IRC_CONFIG_LOOK_BUFFER_POSITION_NEXT,
    IRC_CONFIG_LOOK_BUFFER_POSITION_NEAR_SERVER,
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

enum t_irc_config_look_nick_mode
{
    IRC_CONFIG_LOOK_NICK_MODE_NONE = 0,
    IRC_CONFIG_LOOK_NICK_MODE_PREFIX,
    IRC_CONFIG_LOOK_NICK_MODE_ACTION,
    IRC_CONFIG_LOOK_NICK_MODE_BOTH,
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

extern struct t_config_option *irc_config_look_buffer_open_before_autojoin;
extern struct t_config_option *irc_config_look_buffer_open_before_join;
extern struct t_config_option *irc_config_look_buffer_switch_autojoin;
extern struct t_config_option *irc_config_look_buffer_switch_join;
extern struct t_config_option *irc_config_look_color_nicks_in_names;
extern struct t_config_option *irc_config_look_color_nicks_in_nicklist;
extern struct t_config_option *irc_config_look_color_nicks_in_server_messages;
extern struct t_config_option *irc_config_look_color_pv_nick_like_channel;
extern struct t_config_option *irc_config_look_ctcp_time_format;
extern struct t_config_option *irc_config_look_display_account_message;
extern struct t_config_option *irc_config_look_display_away;
extern struct t_config_option *irc_config_look_display_ctcp_blocked;
extern struct t_config_option *irc_config_look_display_ctcp_reply;
extern struct t_config_option *irc_config_look_display_ctcp_unknown;
extern struct t_config_option *irc_config_look_display_extended_join;
extern struct t_config_option *irc_config_look_display_host_join;
extern struct t_config_option *irc_config_look_display_host_join_local;
extern struct t_config_option *irc_config_look_display_host_notice;
extern struct t_config_option *irc_config_look_display_host_quit;
extern struct t_config_option *irc_config_look_display_host_wallops;
extern struct t_config_option *irc_config_look_display_join_message;
extern struct t_config_option *irc_config_look_display_old_topic;
extern struct t_config_option *irc_config_look_display_pv_away_once;
extern struct t_config_option *irc_config_look_display_pv_back;
extern struct t_config_option *irc_config_look_display_pv_nick_change;
extern struct t_config_option *irc_config_look_display_pv_warning_address;
extern struct t_config_option *irc_config_look_highlight_channel;
extern struct t_config_option *irc_config_look_highlight_pv;
extern struct t_config_option *irc_config_look_highlight_server;
extern struct t_config_option *irc_config_look_highlight_tags_restrict;
extern struct t_config_option *irc_config_look_ignore_tag_messages;
extern struct t_config_option *irc_config_look_item_channel_modes_hide_args;
extern struct t_config_option *irc_config_look_item_display_server;
extern struct t_config_option *irc_config_look_item_nick_modes;
extern struct t_config_option *irc_config_look_item_nick_prefix;
extern struct t_config_option *irc_config_look_join_auto_add_chantype;
extern struct t_config_option *irc_config_look_list_buffer;
extern struct t_config_option *irc_config_look_list_buffer_format_export;
extern struct t_config_option *irc_config_look_list_buffer_scroll_horizontal;
extern struct t_config_option *irc_config_look_list_buffer_sort;
extern struct t_config_option *irc_config_look_list_buffer_topic_strip_colors;
extern struct t_config_option *irc_config_look_msgbuffer_fallback;
extern struct t_config_option *irc_config_look_new_channel_position;
extern struct t_config_option *irc_config_look_new_list_position;
extern struct t_config_option *irc_config_look_new_pv_position;
extern struct t_config_option *irc_config_look_nick_completion_smart;
extern struct t_config_option *irc_config_look_nick_mode;
extern struct t_config_option *irc_config_look_nick_mode_empty;
extern struct t_config_option *irc_config_look_nicks_hide_password;
extern struct t_config_option *irc_config_look_notice_as_pv;
extern struct t_config_option *irc_config_look_notice_welcome_redirect;
extern struct t_config_option *irc_config_look_notice_welcome_tags;
extern struct t_config_option *irc_config_look_notify_tags_ison;
extern struct t_config_option *irc_config_look_notify_tags_whois;
extern struct t_config_option *irc_config_look_open_pv_buffer_echo_msg;
extern struct t_config_option *irc_config_look_part_closes_buffer;
extern struct t_config_option *irc_config_look_pv_buffer;
extern struct t_config_option *irc_config_look_pv_tags;
extern struct t_config_option *irc_config_look_raw_messages;
extern struct t_config_option *irc_config_look_typing_status_nicks;
extern struct t_config_option *irc_config_look_typing_status_self;
extern struct t_config_option *irc_config_look_server_buffer;
extern struct t_config_option *irc_config_look_smart_filter;
extern struct t_config_option *irc_config_look_smart_filter_account;
extern struct t_config_option *irc_config_look_smart_filter_chghost;
extern struct t_config_option *irc_config_look_smart_filter_delay;
extern struct t_config_option *irc_config_look_smart_filter_join;
extern struct t_config_option *irc_config_look_smart_filter_join_unmask;
extern struct t_config_option *irc_config_look_smart_filter_mode;
extern struct t_config_option *irc_config_look_smart_filter_nick;
extern struct t_config_option *irc_config_look_smart_filter_quit;
extern struct t_config_option *irc_config_look_smart_filter_setname;
extern struct t_config_option *irc_config_look_temporary_servers;
extern struct t_config_option *irc_config_look_topic_strip_colors;

extern struct t_config_option *irc_config_color_input_nick;
extern struct t_config_option *irc_config_color_item_lag_counting;
extern struct t_config_option *irc_config_color_item_lag_finished;
extern struct t_config_option *irc_config_color_item_nick_modes;
extern struct t_config_option *irc_config_color_item_tls_version_deprecated;
extern struct t_config_option *irc_config_color_item_tls_version_insecure;
extern struct t_config_option *irc_config_color_item_tls_version_ok;
extern struct t_config_option *irc_config_color_list_buffer_line_selected;
extern struct t_config_option *irc_config_color_list_buffer_line_selected_bg;
extern struct t_config_option *irc_config_color_message_account;
extern struct t_config_option *irc_config_color_message_chghost;
extern struct t_config_option *irc_config_color_message_join;
extern struct t_config_option *irc_config_color_message_kick;
extern struct t_config_option *irc_config_color_message_quit;
extern struct t_config_option *irc_config_color_message_setname;
extern struct t_config_option *irc_config_color_mirc_remap;
extern struct t_config_option *irc_config_color_nick_prefixes;
extern struct t_config_option *irc_config_color_notice;
extern struct t_config_option *irc_config_color_reason_kick;
extern struct t_config_option *irc_config_color_reason_quit;
extern struct t_config_option *irc_config_color_term_remap;
extern struct t_config_option *irc_config_color_topic_current;
extern struct t_config_option *irc_config_color_topic_new;
extern struct t_config_option *irc_config_color_topic_old;

extern struct t_config_option *irc_config_network_autoreconnect_delay_growing;
extern struct t_config_option *irc_config_network_autoreconnect_delay_max;
extern struct t_config_option *irc_config_network_ban_mask_default;
extern struct t_config_option *irc_config_network_colors_receive;
extern struct t_config_option *irc_config_network_colors_send;
extern struct t_config_option *irc_config_network_lag_check;
extern struct t_config_option *irc_config_network_lag_max;
extern struct t_config_option *irc_config_network_lag_min_show;
extern struct t_config_option *irc_config_network_lag_reconnect;
extern struct t_config_option *irc_config_network_lag_refresh_interval;
extern struct t_config_option *irc_config_network_notify_check_ison;
extern struct t_config_option *irc_config_network_notify_check_whois;
extern struct t_config_option *irc_config_network_sasl_fail_unavailable;
extern struct t_config_option *irc_config_network_send_unknown_commands;
extern struct t_config_option *irc_config_network_whois_double_nick;

extern struct t_config_option *irc_config_server_default[];

extern struct t_hashtable *irc_config_hashtable_display_join_message;
extern struct t_hashtable *irc_config_hashtable_nick_prefixes;
extern struct t_hashtable *irc_config_hashtable_color_mirc_remap;
extern struct t_hashtable *irc_config_hashtable_color_term_remap;
extern char **irc_config_nicks_hide_password;
extern int irc_config_num_nicks_hide_password;

extern int irc_config_display_channel_modes_arguments (const char *modes);
extern int irc_config_notice_nick_notify (const char *nick);
extern int irc_config_check_autojoin (const char *autojoin);
extern int irc_config_server_check_value_cb (const void *pointer, void *data,
                                             struct t_config_option *option,
                                             const char *value);
extern void irc_config_server_change_cb (const void *pointer, void *data,
                                         struct t_config_option *option);
struct t_config_option *irc_config_server_new_option (struct t_config_file *config_file,
                                                      struct t_config_section *section,
                                                      int index_option,
                                                      const char *option_name,
                                                      const char *default_value,
                                                      const char *value,
                                                      int null_value_allowed,
                                                      int (*callback_check_value)(const void *pointer,
                                                                                  void *data,
                                                                                  struct t_config_option *option,
                                                                                  const char *value),
                                                      const void *callback_check_value_pointer,
                                                      void *callback_check_value_data,
                                                      void (*callback_change)(const void *pointer,
                                                                              void *data,
                                                                              struct t_config_option *option),
                                                      const void *callback_change_pointer,
                                                      void *callback_change_data);
extern int irc_config_init (void);
extern int irc_config_read (void);
extern int irc_config_write (int write_temp_servers);
extern void irc_config_free (void);

#endif /* WEECHAT_PLUGIN_IRC_CONFIG_H */
