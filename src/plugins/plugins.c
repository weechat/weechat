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


t_plugin_handler *plugins_msg_handlers = NULL;
t_plugin_handler *last_plugin_msg_handler = NULL;
t_plugin_handler *plugins_cmd_handlers = NULL;
t_plugin_handler *last_plugin_cmd_handler = NULL;


/*
 * plugins_init: initialize all plugins
 */

void
plugins_init ()
{
    #ifdef PLUGIN_PERL
    wee_perl_init();
    #endif
}

/*
 * plugins_load: load a plugin
 */

void
plugins_load (int plugin_type, char *filename)
{
    switch (plugin_type)
    {
        case PLUGIN_PERL:
            #ifdef PLUGIN_PERL
            wee_perl_load (filename);
            #endif
            break;
        case PLUGIN_PYTHON:
            /* TODO: load Python script */
            break;
        case PLUGIN_RUBY:
            /* TODO: load Ruby script */
            break;
    }
}

/*
 * plugins_unload: unload a plugin
 */

void
plugins_unload (int plugin_type, char *scriptname)
{
    switch (plugin_type)
    {
        case PLUGIN_PERL:
            #ifdef PLUGIN_PERL
            wee_perl_unload (wee_perl_search (scriptname));
            #endif
            break;
        case PLUGIN_PYTHON:
            /* TODO: load Python script */
            break;
        case PLUGIN_RUBY:
            /* TODO: load Ruby script */
            break;
    }
}

/*
 * plugins_msg_handler_add: add a message handler
 */

void
plugins_msg_handler_add (int plugin_type, char *message, char *function)
{
    t_plugin_handler *new_plugin_handler;
    
    new_plugin_handler = (t_plugin_handler *)malloc (sizeof (t_plugin_handler));
    if (new_plugin_handler)
    {
        new_plugin_handler->plugin_type = plugin_type;
        new_plugin_handler->name = strdup (message);
        new_plugin_handler->function_name = strdup (function);
        
        /* add new handler to list */
        new_plugin_handler->prev_handler = last_plugin_msg_handler;
        new_plugin_handler->next_handler = NULL;
        if (plugins_msg_handlers)
            last_plugin_msg_handler->next_handler = new_plugin_handler;
        else
            plugins_msg_handlers = new_plugin_handler;
        last_plugin_msg_handler = new_plugin_handler;
    }
    else
        gui_printf (NULL,
                    _("%s unable to add handler for \"%s\" message (not enough memory)\n"),
                    WEECHAT_ERROR, message);
}

/*
 * plugins_msg_handler_free: free message handler
 */

void
plugins_msg_handler_free (t_plugin_handler *ptr_plugin_handler)
{
    t_plugin_handler *new_plugins_msg_handlers;
    
    /* remove handler from list */
    if (last_plugin_msg_handler == ptr_plugin_handler)
        last_plugin_msg_handler = ptr_plugin_handler->prev_handler;
    if (ptr_plugin_handler->prev_handler)
    {
        (ptr_plugin_handler->prev_handler)->next_handler = ptr_plugin_handler->next_handler;
        new_plugins_msg_handlers = plugins_msg_handlers;
    }
    else
        new_plugins_msg_handlers = ptr_plugin_handler->next_handler;
    
    if (ptr_plugin_handler->next_handler)
        (ptr_plugin_handler->next_handler)->prev_handler = ptr_plugin_handler->prev_handler;

    /* free data */
    free (ptr_plugin_handler->name);
    free (ptr_plugin_handler->function_name);
    free (ptr_plugin_handler);
    plugins_msg_handlers = new_plugins_msg_handlers;
}

/*
 * plugins_remove_all_msg_handlers: remove all message handlers
 */

void
plugins_msg_handlers_free_all ()
{
    while (plugins_msg_handlers)
        plugins_msg_handler_free (plugins_msg_handlers);
}

/*
 * plugins_end: shutdown plugin interface
 */

void
plugins_end ()
{
    plugins_msg_handlers_free_all ();
    
    #ifdef PLUGIN_PERL
    wee_perl_end();
    #endif
}
