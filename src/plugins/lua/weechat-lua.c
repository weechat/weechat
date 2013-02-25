/*
 * weechat-lua.c - lua plugin for WeeChat
 *
 * Copyright (C) 2006-2007 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#undef _

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "weechat-lua.h"
#include "weechat-lua-api.h"


WEECHAT_PLUGIN_NAME(LUA_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of lua scripts"));
WEECHAT_PLUGIN_AUTHOR("Sebastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

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
 * Puts a WeeChat hashtable on lua stack, as lua table.
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
 * Gets WeeChat hashtable with lua hash (on stack).
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
weechat_lua_tohashtable (lua_State *interpreter, int index, int size,
                         const char *type_keys, const char *type_values)
{
    struct t_hashtable *hashtable;

    hashtable = weechat_hashtable_new (size,
                                       type_keys,
                                       type_values,
                                       NULL,
                                       NULL);
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
                                   plugin_script_str2ptr (weechat_lua_plugin,
                                                          NULL, NULL,
                                                          lua_tostring (interpreter, -1)));
        }
        /* remove value from stack (keep key for next iteration) */
        lua_pop (interpreter, 1);
    }

    return hashtable;
}

/*
 * Executes a lua function.
 */

void *
weechat_lua_exec (struct t_plugin_script *script, int ret_type,
                  const char *function,
                  const char *format, void **argv)
{
    void *ret_value;
    int argc, i, *ret_i;
    lua_State *old_lua_current_interpreter;
    struct t_plugin_script *old_lua_current_script;

    old_lua_current_interpreter = lua_current_interpreter;
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
                case 's': /* string */
                    lua_pushstring (lua_current_interpreter, (char *)argv[i]);
                    break;
                case 'i': /* integer */
                    lua_pushnumber (lua_current_interpreter, *((int *)argv[i]));
                    break;
                case 'h': /* hash */
                    weechat_lua_pushhashtable (lua_current_interpreter, (struct t_hashtable *)argv[i]);
                    break;
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
        lua_current_interpreter = old_lua_current_interpreter;
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
    else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
    {
        ret_value = weechat_lua_tohashtable (lua_current_interpreter, -1,
                                             WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_STRING);
    }
    else
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, function);
        lua_current_script = old_lua_current_script;
        lua_current_interpreter = old_lua_current_interpreter;
        return NULL;
    }

    lua_current_script = old_lua_current_script;
    lua_current_interpreter = old_lua_current_interpreter;

    return ret_value;
}

/*
 * Registers a library to use inside lua script.
 */

void weechat_lua_register_lib (lua_State *L, const char *libname,
                               const luaL_Reg *l)
{
#if LUA_VERSION_NUM >= 502
    if (libname)
    {
        lua_newtable (L);
        luaL_setfuncs (L, l, 0);
        lua_pushvalue (L, -1);
        lua_setglobal (L, libname);
    }
    else
        luaL_setfuncs (L, l, 0);
#else
    luaL_register (L, libname, l);
#endif
}

/*
 * Loads a lua script.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

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

    if ((weechat_lua_plugin->debug >= 2) || !lua_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        LUA_PLUGIN_NAME, filename);
    }

    lua_current_script = NULL;
    lua_registered_script = NULL;

    lua_current_interpreter = luaL_newstate();

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

    weechat_lua_register_lib (lua_current_interpreter, "weechat", weechat_lua_api_funcs);

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

        /* if script was registered, remove it from list */
        if (lua_current_script)
        {
            plugin_script_remove (weechat_lua_plugin, &lua_scripts, &last_lua_script,
                                  lua_current_script);
        }

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

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (weechat_lua_plugin,
                                        lua_scripts,
                                        lua_current_script,
                                        &weechat_lua_api_buffer_input_data_cb,
                                        &weechat_lua_api_buffer_close_cb);

    weechat_hook_signal_send ("lua_script_loaded", WEECHAT_HOOK_SIGNAL_STRING,
                              lua_current_script->filename);

    return 1;
}

/*
 * Callback for weechat_script_auto_load() function.
 */

void
weechat_lua_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    weechat_lua_load (filename);
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
        if (rc)
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

    weechat_hook_signal_send ("lua_script_unloaded",
                              WEECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * Unloads a lua script by name.
 */

void
weechat_lua_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (weechat_lua_plugin, lua_scripts, name);
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

    ptr_script = plugin_script_search (weechat_lua_plugin, lua_scripts, name);
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
 * Unloads all lua scripts.
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
 * Callback for command "/lua".
 */

int
weechat_lua_command_cb (void *data, struct t_gui_buffer *buffer,
                        int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_lua_plugin, lua_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_lua_plugin, lua_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_lua_plugin, lua_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_lua_plugin, &weechat_lua_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_lua_unload_all ();
            plugin_script_auto_load (weechat_lua_plugin, &weechat_lua_load_cb);
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
            plugin_script_display_list (weechat_lua_plugin, lua_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_lua_plugin, lua_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcasecmp (argv[1], "load") == 0)
                 || (weechat_strcasecmp (argv[1], "reload") == 0)
                 || (weechat_strcasecmp (argv[1], "unload") == 0))
        {
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
            if (weechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load lua script */
                path_script = plugin_script_search_path (weechat_lua_plugin,
                                                         ptr_name);
                weechat_lua_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (weechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one lua script */
                weechat_lua_reload_name (ptr_name);
            }
            else if (weechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload lua script */
                weechat_lua_unload_name (ptr_name);
            }
            lua_quiet = 0;
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
 * Adds lua scripts to completion list.
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

    plugin_script_completion (weechat_lua_plugin, completion, lua_scripts);

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for lua scripts.
 */

struct t_hdata *
weechat_lua_hdata_cb (void *data, const char *hdata_name)
{
    /* make C compiler happy */
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &lua_scripts, &last_lua_script,
                                       hdata_name);
}

/*
 * Returns infolist with lua scripts.
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
        return plugin_script_infolist_list_scripts (weechat_lua_plugin,
                                                    lua_scripts, pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps lua plugin data in WeeChat log file.
 */

int
weechat_lua_signal_debug_dump_cb (void *data, const char *signal,
                                  const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (weechat_strcasecmp ((char *)signal_data, LUA_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_lua_plugin, lua_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a buffer is closed.
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
        plugin_script_remove_buffer_callbacks (lua_scripts, signal_data);

    return WEECHAT_RC_OK;
}

/*
 * Timer for executing actions.
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
            plugin_script_action_install (weechat_lua_plugin,
                                          lua_scripts,
                                          &weechat_lua_unload,
                                          &weechat_lua_load,
                                          &lua_quiet,
                                          &lua_action_install_list);
        }
        else if (data == &lua_action_remove_list)
        {
            plugin_script_action_remove (weechat_lua_plugin,
                                         lua_scripts,
                                         &weechat_lua_unload,
                                         &lua_quiet,
                                         &lua_action_remove_list);
        }
        else if (data == &lua_action_autoload_list)
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
            plugin_script_action_add (&lua_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_lua_timer_action_cb,
                                &lua_action_install_list);
        }
        else if (strcmp (signal, "lua_script_remove") == 0)
        {
            plugin_script_action_add (&lua_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_lua_timer_action_cb,
                                &lua_action_remove_list);
        }
        else if (strcmp (signal, "lua_script_autoload") == 0)
        {
            plugin_script_action_add (&lua_action_autoload_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_lua_timer_action_cb,
                                &lua_action_autoload_list);
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
    struct t_plugin_script_init init;

    weechat_lua_plugin = plugin;

    init.callback_command = &weechat_lua_command_cb;
    init.callback_completion = &weechat_lua_completion_cb;
    init.callback_hdata = &weechat_lua_hdata_cb;
    init.callback_infolist = &weechat_lua_infolist_cb;
    init.callback_signal_debug_dump = &weechat_lua_signal_debug_dump_cb;
    init.callback_signal_buffer_closed = &weechat_lua_signal_buffer_closed_cb;
    init.callback_signal_script_action = &weechat_lua_signal_script_action_cb;
    init.callback_load_file = &weechat_lua_load_cb;

    lua_quiet = 1;
    plugin_script_init (weechat_lua_plugin, argc, argv, &init);
    lua_quiet = 0;

    plugin_script_display_short_list (weechat_lua_plugin,
                                      lua_scripts);

    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * Ends lua plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* unload all scripts */
    lua_quiet = 1;
    plugin_script_end (plugin, &lua_scripts, &weechat_lua_unload_all);
    lua_quiet = 0;

    /* free some data */
    if (lua_action_install_list)
        free (lua_action_install_list);
    if (lua_action_remove_list)
        free (lua_action_remove_list);
    if (lua_action_autoload_list)
        free (lua_action_autoload_list);

    return WEECHAT_RC_OK;
}
