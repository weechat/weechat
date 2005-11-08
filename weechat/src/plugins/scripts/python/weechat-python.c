/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* weechat-python.c: Python plugin support for WeeChat */


#include <Python.h>
#include <stdlib.h>
#include <string.h>
#undef _
#include "../../weechat-plugin.h"
#include "../weechat-script.h"


char plugin_name[]        = "Python";
char plugin_version[]     = "0.1";
char plugin_description[] = "Python scripts support";

t_weechat_plugin *python_plugin;

t_plugin_script *python_scripts = NULL;
t_plugin_script *python_current_script = NULL;
char *python_current_script_filename = NULL;
PyThreadState *python_mainThreadState = NULL;


/*
 * weechat_python_exec: execute a Python script
 */

int
weechat_python_exec (t_weechat_plugin *plugin,
                     t_plugin_script *script,
                     char *function, char *server, char *arguments)
{
    PyObject *evMain;
    PyObject *evDict;
    PyObject *evFunc;
    PyObject *rc;
    int ret;

    PyThreadState_Swap (script->interpreter);
    
    evMain = PyImport_AddModule ((char *) "__main__");
    evDict = PyModule_GetDict (evMain);
    evFunc = PyDict_GetItemString (evDict, function);
    
    if ( !(evFunc && PyCallable_Check (evFunc)) )
    {
        plugin->printf_server (plugin,
                               "Python error: unable to run function \"%s\"",
                               function);
        return PLUGIN_RC_KO;
    }
    
    ret = -1;

    rc = PyObject_CallFunction(evFunc, "ss", server == NULL ? "" : server, arguments == NULL ? "" : arguments);

    if (rc)
    {
        ret = (int) PyInt_AsLong(rc);
        Py_XDECREF(rc);
    }
        
    if (ret < 0)
        return PLUGIN_RC_OK;
    else
        return ret;
}

/*
 * weechat_python_handler: general message and command handler for Python
 */

int
weechat_python_handler (t_weechat_plugin *plugin,
                        char *server, char *command, char *arguments,
                        char *handler_args, void *handler_pointer)
{
    /* make gcc happy */
    (void) command;
    
    return weechat_python_exec (plugin, (t_plugin_script *)handler_pointer,
                                handler_args, server, arguments);
}

/*
 * weechat.register: startup function for all WeeChat Python scripts
 */

static PyObject *
weechat_python_register (PyObject *self, PyObject *args)
{
    char *name, *version, *shutdown_func, *description;
    
    /* make gcc happy */
    (void) self;
    
    name = NULL;
    version = NULL;
    shutdown_func = NULL;
    description = NULL;
    
    if (!PyArg_ParseTuple (args, "ssss", &name, &version, &shutdown_func, &description))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"register\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (weechat_script_search (python_plugin, &python_scripts, name))
    {
        /* error: another scripts already exists with this name! */
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to register "
                                      "\"%s\" script (another script "
                                      "already exists with this name)",
                                      name);
        return Py_BuildValue ("i", 0);
    }
    
    /* register script */
    python_current_script = weechat_script_add (python_plugin,
                                                &python_scripts,
                                                (python_current_script_filename) ?
                                                python_current_script_filename : "",
                                                name, version, shutdown_func,
                                                description);
    if (python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python: registered script \"%s\", "
                                      "version %s (%s)",
                                      name, version, description);
    }
    else
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to load script "
                                      "\"%s\" (not enough memory)",
                                      name);
        return Py_BuildValue ("i", 0);
    }
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat.print: print message into a buffer (current or specified one)
 */

static PyObject *
weechat_python_print (PyObject *self, PyObject *args)
{
    char *message, *channel_name, *server_name;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to print message, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    message = NULL;
    channel_name = NULL;
    server_name = NULL;
    
    if (!PyArg_ParseTuple (args, "s|ss", &message, &channel_name, &server_name))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"print\" function");
        return Py_BuildValue ("i", 0);
    }
    
    python_plugin->printf (python_plugin,
                           server_name, channel_name,
                           "%s", message);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat.print_infobar: print message to infobar
 */

static PyObject *
weechat_python_print_infobar (PyObject *self, PyObject *args)
{
    int delay;
    char *message;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to print infobar message, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    delay = 1;
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "is", &delay, &message))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"print_infobar\" function");
        return Py_BuildValue ("i", 0);
    }
    
    python_plugin->infobar_printf (python_plugin, delay, message);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat.command: send command to server
 */

static PyObject *
weechat_python_command (PyObject *self, PyObject *args)
{
    char *command, *channel_name, *server_name;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to run command, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    command = NULL;
    channel_name = NULL;
    server_name = NULL;
    
    if (!PyArg_ParseTuple (args, "s|ss", &command, &channel_name, &server_name))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"command\" function");
        return Py_BuildValue ("i", 0);
    }

    python_plugin->exec_command (python_plugin,
                                 server_name, channel_name,
                                 command);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat.add_message_handler: add handler for messages
 */

static PyObject *
weechat_python_add_message_handler (PyObject *self, PyObject *args)
{
    char *irc_command, *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to add message handler, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    irc_command = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &irc_command, &function))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"add_message_handler\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (python_plugin->msg_handler_add (python_plugin, irc_command,
                                        weechat_python_handler, function,
                                        (void *)python_current_script))
        return Py_BuildValue ("i", 1);
    
    return Py_BuildValue ("i", 0);
}

/*
 * weechat.add_command_handler: define/redefines commands
 */

static PyObject *
weechat_python_add_command_handler (PyObject *self, PyObject *args)
{
    char *command, *function, *description, *arguments, *arguments_description;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to add command handler, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    command = NULL;
    function = NULL;
    description = NULL;
    arguments = NULL;
    arguments_description = NULL;
    
    if (!PyArg_ParseTuple (args, "ss|sss", &command, &function,
                           &description, &arguments, &arguments_description))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"add_command_handler\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (python_plugin->cmd_handler_add (python_plugin,
                                        command,
                                        description,
                                        arguments,
                                        arguments_description,
                                        weechat_python_handler,
                                        function,
                                        (void *)python_current_script))
        return Py_BuildValue ("i", 1);
    
    return Py_BuildValue ("i", 0);
}

/*
 * weechat.remove_handler: remove a handler
 */

static PyObject *
weechat_python_remove_handler (PyObject *self, PyObject *args)
{
    char *command, *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to remove handler, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    command = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &command, &function))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"remove_handler\" function");
        return Py_BuildValue ("i", 0);
    }
    
    weechat_script_remove_handler (python_plugin, python_current_script,
                                   command, function);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat.get_info: get various infos
 */

static PyObject *
weechat_python_get_info (PyObject *self, PyObject *args)
{
    char *arg, *server_name, *info;
    PyObject *object;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to get info, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    arg = NULL;
    server_name = NULL;
    
    if (!PyArg_ParseTuple (args, "s|s", &arg, &server_name))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"get_info\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (arg)
    {
        info = python_plugin->get_info (python_plugin, arg, server_name);
        
        if (info)
        {
            object = Py_BuildValue ("s", info);
            free (info);
            return object;
        }
    }
    
    return Py_BuildValue ("s", "");
}

/*
 * weechat.get_dcc_info: get infos about DCC
 */

static PyObject *
weechat_python_get_dcc_info (PyObject *self, PyObject *args)
{
    t_plugin_dcc_info *dcc_info, *ptr_dcc;
    int dcc_count;
    PyObject *list, *listvalue;
    
    /* make gcc happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to get DCC info, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    dcc_info = python_plugin->get_dcc_info (python_plugin);
    dcc_count = 0;
    
    if (!dcc_info)
        return Py_BuildValue ("i", 0);
    
    for (ptr_dcc = dcc_info; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        dcc_count++;
    }
    
    list = PyList_New (dcc_count);
    
    if (!list)
    {
        python_plugin->free_dcc_info (python_plugin, dcc_info);
        return Py_BuildValue ("i", 0);
    }
    
    dcc_count = 0;
    for(ptr_dcc = dcc_info; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
        listvalue = Py_BuildValue ("{s:s,s:s,s:i,s:i,s:k,s:k,s:k,s:i,s:s,s:s,"
                                   "s:s,s:s,s:k,s:k,s:k,s:k}",
                                   "server",          ptr_dcc->server,
                                   "channel",         ptr_dcc->channel,
                                   "type",            ptr_dcc->type,
                                   "status",          ptr_dcc->status,
                                   "start_time",      ptr_dcc->start_time,
                                   "start_transfer",  ptr_dcc->start_transfer,
                                   "address",         ptr_dcc->addr,
                                   "port",            ptr_dcc->port,
                                   "nick",            ptr_dcc->nick,
                                   "remote_file",     ptr_dcc->filename,
                                   "local_file",      ptr_dcc->local_filename,
                                   "filename_suffix", ptr_dcc->filename_suffix,
                                   "size",            ptr_dcc->size,
                                   "pos",             ptr_dcc->pos,
                                   "start_resume",    ptr_dcc->start_resume,
                                   "cps",             ptr_dcc->bytes_per_sec);
        if (listvalue) 
        {
            if (PyList_SetItem (list, dcc_count, listvalue) != 0)
            {
                PyMem_Free (listvalue);
                PyMem_Free (list);
                python_plugin->free_dcc_info (python_plugin, dcc_info);
                return Py_BuildValue ("i", 0);
            }
            PyMem_Free (listvalue);
        }
        else
        {
            python_plugin->free_dcc_info (python_plugin, dcc_info);
            return Py_BuildValue ("i", 0);
        }
        dcc_count++;
    }
    
    python_plugin->free_dcc_info (python_plugin, dcc_info);
    
    return list;
}

/*
 * weechat.get_config: get value of a WeeChat config option
 */

static PyObject *
weechat_python_get_config (PyObject *self, PyObject *args)
{
    char *option, *return_value;
    PyObject *python_return_value;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to get config option, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"get_config\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (option)
    {
        return_value = python_plugin->get_config (python_plugin, option);
        
        if (return_value)
        {
            python_return_value = Py_BuildValue ("s", return_value);
            free (return_value);
            return python_return_value;
        }
    }
    
    return Py_BuildValue ("s", "");
}

/*
 * weechat.set_config: set value of a WeeChat config option
 */

static PyObject *
weechat_python_set_config (PyObject *self, PyObject *args)
{
    char *option, *value;
    
    /* make gcc happy */
    (void) self;

    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to set config option, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    option = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &option, &value))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"set_config\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (option && value)
    {
        if (python_plugin->set_config (python_plugin, option, value))
            return Py_BuildValue ("i", 1);
    }
    
    return Py_BuildValue ("i", 0);
}

/*
 * weechat.get_plugin_config: get value of a plugin config option
 */

static PyObject *
weechat_python_get_plugin_config (PyObject *self, PyObject *args)
{
    char *option, *return_value;
    PyObject *python_return_value;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to get plugin config option, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"get_plugin_config\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (option)
    {
        return_value = weechat_script_get_plugin_config (python_plugin,
                                                         python_current_script,
                                                         option);
        
        if (return_value)
        {
            python_return_value = Py_BuildValue ("s", return_value);
            free (return_value);
            return python_return_value;
        }
    }
    
    return Py_BuildValue ("s", "");
}

/*
 * weechat.set_plugin_config: set value of a plugin config option
 */

static PyObject *
weechat_python_set_plugin_config (PyObject *self, PyObject *args)
{
    char *option, *value;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to set plugin config option, "
                                      "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    option = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &option, &value))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: wrong parameters for "
                                      "\"set_plugin_config\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (option && value)
    {
        if (weechat_script_set_plugin_config (python_plugin,
                                              python_current_script,
                                              option, value))
            return Py_BuildValue ("i", 1);
    }
    
    return Py_BuildValue ("i", 0);
}

/*
 * Python subroutines
 */

static
PyMethodDef weechat_python_funcs[] = {
    { "register", weechat_python_register, METH_VARARGS, "" },
    { "prnt", weechat_python_print, METH_VARARGS, "" },
    { "print_infobar", weechat_python_print_infobar, METH_VARARGS, "" },
    { "command", weechat_python_command, METH_VARARGS, "" },
    { "add_message_handler", weechat_python_add_message_handler, METH_VARARGS, "" },
    { "add_command_handler", weechat_python_add_command_handler, METH_VARARGS, "" },
    { "remove_handler", weechat_python_remove_handler, METH_VARARGS, "" },
    { "get_info", weechat_python_get_info, METH_VARARGS, "" },
    { "get_dcc_info", weechat_python_get_dcc_info, METH_VARARGS, "" },
    { "get_config", weechat_python_get_config, METH_VARARGS, "" },
    { "set_config", weechat_python_set_config, METH_VARARGS, "" },
    { "get_plugin_config", weechat_python_get_plugin_config, METH_VARARGS, "" },
    { "set_plugin_config", weechat_python_set_plugin_config, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL }
};

/*
 * weechat_python_output : redirection for stdout and stderr
 */

static PyObject *
weechat_python_output (PyObject *self, PyObject *args)
{
    char *msg;
    /* make gcc happy */
    (void) self;
    
    msg = NULL;

    if (!PyArg_ParseTuple (args, "s", &msg))
    {
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to get "
                                      "stdout/stderr message(s)");
        return NULL; 
    }
    
    python_plugin->printf_server (python_plugin,
				  "Python stdin/stdout: %s", msg);
    return Py_BuildValue ("i", 1);
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
weechat_python_load (t_weechat_plugin *plugin, char *filename)
{
    char *argv[] = { "__weechat_plugin__" , NULL };
    FILE *fp;
    PyThreadState *python_current_interpreter;
    PyObject *weechat_module, *weechat_outputs, *weechat_dict;
    
    plugin->printf_server (plugin, "Loading Python script \"%s\"", filename);
    
    if ((fp = fopen (filename, "r")) == NULL)
    {
        plugin->printf_server (plugin,
                               "Python error: unable to open file \"%s\"",
                               filename);
        return 0;
    }

    python_current_script = NULL;
    
    python_current_interpreter = Py_NewInterpreter ();
    PySys_SetArgv(1, argv);

    if (python_current_interpreter == NULL)
    {
        plugin->printf_server (plugin,
                               "Python error: unable to create new sub-interpreter");
        fclose (fp);
        return 0;
    }

    PyThreadState_Swap (python_current_interpreter);
    
    weechat_module = Py_InitModule ("weechat", weechat_python_funcs);

    if ( weechat_module == NULL)
    {
        plugin->printf_server (plugin,
                               "Python error: unable to initialize WeeChat module");
        Py_EndInterpreter (python_current_interpreter);
        fclose (fp);
        return 0;
    }

    /* define some constants */
    weechat_dict = PyModule_GetDict(weechat_module);
    PyDict_SetItemString(weechat_dict, "PLUGIN_RC_OK", PyInt_FromLong((long) PLUGIN_RC_OK));
    PyDict_SetItemString(weechat_dict, "PLUGIN_RC_KO", PyInt_FromLong((long) PLUGIN_RC_KO));
    PyDict_SetItemString(weechat_dict, "PLUGIN_RC_OK_IGNORE_WEECHAT", PyInt_FromLong((long) PLUGIN_RC_OK_IGNORE_WEECHAT));
    PyDict_SetItemString(weechat_dict, "PLUGIN_RC_OK_IGNORE_PLUGINS", PyInt_FromLong((long) PLUGIN_RC_OK_IGNORE_PLUGINS));
    PyDict_SetItemString(weechat_dict, "PLUGIN_RC_OK_IGNORE_ALL", PyInt_FromLong((long) PLUGIN_RC_OK_IGNORE_ALL));
    
    weechat_outputs = Py_InitModule("weechatOutputs", weechat_python_output_funcs);
    if (weechat_outputs == NULL)
    {
        plugin->printf_server (plugin,
                               "Python warning: unable to redirect stdout and stderr");
    }
    else
    {
	if (PySys_SetObject("stdout", weechat_outputs) == -1)
	    plugin->printf_server (plugin,
				   "Python warning: unable to redirect stdout");
	if (PySys_SetObject("stderr", weechat_outputs) == -1)
	    plugin->printf_server (plugin,
				   "Python warning: unable to redirect stderr");
    }
	
    python_current_script_filename = strdup (filename);
    
    if (PyRun_SimpleFile (fp, filename) != 0)
    {
        plugin->printf_server (plugin,
                               "Python error: unable to parse file \"%s\"",
                               filename);
        free (python_current_script_filename);
	Py_EndInterpreter (python_current_interpreter);
        fclose (fp);
	/* if script was registered, removing from list */
	if (python_current_script != NULL)
	    weechat_script_remove (plugin, &python_scripts, python_current_script);
        return 0;
    }
    
    fclose (fp);
    free (python_current_script_filename);
    
    if (python_current_script == NULL)
    {
        plugin->printf_server (plugin,
                               "Python error: function \"register\" not found "
                               "in file \"%s\"",
                               filename);
	Py_EndInterpreter (python_current_interpreter);
        return 0;
    }
    
    python_current_script->interpreter = (PyThreadState *) python_current_interpreter;
        
    return 1;
}

/*
 * weechat_python_unload: unload a Python script
 */

void
weechat_python_unload (t_weechat_plugin *plugin, t_plugin_script *script)
{
    plugin->printf_server (plugin,
                           "Unloading Python script \"%s\"",
                           script->name);
    
    if (script->shutdown_func[0])
        weechat_python_exec (plugin, script, script->shutdown_func, "", "");

    PyThreadState_Swap (script->interpreter);
    Py_EndInterpreter (script->interpreter);
    
    weechat_script_remove (plugin, &python_scripts, script);
}

/*
 * weechat_python_unload_name: unload a Python script by name
 */

void
weechat_python_unload_name (t_weechat_plugin *plugin, char *name)
{
    t_plugin_script *ptr_script;
    
    ptr_script = weechat_script_search (plugin, &python_scripts, name);
    if (ptr_script)
    {
        weechat_python_unload (plugin, ptr_script);
        plugin->printf_server (plugin,
                               "Python script \"%s\" unloaded",
                               name);
    }
    else
    {
        plugin->printf_server (plugin,
                               "Python error: script \"%s\" not loaded",
                               name);
    }
}

/*
 * weechat_python_unload_all: unload all Python scripts
 */

void
weechat_python_unload_all (t_weechat_plugin *plugin)
{
    plugin->printf_server (plugin,
                           "Unloading all Python scripts");
    while (python_scripts)
        weechat_python_unload (plugin, python_scripts);

    plugin->printf_server (plugin,
                           "Python scripts unloaded");
}

/*
 * weechat_python_cmd: /python command handler
 */

int
weechat_python_cmd (t_weechat_plugin *plugin,
                    char *server, char *command, char *arguments,
                    char *handler_args, void *handler_pointer)
{
    int argc, path_length, handler_found;
    char **argv, *path_script, *dir_home;
    t_plugin_script *ptr_script;
    t_plugin_handler *ptr_handler;
    
    /* make gcc happy */
    (void) server;
    (void) command;
    (void) handler_args;
    (void) handler_pointer;
    
    if (arguments)
        argv = plugin->explode_string (plugin, arguments, " ", 0, &argc);
    else
    {
        argv = NULL;
        argc = 0;
    }
    
    switch (argc)
    {
        case 0:
            /* list registered Python scripts */
            plugin->printf_server (plugin, "");
            plugin->printf_server (plugin, "Registered Python scripts:");
            if (python_scripts)
            {
                for (ptr_script = python_scripts;
                     ptr_script; ptr_script = ptr_script->next_script)
                {
                    plugin->printf_server (plugin, "  %s v%s%s%s",
                                           ptr_script->name,
                                           ptr_script->version,
                                           (ptr_script->description[0]) ? " - " : "",
                                           ptr_script->description);
                }
            }
            else
                plugin->printf_server (plugin, "  (none)");
            
            /* list Python message handlers */
            plugin->printf_server (plugin, "");
            plugin->printf_server (plugin, "Python message handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_MESSAGE)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->printf_server (plugin, "  IRC(%s) => Python(%s)",
                                           ptr_handler->irc_command,
                                           ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->printf_server (plugin, "  (none)");
            
            /* list Python command handlers */
            plugin->printf_server (plugin, "");
            plugin->printf_server (plugin, "Python command handlers:");
            handler_found = 0;
            for (ptr_handler = plugin->handlers;
                 ptr_handler; ptr_handler = ptr_handler->next_handler)
            {
                if ((ptr_handler->type == HANDLER_COMMAND)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->printf_server (plugin, "  /%s => Python(%s)",
                                           ptr_handler->command,
                                           ptr_handler->handler_args);
                }
            }
            if (!handler_found)
                plugin->printf_server (plugin, "  (none)");
            break;
        case 1:
            if (plugin->ascii_strcasecmp (plugin, argv[0], "autoload") == 0)
                weechat_script_auto_load (plugin, "python", weechat_python_load);
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "reload") == 0)
            {
                weechat_python_unload_all (plugin);
                weechat_script_auto_load (plugin, "python", weechat_python_load);
            }
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "unload") == 0)
                weechat_python_unload_all (plugin);
            break;
        case 2:
            if (plugin->ascii_strcasecmp (plugin, argv[0], "load") == 0)
            {
                /* load Python script */
                if ((strstr (argv[1], "/")) || (strstr (argv[1], "\\")))
                    path_script = NULL;
                else
                {
                    dir_home = plugin->get_info (plugin, "weechat_dir", NULL);
                    if (dir_home)
                    {
                        path_length = strlen (dir_home) + strlen (argv[1]) + 16;
                        path_script = (char *) malloc (path_length * sizeof (char));
                        if (path_script)
                            snprintf (path_script, path_length, "%s/python/%s",
                                      dir_home, argv[1]);
                        else
                            path_script = NULL;
                        free (dir_home);
                    }
                    else
                        path_script = NULL;
                }
                weechat_python_load (plugin, (path_script) ? path_script : argv[1]);
                if (path_script)
                    free (path_script);
            }
            else if (plugin->ascii_strcasecmp (plugin, argv[0], "unload") == 0)
            {
                /* unload Python script */
                weechat_python_unload_name (plugin, argv[1]);
            }
            else
            {
                plugin->printf_server (plugin,
                                       "Python error: unknown option for "
                                       "\"python\" command");
            }
            break;
        default:
            plugin->printf_server (plugin,
                                   "Python error: wrong argument count for \"python\" command");
    }
    
    if (argv)
        plugin->free_exploded_string (plugin, argv);
    
    return 1;
}

/*
 * weechat_plugin_init: initialize Python plugin
 */

int
weechat_plugin_init (t_weechat_plugin *plugin)
{
    
    python_plugin = plugin;
    
    plugin->printf_server (plugin, "Loading Python module \"weechat\"");
    
    Py_Initialize ();
    if (Py_IsInitialized () == 0)
    {
        plugin->printf_server (plugin,
                               "Python error: unable to launch global interpreter");
        return PLUGIN_RC_KO;
    }

    PyEval_InitThreads();
    
    python_mainThreadState = PyThreadState_Get();
    
    if (python_mainThreadState == NULL)
    {
        plugin->printf_server (plugin,
                               "Python error: unable to get current interpreter state");
        return PLUGIN_RC_KO;
    }
    
    plugin->cmd_handler_add (plugin, "python",
                             "list/load/unload Python scripts",
                             "[load filename] | [autoload] | [reload] | [unload]",
                             "filename: Python script (file) to load\n\n"
                             "Without argument, /python command lists all loaded Python scripts.",
                             weechat_python_cmd, NULL, NULL);

    plugin->mkdir_home (plugin, "python");
    plugin->mkdir_home (plugin, "python/autoload");
    
    weechat_script_auto_load (plugin, "python", weechat_python_load);
    
    /* init ok */
    return PLUGIN_RC_OK;
}

/*
 * weechat_plugin_end: shutdown Python interface
 */

void
weechat_plugin_end (t_weechat_plugin *plugin)
{
    /* unload all scripts */
    weechat_python_unload_all (plugin);

    /* free Python interpreter */
    if (python_mainThreadState != NULL)
    {
        PyThreadState_Swap (python_mainThreadState);
        python_mainThreadState = NULL;
    }
    
    Py_Finalize ();
    if (Py_IsInitialized () != 0)
        python_plugin->printf_server (python_plugin,
                                      "Python error: unable to free interpreter");
    
    python_plugin->printf_server (python_plugin,
                                  "Python plugin ended");
}
