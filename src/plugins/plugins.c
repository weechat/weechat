/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* plugins.c: manages WeeChat plugins (dynamic C libraries) */


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
#include <dirent.h>
#include <dlfcn.h>

#include "../common/weechat.h"
#include "plugins.h"
#include "plugins-config.h"
#include "../common/command.h"
#include "../common/util.h"
#include "../common/weeconfig.h"
#include "../irc/irc.h"
#include "../gui/gui.h"


t_weechat_plugin *weechat_plugins = NULL;
t_weechat_plugin *last_weechat_plugin = NULL;

t_plugin_irc_color plugins_irc_colors[GUI_NUM_IRC_COLORS] =
{ { /*  0 */ WEECHAT_IRC_COLOR_WHITE,        "white"        },
  { /*  1 */ WEECHAT_IRC_COLOR_BLACK,        "black"        },
  { /*  2 */ WEECHAT_IRC_COLOR_BLUE,         "blue"         },
  { /*  3 */ WEECHAT_IRC_COLOR_GREEN,        "green"        },
  { /*  4 */ WEECHAT_IRC_COLOR_LIGHTRED,     "lightred"     },
  { /*  5 */ WEECHAT_IRC_COLOR_RED,          "red"          },
  { /*  6 */ WEECHAT_IRC_COLOR_MAGENTA,      "magenta"      },
  { /*  7 */ WEECHAT_IRC_COLOR_BROWN,        "brown"        },
  { /*  8 */ WEECHAT_IRC_COLOR_YELLOW,       "yellow"       },
  { /*  9 */ WEECHAT_IRC_COLOR_LIGHTGREEN,   "lightgreen"   },
  { /* 10 */ WEECHAT_IRC_COLOR_CYAN,         "cyan"         },
  { /* 11 */ WEECHAT_IRC_COLOR_LIGHTCYAN,    "lightcyan"    },
  { /* 12 */ WEECHAT_IRC_COLOR_LIGHTBLUE,    "lightblue"    },
  { /* 13 */ WEECHAT_IRC_COLOR_LIGHTMAGENTA, "lightmagenta" },
  { /* 14 */ WEECHAT_IRC_COLOR_GRAY,         "gray"         },
  { /* 15 */ WEECHAT_IRC_COLOR_LIGHTGRAY,    "lightgray"    }};


/*
 * plugin_find_server_channel: find server/channel for command execution
 */

int
plugin_find_server_channel (char *server, char *channel,
                            t_irc_server **ptr_server,
                            t_irc_channel **ptr_channel)
{
    (*ptr_server) = NULL;
    (*ptr_channel) = NULL;
    
    /* nothing given => return current server/channel */
    if ((!server || !server[0]) && (!channel || !channel[0]))
    {
        (*ptr_server) = SERVER(gui_current_window->buffer);
        (*ptr_channel) = (BUFFER_IS_CHANNEL(gui_current_window->buffer) ||
                          BUFFER_IS_PRIVATE(gui_current_window->buffer)) ?
            CHANNEL(gui_current_window->buffer) : NULL;
    }
    else
    {
        if (server && server[0])
        {
            (*ptr_server) = server_search (server);
            if (!(*ptr_server))
                return -1;
        }
        else
        {
            (*ptr_server) = SERVER(gui_current_window->buffer);
            if (!(*ptr_server))
                (*ptr_server) = SERVER(gui_buffers);
        }
        
        if (channel && channel[0])
        {
            if ((*ptr_server))
                (*ptr_channel) = channel_search ((*ptr_server), channel);
            if (!(*ptr_channel))
                return -1;
        }
    }
    return 0;
}

/*
 * plugin_exec_on_files: find files in a directory and execute a
 *                       function on each file
 */

void
plugin_exec_on_files (t_weechat_plugin *plugin, char *directory,
                      int (*callback)(t_weechat_plugin *, char *))
{
    char complete_filename[1024];
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    
    dir = opendir (directory);
    if (dir)
    {
        while ((entry = readdir (dir)))
        {
            snprintf (complete_filename, sizeof (complete_filename) - 1,
                      "%s/%s", directory, entry->d_name);
            lstat (complete_filename, &statbuf);
            if (!S_ISDIR(statbuf.st_mode))
            {
                (int) (*callback) (plugin, complete_filename);
            }
        }
        closedir (dir);
    }
}

/*
 * plugin_search: search a plugin by name
 */

t_weechat_plugin *
plugin_search (char *name)
{
    t_weechat_plugin *ptr_plugin;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        if (ascii_strcasecmp (ptr_plugin->name, name) == 0)
            return ptr_plugin;
    }
    
    /* plugin not found */
    return NULL;
}

/*
 * plugin_cmd_handler_search: search a plugin command handler
 *                            return: pointer to handler, NULL if not found
 */

t_plugin_handler *
plugin_cmd_handler_search (char *command)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_COMMAND)
                && (ascii_strcasecmp (ptr_handler->command, command) == 0))
                return ptr_handler;
        }
    }
    
    /* command handler not found */
    return NULL;
}

/*
 * plugin_msg_handler_add: add a message handler
 *                         arguments:
 *                           1. the plugin pointer
 *                           2. the IRC command
 *                           3. the handler function
 *                           4. handler args: a string given to
 *                              handler when called (used by scripts)
 *                           5. handler pointer: a pointer given to
 *                              handler when called (used by scripts)
 */

t_plugin_handler *
plugin_msg_handler_add (t_weechat_plugin *plugin, char *irc_command,
                        t_plugin_handler_func *handler_func,
                        char *handler_args, void *handler_pointer)
{
    t_plugin_handler *new_handler;
    
    new_handler = (t_plugin_handler *)malloc (sizeof (t_plugin_handler));
    if (new_handler)
    {
        new_handler->type = PLUGIN_HANDLER_MESSAGE;
        new_handler->irc_command = strdup (irc_command);
        new_handler->command = NULL;
        new_handler->description = NULL;
        new_handler->arguments = NULL;
        new_handler->arguments_description = NULL;
        new_handler->completion_template = NULL;
        new_handler->interval = 0;
        new_handler->remaining = 0;
        new_handler->event = NULL;
        new_handler->handler = handler_func;
        new_handler->handler_args = (handler_args) ? strdup (handler_args) : NULL;
        new_handler->handler_pointer = handler_pointer;
        new_handler->running = 0;
        
        /* add new handler to list */
        new_handler->prev_handler = plugin->last_handler;
        new_handler->next_handler = NULL;
        if (plugin->handlers)
            (plugin->last_handler)->next_handler = new_handler;
        else
            plugin->handlers = new_handler;
        plugin->last_handler = new_handler;
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add handler for IRC command \"%s\" (not enough memory)\n"),
                    WEECHAT_ERROR, plugin->name, irc_command);
        return NULL;
    }
    return new_handler;
}

/*
 * plugin_cmd_handler_add: add a command handler
 *                         arguments:
 *                           1. the plugin pointer
 *                           2. the WeeChat command
 *                           3. command description (for /help)
 *                           4. command arguments (for /help)
 *                           5. command args description (for /help)
 *                           6. completion template
 *                           7. the handler function
 *                           8. handler args: a string given to
 *                              handler when called (used by scripts)
 *                           9. handler pointer: a pointer given to
 *                              handler when called (used by scripts)
 */

t_plugin_handler *
plugin_cmd_handler_add (t_weechat_plugin *plugin, char *command,
                        char *description, char *arguments,
                        char *arguments_description,
                        char *completion_template,
                        t_plugin_handler_func *handler_func,
                        char *handler_args, void *handler_pointer)
{
    t_plugin_handler *new_handler;
    
    if (plugin_cmd_handler_search (command))
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add handler for \"%s\" command "
                      "(already exists)\n"),
                    WEECHAT_ERROR, plugin->name, command);
        return NULL;
    }
    
    if (ascii_strcasecmp (command, "builtin") == 0)
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add handler for \"%s\" command "
                      "(forbidden)\n"),
                    WEECHAT_ERROR, plugin->name, command);
        return NULL;
    }
        
    new_handler = (t_plugin_handler *)malloc (sizeof (t_plugin_handler));
    if (new_handler)
    {
        new_handler->type = PLUGIN_HANDLER_COMMAND;
        new_handler->irc_command = NULL;
        new_handler->command = strdup (command);
        new_handler->description = (description) ? strdup (description) : NULL;
        new_handler->arguments = (arguments) ? strdup (arguments) : NULL;
        new_handler->arguments_description = (arguments_description) ? strdup (arguments_description) : NULL;
        new_handler->completion_template = (completion_template) ? strdup (completion_template) : strdup ("");
        new_handler->interval = 0;
        new_handler->remaining = 0;
        new_handler->event = NULL;
        new_handler->handler = handler_func;
        new_handler->handler_args = (handler_args) ? strdup (handler_args) : NULL;
        new_handler->handler_pointer = handler_pointer;
        new_handler->running = 0;
        
        /* add new handler to list */
        new_handler->prev_handler = plugin->last_handler;
        new_handler->next_handler = NULL;
        if (plugin->handlers)
            (plugin->last_handler)->next_handler = new_handler;
        else
            plugin->handlers = new_handler;
        plugin->last_handler = new_handler;
        
        /* add command to WeeChat commands list */
        if (!weelist_search (index_commands, command))
            weelist_add (&index_commands, &last_index_command, command);
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add handler for \"%s\" command (not enough memory)\n"),
                    WEECHAT_ERROR, plugin->name, command);
        return NULL;
    }
    return new_handler;
}

/*
 * plugin_timer_handler_add: add a timer handler
 *                           arguments:
 *                             1. the plugin pointer
 *                             2. the interval between two calls
 *                             3. the handler function
 *                             4. handler args: a string given to
 *                                handler when called (used by scripts)
 *                             5. handler pointer: a pointer given to
 *                                handler when called (used by scripts)
 */

t_plugin_handler *
plugin_timer_handler_add (t_weechat_plugin *plugin, int interval,
                          t_plugin_handler_func *handler_func,
                          char *handler_args, void *handler_pointer)
{
    t_plugin_handler *new_handler;
    
    new_handler = (t_plugin_handler *)malloc (sizeof (t_plugin_handler));
    if (new_handler)
    {
        new_handler->type = PLUGIN_HANDLER_TIMER;
        new_handler->irc_command = NULL;
        new_handler->command = NULL;
        new_handler->description = NULL;
        new_handler->arguments = NULL;
        new_handler->arguments_description = NULL;
        new_handler->completion_template = NULL;
        new_handler->interval = interval;
        new_handler->remaining = interval;
        new_handler->event = NULL;
        new_handler->handler = handler_func;
        new_handler->handler_args = (handler_args) ? strdup (handler_args) : NULL;
        new_handler->handler_pointer = handler_pointer;
        new_handler->running = 0;
        
        /* add new handler to list */
        new_handler->prev_handler = plugin->last_handler;
        new_handler->next_handler = NULL;
        if (plugin->handlers)
            (plugin->last_handler)->next_handler = new_handler;
        else
            plugin->handlers = new_handler;
        plugin->last_handler = new_handler;
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add timer handler (not enough memory)\n"),
                    WEECHAT_ERROR, plugin->name);
        return NULL;
    }
    return new_handler;
}

/*
 * plugin_keyboard_handler_add: add a keyboard handler
 *                              arguments:
 *                                1. the plugin pointer
 *                                2. the handler function
 *                                3. handler args: a string given to
 *                                   handler when called (used by scripts)
 *                                4. handler pointer: a pointer given to
 *                                   handler when called (used by scripts)
 */

t_plugin_handler *
plugin_keyboard_handler_add (t_weechat_plugin *plugin,
                             t_plugin_handler_func *handler_func,
                             char *handler_args, void *handler_pointer)
{
    t_plugin_handler *new_handler;
    
    new_handler = (t_plugin_handler *)malloc (sizeof (t_plugin_handler));
    if (new_handler)
    {
        new_handler->type = PLUGIN_HANDLER_KEYBOARD;
        new_handler->irc_command = NULL;
        new_handler->command = NULL;
        new_handler->description = NULL;
        new_handler->arguments = NULL;
        new_handler->arguments_description = NULL;
        new_handler->completion_template = NULL;
        new_handler->interval = 0;
        new_handler->remaining = 0;
        new_handler->event = NULL;
        new_handler->handler = handler_func;
        new_handler->handler_args = (handler_args) ? strdup (handler_args) : NULL;
        new_handler->handler_pointer = handler_pointer;
        new_handler->running = 0;
        
        /* add new handler to list */
        new_handler->prev_handler = plugin->last_handler;
        new_handler->next_handler = NULL;
        if (plugin->handlers)
            (plugin->last_handler)->next_handler = new_handler;
        else
            plugin->handlers = new_handler;
        plugin->last_handler = new_handler;
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add keyboard handler (not enough memory)\n"),
                    WEECHAT_ERROR, plugin->name);
        return NULL;
    }
    return new_handler;
}

/*
 * plugin_event_handler_add: add an event handler
 *                           arguments:
 *                             1. the plugin pointer
 *                             2. the event to catch
 *                             3. the handler function
 *                             4. handler args: a string given to
 *                                handler when called (used by scripts)
 *                             5. handler pointer: a pointer given to
 *                                handler when called (used by scripts)
 */

t_plugin_handler *
plugin_event_handler_add (t_weechat_plugin *plugin, char *event,
                          t_plugin_handler_func *handler_func,
                          char *handler_args, void *handler_pointer)
{
    t_plugin_handler *new_handler;
    
    new_handler = (t_plugin_handler *)malloc (sizeof (t_plugin_handler));
    if (new_handler)
    {
        new_handler->type = PLUGIN_HANDLER_EVENT;
        new_handler->irc_command = NULL;
        new_handler->command = NULL;
        new_handler->description = NULL;
        new_handler->arguments = NULL;
        new_handler->arguments_description = NULL;
        new_handler->completion_template = NULL;
        new_handler->interval = 0;
        new_handler->remaining = 0;
        new_handler->event = strdup (event);
        new_handler->handler = handler_func;
        new_handler->handler_args = (handler_args) ? strdup (handler_args) : NULL;
        new_handler->handler_pointer = handler_pointer;
        new_handler->running = 0;
        
        /* add new handler to list */
        new_handler->prev_handler = plugin->last_handler;
        new_handler->next_handler = NULL;
        if (plugin->handlers)
            (plugin->last_handler)->next_handler = new_handler;
        else
            plugin->handlers = new_handler;
        plugin->last_handler = new_handler;
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add event handler (not enough memory)\n"),
                    WEECHAT_ERROR, plugin->name);
        return NULL;
    }
    return new_handler;
}

/*
 * plugin_msg_handler_exec: execute a message handler
 *                          return: code for informing WeeChat whether message
 *                          should be ignored or not
 */

int
plugin_msg_handler_exec (char *server, char *irc_command, char *irc_message)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    int return_code, final_return_code;
    char *argv[3] = { NULL, NULL, NULL };
    
    argv[0] = server;
    argv[1] = irc_command;
    argv[2] = irc_message;
    
    final_return_code = PLUGIN_RC_OK;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_MESSAGE)
                && (((ascii_strcasecmp (ptr_handler->irc_command, "*") == 0)
                    && (ascii_strncasecmp (irc_command, "weechat_", 8) != 0))
                    || (ascii_strcasecmp (ptr_handler->irc_command, irc_command) == 0)))
            {
                if (ptr_handler->running == 0)
                {
                    ptr_handler->running = 1;
                    return_code = ((int) (ptr_handler->handler) (ptr_plugin,
                                                                 3, argv,
                                                                 ptr_handler->handler_args,
                                                                 ptr_handler->handler_pointer));
                    ptr_handler->running = 0;
                    
                    if (return_code >= 0)
                    {
                        if (return_code & PLUGIN_RC_OK_IGNORE_WEECHAT)
                            final_return_code = PLUGIN_RC_OK_IGNORE_WEECHAT;
                        if (return_code & PLUGIN_RC_OK_IGNORE_PLUGINS)
                            return final_return_code;
                        if (return_code & PLUGIN_RC_OK_WITH_HIGHLIGHT)
                            final_return_code = PLUGIN_RC_OK_WITH_HIGHLIGHT;
                    }
                }
            }
        }
    }
    
    return final_return_code;
}

/*
 * plugin_cmd_handler_exec: execute a command handler
 *                          return: 1 if handler executed, 0 if failed,
 *                                  -1 if no handler found
 */

int
plugin_cmd_handler_exec (char *server, char *command, char *arguments)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    int return_code;
    char empty_arg[1] = { '\0' };
    char *argv[3] = { NULL, NULL, NULL };
    
    argv[0] = server;
    argv[1] = command;
    argv[2] = (arguments) ? arguments : empty_arg;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_COMMAND)
                && (ascii_strcasecmp (ptr_handler->command, command) == 0))
            {
                if (ptr_handler->running == 0)
                {
                    ptr_handler->running = 1;
                    return_code = (int) (ptr_handler->handler) (ptr_plugin,
                                                                3, argv,
                                                                ptr_handler->handler_args,
                                                                ptr_handler->handler_pointer);
                    ptr_handler->running = 0;
                    return (return_code == PLUGIN_RC_KO) ? 0 : 1;
                }
            }
        }
    }
    
    return -1;
}

/*
 * plugin_timer_handler_exec: check timer handlers and execute functions if needed
 *                            return: PLUGIN_RC_OK if all ok
 *                                    PLUGIN_RC_KO if at least one handler failed
 */

int
plugin_timer_handler_exec ()
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    int return_code, final_return_code;
    
    final_return_code = PLUGIN_RC_OK;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if (ptr_handler->type == PLUGIN_HANDLER_TIMER)
            {
                ptr_handler->remaining--;
                if (ptr_handler->remaining <= 0)
                {
                    return_code = ((int) (ptr_handler->handler) (ptr_plugin,
                                                                 0, NULL,
                                                                 ptr_handler->handler_args,
                                                                 ptr_handler->handler_pointer));
                    ptr_handler->remaining = ptr_handler->interval;
                    if (return_code == PLUGIN_RC_KO)
                        final_return_code = PLUGIN_RC_KO;
                }
            }
        }
    }
    
    return final_return_code;
}

/*
 * plugin_keyboard_handler_exec: execute all keyboard handlers
 *                               return: PLUGIN_RC_OK if all ok
 *                                       PLUGIN_RC_KO if at least one handler failed
 */

int
plugin_keyboard_handler_exec (char *key, char *input_before, char *input_after)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    int return_code, final_return_code;
    char *argv[3] = { NULL, NULL, NULL };
    
    argv[0] = key;
    argv[1] = input_before;
    argv[2] = input_after;
    
    final_return_code = PLUGIN_RC_OK;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if (ptr_handler->type == PLUGIN_HANDLER_KEYBOARD)
            {
                return_code = ((int) (ptr_handler->handler) (ptr_plugin,
                                                             3, argv,
                                                             ptr_handler->handler_args,
                                                             ptr_handler->handler_pointer));
                if (return_code == PLUGIN_RC_KO)
                    final_return_code = PLUGIN_RC_KO;
            }
        }
    }
    
    return final_return_code;
}

/*
 * plugin_event_handler_exec: execute an event handler
 *                            return: PLUGIN_RC_OK if all ok
 *                                    PLUGIN_RC_KO if at least one handler failed
 */

int
plugin_event_handler_exec (char *event, char *data)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    int return_code, final_return_code;
    char *argv[1] = { NULL };
    
    argv[0] = data;
    
    final_return_code = PLUGIN_RC_OK;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_EVENT)
                && (ascii_strcasecmp (ptr_handler->event, event) == 0))
            {
                return_code = ((int) (ptr_handler->handler) (ptr_plugin,
                                                             1, argv,
                                                             ptr_handler->handler_args,
                                                             ptr_handler->handler_pointer));
                if (return_code == PLUGIN_RC_KO)
                    final_return_code = PLUGIN_RC_KO;
            }
        }
    }
    
    return final_return_code;
}

/*
 * plugin_handler_remove: remove a handler for a plugin
 */

void
plugin_handler_remove (t_weechat_plugin *plugin,
                       t_plugin_handler *handler)
{
    t_plugin_handler *new_handlers;
    
    /* remove handler from list */
    if (plugin->last_handler == handler)
        plugin->last_handler = handler->prev_handler;
    if (handler->prev_handler)
    {
        (handler->prev_handler)->next_handler = handler->next_handler;
        new_handlers = plugin->handlers;
    }
    else
        new_handlers = handler->next_handler;
    
    if (handler->next_handler)
        (handler->next_handler)->prev_handler = handler->prev_handler;
    
    /* remove command from WeeChat command list, if command handler */
    if ((handler->type == PLUGIN_HANDLER_COMMAND)
        && (!command_used_by_weechat (handler->command)))
        weelist_remove (&index_commands, &last_index_command,
                        weelist_search (index_commands, handler->command));
    
    /* free data */
    if (handler->irc_command)
        free (handler->irc_command);
    if (handler->command)
        free (handler->command);
    if (handler->description)
        free (handler->description);
    if (handler->arguments)
        free (handler->arguments);
    if (handler->arguments_description)
        free (handler->arguments_description);
    if (handler->event)
        free (handler->event);
    if (handler->handler_args)
        free (handler->handler_args);
    
    plugin->handlers = new_handlers;
}

/*
 * plugin_handler_remove_all: remove all handlers for a plugin
 */

void
plugin_handler_remove_all (t_weechat_plugin *plugin)
{
    while (plugin->handlers)
        plugin_handler_remove (plugin, plugin->handlers);
}

/*
 * plugin_modifier_add: add a IRC handler
 *                      arguments:
 *                        1. the plugin pointer
 *                        2. type of modifier
 *                        3. message ("*" means all)
 *                        4. the modifier function
 *                        5. modifier args: a string given to
 *                           modifier when called (used by scripts)
 *                        6. modifier pointer: a pointer given to
 *                           modifier when called (used by scripts)
 */

t_plugin_modifier *
plugin_modifier_add (t_weechat_plugin *plugin, char *type, char *command,
                     t_plugin_modifier_func *modifier_func,
                     char *modifier_args, void *modifier_pointer)
{
    t_plugin_modifier *new_modifier;
    int type_int;

    if (ascii_strcasecmp (type, PLUGIN_MODIFIER_IRC_IN_STR) == 0)
        type_int = PLUGIN_MODIFIER_IRC_IN;
    else if (ascii_strcasecmp (type, PLUGIN_MODIFIER_IRC_USER_STR) == 0)
        type_int = PLUGIN_MODIFIER_IRC_USER;
    else if (ascii_strcasecmp (type, PLUGIN_MODIFIER_IRC_OUT_STR) == 0)
        type_int = PLUGIN_MODIFIER_IRC_OUT;
    else
        return NULL;
    
    new_modifier = (t_plugin_modifier *)malloc (sizeof (t_plugin_modifier));
    if (new_modifier)
    {
        new_modifier->type = type_int;
        new_modifier->command = (command) ? strdup (command) : strdup ("*");
        new_modifier->modifier = modifier_func;
        new_modifier->modifier_args = (modifier_args) ? strdup (modifier_args) : NULL;
        new_modifier->modifier_pointer = modifier_pointer;
        new_modifier->running = 0;
        
        /* add new modifier to list */
        new_modifier->prev_modifier = plugin->last_modifier;
        new_modifier->next_modifier = NULL;
        if (plugin->modifiers)
            (plugin->last_modifier)->next_modifier = new_modifier;
        else
            plugin->modifiers = new_modifier;
        plugin->last_modifier = new_modifier;
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add modifier (not enough memory)\n"),
                    WEECHAT_ERROR, plugin->name);
        return NULL;
    }
    return new_modifier;
}

/*
 * plugin_modifier_exec: execute a modifier
 *                       return: NULL if no modifier was applied on message
 *                               "" (empty string) if message has been dropped by a modifier
 *                               other string if message has been modified
 */

char *
plugin_modifier_exec (t_plugin_modifier_type type,
                      char *server, char *message)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_modifier *ptr_modifier;
    char *argv[2] = { NULL, NULL };
    char *new_msg, *message_modified, *pos, *command;
    int length_command;
    
    argv[0] = server;
    argv[1] = message;
    
    command = NULL;
    length_command = 0;
    if ((type == PLUGIN_MODIFIER_IRC_IN) || (type == PLUGIN_MODIFIER_IRC_OUT))
    {
        /* look for command in message */
        if (message[0] == ':')
        {
            pos = strchr (message, ' ');
            if (pos)
            {
                while (pos[0] == ' ')
                    pos++;
                command = pos;
            }
        }
        else
            command = message;
        if (command)
        {
            pos = strchr (command, ' ');
            if (pos)
                length_command = pos - command;
            else
                length_command = strlen (command);
        }
    }
    
    new_msg = NULL;
    message_modified = NULL;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_modifier = ptr_plugin->modifiers;
             ptr_modifier; ptr_modifier = ptr_modifier->next_modifier)
        {
            if (ptr_modifier->type == type)
            {
                if (ptr_modifier->running == 0)
                {
                    if (((type != PLUGIN_MODIFIER_IRC_IN) && (type != PLUGIN_MODIFIER_IRC_OUT))
                        || (ascii_strcasecmp (ptr_modifier->command, "*") == 0)
                        || (command && (ascii_strncasecmp (ptr_modifier->command, command, length_command) == 0)))
                    {
                        ptr_modifier->running = 1;
                        new_msg = ((char *) (ptr_modifier->modifier) (ptr_plugin,
                                                                      2, argv,
                                                                      ptr_modifier->modifier_args,
                                                                      ptr_modifier->modifier_pointer));
                        ptr_modifier->running = 0;
                        
                        /* message dropped? */
                        if (new_msg && !new_msg[0])
                            return new_msg;
                        
                        /* new message => keep it as base for next modifier */
                        if (new_msg)
                        {
                            /* free any new message allocated before by another modifier */
                            if (argv[1] != message)
                                free (argv[1]);
                            argv[1] = new_msg;
                            message_modified = new_msg;
                        }
                    }
                }
            }
        }
    }
    
    return message_modified;
}

/*
 * plugin_modifier_remove: remove a modifier for a plugin
 */

void
plugin_modifier_remove (t_weechat_plugin *plugin,
                        t_plugin_modifier *modifier)
{
    t_plugin_modifier *new_modifiers;
    
    /* remove modifier from list */
    if (plugin->last_modifier == modifier)
        plugin->last_modifier = modifier->prev_modifier;
    if (modifier->prev_modifier)
    {
        (modifier->prev_modifier)->next_modifier = modifier->next_modifier;
        new_modifiers = plugin->modifiers;
    }
    else
        new_modifiers = modifier->next_modifier;
    
    if (modifier->next_modifier)
        (modifier->next_modifier)->prev_modifier = modifier->prev_modifier;
    
    /* free data */
    if (modifier->command)
        free (modifier->command);
    
    plugin->modifiers = new_modifiers;
}

/*
 * plugin_modifier_remove_all: remove all modifiers for a plugin
 */

void
plugin_modifier_remove_all (t_weechat_plugin *plugin)
{
    while (plugin->modifiers)
        plugin_modifier_remove (plugin, plugin->modifiers);
}

/*
 * plugin_search_full_name: search the full name of a file with a part of name
 *                          and look in WeeChat user's dir, then WeeChat global lib dir
 */

char *
plugin_search_full_name (char *filename)
{
    char *name_with_ext, *final_name;
    int length;
    struct stat st;
    
    /* filename is already a full path */
    if (strchr (filename, '/') || strchr (filename, '\\'))
        return strdup (filename);
    
    length = strlen (filename) + 16;
    if (cfg_plugins_extension && cfg_plugins_extension[0])
        length += strlen (cfg_plugins_extension);
    name_with_ext = (char *)malloc (length);
    if (!name_with_ext)
        return strdup (filename);
    strcpy (name_with_ext, filename);
    if (!strchr (filename, '.')
        && cfg_plugins_extension && cfg_plugins_extension[0])
        strcat (name_with_ext, cfg_plugins_extension);
    
    /* try WeeChat user's dir */
    length = strlen (weechat_home) + strlen (name_with_ext) + 16;
    final_name = (char *)malloc (length);
    if (!final_name)
    {
        free (name_with_ext);
        return strdup (filename);
    }
    snprintf (final_name, length,
              "%s/plugins/%s", weechat_home, name_with_ext);
    if ((stat (final_name, &st) == 0) && (st.st_size > 0))
    {
        free (name_with_ext);
        return final_name;
    }
    free (final_name);
    
    /* try WeeChat global lib dir */
    length = strlen (WEECHAT_LIBDIR) + strlen (name_with_ext) + 16;
    final_name = (char *)malloc (length);
    if (!final_name)
    {
        free (name_with_ext);
        return strdup (filename);
    }
    snprintf (final_name, length,
              "%s/plugins/%s", WEECHAT_LIBDIR, name_with_ext);
    if ((stat (final_name, &st) == 0) && (st.st_size > 0))
    {
        free (name_with_ext);
        return final_name;
    }
    free (final_name);

    return name_with_ext;
}

/*
 * plugin_load: load a WeeChat plugin (a dynamic library)
 *              return: pointer to new WeeChat plugin, NULL if error
 */

t_weechat_plugin *
plugin_load (char *filename)
{
    char *full_name;
    void *handle;
    char *name, *description, *version, *charset;
    t_weechat_init_func *init_func;
    t_weechat_plugin *new_plugin;
    
    if (!filename)
        return NULL;
    
    full_name = plugin_search_full_name (filename);
    
    if (!full_name)
        return NULL;
    
    handle = dlopen (full_name, RTLD_GLOBAL | RTLD_NOW);
    if (!handle)
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s unable to load plugin \"%s\": %s\n"),
                    WEECHAT_ERROR, full_name, dlerror());
        free (full_name);
        return NULL;
    }
    /* look for plugin name */
    name = dlsym (handle, "plugin_name");
    if (!name)
    {
        dlclose (handle);
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s symbol \"plugin_name\" not found in plugin \"%s\", failed to load\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    /* check for plugin with same name */
    if (plugin_search (name))
    {
        dlclose (handle);
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s unable to load plugin \"%s\": a plugin with "
                      "same name already exists\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    /* look for plugin description */
    description = dlsym (handle, "plugin_description");
    if (!description)
    {
        dlclose (handle);
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s symbol \"plugin_description\" not found in plugin \"%s\", failed to load\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    /* look for plugin version */
    version = dlsym (handle, "plugin_version");
    if (!version)
    {
        dlclose (handle);
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s symbol \"plugin_version\" not found in plugin \"%s\", failed to load\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    /* look for plugin charset (optional) */
    charset = dlsym (handle, "plugin_charset");
    /* look for plugin init function */
    init_func = dlsym (handle, "weechat_plugin_init");
    if (!init_func)
    {
        dlclose (handle);
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s function \"weechat_plugin_init\" not found in plugin \"%s\", failed to load\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    
    /* create new plugin */
    new_plugin = (t_weechat_plugin *)malloc (sizeof (t_weechat_plugin));
    if (new_plugin)
    {
        /* variables */
        new_plugin->filename = strdup (full_name);
        new_plugin->handle = handle;
        new_plugin->name = strdup (name);
        new_plugin->description = strdup (description);
        new_plugin->version = strdup (version);
        new_plugin->charset = (charset) ? strdup (charset) : NULL;
        
        /* functions */
        new_plugin->ascii_strcasecmp = &weechat_ascii_strcasecmp;
        new_plugin->explode_string = &weechat_explode_string;
        new_plugin->free_exploded_string = &weechat_free_exploded_string;
        new_plugin->mkdir_home = &weechat_plugin_mkdir_home;
        new_plugin->exec_on_files = &weechat_plugin_exec_on_files;
        new_plugin->msg_handler_add = &weechat_plugin_msg_handler_add;
        new_plugin->cmd_handler_add = &weechat_plugin_cmd_handler_add;
        new_plugin->timer_handler_add = &weechat_plugin_timer_handler_add;
        new_plugin->keyboard_handler_add = &weechat_plugin_keyboard_handler_add;
        new_plugin->event_handler_add = &weechat_plugin_event_handler_add;
        new_plugin->handler_remove = &weechat_plugin_handler_remove;
        new_plugin->handler_remove_all = &weechat_plugin_handler_remove_all;
        new_plugin->modifier_add = &weechat_plugin_modifier_add;
        new_plugin->modifier_remove = &weechat_plugin_modifier_remove;
        new_plugin->modifier_remove_all = &weechat_plugin_modifier_remove_all;
        new_plugin->print = &weechat_plugin_print;
        new_plugin->print_server = &weechat_plugin_print_server;
        new_plugin->print_infobar = &weechat_plugin_print_infobar;
        new_plugin->infobar_remove = &weechat_plugin_infobar_remove;
	new_plugin->log = &weechat_plugin_log;
        new_plugin->exec_command = &weechat_plugin_exec_command;
        new_plugin->get_info = &weechat_plugin_get_info;
        new_plugin->get_dcc_info = &weechat_plugin_get_dcc_info;
        new_plugin->free_dcc_info = &weechat_plugin_free_dcc_info;
        new_plugin->get_config = &weechat_plugin_get_config;
        new_plugin->set_config = &weechat_plugin_set_config;
        new_plugin->get_plugin_config = &weechat_plugin_get_plugin_config;
        new_plugin->set_plugin_config = &weechat_plugin_set_plugin_config;
	new_plugin->get_server_info = &weechat_plugin_get_server_info;
	new_plugin->free_server_info = &weechat_plugin_free_server_info;
	new_plugin->get_channel_info = &weechat_plugin_get_channel_info;
	new_plugin->free_channel_info = &weechat_plugin_free_channel_info;
	new_plugin->get_nick_info = &weechat_plugin_get_nick_info;
	new_plugin->free_nick_info = &weechat_plugin_free_nick_info;
        new_plugin->input_color = &weechat_plugin_input_color;
        new_plugin->get_irc_color = &weechat_plugin_get_irc_color;
        new_plugin->get_window_info = &weechat_plugin_get_window_info;
        new_plugin->free_window_info = &weechat_plugin_free_window_info;
        new_plugin->get_buffer_info = &weechat_plugin_get_buffer_info;
        new_plugin->free_buffer_info = &weechat_plugin_free_buffer_info;
        new_plugin->get_buffer_data = &weechat_plugin_get_buffer_data;
        new_plugin->free_buffer_data = &weechat_plugin_free_buffer_data;
        new_plugin->set_charset = &weechat_plugin_set_charset;
        new_plugin->iconv_to_internal = &weechat_plugin_iconv_to_internal;
        new_plugin->iconv_from_internal = &weechat_plugin_iconv_from_internal;
        
        /* handlers */
        new_plugin->handlers = NULL;
        new_plugin->last_handler = NULL;
        
        /* modifiers */
        new_plugin->modifiers = NULL;
        new_plugin->last_modifier = NULL;
        
        /* add new plugin to list */
        new_plugin->prev_plugin = last_weechat_plugin;
        new_plugin->next_plugin = NULL;
        if (weechat_plugins)
            last_weechat_plugin->next_plugin = new_plugin;
        else
            weechat_plugins = new_plugin;
        last_weechat_plugin = new_plugin;
        
        irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
        gui_printf (NULL,
                    _("Initializing plugin \"%s\" %s\n"),
                    new_plugin->name, new_plugin->version);
        
        /* init plugin */
        if (((t_weechat_init_func *)init_func) (new_plugin) < 0)
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s unable to initialize plugin \"%s\"\n"),
                        WEECHAT_ERROR, full_name);
            plugin_remove (new_plugin);
            free (full_name);
            return NULL;
        }
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s unable to load plugin \"%s\" (not enough memory)\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    
    irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
    gui_printf (NULL,
                _("Plugin \"%s\" (%s) loaded.\n"),
                name, full_name);
    
    free (full_name);
    
    return new_plugin;
}

/*
 * plugin_auto_load_file: load a file found by plugin_auto_load,
 *                        but only it this is really a dynamic library
 */

int
plugin_auto_load_file (t_weechat_plugin *plugin, char *filename)
{
    char *pos;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (cfg_plugins_extension && cfg_plugins_extension[0])
    {
        pos = strstr (filename, cfg_plugins_extension);
        if (pos)
        {
            if (ascii_strcasecmp (pos, cfg_plugins_extension) == 0)
                plugin_load (filename);
        }
    }
    else
        plugin_load (filename);
    return 1;
}

/*
 * plugin_auto_load: auto-load WeeChat plugins
 */

void
plugin_auto_load ()
{
    char *ptr_home, *dir_name, *plugins_path, *plugins_path2;
    char *list_plugins, *pos, *pos2;
    
    if (cfg_plugins_autoload && cfg_plugins_autoload[0])
    {
        if (ascii_strcasecmp (cfg_plugins_autoload, "*") == 0)
        {
            /* auto-load plugins in WeeChat home dir */
            if (cfg_plugins_path && cfg_plugins_path[0])
            {
                ptr_home = getenv ("HOME");
                plugins_path = weechat_strreplace (cfg_plugins_path, "~", ptr_home);
                plugins_path2 = weechat_strreplace ((plugins_path) ?
                                                    plugins_path : cfg_plugins_path,
                                                    "%h", weechat_home);
                plugin_exec_on_files (NULL,
                                      (plugins_path2) ?
                                      plugins_path2 : ((plugins_path) ?
                                                       plugins_path : cfg_plugins_path),
                                      &plugin_auto_load_file);
                if (plugins_path)
                    free (plugins_path);
                if (plugins_path2)
                    free (plugins_path2);
            }
    
            /* auto-load plugins in WeeChat global lib dir */
            dir_name = (char *)malloc (strlen (WEECHAT_LIBDIR) + 16);
            if (dir_name)
            {
                snprintf (dir_name, strlen (WEECHAT_LIBDIR) + 16,
                          "%s/plugins", WEECHAT_LIBDIR);
                plugin_exec_on_files (NULL, dir_name, &plugin_auto_load_file);
                free (dir_name);
            }
        }
        else
        {
            list_plugins = strdup (cfg_plugins_autoload);
            if (list_plugins)
            {
                pos = list_plugins;
                while (pos && pos[0])
                {
                    pos2 = strchr (pos, ',');
                    if (pos2)
                        pos2[0] = '\0';
                    plugin_load (pos);
                    if (pos2)
                        pos = pos2 + 1;
                    else
                        pos = NULL;
                }
                free (list_plugins);
            }
        }
    }
}

/*
 * plugin_remove: remove a WeeChat plugin
 */

void
plugin_remove (t_weechat_plugin *plugin)
{
    t_weechat_plugin *new_weechat_plugins;
    
    /* remove handler from list */
    if (last_weechat_plugin == plugin)
        last_weechat_plugin = plugin->prev_plugin;
    if (plugin->prev_plugin)
    {
        (plugin->prev_plugin)->next_plugin = plugin->next_plugin;
        new_weechat_plugins = weechat_plugins;
    }
    else
        new_weechat_plugins = plugin->next_plugin;
    
    if (plugin->next_plugin)
        (plugin->next_plugin)->prev_plugin = plugin->prev_plugin;
    
    /* remove all handlers and modifiers */
    plugin_handler_remove_all (plugin);
    plugin_modifier_remove_all (plugin);

    /* free data */
    if (plugin->filename)
        free (plugin->filename);
    dlclose (plugin->handle);
    if (plugin->name)
        free (plugin->name);
    if (plugin->description)
        free (plugin->description);
    if (plugin->version)
        free (plugin->version);
    if (plugin->charset)
        free (plugin->charset);
    free (plugin);
    
    weechat_plugins = new_weechat_plugins;
}

/*
 * plugin_unload: unload a WeeChat plugin
 */

void
plugin_unload (t_weechat_plugin *plugin)
{
    t_weechat_end_func *end_func;
    
    end_func = dlsym (plugin->handle, "weechat_plugin_end");
    if (end_func)
        (void) (end_func) (plugin);
    plugin_remove (plugin);
}

/*
 * plugin_unload_name: unload a WeeChat plugin by name
 */

void
plugin_unload_name (char *name)
{
    t_weechat_plugin *ptr_plugin;
    
    ptr_plugin = plugin_search (name);
    if (ptr_plugin)
    {
        plugin_unload (ptr_plugin);
        irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
        gui_printf (NULL, _("Plugin \"%s\" unloaded.\n"), name);
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin \"%s\" not found\n"),
                    WEECHAT_ERROR, name);
    }
}

/*
 * plugin_unload_all: unload all WeeChat plugins
 */

void
plugin_unload_all ()
{
    while (weechat_plugins)
        plugin_unload (last_weechat_plugin);
}

/*
 * plugin_reload_name: reload a WeeChat plugin by name
 */

void
plugin_reload_name (char *name)
{
    t_weechat_plugin *ptr_plugin;
    char *filename;
    
    ptr_plugin = plugin_search (name);
    if (ptr_plugin)
    {
        filename = strdup (ptr_plugin->filename);
        if (filename)
        {
            plugin_unload (ptr_plugin);
            irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
            gui_printf (NULL, _("Plugin \"%s\" unloaded.\n"), name);
            plugin_load (filename);
            free (filename);
        }
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin \"%s\" not found\n"),
                    WEECHAT_ERROR, name);
    }
}

/*
 * plugin_init: init plugin support
 */

void
plugin_init (int auto_load)
{
    /* read plugins options on disk */
    plugin_config_read ();
    
    /* auto-load plugins if asked */
    if (auto_load)
        plugin_auto_load ();
}

/*
 * plugin_end: end plugin support
 */

void
plugin_end ()
{
    /* write plugins config options */
    plugin_config_write ();
    
    /* unload all plugins */
    plugin_unload_all ();
}
