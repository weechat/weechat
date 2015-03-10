/*
 * weechat-js.cpp - javascript plugin for WeeChat
 *
 * Copyright (C) 2013 Koka El Kiwi <admin@kokabsolu.com>
 * Copyright (C) 2015 Sébastien Helleu <flashcode@flashtux.org>
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

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C"
{
#include "../weechat-plugin.h"
#include "../plugin-script.h"
}

#include "weechat-js.h"
#include "weechat-js-api.h"
#include "weechat-js-v8.h"

WEECHAT_PLUGIN_NAME(JS_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("Support of javascript scripts");
WEECHAT_PLUGIN_AUTHOR("Koka El Kiwi <admin@kokabsolu.com>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(3000);

struct t_weechat_plugin *weechat_js_plugin;

int js_quiet = 0;
struct t_plugin_script *js_scripts = NULL;
struct t_plugin_script *last_js_script = NULL;
struct t_plugin_script *js_current_script = NULL;
struct t_plugin_script *js_registered_script = NULL;
const char *js_current_script_filename = NULL;
WeechatJsV8 *js_current_interpreter = NULL;

/*
 * string used to execute action "install":
 * when signal "js_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *js_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "js_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *js_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "js_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *js_action_autoload_list = NULL;


/*
 * Callback called for each key/value in a hashtable.
 */

void
weechat_js_hashtable_map_cb (void *data,
                             struct t_hashtable *hashtable,
                             const char *key,
                             const char *value)
{
    /* make C++ compiler happy */
    (void) hashtable;

    Handle<Object> *obj = (Handle<Object> *)data;

    (*obj)->Set(String::New(key), String::New(value));
}

/*
 * Converts a WeeChat hashtable to a javascript hashtable.
 */

Handle<Object>
weechat_js_hashtable_to_object (struct t_hashtable *hashtable)
{
    Handle<Object> obj = Object::New();

    weechat_hashtable_map_string (hashtable,
                                  &weechat_js_hashtable_map_cb,
                                  &obj);
    return obj;
}

/*
 * Converts a javascript hashtable to a WeeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
weechat_js_object_to_hashtable (Handle<Object> obj,
                                int size,
                                const char *type_keys,
                                const char *type_values)
{
    struct t_hashtable *hashtable;
    unsigned int i;
    Handle<Array> keys;
    Handle<Value> key, value;

    hashtable = weechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);

    if (!hashtable)
        return NULL;

    keys = obj->GetPropertyNames();
    for (i = 0; i < keys->Length(); i++)
    {
        key = keys->Get(i);
        value = obj->Get(key);
        String::Utf8Value str_key(key);
        String::Utf8Value str_value(value);
        if (strcmp (type_values, WEECHAT_HASHTABLE_STRING) == 0)
        {
            weechat_hashtable_set (hashtable, *str_key, *str_value);
        }
        else if (strcmp (type_values, WEECHAT_HASHTABLE_POINTER) == 0)
        {
            weechat_hashtable_set (hashtable, *str_key,
                                   plugin_script_str2ptr (weechat_js_plugin,
                                                          NULL, NULL,
                                                          *str_value));
        }
    }

    return hashtable;
}

/*
 * Executes a javascript function.
 */

void *
weechat_js_exec (struct t_plugin_script *script,
                 int ret_type, const char *function,
                 const char *format, void **argv)
{
    struct t_plugin_script *old_js_current_script;
    WeechatJsV8 *js_v8;
    void *ret_value;
    Handle<Value> argv2[16], ret_js;
    int i, argc, *ret_int;

    ret_value = NULL;

    old_js_current_script = js_current_script;
    js_current_script = script;
    js_v8 = (WeechatJsV8 *)(script->interpreter);

    if (!js_v8->functionExists(function))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to run function \"%s\""),
                        weechat_prefix ("error"), JS_PLUGIN_NAME, function);
        goto end;
    }

    argc = 0;
    if (format && format[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string */
                    argv2[i] = v8::String::New((const char *)argv[i]);
                    break;
                case 'i': /* integer */
                    argv2[i] = v8::Integer::New(*((int *)argv[i]));
                    break;
                case 'h': /* hash */
                    argv2[i] = weechat_js_hashtable_to_object (
                        (struct t_hashtable *)argv[i]);
                    break;
            }
        }
    }

    ret_js = js_v8->execFunction(function,
                                 argc,
                                 (argc > 0) ? argv2 : NULL);

    if (!ret_js.IsEmpty())
    {
        if ((ret_type == WEECHAT_SCRIPT_EXEC_STRING) && (ret_js->IsString()))
        {
            String::Utf8Value temp_str(ret_js);
            ret_value = *temp_str;
        }
        else if ((ret_type == WEECHAT_SCRIPT_EXEC_INT) && (ret_js->IsInt32()))
        {
            ret_int = (int *)malloc (sizeof (*ret_int));
            if (ret_int)
                *ret_int = (int)(ret_js->IntegerValue());
            ret_value = ret_int;
        }
        else if ((ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
                 && (ret_js->IsObject()))
        {
            ret_value = (struct t_hashtable *)weechat_js_object_to_hashtable (
                ret_js->ToObject(),
                WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                WEECHAT_HASHTABLE_STRING,
                WEECHAT_HASHTABLE_STRING);
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: function \"%s\" must "
                                             "return a valid value"),
                            weechat_prefix ("error"), JS_PLUGIN_NAME,
                            function);
        }
    }

    if (!ret_value)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error in function \"%s\""),
                        weechat_prefix ("error"), JS_PLUGIN_NAME, function);
    }

end:
    js_current_script = old_js_current_script;

    return ret_value;
}

/*
 * Loads a javascript script.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weechat_js_load (const char *filename)
{
    char *source;

    source = weechat_file_get_content (filename);
    if (!source)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), JS_PLUGIN_NAME, filename);
        return 0;
    }

    if ((weechat_js_plugin->debug >= 2) || !js_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        JS_PLUGIN_NAME, filename);
    }

    js_current_script = NULL;
    js_registered_script = NULL;

    js_current_interpreter = new WeechatJsV8();

    if (!js_current_interpreter)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        weechat_prefix ("error"), JS_PLUGIN_NAME);
        free (source);
        return 0;
    }

    /* load libs */
    js_current_interpreter->loadLibs();

    js_current_script_filename = filename;

    if (!js_current_interpreter->load(source))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to load file \"%s\""),
                        weechat_prefix ("error"), JS_PLUGIN_NAME);
        delete js_current_interpreter;
        free (source);

        /* if script was registered, remove it from list */
        if (js_current_script)
        {
            plugin_script_remove (weechat_js_plugin,
                                  &js_scripts, &last_js_script,
                                  js_current_script);
            js_current_script = NULL;
        }

        return 0;
    }

    free (source);

    if (!js_current_interpreter->execScript())
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to execute file "
                                         "\"%s\""),
                        weechat_prefix ("error"), JS_PLUGIN_NAME, filename);
        delete js_current_interpreter;

        /* if script was registered, remove it from list */
        if (js_current_script)
        {
            plugin_script_remove (weechat_js_plugin,
                                  &js_scripts, &last_js_script,
                                  js_current_script);
            js_current_script = NULL;
        }
        return 0;
    }

    if (!js_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), JS_PLUGIN_NAME, filename);
        delete js_current_interpreter;
        return 0;
    }

    js_current_script = js_registered_script;

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (weechat_js_plugin,
                                        js_scripts,
                                        js_current_script,
                                        &weechat_js_api_buffer_input_data_cb,
                                        &weechat_js_api_buffer_close_cb);

    weechat_hook_signal_send ("javascript_script_loaded",
                              WEECHAT_HOOK_SIGNAL_STRING,
                              js_current_script->filename);

    return 1;
}

/*
 * Callback for script_auto_load() function.
 */

void
weechat_js_load_cb (void *data, const char *filename)
{
    /* make C++ compiler happy */
    (void) data;

    weechat_js_load (filename);
}

/*
 * Unloads a javascript script.
 */

void
weechat_js_unload (struct t_plugin_script *script)
{
    int *rc;
    char *filename;
    void *interpreter;

    if ((weechat_js_plugin->debug >= 2) || !js_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        JS_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)weechat_js_exec (script, WEECHAT_SCRIPT_EXEC_INT,
                                     script->shutdown_func, NULL, NULL);
        if (rc)
            free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (js_current_script == script)
    {
        js_current_script = (js_current_script->prev_script) ?
            js_current_script->prev_script : js_current_script->next_script;
    }

    plugin_script_remove (weechat_js_plugin,
                          &js_scripts, &last_js_script, script);

    if (interpreter)
        delete((WeechatJsV8 *)interpreter);

    (void) weechat_hook_signal_send ("javascript_script_unloaded",
                                     WEECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * Unloads a javascript script by name.
 */

void
weechat_js_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (weechat_js_plugin, js_scripts, name);
    if (ptr_script)
    {
        weechat_js_unload (ptr_script);
        if (!js_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            JS_PLUGIN_NAME, name);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), JS_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all javascript scripts.
 */

void
weechat_js_unload_all ()
{
    while (js_scripts)
    {
        weechat_js_unload (js_scripts);
    }
}

/*
 * Reloads a javascript script by name.
 */

void
weechat_js_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (weechat_js_plugin, js_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_js_unload (ptr_script);
            if (!js_quiet)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s: script \"%s\" unloaded"),
                                JS_PLUGIN_NAME, name);
            }
            weechat_js_load (filename);
            free (filename);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), JS_PLUGIN_NAME, name);
    }
}

/*
 * Callback for command "/javascript".
 */

int
weechat_js_command_cb (void *data, struct t_gui_buffer *buffer,
                       int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;

    /* make C++ compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_js_plugin, js_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_js_plugin, js_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_js_plugin, js_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_js_plugin, &weechat_js_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_js_unload_all ();
            plugin_script_auto_load (weechat_js_plugin, &weechat_js_load_cb);
        }
        else if (weechat_strcasecmp(argv[1], "unload"))
        {
            weechat_js_unload_all ();
        }
    }
    else
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_js_plugin, js_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_js_plugin, js_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcasecmp (argv[1], "load") == 0)
                 || (weechat_strcasecmp (argv[1], "reload") == 0)
                 || (weechat_strcasecmp (argv[1], "unload") == 0))
        {
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                js_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (weechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load javascript script */
                path_script = plugin_script_search_path (weechat_js_plugin,
                                                         ptr_name);
                weechat_js_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (weechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one javascript script */
                weechat_js_reload_name (ptr_name);
            }
            else if (weechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload javascript script */
                weechat_js_unload_name (ptr_name);
            }
            js_quiet = 0;
        }
        else
            WEECHAT_COMMAND_ERROR;
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds javascript scripts to completion list.
 */

int
weechat_js_completion_cb (void *data, const char *completion_item,
                          struct t_gui_buffer *buffer,
                          struct t_gui_completion *completion)
{
    /* make C++ compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_js_plugin, completion, js_scripts);

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for javascript scripts.
 */

struct t_hdata *
weechat_js_hdata_cb (void *data, const char *hdata_name)
{
    /* make C++ compiler happy */
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &js_scripts, &last_js_script,
                                       hdata_name);
}

/*
 * Returns infolist with javascript scripts.
 */

struct t_infolist *
weechat_js_infolist_cb (void *data, const char *infolist_name,
                        void *pointer, const char *arguments)
{
    /* make C++ compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (weechat_strcasecmp (infolist_name, "javascript_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_js_plugin,
                                                    js_scripts, pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps javascript plugin data in Weechat log file.
 */

int
weechat_js_signal_debug_dump_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C++ compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (weechat_strcasecmp ((char *)signal_data, JS_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_js_plugin, js_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * Display infos about external libraries used.
 */

int
weechat_js_signal_debug_libs_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C++ compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    weechat_printf (NULL, "  %s (v8): %s", JS_PLUGIN_NAME, V8::GetVersion());

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a buffer is closed.
 */

int
weechat_js_signal_buffer_closed_cb (void *data, const char *signal,
                                    const char *type_data, void *signal_data)
{
    /* make C++ compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
    {
        plugin_script_remove_buffer_callbacks (js_scripts,
                                               (struct t_gui_buffer *)signal_data);
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
weechat_js_timer_action_cb (void *data, int remaining_calls)
{
    /* make C++ compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &js_action_install_list)
        {
            plugin_script_action_install (weechat_js_plugin,
                                          js_scripts,
                                          &weechat_js_unload,
                                          &weechat_js_load,
                                          &js_quiet,
                                          &js_action_install_list);
        }
        else if (data == &js_action_remove_list)
        {
            plugin_script_action_remove (weechat_js_plugin,
                                         js_scripts,
                                         &weechat_js_unload,
                                         &js_quiet,
                                         &js_action_remove_list);
        }
        else if (data == &js_action_autoload_list)
        {
            plugin_script_action_autoload (weechat_js_plugin,
                                           &js_quiet,
                                           &js_action_autoload_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove/autoload a
 * script).
 */

int
weechat_js_signal_script_action_cb (void *data, const char *signal,
                                    const char *type_data,
                                    void *signal_data)
{
    /* make C++ compiler happy */
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "javascript_script_install") == 0)
        {
            plugin_script_action_add (&js_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_js_timer_action_cb,
                                &js_action_install_list);
        }
        else if (strcmp (signal, "javascript_script_remove") == 0)
        {
            plugin_script_action_add (&js_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_js_timer_action_cb,
                                &js_action_remove_list);
        }
        else if (strcmp (signal, "javascript_script_autoload") == 0)
        {
            plugin_script_action_add (&js_action_autoload_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_js_timer_action_cb,
                                &js_action_autoload_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes javascript plugin.
 */

EXPORT int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;

    weechat_js_plugin = plugin;

    init.callback_command = &weechat_js_command_cb;
    init.callback_completion = &weechat_js_completion_cb;
    init.callback_hdata = &weechat_js_hdata_cb;
    init.callback_infolist = &weechat_js_infolist_cb;
    init.callback_signal_debug_dump = &weechat_js_signal_debug_dump_cb;
    init.callback_signal_debug_libs = &weechat_js_signal_debug_libs_cb;
    init.callback_signal_buffer_closed = &weechat_js_signal_buffer_closed_cb;
    init.callback_signal_script_action = &weechat_js_signal_script_action_cb;
    init.callback_load_file = &weechat_js_load_cb;

    js_quiet = 1;
    plugin_script_init (plugin, argc, argv, &init);
    js_quiet = 0;

    plugin_script_display_short_list (weechat_js_plugin, js_scripts);

    return WEECHAT_RC_OK;
}

/*
 * Ends javascript plugin.
 */

EXPORT int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    js_quiet = 1;
    plugin_script_end (plugin, &js_scripts, &weechat_js_unload_all);
    js_quiet = 0;

    /* free some data */
    if (js_action_install_list)
        free (js_action_install_list);
    if (js_action_remove_list)
        free (js_action_remove_list);
    if (js_action_autoload_list)
        free (js_action_autoload_list);

    return WEECHAT_RC_OK;
}
