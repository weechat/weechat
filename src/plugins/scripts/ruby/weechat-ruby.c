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

#include <stdlib.h>
#include <string.h>
#undef _
#include "../../weechat-plugin.h"
#include "../weechat-script.h"


char plugin_name[]        = "Ruby";
char plugin_version[]     = "0.1";
char plugin_description[] = "Ruby scripts support";

t_weechat_plugin *ruby_plugin;

t_plugin_script *ruby_scripts = NULL;
t_plugin_script *ruby_current_script = NULL;
char *ruby_current_script_filename = NULL;


/*
 * weechat_ruby_exec: execute a Ruby script
 */

int
weechat_ruby_exec (t_weechat_plugin *plugin,
                   t_plugin_script *script,
                   char *function, char *server, char *arguments)
{
    /* make gcc happy */
    (void) plugin;
    (void) script;
    (void) function;
    (void) server;
    (void) arguments;
    
    /* TODO: exec Ruby script */
    return PLUGIN_RC_OK;
}

/*
 * weechat_ruby_handler: general message and command handler for Ruby
 */

int
weechat_ruby_handler (t_weechat_plugin *plugin,
                      char *server, char *command, char *arguments,
                      char *handler_args, void *handler_pointer)
{
    /* make gcc happy */
    (void) command;
    
    return weechat_ruby_exec (plugin, (t_plugin_script *)handler_pointer,
                              handler_args, server, arguments);
}

/*
 * weechat_ruby_register: startup function for all WeeChat Ruby scripts
 */

static VALUE
weechat_ruby_register (VALUE class, VALUE name, VALUE version,
                       VALUE shutdown_func, VALUE description)
{
    char *c_name, *c_version, *c_shutdown_func, *c_description;
    
    /* make gcc happy */
    (void) class;
    
    if (NIL_P (name) || NIL_P (version) || NIL_P (shutdown_func) || NIL_P (description))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"register\" function");
        return INT2FIX (0);
    }
    
    Check_Type (name, T_STRING);
    Check_Type (version, T_STRING);
    Check_Type (shutdown_func, T_STRING);
    Check_Type (description, T_STRING);
    
    c_name = STR2CSTR (name);
    c_version = STR2CSTR (version);
    c_shutdown_func = STR2CSTR (shutdown_func);
    c_description = STR2CSTR (description);
    
    if (weechat_script_search (ruby_plugin, &ruby_scripts, c_name))
    {
        /* error: another scripts already exists with this name! */
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to register "
                                    "\"%s\" script (another script "
                                    "already exists with this name)",
                                    c_name);
        return INT2FIX (0);
    }
    
    /* register script */
    ruby_current_script = weechat_script_add (ruby_plugin,
                                              &ruby_scripts,
                                              (ruby_current_script_filename) ?
                                              ruby_current_script_filename : "",
                                              c_name, c_version, c_shutdown_func,
                                              c_description);
    if (ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby: registered script \"%s\", "
                                    "version %s (%s)",
                                    c_name, c_version, c_description);
    }
    else
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to load script "
                                    "\"%s\" (not enough memory)",
                                    c_name);
        return INT2FIX (0);
    }
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_print: print message into a buffer (current or specified one)
 */

static VALUE
weechat_ruby_print (VALUE class, VALUE message, VALUE channel_name,
                    VALUE server_name)
{
    char *c_message, *c_channel_name, *c_server_name;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to print message, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_message = NULL;
    c_channel_name = NULL;
    c_server_name = NULL;
    
    if (NIL_P (message))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"print\" function");
        return INT2FIX (0);
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
    
    ruby_plugin->printf (ruby_plugin,
                         c_server_name, c_channel_name,
                         "%s", c_message);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_print_infobar: print message to infobar
 */

static VALUE
weechat_ruby_print_infobar (VALUE class, VALUE delay, VALUE message)
{
    int c_delay;
    char *c_message;

    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to print infobar message, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_delay = 1;
    c_message = NULL;
    
    if (NIL_P (delay) || NIL_P (message))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"print_infobar\" function");
        return INT2FIX (0);
    }
    
    Check_Type (delay, T_FIXNUM);
    Check_Type (message, T_STRING);
    
    c_delay = FIX2INT (delay);
    c_message = STR2CSTR (message);
    
    ruby_plugin->infobar_printf (ruby_plugin, c_delay, c_message);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_command: send command to server
 */

static VALUE
weechat_ruby_command (VALUE class, VALUE command, VALUE channel_name,
                      VALUE server_name)
{
    char *c_command, *c_channel_name, *c_server_name;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to run command, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_command = NULL;
    c_channel_name = NULL;
    c_server_name = NULL;
    
    if (NIL_P (command))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"command\" function");
        return INT2FIX (0);
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
    
    ruby_plugin->exec_command (ruby_plugin,
                               c_server_name, c_channel_name,
                               c_command);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_add_message_handler: add handler for messages
 */

static VALUE
weechat_ruby_add_message_handler (VALUE class, VALUE message, VALUE function)
{
    char *c_message, *c_function;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to add message handler, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_message = NULL;
    c_function = NULL;
    
    if (NIL_P (message) || NIL_P (function))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"add_message_handler\" function");
        return INT2FIX (0);
    }
    
    Check_Type (message, T_STRING);
    Check_Type (function, T_STRING);
    
    c_message = STR2CSTR (message);
    c_function = STR2CSTR (function);
    
    if (ruby_plugin->msg_handler_add (ruby_plugin, c_message,
                                      weechat_ruby_handler, c_function,
                                      (void *)ruby_current_script))
        return INT2FIX (1);
    
    return INT2FIX (0);
}

/*
 * weechat_add_command_handler: define/redefines commands
 */

static VALUE
weechat_ruby_add_command_handler (VALUE class, VALUE command, VALUE function,
                                  VALUE description, VALUE arguments,
                                  VALUE arguments_description)
{
    char *c_command, *c_function, *c_description, *c_arguments;
    char *c_arguments_description;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to add command handler, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_command = NULL;
    c_function = NULL;
    c_description = NULL;
    c_arguments = NULL;
    c_arguments_description = NULL;
    
    if (NIL_P (command) || NIL_P (function))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"add_command_handler\" function");
        return INT2FIX (0);
    }
    
    Check_Type (command, T_STRING);
    Check_Type (function, T_STRING);
    if (!NIL_P (description))
        Check_Type (description, T_STRING);
    if (!NIL_P (arguments))
        Check_Type (arguments, T_STRING);
    if (!NIL_P (arguments_description))
        Check_Type (arguments_description, T_STRING);
    
    c_command = STR2CSTR (command);
    c_function = STR2CSTR (function);
    if (!NIL_P (description))
        c_description = STR2CSTR (description);
    if (!NIL_P (arguments))
        c_arguments = STR2CSTR (arguments);
    if (!NIL_P (arguments_description))
        c_arguments_description = STR2CSTR (arguments_description);
    
    if (ruby_plugin->cmd_handler_add (ruby_plugin,
                                      c_command,
                                      c_description,
                                      c_arguments,
                                      c_arguments_description,
                                      weechat_ruby_handler,
                                      c_function,
                                      (void *)ruby_current_script))
        return INT2FIX (1);
    
    return INT2FIX (0);
}

/*
 * weechat_remove_handler: remove a handler
 */

static VALUE
weechat_ruby_remove_handler (VALUE class, VALUE command, VALUE function)
{
    char *c_command, *c_function;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to remove handler, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_command = NULL;
    c_function = NULL;
    
    if (NIL_P (command) || NIL_P (function))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"remove_handler\" function");
        return INT2FIX (0);
    }
    
    Check_Type (command, T_STRING);
    Check_Type (function, T_STRING);
    
    c_command = STR2CSTR (command);
    c_function = STR2CSTR (function);
    
    weechat_script_remove_handler (ruby_plugin, ruby_current_script,
                                   c_command, c_function);
    
    return INT2FIX (1);
}

/*
 * weechat_ruby_get_info: get various infos
 */

static VALUE
weechat_ruby_get_info (VALUE class, VALUE arg, VALUE server_name)
{
    char *c_arg, *c_server_name, *info;
    VALUE return_value;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to get info, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_arg = NULL;
    c_server_name = NULL;
    
    if (NIL_P (arg))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"get_info\" function");
        return INT2FIX (0);
    }
    
    Check_Type (arg, T_STRING);
    if (!NIL_P (server_name))
        Check_Type (server_name, T_STRING);
    
    c_arg = STR2CSTR (arg);
    if (!NIL_P (server_name))
        c_server_name = STR2CSTR (server_name);
    
    if (c_arg)
    {
        info = ruby_plugin->get_info (ruby_plugin, c_arg, c_server_name);
        
        if (info)
        {
            return_value = rb_str_new2 (info);
            free (info);
            return return_value;
        }
    }
    
    return rb_str_new2 ("");
}

/*
 * weechat_ruby_get_dcc_info: get infos about DCC
 */

static VALUE
weechat_ruby_get_dcc_info (VALUE class)
{
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to get DCC info, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    /* TODO: get dcc info for Ruby */
    return INT2FIX (1);
}

/*
 * weechat_ruby_get_config: get value of a WeeChat config option
 */

static VALUE
weechat_ruby_get_config (VALUE class, VALUE option)
{
    char *c_option, *return_value;
    VALUE ruby_return_value;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to get config option, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"get_config\" function");
        return INT2FIX (0);
    }
    
    Check_Type (option, T_STRING);
    c_option = STR2CSTR (option);
    
    if (c_option)
    {
        return_value = ruby_plugin->get_config (ruby_plugin, c_option);
        
        if (return_value)
        {
            ruby_return_value = rb_str_new2 (return_value);
            free (return_value);
            return ruby_return_value;
        }
    }
    
    return rb_str_new2 ("");
}

/*
 * weechat_ruby_set_config: set value of a WeeChat config option
 */

static VALUE
weechat_ruby_set_config (VALUE class, VALUE option, VALUE value)
{
    char *c_option, *c_value;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to set config option, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_option = NULL;
    c_value = NULL;
    
    if (NIL_P (option))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"set_config\" function");
        return INT2FIX (0);
    }
    
    Check_Type (option, T_STRING);
    Check_Type (value, T_STRING);
    
    c_option = STR2CSTR (option);
    c_value = STR2CSTR (value);
    
    if (c_option && c_value)
    {
        if (ruby_plugin->set_config (ruby_plugin, c_option, c_value))
            return INT2FIX (1);
    }
    
    return INT2FIX (0);
}

/*
 * weechat_ruby_get_plugin_config: get value of a plugin config option
 */

static VALUE
weechat_ruby_get_plugin_config (VALUE class, VALUE option)
{
    char *c_option, *return_value;
    VALUE ruby_return_value;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to get plugin config option, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_option = NULL;
    
    if (NIL_P (option))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"get_plugin_config\" function");
        return INT2FIX (0);
    }
    
    Check_Type (option, T_STRING);
    c_option = STR2CSTR (option);
    
    if (c_option)
    {
        return_value = weechat_script_get_plugin_config (ruby_plugin,
                                                         ruby_current_script,
                                                         c_option);
        
        if (return_value)
        {
            ruby_return_value = rb_str_new2 (return_value);
            free (return_value);
            return ruby_return_value;
        }
    }
    
    return rb_str_new2 ("");
}

/*
 * weechat_ruby_set_plugin_config: set value of a plugin config option
 */

static VALUE
weechat_ruby_set_plugin_config (VALUE class, VALUE option, VALUE value)
{
    char *c_option, *c_value;
    
    /* make gcc happy */
    (void) class;
    
    if (!ruby_current_script)
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: unable to set plugin config option, "
                                    "script not initialized");
        return INT2FIX (0);
    }
    
    c_option = NULL;
    c_value = NULL;
    
    if (NIL_P (option))
    {
        ruby_plugin->printf_server (ruby_plugin,
                                    "Ruby error: wrong parameters for "
                                    "\"set_plugin_config\" function");
        return INT2FIX (0);
    }
    
    Check_Type (option, T_STRING);
    Check_Type (value, T_STRING);
    
    c_option = STR2CSTR (option);
    c_value = STR2CSTR (value);
    
    if (c_option && c_value)
    {
        if (weechat_script_set_plugin_config (ruby_plugin,
                                              ruby_current_script,
                                              c_option, c_value))
            return INT2FIX (1);
    }
    
    return INT2FIX (0);
}

/*
 * Ruby subroutines
 */

/* TODO: write Ruby functions interface */

/*
 * weechat_ruby_load: load a Ruby script
 */

int
weechat_ruby_load (t_weechat_plugin *plugin, char *filename)
{
    /* make gcc happy */
    (void) plugin;
    (void) filename;
    
    /* TODO: load & exec Ruby script */
    return 0;
}

/*
 * weechat_ruby_unload: unload a Ruby script
 */

void
weechat_ruby_unload (t_weechat_plugin *plugin, t_plugin_script *script)
{
    /* make gcc happy */
    (void) plugin;
    (void) script;
    
    /* TODO: unload a Ruby script */
}

/*
 * weechat_ruby_unload_name: unload a Ruby script by name
 */

void
weechat_ruby_unload_name (t_weechat_plugin *plugin, char *name)
{
    /* make gcc happy */
    (void) plugin;
    (void) name;
    
    /* TODO: unload a Ruby script by name */
}

/*
 * weechat_ruby_unload_all: unload all Ruby scripts
 */

void
weechat_ruby_unload_all (t_weechat_plugin *plugin)
{
    /* make gcc happy */
    (void) plugin;
    
    /* TODO: unload all Ruby scripts */
}

/*
 * weechat_ruby_cmd: /ruby command handler
 */

int
weechat_ruby_cmd (t_weechat_plugin *plugin,
                  char *server, char *command, char *arguments,
                  char *handler_args, void *handler_pointer)
{
    int argc, path_length, handler_found;
    char **argv, *path_script, *dir_home;
    t_plugin_script *ptr_script;
    t_plugin_handler *ptr_handler;
    
    /* make gcc happy */
    (void) server;
    (void) command;
    (void) handler_args;
    (void) handler_pointer;
    
    if (arguments)
        argv = plugin->explode_string (plugin, arguments, " ", 0, &argc);
    else
    {
        argv = NULL;
        argc = 0;
    }
    
    switch (argc)
    {
        case 0:
            /* list registered Ruby scripts */
            plugin->printf_server (plugin, "");
            plugin->printf_server (plugin, "Registered Ruby scripts:");
            if (ruby_scripts)
            {
                for (ptr_script = ruby_scripts;
                     ptr_script; ptr_script = ptr_script->next_script)
                {
                    plugin->printf_server (plugin, "  %s v%s%s%s",
                                           ptr_script->name,
                                           ptr_script->version,
                                           (ptr_script->description[0]) ? " - " : "",
                                           ptr_script->description);
                }
            }
            else
                plugin->printf_server (plugin, "  (none)");
            
            /* list Ruby message handlers */
            plugin->printf_server (plugin, "");
            plugin->printf_server (plugin, "Ruby message handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_MESSAGE)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->printf_server (plugin, "  IRC(%s) => Ruby(%s)",
                                           ptr_handler->irc_command,
                                           ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->printf_server (plugin, "  (none)");
            
            /* list Ruby command handlers */
            plugin->printf_server (plugin, "");
            plugin->printf_server (plugin, "Ruby command handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_COMMAND)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->printf_server (plugin, "  /%s => Ruby(%s)",
                                           ptr_handler->command,
                                           ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->printf_server (plugin, "  (none)");
            break;
        case 1:
            if (plugin->ascii_strcasecmp (plugin, argv[0], "autoload") == 0)
                weechat_script_auto_load (plugin, "ruby", weechat_ruby_load);
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "reload") == 0)
            {
                weechat_ruby_unload_all (plugin);
                weechat_script_auto_load (plugin, "ruby", weechat_ruby_load);
            }
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "unload") == 0)
                weechat_ruby_unload_all (plugin);
            break;
        case 2:
            if (plugin->ascii_strcasecmp (plugin, argv[0], "load") == 0)
            {
                /* load Ruby script */
                if ((strstr (argv[1], "/")) || (strstr (argv[1], "\\")))
                    path_script = NULL;
                else
                {
                    dir_home = plugin->get_info (plugin, "weechat_dir", NULL);
                    if (dir_home)
                    {
                        path_length = strlen (dir_home) + strlen (argv[1]) + 16;
                        path_script = (char *) malloc (path_length * sizeof (char));
                        if (path_script)
                            snprintf (path_script, path_length, "%s/ruby/%s",
                                      dir_home, argv[1]);
                        else
                            path_script = NULL;
                        free (dir_home);
                    }
                    else
                        path_script = NULL;
                }
                weechat_ruby_load (plugin, (path_script) ? path_script : argv[1]);
                if (path_script)
                    free (path_script);
            }
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "unload") == 0)
            {
                /* unload Ruby script */
                weechat_ruby_unload_name (plugin, argv[1]);
            }
            else
            {
                plugin->printf_server (plugin,
                                       "Ruby error: unknown option for "
                                       "\"ruby\" command");
            }
            break;
        default:
            plugin->printf_server (plugin,
                                   "Ruby error: wrong argument count for \"ruby\" command");
    }
    
    if (argv)
        plugin->free_exploded_string (plugin, argv);
    
    return 1;
}

/*
 * weechat_plugin_init: initialize Ruby plugin
 */

int
weechat_plugin_init (t_weechat_plugin *plugin)
{
    ruby_plugin = plugin;
    
    plugin->printf_server (plugin, "Loading Ruby module \"weechat\"");
    
    /* TODO: initialize Ruby interpreter */
    
    
    plugin->cmd_handler_add (plugin, "ruby",
                             "list/load/unload Ruby scripts",
                             "[load filename] | [autoload] | [reload] | [unload]",
                             "filename: Ruby script (file) to load\n\n"
                             "Without argument, /ruby command lists all loaded Ruby scripts.",
                             weechat_ruby_cmd, NULL, NULL);
    
    plugin->mkdir_home (plugin, "ruby");
    plugin->mkdir_home (plugin, "ruby/autoload");
    
    weechat_script_auto_load (plugin, "ruby", weechat_ruby_load);
    
    /* init ok */
    return PLUGIN_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Ruby interface
 */

void
weechat_plugin_end (t_weechat_plugin *plugin)
{
    /* unload all scripts */
    weechat_ruby_unload_all (plugin);
    
    /* TODO: free interpreter */
    
    
    ruby_plugin->printf_server (ruby_plugin,
                                "Ruby plugin ended");
}
