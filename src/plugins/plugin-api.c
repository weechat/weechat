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
#include "../core/wee-command.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-input.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../core/wee-util.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-infobar.h"
#include "../gui/gui-input.h"
#include "../gui/gui-keyboard.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-status.h"
#include "../gui/gui-window.h"
#include "plugin.h"
#include "plugin-config.h"
#include "plugin-list.h"


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
 * plugin_api_iconv_to_internal: encode string from a charset to WeeChat
 *                               internal charset
 */

char *
plugin_api_iconv_to_internal (struct t_weechat_plugin *plugin,
                              char *charset, char *string)
{
    if (!plugin || !string)
        return NULL;
    
    return string_iconv_to_internal (charset, string);
}

/*
 * plugin_api_iconv_from_internal: encode string from WeeChat internal
 *                                 charset to another
 */

char *
plugin_api_iconv_from_internal (struct t_weechat_plugin *plugin,
                                char *charset, char *string)
{
    if (!plugin || !string)
        return NULL;
    
    return string_iconv_from_internal (charset, string);
}

/*
 * plugin_api_gettext: translate a string using gettext
 */

char *
plugin_api_gettext (struct t_weechat_plugin *plugin, char *string)
{
    /* make C compiler happy */
    (void) plugin;
    
    return _(string);
}

/*
 * plugin_api_ngettext: translate a string using gettext
 */

char *
plugin_api_ngettext (struct t_weechat_plugin *plugin, char *single,
                     char *plural, int count)
{
    /* make C compiler happy */
    (void) plugin;
    
    return NG_(single, plural, count);
}

/*
 * plugin_api_strcasecmp: locale and case independent string comparison
 */

int
plugin_api_strcasecmp (struct t_weechat_plugin *plugin,
                       char *string1, char *string2)
{
    /* make C compiler happy */
    (void) plugin;
    
    return string_strcasecmp (string1, string2);
}

/*
 * plugin_api_strncasecmp: locale and case independent string comparison
 *                             with max length
 */

int
plugin_api_strncasecmp (struct t_weechat_plugin *plugin,
                        char *string1, char *string2, int max)
{
    /* make C compiler happy */
    (void) plugin;
    
    return string_strncasecmp (string1, string2, max);
}

/*
 * plugin_api_string_replace: replace a string by new one in a string
 */

char *
plugin_api_string_replace (struct t_weechat_plugin *plugin,
                           char *string, char *search, char *replace)
{
    /* make C compiler happy */
    (void) plugin;
    
    return string_replace (string, search, replace);
}

/*
 * plugin_api_string_explode: explode a string
 */

char **
plugin_api_string_explode (struct t_weechat_plugin *plugin, char *string,
                           char *separators, int keep_eol,
                           int num_items_max, int *num_items)
{
    if (!plugin || !string || !separators || !num_items)
        return NULL;
    
    return string_explode (string, separators, keep_eol,
                           num_items_max, num_items);
}

/*
 * plugin_api_string_free_exploded: free exploded string
 */

void
plugin_api_string_free_exploded (struct t_weechat_plugin *plugin,
                                 char **exploded_string)
{
    /* make C compiler happy */
    (void) plugin;
    
    string_free_exploded (exploded_string);
}

/*
 * plugin_api_mkdir_home: create a directory in WeeChat home
 */

int
plugin_api_mkdir_home (struct t_weechat_plugin *plugin, char *directory)
{
    char *dir_name;
    int dir_length;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (!directory)
        return 0;
    
    /* build directory, adding WeeChat home */
    dir_length = strlen (weechat_home) + strlen (directory) + 2;
    dir_name =
        (char *) malloc (dir_length * sizeof (char));
    if (!dir_name)
        return 0;
    
    snprintf (dir_name, dir_length, "%s/%s", weechat_home, directory);
    
    if (mkdir (dir_name, 0755) < 0)
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
 * plugin_api_exec_on_files: find files in a directory and execute a
 *                           function on each file
 */

void
plugin_api_exec_on_files (struct t_weechat_plugin *plugin, char *directory,
                          int (*callback)(char *))
{
    /* make C compiler happy */
    (void) plugin;
    
    if (directory && callback)
        util_exec_on_files (directory, callback);
}

/*
 * plugin_api_prefix: return a prefix for display with printf
 */

char *
plugin_api_prefix (struct t_weechat_plugin *plugin, char *prefix)
{
    static char empty_prefix[] = "";
    
    if (!plugin || !prefix)
        return empty_prefix;
    
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
    
    return empty_prefix;
}

/*
 * plugin_api_color: return a WeeChat color for display with printf
 */

char *
plugin_api_color (struct t_weechat_plugin *plugin, char *color_name)
{
    int num_color;
    
    if (!plugin || !color_name)
        return GUI_NO_COLOR;

    num_color = gui_color_search_config (color_name);
    if (num_color >= 0)
        return GUI_COLOR(num_color);
    
    return GUI_NO_COLOR;
}

/*
 * plugin_api_printf: print a message on a buffer
 */

void
plugin_api_printf (struct t_weechat_plugin *plugin,
                   void *buffer, char *format, ...)
{
    va_list argptr;
    char buf[8192];
    
    if (!plugin || !format
        || !gui_buffer_valid ((struct t_gui_buffer *)buffer))
        return;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    gui_chat_printf ((struct t_gui_buffer *)buffer, "%s", buf);
}

/*
 * plugin_api_printf_date: print a message on a buffer with a specific date
 */

void
plugin_api_printf_date (struct t_weechat_plugin *plugin,
                        void *buffer, time_t date, char *format, ...)
{
    va_list argptr;
    char buf[8192];
    
    if (!plugin || !format
        || !gui_buffer_valid ((struct t_gui_buffer *)buffer))
        return;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    gui_chat_printf_date ((struct t_gui_buffer *)buffer, date, buf);
}

/*
 * plugin_api_log_printf: print a message in WeeChat log file
 */

void
plugin_api_log_printf (struct t_weechat_plugin *plugin, char *format, ...)
{
    va_list argptr;
    char buf[8192];
    
    if (!plugin || !format)
        return;
    
    va_start (argptr, format);
    vsnprintf (buf, sizeof (buf) - 1, format, argptr);
    va_end (argptr);
    
    log_printf ("%s", buf);
}

/*
 * plugin_api_print_infobar: print a message in infobar
 */

void
plugin_api_print_infobar (struct t_weechat_plugin *plugin, int time_displayed,
                          char *message, ...)
{
    (void) plugin;
    (void) time_displayed;
    (void) message;
    
    /*va_list argptr;
    static char buf[1024];
    char *buf2;
    
    if (!plugin || (time_displayed < 0) || !message)
        return;
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    
    buf2 = string_iconv_to_internal (plugin->charset, buf);
    gui_infobar_printf (time_displayed, GUI_COLOR_WIN_INFOBAR, "%s",
                        (buf2) ? buf2 : buf);
    if (buf2)
    free (buf2);*/
}

/*
 * plugin_api_infobar_remove: remove message(s) in infobar
 */

void
plugin_api_infobar_remove (struct t_weechat_plugin *plugin, int how_many)
{
    if (!plugin)
        return;
    
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
 * plugin_api_hook_command: hook a command
 */

struct t_hook *
plugin_api_hook_command (struct t_weechat_plugin *plugin, char *command,
                         char *description, char *args,
                         char *args_desc, char *completion,
                         int (*callback)(void *, void *, int, char **, char **),
                         void *data)
{
    if (plugin && callback)
        return hook_command (plugin, command, description, args,
                             args_desc, completion,
                             callback, data);
    
    return NULL;
}

/*
 * plugin_api_hook_timer: hook a timer
 */

struct t_hook *
plugin_api_hook_timer (struct t_weechat_plugin *plugin, long interval,
                       int max_calls, int (*callback)(void *), void *data)
{
    if (plugin && (interval > 0) && callback)
        return hook_timer (plugin, interval, max_calls, callback, data);
    
    return NULL;
}

/*
 * plugin_api_hook_fd: hook a file descriptor
 */

struct t_hook *
plugin_api_hook_fd (struct t_weechat_plugin *plugin, int fd,
                    int flag_read, int flag_write, int flag_exception,
                    int (*callback)(void *), void *data)
{
    int flags;
    
    if (plugin && (fd >= 0) && callback)
    {
        flags = 0;
        if (flag_read)
            flags |= HOOK_FD_FLAG_READ;
        if (flag_write)
            flags |= HOOK_FD_FLAG_WRITE;
        if (flag_exception)
            flags |= HOOK_FD_FLAG_EXCEPTION;
        return hook_fd (plugin, fd, flags, callback, data);
    }
    
    return NULL;
}

/*
 * plugin_api_hook_print: hook a printed message
 */

struct t_hook *
plugin_api_hook_print (struct t_weechat_plugin *plugin, void *buffer,
                       char *message, int strip_colors,
                       int (*callback)(void *, void *, time_t, char *, char *),
                       void *data)
{
    if (plugin && gui_buffer_valid ((struct t_gui_buffer *)buffer)
        && callback)
        return hook_print (plugin, buffer, message, strip_colors,
                           callback, data);
    
    return NULL;
}

/*
 * plugin_api_hook_event: hook an event
 */

struct t_hook *
plugin_api_hook_event (struct t_weechat_plugin *plugin, char *event,
                       int (*callback)(void *, char *, void *),
                       void *data)
{
    if (plugin && event && event[0] && callback)
        return hook_event (plugin, event, callback, data);
    
    return NULL;
}

/*
 * plugin_api_hook_config: hook a config option
 */

struct t_hook *
plugin_api_hook_config (struct t_weechat_plugin *plugin, char *config_type,
                        char *config_option,
                        int (*callback)(void *, char *, char *, char *),
                        void *data)
{
    if (plugin && callback)
        return hook_config (plugin, config_type, config_option,
                            callback, data);
    
    return NULL;
}

/*
 * plugin_api_unhook: unhook something
 */

void
plugin_api_unhook (struct t_weechat_plugin *plugin, void *hook)
{
    if (plugin && hook
        && (hook_valid_for_plugin (plugin, (struct t_hook *)hook)))
        unhook ((struct t_hook *)hook);
}

/*
 * plugin_api_unhook_all: unhook all for a plugin
 */

void
plugin_api_unhook_all (struct t_weechat_plugin *plugin)
{
    if (plugin)
        unhook_all (plugin);
}

/*
 * plugin_api_buffer_new: create a new buffer
 */

struct t_gui_buffer *
plugin_api_buffer_new (struct t_weechat_plugin *plugin, char *category,
                       char *name)
{
    if (plugin && name && name[0])
        return gui_buffer_new (plugin, category, name);
    
    return NULL;
}

/*
 * plugin_api_buffer_search: search a buffer
 */

struct t_gui_buffer *
plugin_api_buffer_search (struct t_weechat_plugin *plugin, char *category,
                          char *name)
{
    struct t_gui_buffer *ptr_buffer;
    
    if (plugin)
    {
        ptr_buffer = gui_buffer_search_by_category_name (category, name);
        if (ptr_buffer)
            return ptr_buffer;
        return gui_current_window->buffer;
    }
    
    return NULL;
}

/*
 * plugin_api_buffer_close: close a buffer
 */

void
plugin_api_buffer_close (struct t_weechat_plugin *plugin, void *buffer)
{
    if (plugin && buffer
        && gui_buffer_valid ((struct t_gui_buffer *)buffer))
        gui_buffer_free ((struct t_gui_buffer *)buffer, 1);
}

/*
 * plugin_api_buffer_set: set a buffer property
 */

void
plugin_api_buffer_set (struct t_weechat_plugin *plugin, void *buffer,
                       char *property, char *value)
{
    if (plugin && buffer && property && property[0])
        gui_buffer_set ((struct t_gui_buffer *)buffer, property, value);
}

/*
 * plugin_api_buffer_nick_add: add a nick to a buffer nicklist
 */

void
plugin_api_buffer_nick_add (struct t_weechat_plugin *plugin, void *buffer,
                            char *nick, int sort_index, char *color_nick,
                            char prefix, char *color_prefix)
{
    int num_color_nick, num_color_prefix;
    struct t_gui_nick *ptr_nick;

    if (plugin && buffer && gui_buffer_valid ((struct t_gui_buffer *)buffer)
        && nick && nick[0])
    {
        num_color_nick = gui_color_search_config (color_nick);
        if (num_color_nick < 0)
            num_color_nick = GUI_COLOR_NICKLIST;
        
        num_color_prefix = gui_color_search_config (color_prefix);
        if (num_color_prefix < 0)
            num_color_prefix = GUI_COLOR_NICKLIST;
        
        ptr_nick = gui_nicklist_search ((struct t_gui_buffer *)buffer, nick);
        if (ptr_nick)
            gui_nicklist_update ((struct t_gui_buffer *)buffer,
                                 ptr_nick, nick, sort_index, num_color_nick,
                                 prefix, num_color_prefix);
        else
            gui_nicklist_add ((struct t_gui_buffer *)buffer,
                              nick, sort_index, num_color_nick,
                              prefix, num_color_prefix);
    }
}

/*
 * plugin_api_buffer_nick_remove: remove a nick from a buffer nicklist
 */

void
plugin_api_buffer_nick_remove (struct t_weechat_plugin *plugin, void *buffer,
                               char *nick)
{
    if (plugin && buffer && gui_buffer_valid ((struct t_gui_buffer *)buffer)
        && nick && nick[0])
    {
        if (gui_nicklist_remove ((struct t_gui_buffer *)buffer, nick))
            gui_nicklist_draw ((struct t_gui_buffer *)buffer, 0);
        //gui_nicklist_remove ((struct t_gui_buffer *)buffer, nick);
    }
}

/*
 * plugin_api_command: execute a command (simulate user entry)
 */

void
plugin_api_command (struct t_weechat_plugin *plugin, void *buffer,
                    char *command)
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
 *                      WARNING: caller has to free string returned
 *                      by this function after use
 */

char *
plugin_api_info_get (struct t_weechat_plugin *plugin, char *info)
{
    //t_irc_server *ptr_server;
    //t_irc_channel *ptr_channel;
    time_t inactivity;
    char *return_str;
    
    if (!plugin || !info)
        return NULL;
    
    /* below are infos that do NOT need server to return info */
    
    if (string_strcasecmp (info, "version") == 0)
    {
        return strdup (PACKAGE_VERSION);
    }
    else if (string_strcasecmp (info, "dir_separator") == 0)
    {
        return strdup (DIR_SEPARATOR);
    }
    else if (string_strcasecmp (info, "weechat_dir") == 0)
    {
        return strdup (weechat_home);
    }
    else if (string_strcasecmp (info, "weechat_libdir") == 0)
    {
        return strdup (WEECHAT_LIBDIR);
    }
    else if (string_strcasecmp (info, "weechat_sharedir") == 0)
    {
        return strdup (WEECHAT_SHAREDIR);
    }
    else if (string_strcasecmp (info, "charset_terminal") == 0)
    {
        return strdup (local_charset);
    }
    else if (string_strcasecmp (info, "charset_internal") == 0)
    {
        return strdup (WEECHAT_INTERNAL_CHARSET);
    }
    else if (string_strcasecmp (info, "inactivity") == 0)
    {
        if (gui_keyboard_last_activity_time == 0)
            inactivity = 0;
        else
            inactivity = time (NULL) - gui_keyboard_last_activity_time;
        return_str = (char *) malloc (32);
        if (!return_str)
            return NULL;
        snprintf (return_str, 32, "%ld", (long int)inactivity);
        return return_str;
    }
    else if (string_strcasecmp (info, "input") == 0)
    {
        if (gui_current_window->buffer->input)
        {
            return_str = string_iconv_from_internal (plugin->charset,
                                                     gui_current_window->buffer->input_buffer);
            return (return_str) ? return_str : strdup ("");
        }
        else
            return strdup ("");
    }
    else if (string_strcasecmp (info, "input_mask") == 0)
    {
        if (gui_current_window->buffer->input)
            return strdup (gui_current_window->buffer->input_buffer_color_mask);
        else
            return strdup ("");
    }
    else if (string_strcasecmp (info, "input_pos") == 0)
    {
        if (gui_current_window->buffer->input)
        {
            return_str = (char *) malloc (32);
            if (!return_str)
                return NULL;
            snprintf (return_str, 32, "%d",
                      gui_current_window->buffer->input_buffer_pos);
            return return_str;
        }
        else
            return strdup ("");
    }
    
    /* below are infos that need server to return value */
    
    /*plugin_find_server_channel (server, NULL, &ptr_server, &ptr_channel);
    
    if (string_strcasecmp (info, "nick") == 0)
    {
        if (ptr_server && ptr_server->is_connected && ptr_server->nick)
            return strdup (ptr_server->nick);
    }
    else if (string_strcasecmp (info, "channel") == 0)
    {
        if (GUI_BUFFER_IS_CHANNEL(gui_current_window->buffer)
            || GUI_BUFFER_IS_PRIVATE(gui_current_window->buffer))
            return strdup (GUI_CHANNEL(gui_current_window->buffer)->name);
    }
    else if (string_strcasecmp (info, "server") == 0)
    {
        if (ptr_server && ptr_server->is_connected && ptr_server->name)
            return strdup (ptr_server->name);
    }
    else if (string_strcasecmp (info, "type") == 0)
    {
        return_str = (char *) malloc (32);
        if (!return_str)
            return NULL;
        snprintf (return_str, 32, "%d",
                  gui_current_window->buffer->type);
        return return_str;
    }
    else if (string_strcasecmp (info, "away") == 0)
    {
        if (ptr_server && ptr_server->is_connected && ptr_server->is_away)
            return strdup ("1");
        else
            return strdup ("0");
            }*/
    
    /* info not found */
    return NULL;
}

/*
 * plugin_api_list_get_add_buffer: add a buffer in a list
 *                                 return 1 if ok, 0 if error
 */

int
plugin_api_list_get_add_buffer (struct t_plugin_list *list,
                                struct t_gui_buffer *buffer)
{
    struct t_plugin_list_item *ptr_item;
    
    if (!list || !buffer)
        return 0;
    
    ptr_item = plugin_list_new_item (list);
    if (!ptr_item)
        return 0;

    if (!plugin_list_new_var_pointer (ptr_item, "pointer", buffer))
        return 0;
    if (!plugin_list_new_var_int (ptr_item, "number", buffer->number))
        return 0;
    if (!plugin_list_new_var_string (ptr_item, "category", buffer->category))
        return 0;
    if (!plugin_list_new_var_string (ptr_item, "name", buffer->name))
        return 0;
    if (!plugin_list_new_var_int (ptr_item, "type", buffer->type))
        return 0;
    if (!plugin_list_new_var_int (ptr_item, "notify_level", buffer->notify_level))
        return 0;
    if (!plugin_list_new_var_int (ptr_item, "num_displayed", buffer->num_displayed))
        return 0;
    if (!plugin_list_new_var_string (ptr_item, "title", buffer->title))
        return 0;
    if (!plugin_list_new_var_int (ptr_item, "input", buffer->input))
        return 0;
    if (!plugin_list_new_var_string (ptr_item, "input_nick", buffer->input_nick))
        return 0;
    if (!plugin_list_new_var_string (ptr_item, "input_string", buffer->input_buffer))
        return 0;
    
    return 1;
}

/*
 * plugin_api_list_get_add_buffer_line: add a buffer line in a list
 *                                      return 1 if ok, 0 if error
 */

int
plugin_api_list_get_add_buffer_line (struct t_plugin_list *list,
                                     struct t_gui_line *line)
{
    struct t_plugin_list_item *ptr_item;
    
    if (!list || !line)
        return 0;
    
    ptr_item = plugin_list_new_item (list);
    if (!ptr_item)
        return 0;
    
    if (!plugin_list_new_var_time (ptr_item, "date", line->date))
        return 0;
    if (!plugin_list_new_var_time (ptr_item, "date_printed", line->date))
        return 0;
    if (!plugin_list_new_var_string (ptr_item, "str_time", line->str_time))
        return 0;
    if (!plugin_list_new_var_string (ptr_item, "prefix", line->prefix))
        return 0;
    if (!plugin_list_new_var_string (ptr_item, "message", line->message))
        return 0;
    
    return 1;
}

/*
 * plugin_api_list_get: get list with infos about WeeChat structures
 *                      WARNING: caller has to free string returned
 *                      by this function after use, with weechat_list_free()
 */

struct t_plugin_list *
plugin_api_list_get (struct t_weechat_plugin *plugin, char *name,
                     void *pointer)
{
    struct t_plugin_list *ptr_list;
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *ptr_line;
    
    if (!plugin || !name || !name[0])
        return NULL;
    
    if (string_strcasecmp (name, "buffer") == 0)
    {
        /* invalid buffer pointer ? */
        if (pointer && (!gui_buffer_valid ((struct t_gui_buffer *)pointer)))
            return NULL;
        
        ptr_list = plugin_list_new ();
        if (ptr_list)
        {
            if (pointer)
            {
                /* build list with only one buffer */
                if (!plugin_api_list_get_add_buffer (ptr_list,
                                                     (struct t_gui_buffer *)pointer))
                {
                    plugin_list_free (ptr_list);
                    return NULL;
                }
                return ptr_list;
            }
            else
            {
                /* build list with all buffers */
                for (ptr_buffer = gui_buffers; ptr_buffer;
                     ptr_buffer = ptr_buffer->next_buffer)
                {
                    if (!plugin_api_list_get_add_buffer (ptr_list,
                                                         ptr_buffer))
                    {
                        plugin_list_free (ptr_list);
                        return NULL;
                    }
                }
                return ptr_list;
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
            if (!gui_buffer_valid ((struct t_gui_buffer *)pointer))
                return NULL;
        }
        
        ptr_list = plugin_list_new ();
        if (ptr_list)
        {
            for (ptr_line = ((struct t_gui_buffer *)pointer)->lines; ptr_line;
                 ptr_line = ptr_line->next_line)
            {
                if (!plugin_api_list_get_add_buffer_line (ptr_list,
                                                          ptr_line))
                {
                    plugin_list_free (ptr_list);
                    return NULL;
                }
            }
            return ptr_list;
        }
    }
    
    /* list not found */
    return NULL;
}

/*
 * plugin_api_list_next: move item pointer to next item in a list
 *                       return 1 if pointer is still ok
 *                              0 if end of list was reached
 */

int
plugin_api_list_next (struct t_weechat_plugin *plugin, void *list)
{
    if (!plugin || !list
        || !plugin_list_valid ((struct t_plugin_list *)list))
        return 0;
    
    return (plugin_list_next_item ((struct t_plugin_list *)list)) ? 1 : 0;
}

/*
 * plugin_api_list_prev: move item pointer to previous item in a list
 *                       return 1 if pointer is still ok
 *                              0 if beginning of list was reached
 */

int
plugin_api_list_prev (struct t_weechat_plugin *plugin, void *list)
{
    if (!plugin || !list
        || !plugin_list_valid ((struct t_plugin_list *)list))
        return 0;

    return (plugin_list_prev_item ((struct t_plugin_list *)list)) ? 1 : 0;
}

/*
 * plugin_api_list_fields: get list of fields for current list item
 */

char *
plugin_api_list_fields (struct t_weechat_plugin *plugin, void *list)
{
    if (!plugin || !list
        || !plugin_list_valid ((struct t_plugin_list *)list))
        return NULL;
    
    return plugin_list_get_fields ((struct t_plugin_list *)list);
}

/*
 * plugin_api_list_int: get an integer variable value in current list item
 */

int
plugin_api_list_int (struct t_weechat_plugin *plugin, void *list,
                     char *var)
{
    if (!plugin || !list
        || !plugin_list_valid ((struct t_plugin_list *)list)
        || !((struct t_plugin_list *)list)->ptr_item)
        return 0;
    
    return plugin_list_get_int ((struct t_plugin_list *)list, var);
}

/*
 * plugin_api_list_string: get a string variable value in current list item
 */

char *
plugin_api_list_string (struct t_weechat_plugin *plugin, void *list,
                        char *var)
{
    if (!plugin || !list
        || !plugin_list_valid ((struct t_plugin_list *)list)
        || !((struct t_plugin_list *)list)->ptr_item)
        return NULL;
    
    return plugin_list_get_string ((struct t_plugin_list *)list, var);
}

/*
 * plugin_api_list_pointer: get a pointer variable value in current list item
 */

void *
plugin_api_list_pointer (struct t_weechat_plugin *plugin, void *list,
                         char *var)
{
    if (!plugin || !list
        || !plugin_list_valid ((struct t_plugin_list *)list)
        || !((struct t_plugin_list *)list)->ptr_item)
        return NULL;
    
    return plugin_list_get_pointer ((struct t_plugin_list *)list, var);
}

/*
 * plugin_api_list_time: get a time variable value in current list item
 */

time_t
plugin_api_list_time (struct t_weechat_plugin *plugin, void *list,
                      char *var)
{
    if (!plugin || !list
        || !plugin_list_valid ((struct t_plugin_list *)list)
        || !((struct t_plugin_list *)list)->ptr_item)
        return 0;
    
    return plugin_list_get_time ((struct t_plugin_list *)list, var);
}

/*
 * plugin_api_list_free: free a list
 */

void
plugin_api_list_free (struct t_weechat_plugin *plugin, void *list)
{
    if (plugin && list && plugin_list_valid ((struct t_plugin_list *)list))
        plugin_list_free ((struct t_plugin_list *)list);
}

/*
 * plugin_api_get_config_str_value: return string value for any option
 *                                  This function should never be called directly
 *                                  (only used by weechat_get_config)
 */

char *
plugin_api_get_config_str_value (struct t_config_option *option)
{
    char buf_temp[1024], *color_name;
    
    switch (option->type)
    {
        case OPTION_TYPE_BOOLEAN:
            return (*((int *)(option->ptr_int))) ?
                strdup ("on") : strdup ("off");
            break;
        case OPTION_TYPE_INT:
            snprintf (buf_temp, sizeof (buf_temp), "%d",
                      *((int *)(option->ptr_int)));
            return strdup (buf_temp);
            break;
        case OPTION_TYPE_INT_WITH_STRING:
            return strdup (option->array_values[*((int *)(option->ptr_int))]);
            break;
        case OPTION_TYPE_STRING:
            return (*((char **)(option->ptr_string))) ?
                strdup (*((char **)(option->ptr_string))) : strdup ("");
            break;
        case OPTION_TYPE_COLOR:
            color_name = gui_color_get_name (*((int *)(option->ptr_int)));
            return (color_name) ? strdup (color_name) : strdup ("");
            break;
    }
    
    /* should never be executed! */
    return NULL;
}

/*
 * plugin_api_config_get: get value of a config option
 */

char *
plugin_api_config_get (struct t_weechat_plugin *plugin, char *option_name)
{
    struct t_config_option *ptr_option;
    
    /* make C compiler happy */
    (void) plugin;
    
    /* search a WeeChat command */
    ptr_option = config_option_section_option_search (weechat_config_sections,
                                                      weechat_config_options,
                                                      option_name);
    if (ptr_option)
        return plugin_api_get_config_str_value (ptr_option);
    
    /* option not found */
    return NULL;
}

/*
 * plugin_api_config_set: set value of a config option
 */

int
plugin_api_config_set (struct t_weechat_plugin *plugin, char *option_name,
                       char *value)
{
    struct t_config_option *ptr_option;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (!option_name || !value)
        return 0;
    
    /* search and set WeeChat option if found */
    ptr_option = config_option_section_option_search (weechat_config_sections,
                                                      weechat_config_options,
                                                      option_name);
    if (ptr_option)
    {
        if (ptr_option->handler_change)
        {
            if (config_option_set (ptr_option, value) == 0)
            {
                (void) (ptr_option->handler_change());
                return 1;
            }
        }
        else
            return 0;
    }
    
    /* failed to set config option */
    return 0;
}

/*
 * plugin_api_plugin_config_get: get value of a plugin config option
 */

char *
plugin_api_plugin_config_get (struct t_weechat_plugin *plugin, char *option)
{
    struct t_plugin_option *ptr_plugin_option;
    
    if (!option)
        return NULL;
    
    ptr_plugin_option = plugin_config_search (plugin->name, option);
    if (ptr_plugin_option)
        return (ptr_plugin_option->value) ? strdup (ptr_plugin_option->value) : NULL;
    
    /* option not found */
    return NULL;
}

/*
 * plugin_api_plugin_config_set: set value of a plugin config option
 */

int
plugin_api_plugin_config_set (struct t_weechat_plugin *plugin, char *option,
                              char *value)
{
    if (!option)
        return 0;
    
    if (plugin_config_set (plugin->name, option, value))
    {
        plugin_config_write ();
        return 1;
    }
    return 0;
}

/*
 * plugin_api_log: add a message in buffer log file
 */

void
plugin_api_log (struct t_weechat_plugin *plugin,
                char *server, char *channel, char *message, ...)
{
    (void) plugin;
    (void) server;
    (void) channel;
    (void) message;
    
    /*t_gui_buffer *ptr_buffer;
    va_list argptr;
    static char buf[8192];
    char *buf2;
    
    if (!plugin || !message)
        return;
    
    ptr_buffer = gui_buffer_search (server, channel);
    if (ptr_buffer)
    {
	va_start (argptr, message);
	vsnprintf (buf, sizeof (buf) - 1, message, argptr);
	va_end (argptr);

        buf2 = string_iconv_to_internal (plugin->charset, buf);
	gui_log_write_line (ptr_buffer, (buf2) ? buf2 : buf);
        if (buf2)
            free (buf2);
            }*/
}
