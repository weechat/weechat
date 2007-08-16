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

/* plugins-interface.c: WeeChat plugins interface */


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

#include "../common/weechat.h"
#include "plugins.h"
#include "plugins-config.h"
#include "../common/command.h"
#include "../common/log.h"
#include "../common/utf8.h"
#include "../common/util.h"
#include "../common/weeconfig.h"
#include "../irc/irc.h"
#include "../gui/gui.h"


/*
 * weechat_ascii_strcasecmp: locale and case independent string comparison
 */

int
weechat_ascii_strcasecmp (t_weechat_plugin *plugin,
                          char *string1, char *string2)
{
    /* make C compiler happy */
    (void) plugin;
    
    return ascii_strcasecmp (string1, string2);
}

/*
 * weechat_ascii_strncasecmp: locale and case independent string comparison
 *                            with max length
 */

int
weechat_ascii_strncasecmp (t_weechat_plugin *plugin,
                          char *string1, char *string2, int max)
{
    /* make C compiler happy */
    (void) plugin;
    
    return ascii_strncasecmp (string1, string2, max);
}

/*
 * weechat_explode_string: explode a string
 */

char **
weechat_explode_string (t_weechat_plugin *plugin, char *string,
                        char *separators, int num_items_max,
                        int *num_items)
{
    /* make C compiler happy */
    (void) plugin;
    
    if (!plugin || !string || !separators || !num_items)
        return NULL;
    
    return explode_string (string, separators, num_items_max, num_items);
}

/*
 * weechat_free_exploded_string: free exploded string
 */

void
weechat_free_exploded_string (t_weechat_plugin *plugin, char **exploded_string)
{
    /* make C compiler happy */
    (void) plugin;
    
    free_exploded_string (exploded_string);
}

/*
 * weechat_plugin_mkdir_home: create a directory in WeeChat home
 */

int
weechat_plugin_mkdir_home (t_weechat_plugin *plugin, char *directory)
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
 * weechat_plugin_exec_on_files: find files in a directory and execute a
 *                               function on each file
 */

void
weechat_plugin_exec_on_files (t_weechat_plugin *plugin, char *directory,
                              int (*callback)(t_weechat_plugin *, char *))
{
    if (directory && callback)
        plugin_exec_on_files (plugin, directory, callback);
}

/*
 * weechat_plugin_print: print a message on a server or channel buffer
 */

void
weechat_plugin_print (t_weechat_plugin *plugin,
                      char *server, char *channel, char *message, ...)
{
    t_gui_buffer *ptr_buffer;
    va_list argptr;
    static char buf[8192];
    char *buf2;
    
    if (!plugin || !message)
        return;
    
    ptr_buffer = gui_buffer_search (server, channel);
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);

    buf2 = weechat_iconv_to_internal (plugin->charset, buf);
    irc_display_prefix (NULL, ptr_buffer, GUI_PREFIX_PLUGIN);
    gui_printf_keep_colors (ptr_buffer, "%s\n", (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * weechat_plugin_print_server: print a message on server buffer
 */

void
weechat_plugin_print_server (t_weechat_plugin *plugin, char *message, ...)
{
    va_list argptr;
    static char buf[8192];
    char *buf2;
    
    if (!plugin || !message)
        return;
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);

    buf2 = weechat_iconv_to_internal (plugin->charset, buf);
    irc_display_prefix (NULL, NULL, GUI_PREFIX_PLUGIN);
    gui_printf_keep_colors (NULL, "%s\n", (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * weechat_plugin_print_infobar: print a message in infobar
 */

void
weechat_plugin_print_infobar (t_weechat_plugin *plugin, int time_displayed, char *message, ...)
{
    va_list argptr;
    static char buf[1024];
    char *buf2;
    
    if (!plugin || (time_displayed < 0) || !message)
        return;
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    
    buf2 = weechat_iconv_to_internal (plugin->charset, buf);
    gui_infobar_printf (time_displayed, GUI_COLOR_WIN_INFOBAR, "%s",
                        (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * weechat_plugin_infobar_remove: remove message(s) in infobar
 */

void
weechat_plugin_infobar_remove (t_weechat_plugin *plugin, int how_many)
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
 * weechat_plugin_log: add a message in buffer log file
 */

void
weechat_plugin_log (t_weechat_plugin *plugin,
		    char *server, char *channel, char *message, ...)
{
    t_gui_buffer *ptr_buffer;
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

        buf2 = weechat_iconv_to_internal (plugin->charset, buf);
	gui_log_write_line (ptr_buffer, (buf2) ? buf2 : buf);
        if (buf2)
            free (buf2);
    }
}

/*
 * weechat_plugin_msg_handler_add: add a message handler
 */

t_plugin_handler *
weechat_plugin_msg_handler_add (t_weechat_plugin *plugin, char *message,
                                t_plugin_handler_func *handler_func,
                                char *handler_args, void *handler_pointer)
{
    if (plugin && message && handler_func)
        return plugin_msg_handler_add (plugin, message, handler_func,
                                       handler_args, handler_pointer);
    
    return NULL;
}

/*
 * weechat_plugin_cmd_handler_add: add a command handler
 */

t_plugin_handler *
weechat_plugin_cmd_handler_add (t_weechat_plugin *plugin, char *command,
                                char *description, char *arguments,
                                char *arguments_description,
                                char *completion_template,
                                t_plugin_handler_func *handler_func,
                                char *handler_args, void *handler_pointer)
{
    if (plugin && command && handler_func)
        return plugin_cmd_handler_add (plugin, command, description, arguments,
                                       arguments_description,
                                       completion_template,
                                       handler_func,
                                       handler_args, handler_pointer);
    
    return NULL;
}

/*
 * weechat_plugin_timer_handler_add: add a timer handler
 */

t_plugin_handler *
weechat_plugin_timer_handler_add (t_weechat_plugin *plugin, int interval,
                                  t_plugin_handler_func *handler_func,
                                  char *handler_args, void *handler_pointer)
{
    if (plugin && (interval >= 1) && handler_func)
        return plugin_timer_handler_add (plugin, interval, handler_func,
                                         handler_args, handler_pointer);
    
    return NULL;
}

/*
 * weechat_plugin_keyboard_handler_add: add a keyboard handler
 */

t_plugin_handler *
weechat_plugin_keyboard_handler_add (t_weechat_plugin *plugin,
                                     t_plugin_handler_func *handler_func,
                                     char *handler_args, void *handler_pointer)
{
    if (plugin && handler_func)
        return plugin_keyboard_handler_add (plugin, handler_func,
                                            handler_args, handler_pointer);
    
    return NULL;
}

/*
 * weechat_plugin_event_handler_add: add an event handler
 */

t_plugin_handler *
weechat_plugin_event_handler_add (t_weechat_plugin *plugin, char *event,
                                  t_plugin_handler_func *handler_func,
                                  char *handler_args, void *handler_pointer)
{
    if (plugin && event && handler_func)
        return plugin_event_handler_add (plugin, event, handler_func,
                                         handler_args, handler_pointer);
    
    return NULL;
}

/*
 * weechat_plugin_handler_remove: remove a WeeChat handler
 */

void
weechat_plugin_handler_remove (t_weechat_plugin *plugin,
                               t_plugin_handler *handler)
{
    if (plugin && handler)
        plugin_handler_remove (plugin, handler);
}

/*
 * weechat_plugin_handler_remove_all: remove all WeeChat handlers
 */

void
weechat_plugin_handler_remove_all (t_weechat_plugin *plugin)
{
    if (plugin)
        plugin_handler_remove_all (plugin);
}

/*
 * weechat_plugin_modifier_add: add a IRC message modifier
 */

t_plugin_modifier *
weechat_plugin_modifier_add (t_weechat_plugin *plugin,
                             char *type, char *message,
                             t_plugin_modifier_func *modifier_func,
                             char *modifier_args, void *modifier_pointer)
{
    if (plugin && type && modifier_func)
        return plugin_modifier_add (plugin, type, message, modifier_func,
                                    modifier_args, modifier_pointer);
    
    return NULL;
}

/*
 * weechat_plugin_modifier_remove: remove a WeeChat modifier
 */

void
weechat_plugin_modifier_remove (t_weechat_plugin *plugin,
                                t_plugin_modifier *modifier)
{
    if (plugin && modifier)
        plugin_modifier_remove (plugin, modifier);
}

/*
 * weechat_plugin_modifier_remove_all: remove all WeeChat modifiers
 */

void
weechat_plugin_modifier_remove_all (t_weechat_plugin *plugin)
{
    if (plugin)
        plugin_modifier_remove_all (plugin);
}

/*
 * weechat_plugin_exec_command: execute a command (simulate user entry)
 */

void
weechat_plugin_exec_command (t_weechat_plugin *plugin,
                             char *server, char *channel, char *command)
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    char *command2;
    
    if (!plugin || !command)
        return;
    
    if (plugin_find_server_channel (server, channel, &ptr_server, &ptr_channel) < 0)
    {
        irc_display_prefix (NULL, NULL, GUI_PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s server/channel (%s/%s) not found for plugin "
                      "exec command\n"),
                    WEECHAT_ERROR,
                    (server) ? server : "", (channel) ? channel : "");
    }
    else
    {
        command2 = weechat_iconv_to_internal (plugin->charset, command);
        if (ptr_server && ptr_channel)
            user_command (ptr_server, ptr_channel, (command2) ? command2 : command, 0);
        else if (ptr_server && (ptr_server->buffer))
            user_command (ptr_server, NULL, (command2) ? command2 : command, 0);
        else
            user_command (NULL, NULL, (command2) ? command2 : command, 0);
        if (command2)
            free (command2);
    }
}

/*
 * weechat_plugin_get_info: get info about WeeChat
 *                          WARNING: caller has to free string returned
 *                          by this function after use
 */

char *
weechat_plugin_get_info (t_weechat_plugin *plugin, char *info, char *server)
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    time_t inactivity;
    char *return_str;
    
    if (!plugin || !info)
        return NULL;
    
    /* below are infos that do NOT need server to return info */
    
    if (ascii_strcasecmp (info, "version") == 0)
    {
        return strdup (PACKAGE_VERSION);
    }
    else if (ascii_strcasecmp (info, "weechatdir") == 0)
    {
        /* WARNING: deprecated info, you should use "weechat_dir" */
        /* will be removed in a future version */
        return strdup (weechat_home);
    }
    else if (ascii_strcasecmp (info, "weechat_dir") == 0)
    {
        return strdup (weechat_home);
    }
    else if (ascii_strcasecmp (info, "weechat_libdir") == 0)
    {
        return strdup (WEECHAT_LIBDIR);
    }
    else if (ascii_strcasecmp (info, "weechat_sharedir") == 0)
    {
        return strdup (WEECHAT_SHAREDIR);
    }
    else if (ascii_strcasecmp (info, "charset_terminal") == 0)
    {
        return strdup (local_charset);
    }
    else if (ascii_strcasecmp (info, "charset_internal") == 0)
    {
        return strdup (WEECHAT_INTERNAL_CHARSET);
    }
    else if (ascii_strcasecmp (info, "inactivity") == 0)
    {
        if (gui_last_activity_time == 0)
            inactivity = 0;
        else
            inactivity = time (NULL) - gui_last_activity_time;
        return_str = (char *) malloc (32);
        if (!return_str)
            return NULL;
        snprintf (return_str, 32, "%ld", (long int)inactivity);
        return return_str;
    }
    else if (ascii_strcasecmp (info, "input") == 0)
    {
        if (gui_current_window->buffer->has_input)
        {
            return_str = weechat_iconv_from_internal (plugin->charset,
                                                      gui_current_window->buffer->input_buffer);
            return (return_str) ? return_str : strdup ("");
        }
        else
            return strdup ("");
    }
    else if (ascii_strcasecmp (info, "input_mask") == 0)
    {
        if (gui_current_window->buffer->has_input)
            return strdup (gui_current_window->buffer->input_buffer_color_mask);
        else
            return strdup ("");
    }
    else if (ascii_strcasecmp (info, "input_pos") == 0)
    {
        if (gui_current_window->buffer->has_input)
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
    
    plugin_find_server_channel (server, NULL, &ptr_server, &ptr_channel);
    
    if (ascii_strcasecmp (info, "nick") == 0)
    {
        if (ptr_server && ptr_server->is_connected && ptr_server->nick)
            return strdup (ptr_server->nick);
    }
    else if (ascii_strcasecmp (info, "channel") == 0)
    {
        if (GUI_BUFFER_IS_CHANNEL(gui_current_window->buffer)
            || GUI_BUFFER_IS_PRIVATE(gui_current_window->buffer))
            return strdup (GUI_CHANNEL(gui_current_window->buffer)->name);
    }
    else if (ascii_strcasecmp (info, "server") == 0)
    {
        if (ptr_server && ptr_server->is_connected && ptr_server->name)
            return strdup (ptr_server->name);
    }
    else if (ascii_strcasecmp (info, "type") == 0)
    {
        return_str = (char *) malloc (32);
        if (!return_str)
            return NULL;
        snprintf (return_str, 32, "%d",
                  gui_current_window->buffer->type);
        return return_str;
    }
    else if (ascii_strcasecmp (info, "away") == 0)
    {
        if (ptr_server && ptr_server->is_connected && ptr_server->is_away)
            return strdup ("1");
        else
            return strdup ("0");
    }
    
    /* info not found */
    return NULL;
}

/*
 * weechat_plugin_get_dcc_info: get list of DCC files/chats info
 *                              WARNING: caller has to free structure returned
 *                              by this function after use
 */

t_plugin_dcc_info *
weechat_plugin_get_dcc_info (t_weechat_plugin *plugin)
{
    t_plugin_dcc_info *dcc_info, *last_dcc_info, *new_dcc_info;
    t_irc_dcc *ptr_dcc;
    
    if (!plugin)
        return NULL;
    
    if (irc_dcc_list)
    {
        dcc_info = NULL;
        last_dcc_info = NULL;
        for (ptr_dcc = irc_dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
        {
            new_dcc_info = (t_plugin_dcc_info *)malloc (sizeof (t_plugin_dcc_info));
            if (new_dcc_info)
            {
                new_dcc_info->server = (ptr_dcc->server) ? strdup (ptr_dcc->server->name) : strdup ("");
                new_dcc_info->channel = (ptr_dcc->channel) ? strdup (ptr_dcc->channel->name) : strdup ("");
                new_dcc_info->type = ptr_dcc->type;
                new_dcc_info->status = ptr_dcc->status;
                new_dcc_info->start_time = ptr_dcc->start_time;
                new_dcc_info->start_transfer = ptr_dcc->start_transfer;
                new_dcc_info->addr = ptr_dcc->addr;
                new_dcc_info->port = ptr_dcc->port;
                new_dcc_info->nick = (ptr_dcc->nick) ? strdup (ptr_dcc->nick) : strdup ("");
                new_dcc_info->filename = (ptr_dcc->filename) ? strdup (ptr_dcc->filename) : strdup ("");
                new_dcc_info->local_filename = (ptr_dcc->local_filename) ? strdup (ptr_dcc->local_filename) : strdup ("");
                new_dcc_info->filename_suffix = ptr_dcc->filename_suffix;
                new_dcc_info->size = ptr_dcc->size;
                new_dcc_info->pos = ptr_dcc->pos;
                new_dcc_info->start_resume = ptr_dcc->start_resume;
                new_dcc_info->bytes_per_sec = ptr_dcc->bytes_per_sec;
                
                new_dcc_info->prev_dcc = last_dcc_info;
                new_dcc_info->next_dcc = NULL;
                if (!dcc_info)
                    dcc_info = new_dcc_info;
                else
                    last_dcc_info->next_dcc = new_dcc_info;
                last_dcc_info = new_dcc_info;
            }
        }
        
        return dcc_info;
    }
    
    return NULL;
}

/*
 * weechat_plugin_free_dcc_info: free dcc info struct list
 */

void
weechat_plugin_free_dcc_info (t_weechat_plugin *plugin, t_plugin_dcc_info *dcc_info)
{
    t_plugin_dcc_info *new_dcc_info;
    
    if (!plugin || !dcc_info)
        return;
    
    while (dcc_info)
    {
        if (dcc_info->server)
            free (dcc_info->server);
        if (dcc_info->channel)
            free (dcc_info->channel);
        if (dcc_info->nick)
            free (dcc_info->nick);
        if (dcc_info->filename)
            free (dcc_info->filename);
        if (dcc_info->local_filename)
            free (dcc_info->local_filename);
        new_dcc_info = dcc_info->next_dcc;
        free (dcc_info);
        dcc_info = new_dcc_info;
    }
}

/*
 * weechat_plugin_get_config_str_value: return string value for any option
 *                                      This function should never be called directly
 *                                      (only used by weechat_get_config)
 */

char *
weechat_plugin_get_config_str_value (t_config_option *option, void *value)
{
    char buf_temp[1024], *color_name;
    
    if (!value)
    {
        if (option->option_type == OPTION_TYPE_STRING)
            value = option->ptr_string;
        else
            value = option->ptr_int;
    }
    
    switch (option->option_type)
    {
        case OPTION_TYPE_BOOLEAN:
            return (*((int *)value)) ?
                strdup ("on") : strdup ("off");
            break;
        case OPTION_TYPE_INT:
            snprintf (buf_temp, sizeof (buf_temp), "%d",
                      *((int *)value));
            return strdup (buf_temp);
            break;
        case OPTION_TYPE_INT_WITH_STRING:
            return strdup (option->array_values[*((int *)value)]);
            break;
        case OPTION_TYPE_COLOR:
            color_name = gui_color_get_name (*((int *)value));
            return (color_name) ? strdup (color_name) : strdup ("");
            break;
        case OPTION_TYPE_STRING:
            return (*((char **)value)) ? strdup (*((char **)value)) : strdup ("");
            break;
    }
    
    /* should never be executed! */
    return NULL;
}

/*
 * weechat_plugin_get_config: get value of a config option
 */

char *
weechat_plugin_get_config (t_weechat_plugin *plugin, char *option)
{
    int i, j;
    t_irc_server *ptr_server;
    char option_name[256];
    void *ptr_option_value;
    
    /* make C compiler happy */
    (void) plugin;
    
    for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
    {
        if ((i != CONFIG_SECTION_KEYS) && (i != CONFIG_SECTION_ALIAS)
            && (i != CONFIG_SECTION_IGNORE) && (i != CONFIG_SECTION_SERVER))
        {
            for (j = 0; weechat_options[i][j].option_name; j++)
            {
                if ((!option) ||
                    ((option) && (option[0])
                     && (ascii_strcasecmp (weechat_options[i][j].option_name, option) == 0)))
                {
                    return weechat_plugin_get_config_str_value (&weechat_options[i][j], NULL);
                }
            }
        }
    }
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (i = 0; weechat_options[CONFIG_SECTION_SERVER][i].option_name; i++)
        {
            snprintf (option_name, sizeof (option_name), "%s.%s",
                      ptr_server->name, 
                      weechat_options[CONFIG_SECTION_SERVER][i].option_name);
            if ((!option) ||
                ((option) && (option[0])
                 && (ascii_strcasecmp (option_name, option) == 0)))
            {
                ptr_option_value = config_get_server_option_ptr (ptr_server,
                                                                 weechat_options[CONFIG_SECTION_SERVER][i].option_name);
                if (ptr_option_value)
                {
                    return weechat_plugin_get_config_str_value (&weechat_options[CONFIG_SECTION_SERVER][i],
                                                                ptr_option_value);
                }
            }
        }
    }
    
    /* option not found */
    return NULL;
}

/*
 * weechat_plugin_set_config: set value of a config option
 */

int
weechat_plugin_set_config (t_weechat_plugin *plugin, char *option, char *value)
{
    char *pos, *server_name;
    t_irc_server *ptr_server;
    t_config_option *ptr_option;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (!option || !value)
        return 0;
    
    pos = strchr (option, '.');
    if (pos)
    {
        /* server config option modification */
        server_name = (char *)malloc (pos - option + 1);
        if (server_name)
        {
            strncpy (server_name, option, pos - option);
            server_name[pos - option] = '\0';
            ptr_server = irc_server_search (server_name);
            free (server_name);
            if (ptr_server)
                return (config_set_server_value (ptr_server, pos + 1, value) == 0);
        }
    }
    else
    {
        ptr_option = config_option_search (option);
        if (ptr_option)
        {
            if (ptr_option->handler_change)
            {
                if (config_option_set_value (ptr_option, value) == 0)
                {
                    (void) (ptr_option->handler_change());
                    return 1;
                }
            }
        }
    }
    
    /* failed to set config option */
    return 0;
}

/*
 * weechat_plugin_get_plugin_config: get value of a plugin config option
 */

char *
weechat_plugin_get_plugin_config (t_weechat_plugin *plugin, char *option)
{
    t_plugin_option *ptr_plugin_option;
    
    if (!option)
        return NULL;
    
    ptr_plugin_option = plugin_config_search (plugin, option);
    if (ptr_plugin_option)
        return (ptr_plugin_option->value) ? strdup (ptr_plugin_option->value) : NULL;
    
    /* option not found */
    return NULL;
}

/*
 * weechat_plugin_set_plugin_config: set value of a plugin config option
 */

int
weechat_plugin_set_plugin_config (t_weechat_plugin *plugin, char *option, char *value)
{
    if (!option)
        return 0;
    
    if (plugin_config_set (plugin, option, value))
    {
        plugin_config_write ();
        return 1;
    }
    return 0;
}

/*
 * weechat_plugin_get_server_info: get list of server info
 *                                 WARNING: caller has to free structure returned
 *                                 by this function after use
 */

t_plugin_server_info *
weechat_plugin_get_server_info (t_weechat_plugin *plugin)
{
    t_plugin_server_info *server_info, *last_server_info, *new_server_info;
    t_irc_server *ptr_server;

    if (!plugin)
	return NULL;

    if (irc_servers)
    {
	server_info = NULL;
	last_server_info = NULL;
	for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
	{
	    new_server_info = (t_plugin_server_info *) malloc (sizeof (t_plugin_server_info));
	    if (new_server_info)
	    {
		new_server_info->name = (ptr_server->name) ? strdup (ptr_server->name) : strdup ("");
		new_server_info->autoconnect = ptr_server->autoconnect;
		new_server_info->autoreconnect = ptr_server->autoreconnect;
		new_server_info->autoreconnect_delay = ptr_server->autoreconnect_delay;
		new_server_info->temp_server = ptr_server->temp_server;
		new_server_info->address = (ptr_server->address) ? strdup (ptr_server->address) : strdup ("");
		new_server_info->port = ptr_server->port;
		new_server_info->ipv6 = ptr_server->ipv6;
		new_server_info->ssl = ptr_server->ssl;		
		new_server_info->password = (ptr_server->password) ? strdup (ptr_server->password) : strdup ("");
		new_server_info->nick1 = (ptr_server->nick1) ? strdup (ptr_server->nick1) : strdup ("");
		new_server_info->nick2 = (ptr_server->nick2) ? strdup (ptr_server->nick2) : strdup ("");
		new_server_info->nick3 = (ptr_server->nick3) ? strdup (ptr_server->nick3) : strdup ("");
		new_server_info->username = (ptr_server->username) ? strdup (ptr_server->username) : strdup ("");
		new_server_info->realname = (ptr_server->realname) ? strdup (ptr_server->realname) : strdup ("");
		new_server_info->command = (ptr_server->command) ? strdup (ptr_server->command) : strdup ("");
		new_server_info->command_delay = ptr_server->command_delay;
		new_server_info->autojoin = (ptr_server->autojoin) ? strdup (ptr_server->autojoin) : strdup ("");
		new_server_info->autorejoin = ptr_server->autorejoin;
		new_server_info->notify_levels = (ptr_server->notify_levels) ? strdup (ptr_server->notify_levels) : strdup ("");
		new_server_info->is_connected = ptr_server->is_connected;
		new_server_info->ssl_connected = ptr_server->ssl_connected;
		new_server_info->nick = (ptr_server->nick) ? strdup (ptr_server->nick) : strdup ("");
                new_server_info->nick_modes = (ptr_server->nick_modes) ? strdup (ptr_server->nick_modes) : strdup ("");
		new_server_info->is_away = ptr_server->is_away;
		new_server_info->away_time = ptr_server->away_time;
		new_server_info->lag = ptr_server->lag;

		new_server_info->prev_server = last_server_info;
                new_server_info->next_server = NULL;
                if (!server_info)
                    server_info = new_server_info;
		else
                    last_server_info->next_server = new_server_info;
		last_server_info = new_server_info;
	    }
	}
       	
	return server_info;
    }

    return NULL;
}

/*
 * weechat_plugin_free_server_info: free server info struct list
 */

void
weechat_plugin_free_server_info (t_weechat_plugin *plugin, t_plugin_server_info *server_info)
{
    t_plugin_server_info *new_server_info;
    
    if (!plugin || !server_info)
        return;
    
    while (server_info)
    {
        if (server_info->name)
            free (server_info->name);
	if (server_info->address)
            free (server_info->address);
	if (server_info->password)
            free (server_info->password);
	if (server_info->nick1)
            free (server_info->nick1);
	if (server_info->nick2)
            free (server_info->nick2);
	if (server_info->nick3)
            free (server_info->nick3);
	if (server_info->username)
            free (server_info->username);
	if (server_info->realname)
            free (server_info->realname);
	if (server_info->command)
            free (server_info->command);
	if (server_info->autojoin)
            free (server_info->autojoin);
	if (server_info->notify_levels)
            free (server_info->notify_levels);
	if (server_info->nick)
            free (server_info->nick);
	if (server_info->nick_modes)
            free (server_info->nick_modes);
        new_server_info = server_info->next_server;
        free (server_info);
        server_info = new_server_info;
    }
}

/*
 * weechat_plugin_get_channel_info: get list of channel info from a server
 *                                  WARNING: caller has to free structure
 *                                  returned by this function after use
 */

t_plugin_channel_info *
weechat_plugin_get_channel_info (t_weechat_plugin *plugin, char *server)
{
    t_plugin_channel_info *channel_info, *last_channel_info, *new_channel_info;
    t_irc_channel *ptr_channel, *ptr_channels;
    t_irc_server *ptr_server;

    if (!plugin || !server || !server[0])
	return NULL;
    
    ptr_server = irc_server_search (server);
    if (!ptr_server)
	return NULL;

    ptr_channels = ptr_server->channels;

    if (ptr_channels)
    {
	channel_info = NULL;
	last_channel_info = NULL;
	for (ptr_channel = ptr_channels; ptr_channel; ptr_channel = ptr_channel->next_channel)
	{
	    new_channel_info = (t_plugin_channel_info *) malloc (sizeof (t_plugin_channel_info));
	    if (new_channel_info)
	    {
		new_channel_info->type = ptr_channel->type;
		new_channel_info->name = (ptr_channel->name) ? strdup (ptr_channel->name) : strdup ("");
		new_channel_info->topic = (ptr_channel->topic) ? strdup (ptr_channel->topic) : strdup ("");
		new_channel_info->modes = (ptr_channel->modes) ? strdup (ptr_channel->modes) : strdup ("");
		new_channel_info->limit = ptr_channel->limit;
		new_channel_info->key = (ptr_channel->key) ? strdup (ptr_channel->key) : strdup ("");
		new_channel_info->nicks_count = ptr_channel->nicks_count;
		
		new_channel_info->prev_channel = last_channel_info;
                new_channel_info->next_channel = NULL;
                if (!channel_info)
                    channel_info = new_channel_info;		
                else
                    last_channel_info->next_channel = new_channel_info;
		last_channel_info = new_channel_info;
	    }	    
	}
       	
	return channel_info;
    }

    return NULL;
}

/*
 * weechat_plugin_free_channel_info: free channel info struct list
 */

void
weechat_plugin_free_channel_info (t_weechat_plugin *plugin, t_plugin_channel_info *channel_info)
{
    t_plugin_channel_info *new_channel_info;
    
    if (!plugin || !channel_info)
        return;
    
    while (channel_info)
    {
        if (channel_info->name)
            free (channel_info->name);
	if (channel_info->topic)
            free (channel_info->topic);
	if (channel_info->modes)
            free (channel_info->modes);
	if (channel_info->key)
            free (channel_info->key);
        new_channel_info = channel_info->next_channel;
        free (channel_info);
	channel_info = new_channel_info;
    }
}

/*
 * weechat_plugin_get_nick_info: get list of nick info from a server/channel
 *  *                            WARNING: caller has to free structure
 *                               returned by this function after use
 */

t_plugin_nick_info *
weechat_plugin_get_nick_info (t_weechat_plugin *plugin, char *server, char *channel)
{
    t_plugin_nick_info *nick_info, *last_nick_info, *new_nick_info;
    t_irc_nick *ptr_nick, *ptr_nicks;
    t_irc_channel *ptr_channel;
    t_irc_server *ptr_server;

    if (!plugin || !server || !server[0] || !channel || !channel[0])
	return NULL;
    
    ptr_server = irc_server_search (server);
    if (!ptr_server)
	return NULL;

    ptr_channel = irc_channel_search (ptr_server, channel);
    if (!ptr_channel)
	return NULL;
    
    ptr_nicks = ptr_channel->nicks;

    if (ptr_nicks)
    {
	nick_info = NULL;
	last_nick_info = NULL;
	for (ptr_nick = ptr_nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
	{
	    new_nick_info = (t_plugin_nick_info *) malloc (sizeof (t_plugin_nick_info));
	    if (new_nick_info)
	    {
		new_nick_info->nick = (ptr_nick->nick) ? strdup (ptr_nick->nick) : strdup ("");
		new_nick_info->flags = ptr_nick->flags;
		new_nick_info->host = ptr_nick->host;
		
		new_nick_info->prev_nick = last_nick_info;
                new_nick_info->next_nick = NULL;
                if (!nick_info)
                    nick_info = new_nick_info;		
                else
                    last_nick_info->next_nick = new_nick_info;
		last_nick_info = new_nick_info;
	    }	    
	}
       	
	return nick_info;
    }

    return NULL;
}

/*
 * weechat_plugin_free_nick_info: free nick info struct list
 */

void
weechat_plugin_free_nick_info (t_weechat_plugin *plugin, t_plugin_nick_info *nick_info)
{
    t_plugin_nick_info *new_nick_info;
    
    if (!plugin || !nick_info)
        return;
    
    while (nick_info)
    {
        if (nick_info->nick)
            free (nick_info->nick);
        new_nick_info = nick_info->next_nick;
        free (nick_info);
	nick_info = new_nick_info;
    }
}

/*
 * weechat_plugin_input_color: add color in input buffer
 *                             if color < 0, input buffer is refresh
 *                             if start < 0 or length <= 0, color mask is reinit
 *                             otherwise, color is applied from start to start + length
 */

void
weechat_plugin_input_color (t_weechat_plugin *plugin, int color, int start, int length)
{
    int i, begin, end;
    char *pos1, *pos2;
    
    if (!plugin
        || (!gui_current_window->buffer->has_input)
        || (gui_current_window->buffer->input_buffer_size == 0))
        return;
    
    if (color < 0)
        gui_input_draw (gui_current_window->buffer, 0);
    else
    {
        if ((start < 0) || (length <= 0))
            gui_input_init_color_mask (gui_current_window->buffer);
        else
        {
            if (local_utf8)
            {
                begin = start;
                end = start + length - 1;
            }
            else
            {
                pos1 = utf8_add_offset (gui_current_window->buffer->input_buffer,
                                        start);
                pos2 = pos1;
                for (i = 0; i < length; i++)
                    pos2 = utf8_next_char (pos2);
                begin = pos1 - gui_current_window->buffer->input_buffer;
                end = begin + (pos2 - pos1) - 1;
            }
            color %= GUI_NUM_IRC_COLORS;
            for (i = begin; i <= end; i++)
            {
                gui_current_window->buffer->input_buffer_color_mask[i] =
                    '0' + color;
            }
        }
    }
}

/*
 * weechat_plugin_get_irc_color: get number of IRC color with name
 */

int
weechat_plugin_get_irc_color (t_weechat_plugin *plugin, char *color_name)
{
    int i;

    if (!plugin)
        return -1;
    
    for (i = 0; i < GUI_NUM_IRC_COLORS; i++)
    {
        if (strcasecmp (plugins_irc_colors[i].name, color_name) == 0)
            return i;
    }
    
    /* color not found */
    return -1;
}

/*
 * weechat_plugin_get_window_info: get list of window info
 *                                 WARNING: caller has to free structure
 *                                 returned by this function after use
 */

t_plugin_window_info *
weechat_plugin_get_window_info (t_weechat_plugin *plugin)
{
    t_plugin_window_info *window_info, *last_window_info, *new_window_info;
    t_gui_window *ptr_win;

    if (!plugin)
	return NULL;

    if (gui_windows)
    {
	window_info = NULL;
	last_window_info = NULL;
	for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
	{
	    new_window_info = (t_plugin_window_info *) malloc (sizeof (t_plugin_window_info));
	    if (new_window_info)
	    {
                new_window_info->win_x = ptr_win->win_x;
                new_window_info->win_y = ptr_win->win_y;
                new_window_info->win_width = ptr_win->win_width;
                new_window_info->win_height = ptr_win->win_height;
                new_window_info->win_width_pct = ptr_win->win_width_pct;
                new_window_info->win_height_pct = ptr_win->win_height_pct;
                new_window_info->num_buffer = (ptr_win->buffer) ? ptr_win->buffer->number : 0;
                
		new_window_info->prev_window = last_window_info;
                new_window_info->next_window = NULL;
                if (!window_info)
                    window_info = new_window_info;
		else
                    last_window_info->next_window = new_window_info;
		last_window_info = new_window_info;
	    }
	}
       	
	return window_info;
    }
    
    return NULL;
}

/*
 * weechat_plugin_free_window_info: free window info struct list
 */

void
weechat_plugin_free_window_info (t_weechat_plugin *plugin, t_plugin_window_info *window_info)
{
    t_plugin_window_info *new_window_info;
    
    if (!plugin || !window_info)
        return;
    
    while (window_info)
    {
        new_window_info = window_info->next_window;
        free (window_info);
        window_info = new_window_info;
    }
}

/*
 * weechat_plugin_get_buffer_info: get list of buffer info
 *                                 WARNING: caller has to free structure
 *                                 returned by this function after use
 */

t_plugin_buffer_info *
weechat_plugin_get_buffer_info (t_weechat_plugin *plugin)
{
    t_plugin_buffer_info *buffer_info, *last_buffer_info, *new_buffer_info;
    t_gui_buffer *ptr_buffer;

    if (!plugin)
	return NULL;

    if (gui_buffers)
    {
	buffer_info = NULL;
	last_buffer_info = NULL;
	for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
	{
	    new_buffer_info = (t_plugin_buffer_info *) malloc (sizeof (t_plugin_buffer_info));
	    if (new_buffer_info)
	    {
                new_buffer_info->type = ptr_buffer->type;
                new_buffer_info->number = ptr_buffer->number;
                new_buffer_info->num_displayed = ptr_buffer->num_displayed;
                new_buffer_info->server_name = (GUI_SERVER(ptr_buffer)) ? strdup (GUI_SERVER(ptr_buffer)->name) : NULL;
                new_buffer_info->channel_name = (GUI_CHANNEL(ptr_buffer)) ? strdup (GUI_CHANNEL(ptr_buffer)->name) : NULL;
                new_buffer_info->notify_level = ptr_buffer->notify_level;
                new_buffer_info->log_filename = (ptr_buffer->log_filename) ? strdup (ptr_buffer->log_filename) : NULL;
                
		new_buffer_info->prev_buffer = last_buffer_info;
                new_buffer_info->next_buffer = NULL;
                if (!buffer_info)
                    buffer_info = new_buffer_info;
		else
                    last_buffer_info->next_buffer = new_buffer_info;
		last_buffer_info = new_buffer_info;
	    }
	}
       	
	return buffer_info;
    }
    
    return NULL;
}

/*
 * weechat_plugin_free_buffer_info: free buffer info struct list
 */

void
weechat_plugin_free_buffer_info (t_weechat_plugin *plugin, t_plugin_buffer_info *buffer_info)
{
    t_plugin_buffer_info *new_buffer_info;
    
    if (!plugin || !buffer_info)
        return;
    
    while (buffer_info)
    {
        if (buffer_info->server_name)
            free (buffer_info->server_name);
	if (buffer_info->channel_name)
            free (buffer_info->channel_name);
	if (buffer_info->log_filename)
            free (buffer_info->log_filename);
        new_buffer_info = buffer_info->next_buffer;
        free (buffer_info);
        buffer_info = new_buffer_info;
    }
}

/*
 * weechat_plugin_get_buffer_data: get buffer content
 *                                 WARNING: caller has to free structure
 *                                 returned by this function after use
 */

t_plugin_buffer_line *
weechat_plugin_get_buffer_data (t_weechat_plugin *plugin, char *server, char *channel)
{
    t_gui_buffer *ptr_buffer;
    t_plugin_buffer_line *buffer_line, *last_buffer_line, *new_buffer_line;
    t_gui_line *ptr_line;
    char *data1, *data2;
    
    if (!plugin)
	return NULL;
    
    ptr_buffer = gui_buffer_search (server, channel);
    
    if (!ptr_buffer)
        return NULL;
    
    buffer_line = NULL;
    last_buffer_line = NULL;
    for (ptr_line = ptr_buffer->last_line; ptr_line;
         ptr_line = ptr_line->prev_line)
    {
        new_buffer_line = (t_plugin_buffer_line *) malloc (sizeof (t_plugin_buffer_line));
        if (new_buffer_line)
        {
            new_buffer_line->date = ptr_line->date;
            new_buffer_line->nick = (ptr_line->nick) ? strdup (ptr_line->nick) : NULL;
            if (ptr_line->data)
            {
                data1 = (char *) gui_color_decode ((unsigned char *)(ptr_line->data + ptr_line->ofs_start_message), 0, 0);
                data2 = (data1) ? weechat_iconv_from_internal (plugin->charset, data1) : NULL;
                if (data2)
                    new_buffer_line->data = data2;
                else
                    new_buffer_line->data = ptr_line->data;
                if (data1)
                    free (data1);
            }
            else
                new_buffer_line->data = NULL;
            
            new_buffer_line->prev_line = last_buffer_line;
            new_buffer_line->next_line = NULL;
            if (!buffer_line)
                buffer_line = new_buffer_line;
            else
                last_buffer_line->next_line = new_buffer_line;
            last_buffer_line = new_buffer_line;
        }
    }
    
    return buffer_line;
}

/*
 * weechat_plugin_free_buffer_data: free buffer data (lines) struct list
 */

void
weechat_plugin_free_buffer_data (t_weechat_plugin *plugin, t_plugin_buffer_line *buffer_line)
{
    t_plugin_buffer_line *new_buffer_line;
    
    if (!plugin || !buffer_line)
        return;
    
    while (buffer_line)
    {
        if (buffer_line->nick)
            free (buffer_line->nick);
	if (buffer_line->data)
            free (buffer_line->data);
        new_buffer_line = buffer_line->next_line;
        free (buffer_line);
        buffer_line = new_buffer_line;
    }
}

/*
 * weechat_plugin_set_charset: set plugin charset
 */

void
weechat_plugin_set_charset (t_weechat_plugin *plugin, char *charset)
{
    if (!plugin || !charset)
        return;

    if (plugin->charset)
        free (plugin->charset);
    
    plugin->charset = (charset) ? strdup (charset) : NULL;
}

/*
 * weechat_plugin_iconv_to_internal: encode string from a charset to WeeChat
 *                                   internal charset
 */

char *
weechat_plugin_iconv_to_internal (t_weechat_plugin *plugin,
                                  char *charset, char *string)
{
    if (!plugin || !string)
        return NULL;
    
    return weechat_iconv_to_internal (charset, string);
}

/*
 * weechat_plugin_iconv_from_internal: encode string from WeeChat internal
 *                                     charset to another
 */

char *
weechat_plugin_iconv_from_internal (t_weechat_plugin *plugin,
                                    char *charset, char *string)
{
    if (!plugin || !string)
        return NULL;
    
    return weechat_iconv_from_internal (charset, string);
}
