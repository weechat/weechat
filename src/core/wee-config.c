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

/* wee-config.c: WeeChat configuration options */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "weechat.h"
#include "wee-config.h"
#include "wee-alias.h"
#include "wee-command.h"
#include "wee-config-file.h"
#include "wee-list.h"
#include "wee-log.h"
#include "wee-string.h"
#include "wee-util.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-status.h"
#include "../gui/gui-window.h"


/* config, look & feel section */

int cfg_look_color_real_white;
int cfg_look_save_on_exit;
int cfg_look_set_title;
int cfg_look_startup_logo;
int cfg_look_startup_version;
char *cfg_look_weechat_slogan;
int cfg_look_scroll_amount;
char *cfg_look_buffer_time_format;
int cfg_look_color_nicks_number;
int cfg_look_color_actions;
int cfg_look_nicklist;
int cfg_look_nicklist_position;
char *cfg_look_nicklist_position_values[] =
{ "left", "right", "top", "bottom", NULL };
int cfg_look_nicklist_min_size;
int cfg_look_nicklist_max_size;
int cfg_look_nicklist_separator;
int cfg_look_nickmode;
int cfg_look_nickmode_empty;
char *cfg_look_no_nickname;
char *cfg_look_prefix[GUI_CHAT_PREFIX_NUMBER];
int cfg_look_prefix_align;
char *cfg_look_prefix_align_values[] =
{ "none", "left", "right", NULL };
int cfg_look_prefix_align_max;
char *cfg_look_prefix_suffix;
int cfg_look_align_text_offset;
char *cfg_look_nick_completor;
char *cfg_look_nick_completion_ignore;
int cfg_look_nick_completion_smart;
int cfg_look_nick_complete_first;
int cfg_look_infobar;
char *cfg_look_infobar_time_format;
int cfg_look_infobar_seconds;
int cfg_look_infobar_delay_highlight;
int cfg_look_hotlist_names_count;
int cfg_look_hotlist_names_level;
int cfg_look_hotlist_names_length;
int cfg_look_hotlist_sort;
char *cfg_look_hotlist_sort_values[] =
{ "group_time_asc", "group_time_desc",
  "group_number_asc", "group_number_desc",
  "number_asc", "number_desc" };
int cfg_look_day_change;
char *cfg_look_day_change_time_format;
char *cfg_look_read_marker;
char *cfg_look_input_format;
int cfg_look_paste_max_lines;

struct t_config_option weechat_options_look[] =
{ { "look_color_real_white",
    N_("if set, uses real white color, disabled by default for terms with "
       "white background (if you never use white background, you should turn "
       "on this option to see real white instead of default term foreground "
       "color)"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &cfg_look_color_real_white, NULL, weechat_config_change_color },
    { "look_save_on_exit",
    N_("save configuration file on exit"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_save_on_exit, NULL, weechat_config_change_save_on_exit },
  { "look_set_title",
    N_("set title for window (terminal for Curses GUI) with name and version"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_set_title, NULL, weechat_config_change_title },
  { "look_startup_logo",
    N_("display WeeChat logo at startup"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_startup_logo, NULL, weechat_config_change_noop },
  { "look_startup_version",
    N_("display WeeChat version at startup"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_startup_version, NULL, weechat_config_change_noop },
  { "look_weechat_slogan",
    N_("WeeChat slogan (if empty, slogan is not used)"),
    OPTION_TYPE_STRING, 0, 0, 0, "the geekest IRC client!", NULL,
    NULL, &cfg_look_weechat_slogan, weechat_config_change_noop },
  { "look_scroll_amount",
    N_("how many lines to scroll by with scroll_up and scroll_down"),
    OPTION_TYPE_INT, 1, INT_MAX, 3, NULL, NULL,
    &cfg_look_scroll_amount, NULL, weechat_config_change_buffer_content },
  { "look_buffer_time_format",
    N_("time format for buffers"),
    OPTION_TYPE_STRING, 0, 0, 0, "[%H:%M:%S]", NULL,
    NULL, &cfg_look_buffer_time_format, weechat_config_change_buffer_time_format },
  { "look_color_nicks_number",
    N_("number of colors to use for nicks colors"),
    OPTION_TYPE_INT, 1, 10, 10, NULL, NULL,
    &cfg_look_color_nicks_number, NULL, weechat_config_change_nicks_colors },
  { "look_color_actions",
    N_("display actions with different colors"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_color_actions, NULL, weechat_config_change_noop },
  { "look_nicklist",
    N_("display nicklist window (for channel windows)"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_nicklist, NULL, weechat_config_change_buffers },
  { "look_nicklist_position",
    N_("nicklist position (top, left, right (default), bottom)"),
    OPTION_TYPE_INT_WITH_STRING, 0, 0, 0, "right", cfg_look_nicklist_position_values,
    &cfg_look_nicklist_position, NULL, weechat_config_change_buffers },
  { "look_nicklist_min_size",
    N_("min size for nicklist (width or height, depending on look_nicklist_position "
       "(0 = no min size))"),
    OPTION_TYPE_INT, 0, 100, 0, NULL, NULL,
    &cfg_look_nicklist_min_size, NULL, weechat_config_change_buffers },
  { "look_nicklist_max_size",
    N_("max size for nicklist (width or height, depending on look_nicklist_position "
       "(0 = no max size; if min = max and > 0, then size is fixed))"),
    OPTION_TYPE_INT, 0, 100, 0, NULL, NULL,
    &cfg_look_nicklist_max_size, NULL, weechat_config_change_buffers },
  { "look_nicklist_separator",
    N_("separator between chat and nicklist"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_nicklist_separator, NULL, weechat_config_change_buffers },
  { "look_no_nickname",
    N_("text to display instead of nick when not connected"),
    OPTION_TYPE_STRING, 0, 0, 0, "-cmd-", NULL,
    NULL, &cfg_look_no_nickname, weechat_config_change_buffer_content },
  { "look_nickmode",
    N_("display nick mode ((half)op/voice) before each nick"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_nickmode, NULL, weechat_config_change_buffers },
  { "look_nickmode_empty",
    N_("display space if nick mode is not (half)op/voice"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &cfg_look_nickmode_empty, NULL, weechat_config_change_buffers },
  { "look_prefix_info",
    N_("prefix for info messages"),
    OPTION_TYPE_STRING, 0, 0, 0, "-=-", NULL,
    NULL, &cfg_look_prefix[GUI_CHAT_PREFIX_INFO], weechat_config_change_prefix },
  { "look_prefix_error",
    N_("prefix for error messages"),
    OPTION_TYPE_STRING, 0, 0, 0, "=!=", NULL,
    NULL, &cfg_look_prefix[GUI_CHAT_PREFIX_ERROR], weechat_config_change_prefix },
  { "look_prefix_network",
    N_("prefix for network messages"),
    OPTION_TYPE_STRING, 0, 0, 0, "-@-", NULL,
    NULL, &cfg_look_prefix[GUI_CHAT_PREFIX_NETWORK], weechat_config_change_prefix },
  { "look_prefix_action",
    N_("prefix for action messages"),
    OPTION_TYPE_STRING, 0, 0, 0, "-*-", NULL,
    NULL, &cfg_look_prefix[GUI_CHAT_PREFIX_ACTION], weechat_config_change_prefix },
  { "look_prefix_join",
    N_("prefix for join messages"),
    OPTION_TYPE_STRING, 0, 0, 0, "-->", NULL,
    NULL, &cfg_look_prefix[GUI_CHAT_PREFIX_JOIN], weechat_config_change_prefix },
  { "look_prefix_quit",
    N_("prefix for quit messages"),
    OPTION_TYPE_STRING, 0, 0, 0, "<--", NULL,
    NULL, &cfg_look_prefix[GUI_CHAT_PREFIX_QUIT], weechat_config_change_prefix },
  { "look_prefix_align",
    N_("prefix alignment (none, left, right))"),
    OPTION_TYPE_INT_WITH_STRING, 0, 0, 0, "right", cfg_look_prefix_align_values,
    &cfg_look_prefix_align, NULL, weechat_config_change_buffers },
  { "look_prefix_align_max",
    N_("max size for prefix (0 = no max size)"),
    OPTION_TYPE_INT, 0, 64, 0, NULL, NULL,
    &cfg_look_prefix_align_max, NULL, weechat_config_change_buffers },
  { "look_prefix_suffix",
    N_("string displayed after prefix"),
    OPTION_TYPE_STRING, 0, 0, 0, "|", NULL,
    NULL, &cfg_look_prefix_suffix, weechat_config_change_buffers },
  { "look_align_text_offset",
    N_("offset for aligning lines of messages (except first lines), default "
       "is -1 (align after prefix), a null or positive value is offset after "
       "beginning of line"),
    OPTION_TYPE_INT, -1, 64, -1, NULL, NULL,
    &cfg_look_align_text_offset, NULL, weechat_config_change_buffers },
  { "look_nick_completor",
    N_("the string inserted after nick completion"),
    OPTION_TYPE_STRING, 0, 0, 0, ":", NULL,
    NULL, &cfg_look_nick_completor, weechat_config_change_noop },
  { "look_nick_completion_ignore",
    N_("chars ignored for nick completion"),
    OPTION_TYPE_STRING, 0, 0, 0, "[]-^", NULL,
    NULL, &cfg_look_nick_completion_ignore, weechat_config_change_noop },
  { "look_nick_completion_smart",
    N_("smart completion for nicks (completes with last speakers first)"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_nick_completion_smart, NULL, weechat_config_change_noop },
  { "look_nick_complete_first",
    N_("complete only with first nick found"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &cfg_look_nick_complete_first, NULL, weechat_config_change_noop },
  { "look_infobar",
    N_("enable info bar"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_infobar, NULL, weechat_config_change_buffers },
  { "look_infobar_time_format",
    N_("time format for time in infobar"),
    OPTION_TYPE_STRING, 0, 0, 0, "%B, %A %d %Y", NULL,
    NULL, &cfg_look_infobar_time_format, weechat_config_change_buffer_content },
  { "look_infobar_seconds",
    N_("display seconds in infobar time"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_infobar_seconds, NULL, weechat_config_change_buffer_content },
  { "look_infobar_delay_highlight",
    N_("delay (in seconds) for highlight messages in infobar "
       "(0 = disable highlight notifications in infobar)"),
    OPTION_TYPE_INT, 0, INT_MAX, 7, NULL, NULL,
    &cfg_look_infobar_delay_highlight, NULL, weechat_config_change_noop },
  { "look_hotlist_names_count",
    N_("max number of names in hotlist (0 = no name displayed, only buffer numbers)"),
    OPTION_TYPE_INT, 0, 32, 3, NULL, NULL,
    &cfg_look_hotlist_names_count, NULL, weechat_config_change_buffer_content },
  { "look_hotlist_names_level",
    N_("level for displaying names in hotlist (combination of: 1=join/part, "
       "2=message, 4=private, 8=highlight, for example: 12=private+highlight)"),
    OPTION_TYPE_INT, 1, 15, 12, NULL, NULL,
    &cfg_look_hotlist_names_level, NULL, weechat_config_change_buffer_content },
  { "look_hotlist_names_length",
    N_("max length of names in hotlist (0 = no limit)"),
    OPTION_TYPE_INT, 0, 32, 0, NULL, NULL,
    &cfg_look_hotlist_names_length, NULL, weechat_config_change_buffer_content },
  { "look_hotlist_sort",
    N_("hotlist sort type (group_time_asc (default), group_time_desc, "
       "group_number_asc, group_number_desc, number_asc, number_desc)"),
    OPTION_TYPE_INT_WITH_STRING, 0, 0, 0, "group_time_asc", cfg_look_hotlist_sort_values,
    &cfg_look_hotlist_sort, NULL, weechat_config_change_hotlist },
  { "look_day_change",
    N_("display special message when day changes"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_TRUE, NULL, NULL,
    &cfg_look_day_change, NULL, weechat_config_change_noop },
  { "look_day_change_time_format",
    N_("time format for date displayed when day changed"),
    OPTION_TYPE_STRING, 0, 0, 0, "%a, %d %b %Y", NULL,
    NULL, &cfg_look_day_change_time_format, weechat_config_change_noop },
  { "look_read_marker",
    N_("use a marker on servers/channels to show first unread line"),
    OPTION_TYPE_STRING, 0, 1, 0, " ", NULL,
    NULL, &cfg_look_read_marker, weechat_config_change_read_marker },
  { "look_input_format",
    N_("format for input prompt ('%c' is replaced by channel or server, "
       "'%n' by nick and '%m' by nick modes)"),
    OPTION_TYPE_STRING, 0, 0, 0, "[%n(%m)] ", NULL,
    NULL, &cfg_look_input_format, weechat_config_change_buffer_content },
  { "look_paste_max_lines",
    N_("max number of lines for paste without asking user (0 = disable this feature)"),
    OPTION_TYPE_INT, 0, INT_MAX, 3, NULL, NULL,
    &cfg_look_paste_max_lines, NULL, weechat_config_change_noop },
  { NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, colors section */

int cfg_col_separator;
int cfg_col_title;
int cfg_col_title_bg;
int cfg_col_title_more;
int cfg_col_chat;
int cfg_col_chat_bg;
int cfg_col_chat_time;
int cfg_col_chat_time_delimiters;
int cfg_col_chat_prefix[GUI_CHAT_PREFIX_NUMBER];
int cfg_col_chat_prefix_more;
int cfg_col_chat_prefix_suffix;
int cfg_col_chat_buffer;
int cfg_col_chat_server;
int cfg_col_chat_channel;
int cfg_col_chat_nick;
int cfg_col_chat_nick_self;
int cfg_col_chat_nick_other;
int cfg_col_chat_nick_colors[GUI_COLOR_NICK_NUMBER];
int cfg_col_chat_host;
int cfg_col_chat_delimiters;
int cfg_col_chat_highlight;
int cfg_col_chat_read_marker;
int cfg_col_chat_read_marker_bg;
int cfg_col_status;
int cfg_col_status_bg;
int cfg_col_status_delimiters;
int cfg_col_status_channel;
int cfg_col_status_data_msg;
int cfg_col_status_data_private;
int cfg_col_status_data_highlight;
int cfg_col_status_data_other;
int cfg_col_status_more;
int cfg_col_infobar;
int cfg_col_infobar_bg;
int cfg_col_infobar_delimiters;
int cfg_col_infobar_highlight;
int cfg_col_infobar_bg;
int cfg_col_input;
int cfg_col_input_bg;
int cfg_col_input_server;
int cfg_col_input_channel;
int cfg_col_input_nick;
int cfg_col_input_delimiters;
int cfg_col_input_text_not_found;
int cfg_col_input_actions;
int cfg_col_nicklist;
int cfg_col_nicklist_bg;
int cfg_col_nicklist_away;
int cfg_col_nicklist_prefix1;
int cfg_col_nicklist_prefix2;
int cfg_col_nicklist_prefix3;
int cfg_col_nicklist_prefix4;
int cfg_col_nicklist_prefix5;
int cfg_col_nicklist_more;
int cfg_col_nicklist_separator;
int cfg_col_info;
int cfg_col_info_bg;
int cfg_col_info_waiting;
int cfg_col_info_connecting;
int cfg_col_info_active;
int cfg_col_info_done;
int cfg_col_info_failed;
int cfg_col_info_aborted;

struct t_config_option weechat_options_colors[] =
{ /* general color settings */
  { "col_separator",
    N_("color for window separators (when splited)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_SEPARATOR, "blue", NULL,
    &cfg_col_separator, NULL, weechat_config_change_color },
  
  /* title window */
  { "col_title",
    N_("color for title bar"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_TITLE, "default", NULL,
    &cfg_col_title, NULL, weechat_config_change_color },
  { "col_title_bg",
    N_("background color for title bar"),
    OPTION_TYPE_COLOR, 0, 0, -1, "blue", NULL,
    &cfg_col_title_bg, NULL, weechat_config_change_color },
  { "col_title_more",
    N_("color for '+' when scrolling title"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_TITLE_MORE, "lightmagenta", NULL,
    &cfg_col_title_more, NULL, weechat_config_change_color },
  
  /* chat window */
  { "col_chat",
    N_("color for chat text"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT, "default", NULL,
    &cfg_col_chat, NULL, weechat_config_change_color },
  { "col_chat_bg",
    N_("background for chat text"),
    OPTION_TYPE_COLOR, 0, 0, -1, "default", NULL,
    &cfg_col_chat_bg, NULL, weechat_config_change_color },
  { "col_chat_time",
    N_("color for time in chat window"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_TIME, "default", NULL,
    &cfg_col_chat_time, NULL, weechat_config_change_color },
  { "col_chat_time_delimiters",
    N_("color for time delimiters)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_TIME_DELIMITERS, "brown", NULL,
    &cfg_col_chat_time_delimiters, NULL, weechat_config_change_color },
  { "col_chat_prefix_info",
    N_("color for info prefix"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_PREFIX_INFO, "lightcyan", NULL,
    &cfg_col_chat_prefix[GUI_CHAT_PREFIX_INFO], NULL, weechat_config_change_color },
  { "col_chat_prefix_error",
    N_("color for error prefix"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_PREFIX_ERROR, "yellow", NULL,
    &cfg_col_chat_prefix[GUI_CHAT_PREFIX_ERROR], NULL, weechat_config_change_color },
  { "col_chat_prefix_network",
    N_("color for network prefix"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_PREFIX_NETWORK, "lightmagenta", NULL,
    &cfg_col_chat_prefix[GUI_CHAT_PREFIX_NETWORK], NULL, weechat_config_change_color },
  { "col_chat_prefix_action",
    N_("color for action prefix"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_PREFIX_ACTION, "white", NULL,
    &cfg_col_chat_prefix[GUI_CHAT_PREFIX_ACTION], NULL, weechat_config_change_color },
  { "col_chat_prefix_join",
    N_("color for join prefix"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_PREFIX_JOIN, "lightgreen", NULL,
    &cfg_col_chat_prefix[GUI_CHAT_PREFIX_JOIN], NULL, weechat_config_change_color },
  { "col_chat_prefix_quit",
    N_("color for quit prefix"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_PREFIX_QUIT, "lightred", NULL,
    &cfg_col_chat_prefix[GUI_CHAT_PREFIX_QUIT], NULL, weechat_config_change_color },
  { "col_chat_prefix_more",
    N_("color for '+' when prefix is too long"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_PREFIX_MORE, "lightmagenta", NULL,
    &cfg_col_chat_prefix_more, NULL, weechat_config_change_color },
  { "col_chat_prefix_suffix",
    N_("color for text after prefix"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_PREFIX_SUFFIX, "green", NULL,
    &cfg_col_chat_prefix_suffix, NULL, weechat_config_change_color },
  { "col_chat_buffer",
    N_("color for buffer names"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_BUFFER, "white", NULL,
    &cfg_col_chat_buffer, NULL, weechat_config_change_color },
  { "col_chat_server",
    N_("color for server names"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_SERVER, "brown", NULL,
    &cfg_col_chat_server, NULL, weechat_config_change_color },
  { "col_chat_channel",
    N_("color for channel names"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_CHANNEL, "white", NULL,
    &cfg_col_chat_channel, NULL, weechat_config_change_color },
  { "col_chat_nick",
    N_("color for nicks"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK, "lightcyan", NULL,
    &cfg_col_chat_nick, NULL, weechat_config_change_color },
  { "col_chat_nick_self",
    N_("color for local nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK_SELF, "white", NULL,
    &cfg_col_chat_nick_self, NULL, weechat_config_change_color },
  { "col_chat_nick_other",
    N_("color for other nick in private buffer"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK_OTHER, "default", NULL,
    &cfg_col_chat_nick_other, NULL, weechat_config_change_color },
  { "col_chat_nick_color1",
    N_("color #1 for nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK1, "cyan", NULL,
    &cfg_col_chat_nick_colors[0], NULL, weechat_config_change_color },
  { "col_chat_nick_color2",
    N_("color #2 for nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK2, "magenta", NULL,
    &cfg_col_chat_nick_colors[1], NULL, weechat_config_change_color },
  { "col_chat_nick_color3",
    N_("color #3 for nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK3, "green", NULL,
    &cfg_col_chat_nick_colors[2], NULL, weechat_config_change_color },
  { "col_chat_nick_color4",
    N_("color #4 for nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK4, "brown", NULL,
    &cfg_col_chat_nick_colors[3], NULL, weechat_config_change_color },
  { "col_chat_nick_color5",
    N_("color #5 for nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK5, "lightblue", NULL,
    &cfg_col_chat_nick_colors[4], NULL, weechat_config_change_color },
  { "col_chat_nick_color6",
    N_("color #6 for nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK6, "default", NULL,
    &cfg_col_chat_nick_colors[5], NULL, weechat_config_change_color },
  { "col_chat_nick_color7",
    N_("color #7 for nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK7, "lightcyan", NULL,
    &cfg_col_chat_nick_colors[6], NULL, weechat_config_change_color },
  { "col_chat_nick_color8",
    N_("color #8 for nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK8, "lightmagenta", NULL,
    &cfg_col_chat_nick_colors[7], NULL, weechat_config_change_color },
  { "col_chat_nick_color9",
    N_("color #9 for nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK9, "lightgreen", NULL,
    &cfg_col_chat_nick_colors[8], NULL, weechat_config_change_color },
  { "col_chat_nick_color10",
    N_("color #10 for nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_NICK10, "blue", NULL,
    &cfg_col_chat_nick_colors[9], NULL, weechat_config_change_color },
  { "col_chat_host",
    N_("color for hostnames"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_HOST, "cyan", NULL,
    &cfg_col_chat_host, NULL, weechat_config_change_color },
  { "col_chat_delimiters",
    N_("color for delimiters"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_DELIMITERS, "green", NULL,
    &cfg_col_chat_delimiters, NULL, weechat_config_change_color },
  { "col_chat_highlight",
    N_("color for highlighted nick"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_HIGHLIGHT, "yellow", NULL,
    &cfg_col_chat_highlight, NULL, weechat_config_change_color },
  { "col_chat_read_marker",
    N_("color for unread data marker"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_CHAT_READ_MARKER, "yellow", NULL,
    &cfg_col_chat_read_marker, NULL, weechat_config_change_color },
  { "col_chat_read_marker_bg",
    N_("background color for unread data marker"),
    OPTION_TYPE_COLOR, 0, 0, -1, "magenta", NULL,
    &cfg_col_chat_read_marker_bg, NULL, weechat_config_change_color },
  
  /* status window */
  { "col_status",
    N_("color for status bar"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_STATUS, "default", NULL,
    &cfg_col_status, NULL, weechat_config_change_color },
  { "col_status_bg",
    N_("background color for status bar"),
    OPTION_TYPE_COLOR, 0, 0, -1, "blue", NULL,
    &cfg_col_status_bg, NULL, weechat_config_change_color },
  { "col_status_delimiters",
    N_("color for status bar delimiters"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_STATUS_DELIMITERS, "cyan", NULL,
    &cfg_col_status_delimiters, NULL, weechat_config_change_color },
  { "col_status_channel",
    N_("color for current channel in status bar"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_STATUS_CHANNEL, "white", NULL,
    &cfg_col_status_channel, NULL, weechat_config_change_color },
  { "col_status_data_msg",
    N_("color for window with new messages (status bar)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_STATUS_DATA_MSG, "yellow", NULL,
    &cfg_col_status_data_msg, NULL, weechat_config_change_color },
  { "col_status_data_private",
    N_("color for window with private message (status bar)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_STATUS_DATA_PRIVATE, "lightmagenta", NULL,
    &cfg_col_status_data_private, NULL, weechat_config_change_color },
  { "col_status_data_highlight",
    N_("color for window with highlight (status bar)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_STATUS_DATA_HIGHLIGHT, "lightred", NULL,
    &cfg_col_status_data_highlight, NULL, weechat_config_change_color },
  { "col_status_data_other",
    N_("color for window with new data (not messages) (status bar)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_STATUS_DATA_OTHER, "default", NULL,
    &cfg_col_status_data_other, NULL, weechat_config_change_color },
  { "col_status_more",
    N_("color for window with new data (status bar)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_STATUS_MORE, "white", NULL,
    &cfg_col_status_more, NULL, weechat_config_change_color },
  
  /* infobar window */
  { "col_infobar",
    N_("color for info bar text"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INFOBAR, "black", NULL,
    &cfg_col_infobar, NULL, weechat_config_change_color },
  { "col_infobar_bg",
    N_("background color for info bar text"),
    OPTION_TYPE_COLOR, 0, 0, -1, "cyan", NULL,
    &cfg_col_infobar_bg, NULL, weechat_config_change_color },
  { "col_infobar_delimiters",
    N_("color for infobar delimiters"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INFOBAR_DELIMITERS, "blue", NULL,
    &cfg_col_infobar_delimiters, NULL, weechat_config_change_color },
  { "col_infobar_highlight",
    N_("color for info bar highlight notification"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INFOBAR_HIGHLIGHT, "white", NULL,
    &cfg_col_infobar_highlight, NULL, weechat_config_change_color },
  
  /* input window */
  { "col_input",
    N_("color for input text"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INPUT, "default", NULL,
    &cfg_col_input, NULL, weechat_config_change_color },
  { "col_input_bg",
    N_("background color for input text"),
    OPTION_TYPE_COLOR, 0, 0, -1, "default", NULL,
    &cfg_col_input_bg, NULL, weechat_config_change_color },
  { "col_input_server",
    N_("color for input text (server name)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INPUT_SERVER, "brown", NULL,
    &cfg_col_input_server, NULL, weechat_config_change_color },
  { "col_input_channel",
    N_("color for input text (channel name)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INPUT_CHANNEL, "white", NULL,
    &cfg_col_input_channel, NULL, weechat_config_change_color },
  { "col_input_nick",
    N_("color for input text (nick name)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INPUT_NICK, "lightcyan", NULL,
    &cfg_col_input_nick, NULL, weechat_config_change_color },
  { "col_input_delimiters",
    N_("color for input text (delimiters)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INPUT_DELIMITERS, "cyan", NULL,
    &cfg_col_input_delimiters, NULL, weechat_config_change_color },
  { "col_input_text_not_found",
    N_("color for text not found"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INPUT_TEXT_NOT_FOUND, "red", NULL,
    &cfg_col_input_text_not_found, NULL, weechat_config_change_color },
  { "col_input_actions",
    N_("color for actions in input window"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INPUT_ACTIONS, "lightgreen", NULL,
    &cfg_col_input_actions, NULL, weechat_config_change_color },
  
  /* nicklist window */
  { "col_nicklist",
    N_("color for nicklist"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_NICKLIST, "default", NULL,
    &cfg_col_nicklist, NULL, weechat_config_change_color },
  { "col_nicklist_bg",
    N_("background color for nicklist"),
    OPTION_TYPE_COLOR, 0, 0, -1, "default", NULL,
    &cfg_col_nicklist_bg, NULL, weechat_config_change_color },
  { "col_nicklist_away",
    N_("color for away nicknames"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_NICKLIST_AWAY, "cyan", NULL,
    &cfg_col_nicklist_away, NULL, weechat_config_change_color },
  { "col_nicklist_prefix1",
    N_("color for prefix 1"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_NICKLIST_PREFIX1, "lightgreen", NULL,
    &cfg_col_nicklist_prefix1, NULL, weechat_config_change_color },
  { "col_nicklist_prefix2",
    N_("color for prefix 2"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_NICKLIST_PREFIX2, "lightmagenta", NULL,
    &cfg_col_nicklist_prefix2, NULL, weechat_config_change_color },
  { "col_nicklist_prefix3",
    N_("color for prefix 3"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_NICKLIST_PREFIX3, "yellow", NULL,
    &cfg_col_nicklist_prefix3, NULL, weechat_config_change_color },
  { "col_nicklist_prefix4",
    N_("color for prefix 4"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_NICKLIST_PREFIX4, "blue", NULL,
    &cfg_col_nicklist_prefix4, NULL, weechat_config_change_color },
  { "col_nicklist_prefix5",
    N_("color for prefix 5"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_NICKLIST_PREFIX5, "brown", NULL,
    &cfg_col_nicklist_prefix5, NULL, weechat_config_change_color },
  { "col_nicklist_more",
    N_("color for '+' when scrolling nicks (nicklist)"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_NICKLIST_MORE, "lightmagenta", NULL,
    &cfg_col_nicklist_more, NULL, weechat_config_change_color },
  { "col_nicklist_separator",
    N_("color for nicklist separator"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_NICKLIST_SEPARATOR, "blue", NULL,
    &cfg_col_nicklist_separator, NULL, weechat_config_change_color },

  /* status info (for example: file transfers) */
  { "col_info",
    N_("color for status info"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INFO, "default", NULL,
    &cfg_col_info, NULL, weechat_config_change_color },
  { "col_info_bg",
    N_("background color for status info"),
    OPTION_TYPE_COLOR, 0, 0, -1, "default", NULL,
    &cfg_col_info_bg, NULL, weechat_config_change_color },
  { "col_info_waiting",
    N_("color for \"waiting\" status info"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INFO_WAITING, "lightcyan", NULL,
    &cfg_col_info_waiting, NULL, weechat_config_change_color },
  { "col_info_connecting",
    N_("color for \"connecting\" status info"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INFO_CONNECTING, "yellow", NULL,
    &cfg_col_info_connecting, NULL, weechat_config_change_color },
  { "col_info_active",
    N_("color for \"active\" status info"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INFO_ACTIVE, "lightblue", NULL,
    &cfg_col_info_active, NULL, weechat_config_change_color },
  { "col_info_done",
    N_("color for \"done\" status info"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INFO_DONE, "lightgreen", NULL,
    &cfg_col_info_done, NULL, weechat_config_change_color },
  { "col_info_failed",
    N_("color for \"failed\" status info"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INFO_FAILED, "lightred", NULL,
    &cfg_col_info_failed, NULL, weechat_config_change_color },
  { "col_info_aborted",
    N_("color for \"aborted\" status info"),
    OPTION_TYPE_COLOR, 0, 0, GUI_COLOR_INFO_ABORTED, "lightred", NULL,
    &cfg_col_info_aborted, NULL, weechat_config_change_color },
  
  { NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, history section */

int cfg_history_max_lines;
int cfg_history_max_commands;
int cfg_history_display_default;

struct t_config_option weechat_options_history[] =
{ { "history_max_lines",
    N_("maximum number of lines in history "
    "for one server/channel/private window (0 = unlimited)"),
    OPTION_TYPE_INT, 0, INT_MAX, 4096, NULL, NULL,
    &cfg_history_max_lines, NULL, weechat_config_change_noop },
  { "history_max_commands",
    N_("maximum number of user commands in history (0 = unlimited)"),
    OPTION_TYPE_INT, 0, INT_MAX, 100, NULL, NULL,
    &cfg_history_max_commands, NULL, weechat_config_change_noop },
  { "history_display_default",
    N_("maximum number of commands to display by default in history listing "
       "(0 = unlimited)"),
    OPTION_TYPE_INT, 0, INT_MAX, 5, NULL, NULL,
    &cfg_history_display_default, NULL, weechat_config_change_noop },
  { NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, log section */

int cfg_log_plugin_msg;
char *cfg_log_path;
char *cfg_log_time_format;

struct t_config_option weechat_options_log[] =
{ { "log_plugin_msg",
    N_("log messages from plugins"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &cfg_log_plugin_msg, NULL, weechat_config_change_noop },
  { "log_path",
    N_("path for WeeChat log files ('%h' will be replaced by WeeChat home, "
       "~/.weechat by default)"),
    OPTION_TYPE_STRING, 0, 0, 0, "%h/logs/", NULL,
    NULL, &cfg_log_path, weechat_config_change_noop },
  { "log_time_format",
    N_("time format for log (see man strftime for date/time specifiers)"),
    OPTION_TYPE_STRING, 0, 0, 0, "%Y %b %d %H:%M:%S", NULL,
    NULL, &cfg_log_time_format, weechat_config_change_noop },
  { NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, proxy section */

int cfg_proxy_use;
int cfg_proxy_type;
int cfg_proxy_ipv6;
char *cfg_proxy_type_values[] =
{ "http", "socks4", "socks5", NULL };
char *cfg_proxy_address;
int cfg_proxy_port;
char *cfg_proxy_username;
char *cfg_proxy_password;

struct t_config_option weechat_options_proxy[] =
{ { "proxy_use",
    N_("use a proxy server to connect to irc server"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &cfg_proxy_use, NULL, weechat_config_change_noop },
  { "proxy_type",
    N_("proxy type (http (default), socks4, socks5)"),
    OPTION_TYPE_INT_WITH_STRING, 0, 0, 0, "http", cfg_proxy_type_values,
    &cfg_proxy_type, NULL, weechat_config_change_noop },
  { "proxy_ipv6",
    N_("connect to proxy in ipv6"),
    OPTION_TYPE_BOOLEAN, BOOL_FALSE, BOOL_TRUE, BOOL_FALSE, NULL, NULL,
    &cfg_proxy_ipv6, NULL, weechat_config_change_noop },
  { "proxy_address",
    N_("proxy server address (IP or hostname)"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &cfg_proxy_address, weechat_config_change_noop },
  { "proxy_port",
    N_("port for connecting to proxy server"),
    OPTION_TYPE_INT, 0, 65535, 3128, NULL, NULL,
    &cfg_proxy_port, NULL, weechat_config_change_noop },
  { "proxy_username",
    N_("username for proxy server"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &cfg_proxy_username, weechat_config_change_noop },
  { "proxy_password",
    N_("password for proxy server"),
    OPTION_TYPE_STRING, 0, 0, 0, "", NULL,
    NULL, &cfg_proxy_password, weechat_config_change_noop },
  { NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* config, plugins section */

char *cfg_plugins_path;
char *cfg_plugins_autoload;
char *cfg_plugins_extension;

struct t_config_option weechat_options_plugins[] =
{ { "plugins_path",
    N_("path for searching plugins ('%h' will be replaced by WeeChat home, "
       "~/.weechat by default)"),
    OPTION_TYPE_STRING, 0, 0, 0, "%h/plugins", NULL,
    NULL, &cfg_plugins_path, weechat_config_change_noop },
  { "plugins_autoload",
    N_("comma separated list of plugins to load automatically at startup, "
       "\"*\" means all plugins found "
       "(names may be partial, for example \"perl\" is ok for \"perl.so\")"),
    OPTION_TYPE_STRING, 0, 0, 0, "*", NULL,
    NULL, &cfg_plugins_autoload, weechat_config_change_noop },
  { "plugins_extension",
    N_("standard plugins extension in filename, used for autoload "
       "(if empty, then all files are loaded when autoload is \"*\")"),
    OPTION_TYPE_STRING, 0, 0, 0,
#ifdef WIN32
    ".dll",
#else
    ".so",
#endif
    NULL,
    NULL, &cfg_plugins_extension, weechat_config_change_noop },
  { NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

char *weechat_config_sections[] =
{ "look", "colors",
  "history", "log",
  "proxy", "plugins",
  "alias", "keys",
  NULL
};

struct t_config_option *weechat_config_options[] =
{ weechat_options_look, weechat_options_colors,
  weechat_options_history, weechat_options_log,
  weechat_options_proxy, weechat_options_plugins,
  NULL, NULL,
  NULL
};

t_config_func_read_option *weechat_config_read_functions[] =
{ config_file_read_option, config_file_read_option,
  config_file_read_option, config_file_read_option,
  config_file_read_option, config_file_read_option,
  weechat_config_read_alias, weechat_config_read_key,
  NULL
};

t_config_func_write_options *weechat_config_write_functions[] =
{ config_file_write_options, config_file_write_options,
  config_file_write_options, config_file_write_options,
  config_file_write_options, config_file_write_options,
  weechat_config_write_alias, weechat_config_write_keys,
  NULL
};

t_config_func_write_options *weechat_config_write_default_functions[] =
{ config_file_write_options_default_values, config_file_write_options_default_values,
  config_file_write_options_default_values, config_file_write_options_default_values,
  config_file_write_options_default_values, config_file_write_options_default_values,
  weechat_config_write_alias_default_values, weechat_config_write_keys_default_values,
  NULL
};


/*
 * weechat_config_change_noop: called when an option is changed by /set command
 *                             and that no special action is needed after that
 */

void
weechat_config_change_noop ()
{
    /* do nothing */
}

/*
 * weechat_config_change_save_on_exit: called when "save_on_exit" flag is changed
 */

void
weechat_config_change_save_on_exit ()
{
    if (!cfg_look_save_on_exit)
    {
        gui_chat_printf (NULL, "\n");
        gui_chat_printf (NULL,
                         _("Warning: you should now issue /save to write "
                           "\"save_on_exit\" option in configuration file.\n"));
    }
}

/*
 * weechat_config_change_title: called when title is changed
 */

void
weechat_config_change_title ()
{
    if (cfg_look_set_title)
	gui_window_title_set ();
    else
	gui_window_title_reset ();
}

/*
 * weechat_config_change_buffers: called when buffers change (for example nicklist)
 */

void
weechat_config_change_buffers ()
{
    gui_window_refresh_windows ();
}

/*
 * weechat_config_change_buffer_content: called when content of a buffer changes
 */

void
weechat_config_change_buffer_content ()
{
    gui_window_redraw_buffer (gui_current_window->buffer);
}

/*
 * weechat_config_change_buffer_time_format: called when buffer time format changes
 */

void
weechat_config_change_buffer_time_format ()
{
    gui_chat_time_length = util_get_time_length (cfg_look_buffer_time_format);
    gui_chat_change_time_format ();
    gui_window_redraw_buffer (gui_current_window->buffer);
}

/*
 * weechat_config_change_hotlist: called when hotlist changes
 */

void
weechat_config_change_hotlist ()
{
    gui_hotlist_resort ();
    gui_status_draw (gui_current_window->buffer, 1);
}

/*
 * weechat_config_change_read_marker: called when read marker is changed
 */

void
weechat_config_change_read_marker ()
{
    gui_window_redraw_all_buffers ();
}

/*
 * weechat_config_change_prefix: called when a prefix is changed
 */

void
weechat_config_change_prefix ()
{
    int i;
    
    for (i = 0; i < GUI_CHAT_PREFIX_NUMBER; i++)
    {
        if (gui_chat_prefix[i])
            free (gui_chat_prefix[i]);
    }
    gui_chat_prefix_build ();
}

/*
 * weechat_config_change_color: called when a color is changed by /set command
 */

void
weechat_config_change_color ()
{
    gui_color_init_pairs ();
    gui_color_rebuild_weechat ();
    gui_window_refresh_windows ();
}

/*
 * weechat_config_change_nicks_colors: called when number of nicks color changed
 */

void
weechat_config_change_nicks_colors ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_nick *ptr_nick;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->nicks)
        {
            for (ptr_nick = ptr_buffer->nicks; ptr_nick;
                 ptr_nick = ptr_nick->next_nick)
            {
                //gui_nick_find_color (ptr_nick);
            }   
        }
    }
}

/*
 * weechat_config_read_alias: read an alias in configuration file
 *                            Return:  0 = successful
 *                                    -1 = option not found
 *                                    -2 = bad format/value
 */

int
weechat_config_read_alias (struct t_config_option *options,
                           char *option_name, char *value)
{
    /* make C compiler happy */
    (void) options;
    
    if (option_name)
    {
        /* create new alias */
        if (alias_new (option_name, value))
            weelist_add (&weechat_index_commands,
                         &weechat_last_index_command,
                         option_name,
                         WEELIST_POS_SORT);
        else
            return -2;
    }
    else
    {
        /* does nothing for new [alias] section */
    }
    
    /* all ok */
    return 0;
}

/*
 * weechat_config_read_key: read a key in configuration file
 *                          Return:  0 = successful
 *                                  -1 = option not found
 *                                  -2 = bad format/value
 */

int
weechat_config_read_key (struct t_config_option *options,
                         char *option_name, char *value)
{
    /* make C compiler happy */
    (void) options;
    
    if (option_name)
    {
        if (value[0])
        {
            /* bind key (overwrite any binding with same key) */
            gui_keyboard_bind (option_name, value);
        }
        else
        {
            /* unbin key if no value given */
            gui_keyboard_unbind (option_name);
        }
    }
    else
    {
        /* does nothing for new [key] section */
    }
    
    /* all ok */
    return 0;
}

/*
 * weechat_config_read: read WeeChat configuration file
 *                      return:  0 = successful
 *                              -1 = configuration file file not found
 *                              -2 = error in configuration file
 */

int
weechat_config_read ()
{
    return config_file_read (weechat_config_sections, weechat_config_options,
                             weechat_config_read_functions,
                             config_file_read_option,
                             weechat_config_write_default_functions,
                             WEECHAT_CONFIG_NAME);
}

/*
 * weechat_config_write_alias: write alias section in configuration file
 *                             Return:  0 = successful
 *                                     -1 = write error
 */

int
weechat_config_write_alias (FILE *file, char *section_name,
                            struct t_config_option *options)
{
    struct alias *ptr_alias;
    
    /* make C compiler happy */
    (void) options;
    
    string_iconv_fprintf (file, "\n[%s]\n", section_name);
    
    for (ptr_alias = weechat_alias; ptr_alias;
         ptr_alias = ptr_alias->next_alias)
    {
        string_iconv_fprintf (file, "%s = \"%s\"\n",
                              ptr_alias->name, ptr_alias->command);
    }
    
    /* all ok */
    return 0;
}

/*
 * weechat_config_write_keys: write alias section in configuration file
 *                            Return:  0 = successful
 *                                    -1 = write error
 */

int
weechat_config_write_keys (FILE *file, char *section_name,
                           struct t_config_option *options)
{
    t_gui_key *ptr_key;
    char *expanded_name, *function_name;
    
    /* make C compiler happy */
    (void) options;
    
    string_iconv_fprintf (file, "\n[%s]\n", section_name);
    
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_keyboard_get_expanded_name (ptr_key->key);
        if (ptr_key->function)
        {
            function_name = gui_keyboard_function_search_by_ptr (ptr_key->function);
            if (function_name)
                string_iconv_fprintf (file, "%s = \"%s%s%s\"\n",
                                      (expanded_name) ? expanded_name : ptr_key->key,
                                      function_name,
                                      (ptr_key->args) ? " " : "",
                                      (ptr_key->args) ? ptr_key->args : "");
        }
        else
            string_iconv_fprintf (file, "%s = \"%s\"\n",
                                  (expanded_name) ? expanded_name : ptr_key->key,
                                  ptr_key->command);
        if (expanded_name)
            free (expanded_name);
    }
    
    /* all ok */
    return 0;
}

/*
 * weechat_config_write_alias_default_values: write alias section with default values
 *                                            in configuration file
 *                                            Return:  0 = successful
 *                                                    -1 = write error
 */

int
weechat_config_write_alias_default_values (FILE *file, char *section_name,
                                           struct t_config_option *options)
{
    /* make C compiler happy */
    (void) options;
    
    string_iconv_fprintf (file, "\n[%s]\n", section_name);
    
    string_iconv_fprintf (file, "SAY = \"msg *\"\n");
    string_iconv_fprintf (file, "BYE = \"quit\"\n");
    string_iconv_fprintf (file, "EXIT = \"quit\"\n");
    string_iconv_fprintf (file, "SIGNOFF = \"quit\"\n");
    string_iconv_fprintf (file, "C = \"clear\"\n");
    string_iconv_fprintf (file, "CL = \"clear\"\n");
    string_iconv_fprintf (file, "CLOSE = \"buffer close\"\n");
    string_iconv_fprintf (file, "CHAT = \"dcc chat\"\n");
    string_iconv_fprintf (file, "IG = \"ignore\"\n");
    string_iconv_fprintf (file, "J = \"join\"\n");
    string_iconv_fprintf (file, "K = \"kick\"\n");
    string_iconv_fprintf (file, "KB = \"kickban\"\n");
    string_iconv_fprintf (file, "LEAVE = \"part\"\n");
    string_iconv_fprintf (file, "M = \"msg\"\n");
    string_iconv_fprintf (file, "MUB = \"unban *\"\n");
    string_iconv_fprintf (file, "N = \"names\"\n");
    string_iconv_fprintf (file, "Q = \"query\"\n");
    string_iconv_fprintf (file, "T = \"topic\"\n");
    string_iconv_fprintf (file, "UB = \"unban\"\n");
    string_iconv_fprintf (file, "UNIG = \"unignore\"\n");
    string_iconv_fprintf (file, "W = \"who\"\n");
    string_iconv_fprintf (file, "WC = \"window merge\"\n");
    string_iconv_fprintf (file, "WI = \"whois\"\n");
    string_iconv_fprintf (file, "WW = \"whowas\"\n");
    
    /* all ok */
    return 0;
}

/*
 * weechat_config_write_keys_default_values: write keys section with default values
 *                                           in configuration file 
 *                                           Return:  0 = successful
 *                                                   -1 = write error
 */

int
weechat_config_write_keys_default_values (FILE *file, char *section_name,
                                          struct t_config_option *options)
{
    t_gui_key *ptr_key;
    char *expanded_name, *function_name;
    
    /* make C compiler happy */
    (void) options;
    
    string_iconv_fprintf (file, "\n[%s]\n", section_name);
    
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_keyboard_get_expanded_name (ptr_key->key);
        if (ptr_key->function)
        {
            function_name = gui_keyboard_function_search_by_ptr (ptr_key->function);
            if (function_name)
                string_iconv_fprintf (file, "%s = \"%s%s%s\"\n",
                                      (expanded_name) ? expanded_name : ptr_key->key,
                                      function_name,
                                      (ptr_key->args) ? " " : "",
                                      (ptr_key->args) ? ptr_key->args : "");
        }
        else
            string_iconv_fprintf (file, "%s = \"%s\"\n",
                                  (expanded_name) ? expanded_name : ptr_key->key,
                                  ptr_key->command);
        if (expanded_name)
            free (expanded_name);
    }
    
    /* all ok */
    return 0;
}

/*
 * weechat_config_write: write WeeChat configuration file
 *                       return:  0 if ok
 *                              < 0 if error
 */

int
weechat_config_write ()
{
    weechat_log_printf (_("Saving WeeChat configuration to disk\n"));
    return config_file_write (weechat_config_sections, weechat_config_options,
                              weechat_config_write_functions,
                              WEECHAT_CONFIG_NAME);
}

/*
 * weechat_config_print_stdout: print WeeChat options on standard output
 */

void
weechat_config_print_stdout ()
{
    config_option_print_stdout (weechat_config_sections,
                                weechat_config_options);
}
