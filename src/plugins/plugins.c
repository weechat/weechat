/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* plugins.c: manages WeeChat plugins (Perl and/or Python and/or Ruby) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "../common/weechat.h"
#include "plugins.h"
#include "../irc/irc.h"
#include "../gui/gui.h"

#ifdef PLUGIN_PERL
#include "perl/wee-perl.h"
#endif

#ifdef PLUGIN_PYTHON
#include "python/wee-python.h"
#endif


char *plugin_name[3] = { "Perl", "Python", "Ruby" };

t_plugin_handler *plugin_msg_handlers = NULL;
t_plugin_handler *last_plugin_msg_handler = NULL;

t_plugin_handler *plugin_cmd_handlers = NULL;
t_plugin_handler *last_plugin_cmd_handler = NULL;


/*
 * plugin_auto_load: auto-load all scripts in a directory
 */

void
plugin_auto_load (int plugin_type, char *directory)
{
    int dir_length;
    char *dir_name, *current_dir;
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    
    /* build directory, adding WeeChat home */
    dir_length = strlen (weechat_home) + strlen (directory) + 2;
    dir_name =
        (char *) malloc (dir_length * sizeof (char));
    snprintf (dir_name, dir_length, "%s%s%s", weechat_home, DIR_SEPARATOR, directory);
    
    /* save working directory */
    current_dir = (char *) malloc (1024 * sizeof (char));
    if (!getcwd (current_dir, 1024 - 1))
    {
        free (current_dir);
        current_dir = NULL;
    }
    
    /* browse autoload directory */
    dir = opendir (dir_name);
    chdir (dir_name);
    if (dir)
    {
        while ((entry = readdir (dir)))
        {
            lstat (entry->d_name, &statbuf);
            if (! S_ISDIR(statbuf.st_mode))
            {
                wee_log_printf (_("Auto-loading %s script: %s%s%s\n"),
                                plugin_name[plugin_type],
                                dir_name, DIR_SEPARATOR, entry->d_name);
                plugin_load (plugin_type, entry->d_name);
            }
        }
        closedir (dir);
    }
    
    /* restore working directory */
    if (current_dir)
    {
        chdir (current_dir);
        free (current_dir);
    }
    free (dir_name);
}

/*
 * plugin_init: initialize all plugins
 */

void
plugin_init ()
{
    #ifdef PLUGIN_PERL
    wee_perl_init();
    plugin_auto_load (PLUGIN_TYPE_PERL, "perl/autoload");
    #endif
    
    #ifdef PLUGIN_PYTHON
    wee_python_init();
    plugin_auto_load (PLUGIN_TYPE_PYTHON, "python/autoload");
    #endif
}

/*
 * plugin_load: load a plugin
 */

void
plugin_load (int plugin_type, char *filename)
{
    #ifdef PLUGINS
    switch (plugin_type)
    {
        case PLUGIN_TYPE_PERL:
            #ifdef PLUGIN_PERL
            wee_perl_load (filename);
            #endif
            break;
        case PLUGIN_TYPE_PYTHON:
            #ifdef PLUGIN_PYTHON
            wee_python_load (filename);
            #endif
            break;
        case PLUGIN_TYPE_RUBY:
            /* TODO: load Ruby script */
            break;
    }
    #else
    /* make gcc happy */
    (void) plugin_type;
    (void) filename;
    #endif
}

/*
 * plugin_handler_search: look for message/command handler
 */

t_plugin_handler *
plugin_handler_search (t_plugin_handler *plugin_handlers, char *name)
{
    t_plugin_handler *ptr_plugin_handler;
    
    for (ptr_plugin_handler = plugin_handlers; ptr_plugin_handler;
         ptr_plugin_handler = ptr_plugin_handler->next_handler)
    {
        /* handler found */
        if (strcasecmp (ptr_plugin_handler->name, name) == 0)
            return ptr_plugin_handler;
    }
    /* handler not found */
    return NULL;
}

/*
 * plugin_handler_add: add a message/command handler
 */

void
plugin_handler_add (t_plugin_handler **plugin_handlers,
                    t_plugin_handler **last_plugin_handler,
                    int plugin_type, char *name, char *function)
{
    t_plugin_handler *new_plugin_handler;
    
    new_plugin_handler = (t_plugin_handler *)malloc (sizeof (t_plugin_handler));
    if (new_plugin_handler)
    {
        new_plugin_handler->plugin_type = plugin_type;
        new_plugin_handler->name = strdup (name);
        new_plugin_handler->function_name = strdup (function);
        new_plugin_handler->running = 0;
        
        /* add new handler to list */
        new_plugin_handler->prev_handler = *last_plugin_handler;
        new_plugin_handler->next_handler = NULL;
        if (*plugin_handlers)
            (*last_plugin_handler)->next_handler = new_plugin_handler;
        else
            *plugin_handlers = new_plugin_handler;
        *last_plugin_handler = new_plugin_handler;
    }
    else
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s unable to add handler for \"%s\" message (not enough memory)\n"),
                    WEECHAT_ERROR, name);
    }
}

/*
 * plugin_handler_free: free message/command handler
 */

void
plugin_handler_free (t_plugin_handler **plugin_handlers,
                     t_plugin_handler **last_plugin_handler,
                     t_plugin_handler *ptr_plugin_handler)
{
    t_plugin_handler *new_plugin_handlers;
    
    /* remove handler from list */
    if (*last_plugin_handler == ptr_plugin_handler)
        *last_plugin_handler = ptr_plugin_handler->prev_handler;
    if (ptr_plugin_handler->prev_handler)
    {
        (ptr_plugin_handler->prev_handler)->next_handler = ptr_plugin_handler->next_handler;
        new_plugin_handlers = *plugin_handlers;
    }
    else
        new_plugin_handlers = ptr_plugin_handler->next_handler;
    
    if (ptr_plugin_handler->next_handler)
        (ptr_plugin_handler->next_handler)->prev_handler = ptr_plugin_handler->prev_handler;

    /* free data */
    free (ptr_plugin_handler->name);
    free (ptr_plugin_handler->function_name);
    free (ptr_plugin_handler);
    *plugin_handlers = new_plugin_handlers;
}

/*
 * plugin_handler_free_all: remove all message/command handlers
 */

void
plugin_handler_free_all (t_plugin_handler **plugin_handlers,
                         t_plugin_handler **last_plugin_handler)
{
    while (*plugin_handlers)
        plugin_handler_free (plugin_handlers, last_plugin_handler,
                             *plugin_handlers);
}

/*
 * plugin_handler_free_all_type: remove all message/command handlers for one type
 */

void
plugin_handler_free_all_type (t_plugin_handler **plugin_handlers,
                              t_plugin_handler **last_plugin_handler,
                              int plugin_type)
{
    t_plugin_handler *ptr_plugin_handler, *new_plugin_handler;
    
    ptr_plugin_handler = *plugin_handlers;
    while (ptr_plugin_handler)
    {
        if (ptr_plugin_handler->plugin_type == plugin_type)
        {
            new_plugin_handler = ptr_plugin_handler->next_handler;
            plugin_handler_free (plugin_handlers, last_plugin_handler,
                                 ptr_plugin_handler);
            ptr_plugin_handler = new_plugin_handler;
        }
        else
            ptr_plugin_handler = ptr_plugin_handler->next_handler;
    }
}

/*
 * plugin_event_msg: IRC message received => call all handlers for this message
 */

void
plugin_event_msg (char *irc_command, char *server, char *arguments)
{
    #ifdef PLUGINS
    t_plugin_handler *ptr_plugin_handler;
    
    for (ptr_plugin_handler = plugin_msg_handlers; ptr_plugin_handler;
         ptr_plugin_handler = ptr_plugin_handler->next_handler)
    {
        if (strcasecmp (ptr_plugin_handler->name, irc_command) == 0)
        {
            #ifdef PLUGIN_PERL
            if (ptr_plugin_handler->plugin_type == PLUGIN_TYPE_PERL)
            {
                if (ptr_plugin_handler->running == 0)
                {
                    ptr_plugin_handler->running = 1;
                    wee_perl_exec (ptr_plugin_handler->function_name, server, arguments);
                    ptr_plugin_handler->running = 0;
                }
            }
            #endif
            #ifdef PLUGIN_PYTHON
            if (ptr_plugin_handler->plugin_type == PLUGIN_TYPE_PYTHON)
            {
                if (ptr_plugin_handler->running == 0)
                {
                    ptr_plugin_handler->running = 1;
                    wee_python_exec (ptr_plugin_handler->function_name, server, arguments);
                    ptr_plugin_handler->running = 0;
                }
            }
            #endif
        }
    }
    #else
    /* make gcc happy */
    (void) irc_command;
    (void) arguments;
    (void) server;
    #endif
}

/*
 * plugin_exec_command: execute a command handler
 */

int
plugin_exec_command (char *user_command, char *server, char *arguments)
{
    #ifdef PLUGINS
    t_plugin_handler *ptr_plugin_handler;
    
    for (ptr_plugin_handler = plugin_cmd_handlers; ptr_plugin_handler;
         ptr_plugin_handler = ptr_plugin_handler->next_handler)
    {
        if (strcasecmp (ptr_plugin_handler->name, user_command) == 0)
        {
            #ifdef PLUGIN_PERL
            if (ptr_plugin_handler->plugin_type == PLUGIN_TYPE_PERL)
            {
                if (ptr_plugin_handler->running == 0)
                {
                    ptr_plugin_handler->running = 1;
                    wee_perl_exec (ptr_plugin_handler->function_name, server, arguments);
                    ptr_plugin_handler->running = 0;
                }
            }
            #endif
            #ifdef PLUGIN_PYTHON
            if (ptr_plugin_handler->plugin_type == PLUGIN_TYPE_PYTHON)
            {
                if (ptr_plugin_handler->running == 0)
                {
                    ptr_plugin_handler->running = 1;
                    wee_python_exec (ptr_plugin_handler->function_name, server, arguments);
                    ptr_plugin_handler->running = 0;
                }
            }
            #endif
            
            /* command executed */
            return 1;
        }
    }
    #else
    /* make gcc happy */
    (void) user_command;
    (void) arguments;
    (void) server;
    #endif
    
    /* no command executed */
    return 0;
}

/*
 * plugin_find_buffer: find a buffer for text display or command execution
 */

t_gui_buffer *
plugin_find_buffer (char *server, char *channel)
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    
    ptr_server = NULL;
    ptr_channel = NULL;
    ptr_buffer = NULL;
    
    if (server && server[0])
    {
        ptr_server = server_search (server);
        if (!ptr_server)
            return NULL;
    }
    else
    {
        ptr_server = SERVER(gui_current_window->buffer);
        if (!ptr_server)
            ptr_server = SERVER(gui_buffers);
    }
    
    if (channel && channel[0])
    {
        if (ptr_server)
        {
            ptr_channel = channel_search (ptr_server, channel);
            if (ptr_channel)
                ptr_buffer = ptr_channel->buffer;
        }
    }
    else
    {
        ptr_buffer = gui_current_window->buffer;
        if (ptr_buffer->dcc)
            ptr_buffer = gui_buffers;
    }
    
    if (!ptr_buffer)
        return NULL;
    
    return (ptr_buffer->dcc) ? NULL : ptr_buffer;
}

/*
 * plugin_unload: unload all scripts for a plugin type
 */

void
plugin_unload (int plugin_type, char *scriptname)
{
    /* make gcc happy */
    (void) scriptname;
    
    #ifdef PLUGINS
    switch (plugin_type)
    {
        case PLUGIN_TYPE_PERL:
            #ifdef PLUGIN_PERL
            /* unload one Perl script is not allowed */
            wee_perl_end ();
            wee_perl_init ();
            #endif
            break;
        case PLUGIN_TYPE_PYTHON:
            #ifdef PLUGIN_PYTHON
            wee_python_end ();
            wee_python_init ();
            #endif
            break;
        case PLUGIN_TYPE_RUBY:
            /* TODO: unload Ruby scripts */
            break;
    }
    #else
    /* make gcc happy */
    (void) plugin_type;
    #endif
}

/*
 * plugin_end: shutdown plugin interface
 */

void
plugin_end ()
{
    plugin_handler_free_all (&plugin_msg_handlers, &last_plugin_msg_handler);
    plugin_handler_free_all (&plugin_cmd_handlers, &last_plugin_cmd_handler);
    
    #ifdef PLUGIN_PERL
    wee_perl_end();
    #endif
    
    #ifdef PLUGIN_PYTHON
    wee_python_end();
    #endif    
}
