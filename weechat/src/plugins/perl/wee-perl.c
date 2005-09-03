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

/* wee-perl.c: Perl plugin support for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#undef _
#include "../../common/weechat.h"
#include "../plugins.h"
#include "wee-perl.h"
#include "../../common/command.h"
#include "../../irc/irc.h"
#include "../../gui/gui.h"


static PerlInterpreter *my_perl = NULL;

t_plugin_script *perl_scripts = NULL;
t_plugin_script *last_perl_script = NULL;

extern void boot_DynaLoader (pTHX_ CV* cv);


/******************************* Old interface ********************************/

/*
 * IRC::register: startup function for all WeeChat Perl scripts
 */

/*** DEPRECATED function, kept for compatibility only, please don't update! ***/

static XS (XS_IRC_register)
{
    char *name, *version, *shutdown_func, *description;
    int integer;
    t_plugin_script *ptr_perl_script, *perl_script_found, *new_perl_script;
    dXSARGS;
    
    /* make gcc happy */
    (void) items;
    (void) cv;
    
    name = SvPV (ST (0), integer);
    version = SvPV (ST (1), integer);
    shutdown_func = SvPV (ST (2), integer);
    description = SvPV (ST (3), integer);
    
    perl_script_found = NULL;
    for (ptr_perl_script = perl_scripts; ptr_perl_script;
         ptr_perl_script = ptr_perl_script->next_script)
    {
        if (ascii_strcasecmp (ptr_perl_script->name, name) == 0)
        {
            perl_script_found = ptr_perl_script;
            break;
        }
    }
    
    if (perl_script_found)
    {
        /* error: another script already exists with this name! */
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: unable to register \"%s\" script (another script "
                    "already exists with this name)\n"),
                    "Perl", name);
    }
    else
    {
        /* registering script */
        new_perl_script = (t_plugin_script *)malloc (sizeof (t_plugin_script));
        if (new_perl_script)
        {
            new_perl_script->name = strdup (name);
            new_perl_script->version = strdup (version);
            new_perl_script->shutdown_func = strdup (shutdown_func);
            new_perl_script->description = strdup (description);
            
            /* add new script to list */
            new_perl_script->prev_script = last_perl_script;
            new_perl_script->next_script = NULL;
            if (perl_scripts)
                last_perl_script->next_script = new_perl_script;
            else
                perl_scripts = new_perl_script;
            last_perl_script = new_perl_script;
            
            wee_log_printf (_("Registered %s script: \"%s\", version %s (%s)\n"),
                            "Perl", name, version, description);
        }
        else
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s error: unable to load script \"%s\" (not enough memory)\n"),
                        "Perl", name);
        }
    }
    XST_mPV (0, VERSION);
    XSRETURN (1);
}

/*
 * IRC::print: print message to current buffer
 */

/*** DEPRECATED function, kept for compatibility only, please don't update! ***/

static XS (XS_IRC_print)
{
    int i, integer;
    char *message;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    for (i = 0; i < items; i++)
    {
        message = SvPV (ST (i), integer);
        irc_display_prefix (gui_current_window->buffer, PREFIX_PLUGIN);
        gui_printf (gui_current_window->buffer, "%s", message);
    }
    
    XSRETURN_EMPTY;
}

/*
 * IRC::print_with_channel: print message to a specific channel/server
 *                          (server is optional)
 */

/*** DEPRECATED function, kept for compatibility only, please don't update! ***/

static XS (XS_IRC_print_with_channel)
{
    int integer;
    char *message, *channel, *server = NULL;
    t_gui_buffer *ptr_buffer;
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    /* server specified */
    if (items > 2)
    {
        server = SvPV (ST (2), integer);
        if (!server[0])
            server = NULL;
    }
    
    /* look for buffer for printing message */
    channel = SvPV (ST (1), integer);
    ptr_buffer = NULL;
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (!server || (ascii_strcasecmp (ptr_server->name, server)) == 0)
        {
            for (ptr_channel = ptr_server->channels; ptr_channel;
                 ptr_channel = ptr_channel->next_channel)
            {
                if (ascii_strcasecmp (ptr_channel->name, channel) == 0)
                {
                    ptr_buffer = ptr_channel->buffer;
                    break;
                }
            }
        }
        if (ptr_buffer)
            break;
    }
    
    /* buffer found => display message & return 1 */
    if (ptr_buffer)
    {
        message = SvPV (ST (0), integer);
        irc_display_prefix (ptr_buffer, PREFIX_PLUGIN);
        gui_printf (ptr_buffer, "%s", message);
        XSRETURN_YES;
    }
        
    /* no buffer found => return 0 */
    XSRETURN_NO;
}

/*
 * IRC::print_infobar: print message to infobar
 */

/*** DEPRECATED function, kept for compatibility only, please don't update! ***/

static XS (XS_IRC_print_infobar)
{
    int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (items == 2)
        gui_infobar_printf (SvIV (ST (0)), COLOR_WIN_INFOBAR, SvPV (ST (1), integer));
    else
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Perl", "print_infobar");
    }
    
    XSRETURN_EMPTY;
}

/*
 * IRC::command: send command to server
 */

/*** DEPRECATED function, kept for compatibility only, please don't update! ***/

static XS (XS_IRC_command)
{
    int integer;
    char *server, *command, *command2;
    t_irc_server *ptr_server;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (items == 2)
    {
        server = SvPV (ST (0), integer);
        command = SvPV (ST (1), integer);
        for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
        {
            if (ascii_strcasecmp (ptr_server->name, server) == 0)
                break;
        }
        if (!ptr_server)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s error: server not found for \"%s\" function\n"),
                        "Perl", "command");
        }
    }
    else
    {
        ptr_server = SERVER(gui_current_window->buffer);
        command = SvPV (ST (0), integer);
    }
    
    if (ptr_server)
    {
        command2 = (char *) malloc (strlen (command) + 8);
        strcpy (command2, command);
        if (!strstr (command2, "\r\n"))
            strcat (command2, "\r\n");
        server_sendf (ptr_server, command2);
        free (command2);
    }
    
    XSRETURN_EMPTY;
}

/*
 * IRC::add_message_handler: add handler for messages (privmsg, ...)
 */

/*** DEPRECATED function, kept for compatibility only, please don't update! ***/

static XS (XS_IRC_add_message_handler)
{
    char *name, *function;
    int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) items;
    (void) cv;
    
    name = SvPV (ST (0), integer);
    function = SvPV (ST (1), integer);
    plugin_handler_add (&plugin_msg_handlers, &last_plugin_msg_handler,
                        PLUGIN_TYPE_PERL, name, function);
    XSRETURN_EMPTY;
}

/*
 * IRC::add_command_handler: add command handler (define/redefine commands)
 */

/*** DEPRECATED function, kept for compatibility only, please don't update! ***/

static XS (XS_IRC_add_command_handler)
{
    char *name, *function;
    int integer;
    t_plugin_handler *ptr_plugin_handler;
    dXSARGS;
    
    /* make gcc happy */
    (void) items;
    (void) cv;
    
    name = SvPV (ST (0), integer);
    function = SvPV (ST (1), integer);
    if (!weelist_search (index_commands, name))
        weelist_add (&index_commands, &last_index_command, name);
    ptr_plugin_handler = plugin_handler_search (plugin_cmd_handlers, name);
    if (ptr_plugin_handler)
    {
        free (ptr_plugin_handler->function_name);
        ptr_plugin_handler->function_name = strdup (function);
    }
    else
        plugin_handler_add (&plugin_cmd_handlers, &last_plugin_cmd_handler,
                            PLUGIN_TYPE_PERL, name, function);
    XSRETURN_EMPTY;
}

/*
 * IRC::get_info: get various infos
 */

/*** DEPRECATED function, kept for compatibility only, please don't update! ***/

static XS (XS_IRC_get_info)
{
    char *arg, *info = NULL, *server_name;
    t_irc_server *ptr_server;
    int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (items == 2)
    {
        server_name = SvPV (ST (0), integer);
        arg = SvPV (ST (1), integer);
        ptr_server = server_search (server_name);
        if (!ptr_server)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s error: server not found for \"%s\" function\n"),
                        "Perl", "get_info");
        }
    }
    else
    {
        ptr_server = SERVER(gui_current_window->buffer);
        arg = SvPV (ST (0), integer);
    }
    
    if (arg)
    {
        if ( (ascii_strcasecmp (arg, "0") == 0) || (ascii_strcasecmp (arg, "version") == 0) )
        {
            info = PACKAGE_STRING;
        }
        else if ( ptr_server && ( (ascii_strcasecmp (arg, "1") == 0) || (ascii_strcasecmp (arg, "nick") == 0) ) )
        {
            if (ptr_server->nick)
                info = ptr_server->nick;
        }
        else if ( (ascii_strcasecmp (arg, "2") == 0) || (ascii_strcasecmp (arg, "channel") == 0) )
        {
            if (BUFFER_IS_CHANNEL (gui_current_window->buffer))
                info = CHANNEL (gui_current_window->buffer)->name;
        }
        else if ( ptr_server && ( (ascii_strcasecmp (arg, "3") == 0) || (ascii_strcasecmp (arg, "server") == 0) ) )
        {
            if (ptr_server->name)
                info = ptr_server->name;
        }
        else if ( (ascii_strcasecmp (arg, "4") == 0) || (ascii_strcasecmp (arg, "weechatdir") == 0) )
        {
            info = weechat_home;
        }
        else if ( ptr_server && ( (ascii_strcasecmp (arg, "5") == 0) || (ascii_strcasecmp (arg, "away") == 0) ) )
        {
            XST_mIV (0, SERVER(gui_current_window->buffer)->is_away);
            XSRETURN (1);
            return;
        }
        
        if (info)
            XST_mPV (0, info);
        else
            XST_mPV (0, "");
    }
    
    XSRETURN (1);
}

/******************************* New interface ********************************/

/*
 * weechat::register: startup function for all WeeChat Perl scripts
 */

static XS (XS_weechat_register)
{
    char *name, *version, *shutdown_func, *description;
    int integer;
    t_plugin_script *ptr_perl_script, *perl_script_found, *new_perl_script;
    dXSARGS;
    
    /* make gcc happy */
    (void) items;
    (void) cv;
    
    name = SvPV (ST (0), integer);
    version = SvPV (ST (1), integer);
    shutdown_func = SvPV (ST (2), integer);
    description = SvPV (ST (3), integer);
    
    perl_script_found = NULL;
    for (ptr_perl_script = perl_scripts; ptr_perl_script;
         ptr_perl_script = ptr_perl_script->next_script)
    {
        if (ascii_strcasecmp (ptr_perl_script->name, name) == 0)
        {
            perl_script_found = ptr_perl_script;
            break;
        }
    }
    
    if (perl_script_found)
    {
        /* error: another script already exists with this name! */
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: unable to register \"%s\" script (another script "
                    "already exists with this name)\n"),
                    "Perl", name);
    }
    else
    {
        /* registering script */
        new_perl_script = (t_plugin_script *)malloc (sizeof (t_plugin_script));
        if (new_perl_script)
        {
            new_perl_script->name = strdup (name);
            new_perl_script->version = strdup (version);
            new_perl_script->shutdown_func = strdup (shutdown_func);
            new_perl_script->description = strdup (description);
            
            /* add new script to list */
            new_perl_script->prev_script = last_perl_script;
            new_perl_script->next_script = NULL;
            if (perl_scripts)
                last_perl_script->next_script = new_perl_script;
            else
                perl_scripts = new_perl_script;
            last_perl_script = new_perl_script;
            
            wee_log_printf (_("Registered %s script: \"%s\", version %s (%s)\n"),
                            "Perl", name, version, description);
        }
        else
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s error: unable to load script \"%s\" (not enough memory)\n"),
                        "Perl", name);
        }
    }
    XST_mPV (0, VERSION);
    XSRETURN (1);
}

/*
 * weechat::print: print message into a buffer (current or specified one)
 */

static XS (XS_weechat_print)
{
    int integer;
    char *message, *channel_name, *server_name;
    t_gui_buffer *ptr_buffer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if ((items < 1) || (items > 3))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Perl", "print");
        XSRETURN_NO;
        return;
    }
    
    channel_name = NULL;
    server_name = NULL;
    
    if (items > 1)
    {
        channel_name = SvPV (ST (1), integer);
        if (items > 2)
            server_name = SvPV (ST (2), integer);
    }
    
    ptr_buffer = plugin_find_buffer (server_name, channel_name);
    if (ptr_buffer)
    {
        message = SvPV (ST (0), integer);
        irc_display_prefix (ptr_buffer, PREFIX_PLUGIN);
        gui_printf (ptr_buffer, "%s%s",
                    message,
                    ((strlen (message) == 0) || (message[strlen (message) - 1] != '\n')) ? "\n" : "");
                    
        XSRETURN_YES;
    }
    
    /* buffer not found */
    XSRETURN_NO;
}

/*
 * weechat::print_infobar: print message to infobar
 */

static XS (XS_weechat_print_infobar)
{
    int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (items != 2)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Perl", "print_infobar");
        XSRETURN_NO;
    }
    
    gui_infobar_printf (SvIV (ST (0)), COLOR_WIN_INFOBAR, SvPV (ST (1), integer));
    XSRETURN_YES;
}

/*
 * weechat::command: send command to server
 */

static XS (XS_weechat_command)
{
    int integer;
    char *command, *channel_name, *server_name;
    t_gui_buffer *ptr_buffer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if ((items < 1) || (items > 3))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Perl", "command");
        XSRETURN_NO;
        return;
    }
    
    channel_name = NULL;
    server_name = NULL;
    
    if (items > 1)
    {
        channel_name = SvPV (ST (1), integer);
        if (items > 2)
            server_name = SvPV (ST (2), integer);
    }
    
    ptr_buffer = plugin_find_buffer (server_name, channel_name);
    if (ptr_buffer)
    {
        command = SvPV (ST (0), integer);
        user_command (SERVER(ptr_buffer), ptr_buffer, command);
        XSRETURN_YES;
    }
    
    /* buffer not found */
    XSRETURN_NO;
}

/*
 * weechat::add_message_handler: add handler for messages (privmsg, ...)
 */

static XS (XS_weechat_add_message_handler)
{
    char *name, *function;
    int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (items != 2)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Perl", "add_message_handler");
        XSRETURN_NO;
    }
    
    name = SvPV (ST (0), integer);
    function = SvPV (ST (1), integer);
    plugin_handler_add (&plugin_msg_handlers, &last_plugin_msg_handler,
                        PLUGIN_TYPE_PERL, name, function);
    XSRETURN_YES;
}

/*
 * weechat::add_command_handler: add command handler (define/redefine commands)
 */

static XS (XS_weechat_add_command_handler)
{
    char *name, *function;
    int integer;
    t_plugin_handler *ptr_plugin_handler;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if (items != 2)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Perl", "add_command_handler");
        XSRETURN_NO;
    }
    
    name = SvPV (ST (0), integer);
    function = SvPV (ST (1), integer);
    if (!weelist_search (index_commands, name))
        weelist_add (&index_commands, &last_index_command, name);
    ptr_plugin_handler = plugin_handler_search (plugin_cmd_handlers, name);
    if (ptr_plugin_handler)
    {
        free (ptr_plugin_handler->function_name);
        ptr_plugin_handler->function_name = strdup (function);
    }
    else
        plugin_handler_add (&plugin_cmd_handlers, &last_plugin_cmd_handler,
                            PLUGIN_TYPE_PERL, name, function);
    
    XSRETURN_YES;
}

/*
 * weechat::get_info: get various infos
 */

static XS (XS_weechat_get_info)
{
    char *arg, *info = NULL, *server_name;
    t_irc_server *ptr_server;
    int integer;
    dXSARGS;
    
    /* make gcc happy */
    (void) cv;
    
    if ((items < 1) || (items > 2))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Perl", "get_info");
        XSRETURN_NO;
    }
    
    if (items == 2)
    {
        server_name = SvPV (ST (1), integer);
        ptr_server = server_search (server_name);
        if (!ptr_server)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s error: server not found for \"%s\" function\n"),
                        "Perl", "get_info");
            XSRETURN_NO;
        }
    }
    else
        ptr_server = SERVER(gui_current_window->buffer);
    
    arg = SvPV (ST (0), integer);
    if (arg)
    {
        if ( (ascii_strcasecmp (arg, "0") == 0) || (ascii_strcasecmp (arg, "version") == 0) )
        {
            info = PACKAGE_STRING;
        }
        else if ( ptr_server && ( (ascii_strcasecmp (arg, "1") == 0) || (ascii_strcasecmp (arg, "nick") == 0) ) )
        {
            if (ptr_server->nick)
                info = ptr_server->nick;
        }
        else if ( (ascii_strcasecmp (arg, "2") == 0) || (ascii_strcasecmp (arg, "channel") == 0) )
        {
            if (BUFFER_IS_CHANNEL (gui_current_window->buffer))
                info = CHANNEL (gui_current_window->buffer)->name;
        }
        else if ( ptr_server && ( (ascii_strcasecmp (arg, "3") == 0) || (ascii_strcasecmp (arg, "server") == 0) ) )
        {
            if (ptr_server->name)
                info = ptr_server->name;
        }
        else if ( (ascii_strcasecmp (arg, "4") == 0) || (ascii_strcasecmp (arg, "weechatdir") == 0) )
        {
            info = weechat_home;
        }
        else if ( ptr_server && ( (ascii_strcasecmp (arg, "5") == 0) || (ascii_strcasecmp (arg, "away") == 0) ) )
        {
            XST_mIV (0, SERVER(gui_current_window->buffer)->is_away);
            XSRETURN (1);
            return;
        }
        else if ( (ascii_strcasecmp (arg, "100") == 0) || (ascii_strcasecmp (arg, "dccs") == 0) )
        {
            int nItems = 0;
            t_irc_dcc *p = dcc_list;
            
            POPs;
            if (items == 2)
                POPs;
            
            for(; p; p = p->next_dcc)
            {
                HV *infohash = (HV *) sv_2mortal((SV *) newHV());
                hv_store (infohash, "address32",    9, newSViv(p->addr), 0);
                hv_store (infohash, "cps",          3, newSViv(p->bytes_per_sec), 0);
                hv_store (infohash, "remote_file", 11, newSVpv(p->filename, 0), 0);
                hv_store (infohash, "local_file",  10, newSVpv(p->local_filename, 0), 0);
                hv_store (infohash, "nick",         4, newSVpv(p->nick, 0), 0);
                hv_store (infohash, "port",         4, newSViv(p->port), 0);
                hv_store (infohash, "pos",          3, newSVnv(p->pos), 0);
                hv_store (infohash, "size",         4, newSVnv(p->size), 0);
                hv_store (infohash, "status",       6, newSViv(p->status), 0);
                hv_store (infohash, "type",         4, newSViv(p->type), 0);
                
                XPUSHs(newRV((SV *) infohash));
                ++nItems;
            }
            XSRETURN(nItems);
            return;
        }
        
        if (info)
            XST_mPV (0, info);
        else
            XST_mPV (0, "");
    }
    
    XSRETURN (1);
}

/*
 * xs_init: initialize subroutines
 */

void
xs_init (pTHX)
{
    newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, __FILE__);
    
    /* DEPRECATED & old interface (WeeChat <= 0.1.1), kept for compatibility */
    newXS ("IRC::register", XS_IRC_register, "IRC");
    newXS ("IRC::print", XS_IRC_print, "IRC");
    newXS ("IRC::print_with_channel", XS_IRC_print_with_channel, "IRC");
    newXS ("IRC::print_infobar", XS_IRC_print_infobar, "IRC");
    newXS ("IRC::command", XS_IRC_command, "IRC");
    newXS ("IRC::add_message_handler", XS_IRC_add_message_handler, "IRC");
    newXS ("IRC::add_command_handler", XS_IRC_add_command_handler, "IRC");
    newXS ("IRC::get_info", XS_IRC_get_info, "IRC");
    
    /* new interface (WeeChat >= 0.1.2) */
    newXS ("weechat::register", XS_weechat_register, "weechat");
    newXS ("weechat::print", XS_weechat_print, "weechat");
    newXS ("weechat::print_infobar", XS_weechat_print_infobar, "weechat");
    newXS ("weechat::command", XS_weechat_command, "weechat");
    newXS ("weechat::add_message_handler", XS_weechat_add_message_handler, "weechat");
    newXS ("weechat::add_command_handler", XS_weechat_add_command_handler, "weechat");
    newXS ("weechat::get_info", XS_weechat_get_info, "weechat");
}

/*
 * wee_perl_init: initialize Perl interface for WeeChat
 */

void
wee_perl_init ()
{
    char *perl_args[] = { "", "-e", "0" };
    /* Following Perl code is extracted/modified from X-Chat IRC client */
    /* X-Chat is (c) 1998-2005 Peter Zelezny */
    char *weechat_perl_func =
    {
        "sub wee_perl_load_file"
        "{"
        "    my $filename = shift;"
        "    local $/ = undef;"
        "    open FILE, $filename or return \"__WEECHAT_ERROR__\";"
        "    $_ = <FILE>;"
        "    close FILE;"
        "    return $_;"
        "}"
        "sub wee_perl_load_eval_file"
        "{"
        "    my $filename = shift;"
        "    my $content = wee_perl_load_file ($filename);"
        "    if ($content eq \"__WEECHAT_ERROR__\")"
        "    {"
        "        weechat::print \"WeeChat Error: Perl script '$filename' not found.\\n\", \"\";"
        "        return 1;"
        "    }"
        "    eval $content;"
        "    if ($@)"
        "    {"
        "        weechat::print \"WeeChat error: unable to load Perl script '$filename':\\n\", \"\";"
        "        weechat::print \"$@\\n\";"
        "        return 2;"
        "    }"
        "    return 0;"
        "}"
        "$SIG{__WARN__} = sub { weechat::print \"$_[0]\n\", \"\"; };"
    };
    
    wee_log_printf (_("Loading %s module \"weechat\"\n"), "Perl");
    my_perl = perl_alloc ();
    perl_construct (my_perl);
    perl_parse (my_perl, xs_init, 3, perl_args, NULL);
    eval_pv (weechat_perl_func, TRUE);
}

/*
 * wee_perl_search: search a (loaded) Perl script by name
 */

t_plugin_script *
wee_perl_search (char *name)
{
    t_plugin_script *ptr_perl_script;
    
    for (ptr_perl_script = perl_scripts; ptr_perl_script;
         ptr_perl_script = ptr_perl_script->next_script)
    {
        if (strcmp (ptr_perl_script->name, name) == 0)
            return ptr_perl_script;
    }
    
    /* script not found */
    return NULL;
}

/*
 * wee_perl_exec: execute a Perl script
 */

int
wee_perl_exec (char *function, char *server, char *arguments)
{
    char *argv[3];
    int count, return_code;
    SV *sv;
    
    /* call Perl function */
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(sp);
    if (!server)
        argv[0] = strdup ("");
    else
        argv[0] = server;
    argv[1] = arguments;
    argv[2] = NULL;
    count = perl_call_argv (function, G_EVAL | G_SCALAR, argv);
    SPAGAIN;
    
    /* check if ok */
    sv = GvSV (gv_fetchpv ("@", TRUE, SVt_PV));
    return_code = 1;
    if (SvTRUE (sv))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Perl error: %s"),
                    SvPV (sv, count));
        POPs;
    }
    else
    {
        if (count != 1)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s error: too much values from \"%s\" (%d). Expected: 1.\n"),
                        "Perl", function, count);
        }
        else
            return_code = POPi;
    }
    
    PUTBACK;
    FREETMPS;
    LEAVE;
    
    return return_code;
}

/*
 * wee_perl_load: load a Perl script
 */

int
wee_perl_load (char *filename)
{
    /* execute Perl script */
    wee_log_printf (_("Loading %s script \"%s\"\n"), "Perl", filename);
    irc_display_prefix (NULL, PREFIX_PLUGIN);
    gui_printf (NULL, _("Loading %s script \"%s\"\n"), "Perl", filename);
    return wee_perl_exec ("wee_perl_load_eval_file", filename, "");
}

/*
 * wee_perl_script_free: free a Perl script
 */

void
wee_perl_script_free (t_plugin_script *ptr_perl_script)
{
    t_plugin_script *new_perl_scripts;

    /* remove script from list */
    if (last_perl_script == ptr_perl_script)
        last_perl_script = ptr_perl_script->prev_script;
    if (ptr_perl_script->prev_script)
    {
        (ptr_perl_script->prev_script)->next_script = ptr_perl_script->next_script;
        new_perl_scripts = perl_scripts;
    }
    else
        new_perl_scripts = ptr_perl_script->next_script;
    
    if (ptr_perl_script->next_script)
        (ptr_perl_script->next_script)->prev_script = ptr_perl_script->prev_script;

    /* free data */
    if (ptr_perl_script->name)
        free (ptr_perl_script->name);
    if (ptr_perl_script->version)
        free (ptr_perl_script->version);
    if (ptr_perl_script->shutdown_func)
        free (ptr_perl_script->shutdown_func);
    if (ptr_perl_script->description)
        free (ptr_perl_script->description);
    free (ptr_perl_script);
    perl_scripts = new_perl_scripts;
}

/*
 * wee_perl_unload: unload a Perl script
 */

void
wee_perl_unload (t_plugin_script *ptr_perl_script)
{
    if (ptr_perl_script)
    {
        wee_log_printf (_("Unloading %s script \"%s\"\n"),
                        "Perl", ptr_perl_script->name);
        
        /* call shutdown callback function */
        if (ptr_perl_script->shutdown_func[0])
            wee_perl_exec (ptr_perl_script->shutdown_func, "", "");
        wee_perl_script_free (ptr_perl_script);
    }
}

/*
 * wee_perl_unload_all: unload all Perl scripts
 */

void
wee_perl_unload_all ()
{
    wee_log_printf (_("Unloading all %s scripts...\n"), "Perl");
    while (perl_scripts)
        wee_perl_unload (perl_scripts);
    
    irc_display_prefix (NULL, PREFIX_PLUGIN);
    gui_printf (NULL, _("%s scripts unloaded\n"), "Perl");
}

/*
 * wee_perl_end: shutdown Perl interface
 */

void
wee_perl_end ()
{
    /* unload all scripts */
    wee_perl_unload_all ();
    
    /* free all handlers */
    plugin_handler_free_all_type (&plugin_msg_handlers,
                                  &last_plugin_msg_handler,
                                  PLUGIN_TYPE_PERL);
    plugin_handler_free_all_type (&plugin_cmd_handlers,
                                  &last_plugin_cmd_handler,
                                  PLUGIN_TYPE_PERL);
    
    /* free Perl interpreter */
    if (my_perl)
    {
        perl_destruct (my_perl);
        perl_free (my_perl);
        my_perl = NULL;
    }
}
