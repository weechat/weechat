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

/* plugin-api.c: WeeChat <--> plugin API */


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

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-input.h"
#include "../core/wee-string.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-infobar.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-window.h"
#include "plugin.h"
#include "plugin-config.h"
#include "plugin-infolist.h"


/*
 * plugin_api_charset_set: set plugin charset
 */

void
plugin_api_charset_set (struct t_weechat_plugin *plugin, char *charset)
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

char *
plugin_api_gettext (char *string)
{
    return _(string);
}

/*
 * plugin_api_ngettext: translate a string using gettext
 */

char *
plugin_api_ngettext (char *single, char *plural, int count)
{
    return NG_(single, plural, count);
}

/*
 * plugin_api_mkdir_home: create a directory in WeeChat home
 */

int
plugin_api_mkdir_home (char *directory, int mode)
{
    char *dir_name;
    int dir_length;
    
    if (!directory)
        return 0;
    
    /* build directory, adding WeeChat home */
    dir_length = strlen (weechat_home) + strlen (directory) + 2;
    dir_name =
        (char *)malloc (dir_length * sizeof (char));
    if (!dir_name)
        return 0;
    
    snprintf (dir_name, dir_length, "%s/%s", weechat_home, directory);
    
    if (mkdir (dir_name, mode) < 0)
    {
        if (errno != EEXIST)
        {
            free (dir_name);
            return 0;
        }
    }
    
    free (dir_name);
    return 1;
}

/*
 * plugin_api_mkdir: create a directory
 */

int
plugin_api_mkdir (char *directory, int mode)
{
    if (!directory)
        return 0;
    
    if (mkdir (directory, mode) < 0)
    {
        if (errno != EEXIST)
            return 0;
    }
    
    return 1;
}

/*
 * plugin_api_config_get_weechat: get value of a WeeChat config option
 */

struct t_config_option *
plugin_api_config_get_weechat (char *option_name)
{
    return config_file_search_option (weechat_config_file, NULL,
                                      option_name);
}

/*
 * plugin_api_config_get_plugin: get value of a plugin config option
 */

char *
plugin_api_config_get_plugin (struct t_weechat_plugin *plugin,
                              char *option_name)
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
                              char *option_name, char *value)
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

char *
plugin_api_prefix (char *prefix)
{
    if (!prefix)
        return gui_chat_prefix_empty;
    
    if (string_strcasecmp (prefix, "info") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_INFO];
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

char *
plugin_api_color (char *color_name)
{
    int num_color;
    
    if (!color_name)
        return GUI_NO_COLOR;

    num_color = gui_color_search_config (color_name);
    if (num_color >= 0)
        return GUI_COLOR(num_color);
    
    return GUI_NO_COLOR;
}

/*
 * plugin_api_infobar_printf: print a message in infobar
 */

void
plugin_api_infobar_printf (struct t_weechat_plugin *plugin, int delay,
                           char *color_name, char *format, ...)
{
    va_list argptr;
    static char buf[1024];
    char *buf2;
    int num_color;
    
    if (!plugin || !format)
        return;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    buf2 = string_iconv_to_internal (plugin->charset, buf);
    if (color_name && color_name[0])
    {
        num_color = gui_color_search_config (color_name);
        if (num_color < 0)
            num_color = GUI_COLOR_INFOBAR;
    }
    else
        num_color = GUI_COLOR_INFOBAR;
    
    gui_infobar_printf (delay, num_color,
                        "%s",
                        (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * plugin_api_infobar_remove: remove message(s) in infobar
 */

void
plugin_api_infobar_remove (int how_many)
{
    if (how_many <= 0)
        gui_infobar_remove_all ();
    else
    {
        while ((gui_infobar) && (how_many > 0))
        {
            gui_infobar_remove ();
            how_many--;
        }
    }
    gui_infobar_draw (gui_current_window->buffer, 1);
}

/*
 * plugin_api_command: execute a command (simulate user entry)
 */

void
plugin_api_command (struct t_weechat_plugin *plugin,
                    struct t_gui_buffer *buffer, char *command)
{
    char *command2;
    
    if (!plugin || !command)
        return;
    
    command2 = string_iconv_to_internal (plugin->charset, command);
    if (!buffer)
        buffer = gui_current_window->buffer;
    input_data (buffer, (command2) ? command2 : command, 0);
    if (command2)
        free (command2);
}

/*
 * plugin_api_info_get: get info about WeeChat
 */

char *
plugin_api_info_get (struct t_weechat_plugin *plugin, char *info)
{
    time_t inactivity;
    static char keyboard_inactivity[32];
    
    if (!plugin || !info)
        return NULL;
    
    if (string_strcasecmp (info, "version") == 0)
    {
        return PACKAGE_VERSION;
    }
    if (string_strcasecmp (info, "date") == 0)
    {
        return __DATE__;
    }
    else if (string_strcasecmp (info, "dir_separator") == 0)
    {
        return DIR_SEPARATOR;
    }
    else if (string_strcasecmp (info, "weechat_dir") == 0)
    {
        return weechat_home;
    }
    else if (string_strcasecmp (info, "weechat_libdir") == 0)
    {
        return WEECHAT_LIBDIR;
    }
    else if (string_strcasecmp (info, "weechat_sharedir") == 0)
    {
        return WEECHAT_SHAREDIR;
    }
    else if (string_strcasecmp (info, "charset_terminal") == 0)
    {
        return local_charset;
    }
    else if (string_strcasecmp (info, "charset_internal") == 0)
    {
        return WEECHAT_INTERNAL_CHARSET;
    }
    else if (string_strcasecmp (info, "inactivity") == 0)
    {
        if (gui_keyboard_last_activity_time == 0)
            inactivity = 0;
        else
            inactivity = time (NULL) - gui_keyboard_last_activity_time;
        snprintf (keyboard_inactivity, sizeof (keyboard_inactivity),
                  "%ld", (long int)inactivity);
        return keyboard_inactivity;
    }
    
    /* info not found */
    return NULL;
}

/*
 * plugin_api_infolist_get_add_buffer: add a buffer in a list
 *                                     return 1 if ok, 0 if error
 */

int
plugin_api_infolist_get_add_buffer (struct t_plugin_infolist *infolist,
                                    struct t_gui_buffer *buffer)
{
    struct t_plugin_infolist_item *ptr_item;
    
    if (!infolist || !buffer)
        return 0;
    
    ptr_item = plugin_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!plugin_infolist_new_var_pointer (ptr_item, "pointer", buffer))
        return 0;
    if (!plugin_infolist_new_var_integer (ptr_item, "number", buffer->number))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "category", buffer->category))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "name", buffer->name))
        return 0;
    if (!plugin_infolist_new_var_integer (ptr_item, "type", buffer->type))
        return 0;
    if (!plugin_infolist_new_var_integer (ptr_item, "notify_level", buffer->notify_level))
        return 0;
    if (!plugin_infolist_new_var_integer (ptr_item, "num_displayed", buffer->num_displayed))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "title", buffer->title))
        return 0;
    if (!plugin_infolist_new_var_integer (ptr_item, "input", buffer->input))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "input_nick", buffer->input_nick))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "input_string", buffer->input_buffer))
        return 0;
    
    return 1;
}

/*
 * plugin_api_infolist_get_add_buffer_line: add a buffer line in a list
 *                                          return 1 if ok, 0 if error
 */

int
plugin_api_infolist_get_add_buffer_line (struct t_plugin_infolist *infolist,
                                         struct t_gui_line *line)
{
    struct t_plugin_infolist_item *ptr_item;
    
    if (!infolist || !line)
        return 0;
    
    ptr_item = plugin_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!plugin_infolist_new_var_time (ptr_item, "date", line->date))
        return 0;
    if (!plugin_infolist_new_var_time (ptr_item, "date_printed", line->date))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "str_time", line->str_time))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "prefix", line->prefix))
        return 0;
    if (!plugin_infolist_new_var_string (ptr_item, "message", line->message))
        return 0;
    
    return 1;
}

/*
 * plugin_api_infolist_get: get list with infos about WeeChat structures
 *                          WARNING: caller has to free string returned
 *                          by this function after use, with weechat_infolist_free()
 */

struct t_plugin_infolist *
plugin_api_infolist_get (char *name, void *pointer)
{
    struct t_plugin_infolist *ptr_infolist;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *ptr_line;
    
    if (!name || !name[0])
        return NULL;
    
    if (string_strcasecmp (name, "buffer") == 0)
    {
        /* invalid buffer pointer ? */
        if (pointer && (!gui_buffer_valid (pointer)))
            return NULL;
        
        ptr_infolist = plugin_infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one buffer */
                if (!plugin_api_infolist_get_add_buffer (ptr_infolist, pointer))
                {
                    plugin_infolist_free (ptr_infolist);
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
                    if (!plugin_api_infolist_get_add_buffer (ptr_infolist,
                                                             ptr_buffer))
                    {
                        plugin_infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
                return ptr_infolist;
            }
        }
    }
    else if (string_strcasecmp (name, "buffer_lines") == 0)
    {
        if (!pointer)
            pointer = gui_buffers;
        else
        {
            /* invalid buffer pointer ? */
            if (!gui_buffer_valid (pointer))
                return NULL;
        }
        
        ptr_infolist = plugin_infolist_new ();
        if (ptr_infolist)
        {
            for (ptr_line = ((struct t_gui_buffer *)pointer)->lines; ptr_line;
                 ptr_line = ptr_line->next_line)
            {
                if (!plugin_api_infolist_get_add_buffer_line (ptr_infolist,
                                                              ptr_line))
                {
                    plugin_infolist_free (ptr_infolist);
                    return NULL;
                }
            }
            return ptr_infolist;
        }
    }
    
    /* list not found */
    return NULL;
}

/*
 * plugin_api_infolist_next: move item pointer to next item in a list
 *                           return 1 if pointer is still ok
 *                                  0 if end of list was reached
 */

int
plugin_api_infolist_next (struct t_plugin_infolist *infolist)
{
    if (!infolist || !plugin_infolist_valid (infolist))
        return 0;
    
    return (plugin_infolist_next_item (infolist)) ? 1 : 0;
}

/*
 * plugin_api_infolist_prev: move item pointer to previous item in a list
 *                           return 1 if pointer is still ok
 *                                  0 if beginning of list was reached
 */

int
plugin_api_infolist_prev (struct t_plugin_infolist *infolist)
{
    if (!infolist || !plugin_infolist_valid (infolist))
        return 0;

    return (plugin_infolist_prev_item (infolist)) ? 1 : 0;
}

/*
 * plugin_api_infolist_fields: get list of fields for current list item
 */

char *
plugin_api_infolist_fields (struct t_plugin_infolist *infolist)
{
    if (!infolist || !plugin_infolist_valid (infolist))
        return NULL;
    
    return plugin_infolist_get_fields (infolist);
}

/*
 * plugin_api_infolist_integer: get an integer variable value in current list item
 */

int
plugin_api_infolist_integer (struct t_plugin_infolist *infolist, char *var)
{
    if (!infolist || !plugin_infolist_valid (infolist)
        || !((struct t_plugin_infolist *)infolist)->ptr_item)
        return 0;
    
    return plugin_infolist_get_integer (infolist, var);
}

/*
 * plugin_api_infolist_string: get a string variable value in current list item
 */

char *
plugin_api_infolist_string (struct t_plugin_infolist *infolist, char *var)
{
    if (!infolist || !plugin_infolist_valid (infolist)
        || !((struct t_plugin_infolist *)infolist)->ptr_item)
        return NULL;
    
    return plugin_infolist_get_string (infolist, var);
}

/*
 * plugin_api_infolist_pointer: get a pointer variable value in current list item
 */

void *
plugin_api_infolist_pointer (struct t_plugin_infolist *infolist, char *var)
{
    if (!infolist || !plugin_infolist_valid (infolist)
        || !((struct t_plugin_infolist *)infolist)->ptr_item)
        return NULL;
    
    return plugin_infolist_get_pointer (infolist, var);
}

/*
 * plugin_api_infolist_time: get a time variable value in current list item
 */

time_t
plugin_api_infolist_time (struct t_plugin_infolist *infolist, char *var)
{
    if (!infolist || !plugin_infolist_valid (infolist)
        || !((struct t_plugin_infolist *)infolist)->ptr_item)
        return 0;
    
    return plugin_infolist_get_time (infolist, var);
}

/*
 * plugin_api_infolist_free: free an infolist
 */

void
plugin_api_infolist_free (struct t_plugin_infolist *infolist)
{
    if (infolist && plugin_infolist_valid (infolist))
        plugin_infolist_free (infolist);
}
