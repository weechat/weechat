/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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
#include <string.h>
#include "../common/weechat.h"
#include "plugins.h"
#include "../gui/gui.h"

#ifdef PLUGIN_PERL
#include "perl/wee-perl.h"
#endif


t_plugin_handler *plugin_msg_handlers = NULL;
t_plugin_handler *last_plugin_msg_handler = NULL;

t_plugin_handler *plugin_cmd_handlers = NULL;
t_plugin_handler *last_plugin_cmd_handler = NULL;


/*
 * plugin_init: initialize all plugins
 */

void
plugin_init ()
{
    #ifdef PLUGIN_PERL
    wee_perl_init();
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
            /* TODO: load Python script */
            break;
        case PLUGIN_TYPE_RUBY:
            /* TODO: load Ruby script */
            break;
    }
    #endif
}

/*
 * plugin_unload: unload a plugin
 */

void
plugin_unload (int plugin_type, char *scriptname)
{
    #ifdef PLUGINS
    switch (plugin_type)
    {
        case PLUGIN_TYPE_PERL:
            #ifdef PLUGIN_PERL
            wee_perl_unload (wee_perl_search (scriptname));
            #endif
            break;
        case PLUGIN_TYPE_PYTHON:
            /* TODO: load Python script */
            break;
        case PLUGIN_TYPE_RUBY:
            /* TODO: load Ruby script */
            break;
    }
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
        gui_printf (NULL,
                    _("%s unable to add handler for \"%s\" message (not enough memory)\n"),
                    WEECHAT_ERROR, name);
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
 * plugin_event_msg: IRC message received => call all handlers for this message
 */

void
plugin_event_msg (char *irc_command, char *arguments)
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
                wee_perl_exec (ptr_plugin_handler->function_name, arguments);
            #endif
        }
    }
    #else
    /* make gcc happy */
    (void) irc_command;
    (void) arguments;
    #endif
}

/*
 * plugin_exec_command: execute a command handler
 */

int
plugin_exec_command (char *user_command, char *arguments)
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
                wee_perl_exec (ptr_plugin_handler->function_name, arguments);
            #endif
            
            /* command executed */
            return 1;
        }
    }
    #else
    /* make gcc happy */
    (void) user_command;
    (void) arguments;
    #endif
    
    /* no command executed */
    return 0;
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
}
