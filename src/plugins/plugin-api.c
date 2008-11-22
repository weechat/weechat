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

/* plugin-api.c: WeeChat <--> plugin API (extra functions) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-input.h"
#include "../core/wee-string.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "plugin.h"
#include "plugin-config.h"


/*
 * plugin_api_charset_set: set plugin charset
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
 * plugin_api_gettext: translate a string using gettext
 */

const char *
plugin_api_gettext (const char *string)
{
    return _(string);
}

/*
 * plugin_api_ngettext: translate a string using gettext
 */

const char *
plugin_api_ngettext (const char *single, const char *plural, int count)
{
    return NG_(single, plural, count);
}

/*
 * plugin_api_config_get: get value of an option
 */

struct t_config_option *
plugin_api_config_get (const char *option_name)
{
    struct t_config_option *ptr_option;
    
    config_file_search_with_string (option_name, NULL, NULL, &ptr_option, NULL);
    
    return ptr_option;
}

/*
 * plugin_api_config_get_plugin: get value of a plugin config option
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
 * plugin_api_config_set_plugin: set value of a plugin config option
 */

int
plugin_api_config_set_plugin (struct t_weechat_plugin *plugin,
                              const char *option_name, const char *value)
{
    if (!plugin || !option_name)
        return 0;
    
    if (plugin_config_set (plugin->name, option_name, value))
        return 1;
    
    return 0;
}

/*
 * plugin_api_prefix: return a prefix for display with printf
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
 * plugin_api_color: return a WeeChat color for display with printf
 */

const char *
plugin_api_color (const char *color_name)
{
    int num_color;
    
    if (!color_name)
        return GUI_NO_COLOR;
    
    /* name is a weechat color option ? => then return this color */
    num_color = gui_color_search_config_int (color_name);
    if (num_color >= 0)
        return GUI_COLOR(num_color);

    return gui_color_get_custom (color_name);
}

/*
 * plugin_api_command: execute a command (simulate user entry)
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
 * plugin_api_info_get_inernal: get info about WeeChat
 */

const char *
plugin_api_info_get_internal (void *data, const char *info_name,
                              const char *arguments)
{
    time_t inactivity;
    static char value[32];
    
    /* make C compiler happy */
    (void) data;
    (void) arguments;
    
    if (!info_name)
        return NULL;
    
    if (string_strcasecmp (info_name, "version") == 0)
    {
        return PACKAGE_VERSION;
    }
    if (string_strcasecmp (info_name, "date") == 0)
    {
        return __DATE__;
    }
    else if (string_strcasecmp (info_name, "dir_separator") == 0)
    {
        return DIR_SEPARATOR;
    }
    else if (string_strcasecmp (info_name, "weechat_dir") == 0)
    {
        return weechat_home;
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
    else if (string_strcasecmp (info_name, "charset_terminal") == 0)
    {
        return weechat_local_charset;
    }
    else if (string_strcasecmp (info_name, "charset_internal") == 0)
    {
        return WEECHAT_INTERNAL_CHARSET;
    }
    else if (string_strcasecmp (info_name, "inactivity") == 0)
    {
        if (gui_keyboard_last_activity_time == 0)
            inactivity = 0;
        else
            inactivity = time (NULL) - gui_keyboard_last_activity_time;
        snprintf (value, sizeof (value), "%ld", (long int)inactivity);
        return value;
    }
    else if (string_strcasecmp (info_name, "filters_enabled") == 0)
    {
        snprintf (value, sizeof (value), "%d", gui_filters_enabled);
        return value;
    }
    
    /* info not found */
    return NULL;
}

/*
 * plugin_api_infolist_get_internal: get list with infos about WeeChat structures
 *                                   WARNING: caller has to free string returned
 *                                            by this function after use, with
 *                                            weechat_infolist_free()
 */

struct t_infolist *
plugin_api_infolist_get_internal (void *data, const char *infolist_name,
                                  void *pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *ptr_line;
    struct t_gui_filter *ptr_filter;
    struct t_gui_window *ptr_window;
    struct t_gui_hotlist *ptr_hotlist;
    struct t_weechat_plugin *ptr_plugin;
    
    /* make C compiler happy */
    (void) data;
    
    if (!infolist_name || !infolist_name[0])
        return NULL;
    
    if (string_strcasecmp (infolist_name, "buffer") == 0)
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
                /* build list with all buffers */
                for (ptr_buffer = gui_buffers; ptr_buffer;
                     ptr_buffer = ptr_buffer->next_buffer)
                {
                    if (!gui_buffer_add_to_infolist (ptr_infolist, ptr_buffer))
                    {
                        infolist_free (ptr_infolist);
                        return NULL;
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
            for (ptr_line = ((struct t_gui_buffer *)pointer)->lines; ptr_line;
                 ptr_line = ptr_line->next_line)
            {
                if (!gui_buffer_line_add_to_infolist (ptr_infolist,
                                                      pointer, ptr_line))
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
                if (!gui_filter_add_to_infolist (ptr_infolist, ptr_filter))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
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
            if (!gui_nicklist_add_to_infolist (ptr_infolist, pointer))
            {
                infolist_free (ptr_infolist);
                return NULL;
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
                    if ((string_strcasecmp (arguments, "current") == 0)
                        && gui_current_window)
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
                /* build list with all plugins */
                for (ptr_plugin = weechat_plugins; ptr_plugin;
                     ptr_plugin = ptr_plugin->next_plugin)
                {
                    if (!plugin_add_to_infolist (ptr_infolist, ptr_plugin))
                    {
                        infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
                return ptr_infolist;
            }
        }
    }
    
    /* infolist not found */
    return NULL;
}

/*
 * plugin_api_infolist_next: move item pointer to next item in a list
 *                           return 1 if pointer is still ok
 *                                  0 if end of list was reached
 */

int
plugin_api_infolist_next (struct t_infolist *infolist)
{
    if (!infolist || !infolist_valid (infolist))
        return 0;
    
    return (infolist_next (infolist)) ? 1 : 0;
}

/*
 * plugin_api_infolist_prev: move item pointer to previous item in a list
 *                           return 1 if pointer is still ok
 *                                  0 if beginning of list was reached
 */

int
plugin_api_infolist_prev (struct t_infolist *infolist)
{
    if (!infolist || !infolist_valid (infolist))
        return 0;

    return (infolist_prev (infolist)) ? 1 : 0;
}

/*
 * plugin_api_infolist_reset_item_cursor: reset item cursor in infolist
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
 * plugin_api_infolist_fields: get list of fields for current list item
 */

const char *
plugin_api_infolist_fields (struct t_infolist *infolist)
{
    if (!infolist || !infolist_valid (infolist))
        return NULL;
    
    return infolist_fields (infolist);
}

/*
 * plugin_api_infolist_integer: get an integer variable value in current list item
 */

int
plugin_api_infolist_integer (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return 0;
    
    return infolist_integer (infolist, var);
}

/*
 * plugin_api_infolist_string: get a string variable value in current list item
 */

const char *
plugin_api_infolist_string (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return NULL;
    
    return infolist_string (infolist, var);
}

/*
 * plugin_api_infolist_pointer: get a pointer variable value in current list item
 */

void *
plugin_api_infolist_pointer (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return NULL;
    
    return infolist_pointer (infolist, var);
}

/*
 * plugin_api_infolist_buffer: get a buffer variable value in current list item
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
 * plugin_api_infolist_time: get a time variable value in current list item
 */

time_t
plugin_api_infolist_time (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return 0;
    
    return infolist_time (infolist, var);
}

/*
 * plugin_api_infolist_free: free an infolist
 */

void
plugin_api_infolist_free (struct t_infolist *infolist)
{
    if (infolist && infolist_valid (infolist))
        infolist_free (infolist);
}

/*
 * plugin_api_init: init plugin API
 */

void
plugin_api_init ()
{
    /* WeeChat core info hooks */
    hook_info (NULL, "version", N_("WeeChat version"),
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "date", N_("WeeChat compilation date"),
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "dir_separator", N_("directory separator"),
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_dir", N_("WeeChat directory"),
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_libdir", N_("WeeChat \"lib\" directory"),
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_sharedir", N_("WeeChat \"share\" directory"),
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "weechat_localedir", N_("WeeChat \"locale\" directory"),
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "charset_terminal", N_("terminal charset"),
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "charset_internal", N_("WeeChat internal charset"),
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "inactivity", N_("keyboard inactivity (seconds)"),
               &plugin_api_info_get_internal, NULL);
    hook_info (NULL, "filters_enabled", N_("1 if filters are enabled"),
               &plugin_api_info_get_internal, NULL);
    
    /* WeeChat core infolist hooks */
    hook_infolist (NULL, "buffer", N_("list of buffers"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "buffer_lines", N_("lines of a buffer"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "filter", N_("list of filters"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "nicklist", N_("nicks in nicklist for a buffer"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "window", N_("list of windows"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "hotlist", N_("list of buffers in hotlist"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "option", N_("list of options"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "hook", N_("list of hooks"),
                   &plugin_api_infolist_get_internal, NULL);
    hook_infolist (NULL, "plugin", N_("list of plugins"),
                   &plugin_api_infolist_get_internal, NULL);
}
