/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2006 Emmanuel Bouthenot <kolter@openics.org>
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

#ifndef __WEECHAT_CONFIG_H
#define __WEECHAT_CONFIG_H 1

#include <regex.h>
#include "wee-config-file.h"

struct t_gui_buffer;

#define WEECHAT_CONFIG_NAME "weechat"

enum t_config_look_nicklist
{
    CONFIG_LOOK_NICKLIST_LEFT = 0,
    CONFIG_LOOK_NICKLIST_RIGHT,
    CONFIG_LOOK_NICKLIST_TOP,
    CONFIG_LOOK_NICKLIST_BOTTOM,
};

enum t_config_look_align_end_of_lines
{
    CONFIG_LOOK_ALIGN_END_OF_LINES_TIME = 0,
    CONFIG_LOOK_ALIGN_END_OF_LINES_BUFFER,
    CONFIG_LOOK_ALIGN_END_OF_LINES_PREFIX,
    CONFIG_LOOK_ALIGN_END_OF_LINES_SUFFIX,
    CONFIG_LOOK_ALIGN_END_OF_LINES_MESSAGE,
};

enum t_config_look_prefix_align
{
    CONFIG_LOOK_PREFIX_ALIGN_NONE = 0,
    CONFIG_LOOK_PREFIX_ALIGN_LEFT,
    CONFIG_LOOK_PREFIX_ALIGN_RIGHT,
};

enum t_config_look_prefix_buffer_align
{
    CONFIG_LOOK_PREFIX_BUFFER_ALIGN_NONE = 0,
    CONFIG_LOOK_PREFIX_BUFFER_ALIGN_LEFT,
    CONFIG_LOOK_PREFIX_BUFFER_ALIGN_RIGHT,
};

enum t_config_look_hotlist_sort
{
    CONFIG_LOOK_HOTLIST_SORT_GROUP_TIME_ASC = 0,
    CONFIG_LOOK_HOTLIST_SORT_GROUP_TIME_DESC,
    CONFIG_LOOK_HOTLIST_SORT_GROUP_NUMBER_ASC,
    CONFIG_LOOK_HOTLIST_SORT_GROUP_NUMBER_DESC,
    CONFIG_LOOK_HOTLIST_SORT_NUMBER_ASC,
    CONFIG_LOOK_HOTLIST_SORT_NUMBER_DESC,
};

enum t_config_look_input_share
{
    CONFIG_LOOK_INPUT_SHARE_NONE = 0,
    CONFIG_LOOK_INPUT_SHARE_COMMANDS,
    CONFIG_LOOK_INPUT_SHARE_TEXT,
    CONFIG_LOOK_INPUT_SHARE_ALL,
};

enum t_config_look_read_marker
{
    CONFIG_LOOK_READ_MARKER_NONE = 0,
    CONFIG_LOOK_READ_MARKER_LINE,
    CONFIG_LOOK_READ_MARKER_CHAR,
};

enum t_config_look_save_layout_on_exit
{
    CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_NONE = 0,
    CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_BUFFERS,
    CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_WINDOWS,
    CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_ALL,
};

extern struct t_config_file *weechat_config_file;
extern struct t_config_section *weechat_config_section_color;
extern struct t_config_section *weechat_config_section_proxy;
extern struct t_config_section *weechat_config_section_bar;
extern struct t_config_section *weechat_config_section_notify;

extern struct t_config_option *config_startup_command_after_plugins;
extern struct t_config_option *config_startup_command_before_plugins;
extern struct t_config_option *config_startup_display_logo;
extern struct t_config_option *config_startup_display_version;
extern struct t_config_option *config_startup_sys_rlimit;

extern struct t_config_option *config_look_align_end_of_lines;
extern struct t_config_option *config_look_bar_more_left;
extern struct t_config_option *config_look_bar_more_right;
extern struct t_config_option *config_look_bar_more_up;
extern struct t_config_option *config_look_bar_more_down;
extern struct t_config_option *config_look_buffer_notify_default;
extern struct t_config_option *config_look_buffer_time_format;
extern struct t_config_option *config_look_command_chars;
extern struct t_config_option *config_look_confirm_quit;
extern struct t_config_option *config_look_color_basic_force_bold;
extern struct t_config_option *config_look_color_inactive_window;
extern struct t_config_option *config_look_color_inactive_buffer;
extern struct t_config_option *config_look_color_inactive_time;
extern struct t_config_option *config_look_color_inactive_prefix_buffer;
extern struct t_config_option *config_look_color_inactive_prefix;
extern struct t_config_option *config_look_color_inactive_message;
extern struct t_config_option *config_look_color_nick_offline;
extern struct t_config_option *config_look_color_pairs_auto_reset;
extern struct t_config_option *config_look_color_real_white;
extern struct t_config_option *config_look_day_change;
extern struct t_config_option *config_look_day_change_time_format;
extern struct t_config_option *config_look_eat_newline_glitch;
extern struct t_config_option *config_look_highlight;
extern struct t_config_option *config_look_highlight_regex;
extern struct t_config_option *config_look_hotlist_add_buffer_if_away;
extern struct t_config_option *config_look_hotlist_buffer_separator;
extern struct t_config_option *config_look_hotlist_count_max;
extern struct t_config_option *config_look_hotlist_count_min_msg;
extern struct t_config_option *config_look_hotlist_names_count;
extern struct t_config_option *config_look_hotlist_names_length;
extern struct t_config_option *config_look_hotlist_names_level;
extern struct t_config_option *config_look_hotlist_names_merged_buffers;
extern struct t_config_option *config_look_hotlist_short_names;
extern struct t_config_option *config_look_hotlist_sort;
extern struct t_config_option *config_look_hotlist_unique_numbers;
extern struct t_config_option *config_look_input_cursor_scroll;
extern struct t_config_option *config_look_input_share;
extern struct t_config_option *config_look_input_share_overwrite;
extern struct t_config_option *config_look_input_undo_max;
extern struct t_config_option *config_look_item_time_format;
extern struct t_config_option *config_look_item_buffer_filter;
extern struct t_config_option *config_look_jump_current_to_previous_buffer;
extern struct t_config_option *config_look_jump_previous_buffer_when_closing;
extern struct t_config_option *config_look_jump_smart_back_to_buffer;
extern struct t_config_option *config_look_nick_prefix;
extern struct t_config_option *config_look_nick_suffix;
extern struct t_config_option *config_look_mouse;
extern struct t_config_option *config_look_mouse_timer_delay;
extern struct t_config_option *config_look_paste_bracketed;
extern struct t_config_option *config_look_paste_bracketed_timer_delay;
extern struct t_config_option *config_look_paste_max_lines;
extern struct t_config_option *config_look_prefix[];
extern struct t_config_option *config_look_prefix_align;
extern struct t_config_option *config_look_prefix_align_max;
extern struct t_config_option *config_look_prefix_align_min;
extern struct t_config_option *config_look_prefix_align_more;
extern struct t_config_option *config_look_prefix_buffer_align;
extern struct t_config_option *config_look_prefix_buffer_align_max;
extern struct t_config_option *config_look_prefix_buffer_align_more;
extern struct t_config_option *config_look_prefix_same_nick;
extern struct t_config_option *config_look_prefix_suffix;
extern struct t_config_option *config_look_read_marker;
extern struct t_config_option *config_look_read_marker_always_show;
extern struct t_config_option *config_look_read_marker_string;
extern struct t_config_option *config_look_save_config_on_exit;
extern struct t_config_option *config_look_save_layout_on_exit;
extern struct t_config_option *config_look_scroll_amount;
extern struct t_config_option *config_look_scroll_bottom_after_switch;
extern struct t_config_option *config_look_scroll_page_percent;
extern struct t_config_option *config_look_search_text_not_found_alert;
extern struct t_config_option *config_look_separator_horizontal;
extern struct t_config_option *config_look_separator_vertical;
extern struct t_config_option *config_look_set_title;
extern struct t_config_option *config_look_time_format;
extern struct t_config_option *config_look_window_separator_horizontal;
extern struct t_config_option *config_look_window_separator_vertical;

extern struct t_config_option *config_color_separator;
extern struct t_config_option *config_color_bar_more;
extern struct t_config_option *config_color_chat;
extern struct t_config_option *config_color_chat_bg;
extern struct t_config_option *config_color_chat_inactive_window;
extern struct t_config_option *config_color_chat_inactive_buffer;
extern struct t_config_option *config_color_chat_time;
extern struct t_config_option *config_color_chat_time_delimiters;
extern struct t_config_option *config_color_chat_prefix_buffer;
extern struct t_config_option *config_color_chat_prefix_buffer_inactive_buffer;
extern struct t_config_option *config_color_chat_prefix[];
extern struct t_config_option *config_color_chat_prefix_more;
extern struct t_config_option *config_color_chat_prefix_suffix;
extern struct t_config_option *config_color_chat_buffer;
extern struct t_config_option *config_color_chat_server;
extern struct t_config_option *config_color_chat_channel;
extern struct t_config_option *config_color_chat_nick;
extern struct t_config_option *config_color_chat_nick_colors;
extern struct t_config_option *config_color_chat_nick_prefix;
extern struct t_config_option *config_color_chat_nick_suffix;
extern struct t_config_option *config_color_chat_nick_self;
extern struct t_config_option *config_color_chat_nick_offline;
extern struct t_config_option *config_color_chat_nick_offline_highlight;
extern struct t_config_option *config_color_chat_nick_offline_highlight_bg;
extern struct t_config_option *config_color_chat_nick_other;
extern struct t_config_option *config_color_chat_host;
extern struct t_config_option *config_color_chat_delimiters;
extern struct t_config_option *config_color_chat_highlight;
extern struct t_config_option *config_color_chat_highlight_bg;
extern struct t_config_option *config_color_chat_read_marker;
extern struct t_config_option *config_color_chat_read_marker_bg;
extern struct t_config_option *config_color_chat_tags;
extern struct t_config_option *config_color_chat_text_found;
extern struct t_config_option *config_color_chat_text_found_bg;
extern struct t_config_option *config_color_chat_value;
extern struct t_config_option *config_color_status_number;
extern struct t_config_option *config_color_status_name;
extern struct t_config_option *config_color_status_name_ssl;
extern struct t_config_option *config_color_status_filter;
extern struct t_config_option *config_color_status_data_msg;
extern struct t_config_option *config_color_status_data_private;
extern struct t_config_option *config_color_status_data_highlight;
extern struct t_config_option *config_color_status_data_other;
extern struct t_config_option *config_color_status_count_msg;
extern struct t_config_option *config_color_status_count_private;
extern struct t_config_option *config_color_status_count_highlight;
extern struct t_config_option *config_color_status_count_other;
extern struct t_config_option *config_color_status_more;
extern struct t_config_option *config_color_status_time;
extern struct t_config_option *config_color_input_text_not_found;
extern struct t_config_option *config_color_input_actions;
extern struct t_config_option *config_color_nicklist_group;
extern struct t_config_option *config_color_nicklist_away;
extern struct t_config_option *config_color_nicklist_offline;

extern struct t_config_option *config_completion_base_word_until_cursor;
extern struct t_config_option *config_completion_default_template;
extern struct t_config_option *config_completion_nick_add_space;
extern struct t_config_option *config_completion_nick_completer;
extern struct t_config_option *config_completion_nick_first_only;
extern struct t_config_option *config_completion_nick_ignore_chars;
extern struct t_config_option *config_completion_partial_completion_alert;
extern struct t_config_option *config_completion_partial_completion_command;
extern struct t_config_option *config_completion_partial_completion_command_arg;
extern struct t_config_option *config_completion_partial_completion_other;
extern struct t_config_option *config_completion_partial_completion_count;

extern struct t_config_option *config_history_max_buffer_lines_number;
extern struct t_config_option *config_history_max_buffer_lines_minutes;
extern struct t_config_option *config_history_max_commands;
extern struct t_config_option *config_history_max_visited_buffers;
extern struct t_config_option *config_history_display_default;

extern struct t_config_option *config_network_connection_timeout;
extern struct t_config_option *config_network_gnutls_ca_file;
extern struct t_config_option *config_network_gnutls_handshake_timeout;

extern struct t_config_option *config_plugin_autoload;
extern struct t_config_option *config_plugin_debug;
extern struct t_config_option *config_plugin_extension;
extern struct t_config_option *config_plugin_path;
extern struct t_config_option *config_plugin_save_config_on_unload;

extern int config_length_nick_prefix_suffix;
extern int config_length_prefix_same_nick;
extern regex_t *config_highlight_regex;
extern char **config_highlight_tags;
extern int config_num_highlight_tags;
extern char **config_plugin_extensions;
extern int config_num_plugin_extensions;


extern struct t_config_option *config_weechat_debug_get (const char *plugin_name);
extern int config_weechat_debug_set (const char *plugin_name,
                                     const char *value);
extern void config_weechat_debug_set_all ();
extern int config_weechat_notify_set (struct t_gui_buffer *buffer,
                                      const char *notify);
extern int config_weechat_init ();
extern int config_weechat_read ();
extern int config_weechat_write ();
extern void config_weechat_free ();

#endif /* __WEECHAT_CONFIG_H */
