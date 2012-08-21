/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2007 Emmanuel Bouthenot <kolter@openics.org>
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
 * weechat-python.c: python plugin for WeeChat
 */

#undef _

#include <Python.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../weechat-plugin.h"
#include "../plugin-script.h"
#include "weechat-python.h"
#include "weechat-python-api.h"


WEECHAT_PLUGIN_NAME(PYTHON_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Support of python scripts"));
WEECHAT_PLUGIN_AUTHOR("Sebastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_python_plugin = NULL;

int python_quiet;
struct t_plugin_script *python_scripts = NULL;
struct t_plugin_script *last_python_script = NULL;
struct t_plugin_script *python_current_script = NULL;
struct t_plugin_script *python_registered_script = NULL;
const char *python_current_script_filename = NULL;
PyThreadState *python_mainThreadState = NULL;
char *python2_bin = NULL;

/* outputs subroutines */
static PyObject *weechat_python_output (PyObject *self, PyObject *args);
static PyMethodDef weechat_python_output_funcs[] = {
    { "write", weechat_python_output, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL }
};

#if PY_MAJOR_VERSION >= 3
/* module definition for python >= 3.x */
static struct PyModuleDef moduleDef = {
    PyModuleDef_HEAD_INIT,
    "weechat",
    NULL,
    -1,
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
    -1,
    weechat_python_output_funcs,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif

/*
 * string used to execute action "install":
 * when signal "python_install_script" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *python_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "python_remove_script" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *python_action_remove_list = NULL;

char python_buffer_output[128];


/*
 * weechat_python_set_python2_bin: set path to python 2.x interpreter
 */

void
weechat_python_set_python2_bin ()
{
    const char *dir_separator;
    char *path, **paths, bin[4096];
    char *versions[] = { "2.7", "2.6", "2.5", "2.4", "2.3", "2.2", "2", NULL };
    int num_paths, i, j, rc;
    struct stat stat_buf;

    python2_bin = NULL;

    dir_separator = weechat_info_get ("dir_separator", "");
    path = getenv ("PATH");

    if (dir_separator && path)
    {
        paths = weechat_string_split (path, ":", 0, 0, &num_paths);
        if (paths)
        {
            for (i = 0; i < num_paths; i++)
            {
                for (j = 0; versions[j]; j++)
                {
                    snprintf (bin, sizeof (bin), "%s%s%s%s",
                              paths[i], dir_separator, "python",
                              versions[j]);
                    rc = stat (bin, &stat_buf);
                    if ((rc == 0) && (S_ISREG(stat_buf.st_mode)))
                    {
                        python2_bin = strdup (bin);
                        break;
                    }
                }
                if (python2_bin)
                    break;
            }
            weechat_string_free_split (paths);
        }
    }

    if (!python2_bin)
        python2_bin = strdup ("python");
}

/*
 * weechat_python_unicode_to_string: convert a python unicode to a C UTF-8
 *                                   string
 *                                   Note: result has to be freed after use
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
 * weechat_python_hashtable_map_cb: callback called for each key/value in a
 *                                  hashtable
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

    dict_key = Py_BuildValue ("s", key);
    dict_value = Py_BuildValue ("s", value);

    PyDict_SetItem (dict, dict_key, dict_value);

    Py_DECREF (dict_key);
    Py_DECREF (dict_value);
}

/*
 * weechat_python_hashtable_to_dict: get python dictionary with a WeeChat
 *                                   hashtable
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

    weechat_hashtable_map_string (hashtable,
                                  &weechat_python_hashtable_map_cb,
                                  dict);

    return dict;
}

/*
 * weechat_python_dict_to_hashtable: get WeeChat hashtable with python
 *                                   dictionary
 *                                   Hashtable returned has type string for
 *                                   both keys and values
 *                                   Note: hashtable has to be released after
 *                                   use with call to weechat_hashtable_free()
 */

struct t_hashtable *
weechat_python_dict_to_hashtable (PyObject *dict, int hashtable_size)
{
    struct t_hashtable *hashtable;
    PyObject *key, *value;
    Py_ssize_t pos;
    char *str_key, *str_value;

    hashtable = weechat_hashtable_new (hashtable_size,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL,
                                       NULL);
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
            str_key = weechat_python_unicode_to_string (key);
        if (PyBytes_Check (value))
        {
            if (PyBytes_AsString (value))
                str_value = strdup (PyBytes_AsString (value));
        }
        else
            str_value = weechat_python_unicode_to_string (value);

        if (str_key)
            weechat_hashtable_set (hashtable, str_key, str_value);

        if (str_key)
            free (str_key);
        if (str_value)
            free (str_value);
    }

    return hashtable;
}

/*
 * weechat_python_exec: execute a python function
 */

void *
weechat_python_exec (struct t_plugin_script *script,
                     int ret_type, const char *function,
                     char *format, void **argv)
{
    struct t_plugin_script *old_python_current_script;
    PyThreadState *old_interpreter;
    PyObject *evMain, *evDict, *evFunc, *rc;
    void *argv2[16], *ret_value;
    int i, argc, *ret_int;

    /* PyEval_AcquireLock (); */

    old_python_current_script = python_current_script;
    old_interpreter = NULL;
    if (script->interpreter)
    {
        old_interpreter = PyThreadState_Swap (NULL);
        PyThreadState_Swap (script->interpreter);
    }

    evMain = PyImport_AddModule ((char *) "__main__");
    evDict = PyModule_GetDict (evMain);
    evFunc = PyDict_GetItemString (evDict, function);

    if ( !(evFunc && PyCallable_Check (evFunc)) )
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to run function \"%s\""),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, function);
        /* PyEval_ReleaseThread (python_current_script->interpreter); */
        if (old_interpreter)
            PyThreadState_Swap (old_interpreter);
        return NULL;
    }

    python_current_script = script;

    if (argv && argv[0])
    {
        argc = strlen (format);
        for (i = 0; i < 16; i++)
        {
            argv2[i] = (i < argc) ? argv[i] : NULL;
        }
        rc = PyObject_CallFunction (evFunc, format,
                                    argv2[0], argv2[1],
                                    argv2[2], argv2[3],
                                    argv2[4], argv2[5],
                                    argv2[6], argv2[7],
                                    argv2[8], argv2[9],
                                    argv2[10], argv2[11],
                                    argv2[12], argv2[13],
                                    argv2[14], argv2[15]);
    }
    else
    {
        rc = PyObject_CallFunction (evFunc, NULL);
    }

    ret_value = NULL;

    /*
     * ugly hack : rc = NULL while 'return weechat.WEECHAT_RC_OK ....
     * because of '#define WEECHAT_RC_OK 0'
     */
    if (rc ==  NULL)
        rc = PyLong_FromLong ((long)0);

    if (PyErr_Occurred())
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
    else if ((ret_type == WEECHAT_SCRIPT_EXEC_INT) && (PyLong_Check (rc)))
    {
        ret_int = malloc (sizeof (*ret_int));
        if (ret_int)
            *ret_int = (int) PyLong_AsLong(rc);
        ret_value = ret_int;
        Py_XDECREF(rc);
    }
    else if (ret_type == WEECHAT_SCRIPT_EXEC_HASHTABLE)
    {
        ret_value = weechat_python_dict_to_hashtable (rc,
                                                      WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE);
        Py_XDECREF(rc);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"%s\" must return "
                                         "a valid value"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, function);
    }

    if (!ret_value)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error in function \"%s\""),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, function);
    }

    /* PyEval_ReleaseThread (python_current_script->interpreter); */

    python_current_script = old_python_current_script;

    if (old_interpreter)
        PyThreadState_Swap (old_interpreter);

    return ret_value;
}

/*
 * weechat_python_output: redirection for stdout and stderr
 */

static PyObject *
weechat_python_output (PyObject *self, PyObject *args)
{
    char *msg, *m, *p;

    /* make C compiler happy */
    (void) self;

    msg = NULL;

    if (!PyArg_ParseTuple (args, "s", &msg))
    {
        if (strlen(python_buffer_output) > 0)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: stdout/stderr: %s%s"),
                            PYTHON_PLUGIN_NAME, python_buffer_output, "");
            python_buffer_output[0] = '\0';
        }
    }
    else
    {
        m = msg;
        while ((p = strchr (m, '\n')) != NULL)
        {
            *p = '\0';
            if (strlen (m) + strlen (python_buffer_output) > 0)
            {
                weechat_printf (NULL,
                                weechat_gettext ("%s: stdout/stderr: %s%s"),
                                PYTHON_PLUGIN_NAME, python_buffer_output, m);
            }
            *p = '\n';
            python_buffer_output[0] = '\0';
            m = ++p;
        }

        if (strlen(m) + strlen(python_buffer_output) > sizeof(python_buffer_output))
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: stdout/stderr: %s%s"),
                            PYTHON_PLUGIN_NAME, python_buffer_output, m);
            python_buffer_output[0] = '\0';
        }
        else
            strcat (python_buffer_output, m);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/*
 * weechat_python_init_module_weechat: initialize the "weechat" module
 */

#if PY_MAJOR_VERSION >= 3
static PyObject *weechat_python_init_module_weechat ()
#else
void weechat_python_init_module_weechat ()
#endif
{
    PyObject *weechat_module, *weechat_dict;

#if PY_MAJOR_VERSION >= 3
    /* python >= 3.x */
    weechat_module = PyModule_Create (&moduleDef);
#else
    /* python <= 2.x */
    weechat_module = Py_InitModule ("weechat", weechat_python_funcs);
#endif

    if (!weechat_module)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to initialize WeeChat "
                                         "module"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
#if PY_MAJOR_VERSION >= 3
        return NULL;
#else
        return;
#endif
    }

    /* define some constants */
    weechat_dict = PyModule_GetDict(weechat_module);
    PyDict_SetItemString(weechat_dict, "WEECHAT_RC_OK", PyLong_FromLong((long) WEECHAT_RC_OK));
    PyDict_SetItemString(weechat_dict, "WEECHAT_RC_OK_EAT", PyLong_FromLong((long) WEECHAT_RC_OK_EAT));
    PyDict_SetItemString(weechat_dict, "WEECHAT_RC_ERROR", PyLong_FromLong((long) WEECHAT_RC_ERROR));

    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_READ_OK", PyLong_FromLong((long) WEECHAT_CONFIG_READ_OK));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_READ_MEMORY_ERROR", PyLong_FromLong((long) WEECHAT_CONFIG_READ_MEMORY_ERROR));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_READ_FILE_NOT_FOUND", PyLong_FromLong((long) WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_WRITE_OK", PyLong_FromLong((long) WEECHAT_CONFIG_WRITE_OK));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_WRITE_ERROR", PyLong_FromLong((long) WEECHAT_CONFIG_WRITE_ERROR));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_WRITE_MEMORY_ERROR", PyLong_FromLong((long) WEECHAT_CONFIG_WRITE_MEMORY_ERROR));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_OPTION_SET_OK_CHANGED", PyLong_FromLong((long) WEECHAT_CONFIG_OPTION_SET_OK_CHANGED));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE", PyLong_FromLong((long) WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_OPTION_SET_ERROR", PyLong_FromLong((long) WEECHAT_CONFIG_OPTION_SET_ERROR));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND", PyLong_FromLong((long) WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET", PyLong_FromLong((long) WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_OPTION_UNSET_OK_RESET", PyLong_FromLong((long) WEECHAT_CONFIG_OPTION_UNSET_OK_RESET));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED", PyLong_FromLong((long) WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED));
    PyDict_SetItemString(weechat_dict, "WEECHAT_CONFIG_OPTION_UNSET_ERROR", PyLong_FromLong((long) WEECHAT_CONFIG_OPTION_UNSET_ERROR));

    PyDict_SetItemString(weechat_dict, "WEECHAT_LIST_POS_SORT", PyUnicode_FromString(WEECHAT_LIST_POS_SORT));
    PyDict_SetItemString(weechat_dict, "WEECHAT_LIST_POS_BEGINNING", PyUnicode_FromString(WEECHAT_LIST_POS_BEGINNING));
    PyDict_SetItemString(weechat_dict, "WEECHAT_LIST_POS_END", PyUnicode_FromString(WEECHAT_LIST_POS_END));

    PyDict_SetItemString(weechat_dict, "WEECHAT_HOTLIST_LOW", PyUnicode_FromString(WEECHAT_HOTLIST_LOW));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOTLIST_MESSAGE", PyUnicode_FromString(WEECHAT_HOTLIST_MESSAGE));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOTLIST_PRIVATE", PyUnicode_FromString(WEECHAT_HOTLIST_PRIVATE));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOTLIST_HIGHLIGHT", PyUnicode_FromString(WEECHAT_HOTLIST_HIGHLIGHT));

    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_PROCESS_RUNNING", PyLong_FromLong((long) WEECHAT_HOOK_PROCESS_RUNNING));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_PROCESS_ERROR", PyLong_FromLong((long) WEECHAT_HOOK_PROCESS_ERROR));

    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_CONNECT_OK", PyLong_FromLong((long) WEECHAT_HOOK_CONNECT_OK));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND", PyLong_FromLong((long) WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND", PyLong_FromLong((long) WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED", PyLong_FromLong((long) WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_CONNECT_PROXY_ERROR", PyLong_FromLong((long) WEECHAT_HOOK_CONNECT_PROXY_ERROR));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR", PyLong_FromLong((long) WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR", PyLong_FromLong((long) WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR", PyLong_FromLong((long) WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_CONNECT_MEMORY_ERROR", PyLong_FromLong((long) WEECHAT_HOOK_CONNECT_MEMORY_ERROR));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_CONNECT_TIMEOUT", PyLong_FromLong((long) WEECHAT_HOOK_CONNECT_TIMEOUT));

    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_SIGNAL_STRING", PyUnicode_FromString(WEECHAT_HOOK_SIGNAL_STRING));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_SIGNAL_INT", PyUnicode_FromString(WEECHAT_HOOK_SIGNAL_INT));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_SIGNAL_POINTER", PyUnicode_FromString(WEECHAT_HOOK_SIGNAL_POINTER));

#if PY_MAJOR_VERSION >= 3
    return weechat_module;
#endif
}

/*
 * weechat_python_load: load a Python script
 */

int
weechat_python_load (const char *filename)
{
    char *argv[] = { "__weechat_plugin__" , NULL };
#if PY_MAJOR_VERSION >= 3
    wchar_t *wargv[] = { NULL, NULL };
#endif
    FILE *fp;
    PyThreadState *python_current_interpreter;
    PyObject *weechat_outputs, *python_path, *path;
    const char *weechat_home;
    char *str_home;
    int len;

    if ((fp = fopen (filename, "r")) == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, filename);
        return 0;
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
#if PY_MAJOR_VERSION >= 3
    /* python >= 3.x */
    len = strlen (argv[0]);
    wargv[0] = malloc ((len + 1) * sizeof (wargv[0][0]));
    if (wargv[0])
    {
        if (mbstowcs (wargv[0], argv[0], len) == (size_t)(-1))
        {
            free (wargv[0]);
            wargv[0] = NULL;
        }
        PySys_SetArgv(1, wargv);
        if (wargv[0])
            free (wargv[0]);
    }
#else
    /* python <= 2.x */
    PySys_SetArgv(1, argv);
#endif

    if (!python_current_interpreter)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        fclose (fp);
        /* PyEval_ReleaseLock (); */
        return 0;
    }

    PyThreadState_Swap (python_current_interpreter);

    /* adding $weechat_dir/python in $PYTHONPATH */
    python_path = PySys_GetObject ("path");
    weechat_home = weechat_info_get ("weechat_dir", "");
    if (weechat_home)
    {
        len = strlen (weechat_home) + 1 + strlen (PYTHON_PLUGIN_NAME) + 1;
        str_home = malloc (len);
        if (str_home)
        {
            snprintf (str_home, len, "%s/python", weechat_home);
            path = PyBytes_FromString (str_home);
            if (path != NULL)
            {
                PyList_Insert (python_path, 0, path);
                Py_DECREF (path);
            }
            free (str_home);
        }
    }

#if PY_MAJOR_VERSION >= 3
    /* python 3.x (or newer) */
    weechat_outputs = PyModule_Create (&moduleDefOutputs);
#else
    /* python 2.x */
    weechat_outputs = Py_InitModule("weechatOutputs",
                                    weechat_python_output_funcs);
#endif

    if (!weechat_outputs)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to redirect stdout and "
                                         "stderr"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
    }
    else
    {
        if (PySys_SetObject("stdout", weechat_outputs) == -1)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to redirect stdout"),
                            weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        }
        if (PySys_SetObject("stderr", weechat_outputs) == -1)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to redirect stderr"),
                            weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        }
    }

    python_current_script_filename = filename;

    if (PyRun_SimpleFile (fp, filename) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to parse file \"%s\""),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, filename);
        fclose (fp);

        if (PyErr_Occurred ())
            PyErr_Print ();

        /* if script was registered, remove it from list */
        if (python_current_script != NULL)
        {
            plugin_script_remove (weechat_python_plugin,
                                  &python_scripts, &last_python_script,
                                  python_current_script);
        }

        Py_EndInterpreter (python_current_interpreter);
        /* PyEval_ReleaseLock (); */

        return 0;
    }

    if (PyErr_Occurred ())
        PyErr_Print ();

    fclose (fp);

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

        return 0;
    }
    python_current_script = python_registered_script;

    python_current_script->interpreter = (PyThreadState *) python_current_interpreter;
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

    weechat_hook_signal_send ("python_script_loaded", WEECHAT_HOOK_SIGNAL_STRING,
                              python_current_script->filename);

    return 1;
}

/*
 * weechat_python_load_cb: callback for script_auto_load() function
 */

void
weechat_python_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;

    weechat_python_load (filename);
}

/*
 * weechat_python_unload: unload a Python script
 */

void
weechat_python_unload (struct t_plugin_script *script)
{
    int *rc;
    void *interpreter;
    PyThreadState *old_interpreter;
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
        if (rc)
            free (rc);
    }

    filename = strdup (script->filename);
    old_interpreter = PyThreadState_Swap (NULL);
    interpreter = script->interpreter;

    if (python_current_script == script)
        python_current_script = (python_current_script->prev_script) ?
            python_current_script->prev_script : python_current_script->next_script;

    plugin_script_remove (weechat_python_plugin, &python_scripts, &last_python_script,
                          script);

    if (interpreter)
    {
        PyThreadState_Swap (interpreter);
        Py_EndInterpreter (interpreter);
    }

    if (old_interpreter)
        PyThreadState_Swap (old_interpreter);

    weechat_hook_signal_send ("python_script_unloaded",
                              WEECHAT_HOOK_SIGNAL_STRING, filename);
    if (filename)
        free (filename);
}

/*
 * weechat_python_unload_name: unload a Python script by name
 */

void
weechat_python_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;

    ptr_script = plugin_script_search (weechat_python_plugin, python_scripts, name);
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
 * weechat_python_unload_all: unload all Python scripts
 */

void
weechat_python_unload_all ()
{
    while (python_scripts)
    {
        weechat_python_unload (python_scripts);
    }
}

/*
 * weechat_python_reload_name: reload a Python script by name
 */

void
weechat_python_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;

    ptr_script = plugin_script_search (weechat_python_plugin, python_scripts, name);
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
            weechat_python_load (filename);
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
 * weechat_python_cmd: callback for "/python" command
 */

int
weechat_python_command_cb (void *data, struct t_gui_buffer *buffer,
                           int argc, char **argv, char **argv_eol)
{
    char *ptr_name, *path_script;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        plugin_script_display_list (weechat_python_plugin, python_scripts,
                                    NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_python_plugin, python_scripts,
                                        NULL, 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_python_plugin, python_scripts,
                                        NULL, 1);
        }
        else if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            plugin_script_auto_load (weechat_python_plugin, &weechat_python_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_python_unload_all ();
            plugin_script_auto_load (weechat_python_plugin, &weechat_python_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            weechat_python_unload_all ();
        }
    }
    else
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            plugin_script_display_list (weechat_python_plugin, python_scripts,
                                        argv_eol[2], 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            plugin_script_display_list (weechat_python_plugin, python_scripts,
                                        argv_eol[2], 1);
        }
        else if ((weechat_strcasecmp (argv[1], "load") == 0)
                 || (weechat_strcasecmp (argv[1], "reload") == 0)
                 || (weechat_strcasecmp (argv[1], "unload") == 0))
        {
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
            if (weechat_strcasecmp (argv[1], "load") == 0)
            {
                /* load Python script */
                path_script = plugin_script_search_path (weechat_python_plugin,
                                                         ptr_name);
                weechat_python_load ((path_script) ? path_script : ptr_name);
                if (path_script)
                    free (path_script);
            }
            else if (weechat_strcasecmp (argv[1], "reload") == 0)
            {
                /* reload one Python script */
                weechat_python_reload_name (ptr_name);
            }
            else if (weechat_strcasecmp (argv[1], "unload") == 0)
            {
                /* unload Python script */
                weechat_python_unload_name (ptr_name);
            }
            python_quiet = 0;
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unknown option for "
                                             "command \"%s\""),
                            weechat_prefix ("error"), PYTHON_PLUGIN_NAME,
                            "python");
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_python_completion_cb: callback for script completion
 */

int
weechat_python_completion_cb (void *data, const char *completion_item,
                              struct t_gui_buffer *buffer,
                              struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    plugin_script_completion (weechat_python_plugin, completion, python_scripts);

    return WEECHAT_RC_OK;
}

/*
 * weechat_python_info_cb: callback for info
 */

const char *
weechat_python_info_cb (void *data, const char *info_name,
                        const char *arguments)
{
    int rc;
    struct stat stat_buf;

    /* make C compiler happy */
    (void) data;
    (void) arguments;

    if (weechat_strcasecmp (info_name, "python2_bin") == 0)
    {
        if (python2_bin && (strcmp (python2_bin, "python") != 0))
        {
            rc = stat (python2_bin, &stat_buf);
            if ((rc != 0) || (!S_ISREG(stat_buf.st_mode)))
            {
                free (python2_bin);
                weechat_python_set_python2_bin ();
            }
        }
        return python2_bin;
    }

    return NULL;
}

/*
 * weechat_python_hdata_cb: callback for hdata
 */

struct t_hdata *
weechat_python_hdata_cb (void *data, const char *hdata_name)
{
    /* make C compiler happy */
    (void) data;

    return plugin_script_hdata_script (weechat_plugin,
                                       &python_scripts, &last_python_script,
                                       hdata_name);
}

/*
 * weechat_python_infolist_cb: callback for infolist
 */

struct t_infolist *
weechat_python_infolist_cb (void *data, const char *infolist_name,
                            void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (weechat_strcasecmp (infolist_name, "python_script") == 0)
    {
        return plugin_script_infolist_list_scripts (weechat_python_plugin,
                                                    python_scripts, pointer,
                                                    arguments);
    }

    return NULL;
}

/*
 * weechat_python_signal_debug_dump_cb: dump Python plugin data in WeeChat log
 *                                      file
 */

int
weechat_python_signal_debug_dump_cb (void *data, const char *signal,
                                     const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (weechat_strcasecmp ((char *)signal_data, PYTHON_PLUGIN_NAME) == 0))
    {
        plugin_script_print_log (weechat_python_plugin, python_scripts);
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_python_signal_buffer_closed_cb: callback called when a buffer is
 *                                         closed
 */

int
weechat_python_signal_buffer_closed_cb (void *data, const char *signal,
                                        const char *type_data,
                                        void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (signal_data)
        plugin_script_remove_buffer_callbacks (python_scripts, signal_data);

    return WEECHAT_RC_OK;
}

/*
 * weechat_python_timer_action_cb: timer for executing actions
 */

int
weechat_python_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &python_action_install_list)
        {
            plugin_script_action_install (weechat_python_plugin,
                                          python_scripts,
                                          &weechat_python_unload,
                                          &weechat_python_load,
                                          &python_quiet,
                                          &python_action_install_list);
        }
        else if (data == &python_action_remove_list)
        {
            plugin_script_action_remove (weechat_python_plugin,
                                         python_scripts,
                                         &weechat_python_unload,
                                         &python_quiet,
                                         &python_action_remove_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_python_signal_script_action_cb: callback called when a script action
 *                                         is asked (install/remove a script)
 */

int
weechat_python_signal_script_action_cb (void *data, const char *signal,
                                        const char *type_data,
                                        void *signal_data)
{
    /* make C compiler happy */
    (void) data;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "python_script_install") == 0)
        {
            plugin_script_action_add (&python_action_install_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_python_timer_action_cb,
                                &python_action_install_list);
        }
        else if (strcmp (signal, "python_script_remove") == 0)
        {
            plugin_script_action_add (&python_action_remove_list,
                                      (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_python_timer_action_cb,
                                &python_action_remove_list);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize Python plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    struct t_plugin_script_init init;

    weechat_python_plugin = plugin;

    /*
     * hook info to get path to python 2.x interpreter
     * (some scripts using hook_process need that)
     */
    weechat_python_set_python2_bin ();
    weechat_hook_info ("python2_bin",
                       N_("path to python 2.x interpreter"),
                       NULL,
                       &weechat_python_info_cb, NULL);

    /* init stdout/stderr buffer */
    python_buffer_output[0] = '\0';

    PyImport_AppendInittab("weechat",
                           &weechat_python_init_module_weechat);

    Py_Initialize ();
    if (Py_IsInitialized () == 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to launch global "
                                         "interpreter"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    /* PyEval_InitThreads(); */
    /* python_mainThreadState = PyThreadState_Swap(NULL); */
    python_mainThreadState = PyEval_SaveThread();
    /* PyEval_ReleaseLock (); */

    if (!python_mainThreadState)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to get current "
                                         "interpreter state"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    init.callback_command = &weechat_python_command_cb;
    init.callback_completion = &weechat_python_completion_cb;
    init.callback_hdata = &weechat_python_hdata_cb;
    init.callback_infolist = &weechat_python_infolist_cb;
    init.callback_signal_debug_dump = &weechat_python_signal_debug_dump_cb;
    init.callback_signal_buffer_closed = &weechat_python_signal_buffer_closed_cb;
    init.callback_signal_script_action = &weechat_python_signal_script_action_cb;
    init.callback_load_file = &weechat_python_load_cb;

    python_quiet = 1;
    plugin_script_init (weechat_python_plugin, argc, argv, &init);
    python_quiet = 0;

    plugin_script_display_short_list (weechat_python_plugin,
                                      python_scripts);

    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Python interface
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* unload all scripts */
    python_quiet = 1;
    plugin_script_end (plugin, &python_scripts, &weechat_python_unload_all);
    python_quiet = 0;

    /* free Python interpreter */
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
    if (python2_bin)
        free (python2_bin);
    if (python_action_install_list)
        free (python_action_install_list);
    if (python_action_remove_list)
        free (python_action_remove_list);

    return WEECHAT_RC_OK;
}
