/*
 * weechat-guile.c - guile (scheme) plugin for WeeChat
 *
 * Copyright (C) 2011-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#undef _

#include <libguile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "weechat-guile.h"
#include "weechat-guile-api.h"


WEECHAT_PLUGIN_NAME(GUILE_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of scheme scripts (with Guile)"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(GUILE_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_guile_plugin = NULL;

struct t_plugin_script_data guile_data;

struct t_config_file *guile_config_file = NULL;
struct t_config_option *guile_config_look_check_license = NULL;
struct t_config_option *guile_config_look_eval_keep_context = NULL;

int guile_quiet = 0;

struct t_plugin_script *guile_script_eval = NULL;
int guile_eval_mode = 0;
int guile_eval_send_input = 0;
int guile_eval_exec_commands = 0;
struct t_gui_buffer *guile_eval_buffer = NULL;
#define GUILE_EVAL_SCRIPT                                               \
    "(weechat:register \"" WEECHAT_SCRIPT_EVAL_NAME "\" \"\" \"1.0\" "  \
    "\"" WEECHAT_LICENSE "\" \"Evaluation of source code\" "            \
    "\"\" \"\")\n"                                                      \
    "\n"                                                                \
    "(define (script_guile_eval code)\n"                                \
    "  (eval-string code)\n"                                            \
    ")\n"

struct t_plugin_script *guile_scripts = NULL;
struct t_plugin_script *last_guile_script = NULL;
struct t_plugin_script *guile_current_script = NULL;
struct t_plugin_script *guile_registered_script = NULL;
const char *guile_current_script_filename = NULL;
SCM guile_module_weechat;
SCM guile_port;
char **guile_buffer_output = NULL;

struct t_guile_function
{
    SCM proc;                          /* proc to call                      */
    SCM *argv;                         /* arguments for proc                */
    size_t nargs;                      /* length of arguments               */
};

/*
 * string used to execute action "install":
 * when signal "guile_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *guile_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "guile_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *guile_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "guile_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *guile_action_autoload_list = NULL;


/*
 * Flushes output.
 */

void
weechat_guile_output_flush (void)
{
    const char *ptr_command;
    char *temp_buffer, *command;

    if (!(*guile_buffer_output)[0])
        return;

    /* if there's no buffer, we catch the output, so there's no flush */
    if (guile_eval_mode && !guile_eval_buffer)
        return;

    temp_buffer = strdup (*guile_buffer_output);
    if (!temp_buffer)
        return;

    weechat_string_dyn_copy (guile_buffer_output, NULL);

    if (guile_eval_mode)
    {
        if (guile_eval_send_input)
        {
            if (guile_eval_exec_commands)
                ptr_command = temp_buffer;
            else
                ptr_command = weechat_string_input_for_buffer (temp_buffer);
            if (ptr_command)
            {
                weechat_command (guile_eval_buffer, temp_buffer);
            }
            else
            {
                if (weechat_asprintf (&command,
                                      "%c%s",
                                      temp_buffer[0],
                                      temp_buffer) >= 0)
                {
                    weechat_command (guile_eval_buffer,
                                     (command[0]) ? command : " ");
                    free (command);
                }
            }
        }
        else
        {
            weechat_printf (guile_eval_buffer, "%s", temp_buffer);
        }
    }
    else
    {
        /* script (no eval mode) */
        weechat_printf (
            NULL,
            weechat_gettext ("%s: stdout/stderr (%s): %s"),
            GUILE_PLUGIN_NAME,
            (guile_current_script) ? guile_current_script->name : "?",
            temp_buffer);
    }

    free (temp_buffer);
}

/*
 * Executes scheme procedure with internal catch and returns value.
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
 * Encapsulates call to scm_call_n (to give arguments).
 */

SCM
weechat_guile_scm_call_n (void *proc)
{
    struct t_guile_function *guile_function;

    guile_function = (struct t_guile_function *)proc;

    return scm_call_n (guile_function->proc,
                       guile_function->argv, guile_function->nargs);
}

/*
 * Executes scheme function (with optional args) and returns value.
 */

SCM
weechat_guile_exec_function (const char *function, SCM *argv, size_t nargs)
{
    SCM func, func2, value;
    struct t_guile_function guile_function;

    func = weechat_guile_catch (scm_c_lookup, (void *)function);
    func2 = weechat_guile_catch (scm_variable_ref, func);

    if (argv)
    {
        guile_function.proc = func2;
        guile_function.argv = argv;
        guile_function.nargs = nargs;
        value = weechat_guile_catch (weechat_guile_scm_call_n, &guile_function);
    }
    else
    {
        value = weechat_guile_catch (scm_call_0, func2);
    }

    return value;
}

/*
 * Callback called for each key/value in a hashtable.
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
 * Converts a WeeChat hashtable to a guile alist.
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
 * Converts a guile alist to a WeeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
weechat_guile_alist_to_hashtable (SCM alist, int size, const char *type_keys,
                                  const char *type_values)
{
    struct t_hashtable *hashtable;
    int length, i;
    SCM pair;
    char *str, *str2;

    hashtable = weechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    length = scm_to_int (scm_length (alist));
    for (i = 0; i < length; i++)
    {
        pair = scm_list_ref (alist, scm_from_int (i));
        if (strcmp (type_values, WEECHAT_HASHTABLE_STRING) == 0)
        {
            str = scm_to_locale_string (scm_list_ref (pair, scm_from_int (0)));
            str2 = scm_to_locale_string (scm_list_ref (pair, scm_from_int (1)));
            weechat_hashtable_set (hashtable, str, str2);
            free (str);
            free (str2);
        }
        else if (strcmp (type_values, WEECHAT_HASHTABLE_POINTER) == 0)
        {
            str = scm_to_locale_string (scm_list_ref (pair, scm_from_int (0)));
            str2 = scm_to_locale_string (scm_list_ref (pair, scm_from_int (1)));
            weechat_hashtable_set (hashtable, str,
                                   plugin_script_str2ptr (weechat_guile_plugin,
                                                          NULL, NULL, str2));
            free (str);
            free (str2);
        }
    }

    return hashtable;
}

/*
 * Executes a guile function.
 */

void *
weechat_guile_exec (struct t_plugin_script *script,
                    int ret_type, const char *function,
                    char *format, void **argv)
{
    struct t_plugin_script *old_guile_current_script;
    SCM rc, old_current_module;
    void *argv2[17], *ret_value, *ret_temp;
    int i, argc, *ret_int;

    ret_value = NULL;

    old_guile_current_script = guile_current_script;
    old_current_module = NULL;
    if (script->interpreter)
    {
        old_current_module = scm_current_module ();
        scm_set_current_module ((SCM)(script->interpreter));
    }
    guile_current_script = script;

    if (argv && argv[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string or null */
                    if (argv[i])
                        argv2[i] = scm_from_locale_string ((char *)argv[i]);
                    else
                        argv2[i] = SCM_ELISP_NIL;
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
        rc = weechat_guile_exec_function (function, (SCM *)argv2, argc);
    }
    else
    {
        rc = weechat_guile_exec_function (function, NULL, 0);
    }

    weechat_guile_output_flush ();

    if ((ret_type == WEECHAT_SCRIPT_EXEC_STRING) && (scm_is_string (rc)))
    {
        ret_value = scm_to_locale_string (rc);
    }
    else if ((ret_type == WEECHAT_SCRIPT_EXEC_POINTER) && (scm_is_string (rc)))
    {
        ret_temp = scm_to_locale_string (rc);
        if (ret_temp)
        {
            ret_value = plugin_script_str2ptr (weechat_guile_plugin,
                                               script->name, function,
                                               ret_temp);
            free (ret_temp);
        }
        else
        {
            ret_value = NULL;
        }
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
        if (ret_type != WEECHAT_SCRIPT_EXEC_IGNORE)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: function \"%s\" must "
                                             "return a valid value"),
                            weechat_prefix ("error"), GUILE_PLUGIN_NAME,
                            function);
        }
    }

    if ((ret_type != WEECHAT_SCRIPT_EXEC_IGNORE) && !ret_value)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error in function \"%s\""),
                        weechat_prefix ("error"), GUILE_PLUGIN_NAME, function);
    }

    if (old_current_module)
        scm_set_current_module (old_current_module);

    guile_current_script = old_guile_current_script;

    return ret_value;
}

/*
 * Initializes guile module for a script file.
 */

void
weechat_guile_module_init_file (void *filename)
{
    SCM rc;

    weechat_guile_catch (scm_c_eval_string, "(use-modules (weechat))");
    rc = weechat_guile_catch (scm_c_primitive_load, filename);

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
 * Initializes guile module for a string with guile code.
 */

void
weechat_guile_module_init_code (void *code)
{
    SCM rc;

    weechat_guile_catch (scm_c_eval_string, "(use-modules (weechat))");
    rc = weechat_guile_catch (scm_c_eval_string, code);

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
 * Loads a guile script.
 *
 * If code is NULL, the content of filename is read and executed.
 * If code is not NULL, it is executed (the file is not read).
 *
 * Returns pointer to new registered script, NULL if error.
 */

struct t_plugin_script *
weechat_guile_load (const char *filename, const char *code)
{
    char *filename2, *ptr_base_name, *base_name;
    SCM module;
    struct stat buf;

    if (!code)
    {
        if (stat (filename, &buf) != 0)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: script \"%s\" not found"),
                            weechat_prefix ("error"), GUILE_PLUGIN_NAME,
                            filename);
            return NULL;
        }
    }

    if ((weechat_guile_plugin->debug >= 2) || !guile_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        GUILE_PLUGIN_NAME, filename);
    }

    guile_current_script = NULL;
    guile_registered_script = NULL;
    guile_current_script_filename = filename;

    if (code)
    {
        module = scm_c_define_module (filename,
                                      &weechat_guile_module_init_code,
                                      (char *)code);
    }
    else
    {
        filename2 = strdup (filename);
        if (!filename2)
            return NULL;
        ptr_base_name = basename (filename2);
        base_name = strdup (ptr_base_name);
        module = scm_c_define_module (base_name,
                                      &weechat_guile_module_init_file,
                                      filename2);
        free (filename2);
    }

    if (!guile_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), GUILE_PLUGIN_NAME, filename);
        return NULL;
    }

    weechat_guile_catch (scm_gc_protect_object, (void *)module);

    guile_current_script = guile_registered_script;

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (weechat_guile_plugin,
                                        guile_scripts,
                                        guile_current_script,
                                        &weechat_guile_api_buffer_input_data_cb,
                                        &weechat_guile_api_buffer_close_cb);

    (void) weechat_hook_signal_send ("guile_script_loaded",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     guile_current_script->filename);

    return guile_current_script;
}

/*
 * Callback for script_auto_load() function.
 */

void
weechat_guile_load_cb (void *data, const char *filename)
{
    const char *pos_dot;

    /* make C compiler happy */
    (void) data;

    pos_dot = strrchr (filename, '.');
    if (pos_dot && (strcmp (pos_dot, ".scm") == 0))
        weechat_guile_load (filename, NULL);
}

/*
 * Unloads a guile script.
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

    if (guile_current_script)
        scm_set_current_module ((SCM)(guile_current_script->interpreter));

    (void) weechat_hook_signal_send ("guile_script_unloaded",
                                     WEECHAT_HOOK_SIGNAL_STRING, filename);
    free (filename);
}

/*
 * Unloads a guile script by name.
 */

void
weechat_guile_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (guile_scripts, name);
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
 * Unloads all guile scripts.
 */

void
weechat_guile_unload_all (void)
{
    while (guile_scripts)
    {
        weechat_guile_unload (guile_scripts);
    }
}

/*
 * Reloads a guile script by name.
 */

void
weechat_guile_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (guile_scripts, name);
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
            weechat_guile_load (filename, NULL);
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
 * Evaluates guile source code.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weechat_guile_eval (struct t_gui_buffer *buffer, int send_to_buffer_as_input,
                    int exec_commands, const char *code)
{
    void *func_argv[1], *result;
    int old_guile_quiet;

    if (!guile_script_eval)
    {
        old_guile_quiet = guile_quiet;
        guile_quiet = 1;
        guile_script_eval = weechat_guile_load (WEECHAT_SCRIPT_EVAL_NAME,
                                                GUILE_EVAL_SCRIPT);
        guile_quiet = old_guile_quiet;
        if (!guile_script_eval)
            return 0;
    }

    weechat_guile_output_flush ();

    guile_eval_mode = 1;
    guile_eval_send_input = send_to_buffer_as_input;
    guile_eval_exec_commands = exec_commands;
    guile_eval_buffer = buffer;

    func_argv[0] = (char *)code;
    result = weechat_guile_exec (guile_script_eval,
                                  WEECHAT_SCRIPT_EXEC_IGNORE,
                                  "script_guile_eval",
                                  "s", func_argv);
    /* result is ignored */
    free (result);

    weechat_guile_output_flush ();

    guile_eval_mode = 0;
    guile_eval_send_input = 0;
    guile_eval_exec_commands = 0;
    guile_eval_buffer = NULL;

    if (!weechat_config_boolean (guile_config_look_eval_keep_context))
    {
        old_guile_quiet = guile_quiet;
        guile_quiet = 1;
        weechat_guile_unload (guile_script_eval);
        guile_quiet = old_guile_quiet;
        guile_script_eval = NULL;
    }

    return 1;
}

/*
 * Callback for command "/guile".
 */

int
weechat_guile_command_cb (const void *pointer, void *data,
                          struct t_gui_buffer *buffer,
                          int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *ptr_code, *path_script;
    int i, send_to_buffer_as_input, exec_commands, old_guile_quiet;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_guile_plugin, guile_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_guile_plugin, guile_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_guile_plugin, guile_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_guile_plugin, &weechat_guile_load_cb);
        }
        else if (weechat_strcmp (argv[1], "reload") == 0)
        {
            weechat_guile_unload_all ();
            plugin_script_auto_load (weechat_guile_plugin, &weechat_guile_load_cb);
        }
        else if (weechat_strcmp (argv[1], "unload") == 0)
        {
            weechat_guile_unload_all ();
        }
        else if (weechat_strcmp (argv[1], "version") == 0)
        {
            plugin_script_display_interpreter (weechat_guile_plugin, 0);
        }
        else
            WEECHAT_COMMAND_ERROR;
    }
    else
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_guile_plugin, guile_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_guile_plugin, guile_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcmp (argv[1], "load") == 0)
                 || (weechat_strcmp (argv[1], "reload") == 0)
                 || (weechat_strcmp (argv[1], "unload") == 0))
        {
            old_guile_quiet = guile_quiet;
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
            if (weechat_strcmp (argv[1], "load") == 0)
            {
                /* load guile script */
                path_script = plugin_script_search_path (weechat_guile_plugin,
                                                         ptr_name, 1);
                weechat_guile_load ((path_script) ? path_script : ptr_name,
                                    NULL);
                free (path_script);
            }
            else if (weechat_strcmp (argv[1], "reload") == 0)
            {
                /* reload one guile script */
                weechat_guile_reload_name (ptr_name);
            }
            else if (weechat_strcmp (argv[1], "unload") == 0)
            {
                /* unload guile script */
                weechat_guile_unload_name (ptr_name);
            }
            guile_quiet = old_guile_quiet;
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
            if (!weechat_guile_eval (buffer, send_to_buffer_as_input,
                                     exec_commands, ptr_code))
                WEECHAT_COMMAND_ERROR;
        }
        else
            WEECHAT_COMMAND_ERROR;
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds guile scripts to completion list.
 */

int
weechat_guile_completion_cb (const void *pointer, void *data,
                             const char *completion_item,
                             struct t_gui_buffer *buffer,
                             struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_guile_plugin, completion, guile_scripts);

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for guile scripts.
 */

struct t_hdata *
weechat_guile_hdata_cb (const void *pointer, void *data,
                        const char *hdata_name)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &guile_scripts, &last_guile_script,
                                       hdata_name);
}

/*
 * Returns guile info "guile_eval".
 */

char *
weechat_guile_info_eval_cb (const void *pointer, void *data,
                            const char *info_name,
                            const char *arguments)
{
    char *output;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    weechat_guile_eval (NULL, 0, 0, (arguments) ? arguments : "");
    output = strdup (*guile_buffer_output);
    weechat_string_dyn_copy (guile_buffer_output, NULL);

    return output;
}

/*
 * Returns infolist with guile scripts.
 */

struct t_infolist *
weechat_guile_infolist_cb (const void *pointer, void *data,
                           const char *infolist_name,
                           void *obj_pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (strcmp (infolist_name, "guile_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_guile_plugin,
                                                    guile_scripts, obj_pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps guile plugin data in WeeChat log file.
 */

int
weechat_guile_signal_debug_dump_cb (const void *pointer, void *data,
                                    const char *signal,
                                    const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, GUILE_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_guile_plugin, guile_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
weechat_guile_timer_action_cb (const void *pointer, void *data,
                               int remaining_calls)
{
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    if (pointer)
    {
        if (pointer == &guile_action_install_list)
        {
            plugin_script_action_install (weechat_guile_plugin,
                                          guile_scripts,
                                          &weechat_guile_unload,
                                          &weechat_guile_load,
                                          &guile_quiet,
                                          &guile_action_install_list);
        }
        else if (pointer == &guile_action_remove_list)
        {
            plugin_script_action_remove (weechat_guile_plugin,
                                         guile_scripts,
                                         &weechat_guile_unload,
                                         &guile_quiet,
                                         &guile_action_remove_list);
        }
        else if (pointer == &guile_action_autoload_list)
        {
            plugin_script_action_autoload (weechat_guile_plugin,
                                           &guile_quiet,
                                           &guile_action_autoload_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove a script).
 */

int
weechat_guile_signal_script_action_cb (const void *pointer, void *data,
                                       const char *signal,
                                       const char *type_data,
                                       void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "guile_script_install") == 0)
        {
            plugin_script_action_add (&guile_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_guile_timer_action_cb,
                                &guile_action_install_list, NULL);
        }
        else if (strcmp (signal, "guile_script_remove") == 0)
        {
            plugin_script_action_add (&guile_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_guile_timer_action_cb,
                                &guile_action_remove_list, NULL);
        }
        else if (strcmp (signal, "guile_script_autoload") == 0)
        {
            plugin_script_action_add (&guile_action_autoload_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_guile_timer_action_cb,
                                &guile_action_autoload_list, NULL);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Fills input.
 */

#if SCM_MAJOR_VERSION >= 3 || (SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION >= 2)
/* Guile >= 2.2 */
size_t
weechat_guile_port_fill_input (SCM port, SCM dst, size_t start, size_t count)
{
    /* make C compiler happy */
    (void) port;

    memset (SCM_BYTEVECTOR_CONTENTS (dst) + start, ' ', count);

    return count;
}
#else
/* Guile < 2.2 */
int
weechat_guile_port_fill_input (SCM port)
{
    /* make C compiler happy */
    (void) port;

    return ' ';
}
#endif

/*
 * Write.
 */

#if SCM_MAJOR_VERSION >= 3 || (SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION >= 2)
/* Guile >= 2.2 */
size_t
weechat_guile_port_write (SCM port, SCM src, size_t start, size_t count)
{
    char *data2, *ptr_data, *ptr_newline;
    const signed char *data;

    /* make C compiler happy */
    (void) port;

    data = SCM_BYTEVECTOR_CONTENTS (src);

    data2 = malloc (count + 1);
    if (!data2)
        return 0;

    memcpy (data2, data + start, count);
    data2[count] = '\0';

    ptr_data = data2;
    while ((ptr_newline = strchr (ptr_data, '\n')) != NULL)
    {
        weechat_string_dyn_concat (guile_buffer_output,
                                   ptr_data,
                                   ptr_newline - ptr_data);
        weechat_guile_output_flush ();
        ptr_data = ++ptr_newline;
    }
    weechat_string_dyn_concat (guile_buffer_output, ptr_data, -1);

    free (data2);

    return count;
}
#else
/* Guile < 2.2 */
void
weechat_guile_port_write (SCM port, const void *data, size_t size)
{
    char *data2, *ptr_data, *ptr_newline;

    /* make C compiler happy */
    (void) port;

    data2 = malloc (size + 1);
    if (!data2)
        return;

    memcpy (data2, data, size);
    data2[size] = '\0';

    ptr_data = data2;
    while ((ptr_newline = strchr (ptr_data, '\n')) != NULL)
    {
        weechat_string_dyn_concat (guile_buffer_output,
                                   ptr_data,
                                   ptr_newline - ptr_data);
        weechat_guile_output_flush ();
        ptr_data = ++ptr_newline;
    }
    weechat_string_dyn_concat (guile_buffer_output, ptr_data, -1);

    free (data2);
}
#endif

/*
 * Callback called by scm_with_guile().
 */

void *
weechat_guile_init (void *data)
{
    /* make C compiler happy */
    (void) data;

    return NULL;
}

/*
 * Initializes guile plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    char str_version[128];
    int old_guile_quiet;

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_guile_plugin = plugin;

    guile_quiet = 0;
    guile_eval_mode = 0;
    guile_eval_send_input = 0;
    guile_eval_exec_commands = 0;

    /* set interpreter name and version */
    weechat_hashtable_set (plugin->variables, "interpreter_name",
                           plugin->name);
#if defined(SCM_MAJOR_VERSION) && defined(SCM_MINOR_VERSION) && defined(SCM_MICRO_VERSION)
    snprintf (str_version, sizeof (str_version),
              "%d.%d.%d",
              SCM_MAJOR_VERSION,
              SCM_MINOR_VERSION,
              SCM_MICRO_VERSION);
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           str_version);
#else
    (void) str_version;
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           "");
#endif /* defined(SCM_MAJOR_VERSION) && defined(SCM_MINOR_VERSION) && defined(SCM_MICRO_VERSION) */

    /* init stdout/stderr buffer */
    guile_buffer_output = weechat_string_dyn_alloc (256);
    if (!guile_buffer_output)
        return WEECHAT_RC_ERROR;

#if defined(HAVE_GUILE_GMP_MEMORY_FUNCTIONS) && (SCM_MAJOR_VERSION < 3 || (SCM_MAJOR_VERSION == 3 && SCM_MINOR_VERSION == 0 && SCM_MICRO_VERSION < 8))
    /*
     * prevent guile to use its own gmp allocator, because it can conflict
     * with other plugins using GnuTLS like relay, which can crash WeeChat
     * on unload (or exit); this is not needed anymore with Guile ≥ 3.0.8
     */
    scm_install_gmp_memory_functions = 0;
#endif /* defined(HAVE_GUILE_GMP_MEMORY_FUNCTIONS) && (SCM_MAJOR_VERSION < 3 || (SCM_MAJOR_VERSION == 3 && SCM_MINOR_VERSION == 0 && SCM_MICRO_VERSION < 8)) */

#if defined(__MACH__) || SCM_MAJOR_VERSION < 3
    /*
     * on GNU/Hurd or if using Guile < 3, use scm_with_guile() instead of
     * scm_init_guile() to prevent crash on exit
     */
    scm_with_guile (&weechat_guile_init, NULL);
#else
    /* any other OS (not GNU/Hurd) or Guile >= 3.x */
    scm_init_guile ();
#endif

    guile_module_weechat = scm_c_define_module ("weechat",
                                                &weechat_guile_api_module_init,
                                                NULL);
    scm_c_use_module ("weechat");
    weechat_guile_catch (scm_gc_protect_object, (void *)guile_module_weechat);

    guile_data.config_file = &guile_config_file;
    guile_data.config_look_check_license = &guile_config_look_check_license;
    guile_data.config_look_eval_keep_context = &guile_config_look_eval_keep_context;
    guile_data.scripts = &guile_scripts;
    guile_data.last_script = &last_guile_script;
    guile_data.callback_command = &weechat_guile_command_cb;
    guile_data.callback_completion = &weechat_guile_completion_cb;
    guile_data.callback_hdata = &weechat_guile_hdata_cb;
    guile_data.callback_info_eval = &weechat_guile_info_eval_cb;
    guile_data.callback_infolist = &weechat_guile_infolist_cb;
    guile_data.callback_signal_debug_dump = &weechat_guile_signal_debug_dump_cb;
    guile_data.callback_signal_script_action = &weechat_guile_signal_script_action_cb;
    guile_data.callback_load_file = &weechat_guile_load_cb;
    guile_data.init_before_autoload = NULL;
    guile_data.unload_all = &weechat_guile_unload_all;

    old_guile_quiet = guile_quiet;
    guile_quiet = 1;
    plugin_script_init (weechat_guile_plugin, &guile_data);
    guile_quiet = old_guile_quiet;

    plugin_script_display_short_list (weechat_guile_plugin,
                                      guile_scripts);

    /* init OK */
    return WEECHAT_RC_OK;
}

/*
 * Ends guile plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    int old_guile_quiet;

    /* unload all scripts */
    old_guile_quiet = guile_quiet;
    guile_quiet = 1;
    if (guile_script_eval)
    {
        weechat_guile_unload (guile_script_eval);
        guile_script_eval = NULL;
    }
    plugin_script_end (plugin, &guile_data);
    guile_quiet = old_guile_quiet;

    /* unprotect module */
    weechat_guile_catch (scm_gc_unprotect_object, (void *)guile_module_weechat);

    /* free some data */
    free (guile_action_install_list);
    guile_action_install_list = NULL;
    free (guile_action_remove_list);
    guile_action_remove_list = NULL;
    free (guile_action_autoload_list);
    guile_action_autoload_list = NULL;
    weechat_string_dyn_free (guile_buffer_output, 1);
    guile_buffer_output = NULL;

    return WEECHAT_RC_OK;
}
