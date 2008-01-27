/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_CONFIG_H
#define __WEECHAT_CONFIG_H 1

#include "wee-config-file.h"

#define WEECHAT_CONFIG_FILENAME "weechat.rc"

#define CONFIG_LOOK_NICKLIST_LEFT   0
#define CONFIG_LOOK_NICKLIST_RIGHT  1
#define CONFIG_LOOK_NICKLIST_TOP    2
#define CONFIG_LOOK_NICKLIST_BOTTOM 3

#define CONFIG_LOOK_PREFIX_ALIGN_NONE  0
#define CONFIG_LOOK_PREFIX_ALIGN_LEFT  1
#define CONFIG_LOOK_PREFIX_ALIGN_RIGHT 2

#define CONFIG_LOOK_HOTLIST_SORT_GROUP_TIME_ASC    0
#define CONFIG_LOOK_HOTLIST_SORT_GROUP_TIME_DESC   1
#define CONFIG_LOOK_HOTLIST_SORT_GROUP_NUMBER_ASC  2
#define CONFIG_LOOK_HOTLIST_SORT_GROUP_NUMBER_DESC 3
#define CONFIG_LOOK_HOTLIST_SORT_NUMBER_ASC        4
#define CONFIG_LOOK_HOTLIST_SORT_NUMBER_DESC       5

extern struct t_config_file *weechat_config_file;

extern struct t_config_option *config_look_color_real_white;
extern struct t_config_option *config_look_save_on_exit;
extern struct t_config_option *config_look_set_title;
extern struct t_config_option *config_look_startup_logo;
extern struct t_config_option *config_look_startup_version;
extern struct t_config_option *config_look_weechat_slogan;
extern struct t_config_option *config_look_one_server_buffer;
extern struct t_config_option *config_look_open_near_server;
extern struct t_config_option *config_look_scroll_amount;
extern struct t_config_option *config_look_buffer_time_format;
extern struct t_config_option *config_look_color_nicks_number;
extern struct t_config_option *config_look_color_actions;
extern struct t_config_option *config_look_nicklist;
extern struct t_config_option *config_look_nicklist_position;
extern struct t_config_option *config_look_nicklist_min_size;
extern struct t_config_option *config_look_nicklist_max_size;
extern struct t_config_option *config_look_nicklist_separator;
extern struct t_config_option *config_look_nickmode;
extern struct t_config_option *config_look_nickmode_empty;
extern struct t_config_option *config_look_no_nickname;
extern struct t_config_option *config_look_prefix[];
extern struct t_config_option *config_look_prefix_align;
extern struct t_config_option *config_look_prefix_align_max;
extern struct t_config_option *config_look_prefix_suffix;
extern struct t_config_option *config_look_nick_completor;
extern struct t_config_option *config_look_nick_completion_ignore;
extern struct t_config_option *config_look_nick_complete_first;
extern struct t_config_option *config_look_infobar;
extern struct t_config_option *config_look_infobar_time_format;
extern struct t_config_option *config_look_infobar_seconds;
extern struct t_config_option *config_look_infobar_delay_highlight;
extern struct t_config_option *config_look_hotlist_names_count;
extern struct t_config_option *config_look_hotlist_names_level;
extern struct t_config_option *config_look_hotlist_names_length;
extern struct t_config_option *config_look_hotlist_sort;
extern struct t_config_option *config_look_day_change;
extern struct t_config_option *config_look_day_change_time_format;
extern struct t_config_option *config_look_read_marker;
extern struct t_config_option *config_look_input_format;
extern struct t_config_option *config_look_paste_max_lines;
extern struct t_config_option *config_look_default_msg_quit;

extern struct t_config_option *config_color_separator;
extern struct t_config_option *config_color_title;
extern struct t_config_option *config_color_title_bg;
extern struct t_config_option *config_color_title_more;
extern struct t_config_option *config_color_chat;
extern struct t_config_option *config_color_chat_bg;
extern struct t_config_option *config_color_chat_time;
extern struct t_config_option *config_color_chat_time_delimiters;
extern struct t_config_option *config_color_chat_prefix[];
extern struct t_config_option *config_color_chat_prefix_more;
extern struct t_config_option *config_color_chat_prefix_suffix;
extern struct t_config_option *config_color_chat_buffer;
extern struct t_config_option *config_color_chat_server;
extern struct t_config_option *config_color_chat_channel;
extern struct t_config_option *config_color_chat_nick;
extern struct t_config_option *config_color_chat_nick_self;
extern struct t_config_option *config_color_chat_nick_other;
extern struct t_config_option *config_color_chat_nick_colors[];
extern struct t_config_option *config_color_chat_host;
extern struct t_config_option *config_color_chat_delimiters;
extern struct t_config_option *config_color_chat_highlight;
extern struct t_config_option *config_color_chat_read_marker;
extern struct t_config_option *config_color_chat_read_marker_bg;
extern struct t_config_option *config_color_status;
extern struct t_config_option *config_color_status_bg;
extern struct t_config_option *config_color_status_delimiters;
extern struct t_config_option *config_color_status_channel;
extern struct t_config_option *config_color_status_data_msg;
extern struct t_config_option *config_color_status_data_private;
extern struct t_config_option *config_color_status_data_highlight;
extern struct t_config_option *config_color_status_data_other;
extern struct t_config_option *config_color_status_more;
extern struct t_config_option *config_color_infobar;
extern struct t_config_option *config_color_infobar_bg;
extern struct t_config_option *config_color_infobar_delimiters;
extern struct t_config_option *config_color_infobar_highlight;
extern struct t_config_option *config_color_infobar_bg;
extern struct t_config_option *config_color_input;
extern struct t_config_option *config_color_input_bg;
extern struct t_config_option *config_color_input_server;
extern struct t_config_option *config_color_input_channel;
extern struct t_config_option *config_color_input_nick;
extern struct t_config_option *config_color_input_delimiters;
extern struct t_config_option *config_color_input_text_not_found;
extern struct t_config_option *config_color_input_actions;
extern struct t_config_option *config_color_nicklist;
extern struct t_config_option *config_color_nicklist_bg;
extern struct t_config_option *config_color_nicklist_group;
extern struct t_config_option *config_color_nicklist_away;
extern struct t_config_option *config_color_nicklist_prefix1;
extern struct t_config_option *config_color_nicklist_prefix2;
extern struct t_config_option *config_color_nicklist_prefix3;
extern struct t_config_option *config_color_nicklist_prefix4;
extern struct t_config_option *config_color_nicklist_prefix5;
extern struct t_config_option *config_color_nicklist_more;
extern struct t_config_option *config_color_nicklist_separator;
extern struct t_config_option *config_color_info;
extern struct t_config_option *config_color_info_bg;
extern struct t_config_option *config_color_info_waiting;
extern struct t_config_option *config_color_info_connecting;
extern struct t_config_option *config_color_info_active;
extern struct t_config_option *config_color_info_done;
extern struct t_config_option *config_color_info_failed;
extern struct t_config_option *config_color_info_aborted;

extern struct t_config_option *config_history_max_lines;
extern struct t_config_option *config_history_max_commands;
extern struct t_config_option *config_history_display_default;

extern struct t_config_option *config_proxy_use;
extern struct t_config_option *config_proxy_type;
extern struct t_config_option *config_proxy_ipv6;
extern struct t_config_option *config_proxy_address;
extern struct t_config_option *config_proxy_port;
extern struct t_config_option *config_proxy_username;
extern struct t_config_option *config_proxy_password;

extern struct t_config_option *config_plugins_path;
extern struct t_config_option *config_plugins_autoload;
extern struct t_config_option *config_plugins_extension;
extern struct t_config_option *config_plugins_save_config_on_unload;

extern int config_weechat_init ();
extern int config_weechat_read ();
extern int config_weechat_reload ();
extern int config_weechat_write ();
extern void config_weechat_print_stdout ();

#endif /* wee-config.h */
