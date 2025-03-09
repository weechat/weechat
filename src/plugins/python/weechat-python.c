/*
 * weechat-python.c - python plugin for WeeChat
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2007 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2012 Simon Arlott
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

#include <Python.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "weechat-python.h"
#include "weechat-python-api.h"


WEECHAT_PLUGIN_NAME(PYTHON_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of python scripts"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(PYTHON_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_python_plugin = NULL;

struct t_plugin_script_data python_data;

struct t_config_file *python_config_file = NULL;
struct t_config_option *python_config_look_check_license = NULL;
struct t_config_option *python_config_look_eval_keep_context = NULL;

int python_quiet = 0;

struct t_plugin_script *python_script_eval = NULL;
int python_eval_mode = 0;
int python_eval_send_input = 0;
int python_eval_exec_commands = 0;
struct t_gui_buffer *python_eval_buffer = NULL;
#define PYTHON_EVAL_SCRIPT                                              \
    "import weechat\n"                                                  \
    "\n"                                                                \
    "def script_python_eval(code):\n"                                   \
    "    exec(code)\n"                                                  \
    "\n"                                                                \
    "weechat.register('" WEECHAT_SCRIPT_EVAL_NAME "', '', '1.0', "      \
    "'" WEECHAT_LICENSE "', 'Evaluation of source code', '', '')\n"

struct t_plugin_script *python_scripts = NULL;
struct t_plugin_script *last_python_script = NULL;
struct t_plugin_script *python_current_script = NULL;
struct t_plugin_script *python_registered_script = NULL;
const char *python_current_script_filename = NULL;
PyThreadState *python_mainThreadState = NULL;
PyThreadState *python_current_interpreter = NULL;
char **python_buffer_output = NULL;

/* outputs subroutines */
static PyObject *weechat_python_output (PyObject *self, PyObject *args);
static PyMethodDef weechat_python_output_funcs[] = {
    { "write", weechat_python_output, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef moduleDef = {
    PyModuleDef_HEAD_INIT,
    "weechat",
    NULL,
    0,
    weechat_python_funcs,
    NULL,
    NULL,
    NULL,
    NULL
};
static struct PyModuleDef moduleDefOutputs = {
    PyModuleDef_HEAD_INIT,
    "weechatOutputs",
    NULL,
    0,
    weechat_python_output_funcs,
    NULL,
    NULL,
    NULL,
    NULL
};

/*
 * string used to execute action "install":
 * when signal "python_script_install" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *python_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "python_script_remove" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *python_action_remove_list = NULL;

/*
 * string used to execute action "autoload":
 * when signal "python_script_autoload" is received, name of string
 * is added to this string, to autoload or disable autoload later by a timer
 * (when nothing is running in script)
 */
char *python_action_autoload_list = NULL;


/*
 * Converts a python unicode to a C UTF-8 string.
 *
 * Note: result must be freed after use.
 */

char *
weechat_python_unicode_to_string (PyObject *obj)
{
    PyObject *utf8string;
    char *str;

    str = NULL;

    utf8string = PyUnicode_AsUTF8String (obj);
    if (utf8string)
    {
        if (PyBytes_AsString (utf8string))
            str = strdup (PyBytes_AsString (utf8string));
        Py_XDECREF(utf8string);
    }

    return str;
}

/*
 * Callback called for each key/value in a hashtable.
 */

void
weechat_python_hashtable_map_cb (void *data,
                                 struct t_hashtable *hashtable,
                                 const char *key,
                                 const char *value)
{
    PyObject *dict, *dict_key, *dict_value;

    /* make C compiler happy */
    (void) hashtable;

    dict = (PyObject *)data;

    /* key */
    if (weechat_utf8_is_valid (key, -1, NULL))
        dict_key = Py_BuildValue ("s", key);  /* str */
    else
        dict_key = Py_BuildValue ("y", key);  /* bytes */
    /* value */
    if (weechat_utf8_is_valid (value, -1, NULL))
        dict_value = Py_BuildValue ("s", value);  /* str */
    else
        dict_value = Py_BuildValue ("y", value);  /* bytes */

    if (dict_key && dict_value)
        PyDict_SetItem (dict, dict_key, dict_value);

    if (dict_key)
        Py_DECREF (dict_key);
    if (dict_value)
        Py_DECREF (dict_value);
}

/*
 * Converts a WeeChat hashtable to a python dictionary.
 */

PyObject *
weechat_python_hashtable_to_dict (struct t_hashtable *hashtable)
{
    PyObject *dict;

    dict = PyDict_New ();
    if (!dict)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    weechat_hashtable_map_string (hashtable, &weechat_python_hashtable_map_cb,
                                  dict);

    return dict;
}

/*
 * Converts a python dictionary to a WeeChat hashtable.
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
weechat_python_dict_to_hashtable (PyObject *dict, int size,
                                  const char *type_keys,
                                  const char *type_values)
{
    struct t_hashtable *hashtable;
    PyObject *key, *value;
    Py_ssize_t pos;
    char *str_key, *str_value;

    hashtable = weechat_hashtable_new (size, type_keys, type_values,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    pos = 0;
    while (PyDict_Next (dict, &pos, &key, &value))
    {
        str_key = NULL;
        str_value = NULL;
        if (PyBytes_Check (key))
        {
            if (PyBytes_AsString (key))
                str_key = strdup (PyBytes_AsString (key));
        }
        else
        {
            str_key = weechat_python_unicode_to_string (key);
        }
        if (PyBytes_Check (value))
        {
            if (PyBytes_AsString (value))
                str_value = strdup (PyBytes_AsString (value));
        }
        else
        {
            str_value = weechat_python_unicode_to_string (value);
        }

        if (str_key)
        {
            if (strcmp (type_values, WEECHAT_HASHTABLE_STRING) == 0)
            {
                weechat_hashtable_set (hashtable, str_key, str_value);
            }
            else if (strcmp (type_values, WEECHAT_HASHTABLE_POINTER) == 0)
            {
                weechat_hashtable_set (hashtable, str_key,
                                       plugin_script_str2ptr (weechat_python_plugin,
                                                              NULL, NULL,
                                                              str_value));
            }
        }

        free (str_key);
        free (str_value);
    }

    return hashtable;
}

/*
 * Flushes output.
 */

void
weechat_python_output_flush (void)
{
    const char *ptr_command;
    char *temp_buffer, *command;

    if (!(*python_buffer_output)[0])
        return;

    /* if there's no buffer, we catch the output, so there's no flush */
    if (python_eval_mode && !python_eval_buffer)
        return;

    temp_buffer = strdup (*python_buffer_output);
    if (!temp_buffer)
        return;

    weechat_string_dyn_copy (python_buffer_output, NULL);

    if (python_eval_mode)
    {
        if (python_eval_send_input)
        {
            if (python_eval_exec_commands)
                ptr_command = temp_buffer;
            else
                ptr_command = weechat_string_input_for_buffer (temp_buffer);
            if (ptr_command)
            {
                weechat_command (python_eval_buffer, temp_buffer);
            }
            else
            {
                if (weechat_asprintf (&command,
                                      "%c%s",
                                      temp_buffer[0],
                                      temp_buffer) >= 0)
                {
                    weechat_command (python_eval_buffer,
                                     (command[0]) ? command : " ");
                    free (command);
                }
            }
        }
        else
        {
            weechat_printf (python_eval_buffer, "%s", temp_buffer);
        }
    }
    else
    {
        /* script (no eval mode) */
        weechat_printf (
            NULL,
            weechat_gettext ("%s: stdout/stderr (%s): %s"),
            PYTHON_PLUGIN_NAME,
            (python_current_script) ? python_current_script->name : "?",
            temp_buffer);
    }

    free (temp_buffer);
}

/*
 * Redirection for stdout and stderr.
 */

static PyObject *
weechat_python_output (PyObject *self, PyObject *args)
{
    char *msg, *ptr_msg, *ptr_newline;

    /* make C compiler happy */
    (void) self;

    msg = NULL;

    if (!PyArg_ParseTuple (args, "s", &msg))
    {
        weechat_python_output_flush ();
    }
    else
    {
        ptr_msg = msg;
        while ((ptr_newline = strchr (ptr_msg, '\n')) != NULL)
        {
            weechat_string_dyn_concat (python_buffer_output,
                                       ptr_msg,
                                       ptr_newline - ptr_msg);
            weechat_python_output_flush ();
            ptr_msg = ++ptr_newline;
        }
        weechat_string_dyn_concat (python_buffer_output, ptr_msg, -1);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/*
 * Executes a python function.
 */

void *
weechat_python_exec (struct t_plugin_script *script,
                     int ret_type, const char *function,
                     const char *format, void **argv)
{
    struct t_plugin_script *old_python_current_script;
    PyThreadState *old_interpreter;
    PyObject *evMain, *evDict, *evFunc, *rc;
    void *argv2[16], *ret_value, *ret_temp;
    char format2[17];
    int i, argc, *ret_int;

    ret_value = NULL;

    /* PyEval_AcquireLock (); */

    old_python_current_script = python_current_script;
    python_current_script = script;
    old_interpreter = NULL;

    if (script->interpreter)
    {
        old_interpreter = PyThreadState_Swap (NULL);
        PyThreadState_Swap (script->interpreter);
    }

    evMain = PyImport_AddModule ((char *) "__main__");
    /*
     * FIXME: sometimes NULL is returned with nested calls of hook callbacks,
     * to prevent any crash, we just skip execution of the function
     */
    if (!evMain)
        goto end;
    evDict = PyModule_GetDict (evMain);
    evFunc = PyDict_GetItemString (evDict, function);

    if ( !(evFunc && PyCallable_Check (evFunc)) )
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to run function \"%s\""),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, function);
        /* PyEval_ReleaseThread (python_current_script->interpreter); */
        goto end;
    }

    if (argv && argv[0])
    {
        argc = strlen (format);
        for (i = 0; i < 16; i++)
        {
            if (i < argc)
            {
                switch (format[i])
                {
                    case 's': /* string or null */
                        argv2[i] = argv[i];
                        if (weechat_utf8_is_valid (argv2[i], -1, NULL))
                            format2[i] = 's';  /* str */
                        else
                            format2[i] = 'y';  /* bytes */
                        break;
                    case 'i': /* integer */
                        argv2[i] = PyLong_FromLong ((long)(*((int *)argv[i])));
                        format2[i] = 'O';
                        break;
                    case 'h': /* hash */
                        argv2[i] = weechat_python_hashtable_to_dict (
                            (struct t_hashtable *)argv[i]);
                        format2[i] = 'O';
                        break;
                    case 'O': /* object */
                        argv2[i] = argv[i];
                        format2[i] = 'O';
                        break;
                }
            }
            else
            {
                argv2[i] = NULL;
                format2[i] = '\0';
            }
        }
        format2[argc] = '\0';

        rc = PyObject_CallFunction (evFunc, format2,
                                    argv2[0], argv2[1],
                                    argv2[2], argv2[3],
                                    argv2[4], argv2[5],
                                    argv2[6], argv2[7],
                                    argv2[8], argv2[9],
                                    argv2[10], argv2[11],
                                    argv2[12], argv2[13],
                                    argv2[14], argv2[15]);

        for (i = 0; i < argc; i++)
        {
            if (argv2[i] && (format2[i] == 'O'))
                Py_XDECREF((PyObject *)argv2[i]);
        }
    }
    else
    {
        rc = PyObject_CallFunction (evFunc, NULL);
    }

    weechat_python_output_flush ();

    /*
     * ugly hack : rc = NULL while 'return weechat.WEECHAT_RC_OK ....
     * because of '#define WEECHAT_RC_OK 0'
     */
    if (rc ==  NULL)
        rc = PyLong_FromLong ((long)0);

    if (PyErr_Occurred ())
    {
        PyErr_Print ();
        Py_XDECREF(rc);
    }
    else if ((ret_type == WEECHAT_SCRIPT_EXEC_STRING) && (PyUnicode_Check (rc)))
    {
        ret_value = weechat_python_unicode_to_string (rc);
        Py_XDECREF(rc);
    }
    else if ((ret_type == WEECHAT_SCRIPT_EXEC_STRING) && (PyBytes_Check (rc)))
    {
        if (PyBytes_AsString (rc))
            ret_value = strdup (PyBytes_AsString (rc));
        else
            ret_value = NULL;
        Py_XDECREF(rc);
    }
    else if ((ret_type == WEECHAT_SCRIPT_EXEC_POINTER) && (PyUnicode_Check (rc)))
    {
        ret_temp = weechat_python_unicode_to_string (rc);
        if (ret_temp)
        {
            ret_value = plugin_script_str2ptr (weechat_python_plugin,
                                               script->name, function,
                                               ret_temp);
            free (ret_temp);
        }
        else
        {
            ret_value = NULL;
        }
        Py_XDECREF(rc);
    }
    else if ((ret_type == WEECHAT_SCRIPT_EXEC_POINTER) && (PyBytes_Check (rc)))
    {
        if (PyBytes_AsString (rc))
        {
            ret_value = plugin_script_str2ptr (weechat_python_plugin,
                                               script->name, function,
                                               PyBytes_AsString (rc));
        }
        else
        {
            ret_value = NULL;
        }
        Py_XDECREF(rc);
    }
    else if ((ret_type == WEECHAT_SCRIPT_EXEC_INT) && (PY_INTEGER_CHECK(rc)))
    {
        ret_int = malloc (sizeof (*ret_int));
        if (ret_int)
            *ret_int = (int) PyLong_AsLong (rc);
        ret_value = ret_int;
        Py_XDECREF(rc);
    }
    else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
    {
        ret_value = weechat_python_dict_to_hashtable (rc,
                                                      WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE,
                                                      WEECHAT_HASHTABLE_STRING,
                                                      WEECHAT_HASHTABLE_STRING);
        Py_XDECREF(rc);
    }
    else
    {
        if (ret_type != WEECHAT_SCRIPT_EXEC_IGNORE)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: function \"%s\" must "
                                             "return a valid value"),
                            weechat_prefix ("error"), PYTHON_PLUGIN_NAME,
                            function);
        }
        Py_XDECREF(rc);
    }

    if ((ret_type != WEECHAT_SCRIPT_EXEC_IGNORE) && !ret_value)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error in function \"%s\""),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME,
                        function);
    }

    /* PyEval_ReleaseThread (python_current_script->interpreter); */

end:
    python_current_script = old_python_current_script;

    if (old_interpreter)
        PyThreadState_Swap (old_interpreter);

    return ret_value;
}

/*
 * Initializes the "weechat" module.
 */

static PyObject *weechat_python_init_module_weechat (void)
{
    PyObject *weechat_module;
    int i;

    weechat_module = PyModule_Create (&moduleDef);

    if (!weechat_module)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to initialize WeeChat "
                                         "module"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        return NULL;
    }

    /* define some constants */
    for (i = 0; weechat_script_constants[i].name; i++)
    {
        if (weechat_script_constants[i].value_string)
        {
            PyModule_AddStringConstant (
                weechat_module,
                weechat_script_constants[i].name,
                weechat_script_constants[i].value_string);
        }
        else
        {
            PyModule_AddIntConstant (
                weechat_module,
                weechat_script_constants[i].name,
                (long)weechat_script_constants[i].value_integer);
        }
    }

    return weechat_module;
}

/*
 * Sets weechat output (to redirect stdout/stderr to WeeChat buffer).
 */

void
weechat_python_set_output (void)
{
    PyObject *weechat_outputs;

    weechat_outputs = PyModule_Create (&moduleDefOutputs);

    if (weechat_outputs)
    {
        if (PySys_SetObject ("stdout", weechat_outputs) == -1)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to redirect stdout"),
                            weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        }
        if (PySys_SetObject ("stderr", weechat_outputs) == -1)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to redirect stderr"),
                            weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to redirect stdout and "
                                         "stderr"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
    }
}

/*
 * Loads a python script.
 *
 * If code is NULL, the content of filename is read and executed.
 * If code is not NULL, it is executed (the file is not read).
 *
 * Returns pointer to new registered script, NULL if error.
 */

struct t_plugin_script *
weechat_python_load (const char *filename, const char *code)
{
    FILE *fp;
    PyObject *python_path, *path, *module_main, *globals, *rc;
    char *weechat_sharedir, *weechat_data_dir;
    char *str_sharedir, *str_home;

    fp = NULL;

    if (!code)
    {
        fp = fopen (filename, "r");
        if (!fp)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: script \"%s\" not found"),
                            weechat_prefix ("error"), PYTHON_PLUGIN_NAME,
                            filename);
            return NULL;
        }
    }

    if ((weechat_python_plugin->debug >= 2) || !python_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: loading script \"%s\""),
                        PYTHON_PLUGIN_NAME, filename);
    }

    python_current_script = NULL;
    python_registered_script = NULL;

    /* PyEval_AcquireLock (); */
    python_current_interpreter = Py_NewInterpreter ();
    if (!python_current_interpreter)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        if (fp)
            fclose (fp);
        /* PyEval_ReleaseLock (); */
        return NULL;
    }

    PyThreadState_Swap (python_current_interpreter);

    /* adding $weechat_sharedir/python in $PYTHONPATH */
    python_path = PySys_GetObject ("path");
    weechat_sharedir = weechat_info_get ("weechat_sharedir", "");
    if (weechat_sharedir)
    {
        if (weechat_asprintf (&str_sharedir,
                              "%s/%s",
                              weechat_sharedir,
                              PYTHON_PLUGIN_NAME) >= 0)
        {
            path = PyUnicode_FromString (str_sharedir);
            if (path != NULL)
            {
                PyList_Insert (python_path, 0, path);
                Py_DECREF (path);
            }
            free (str_sharedir);
        }
        free (weechat_sharedir);
    }

    /* add {weechat_data_dir}/python in $PYTHONPATH */
    weechat_data_dir = weechat_info_get ("weechat_data_dir", "");
    if (weechat_data_dir)
    {
        if (weechat_asprintf (&str_home,
                              "%s/%s",
                              weechat_data_dir,
                              PYTHON_PLUGIN_NAME) >= 0)
        {
            path = PyUnicode_FromString (str_home);
            if (path != NULL)
            {
                PyList_Insert (python_path, 0, path);
                Py_DECREF (path);
            }
            free (str_home);
        }
        free (weechat_data_dir);
    }


    weechat_python_set_output ();

    python_current_script_filename = filename;

    if (code)
    {
        /* execute code without reading file */
        module_main = PyImport_AddModule ("__main__");
        globals = PyModule_GetDict (module_main);
        rc = PyRun_String (code, Py_file_input, globals, NULL);
        if (PyErr_Occurred ())
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to execute source "
                                             "code"),
                            weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
            PyErr_Print ();
            if (rc)
                Py_XDECREF(rc);

            /* if script was registered, remove it from list */
            if (python_current_script)
            {
                plugin_script_remove (weechat_python_plugin,
                                      &python_scripts, &last_python_script,
                                      python_current_script);
                python_current_script = NULL;
            }

            Py_EndInterpreter (python_current_interpreter);
            /* PyEval_ReleaseLock (); */

            return NULL;
        }
        if (rc)
            Py_XDECREF(rc);
    }
    else
    {
        /* read and execute code from file */
        if (PyRun_SimpleFile (fp, filename) != 0)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to parse file \"%s\""),
                            weechat_prefix ("error"), PYTHON_PLUGIN_NAME, filename);
            fclose (fp);

            if (PyErr_Occurred ())
                PyErr_Print ();

            /* if script was registered, remove it from list */
            if (python_current_script)
            {
                plugin_script_remove (weechat_python_plugin,
                                      &python_scripts, &last_python_script,
                                      python_current_script);
                python_current_script = NULL;
            }

            Py_EndInterpreter (python_current_interpreter);
            /* PyEval_ReleaseLock (); */

            return NULL;
        }
        fclose (fp);
    }

    if (PyErr_Occurred ())
        PyErr_Print ();

    if (!python_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, filename);

        if (PyErr_Occurred ())
            PyErr_Print ();
        Py_EndInterpreter (python_current_interpreter);
        /* PyEval_ReleaseLock (); */

        return NULL;
    }
    python_current_script = python_registered_script;

    /* PyEval_ReleaseThread (python_current_script->interpreter); */

    /*
     * set input/close callbacks for buffers created by this script
     * (to restore callbacks after upgrade)
     */
    plugin_script_set_buffer_callbacks (weechat_python_plugin,
                                        python_scripts,
                                        python_current_script,
                                        &weechat_python_api_buffer_input_data_cb,
                                        &weechat_python_api_buffer_close_cb);

    (void) weechat_hook_signal_send ("python_script_loaded",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     python_current_script->filename);

    return python_current_script;
}

/*
 * Callback for script_auto_load() function.
 */

void
weechat_python_load_cb (void *data, const char *filename)
{
    const char *pos_dot;

    /* make C compiler happy */
    (void) data;

    pos_dot = strrchr (filename, '.');
    if (pos_dot && (strcmp (pos_dot, ".py") == 0))
        weechat_python_load (filename, NULL);
}

/*
 * Unloads a python script.
 */

void
weechat_python_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;
    char *filename;

    if ((weechat_python_plugin->debug >= 2) || !python_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        PYTHON_PLUGIN_NAME, script->name);
    }

    if (script->shutdown_func && script->shutdown_func[0])
    {
        rc = (int *) weechat_python_exec (script, WEECHAT_SCRIPT_EXEC_INT,
                                          script->shutdown_func, NULL, NULL);
        free (rc);
    }

    filename = strdup (script->filename);
    interpreter = script->interpreter;

    if (python_current_script == script)
    {
        python_current_script = (python_current_script->prev_script) ?
            python_current_script->prev_script : python_current_script->next_script;
    }

    plugin_script_remove (weechat_python_plugin, &python_scripts, &last_python_script,
                          script);

    if (interpreter)
    {
        PyThreadState_Swap (interpreter);
        Py_EndInterpreter (interpreter);
    }

    if (python_current_script)
        PyThreadState_Swap (python_current_script->interpreter);

    (void) weechat_hook_signal_send ("python_script_unloaded",
                                     WEECHAT_HOOK_SIGNAL_STRING, filename);
    free (filename);
}

/*
 * Unloads a python script by name.
 */

void
weechat_python_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (python_scripts, name);
    if (ptr_script)
    {
        weechat_python_unload (ptr_script);
        if (!python_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            PYTHON_PLUGIN_NAME, name);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, name);
    }
}

/*
 * Unloads all python scripts.
 */

void
weechat_python_unload_all (void)
{
    while (python_scripts)
    {
        weechat_python_unload (python_scripts);
    }
}

/*
 * Reloads a python script by name.
 */

void
weechat_python_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (python_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_python_unload (ptr_script);
            if (!python_quiet)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s: script \"%s\" unloaded"),
                                PYTHON_PLUGIN_NAME, name);
            }
            weechat_python_load (filename, NULL);
            free (filename);
        }
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, name);
    }
}

/*
 * Evaluates python source code.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weechat_python_eval (struct t_gui_buffer *buffer, int send_to_buffer_as_input,
                     int exec_commands, const char *code)
{
    void *func_argv[1], *result;
    int old_python_quiet;

    if (!python_script_eval)
    {
        old_python_quiet = python_quiet;
        python_quiet = 1;
        python_script_eval = weechat_python_load (WEECHAT_SCRIPT_EVAL_NAME,
                                                  PYTHON_EVAL_SCRIPT);
        python_quiet = old_python_quiet;
        if (!python_script_eval)
            return 0;
    }

    weechat_python_output_flush ();

    python_eval_mode = 1;
    python_eval_send_input = send_to_buffer_as_input;
    python_eval_exec_commands = exec_commands;
    python_eval_buffer = buffer;

    func_argv[0] = (char *)code;
    result = weechat_python_exec (python_script_eval,
                                  WEECHAT_SCRIPT_EXEC_IGNORE,
                                  "script_python_eval",
                                  "s", func_argv);
    /* result is ignored */
    free (result);

    weechat_python_output_flush ();

    python_eval_mode = 0;
    python_eval_send_input = 0;
    python_eval_exec_commands = 0;
    python_eval_buffer = NULL;

    if (!weechat_config_boolean (python_config_look_eval_keep_context))
    {
        old_python_quiet = python_quiet;
        python_quiet = 1;
        weechat_python_unload (python_script_eval);
        python_quiet = old_python_quiet;
        python_script_eval = NULL;
    }

    return 1;
}

/*
 * Function called before the auto-load of scripts.
 */

void
weechat_python_init_before_autoload (void)
{
#if PY_VERSION_HEX >= 0x030C0000 && PY_VERSION_HEX < 0x030D0000
    /*
     * Workaround for crash when ending interpreters in Python 3.12:
     * the first time we load a script, we eval an empty string.
     * See https://github.com/weechat/weechat/issues/2046
     */
    weechat_python_eval (NULL, 0, 0, "");
#endif
}

/*
 * Callback for command "/python".
 */

int
weechat_python_command_cb (const void *pointer, void *data,
                           struct t_gui_buffer *buffer,
                           int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *ptr_code, *path_script;
    int i, send_to_buffer_as_input, exec_commands, old_python_quiet;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_python_plugin, python_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_python_plugin, python_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_python_plugin, python_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_python_plugin, &weechat_python_load_cb);
        }
        else if (weechat_strcmp (argv[1], "reload") == 0)
        {
            weechat_python_unload_all ();
            plugin_script_auto_load (weechat_python_plugin, &weechat_python_load_cb);
        }
        else if (weechat_strcmp (argv[1], "unload") == 0)
        {
            weechat_python_unload_all ();
        }
        else if (weechat_strcmp (argv[1], "version") == 0)
        {
            plugin_script_display_interpreter (weechat_python_plugin, 0);
        }
        else
            WEECHAT_COMMAND_ERROR;
    }
    else
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_python_plugin, python_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_python_plugin, python_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcmp (argv[1], "load") == 0)
                 || (weechat_strcmp (argv[1], "reload") == 0)
                 || (weechat_strcmp (argv[1], "unload") == 0))
        {
            old_python_quiet = python_quiet;
            ptr_name = argv_eol[2];
            if (strncmp (ptr_name, "-q ", 3) == 0)
            {
                python_quiet = 1;
                ptr_name += 3;
                while (ptr_name[0] == ' ')
                {
                    ptr_name++;
                }
            }
            if (weechat_strcmp (argv[1], "load") == 0)
            {
                /* load python script */
                path_script = plugin_script_search_path (weechat_python_plugin,
                                                         ptr_name, 1);
                weechat_python_load ((path_script) ? path_script : ptr_name,
                                     NULL);
                free (path_script);
            }
            else if (weechat_strcmp (argv[1], "reload") == 0)
            {
                /* reload one python script */
                weechat_python_reload_name (ptr_name);
            }
            else if (weechat_strcmp (argv[1], "unload") == 0)
            {
                /* unload python script */
                weechat_python_unload_name (ptr_name);
            }
            python_quiet = old_python_quiet;
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
            if (!weechat_python_eval (buffer, send_to_buffer_as_input,
                                      exec_commands, ptr_code))
                WEECHAT_COMMAND_ERROR;
        }
        else
            WEECHAT_COMMAND_ERROR;
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds python scripts to completion list.
 */

int
weechat_python_completion_cb (const void *pointer, void *data,
                              const char *completion_item,
                              struct t_gui_buffer *buffer,
                              struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_python_plugin, completion, python_scripts);

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for python scripts.
 */

struct t_hdata *
weechat_python_hdata_cb (const void *pointer, void *data,
                         const char *hdata_name)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &python_scripts, &last_python_script,
                                       hdata_name);
}

/*
 * Returns python info "python_eval".
 */

char *
weechat_python_info_eval_cb (const void *pointer, void *data,
                             const char *info_name,
                             const char *arguments)
{
    char *output;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    weechat_python_eval (NULL, 0, 0, (arguments) ? arguments : "");
    output = strdup (*python_buffer_output);
    weechat_string_dyn_copy (python_buffer_output, NULL);

    return output;
}

/*
 * Returns infolist with list of functions in python scripting API.
 */

struct t_infolist *
weechat_python_infolist_functions (void)
{
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    int i;

    infolist = weechat_infolist_new ();
    if (!infolist)
        return NULL;

    for (i = 0; weechat_python_funcs[i].ml_name; i++)
    {
        item = weechat_infolist_new_item (infolist);
        if (!item)
        {
            weechat_infolist_free (infolist);
            return NULL;
        }
        if (!weechat_infolist_new_var_string (item, "name", weechat_python_funcs[i].ml_name))
        {
            weechat_infolist_free (infolist);
            return NULL;
        }
    }
    return infolist;
}

/*
 * Returns infolist with list of constants in python scripting API.
 */

struct t_infolist *
weechat_python_infolist_constants (void)
{
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    int i;

    infolist = weechat_infolist_new ();
    if (!infolist)
        goto error;

    for (i = 0; weechat_script_constants[i].name; i++)
    {
        item = weechat_infolist_new_item (infolist);
        if (!item)
            goto error;
        if (!weechat_infolist_new_var_string (item, "name", weechat_script_constants[i].name))
            goto error;
        if (weechat_script_constants[i].value_string)
        {
            if (!weechat_infolist_new_var_string (item, "type", "string"))
                goto error;
            if (!weechat_infolist_new_var_string (item, "value_string", weechat_script_constants[i].value_string))
                goto error;
        }
        else
        {
            if (!weechat_infolist_new_var_string (item, "type", "integer"))
                goto error;
            if (!weechat_infolist_new_var_integer (item, "value_integer", weechat_script_constants[i].value_integer))
                goto error;
        }
    }
    return infolist;

error:
    weechat_infolist_free (infolist);
    return NULL;
}

/*
 * Returns infolist with python scripts.
 */

struct t_infolist *
weechat_python_infolist_cb (const void *pointer, void *data,
                            const char *infolist_name,
                            void *obj_pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (strcmp (infolist_name, "python_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_python_plugin,
                                                    python_scripts,
                                                    obj_pointer,
                                                    arguments);
    }

    if (strcmp (infolist_name, "python_function") == 0)
        return weechat_python_infolist_functions ();

    if (strcmp (infolist_name, "python_constant") == 0)
        return weechat_python_infolist_constants ();

    return NULL;
}

/*
 * Dumps python plugin data in WeeChat log file.
 */

int
weechat_python_signal_debug_dump_cb (const void *pointer, void *data,
                                     const char *signal,
                                     const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, PYTHON_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_python_plugin, python_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * Timer for executing actions.
 */

int
weechat_python_timer_action_cb (const void *pointer, void *data,
                                int remaining_calls)
{
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    if (pointer)
    {
        if (pointer == &python_action_install_list)
        {
            plugin_script_action_install (weechat_python_plugin,
                                          python_scripts,
                                          &weechat_python_unload,
                                          &weechat_python_load,
                                          &python_quiet,
                                          &python_action_install_list);
        }
        else if (pointer == &python_action_remove_list)
        {
            plugin_script_action_remove (weechat_python_plugin,
                                         python_scripts,
                                         &weechat_python_unload,
                                         &python_quiet,
                                         &python_action_remove_list);
        }
        else if (pointer == &python_action_autoload_list)
        {
            plugin_script_action_autoload (weechat_python_plugin,
                                           &python_quiet,
                                           &python_action_autoload_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when a script action is asked (install/remove/autoload a
 * script).
 */

int
weechat_python_signal_script_action_cb (const void *pointer, void *data,
                                        const char *signal,
                                        const char *type_data,
                                        void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "python_script_install") == 0)
        {
            plugin_script_action_add (&python_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_python_timer_action_cb,
                                &python_action_install_list, NULL);
        }
        else if (strcmp (signal, "python_script_remove") == 0)
        {
            plugin_script_action_add (&python_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_python_timer_action_cb,
                                &python_action_remove_list, NULL);
        }
        else if (strcmp (signal, "python_script_autoload") == 0)
        {
            plugin_script_action_add (&python_action_autoload_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_python_timer_action_cb,
                                &python_action_autoload_list, NULL);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes python plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int old_python_quiet;

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_python_plugin = plugin;

    python_quiet = 0;
    python_eval_mode = 0;
    python_eval_send_input = 0;
    python_eval_exec_commands = 0;

    /* set interpreter name and version */
    weechat_hashtable_set (plugin->variables, "interpreter_name",
                           plugin->name);
#ifdef PY_VERSION
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           PY_VERSION);
#else
    weechat_hashtable_set (plugin->variables, "interpreter_version",
                           "");
#endif /* PY_VERSION */

    /* init stdout/stderr buffer */
    python_buffer_output = weechat_string_dyn_alloc (256);
    if (!python_buffer_output)
        return WEECHAT_RC_ERROR;

    PyImport_AppendInittab ("weechat",
                            &weechat_python_init_module_weechat);

    Py_Initialize ();
    if (Py_IsInitialized () == 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to launch global "
                                         "interpreter"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        weechat_string_dyn_free (python_buffer_output, 1);
        return WEECHAT_RC_ERROR;
    }

    /* PyEval_InitThreads(); */
    /* python_mainThreadState = PyThreadState_Swap(NULL); */
#if PY_VERSION_HEX >= 0x03070000
    python_mainThreadState = PyThreadState_Get ();
#else
    python_mainThreadState = PyEval_SaveThread ();
#endif
    /* PyEval_ReleaseLock (); */

    if (!python_mainThreadState)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to get current "
                                         "interpreter state"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        weechat_string_dyn_free (python_buffer_output, 1);
        return WEECHAT_RC_ERROR;
    }

    python_data.config_file = &python_config_file;
    python_data.config_look_check_license = &python_config_look_check_license;
    python_data.config_look_eval_keep_context = &python_config_look_eval_keep_context;
    python_data.scripts = &python_scripts;
    python_data.last_script = &last_python_script;
    python_data.callback_command = &weechat_python_command_cb;
    python_data.callback_completion = &weechat_python_completion_cb;
    python_data.callback_hdata = &weechat_python_hdata_cb;
    python_data.callback_info_eval = &weechat_python_info_eval_cb;
    python_data.callback_infolist = &weechat_python_infolist_cb;
    python_data.callback_signal_debug_dump = &weechat_python_signal_debug_dump_cb;
    python_data.callback_signal_script_action = &weechat_python_signal_script_action_cb;
    python_data.callback_load_file = &weechat_python_load_cb;
    python_data.init_before_autoload = &weechat_python_init_before_autoload;
    python_data.unload_all = &weechat_python_unload_all;

    old_python_quiet = python_quiet;
    python_quiet = 1;
    plugin_script_init (weechat_python_plugin, &python_data);
    python_quiet = old_python_quiet;

    plugin_script_display_short_list (weechat_python_plugin,
                                      python_scripts);

    weechat_hook_infolist ("python_function",
                           N_("list of scripting API functions"),
                           "",
                           "",
                           &weechat_python_infolist_cb, NULL, NULL);
    weechat_hook_infolist ("python_constant",
                           N_("list of scripting API constants"),
                           "",
                           "",
                           &weechat_python_infolist_cb, NULL, NULL);

    /* init OK */
    return WEECHAT_RC_OK;
}

/*
 * Ends python plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    int old_python_quiet;

    /* unload all scripts */
    old_python_quiet = python_quiet;
    python_quiet = 1;
    plugin_script_end (plugin, &python_data);
    if (python_script_eval)
    {
        weechat_python_unload (python_script_eval);
        python_script_eval = NULL;
    }
    python_quiet = old_python_quiet;

    /* free python interpreter */
    if (python_mainThreadState != NULL)
    {
        /* PyEval_AcquireLock (); */
        PyThreadState_Swap (python_mainThreadState);
        /* PyEval_ReleaseLock (); */
        python_mainThreadState = NULL;
    }

    Py_Finalize ();
    if (Py_IsInitialized () != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to free interpreter"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
    }

    /* free some data */
    if (python_action_install_list)
    {
        free (python_action_install_list);
        python_action_install_list = NULL;
    }
    if (python_action_remove_list)
    {
        free (python_action_remove_list);
        python_action_remove_list = NULL;
    }
    if (python_action_autoload_list)
    {
        free (python_action_autoload_list);
        python_action_autoload_list = NULL;
    }
    weechat_string_dyn_free (python_buffer_output, 1);
    python_buffer_output = NULL;

    return WEECHAT_RC_OK;
}
