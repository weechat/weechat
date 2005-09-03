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

/* wee-ruby.c: Ruby plugin support for WeeChat */


#include <ruby.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#undef _
#include "../../common/weechat.h"
#include "../plugins.h"
#include "wee-ruby.h"
#include "../../common/command.h"
#include "../../irc/irc.h"
#include "../../gui/gui.h"


t_plugin_script *ruby_scripts = NULL;
t_plugin_script *last_ruby_script = NULL;


/*
 * register: startup function for all WeeChat Ruby scripts
 */

static VALUE
wee_ruby_register (VALUE class, VALUE name, VALUE version, VALUE shutdown_func, VALUE description)
{
    char *c_name, *c_version, *c_shutdown_func, *c_description;
    t_plugin_script *ptr_ruby_script, *ruby_script_found, *new_ruby_script;
    
    if (NIL_P (name) || NIL_P (version) || NIL_P (shutdown_func) || NIL_P (description))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Ruby", "register");
        return Qnil;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (version, T_STRING);
    Check_Type (shutdown_func, T_STRING);
    Check_Type (description, T_STRING);
    
    c_name = STR2CSTR (name);
    c_version = STR2CSTR (version);
    c_shutdown_func = STR2CSTR (shutdown_func);
    c_description = STR2CSTR (description);
    
    ruby_script_found = NULL;
    for (ptr_ruby_script = ruby_scripts; ptr_ruby_script;
         ptr_ruby_script = ptr_ruby_script->next_script)
    {
        if (ascii_strcasecmp (ptr_ruby_script->name, c_name) == 0)
        {
            ruby_script_found = ptr_ruby_script;
            break;
        }
    }
    
    if (ruby_script_found)
    {
        /* error: another script already exists with this name! */
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: unable to register \"%s\" script (another script "
                      "already exists with this name)\n"),
                    "Ruby", name);
    }
    else
    {
        /* registering script */
        new_ruby_script = (t_plugin_script *)malloc (sizeof (t_plugin_script));
        if (new_ruby_script)
        {
            new_ruby_script->name = strdup (c_name);
            new_ruby_script->version = strdup (c_version);
            new_ruby_script->shutdown_func = strdup (c_shutdown_func);
            new_ruby_script->description = strdup (c_description);
            
            /* add new script to list */
            new_ruby_script->prev_script = last_ruby_script;
            new_ruby_script->next_script = NULL;
            if (ruby_scripts)
                last_ruby_script->next_script = new_ruby_script;
            else
                ruby_scripts = new_ruby_script;
            last_ruby_script = new_ruby_script;
            
            wee_log_printf (_("Registered %s script: \"%s\", version %s (%s)\n"),
                            "Ruby", c_name, c_version, c_description);
        }
        else
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s error: unable to load script \"%s\" (not enough memory)\n"),
                        "Ruby", c_name);
        }
    }
    
    return Qnil;
}

/*
 * print: print message into a buffer (current or specified one)
 */

static VALUE
wee_ruby_print (VALUE class, VALUE message, VALUE channel_name, VALUE server_name)
{
    char *c_message, *c_channel_name, *c_server_name;
    t_gui_buffer *ptr_buffer;
    
    c_message = NULL;
    c_channel_name = NULL;
    c_server_name = NULL;
    
    if (NIL_P (message))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Ruby", "print");
        return Qnil;
    }
    
    Check_Type (message, T_STRING);
    if (!NIL_P (channel_name))
        Check_Type (channel_name, T_STRING);
    if (!NIL_P (server_name))
        Check_Type (server_name, T_STRING);
    
    c_message = STR2CSTR (message);
    if (!NIL_P (channel_name))
        c_channel_name = STR2CSTR (channel_name);
    if (!NIL_P (server_name))
        c_server_name = STR2CSTR (server_name);
    
    ptr_buffer = plugin_find_buffer (c_server_name, c_channel_name);
    if (ptr_buffer)
    {
        irc_display_prefix (ptr_buffer, PREFIX_PLUGIN);
        gui_printf (ptr_buffer, "%s\n", c_message);
        return INT2FIX (1);
    }
    
    /* buffer not found */
    return INT2FIX (0);
}

/*
 * print_infobar: print message to infobar
 */

static VALUE
wee_ruby_print_infobar (VALUE class, VALUE delay, VALUE message)
{
    int c_delay;
    char *c_message;
    
    c_delay = 1;
    c_message = NULL;
    
    if (NIL_P (delay) || NIL_P (message))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Ruby", "print_infobar");
        return Qfalse;
    }
    
    Check_Type (delay, T_FIXNUM);
    Check_Type (message, T_STRING);
    
    c_delay = FIX2INT (delay);
    c_message = STR2CSTR (message);
    
    gui_infobar_printf (delay, COLOR_WIN_INFOBAR, c_message);
    
    return Qtrue;
}

/*
 * command: send command to server
 */

static VALUE
wee_ruby_command (VALUE class, VALUE command, VALUE channel_name, VALUE server_name)
{
    char *c_command, *c_channel_name, *c_server_name;
    t_gui_buffer *ptr_buffer;
    
    c_command = NULL;
    c_channel_name = NULL;
    c_server_name = NULL;
    
    if (NIL_P (command))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Ruby", "command");
        return Qnil;
    }
    
    Check_Type (command, T_STRING);
    if (!NIL_P (channel_name))
        Check_Type (channel_name, T_STRING);
    if (!NIL_P (server_name))
        Check_Type (server_name, T_STRING);
    
    c_command = STR2CSTR (command);
    if (!NIL_P (channel_name))
        c_channel_name = STR2CSTR (channel_name);
    if (!NIL_P (server_name))
        c_server_name = STR2CSTR (server_name);
    
    ptr_buffer = plugin_find_buffer (c_server_name, c_channel_name);
    if (ptr_buffer)
    {
        user_command (SERVER(ptr_buffer), ptr_buffer, c_command);
        return INT2FIX (1);
    }
    
    /* buffer not found */
    return INT2FIX (0);
}

/*
 * add_message_handler: add handler for messages
 */

static VALUE
wee_ruby_add_message_handler (VALUE class, VALUE message, VALUE function)
{
    char *c_message, *c_function;
    
    if (NIL_P (message) || NIL_P (function))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Ruby", "add_message_handler");
        return Qnil;
    }
    
    Check_Type (message, T_STRING);
    Check_Type (function, T_STRING);
    
    c_message = STR2CSTR (message);
    c_function = STR2CSTR (function);
    
    plugin_handler_add (&plugin_msg_handlers, &last_plugin_msg_handler,
                        PLUGIN_TYPE_RUBY, c_message, c_function);
    
    return Qtrue;
}

/*
 * add_command_handler: define/redefines commands
 */

static VALUE
wee_ruby_add_command_handler (VALUE class, VALUE name, VALUE function)
{
    char *c_name, *c_function;
    t_plugin_handler *ptr_plugin_handler;
    
    if (NIL_P (name) || NIL_P (function))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Ruby", "add_command_handler");
        return Qnil;
    }
    
    Check_Type (name, T_STRING);
    Check_Type (function, T_STRING);
    
    c_name = STR2CSTR (name);
    c_function = STR2CSTR (function);
    
    if (!weelist_search (index_commands, c_name))
        weelist_add (&index_commands, &last_index_command, c_name);
    
    ptr_plugin_handler = plugin_handler_search (plugin_cmd_handlers, c_name);
    if (ptr_plugin_handler)
    {
        free (ptr_plugin_handler->function_name);
        ptr_plugin_handler->function_name = strdup (c_function);
    }
    else
        plugin_handler_add (&plugin_cmd_handlers, &last_plugin_cmd_handler,
                            PLUGIN_TYPE_PYTHON, c_name, c_function);
    
    return Qtrue;
}

/*
 * get_info: get various infos
 */

static VALUE
wee_ruby_get_info (VALUE class, VALUE arg, VALUE server_name)
{
    char *c_arg, *info, *c_server_name;
    t_irc_server *ptr_server;
    
    if (NIL_P (arg))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Ruby", "get_info");
        return Qnil; 
    }
    
    Check_Type (arg, T_STRING);
    if (!NIL_P (server_name))
        Check_Type (server_name, T_STRING);
    
    c_arg = STR2CSTR (arg);
    if (!NIL_P (server_name))
        c_server_name = STR2CSTR (server_name);
    
    if (c_server_name == NULL)
    {
        ptr_server = SERVER(gui_current_window->buffer);
    }
    else
    {
        for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
        {
            if (ascii_strcasecmp (ptr_server->name, c_server_name) == 0)
                break;
        }
        if (!ptr_server)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s error: server not found for \"%s\" function\n"),
                        "Ruby", "get_info");
            return Qnil;
        }
    }
    
    if (ptr_server && c_arg)
    {
        if ( (ascii_strcasecmp (c_arg, "0") == 0) || (ascii_strcasecmp (c_arg, "version") == 0) )
        {
            info = PACKAGE_STRING;
        }
        else if ( (ascii_strcasecmp (c_arg, "1") == 0) || (ascii_strcasecmp (c_arg, "nick") == 0) )
        {
            if (ptr_server->nick)
                info = ptr_server->nick;
        }
        else if ( (ascii_strcasecmp (c_arg, "2") == 0) || (ascii_strcasecmp (c_arg, "channel") == 0) )
        {
            if (BUFFER_IS_CHANNEL (gui_current_window->buffer))
                info = CHANNEL (gui_current_window->buffer)->name;
        }
        else if ( (ascii_strcasecmp (c_arg, "3") == 0) || (ascii_strcasecmp (c_arg, "server") == 0) )
        {
            if (ptr_server->name)
                info = ptr_server->name;
        }
        else if ( (ascii_strcasecmp (c_arg, "4") == 0) || (ascii_strcasecmp (c_arg, "weechatdir") == 0) )
        {
            info = weechat_home;
        }
        else if ( (ascii_strcasecmp (c_arg, "5") == 0) || (ascii_strcasecmp (c_arg, "away") == 0) )
        {	 
            return INT2FIX (SERVER(gui_current_window->buffer)->is_away);
        }
	else if ( (ascii_strcasecmp (c_arg, "100") == 0) || (ascii_strcasecmp (c_arg, "dccs") == 0) )
        {
            /* TODO: build dcc list */
	}
        
        if (info)
            return rb_str_new2 (info);
        else
            return rb_str_new2 ("");
    }
    
    return INT2FIX (1);
}

/*
 * Ruby subroutines
 */

/*
 * wee_ruby_init: initialize Ruby interface for WeeChat
 */

void
wee_ruby_init ()
{

    /* TODO: init Ruby environment */
    /* ruby_init ();
    if ()
    {
        irc_display_prefix (NULL, PREFIX_PLUGIN);
        gui_printf (NULL, _("%s error: error while launching interpreter\n"),
                    "Ruby");
    }
    else
    {
        wee_log_printf (_("Loading %s module \"weechat\"\n"), "Ruby");
    }*/
}

/*
 * wee_ruby_search: search a (loaded) Ruby script by name
 */

t_plugin_script *
wee_ruby_search (char *name)
{
    t_plugin_script *ptr_ruby_script;
    
    for (ptr_ruby_script = ruby_scripts; ptr_ruby_script;
         ptr_ruby_script = ptr_ruby_script->next_script)
    {
        if (strcmp (ptr_ruby_script->name, name) == 0)
            return ptr_ruby_script;
    }
    
    /* script not found */
    return NULL;
}

/*
 * wee_ruby_exec: execute a Ruby script
 */

int
wee_ruby_exec (char *function, char *server, char *arguments)
{  
    /* TODO: exec Ruby script */
}

/*
 * wee_ruby_load: load a Ruby script
 */

int
wee_ruby_load (char *filename)
{
    FILE *fp;
    
    /* TODO: load & exec Ruby script */
    gui_printf (NULL, "Ruby scripts not developed!\n");
    /* execute Ruby script */
    /*wee_log_printf (_("Loading %s script \"%s\"\n"), "Ruby", filename);
    irc_display_prefix (NULL, PREFIX_PLUGIN);
    gui_printf (NULL, _("Loading %s script \"%s\"\n"), "Ruby", filename);
    
    if ((fp = fopen (filename, "r")) == NULL)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: error while opening file \"%s\"\n"),
                    "Ruby", filename);
        return 1;
    }
    
    if (xxxxxxx (fp, filename) != 0)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: error while parsing file \"%s\"\n"),
                    "Ruby", filename);
        return 1;
    }
    
    fclose (fp);
    return 0;*/
}

/*
 * wee_ruby_script_free: free a Ruby script
 */

void
wee_ruby_script_free (t_plugin_script *ptr_ruby_script)
{
    t_plugin_script *new_ruby_scripts;

    /* remove script from list */
    if (last_ruby_script == ptr_ruby_script)
        last_ruby_script = ptr_ruby_script->prev_script;
    if (ptr_ruby_script->prev_script)
    {
        (ptr_ruby_script->prev_script)->next_script = ptr_ruby_script->next_script;
        new_ruby_scripts = ruby_scripts;
    }
    else
        new_ruby_scripts = ptr_ruby_script->next_script;
    
    if (ptr_ruby_script->next_script)
        (ptr_ruby_script->next_script)->prev_script = ptr_ruby_script->prev_script;

    /* free data */
    if (ptr_ruby_script->name)
        free (ptr_ruby_script->name);
    if (ptr_ruby_script->version)
        free (ptr_ruby_script->version);
    if (ptr_ruby_script->shutdown_func)
        free (ptr_ruby_script->shutdown_func);
    if (ptr_ruby_script->description)
        free (ptr_ruby_script->description);
    free (ptr_ruby_script);
    ruby_scripts = new_ruby_scripts;
}

/*
 * wee_ruby_unload: unload a Ruby script
 */

void
wee_ruby_unload (t_plugin_script *ptr_ruby_script)
{
    if (ptr_ruby_script)
    {
        wee_log_printf (_("Unloading %s script \"%s\"\n"),
                        "Ruby", ptr_ruby_script->name);
        
        /* call shutdown callback function */
        if (ptr_ruby_script->shutdown_func[0])
            wee_ruby_exec (ptr_ruby_script->shutdown_func, "", "");
        wee_ruby_script_free (ptr_ruby_script);
    }
}

/*
 * wee_ruby_unload_all: unload all Ruby scripts
 */

void
wee_ruby_unload_all ()
{
    wee_log_printf (_("Unloading all %s scripts...\n"), "Ruby");
    while (ruby_scripts)
        wee_ruby_unload (ruby_scripts);
    
    irc_display_prefix (NULL, PREFIX_PLUGIN);
    gui_printf (NULL, _("%s scripts unloaded\n"), "Ruby");
}

/*
 * wee_ruby_end: shutdown Ruby interface
 */

void
wee_ruby_end ()
{
    /* unload all scripts */
    wee_ruby_unload_all ();
    
    /* free all handlers */
    plugin_handler_free_all_type (&plugin_msg_handlers,
                                  &last_plugin_msg_handler,
                                  PLUGIN_TYPE_RUBY);
    plugin_handler_free_all_type (&plugin_cmd_handlers,
                                  &last_plugin_cmd_handler,
                                  PLUGIN_TYPE_RUBY);
    
    /* TODO: free Ruby interpreter */
    /* free Ruby interpreter */
    /* xxxxx ();
    if ()
    {
        irc_display_prefix (NULL, PREFIX_PLUGIN);
        gui_printf (NULL, _("%s error: error while freeing interpreter\n"),
                    "Ruby");
    }*/
}
