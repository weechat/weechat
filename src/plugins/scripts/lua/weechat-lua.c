/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* weechat-lua.c: Lua plugin for WeeChat */

#undef _

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "weechat-lua.h"
#include "weechat-lua-api.h"


WEECHAT_PLUGIN_NAME("lua");
WEECHAT_PLUGIN_DESCRIPTION("Lua plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL");

struct t_weechat_plugin *weechat_lua_plugin;

struct t_plugin_script *lua_scripts = NULL;
struct t_plugin_script *lua_current_script = NULL;
char *lua_current_script_filename = NULL;
lua_State *lua_current_interpreter = NULL;


/*
 * weechat_lua_exec: execute a Lua script
 */

void *
weechat_lua_exec (struct t_plugin_script *script,
		  int ret_type, char *function, char **argv)
{
    void *ret_value;
    int argc, *ret_i;
    
    lua_current_interpreter = script->interpreter;
    
    lua_getglobal (lua_current_interpreter, function);
    lua_current_script = script;
    
    if (argv && argv[0])
    {
        lua_pushstring (lua_current_interpreter, argv[0]);
        argc = 1;
        if (argv[1])
        {
            argc = 2;
            lua_pushstring (lua_current_interpreter, argv[1]);
            if (argv[2])
            {
                argc = 3;
                lua_pushstring (lua_current_interpreter, argv[2]);
                if (argv[3])
                {
                    argc = 4;
                    lua_pushstring (lua_current_interpreter, argv[3]);
                    if (argv[4])
                    {
                        argc = 5;
                        lua_pushstring (lua_current_interpreter, argv[4]);
                    }
                }
            }
        }
    }
    
    if (lua_pcall (lua_current_interpreter, argc, 1, 0) != 0)
    {
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to run function \"%s\""),
                        weechat_prefix ("error"), "lua", function);
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), "lua",
                        lua_tostring (lua_current_interpreter, -1));
        return NULL;
    }
    
    if (ret_type == WEECHAT_SCRIPT_EXEC_STRING)
	ret_value = strdup ((char *) lua_tostring (lua_current_interpreter, -1));
    else if (ret_type == WEECHAT_SCRIPT_EXEC_INT)
    {
	ret_i = (int *)malloc (sizeof (int));
	if (ret_i)
	    *ret_i = lua_tonumber (lua_current_interpreter, -1);
	ret_value = ret_i;
    }
    else
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS(function);
	return NULL;
    }
    
    return ret_value; 
}

int
weechat_lua_load (char *filename)
{
    FILE *fp;
    char *weechat_lua_code = {
	"weechat_outputs = {\n"
	"    write = function (self, str)\n"
	"        weechat.print(\"\", \"lua: stdout/stderr: \" .. str)\n"
        "    end\n"
	"}\n"
	"io.stdout = weechat_outputs\n"
	"io.stderr = weechat_outputs\n"
    };
    
    if ((fp = fopen (filename, "r")) == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), "lua", filename);
        return 0;
    }
    
    weechat_printf (NULL,
                    weechat_gettext ("%s%s: loading script \"%s\""),
                    weechat_prefix ("info"), "lua", filename);
    
    lua_current_script = NULL;
    
    lua_current_interpreter = lua_open ();

    if (lua_current_interpreter == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        weechat_prefix ("error"), "lua");
        fclose (fp);
        return 0;
    }

#ifdef LUA_VERSION_NUM /* LUA_VERSION_NUM is defined only in lua >= 5.1.0 */
    luaL_openlibs (lua_current_interpreter);
#else
    luaopen_base (lua_current_interpreter);
    luaopen_string (lua_current_interpreter);
    luaopen_table (lua_current_interpreter);
    luaopen_math (lua_current_interpreter);
    luaopen_io (lua_current_interpreter);
    luaopen_debug (lua_current_interpreter);
#endif
    
    luaL_openlib (lua_current_interpreter, "weechat", weechat_lua_api_funcs, 0);

#ifdef LUA_VERSION_NUM
    if (luaL_dostring (lua_current_interpreter, weechat_lua_code) != 0)
#else    
    if (lua_dostring (lua_current_interpreter, weechat_lua_code) != 0)
#endif
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to redirect stdout "
                                         "and stderr"),
                        weechat_prefix ("error"), "lua");
    }
    
    lua_current_script_filename = filename;
    
    if (luaL_loadfile (lua_current_interpreter, filename) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to load file \"%s\""),
                        weechat_prefix ("error"), "lua", filename);
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), "lua",
                        lua_tostring (lua_current_interpreter, -1));
        lua_close (lua_current_interpreter);
        fclose (fp);
        return 0;
    }
    
    if (lua_pcall (lua_current_interpreter, 0, 0, 0) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to execute file "
                                         "\"%s\""),
                        weechat_prefix ("error"), "lua", filename);
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), "lua",
                        lua_tostring (lua_current_interpreter, -1));
        lua_close (lua_current_interpreter);
        fclose (fp);
	/* if script was registered, removing from list */
	if (lua_current_script)
	    script_remove (weechat_lua_plugin, &lua_scripts,
                           lua_current_script);
        return 0;
    }
    fclose (fp);
    
    if (lua_current_script == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), "lua", filename);
	lua_close (lua_current_interpreter);
        return 0;
    }
    
    lua_current_script->interpreter = (lua_State *) lua_current_interpreter;
    
    return 1;
}

/*
 * weechat_lua_load_cb: callback for weechat_script_auto_load() function
 */

int
weechat_lua_load_cb (void *data, char *filename)
{
    /* make C compiler happy */
    (void) data;
    
    return weechat_lua_load (filename);
}

/*
 * weechat_lua_unload: unload a Lua script
 */

void
weechat_lua_unload (struct t_plugin_script *script)
{
    int *r;
    char *lua_argv[1] = { NULL };
    
    weechat_printf (NULL,
                    weechat_gettext ("%s%s: unloading script \"%s\""),
                    weechat_prefix ("info"), "lua", script->name);
    
    if (script->shutdown_func && script->shutdown_func[0])
    {
        r = weechat_lua_exec (script,
                              WEECHAT_SCRIPT_EXEC_INT,
			      script->shutdown_func,
                              lua_argv);
	if (r)
	    free (r);
    }
    
    lua_close (script->interpreter);
    
    script_remove (weechat_lua_plugin, &lua_scripts, script);
}

/*
 * weechat_lua_unload_name: unload a Lua script by name
 */

void
weechat_lua_unload_name (char *name)
{
    struct t_plugin_script *ptr_script;
    
    ptr_script = script_search (weechat_lua_plugin, lua_scripts, name);
    if (ptr_script)
    {
        weechat_lua_unload (ptr_script);
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" unloaded"),
                        weechat_prefix ("info"), "lua", name);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), "lua", name);
    }
}

/*
 * weechat_lua_unload_all: unload all Lua scripts
 */

void
weechat_lua_unload_all ()
{
    while (lua_scripts)
    {
        weechat_lua_unload (lua_scripts);
    }
}

/*
 * weechat_lua_command_cb: callback for "/lua" command
 */

int
weechat_lua_command_cb (void *data, struct t_gui_buffer *buffer,
                        int argc, char **argv, char **argv_eol)
{
    char *path_script;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    if (argc == 1)
    {
        script_display_list (weechat_lua_plugin, lua_scripts,
                             NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            script_display_list (weechat_lua_plugin, lua_scripts,
                                 NULL, 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            script_display_list (weechat_lua_plugin, lua_scripts,
                                 NULL, 1);
        }
        else if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            script_auto_load (weechat_lua_plugin, &weechat_lua_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_lua_unload_all ();
            script_auto_load (weechat_lua_plugin, &weechat_lua_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            weechat_lua_unload_all ();
        }
    }
    else
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            script_display_list (weechat_lua_plugin, lua_scripts,
                                 argv_eol[2], 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            script_display_list (weechat_lua_plugin, lua_scripts,
                                 argv_eol[2], 1);
        }
        else if (weechat_strcasecmp (argv[1], "load") == 0)
        {
            /* load Lua script */
            path_script = script_search_full_name (weechat_lua_plugin,
                                                   argv_eol[2]);
            weechat_lua_load ((path_script) ? path_script : argv_eol[2]);
            if (path_script)
                free (path_script);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            /* unload Lua script */
            weechat_lua_unload_name (argv_eol[2]);
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unknown option for "
                                             "command \"%s\""),
                            weechat_prefix ("error"), "lua", "lua");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_lua_completion_cb: callback for script completion
 */

int
weechat_lua_completion_cb (void *data, char *completion,
                           struct t_gui_buffer *buffer,
                           struct t_weelist *list)
{
    /* make C compiler happy */
    (void) data;
    (void) completion;
    (void) buffer;
    
    script_completion (weechat_lua_plugin, list, lua_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_lua_dump_data_cb: dump Lua plugin data in WeeChat log file
 */

int
weechat_lua_dump_data_cb (void *data, char *signal, char *type_data,
                          void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    script_print_log (weechat_lua_plugin, lua_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize Lua plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    
    weechat_lua_plugin = plugin;

    script_init (weechat_lua_plugin,
                 &weechat_lua_command_cb,
                 &weechat_lua_completion_cb,
                 &weechat_lua_dump_data_cb,
                 &weechat_lua_load_cb);
    
    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Lua interface
 */

void
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    /* unload all scripts */
    weechat_lua_unload_all ();
}
