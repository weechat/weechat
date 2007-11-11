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


#ifndef __WEECHAT_CONFIG_H
#define __WEECHAT_CONFIG_H 1

#include "wee-config-option.h"
#include "wee-config-file.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"

#define WEECHAT_CONFIG_NAME "weechat.rc"

#define CFG_LOOK_NICKLIST_LEFT   0
#define CFG_LOOK_NICKLIST_RIGHT  1
#define CFG_LOOK_NICKLIST_TOP    2
#define CFG_LOOK_NICKLIST_BOTTOM 3

#define CFG_LOOK_PREFIX_ALIGN_NONE  0
#define CFG_LOOK_PREFIX_ALIGN_LEFT  1
#define CFG_LOOK_PREFIX_ALIGN_RIGHT 2

#define CFG_LOOK_HOTLIST_SORT_GROUP_TIME_ASC    0
#define CFG_LOOK_HOTLIST_SORT_GROUP_TIME_DESC   1
#define CFG_LOOK_HOTLIST_SORT_GROUP_NUMBER_ASC  2
#define CFG_LOOK_HOTLIST_SORT_GROUP_NUMBER_DESC 3
#define CFG_LOOK_HOTLIST_SORT_NUMBER_ASC        4
#define CFG_LOOK_HOTLIST_SORT_NUMBER_DESC       5

extern struct t_config_option weechat_options_look[];
extern int cfg_look_color_real_white;
extern int cfg_look_save_on_exit;
extern int cfg_look_set_title;
extern int cfg_look_startup_logo;
extern int cfg_look_startup_version;
extern char *cfg_look_weechat_slogan;
extern int cfg_look_one_server_buffer;
extern int cfg_look_open_near_server;
extern int cfg_look_scroll_amount;
extern char *cfg_look_buffer_time_format;
extern int cfg_look_color_nicks_number;
extern int cfg_look_color_actions;
extern int cfg_look_nicklist;
extern int cfg_look_nicklist_position;
extern int cfg_look_nicklist_min_size;
extern int cfg_look_nicklist_max_size;
extern int cfg_look_nicklist_separator;
extern int cfg_look_nickmode;
extern int cfg_look_nickmode_empty;
extern char *cfg_look_no_nickname;
extern char *cfg_look_prefix[GUI_CHAT_PREFIX_NUMBER];
extern int cfg_look_prefix_align;
extern int cfg_look_prefix_align_max;
extern char *cfg_look_prefix_suffix;
extern int cfg_look_align_text_offset;
extern char *cfg_look_nick_completor;
extern char *cfg_look_nick_completion_ignore;
extern int cfg_look_nick_completion_smart;
extern int cfg_look_nick_complete_first;
extern int cfg_look_infobar;
extern char *cfg_look_infobar_time_format;
extern int cfg_look_infobar_seconds;
extern int cfg_look_infobar_delay_highlight;
extern int cfg_look_hotlist_names_count;
extern int cfg_look_hotlist_names_level;
extern int cfg_look_hotlist_names_length;
extern int cfg_look_hotlist_sort;
extern int cfg_look_day_change;
extern char *cfg_look_day_change_time_format;
extern char *cfg_look_read_marker;
extern char *cfg_look_input_format;
extern int cfg_look_paste_max_lines;

extern struct t_config_option weechat_options_colors[];
extern int cfg_col_separator;
extern int cfg_col_title;
extern int cfg_col_title_bg;
extern int cfg_col_title_more;
extern int cfg_col_chat;
extern int cfg_col_chat_bg;
extern int cfg_col_chat_time;
extern int cfg_col_chat_time_delimiters;
extern int cfg_col_chat_prefix[GUI_CHAT_PREFIX_NUMBER];
extern int cfg_col_chat_prefix_more;
extern int cfg_col_chat_prefix_suffix;
extern int cfg_col_chat_buffer;
extern int cfg_col_chat_server;
extern int cfg_col_chat_channel;
extern int cfg_col_chat_nick;
extern int cfg_col_chat_nick_self;
extern int cfg_col_chat_nick_other;
extern int cfg_col_chat_nick_colors[GUI_COLOR_NICK_NUMBER];
extern int cfg_col_chat_host;
extern int cfg_col_chat_delimiters;
extern int cfg_col_chat_highlight;
extern int cfg_col_chat_read_marker;
extern int cfg_col_chat_read_marker_bg;
extern int cfg_col_status;
extern int cfg_col_status_bg;
extern int cfg_col_status_delimiters;
extern int cfg_col_status_channel;
extern int cfg_col_status_data_msg;
extern int cfg_col_status_data_private;
extern int cfg_col_status_data_highlight;
extern int cfg_col_status_data_other;
extern int cfg_col_status_more;
extern int cfg_col_infobar;
extern int cfg_col_infobar_bg;
extern int cfg_col_infobar_delimiters;
extern int cfg_col_infobar_highlight;
extern int cfg_col_infobar_bg;
extern int cfg_col_input;
extern int cfg_col_input_bg;
extern int cfg_col_input_server;
extern int cfg_col_input_channel;
extern int cfg_col_input_nick;
extern int cfg_col_input_delimiters;
extern int cfg_col_input_text_not_found;
extern int cfg_col_input_actions;
extern int cfg_col_nicklist;
extern int cfg_col_nicklist_bg;
extern int cfg_col_nicklist_away;
extern int cfg_col_nicklist_prefix1;
extern int cfg_col_nicklist_prefix2;
extern int cfg_col_nicklist_prefix3;
extern int cfg_col_nicklist_prefix4;
extern int cfg_col_nicklist_prefix5;
extern int cfg_col_nicklist_more;
extern int cfg_col_nicklist_separator;
extern int cfg_col_info;
extern int cfg_col_info_bg;
extern int cfg_col_info_waiting;
extern int cfg_col_info_connecting;
extern int cfg_col_info_active;
extern int cfg_col_info_done;
extern int cfg_col_info_failed;
extern int cfg_col_info_aborted;

extern struct t_config_option weechat_options_history[];
extern int cfg_history_max_lines;
extern int cfg_history_max_commands;
extern int cfg_history_display_default;

extern struct t_config_option weechat_options_proxy[];
extern int cfg_proxy_use;
extern int cfg_proxy_type;
extern char *cfg_proxy_type_values[];
extern int cfg_proxy_ipv6;
extern char *cfg_proxy_address;
extern int cfg_proxy_port;
extern char *cfg_proxy_username;
extern char *cfg_proxy_password;

extern struct t_config_option weechat_options_plugins[];
extern char *cfg_plugins_path;
extern char *cfg_plugins_autoload;
extern char *cfg_plugins_extension;

extern char *weechat_config_sections[];
extern struct t_config_option *weechat_config_options[];

extern void weechat_config_change_noop ();
extern void weechat_config_change_save_on_exit ();
extern void weechat_config_change_title ();
extern void weechat_config_change_buffers ();
extern void weechat_config_change_buffer_content ();
extern void weechat_config_change_buffer_time_format ();
extern void weechat_config_change_hotlist ();
extern void weechat_config_change_read_marker ();
extern void weechat_config_change_prefix ();
extern void weechat_config_change_color ();
extern void weechat_config_change_nicks_colors ();

extern int weechat_config_read_alias (struct t_config_option *, char *, char *);
extern int weechat_config_read_key (struct t_config_option *, char *, char *);
extern int weechat_config_read ();
extern int weechat_config_write_alias (FILE *, char *, struct t_config_option *);
extern int weechat_config_write_keys (FILE *, char *, struct t_config_option *);
extern int weechat_config_write_alias_default_values (FILE *, char *,
                                                      struct t_config_option *);
extern int weechat_config_write_keys_default_values (FILE *, char *,
                                                     struct t_config_option *);
extern int weechat_config_write ();
extern void weechat_config_print_stdout ();

#endif /* wee-config.h */
