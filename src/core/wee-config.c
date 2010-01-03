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
#include "wee-hook.h"
#include "wee-log.h"
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
#include "../gui/gui-keyboard.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


struct t_config_file *weechat_config_file = NULL;
struct t_config_section *weechat_config_section_debug = NULL;
struct t_config_section *weechat_config_section_proxy = NULL;
struct t_config_section *weechat_config_section_bar = NULL;
struct t_config_section *weechat_config_section_notify = NULL;

/* config, startup section */

struct t_config_option *config_startup_command_after_plugins;
struct t_config_option *config_startup_command_before_plugins;
struct t_config_option *config_startup_display_logo;
struct t_config_option *config_startup_display_version;
struct t_config_option *config_startup_weechat_slogan;

/* config, look & feel section */

struct t_config_option *config_look_buffer_notify_default;
struct t_config_option *config_look_buffer_time_format;
struct t_config_option *config_look_color_nicks_number;
struct t_config_option *config_look_color_real_white;
struct t_config_option *config_look_day_change;
struct t_config_option *config_look_day_change_time_format;
struct t_config_option *config_look_highlight;
struct t_config_option *config_look_hline_char;
struct t_config_option *config_look_hotlist_names_count;
struct t_config_option *config_look_hotlist_names_length;
struct t_config_option *config_look_hotlist_names_level;
struct t_config_option *config_look_hotlist_names_merged_buffers;
struct t_config_option *config_look_hotlist_short_names;
struct t_config_option *config_look_hotlist_sort;
struct t_config_option *config_look_item_time_format;
struct t_config_option *config_look_jump_current_to_previous_buffer;
struct t_config_option *config_look_jump_previous_buffer_when_closing;
struct t_config_option *config_look_nickmode;
struct t_config_option *config_look_nickmode_empty;
struct t_config_option *config_look_paste_max_lines;
struct t_config_option *config_look_prefix[GUI_CHAT_NUM_PREFIXES];
struct t_config_option *config_look_prefix_align;
struct t_config_option *config_look_prefix_align_max;
struct t_config_option *config_look_prefix_align_more;
struct t_config_option *config_look_prefix_buffer_align;
struct t_config_option *config_look_prefix_buffer_align_max;
struct t_config_option *config_look_prefix_buffer_align_more;
struct t_config_option *config_look_prefix_suffix;
struct t_config_option *config_look_read_marker;
struct t_config_option *config_look_save_config_on_exit;
struct t_config_option *config_look_save_layout_on_exit;
struct t_config_option *config_look_scroll_amount;
struct t_config_option *config_look_scroll_page_percent;
struct t_config_option *config_look_search_text_not_found_alert;
struct t_config_option *config_look_set_title;

/* config, colors section */

struct t_config_option *config_color_separator;
struct t_config_option *config_color_bar_more;
struct t_config_option *config_color_chat;
struct t_config_option *config_color_chat_bg;
struct t_config_option *config_color_chat_time;
struct t_config_option *config_color_chat_time_delimiters;
struct t_config_option *config_color_chat_prefix_buffer;
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
struct t_config_option *config_color_chat_highlight_bg;
struct t_config_option *config_color_chat_read_marker;
struct t_config_option *config_color_chat_read_marker_bg;
struct t_config_option *config_color_chat_text_found;
struct t_config_option *config_color_chat_text_found_bg;
struct t_config_option *config_color_chat_value;
struct t_config_option *config_color_status_number;
struct t_config_option *config_color_status_name;
struct t_config_option *config_color_status_filter;
struct t_config_option *config_color_status_data_msg;
struct t_config_option *config_color_status_data_private;
struct t_config_option *config_color_status_data_highlight;
struct t_config_option *config_color_status_data_other;
struct t_config_option *config_color_status_more;
struct t_config_option *config_color_status_time;
struct t_config_option *config_color_input_text_not_found;
struct t_config_option *config_color_input_actions;
struct t_config_option *config_color_nicklist_group;
struct t_config_option *config_color_nicklist_away;
struct t_config_option *config_color_nicklist_prefix1;
struct t_config_option *config_color_nicklist_prefix2;
struct t_config_option *config_color_nicklist_prefix3;
struct t_config_option *config_color_nicklist_prefix4;
struct t_config_option *config_color_nicklist_prefix5;
struct t_config_option *config_color_nicklist_more;

/* config, completion section */

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

struct t_config_option *config_history_max_lines;
struct t_config_option *config_history_max_commands;
struct t_config_option *config_history_max_visited_buffers;
struct t_config_option *config_history_display_default;

/* config, network section */

struct t_config_option *config_network_gnutls_ca_file;

/* config, plugin section */

struct t_config_option *config_plugin_autoload;
struct t_config_option *config_plugin_debug;
struct t_config_option *config_plugin_extension;
struct t_config_option *config_plugin_path;
struct t_config_option *config_plugin_save_config_on_unload;

/* other */

struct t_hook *config_day_change_timer = NULL;
int config_day_change_old_day = -1;


/*
 * config_change_save_config_on_exit: called when "save_config_on_exit" flag is changed
 */

void
config_change_save_config_on_exit (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if (!CONFIG_BOOLEAN(config_look_save_config_on_exit))
    {
        gui_chat_printf (NULL,
                         _("Warning: you should now issue /save to write "
                           "\"save_config_on_exit\" option in configuration "
                           "file"));
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
    
    if (CONFIG_BOOLEAN(config_look_set_title))
	gui_window_set_title (PACKAGE_NAME " " PACKAGE_VERSION);
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
        gui_current_window->refresh_needed = 1;
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
        gui_window_ask_refresh (1);
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
    
    gui_window_ask_refresh (1);
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
        gui_color_init_weechat ();
        gui_window_refresh_windows ();
    }
}

/*
 * config_day_change_timer_cb: timer callback for displaying
 *                             "Day changed to xxx" message
 */

int
config_day_change_timer_cb (void *data, int remaining_calls)
{
    struct timeval tv_time;
    struct tm *local_time;
    char text_time[1024], *text_time2;
    struct t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;
    
    gettimeofday (&tv_time, NULL);
    local_time = localtime (&tv_time.tv_sec);

    if ((config_day_change_old_day >= 0)
        && (local_time->tm_mday != config_day_change_old_day))
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
                gui_chat_printf (ptr_buffer,
                                 _("\t\tDay changed to %s"),
                                 (text_time2) ?
                                 text_time2 : text_time);
        }
        if (text_time2)
            free (text_time2);
        gui_add_hotlist = 1;
    }
    
    config_day_change_old_day = local_time->tm_mday;
    
    return WEECHAT_RC_OK;
}

/*
 * config_change_day_change: called when day_change option changed
 */

void
config_change_day_change (void *data, struct t_config_option *option)
{
    struct timeval tv_time;
    struct tm *local_time;
    
    /* make C compiler happy */
    (void) data;
    (void) option;
    
    if (CONFIG_BOOLEAN(config_look_day_change))
    {
        if (!config_day_change_timer)
        {
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
    }
    else
    {
        if (config_day_change_timer)
        {
            unhook (config_day_change_timer);
            config_day_change_timer = NULL;
            config_day_change_old_day = -1;
        }
    }
}

/*
 * config_weechat_reload_cb: reload WeeChat configuration file
 *                           return one of these values:
 *                             WEECHAT_CONFIG_READ_OK
 *                             WEECHAT_CONFIG_READ_MEMORY_ERROR
 *                             WEECHAT_CONFIG_READ_FILE_NOT_FOUND
 */

int
config_weechat_reload_cb (void *data, struct t_config_file *config_file)
{
    int rc;
    
    /* make C compiler happy */
    (void) data;
    
    /* remove all keys */
    gui_keyboard_free_all (&gui_keys, &last_gui_key);
    
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
    
    if (rc == WEECHAT_CONFIG_READ_OK)
    {
        gui_buffer_notify_set_all ();
        proxy_use_temp_proxies ();
        gui_bar_use_temp_bars ();
        gui_bar_create_default ();
        /* if no key was found config file, then we use default bindings */
        if (!gui_keys)
            gui_keyboard_default_bindings ();
    }
    
    return rc;
}

/*
 * config_weechat_debug_get: get debug level for a plugin (or "core")
 */

struct t_config_option *
config_weechat_debug_get (const char *plugin_name)
{
    return config_file_search_option (weechat_config_file,
                                      weechat_config_section_debug,
                                      plugin_name);
}

/*
 * config_weechat_debug_set_all: set debug for "core" and all plugins, using
 *                               values from [debug] section
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
 * config_weechat_debug_change_cb: called when a debug option is changed
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
 * config_weechat_debug_create_option_cb: create option in "debug" section
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
 * config_weechat_debug_delete_option_cb: delete option in "debug" section
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
 * config_weechat_debug_set: set debug level for a plugin (or "core")
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
 * config_weechat_proxy_read_cb: read proxy option in config file
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
                }
                
                free (proxy_name);
            }
        }
    }
    
    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * config_weechat_bar_read_cb: read bar option in config file
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
                }
                
                free (bar_name);
            }
        }
    }
    
    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * config_weechat_layout_read_cb: read layout option in config file
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
 * config_weechat_layout_write: write windows layout in configuration file
 */

void
config_weechat_layout_write_tree (struct t_config_file *config_file,
                                  struct t_gui_layout_window *layout_window)
{
    config_file_write_line (config_file, "window", "\"%d;%d;%d;%d;%s;%s\"",
                            layout_window->internal_id,
                            (layout_window->parent_node) ?
                            layout_window->parent_node->internal_id : 0,
                            layout_window->split_pct,
                            layout_window->split_horiz,
                            (layout_window->plugin_name) ?
                            layout_window->plugin_name : "-",
                            (layout_window->buffer_name) ?
                            layout_window->buffer_name : "-");
    
    if (layout_window->child1)
        config_weechat_layout_write_tree (config_file, layout_window->child1);
    
    if (layout_window->child2)
        config_weechat_layout_write_tree (config_file, layout_window->child2);
}

/*
 * config_weechat_layout_write_cb: write layout section in configuration file
 */

void
config_weechat_layout_write_cb (void *data, struct t_config_file *config_file,
                                const char *section_name)
{
    struct t_gui_layout_buffer *ptr_layout_buffer;
    
    /* make C compiler happy */
    (void) data;
    
    config_file_write_line (config_file, section_name, NULL);
    
    for (ptr_layout_buffer = gui_layout_buffers; ptr_layout_buffer;
         ptr_layout_buffer = ptr_layout_buffer->next_layout)
    {
        config_file_write_line (config_file, "buffer", "\"%s;%s;%d\"",
                                ptr_layout_buffer->plugin_name,
                                ptr_layout_buffer->buffer_name,
                                ptr_layout_buffer->number);
    }
    
    if (gui_layout_windows)
        config_weechat_layout_write_tree (config_file, gui_layout_windows);
}

/*
 * config_weechat_notify_change_cb: callback when notify option is changed
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
 * config_weechat_notify_create_option_cb: callback to create option in "notify"
 *                                         section
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
 * config_weechat_notify_delete_option_cb: called when a notify option is
 *                                         deleted
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
 * config_weechat_notify_set: set a notify level for a buffer
 *                            negative value will reset notify for buffer to
 *                            default value (and remove buffer from config file)
 *                            return 1 if ok, 0 if error
 */

int
config_weechat_notify_set (struct t_gui_buffer *buffer, const char *notify)
{
    const char *plugin_name;
    char *option_name;
    int i, value, length;
    
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
    
    plugin_name = plugin_get_name (buffer->plugin);
    length = strlen (plugin_name) + 1 + strlen (buffer->name) + 1;
    option_name = malloc (length);
    if (option_name)
    {
        snprintf (option_name, length, "%s.%s", plugin_name, buffer->name);
        
        /* create/update option */
        config_weechat_notify_create_option_cb (NULL,
                                                weechat_config_file,
                                                weechat_config_section_notify,
                                                option_name,
                                                (value < 0) ?
                                                NULL : gui_buffer_notify_string[value]);
        return 1;
    }
    
    return 0;
}

/*
 * config_weechat_filter_read_cb: read filter option from config file
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
 * config_weechat_filter_write_cb: write filter section in configuration file
 */

void
config_weechat_filter_write_cb (void *data, struct t_config_file *config_file,
                                const char *section_name)
{
    struct t_gui_filter *ptr_filter;
    
    /* make C compiler happy */
    (void) data;
    
    config_file_write_line (config_file, section_name, NULL);
    
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        config_file_write_line (config_file,
                                ptr_filter->name,
                                "%s;%s%s%s;%s;%s",
                                (ptr_filter->enabled) ? "on" : "off",
                                (ptr_filter->plugin_name) ? ptr_filter->plugin_name : "",
                                (ptr_filter->plugin_name) ? "." : "",
                                ptr_filter->buffer_name,
                                ptr_filter->tags,
                                ptr_filter->regex);
    }
}

/*
 * config_weechat_key_read_cb: read key option in config file
 */

int
config_weechat_key_read_cb (void *data, struct t_config_file *config_file,
                            struct t_config_section *section,
                            const char *option_name, const char *value)
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
            gui_keyboard_unbind (NULL, option_name, 1);
        }
    }
    
    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * config_weechat_key_write_cb: write key section in configuration file
 */

void
config_weechat_key_write_cb (void *data, struct t_config_file *config_file,
                             const char *section_name)
{
    struct t_gui_key *ptr_key;
    char *expanded_name;
    
    /* make C compiler happy */
    (void) data;
    
    config_file_write_line (config_file, section_name, NULL);
    
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_keyboard_get_expanded_name (ptr_key->key);
        if (expanded_name)
        {
            config_file_write_line (config_file,
                                    (expanded_name) ?
                                    expanded_name : ptr_key->key,
                                    "\"%s\"",
                                    ptr_key->command);
            free (expanded_name);
        }
    }
}

/*
 * config_weechat_init_options: init WeeChat config structure (all core options)
 *                              return: 1 if ok, 0 if error
 */

int
config_weechat_init_options ()
{
    struct t_config_section *ptr_section;
    
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
    config_startup_weechat_slogan = config_file_new_option (
        weechat_config_file, ptr_section,
        "weechat_slogan", "string",
        N_("WeeChat slogan (if empty, slogan is not used)"),
        NULL, 0, 0, _("the geekiest chat client!"), NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    
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
    
    config_look_buffer_notify_default = config_file_new_option (
        weechat_config_file, ptr_section,
        "buffer_notify_default", "integer",
        N_("default notify level for buffers (used to tell WeeChat if buffer "
           "must be displayed in hotlist or not, according to importance "
           "of message)"),
        "none|highlight|message|all", 0, 0, "all", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_buffer_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "buffer_time_format", "string",
        N_("time format for buffers"),
        NULL, 0, 0, "%H:%M:%S", NULL, 0, NULL, NULL, &config_change_buffer_time_format, NULL, NULL, NULL);
    config_look_color_nicks_number = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_nicks_number", "integer",
        N_("number of colors to use for nicks colors"),
        NULL, 1, 10, "10", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_color_real_white = config_file_new_option (
        weechat_config_file, ptr_section,
        "color_real_white", "boolean",
        N_("if set, uses real white color, disabled by default "
           "for terms with white background (if you never use "
           "white background, you should turn on this option to "
           "see real white instead of default term foreground "
           "color)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_look_day_change = config_file_new_option (
        weechat_config_file, ptr_section,
        "day_change", "boolean",
        N_("display special message when day changes"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_day_change, NULL, NULL, NULL);
    config_look_day_change_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "day_change_time_format", "string",
        N_("time format for date displayed when day changed"),
        NULL, 0, 0, "%a, %d %b %Y", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_highlight = config_file_new_option (
        weechat_config_file, ptr_section,
        "highlight", "string",
        N_("comma separated list of words to highlight (case insensitive "
           "comparison, words may begin or end with \"*\" for partial match)"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_hline_char = config_file_new_option (
        weechat_config_file, ptr_section,
        "hline_char", "string",
        N_("char used to draw horizontal lines, note that empty value will "
           "draw a real line with ncurses, but may cause bugs with URL "
           "selection under some terminals"),
        NULL, 0, 0, "-", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
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
        0, 0, "group_time_asc", NULL, 0, NULL, NULL, &config_change_hotlist, NULL, NULL, NULL);
    config_look_item_time_format = config_file_new_option (
        weechat_config_file, ptr_section,
        "item_time_format", "string",
        N_("time format for \"time\" bar item"),
        NULL, 0, 0, "%H:%M", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
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
    config_look_nickmode = config_file_new_option (
        weechat_config_file, ptr_section,
        "nickmode", "boolean",
        N_("display nick mode ((half)op/voice) before each nick"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_nickmode_empty = config_file_new_option (
        weechat_config_file, ptr_section,
        "nickmode_empty", "boolean",
        N_("display space if nick mode is not (half)op/voice"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_paste_max_lines = config_file_new_option (
        weechat_config_file, ptr_section,
        "paste_max_lines", "integer",
        N_("max number of lines for paste without asking user "
           "(0 = disable this feature)"),
        NULL, 0, INT_MAX, "3", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_ERROR] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_error", "string",
        N_("prefix for error messages"),
        NULL, 0, 0, "=!=", NULL, 0, NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_NETWORK] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_network", "string",
        N_("prefix for network messages"),
        NULL, 0, 0, "--", NULL, 0, NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_ACTION] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_action", "string",
        N_("prefix for action messages"),
        NULL, 0, 0, " *", NULL, 0, NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_JOIN] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_join", "string",
        N_("prefix for join messages"),
        NULL, 0, 0, "-->", NULL, 0, NULL, NULL, &config_change_prefix, NULL, NULL, NULL);
    config_look_prefix[GUI_CHAT_PREFIX_QUIT] = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_quit", "string",
        N_("prefix for quit messages"),
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
    config_look_prefix_align_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_align_more", "boolean",
        N_("display '+' if prefix is truncated"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
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
        "prefix_buffer_align_more", "boolean",
        N_("display '+' if buffer name is truncated (when many buffers are "
           "merged with same number)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_prefix_suffix = config_file_new_option (
        weechat_config_file, ptr_section,
        "prefix_suffix", "string",
        N_("string displayed after prefix"),
        NULL, 0, 0, "|", NULL, 0, NULL, NULL, &config_change_buffers, NULL, NULL, NULL);
    config_look_read_marker = config_file_new_option (
        weechat_config_file, ptr_section,
        "read_marker", "integer",
        N_("use a marker (line or char) on buffers to show first unread line"),
        "none|line|dotted-line|char",
        0, 0, "dotted-line", NULL, 0, NULL, NULL, &config_change_read_marker, NULL, NULL, NULL);
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
    config_look_set_title = config_file_new_option (
        weechat_config_file, ptr_section,
        "set_title", "boolean",
        N_("set title for window (terminal for Curses GUI) with "
           "name and version"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, &config_change_title, NULL, NULL, NULL);
    
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
    /* chat window */
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
    config_color_chat_nick_self = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_self", "color",
        N_("text color for local nick in chat window"),
        NULL, GUI_COLOR_CHAT_NICK_SELF, 0, "white", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_other = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_other", "color",
        N_("text color for other nick in private buffer"),
        NULL, GUI_COLOR_CHAT_NICK_OTHER, 0, "cyan", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[0] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color01", "color",
        N_("text color #1 for nick"),
        NULL, GUI_COLOR_CHAT_NICK1, 0, "cyan", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[1] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color02", "color",
        N_("text color #2 for nick"),
        NULL, GUI_COLOR_CHAT_NICK2, 0, "magenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[2] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color03", "color",
        N_("text color #3 for nick"),
        NULL, GUI_COLOR_CHAT_NICK3, 0, "green", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[3] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color04", "color",
        N_("text color #4 for nick"),
        NULL, GUI_COLOR_CHAT_NICK4, 0, "brown", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[4] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color05", "color",
        N_("text color #5 for nick"),
        NULL, GUI_COLOR_CHAT_NICK5, 0, "lightblue", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[5] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color06", "color",
        N_("text color #6 for nick"),
        NULL, GUI_COLOR_CHAT_NICK6, 0, "default", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[6] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color07", "color",
        N_("text color #7 for nick"),
        NULL, GUI_COLOR_CHAT_NICK7, 0, "lightcyan", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[7] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color08", "color",
        N_("text color #8 for nick"),
        NULL, GUI_COLOR_CHAT_NICK8, 0, "lightmagenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[8] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color09", "color",
        N_("text color #9 for nick"),
        NULL, GUI_COLOR_CHAT_NICK9, 0, "lightgreen", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_chat_nick_colors[9] = config_file_new_option (
        weechat_config_file, ptr_section,
        "chat_nick_color10", "color",
        N_("text color #10 for nick"),
        NULL, GUI_COLOR_CHAT_NICK10, 0, "blue", NULL, 0,
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
    /* status window */
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
    /* input window */
    config_color_input_text_not_found = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_text_not_found", "color",
        N_("text color for unsucessful text search in input line"),
        NULL, -1, 0, "red", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_input_actions = config_file_new_option (
        weechat_config_file, ptr_section,
        "input_actions", "color",
        N_("text color for actions in input line"),
        NULL, -1, 0, "lightgreen", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    /* nicklist window */
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
    config_color_nicklist_prefix1 = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_prefix1", "color",
        N_("text color for prefix #1 in nicklist"),
        NULL, -1, 0, "lightgreen", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_prefix2 = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_prefix2", "color",
        N_("text color for prefix #2 in nicklist"),
        NULL, -1, 0, "lightmagenta", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_prefix3 = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_prefix3", "color",
        N_("text color for prefix #3 in nicklist"),
        NULL, -1, 0, "yellow", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_prefix4 = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_prefix4", "color",
        N_("text color for prefix #4 in nicklist"),
        NULL, -1, 0, "blue", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_prefix5 = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_prefix5", "color",
        N_("text color for prefix #5 in nicklist"),
        NULL, -1, 0, "brown", NULL, 0,
        NULL, NULL, &config_change_color, NULL, NULL, NULL);
    config_color_nicklist_more = config_file_new_option (
        weechat_config_file, ptr_section,
        "nicklist_more", "color",
        N_("text color for '+' when scrolling nicks in nicklist"),
        NULL, -1, 0, "lightmagenta", NULL, 0,
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
        N_("string inserted after nick completion"),
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
        NULL, 0, 0, "[]-^", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
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
    
    config_history_max_lines = config_file_new_option (
        weechat_config_file, ptr_section,
        "max_lines", "integer",
        N_("maximum number of lines in history per buffer "
           "(0 = unlimited)"),
        NULL, 0, INT_MAX, "4096", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
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
    
    config_network_gnutls_ca_file = config_file_new_option (
        weechat_config_file, ptr_section,
        "gnutls_ca_file", "string",
        N_("file containing the certificate authorities"),
        NULL, 0, 0, "%h/ssl/CAs.pem", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    
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
           "at startup, \"*\" means all plugins found (names may "
           "be partial, for example \"perl\" is ok for "
           "\"perl.so\")"),
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
        N_("standard plugins extension in filename (for example "
           "\".so\" under Linux or \".dll\" under Microsoft Windows)"),
        NULL, 0, 0,
#if defined(WIN32) || defined(__CYGWIN__)
        ".dll",
#else
        ".so",
#endif              
        NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
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
    ptr_section = config_file_new_section (weechat_config_file, "key",
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
    
    return 1;
}

/*
 * config_weechat_init: init WeeChat config structure
 *                      return: 1 if ok, 0 if error
 */

int
config_weechat_init ()
{
    int rc;
    
    rc = config_weechat_init_options ();
    
    if (!rc)
    {
        gui_chat_printf (NULL,
                         _("FATAL: error initializing configuration options"));
    }
    
    return rc;
}

/*
 * config_weechat_read: read WeeChat configuration file
 *                      return one of these values:
 *                        WEECHAT_CONFIG_READ_OK
 *                        WEECHAT_CONFIG_READ_MEMORY_ERROR
 *                        WEECHAT_CONFIG_READ_FILE_NOT_FOUND
 */

int
config_weechat_read ()
{
    int rc;
    
    rc = config_file_read (weechat_config_file);
    if (rc == WEECHAT_CONFIG_READ_OK)
    {
        config_change_day_change (NULL, NULL);
        proxy_use_temp_proxies ();
        gui_bar_use_temp_bars ();
        gui_bar_create_default ();
        /* if no key was found config file, then we use default bindings */
        if (!gui_keys)
            gui_keyboard_default_bindings ();
    }
    
    if (rc != WEECHAT_CONFIG_READ_OK)
    {
        gui_chat_printf (NULL,
                         _("%sError reading configuration"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    }
    
    return rc;
}

/*
 * config_weechat_write: write WeeChat configuration file
 *                       return one of these values:
 *                         WEECHAT_CONFIG_WRITE_OK
 *                         WEECHAT_CONFIG_WRITE_ERROR
 *                         WEECHAT_CONFIG_WRITE_MEMORY_ERROR
 */

int
config_weechat_write ()
{
    return config_file_write (weechat_config_file);
}
