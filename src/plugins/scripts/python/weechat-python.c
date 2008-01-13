/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* weechat-python.c: Python plugin for WeeChat */

#undef _

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Python.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "weechat-python.h"
#include "weechat-python-api.h"


WEECHAT_PLUGIN_NAME("python");
WEECHAT_PLUGIN_DESCRIPTION("Python plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL");

struct t_weechat_plugin *weechat_python_plugin = NULL;

struct t_plugin_script *python_scripts = NULL;
struct t_plugin_script *python_current_script = NULL;
char *python_current_script_filename = NULL;
PyThreadState *python_mainThreadState = NULL;

char python_buffer_output[128];


/*
 * weechat_python_exec: execute a Python script
 */

void *
weechat_python_exec (struct t_plugin_script *script,
		     int ret_type, char *function, char **argv)
{
    PyObject *evMain;
    PyObject *evDict;
    PyObject *evFunc;
    PyObject *rc;
    void *ret_value;
    int *ret_i;
    
    /* PyEval_AcquireLock (); */
    PyThreadState_Swap (script->interpreter);
    
    evMain = PyImport_AddModule ((char *) "__main__");
    evDict = PyModule_GetDict (evMain);
    evFunc = PyDict_GetItemString (evDict, function);
    
    if ( !(evFunc && PyCallable_Check (evFunc)) )
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s unable to run function \"%s\""),
                        weechat_prefix ("error"), "python", function);
	/* PyEval_ReleaseThread (python_current_script->interpreter); */
	return NULL;
    }
    
    python_current_script = script;
    
    if (argv && argv[0])
    {
        if (argv[1])
        {
            if (argv[2])
            {
                if (argv[3])
                {
                    if (argv[4])
                    {
                        rc = PyObject_CallFunction (evFunc, "sssss", argv[0],
                                                    argv[1], argv[2], argv[3],
                                                    argv[4]);
                    }
                    else
                        rc = PyObject_CallFunction (evFunc, "ssss", argv[0],
                                                    argv[1], argv[2], argv[3]);
                }
                else
                    rc = PyObject_CallFunction (evFunc, "sss", argv[0],
                                                argv[1], argv[2]);
            }
            else
                rc = PyObject_CallFunction (evFunc, "ss", argv[0], argv[1]);
        }
        else
            rc = PyObject_CallFunction (evFunc, "s", argv[0]);
    }
    else
        rc = PyObject_CallFunction (evFunc, NULL);
    
    ret_value = NULL;
    
    /* 
       ugly hack : rc = NULL while 'return weechat.PLUGIN_RC_OK .... 
       because of '#define PLUGIN_RC_OK 0'
    */
    if (rc ==  NULL)
	rc = PyInt_FromLong (0);
    
    if (PyString_Check (rc) && (ret_type == WEECHAT_SCRIPT_EXEC_STRING))
    {
	if (PyString_AsString (rc))
	    ret_value = strdup (PyString_AsString(rc));
	else
	    ret_value = NULL;
	
	Py_XDECREF(rc);
    }
    else if (PyInt_Check (rc) && (ret_type == WEECHAT_SCRIPT_EXEC_INT))
    {
	
	ret_i = (int *)malloc (sizeof(int));
	if (ret_i)
	    *ret_i = (int) PyInt_AsLong(rc);
	ret_value = ret_i;
	
	Py_XDECREF(rc);
    }
    else
    {
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"%s\" must return "
                                         "a valid value"),
                        weechat_prefix ("error"), "python", function);
	/* PyEval_ReleaseThread (python_current_script->interpreter); */
	return NULL;
    }
    
    if (ret_value == NULL)
    {
	weechat_printf (NULL,
                        weechat_gettext ("%s%s: not enough memory for "
                                         "function \"%s\""),
                        weechat_prefix ("error"), "python", function);
	/* PyEval_ReleaseThread (python_current_script->interpreter); */
	return NULL;
    }
    
    if (PyErr_Occurred ()) PyErr_Print ();
    
    /* PyEval_ReleaseThread (python_current_script->interpreter); */
    
    return ret_value;
}

/*
 * weechat_python_output : redirection for stdout and stderr
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
                            "python", python_buffer_output, "");
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
                                "python", python_buffer_output, m);
            }
	    *p = '\n';
	    python_buffer_output[0] = '\0';
	    m = ++p;
	}
	
	if (strlen(m) + strlen(python_buffer_output) > sizeof(python_buffer_output))
	{
	    weechat_printf (NULL,
                            weechat_gettext ("%s: stdout/stderr : %s%s"),
                            "python", python_buffer_output, m);
	    python_buffer_output[0] = '\0';
	}
	else
	    strcat (python_buffer_output, m);
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

/*
 * Outputs subroutines
 */

static
PyMethodDef weechat_python_output_funcs[] = {
    { "write", weechat_python_output, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL }
};

/*
 * weechat_python_load: load a Python script
 */

int
weechat_python_load (char *filename)
{
    char *argv[] = { "__weechat_plugin__" , NULL };
    FILE *fp;
    PyThreadState *python_current_interpreter;
    PyObject *weechat_module, *weechat_outputs, *weechat_dict;
    PyObject *python_path, *path;
    char *w_home, *p_home;
    int len;
    
    if ((fp = fopen (filename, "r")) == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), "python", filename);
        return 0;
    }
    
    weechat_printf (NULL,
                    weechat_gettext ("%s%s: loading script \"%s\""),
                    weechat_prefix ("info"), "python", filename);
    
    python_current_script = NULL;
    
    /* PyEval_AcquireLock (); */
    python_current_interpreter = Py_NewInterpreter ();
    PySys_SetArgv(1, argv);
    
    if (python_current_interpreter == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to create new "
                                         "sub-interpreter"),
                        weechat_prefix ("error"), "python");
        fclose (fp);
	/* PyEval_ReleaseLock (); */
	return 0;
    }
    
    /* PyThreadState_Swap (python_current_interpreter); */
    
    weechat_module = Py_InitModule ("weechat", weechat_python_funcs);

    if ( weechat_module == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to initialize WeeChat "
                                         "module"),
                        weechat_prefix ("error"), "python");
        fclose (fp);
	
	Py_EndInterpreter (python_current_interpreter);
        /* PyEval_ReleaseLock (); */
        
	return 0;
    }

    /* adding $weechat_dir/python in $PYTHONPATH */    
    python_path = PySys_GetObject ("path");
    w_home = weechat_info_get ("weechat_dir");
    if (w_home)
    {
	len = strlen (w_home) + 1 + strlen("python") + 1;
	p_home = (char *)malloc (len * sizeof(char));
	if (p_home)
	{
	    snprintf (p_home, len, "%s/python", w_home);
	    path = PyString_FromString (p_home);
	    if (path != NULL)
	    {
		PyList_Insert (python_path, 0, path);
		Py_DECREF (path);
	    }
	    free (p_home);
	}
    }
    
    /* define some constants */
    weechat_dict = PyModule_GetDict(weechat_module);
    PyDict_SetItemString(weechat_dict, "WEECHAT_RC_OK", PyInt_FromLong((long) WEECHAT_RC_OK));
    PyDict_SetItemString(weechat_dict, "WEECHAT_RC_ERROR", PyInt_FromLong((long) WEECHAT_RC_ERROR));
    PyDict_SetItemString(weechat_dict, "WEECHAT_RC_OK_IGNORE_WEECHAT", PyInt_FromLong((long) WEECHAT_RC_OK_IGNORE_WEECHAT));
    PyDict_SetItemString(weechat_dict, "WEECHAT_RC_OK_IGNORE_PLUGINS", PyInt_FromLong((long) WEECHAT_RC_OK_IGNORE_PLUGINS));
    PyDict_SetItemString(weechat_dict, "WEECHAT_RC_OK_IGNORE_ALL", PyInt_FromLong((long) WEECHAT_RC_OK_IGNORE_ALL));
    PyDict_SetItemString(weechat_dict, "WEECHAT_RC_OK_WITH_HIGHLIGHT", PyInt_FromLong((long) WEECHAT_RC_OK_WITH_HIGHLIGHT));
    PyDict_SetItemString(weechat_dict, "WEECHAT_LIST_POS_SORT", PyString_FromString(WEECHAT_LIST_POS_SORT));
    PyDict_SetItemString(weechat_dict, "WEECHAT_LIST_POS_BEGINNING", PyString_FromString(WEECHAT_LIST_POS_BEGINNING));
    PyDict_SetItemString(weechat_dict, "WEECHAT_LIST_POS_END", PyString_FromString(WEECHAT_LIST_POS_END));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOTLIST_LOW", PyString_FromString(WEECHAT_HOTLIST_LOW));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOTLIST_MESSAGE", PyString_FromString(WEECHAT_HOTLIST_MESSAGE));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOTLIST_PRIVATE", PyString_FromString(WEECHAT_HOTLIST_PRIVATE));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOTLIST_HIGHLIGHT", PyString_FromString(WEECHAT_HOTLIST_HIGHLIGHT));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_SIGNAL_STRING", PyString_FromString(WEECHAT_HOOK_SIGNAL_STRING));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_SIGNAL_INT", PyString_FromString(WEECHAT_HOOK_SIGNAL_INT));
    PyDict_SetItemString(weechat_dict, "WEECHAT_HOOK_SIGNAL_POINTER", PyString_FromString(WEECHAT_HOOK_SIGNAL_POINTER));
    
    weechat_outputs = Py_InitModule("weechatOutputs", weechat_python_output_funcs);
    if (weechat_outputs == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to redirect stdout and "
                                         "stderr"),
                        weechat_prefix ("error"), "python");
    }
    else
    {
	if (PySys_SetObject("stdout", weechat_outputs) == -1)
        {
	    weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to redirect stdout"),
                            weechat_prefix ("error"), "python");
        }
	if (PySys_SetObject("stderr", weechat_outputs) == -1)
        {
	    weechat_printf (NULL,
                            weechat_gettext ("%s%s: unable to redirect stderr"),
                            weechat_prefix ("error"), "python");
        }
    }
    
    python_current_script_filename = filename;
    
    if (PyRun_SimpleFile (fp, filename) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to parse file \"%s\""),
                        weechat_prefix ("error"), "python", filename);
	fclose (fp);
	
	if (PyErr_Occurred ()) PyErr_Print ();
	Py_EndInterpreter (python_current_interpreter);
	/* PyEval_ReleaseLock (); */
        
	/* if script was registered, removing from list */
	if (python_current_script != NULL)
        {
	    script_remove (weechat_python_plugin,
                           &python_scripts, python_current_script);
        }
        return 0;
    }

    if (PyErr_Occurred ()) PyErr_Print ();

    fclose (fp);
    
    if (python_current_script == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), "python", filename);
	
	if (PyErr_Occurred ()) PyErr_Print ();
	Py_EndInterpreter (python_current_interpreter);
	/* PyEval_ReleaseLock (); */
        
	return 0;
    }
    
    python_current_script->interpreter = (PyThreadState *) python_current_interpreter;
    /* PyEval_ReleaseThread (python_current_script->interpreter); */
    
    return 1;
}

/*
 * weechat_python_load_cb: callback for script_auto_load() function
 */

int
weechat_python_load_cb (void *data, char *filename)
{
    /* make C compiler happy */
    (void) data;
    
    return weechat_python_load (filename);
}

/*
 * weechat_python_unload: unload a Python script
 */

void
weechat_python_unload (struct t_plugin_script *script)
{
    int *r;
    
    weechat_printf (NULL,
                    weechat_gettext ("%s%s: unloading script \"%s\""),
                    weechat_prefix ("info"), "python", script->name);
    
    if (script->shutdown_func && script->shutdown_func[0])
    {
        r = (int *) weechat_python_exec (script, WEECHAT_SCRIPT_EXEC_INT,
					 script->shutdown_func, NULL);
	if (r)
	    free (r);
    }

    PyThreadState_Swap (script->interpreter);
    Py_EndInterpreter (script->interpreter);
    
    script_remove (weechat_python_plugin, &python_scripts, script);
}

/*
 * weechat_python_unload_name: unload a Python script by name
 */

void
weechat_python_unload_name (char *name)
{
    struct t_plugin_script *ptr_script;
    
    ptr_script = script_search (weechat_python_plugin, python_scripts, name);
    if (ptr_script)
    {
        weechat_python_unload (ptr_script);
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" unloaded"),
                        weechat_prefix ("info"), "python", name);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), "python", name);
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
 * weechat_python_cmd: callback for "/python" command
 */

int
weechat_python_command_cb (void *data, struct t_gui_buffer *buffer,
                           int argc, char **argv, char **argv_eol)
{
    //int handler_found, modifier_found;
    char *path_script;
    struct t_plugin_script *ptr_script;
    //struct t_plugin_handler *ptr_handler;
    //struct t_plugin_modifier *ptr_modifier;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        /* list registered Python scripts */
        weechat_printf (NULL, "");
        weechat_printf (NULL,
                        weechat_gettext ("Registered %s scripts:"),
                        "python");
        if (python_scripts)
        {
            for (ptr_script = python_scripts; ptr_script;
                 ptr_script = ptr_script->next_script)
            {
                weechat_printf (NULL,
                                weechat_gettext ("  %s v%s (%s), by %s, "
                                                 "license %s"),
                                ptr_script->name,
                                ptr_script->version,
                                ptr_script->description,
                                ptr_script->author,
                                ptr_script->license);
            }
        }
        else
            weechat_printf (NULL, weechat_gettext ("  (none)"));
        
        /*
        // list Python message handlers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Python message handlers:");
        handler_found = 0;
        for (ptr_handler = plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_MESSAGE)
                && (ptr_handler->handler_args))
            {
                handler_found = 1;
                plugin->print_server (plugin, "  IRC(%s) => Python(%s)",
                                      ptr_handler->irc_command,
                                      ptr_handler->handler_args);
            }
        }
        if (!handler_found)
            plugin->print_server (plugin, "  (none)");
        
        // list Python command handlers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Python command handlers:");
        handler_found = 0;
        for (ptr_handler = plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_COMMAND)
                && (ptr_handler->handler_args))
            {
                handler_found = 1;
                plugin->print_server (plugin, "  /%s => Python(%s)",
                                      ptr_handler->command,
                                      ptr_handler->handler_args);
            }
        }
        if (!handler_found)
            plugin->print_server (plugin, "  (none)");
        
        // list Python timer handlers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Python timer handlers:");
        handler_found = 0;
        for (ptr_handler = plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_TIMER)
                && (ptr_handler->handler_args))
            {
                handler_found = 1;
                plugin->print_server (plugin, "  %d seconds => Python(%s)",
                                      ptr_handler->interval,
                                      ptr_handler->handler_args);
            }
        }
        if (!handler_found)
            plugin->print_server (plugin, "  (none)");
        
        // list Python keyboard handlers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Python keyboard handlers:");
        handler_found = 0;
        for (ptr_handler = plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_KEYBOARD)
                && (ptr_handler->handler_args))
            {
                handler_found = 1;
                plugin->print_server (plugin, "  Python(%s)",
                                      ptr_handler->handler_args);
            }
        }
        if (!handler_found)
            plugin->print_server (plugin, "  (none)");
        
        // list Python event handlers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Python event handlers:");
        handler_found = 0;
        for (ptr_handler = plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == PLUGIN_HANDLER_EVENT)
                && (ptr_handler->handler_args))
            {
                handler_found = 1;
                plugin->print_server (plugin, "  %s => Python(%s)",
                                      ptr_handler->event,
                                      ptr_handler->handler_args);
            }
        }
        if (!handler_found)
            plugin->print_server (plugin, "  (none)");
        
        // list Python modifiers
        plugin->print_server (plugin, "");
        plugin->print_server (plugin, "Python modifiers:");
        modifier_found = 0;
        for (ptr_modifier = plugin->modifiers;
             ptr_modifier; ptr_modifier = ptr_modifier->next_modifier)
        {
            modifier_found = 1;
            if (ptr_modifier->type == PLUGIN_MODIFIER_IRC_IN)
                plugin->print_server (plugin, "  IRC(%s, %s) => Python(%s)",
                                      ptr_modifier->command,
                                      PLUGIN_MODIFIER_IRC_IN_STR,
                                      ptr_modifier->modifier_args);
            else if (ptr_modifier->type == PLUGIN_MODIFIER_IRC_USER)
                plugin->print_server (plugin, "  IRC(%s, %s) => Python(%s)",
                                      ptr_modifier->command,
                                      PLUGIN_MODIFIER_IRC_USER_STR,
                                      ptr_modifier->modifier_args);
            else if (ptr_modifier->type == PLUGIN_MODIFIER_IRC_OUT)
                plugin->print_server (plugin, "  IRC(%s, %s) => Python(%s)",
                                      ptr_modifier->command,
                                      PLUGIN_MODIFIER_IRC_OUT_STR,
                                      ptr_modifier->modifier_args);
        }
        if (!modifier_found)
            plugin->print_server (plugin, "  (none)");
        */
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            script_auto_load (weechat_python_plugin,
                              "python", &weechat_python_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_python_unload_all ();
            script_auto_load (weechat_python_plugin,
                              "python", &weechat_python_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            weechat_python_unload_all ();
        }
    }
    else
    {
        if (weechat_strcasecmp (argv[1], "load") == 0)
        {
            /* load Python script */
            path_script = script_search_full_name (weechat_python_plugin,
                                                   "python", argv_eol[2]);
            weechat_python_load ((path_script) ? path_script : argv_eol[2]);
            if (path_script)
                free (path_script);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            /* unload Python script */
            weechat_python_unload_name (argv_eol[2]);
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unknown option for "
                                             "command \"%s\""),
                            weechat_prefix ("error"), "python", "python");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_python_dump_data_cb: dump Python plugin data in WeeChat log file
 */

int
weechat_python_dump_data_cb (void *data, char *signal, char *type_data,
                             void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    script_print_log (weechat_python_plugin, python_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize Python plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin)
{
    weechat_python_plugin = plugin;
    
    /* init stdout/stderr buffer */
    python_buffer_output[0] = '\0';
    
    Py_Initialize ();
    if (Py_IsInitialized () == 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to launch global "
                                         "interpreter"),
                        weechat_prefix ("error"), "python");
        return WEECHAT_RC_ERROR;
    }
    
    PyEval_InitThreads();
    /* python_mainThreadState = PyThreadState_Swap(NULL); */
    python_mainThreadState = PyEval_SaveThread();
    /* PyEval_ReleaseLock (); */

    if (python_mainThreadState == NULL)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to get current "
                                         "interpreter state"),
                        weechat_prefix ("error"), "python");
        return WEECHAT_RC_ERROR;
    }
    
    weechat_hook_command ("python",
                          weechat_gettext ("list/load/unload Python scripts"),
                          weechat_gettext ("[load filename] | [autoload] | "
                                           "[reload] | [unload [script]]"),
                          weechat_gettext ("filename: Python script (file) to "
                                           "load\n"
                                           "script: script name to unload\n\n"
                                           "Without argument, /python command "
                                           "lists all loaded Python scripts."),
                          "load|autoload|reload|unload %f",
                          &weechat_python_command_cb, NULL);
    
    weechat_mkdir_home ("python", 0644);
    weechat_mkdir_home ("python/autoload", 0644);
    
    weechat_hook_signal ("dump_data", &weechat_python_dump_data_cb, NULL);
    
    script_init (weechat_python_plugin);
    script_auto_load (weechat_python_plugin, "python", &weechat_python_load_cb);
    
    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Python interface
 */

void
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    /* unload all scripts */
    weechat_python_unload_all ();
    
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
                        weechat_prefix ("error"), "python");
    }
}
