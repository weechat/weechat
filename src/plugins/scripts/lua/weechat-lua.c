/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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


WEECHAT_PLUGIN_NAME(LUA_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("Lua plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_lua_plugin;

int lua_quiet = 0;
struct t_plugin_script *lua_scripts = NULL;
struct t_plugin_script *last_lua_script = NULL;
struct t_plugin_script *lua_current_script = NULL;
struct t_plugin_script *lua_registered_script = NULL;
const char *lua_current_script_filename = NULL;
lua_State *lua_current_interpreter = NULL;

/*
 * string used to execute action "install":
 * when signal "lua_install_script" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *lua_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "lua_remove_script" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *lua_action_remove_list = NULL;


/*
 * weechat_lua_exec: execute a Lua script
 */

void *
weechat_lua_exec (struct t_plugin_script *script,
                  int ret_type, const char *function, char **argv)
{
    void *ret_value;
    int argc, *ret_i;
    struct t_plugin_script *old_lua_current_script;
    
    lua_current_interpreter = script->interpreter;
    
    lua_getglobal (lua_current_interpreter, function);

    old_lua_current_script = lua_current_script;
    lua_current_script = script;
    
    argc = 0;
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
                        if (argv[5])
                        {
                            argc = 6;
                            lua_pushstring (lua_current_interpreter, argv[5]);
                            if (argv[6])
                            {
                                argc = 7;
                                lua_pushstring (lua_current_interpreter,
                                                argv[6]);
                                if (argv[7])
                                {
                                    argc = 8;
                                    lua_pushstring (lua_current_interpreter,
                                                    argv[7]);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (lua_pcall (lua_current_interpreter, argc, 1, 0) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to run function \"%s\""),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, function);
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME,
                        lua_tostring (lua_current_interpreter, -1));
        lua_current_script = old_lua_current_script;
        return NULL;
    }
    
    if (ret_type == WEECHAT_SCRIPT_EXEC_STRING)
        ret_value = strdup ((char *) lua_tostring (lua_current_interpreter, -1));
    else if (ret_type == WEECHAT_SCRIPT_EXEC_INT)
    {
        ret_i = malloc (sizeof (*ret_i));
        if (ret_i)
            *ret_i = lua_tonumber (lua_current_interpreter, -1);
        ret_value = ret_i;
    }
    else
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, function);
        lua_current_script = old_lua_current_script;
        return NULL;
    }
    
    lua_current_script = old_lua_current_script;
    
    return ret_value;
}

int
weechat_lua_load (const char *filename)
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
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, filename);
        return 0;
    }
    
    if ((weechat_lua_plugin->debug >= 1) || !lua_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        LUA_PLUGIN_NAME, filename);
    }
    
    lua_current_script = NULL;
    lua_registered_script = NULL;
    
    lua_current_interpreter = lua_open ();

    if (lua_current_interpreter == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME);
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
                        weechat_prefix ("error"), LUA_PLUGIN_NAME);
    }
    
    lua_current_script_filename = filename;
    
    if (luaL_loadfile (lua_current_interpreter, filename) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to load file \"%s\""),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, filename);
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME,
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
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, filename);
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME,
                        lua_tostring (lua_current_interpreter, -1));
        lua_close (lua_current_interpreter);
        fclose (fp);
        /* if script was registered, removing from list */
        if (lua_current_script)
            script_remove (weechat_lua_plugin, &lua_scripts, &last_lua_script,
                           lua_current_script);
        return 0;
    }
    fclose (fp);
    
    if (!lua_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, filename);
        lua_close (lua_current_interpreter);
        return 0;
    }
    lua_current_script = lua_registered_script;
    
    lua_current_script->interpreter = (lua_State *) lua_current_interpreter;
    
    return 1;
}

/*
 * weechat_lua_load_cb: callback for weechat_script_auto_load() function
 */

void
weechat_lua_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;
    
    weechat_lua_load (filename);
}

/*
 * weechat_lua_unload: unload a Lua script
 */

void
weechat_lua_unload (struct t_plugin_script *script)
{
    int *r;
    char *lua_argv[1] = { NULL };
    void *interpreter;
    
    if ((weechat_lua_plugin->debug >= 1) || !lua_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        LUA_PLUGIN_NAME, script->name);
    }
    
    if (script->shutdown_func && script->shutdown_func[0])
    {
        r = weechat_lua_exec (script,
                              WEECHAT_SCRIPT_EXEC_INT,
                              script->shutdown_func,
                              lua_argv);
        if (r)
            free (r);
    }
    
    interpreter = script->interpreter;
    
    if (lua_current_script == script)
        lua_current_script = (lua_current_script->prev_script) ?
            lua_current_script->prev_script : lua_current_script->next_script;
    
    script_remove (weechat_lua_plugin, &lua_scripts, &last_lua_script, script);
    
    lua_close (script->interpreter);
}

/*
 * weechat_lua_unload_name: unload a Lua script by name
 */

void
weechat_lua_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    
    ptr_script = script_search (weechat_lua_plugin, lua_scripts, name);
    if (ptr_script)
    {
        weechat_lua_unload (ptr_script);
        weechat_printf (NULL,
                        weechat_gettext ("%s: script \"%s\" unloaded"),
                        LUA_PLUGIN_NAME, name);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, name);
    }
}

/*
 * weechat_lua_reload_name: reload a Lua script by name
 */

void
weechat_lua_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;
    
    ptr_script = script_search (weechat_lua_plugin, lua_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_lua_unload (ptr_script);
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            LUA_PLUGIN_NAME, name);
            weechat_lua_load (filename);
            free (filename);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, name);
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
            path_script = script_search_path (weechat_lua_plugin,
                                              argv_eol[2]);
            weechat_lua_load ((path_script) ? path_script : argv_eol[2]);
            if (path_script)
                free (path_script);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            /* reload one Lua script */
            weechat_lua_reload_name (argv_eol[2]);
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
                            weechat_prefix ("error"), LUA_PLUGIN_NAME, "lua");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_lua_completion_cb: callback for script completion
 */

int
weechat_lua_completion_cb (void *data, const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    script_completion (weechat_lua_plugin, completion, lua_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_lua_infolist_cb: callback for infolist
 */

struct t_infolist *
weechat_lua_infolist_cb (void *data, const char *infolist_name,
                         void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;
    
    if (!infolist_name || !infolist_name[0])
        return NULL;
    
    if (weechat_strcasecmp (infolist_name, "lua_script") == 0)
    {
        return script_infolist_list_scripts (weechat_lua_plugin,
                                             lua_scripts, pointer,
                                             arguments);
    }
    
    return NULL;
}

/*
 * weechat_lua_signal_debug_dump_cb: dump Lua plugin data in WeeChat log file
 */

int
weechat_lua_signal_debug_dump_cb (void *data, const char *signal,
                                  const char *type_data, void *signal_data)
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
 * weechat_lua_signal_buffer_closed_cb: callback called when a buffer is closed
 */

int
weechat_lua_signal_buffer_closed_cb (void *data, const char *signal,
                                     const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    if (signal_data)
        script_remove_buffer_callbacks (lua_scripts, signal_data);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_lua_timer_action_cb: timer for executing actions
 */

int
weechat_lua_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &lua_action_install_list)
        {
            script_action_install (weechat_lua_plugin,
                                   lua_scripts,
                                   &weechat_lua_unload,
                                   &weechat_lua_load,
                                   &lua_action_install_list);
        }
        else if (data == &lua_action_remove_list)
        {
            script_action_remove (weechat_lua_plugin,
                                  lua_scripts,
                                  &weechat_lua_unload,
                                  &lua_action_remove_list);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_lua_signal_script_action_cb: callback called when a script action
 *                                      is asked (install/remove a script)
 */

int
weechat_lua_signal_script_action_cb (void *data, const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "lua_script_install") == 0)
        {
            script_action_add (&lua_action_install_list,
                               (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_lua_timer_action_cb,
                                &lua_action_install_list);
        }
        else if (strcmp (signal, "lua_script_remove") == 0)
        {
            script_action_add (&lua_action_remove_list,
                               (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_lua_timer_action_cb,
                                &lua_action_remove_list);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize Lua plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    weechat_lua_plugin = plugin;
    
    lua_quiet = 1;
    script_init (weechat_lua_plugin,
                 argc,
                 argv,
                 &lua_scripts,
                 &weechat_lua_command_cb,
                 &weechat_lua_completion_cb,
                 &weechat_lua_infolist_cb,
                 &weechat_lua_signal_debug_dump_cb,
                 &weechat_lua_signal_buffer_closed_cb,
                 &weechat_lua_signal_script_action_cb,
                 &weechat_lua_load_cb,
                 &weechat_lua_api_buffer_input_data_cb,
                 &weechat_lua_api_buffer_close_cb);
    lua_quiet = 0;
    
    script_display_short_list (weechat_lua_plugin,
                               lua_scripts);
    
    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Lua interface
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* unload all scripts */
    lua_quiet = 1;
    script_end (plugin, &lua_scripts, &weechat_lua_unload_all);
    lua_quiet = 0;
    
    return WEECHAT_RC_OK;
}
