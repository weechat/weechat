/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEECHAT_CONFIG_H
#define WEECHAT_CONFIG_H

#include <regex.h>
#include <wctype.h>

#include "core-config-file.h"

struct t_gui_buffer;

#define WEECHAT_CONFIG_NAME "weechat"
#define WEECHAT_CONFIG_PRIO_NAME "110000|weechat"

#define WEECHAT_CONFIG_VERSION 3

#define TAB_MAX_WIDTH 64

enum t_config_look_align_end_of_lines
{
    CONFIG_LOOK_ALIGN_END_OF_LINES_TIME = 0,
    CONFIG_LOOK_ALIGN_END_OF_LINES_BUFFER,
    CONFIG_LOOK_ALIGN_END_OF_LINES_PREFIX,
    CONFIG_LOOK_ALIGN_END_OF_LINES_SUFFIX,
    CONFIG_LOOK_ALIGN_END_OF_LINES_MESSAGE,
};

enum t_config_look_buffer_position
{
    CONFIG_LOOK_BUFFER_POSITION_END = 0,
    CONFIG_LOOK_BUFFER_POSITION_FIRST_GAP,
};

enum t_config_look_buffer_search_history
{
    CONFIG_LOOK_BUFFER_SEARCH_HISTORY_LOCAL = 0,
    CONFIG_LOOK_BUFFER_SEARCH_HISTORY_GLOBAL,
};

enum t_config_look_buffer_search_where
{
    CONFIG_LOOK_BUFFER_SEARCH_PREFIX = 0,
    CONFIG_LOOK_BUFFER_SEARCH_MESSAGE,
    CONFIG_LOOK_BUFFER_SEARCH_PREFIX_MESSAGE,
};

enum t_config_look_nick_color_hash
{
    CONFIG_LOOK_NICK_COLOR_HASH_DJB2 = 0,
    CONFIG_LOOK_NICK_COLOR_HASH_SUM,
    CONFIG_LOOK_NICK_COLOR_HASH_DJB2_32,
    CONFIG_LOOK_NICK_COLOR_HASH_SUM_32,
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

enum t_config_look_hotlist_remove
{
    CONFIG_LOOK_HOTLIST_REMOVE_BUFFER = 0,
    CONFIG_LOOK_HOTLIST_REMOVE_MERGED,
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

struct t_config_look_word_char_item
{
    char exclude;                      /* 1 if char is NOT a word char      */
    wctype_t wc_class;                 /* class of wide characters (wctype) */
    wint_t char1;                      /* first char of range               */
    wint_t char2;                      /* second char of range              */
};

extern struct t_config_file *weechat_config_file;

extern struct t_config_section *weechat_config_section_debug;
extern struct t_config_section *weechat_config_section_startup;
extern struct t_config_section *weechat_config_section_look;
extern struct t_config_section *weechat_config_section_palette;
extern struct t_config_section *weechat_config_section_color;
extern struct t_config_section *weechat_config_section_completion;
extern struct t_config_section *weechat_config_section_history;
extern struct t_config_section *weechat_config_section_network;
extern struct t_config_section *weechat_config_section_proxy;
extern struct t_config_section *weechat_config_section_plugin;
extern struct t_config_section *weechat_config_section_signal;
extern struct t_config_section *weechat_config_section_bar;
extern struct t_config_section *weechat_config_section_custom_bar_item;
extern struct t_config_section *weechat_config_section_layout;
extern struct t_config_section *weechat_config_section_buffer;
extern struct t_config_section *weechat_config_section_notify;
extern struct t_config_section *weechat_config_section_filter;
extern struct t_config_section *weechat_config_section_key[];

extern struct t_config_option *config_startup_command_after_plugins;
extern struct t_config_option *config_startup_command_before_plugins;
extern struct t_config_option *config_startup_display_logo;
extern struct t_config_option *config_startup_display_version;
extern struct t_config_option *config_startup_sys_rlimit;

extern struct t_config_option *config_look_align_end_of_lines;
extern struct t_config_option *config_look_align_multiline_words;
extern struct t_config_option *config_look_bar_more_down;
extern struct t_config_option *config_look_bar_more_left;
extern struct t_config_option *config_look_bar_more_right;
extern struct t_config_option *config_look_bar_more_up;
extern struct t_config_option *config_look_bare_display_exit_on_input;
extern struct t_config_option *config_look_bare_display_time_format;
extern struct t_config_option *config_look_buffer_auto_renumber;
extern struct t_config_option *config_look_buffer_notify_default;
extern struct t_config_option *config_look_buffer_position;
extern struct t_config_option *config_look_buffer_search_case_sensitive;
extern struct t_config_option *config_look_buffer_search_history;
extern struct t_config_option *config_look_buffer_search_force_default;
extern struct t_config_option *config_look_buffer_search_regex;
extern struct t_config_option *config_look_buffer_search_where;
extern struct t_config_option *config_look_buffer_time_format;
extern struct t_config_option *config_look_buffer_time_same;
extern struct t_config_option *config_look_chat_space_right;
extern struct t_config_option *config_look_color_basic_force_bold;
extern struct t_config_option *config_look_color_inactive_buffer;
extern struct t_config_option *config_look_color_inactive_message;
extern struct t_config_option *config_look_color_inactive_prefix;
extern struct t_config_option *config_look_color_inactive_prefix_buffer;
extern struct t_config_option *config_look_color_inactive_time;
extern struct t_config_option *config_look_color_inactive_window;
extern struct t_config_option *config_look_color_nick_offline;
extern struct t_config_option *config_look_color_pairs_auto_reset;
extern struct t_config_option *config_look_color_real_white;
extern struct t_config_option *config_look_command_chars;
extern struct t_config_option *config_look_command_incomplete;
extern struct t_config_option *config_look_config_permissions;
extern struct t_config_option *config_look_confirm_quit;
extern struct t_config_option *config_look_confirm_upgrade;
extern struct t_config_option *config_look_day_change;
extern struct t_config_option *config_look_day_change_message_1date;
extern struct t_config_option *config_look_day_change_message_2dates;
extern struct t_config_option *config_look_eat_newline_glitch;
extern struct t_config_option *config_look_emphasized_attributes;
extern struct t_config_option *config_look_highlight;
extern struct t_config_option *config_look_highlight_disable_regex;
extern struct t_config_option *config_look_highlight_prefix;
extern struct t_config_option *config_look_highlight_regex;
extern struct t_config_option *config_look_highlight_tags;
extern struct t_config_option *config_look_hotlist_add_conditions;
extern struct t_config_option *config_look_hotlist_buffer_separator;
extern struct t_config_option *config_look_hotlist_count_max;
extern struct t_config_option *config_look_hotlist_count_min_msg;
extern struct t_config_option *config_look_hotlist_names_count;
extern struct t_config_option *config_look_hotlist_names_length;
extern struct t_config_option *config_look_hotlist_names_level;
extern struct t_config_option *config_look_hotlist_names_merged_buffers;
extern struct t_config_option *config_look_hotlist_prefix;
extern struct t_config_option *config_look_hotlist_remove;
extern struct t_config_option *config_look_hotlist_short_names;
extern struct t_config_option *config_look_hotlist_sort;
extern struct t_config_option *config_look_hotlist_suffix;
extern struct t_config_option *config_look_hotlist_unique_numbers;
extern struct t_config_option *config_look_hotlist_update_on_buffer_switch;
extern struct t_config_option *config_look_input_cursor_scroll;
extern struct t_config_option *config_look_input_multiline_lead_linebreak;
extern struct t_config_option *config_look_input_share;
extern struct t_config_option *config_look_input_share_overwrite;
extern struct t_config_option *config_look_input_undo_max;
extern struct t_config_option *config_look_item_away_message;
extern struct t_config_option *config_look_item_buffer_filter;
extern struct t_config_option *config_look_item_buffer_zoom;
extern struct t_config_option *config_look_item_mouse_status;
extern struct t_config_option *config_look_item_time_format;
extern struct t_config_option *config_look_jump_current_to_previous_buffer;
extern struct t_config_option *config_look_jump_previous_buffer_when_closing;
extern struct t_config_option *config_look_jump_smart_back_to_buffer;
extern struct t_config_option *config_look_key_bind_safe;
extern struct t_config_option *config_look_key_grab_delay;
extern struct t_config_option *config_look_mouse;
extern struct t_config_option *config_look_nick_color_force;
extern struct t_config_option *config_look_nick_color_hash;
extern struct t_config_option *config_look_nick_color_hash_salt;
extern struct t_config_option *config_look_nick_color_stop_chars;
extern struct t_config_option *config_look_nick_prefix;
extern struct t_config_option *config_look_nick_suffix;
extern struct t_config_option *config_look_paste_auto_add_newline;
extern struct t_config_option *config_look_paste_bracketed;
extern struct t_config_option *config_look_paste_bracketed_timer_delay;
extern struct t_config_option *config_look_paste_max_lines;
extern struct t_config_option *config_look_prefix[];
extern struct t_config_option *config_look_prefix_align;
extern struct t_config_option *config_look_prefix_align_max;
extern struct t_config_option *config_look_prefix_align_min;
extern struct t_config_option *config_look_prefix_align_more;
extern struct t_config_option *config_look_prefix_align_more_after;
extern struct t_config_option *config_look_prefix_buffer_align;
extern struct t_config_option *config_look_prefix_buffer_align_max;
extern struct t_config_option *config_look_prefix_buffer_align_more;
extern struct t_config_option *config_look_prefix_buffer_align_more_after;
extern struct t_config_option *config_look_prefix_same_nick;
extern struct t_config_option *config_look_prefix_same_nick_middle;
extern struct t_config_option *config_look_prefix_suffix;
extern struct t_config_option *config_look_quote_nick_prefix;
extern struct t_config_option *config_look_quote_nick_suffix;
extern struct t_config_option *config_look_quote_time_format;
extern struct t_config_option *config_look_read_marker;
extern struct t_config_option *config_look_read_marker_always_show;
extern struct t_config_option *config_look_read_marker_string;
extern struct t_config_option *config_look_read_marker_update_on_buffer_switch;
extern struct t_config_option *config_look_save_config_on_exit;
extern struct t_config_option *config_look_save_config_with_fsync;
extern struct t_config_option *config_look_save_layout_on_exit;
extern struct t_config_option *config_look_scroll_amount;
extern struct t_config_option *config_look_scroll_bottom_after_switch;
extern struct t_config_option *config_look_scroll_page_percent;
extern struct t_config_option *config_look_search_text_not_found_alert;
extern struct t_config_option *config_look_separator_horizontal;
extern struct t_config_option *config_look_separator_vertical;
extern struct t_config_option *config_look_tab_width;
extern struct t_config_option *config_look_time_format;
extern struct t_config_option *config_look_window_auto_zoom;
extern struct t_config_option *config_look_window_separator_horizontal;
extern struct t_config_option *config_look_window_separator_vertical;
extern struct t_config_option *config_look_window_title;
extern struct t_config_option *config_look_word_chars_highlight;
extern struct t_config_option *config_look_word_chars_input;

extern struct t_config_option *config_color_bar_more;
extern struct t_config_option *config_color_chat;
extern struct t_config_option *config_color_chat_bg;
extern struct t_config_option *config_color_chat_buffer;
extern struct t_config_option *config_color_chat_channel;
extern struct t_config_option *config_color_chat_day_change;
extern struct t_config_option *config_color_chat_delimiters;
extern struct t_config_option *config_color_chat_highlight;
extern struct t_config_option *config_color_chat_highlight_bg;
extern struct t_config_option *config_color_chat_host;
extern struct t_config_option *config_color_chat_inactive_buffer;
extern struct t_config_option *config_color_chat_inactive_window;
extern struct t_config_option *config_color_chat_nick;
extern struct t_config_option *config_color_chat_nick_colors;
extern struct t_config_option *config_color_chat_nick_offline;
extern struct t_config_option *config_color_chat_nick_offline_highlight;
extern struct t_config_option *config_color_chat_nick_offline_highlight_bg;
extern struct t_config_option *config_color_chat_nick_other;
extern struct t_config_option *config_color_chat_nick_prefix;
extern struct t_config_option *config_color_chat_nick_self;
extern struct t_config_option *config_color_chat_nick_suffix;
extern struct t_config_option *config_color_chat_prefix[];
extern struct t_config_option *config_color_chat_prefix_buffer;
extern struct t_config_option *config_color_chat_prefix_buffer_inactive_buffer;
extern struct t_config_option *config_color_chat_prefix_more;
extern struct t_config_option *config_color_chat_prefix_suffix;
extern struct t_config_option *config_color_chat_read_marker;
extern struct t_config_option *config_color_chat_read_marker_bg;
extern struct t_config_option *config_color_chat_server;
extern struct t_config_option *config_color_chat_status_disabled;
extern struct t_config_option *config_color_chat_status_enabled;
extern struct t_config_option *config_color_chat_tags;
extern struct t_config_option *config_color_chat_text_found;
extern struct t_config_option *config_color_chat_text_found_bg;
extern struct t_config_option *config_color_chat_time;
extern struct t_config_option *config_color_chat_time_delimiters;
extern struct t_config_option *config_color_chat_value;
extern struct t_config_option *config_color_chat_value_null;
extern struct t_config_option *config_color_emphasized;
extern struct t_config_option *config_color_emphasized_bg;
extern struct t_config_option *config_color_eval_syntax_colors;
extern struct t_config_option *config_color_input_actions;
extern struct t_config_option *config_color_input_text_not_found;
extern struct t_config_option *config_color_item_away;
extern struct t_config_option *config_color_nicklist_away;
extern struct t_config_option *config_color_nicklist_group;
extern struct t_config_option *config_color_separator;
extern struct t_config_option *config_color_status_count_highlight;
extern struct t_config_option *config_color_status_count_msg;
extern struct t_config_option *config_color_status_count_other;
extern struct t_config_option *config_color_status_count_private;
extern struct t_config_option *config_color_status_data_highlight;
extern struct t_config_option *config_color_status_data_msg;
extern struct t_config_option *config_color_status_data_other;
extern struct t_config_option *config_color_status_data_private;
extern struct t_config_option *config_color_status_filter;
extern struct t_config_option *config_color_status_modes;
extern struct t_config_option *config_color_status_more;
extern struct t_config_option *config_color_status_mouse;
extern struct t_config_option *config_color_status_name;
extern struct t_config_option *config_color_status_name_tls;
extern struct t_config_option *config_color_status_nicklist_count;
extern struct t_config_option *config_color_status_number;
extern struct t_config_option *config_color_status_time;

extern struct t_config_option *config_completion_base_word_until_cursor;
extern struct t_config_option *config_completion_case_sensitive;
extern struct t_config_option *config_completion_command_inline;
extern struct t_config_option *config_completion_default_template;
extern struct t_config_option *config_completion_nick_add_space;
extern struct t_config_option *config_completion_nick_case_sensitive;
extern struct t_config_option *config_completion_nick_completer;
extern struct t_config_option *config_completion_nick_first_only;
extern struct t_config_option *config_completion_nick_ignore_chars;
extern struct t_config_option *config_completion_partial_completion_alert;
extern struct t_config_option *config_completion_partial_completion_command;
extern struct t_config_option *config_completion_partial_completion_command_arg;
extern struct t_config_option *config_completion_partial_completion_count;
extern struct t_config_option *config_completion_partial_completion_other;
extern struct t_config_option *config_completion_partial_completion_templates;

extern struct t_config_option *config_history_display_default;
extern struct t_config_option *config_history_max_buffer_lines_minutes;
extern struct t_config_option *config_history_max_buffer_lines_number;
extern struct t_config_option *config_history_max_commands;
extern struct t_config_option *config_history_max_visited_buffers;

extern struct t_config_option *config_network_connection_timeout;
extern struct t_config_option *config_network_gnutls_ca_system;
extern struct t_config_option *config_network_gnutls_ca_user;
extern struct t_config_option *config_network_gnutls_handshake_timeout;
extern struct t_config_option *config_network_proxy_curl;

extern struct t_config_option *config_plugin_autoload;
extern struct t_config_option *config_plugin_extension;
extern struct t_config_option *config_plugin_path;
extern struct t_config_option *config_plugin_save_config_on_unload;

extern struct t_config_option *config_signal_sighup;
extern struct t_config_option *config_signal_sigquit;
extern struct t_config_option *config_signal_sigterm;
extern struct t_config_option *config_signal_sigusr1;
extern struct t_config_option *config_signal_sigusr2;

extern int config_length_nick_prefix_suffix;
extern int config_length_prefix_same_nick;
extern int config_length_prefix_same_nick_middle;
extern int config_emphasized_attributes;
extern regex_t *config_highlight_disable_regex;
extern regex_t *config_highlight_regex;
extern char ***config_highlight_tags;
extern int config_num_highlight_tags;
extern char **config_plugin_extensions;
extern int config_num_plugin_extensions;
extern char config_tab_spaces[];
extern struct t_config_look_word_char_item *config_word_chars_highlight;
extern int config_word_chars_highlight_count;
extern struct t_config_look_word_char_item *config_word_chars_input;
extern int config_word_chars_input_count;
extern char **config_nick_colors;
extern int config_num_nick_colors;
extern struct t_hashtable *config_hashtable_nick_color_force;
extern char **config_eval_syntax_colors;
extern int config_num_eval_syntax_colors;
extern char *config_buffer_time_same_evaluated;
extern struct t_hashtable *config_hashtable_completion_partial_templates;
extern char **config_hotlist_sort_fields;
extern int config_num_hotlist_sort_fields;

extern void config_set_nick_colors ();
extern struct t_config_option *config_weechat_debug_get (const char *plugin_name);
extern int config_weechat_debug_set (const char *plugin_name,
                                     const char *value);
extern void config_weechat_debug_set_all ();
extern int config_weechat_buffer_set (struct t_gui_buffer *buffer,
                                      const char *property, const char *value);
extern int config_weechat_notify_set (struct t_gui_buffer *buffer,
                                      const char *notify);
extern void config_get_item_time (char *text_time, int max_length);
extern int config_weechat_get_key_context (struct t_config_section *section);
extern int config_weechat_init ();
extern int config_weechat_read ();
extern int config_weechat_write ();
extern void config_weechat_free ();

#endif /* WEECHAT_CONFIG_H */
