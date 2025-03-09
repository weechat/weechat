/*
 * weechat-tcl.c - tcl plugin for WeeChat
 *
 * Copyright (C) 2008-2010 Dmitry Kobylin <fnfal@academ.tsc.ru>
 * Copyright (C) 2008-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include <tcl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "weechat-tcl.h"
#include "weechat-tcl-api.h"


WEECHAT_PLUGIN_NAME(TCL_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of tcl scripts"));
WEECHAT_PLUGIN_AUTHOR("Dmitry Kobylin <fnfal@academ.tsc.ru>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(TCL_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_tcl_plugin = NULL;

struct t_plugin_script_data tcl_data;

struct t_config_file *tcl_config_file = NULL;
struct t_config_option *tcl_config_look_check_license = NULL;
struct t_config_option *tcl_config_look_eval_keep_context = NULL;

int tcl_quiet = 0;

struct t_plugin_script *tcl_script_eval = NULL;
int tcl_eval_mode = 0;
int tcl_eval_send_input = 0;
int tcl_eval_exec_commands = 0;

struct t_plugin_script *tcl_scripts = NULL;
struct t_plugin_script *last_tcl_script = NULL;
struct t_plugin_script *tcl_current_script = NULL;
struct t_plugin_script *tcl_registered_script = NULL;
const char *tcl_current_script_filename = NULL;

/*
 * string used to execute action "install":
 * when signal "tcl_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *tcl_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "tcl_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *tcl_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "tcl_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *tcl_action_autoload_list = NULL;

Tcl_Interp* cinterp;


/*
 * Callback called for each key/value in a hashtable.
 */

void
weechat_tcl_hashtable_map_cb (void *data,
                              struct t_hashtable *hashtable,
                              const char *key,
                              const char *value)
{
    void **data_array;
    Tcl_Interp *interp;
    Tcl_Obj *dict;

    /* make C compiler happy */
    (void) hashtable;

    data_array = (void **)data;
    interp = data_array[0];
    dict = data_array[1];

    Tcl_DictObjPut (interp, dict,
                    Tcl_NewStringObj (key, -1),
                    Tcl_NewStringObj (value, -1));
}

/*
 * Converts a WeeChat hashtable to a tcl dict.
 */

Tcl_Obj *
weechat_tcl_hashtable_to_dict (Tcl_Interp *interp,
                               struct t_hashtable *hashtable)
{
    Tcl_Obj *dict;
    void *data[2];

    dict = Tcl_NewDictObj ();
    if (!dict)
        return NULL;

    data[0] = interp;
    data[1] = dict;

    weechat_hashtable_map_string (hashtable, &weechat_tcl_hashtable_map_cb,
                                  data);

    return dict;
}

/*
 * Converts a tcl dict to a WeeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
weechat_tcl_dict_to_hashtable (Tcl_Interp *interp, Tcl_Obj *dict,
                               int size, const char *type_keys,
                               const char *type_values)
{
    struct t_hashtable *hashtable;
    Tcl_DictSearch search;
    Tcl_Obj *key, *value;
    int done;

    hashtable = weechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    if (Tcl_DictObjFirst (interp, dict, &search, &key, &value, &done) == TCL_OK)
    {
        for (; !done ; Tcl_DictObjNext (&search, &key, &value, &done))
        {
            if (strcmp (type_values, WEECHAT_HASHTABLE_STRING) == 0)
            {
                weechat_hashtable_set (hashtable,
                                       Tcl_GetString (key),
                                       Tcl_GetString (value));
            }
            else if (strcmp (type_values, WEECHAT_HASHTABLE_POINTER) == 0)
            {
                weechat_hashtable_set (hashtable,
                                       Tcl_GetString (key),
                                       plugin_script_str2ptr (
                                           weechat_tcl_plugin,
                                           NULL, NULL,
                                           Tcl_GetString (value)));
            }
        }
    }

    Tcl_DictObjDone (&search);

    return hashtable;
}

/*
 * Executes a tcl function.
 */

void *
weechat_tcl_exec (struct t_plugin_script *script,
                  int ret_type, const char *function,
                  const char *format, void **argv)
{
    int argc, i;
    int *ret_i;
    char *ret_cv;
    void *ret_val;
    Tcl_Obj *cmdlist;
    Tcl_Interp *interp;
    struct t_plugin_script *old_tcl_script;

    old_tcl_script = tcl_current_script;
    tcl_current_script = script;
    interp = (Tcl_Interp*)script->interpreter;

    if (function && function[0])
    {
        cmdlist = Tcl_NewListObj (0, NULL);
        Tcl_IncrRefCount (cmdlist); /* +1 */
        Tcl_ListObjAppendElement (interp, cmdlist, Tcl_NewStringObj (function,-1));
    }
    else
    {
        tcl_current_script = old_tcl_script;
        return NULL;
    }

    if (format && format[0])
    {
        argc = strlen (format);
        for (i = 0; i < argc; i++)
        {
            switch (format[i])
            {
                case 's': /* string */
                    Tcl_ListObjAppendElement (interp, cmdlist,
                                              Tcl_NewStringObj (argv[i], -1));
                    break;
                case 'i': /* integer */
                    Tcl_ListObjAppendElement (interp, cmdlist,
                                              Tcl_NewIntObj (*((int *)argv[i])));
                    break;
                case 'h': /* hash */
                    Tcl_ListObjAppendElement (interp, cmdlist,
                                              weechat_tcl_hashtable_to_dict (interp, argv[i]));
                    break;
            }
        }
    }

    if (Tcl_EvalObjEx (interp, cmdlist, TCL_EVAL_DIRECT) == TCL_OK)
    {
        Tcl_DecrRefCount (cmdlist);  /* -1 */
        ret_val = NULL;
        if (ret_type == WEECHAT_SCRIPT_EXEC_STRING)
        {
            ret_cv = Tcl_GetString (Tcl_GetObjResult (interp));
            if (ret_cv)
                ret_val = (void *)strdup (ret_cv);
            else
                ret_val = NULL;
        }
        else if (ret_type == WEECHAT_SCRIPT_EXEC_POINTER)
        {
            ret_cv = Tcl_GetString (Tcl_GetObjResult (interp));
            if (ret_cv)
            {
                ret_val = plugin_script_str2ptr (weechat_tcl_plugin,
                                                 script->name, function,
                                                 ret_cv);
            }
            else
                ret_val = NULL;
        }
        else if ( ret_type == WEECHAT_SCRIPT_EXEC_INT
                  && Tcl_GetIntFromObj (interp, Tcl_GetObjResult (interp), &i) == TCL_OK)
        {
            ret_i = (int *)malloc (sizeof (*ret_i));
            if (ret_i)
                *ret_i = i;
            ret_val = (void *)ret_i;
        }
        else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
        {
            ret_val = weechat_tcl_dict_to_hashtable (interp,
                                                     Tcl_GetObjResult (interp),
                                                     WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     WEECHAT_HASHTABLE_STRING);
        }

        tcl_current_script = old_tcl_script;
        if (ret_val)
            return ret_val;

        if (ret_type != WEECHAT_SCRIPT_EXEC_IGNORE)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: function \"%s\" must "
                                             "return a valid value"),
                            weechat_prefix ("error"), TCL_PLUGIN_NAME,
                            function);
        }

        return NULL;
    }

    Tcl_DecrRefCount (cmdlist);  /* -1 */
    weechat_printf (NULL,
                    weechat_gettext ("%s%s: unable to run function \"%s\": %s"),
                    weechat_prefix ("error"), TCL_PLUGIN_NAME, function,
                    Tcl_GetString (Tcl_GetObjResult (interp)));
    tcl_current_script = old_tcl_script;

    return NULL;
}

/*
 * Loads a tcl script.
 *
 * If code is NULL, the content of filename is read and executed.
 * If code is not NULL, it is executed (the file is not read).
 *
 * Returns pointer to new registered script, NULL if error.
 */

struct t_plugin_script *
weechat_tcl_load (const char *filename, const char *code)
{
    Tcl_Interp *interp;
    struct stat buf;

    /* make C compiler happy */
    /* TODO: implement load of code in TCL */
    (void) code;

    if (stat (filename, &buf) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, filename);
        return NULL;
    }

    if ((weechat_tcl_plugin->debug >= 2) || !tcl_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        TCL_PLUGIN_NAME, filename);
    }

    tcl_current_script = NULL;
    tcl_registered_script = NULL;

    if (!(interp = Tcl_CreateInterp ())) {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "interpreter"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME);
        return NULL;
    }
    tcl_current_script_filename = filename;

    weechat_tcl_api_init (interp);

    if (Tcl_EvalFile (interp, filename) != TCL_OK)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error occurred while "
                                         "parsing file \"%s\": %s"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, filename,
                        Tcl_GetString (Tcl_GetObjResult (interp)));

        /* if script was registered, remove it from list */
        if (tcl_current_script)
        {
            plugin_script_remove (weechat_tcl_plugin,
                                  &tcl_scripts, &last_tcl_script,
                                  tcl_current_script);
            tcl_current_script = NULL;
        }

        return NULL;
    }

    if (!tcl_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, filename);
        Tcl_DeleteInterp (interp);
        return NULL;
    }
    tcl_current_script = tcl_registered_script;

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (weechat_tcl_plugin,
                                        tcl_scripts,
                                        tcl_current_script,
                                        &weechat_tcl_api_buffer_input_data_cb,
                                        &weechat_tcl_api_buffer_close_cb);

    (void) weechat_hook_signal_send ("tcl_script_loaded",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     tcl_current_script->filename);

    return tcl_current_script;
}

/*
 * Callback for weechat_script_auto_load() function.
 */

void
weechat_tcl_load_cb (void *data, const char *filename)
{
    const char *pos_dot;

    /* make C compiler happy */
    (void) data;

    pos_dot = strrchr (filename, '.');
    if (pos_dot && (strcmp (pos_dot, ".tcl") == 0))
        weechat_tcl_load (filename, NULL);
}

/*
 * Unloads a tcl script.
 */

void
weechat_tcl_unload (struct t_plugin_script *script)
{
    Tcl_Interp* interp;
    int *rc;
    char *filename;

    if ((weechat_tcl_plugin->debug >= 2) || !tcl_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        TCL_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *)weechat_tcl_exec (script,
                                      WEECHAT_SCRIPT_EXEC_INT,
                                      script->shutdown_func,
                                      NULL, NULL);
        free (rc);
    }

    filename = strdup (script->filename);
    interp = (Tcl_Interp*)script->interpreter;

    if (tcl_current_script == script)
        tcl_current_script = (tcl_current_script->prev_script) ?
            tcl_current_script->prev_script : tcl_current_script->next_script;

    plugin_script_remove (weechat_tcl_plugin, &tcl_scripts, &last_tcl_script, script);

    Tcl_DeleteInterp (interp);

    (void) weechat_hook_signal_send ("tcl_script_unloaded",
                                     WEECHAT_HOOK_SIGNAL_STRING, filename);
    free (filename);
}

/*
 * Unloads a tcl script by name.
 */

void
weechat_tcl_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (tcl_scripts, name);
    if (ptr_script)
    {
        weechat_tcl_unload (ptr_script);
        if (!tcl_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            TCL_PLUGIN_NAME, name);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all tcl scripts.
 */

void
weechat_tcl_unload_all (void)
{
    while (tcl_scripts)
    {
        weechat_tcl_unload (tcl_scripts);
    }
}

/*
 * Reloads a tcl script by name.
 */

void
weechat_tcl_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (tcl_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_tcl_unload (ptr_script);
            if (!tcl_quiet)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s: script \"%s\" unloaded"),
                                TCL_PLUGIN_NAME, name);
            }
            weechat_tcl_load (filename, NULL);
            free (filename);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, name);
    }
}

/*
 * Evaluates TCL source code.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weechat_tcl_eval (struct t_gui_buffer *buffer, int send_to_buffer_as_input,
                  int exec_commands, const char *code)
{
    /* TODO: implement tcl eval */
    (void) buffer;
    (void) send_to_buffer_as_input;
    (void) exec_commands;
    (void) code;

    return 1;
}

/*
 * Callback for command "/tcl".
 */

int
weechat_tcl_command_cb (const void *pointer, void *data,
                        struct t_gui_buffer *buffer,
                         int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *ptr_code, *path_script;
    int i, send_to_buffer_as_input, exec_commands, old_tcl_quiet;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_tcl_plugin, tcl_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_tcl_plugin, tcl_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_tcl_plugin, tcl_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_tcl_plugin, &weechat_tcl_load_cb);
        }
        else if (weechat_strcmp (argv[1], "reload") == 0)
        {
            weechat_tcl_unload_all ();
            plugin_script_auto_load (weechat_tcl_plugin, &weechat_tcl_load_cb);
        }
        else if (weechat_strcmp (argv[1], "unload") == 0)
        {
            weechat_tcl_unload_all ();
        }
        else if (weechat_strcmp (argv[1], "version") == 0)
        {
            plugin_script_display_interpreter (weechat_tcl_plugin, 0);
        }
        else
            WEECHAT_COMMAND_ERROR;
    }
    else
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_tcl_plugin, tcl_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_tcl_plugin, tcl_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcmp (argv[1], "load") == 0)
                 || (weechat_strcmp (argv[1], "reload") == 0)
                 || (weechat_strcmp (argv[1], "unload") == 0))
        {
            old_tcl_quiet = tcl_quiet;
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                tcl_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (weechat_strcmp (argv[1], "load") == 0)
            {
                /* load tcl script */
                path_script = plugin_script_search_path (weechat_tcl_plugin,
                                                         ptr_name, 1);
                weechat_tcl_load ((path_script) ? path_script : ptr_name,
                                  NULL);
                free (path_script);
            }
            else if (weechat_strcmp (argv[1], "reload") == 0)
            {
                /* reload one tcl script */
                weechat_tcl_reload_name (ptr_name);
            }
            else if (weechat_strcmp (argv[1], "unload") == 0)
            {
                /* unload tcl script */
                weechat_tcl_unload_name (ptr_name);
            }
            tcl_quiet = old_tcl_quiet;
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
            if (!weechat_tcl_eval (buffer, send_to_buffer_as_input,
                                   exec_commands, ptr_code))
                WEECHAT_COMMAND_ERROR;
            /* TODO: implement /tcl eval */
            weechat_printf (NULL,
                            _("%sCommand \"/%s eval\" is not yet implemented"),
                            weechat_prefix ("error"),
                            weechat_tcl_plugin->name);
        }
        else
            WEECHAT_COMMAND_ERROR;
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds tcl scripts to completion list.
 */

int
weechat_tcl_completion_cb (const void *pointer, void *data,
                           const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_tcl_plugin, completion, tcl_scripts);

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for tcl scripts.
 */

struct t_hdata *
weechat_tcl_hdata_cb (const void *pointer, void *data,
                      const char *hdata_name)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &tcl_scripts, &last_tcl_script,
                                       hdata_name);
}

/*
 * Returns tcl info "tcl_eval".
 */

char *
weechat_tcl_info_eval_cb (const void *pointer, void *data,
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
 * Returns infolist with tcl scripts.
 */

struct t_infolist *
weechat_tcl_infolist_cb (const void *pointer, void *data,
                         const char *infolist_name,
                         void *obj_pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (strcmp (infolist_name, "tcl_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_tcl_plugin,
                                                    tcl_scripts, obj_pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * Dumps tcl plugin data in WeeChat log file.
 */

int
weechat_tcl_signal_debug_dump_cb (const void *pointer, void *data,
                                  const char *signal,
                                  const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, TCL_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_tcl_plugin, tcl_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
weechat_tcl_timer_action_cb (const void *pointer, void *data,
                             int remaining_calls)
{
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    if (pointer)
    {
        if (pointer == &tcl_action_install_list)
        {
            plugin_script_action_install (weechat_tcl_plugin,
                                          tcl_scripts,
                                          &weechat_tcl_unload,
                                          &weechat_tcl_load,
                                          &tcl_quiet,
                                          &tcl_action_install_list);
        }
        else if (pointer == &tcl_action_remove_list)
        {
            plugin_script_action_remove (weechat_tcl_plugin,
                                         tcl_scripts,
                                         &weechat_tcl_unload,
                                         &tcl_quiet,
                                         &tcl_action_remove_list);
        }
        else if (pointer == &tcl_action_autoload_list)
        {
            plugin_script_action_autoload (weechat_tcl_plugin,
                                           &tcl_quiet,
                                           &tcl_action_autoload_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove a script).
 */

int
weechat_tcl_signal_script_action_cb (const void *pointer, void *data,
                                     const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "tcl_script_install") == 0)
        {
            plugin_script_action_add (&tcl_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_tcl_timer_action_cb,
                                &tcl_action_install_list, NULL);
        }
        else if (strcmp (signal, "tcl_script_remove") == 0)
        {
            plugin_script_action_add (&tcl_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_tcl_timer_action_cb,
                                &tcl_action_remove_list, NULL);
        }
        else if (strcmp (signal, "tcl_script_autoload") == 0)
        {
            plugin_script_action_add (&tcl_action_autoload_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_tcl_timer_action_cb,
                                &tcl_action_autoload_list, NULL);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes tcl plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int old_tcl_quiet;

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_tcl_plugin = plugin;

    tcl_quiet = 0;
    tcl_eval_mode = 0;
    tcl_eval_send_input = 0;
    tcl_eval_exec_commands = 0;

    /* set interpreter name and version */
    weechat_hashtable_set (plugin->variables, "interpreter_name",
                           plugin->name);
#ifdef TCL_VERSION
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           TCL_VERSION);
#else
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           "");
#endif /* TCL_VERSION */

    tcl_data.config_file = &tcl_config_file;
    tcl_data.config_look_check_license = &tcl_config_look_check_license;
    tcl_data.config_look_eval_keep_context = &tcl_config_look_eval_keep_context;
    tcl_data.scripts = &tcl_scripts;
    tcl_data.last_script = &last_tcl_script;
    tcl_data.callback_command = &weechat_tcl_command_cb;
    tcl_data.callback_completion = &weechat_tcl_completion_cb;
    tcl_data.callback_hdata = &weechat_tcl_hdata_cb;
    tcl_data.callback_info_eval = &weechat_tcl_info_eval_cb;
    tcl_data.callback_infolist = &weechat_tcl_infolist_cb;
    tcl_data.callback_signal_debug_dump = &weechat_tcl_signal_debug_dump_cb;
    tcl_data.callback_signal_script_action = &weechat_tcl_signal_script_action_cb;
    tcl_data.callback_load_file = &weechat_tcl_load_cb;
    tcl_data.init_before_autoload = NULL;
    tcl_data.unload_all = &weechat_tcl_unload_all;

    old_tcl_quiet = tcl_quiet;
    tcl_quiet = 1;
    plugin_script_init (weechat_tcl_plugin, &tcl_data);
    tcl_quiet = old_tcl_quiet;

    plugin_script_display_short_list (weechat_tcl_plugin,
                                      tcl_scripts);

    /* init OK */
    return WEECHAT_RC_OK;
}

/*
 * Ends tcl plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    int old_tcl_quiet;

    /* unload all scripts */
    old_tcl_quiet = tcl_quiet;
    tcl_quiet = 1;
    if (tcl_script_eval)
    {
        weechat_tcl_unload (tcl_script_eval);
        tcl_script_eval = NULL;
    }
    plugin_script_end (plugin, &tcl_data);
    tcl_quiet = old_tcl_quiet;

    /* free some data */
    if (tcl_action_install_list)
    {
        free (tcl_action_install_list);
        tcl_action_install_list = NULL;
    }
    if (tcl_action_remove_list)
    {
        free (tcl_action_remove_list);
        tcl_action_remove_list = NULL;
    }
    if (tcl_action_autoload_list)
    {
        free (tcl_action_autoload_list);
        tcl_action_autoload_list = NULL;
    }
    /* weechat_string_dyn_free (tcl_buffer_output, 1); */

    return WEECHAT_RC_OK;
}
