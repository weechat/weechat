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
#include "wee-config-file.h"
#include "wee-command.h"
#include "wee-log.h"
#include "wee-util.h"
#include "wee-list.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-status.h"
#include "../gui/gui-window.h"


struct t_config_file *weechat_config = NULL;

/* config, look & feel section */

struct t_config_option *config_look_color_real_white;
struct t_config_option *config_look_save_on_exit;
struct t_config_option *config_look_set_title;
struct t_config_option *config_look_startup_logo;
struct t_config_option *config_look_startup_version;
struct t_config_option *config_look_weechat_slogan;
struct t_config_option *config_look_scroll_amount;
struct t_config_option *config_look_buffer_time_format;
struct t_config_option *config_look_color_nicks_number;
struct t_config_option *config_look_nicklist;
struct t_config_option *config_look_nicklist_position;
struct t_config_option *config_look_nicklist_min_size;
struct t_config_option *config_look_nicklist_max_size;
struct t_config_option *config_look_nicklist_separator;
struct t_config_option *config_look_nickmode;
struct t_config_option *config_look_nickmode_empty;
struct t_config_option *config_look_no_nickname;
struct t_config_option *config_look_prefix[GUI_CHAT_PREFIX_NUMBER];
struct t_config_option *config_look_prefix_align;
struct t_config_option *config_look_prefix_align_max;
struct t_config_option *config_look_prefix_suffix;
struct t_config_option *config_look_nick_completor;
struct t_config_option *config_look_nick_completion_ignore;
struct t_config_option *config_look_nick_completion_smart;
struct t_config_option *config_look_nick_complete_first;
struct t_config_option *config_look_infobar;
struct t_config_option *config_look_infobar_time_format;
struct t_config_option *config_look_infobar_seconds;
struct t_config_option *config_look_infobar_delay_highlight;
struct t_config_option *config_look_hotlist_names_count;
struct t_config_option *config_look_hotlist_names_level;
struct t_config_option *config_look_hotlist_names_length;
struct t_config_option *config_look_hotlist_sort;
struct t_config_option *config_look_day_change;
struct t_config_option *config_look_day_change_time_format;
struct t_config_option *config_look_read_marker;
struct t_config_option *config_look_input_format;
struct t_config_option *config_look_paste_max_lines;

/* config, colors section */

struct t_config_option *config_color_separator;
struct t_config_option *config_color_title;
struct t_config_option *config_color_title_bg;
struct t_config_option *config_color_title_more;
struct t_config_option *config_color_chat;
struct t_config_option *config_color_chat_bg;
struct t_config_option *config_color_chat_time;
struct t_config_option *config_color_chat_time_delimiters;
struct t_config_option *config_color_chat_prefix[GUI_CHAT_PREFIX_NUMBER];
struct t_config_option *config_color_chat_prefix_more;
struct t_config_option *config_color_chat_prefix_suffix;
struct t_config_option *config_color_chat_buffer;
struct t_config_option *config_color_chat_server;
struct t_config_option *config_color_chat_channel;
struct t_config_option *config_color_chat_nick;
struct t_config_option *config_color_chat_nick_self;
struct t_config_option *config_color_chat_nick_other;
struct t_config_option *config_color_chat_nick_colors[GUI_COLOR_NICK_NUMBER];
struct t_config_option *config_color_chat_host;
struct t_config_option *config_color_chat_delimiters;
struct t_config_option *config_color_chat_highlight;
struct t_config_option *config_color_chat_read_marker;
struct t_config_option *config_color_chat_read_marker_bg;
struct t_config_option *config_color_status;
struct t_config_option *config_color_status_bg;
struct t_config_option *config_color_status_delimiters;
struct t_config_option *config_color_status_channel;
struct t_config_option *config_color_status_data_msg;
struct t_config_option *config_color_status_data_private;
struct t_config_option *config_color_status_data_highlight;
struct t_config_option *config_color_status_data_other;
struct t_config_option *config_color_status_more;
struct t_config_option *config_color_infobar;
struct t_config_option *config_color_infobar_bg;
struct t_config_option *config_color_infobar_delimiters;
struct t_config_option *config_color_infobar_highlight;
struct t_config_option *config_color_infobar_bg;
struct t_config_option *config_color_input;
struct t_config_option *config_color_input_bg;
struct t_config_option *config_color_input_server;
struct t_config_option *config_color_input_channel;
struct t_config_option *config_color_input_nick;
struct t_config_option *config_color_input_delimiters;
struct t_config_option *config_color_input_text_not_found;
struct t_config_option *config_color_input_actions;
struct t_config_option *config_color_nicklist;
struct t_config_option *config_color_nicklist_bg;
struct t_config_option *config_color_nicklist_away;
struct t_config_option *config_color_nicklist_prefix1;
struct t_config_option *config_color_nicklist_prefix2;
struct t_config_option *config_color_nicklist_prefix3;
struct t_config_option *config_color_nicklist_prefix4;
struct t_config_option *config_color_nicklist_prefix5;
struct t_config_option *config_color_nicklist_more;
struct t_config_option *config_color_nicklist_separator;
struct t_config_option *config_color_info;
struct t_config_option *config_color_info_bg;
struct t_config_option *config_color_info_waiting;
struct t_config_option *config_color_info_connecting;
struct t_config_option *config_color_info_active;
struct t_config_option *config_color_info_done;
struct t_config_option *config_color_info_failed;
struct t_config_option *config_color_info_aborted;

/* config, history section */

struct t_config_option *config_history_max_lines;
struct t_config_option *config_history_max_commands;
struct t_config_option *config_history_display_default;

/* config, proxy section */

struct t_config_option *config_proxy_use;
struct t_config_option *config_proxy_type;
struct t_config_option *config_proxy_ipv6;
struct t_config_option *config_proxy_address;
struct t_config_option *config_proxy_port;
struct t_config_option *config_proxy_username;
struct t_config_option *config_proxy_password;

/* config, plugins section */

struct t_config_option *config_plugins_path;
struct t_config_option *config_plugins_autoload;
struct t_config_option *config_plugins_extension;


/*
 * config_change_save_on_exit: called when "save_on_exit" flag is changed
 */

void
config_change_save_on_exit ()
{
    if (!config_look_save_on_exit)
    {
        gui_chat_printf (NULL, "\n");
        gui_chat_printf (NULL,
                         _("Warning: you should now issue /save to write "
                           "\"save_on_exit\" option in configuration file.\n"));
    }
}

/*
 * config_change_title: called when title is changed
 */

void
config_change_title ()
{
    if (config_look_set_title)
	gui_window_title_set ();
    else
	gui_window_title_reset ();
}

/*
 * config_change_buffers: called when buffers change (for example nicklist)
 */

void
config_change_buffers ()
{
    gui_window_refresh_windows ();
}

/*
 * config_change_buffer_content: called when content of a buffer changes
 */

void
config_change_buffer_content ()
{
    gui_window_redraw_buffer (gui_current_window->buffer);
}

/*
 * config_change_buffer_time_format: called when buffer time format changes
 */

void
config_change_buffer_time_format ()
{
    gui_chat_time_length = util_get_time_length (CONFIG_STRING(config_look_buffer_time_format));
    gui_chat_change_time_format ();
    gui_window_redraw_buffer (gui_current_window->buffer);
}

/*
 * config_change_hotlist: called when hotlist changes
 */

void
config_change_hotlist ()
{
    gui_hotlist_resort ();
    gui_status_draw (gui_current_window->buffer, 1);
}

/*
 * config_change_read_marker: called when read marker is changed
 */

void
config_change_read_marker ()
{
    gui_window_redraw_all_buffers ();
}

/*
 * config_change_prefix: called when a prefix is changed
 */

void
config_change_prefix ()
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
 * config_change_color: called when a color is changed by /set command
 */

void
config_change_color ()
{
    if (gui_ok)
    {
        gui_color_init_pairs ();
        gui_color_rebuild_weechat ();
        gui_window_refresh_windows ();
    }
}

/*
 * config_change_nicks_colors: called when number of nicks color changed
 */

void
config_change_nicks_colors ()
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
 * config_weechat_read_key: read a key in configuration file
 */

void
config_weechat_read_key (void *config_file,
                         char *option_name, char *value)
{
    /* make C compiler happy */
    (void) config_file;
    
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

/*
 * config_weechat_write_keys: write alias section in configuration file
 *                            Return:  0 = successful
 *                                    -1 = write error
 */

void
config_weechat_write_keys (void *config_file)
{
    t_gui_key *ptr_key;
    char *expanded_name, *function_name, *string;
    
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_keyboard_get_expanded_name (ptr_key->key);
        if (ptr_key->function)
        {
            function_name = gui_keyboard_function_search_by_ptr (ptr_key->function);
            if (function_name)
            {
                string = (char *)malloc (strlen (function_name) + 8 +
                                         ((ptr_key->args) ? strlen (ptr_key->args) : 0));
                if (string)
                {
                    strcpy (string, "\"");
                    strcat (string, function_name);
                    if (ptr_key->args)
                    {
                        strcat (string, " ");
                        strcat (string, ptr_key->args);
                    }
                    strcat (string, "\"");
                    config_file_write_line (config_file,
                                            (expanded_name) ?
                                            expanded_name : ptr_key->key,
                                            string);
                    free (string);
                }
            }
        }
        else
        {
            string = (char *)malloc (strlen (ptr_key->command) + 4);
            if (string)
            {
                strcpy (string, "\"");
                strcat (string, ptr_key->command);
                strcat (string, "\"");
                config_file_write_line (config_file,
                                        (expanded_name) ?
                                        expanded_name : ptr_key->key,
                                        string);
                free (string);
            }
        }
        if (expanded_name)
            free (expanded_name);
    }
}

/*
 * config_weechat_init: init WeeChat config structure
 */

void
config_weechat_init ()
{
    struct t_config_section *section;
    
    weechat_config = config_file_new (NULL, WEECHAT_CONFIG_FILENAME);
    if (weechat_config)
    {
        /* look */
        section = config_file_new_section (weechat_config, "look",
                                           NULL, NULL, NULL);
        if (section)
        {
            config_look_color_real_white = config_file_new_option_boolean (
                section, "look_color_real_white",
                N_("if set, uses real white color, disabled by default "
                   "for terms with white background (if you never use "
                   "white background, you should turn on this option to "
                   "see real white instead of default term foreground "
                   "color)"),
                CONFIG_BOOLEAN_FALSE, &config_change_color);
            config_look_save_on_exit = config_file_new_option_boolean (
                section, "look_save_on_exit",
                N_("save configuration file on exit"),
                CONFIG_BOOLEAN_TRUE, &config_change_save_on_exit);
            config_look_set_title = config_file_new_option_boolean (
                section, "look_set_title",
                N_("set title for window (terminal for Curses GUI) with "
                   "name and version"),
                CONFIG_BOOLEAN_TRUE, &config_change_title);
            config_look_startup_logo = config_file_new_option_boolean (
                section, "look_startup_logo",
                N_("display WeeChat logo at startup"),
                CONFIG_BOOLEAN_TRUE, NULL);
            config_look_startup_version = config_file_new_option_boolean (
                section, "look_startup_version",
                N_("display WeeChat version at startup"),
                CONFIG_BOOLEAN_TRUE, NULL);
            config_look_weechat_slogan = config_file_new_option_string (
                section, "look_weechat_slogan",
                N_("WeeChat slogan (if empty, slogan is not used)"),
                0, 0, "the geekest IRC client!", NULL);
            config_look_scroll_amount = config_file_new_option_integer (
                section, "look_scroll_amount",
                N_("how many lines to scroll by with scroll_up and "
                   "scroll_down"),
                1, INT_MAX, 3, &config_change_buffer_content);
            config_look_buffer_time_format = config_file_new_option_string (
                section, "look_buffer_time_format",
                N_("time format for buffers"),
                0, 0, "[%H:%M:%S]", &config_change_buffer_time_format);
            config_look_color_nicks_number = config_file_new_option_integer (
                section, "look_color_nicks_number",
                N_("number of colors to use for nicks colors"),
                1, 10, 10, &config_change_nicks_colors);
            config_look_nicklist = config_file_new_option_boolean (
                section, "look_nicklist",
                N_("display nicklist (on buffers with nicklist enabled"),
                CONFIG_BOOLEAN_TRUE, &config_change_buffers);
            config_look_nicklist_position = config_file_new_option_integer_with_string (
                section, "look_nicklist_position",
                N_("nicklist position (top, left, right (default), "
                   "bottom)"),
                "left|right|top|bottom", 1, &config_change_buffers);
            config_look_nicklist_min_size = config_file_new_option_integer (
                section, "look_nicklist_min_size",
                N_("min size for nicklist (width or height, depending on "
                   "look_nicklist_position "),
                0, 100, 0, &config_change_buffers);
            config_look_nicklist_max_size = config_file_new_option_integer (
                section, "look_nicklist_max_size",
                N_("max size for nicklist (width or height, depending on "
                   "look_nicklist_position (0 = no max size; if min = max "
                   "and > 0, then size is fixed))"),
                0, 100, 0, &config_change_buffers);
            config_look_nicklist_separator = config_file_new_option_boolean (
                section, "look_nicklist_separator",
                N_("separator between chat and nicklist"),
                CONFIG_BOOLEAN_TRUE, &config_change_buffers);
            config_look_nickmode = config_file_new_option_boolean (
                section, "look_nickmode",
                N_("display nick mode ((half)op/voice) before each nick"),
                CONFIG_BOOLEAN_TRUE, &config_change_buffers);
            config_look_nickmode_empty = config_file_new_option_boolean (
                section, "look_nickmode_empty",
                N_("display space if nick mode is not (half)op/voice"),
                CONFIG_BOOLEAN_FALSE, &config_change_buffers);
            config_look_prefix[GUI_CHAT_PREFIX_INFO] = config_file_new_option_string (
                section, "look_prefix_info",
                N_("prefix for info messages"),
                0, 0, "-=-", &config_change_prefix);
            config_look_prefix[GUI_CHAT_PREFIX_ERROR] = config_file_new_option_string (
                section, "look_prefix_error",
                N_("prefix for error messages"),
                0, 0, "=!=", &config_change_prefix);
            config_look_prefix[GUI_CHAT_PREFIX_NETWORK] = config_file_new_option_string (
                section, "look_prefix_network",
                N_("prefix for network messages"),
                0, 0, "-@-", &config_change_prefix);
            config_look_prefix[GUI_CHAT_PREFIX_ACTION] = config_file_new_option_string (
                section, "look_prefix_action",
                N_("prefix for action messages"),
                0, 0, "-*-", &config_change_prefix);
            config_look_prefix[GUI_CHAT_PREFIX_JOIN] = config_file_new_option_string (
                section, "look_prefix_join",
                N_("prefix for join messages"),
                0, 0, "-->", &config_change_prefix);
            config_look_prefix[GUI_CHAT_PREFIX_QUIT] = config_file_new_option_string (
                section, "look_prefix_quit",
                N_("prefix for quit messages"),
                0, 0, "<--", &config_change_prefix);
            config_look_prefix_align = config_file_new_option_integer_with_string (
                section, "look_prefix_align",
                N_("prefix alignment (none, left, right (default))"),
                "none|left|right", 2, &config_change_buffers);
            config_look_prefix_align_max = config_file_new_option_integer (
                section, "look_prefix_align_max",
                N_("max size for prefix (0 = no max size)"),
                0, 64, 0, &config_change_buffers);
            config_look_prefix_suffix = config_file_new_option_string (
                section, "look_prefix_suffix",
                N_("string displayed after prefix"),
                0, 0, "|", &config_change_buffers);
            config_look_nick_completor = config_file_new_option_string (
                section, "look_nick_completor",
                N_("the string inserted after nick completion"),
                0, 0, ":", NULL);
            config_look_nick_completion_ignore = config_file_new_option_string (
                section, "look_nick_completion_ignore",
                N_("chars ignored for nick completion"),
                0, 0, "[]-^", NULL);
            config_look_nick_completion_smart = config_file_new_option_boolean (
                section, "look_nick_completion_smart",
                N_("smart completion for nicks (completes with last speakers first)"),
                CONFIG_BOOLEAN_TRUE, NULL);
            config_look_nick_complete_first = config_file_new_option_boolean (
                section, "look_nick_complete_first",
                N_("complete only with first nick found"),
                CONFIG_BOOLEAN_FALSE, NULL);
            config_look_infobar = config_file_new_option_boolean (
                section, "look_infobar",
                N_("enable info bar"),
                CONFIG_BOOLEAN_TRUE, &config_change_buffers);
            config_look_infobar_time_format = config_file_new_option_string (
                section, "look_infobar_time_format",
                N_("time format for time in infobar"),
                0, 0, "%B, %A %d %Y", &config_change_buffer_content);
            config_look_infobar_seconds = config_file_new_option_boolean (
                section, "look_infobar_seconds",
                N_("display seconds in infobar time"),
                CONFIG_BOOLEAN_TRUE, &config_change_buffer_content);
            config_look_infobar_delay_highlight = config_file_new_option_integer (
                section, "look_infobar_delay_highlight",
                N_("delay (in seconds) for highlight messages in "
                   "infobar (0 = disable highlight notifications in "
                   "infobar)"),
                0, INT_MAX, 7, NULL);
            config_look_hotlist_names_count = config_file_new_option_integer (
                section, "look_hotlist_names_count",
                N_("max number of names in hotlist (0 = no name "
                   "displayed, only buffer numbers)"),
                0, 32, 3, &config_change_buffer_content);
            config_look_hotlist_names_level = config_file_new_option_integer (
                section, "look_hotlist_names_level",
                N_("level for displaying names in hotlist (combination "
                   "of: 1=join/part, 2=message, 4=private, 8=highlight, "
                   "for example: 12=private+highlight)"),
                1, 15, 12, &config_change_buffer_content);
            config_look_hotlist_names_length = config_file_new_option_integer (
                section, "look_hotlist_names_length",
                N_("max length of names in hotlist (0 = no limit)"),
                0, 32, 0, &config_change_buffer_content);
            config_look_hotlist_sort = config_file_new_option_integer_with_string (
                section, "look_hotlist_sort",
                N_("hotlist sort type (group_time_asc (default), "
                   "group_time_desc, group_number_asc, group_number_desc, "
                   "number_asc, number_desc)"),
                "group_time_asc|group_time_desc|group_number_asc|"
                "group_number_desc|number_asc|number_desc",
                0, &config_change_hotlist);
            config_look_day_change = config_file_new_option_boolean (
                section, "look_day_change",
                N_("display special message when day changes"),
                CONFIG_BOOLEAN_TRUE, NULL);
            config_look_day_change_time_format = config_file_new_option_string (
                section, "look_day_change_time_format",
                N_("time format for date displayed when day changed"),
                0, 0, "%a, %d %b %Y", NULL);
            config_look_read_marker = config_file_new_option_string (
                section, "look_read_marker",
                N_("use a marker on servers/channels to show first unread "
                   "line"),
                0, 1, " ", &config_change_read_marker);
            config_look_input_format = config_file_new_option_string (
                section, "look_input_format",
                N_("format for input prompt ('%c' is replaced by channel "
                   "or server, '%n' by nick and '%m' by nick modes)"),
                0, 0, "[%n(%m)] ", &config_change_buffer_content);
            config_look_paste_max_lines = config_file_new_option_integer (
                section, "look_paste_max_lines",
                N_("max number of lines for paste without asking user "
                   "(0 = disable this feature)"),
                0, INT_MAX, 3, NULL);
        }
            
        /* colors */
        section = config_file_new_section (weechat_config, "colors",
                                           NULL, NULL, NULL);
        if (section)
        {
            /* general color settings */
            config_color_separator = config_file_new_option_color (
                section, "color_separator",
                N_("color for window separators (when splited)"),
                GUI_COLOR_SEPARATOR, "blue", &config_change_color);
            /* title window */
            config_color_title = config_file_new_option_color (
                section, "color_title",
                N_("color for title bar"),
                GUI_COLOR_TITLE, "default", &config_change_color);
            config_color_title_bg = config_file_new_option_color (
                section, "color_title_bg",
                N_("background color for title bar"),
                -1, "blue", &config_change_color);
            config_color_title_more = config_file_new_option_color (
                section, "color_title_more",
                N_("color for '+' when scrolling title"),
                GUI_COLOR_TITLE_MORE, "lightmagenta", &config_change_color);
            /* chat window */
            config_color_chat = config_file_new_option_color (
                section, "color_chat",
                N_("color for chat text"),
                GUI_COLOR_CHAT, "default", &config_change_color);
            config_color_chat_bg = config_file_new_option_color (
                section, "color_chat_bg",
                N_("background for chat text"),
                -1, "default", &config_change_color);
            config_color_chat_time = config_file_new_option_color (
                section, "color_chat_time",
                N_("color for time in chat window"),
                GUI_COLOR_CHAT_TIME, "default", &config_change_color);
            config_color_chat_time_delimiters = config_file_new_option_color (
                section, "color_chat_time_delimiters",
                N_("color for time delimiters)"),
                GUI_COLOR_CHAT_TIME_DELIMITERS, "brown", &config_change_color);
            config_color_chat_prefix[GUI_CHAT_PREFIX_INFO] = config_file_new_option_color (
                section, "color_chat_prefix_info",
                N_("color for info prefix"),
                GUI_COLOR_CHAT_PREFIX_INFO, "lightcyan", &config_change_color);
            config_color_chat_prefix[GUI_CHAT_PREFIX_ERROR] = config_file_new_option_color (
                section, "color_chat_prefix_error",
                N_("color for error prefix"),
                GUI_COLOR_CHAT_PREFIX_ERROR, "yellow", &config_change_color);
            config_color_chat_prefix[GUI_CHAT_PREFIX_NETWORK] = config_file_new_option_color (
                section, "color_chat_prefix_network",
                N_("color for network prefix"),
                GUI_COLOR_CHAT_PREFIX_NETWORK, "lightmagenta", &config_change_color);
            config_color_chat_prefix[GUI_CHAT_PREFIX_ACTION] = config_file_new_option_color (
                section, "color_chat_prefix_action",
                N_("color for action prefix"),
                GUI_COLOR_CHAT_PREFIX_ACTION, "white", &config_change_color);
            config_color_chat_prefix[GUI_CHAT_PREFIX_JOIN] = config_file_new_option_color (
                section, "color_chat_prefix_join",
                N_("color for join prefix"),
                GUI_COLOR_CHAT_PREFIX_JOIN, "lightgreen", &config_change_color);
            config_color_chat_prefix[GUI_CHAT_PREFIX_QUIT] = config_file_new_option_color (
                section, "color_chat_prefix_quit",
                N_("color for quit prefix"),
                GUI_COLOR_CHAT_PREFIX_QUIT, "lightred", &config_change_color);
            config_color_chat_prefix_more = config_file_new_option_color (
                section, "color_chat_prefix_more",
                N_("color for '+' when prefix is too long"),
                GUI_COLOR_CHAT_PREFIX_MORE, "lightmagenta", &config_change_color);
            config_color_chat_prefix_suffix = config_file_new_option_color (
                section, "color_chat_prefix_suffix",
                N_("color for text after prefix"),
                GUI_COLOR_CHAT_PREFIX_SUFFIX, "green", &config_change_color);
            config_color_chat_buffer = config_file_new_option_color (
                section, "color_chat_buffer",
                N_("color for buffer names"),
                GUI_COLOR_CHAT_BUFFER, "white", &config_change_color);
            config_color_chat_server = config_file_new_option_color (
                section, "color_chat_server",
                N_("color for server names"),
                GUI_COLOR_CHAT_SERVER, "brown", &config_change_color);
            config_color_chat_channel = config_file_new_option_color (
                section, "color_chat_channel",
                N_("color for channel names"),
                GUI_COLOR_CHAT_CHANNEL, "white", &config_change_color);
            config_color_chat_nick = config_file_new_option_color (
                section, "color_chat_nick",
                N_("color for nicks"),
                GUI_COLOR_CHAT_NICK, "lightcyan", &config_change_color);
            config_color_chat_nick_self = config_file_new_option_color (
                section, "color_chat_nick_self",
                N_("color for local nick"),
                GUI_COLOR_CHAT_NICK_SELF, "white", &config_change_color);
            config_color_chat_nick_other = config_file_new_option_color (
                section, "color_chat_nick_other",
                N_("color for other nick in private buffer"),
                GUI_COLOR_CHAT_NICK_OTHER, "default", &config_change_color);
            config_color_chat_nick_colors[0] = config_file_new_option_color (
                section, "color_chat_nick_color1",
                N_("color #1 for nick"),
                GUI_COLOR_CHAT_NICK1, "cyan", &config_change_color);
            config_color_chat_nick_colors[1] = config_file_new_option_color (
                section, "color_chat_nick_color2",
                N_("color #2 for nick"),
                GUI_COLOR_CHAT_NICK2, "magenta", &config_change_color);
            config_color_chat_nick_colors[2] = config_file_new_option_color (
                section, "color_chat_nick_color3",
                N_("color #3 for nick"),
                GUI_COLOR_CHAT_NICK3, "green", &config_change_color);
            config_color_chat_nick_colors[3] = config_file_new_option_color (
                section, "color_chat_nick_color4",
                N_("color #4 for nick"),
                GUI_COLOR_CHAT_NICK4, "brown", &config_change_color);
            config_color_chat_nick_colors[4] = config_file_new_option_color (
                section, "color_chat_nick_color5",
                N_("color #5 for nick"),
                GUI_COLOR_CHAT_NICK5, "lightblue", &config_change_color);
            config_color_chat_nick_colors[5] = config_file_new_option_color (
                section, "color_chat_nick_color6",
                N_("color #6 for nick"),
                GUI_COLOR_CHAT_NICK6, "default", &config_change_color);
            config_color_chat_nick_colors[6] = config_file_new_option_color (
                section, "color_chat_nick_color7",
                N_("color #7 for nick"),
                GUI_COLOR_CHAT_NICK7, "lightcyan", &config_change_color);
            config_color_chat_nick_colors[7] = config_file_new_option_color (
                section, "color_chat_nick_color8",
                N_("color #8 for nick"),
                GUI_COLOR_CHAT_NICK8, "lightmagenta", &config_change_color);
            config_color_chat_nick_colors[8] = config_file_new_option_color (
                section, "color_chat_nick_color9",
                N_("color #9 for nick"),
                GUI_COLOR_CHAT_NICK9, "lightgreen", &config_change_color);
            config_color_chat_nick_colors[9] = config_file_new_option_color (
                section, "color_chat_nick_color10",
                N_("color #10 for nick"),
                GUI_COLOR_CHAT_NICK10, "blue", &config_change_color);
            config_color_chat_host = config_file_new_option_color (
                section, "color_chat_host",
                N_("color for hostnames"),
                GUI_COLOR_CHAT_HOST, "cyan", &config_change_color);
            config_color_chat_delimiters = config_file_new_option_color (
                section, "color_chat_delimiters",
                N_("color for delimiters"),
                GUI_COLOR_CHAT_DELIMITERS, "green", &config_change_color);
            config_color_chat_highlight = config_file_new_option_color (
                section, "color_chat_highlight",
                N_("color for highlighted nick"),
                GUI_COLOR_CHAT_HIGHLIGHT, "yellow", &config_change_color);
            config_color_chat_read_marker = config_file_new_option_color (
                section, "color_chat_read_marker",
                N_("color for unread data marker"),
                GUI_COLOR_CHAT_READ_MARKER, "yellow", &config_change_color);
            config_color_chat_read_marker_bg = config_file_new_option_color (
                section, "color_chat_read_marker_bg",
                N_("background color for unread data marker"),
                -1, "magenta", &config_change_color);
            /* status window */
            config_color_status = config_file_new_option_color (
                section, "color_status",
                N_("color for status bar"),
                GUI_COLOR_STATUS, "default", &config_change_color);
            config_color_status_bg = config_file_new_option_color (
                section, "color_status_bg",
                N_("background color for status bar"),
                -1, "blue", &config_change_color);
            config_color_status_delimiters = config_file_new_option_color (
                section, "color_status_delimiters",
                N_("color for status bar delimiters"),
                GUI_COLOR_STATUS_DELIMITERS, "cyan", &config_change_color);
            config_color_status_channel = config_file_new_option_color (
                section, "color_status_channel",
                N_("color for current channel in status bar"),
                GUI_COLOR_STATUS_CHANNEL, "white", &config_change_color);
            config_color_status_data_msg = config_file_new_option_color (
                section, "color_status_data_msg",
                N_("color for window with new messages (status bar)"),
                GUI_COLOR_STATUS_DATA_MSG, "yellow", &config_change_color);
            config_color_status_data_private = config_file_new_option_color (
                section, "color_status_data_private",
                N_("color for window with private message (status bar)"),
                GUI_COLOR_STATUS_DATA_PRIVATE, "lightgreen", &config_change_color);
            config_color_status_data_highlight = config_file_new_option_color (
                section, "color_status_data_highlight",
                N_("color for window with highlight (status bar)"),
                GUI_COLOR_STATUS_DATA_HIGHLIGHT, "lightmagenta", &config_change_color);
            config_color_status_data_other = config_file_new_option_color (
                section, "color_status_data_other",
                N_("color for window with new data (not messages) "
                   "(status bar)"),
                GUI_COLOR_STATUS_DATA_OTHER, "default", &config_change_color);
            config_color_status_more = config_file_new_option_color (
                section, "color_status_more",
                N_("color for window with new data (status bar)"),
                GUI_COLOR_STATUS_MORE, "white", &config_change_color);
            /* infobar window */
            config_color_infobar = config_file_new_option_color (
                section, "color_infobar",
                N_("color for infobar text"),
                GUI_COLOR_INFOBAR, "black", &config_change_color);
            config_color_infobar_bg = config_file_new_option_color (
                section, "color_infobar_bg",
                N_("background color for info bar text"),
                -1, "cyan", &config_change_color);
            config_color_infobar_delimiters = config_file_new_option_color (
                section, "color_infobar_delimiters",
                N_("color for infobar delimiters"),
                GUI_COLOR_INFOBAR_DELIMITERS, "blue", &config_change_color);
            config_color_infobar_highlight = config_file_new_option_color (
                section, "color_infobar_highlight",
                N_("color for infobar highlight notification"),
                GUI_COLOR_INFOBAR_HIGHLIGHT, "white", &config_change_color);
            /* input window */
            config_color_input = config_file_new_option_color (
                section, "color_input",
                N_("color for input text"),
                GUI_COLOR_INPUT, "default", &config_change_color);
            config_color_input_bg = config_file_new_option_color (
                section, "color_input_bg",
                N_("background color for input text"),
                -1, "default", &config_change_color);
            config_color_input_server = config_file_new_option_color (
                section, "color_input_server",
                N_("color for input text (server name)"),
                GUI_COLOR_INPUT_SERVER, "brown", &config_change_color);
            config_color_input_channel = config_file_new_option_color (
                section, "color_input_channel",
                N_("color for input text (channel name)"),
                GUI_COLOR_INPUT_CHANNEL, "white", &config_change_color);
            config_color_input_nick = config_file_new_option_color (
                section, "color_input_nick",
                N_("color for input text (nick name)"),
                GUI_COLOR_INPUT_NICK, "lightcyan", &config_change_color);
            config_color_input_delimiters = config_file_new_option_color (
                section, "color_input_delimiters",
                N_("color for input text (delimiters)"),
                GUI_COLOR_INPUT_DELIMITERS, "cyan", &config_change_color);
            config_color_input_text_not_found = config_file_new_option_color (
                section, "color_input_text_not_found",
                N_("color for text not found"),
                GUI_COLOR_INPUT_TEXT_NOT_FOUND, "red", &config_change_color);
            config_color_input_actions = config_file_new_option_color (
                section, "color_input_actions",
                N_("color for actions in input window"),
                GUI_COLOR_INPUT_ACTIONS, "lightgreen", &config_change_color);
            /* nicklist window */
            config_color_nicklist = config_file_new_option_color (
                section, "color_nicklist",
                N_("color for nicklist"),
                GUI_COLOR_NICKLIST, "default", &config_change_color);
            config_color_nicklist_bg = config_file_new_option_color (
                section, "color_nicklist_bg",
                N_("background color for nicklist"),
                -1, "default", &config_change_color);
            config_color_nicklist_away = config_file_new_option_color (
                section, "color_nicklist_away",
                N_("color for away nicknames"),
                GUI_COLOR_NICKLIST_AWAY, "cyan", &config_change_color);
            config_color_nicklist_prefix1 = config_file_new_option_color (
                section, "color_nicklist_prefix1",
                N_("color for prefix 1"),
                GUI_COLOR_NICKLIST_PREFIX1, "lightgreen", &config_change_color);
            config_color_nicklist_prefix2 = config_file_new_option_color (
                section, "color_nicklist_prefix2",
                N_("color for prefix 2"),
                GUI_COLOR_NICKLIST_PREFIX2, "lightmagenta", &config_change_color);
            config_color_nicklist_prefix3 = config_file_new_option_color (
                section, "color_nicklist_prefix3",
                N_("color for prefix 3"),
                GUI_COLOR_NICKLIST_PREFIX3, "yellow", &config_change_color);
            config_color_nicklist_prefix4 = config_file_new_option_color (
                section, "color_nicklist_prefix4",
                N_("color for prefix 4"),
                GUI_COLOR_NICKLIST_PREFIX4, "blue", &config_change_color);
            config_color_nicklist_prefix5 = config_file_new_option_color (
                section, "color_nicklist_prefix5",
                N_("color for prefix 5"),
                GUI_COLOR_NICKLIST_PREFIX5, "brown", &config_change_color);
            config_color_nicklist_more = config_file_new_option_color (
                section, "color_nicklist_more",
                N_("color for '+' when scrolling nicks (nicklist)"),
                GUI_COLOR_NICKLIST_MORE, "lightmagenta", &config_change_color);
            config_color_nicklist_separator = config_file_new_option_color (
                section, "color_nicklist_separator",
                N_("color for nicklist separator"),
                GUI_COLOR_NICKLIST_SEPARATOR, "blue", &config_change_color);
            /* status info */
            config_color_info = config_file_new_option_color (
                section, "color_info",
                N_("color for status info"),
                GUI_COLOR_INFO, "default", &config_change_color);
            config_color_info_bg = config_file_new_option_color (
                section, "color_info_bg",
                N_("background color for status info"),
                -1, "default", &config_change_color);
            config_color_info_waiting = config_file_new_option_color (
                section, "color_info_waiting",
                N_("color for \"waiting\" status info"),
                GUI_COLOR_INFO_WAITING, "lightcyan", &config_change_color);
            config_color_info_connecting = config_file_new_option_color (
                section, "color_info_connecting",
                N_("color for \"connecting\" status info"),
                GUI_COLOR_INFO_CONNECTING, "yellow", &config_change_color);
            config_color_info_active = config_file_new_option_color (
                section, "color_info_active",
                N_("color for \"active\" status info"),
                GUI_COLOR_INFO_ACTIVE, "lightblue", &config_change_color);
            config_color_info_done = config_file_new_option_color (
                section, "color_info_done",
                N_("color for \"done\" status info"),
                GUI_COLOR_INFO_DONE, "lightgreen", &config_change_color);
            config_color_info_failed = config_file_new_option_color (
                section, "color_info_failed",
                N_("color for \"failed\" status info"),
                GUI_COLOR_INFO_FAILED, "lightred", &config_change_color);
            config_color_info_aborted = config_file_new_option_color (
                section, "color_info_aborted",
                N_("color for \"aborted\" status info"),
                GUI_COLOR_INFO_ABORTED, "lightred", &config_change_color);
        }

        /* history */
        section = config_file_new_section (weechat_config, "history",
                                           NULL, NULL, NULL);
        if (section)
        {
            config_history_max_lines = config_file_new_option_integer (
                section, "history_max_lines",
                N_("maximum number of lines in history per buffer "
                   "(0 = unlimited)"),
                0, INT_MAX, 4096, NULL);
            config_history_max_commands = config_file_new_option_integer (
                section, "history_max_commands",
                N_("maximum number of user commands in history (0 = "
                   "unlimited)"),
                0, INT_MAX, 100, NULL);
            config_history_display_default = config_file_new_option_integer (
                section, "history_display_default",
                N_("maximum number of commands to display by default in "
                   "history listing (0 = unlimited)"),
                0, INT_MAX, 5, NULL);
        }

        /* proxy */
        section = config_file_new_section (weechat_config, "proxy",
                                           NULL, NULL, NULL);
        if (section)
        {
            config_proxy_use = config_file_new_option_boolean (
                section, "proxy_use",
                N_("use a proxy server"),
                CONFIG_BOOLEAN_FALSE, NULL);
            config_proxy_type = config_file_new_option_integer_with_string (
                section, "proxy_type",
                N_("proxy type (http (default), socks4, socks5)"),
                "http|socks4|socks5", 0, NULL);
            config_proxy_ipv6 = config_file_new_option_boolean (
                section, "proxy_ipv6",
                N_("connect to proxy using ipv6"),
                CONFIG_BOOLEAN_FALSE, NULL);
            config_proxy_address = config_file_new_option_string (
                section, "proxy_address",
                N_("proxy server address (IP or hostname)"),
                0, 0, "", NULL);
            config_proxy_port = config_file_new_option_integer (
                section, "proxy_port",
                N_("port for connecting to proxy server"),
                0, 65535, 3128, NULL);
            config_proxy_username = config_file_new_option_string (
                section, "proxy_username",
                N_("username for proxy server"),
                0, 0, "", NULL);
            config_proxy_password = config_file_new_option_string (
                section, "proxy_password",
                N_("password for proxy server"),
                0, 0, "", NULL);
        }
            
        /* plugins */
        section = config_file_new_section (weechat_config, "plugins",
                                           NULL, NULL, NULL);
        if (section)
        {
            config_plugins_path = config_file_new_option_string (
                section, "plugins_path",
                N_("path for searching plugins ('%h' will be replaced by "
                   "WeeChat home, ~/.weechat by default)"),
                0, 0, "%h/plugins", NULL);
            config_plugins_autoload = config_file_new_option_string (
                section, "plugins_autoload",
                N_("comma separated list of plugins to load automatically "
                   "at startup, \"*\" means all plugins found (names may "
                   "be partial, for example \"perl\" is ok for "
                   "\"perl.so\")"),
                0, 0, "*", NULL);
            config_plugins_extension = config_file_new_option_string (
                section, "plugins_extension",
                N_("standard plugins extension in filename (for example "
                   "\".so\" under Linux or \".dll\" under Windows)"),
                0, 0,
#ifdef WIN32
                ".dll",
#else
                ".so",
#endif              
                NULL);
        }
        
        /* keys */
        section = config_file_new_section (weechat_config, "keys",
                                           &config_weechat_read_key,
                                           &config_weechat_write_keys,
                                           &config_weechat_write_keys);
    }
}

/*
 * config_weechat_read: read WeeChat configuration file
 *                      return:  0 = successful
 *                              -1 = configuration file file not found
 *                              -2 = error in configuration file
 */

int
config_weechat_read ()
{
    return config_file_read (weechat_config);
}

/*
 * config_weechat_reload: reload WeeChat configuration file
 *                        return:  0 = successful
 *                                -1 = configuration file file not found
 *                                -2 = error in configuration file
 */

int
config_weechat_reload ()
{
    /* remove all keys */
    gui_keyboard_free_all ();
    
    /* reload configuration file */
    return config_file_reload (weechat_config);
}

/*
 * config_weechat_write: write WeeChat configuration file
 *                       return:  0 if ok
 *                              < 0 if error
 */

int
config_weechat_write ()
{
    log_printf (_("Saving WeeChat configuration to disk\n"));
    return config_file_write (weechat_config, 0);
}
