/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * wee-config.c: WeeChat configuration options (file weechat.conf)
 */

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
#include <regex.h>

#include "weechat.h"
#include "wee-config.h"
#include "wee-hook.h"
#include "wee-log.h"
#include "wee-network.h"
#include "wee-utf8.h"
#include "wee-util.h"
#include "wee-list.h"
#include "wee-proxy.h"
#include "wee-string.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-key.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-line.h"
#include "../gui/gui-main.h"
#include "../gui/gui-mouse.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


struct t_config_file *weechat_config_file = NULL;
struct t_config_section *weechat_config_section_debug = NULL;
struct t_config_section *weechat_config_section_color = NULL;
struct t_config_section *weechat_config_section_proxy = NULL;
struct t_config_section *weechat_config_section_bar = NULL;
struct t_config_section *weechat_config_section_notify = NULL;

/* config, startup section */

struct t_config_option *config_startup_command_after_plugins;
struct t_config_option *config_startup_command_before_plugins;
struct t_config_option *config_startup_display_logo;
struct t_config_option *config_startup_display_version;
struct t_config_option *config_startup_sys_rlimit;

/* config, look & feel section */

struct t_config_option *config_look_align_end_of_lines;
struct t_config_option *config_look_bar_more_left;
struct t_config_option *config_look_bar_more_right;
struct t_config_option *config_look_bar_more_up;
struct t_config_option *config_look_bar_more_down;
struct t_config_option *config_look_buffer_notify_default;
struct t_config_option *config_look_buffer_time_format;
struct t_config_option *config_look_color_basic_force_bold;
struct t_config_option *config_look_color_inactive_window;
struct t_config_option *config_look_color_inactive_buffer;
struct t_config_option *config_look_color_inactive_time;
struct t_config_option *config_look_color_inactive_prefix_buffer;
struct t_config_option *config_look_color_inactive_prefix;
struct t_config_option *config_look_color_inactive_message;
struct t_config_option *config_look_color_nick_offline;
struct t_config_option *config_look_color_pairs_auto_reset;
struct t_config_option *config_look_color_real_white;
struct t_config_option *config_look_command_chars;
struct t_config_option *config_look_confirm_quit;
struct t_config_option *config_look_day_change;
struct t_config_option *config_look_day_change_time_format;
struct t_config_option *config_look_eat_newline_glitch;
struct t_config_option *config_look_highlight;
struct t_config_option *config_look_highlight_regex;
struct t_config_option *config_look_highlight_tags;
struct t_config_option *config_look_hotlist_add_buffer_if_away;
struct t_config_option *config_look_hotlist_buffer_separator;
struct t_config_option *config_look_hotlist_count_max;
struct t_config_option *config_look_hotlist_count_min_msg;
struct t_config_option *config_look_hotlist_names_count;
struct t_config_option *config_look_hotlist_names_length;
struct t_config_option *config_look_hotlist_names_level;
struct t_config_option *config_look_hotlist_names_merged_buffers;
struct t_config_option *config_look_hotlist_short_names;
struct t_config_option *config_look_hotlist_sort;
struct t_config_option *config_look_hotlist_unique_numbers;
struct t_config_option *config_look_input_cursor_scroll;
struct t_config_option *config_look_input_share;
struct t_config_option *config_look_input_share_overwrite;
struct t_config_option *config_look_input_undo_max;
struct t_config_option *config_look_item_time_format;
struct t_config_option *config_look_item_buffer_filter;
struct t_config_option *config_look_jump_current_to_previous_buffer;
struct t_config_option *config_look_jump_previous_buffer_when_closing;
struct t_config_option *config_look_jump_smart_back_to_buffer;
struct t_config_option *config_look_mouse;
struct t_config_option *config_look_mouse_timer_delay;
struct t_config_option *config_look_paste_bracketed;
struct t_config_option *config_look_paste_bracketed_timer_delay;
struct t_config_option *config_look_paste_max_lines;
struct t_config_option *config_look_prefix[GUI_CHAT_NUM_PREFIXES];
struct t_config_option *config_look_prefix_align;
struct t_config_option *config_look_prefix_align_max;
struct t_config_option *config_look_prefix_align_min;
struct t_config_option *config_look_prefix_align_more;
struct t_config_option *config_look_prefix_buffer_align;
struct t_config_option *config_look_prefix_buffer_align_max;
struct t_config_option *config_look_prefix_buffer_align_more;
struct t_config_option *config_look_prefix_same_nick;
struct t_config_option *config_look_prefix_suffix;
struct t_config_option *config_look_read_marker;
struct t_config_option *config_look_read_marker_always_show;
struct t_config_option *config_look_read_marker_string;
struct t_config_option *config_look_save_config_on_exit;
struct t_config_option *config_look_save_layout_on_exit;
struct t_config_option *config_look_scroll_amount;
struct t_config_option *config_look_scroll_bottom_after_switch;
struct t_config_option *config_look_scroll_page_percent;
struct t_config_option *config_look_search_text_not_found_alert;
struct t_config_option *config_look_separator_horizontal;
struct t_config_option *config_look_separator_vertical;
struct t_config_option *config_look_set_title;
struct t_config_option *config_look_time_format;
struct t_config_option *config_look_window_separator_horizontal;
struct t_config_option *config_look_window_separator_vertical;

/* config, colors section */

struct t_config_option *config_color_separator;
struct t_config_option *config_color_bar_more;
struct t_config_option *config_color_chat;
struct t_config_option *config_color_chat_bg;
struct t_config_option *config_color_chat_inactive_window;
struct t_config_option *config_color_chat_inactive_buffer;
struct t_config_option *config_color_chat_time;
struct t_config_option *config_color_chat_time_delimiters;
struct t_config_option *config_color_chat_prefix_buffer;
struct t_config_option *config_color_chat_prefix_buffer_inactive_buffer;
struct t_config_option *config_color_chat_prefix[GUI_CHAT_NUM_PREFIXES];
struct t_config_option *config_color_chat_prefix_more;
struct t_config_option *config_color_chat_prefix_suffix;
struct t_config_option *config_color_chat_buffer;
struct t_config_option *config_color_chat_server;
struct t_config_option *config_color_chat_channel;
struct t_config_option *config_color_chat_nick;
struct t_config_option *config_color_chat_nick_colors;
struct t_config_option *config_color_chat_nick_self;
struct t_config_option *config_color_chat_nick_offline;
struct t_config_option *config_color_chat_nick_offline_highlight;
struct t_config_option *config_color_chat_nick_offline_highlight_bg;
struct t_config_option *config_color_chat_nick_other;
struct t_config_option *config_color_chat_host;
struct t_config_option *config_color_chat_delimiters;
struct t_config_option *config_color_chat_highlight;
struct t_config_option *config_color_chat_highlight_bg;
struct t_config_option *config_color_chat_read_marker;
struct t_config_option *config_color_chat_read_marker_bg;
struct t_config_option *config_color_chat_tags;
struct t_config_option *config_color_chat_text_found;
struct t_config_option *config_color_chat_text_found_bg;
struct t_config_option *config_color_chat_value;
struct t_config_option *config_color_status_number;
struct t_config_option *config_color_status_name;
struct t_config_option *config_color_status_name_ssl;
struct t_config_option *config_color_status_filter;
struct t_config_option *config_color_status_data_msg;
struct t_config_option *config_color_status_data_private;
struct t_config_option *config_color_status_data_highlight;
struct t_config_option *config_color_status_data_other;
struct t_config_option *config_color_status_count_msg;
struct t_config_option *config_color_status_count_private;
struct t_config_option *config_color_status_count_highlight;
struct t_config_option *config_color_status_count_other;
struct t_config_option *config_color_status_more;
struct t_config_option *config_color_status_time;
struct t_config_option *config_color_input_text_not_found;
struct t_config_option *config_color_input_actions;
struct t_config_option *config_color_nicklist_group;
struct t_config_option *config_color_nicklist_away;
struct t_config_option *config_color_nicklist_offline;

/* config, completion section */

struct t_config_option *config_completion_base_word_until_cursor;
struct t_config_option *config_completion_default_template;
struct t_config_option *config_completion_nick_add_space;
struct t_config_option *config_completion_nick_completer;
struct t_config_option *config_completion_nick_first_only;
struct t_config_option *config_completion_nick_ignore_chars;
struct t_config_option *config_completion_partial_completion_alert;
struct t_config_option *config_completion_partial_completion_command;
struct t_config_option *config_completion_partial_completion_command_arg;
struct t_config_option *config_completion_partial_completion_other;
struct t_config_option *config_completion_partial_completion_count;

/* config, history section */

struct t_config_option *config_history_max_buffer_lines_number;
struct t_config_option *config_history_max_buffer_lines_minutes;
struct t_config_option *config_history_max_commands;
struct t_config_option *config_history_max_visited_buffers;
struct t_config_option *config_history_display_default;

/* config, network section */

struct t_config_option *config_network_connection_timeout;
struct t_config_option *config_network_gnutls_ca_file;
struct t_config_option *config_network_gnutls_handshake_timeout;

/* config, plugin section */

struct t_config_option *config_plugin_autoload;
struct t_config_option *config_plugin_debug;
struct t_config_option *config_plugin_extension;
struct t_config_option *config_plugin_path;
struct t_config_option *config_plugin_save_config_on_unload;

/* other */

int config_length_prefix_same_nick = 0;
struct t_hook *config_day_change_timer = NULL;
int config_day_change_old_day = -1;
regex_t *config_highlight_regex = NULL;
char **config_highlight_tags = NULL;
int config_num_highlight_tags = 0;
char **config_plugin_extensions = NULL;
int config_num_plugin_extensions = 0;


/*
 * Callback for changes on option "weechat.startup.sys_rlimit".
 */

void
config_change_sys_rlimit (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (gui_init_ok)
        util_setrlimit ();
}

/*
 * Callback for changes on option "weechat.look.save_config_on_exit".
 */

void
config_change_save_config_on_exit (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (gui_init_ok && !CONFIG_BOOLEAN(config_look_save_config_on_exit))
    {
        gui_chat_printf (NULL,
                         _("Warning: you should now issue /save to write "
                           "\"save_config_on_exit\" option in configuration "
                           "file"));
    }
}

/*
 * Callback for changes on option "weechat.look.set_title".
 */

void
config_change_title (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (CONFIG_BOOLEAN(config_look_set_title))
        gui_window_set_title (PACKAGE_NAME " " PACKAGE_VERSION);
}

/*
 * Callback for changes on options that require a refresh of buffers.
 */

void
config_change_buffers (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on options that require a refresh of content of buffer.
 */

void
config_change_buffer_content (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (gui_init_ok)
        gui_current_window->refresh_needed = 1;
}

/*
 * Callback for changes on option "weechat.look.mouse".
 */

void
config_change_mouse (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (gui_init_ok)
    {
        if (CONFIG_BOOLEAN(config_look_mouse))
            gui_mouse_enable ();
        else
            gui_mouse_disable ();
    }
}

/*
 * Callback for changes on option "weechat.look.buffer_notify_default".
 */

void
config_change_buffer_notify_default (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    gui_buffer_notify_set_all ();
}

/*
 * Callback for changes on option "weechat.look.buffer_time_format".
 */

void
config_change_buffer_time_format (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    gui_chat_time_length = gui_chat_get_time_length ();
    gui_chat_change_time_format ();
    if (gui_init_ok)
        gui_window_ask_refresh (1);
}

/*
 * Computes the "prefix_max_length" on all buffers.
 */

void
config_compute_prefix_max_length_all_buffers ()
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->own_lines)
            gui_line_compute_prefix_max_length (ptr_buffer->own_lines);
        if (ptr_buffer->mixed_lines)
            gui_line_compute_prefix_max_length (ptr_buffer->mixed_lines);
    }
}

/*
 * Callback for changes on option "weechat.look.prefix_same_nick".
 */

void
config_change_prefix_same_nick (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    config_length_prefix_same_nick =
        gui_chat_strlen_screen (CONFIG_STRING(config_look_prefix_same_nick));

    config_compute_prefix_max_length_all_buffers ();
    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on option "weechat.look.eat_newline_glitch".
 */

void
config_change_eat_newline_glitch (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (gui_init_ok)
    {
        if (CONFIG_BOOLEAN(config_look_eat_newline_glitch))
        {
            gui_chat_printf (NULL,
                             _("WARNING: this option can cause serious display "
                               "bugs, if you have such problems, you must "
                               "turn off this option."));
            gui_term_set_eat_newline_glitch (0);
        }
        else
            gui_term_set_eat_newline_glitch (1);
    }
}

/*
 * Callback for changes on option "weechat.look.highlight_regex".
 */

void
config_change_highlight_regex (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (config_highlight_regex)
    {
        regfree (config_highlight_regex);
        free (config_highlight_regex);
        config_highlight_regex = NULL;
    }

    if (CONFIG_STRING(config_look_highlight_regex)
        && CONFIG_STRING(config_look_highlight_regex)[0])
    {
        config_highlight_regex = malloc (sizeof (*config_highlight_regex));
        if (config_highlight_regex)
        {
            if (string_regcomp (config_highlight_regex,
                                CONFIG_STRING(config_look_highlight_regex),
                                REG_EXTENDED | REG_ICASE) != 0)
            {
                free (config_highlight_regex);
                config_highlight_regex = NULL;
            }
        }
    }
}

/*
 * Callback for changes on option "weechat.look.highlight_tags".
 */

void
config_change_highlight_tags (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (config_highlight_tags)
    {
        string_free_split (config_highlight_tags);
        config_highlight_tags = NULL;
    }
    config_num_highlight_tags = 0;

    if (CONFIG_STRING(config_look_highlight_tags)
        && CONFIG_STRING(config_look_highlight_tags)[0])
    {
        config_highlight_tags = string_split (CONFIG_STRING(config_look_highlight_tags),
                                              ",", 0, 0, &config_num_highlight_tags);
    }
}

/*
 * Callback for changes on option "weechat.look.hotlist_sort".
 */

void
config_change_hotlist_sort (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    gui_hotlist_resort ();
}

/*
 * Callback for changes on option "weechat.look.paste_bracketed".
 */

void
config_change_paste_bracketed (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (gui_init_ok)
        gui_window_set_bracketed_paste_mode (CONFIG_BOOLEAN(config_look_paste_bracketed));
}

/*
 * Callback for changes on option "weechat.look.read_marker".
 */

void
config_change_read_marker (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on a prefix option.
 */

void
config_change_prefix (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    gui_chat_prefix_build ();
}

/*
 * Callback for changes on option "weechat.look.prefix_align_min".
 */

void
config_change_prefix_align_min (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    config_compute_prefix_max_length_all_buffers ();
    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on option "weechat.look.prefix_align_more".
 */

int
config_check_prefix_align_more (void *data, struct t_config_option *option,
                                const char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    return (utf8_strlen_screen (value) == 1) ? 1 : 0;
}

/*
 * Callback for changes on option "weechat.look.prefix_buffer_align_more".
 */

int
config_check_prefix_buffer_align_more (void *data,
                                       struct t_config_option *option,
                                       const char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    return (utf8_strlen_screen (value) == 1) ? 1 : 0;
}

/*
 * Callback for changes on a color option.
 */

void
config_change_color (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (gui_init_ok)
    {
        gui_color_init_weechat ();
        gui_window_ask_refresh (1);
    }
}

/*
 * Callback for changes on option "weechat.color.chat_nick_colors".
 */

void
config_change_nick_colors (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    gui_color_buffer_display ();
}

/*
 * Callback for changes on option "weechat.network.gnutls_ca_file".
 */

void
config_change_network_gnutls_ca_file (void *data,
                                      struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (network_init_ok)
        network_set_gnutls_ca_file ();
}

/*
 * Callback for changes on option "weechat.plugin.extension".
 */

void
config_change_plugin_extension (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (config_plugin_extensions)
    {
        string_free_split (config_plugin_extensions);
        config_plugin_extensions = NULL;
    }
    config_num_plugin_extensions = 0;

    if (CONFIG_STRING(config_plugin_extension)
        && CONFIG_STRING(config_plugin_extension)[0])
    {
        config_plugin_extensions = string_split (CONFIG_STRING(config_plugin_extension),
                                                 ",", 0, 0, &config_num_plugin_extensions);
    }
}

/*
 * Displays message "Day changed to xxx".
 */

int
config_day_change_timer_cb (void *data, int remaining_calls)
{
    struct timeval tv_time;
    struct tm *local_time;
    int new_mday;
    char text_time[256], *text_time2;
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    gettimeofday (&tv_time, NULL);
    local_time = localtime (&tv_time.tv_sec);
    new_mday = local_time->tm_mday;

    if ((config_day_change_old_day >= 0)
        && (new_mday != config_day_change_old_day))
    {
        if (CONFIG_BOOLEAN(config_look_day_change))
        {
            strftime (text_time, sizeof (text_time),
                      CONFIG_STRING(config_look_day_change_time_format),
                      local_time);
            text_time2 = string_iconv_to_internal (NULL, text_time);
            gui_add_hotlist = 0;
            for (ptr_buffer = gui_buffers; ptr_buffer;
                 ptr_buffer = ptr_buffer->next_buffer)
            {
                if (ptr_buffer->type == GUI_BUFFER_TYPE_FORMATTED)
                {
                    gui_chat_printf (ptr_buffer,
                                     _("\t\tDay changed to %s"),
                                     (text_time2) ?
                                     text_time2 : text_time);
                }
            }
            if (text_time2)
                free (text_time2);
            gui_add_hotlist = 1;
        }

        /* send signal "day_changed" */
        strftime (text_time, sizeof (text_time), "%Y-%m-%d", local_time);
        hook_signal_send ("day_changed", WEECHAT_HOOK_SIGNAL_STRING, text_time);
    }

    config_day_change_old_day = new_mday;

    return WEECHAT_RC_OK;
}

/*
 * Initializes some things after reading/reloading WeeChat configuration file.
 */

void
config_weechat_init_after_read ()
{
    int i;

    util_setrlimit ();

    gui_buffer_notify_set_all ();

    proxy_use_temp_proxies ();

    gui_bar_use_temp_bars ();
    if (gui_bars)
    {
        /*
         * at least one bar defined => just ensure that at least one bar is
         * using item "input_text"
         */
        gui_bar_create_default_input ();
    }
    else
    {
        /* no bar defined => create default bars */
        gui_bar_create_default ();
    }

    /* if no key was found configuration file, then we use default bindings */
    for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
    {
        if (!gui_keys[i])
            gui_key_default_bindings (i);
    }

    /* apply filters on all buffers */
    gui_filter_all_buffers ();
}

/*
 * Reloads WeeChat configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_READ_OK: OK
 *   WEECHAT_CONFIG_READ_MEMORY_ERROR: not enough memory
 *   WEECHAT_CONFIG_READ_FILE_NOT_FOUND: file not found
 */

int
config_weechat_reload_cb (void *data, struct t_config_file *config_file)
{
    int i, rc;

    /* make C compiler happy */
    (void) data;

    /* remove all keys */
    for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
    {
        gui_key_free_all (&gui_keys[i], &last_gui_key[i],
                          &gui_keys_count[i]);
    }

    /* remove all proxies */
    proxy_free_all ();

    /* remove all bars */
    gui_bar_free_all ();

    /* remove layout */
    gui_layout_buffer_reset (&gui_layout_buffers, &last_gui_layout_buffer);
    gui_layout_window_reset (&gui_layout_windows);

    /* remove all notify levels */
    config_file_section_free_options (weechat_config_section_notify);

    /* remove all filters */
    gui_filter_free_all ();

    rc = config_file_reload (config_file);

    config_weechat_init_after_read ();

    return rc;
}

/*
 * Gets debug level for a plugin (or "core").
 */

struct t_config_option *
config_weechat_debug_get (const char *plugin_name)
{
    return config_file_search_option (weechat_config_file,
                                      weechat_config_section_debug,
                                      plugin_name);
}

/*
 * Sets debug level for "core" and all plugins, using values from section
 * "debug".
 */

void
config_weechat_debug_set_all ()
{
    struct t_config_option *ptr_option;
    struct t_weechat_plugin *ptr_plugin;

    /* set debug for core */
    ptr_option = config_weechat_debug_get (PLUGIN_CORE);
    weechat_debug_core = (ptr_option) ? CONFIG_INTEGER(ptr_option) : 0;

    /* set debug for plugins */
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        ptr_option = config_weechat_debug_get (ptr_plugin->name);
        ptr_plugin->debug = (ptr_option) ? CONFIG_INTEGER(ptr_option) : 0;
    }
}

/*
 * Callback for changes on a debug option.
 */

void
config_weechat_debug_change_cb (void *data,
                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    config_weechat_debug_set_all ();
}

/*
 * Callback called when an option is created in section "debug".
 */

int
config_weechat_debug_create_option_cb (void *data,
                                       struct t_config_file *config_file,
                                       struct t_config_section *section,
                                       const char *option_name,
                                       const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = config_file_search_option (config_file, section,
                                                option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = config_file_option_set (ptr_option, value, 1);
            else
            {
                config_file_option_free (ptr_option);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = config_file_new_option (
                    config_file, section,
                    option_name, "integer",
                    _("debug level for plugin (\"core\" for WeeChat core)"),
                    NULL, 0, 32, "0", value, 0, NULL, NULL,
                    &config_weechat_debug_change_cb, NULL,
                    NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    /* set debug level for "core" and all plugins */
    config_weechat_debug_set_all ();

    return rc;
}

/*
 * Callback called when an option is deleted in section "debug".
 */

int
config_weechat_debug_delete_option_cb (void *data,
                                       struct t_config_file *config_file,
                                       struct t_config_section *section,
                                       struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    config_file_option_free (option);

    config_weechat_debug_set_all ();

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Sets debug level for a plugin (or "core").
 */

int
config_weechat_debug_set (const char *plugin_name, const char *value)
{
    return config_weechat_debug_create_option_cb (NULL,
                                                  weechat_config_file,
                                                  weechat_config_section_debug,
                                                  plugin_name,
                                                  value);
}

/*
 * Callback for changes on a palette option.
 */

void
config_weechat_palette_change_cb (void *data,
                                  struct t_config_option *option)
{
    char *error;
    int number;

    /* make C compiler happy */
    (void) data;
    (void) option;

    error = NULL;
    number = (int)strtol (option->name, &error, 10);
    if (error && !error[0])
    {
        gui_color_palette_add (number, CONFIG_STRING(option));
    }
}

/*
 * Callback called when an option is created in section "palette".
 */

int
config_weechat_palette_create_option_cb (void *data,
                                         struct t_config_file *config_file,
                                         struct t_config_section *section,
                                         const char *option_name,
                                         const char *value)
{
    struct t_config_option *ptr_option;
    char *error;
    int rc, number;

    /* make C compiler happy */
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    error = NULL;
    number = (int)strtol (option_name, &error, 10);
    if (error && !error[0])
    {
        if (option_name)
        {
            ptr_option = config_file_search_option (config_file, section,
                                                    option_name);
            if (ptr_option)
            {
                if (value)
                    rc = config_file_option_set (ptr_option, value, 1);
                else
                {
                    config_file_option_free (ptr_option);
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                }
            }
            else
            {
                if (value)
                {
                    ptr_option = config_file_new_option (
                        config_file, section,
                        option_name, "string",
                        _("alias for color"),
                        NULL, 0, 0, "", value, 0, NULL, NULL,
                        &config_weechat_palette_change_cb, NULL,
                        NULL, NULL);
                    rc = (ptr_option) ?
                        WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
                    if (ptr_option)
                        gui_color_palette_add (number, value);
                }
                else
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sError: palette option must be numeric"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    }

    return rc;
}

/*
 * Callback called when an option is deleted in section "palette".
 */

int
config_weechat_palette_delete_option_cb (void *data,
                                         struct t_config_file *config_file,
                                         struct t_config_section *section,
                                         struct t_config_option *option)
{
    char *error;
    int number;

    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    error = NULL;
    number = (int)strtol (option->name, &error, 10);
    if (error && !error[0])
        gui_color_palette_remove (number);

    config_file_option_free (option);

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Reads a proxy option in WeeChat configuration file.
 */

int
config_weechat_proxy_read_cb (void *data, struct t_config_file *config_file,
                              struct t_config_section *section,
                              const char *option_name, const char *value)
{
    char *pos_option, *proxy_name;
    struct t_proxy *ptr_temp_proxy;
    int index_option;

    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    if (option_name)
    {
        pos_option = strchr (option_name, '.');
        if (pos_option)
        {
            proxy_name = string_strndup (option_name, pos_option - option_name);
            if (proxy_name)
            {
                pos_option++;
                for (ptr_temp_proxy = weechat_temp_proxies; ptr_temp_proxy;
                     ptr_temp_proxy = ptr_temp_proxy->next_proxy)
                {
                    if (strcmp (ptr_temp_proxy->name, proxy_name) == 0)
                        break;
                }
                if (!ptr_temp_proxy)
                {
                    /* create new temp proxy */
                    ptr_temp_proxy = proxy_alloc (proxy_name);
                    if (ptr_temp_proxy)
                    {
                        /* add new temp proxy at end of queue */
                        ptr_temp_proxy->prev_proxy = last_weechat_temp_proxy;
                        ptr_temp_proxy->next_proxy = NULL;

                        if (!weechat_temp_proxies)
                            weechat_temp_proxies = ptr_temp_proxy;
                        else
                            last_weechat_temp_proxy->next_proxy = ptr_temp_proxy;
                        last_weechat_temp_proxy = ptr_temp_proxy;
                    }
                }

                if (ptr_temp_proxy)
                {
                    index_option = proxy_search_option (pos_option);
                    if (index_option >= 0)
                    {
                        proxy_create_option_temp (ptr_temp_proxy, index_option,
                                                  value);
                    }
                    else
                    {
                        gui_chat_printf (NULL,
                                         _("%sWarning: unknown option for "
                                           "section \"%s\": %s (value: \"%s\")"),
                                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                         section->name,
                                         option_name,
                                         value);
                    }
                }

                free (proxy_name);
            }
        }
    }

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Reads a bar option in WeeChat configuration file.
 */

int
config_weechat_bar_read_cb (void *data, struct t_config_file *config_file,
                            struct t_config_section *section,
                            const char *option_name, const char *value)
{
    char *pos_option, *bar_name;
    struct t_gui_bar *ptr_temp_bar;
    int index_option;

    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    if (option_name)
    {
        pos_option = strchr (option_name, '.');
        if (pos_option)
        {
            bar_name = string_strndup (option_name, pos_option - option_name);
            if (bar_name)
            {
                pos_option++;
                for (ptr_temp_bar = gui_temp_bars; ptr_temp_bar;
                     ptr_temp_bar = ptr_temp_bar->next_bar)
                {
                    if (strcmp (ptr_temp_bar->name, bar_name) == 0)
                        break;
                }
                if (!ptr_temp_bar)
                {
                    /* create new temp bar */
                    ptr_temp_bar = gui_bar_alloc (bar_name);
                    if (ptr_temp_bar)
                    {
                        /* add new temp bar at end of queue */
                        ptr_temp_bar->prev_bar = last_gui_temp_bar;
                        ptr_temp_bar->next_bar = NULL;

                        if (!gui_temp_bars)
                            gui_temp_bars = ptr_temp_bar;
                        else
                            last_gui_temp_bar->next_bar = ptr_temp_bar;
                        last_gui_temp_bar = ptr_temp_bar;
                    }
                }

                if (ptr_temp_bar)
                {
                    index_option = gui_bar_search_option (pos_option);
                    if (index_option >= 0)
                    {
                        gui_bar_create_option_temp (ptr_temp_bar, index_option,
                                                    value);
                    }
                    else
                    {
                        gui_chat_printf (NULL,
                                         _("%sWarning: unknown option for "
                                           "section \"%s\": %s (value: \"%s\")"),
                                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                         section->name,
                                         option_name,
                                         value);
                    }
                }

                free (bar_name);
            }
        }
    }

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Reads a layout option in WeeChat configuration file.
 */

int
config_weechat_layout_read_cb (void *data, struct t_config_file *config_file,
                               struct t_config_section *section,
                               const char *option_name, const char *value)
{
    int argc;
    char **argv, *error1, *error2, *error3, *error4;
    long number1, number2, number3, number4;
    struct t_gui_layout_window *parent;

    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    if (option_name && value && value[0])
    {
        if (string_strcasecmp (option_name, "buffer") == 0)
        {
            argv = string_split (value, ";", 0, 0, &argc);
            if (argv)
            {
                if (argc >= 3)
                {
                    error1 = NULL;
                    number1 = strtol (argv[2], &error1, 10);
                    if (error1 && !error1[0])
                    {
                        gui_layout_buffer_add (&gui_layout_buffers,
                                               &last_gui_layout_buffer,
                                               argv[0], argv[1], number1);
                    }
                }
                string_free_split (argv);
            }
        }
        else if (string_strcasecmp (option_name, "window") == 0)
        {
            argv = string_split (value, ";", 0, 0, &argc);
            if (argv)
            {
                if (argc >= 6)
                {
                    error1 = NULL;
                    number1 = strtol (argv[0], &error1, 10);
                    error2 = NULL;
                    number2 = strtol (argv[1], &error2, 10);
                    error3 = NULL;
                    number3 = strtol (argv[2], &error3, 10);
                    error4 = NULL;
                    number4 = strtol (argv[3], &error4, 10);
                    if (error1 && !error1[0] && error2 && !error2[0]
                        && error3 && !error3[0] && error4 && !error4[0])
                    {
                        parent = gui_layout_window_search_by_id (gui_layout_windows,
                                                                 number2);
                        gui_layout_window_add (&gui_layout_windows,
                                               number1,
                                               parent,
                                               number3,
                                               number4,
                                               (strcmp (argv[4], "-") != 0) ?
                                               argv[4] : NULL,
                                               (strcmp (argv[4], "-") != 0) ?
                                               argv[5] : NULL);
                    }
                }
                string_free_split (argv);
            }
        }
    }

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Writes layout of windows in WeeChat configuration file.
 */

int
config_weechat_layout_write_tree (struct t_config_file *config_file,
                                  struct t_gui_layout_window *layout_window)
{
    if (!config_file_write_line (config_file, "window", "\"%d;%d;%d;%d;%s;%s\"",
                                 layout_window->internal_id,
                                 (layout_window->parent_node) ?
                                 layout_window->parent_node->internal_id : 0,
                                 layout_window->split_pct,
                                 layout_window->split_horiz,
                                 (layout_window->plugin_name) ?
                                 layout_window->plugin_name : "-",
                                 (layout_window->buffer_name) ?
                                 layout_window->buffer_name : "-"))
        return WEECHAT_CONFIG_WRITE_ERROR;

    if (layout_window->child1)
    {
        if (config_weechat_layout_write_tree (config_file,
                                              layout_window->child1) != WEECHAT_CONFIG_WRITE_OK)
            return WEECHAT_CONFIG_WRITE_ERROR;
    }

    if (layout_window->child2)
    {
        if (config_weechat_layout_write_tree (config_file,
                                              layout_window->child2) != WEECHAT_CONFIG_WRITE_OK)
            return WEECHAT_CONFIG_WRITE_ERROR;
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Writes section "layout" in WeeChat configuration file.
 */

int
config_weechat_layout_write_cb (void *data, struct t_config_file *config_file,
                                const char *section_name)
{
    struct t_gui_layout_buffer *ptr_layout_buffer;

    /* make C compiler happy */
    (void) data;

    if (!config_file_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    for (ptr_layout_buffer = gui_layout_buffers; ptr_layout_buffer;
         ptr_layout_buffer = ptr_layout_buffer->next_layout)
    {
        if (!config_file_write_line (config_file, "buffer", "\"%s;%s;%d\"",
                                     ptr_layout_buffer->plugin_name,
                                     ptr_layout_buffer->buffer_name,
                                     ptr_layout_buffer->number))
            return WEECHAT_CONFIG_WRITE_ERROR;
    }

    if (gui_layout_windows)
    {
        if (config_weechat_layout_write_tree (config_file,
                                              gui_layout_windows) != WEECHAT_CONFIG_WRITE_OK)
            return WEECHAT_CONFIG_WRITE_ERROR;
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Callback for changes on a notify option.
 */

void
config_weechat_notify_change_cb (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    gui_buffer_notify_set_all ();
}

/*
 * Callback called when an option is created in section "notify".
 */

int
config_weechat_notify_create_option_cb (void *data,
                                        struct t_config_file *config_file,
                                        struct t_config_section *section,
                                        const char *option_name,
                                        const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = config_file_search_option (config_file, section,
                                                option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = config_file_option_set (ptr_option, value, 1);
            else
            {
                config_file_option_free (ptr_option);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = config_file_new_option (
                    config_file, section,
                    option_name, "integer", _("Notify level for buffer"),
                    "none|highlight|message|all",
                    0, 0, "", value, 0, NULL, NULL,
                    &config_weechat_notify_change_cb, NULL,
                    NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
        gui_buffer_notify_set_all ();

    return rc;
}

/*
 * Callback called when an option is deleted in section "notify".
 */

int
config_weechat_notify_delete_option_cb (void *data,
                                        struct t_config_file *config_file,
                                        struct t_config_section *section,
                                        struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    config_file_option_free (option);

    gui_buffer_notify_set_all ();

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Sets a notify level for a buffer.
 *
 * A negative value resets notify for buffer to its default value (and
 * removes buffer from config file).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
config_weechat_notify_set (struct t_gui_buffer *buffer, const char *notify)
{
    int i, value;

    if (!buffer || !notify)
        return 0;

    value = -1;
    for (i = 0; i < GUI_BUFFER_NUM_NOTIFY; i++)
    {
        if (strcmp (gui_buffer_notify_string[i], notify) == 0)
        {
            value = i;
            break;
        }
    }
    if ((value < 0) && (strcmp (notify, "reset") != 0))
        return 0;

    /* create/update option */
    config_weechat_notify_create_option_cb (NULL,
                                            weechat_config_file,
                                            weechat_config_section_notify,
                                            buffer->full_name,
                                            (value < 0) ?
                                            NULL : gui_buffer_notify_string[value]);
    return 1;
}

/*
 * Reads a filter option in WeeChat configuration file.
 */

int
config_weechat_filter_read_cb (void *data,
                               struct t_config_file *config_file,
                               struct t_config_section *section,
                               const char *option_name, const char *value)
{
    char **argv, **argv_eol;
    int argc;

    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    if (option_name && value && value[0])
    {
        argv = string_split (value, ";", 0, 0, &argc);
        argv_eol = string_split (value, ";", 1, 0, NULL);
        if (argv && argv_eol && (argc >= 4))
        {
            gui_filter_new ((string_strcasecmp (argv[0], "on") == 0) ? 1 : 0,
                            option_name, argv[1], argv[2], argv_eol[3]);
        }
        if (argv)
            string_free_split (argv);
        if (argv_eol)
            string_free_split (argv_eol);
    }

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Writes a filter option in WeeChat configuration file.
 */

int
config_weechat_filter_write_cb (void *data, struct t_config_file *config_file,
                                const char *section_name)
{
    struct t_gui_filter *ptr_filter;

    /* make C compiler happy */
    (void) data;

    if (!config_file_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (!config_file_write_line (config_file,
                                     ptr_filter->name,
                                     "%s;%s;%s;%s",
                                     (ptr_filter->enabled) ? "on" : "off",
                                     ptr_filter->buffer_name,
                                     ptr_filter->tags,
                                     ptr_filter->regex))
            return WEECHAT_CONFIG_WRITE_ERROR;
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Reads a key option in WeeChat configuration file.
 */

int
config_weechat_key_read_cb (void *data, struct t_config_file *config_file,
                            struct t_config_section *section,
                            const char *option_name, const char *value)
{
    int context;
    char *pos;

    /* make C compiler happy */
    (void) data;
    (void) config_file;

    if (option_name)
    {
        context = GUI_KEY_CONTEXT_DEFAULT;
        pos = strchr (section->name, '_');
        if (pos)
        {
            context = gui_key_search_context (pos + 1);
            if (context < 0)
                context = GUI_KEY_CONTEXT_DEFAULT;
        }

        if (value && value[0])
        {
            /* bind key (overwrite any binding with same key) */
            gui_key_bind (NULL, context, option_name, value);
        }
        else
        {
            /* unbind key if no value given */
            gui_key_unbind (NULL, context, option_name);
        }
    }

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Writes section "key" in WeeChat configuration file.
 */

int
config_weechat_key_write_cb (void *data, struct t_config_file *config_file,
                             const char *section_name)
{
    struct t_gui_key *ptr_key;
    char *pos, *expanded_name;
    int rc, context;

    /* make C compiler happy */
    (void) data;

    if (!config_file_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    context = GUI_KEY_CONTEXT_DEFAULT;
    pos = strchr (section_name, '_');
    if (pos)
    {
        context = gui_key_search_context (pos + 1);
        if (context < 0)
            context = GUI_KEY_CONTEXT_DEFAULT;
    }
    for (ptr_key = gui_keys[context]; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_key_get_expanded_name (ptr_key->key);
        if (expanded_name)
        {
            rc = config_file_write_line (config_file,
                                         (expanded_name) ?
                                         expanded_name : ptr_key->key,
                                         "\"%s\"",
                                         ptr_key->command);
            free (expanded_name);
            if (!rc)
                return WEECHAT_CONFIG_WRITE_ERROR;
        }
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Creates options in WeeChat configuration.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
config_weechat_init_options ()
{
    struct t_config_section *ptr_section;
    int i;
    char section_name[128];

    weechat_config_file = config_file_new (NULL, WEECHAT_CONFIG_NAME,
                                           &config_weechat_reload_cb, NULL);
    if (!weechat_config_file)
        return 0;

    /* debug */
    ptr_section = config_file_new_section (weechat_config_file, "debug",
                                           1, 1,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL,
                                           &config_weechat_debug_create_option_cb, NULL,
                                           &config_weechat_debug_delete_option_cb, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    weechat_config_section_debug = ptr_section;

    /* startup */
    ptr_section = config_file_new_section (weechat_config_file, "startup",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    config_startup_command_after_plugins = config_file_new_option (
        weechat_config_file, ptr_section,
        "command_after_plugins", "string",
        N_("command executed when WeeChat starts, after loading plugins"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_startup_command_before_plugins = config_file_new_option (
        weechat_config_file, ptr_section,
        "command_before_plugins", "string",
        N_("command executed when WeeChat starts, before loading plugins"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_startup_display_logo = config_file_new_option (
        weechat_config_file, ptr_section,
        "display_logo", "boolean",
        N_("display WeeChat logo at startup"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_startup_display_version = config_file_new_option (
        weechat_config_file, ptr_section,
        "display_version", "boolean",
        N_("display WeeChat version at startup"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_startup_sys_rlimit = config_file_new_option (
        weechat_config_file, ptr_section,
        "sys_rlimit", "string",
        N_("set resource limits for WeeChat process, format is: "
           "\"res1:limit1,res2:limit2\"; resource name is the end of constant "
           "(RLIMIT_XXX) in lower case (see man setrlimit for values); limit "
           "-1 means \"unlimited\"; example: set unlimited size for core file "
           "and max 1GB of virtual memory: \"core:-1,as:1000000000\""),
        NULL, 0, 0, "", NULL, 0, NULL, NULL,
        &config_change_sys_rlimit, NULL, NULL, NULL);

    /* look */
    ptr_section = config_file_new_section (weechat_config_file, "look",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    config_look_align_end_of_lines = config_file_new_option (
        weechat_config_file, ptr_section,
        "align_end_of_lines", "integer",
        N_("alignment for end of lines (all lines after the first): they "
           "are starting under this data (time, buffer, prefix, suffix, "
           "message (default))"),
        "time|buffer|prefix|suffix|message", 0, 0, "message", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_bar_more_left = config_file_new_option (
        weechat_config_file, ptr_section,
        "bar_more_left", "string",
        N_("string displayed when bar can be scrolled to the left "
           "(for bars with filling \"horizontal\")"),
        NULL, 0, 0, "<<", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_bar_more_right = config_file_new_option (
        weechat_config_file, ptr_section,
        "bar_more_right", "string",
        N_("string displayed when bar can be scrolled to the right "
           "(for bars with filling \"horizontal\")"),
        NULL, 0, 0, ">>", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_bar_more_up = config_file_new_option (
        weechat_config_file, ptr_section,
        "bar_more_up", "string",
        N_("string displayed when bar can be scrolled up "
           "(for bars with filling different from \"horizontal\")"),
        NULL, 0, 0, "--", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_bar_more_down = config_file_new_option (
        weechat_config_file, ptr_section,
        "bar_more_down", "string",
        N_("string displayed when bar can be scrolled down "
           "(for bars with filling different from \"horizontal\")"),
        NULL, 0, 0, "++", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_buffer_notify_default = config_file_new_option (
        weechat_config_file, ptr_section,
        "buffer_notify_default", "integer",
        N_("default notify level for buffers (used to tell WeeChat if buffer "
           "must be displayed in hotlist or not, according to importance "
           "of message): all=all messages (default), "
           "message=messages+highlights, highlight=highlights only, "
           "none=never display in hotlist"),
        "none|highlight|message|all", 0, 0, "all", NULL, 0,
        NULL, NULL, &config_change_buffer_notify_default, NULL, NULL, NULL);
    config_look_buffer_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "buffer_time_format", "string",
        N_("time format for each line displayed in buffers (see man strftime "
           "for date/time specifiers), colors are allowed with format "
           "\"${color}\", for example french time: "
           "\"${lightblue}%H${white}%M${lightred}%S\""),
        NULL, 0, 0, "%H:%M:%S", NULL, 0, NULL, NULL, &config_change_buffer_time_format, NULL, NULL, NULL);
    config_look_color_basic_force_bold = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_basic_force_bold", "boolean",
        N_("force \"bold\" attribute for light colors and \"darkgray\" in "
           "basic colors (this option is disabled by default: bold is used "
           "only if terminal has less than 16 colors)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_look_color_inactive_window = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_inactive_window", "boolean",
        N_("use a different color for lines in inactive window (when window "
           "is not current window)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_color_inactive_buffer = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_inactive_buffer", "boolean",
        N_("use a different color for lines in inactive buffer (when line is "
           "from a merged buffer not selected)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_color_inactive_time = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_inactive_time", "boolean",
        N_("use a different color for inactive time (when window is not "
           "current window, or if line is from a merged buffer not selected)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_color_inactive_prefix_buffer = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_inactive_prefix_buffer", "boolean",
        N_("use a different color for inactive buffer name in prefix (when "
           "window is not current window, or if line is from a merged buffer "
           "not selected)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_color_inactive_prefix = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_inactive_prefix", "boolean",
        N_("use a different color for inactive prefix (when window is not "
           "current window, or if line is from a merged buffer not selected)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_color_inactive_message = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_inactive_message", "boolean",
        N_("use a different color for inactive message (when window is not "
           "current window, or if line is from a merged buffer not selected)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_color_nick_offline = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_nick_offline", "boolean",
        N_("use a different color for offline nicks (not in nicklist any more)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_color_pairs_auto_reset = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_pairs_auto_reset", "integer",
        N_("automatically reset table of color pairs when number of available "
           "pairs is lower or equal to this number (-1 = disable automatic "
           "reset, and then a manual \"/color reset\" is needed when table "
            "is full)"),
        NULL, -1, 256, "5", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_color_real_white = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_real_white", "boolean",
        N_("if set, uses real white color, disabled by default "
           "for terms with white background (if you never use "
           "white background, you should turn on this option to "
           "see real white instead of default term foreground "
           "color)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_look_command_chars = config_file_new_option (
        weechat_config_file, ptr_section,
        "command_chars", "string",
        N_("chars used to determine if input string is a command or not: "
           "input must start with one of these chars; the slash (\"/\") is "
           "always considered as command prefix (example: \".$\")"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_confirm_quit = config_file_new_option (
        weechat_config_file, ptr_section,
        "confirm_quit", "boolean",
        N_("if set, /quit command must be confirmed with extra argument "
           "\"-yes\" (see /help quit)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_day_change = config_file_new_option (
        weechat_config_file, ptr_section,
        "day_change", "boolean",
        N_("display special message when day changes"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_day_change_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "day_change_time_format", "string",
        N_("time format for date displayed when day changed"),
        NULL, 0, 0, "%a, %d %b %Y", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_eat_newline_glitch = config_file_new_option (
        weechat_config_file, ptr_section,
        "eat_newline_glitch", "boolean",
        N_("if set, the eat_newline_glitch will be set to 0; this is used to "
           "not add new line char at end of each line, and then not break "
           "text when you copy/paste text from WeeChat to another application "
           "(this option is disabled by default because it can cause serious "
           "display bugs)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL,
        &config_change_eat_newline_glitch, NULL, NULL, NULL);
    config_look_highlight = config_file_new_option (
        weechat_config_file, ptr_section,
        "highlight", "string",
        N_("comma separated list of words to highlight; case insensitive "
           "comparison (use \"(?-i)\" at beginning of words to make them case "
           "sensitive), words may begin or end with \"*\" for partial match; "
           "example: \"test,(?-i)*toto*,flash*\""),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_highlight_regex = config_file_new_option (
        weechat_config_file, ptr_section,
        "highlight_regex", "string",
        N_("regular expression used to check if a message has highlight or not, "
           "at least one match in string must be surrounded by word chars "
            "(alphanumeric, \"-\", \"_\" or \"|\"), regular expression is case "
           "insensitive (use \"(?-i)\" at beginning to make it case sensitive), "
           "examples: \"flashcode|flashy\", \"(?-i)FlashCode|flashy\""),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, &config_change_highlight_regex, NULL, NULL, NULL);
    config_look_highlight_tags = config_file_new_option (
        weechat_config_file, ptr_section,
        "highlight_tags", "string",
        N_("comma separated list of tags to highlight (case insensitive "
           "comparison, examples: \"irc_notice\" for IRC notices, "
           "\"nick_flashcode\" for messages from nick \"FlashCode\")"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, &config_change_highlight_tags, NULL, NULL, NULL);
    config_look_hotlist_add_buffer_if_away = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_add_buffer_if_away", "boolean",
        N_("add any buffer to hotlist (including current or visible buffers) "
           "if local variable \"away\" is set on buffer"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_hotlist_buffer_separator = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_buffer_separator", "string",
        N_("string displayed between buffers in hotlist"),
        NULL, 0, 0, ", ", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_count_max = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_count_max", "integer",
        N_("max number of messages count to display in hotlist for a buffer "
           "(0 = never display messages count)"),
        NULL, 0, GUI_HOTLIST_NUM_PRIORITIES, "2", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_count_min_msg = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_count_min_msg", "integer",
        N_("display messages count if number of messages is greater or equal "
           "to this value"),
        NULL, 1, 100, "2", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_names_count = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_names_count", "integer",
        N_("max number of names in hotlist (0 = no name "
           "displayed, only buffer numbers)"),
        NULL, 0, 32, "3", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_names_length = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_names_length", "integer",
        N_("max length of names in hotlist (0 = no limit)"),
        NULL, 0, 32, "0", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_names_level = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_names_level", "integer",
        N_("level for displaying names in hotlist (combination "
           "of: 1=join/part, 2=message, 4=private, 8=highlight, "
           "for example: 12=private+highlight)"),
        NULL, 1, 15, "12", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_names_merged_buffers = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_names_merged_buffers", "boolean",
        N_("if set, force display of names in hotlist for merged buffers"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_short_names = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_short_names", "boolean",
        N_("if set, uses short names to display buffer names in hotlist (start "
           "after first '.' in name)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_sort = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_sort", "integer",
        N_("hotlist sort type (group_time_asc (default), "
           "group_time_desc, group_number_asc, group_number_desc, "
           "number_asc, number_desc)"),
        "group_time_asc|group_time_desc|group_number_asc|"
        "group_number_desc|number_asc|number_desc",
        0, 0, "group_time_asc", NULL, 0, NULL, NULL, &config_change_hotlist_sort, NULL, NULL, NULL);
    config_look_hotlist_unique_numbers = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_unique_numbers", "boolean",
        N_("keep only unique numbers in hotlist (this applies only on hotlist "
           "items where name is NOT displayed after number)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_input_cursor_scroll = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_cursor_scroll", "integer",
        N_("number of chars displayed after end of input line when scrolling "
           "to display end of line"),
        NULL, 0, 100, "20", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_input_share = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_share", "integer",
        N_("share commands, text, or both in input for all buffers (there is "
           "still local history for each buffer)"),
        "none|commands|text|all",
        0, 0, "none", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_input_share_overwrite = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_share_overwrite", "boolean",
        N_("if set and input is shared, always overwrite input in target "
           "buffer"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_input_undo_max = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_undo_max", "integer",
        N_("max number of \"undo\" for command line, by buffer (0 = undo "
           "disabled)"),
        NULL, 0, 65535, "32", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_item_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "item_time_format", "string",
        N_("time format for \"time\" bar item (see man strftime for date/time "
           "specifiers)"),
        NULL, 0, 0, "%H:%M", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_item_buffer_filter = config_file_new_option (
        weechat_config_file, ptr_section,
        "item_buffer_filter", "string",
        N_("string used to show that some lines are filtered in current buffer "
           "(bar item \"buffer_filter\")"),
        NULL, 0, 0, "*", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_jump_current_to_previous_buffer = config_file_new_option (
        weechat_config_file, ptr_section,
        "jump_current_to_previous_buffer", "boolean",
        N_("jump to previous buffer displayed when jumping to current buffer "
           "number with /buffer *N (where N is a buffer number), to easily "
           "switch to another buffer, then come back to current buffer"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_jump_previous_buffer_when_closing = config_file_new_option (
        weechat_config_file, ptr_section,
        "jump_previous_buffer_when_closing", "boolean",
        N_("jump to previously visited buffer when closing a buffer (if "
           "disabled, then jump to buffer number - 1)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_jump_smart_back_to_buffer = config_file_new_option (
        weechat_config_file, ptr_section,
        "jump_smart_back_to_buffer", "boolean",
        N_("jump back to initial buffer after reaching end of hotlist"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_mouse = config_file_new_option (
        weechat_config_file, ptr_section,
        "mouse", "boolean",
        N_("enable mouse support"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_mouse, NULL, NULL, NULL);
    config_look_mouse_timer_delay = config_file_new_option (
        weechat_config_file, ptr_section,
        "mouse_timer_delay", "integer",
        N_("delay (in milliseconds) to grab a mouse event: WeeChat will "
           "wait this delay before processing event"),
        NULL, 1, 10000, "100", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_paste_bracketed = config_file_new_option (
        weechat_config_file, ptr_section,
        "paste_bracketed", "boolean",
        N_("enable terminal \"bracketed paste mode\" (not supported in all "
           "terminals/multiplexers): in this mode, pasted text is bracketed "
           "with control sequences so that WeeChat can differentiate pasted "
           "text from typed-in text (\"ESC[200~\", followed by the pasted text, "
           "followed by \"ESC[201~\")"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_paste_bracketed, NULL, NULL, NULL);
    config_look_paste_bracketed_timer_delay = config_file_new_option (
        weechat_config_file, ptr_section,
        "paste_bracketed_timer_delay", "integer",
        N_("force end of bracketed paste after this delay (in seconds) if the "
           "control sequence for end of bracketed paste (\"ESC[201~\") was not "
           "received in time"),
        NULL, 1, 60, "10", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_paste_max_lines = config_file_new_option (
        weechat_config_file, ptr_section,
        "paste_max_lines", "integer",
        N_("max number of lines for paste without asking user "
           "(-1 = disable this feature)"),
        NULL, -1, INT_MAX, "1", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_ERROR] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_error", "string",
        N_("prefix for error messages, colors are allowed with format "
           "\"${color}\""),
        NULL, 0, 0, "=!=", NULL, 0, NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_NETWORK] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_network", "string",
        N_("prefix for network messages, colors are allowed with format "
           "\"${color}\""),
        NULL, 0, 0, "--", NULL, 0, NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_ACTION] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_action", "string",
        N_("prefix for action messages, colors are allowed with format "
           "\"${color}\""),
        NULL, 0, 0, " *", NULL, 0, NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_JOIN] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_join", "string",
        N_("prefix for join messages, colors are allowed with format "
           "\"${color}\""),
        NULL, 0, 0, "-->", NULL, 0, NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_QUIT] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_quit", "string",
        N_("prefix for quit messages, colors are allowed with format "
           "\"${color}\""),
        NULL, 0, 0, "<--", NULL, 0, NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix_align = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_align", "integer",
        N_("prefix alignment (none, left, right (default))"),
        "none|left|right", 0, 0, "right", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_prefix_align_max = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_align_max", "integer",
        N_("max size for prefix (0 = no max size)"),
        NULL, 0, 128, "0", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_prefix_align_min = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_align_min", "integer",
        N_("min size for prefix"),
        NULL, 0, 128, "0", NULL, 0, NULL, NULL, &config_change_prefix_align_min, NULL, NULL, NULL);
    config_look_prefix_align_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_align_more", "string",
        N_("char to display if prefix is truncated (must be exactly one char "
           "on screen)"),
        NULL, 0, 0, "+", NULL, 0,
        &config_check_prefix_align_more, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_prefix_buffer_align = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_buffer_align", "integer",
        N_("prefix alignment for buffer name, when many buffers are merged "
           "with same number (none, left, right (default))"),
        "none|left|right", 0, 0, "right", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_prefix_buffer_align_max = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_buffer_align_max", "integer",
        N_("max size for buffer name, when many buffers are merged with same "
           "number (0 = no max size)"),
        NULL, 0, 128, "0", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_prefix_buffer_align_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_buffer_align_more", "string",
        N_("char to display if buffer name is truncated (when many buffers are "
           "merged with same number) (must be exactly one char on screen)"),
        NULL, 0, 0, "+", NULL, 0,
        &config_check_prefix_buffer_align_more, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_prefix_same_nick = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_same_nick", "string",
        N_("prefix displayed for a message with same nick as previous "
           "message: use a space \" \" to hide prefix, another string to "
           "display this string instead of prefix, or an empty string to "
           "disable feature (display prefix)"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, &config_change_prefix_same_nick, NULL, NULL, NULL);
    config_look_prefix_suffix = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_suffix", "string",
        N_("string displayed after prefix"),
        NULL, 0, 0, "|", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_read_marker = config_file_new_option (
        weechat_config_file, ptr_section,
        "read_marker", "integer",
        N_("use a marker (line or char) on buffers to show first unread line"),
        "none|line|char",
        0, 0, "line", NULL, 0, NULL, NULL, &config_change_read_marker, NULL, NULL, NULL);
    config_look_read_marker_always_show = config_file_new_option (
        weechat_config_file, ptr_section,
        "read_marker_always_show", "boolean",
        N_("always show read marker, even if it is after last buffer line"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_read_marker_string = config_file_new_option (
        weechat_config_file, ptr_section,
        "read_marker_string", "string",
        N_("string used to draw read marker line (string is repeated until "
           "end of line)"),
        NULL, 0, 0, "- ", NULL, 0, NULL, NULL, &config_change_read_marker, NULL, NULL, NULL);
    config_look_save_config_on_exit = config_file_new_option (
        weechat_config_file, ptr_section,
        "save_config_on_exit", "boolean",
        N_("save configuration file on exit"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_save_config_on_exit, NULL, NULL, NULL);
    config_look_save_layout_on_exit = config_file_new_option (
        weechat_config_file, ptr_section,
        "save_layout_on_exit", "integer",
        N_("save layout on exit (buffers, windows, or both)"),
        "none|buffers|windows|all", 0, 0, "none", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_scroll_amount = config_file_new_option (
        weechat_config_file, ptr_section,
        "scroll_amount", "integer",
        N_("how many lines to scroll by with scroll_up and "
           "scroll_down"),
        NULL, 1, INT_MAX, "3", NULL, 0, NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_scroll_bottom_after_switch = config_file_new_option (
        weechat_config_file, ptr_section,
        "scroll_bottom_after_switch", "boolean",
        N_("scroll to bottom of window after switch to another buffer (do not "
           "remember scroll position in windows); the scroll is done only for "
            "buffers with formatted content (not free content)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_scroll_page_percent = config_file_new_option (
        weechat_config_file, ptr_section,
        "scroll_page_percent", "integer",
        N_("percent of screen to scroll when scrolling one page up or down "
           "(for example 100 means one page, 50 half-page)"),
        NULL, 1, 100, "100", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_search_text_not_found_alert = config_file_new_option (
        weechat_config_file, ptr_section,
        "search_text_not_found_alert", "boolean",
        N_("alert user when text sought is not found in buffer"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_separator_horizontal = config_file_new_option (
        weechat_config_file, ptr_section,
        "separator_horizontal", "string",
        N_("char used to draw horizontal separators around bars and windows "
           "(empty value will draw a real line with ncurses, but may cause bugs "
           "with URL selection under some terminals), wide chars are NOT "
           "allowed here"),
        NULL, 0, 0, "-", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_separator_vertical = config_file_new_option (
        weechat_config_file, ptr_section,
        "separator_vertical", "string",
        N_("char used to draw vertical separators around bars and windows "
           "(empty value will draw a real line with ncurses), wide chars are "
           "NOT allowed here"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_set_title = config_file_new_option (
        weechat_config_file, ptr_section,
        "set_title", "boolean",
        N_("set title for window (terminal for Curses GUI) with "
           "name and version"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_title, NULL, NULL, NULL);
    config_look_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "time_format", "string",
        N_("time format for dates converted to strings and displayed in "
           "messages"),
        NULL, 0, 0, "%a, %d %b %Y %T", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_window_separator_horizontal = config_file_new_option (
        weechat_config_file, ptr_section,
        "window_separator_horizontal", "boolean",
        N_("display an horizontal separator between windows"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_window_separator_vertical = config_file_new_option (
        weechat_config_file, ptr_section,
        "window_separator_vertical", "boolean",
        N_("display a vertical separator between windows"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);

    /* palette */
    ptr_section = config_file_new_section (weechat_config_file, "palette",
                                           1, 1,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL,
                                           &config_weechat_palette_create_option_cb, NULL,
                                           &config_weechat_palette_delete_option_cb, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    /* colors */
    ptr_section = config_file_new_section (weechat_config_file, "color",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    weechat_config_section_color = ptr_section;

    /* general color settings */
    config_color_separator = config_file_new_option (
        weechat_config_file, ptr_section,
        "separator", "color",
        N_("background color for window separators (when split)"),
        NULL, GUI_COLOR_SEPARATOR, 0, "blue", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* bar colors */
    config_color_bar_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "bar_more", "color",
        N_("text color for '+' when scrolling bars"),
        NULL, -1, 0, "lightmagenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* chat area */
    config_color_chat = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat", "color",
        N_("text color for chat"),
        NULL, GUI_COLOR_CHAT, 0, "default", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_bg", "color",
        N_("background color for chat"),
        NULL, -1, 0, "default", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_inactive_window = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_inactive_window", "color",
        N_("text color for chat when window is inactive (not current selected "
           "window)"),
        NULL, GUI_COLOR_CHAT_INACTIVE_WINDOW, 0, "darkgray", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_inactive_buffer = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_inactive_buffer", "color",
        N_("text color for chat when line is inactive (buffer is merged with "
           "other buffers and is not selected)"),
        NULL, GUI_COLOR_CHAT_INACTIVE_BUFFER, 0, "darkgray", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_time = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_time", "color",
        N_("text color for time in chat window"),
        NULL, GUI_COLOR_CHAT_TIME, 0, "default", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_time_delimiters = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_time_delimiters", "color",
        N_("text color for time delimiters"),
        NULL, GUI_COLOR_CHAT_TIME_DELIMITERS, 0, "brown", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix_buffer = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_buffer", "color",
        N_("text color for buffer name (before prefix, when many buffers are "
           "merged with same number)"),
        NULL, GUI_COLOR_CHAT_PREFIX_BUFFER, 0, "brown", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix_buffer_inactive_buffer = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_buffer_inactive_buffer", "color",
        N_("text color for inactive buffer name (before prefix, when many "
           "buffers are merged with same number and if buffer is not "
            "selected)"),
        NULL, GUI_COLOR_CHAT_PREFIX_BUFFER_INACTIVE_BUFFER, 0, "darkgray", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix[GUI_CHAT_PREFIX_ERROR] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_error", "color",
        N_("text color for error prefix"),
        NULL, GUI_COLOR_CHAT_PREFIX_ERROR, 0, "yellow", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix[GUI_CHAT_PREFIX_NETWORK] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_network", "color",
        N_("text color for network prefix"),
        NULL, GUI_COLOR_CHAT_PREFIX_NETWORK, 0, "magenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix[GUI_CHAT_PREFIX_ACTION] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_action", "color",
        N_("text color for action prefix"),
        NULL, GUI_COLOR_CHAT_PREFIX_ACTION, 0, "white", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix[GUI_CHAT_PREFIX_JOIN] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_join", "color",
        N_("text color for join prefix"),
        NULL, GUI_COLOR_CHAT_PREFIX_JOIN, 0, "lightgreen", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix[GUI_CHAT_PREFIX_QUIT] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_quit", "color",
        N_("text color for quit prefix"),
        NULL, GUI_COLOR_CHAT_PREFIX_QUIT, 0, "lightred", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_more", "color",
        N_("text color for '+' when prefix is too long"),
        NULL, GUI_COLOR_CHAT_PREFIX_MORE, 0, "lightmagenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix_suffix = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_suffix", "color",
        N_("text color for suffix (after prefix)"),
        NULL, GUI_COLOR_CHAT_PREFIX_SUFFIX, 0, "green", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_buffer = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_buffer", "color",
        N_("text color for buffer names"),
        NULL, GUI_COLOR_CHAT_BUFFER, 0, "white", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_server = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_server", "color",
        N_("text color for server names"),
        NULL, GUI_COLOR_CHAT_SERVER, 0, "brown", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_channel = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_channel", "color",
        N_("text color for channel names"),
        NULL, GUI_COLOR_CHAT_CHANNEL, 0, "white", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick", "color",
        N_("text color for nicks in chat window"),
        NULL, GUI_COLOR_CHAT_NICK, 0, "lightcyan", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_colors", "string",
        /* TRANSLATORS: please do not translate "lightred:blue" */
        N_("text color for nicks (comma separated list of colors, background "
           "is allowed with format: \"fg:bg\", for example: "
           "\"lightred:blue\")"),
        NULL, 0, 0, "cyan,magenta,green,brown,lightblue,default,lightcyan,"
        "lightmagenta,lightgreen,blue", NULL, 0,
        NULL, NULL, &config_change_nick_colors, NULL, NULL, NULL);
    config_color_chat_nick_self = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_self", "color",
        N_("text color for local nick in chat window"),
        NULL, GUI_COLOR_CHAT_NICK_SELF, 0, "white", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_offline = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_offline", "color",
        N_("text color for offline nick (not in nicklist any more)"),
        NULL, GUI_COLOR_CHAT_NICK_OFFLINE, 0, "darkgray", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_offline_highlight = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_offline_highlight", "color",
        N_("text color for offline nick with highlight"),
        NULL, -1, 0, "default", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_offline_highlight_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_offline_highlight_bg", "color",
        N_("background color for offline nick with highlight"),
        NULL, -1, 0, "darkgray", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_other = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_other", "color",
        N_("text color for other nick in private buffer"),
        NULL, GUI_COLOR_CHAT_NICK_OTHER, 0, "cyan", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_host = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_host", "color",
        N_("text color for hostnames"),
        NULL, GUI_COLOR_CHAT_HOST, 0, "cyan", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_delimiters = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_delimiters", "color",
        N_("text color for delimiters"),
        NULL, GUI_COLOR_CHAT_DELIMITERS, 0, "green", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_highlight = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_highlight", "color",
        N_("text color for highlighted prefix"),
        NULL, GUI_COLOR_CHAT_HIGHLIGHT, 0, "yellow", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_highlight_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_highlight_bg", "color",
        N_("background color for highlighted prefix"),
        NULL, -1, 0, "magenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_read_marker = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_read_marker", "color",
        N_("text color for unread data marker"),
        NULL, GUI_COLOR_CHAT_READ_MARKER, 0, "magenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_read_marker_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_read_marker_bg", "color",
        N_("background color for unread data marker"),
        NULL, -1, 0, "default", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_tags = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_tags", "color",
        N_("text color for tags after messages (displayed with command /debug "
           "tags)"),
        NULL, GUI_COLOR_CHAT_TAGS, 0, "red", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_text_found = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_text_found", "color",
        N_("text color for marker on lines where text sought is found"),
        NULL, GUI_COLOR_CHAT_TEXT_FOUND, 0, "yellow", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_text_found_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_text_found_bg", "color",
        N_("background color for marker on lines where text sought is found"),
        NULL, -1, 0, "lightmagenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_value = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_value", "color",
        N_("text color for values"),
        NULL, GUI_COLOR_CHAT_VALUE, 0, "cyan", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* status bar */
    config_color_status_number = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_number", "color",
        N_("text color for current buffer number in status bar"),
        NULL, -1, 0, "yellow", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_name = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_name", "color",
        N_("text color for current buffer name in status bar"),
        NULL, -1, 0, "white", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_name_ssl = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_name_ssl", "color",
        N_("text color for current buffer name in status bar, if data are "
           "secured with a protocol like SSL"),
        NULL, -1, 0, "lightgreen", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_filter = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_filter", "color",
        N_("text color for filter indicator in status bar"),
        NULL, -1, 0, "green", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_data_msg = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_data_msg", "color",
        N_("text color for buffer with new messages (status bar)"),
        NULL, -1, 0, "yellow", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_data_private = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_data_private", "color",
        N_("text color for buffer with private message (status bar)"),
        NULL, -1, 0, "lightgreen", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_data_highlight = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_data_highlight", "color",
        N_("text color for buffer with highlight (status bar)"),
        NULL, -1, 0, "lightmagenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_data_other = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_data_other", "color",
        N_("text color for buffer with new data (not messages) "
           "(status bar)"),
        NULL, -1, 0, "default", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_count_msg = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_count_msg", "color",
        N_("text color for count of messages in hotlist (status bar)"),
        NULL, -1, 0, "brown", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_count_private = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_count_private", "color",
        N_("text color for count of private messages in hotlist (status bar)"),
        NULL, -1, 0, "green", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_count_highlight = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_count_highlight", "color",
        N_("text color for count of highlight messages in hotlist (status bar)"),
        NULL, -1, 0, "magenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_count_other = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_count_other", "color",
        N_("text color for count of other messages in hotlist (status bar)"),
        NULL, -1, 0, "default", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_more", "color",
        N_("text color for buffer with new data (status bar)"),
        NULL, -1, 0, "yellow", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_time = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_time", "color",
        N_("text color for time (status bar)"),
        NULL, -1, 0, "default", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* input bar */
    config_color_input_text_not_found = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_text_not_found", "color",
        N_("text color for unsuccessful text search in input line"),
        NULL, -1, 0, "red", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_input_actions = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_actions", "color",
        N_("text color for actions in input line"),
        NULL, -1, 0, "lightgreen", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* nicklist bar */
    config_color_nicklist_group = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_group", "color",
        N_("text color for groups in nicklist"),
        NULL, -1, 0, "green", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_away = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_away", "color",
        N_("text color for away nicknames"),
        NULL, -1, 0, "cyan", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_offline = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_offline", "color",
        N_("text color for offline nicknames"),
        NULL, -1, 0, "blue", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);

    /* completion */
    ptr_section = config_file_new_section (weechat_config_file, "completion",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    config_completion_base_word_until_cursor = config_file_new_option (
        weechat_config_file, ptr_section,
        "base_word_until_cursor", "boolean",
        N_("if enabled, the base word to complete ends at char before cursor; "
           "otherwise the base word ends at first space after cursor"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_completion_default_template = config_file_new_option (
        weechat_config_file, ptr_section,
        "default_template", "string",
        N_("default completion template (please see documentation for template "
           "codes and values)"),
        NULL, 0, 0, "%(nicks)|%(irc_channels)", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL);
    config_completion_nick_add_space = config_file_new_option (
        weechat_config_file, ptr_section,
        "nick_add_space", "boolean",
        N_("add space after nick completion (when nick is not first word on "
           "command line)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_completion_nick_completer = config_file_new_option (
        weechat_config_file, ptr_section,
        "nick_completer", "string",
        N_("string inserted after nick completion (when nick is first word on "
           "command line)"),
        NULL, 0, 0, ":", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_completion_nick_first_only = config_file_new_option (
        weechat_config_file, ptr_section,
        "nick_first_only", "boolean",
        N_("complete only with first nick found"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_completion_nick_ignore_chars = config_file_new_option (
        weechat_config_file, ptr_section,
        "nick_ignore_chars", "string",
        N_("chars ignored for nick completion"),
        NULL, 0, 0, "[]`_-^", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_completion_partial_completion_alert = config_file_new_option (
        weechat_config_file, ptr_section,
        "partial_completion_alert", "boolean",
        N_("alert user when a partial completion occurs"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_completion_partial_completion_command = config_file_new_option (
        weechat_config_file, ptr_section,
        "partial_completion_command", "boolean",
        N_("partially complete command names (stop when many commands found "
           "begin with same letters)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_completion_partial_completion_command_arg = config_file_new_option (
        weechat_config_file, ptr_section,
        "partial_completion_command_arg", "boolean",
        N_("partially complete command arguments (stop when many arguments "
           "found begin with same prefix)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_completion_partial_completion_other = config_file_new_option (
        weechat_config_file, ptr_section,
        "partial_completion_other", "boolean",
        N_("partially complete outside commands (stop when many words found "
           "begin with same letters)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_completion_partial_completion_count = config_file_new_option (
        weechat_config_file, ptr_section,
        "partial_completion_count", "boolean",
        N_("display count for each partial completion in bar item"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* history */
    ptr_section = config_file_new_section (weechat_config_file, "history",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    config_history_max_buffer_lines_number = config_file_new_option (
        weechat_config_file, ptr_section,
        "max_buffer_lines_number", "integer",
        N_("maximum number of lines in history per buffer "
           "(0 = unlimited)"),
        NULL, 0, INT_MAX, "4096", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_history_max_buffer_lines_minutes = config_file_new_option (
        weechat_config_file, ptr_section,
        "max_buffer_lines_minutes", "integer",
        N_("maximum number of minutes in history per buffer "
           "(0 = unlimited, examples: 1440 = one day, 10080 = one week, "
           "43200 = one month, 525600 = one year)"),
        NULL, 0, INT_MAX, "0", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_history_max_commands = config_file_new_option (
        weechat_config_file, ptr_section,
        "max_commands", "integer",
        N_("maximum number of user commands in history (0 = "
           "unlimited)"),
        NULL, 0, INT_MAX, "100", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_history_max_visited_buffers = config_file_new_option (
        weechat_config_file, ptr_section,
        "max_visited_buffers", "integer",
        N_("maximum number of visited buffers to keep in memory"),
        NULL, 0, 1000, "50", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_history_display_default = config_file_new_option (
        weechat_config_file, ptr_section,
        "display_default", "integer",
        N_("maximum number of commands to display by default in "
           "history listing (0 = unlimited)"),
        NULL, 0, INT_MAX, "5", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* proxies */
    ptr_section = config_file_new_section (weechat_config_file, "proxy",
                                           0, 0,
                                           &config_weechat_proxy_read_cb, NULL,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    weechat_config_section_proxy = ptr_section;

    /* network */
    ptr_section = config_file_new_section (weechat_config_file, "network",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    config_network_connection_timeout = config_file_new_option (
        weechat_config_file, ptr_section,
        "connection_timeout", "integer",
        N_("timeout (in seconds) for connection to a remote host (made in a "
           "child process)"),
        NULL, 1, INT_MAX, "60", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_network_gnutls_ca_file = config_file_new_option (
        weechat_config_file, ptr_section,
        "gnutls_ca_file", "string",
        N_("file containing the certificate authorities (\"%h\" will be "
           "replaced by WeeChat home, \"~/.weechat\" by default)"),
        NULL, 0, 0, "/etc/ssl/certs/ca-certificates.crt", NULL, 0, NULL, NULL,
        &config_change_network_gnutls_ca_file, NULL, NULL, NULL);
    config_network_gnutls_handshake_timeout = config_file_new_option (
        weechat_config_file, ptr_section,
        "gnutls_handshake_timeout", "integer",
        N_("timeout (in seconds) for gnutls handshake"),
        NULL, 1, INT_MAX, "30", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* plugin */
    ptr_section = config_file_new_section (weechat_config_file, "plugin",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    config_plugin_autoload = config_file_new_option (
        weechat_config_file, ptr_section,
        "autoload", "string",
        N_("comma separated list of plugins to load automatically "
           "at startup, \"*\" means all plugins found, a name beginning with "
           "\"!\" is a negative value to prevent a plugin from being loaded, "
           "names can start or end with \"*\" to match several plugins "
           "(examples: \"*\" or \"*,!lua,!tcl\")"),
        NULL, 0, 0, "*", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_plugin_debug = config_file_new_option (
        weechat_config_file, ptr_section,
        "debug", "boolean",
        N_("enable debug messages by default in all plugins (option disabled "
           "by default, which is highly recommended)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_plugin_extension = config_file_new_option (
        weechat_config_file, ptr_section,
        "extension", "string",
        N_("comma separated list of file name extensions for plugins"),
        NULL, 0, 0, ".so,.dll", NULL, 0, NULL, NULL,
        &config_change_plugin_extension, NULL, NULL, NULL);
    config_plugin_path = config_file_new_option (
        weechat_config_file, ptr_section,
        "path", "string",
        N_("path for searching plugins (\"%h\" will be replaced by "
           "WeeChat home, \"~/.weechat\" by default)"),
        NULL, 0, 0, "%h/plugins", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_plugin_save_config_on_unload = config_file_new_option (
        weechat_config_file, ptr_section,
        "save_config_on_unload", "boolean",
        N_("save configuration files when unloading plugins"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* bars */
    ptr_section = config_file_new_section (weechat_config_file, "bar",
                                           0, 0,
                                           &config_weechat_bar_read_cb, NULL,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    weechat_config_section_bar = ptr_section;

    /* layout */
    ptr_section = config_file_new_section (weechat_config_file, "layout",
                                           0, 0,
                                           &config_weechat_layout_read_cb, NULL,
                                           &config_weechat_layout_write_cb, NULL,
                                           NULL, NULL, NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    /* notify */
    ptr_section = config_file_new_section (weechat_config_file, "notify",
                                           1, 1,
                                           NULL, NULL,
                                           NULL, NULL,
                                           NULL, NULL,
                                           &config_weechat_notify_create_option_cb, NULL,
                                           &config_weechat_notify_delete_option_cb, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    weechat_config_section_notify = ptr_section;

    /* filters */
    ptr_section = config_file_new_section (weechat_config_file, "filter",
                                           0, 0,
                                           &config_weechat_filter_read_cb, NULL,
                                           &config_weechat_filter_write_cb, NULL,
                                           &config_weechat_filter_write_cb, NULL,
                                           NULL, NULL, NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }

    /* keys */
    for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
    {
        snprintf (section_name, sizeof (section_name),
                  "key%s%s",
                  (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
                  (i == GUI_KEY_CONTEXT_DEFAULT) ? "" : gui_key_context_string[i]);
        ptr_section = config_file_new_section (weechat_config_file, section_name,
                                               0, 0,
                                               &config_weechat_key_read_cb, NULL,
                                               &config_weechat_key_write_cb, NULL,
                                               &config_weechat_key_write_cb, NULL,
                                               NULL, NULL, NULL, NULL);
        if (!ptr_section)
        {
            config_file_free (weechat_config_file);
            return 0;
        }
    }

    return 1;
}

/*
 * Initializes WeeChat configuration.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
config_weechat_init ()
{
    int rc;
    struct timeval tv_time;
    struct tm *local_time;

    rc = config_weechat_init_options ();

    if (!rc)
    {
        gui_chat_printf (NULL,
                         _("FATAL: error initializing configuration options"));
    }

    if (!config_day_change_timer)
    {
        /* create timer to check if day has changed */
        gettimeofday (&tv_time, NULL);
        local_time = localtime (&tv_time.tv_sec);
        config_day_change_old_day = local_time->tm_mday;
        config_day_change_timer = hook_timer (NULL,
                                              60 * 1000, /* each minute */
                                              60, /* when second is 00 */
                                              0,
                                              &config_day_change_timer_cb,
                                              NULL);
    }
    if (!config_highlight_regex)
        config_change_highlight_regex (NULL, NULL);
    if (!config_highlight_tags)
        config_change_highlight_tags (NULL, NULL);
    if (!config_plugin_extensions)
        config_change_plugin_extension (NULL, NULL);

    return rc;
}

/*
 * Reads WeeChat configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_READ_OK: OK
 *   WEECHAT_CONFIG_READ_MEMORY_ERROR: not enough memory
 *   WEECHAT_CONFIG_READ_FILE_NOT_FOUND: file not found
 */

int
config_weechat_read ()
{
    int rc;

    rc = config_file_read (weechat_config_file);

    config_weechat_init_after_read ();

    if (rc != WEECHAT_CONFIG_READ_OK)
    {
        gui_chat_printf (NULL,
                         _("%sError reading configuration"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    }

    return rc;
}

/*
 * Writes WeeChat configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_WRITE_OK: OK
 *   WEECHAT_CONFIG_WRITE_ERROR: error
 *   WEECHAT_CONFIG_WRITE_MEMORY_ERROR: not enough memory
 */

int
config_weechat_write ()
{
    return config_file_write (weechat_config_file);
}

/*
 * Frees WeeChat configuration file and variables.
 */

void
config_weechat_free ()
{
    config_file_free (weechat_config_file);

    if (config_highlight_regex)
    {
        regfree (config_highlight_regex);
        free (config_highlight_regex);
        config_highlight_regex = NULL;
    }

    if (config_highlight_tags)
    {
        string_free_split (config_highlight_tags);
        config_highlight_tags = NULL;
        config_num_highlight_tags = 0;
    }

    if (config_plugin_extensions)
    {
        string_free_split (config_plugin_extensions);
        config_plugin_extensions = NULL;
        config_num_plugin_extensions = 0;
    }
}
