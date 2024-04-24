/*
 * weechat-js.cpp - javascript plugin for WeeChat
 *
 * Copyright (C) 2013 Koka El Kiwi <kokakiwi@kokakiwi.net>
 * Copyright (C) 2015-2024 Sébastien Helleu <flashcode@flashtux.org>
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
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of javascript scripts"));
WEECHAT_PLUGIN_AUTHOR("Koka El Kiwi <kokakiwi@kokakiwi.net>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(JS_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_js_plugin = NULL;

struct t_plugin_script_data js_data;

struct t_config_file *js_config_file = NULL;
struct t_config_option *js_config_look_check_license = NULL;
struct t_config_option *js_config_look_eval_keep_context = NULL;

int js_quiet = 0;

struct t_plugin_script *js_script_eval = NULL;
int js_eval_mode = 0;
int js_eval_send_input = 0;
int js_eval_exec_commands = 0;
struct t_gui_buffer *js_eval_buffer = NULL;

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

    v8::Handle<v8::Object> *obj = (v8::Handle<v8::Object> *)data;

    (*obj)->Set(v8::String::New(key), v8::String::New(value));
}

/*
 * Converts a WeeChat hashtable to a javascript hashtable.
 */

v8::Handle<v8::Object>
weechat_js_hashtable_to_object (struct t_hashtable *hashtable)
{
    v8::Handle<v8::Object> obj = v8::Object::New();

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
weechat_js_object_to_hashtable (v8::Handle<v8::Object> obj,
                                int size,
                                const char *type_keys,
                                const char *type_values)
{
    struct t_hashtable *hashtable;
    unsigned int i;
    v8::Handle<v8::Array> keys;
    v8::Handle<v8::Value> key, value;

    hashtable = weechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);

    if (!hashtable)
        return NULL;

    keys = obj->GetPropertyNames();
    for (i = 0; i < keys->Length(); i++)
    {
        key = keys->Get(i);
        value = obj->Get(key);
        v8::String::Utf8Value str_key(key);
        v8::String::Utf8Value str_value(value);
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
    v8::Handle<v8::Value> argv2[16], ret_js;
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
                case 's': /* string or null */
                    if (argv[i])
                        argv2[i] = v8::String::New((const char *)argv[i]);
                    else
                        argv2[i] = v8::Null();
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
            v8::String::Utf8Value temp_str(ret_js);
            ret_value = (*temp_str) ? strdup(*temp_str) : NULL;
        }
        else if ((ret_type == WEECHAT_SCRIPT_EXEC_POINTER) && (ret_js->IsString()))
        {
            v8::String::Utf8Value temp_str(ret_js);
            if (*temp_str)
            {
                ret_value = plugin_script_str2ptr (weechat_js_plugin,
                                                   script->name, function,
                                                   *temp_str);
            }
            else
            {
                ret_value = NULL;
            }
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
            if (ret_type != WEECHAT_SCRIPT_EXEC_IGNORE)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s%s: function \"%s\" must "
                                                 "return a valid value"),
                                weechat_prefix ("error"), JS_PLUGIN_NAME,
                                function);
            }
        }
    }

    if ((ret_type != WEECHAT_SCRIPT_EXEC_IGNORE) && !ret_value)
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
 * If code is NULL, the content of filename is read and executed.
 * If code is not NULL, it is executed (the file is not read).
 *
 * Returns pointer to new registered script, NULL if error.
 */

struct t_plugin_script *
weechat_js_load (const char *filename, const char *code)
{
    char *source;

    /* make C compiler happy */
    /* TODO: implement load of code in Javascript */
    (void) code;

    source = weechat_file_get_content (filename);
    if (!source)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), JS_PLUGIN_NAME, filename);
        return NULL;
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
        return NULL;
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

        return NULL;
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
        return NULL;
    }

    if (!js_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), JS_PLUGIN_NAME, filename);
        delete js_current_interpreter;
        return NULL;
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

    return js_current_script;
}

/*
 * Callback for script_auto_load() function.
 */

void
weechat_js_load_cb (void *data, const char *filename)
{
    const char *pos_dot;

    /* make C++ compiler happy */
    (void) data;

    pos_dot = strrchr (filename, '.');
    if (pos_dot && (strcmp (pos_dot, ".js") == 0))
        weechat_js_load (filename, NULL);
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
    free (filename);
}

/*
 * Unloads a javascript script by name.
 */

void
weechat_js_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (js_scripts, name);
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

    ptr_script = plugin_script_search (js_scripts, name);
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
            weechat_js_load (filename, NULL);
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
 * Evaluates javascript source code.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weechat_js_eval (struct t_gui_buffer *buffer, int send_to_buffer_as_input,
                 int exec_commands, const char *code)
{
    /* TODO: implement javascript eval */
    (void) buffer;
    (void) send_to_buffer_as_input;
    (void) exec_commands;
    (void) code;

    return 1;
}

/*
 * Callback for command "/javascript".
 */

int
weechat_js_command_cb (const void *pointer, void *data,
                       struct t_gui_buffer *buffer,
                       int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *ptr_code, *path_script;
    int i, send_to_buffer_as_input, exec_commands;

    /* make C++ compiler happy */
    (void) pointer;
    (void) data;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_js_plugin, js_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_js_plugin, js_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_js_plugin, js_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_js_plugin, &weechat_js_load_cb);
        }
        else if (weechat_strcmp (argv[1], "reload") == 0)
        {
            weechat_js_unload_all ();
            plugin_script_auto_load (weechat_js_plugin, &weechat_js_load_cb);
        }
        else if (weechat_strcmp(argv[1], "unload") == 0)
        {
            weechat_js_unload_all ();
        }
        else if (weechat_strcmp (argv[1], "version") == 0)
        {
            plugin_script_display_interpreter (weechat_js_plugin, 0);
        }
        else
            WEECHAT_COMMAND_ERROR;
    }
    else
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_js_plugin, js_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_js_plugin, js_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcmp (argv[1], "load") == 0)
                 || (weechat_strcmp (argv[1], "reload") == 0)
                 || (weechat_strcmp (argv[1], "unload") == 0))
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
            if (weechat_strcmp (argv[1], "load") == 0)
            {
                /* load javascript script */
                path_script = plugin_script_search_path (weechat_js_plugin,
                                                         ptr_name, 1);
                weechat_js_load ((path_script) ? path_script : ptr_name,
                                 NULL);
                free (path_script);
            }
            else if (weechat_strcmp (argv[1], "reload") == 0)
            {
                /* reload one javascript script */
                weechat_js_reload_name (ptr_name);
            }
            else if (weechat_strcmp (argv[1], "unload") == 0)
            {
                /* unload javascript script */
                weechat_js_unload_name (ptr_name);
            }
            js_quiet = 0;
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
            if (!weechat_js_eval (buffer, send_to_buffer_as_input,
                                  exec_commands, ptr_code))
                WEECHAT_COMMAND_ERROR;
            /* TODO: implement /javascript eval */
            weechat_printf (NULL,
                            _("%sCommand \"/%s eval\" is not yet implemented"),
                            weechat_prefix ("error"),
                            weechat_js_plugin->name);
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
weechat_js_completion_cb (const void *pointer, void *data,
                          const char *completion_item,
                          struct t_gui_buffer *buffer,
                          struct t_gui_completion *completion)
{
    /* make C++ compiler happy */
    (void) pointer;
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
weechat_js_hdata_cb (const void *pointer, void *data,
                     const char *hdata_name)
{
    /* make C++ compiler happy */
    (void) pointer;
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &js_scripts, &last_js_script,
                                       hdata_name);
}

/*
 * Returns javascript info "javascript_eval".
 */

char *
weechat_js_info_eval_cb (const void *pointer, void *data,
                         const char *info_name,
                         const char *arguments)
{
    const char *not_implemented = "not yet implemented";

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return strdup (not_implemented);
}

/*
 * Returns infolist with javascript scripts.
 */

struct t_infolist *
weechat_js_infolist_cb (const void *pointer, void *data,
                        const char *infolist_name,
                        void *obj_pointer, const char *arguments)
{
    /* make C++ compiler happy */
    (void) pointer;
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (strcmp (infolist_name, "javascript_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_js_plugin,
                                                    js_scripts, obj_pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps javascript plugin data in Weechat log file.
 */

int
weechat_js_signal_debug_dump_cb (const void *pointer, void *data,
                                 const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C++ compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, JS_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_js_plugin, js_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
weechat_js_timer_action_cb (const void *pointer, void *data,
                            int remaining_calls)
{
    /* make C++ compiler happy */
    (void) data;
    (void) remaining_calls;

    if (pointer)
    {
        if (pointer == &js_action_install_list)
        {
            plugin_script_action_install (weechat_js_plugin,
                                          js_scripts,
                                          &weechat_js_unload,
                                          &weechat_js_load,
                                          &js_quiet,
                                          &js_action_install_list);
        }
        else if (pointer == &js_action_remove_list)
        {
            plugin_script_action_remove (weechat_js_plugin,
                                         js_scripts,
                                         &weechat_js_unload,
                                         &js_quiet,
                                         &js_action_remove_list);
        }
        else if (pointer == &js_action_autoload_list)
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
weechat_js_signal_script_action_cb (const void *pointer, void *data,
                                    const char *signal,
                                    const char *type_data,
                                    void *signal_data)
{
    /* make C++ compiler happy */
    (void) pointer;
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "javascript_script_install") == 0)
        {
            plugin_script_action_add (&js_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_js_timer_action_cb,
                                &js_action_install_list, NULL);
        }
        else if (strcmp (signal, "javascript_script_remove") == 0)
        {
            plugin_script_action_add (&js_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_js_timer_action_cb,
                                &js_action_remove_list, NULL);
        }
        else if (strcmp (signal, "javascript_script_autoload") == 0)
        {
            plugin_script_action_add (&js_action_autoload_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_js_timer_action_cb,
                                &js_action_autoload_list, NULL);
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
    char str_interpreter[64];

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_js_plugin = plugin;

    js_quiet = 0;
    js_eval_mode = 0;
    js_eval_send_input = 0;
    js_eval_exec_commands = 0;

    /* set interpreter name and version */
    snprintf (str_interpreter, sizeof (str_interpreter),
              "%s (v8)", plugin->name);
    weechat_hashtable_set (plugin->variables, "interpreter_name",
                           str_interpreter);
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           v8::V8::GetVersion());

    js_data.config_file = &js_config_file;
    js_data.config_look_check_license = &js_config_look_check_license;
    js_data.config_look_eval_keep_context = &js_config_look_eval_keep_context;
    js_data.scripts = &js_scripts;
    js_data.last_script = &last_js_script;
    js_data.callback_command = &weechat_js_command_cb;
    js_data.callback_completion = &weechat_js_completion_cb;
    js_data.callback_hdata = &weechat_js_hdata_cb;
    js_data.callback_info_eval = &weechat_js_info_eval_cb;
    js_data.callback_infolist = &weechat_js_infolist_cb;
    js_data.callback_signal_debug_dump = &weechat_js_signal_debug_dump_cb;
    js_data.callback_signal_script_action = &weechat_js_signal_script_action_cb;
    js_data.callback_load_file = &weechat_js_load_cb;
    js_data.unload_all = &weechat_js_unload_all;

    js_quiet = 1;
    plugin_script_init (plugin, &js_data);
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
    if (js_script_eval)
    {
        weechat_js_unload (js_script_eval);
        js_script_eval = NULL;
    }
    plugin_script_end (plugin, &js_data);
    js_quiet = 0;

    /* free some data */
    if (js_action_install_list)
    {
        free (js_action_install_list);
        js_action_install_list = NULL;
    }
    if (js_action_remove_list)
    {
        free (js_action_remove_list);
        js_action_remove_list = NULL;
    }
    if (js_action_autoload_list)
    {
        free (js_action_autoload_list);
        js_action_autoload_list = NULL;
    }
    /* weechat_string_dyn_free (js_buffer_output, 1); */

    return WEECHAT_RC_OK;
}
