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
#include "wee-hook.h"
#include "wee-log.h"
#include "wee-util.h"
#include "wee-list.h"
#include "wee-string.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-infobar.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-status.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


struct t_config_file *weechat_config_file = NULL;
struct t_config_section *weechat_config_section_bar = NULL;

/* config, startup section */

struct t_config_option *config_startup_display_logo;
struct t_config_option *config_startup_display_version;
struct t_config_option *config_startup_command_before_plugins;
struct t_config_option *config_startup_command_after_plugins;
struct t_config_option *config_startup_weechat_slogan;

/* config, look & feel section */

struct t_config_option *config_look_color_real_white;
struct t_config_option *config_look_save_on_exit;
struct t_config_option *config_look_set_title;
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
struct t_config_option *config_look_prefix[GUI_CHAT_NUM_PREFIXES];
struct t_config_option *config_look_prefix_align;
struct t_config_option *config_look_prefix_align_max;
struct t_config_option *config_look_prefix_suffix;
struct t_config_option *config_look_nick_completor;
struct t_config_option *config_look_nick_completion_ignore;
struct t_config_option *config_look_nick_complete_first;
struct t_config_option *config_look_infobar;
struct t_config_option *config_look_infobar_time_format;
struct t_config_option *config_look_infobar_seconds;
struct t_config_option *config_look_infobar_delay_highlight;
struct t_config_option *config_look_item_time_format;
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
struct t_config_option *config_color_chat_prefix[GUI_CHAT_NUM_PREFIXES];
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
struct t_config_option *config_color_status_number;
struct t_config_option *config_color_status_category;
struct t_config_option *config_color_status_name;
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
struct t_config_option *config_color_nicklist_group;
struct t_config_option *config_color_nicklist_away;
struct t_config_option *config_color_nicklist_prefix1;
struct t_config_option *config_color_nicklist_prefix2;
struct t_config_option *config_color_nicklist_prefix3;
struct t_config_option *config_color_nicklist_prefix4;
struct t_config_option *config_color_nicklist_prefix5;
struct t_config_option *config_color_nicklist_more;
struct t_config_option *config_color_nicklist_separator;

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

/* config, plugin section */

struct t_config_option *config_plugin_path;
struct t_config_option *config_plugin_autoload;
struct t_config_option *config_plugin_extension;
struct t_config_option *config_plugin_save_config_on_unload;

/* hooks */

struct t_hook *config_day_change_timer = NULL;


/*
 * config_change_save_on_exit: called when "save_on_exit" flag is changed
 */

void
config_change_save_on_exit (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if (!config_look_save_on_exit)
    {
        gui_chat_printf (NULL,
                         _("Warning: you should now issue /save to write "
                           "\"save_on_exit\" option in configuration file"));
    }
}

/*
 * config_change_title: called when title is changed
 */

void
config_change_title (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if (config_look_set_title)
	gui_window_title_set ();
    else
	gui_window_title_reset ();
}

/*
 * config_change_buffers: called when buffers change (for example nicklist)
 */

void
config_change_buffers (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    gui_window_refresh_windows ();
}

/*
 * config_change_buffer_content: called when content of a buffer changes
 */

void
config_change_buffer_content (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if (gui_ok)
        gui_window_redraw_buffer (gui_current_window->buffer);
}

/*
 * config_change_buffer_time_format: called when buffer time format changes
 */

void
config_change_buffer_time_format (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    gui_chat_time_length = util_get_time_length (CONFIG_STRING(config_look_buffer_time_format));
    gui_chat_change_time_format ();
    if (gui_ok)
        gui_window_redraw_buffer (gui_current_window->buffer);
}

/*
 * config_change_hotlist: called when hotlist changes
 */

void
config_change_hotlist (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    gui_hotlist_resort ();
    gui_status_refresh_needed = 1;
}

/*
 * config_change_read_marker: called when read marker is changed
 */

void
config_change_read_marker (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    gui_window_redraw_all_buffers ();
}

/*
 * config_change_prefix: called when a prefix is changed
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
 * config_change_color: called when a color is changed by /set command
 */

void
config_change_color (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
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
config_change_nicks_colors (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    /* TODO: change nicks colors */
    /*
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
                gui_nick_find_color (ptr_nick);
            }   
        }
    }
    */
}

/*
 * config_change_infobar_seconds: called when display of seconds in infobar changed
 */

void
config_change_infobar_seconds (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    int seconds;
    
    if (gui_infobar_refresh_timer)
        unhook (gui_infobar_refresh_timer);
    
    seconds = (CONFIG_BOOLEAN(config_look_infobar_seconds)) ? 1 : 60;
    gui_infobar_refresh_timer = hook_timer (NULL, seconds * 1000, seconds, 0,
                                            gui_infobar_refresh_timer_cb, NULL);
    (void) gui_infobar_refresh_timer_cb ("force");
}

/*
 * config_change_item_time_format: called when time format for time item changed
 */

void
config_change_item_time_format (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    int seconds;
    
    if (gui_infobar_refresh_timer)
        unhook (gui_infobar_refresh_timer);
    
    seconds = (CONFIG_BOOLEAN(config_look_infobar_seconds)) ? 1 : 60;
    gui_infobar_refresh_timer = hook_timer (NULL, seconds * 1000, seconds, 0,
                                            gui_infobar_refresh_timer_cb, NULL);
    (void) gui_infobar_refresh_timer_cb ("force");
}

/*
 * config_day_change_timer_cb: timer callback for displaying
 *                             "Day changed to xxx" message
 */

int
config_day_change_timer_cb (void *data)
{
    struct timeval tv_time;
    struct tm *local_time;
    char text_time[1024], *text_time2;
    struct t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) data;
    
    gettimeofday (&tv_time, NULL);
    local_time = localtime (&tv_time.tv_sec);
    
    strftime (text_time, sizeof (text_time),
              CONFIG_STRING(config_look_day_change_time_format),
              local_time);
    text_time2 = string_iconv_to_internal (NULL, text_time);
    gui_add_hotlist = 0;
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->type == GUI_BUFFER_TYPE_FORMATED)
            gui_chat_printf (ptr_buffer,
                             _("\t\tDay changed to %s"),
                             (text_time2) ?
                             text_time2 : text_time);
    }
    if (text_time2)
        free (text_time2);
    gui_add_hotlist = 1;

    return WEECHAT_RC_OK;
}

/*
 * config_change_day_change: called when day_change option changed
 */

void
config_change_day_change (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if (CONFIG_BOOLEAN(config_look_day_change))
    {
        if (!config_day_change_timer)
            config_day_change_timer = hook_timer (NULL,
                                                  24 * 3600 * 1000,
                                                  24 * 3600,
                                                  0,
                                                  &config_day_change_timer_cb,
                                                  NULL);
    }
    else
    {
        if (config_day_change_timer)
        {
            unhook (config_day_change_timer);
            config_day_change_timer = NULL;
        }
    }
}

/*
 * config_weechat_reload: reload WeeChat configuration file
 *                        return:  0 = successful
 *                                -1 = config file file not found
 *                                -2 = error in config file
 */

int
config_weechat_reload (void *data, struct t_config_file *config_file)
{
    int rc;
    
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    
    /* remove all keys */
    gui_keyboard_free_all (&gui_keys, &last_gui_key);
    
    /* remove all bars */
    gui_bar_free_all ();
    
    /* remove all filters */
    gui_filter_free_all ();
    
    rc = config_file_reload (weechat_config_file);
    
    if (rc == 0)
    {
        gui_bar_use_temp_bars ();
    }
    
    return rc;
}

/*
 * config_weechat_bar_read: read bar option in config file
 *                          return: 1 if ok, 0 if error
 */

int
config_weechat_bar_read (void *data, struct t_config_file *config_file,
                         struct t_config_section *section,
                         char *option_name, char *value)
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
                }
                
                free (bar_name);
            }
        }
    }
    
    return 1;
}

/*
 * config_weechat_filter_read: read filter option from config file
 *                             return 1 if ok, 0 if error
 */

int
config_weechat_filter_read (void *data,
                            struct t_config_file *config_file,
                            struct t_config_section *section,
                            char *option_name, char *value)
{
    char **argv, **argv_eol;
    int argc;
    
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;
    
    if (option_name)
    {
        if (value && value[0])
        {
            argv = string_explode (value, ";", 0, 0, &argc);
            argv_eol = string_explode (value, ";", 1, 0, NULL);
            if (argv && argv_eol && (argc >= 3))
            {
                gui_filter_new (argv[0], argv[1], argv_eol[2]);
            }
            if (argv)
                string_free_exploded (argv);
            if (argv_eol)
                string_free_exploded (argv_eol);
        }
    }
    
    return 1;
}

/*
 * config_weechat_filter_write: write filter section in configuration file
 *                              Return:  0 = successful
 *                                      -1 = write error
 */

void
config_weechat_filter_write (void *data, struct t_config_file *config_file,
                             char *section_name)
{
    struct t_gui_filter *ptr_filter;
    
    /* make C compiler happy */
    (void) data;
    
    config_file_write_line (config_file, section_name, NULL);
    
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        config_file_write_line (config_file,
                                "filter",
                                "%s;%s;%s",
                                ptr_filter->buffer,
                                ptr_filter->tags,
                                ptr_filter->regex);
    }
}

/*
 * config_weechat_key_read: read key option in config file
 *                          return 1 if ok, 0 if error
 */

int
config_weechat_key_read (void *data, struct t_config_file *config_file,
                         struct t_config_section *section,
                         char *option_name, char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;
    
    if (option_name)
    {
        if (value && value[0])
        {
            /* bind key (overwrite any binding with same key) */
            gui_keyboard_bind (NULL, option_name, value);
        }
        else
        {
            /* unbind key if no value given */
            gui_keyboard_unbind (NULL, option_name);
        }
    }
    
    return 1;
}

/*
 * config_weechat_key_write: write key section in configuration file
 *                           Return:  0 = successful
 *                                   -1 = write error
 */

void
config_weechat_key_write (void *data, struct t_config_file *config_file,
                          char *section_name)
{
    struct t_gui_key *ptr_key;
    char *expanded_name;
    
    /* make C compiler happy */
    (void) data;
    
    config_file_write_line (config_file, section_name, NULL);
    
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_keyboard_get_expanded_name (ptr_key->key);
        config_file_write_line (config_file,
                                (expanded_name) ?
                                expanded_name : ptr_key->key,
                                "\"%s\"",
                                ptr_key->command);
        if (expanded_name)
            free (expanded_name);
    }
}

/*
 * config_weechat_init: init WeeChat config structure
 *                      return: 1 if ok, 0 if error
 */

int
config_weechat_init ()
{
    struct t_config_section *ptr_section;
    
    weechat_config_file = config_file_new (NULL, WEECHAT_CONFIG_NAME,
                                           &config_weechat_reload, NULL);
    if (!weechat_config_file)
        return 0;
    
    /* startup */
    ptr_section = config_file_new_section (weechat_config_file, "startup",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }
    
    config_startup_display_logo = config_file_new_option (
        weechat_config_file, ptr_section,
        "display_logo", "boolean",
        N_("display WeeChat logo at startup"),
        NULL, 0, 0, "on", NULL, NULL, NULL, NULL, NULL, NULL);
    config_startup_display_version = config_file_new_option (
        weechat_config_file, ptr_section,
        "display_version", "boolean",
        N_("display WeeChat version at startup"),
        NULL, 0, 0, "on", NULL, NULL, NULL, NULL, NULL, NULL);
    config_startup_command_before_plugins = config_file_new_option (
        weechat_config_file, ptr_section,
        "command_before_plugins", "string",
        N_("command executed when WeeChat starts, before loading plugins"),
        NULL, 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL);
    config_startup_command_after_plugins = config_file_new_option (
        weechat_config_file, ptr_section,
        "command_after_plugins", "string",
        N_("command executed when WeeChat starts, after loading plugins"),
        NULL, 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL);
    config_startup_weechat_slogan = config_file_new_option (
        weechat_config_file, ptr_section,
        "weechat_slogan", "string",
        N_("WeeChat slogan (if empty, slogan is not used)"),
        NULL, 0, 0, "the geekest IRC client!", NULL, NULL, NULL, NULL, NULL, NULL);
    
    /* look */
    ptr_section = config_file_new_section (weechat_config_file, "look",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }
    
    config_look_color_real_white = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_real_white", "boolean",
        N_("if set, uses real white color, disabled by default "
           "for terms with white background (if you never use "
           "white background, you should turn on this option to "
           "see real white instead of default term foreground "
           "color)"),
        NULL, 0, 0, "off", NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_look_save_on_exit = config_file_new_option (
        weechat_config_file, ptr_section,
        "save_on_exit", "boolean",
        N_("save configuration file on exit"),
        NULL, 0, 0, "on", NULL, NULL, &config_change_save_on_exit, NULL, NULL, NULL);
    config_look_set_title = config_file_new_option (
        weechat_config_file, ptr_section,
        "set_title", "boolean",
        N_("set title for window (terminal for Curses GUI) with "
           "name and version"),
        NULL, 0, 0, "on", NULL, NULL, &config_change_title, NULL, NULL, NULL);
    config_look_scroll_amount = config_file_new_option (
        weechat_config_file, ptr_section,
        "scroll_amount", "integer",
        N_("how many lines to scroll by with scroll_up and "
           "scroll_down"),
        NULL, 1, INT_MAX, "3", NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_buffer_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "buffer_time_format", "string",
        N_("time format for buffers"),
        NULL, 0, 0, "[%H:%M:%S]", NULL, NULL, &config_change_buffer_time_format, NULL, NULL, NULL);
    config_look_color_nicks_number = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_nicks_number", "integer",
        N_("number of colors to use for nicks colors"),
        NULL, 1, 10, "10", NULL, NULL, &config_change_nicks_colors, NULL, NULL, NULL);
    config_look_nicklist = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist", "boolean",
        N_("display nicklist (on buffers with nicklist enabled)"),
        NULL, 0, 0, "on", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_nicklist_position = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_position", "integer",
        N_("nicklist position (top, left, right (default), "
           "bottom)"),
        "left|right|top|bottom", 0, 0, "right", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_nicklist_min_size = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_min_size", "integer",
        N_("min size for nicklist (width or height, depending on "
           "nicklist_position (0 = no min size))"),
        NULL, 0, 100, "0", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_nicklist_max_size = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_max_size", "integer",
        N_("max size for nicklist (width or height, depending on "
           "nicklist_position (0 = no max size; if min = max "
           "and > 0, then size is fixed))"),
        NULL, 0, 100, "0", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_nicklist_separator = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_separator", "boolean",
        N_("separator between chat and nicklist"),
        NULL, 0, 0, "on", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_nickmode = config_file_new_option (
        weechat_config_file, ptr_section,
        "nickmode", "boolean",
        N_("display nick mode ((half)op/voice) before each nick"),
        NULL, 0, 0, "on", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_nickmode_empty = config_file_new_option (
        weechat_config_file, ptr_section,
        "nickmode_empty", "boolean",
        N_("display space if nick mode is not (half)op/voice"),
        NULL, 0, 0, "off", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_ERROR] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_error", "string",
        N_("prefix for error messages"),
        NULL, 0, 0, "=!=", NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_NETWORK] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_network", "string",
        N_("prefix for network messages"),
        NULL, 0, 0, "--", NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_ACTION] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_action", "string",
        N_("prefix for action messages"),
        NULL, 0, 0, "*", NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_JOIN] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_join", "string",
        N_("prefix for join messages"),
        NULL, 0, 0, "-->", NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_QUIT] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_quit", "string",
        N_("prefix for quit messages"),
        NULL, 0, 0, "<--", NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix_align = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_align", "integer",
        N_("prefix alignment (none, left, right (default))"),
        "none|left|right", 0, 0, "right", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_prefix_align_max = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_align_max", "integer",
        N_("max size for prefix (0 = no max size)"),
        NULL, 0, 64, "0", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_prefix_suffix = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_suffix", "string",
        N_("string displayed after prefix"),
        NULL, 0, 0, "|", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_nick_completor = config_file_new_option (
        weechat_config_file, ptr_section,
        "nick_completor", "string",
        N_("string inserted after nick completion"),
        NULL, 0, 0, ":", NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_nick_completion_ignore = config_file_new_option (
        weechat_config_file, ptr_section,
        "nick_completion_ignore", "string",
        N_("chars ignored for nick completion"),
        NULL, 0, 0, "[]-^", NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_nick_complete_first = config_file_new_option (
        weechat_config_file, ptr_section,
        "nick_complete_first", "boolean",
        N_("complete only with first nick found"),
        NULL, 0, 0, "off", NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_infobar = config_file_new_option (
        weechat_config_file, ptr_section,
        "infobar", "boolean",
        N_("enable info bar"),
        NULL, 0, 0, "on", NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_infobar_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "infobar_time_format", "string",
        N_("time format for time in infobar"),
        NULL, 0, 0, "%B, %A %d %Y", NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_infobar_seconds = config_file_new_option (
        weechat_config_file, ptr_section,
        "infobar_seconds", "boolean",
        N_("display seconds in infobar time"),
        NULL, 0, 0, "on", NULL, NULL, &config_change_infobar_seconds, NULL, NULL, NULL);
    config_look_infobar_delay_highlight = config_file_new_option (
        weechat_config_file, ptr_section,
        "infobar_delay_highlight", "integer",
        N_("delay (in seconds) for highlight messages in "
           "infobar (0 = disable highlight notifications in "
           "infobar)"),
        NULL, 0, INT_MAX, "7", NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_item_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "item_time_format", "string",
        N_("time format for \"time\" bar item"),
        NULL, 0, 0, "%H:%M", NULL, NULL, &config_change_item_time_format, NULL, NULL, NULL);
    config_look_hotlist_names_count = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_names_count", "integer",
        N_("max number of names in hotlist (0 = no name "
           "displayed, only buffer numbers)"),
        NULL, 0, 32, "3", NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_names_level = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_names_level", "integer",
        N_("level for displaying names in hotlist (combination "
           "of: 1=join/part, 2=message, 4=private, 8=highlight, "
           "for example: 12=private+highlight)"),
        NULL, 1, 15, "12", NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_names_length = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_names_length", "integer",
        N_("max length of names in hotlist (0 = no limit)"),
        NULL, 0, 32, "0", NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_hotlist_sort = config_file_new_option (
        weechat_config_file, ptr_section,
        "hotlist_sort", "integer",
        N_("hotlist sort type (group_time_asc (default), "
           "group_time_desc, group_number_asc, group_number_desc, "
           "number_asc, number_desc)"),
        "group_time_asc|group_time_desc|group_number_asc|"
        "group_number_desc|number_asc|number_desc",
        0, 0, "group_time_asc", NULL, NULL, &config_change_hotlist, NULL, NULL, NULL);
    config_look_day_change = config_file_new_option (
        weechat_config_file, ptr_section,
        "day_change", "boolean",
        N_("display special message when day changes"),
        NULL, 0, 0, "on", NULL, NULL, &config_change_day_change, NULL, NULL, NULL);
    config_look_day_change_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "day_change_time_format", "string",
        N_("time format for date displayed when day changed"),
        NULL, 0, 0, "%a, %d %b %Y", NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_read_marker = config_file_new_option (
        weechat_config_file, ptr_section,
        "read_marker", "integer",
        N_("use a marker (line or char) on buffers to show first unread line"),
        "none|line|dotted-line|char",
        0, 0, "dotted-line", NULL, NULL, &config_change_read_marker, NULL, NULL, NULL);
    config_look_input_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_format", "string",
        N_("format for input prompt ('%c' is replaced by channel "
           "or server, '%n' by nick and '%m' by nick modes)"),
        NULL, 0, 0, "[%n(%m)] ", NULL, NULL, &config_change_buffer_content, NULL, NULL, NULL);
    config_look_paste_max_lines = config_file_new_option (
        weechat_config_file, ptr_section,
        "paste_max_lines", "integer",
        N_("max number of lines for paste without asking user "
           "(0 = disable this feature)"),
        NULL, 0, INT_MAX, "3", NULL, NULL, NULL, NULL, NULL, NULL);
    
    /* colors */
    ptr_section = config_file_new_section (weechat_config_file, "color",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }
    
    /* general color settings */
    config_color_separator = config_file_new_option (
        weechat_config_file, ptr_section,
        "separator", "color",
        N_("background color for window separators (when splited)"),
        NULL, GUI_COLOR_SEPARATOR, 0, "blue",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* title window */
    config_color_title = config_file_new_option (
        weechat_config_file, ptr_section,
        "title", "color",
        N_("text color for title bar"),
        NULL, GUI_COLOR_TITLE, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_title_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "title_bg", "color",
        N_("background color for title bar"),
        NULL, -1, 0, "blue",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_title_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "title_more", "color",
        N_("text color for '+' when scrolling title"),
        NULL, GUI_COLOR_TITLE_MORE, 0, "lightmagenta",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* chat window */
    config_color_chat = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat", "color",
        N_("text color for chat"),
        NULL, GUI_COLOR_CHAT, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_bg", "color",
        N_("background color for chat"),
        NULL, -1, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_time = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_time", "color",
        N_("text color for time in chat window"),
        NULL, GUI_COLOR_CHAT_TIME, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_time_delimiters = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_time_delimiters", "color",
        N_("text color for time delimiters"),
        NULL, GUI_COLOR_CHAT_TIME_DELIMITERS, 0, "brown",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix[GUI_CHAT_PREFIX_ERROR] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_error", "color",
        N_("text color for error prefix"),
        NULL, GUI_COLOR_CHAT_PREFIX_ERROR, 0, "yellow",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix[GUI_CHAT_PREFIX_NETWORK] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_network", "color",
        N_("text color for network prefix"),
        NULL, GUI_COLOR_CHAT_PREFIX_NETWORK, 0, "magenta",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix[GUI_CHAT_PREFIX_ACTION] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_action", "color",
        N_("text color for action prefix"),
        NULL, GUI_COLOR_CHAT_PREFIX_ACTION, 0, "white",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix[GUI_CHAT_PREFIX_JOIN] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_join", "color",
        N_("text color for join prefix"),
        NULL, GUI_COLOR_CHAT_PREFIX_JOIN, 0, "lightgreen",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix[GUI_CHAT_PREFIX_QUIT] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_quit", "color",
        N_("text color for quit prefix"),
        NULL, GUI_COLOR_CHAT_PREFIX_QUIT, 0, "lightred",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_more", "color",
        N_("text color for '+' when prefix is too long"),
        NULL, GUI_COLOR_CHAT_PREFIX_MORE, 0, "lightmagenta",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_prefix_suffix = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_prefix_suffix", "color",
        N_("text color for suffix (after prefix)"),
        NULL, GUI_COLOR_CHAT_PREFIX_SUFFIX, 0, "green",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_buffer = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_buffer", "color",
        N_("text color for buffer names"),
        NULL, GUI_COLOR_CHAT_BUFFER, 0, "white",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_server = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_server", "color",
        N_("text color for server names"),
        NULL, GUI_COLOR_CHAT_SERVER, 0, "brown",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_channel = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_channel", "color",
        N_("text color for channel names"),
        NULL, GUI_COLOR_CHAT_CHANNEL, 0, "white",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick", "color",
        N_("text color for nicks in chat window"),
        NULL, GUI_COLOR_CHAT_NICK, 0, "lightcyan",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_self = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_self", "color",
        N_("text color for local nick in chat window"),
        NULL, GUI_COLOR_CHAT_NICK_SELF, 0, "white",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_other = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_other", "color",
        N_("text color for other nick in private buffer"),
        NULL, GUI_COLOR_CHAT_NICK_OTHER, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[0] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color01", "color",
        N_("text color #1 for nick"),
        NULL, GUI_COLOR_CHAT_NICK1, 0, "cyan",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[1] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color02", "color",
        N_("text color #2 for nick"),
        NULL, GUI_COLOR_CHAT_NICK2, 0, "magenta",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[2] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color03", "color",
        N_("text color #3 for nick"),
        NULL, GUI_COLOR_CHAT_NICK3, 0, "green",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[3] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color04", "color",
        N_("text color #4 for nick"),
        NULL, GUI_COLOR_CHAT_NICK4, 0, "brown",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[4] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color05", "color",
        N_("text color #5 for nick"),
        NULL, GUI_COLOR_CHAT_NICK5, 0, "lightblue",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[5] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color06", "color",
        N_("text color #6 for nick"),
        NULL, GUI_COLOR_CHAT_NICK6, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[6] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color07", "color",
        N_("text color #7 for nick"),
        NULL, GUI_COLOR_CHAT_NICK7, 0, "lightcyan",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[7] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color08", "color",
        N_("text color #8 for nick"),
        NULL, GUI_COLOR_CHAT_NICK8, 0, "lightmagenta",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[8] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color09", "color",
        N_("text color #9 for nick"),
        NULL, GUI_COLOR_CHAT_NICK9, 0, "lightgreen",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[9] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color10", "color",
        N_("text color #10 for nick"),
        NULL, GUI_COLOR_CHAT_NICK10, 0, "blue",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_host = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_host", "color",
        N_("text color for hostnames"),
        NULL, GUI_COLOR_CHAT_HOST, 0, "cyan",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_delimiters = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_delimiters", "color",
        N_("text color for delimiters"),
        NULL, GUI_COLOR_CHAT_DELIMITERS, 0, "green",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_highlight = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_highlight", "color",
        N_("text color for highlighted nick"),
        NULL, GUI_COLOR_CHAT_HIGHLIGHT, 0, "yellow",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_read_marker = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_read_marker", "color",
        N_("text color for unread data marker"),
        NULL, GUI_COLOR_CHAT_READ_MARKER, 0, "magenta",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_read_marker_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_read_marker_bg", "color",
        N_("background color for unread data marker"),
        NULL, -1, 0, "default", NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* status window */
    config_color_status = config_file_new_option (
        weechat_config_file, ptr_section,
        "status", "color",
        N_("text color for status bar"),
        NULL, GUI_COLOR_STATUS, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_bg", "color",
        N_("background color for status bar"),
        NULL, -1, 0, "blue", NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_delimiters = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_delimiters", "color",
        N_("text color for status bar delimiters"),
        NULL, GUI_COLOR_STATUS_DELIMITERS, 0, "cyan",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_number = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_number", "color",
        N_("text color for current buffer number in status bar"),
        NULL, GUI_COLOR_STATUS_NUMBER, 0, "yellow",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_category = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_category", "color",
        N_("text color for current buffer category in status bar"),
        NULL, GUI_COLOR_STATUS_CATEGORY, 0, "lightgreen",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_name = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_name", "color",
        N_("text color for current buffer name in status bar"),
        NULL, GUI_COLOR_STATUS_NAME, 0, "white",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_data_msg = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_data_msg", "color",
        N_("text color for buffer with new messages (status bar)"),
        NULL, GUI_COLOR_STATUS_DATA_MSG, 0, "yellow",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_data_private = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_data_private", "color",
        N_("text color for buffer with private message (status bar)"),
        NULL, GUI_COLOR_STATUS_DATA_PRIVATE, 0, "lightgreen",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_data_highlight = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_data_highlight", "color",
        N_("text color for buffer with highlight (status bar)"),
        NULL, GUI_COLOR_STATUS_DATA_HIGHLIGHT, 0, "lightmagenta",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_data_other = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_data_other", "color",
        N_("text color for buffer with new data (not messages) "
           "(status bar)"),
        NULL, GUI_COLOR_STATUS_DATA_OTHER, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_status_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "status_more", "color",
        N_("text color for buffer with new data (status bar)"),
        NULL, GUI_COLOR_STATUS_MORE, 0, "yellow",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* infobar window */
    config_color_infobar = config_file_new_option (
        weechat_config_file, ptr_section,
        "infobar", "color",
        N_("text color for infobar"),
        NULL, GUI_COLOR_INFOBAR, 0, "black",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_infobar_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "infobar_bg", "color",
        N_("background color for infobar"),
        NULL, -1, 0, "cyan",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_infobar_delimiters = config_file_new_option (
        weechat_config_file, ptr_section,
        "infobar_delimiters", "color",
        N_("text color for infobar delimiters"),
        NULL, GUI_COLOR_INFOBAR_DELIMITERS, 0, "blue",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_infobar_highlight = config_file_new_option (
        weechat_config_file, ptr_section,
        "infobar_highlight", "color",
        N_("text color for infobar highlight notification"),
        NULL, GUI_COLOR_INFOBAR_HIGHLIGHT, 0, "white",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* input window */
    config_color_input = config_file_new_option (
        weechat_config_file, ptr_section,
        "input", "color",
        N_("text color for input line"),
        NULL, GUI_COLOR_INPUT, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_input_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_bg", "color",
        N_("background color for input line"),
        NULL, -1, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_input_server = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_server", "color",
        N_("text color for server name in input line"),
        NULL, GUI_COLOR_INPUT_SERVER, 0, "brown",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_input_channel = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_channel", "color",
        N_("text color for channel name in input line"),
        NULL, GUI_COLOR_INPUT_CHANNEL, 0, "white",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_input_nick = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_nick", "color",
        N_("text color for nick name in input line"),
        NULL, GUI_COLOR_INPUT_NICK, 0, "lightcyan",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_input_delimiters = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_delimiters", "color",
        N_("text color for delimiters in input line"),
        NULL, GUI_COLOR_INPUT_DELIMITERS, 0, "cyan",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_input_text_not_found = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_text_not_found", "color",
        N_("text color for unsucessful text search in input line"),
        NULL, GUI_COLOR_INPUT_TEXT_NOT_FOUND, 0, "red",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_input_actions = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_actions", "color",
        N_("text color for actions in input line"),
        NULL, GUI_COLOR_INPUT_ACTIONS, 0, "lightgreen",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* nicklist window */
    config_color_nicklist = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist", "color",
        N_("text color for nicklist"),
        NULL, GUI_COLOR_NICKLIST, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_bg = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_bg", "color",
        N_("background color for nicklist"),
        NULL, -1, 0, "default",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_group = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_group", "color",
        N_("text color for groups in nicklist"),
        NULL, GUI_COLOR_NICKLIST_GROUP, 0, "green",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_away = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_away", "color",
        N_("text color for away nicknames"),
        NULL, GUI_COLOR_NICKLIST_AWAY, 0, "cyan",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_prefix1 = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_prefix1", "color",
        N_("text color for prefix #1 in nicklist"),
        NULL, GUI_COLOR_NICKLIST_PREFIX1, 0, "lightgreen",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_prefix2 = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_prefix2", "color",
        N_("text color for prefix #2 in nicklist"),
        NULL, GUI_COLOR_NICKLIST_PREFIX2, 0, "lightmagenta",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_prefix3 = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_prefix3", "color",
        N_("text color for prefix #3 in nicklist"),
        NULL, GUI_COLOR_NICKLIST_PREFIX3, 0, "yellow",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_prefix4 = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_prefix4", "color",
        N_("text color for prefix #4 in nicklist"),
        NULL, GUI_COLOR_NICKLIST_PREFIX4, 0, "blue",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_prefix5 = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_prefix5", "color",
        N_("text color for prefix #5 in nicklist"),
        NULL, GUI_COLOR_NICKLIST_PREFIX5, 0, "brown",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_more", "color",
        N_("text color for '+' when scrolling nicks in nicklist"),
        NULL, GUI_COLOR_NICKLIST_MORE, 0, "lightmagenta",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_separator = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_separator", "color",
        N_("text color for nicklist separator"),
        NULL, GUI_COLOR_NICKLIST_SEPARATOR, 0, "blue",
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    
    /* history */
    ptr_section = config_file_new_section (weechat_config_file, "history",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }
    
    config_history_max_lines = config_file_new_option (
        weechat_config_file, ptr_section,
        "max_lines", "integer",
        N_("maximum number of lines in history per buffer "
           "(0 = unlimited)"),
        NULL, 0, INT_MAX, "4096", NULL, NULL, NULL, NULL, NULL, NULL);
    config_history_max_commands = config_file_new_option (
        weechat_config_file, ptr_section,
        "max_commands", "integer",
        N_("maximum number of user commands in history (0 = "
           "unlimited)"),
        NULL, 0, INT_MAX, "100", NULL, NULL, NULL, NULL, NULL, NULL);
    config_history_display_default = config_file_new_option (
        weechat_config_file, ptr_section,
        "display_default", "integer",
        N_("maximum number of commands to display by default in "
           "history listing (0 = unlimited)"),
        NULL, 0, INT_MAX, "5", NULL, NULL, NULL, NULL, NULL, NULL);

    /* proxy */
    ptr_section = config_file_new_section (weechat_config_file, "proxy",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }
    
    config_proxy_use = config_file_new_option (
        weechat_config_file, ptr_section,
        "use", "boolean",
        N_("use a proxy server"),
        NULL, 0, 0, "off", NULL, NULL, NULL, NULL, NULL, NULL);
    config_proxy_type = config_file_new_option (
        weechat_config_file, ptr_section,
        "type", "integer",
        N_("proxy type (http (default), socks4, socks5)"),
        "http|socks4|socks5", 0, 0, "http", NULL, NULL, NULL, NULL, NULL, NULL);
    config_proxy_ipv6 = config_file_new_option (
        weechat_config_file, ptr_section,
        "ipv6", "boolean",
        N_("connect to proxy using ipv6"),
        NULL, 0, 0, "off", NULL, NULL, NULL, NULL, NULL, NULL);
    config_proxy_address = config_file_new_option (
        weechat_config_file, ptr_section,
        "address", "string",
        N_("proxy server address (IP or hostname)"),
        NULL, 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL);
    config_proxy_port = config_file_new_option (
        weechat_config_file, ptr_section,
        "port", "integer",
        N_("port for connecting to proxy server"),
        NULL, 0, 65535, "3128", NULL, NULL, NULL, NULL, NULL, NULL);
    config_proxy_username = config_file_new_option (
        weechat_config_file, ptr_section,
        "username", "string",
        N_("username for proxy server"),
        NULL, 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL);
    config_proxy_password = config_file_new_option (
        weechat_config_file, ptr_section,
        "password", "string",
        N_("password for proxy server"),
        NULL, 0, 0, "", NULL, NULL, NULL, NULL, NULL, NULL);
            
    /* plugin */
    ptr_section = config_file_new_section (weechat_config_file, "plugin",
                                           0, 0,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }
    
    config_plugin_path = config_file_new_option (
        weechat_config_file, ptr_section,
        "path", "string",
        N_("path for searching plugins ('%h' will be replaced by "
           "WeeChat home, ~/.weechat by default)"),
        NULL, 0, 0, "%h/plugins", NULL, NULL, NULL, NULL, NULL, NULL);
    config_plugin_autoload = config_file_new_option (
        weechat_config_file, ptr_section,
        "autoload", "string",
        N_("comma separated list of plugins to load automatically "
           "at startup, \"*\" means all plugins found (names may "
           "be partial, for example \"perl\" is ok for "
           "\"perl.so\")"),
        NULL, 0, 0, "*", NULL, NULL, NULL, NULL, NULL, NULL);
    config_plugin_extension = config_file_new_option (
        weechat_config_file, ptr_section,
        "extension", "string",
        N_("standard plugins extension in filename (for example "
           "\".so\" under Linux or \".dll\" under Microsoft Windows)"),
        NULL, 0, 0,
#ifdef WIN32
        ".dll",
#else
        ".so",
#endif              
        NULL, NULL, NULL, NULL, NULL, NULL);
    config_plugin_save_config_on_unload = config_file_new_option (
        weechat_config_file, ptr_section,
        "save_config_on_unload", "boolean",
        N_("save configuration files when unloading plugins"),
        NULL, 0, 0, "on", NULL, NULL, NULL, NULL, NULL, NULL);
    
    /* bars */
    ptr_section = config_file_new_section (weechat_config_file, "bar",
                                           0, 0,
                                           &config_weechat_bar_read, NULL,
                                           NULL, NULL,
                                           NULL, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }
    
    weechat_config_section_bar = ptr_section;
    
    /* filters */
    ptr_section = config_file_new_section (weechat_config_file, "filter",
                                           0, 0,
                                           &config_weechat_filter_read, NULL,
                                           &config_weechat_filter_write, NULL,
                                           &config_weechat_filter_write, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }
    
    /* keys */
    ptr_section = config_file_new_section (weechat_config_file, "key",
                                           0, 0,
                                           &config_weechat_key_read, NULL,
                                           &config_weechat_key_write, NULL,
                                           &config_weechat_key_write, NULL,
                                           NULL, NULL);
    if (!ptr_section)
    {
        config_file_free (weechat_config_file);
        return 0;
    }
    
    return 1;
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
    int rc;
    
    rc = config_file_read (weechat_config_file);
    if (rc == 0)
    {
        config_change_infobar_seconds (NULL, NULL);
        config_change_day_change (NULL, NULL);
        gui_bar_use_temp_bars ();
    }
    
    return rc;
}

/*
 * config_weechat_write: write WeeChat configuration file
 *                       return:  0 if ok
 *                              < 0 if error
 */

int
config_weechat_write ()
{
    return config_file_write (weechat_config_file);
}
