/*
 * Copyright (C) 2011-2012 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * weechat-guile.c: guile (scheme) plugin for WeeChat
 */

#undef _

#include <libguile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "weechat-guile.h"
#include "weechat-guile-api.h"


WEECHAT_PLUGIN_NAME(GUILE_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of scheme scripts (with Guile)"));
WEECHAT_PLUGIN_AUTHOR("Sebastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_guile_plugin = NULL;

int guile_quiet;
struct t_plugin_script *guile_scripts = NULL;
struct t_plugin_script *last_guile_script = NULL;
struct t_plugin_script *guile_current_script = NULL;
struct t_plugin_script *guile_registered_script = NULL;
const char *guile_current_script_filename = NULL;
SCM guile_module_weechat;
SCM guile_port;
char *guile_stdout = NULL;

struct t_guile_function
{
    SCM proc;
    SCM args;
};

/*
 * string used to execute action "install":
 * when signal "guile_install_script" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *guile_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "guile_remove_script" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *guile_action_remove_list = NULL;


/*
 * weechat_guile_stdout_flush: flush stdout
 */

void
weechat_guile_stdout_flush ()
{
    if (guile_stdout)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: stdout/stderr: %s%s"),
                        GUILE_PLUGIN_NAME, guile_stdout, "");
        free (guile_stdout);
        guile_stdout = NULL;
    }
}

/*
 * weechat_guile_catch: execute scheme procedure with internal catch
 *                      and return value
 */

SCM
weechat_guile_catch (void *procedure, void *data)
{
    SCM value;

    value = scm_internal_catch (SCM_BOOL_T,
                                (scm_t_catch_body)procedure,
                                data,
                                (scm_t_catch_handler) scm_handle_by_message_noexit,
                                NULL);
    return value;
}

/*
 * weechat_guile_scm_call_1: encapsulate call to scm_call_1 (to give arguments)
 */

SCM
weechat_guile_scm_call_1 (void *proc)
{
    struct t_guile_function *guile_function;

    guile_function = (struct t_guile_function *)proc;

    return scm_call_1 (guile_function->proc, guile_function->args);
}

/*
 * weechat_guile_exec_function: execute scheme function (with optional args)
 *                              and return value
 */

SCM
weechat_guile_exec_function (const char *function, SCM args)
{
    SCM func, func2, value;
    struct t_guile_function guile_function;

    func = weechat_guile_catch (scm_c_lookup, (void *)function);
    func2 = weechat_guile_catch (scm_variable_ref, func);

    if (args)
    {
        guile_function.proc = func2;
        guile_function.args = args;
        value = weechat_guile_catch (weechat_guile_scm_call_1, &guile_function);
    }
    else
    {
        value = weechat_guile_catch (scm_call_0, func2);
    }

    return value;
}

/*
 * weechat_guile_hashtable_map_cb: callback called for each key/value in a
 *                                 hashtable
 */

void
weechat_guile_hashtable_map_cb (void *data,
                                struct t_hashtable *hashtable,
                                const char *key,
                                const char *value)
{
    SCM *alist, pair, list;

    /* make C compiler happy */
    (void) hashtable;

    alist = (SCM *)data;

    pair = scm_cons (scm_from_locale_string (key),
                     scm_from_locale_string (value));
    list = scm_list_1 (pair);

    *alist = scm_append (scm_list_2 (*alist, list));
}

/*
 * weechat_guile_hashtable_to_alist: get Guile alist with a WeeChat hashtable
 */

SCM
weechat_guile_hashtable_to_alist (struct t_hashtable *hashtable)
{
    SCM alist;

    alist = scm_list_n (SCM_UNDEFINED);

    weechat_hashtable_map_string (hashtable,
                                  &weechat_guile_hashtable_map_cb,
                                  &alist);

    return alist;
}

/*
 * weechat_guile_alist_to_hashtable: get WeeChat hashtable with Guile alist
 *                                   Note: hashtable has to be released after
 *                                   use with call to weechat_hashtable_free()
 */

struct t_hashtable *
weechat_guile_alist_to_hashtable (SCM alist, int size, const char *type_keys,
                                  const char *type_values)
{
    struct t_hashtable *hashtable;
    int length, i;
    SCM pair;

    hashtable = weechat_hashtable_new (size,
                                       type_keys,
                                       type_values,
                                       NULL,
                                       NULL);
    if (!hashtable)
        return NULL;

    length = scm_to_int (scm_length (alist));
    for (i = 0; i < length; i++)
    {
        pair = scm_list_ref (alist, scm_from_int (i));
        if (strcmp (type_values, WEECHAT_HASHTABLE_STRING) == 0)
        {
            weechat_hashtable_set (hashtable,
                                   scm_i_string_chars (scm_list_ref (pair,
                                                                     scm_from_int (0))),
                                   scm_i_string_chars (scm_list_ref (pair,
                                                                     scm_from_int (1))));
        }
        else if (strcmp (type_values, WEECHAT_HASHTABLE_POINTER) == 0)
        {
            weechat_hashtable_set (hashtable,
                                   scm_i_string_chars (scm_list_ref (pair,
                                                                     scm_from_int (0))),
                                   plugin_script_str2ptr (weechat_guile_plugin,
                                                          NULL, NULL,
                                                          scm_i_string_chars (scm_list_ref (pair,
                                                                                            scm_from_int (1)))));
        }
    }

    return hashtable;
}

/*
 * weechat_guile_exec: execute a Guile function
 */

void *
weechat_guile_exec (struct t_plugin_script *script,
                    int ret_type, const char *function,
                    char *format, void **argv)
{
    struct t_plugin_script *old_guile_current_script;
    SCM argv_list, rc;
    void *argv2[17], *ret_value;
    int i, argc, *ret_int;

    old_guile_current_script = guile_current_script;
    scm_set_current_module ((SCM)(script->interpreter));
    guile_current_script = script;

    if (argv && argv[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string */
                    argv2[i] = scm_from_locale_string ((char *)argv[i]);
                    break;
                case 'i': /* integer */
                    argv2[i] = scm_from_int (*((int *)argv[i]));
                    break;
                case 'h': /* hash */
                    argv2[i] = weechat_guile_hashtable_to_alist (argv[i]);
                    break;
            }
        }
        for (i = argc; i < 17; i++)
        {
            argv2[i] = SCM_UNDEFINED;
        }
        argv_list = scm_list_n (argv2[0], argv2[1],
                                argv2[2], argv2[3],
                                argv2[4], argv2[5],
                                argv2[6], argv2[7],
                                argv2[8], argv2[9],
                                argv2[10], argv2[11],
                                argv2[12], argv2[13],
                                argv2[14], argv2[15],
                                argv2[16]);
        rc = weechat_guile_exec_function (function, argv_list);
    }
    else
    {
        rc = weechat_guile_exec_function (function, NULL);
    }

    ret_value = NULL;

    if ((ret_type == WEECHAT_SCRIPT_EXEC_STRING) && (scm_is_string (rc)))
    {
        ret_value = scm_to_locale_string (rc);
    }
    else if ((ret_type == WEECHAT_SCRIPT_EXEC_INT) && (scm_is_integer (rc)))
    {
        ret_int = malloc (sizeof (*ret_int));
        if (ret_int)
            *ret_int = scm_to_int (rc);
        ret_value = ret_int;
    }
    else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
    {
        ret_value = weechat_guile_alist_to_hashtable (rc,
                                                      WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                      WEECHAT_HASHTABLE_STRING,
                                                      WEECHAT_HASHTABLE_STRING);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"%s\" must return "
                                         "a valid value"),
                        weechat_prefix ("error"), GUILE_PLUGIN_NAME, function);
    }

    if (ret_value == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error in function \"%s\""),
                        weechat_prefix ("error"), GUILE_PLUGIN_NAME, function);
    }

    if (old_guile_current_script)
        scm_set_current_module ((SCM)(old_guile_current_script->interpreter));

    guile_current_script = old_guile_current_script;

    return ret_value;
}

/*
 * weechat_guile_module_init_script: init Guile module for script
 */

void
weechat_guile_module_init_script (void *data)
{
    SCM rc;

    weechat_guile_catch (scm_c_eval_string, "(use-modules (weechat))");
    rc = weechat_guile_catch (scm_c_primitive_load, data);

    /* error loading script? */
    if (rc == SCM_BOOL_F)
    {
        /* if script was registered, remove it from list */
        if (guile_current_script)
        {
            plugin_script_remove (weechat_guile_plugin,
                                  &guile_scripts, &last_guile_script,
                                  guile_current_script);
        }
        guile_current_script = NULL;
        guile_registered_script = NULL;
    }
}

/*
 * weechat_guile_load: load a Guile script
 */

int
weechat_guile_load (const char *filename)
{
    char *filename2, *ptr_base_name, *base_name;
    SCM module;

    if ((weechat_guile_plugin->debug >= 2) || !guile_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        GUILE_PLUGIN_NAME, filename);
    }

    guile_current_script = NULL;
    guile_registered_script = NULL;
    guile_current_script_filename = filename;

    filename2 = strdup (filename);
    if (!filename2)
        return 0;

    ptr_base_name = basename (filename2);
    base_name = strdup (ptr_base_name);
    module = scm_c_define_module (base_name,
                                  &weechat_guile_module_init_script, filename2);
    free (filename2);

    if (!guile_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), GUILE_PLUGIN_NAME, filename);
        return 0;
    }

    weechat_guile_catch (scm_gc_protect_object, (void *)module);

    guile_current_script = guile_registered_script;
    guile_current_script->interpreter = (void *)module;

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (weechat_guile_plugin,
                                        guile_scripts,
                                        guile_current_script,
                                        &weechat_guile_api_buffer_input_data_cb,
                                        &weechat_guile_api_buffer_close_cb);

    weechat_hook_signal_send ("guile_script_loaded", WEECHAT_HOOK_SIGNAL_STRING,
                              guile_current_script->filename);

    return 1;
}

/*
 * weechat_guile_load_cb: callback for script_auto_load() function
 */

void
weechat_guile_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    weechat_guile_load (filename);
}

/*
 * weechat_guile_unload: unload a Guile script
 */

void
weechat_guile_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;
    char *filename;

    if ((weechat_guile_plugin->debug >= 2) || !guile_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        GUILE_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)weechat_guile_exec (script, WEECHAT_SCRIPT_EXEC_INT,
                                        script->shutdown_func, NULL, NULL);
        if (rc)
            free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (guile_current_script == script)
        guile_current_script = (guile_current_script->prev_script) ?
            guile_current_script->prev_script : guile_current_script->next_script;

    plugin_script_remove (weechat_guile_plugin, &guile_scripts, &last_guile_script,
                          script);

    if (interpreter)
        weechat_guile_catch (scm_gc_unprotect_object, interpreter);

    weechat_hook_signal_send ("guile_script_unloaded",
                              WEECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * weechat_guile_unload_name: unload a Guile script by name
 */

void
weechat_guile_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (weechat_guile_plugin, guile_scripts, name);
    if (ptr_script)
    {
        weechat_guile_unload (ptr_script);
        if (!guile_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            GUILE_PLUGIN_NAME, name);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), GUILE_PLUGIN_NAME, name);
    }
}

/*
 * weechat_guile_unload_all: unload all Guile scripts
 */

void
weechat_guile_unload_all ()
{
    while (guile_scripts)
    {
        weechat_guile_unload (guile_scripts);
    }
}

/*
 * weechat_guile_reload_name: reload a Guile script by name
 */

void
weechat_guile_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (weechat_guile_plugin, guile_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_guile_unload (ptr_script);
            if (!guile_quiet)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s: script \"%s\" unloaded"),
                                GUILE_PLUGIN_NAME, name);
            }
            weechat_guile_load (filename);
            free (filename);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), GUILE_PLUGIN_NAME, name);
    }
}

/*
 * weechat_guile_cmd: callback for "/guile" command
 */

int
weechat_guile_command_cb (void *data, struct t_gui_buffer *buffer,
                          int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;
    SCM value;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_guile_plugin, guile_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_guile_plugin, guile_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_guile_plugin, guile_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_guile_plugin, &weechat_guile_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_guile_unload_all ();
            plugin_script_auto_load (weechat_guile_plugin, &weechat_guile_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            weechat_guile_unload_all ();
        }
    }
    else
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_guile_plugin, guile_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_guile_plugin, guile_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcasecmp (argv[1], "load") == 0)
                 || (weechat_strcasecmp (argv[1], "reload") == 0)
                 || (weechat_strcasecmp (argv[1], "unload") == 0))
        {
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                guile_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (weechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load Guile script */
                path_script = plugin_script_search_path (weechat_guile_plugin,
                                                         ptr_name);
                weechat_guile_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (weechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one Guile script */
                weechat_guile_reload_name (ptr_name);
            }
            else if (weechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload Guile script */
                weechat_guile_unload_name (ptr_name);
            }
            guile_quiet = 0;
        }
        else if (weechat_strcasecmp (argv[1], "eval") == 0)
        {
            /* eval Guile code */
            value = weechat_guile_catch (scm_c_eval_string, argv_eol[2]);
            if (!SCM_EQ_P (value, SCM_UNDEFINED)
                && !SCM_EQ_P (value, SCM_UNSPECIFIED))
            {
                scm_display (value, guile_port);
            }
            weechat_guile_stdout_flush ();
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unknown option for "
                                             "command \"%s\""),
                            weechat_prefix ("error"), GUILE_PLUGIN_NAME,
                            "guile");
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_guile_completion_cb: callback for script completion
 */

int
weechat_guile_completion_cb (void *data, const char *completion_item,
                             struct t_gui_buffer *buffer,
                             struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_guile_plugin, completion, guile_scripts);

    return WEECHAT_RC_OK;
}

/*
 * weechat_guile_hdata_cb: callback for hdata
 */

struct t_hdata *
weechat_guile_hdata_cb (void *data, const char *hdata_name)
{
    /* make C compiler happy */
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &guile_scripts, &last_guile_script,
                                       hdata_name);
}

/*
 * weechat_guile_infolist_cb: callback for infolist
 */

struct t_infolist *
weechat_guile_infolist_cb (void *data, const char *infolist_name,
                           void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (weechat_strcasecmp (infolist_name, "guile_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_guile_plugin,
                                                    guile_scripts, pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * weechat_guile_signal_debug_dump_cb: dump Guile plugin data in WeeChat log
 *                                     file
 */

int
weechat_guile_signal_debug_dump_cb (void *data, const char *signal,
                                    const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (weechat_strcasecmp ((char *)signal_data, GUILE_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_guile_plugin, guile_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_guile_signal_buffer_closed_cb: callback called when a buffer is
 *                                        closed
 */

int
weechat_guile_signal_buffer_closed_cb (void *data, const char *signal,
                                       const char *type_data,
                                       void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
        plugin_script_remove_buffer_callbacks (guile_scripts, signal_data);

    return WEECHAT_RC_OK;
}

/*
 * weechat_guile_timer_action_cb: timer for executing actions
 */

int
weechat_guile_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &guile_action_install_list)
        {
            plugin_script_action_install (weechat_guile_plugin,
                                          guile_scripts,
                                          &weechat_guile_unload,
                                          &weechat_guile_load,
                                          &guile_quiet,
                                          &guile_action_install_list);
        }
        else if (data == &guile_action_remove_list)
        {
            plugin_script_action_remove (weechat_guile_plugin,
                                         guile_scripts,
                                         &weechat_guile_unload,
                                         &guile_quiet,
                                         &guile_action_remove_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_guile_signal_script_action_cb: callback called when a script action
 *                                        is asked (install/remove a script)
 */

int
weechat_guile_signal_script_action_cb (void *data, const char *signal,
                                       const char *type_data,
                                       void *signal_data)
{
    /* make C compiler happy */
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "guile_script_install") == 0)
        {
            plugin_script_action_add (&guile_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_guile_timer_action_cb,
                                &guile_action_install_list);
        }
        else if (strcmp (signal, "guile_script_remove") == 0)
        {
            plugin_script_action_add (&guile_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_guile_timer_action_cb,
                                &guile_action_remove_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_guile_port_fill_input: fill input
 */

int
weechat_guile_port_fill_input (SCM port)
{
    /* make C compiler happy */
    (void) port;

    return ' ';
}

/*
 * weechat_guile_port_write: write
 */

void
weechat_guile_port_write (SCM port, const void *data, size_t size)
{
    char *new_stdout;
    int length_stdout;

    /* make C compiler happy */
    (void) port;

    /* concatenate str to guile_stdout */
    if (guile_stdout)
    {
        length_stdout = strlen (guile_stdout);
        new_stdout = realloc (guile_stdout, length_stdout + size + 1);
        if (!new_stdout)
        {
            free (guile_stdout);
            return;
        }
        guile_stdout = new_stdout;
        memcpy (guile_stdout + length_stdout, data, size);
        guile_stdout[length_stdout + size] = '\0';
    }
    else
    {
        guile_stdout = malloc (size + 1);
        if (guile_stdout)
        {
            memcpy (guile_stdout, data, size);
            guile_stdout[size] = '\0';
        }
    }

    /* flush stdout if at least "\n" is in string */
    if (guile_stdout && (strchr (guile_stdout, '\n')))
        weechat_guile_stdout_flush ();
}

/*
 * weechat_plugin_init: initialize Guile plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;

    weechat_guile_plugin = plugin;

    guile_stdout = NULL;

    scm_init_guile ();

    guile_module_weechat = scm_c_define_module ("weechat",
                                                &weechat_guile_api_module_init,
                                                NULL);
    scm_c_use_module ("weechat");
    weechat_guile_catch (scm_gc_protect_object, (void *)guile_module_weechat);

    init.callback_command = &weechat_guile_command_cb;
    init.callback_completion = &weechat_guile_completion_cb;
    init.callback_hdata = &weechat_guile_hdata_cb;
    init.callback_infolist = &weechat_guile_infolist_cb;
    init.callback_signal_debug_dump = &weechat_guile_signal_debug_dump_cb;
    init.callback_signal_buffer_closed = &weechat_guile_signal_buffer_closed_cb;
    init.callback_signal_script_action = &weechat_guile_signal_script_action_cb;
    init.callback_load_file = &weechat_guile_load_cb;

    guile_quiet = 1;
    plugin_script_init (weechat_guile_plugin, argc, argv, &init);
    guile_quiet = 0;

    plugin_script_display_short_list (weechat_guile_plugin,
                                      guile_scripts);

    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Guile interface
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* unload all scripts */
    guile_quiet = 1;
    plugin_script_end (plugin, &guile_scripts, &weechat_guile_unload_all);
    guile_quiet = 0;

    /* unprotect module */
    weechat_guile_catch (scm_gc_unprotect_object, (void *)guile_module_weechat);

    /* free some data */
    if (guile_action_install_list)
        free (guile_action_install_list);
    if (guile_action_remove_list)
        free (guile_action_remove_list);

    return WEECHAT_RC_OK;
}
