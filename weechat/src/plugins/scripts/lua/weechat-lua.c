/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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

/* weechat-lua.c: Lua plugin support for WeeChat */


#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#undef _
#include "../../weechat-plugin.h"
#include "../weechat-script.h"


char plugin_name[]        = "Lua";
char plugin_version[]     = "0.1";
char plugin_description[] = "Lua scripts support";

t_weechat_plugin *lua_plugin;

t_plugin_script *lua_scripts = NULL;
t_plugin_script *lua_current_script = NULL;
char *lua_current_script_filename = NULL;
lua_State *lua_current_interpreter = NULL;


/*
 * weechat_lua_exec: execute a Lua script
 */

int
weechat_lua_exec (t_weechat_plugin *plugin,
		  t_plugin_script *script,
		  char *function, char *arg1, char *arg2, char *arg3)
{

    lua_current_interpreter = script->interpreter;

    lua_getglobal (lua_current_interpreter, function);
    lua_current_script = script;
    
    if (arg1)
    {
        lua_pushstring (lua_current_interpreter, (arg1) ? arg1 : "");
        if (arg2)
        {
            lua_pushstring (lua_current_interpreter, (arg2) ? arg2 : "");
            if (arg3)
                lua_pushstring (lua_current_interpreter, (arg3) ? arg3 : "");
        }
    }
    
    if (lua_pcall (lua_current_interpreter,
                   (arg1) ? ((arg2) ? ((arg3) ? 3 : 2) : 1) : 0, 1, 0) != 0)
    {
	plugin->print_server (plugin,
                              "Lua error: unable to run function \"%s\"",
                              function);
	plugin->print_server (plugin,
                              "Lua error: %s",
                              lua_tostring (lua_current_interpreter, -1));
        return PLUGIN_RC_KO;
    }
    
    return lua_tonumber (lua_current_interpreter, -1);
}

/*
 * weechat_lua_cmd_msg_handler: general command/message handler for Lua
 */

int
weechat_lua_cmd_msg_handler (t_weechat_plugin *plugin,
                             int argc, char **argv,
                             char *handler_args, void *handler_pointer)
{
    if (argc >= 3)
        return weechat_lua_exec (plugin, (t_plugin_script *)handler_pointer,
                                 handler_args, argv[0], argv[2], NULL);
    else
        return PLUGIN_RC_KO;
}

/*
 * weechat_lua_timer_handler: general timer handler for Lua
 */

int
weechat_lua_timer_handler (t_weechat_plugin *plugin,
                           int argc, char **argv,
                           char *handler_args, void *handler_pointer)
{
    /* make gcc happy */
    (void) argc;
    (void) argv;
    
    return weechat_lua_exec (plugin, (t_plugin_script *)handler_pointer,
                             handler_args, NULL, NULL, NULL);
}

/*
 * weechat_lua_keyboard_handler: general keyboard handler for Lua
 */

int
weechat_lua_keyboard_handler (t_weechat_plugin *plugin,
                              int argc, char **argv,
                              char *handler_args, void *handler_pointer)
{
    if (argc >= 2)
        return weechat_lua_exec (plugin, (t_plugin_script *)handler_pointer,
                                 handler_args, argv[0], argv[1], argv[2]);
    else
        return PLUGIN_RC_KO;
}

/*
 * weechat_lua_register: startup function for all WeeChat Lua scripts
 */

static int
weechat_lua_register (lua_State *L)
{
    const char *name, *version, *shutdown_func, *description;
    int n;
    
    /* make gcc happy */
    (void) L;

    lua_current_script = NULL;
    
    name = NULL;
    version = NULL;
    shutdown_func = NULL;
    description = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n != 4)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"register\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    name = lua_tostring (lua_current_interpreter, -4);
    version = lua_tostring (lua_current_interpreter, -3);
    shutdown_func = lua_tostring (lua_current_interpreter, -2);
    description = lua_tostring (lua_current_interpreter, -1);

    if (weechat_script_search (lua_plugin, &lua_scripts, (char *) name))
    {
        /* error: another scripts already exists with this name! */
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to register "
                                  "\"%s\" script (another script "
                                  "already exists with this name)",
                                  name);
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    /* register script */
    lua_current_script = weechat_script_add (lua_plugin,
					     &lua_scripts,
					     (lua_current_script_filename) ?
					     lua_current_script_filename : "",
					     (char *) name, 
					     (char *) version, 
					     (char *) shutdown_func,
					     (char *) description);
    if (lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua: registered script \"%s\", "
                                  "version %s (%s)",
                                  name, version, description);
    }
    else
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to load script "
                                  "\"%s\" (not enough memory)",
                                  name);
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_print: print message into a buffer (current or specified one)
 */

static int
weechat_lua_print  (lua_State *L)
{
    const char *message, *channel_name, *server_name;
    int n;
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to print message, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    message = NULL;
    channel_name = NULL;
    server_name = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    switch (n)
    {
        case 1:
            message = lua_tostring (lua_current_interpreter, -1);
            break;
        case 2:
            channel_name = lua_tostring (lua_current_interpreter, -2);
            message = lua_tostring (lua_current_interpreter, -1);
            break;
        case 3:
            server_name = lua_tostring (lua_current_interpreter, -3);
            channel_name = lua_tostring (lua_current_interpreter, -2);
            message = lua_tostring (lua_current_interpreter, -1);
            break;
        default:
            lua_plugin->print_server (lua_plugin,
                                      "Lua error: wrong parameters for "
                                      "\"print\" function");
            lua_pushnumber (lua_current_interpreter, 0);
            return 1;
    }
    
    lua_plugin->print (lua_plugin,
                       (char *) server_name,
                       (char *) channel_name,
                       "%s", (char *) message);
    
    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_print_infobar: print message to infobar
 */

static int
weechat_lua_print_infobar  (lua_State *L)
{
    const char *message;
    int delay, n;
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to print infobar message, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    delay = 1;
    message = NULL;

    n = lua_gettop (lua_current_interpreter);

    if (n != 2)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"print_infobar\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    delay = lua_tonumber (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);
    
    lua_plugin->print_infobar (lua_plugin, delay, "%s", (char *) message);
    
    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_remove_infobar: remove message(s) in infobar
 */

static int
weechat_lua_remove_infobar  (lua_State *L)
{
    int n, how_many;
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to remove infobar message(s), "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    how_many = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n == 1)
        how_many = lua_tonumber (lua_current_interpreter, -1);
    
    lua_plugin->infobar_remove (lua_plugin, how_many);
    
    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_print: log message in server/channel (current or specified ones)
 */

static int
weechat_lua_log  (lua_State *L)
{
    const char *message, *channel_name, *server_name;
    int n;
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to print message, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    message = NULL;
    channel_name = NULL;
    server_name = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    switch (n)
    {
        case 1:
            message = lua_tostring (lua_current_interpreter, -1);
            break;
        case 2:
            channel_name = lua_tostring (lua_current_interpreter, -2);
            message = lua_tostring (lua_current_interpreter, -1);
            break;
        case 3:
            server_name = lua_tostring (lua_current_interpreter, -3);
            channel_name = lua_tostring (lua_current_interpreter, -2);
            message = lua_tostring (lua_current_interpreter, -1);
            break;
        default:
            lua_plugin->print_server (lua_plugin,
                                      "Lua error: wrong parameters for "
                                      "\"log\" function");
            lua_pushnumber (lua_current_interpreter, 0);
            return 1;
    }
    
    lua_plugin->log (lua_plugin,
		     (char *) server_name,
		     (char *) channel_name,
		     "%s", (char *) message);
    
    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_command: send command to server
 */

static int
weechat_lua_command  (lua_State *L)
{
    const char *command, *channel_name, *server_name;
    int n;
    
    /* make gcc happy */
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to run command, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }

    command = NULL;
    channel_name = NULL;
    server_name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    switch (n)
    {
        case 1:
            command = lua_tostring (lua_current_interpreter, -1);
            break;
        case 2:
            channel_name = lua_tostring (lua_current_interpreter, -2);
            command = lua_tostring (lua_current_interpreter, -1);
            break;
        case 3:
            server_name = lua_tostring (lua_current_interpreter, -3);
            channel_name = lua_tostring (lua_current_interpreter, -2);
            command = lua_tostring (lua_current_interpreter, -1);
            break;
        default:
            lua_plugin->print_server (lua_plugin,
                                      "Lua error: wrong parameters for "
                                      "\"command\" function");
            lua_pushnumber (lua_current_interpreter, 0);
            return 1;
    }

    lua_plugin->exec_command (lua_plugin,
			      (char *) server_name,
			      (char *) channel_name,
			      (char *) command);

    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_add_message_handler: add handler for messages
 */

static int
weechat_lua_add_message_handler  (lua_State *L)
{
    const char *irc_command, *function;
    int n;
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to add message handler, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    irc_command = NULL;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n != 2)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"add_message_handler\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }

    irc_command = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    if (!lua_plugin->msg_handler_add (lua_plugin, (char *) irc_command,
				     weechat_lua_cmd_msg_handler,
                                      (char *) function,
				     (void *)lua_current_script))
    {
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }

    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_add_command_handler: define/redefines commands
 */

static int
weechat_lua_add_command_handler  (lua_State *L)
{
    const char *command, *function, *description, *arguments, *arguments_description;
    const char *completion_template;
    int n;
    
    /* make gcc happy */
    (void) L;
        
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to add command handler, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    command = NULL;
    function = NULL;
    description = NULL;
    arguments = NULL;
    arguments_description = NULL;
    completion_template = NULL;

    n = lua_gettop (lua_current_interpreter);
    
    switch (n)
    {
        case 2:
            command = lua_tostring (lua_current_interpreter, -2);
            function = lua_tostring (lua_current_interpreter, -1);
            break;
        case 6:
            command = lua_tostring (lua_current_interpreter, -6);
            function = lua_tostring (lua_current_interpreter, -5);
            description = lua_tostring (lua_current_interpreter, -4);
            arguments = lua_tostring (lua_current_interpreter, -3);
            arguments_description = lua_tostring (lua_current_interpreter, -2);
            completion_template = lua_tostring (lua_current_interpreter, -1);
            break;
        default:
            lua_plugin->print_server (lua_plugin,
                                      "Lua error: wrong parameters for "
                                      "\"add_command_handler\" function");
            lua_pushnumber (lua_current_interpreter, 0);
            return 1;
    }
    
    if (!lua_plugin->cmd_handler_add (lua_plugin,
				      (char *) command,
				      (char *) description,
				      (char *) arguments,
				      (char *) arguments_description,
				      (char *) completion_template,
				      weechat_lua_cmd_msg_handler,
				      (char *) function,
				      (void *)lua_current_script))
    {
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_add_timer_handler: add a timer handler
 */

static int
weechat_lua_add_timer_handler  (lua_State *L)
{
    int interval;
    const char *function;
    int n;
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to add timer handler, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    interval = 10;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n != 2)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"add_timer_handler\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    interval = lua_tonumber (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    if (!lua_plugin->timer_handler_add (lua_plugin, interval,
                                        weechat_lua_timer_handler,
                                        (char *) function,
                                        (void *)lua_current_script))
    {
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }

    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_add_keyboard_handler: add a keyboard handler
 */

static int
weechat_lua_add_keyboard_handler  (lua_State *L)
{
    const char *function;
    int n;
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to add keyboard handler, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n != 1)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"add_keyboard_handler\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    function = lua_tostring (lua_current_interpreter, -1);
    
    if (!lua_plugin->keyboard_handler_add (lua_plugin,
                                           weechat_lua_keyboard_handler,
                                           (char *) function,
                                           (void *)lua_current_script))
    {
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }

    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_remove_handler: remove a command/message handler
 */

static int
weechat_lua_remove_handler (lua_State *L)
{
    const char *command, *function;
    int n;
    
    /* make gcc happy */
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to remove handler, "
                                  "script not initialized");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    command = NULL;
    function = NULL;
 
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 2)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"remove_handler\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }

    command = lua_tostring (lua_current_interpreter, -2);
    function = lua_tostring (lua_current_interpreter, -1);
    
    weechat_script_remove_handler (lua_plugin, lua_current_script,
                                   (char *) command, (char *) function);
    
    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_remove_timer_handler: remove a timer handler
 */

static int
weechat_lua_remove_timer_handler (lua_State *L)
{
    const char *function;
    int n;
    
    /* make gcc happy */
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to remove timer handler, "
                                  "script not initialized");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    function = NULL;
 
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 1)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"remove_timer_handler\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }

    function = lua_tostring (lua_current_interpreter, -1);
    
    weechat_script_remove_timer_handler (lua_plugin, lua_current_script,
                                         (char *) function);
    
    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_remove_keyboard_handler: remove a keyboard handler
 */

static int
weechat_lua_remove_keyboard_handler (lua_State *L)
{
    const char *function;
    int n;
    
    /* make gcc happy */
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to remove keyboard handler, "
                                  "script not initialized");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    function = NULL;
 
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 1)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"remove_keyboard_handler\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }

    function = lua_tostring (lua_current_interpreter, -1);
    
    weechat_script_remove_keyboard_handler (lua_plugin, lua_current_script,
                                            (char *) function);
    
    lua_pushnumber (lua_current_interpreter, 1);
    return 1;
}

/*
 * weechat_lua_get_info: get various infos
 */

static int
weechat_lua_get_info  (lua_State *L)
{
    const char *arg, *server_name;
    char *info;
    int n;
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get info, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    arg = NULL;
    server_name = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    switch (n)
    {
        case 1:
            arg = lua_tostring (lua_current_interpreter, -1);
            break;
        case 2:
            arg = lua_tostring (lua_current_interpreter, -2);
            server_name = lua_tostring (lua_current_interpreter, -1);
            break;
        default:
            lua_plugin->print_server (lua_plugin,
                                      "Lua error: wrong parameters for "
                                      "\"get_info\" function");
            lua_pushnumber (lua_current_interpreter, 0);
            return 1;
    }

    info = lua_plugin->get_info (lua_plugin, (char *) arg, (char *) server_name);
    if (info)
	lua_pushstring (lua_current_interpreter, info);
    else
	lua_pushstring (lua_current_interpreter, "");
    
    return  1;
}

/*
 * weechat_lua_get_dcc_info: get infos about DCC
 */

static int
weechat_lua_get_dcc_info  (lua_State *L)
{
    t_plugin_dcc_info *dcc_info, *ptr_dcc;
    char timebuffer1[64];
    char timebuffer2[64];
    struct in_addr in;
    int i;
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get DCC info, "
                                  "script not initialized");
	lua_pushnil (lua_current_interpreter);
	return 1;
    }
    
    dcc_info = lua_plugin->get_dcc_info (lua_plugin);
    if (!dcc_info)
    {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }
    
    lua_newtable (lua_current_interpreter);

    for (i = 0, ptr_dcc = dcc_info; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc, i++)
    {
	strftime(timebuffer1, sizeof(timebuffer1), "%F %T",
		 localtime(&ptr_dcc->start_time));
	strftime(timebuffer2, sizeof(timebuffer2), "%F %T",
		 localtime(&ptr_dcc->start_transfer));
	in.s_addr = htonl(ptr_dcc->addr);
	
	lua_pushnumber (lua_current_interpreter, i);
	lua_newtable (lua_current_interpreter);

	lua_pushstring (lua_current_interpreter, "server");
	lua_pushstring (lua_current_interpreter, ptr_dcc->server);
	lua_rawset (lua_current_interpreter, -3);
		    
	lua_pushstring (lua_current_interpreter, "channel");
	lua_pushstring (lua_current_interpreter, ptr_dcc->channel);
	lua_rawset (lua_current_interpreter, -3);
		    
	lua_pushstring (lua_current_interpreter, "type");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->type);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "status");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->status);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "start_time");
	lua_pushstring (lua_current_interpreter, timebuffer1);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "start_transfer");
	lua_pushstring (lua_current_interpreter, timebuffer2);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "address");
	lua_pushstring (lua_current_interpreter, inet_ntoa(in));
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "port");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->port);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "nick");
	lua_pushstring (lua_current_interpreter, ptr_dcc->nick);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "remote_file");
	lua_pushstring (lua_current_interpreter, ptr_dcc->filename);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "local_file");
	lua_pushstring (lua_current_interpreter, ptr_dcc->local_filename);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "filename_suffix");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->filename_suffix);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "size");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->size);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "pos");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->pos);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "start_resume");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->start_resume);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "cps");
	lua_pushnumber (lua_current_interpreter, ptr_dcc->bytes_per_sec);
	lua_rawset (lua_current_interpreter, -3);

	lua_rawset (lua_current_interpreter, -3);
    }
    
    lua_plugin->free_dcc_info (lua_plugin, dcc_info);
    
    return 1;
}

/*
 * weechat_lua_get_config: get value of a WeeChat config option
 */

static int
weechat_lua_get_config  (lua_State *L)
{
    const char *option;
    char *return_value;
    int n;
    
    /* make gcc happy */
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get config option, "
                                  "script not initialized");	
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = NULL;
 
    n = lua_gettop (lua_current_interpreter);

    if (n != 1)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"get_config\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    return_value = lua_plugin->get_config (lua_plugin, (char *) option);    
    if (return_value)
	lua_pushstring (lua_current_interpreter, return_value);
    else
	lua_pushstring (lua_current_interpreter, "");
    
    return 1;
}

/*
 * weechat_lua_set_config: set value of a WeeChat config option
 */

static int
weechat_lua_set_config  (lua_State *L)
{
    const char *option, *value;
    int n;
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to set config option, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = NULL;
    value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 2)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"set_config\" function");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    if (lua_plugin->set_config (lua_plugin, (char *) option, (char *) value))
	lua_pushnumber (lua_current_interpreter, 1);
    else
	lua_pushnumber (lua_current_interpreter, 0);
    
    return 1;
}

/*
 * weechat_lua_get_plugin_config: get value of a plugin config option
 */

static int
weechat_lua_get_plugin_config  (lua_State *L)
{
    const char *option;
    char *return_value;
    int n;
    
    /* make gcc happy */
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get plugin config option, "
                                  "script not initialized");	
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = NULL;
 
    n = lua_gettop (lua_current_interpreter);

    if (n != 1)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"get_plugin_config\" function");
        lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    return_value = weechat_script_get_plugin_config (lua_plugin,
						     lua_current_script,
						     (char *) option);
    if (return_value)
	lua_pushstring (lua_current_interpreter, return_value);
    else
	lua_pushstring (lua_current_interpreter, "");
    
    return 1;
}

/*
 * weechat_lua_set_plugin_config: set value of a plugin config option
 */

static int
weechat_lua_set_plugin_config  (lua_State *L)
{
    const char *option, *value;
    int n;
    
    /* make gcc happy */
    (void) L;
 	
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to set plugin config option, "
                                  "script not initialized");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = NULL;
    value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 2)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"set_plugin_config\" function");
	lua_pushnumber (lua_current_interpreter, 0);
	return 1;
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);

    if (weechat_script_set_plugin_config (lua_plugin,
					  lua_current_script,
					  (char *) option, (char *) value))
	lua_pushnumber (lua_current_interpreter, 1);
    else
	lua_pushnumber (lua_current_interpreter, 0);
    
    return 1;
}

/*
 * weechat_lua_get_server_info: get infos about servers
 */

static int
weechat_lua_get_server_info  (lua_State *L)
{
    t_plugin_server_info *server_info, *ptr_server;
    char timebuffer[64];
    
    /* make gcc happy */
    (void) L;
    
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get server infos, "
                                  "script not initialized");
	lua_pushnil (lua_current_interpreter);
	return 1;
    }
    
    server_info = lua_plugin->get_server_info (lua_plugin);
    if  (!server_info) {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }

    lua_newtable (lua_current_interpreter);

    for (ptr_server = server_info; ptr_server; ptr_server = ptr_server->next_server)
    {
	strftime(timebuffer, sizeof(timebuffer), "%F %T",
		 localtime(&ptr_server->away_time));
	
	lua_pushstring (lua_current_interpreter, ptr_server->name);
	lua_newtable (lua_current_interpreter);
	
	lua_pushstring (lua_current_interpreter, "autoconnect");
	lua_pushnumber (lua_current_interpreter, ptr_server->autoconnect);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "autoreconnect");
	lua_pushnumber (lua_current_interpreter, ptr_server->autoreconnect);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "autoreconnect_delay");
	lua_pushnumber (lua_current_interpreter, ptr_server->autoreconnect_delay);
	lua_rawset (lua_current_interpreter, -3);
		
	lua_pushstring (lua_current_interpreter, "command_line");
	lua_pushnumber (lua_current_interpreter, ptr_server->command_line);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "address");
	lua_pushstring (lua_current_interpreter, ptr_server->address);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "port");
	lua_pushnumber (lua_current_interpreter, ptr_server->port);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "ipv6");
	lua_pushnumber (lua_current_interpreter, ptr_server->ipv6);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "ssl");
	lua_pushnumber (lua_current_interpreter, ptr_server->ssl);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "password");
	lua_pushstring (lua_current_interpreter, ptr_server->password);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "nick1");
	lua_pushstring (lua_current_interpreter, ptr_server->nick1);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "nick2");
	lua_pushstring (lua_current_interpreter, ptr_server->nick2);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "nick3");
	lua_pushstring (lua_current_interpreter, ptr_server->nick3);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "username");
	lua_pushstring (lua_current_interpreter, ptr_server->username);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "realname");
	lua_pushstring (lua_current_interpreter, ptr_server->realname);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "command");
	lua_pushstring (lua_current_interpreter, ptr_server->command);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "command_delay");
	lua_pushnumber (lua_current_interpreter, ptr_server->command_delay);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "autojoin");
	lua_pushstring (lua_current_interpreter, ptr_server->autojoin);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "autorejoin");
	lua_pushnumber (lua_current_interpreter, ptr_server->autorejoin);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "notify_levels");
	lua_pushstring (lua_current_interpreter, ptr_server->notify_levels);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "charset_decode_iso");
	lua_pushstring (lua_current_interpreter, ptr_server->charset_decode_iso);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "charset_decode_utf");
	lua_pushstring (lua_current_interpreter, ptr_server->charset_decode_utf);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "charset_encode");
	lua_pushstring (lua_current_interpreter, ptr_server->charset_encode);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "is_connected");
	lua_pushnumber (lua_current_interpreter, ptr_server->is_connected);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "ssl_connected");
	lua_pushnumber (lua_current_interpreter, ptr_server->ssl_connected);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "nick");
	lua_pushstring (lua_current_interpreter, ptr_server->nick);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "away_time");
	lua_pushstring (lua_current_interpreter, timebuffer);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "lag");
	lua_pushnumber (lua_current_interpreter, ptr_server->lag);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_rawset (lua_current_interpreter, -3);
    }

    lua_plugin->free_server_info(lua_plugin, server_info);
    
    return 1;
}

/*
 * weechat_lua_get_channel_info: get infos about channels
 */

static int
weechat_lua_get_channel_info  (lua_State *L)
{
    t_plugin_channel_info *channel_info, *ptr_channel;
    const char *server;
    int n;
    
    /* make gcc happy */
    (void) L;
 
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get channel infos, "
                                  "script not initialized");
	lua_pushnil (lua_current_interpreter);
	return 1;
    }
    
    server = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 1)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"get_channel_info\" function");
        lua_pushnil (lua_current_interpreter);
	return 1;
    }

    server = lua_tostring (lua_current_interpreter, -1);
    
    channel_info = lua_plugin->get_channel_info (lua_plugin, (char *) server);
    if  (!channel_info)
    {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }

    lua_newtable (lua_current_interpreter);

    for (ptr_channel = channel_info; ptr_channel; ptr_channel = ptr_channel->next_channel)
    {
	lua_pushstring (lua_current_interpreter, ptr_channel->name);
	lua_newtable (lua_current_interpreter);
	
	lua_pushstring (lua_current_interpreter, "type");
	lua_pushnumber (lua_current_interpreter, ptr_channel->type);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "topic");
	lua_pushstring (lua_current_interpreter, ptr_channel->topic);
	lua_rawset (lua_current_interpreter, -3);
	
	lua_pushstring (lua_current_interpreter, "modes");
	lua_pushstring (lua_current_interpreter, ptr_channel->modes);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "limit");
	lua_pushnumber (lua_current_interpreter, ptr_channel->limit);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "key");
	lua_pushstring (lua_current_interpreter, ptr_channel->key);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "nicks_count");
	lua_pushnumber (lua_current_interpreter, ptr_channel->nicks_count);
	lua_rawset (lua_current_interpreter, -3);

	lua_rawset (lua_current_interpreter, -3);
    }    

    lua_plugin->free_channel_info(lua_plugin, channel_info);
    
    return 1;
}

/*
 * weechat_lua_get_nick_info: get infos about nicks
 */

static int
weechat_lua_get_nick_info  (lua_State *L)
{
    t_plugin_nick_info *nick_info, *ptr_nick;
    const char *server, *channel;
    int n;
    
    /* make gcc happy */
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get nick infos, "
                                  "script not initialized");
		lua_pushnil (lua_current_interpreter);
	return 1;
    }
    
    server = NULL;
    channel = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 2)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"get_nick_info\" function");
        lua_pushnil (lua_current_interpreter);
	return 1;
    }

    server = lua_tostring (lua_current_interpreter, -2);
    channel = lua_tostring (lua_current_interpreter, -1);
    
    nick_info = lua_plugin->get_nick_info (lua_plugin, (char *) server, (char *) channel);
    if  (!nick_info)
    {
	lua_pushboolean (lua_current_interpreter, 0);
	return 1;
    }

    lua_newtable (lua_current_interpreter);
    
    for(ptr_nick = nick_info; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
	lua_pushstring (lua_current_interpreter, ptr_nick->nick);
	lua_newtable (lua_current_interpreter);
	
	lua_pushstring (lua_current_interpreter, "flags");
	lua_pushnumber (lua_current_interpreter, ptr_nick->flags);
	lua_rawset (lua_current_interpreter, -3);

	lua_pushstring (lua_current_interpreter, "host");
	lua_pushstring (lua_current_interpreter,
			ptr_nick->host ? ptr_nick->host : "");
	lua_rawset (lua_current_interpreter, -3);
	
	lua_rawset (lua_current_interpreter, -3);
    }
    
    lua_plugin->free_nick_info(lua_plugin, nick_info);
    
    return 1;
}

/*
 * weechat_lua_get_irc_color:
 *          get the numeric value which identify an irc color by its name
 */

static int
weechat_lua_get_irc_color (lua_State *L)
{
    const char *color;
    int n;
    
    /* make gcc happy */
    (void) L;
     
    if (!lua_current_script)
    {
        lua_plugin->print_server (lua_plugin,
                                  "Lua error: unable to get irc color, "
                                  "script not initialized");
        lua_pushnumber (lua_current_interpreter, -1);
	return 1;
    }
    
    color = NULL;
 
    n = lua_gettop (lua_current_interpreter);
    
    if (n != 1)
    {
	lua_plugin->print_server (lua_plugin,
                                  "Lua error: wrong parameters for "
                                  "\"get_irc_color\" function");
        lua_pushnumber (lua_current_interpreter, -1);
	return 1;
    }

    color = lua_tostring (lua_current_interpreter, -1);
    
    lua_pushnumber (lua_current_interpreter,
		    lua_plugin->get_irc_color (lua_plugin, (char *) color));
    return 1;
}

/*
 * Lua constant as functions
 */

static int
weechat_lua_constant_plugin_rc_ok  (lua_State *L)
{
    /* make gcc happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, PLUGIN_RC_OK);
    return 1;
}

static int
weechat_lua_constant_plugin_rc_ko  (lua_State *L)
{
    /* make gcc happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, PLUGIN_RC_KO);
    return 1;
}
    
static int
weechat_lua_constant_plugin_rc_ok_ignore_weechat  (lua_State *L)
{
    /* make gcc happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, PLUGIN_RC_OK_IGNORE_WEECHAT);
    return 1;
}

static int
weechat_lua_constant_plugin_rc_ok_ignore_plugins  (lua_State *L)
{
    /* make gcc happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, PLUGIN_RC_OK_IGNORE_PLUGINS);
    return 1;
}

static int
weechat_lua_constant_plugin_rc_ok_ignore_all  (lua_State *L)
{
    /* make gcc happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, PLUGIN_RC_OK_IGNORE_ALL);
    return 1;
}

/*
 * Lua subroutines
 */

static
const struct luaL_reg weechat_lua_funcs[] = {
    { "register", weechat_lua_register},
    { "print", weechat_lua_print},
    { "print_infobar", weechat_lua_print_infobar},
    { "remove_infobar", weechat_lua_remove_infobar},
    { "log", weechat_lua_log},
    { "command", weechat_lua_command},
    { "add_message_handler", weechat_lua_add_message_handler},
    { "add_command_handler", weechat_lua_add_command_handler},
    { "add_timer_handler", weechat_lua_add_timer_handler},
    { "add_keyboard_handler", weechat_lua_add_keyboard_handler},
    { "remove_handler", weechat_lua_remove_handler},
    { "remove_timer_handler", weechat_lua_remove_timer_handler},
    { "remove_keyboard_handler", weechat_lua_remove_keyboard_handler},
    { "get_info", weechat_lua_get_info},
    { "get_dcc_info", weechat_lua_get_dcc_info},
    { "get_config", weechat_lua_get_config},
    { "set_config", weechat_lua_set_config},
    { "get_plugin_config", weechat_lua_get_plugin_config},
    { "set_plugin_config", weechat_lua_set_plugin_config},
    { "get_server_info", weechat_lua_get_server_info},
    { "get_channel_info", weechat_lua_get_channel_info},
    { "get_nick_info", weechat_lua_get_nick_info},
    { "get_irc_color", weechat_lua_get_irc_color},
    /* define constants as function which returns values */
    { "PLUGIN_RC_OK", weechat_lua_constant_plugin_rc_ok},
    { "PLUGIN_RC_KO", weechat_lua_constant_plugin_rc_ko},
    { "PLUGIN_RC_OK_IGNORE_WEECHAT", weechat_lua_constant_plugin_rc_ok_ignore_weechat},
    { "PLUGIN_RC_OK_IGNORE_PLUGINS", weechat_lua_constant_plugin_rc_ok_ignore_plugins},
    { "PLUGIN_RC_OK_IGNORE_ALL", weechat_lua_constant_plugin_rc_ok_ignore_all},
    { NULL, NULL}
};

int
weechat_lua_load (t_weechat_plugin *plugin, char *filename)
{
    FILE *fp;
    char *weechat_lua_code = {
	"weechat_outputs = {\n"
	"    write = function (self, str)\n"
	"        weechat.print(\"Lua stdout/stderr : \" .. str)\n"
        "    end\n"
	"}\n"
	"io.stdout = weechat_outputs\n"
	"io.stderr = weechat_outputs\n"
    };
    
    plugin->print_server (plugin, "Loading Lua script \"%s\"", filename);
    
    if ((fp = fopen (filename, "r")) == NULL)
    {
        plugin->print_server (plugin,
                              "Lua error: unable to open file \"%s\"",
                              filename);
        return 0;
    }

    lua_current_script = NULL;
    
    lua_current_interpreter = lua_open ();

    if (lua_current_interpreter == NULL)
    {
        plugin->print_server (plugin,
                              "Lua error: unable to create new sub-interpreter");
        fclose (fp);
        return 0;
    }

    luaopen_base (lua_current_interpreter);
    luaopen_table (lua_current_interpreter);
    luaopen_io (lua_current_interpreter);
    luaopen_string (lua_current_interpreter);
    luaopen_math (lua_current_interpreter);
    luaopen_debug (lua_current_interpreter);
    
    luaL_openlib (lua_current_interpreter, "weechat", weechat_lua_funcs, 0);
    
    if (lua_dostring (lua_current_interpreter, weechat_lua_code) != 0)
        plugin->print_server (plugin,
                              "Lua warning: unable to redirect stdout and stderr");
    
    lua_current_script_filename = strdup (filename);
    
    if (luaL_loadfile (lua_current_interpreter, filename) != 0)
    {
        plugin->print_server (plugin,
                              "Lua error: unable to load file \"%s\"",
                              filename);
	plugin->print_server (plugin,
                              "Lua error: %s",
                              lua_tostring (lua_current_interpreter, -1));
        free (lua_current_script_filename);
        lua_close (lua_current_interpreter);
        fclose (fp);
        return 0;
    }

    if (lua_pcall (lua_current_interpreter, 0, 0, 0) != 0)
    {
        plugin->print_server (plugin,
                              "Lua error: unable to execute file \"%s\"",
                              filename);
	plugin->print_server (plugin,
                              "Lua error: %s",
                              lua_tostring (lua_current_interpreter, -1));
        free (lua_current_script_filename);
        lua_close (lua_current_interpreter);
        fclose (fp);
	/* if script was registered, removing from list */
	if (lua_current_script != NULL)
	    weechat_script_remove (plugin, &lua_scripts, lua_current_script);
        return 0;
    }

    fclose (fp);
    free (lua_current_script_filename);
    
    if (lua_current_script == NULL)
    {
        plugin->print_server (plugin,
                              "Lua error: function \"register\" not found "
                              "in file \"%s\"",
                              filename);
	lua_close (lua_current_interpreter);
        return 0;
    }
    
    lua_current_script->interpreter = (lua_State *) lua_current_interpreter;
    
    return 1;
}

/*
 * weechat_lua_unload: unload a Lua script
 */

void
weechat_lua_unload (t_weechat_plugin *plugin, t_plugin_script *script)
{
    plugin->print_server (plugin,
                          "Unloading Lua script \"%s\"",
                          script->name);
    
    if (script->shutdown_func[0])
        weechat_lua_exec (plugin, script, script->shutdown_func, NULL, NULL, NULL);
    
    lua_close (script->interpreter);
    
    weechat_script_remove (plugin, &lua_scripts, script);
}

/*
 * weechat_lua_unload_name: unload a Lua script by name
 */

void
weechat_lua_unload_name (t_weechat_plugin *plugin, char *name)
{
    t_plugin_script *ptr_script;
    
    ptr_script = weechat_script_search (plugin, &lua_scripts, name);
    if (ptr_script)
    {
        weechat_lua_unload (plugin, ptr_script);
        plugin->print_server (plugin,
                              "Lua script \"%s\" unloaded",
                              name);
    }
    else
    {
        plugin->print_server (plugin,
                              "Lua error: script \"%s\" not loaded",
                              name);
    }
}

/*
 * weechat_lua_unload_all: unload all Lua scripts
 */

void
weechat_lua_unload_all (t_weechat_plugin *plugin)
{
    plugin->print_server (plugin,
                          "Unloading all Lua scripts");
    while (lua_scripts)
        weechat_lua_unload (plugin, lua_scripts);

    plugin->print_server (plugin,
                          "Lua scripts unloaded");
}

/*
 * weechat_lua_cmd: /lua command handler
 */

int
weechat_lua_cmd (t_weechat_plugin *plugin,
                 int cmd_argc, char **cmd_argv,
                 char *handler_args, void *handler_pointer)
{
    int argc, handler_found;
    char **argv, *path_script;
    t_plugin_script *ptr_script;
    t_plugin_handler *ptr_handler;
    
    /* make gcc happy */
    (void) handler_args;
    (void) handler_pointer;
    
    if (cmd_argc < 3)
        return PLUGIN_RC_KO;
    
    if (cmd_argv[2])
        argv = plugin->explode_string (plugin, cmd_argv[2], " ", 0, &argc);
    else
    {
        argv = NULL;
        argc = 0;
    }
    
    switch (argc)
    {
        case 0:
            /* list registered Lua scripts */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Registered Lua scripts:");
            if (lua_scripts)
            {
                for (ptr_script = lua_scripts;
                     ptr_script; ptr_script = ptr_script->next_script)
                {
                    plugin->print_server (plugin, "  %s v%s%s%s",
                                          ptr_script->name,
                                          ptr_script->version,
                                          (ptr_script->description[0]) ? " - " : "",
                                          ptr_script->description);
                }
            }
            else
                plugin->print_server (plugin, "  (none)");
            
            /* list Lua message handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Lua message handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_MESSAGE)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->print_server (plugin, "  IRC(%s) => Lua(%s)",
                                          ptr_handler->irc_command,
                                          ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->print_server (plugin, "  (none)");
            
            /* list Lua command handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Lua command handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_COMMAND)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->print_server (plugin, "  /%s => Lua(%s)",
                                          ptr_handler->command,
                                          ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->print_server (plugin, "  (none)");
            
            /* list Lua timer handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Lua timer handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_TIMER)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->print_server (plugin, "  %d seconds => Lua(%s)",
                                          ptr_handler->interval,
                                          ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->print_server (plugin, "  (none)");
            
            /* list Lua keyboard handlers */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Lua keyboard handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_KEYBOARD)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->print_server (plugin, "  Lua(%s)",
                                          ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->print_server (plugin, "  (none)");
            break;
        case 1:
            if (plugin->ascii_strcasecmp (plugin, argv[0], "autoload") == 0)
                weechat_script_auto_load (plugin, "lua", weechat_lua_load);
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "reload") == 0)
            {
                weechat_lua_unload_all (plugin);
                weechat_script_auto_load (plugin, "lua", weechat_lua_load);
            }
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "unload") == 0)
                weechat_lua_unload_all (plugin);
            break;
        case 2:
            if (plugin->ascii_strcasecmp (plugin, argv[0], "load") == 0)
            {
                /* load Lua script */
                path_script = weechat_script_search_full_name (plugin, "lua", argv[1]);
                weechat_lua_load (plugin, (path_script) ? path_script : argv[1]);
                if (path_script)
                    free (path_script);
            }
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "unload") == 0)
            {
                /* unload Lua script */
                weechat_lua_unload_name (plugin, argv[1]);
            }
            else
            {
                plugin->print_server (plugin,
                                      "Lua error: unknown option for "
                                      "\"lua\" command");
            }
            break;
        default:
            plugin->print_server (plugin,
                                  "Lua error: wrong argument count for \"lua\" command");
    }
    
    if (argv)
        plugin->free_exploded_string (plugin, argv);
    
    return PLUGIN_RC_OK;
}

/*
 * weechat_plugin_init: initialize Lua plugin
 */

int
weechat_plugin_init (t_weechat_plugin *plugin)
{
    
    lua_plugin = plugin;
    
    plugin->print_server (plugin, "Loading Lua module \"weechat\"");
        
    plugin->cmd_handler_add (plugin, "lua",
                             "list/load/unload Lua scripts",
                             "[load filename] | [autoload] | [reload] | [unload]",
                             "filename: Lua script (file) to load\n\n"
                             "Without argument, /lua command lists all loaded Lua scripts.",
                             "load|autoload|reload|unload",
                             weechat_lua_cmd, NULL, NULL);

    plugin->mkdir_home (plugin, "lua");
    plugin->mkdir_home (plugin, "lua/autoload");
    
    weechat_script_auto_load (plugin, "lua", weechat_lua_load);
    
    /* init ok */
    return PLUGIN_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Lua interface
 */

void
weechat_plugin_end (t_weechat_plugin *plugin)
{
    /* unload all scripts */
    weechat_lua_unload_all (plugin);
    
    lua_plugin->print_server (lua_plugin,
                              "Lua plugin ended");
}
