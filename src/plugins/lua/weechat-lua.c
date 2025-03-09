/*
 * weechat-lua.c - lua plugin for WeeChat
 *
 * Copyright (C) 2006-2007 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#undef _

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "weechat-lua.h"
#include "weechat-lua-api.h"


WEECHAT_PLUGIN_NAME(LUA_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of lua scripts"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(LUA_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_lua_plugin = NULL;

struct t_plugin_script_data lua_data;

struct t_config_file *lua_config_file = NULL;
struct t_config_option *lua_config_look_check_license = NULL;
struct t_config_option *lua_config_look_eval_keep_context = NULL;

int lua_quiet = 0;

struct t_plugin_script *lua_script_eval = NULL;
int lua_eval_mode = 0;
int lua_eval_send_input = 0;
int lua_eval_exec_commands = 0;
struct t_gui_buffer *lua_eval_buffer = NULL;
#if LUA_VERSION_NUM >= 502
#define LUA_LOAD "load"
#else
#define LUA_LOAD "loadstring"
#endif /* LUA_VERSION_NUM >= 502 */
#define LUA_EVAL_SCRIPT                                                 \
    "function script_lua_eval(code)\n"                                  \
    "    assert(" LUA_LOAD "(code))()\n"                                \
    "end\n"                                                             \
    "\n"                                                                \
    "weechat.register('" WEECHAT_SCRIPT_EVAL_NAME "', '', '1.0', "      \
    "'" WEECHAT_LICENSE "', 'Evaluation of source code', '', '')\n"

struct t_plugin_script *lua_scripts = NULL;
struct t_plugin_script *last_lua_script = NULL;
struct t_plugin_script *lua_current_script = NULL;
struct t_plugin_script *lua_registered_script = NULL;
const char *lua_current_script_filename = NULL;
lua_State *lua_current_interpreter = NULL;
char **lua_buffer_output = NULL;

/*
 * string used to execute action "install":
 * when signal "lua_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *lua_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "lua_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *lua_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "lua_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *lua_action_autoload_list = NULL;


/*
 * Callback called for each key/value in a hashtable.
 */

void
weechat_lua_hashtable_map_cb (void *data,
                              struct t_hashtable *hashtable,
                              const char *key,
                              const char *value)
{
    lua_State *interpreter;

    /* make C compiler happy */
    (void) hashtable;

    interpreter = (lua_State *)data;

    lua_pushstring (interpreter, key);
    lua_pushstring (interpreter, value);
    lua_rawset (interpreter, -3);
}

/*
 * Converts a WeeChat hashtable to a lua hash (as lua table on the stack).
 */

void
weechat_lua_pushhashtable (lua_State *interpreter, struct t_hashtable *hashtable)
{
    lua_newtable (interpreter);

    weechat_hashtable_map_string (hashtable,
                                  &weechat_lua_hashtable_map_cb,
                                  interpreter);
}

/*
 * Converts a lua hash (on stack) to a WeeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
weechat_lua_tohashtable (lua_State *interpreter, int index, int size,
                         const char *type_keys, const char *type_values)
{
    struct t_hashtable *hashtable;

    hashtable = weechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    lua_pushnil (interpreter);
    while (lua_next (interpreter, index - 1) != 0)
    {
        if (strcmp (type_values, WEECHAT_HASHTABLE_STRING) == 0)
        {
            weechat_hashtable_set (hashtable,
                                   lua_tostring (interpreter, -2),
                                   lua_tostring (interpreter, -1));
        }
        else if (strcmp (type_values, WEECHAT_HASHTABLE_POINTER) == 0)
        {
            weechat_hashtable_set (hashtable,
                                   lua_tostring (interpreter, -2),
                                   plugin_script_str2ptr (
                                       weechat_lua_plugin,
                                       NULL, NULL,
                                       lua_tostring (interpreter, -1)));
        }
        /* remove value from stack (keep key for next iteration) */
        lua_pop (interpreter, 1);
    }

    return hashtable;
}

/*
 * Flushes output.
 */

void
weechat_lua_output_flush (void)
{
    const char *ptr_command;
    char *temp_buffer, *command;

    if (!(*lua_buffer_output)[0])
        return;

    /* if there's no buffer, we catch the output, so there's no flush */
    if (lua_eval_mode && !lua_eval_buffer)
        return;

    temp_buffer = strdup (*lua_buffer_output);
    if (!temp_buffer)
        return;

    weechat_string_dyn_copy (lua_buffer_output, NULL);

    if (lua_eval_mode)
    {
        if (lua_eval_send_input)
        {
            if (lua_eval_exec_commands)
                ptr_command = temp_buffer;
            else
                ptr_command = weechat_string_input_for_buffer (temp_buffer);
            if (ptr_command)
            {
                weechat_command (lua_eval_buffer, temp_buffer);
            }
            else
            {
                if (weechat_asprintf (&command,
                                      "%c%s",
                                      temp_buffer[0],
                                      temp_buffer) >= 0)
                {
                    weechat_command (lua_eval_buffer,
                                     (command[0]) ? command : " ");
                    free (command);
                }
            }
        }
        else
        {
            weechat_printf (lua_eval_buffer, "%s", temp_buffer);
        }
    }
    else
    {
        /* script (no eval mode) */
        weechat_printf (
            NULL,
            weechat_gettext ("%s: stdout/stderr (%s): %s"),
            LUA_PLUGIN_NAME,
            (lua_current_script) ? lua_current_script->name : "?",
            temp_buffer);
    }

    free (temp_buffer);
}

/*
 * Redirection for stdout and stderr.
 */

int
weechat_lua_output (lua_State *L)
{
    int i, argument_count;
    const char *stringified_result;
    char *ptr_msg, *ptr_newline;

    argument_count = lua_gettop (L);
    for (i = 1; i <= argument_count; ++i)
    {
        /* call tostring with the given value */
        lua_getglobal (L, "tostring");
        lua_pushvalue (L, i);
        lua_call (L, 1, 1);
        /* get the stringified value */
        stringified_result = lua_tostring (L, -1);
        /* handle stringification failure */
        if (!stringified_result)
        {
            return luaL_error (L, "%s must return a string to %s",
                               "tostring", "print");
        }

        /* discard tostring's value */
        lua_remove (L, -1);

        ptr_msg = (char *)stringified_result;

        while ((ptr_newline = strchr(ptr_msg, '\n')) != NULL)
        {
            weechat_string_dyn_concat (lua_buffer_output,
                                       ptr_msg,
                                       ptr_newline - ptr_msg);
            weechat_lua_output_flush ();
            ptr_msg = ++ptr_newline;
        }
        weechat_string_dyn_concat (lua_buffer_output, ptr_msg, -1);
  }
  return 0;
}

/*
 * Executes a lua function.
 */

void *
weechat_lua_exec (struct t_plugin_script *script, int ret_type,
                  const char *function, const char *format, void **argv)
{
    void *ret_value;
    int argc, i, *ret_i, rc;
    lua_State *old_lua_current_interpreter;
    struct t_plugin_script *old_lua_current_script;

    old_lua_current_interpreter = lua_current_interpreter;
    if (script->interpreter)
        lua_current_interpreter = script->interpreter;

    lua_getglobal (lua_current_interpreter, function);

    old_lua_current_script = lua_current_script;
    lua_current_script = script;

    argc = 0;
    if (format && format[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string or null */
                    if (argv[i])
                        lua_pushstring (lua_current_interpreter, (char *)argv[i]);
                    else
                        lua_pushnil (lua_current_interpreter);
                    break;
                case 'i': /* integer */
#if LUA_VERSION_NUM >= 503
                    lua_pushinteger (lua_current_interpreter, *((int *)argv[i]));
#else
                    lua_pushnumber (lua_current_interpreter, *((int *)argv[i]));
#endif /* LUA_VERSION_NUM >= 503 */
                    break;
                case 'h': /* hash */
                    weechat_lua_pushhashtable (lua_current_interpreter,
                                               (struct t_hashtable *)argv[i]);
                    break;
            }
        }
    }

    ret_value = NULL;

    rc = lua_pcall (lua_current_interpreter, argc, 1, 0);

    weechat_lua_output_flush ();

    if (rc == 0)
    {
        if (ret_type == WEECHAT_SCRIPT_EXEC_STRING)
        {
            ret_value = (char *) lua_tostring (lua_current_interpreter, -1);
            if (ret_value)
            {
                ret_value = strdup (ret_value);
            }
            else
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: function \"%s\" must "
                                                 "return a valid value"),
                                weechat_prefix ("error"), LUA_PLUGIN_NAME,
                                function);
            }
        }
        else if (ret_type == WEECHAT_SCRIPT_EXEC_POINTER)
        {
            ret_value = (char *) lua_tostring (lua_current_interpreter, -1);
            if (ret_value)
            {
                ret_value = plugin_script_str2ptr (weechat_lua_plugin,
                                                   script->name, function,
                                                   ret_value);
            }
            else
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: function \"%s\" must "
                                                 "return a valid value"),
                                weechat_prefix ("error"), LUA_PLUGIN_NAME,
                                function);
            }
        }
        else if (ret_type == WEECHAT_SCRIPT_EXEC_INT)
        {
            ret_i = malloc (sizeof (*ret_i));
            if (ret_i)
                *ret_i = lua_tonumber (lua_current_interpreter, -1);
            ret_value = ret_i;
        }
        else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
        {
            ret_value = weechat_lua_tohashtable (lua_current_interpreter, -1,
                                                 WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                 WEECHAT_HASHTABLE_STRING,
                                                 WEECHAT_HASHTABLE_STRING);
        }
        else
        {
            if (ret_type != WEECHAT_SCRIPT_EXEC_IGNORE)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: function \"%s\" must "
                                                 "return a valid value"),
                                weechat_prefix ("error"), LUA_PLUGIN_NAME,
                                function);
            }
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to run function \"%s\""),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, function);
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME,
                        lua_tostring (lua_current_interpreter, -1));
    }

    if ((ret_type != WEECHAT_SCRIPT_EXEC_IGNORE) && !ret_value)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error in function \"%s\""),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, function);
    }

    lua_pop (lua_current_interpreter, 1);

    lua_current_script = old_lua_current_script;
    lua_current_interpreter = old_lua_current_interpreter;

    return ret_value;
}

/*
 * Called when a constant is modified.
 */

int
weechat_lua_newindex (lua_State *L)
{
    luaL_error (L, "Error: read-only constant");

    return 0;
}

/*
 * Registers a library to use inside lua script.
 */

void
weechat_lua_register_lib (lua_State *L, const char *libname,
                          const luaL_Reg *lua_api_funcs)
{
    int i;

#if LUA_VERSION_NUM >= 502
    if (libname)
    {
        lua_newtable (L);
        luaL_setfuncs (L, lua_api_funcs, 0);
        lua_pushvalue (L, -1);
        lua_setglobal (L, libname);
    }
    else
        luaL_setfuncs (L, lua_api_funcs, 0);
#else
    luaL_register (L, libname, lua_api_funcs);
#endif /* LUA_VERSION_NUM >= 502 */

    luaL_newmetatable (L, "weechat");
    lua_pushliteral (L, "__index");
    lua_newtable (L);

    /* define constants */
    for (i = 0; weechat_script_constants[i].name; i++)
    {
        lua_pushstring (L, weechat_script_constants[i].name);
        if (weechat_script_constants[i].value_string)
            lua_pushstring (L, weechat_script_constants[i].value_string);
        else
#if LUA_VERSION_NUM >= 503
            lua_pushinteger (L, weechat_script_constants[i].value_integer);
#else
            lua_pushnumber (L, weechat_script_constants[i].value_integer);
#endif /* LUA_VERSION_NUM >= 503 */
        lua_settable (L, -3);
    }

    lua_settable (L, -3);

    lua_pushliteral (L, "__newindex");
    lua_pushcfunction (L, weechat_lua_newindex);
    lua_settable (L, -3);

    lua_setmetatable (L, -2);
    lua_pop (L, 1);
}

/*
 * Loads a lua script.
 *
 * If code is NULL, the content of filename is read and executed.
 * If code is not NULL, it is executed (the file is not read).
 *
 * Returns pointer to new registered script, NULL if error.
 */

struct t_plugin_script *
weechat_lua_load (const char *filename, const char *code)
{
    FILE *fp;

    fp = NULL;

    if (!code)
    {
        fp = fopen (filename, "r");
        if (!fp)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: script \"%s\" not found"),
                            weechat_prefix ("error"), LUA_PLUGIN_NAME,
                            filename);
            return NULL;
        }
    }

    if ((weechat_lua_plugin->debug >= 2) || !lua_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        LUA_PLUGIN_NAME, filename);
    }

    lua_current_script = NULL;
    lua_registered_script = NULL;

    lua_current_interpreter = luaL_newstate ();

    if (lua_current_interpreter == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME);
        if (fp)
            fclose (fp);
        return NULL;
    }

#ifndef LUA_VERSION_NUM /* Lua ≤ 5.0 */
    luaopen_base (lua_current_interpreter);
    luaopen_string (lua_current_interpreter);
    luaopen_table (lua_current_interpreter);
    luaopen_math (lua_current_interpreter);
    luaopen_io (lua_current_interpreter);
    luaopen_debug (lua_current_interpreter);
#else
    luaL_openlibs (lua_current_interpreter);
#endif /* LUA_VERSION_NUM */

    weechat_lua_register_lib (lua_current_interpreter, "weechat",
                              weechat_lua_api_funcs);

    /* Remove references to stdout and stderr */
    lua_getglobal (lua_current_interpreter, "io");
    if (lua_istable (lua_current_interpreter, -1))
    {
        /*
         * io.stdout = nil
         * io.stderr = nil
        */
        lua_pushnil (lua_current_interpreter);
        lua_setfield (lua_current_interpreter, -2, "stdout");
        lua_pushnil (lua_current_interpreter);
        lua_setfield (lua_current_interpreter, -2, "stderr");

        /* io.write = weechat_lua_output [C] */
        lua_pushcfunction (lua_current_interpreter, weechat_lua_output);
        lua_setfield (lua_current_interpreter, -2, "write");
    }
    lua_pop (lua_current_interpreter, 1); /* remove the `ìo` table|(nil value) from the stack */

    /* print = weechat_lua_output [C] */
    lua_pushcfunction (lua_current_interpreter, weechat_lua_output);
    lua_setglobal (lua_current_interpreter, "print");

    /* debug.debug = nil */
    lua_getglobal (lua_current_interpreter, "debug");
    if (lua_istable (lua_current_interpreter, -1))
    {
        lua_pushnil (lua_current_interpreter);
        lua_setfield (lua_current_interpreter, -2, "debug");
    }
    lua_pop (lua_current_interpreter, 1); /* remove the `debug` table|(nil value) from the stack */

    lua_current_script_filename = filename;

    if (code)
    {
        /* execute code without reading file */
        if (luaL_loadstring (lua_current_interpreter, code) != 0)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to load source "
                                             "code"),
                            weechat_prefix ("error"), LUA_PLUGIN_NAME);
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: error: %s"),
                            weechat_prefix ("error"), LUA_PLUGIN_NAME,
                            lua_tostring (lua_current_interpreter, -1));
            lua_close (lua_current_interpreter);
            return NULL;
        }
    }
    else
    {
        /* read and execute code from file */
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
            return NULL;
        }
    }

    if (lua_pcall (lua_current_interpreter, 0, 0, 0) != 0)
    {
        if (code)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to execute source "
                                             "code"),
                            weechat_prefix ("error"), LUA_PLUGIN_NAME);
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to execute file "
                                             "\"%s\""),
                            weechat_prefix ("error"), LUA_PLUGIN_NAME,
                            filename);
        }
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error: %s"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME,
                        lua_tostring (lua_current_interpreter, -1));
        lua_close (lua_current_interpreter);
        if (fp)
            fclose (fp);

        /* if script was registered, remove it from list */
        if (lua_current_script)
        {
            plugin_script_remove (weechat_lua_plugin,
                                  &lua_scripts, &last_lua_script,
                                  lua_current_script);
            lua_current_script = NULL;
        }

        return NULL;
    }

    if (fp)
        fclose (fp);

    if (!lua_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, filename);
        lua_close (lua_current_interpreter);
        return NULL;
    }
    lua_current_script = lua_registered_script;

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (weechat_lua_plugin,
                                        lua_scripts,
                                        lua_current_script,
                                        &weechat_lua_api_buffer_input_data_cb,
                                        &weechat_lua_api_buffer_close_cb);

    (void) weechat_hook_signal_send ("lua_script_loaded",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     lua_current_script->filename);

    return lua_current_script;
}

/*
 * Callback for weechat_script_auto_load() function.
 */

void
weechat_lua_load_cb (void *data, const char *filename)
{
    const char *pos_dot;

    /* make C compiler happy */
    (void) data;

    pos_dot = strrchr (filename, '.');
    if (pos_dot && (strcmp (pos_dot, ".lua") == 0))
        weechat_lua_load (filename, NULL);
}

/*
 * Unloads a lua script.
 */

void
weechat_lua_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;
    char *filename;

    if ((weechat_lua_plugin->debug >= 2) || !lua_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        LUA_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)weechat_lua_exec (script,
                                      WEECHAT_SCRIPT_EXEC_INT,
                                      script->shutdown_func,
                                      NULL, NULL);
        free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (lua_current_script == script)
        lua_current_script = (lua_current_script->prev_script) ?
            lua_current_script->prev_script : lua_current_script->next_script;

    plugin_script_remove (weechat_lua_plugin, &lua_scripts, &last_lua_script, script);

    if (interpreter)
        lua_close (interpreter);

    if (lua_current_script)
        lua_current_interpreter = lua_current_script->interpreter;

    (void) weechat_hook_signal_send ("lua_script_unloaded",
                                     WEECHAT_HOOK_SIGNAL_STRING, filename);
    free (filename);
}

/*
 * Unloads a lua script by name.
 */

void
weechat_lua_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (lua_scripts, name);
    if (ptr_script)
    {
        weechat_lua_unload (ptr_script);
        if (!lua_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            LUA_PLUGIN_NAME, name);
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
 * Reloads a lua script by name.
 */

void
weechat_lua_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (lua_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_lua_unload (ptr_script);
            if (!lua_quiet)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s: script \"%s\" unloaded"),
                                LUA_PLUGIN_NAME, name);
            }
            weechat_lua_load (filename, NULL);
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
 * Unloads all lua scripts.
 */

void
weechat_lua_unload_all (void)
{
    while (lua_scripts)
    {
        weechat_lua_unload (lua_scripts);
    }
}

/*
 * Evaluates lua source code.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weechat_lua_eval (struct t_gui_buffer *buffer, int send_to_buffer_as_input,
                  int exec_commands, const char *code)
{
    void *func_argv[1], *result;
    int old_lua_quiet;

    if (!lua_script_eval)
    {
        old_lua_quiet = lua_quiet;
        lua_quiet = 1;
        lua_script_eval = weechat_lua_load (WEECHAT_SCRIPT_EVAL_NAME,
                                            LUA_EVAL_SCRIPT);
        lua_quiet = old_lua_quiet;
        if (!lua_script_eval)
            return 0;
    }

    weechat_lua_output_flush ();

    lua_eval_mode = 1;
    lua_eval_send_input = send_to_buffer_as_input;
    lua_eval_exec_commands = exec_commands;
    lua_eval_buffer = buffer;

    func_argv[0] = (char *)code;
    result = weechat_lua_exec (lua_script_eval,
                               WEECHAT_SCRIPT_EXEC_IGNORE,
                               "script_lua_eval",
                               "s", func_argv);
    /* result is ignored */
    free (result);

    weechat_lua_output_flush ();

    lua_eval_mode = 0;
    lua_eval_send_input = 0;
    lua_eval_exec_commands = 0;
    lua_eval_buffer = NULL;

    if (!weechat_config_boolean (lua_config_look_eval_keep_context))
    {
        old_lua_quiet = lua_quiet;
        lua_quiet = 1;
        weechat_lua_unload (lua_script_eval);
        lua_quiet = old_lua_quiet;
        lua_script_eval = NULL;
    }

    return 1;
}

/*
 * Callback for command "/lua".
 */

int
weechat_lua_command_cb (const void *pointer, void *data,
                        struct t_gui_buffer *buffer,
                        int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *ptr_code, *path_script;
    int i, send_to_buffer_as_input, exec_commands, old_lua_quiet;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_lua_plugin, lua_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_lua_plugin, lua_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_lua_plugin, lua_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_lua_plugin, &weechat_lua_load_cb);
        }
        else if (weechat_strcmp (argv[1], "reload") == 0)
        {
            weechat_lua_unload_all ();
            plugin_script_auto_load (weechat_lua_plugin, &weechat_lua_load_cb);
        }
        else if (weechat_strcmp (argv[1], "unload") == 0)
        {
            weechat_lua_unload_all ();
        }
        else if (weechat_strcmp (argv[1], "version") == 0)
        {
            plugin_script_display_interpreter (weechat_lua_plugin, 0);
        }
        else
            WEECHAT_COMMAND_ERROR;
    }
    else
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_lua_plugin, lua_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_lua_plugin, lua_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcmp (argv[1], "load") == 0)
                 || (weechat_strcmp (argv[1], "reload") == 0)
                 || (weechat_strcmp (argv[1], "unload") == 0))
        {
            old_lua_quiet = lua_quiet;
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                lua_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (weechat_strcmp (argv[1], "load") == 0)
            {
                /* load lua script */
                path_script = plugin_script_search_path (weechat_lua_plugin,
                                                         ptr_name, 1);
                weechat_lua_load ((path_script) ? path_script : ptr_name,
                                  NULL);
                free (path_script);
            }
            else if (weechat_strcmp (argv[1], "reload") == 0)
            {
                /* reload one lua script */
                weechat_lua_reload_name (ptr_name);
            }
            else if (weechat_strcmp (argv[1], "unload") == 0)
            {
                /* unload lua script */
                weechat_lua_unload_name (ptr_name);
            }
            lua_quiet = old_lua_quiet;
        }
        else if (weechat_strcmp (argv[1], "eval") == 0)
        {
            send_to_buffer_as_input = 0;
            exec_commands = 0;
            ptr_code = argv_eol[2];
            for (i = 2; i < argc; i++)
            {
                if (argv[i][0] == '-')
                {
                    if (strcmp (argv[i], "-o") == 0)
                    {
                        if (i + 1 >= argc)
                            WEECHAT_COMMAND_ERROR;
                        send_to_buffer_as_input = 1;
                        exec_commands = 0;
                        ptr_code = argv_eol[i + 1];
                    }
                    else if (strcmp (argv[i], "-oc") == 0)
                    {
                        if (i + 1 >= argc)
                            WEECHAT_COMMAND_ERROR;
                        send_to_buffer_as_input = 1;
                        exec_commands = 1;
                        ptr_code = argv_eol[i + 1];
                    }
                }
                else
                    break;
            }
            if (!weechat_lua_eval (buffer, send_to_buffer_as_input,
                                   exec_commands, ptr_code))
                WEECHAT_COMMAND_ERROR;
        }
        else
            WEECHAT_COMMAND_ERROR;
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds lua scripts to completion list.
 */

int
weechat_lua_completion_cb (const void *pointer, void *data,
                           const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_lua_plugin, completion, lua_scripts);

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for lua scripts.
 */

struct t_hdata *
weechat_lua_hdata_cb (const void *pointer, void *data,
                      const char *hdata_name)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &lua_scripts, &last_lua_script,
                                       hdata_name);
}

/*
 * Returns lua info "lua_eval".
 */

char *
weechat_lua_info_eval_cb (const void *pointer, void *data,
                          const char *info_name,
                          const char *arguments)
{
    char *output;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    weechat_lua_eval (NULL, 0, 0, (arguments) ? arguments : "");
    output = strdup (*lua_buffer_output);
    weechat_string_dyn_copy (lua_buffer_output, NULL);

    return output;
}

/*
 * Returns infolist with lua scripts.
 */

struct t_infolist *
weechat_lua_infolist_cb (const void *pointer, void *data,
                         const char *infolist_name,
                         void *obj_pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (strcmp (infolist_name, "lua_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_lua_plugin,
                                                    lua_scripts, obj_pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps lua plugin data in WeeChat log file.
 */

int
weechat_lua_signal_debug_dump_cb (const void *pointer, void *data,
                                  const char *signal,
                                  const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, LUA_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_lua_plugin, lua_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
weechat_lua_timer_action_cb (const void *pointer, void *data,
                             int remaining_calls)
{
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    if (pointer)
    {
        if (pointer == &lua_action_install_list)
        {
            plugin_script_action_install (weechat_lua_plugin,
                                          lua_scripts,
                                          &weechat_lua_unload,
                                          &weechat_lua_load,
                                          &lua_quiet,
                                          &lua_action_install_list);
        }
        else if (pointer == &lua_action_remove_list)
        {
            plugin_script_action_remove (weechat_lua_plugin,
                                         lua_scripts,
                                         &weechat_lua_unload,
                                         &lua_quiet,
                                         &lua_action_remove_list);
        }
        else if (pointer == &lua_action_autoload_list)
        {
            plugin_script_action_autoload (weechat_lua_plugin,
                                           &lua_quiet,
                                           &lua_action_autoload_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove a script).
 */

int
weechat_lua_signal_script_action_cb (const void *pointer, void *data,
                                     const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "lua_script_install") == 0)
        {
            plugin_script_action_add (&lua_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_lua_timer_action_cb,
                                &lua_action_install_list, NULL);
        }
        else if (strcmp (signal, "lua_script_remove") == 0)
        {
            plugin_script_action_add (&lua_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_lua_timer_action_cb,
                                &lua_action_remove_list, NULL);
        }
        else if (strcmp (signal, "lua_script_autoload") == 0)
        {
            plugin_script_action_add (&lua_action_autoload_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_lua_timer_action_cb,
                                &lua_action_autoload_list, NULL);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes lua plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int old_lua_quiet;

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_lua_plugin = plugin;

    lua_quiet = 0;
    lua_eval_mode = 0;
    lua_eval_send_input = 0;
    lua_eval_exec_commands = 0;

    /* set interpreter name and version */
    weechat_hashtable_set (plugin->variables, "interpreter_name",
                           plugin->name);
#if defined(LUA_VERSION_MAJOR) && defined(LUA_VERSION_MINOR)
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           LUA_VERSION_MAJOR "." LUA_VERSION_MINOR);
#elif defined(LUA_VERSION)
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           LUA_VERSION);
#else
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           "");
#endif /* LUA_VERSION */

    /* init stdout/stderr buffer */
    lua_buffer_output = weechat_string_dyn_alloc (256);
    if (!lua_buffer_output)
        return WEECHAT_RC_ERROR;

    lua_data.config_file = &lua_config_file;
    lua_data.config_look_check_license = &lua_config_look_check_license;
    lua_data.config_look_eval_keep_context = &lua_config_look_eval_keep_context;
    lua_data.scripts = &lua_scripts;
    lua_data.last_script = &last_lua_script;
    lua_data.callback_command = &weechat_lua_command_cb;
    lua_data.callback_completion = &weechat_lua_completion_cb;
    lua_data.callback_hdata = &weechat_lua_hdata_cb;
    lua_data.callback_info_eval = &weechat_lua_info_eval_cb;
    lua_data.callback_infolist = &weechat_lua_infolist_cb;
    lua_data.callback_signal_debug_dump = &weechat_lua_signal_debug_dump_cb;
    lua_data.callback_signal_script_action = &weechat_lua_signal_script_action_cb;
    lua_data.callback_load_file = &weechat_lua_load_cb;
    lua_data.init_before_autoload = NULL;
    lua_data.unload_all = &weechat_lua_unload_all;

    old_lua_quiet = lua_quiet;
    lua_quiet = 1;
    plugin_script_init (weechat_lua_plugin, &lua_data);
    lua_quiet = old_lua_quiet;

    plugin_script_display_short_list (weechat_lua_plugin,
                                      lua_scripts);

    /* init OK */
    return WEECHAT_RC_OK;
}

/*
 * Ends lua plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    int old_lua_quiet;

    /* unload all scripts */
    old_lua_quiet = lua_quiet;
    lua_quiet = 1;
    if (lua_script_eval)
    {
        weechat_lua_unload (lua_script_eval);
        lua_script_eval = NULL;
    }
    plugin_script_end (plugin, &lua_data);
    lua_quiet = old_lua_quiet;

    /* free some data */
    if (lua_action_install_list)
    {
        free (lua_action_install_list);
        lua_action_install_list = NULL;
    }
    if (lua_action_remove_list)
    {
        free (lua_action_remove_list);
        lua_action_remove_list = NULL;
    }
    if (lua_action_autoload_list)
    {
        free (lua_action_autoload_list);
        lua_action_autoload_list = NULL;
    }
    weechat_string_dyn_free (lua_buffer_output, 1);
    lua_buffer_output = NULL;

    return WEECHAT_RC_OK;
}
