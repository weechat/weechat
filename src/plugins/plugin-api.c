/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * plugin-api.c: extra functions for plugin API
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <locale.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-input.h"
#include "../core/wee-string.h"
#include "../core/wee-url.h"
#include "../core/wee-util.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-bar-window.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-color.h"
#include "../gui/gui-cursor.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-history.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-key.h"
#include "../gui/gui-line.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "plugin.h"
#include "plugin-api.h"
#include "plugin-config.h"


/*
 * Sets plugin charset.
 */

void
plugin_api_charset_set (struct t_weechat_plugin *plugin, const char *charset)
{
    if (!plugin || !charset)
        return;

    if (plugin->charset)
        free (plugin->charset);

    plugin->charset = (charset) ? strdup (charset) : NULL;
}

/*
 * Translates a string using gettext.
 */

const char *
plugin_api_gettext (const char *string)
{
    return _(string);
}

/*
 * Translates a string using gettext (with plural form).
 */

const char *
plugin_api_ngettext (const char *single, const char *plural, int count)
{
    /* make C compiler happy */
    (void) single;
    (void) count;

    return NG_(single, plural, count);
}

/*
 * Gets pointer on an option.
 */

struct t_config_option *
plugin_api_config_get (const char *option_name)
{
    struct t_config_option *ptr_option;

    config_file_search_with_string (option_name, NULL, NULL, &ptr_option, NULL);

    return ptr_option;
}

/*
 * Gets value of a plugin option.
 */

const char *
plugin_api_config_get_plugin (struct t_weechat_plugin *plugin,
                              const char *option_name)
{
    struct t_config_option *ptr_option;

    if (!plugin || !option_name)
        return NULL;

    ptr_option = plugin_config_search (plugin->name, option_name);
    if (ptr_option)
        return ptr_option->value;

    /* option not found */
    return NULL;
}

/*
 * Checks if a plugin option is set.
 *
 * Returns:
 *   1: plugin option is set
 *   0: plugin option does not exist
 */

int
plugin_api_config_is_set_plugin (struct t_weechat_plugin *plugin,
                                 const char *option_name)
{
    struct t_config_option *ptr_option;

    if (!plugin || !option_name)
        return 0;

    ptr_option = plugin_config_search (plugin->name, option_name);
    if (ptr_option)
        return 1;

    return 0;
}

/*
 * Sets value of a plugin option.
 */

int
plugin_api_config_set_plugin (struct t_weechat_plugin *plugin,
                              const char *option_name, const char *value)
{
    if (!plugin || !option_name)
        return WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND;

    return plugin_config_set (plugin->name, option_name, value);
}

/*
 * Sets description of a plugin option.
 */

void
plugin_api_config_set_desc_plugin (struct t_weechat_plugin *plugin,
                                   const char *option_name,
                                   const char *description)
{
    if (plugin && option_name)
        plugin_config_set_desc (plugin->name, option_name, description);
}

/*
 * Unsets a plugin option.
 */

int
plugin_api_config_unset_plugin (struct t_weechat_plugin *plugin,
                                const char *option_name)
{
    struct t_config_option *ptr_option;

    if (!plugin || !option_name)
        return WEECHAT_CONFIG_OPTION_UNSET_ERROR;

    ptr_option = plugin_config_search (plugin->name, option_name);
    if (!ptr_option)
        return WEECHAT_CONFIG_OPTION_UNSET_ERROR;

    return config_file_option_unset (ptr_option);
}

/*
 * Returns a prefix for display with printf.
 */

const char *
plugin_api_prefix (const char *prefix)
{
    if (!prefix)
        return gui_chat_prefix_empty;

    if (string_strcasecmp (prefix, "error") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_ERROR];
    if (string_strcasecmp (prefix, "network") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_NETWORK];
    if (string_strcasecmp (prefix, "action") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_ACTION];
    if (string_strcasecmp (prefix, "join") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_JOIN];
    if (string_strcasecmp (prefix, "quit") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_QUIT];

    return gui_chat_prefix_empty;
}

/*
 * Returns a WeeChat color for display with printf.
 */

const char *
plugin_api_color (const char *color_name)
{
    const char *str_color;

    if (!color_name)
        return GUI_NO_COLOR;

    /* name is a weechat color option ? => then return this color */
    str_color = gui_color_search_config (color_name);
    if (str_color)
        return str_color;

    return gui_color_get_custom (color_name);
}

/*
 * Executes a command on a buffer (simulates user entry).
 */

void
plugin_api_command (struct t_weechat_plugin *plugin,
                    struct t_gui_buffer *buffer, const char *command)
{
    char *command2;

    if (!plugin || !command)
        return;

    command2 = string_iconv_to_internal (plugin->charset, command);
    if (!buffer)
        buffer = gui_current_window->buffer;
    input_data (buffer, (command2) ? command2 : command);
    if (command2)
        free (command2);
}

/*
 * Gets info about WeeChat.
 */

const char *
plugin_api_info_get_internal (void *data, const char *info_name,
                              const char *arguments)
{
    time_t inactivity;
    static char value[32], version_number[32] = { '\0' };
    static char weechat_dir_absolute_path[PATH_MAX] = { '\0' };

    /* make C compiler happy */
    (void) data;
    (void) arguments;

    if (!info_name)
        return NULL;

    if (string_strcasecmp (info_name, "version") == 0)
    {
        return PACKAGE_VERSION;
    }
    else if (string_strcasecmp (info_name, "version_number") == 0)
    {
        if (!version_number[0])
        {
            snprintf (version_number, sizeof (version_number), "%d",
                      util_version_number (PACKAGE_VERSION));
        }
        return version_number;
    }
    else if (string_strcasecmp (info_name, "date") == 0)
    {
        return __DATE__;
    }
    else if (string_strcasecmp (info_name, "dir_separator") == 0)
    {
        return DIR_SEPARATOR;
    }
    else if (string_strcasecmp (info_name, "weechat_dir") == 0)
    {
        if (!weechat_dir_absolute_path[0])
        {
            if (!realpath (weechat_home, weechat_dir_absolute_path))
                return NULL;
        }
        return (weechat_dir_absolute_path[0]) ?
            weechat_dir_absolute_path : weechat_home;
    }
    else if (string_strcasecmp (info_name, "weechat_libdir") == 0)
    {
        return WEECHAT_LIBDIR;
    }
    else if (string_strcasecmp (info_name, "weechat_sharedir") == 0)
    {
        return WEECHAT_SHAREDIR;
    }
    else if (string_strcasecmp (info_name, "weechat_localedir") == 0)
    {
        return LOCALEDIR;
    }
    else if (string_strcasecmp (info_name, "weechat_site") == 0)
    {
        return WEECHAT_WEBSITE;
    }
    else if (string_strcasecmp (info_name, "weechat_site_download") == 0)
    {
        return WEECHAT_WEBSITE_DOWNLOAD;
    }
    else if (string_strcasecmp (info_name, "weechat_upgrading") == 0)
    {
        snprintf (value, sizeof (value), "%d", weechat_upgrading);
        return value;
    }
    else if (string_strcasecmp (info_name, "charset_terminal") == 0)
    {
        return weechat_local_charset;
    }
    else if (string_strcasecmp (info_name, "charset_internal") == 0)
    {
        return WEECHAT_INTERNAL_CHARSET;
    }
    else if (string_strcasecmp (info_name, "locale") == 0)
    {
        return setlocale (LC_MESSAGES, NULL);
    }
    else if (string_strcasecmp (info_name, "inactivity") == 0)
    {
        if (gui_key_last_activity_time == 0)
            inactivity = 0;
        else
            inactivity = time (NULL) - gui_key_last_activity_time;
        snprintf (value, sizeof (value), "%ld", (long int)inactivity);
        return value;
    }
    else if (string_strcasecmp (info_name, "filters_enabled") == 0)
    {
        snprintf (value, sizeof (value), "%d", gui_filters_enabled);
        return value;
    }
    else if (string_strcasecmp (info_name, "cursor_mode") == 0)
    {
        snprintf (value, sizeof (value), "%d", gui_cursor_mode);
        return value;
    }

    /* info not found */
    return NULL;
}

/*
 * Gets infolist about WeeChat.
 *
 * Note: result must be freed with function "weechat_infolist_free".
 */

struct t_infolist *
plugin_api_infolist_get_internal (void *data, const char *infolist_name,
                                  void *pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_item *ptr_bar_item;
    struct t_gui_bar_window *ptr_bar_window;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *ptr_line;
    struct t_gui_history *ptr_history;
    struct t_gui_filter *ptr_filter;
    struct t_gui_window *ptr_window;
    struct t_gui_hotlist *ptr_hotlist;
    struct t_gui_key *ptr_key;
    struct t_weechat_plugin *ptr_plugin;
    int context, number, i;
    char *error;

    /* make C compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (string_strcasecmp (infolist_name, "bar") == 0)
    {
        /* invalid bar pointer ? */
        if (pointer && (!gui_bar_valid (pointer)))
            return NULL;

        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one bar */
                if (!gui_bar_add_to_infolist (ptr_infolist, pointer))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                /* build list with all bars matching arguments */
                for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
                {
                    if (!arguments || !arguments[0]
                        || string_match (ptr_bar->name, arguments, 0))
                    {
                        if (!gui_bar_add_to_infolist (ptr_infolist, ptr_bar))
                        {
                            infolist_free (ptr_infolist);
                            return NULL;
                        }
                    }
                }
                return ptr_infolist;
            }
        }
    }
    else if (string_strcasecmp (infolist_name, "bar_item") == 0)
    {
        /* invalid bar item pointer ? */
        if (pointer && (!gui_bar_item_valid (pointer)))
            return NULL;

        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one bar item */
                if (!gui_bar_item_add_to_infolist (ptr_infolist, pointer))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                /* build list with all bar items matching arguments */
                for (ptr_bar_item = gui_bar_items; ptr_bar_item;
                     ptr_bar_item = ptr_bar_item->next_item)
                {
                    if (!arguments || !arguments[0]
                        || string_match (ptr_bar_item->name, arguments, 0))
                    {
                        if (!gui_bar_item_add_to_infolist (ptr_infolist, ptr_bar_item))
                        {
                            infolist_free (ptr_infolist);
                            return NULL;
                        }
                    }
                }
                return ptr_infolist;
            }
        }
    }
    else if (string_strcasecmp (infolist_name, "bar_window") == 0)
    {
        /* invalid bar window pointer ? */
        if (pointer && (!gui_bar_window_valid (pointer)))
            return NULL;

        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one bar window */
                if (!gui_bar_window_add_to_infolist (ptr_infolist, pointer))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                /* build list with all bar windows (from root and window bars) */
                for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
                {
                    if (ptr_bar->bar_window)
                    {
                        if (!gui_bar_window_add_to_infolist (ptr_infolist, ptr_bar->bar_window))
                        {
                            infolist_free (ptr_infolist);
                            return NULL;
                        }
                    }
                }
                for (ptr_window = gui_windows; ptr_window;
                     ptr_window = ptr_window->next_window)
                {
                    for (ptr_bar_window = ptr_window->bar_windows;
                         ptr_bar_window;
                         ptr_bar_window = ptr_bar_window->next_bar_window)
                    {
                        if (!gui_bar_window_add_to_infolist (ptr_infolist, ptr_bar_window))
                        {
                            infolist_free (ptr_infolist);
                            return NULL;
                        }
                    }
                }
                return ptr_infolist;
            }
        }
    }
    else if (string_strcasecmp (infolist_name, "buffer") == 0)
    {
        /* invalid buffer pointer ? */
        if (pointer && (!gui_buffer_valid (pointer)))
            return NULL;

        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one buffer */
                if (!gui_buffer_add_to_infolist (ptr_infolist, pointer))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                /* build list with all buffers matching arguments */
                for (ptr_buffer = gui_buffers; ptr_buffer;
                     ptr_buffer = ptr_buffer->next_buffer)
                {
                    if (!arguments || !arguments[0]
                        || string_match (ptr_buffer->full_name, arguments, 0))
                    {
                        if (!gui_buffer_add_to_infolist (ptr_infolist, ptr_buffer))
                        {
                            infolist_free (ptr_infolist);
                            return NULL;
                        }
                    }
                }
                return ptr_infolist;
            }
        }
    }
    else if (string_strcasecmp (infolist_name, "buffer_lines") == 0)
    {
        if (!pointer)
            pointer = gui_buffers;
        else
        {
            /* invalid buffer pointer ? */
            if (!gui_buffer_valid (pointer))
                return NULL;
        }

        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            for (ptr_line = ((struct t_gui_buffer *)pointer)->own_lines->first_line;
                 ptr_line; ptr_line = ptr_line->next_line)
            {
                if (!gui_line_add_to_infolist (ptr_infolist,
                                               ((struct t_gui_buffer *)pointer)->own_lines,
                                               ptr_line))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
            return ptr_infolist;
        }
    }
    else if (string_strcasecmp (infolist_name, "filter") == 0)
    {
        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            for (ptr_filter = gui_filters; ptr_filter;
                 ptr_filter = ptr_filter->next_filter)
            {
                if (!arguments || !arguments[0]
                    || string_match (ptr_filter->name, arguments, 0))
                {
                    if (!gui_filter_add_to_infolist (ptr_infolist, ptr_filter))
                    {
                        infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
            }
            return ptr_infolist;
        }
    }
    else if (string_strcasecmp (infolist_name, "history") == 0)
    {
        /* invalid buffer pointer ? */
        if (pointer && (!gui_buffer_valid (pointer)))
            return NULL;

        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            for (ptr_history = (pointer) ?
                     ((struct t_gui_buffer *)pointer)->history : gui_history;
                 ptr_history; ptr_history = ptr_history->next_history)
            {
                if (!gui_history_add_to_infolist (ptr_infolist, ptr_history))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
            return ptr_infolist;
        }
    }
    else if (string_strcasecmp (infolist_name, "hook") == 0)
    {
        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            if (!hook_add_to_infolist (ptr_infolist, arguments))
            {
                infolist_free (ptr_infolist);
                return NULL;
            }
            return ptr_infolist;
        }
    }
    else if (string_strcasecmp (infolist_name, "hotlist") == 0)
    {
        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            for (ptr_hotlist = gui_hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if (!gui_hotlist_add_to_infolist (ptr_infolist, ptr_hotlist))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
            return ptr_infolist;
        }
    }
    else if (string_strcasecmp (infolist_name, "key") == 0)
    {
        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            if (arguments && arguments[0])
                context = gui_key_search_context (arguments);
            else
                context = GUI_KEY_CONTEXT_DEFAULT;
            if (context >= 0)
            {
                for (ptr_key = gui_keys[context]; ptr_key;
                     ptr_key = ptr_key->next_key)
                {
                    if (!gui_key_add_to_infolist (ptr_infolist, ptr_key))
                    {
                        infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
            }
            return ptr_infolist;
        }
    }
    else if (string_strcasecmp (infolist_name, "nicklist") == 0)
    {
        /* invalid buffer pointer ? */
        if (!pointer || (!gui_buffer_valid (pointer)))
            return NULL;

        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            if (!gui_nicklist_add_to_infolist (ptr_infolist, pointer, arguments))
            {
                infolist_free (ptr_infolist);
                return NULL;
            }
            return ptr_infolist;
        }
    }
    else if (string_strcasecmp (infolist_name, "option") == 0)
    {
        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            if (!config_file_add_to_infolist (ptr_infolist, arguments))
            {
                infolist_free (ptr_infolist);
                return NULL;
            }
            return ptr_infolist;
        }
    }
    else if (string_strcasecmp (infolist_name, "plugin") == 0)
    {
        /* invalid plugin pointer ? */
        if (pointer && (!plugin_valid (pointer)))
            return NULL;

        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one plugin */
                if (!plugin_add_to_infolist (ptr_infolist, pointer))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                /* build list with all plugins matching arguments */
                for (ptr_plugin = weechat_plugins; ptr_plugin;
                     ptr_plugin = ptr_plugin->next_plugin)
                {
                    if (!arguments || !arguments[0]
                        || string_match (ptr_plugin->name, arguments, 0))
                    {
                        if (!plugin_add_to_infolist (ptr_infolist, ptr_plugin))
                        {
                            infolist_free (ptr_infolist);
                            return NULL;
                        }
                    }
                }
                return ptr_infolist;
            }
        }
    }
    else if (string_strcasecmp (infolist_name, "url_options") == 0)
    {
        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            for (i = 0; url_options[i].name; i++)
            {
                if (!weeurl_option_add_to_infolist (ptr_infolist, &url_options[i]))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
            return ptr_infolist;
        }
    }
    else if (string_strcasecmp (infolist_name, "window") == 0)
    {
        /* invalid window pointer ? */
        if (pointer && (!gui_window_valid (pointer)))
            return NULL;

        ptr_infolist = infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one window */
                if (!gui_window_add_to_infolist (ptr_infolist, pointer))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
                return ptr_infolist;
            }
            else
            {
                if (arguments && arguments[0])
                {
                    if ((string_strcasecmp (arguments, "current") == 0))
                    {
                        if (gui_current_window)
                        {
                            if (!gui_window_add_to_infolist (ptr_infolist,
                                                             gui_current_window))
                            {
                                infolist_free (ptr_infolist);
                                return NULL;
                            }
                            return ptr_infolist;
                        }
                        return NULL;
                    }
                    /* check if argument is a window number */
                    error = NULL;
                    number = (int)strtol (arguments, &error, 10);
                    if (error && !error[0])
                    {
                        ptr_window = gui_window_search_by_number (number);
                        if (ptr_window)
                        {
                            if (!gui_window_add_to_infolist (ptr_infolist,
                                                             ptr_window))
                            {
                                infolist_free (ptr_infolist);
                                return NULL;
                            }
                            return ptr_infolist;
                        }
                    }
                    return NULL;
                }
                else
                {
                    /* build list with all windows */
                    for (ptr_window = gui_windows; ptr_window;
                         ptr_window = ptr_window->next_window)
                    {
                        if (!gui_window_add_to_infolist (ptr_infolist,
                                                         ptr_window))
                        {
                            infolist_free (ptr_infolist);
                            return NULL;
                        }
                    }
                    return ptr_infolist;
                }
            }
        }
    }

    /* infolist not found */
    return NULL;
}

/*
 * Moves item pointer to next item in an infolist.
 *
 * Returns:
 *   1: pointer is still OK
 *   0: end of infolist was reached
 */

int
plugin_api_infolist_next (struct t_infolist *infolist)
{
    if (!infolist || !infolist_valid (infolist))
        return 0;

    return (infolist_next (infolist)) ? 1 : 0;
}

/*
 * Moves pointer to previous item in an infolist.
 *
 * Returns:
 *   1: pointer is still OK
 *   0: beginning of infolist was reached
 */

int
plugin_api_infolist_prev (struct t_infolist *infolist)
{
    if (!infolist || !infolist_valid (infolist))
        return 0;

    return (infolist_prev (infolist)) ? 1 : 0;
}

/*
 * Resets item cursor in infolist.
 */

void
plugin_api_infolist_reset_item_cursor (struct t_infolist *infolist)
{
    if (infolist && infolist_valid (infolist))
    {
        infolist_reset_item_cursor (infolist);
    }
}

/*
 * Gets list of fields for current infolist item.
 */

const char *
plugin_api_infolist_fields (struct t_infolist *infolist)
{
    if (!infolist || !infolist_valid (infolist))
        return NULL;

    return infolist_fields (infolist);
}

/*
 * Gets integer value for a variable in current infolist item.
 */

int
plugin_api_infolist_integer (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return 0;

    return infolist_integer (infolist, var);
}

/*
 * Gets string value for a variable in current infolist item.
 */

const char *
plugin_api_infolist_string (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return NULL;

    return infolist_string (infolist, var);
}

/*
 * Gets pointer value for a variable in current infolist item.
 */

void *
plugin_api_infolist_pointer (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return NULL;

    return infolist_pointer (infolist, var);
}

/*
 * Gets buffer value for a variable in current infolist item.
 *
 * Argument "size" is set with the size of buffer.
 */

void *
plugin_api_infolist_buffer (struct t_infolist *infolist, const char *var,
                            int *size)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return NULL;

    return infolist_buffer (infolist, var, size);
}

/*
 * Gets time value for a variable in current infolist item.
 */

time_t
plugin_api_infolist_time (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return 0;

    return infolist_time (infolist, var);
}

/*
 * Frees an infolist.
 */

void
plugin_api_infolist_free (struct t_infolist *infolist)
{
    if (infolist && infolist_valid (infolist))
        infolist_free (infolist);
}

/*
 * Initializes plugin API.
 */

void
plugin_api_init ()
{
    /* WeeChat core info hooks */
    hook_info (NULL, "version", N_("WeeChat version"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "version_number", N_("WeeChat version (as number)"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "date", N_("WeeChat compilation date"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "dir_separator", N_("directory separator"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_dir", N_("WeeChat directory"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_libdir", N_("WeeChat \"lib\" directory"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_sharedir", N_("WeeChat \"share\" directory"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_localedir", N_("WeeChat \"locale\" directory"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_site", N_("WeeChat site"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_site_download", N_("WeeChat site, download page"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_upgrading", N_("1 if WeeChat is upgrading (command `/upgrade`)"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "charset_terminal", N_("terminal charset"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "charset_internal", N_("WeeChat internal charset"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "locale", N_("locale used for translating messages"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "inactivity", N_("keyboard inactivity (seconds)"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "filters_enabled", N_("1 if filters are enabled"), NULL,
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "cursor_mode", N_("1 if cursor mode is enabled"), NULL,
               &plugin_api_info_get_internal, NULL);

    /* WeeChat core infolist hooks */
    hook_infolist (NULL, "bar", N_("list of bars"),
                   N_("bar pointer (optional)"),
                   N_("bar name (can start or end with \"*\" as wildcard) (optional)"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "bar_item", N_("list of bar items"),
                   N_("bar item pointer (optional)"),
                   N_("bar item name (can start or end with \"*\" as wildcard) (optional)"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "bar_window", N_("list of bar windows"),
                   N_("bar window pointer (optional)"),
                   NULL,
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "buffer", N_("list of buffers"),
                   N_("buffer pointer (optional)"),
                   N_("buffer name (can start or end with \"*\" as wildcard) (optional)"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "buffer_lines", N_("lines of a buffer"),
                   N_("buffer pointer"),
                   NULL,
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "filter", N_("list of filters"),
                   NULL,
                   N_("filter name (can start or end with \"*\" as wildcard) (optional)"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "history", N_("history of commands"),
                   N_("buffer pointer (if not set, return global history) (optional)"),
                   NULL,
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "hook", N_("list of hooks"),
                   NULL,
                   N_("type,arguments (type is command/timer/.., arguments to "
                      "get only some hooks (can start or end with \"*\" as "
                      "wildcard), both are optional)"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "hotlist", N_("list of buffers in hotlist"),
                   NULL,
                   NULL,
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "key", N_("list of key bindings"),
                   NULL,
                   N_("context (\"default\", \"search\", \"cursor\" or "
                      "\"mouse\") (optional)"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "nicklist", N_("nicks in nicklist for a buffer"),
                   N_("buffer pointer"),
                   N_("nick_xxx or group_xxx to get only nick/group xxx "
                      "(optional)"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "option", N_("list of options"),
                   NULL,
                   N_("option name (can start or end with \"*\" as wildcard) (optional)"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "plugin", N_("list of plugins"),
                   N_("plugin pointer (optional)"),
                   N_("plugin name (can start or end with \"*\" as wildcard) (optional)"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "url_options", N_("options for URL"),
                   NULL,
                   NULL,
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "window", N_("list of windows"),
                   N_("window pointer (optional)"),
                   N_("\"current\" for current window or a window number (optional)"),
                   &plugin_api_infolist_get_internal, NULL);

    /* WeeChat core hdata */
    hook_hdata (NULL, "bar", N_("bar"),
                &gui_bar_hdata_bar_cb, NULL);
    hook_hdata (NULL, "bar_item", N_("bar item"),
                &gui_bar_item_hdata_bar_item_cb, NULL);
    hook_hdata (NULL, "bar_window", N_("bar window"),
                &gui_bar_window_hdata_bar_window_cb, NULL);
    hook_hdata (NULL, "buffer", N_("buffer"),
                &gui_buffer_hdata_buffer_cb, NULL);
    hook_hdata (NULL, "completion", N_("structure with completion"),
                &gui_completion_hdata_completion_cb, NULL);
    hook_hdata (NULL, "completion_partial", N_("structure with partial completion"),
                &gui_completion_hdata_completion_partial_cb, NULL);
    hook_hdata (NULL, "config_file", N_("config file"),
                &config_file_hdata_config_file_cb, NULL);
    hook_hdata (NULL, "config_section", N_("config section"),
                &config_file_hdata_config_section_cb, NULL);
    hook_hdata (NULL, "config_option", N_("config option"),
                &config_file_hdata_config_option_cb, NULL);
    hook_hdata (NULL, "filter", N_("filter"),
                &gui_filter_hdata_filter_cb, NULL);
    hook_hdata (NULL, "history", N_("history of commands in buffer"),
                &gui_history_hdata_history_cb, NULL);
    hook_hdata (NULL, "hotlist", N_("hotlist"),
                &gui_hotlist_hdata_hotlist_cb, NULL);
    hook_hdata (NULL, "input_undo", N_("structure with undo for input line"),
                &gui_buffer_hdata_input_undo_cb, NULL);
    hook_hdata (NULL, "key", N_("a key (keyboard shortcut)"),
                &gui_key_hdata_key_cb, NULL);
    hook_hdata (NULL, "lines", N_("structure with lines"),
                &gui_line_hdata_lines_cb, NULL);
    hook_hdata (NULL, "line", N_("structure with one line"),
                &gui_line_hdata_line_cb, NULL);
    hook_hdata (NULL, "line_data", N_("structure with one line data"),
                &gui_line_hdata_line_data_cb, NULL);
    hook_hdata (NULL, "nick_group", N_("group in nicklist"),
                &gui_nicklist_hdata_nick_group_cb, NULL);
    hook_hdata (NULL, "nick", N_("nick in nicklist"),
                &gui_nicklist_hdata_nick_cb, NULL);
    hook_hdata (NULL, "plugin", N_("plugin"),
                &plugin_hdata_plugin_cb, NULL);
    hook_hdata (NULL, "window", N_("window"),
                &gui_window_hdata_window_cb, NULL);
    hook_hdata (NULL, "window_scroll", N_("scroll info in window"),
                &gui_window_hdata_window_scroll_cb, NULL);
    hook_hdata (NULL, "window_tree", N_("tree of windows"),
                &gui_window_hdata_window_tree_cb, NULL);
}
