/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* weechat-python.c: Python plugin support for WeeChat */

#undef _

#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

char python_buffer_output[128];

/*
 * weechat_python_exec: execute a Python script
 */

void *
weechat_python_exec (t_weechat_plugin *plugin,
                     t_plugin_script *script,
		     int ret_type,
                     char *function, char *arg1, char *arg2, char *arg3)
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
        plugin->print_server (plugin,
                              "Python error: unable to run function \"%s\"",
                              function);
	/* PyEval_ReleaseThread (python_current_script->interpreter); */
	return NULL;
    }

    python_current_script = script;
    
    if (arg1)
    {
        if (arg2)
        {
            if (arg3)
                rc = PyObject_CallFunction (evFunc, "sss", arg1, arg2, arg3);
            else
                rc = PyObject_CallFunction (evFunc, "ss", arg1, arg2);
        }
        else
            rc = PyObject_CallFunction (evFunc, "s", arg1);
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
    
    if (PyString_Check (rc) && (ret_type == SCRIPT_EXEC_STRING))
    {
	if (PyString_AsString (rc))
	    ret_value = strdup (PyString_AsString(rc));
	else
	    ret_value = NULL;
	
	Py_XDECREF(rc);
    }
    else if (PyInt_Check (rc) && (ret_type == SCRIPT_EXEC_INT))
    {
	
	ret_i = (int *) malloc (sizeof(int));
	if (ret_i)
	    *ret_i = (int) PyInt_AsLong(rc);
	ret_value = ret_i;
	
	Py_XDECREF(rc);
    }
    else
    {
	python_plugin->print_server (python_plugin, 
				     "Python error: function \"%s\" must return a valid value",
				     function);
	/* PyEval_ReleaseThread (python_current_script->interpreter); */
	return NULL;
    }
    
    if (ret_value == NULL)
    {
	plugin->print_server (plugin,
                              "Python error: unable to alloc memory in function \"%s\"",
                              function);
	/* PyEval_ReleaseThread (python_current_script->interpreter); */
	return NULL;
    }
    
    if (PyErr_Occurred ()) PyErr_Print ();
    
    /* PyEval_ReleaseThread (python_current_script->interpreter); */
    
    return ret_value;
}

/*
 * weechat_python_cmd_msg_handler: general command/message handler for Python
 */

int
weechat_python_cmd_msg_handler (t_weechat_plugin *plugin,
                                int argc, char **argv,
                                char *handler_args, void *handler_pointer)
{
    int *r;
    int ret;
    
    if (argc >= 3)
    {
        r = (int *) weechat_python_exec (plugin, (t_plugin_script *)handler_pointer,
					 SCRIPT_EXEC_INT,
					 handler_args, argv[0], argv[2], NULL);
	if (r == NULL)
	    ret = PLUGIN_RC_KO;
	else
	{
	    ret = *r;
	    free (r);
	}
	return ret;
    }
    else
        return PLUGIN_RC_KO;
}

/*
 * weechat_python_timer_handler: general timer handler for Python
 */

int
weechat_python_timer_handler (t_weechat_plugin *plugin,
                              int argc, char **argv,
                              char *handler_args, void *handler_pointer)
{
    /* make gcc happy */
    (void) argc;
    (void) argv;
    int *r;
    int ret;
    
    r = (int *) weechat_python_exec (plugin, (t_plugin_script *)handler_pointer,
				     SCRIPT_EXEC_INT,
				     handler_args, NULL, NULL, NULL);
    if (r == NULL)
	ret = PLUGIN_RC_KO;
    else
    {
	ret = *r;
	free (r);
    }
    return ret;
}

/*
 * weechat_python_keyboard_handler: general keyboard handler for Python
 */

int
weechat_python_keyboard_handler (t_weechat_plugin *plugin,
                                 int argc, char **argv,
                                 char *handler_args, void *handler_pointer)
{
    int *r;
    int ret;
    
    if (argc >= 3)
    {
        r = (int *) weechat_python_exec (plugin, (t_plugin_script *)handler_pointer,
					 SCRIPT_EXEC_INT,
					 handler_args, argv[0], argv[1], argv[2]);
	if (r == NULL)
	    ret = PLUGIN_RC_KO;
	else
	{
	    ret = *r;
	    free (r);
	}
	return ret;
    }
    else
        return PLUGIN_RC_KO;
}

/*
 * weechat_python_modifier: general modifier for Python
 */

char *
weechat_python_modifier (t_weechat_plugin *plugin,
                         int argc, char **argv,
                         char *modifier_args, void *modifier_pointer)
{    
    if (argc >= 2)
        return (char *) weechat_python_exec (plugin, (t_plugin_script *)modifier_pointer,
					     SCRIPT_EXEC_STRING,
					     modifier_args, argv[0], argv[1], NULL);
    else
        return NULL;
}

/*
 * weechat_python_register: startup function for all WeeChat Python scripts
 */

static PyObject *
weechat_python_register (PyObject *self, PyObject *args)
{
    char *name, *version, *shutdown_func, *description, *charset;
    
    /* make gcc happy */
    (void) self;

    python_current_script = NULL;
    
    name = NULL;
    version = NULL;
    shutdown_func = NULL;
    description = NULL;
    charset = NULL;
    
    if (!PyArg_ParseTuple (args, "ssss|s", &name, &version, &shutdown_func,
                           &description, &charset))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"register\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (weechat_script_search (python_plugin, &python_scripts, name))
    {
        /* error: another scripts already exists with this name! */
        python_plugin->print_server (python_plugin,
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
                                                description, charset);
    if (python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python: registered script \"%s\", "
                                     "version %s (%s)",
                                     name, version, description);
    }
    else
        return Py_BuildValue ("i", 0);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_set_charset: set script charset
 */

static PyObject *
weechat_python_set_charset (PyObject *self, PyObject *args)
{
    char *charset;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to set charset, "
                                     "script not initialized");
	Py_INCREF(Py_None);
        return Py_None;
    }
    
    charset = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &charset))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"set_charset\" function");
	Py_INCREF(Py_None);
        return Py_None;
    }
    
    if (charset)
        weechat_script_set_charset (python_plugin,
                                    python_current_script,
                                    charset);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_print: print message into a buffer (current or specified one)
 */

static PyObject *
weechat_python_print (PyObject *self, PyObject *args)
{
    char *message, *channel_name, *server_name;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to print message, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    message = NULL;
    channel_name = NULL;
    server_name = NULL;
    
    if (!PyArg_ParseTuple (args, "s|ss", &message, &channel_name, &server_name))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"print\" function");
        return Py_BuildValue ("i", 0);
    }
    
    python_plugin->print (python_plugin,
                          server_name, channel_name,
                          "%s", message);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_print_server: print message into a buffer server
 */

static PyObject *
weechat_python_print_server (PyObject *self, PyObject *args)
{
    char *message;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to print message (server), "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &message))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"print_server\" function");
        return Py_BuildValue ("i", 0);
    }
    
    python_plugin->print_server (python_plugin, "%s", message);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_print_infobar: print message to infobar
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
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to print infobar message, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    delay = 1;
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "is", &delay, &message))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"print_infobar\" function");
        return Py_BuildValue ("i", 0);
    }
    
    python_plugin->print_infobar (python_plugin, delay, "%s", message);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_remove_infobar: remove message(s) from infobar
 */

static PyObject *
weechat_python_remove_infobar (PyObject *self, PyObject *args)
{
    int how_many;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to remove infobar message(s), "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    how_many = 0;
    
    if (!PyArg_ParseTuple (args, "|is", &how_many))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"infobar_remove\" function");
        return Py_BuildValue ("i", 0);
    }
    
    python_plugin->infobar_remove (python_plugin, how_many);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_log: log message in server/channel (current or specified ones)
 */

static PyObject *
weechat_python_log (PyObject *self, PyObject *args)
{
    char *message, *channel_name, *server_name;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to log message, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    message = NULL;
    channel_name = NULL;
    server_name = NULL;
    
    if (!PyArg_ParseTuple (args, "s|ss", &message, &channel_name, &server_name))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"log\" function");
        return Py_BuildValue ("i", 0);
    }
    
    python_plugin->log (python_plugin,
                           server_name, channel_name,
                           "%s", message);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_command: send command to server
 */

static PyObject *
weechat_python_command (PyObject *self, PyObject *args)
{
    char *command, *channel_name, *server_name;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to run command, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    command = NULL;
    channel_name = NULL;
    server_name = NULL;
    
    if (!PyArg_ParseTuple (args, "s|ss", &command, &channel_name, &server_name))
    {
        python_plugin->print_server (python_plugin,
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
 * weechat_python_add_message_handler: add handler for messages
 */

static PyObject *
weechat_python_add_message_handler (PyObject *self, PyObject *args)
{
    char *irc_command, *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to add message handler, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    irc_command = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &irc_command, &function))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"add_message_handler\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (python_plugin->msg_handler_add (python_plugin, irc_command,
                                        weechat_python_cmd_msg_handler,
                                        function,
                                        (void *)python_current_script))
        return Py_BuildValue ("i", 1);
    
    return Py_BuildValue ("i", 0);
}

/*
 * weechat_python_add_command_handler: define/redefines commands
 */

static PyObject *
weechat_python_add_command_handler (PyObject *self, PyObject *args)
{
    char *command, *function, *description, *arguments, *arguments_description;
    char *completion_template;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to add command handler, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    command = NULL;
    function = NULL;
    description = NULL;
    arguments = NULL;
    arguments_description = NULL;
    completion_template = NULL;
    
    if (!PyArg_ParseTuple (args, "ss|ssss", &command, &function,
                           &description, &arguments, &arguments_description,
                           &completion_template))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"add_command_handler\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (python_plugin->cmd_handler_add (python_plugin,
                                        command,
                                        description,
                                        arguments,
                                        arguments_description,
                                        completion_template,
                                        weechat_python_cmd_msg_handler,
                                        function,
                                        (void *)python_current_script))
        return Py_BuildValue ("i", 1);
    
    return Py_BuildValue ("i", 0);
}

/*
 * weechat_python_add_timer_handler: add a timer handler
 */

static PyObject *
weechat_python_add_timer_handler (PyObject *self, PyObject *args)
{
    int interval;
    char *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to add timer handler, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    interval = 10;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "is", &interval, &function))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"add_timer_handler\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (python_plugin->timer_handler_add (python_plugin, interval,
                                          weechat_python_timer_handler,
                                          function,
                                          (void *)python_current_script))
        return Py_BuildValue ("i", 1);
    
    return Py_BuildValue ("i", 0);
}

/*
 * weechat_python_add_keyboard_handler: add a keyboard handler
 */

static PyObject *
weechat_python_add_keyboard_handler (PyObject *self, PyObject *args)
{
    char *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to add keyboard handler, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &function))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"add_keyboard_handler\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (python_plugin->keyboard_handler_add (python_plugin,
                                             weechat_python_keyboard_handler,
                                             function,
                                             (void *)python_current_script))
        return Py_BuildValue ("i", 1);
    
    return Py_BuildValue ("i", 0);
}

/*
 * weechat_python_remove_handler: remove a handler
 */

static PyObject *
weechat_python_remove_handler (PyObject *self, PyObject *args)
{
    char *command, *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to remove handler, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    command = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &command, &function))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"remove_handler\" function");
        return Py_BuildValue ("i", 0);
    }
    
    weechat_script_remove_handler (python_plugin, python_current_script,
                                   command, function);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_remove_timer_handler: remove a timer handler
 */

static PyObject *
weechat_python_remove_timer_handler (PyObject *self, PyObject *args)
{
    char *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to remove timer handler, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &function))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"remove_timer_handler\" function");
        return Py_BuildValue ("i", 0);
    }
    
    weechat_script_remove_timer_handler (python_plugin, python_current_script,
                                         function);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_remove_keyboard_handler: remove a keyboard handler
 */

static PyObject *
weechat_python_remove_keyboard_handler (PyObject *self, PyObject *args)
{
    char *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to remove keyboard handler, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &function))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"remove_keyboard_handler\" function");
        return Py_BuildValue ("i", 0);
    }
    
    weechat_script_remove_keyboard_handler (python_plugin, python_current_script,
                                            function);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_add_modifier: add a modifier
 */

static PyObject *
weechat_python_add_modifier (PyObject *self, PyObject *args)
{
    char *type, *command, *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to add modifier, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    type = NULL;
    command = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &type, &command, &function))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"add_modifier\" function");
        return Py_BuildValue ("i", 0);
    }
    
    if (python_plugin->modifier_add (python_plugin, type, command,
                                     weechat_python_modifier,
                                     function,
                                     (void *)python_current_script))	    
        return Py_BuildValue ("i", 1);
    
    return Py_BuildValue ("i", 0);
}

/*
 * weechat_python_remove_modifier: remove a modifier
 */

static PyObject *
weechat_python_remove_modifier (PyObject *self, PyObject *args)
{
    char *type, *command, *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to remove modifier, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    type = NULL;
    command = NULL;
    function = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &type, &command, &function))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"remove_modifier\" function");
        return Py_BuildValue ("i", 0);
    }
    
    weechat_script_remove_modifier (python_plugin, python_current_script,
                                    type, command, function);
    
    return Py_BuildValue ("i", 1);
}

/*
 * weechat_python_get_info: get various infos
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
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get info, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    arg = NULL;
    server_name = NULL;
    
    if (!PyArg_ParseTuple (args, "s|s", &arg, &server_name))
    {
        python_plugin->print_server (python_plugin,
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
 * weechat_python_get_dcc_info: get infos about DCC
 */

static PyObject *
weechat_python_get_dcc_info (PyObject *self, PyObject *args)
{
    t_plugin_dcc_info *dcc_info, *ptr_dcc;
    PyObject *dcc_list, *dcc_list_member;
    char timebuffer1[64];
    char timebuffer2[64];
    struct in_addr in;
    
    /* make gcc happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get DCC info, "
                                     "script not initialized");
	Py_INCREF(Py_None);
        return Py_None;
    }

    dcc_list = PyList_New (0);    
    if (!dcc_list)
    {
	Py_INCREF(Py_None);
	return Py_None;
    }
    
    dcc_info = python_plugin->get_dcc_info (python_plugin);
    if (!dcc_info)
	return dcc_list;
    
    for(ptr_dcc = dcc_info; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
    {
	strftime(timebuffer1, sizeof(timebuffer1), "%F %T",
		 localtime(&ptr_dcc->start_time));
	strftime(timebuffer2, sizeof(timebuffer2), "%F %T",
		 localtime(&ptr_dcc->start_transfer));
	in.s_addr = htonl(ptr_dcc->addr);
	
	dcc_list_member= PyDict_New();
	
        if (dcc_list_member) 
        {
	    
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "server"),
			   Py_BuildValue("s", ptr_dcc->server));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "channel"),
			   Py_BuildValue("s", ptr_dcc->channel));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "type"),
			   Py_BuildValue("i", ptr_dcc->type));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "status"),
			   Py_BuildValue("i", ptr_dcc->status));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "start_time"),
			   Py_BuildValue("s", timebuffer1));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "start_transfer"),
			   Py_BuildValue("s", timebuffer2));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "address"),
			   Py_BuildValue("s", inet_ntoa(in)));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "port"),
			   Py_BuildValue("i", ptr_dcc->port));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "nick"),
			   Py_BuildValue("s", ptr_dcc->nick));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "remote_file"),
			   Py_BuildValue("s", ptr_dcc->filename));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "local_file"),
			   Py_BuildValue("s", ptr_dcc->local_filename));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "filename_suffix"),
			   Py_BuildValue("i", ptr_dcc->filename_suffix));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "size"),
			   Py_BuildValue("k", ptr_dcc->size));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "pos"),
			   Py_BuildValue("k", ptr_dcc->pos));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "start_resume"),
			   Py_BuildValue("k", ptr_dcc->start_resume));
	    PyDict_SetItem(dcc_list_member, Py_BuildValue("s", "cps"),
			   Py_BuildValue("k", ptr_dcc->bytes_per_sec));	    
	    
            PyList_Append(dcc_list, dcc_list_member);
	    Py_DECREF (dcc_list_member);
        }
    }
    
    python_plugin->free_dcc_info (python_plugin, dcc_info);
    
    return dcc_list;
}

/*
 * weechat_python_get_config: get value of a WeeChat config option
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
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get config option, "
                                     "script not initialized");
	Py_INCREF(Py_None);
        return Py_None;
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"get_config\" function");
	Py_INCREF(Py_None);
        return Py_None;
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
 * weechat_python_set_config: set value of a WeeChat config option
 */

static PyObject *
weechat_python_set_config (PyObject *self, PyObject *args)
{
    char *option, *value;
    
    /* make gcc happy */
    (void) self;

    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to set config option, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    option = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &option, &value))
    {
        python_plugin->print_server (python_plugin,
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
 * weechat_python_get_plugin_config: get value of a plugin config option
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
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get plugin config option, "
                                     "script not initialized");
	Py_INCREF(Py_None);
        return Py_None;
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"get_plugin_config\" function");
	Py_INCREF(Py_None);
        return Py_None;
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
 * weechat_python_set_plugin_config: set value of a plugin config option
 */

static PyObject *
weechat_python_set_plugin_config (PyObject *self, PyObject *args)
{
    char *option, *value;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to set plugin config option, "
                                     "script not initialized");
        return Py_BuildValue ("i", 0);
    }
    
    option = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &option, &value))
    {
        python_plugin->print_server (python_plugin,
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
 * weechat_python_get_server_info: get infos about servers
 */

static PyObject *
weechat_python_get_server_info (PyObject *self, PyObject *args)
{
    t_plugin_server_info *server_info, *ptr_server;
    PyObject *server_hash, *server_hash_member;
    char timebuffer[64];
    
    /* make gcc happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get server infos, "
                                     "script not initialized");
	Py_INCREF(Py_None);
        return Py_None;
    }
    
    server_hash = PyDict_New ();
    if (!server_hash)
    {
	Py_INCREF(Py_None);
	return Py_None;
    }

    server_info = python_plugin->get_server_info (python_plugin);
    if  (!server_info)
	return server_hash;

    for(ptr_server = server_info; ptr_server; ptr_server = ptr_server->next_server)
    {
	strftime(timebuffer, sizeof(timebuffer), "%F %T",
		 localtime(&ptr_server->away_time));
	
	server_hash_member = PyDict_New();
	
	if (server_hash_member)
	{
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "autoconnect"),
			   Py_BuildValue("i", ptr_server->autoconnect));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "autoreconnect"),
			   Py_BuildValue("i", ptr_server->autoreconnect));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "autoreconnect_delay"),
			   Py_BuildValue("i", ptr_server->autoreconnect_delay));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "command_line"),
			   Py_BuildValue("i", ptr_server->command_line));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "address"),
			   Py_BuildValue("s", ptr_server->address));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "port"),
			   Py_BuildValue("i", ptr_server->port));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "ipv6"),
			   Py_BuildValue("i", ptr_server->ipv6));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "ssl"),
			   Py_BuildValue("i", ptr_server->ssl));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "password"),
			   Py_BuildValue("s", ptr_server->password));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "nick1"),
			   Py_BuildValue("s", ptr_server->nick1));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "nick2"),
			   Py_BuildValue("s", ptr_server->nick2));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "nick3"),
			   Py_BuildValue("s", ptr_server->nick3));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "username"),
			   Py_BuildValue("s", ptr_server->username));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "realname"),
			   Py_BuildValue("s", ptr_server->realname));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "command"),
			   Py_BuildValue("s", ptr_server->command));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "command_delay"),
			   Py_BuildValue("i", ptr_server->command_delay));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "autojoin"),
			   Py_BuildValue("s", ptr_server->autojoin));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "autorejoin"),
			   Py_BuildValue("i", ptr_server->autorejoin));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "notify_levels"),
			   Py_BuildValue("s", ptr_server->notify_levels));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "is_connected"),
			   Py_BuildValue("i", ptr_server->is_connected));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "ssl_connected"),
			   Py_BuildValue("i", ptr_server->ssl_connected));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "nick"),
			   Py_BuildValue("s", ptr_server->nick));
            PyDict_SetItem(server_hash_member, Py_BuildValue("s", "nick_modes"),
			   Py_BuildValue("s", ptr_server->nick_modes));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "away_time"),
			   Py_BuildValue("s", timebuffer));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "lag"),
			   Py_BuildValue("i", ptr_server->lag));
	    
	    PyDict_SetItem(server_hash, Py_BuildValue("s", ptr_server->name), server_hash_member);
	    Py_DECREF (server_hash_member);
	}
    }    

    python_plugin->free_server_info(python_plugin, server_info);
    
    return server_hash;
}

/*
 * weechat_python_get_channel_info: get infos about channels
 */

static PyObject *
weechat_python_get_channel_info (PyObject *self, PyObject *args)
{
    t_plugin_channel_info *channel_info, *ptr_channel;
    PyObject *channel_hash, *channel_hash_member;
    char *server;
    
    /* make gcc happy */
    (void) self;
        
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get channel infos, "
                                     "script not initialized");
	Py_INCREF(Py_None);
        return Py_None;
    }
    
    server = NULL;
    if (!PyArg_ParseTuple (args, "s", &server))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"get_channel_info\" function");
	Py_INCREF(Py_None);
        return Py_None;
    }
    
    channel_hash = PyDict_New ();
    if (!channel_hash)
    {
	Py_INCREF(Py_None);
	return Py_None;
    }

    channel_info = python_plugin->get_channel_info (python_plugin, server);
    if  (!channel_info)
	return channel_hash;

    for(ptr_channel = channel_info; ptr_channel; ptr_channel = ptr_channel->next_channel)
    {
	channel_hash_member = PyDict_New();
	
	if (channel_hash_member)
	{
	    PyDict_SetItem(channel_hash_member, Py_BuildValue("s", "type"),
			   Py_BuildValue("i", ptr_channel->type));
	    PyDict_SetItem(channel_hash_member, Py_BuildValue("s", "topic"),
			   Py_BuildValue("s", ptr_channel->topic));
	    PyDict_SetItem(channel_hash_member, Py_BuildValue("s", "modes"),
			   Py_BuildValue("s", ptr_channel->modes));
	    PyDict_SetItem(channel_hash_member, Py_BuildValue("s", "limit"),
			   Py_BuildValue("i", ptr_channel->limit));
	    PyDict_SetItem(channel_hash_member, Py_BuildValue("s", "key"),
			   Py_BuildValue("s", ptr_channel->key));
	    PyDict_SetItem(channel_hash_member, Py_BuildValue("s", "nicks_count"),
			   Py_BuildValue("i", ptr_channel->nicks_count));
	    
	    PyDict_SetItem(channel_hash, Py_BuildValue("s", ptr_channel->name), channel_hash_member);
	    Py_DECREF (channel_hash_member);
	}
    }    

    python_plugin->free_channel_info(python_plugin, channel_info);
    
    return channel_hash;
}

/*
 * weechat_python_get_nick_info: get infos about nicks
 */

static PyObject *
weechat_python_get_nick_info (PyObject *self, PyObject *args)
{
    t_plugin_nick_info *nick_info, *ptr_nick;
    PyObject *nick_hash, *nick_hash_member;
    char *server, *channel;
    
    /* make gcc happy */
    (void) self;
        
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get nick infos, "
                                     "script not initialized");
	Py_INCREF(Py_None);
        return Py_None;
    }
    
    server = NULL;
    channel = NULL;
    if (!PyArg_ParseTuple (args, "ss", &server, &channel))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"get_nick_info\" function");
	Py_INCREF(Py_None);
        return Py_None;
    }
    
    nick_hash = PyDict_New ();
    if (!nick_hash)
    {
	Py_INCREF(Py_None);
	return Py_None;
    }

    nick_info = python_plugin->get_nick_info (python_plugin, server, channel);
    if  (!nick_info)
	return nick_hash;

    for(ptr_nick = nick_info; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
	nick_hash_member = PyDict_New();
	
	if (nick_hash_member)
	{
	    PyDict_SetItem(nick_hash_member, Py_BuildValue("s", "flags"),
			   Py_BuildValue("i", ptr_nick->flags));
	    PyDict_SetItem(nick_hash_member, Py_BuildValue("s", "host"),
			   Py_BuildValue("s", ptr_nick->host ? ptr_nick->host : ""));
	    
	    PyDict_SetItem(nick_hash, Py_BuildValue("s", ptr_nick->nick), nick_hash_member);
	    Py_DECREF (nick_hash_member);
	}
    }    

    python_plugin->free_nick_info(python_plugin, nick_info);
    
    return nick_hash;
}

/*
 * weechat_python_get_irc_color: 
 *          get the numeric value which identify an irc color by its name
 */

static PyObject *
weechat_python_get_irc_color (PyObject *self, PyObject *args)
{
    char *color;
    
    /* make gcc happy */
    (void) self;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get irc color, "
                                     "script not initialized");
        return Py_BuildValue ("i", -1);
    }
    
    color = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &color))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"get_irc_color\" function");
        return Py_BuildValue ("i", -1);
    }
    
    if (color)
	return Py_BuildValue ("i", python_plugin->get_irc_color (python_plugin, color));
    
    return Py_BuildValue ("i", -1);
}

/*
 * weechat_python_get_window_info: get infos about windows
 */

static PyObject *
weechat_python_get_window_info (PyObject *self, PyObject *args)
{
    t_plugin_window_info *window_info, *ptr_window;
    PyObject *window_list, *window_list_member;
    
    /* make gcc happy */
    (void) self;
    (void) args;
        
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get window info, "
                                     "script not initialized");
	Py_INCREF(Py_None);
        return Py_None;
    }
        
    window_list = PyList_New (0);
    if (!window_list) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    window_info = python_plugin->get_window_info (python_plugin);
    if  (!window_info)
	return window_list;

    for(ptr_window = window_info; ptr_window; ptr_window = ptr_window->next_window)
    {
	window_list_member = PyDict_New();
	
	if (window_list_member)
	{
	    PyDict_SetItem(window_list_member, Py_BuildValue("s", "num_buffer"),
			   Py_BuildValue("i", ptr_window->num_buffer));
	    PyDict_SetItem(window_list_member, Py_BuildValue("s", "win_x"),
			   Py_BuildValue("i", ptr_window->win_x));
	    PyDict_SetItem(window_list_member, Py_BuildValue("s", "win_y"),
			   Py_BuildValue("i", ptr_window->win_y));
	    PyDict_SetItem(window_list_member, Py_BuildValue("s", "win_width"),
			   Py_BuildValue("i", ptr_window->win_width));
	    PyDict_SetItem(window_list_member, Py_BuildValue("s", "win_height"),
			   Py_BuildValue("i", ptr_window->win_height));
	    PyDict_SetItem(window_list_member, Py_BuildValue("s", "win_width_pct"),
			   Py_BuildValue("i", ptr_window->win_width_pct));
	    PyDict_SetItem(window_list_member, Py_BuildValue("s", "win_height_pct"),
			   Py_BuildValue("i", ptr_window->win_height_pct));
	    
	    PyList_Append(window_list, window_list_member);
	    Py_DECREF (window_list_member);
	}
    }
    
    python_plugin->free_window_info(python_plugin, window_info);
    
    return window_list;
}

/*
 * weechat_python_get_buffer_info: get infos about buffers
 */

static PyObject *
weechat_python_get_buffer_info (PyObject *self, PyObject *args)
{
    t_plugin_buffer_info *buffer_info, *ptr_buffer;
    PyObject *buffer_hash, *buffer_hash_member;
    
    /* make gcc happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get buffer info, "
                                     "script not initialized");
	Py_INCREF(Py_None);
        return Py_None;
    }
    
    buffer_hash = PyDict_New ();
    if (!buffer_hash)
    {
	Py_INCREF(Py_None);
	return Py_None;
    }

    buffer_info = python_plugin->get_buffer_info (python_plugin);
    if  (!buffer_info)
	return buffer_hash;

    for(ptr_buffer = buffer_info; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
	buffer_hash_member = PyDict_New();
	
	if (buffer_hash_member)
	{
	    PyDict_SetItem(buffer_hash_member, Py_BuildValue("s", "type"),
			   Py_BuildValue("i", ptr_buffer->type));
	    PyDict_SetItem(buffer_hash_member, Py_BuildValue("s", "num_displayed"),
			   Py_BuildValue("i", ptr_buffer->num_displayed));
	    PyDict_SetItem(buffer_hash_member, Py_BuildValue("s", "server"),
			   Py_BuildValue("s", ptr_buffer->server_name == NULL ? "" : ptr_buffer->server_name));
	    PyDict_SetItem(buffer_hash_member, Py_BuildValue("s", "channel"),
			   Py_BuildValue("s", ptr_buffer->channel_name == NULL ? "" : ptr_buffer->channel_name));
	    PyDict_SetItem(buffer_hash_member, Py_BuildValue("s", "notify_level"),
			   Py_BuildValue("i", ptr_buffer->notify_level));
	    PyDict_SetItem(buffer_hash_member, Py_BuildValue("s", "log_filename"),
			   Py_BuildValue("s", ptr_buffer->log_filename == NULL ? "" : ptr_buffer->log_filename));
	    
	    PyDict_SetItem(buffer_hash, 
			   Py_BuildValue("i", ptr_buffer->number), buffer_hash_member);
	    Py_DECREF (buffer_hash_member);
	}
    }    
    
    python_plugin->free_buffer_info(python_plugin, buffer_info);
    
    return buffer_hash;
}

/*
 * weechat_python_get_buffer_data: get buffer content
 */

static PyObject *
weechat_python_get_buffer_data (PyObject *self, PyObject *args)
{
    t_plugin_buffer_line *buffer_data, *ptr_data;
    PyObject *data_list, *data_list_member;
    char *server, *channel;
    char timebuffer[64];
    
    /* make gcc happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get buffer data, "
                                     "script not initialized");
	Py_INCREF(Py_None);
        return Py_None;
    }

    if (!PyArg_ParseTuple (args, "|ss", &server, &channel))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"get_buffer_data\" function");
	Py_INCREF(Py_None);
        return Py_None;
    }

    data_list = PyList_New (0);    
    if (!data_list)
    {
	Py_INCREF(Py_None);
	return Py_None;
    }
    
    buffer_data = python_plugin->get_buffer_data (python_plugin, server, channel);
    if (!buffer_data)
	return data_list;
    
    for(ptr_data = buffer_data; ptr_data; ptr_data = ptr_data->next_line)
    {
	data_list_member= PyDict_New();
	
        if (data_list_member) 
        {
	    strftime(timebuffer, sizeof(timebuffer), "%F %T",
		     localtime(&ptr_data->date));
	    
	    PyDict_SetItem(data_list_member, Py_BuildValue("s", "date"),
			   Py_BuildValue("s", timebuffer));
	    PyDict_SetItem(data_list_member, Py_BuildValue("s", "nick"),
			   Py_BuildValue("s", ptr_data->nick == NULL ? "" : ptr_data->nick));
	    PyDict_SetItem(data_list_member, Py_BuildValue("s", "data"),
			   Py_BuildValue("s", ptr_data->data == NULL ? "" : ptr_data->data));
	    	    
            PyList_Append(data_list, data_list_member);
	    Py_DECREF (data_list_member);
        }
    }
    
    python_plugin->free_buffer_data (python_plugin, buffer_data);
    
    return data_list;
}

/*
 * Python subroutines
 */

static
PyMethodDef weechat_python_funcs[] = {
    { "register", weechat_python_register, METH_VARARGS, "" },
    { "set_charset", weechat_python_set_charset, METH_VARARGS, "" },
    { "prnt", weechat_python_print, METH_VARARGS, "" },
    { "print_server", weechat_python_print_server, METH_VARARGS, "" },
    { "print_infobar", weechat_python_print_infobar, METH_VARARGS, "" },
    { "remove_infobar", weechat_python_remove_infobar, METH_VARARGS, "" },
    { "log", weechat_python_log, METH_VARARGS, "" },
    { "command", weechat_python_command, METH_VARARGS, "" },
    { "add_message_handler", weechat_python_add_message_handler, METH_VARARGS, "" },
    { "add_command_handler", weechat_python_add_command_handler, METH_VARARGS, "" },
    { "add_timer_handler", weechat_python_add_timer_handler, METH_VARARGS, "" },
    { "add_keyboard_handler", weechat_python_add_keyboard_handler, METH_VARARGS, "" },
    { "remove_handler", weechat_python_remove_handler, METH_VARARGS, "" },
    { "remove_timer_handler", weechat_python_remove_timer_handler, METH_VARARGS, "" },
    { "remove_keyboard_handler", weechat_python_remove_keyboard_handler, METH_VARARGS, "" },
    { "add_modifier", weechat_python_add_modifier, METH_VARARGS, "" },
    { "remove_modifier", weechat_python_remove_modifier, METH_VARARGS, "" },
    { "get_info", weechat_python_get_info, METH_VARARGS, "" },
    { "get_dcc_info", weechat_python_get_dcc_info, METH_VARARGS, "" },
    { "get_config", weechat_python_get_config, METH_VARARGS, "" },
    { "set_config", weechat_python_set_config, METH_VARARGS, "" },
    { "get_plugin_config", weechat_python_get_plugin_config, METH_VARARGS, "" },
    { "set_plugin_config", weechat_python_set_plugin_config, METH_VARARGS, "" },
    { "get_server_info", weechat_python_get_server_info, METH_VARARGS, "" },
    { "get_channel_info", weechat_python_get_channel_info, METH_VARARGS, "" },
    { "get_nick_info", weechat_python_get_nick_info, METH_VARARGS, "" },
    { "get_irc_color", weechat_python_get_irc_color, METH_VARARGS, "" },
    { "get_window_info", weechat_python_get_window_info, METH_VARARGS, "" },
    { "get_buffer_info", weechat_python_get_buffer_info, METH_VARARGS, "" },
    { "get_buffer_data", weechat_python_get_buffer_data, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL }
};

/*
 * weechat_python_output : redirection for stdout and stderr
 */

static PyObject *
weechat_python_output (PyObject *self, PyObject *args)
{
    char *msg, *m, *p;
    /* make gcc happy */
    (void) self;
    
    msg = NULL;

    if (!PyArg_ParseTuple (args, "s", &msg))
    {
        if (strlen(python_buffer_output) > 0)
	{
	    python_plugin->print_server (python_plugin,
					 "Python stdout/stderr : %s",
					 python_buffer_output);
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
		python_plugin->print_server (python_plugin,
					     "Python stdout/stderr : %s%s", 
					     python_buffer_output, m);
	    *p = '\n';
	    python_buffer_output[0] = '\0';
	    m = ++p;
	}
	
	if (strlen(m) + strlen(python_buffer_output) > sizeof(python_buffer_output))
	{
	    python_plugin->print_server (python_plugin,
					 "Python stdout/stderr : %s%s",
					 python_buffer_output, m);
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
weechat_python_load (t_weechat_plugin *plugin, char *filename)
{
    char *argv[] = { "__weechat_plugin__" , NULL };
    FILE *fp;
    PyThreadState *python_current_interpreter;
    PyObject *weechat_module, *weechat_outputs, *weechat_dict;
    PyObject *python_path, *path;
    char *w_home, *p_home;
    int len;
    
    plugin->print_server (plugin, "Loading Python script \"%s\"", filename);
    
    if ((fp = fopen (filename, "r")) == NULL)
    {
        plugin->print_server (plugin, "Python error: script \"%s\" not found",
                              filename);
        return 0;
    }
    
    python_current_script = NULL;
    
    /* PyEval_AcquireLock (); */
    python_current_interpreter = Py_NewInterpreter ();
    PySys_SetArgv(1, argv);

    if (python_current_interpreter == NULL)
    {
        plugin->print_server (plugin,
                              "Python error: unable to create new sub-interpreter");
        fclose (fp);
	/* PyEval_ReleaseLock (); */
	return 0;
    }

    /* PyThreadState_Swap (python_current_interpreter); */
    
    weechat_module = Py_InitModule ("weechat", weechat_python_funcs);

    if ( weechat_module == NULL)
    {
        plugin->print_server (plugin,
                              "Python error: unable to initialize WeeChat module");
        fclose (fp);
	
	Py_EndInterpreter (python_current_interpreter);
        /* PyEval_ReleaseLock (); */
        
	return 0;
    }

    /* adding $weechat_dir/python in $PYTHONPATH */    
    python_path = PySys_GetObject ("path");
    w_home = plugin->get_info (plugin, "weechat_dir", NULL);
    if (w_home)
    {
	len = strlen (w_home) + 1 + strlen("python") + 1;
	p_home = (char *) malloc (len * sizeof(char));
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
	free (w_home);
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
        plugin->print_server (plugin,
                              "Python warning: unable to redirect stdout and stderr");
    }
    else
    {
	if (PySys_SetObject("stdout", weechat_outputs) == -1)
	    plugin->print_server (plugin,
                                  "Python warning: unable to redirect stdout");
	if (PySys_SetObject("stderr", weechat_outputs) == -1)
	    plugin->print_server (plugin,
                                  "Python warning: unable to redirect stderr");
    }
	
    python_current_script_filename = strdup (filename);
    
    if (PyRun_SimpleFile (fp, filename) != 0)
    {
        plugin->print_server (plugin,
                              "Python error: unable to parse file \"%s\"",
                              filename);
	free (python_current_script_filename);
	fclose (fp);
	
	if (PyErr_Occurred ()) PyErr_Print ();
	Py_EndInterpreter (python_current_interpreter);
	/* PyEval_ReleaseLock (); */
        
	/* if script was registered, removing from list */
	if (python_current_script != NULL)
	    weechat_script_remove (plugin, &python_scripts, python_current_script);
        return 0;
    }

    if (PyErr_Occurred ()) PyErr_Print ();

    fclose (fp);
    free (python_current_script_filename);
    
    if (python_current_script == NULL)
    {
        plugin->print_server (plugin,
                              "Python error: function \"register\" not found "
                              "(or failed) in file \"%s\"",
                              filename);
	
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
 * weechat_python_unload: unload a Python script
 */

void
weechat_python_unload (t_weechat_plugin *plugin, t_plugin_script *script)
{
    int *r;
    
    plugin->print_server (plugin,
                          "Unloading Python script \"%s\"",
                          script->name);
    
    if (script->shutdown_func[0])
    {
        r = (int *) weechat_python_exec (plugin, script, SCRIPT_EXEC_INT,
					 script->shutdown_func, NULL, NULL, NULL);
	if (r)
	    free (r);
    }

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
        plugin->print_server (plugin,
                              "Python script \"%s\" unloaded",
                              name);
    }
    else
    {
        plugin->print_server (plugin,
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
    plugin->print_server (plugin,
                          "Unloading all Python scripts");
    while (python_scripts)
        weechat_python_unload (plugin, python_scripts);

    plugin->print_server (plugin,
                          "Python scripts unloaded");
}

/*
 * weechat_python_cmd: /python command handler
 */

int
weechat_python_cmd (t_weechat_plugin *plugin,
                    int cmd_argc, char **cmd_argv,
                    char *handler_args, void *handler_pointer)
{
    int argc, handler_found, modifier_found;
    char **argv, *path_script;
    t_plugin_script *ptr_script;
    t_plugin_handler *ptr_handler;
    t_plugin_modifier *ptr_modifier;
    
    /* make gcc happy */
    (void) handler_args;
    (void) handler_pointer;
    
    if (cmd_argc < 3)
        return PLUGIN_RC_KO;
    
    if (cmd_argv[2])
        argv = plugin->explode_string (plugin, cmd_argv[2], " ", 0, &argc);
    else
    {
        argv = NULL;
        argc = 0;
    }
    
    switch (argc)
    {
        case 0:
            /* list registered Python scripts */
            plugin->print_server (plugin, "");
            plugin->print_server (plugin, "Registered Python scripts:");
            if (python_scripts)
            {
                for (ptr_script = python_scripts;
                     ptr_script; ptr_script = ptr_script->next_script)
                {
                    plugin->print_server (plugin, "  %s v%s%s%s",
                                          ptr_script->name,
                                          ptr_script->version,
                                          (ptr_script->description[0]) ? " - " : "",
                                          ptr_script->description);
                }
            }
            else
                plugin->print_server (plugin, "  (none)");
            
            /* list Python message handlers */
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
            
            /* list Python command handlers */
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
            
            /* list Python timer handlers */
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
            
            /* list Python keyboard handlers */
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

	    /* list Python modifiers */
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
                path_script = weechat_script_search_full_name (plugin, "python", argv[1]);
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
                plugin->print_server (plugin,
                                      "Python error: unknown option for "
                                      "\"python\" command");
            }
            break;
        default:
            plugin->print_server (plugin,
                                  "Python error: wrong argument count for \"python\" command");
    }
    
    if (argv)
        plugin->free_exploded_string (plugin, argv);
    
    return PLUGIN_RC_OK;
}

/*
 * weechat_plugin_init: initialize Python plugin
 */

int
weechat_plugin_init (t_weechat_plugin *plugin)
{
    
    python_plugin = plugin;
    
    plugin->print_server (plugin, "Loading Python module \"weechat\"");

    /* init stdout/stderr buffer */
    python_buffer_output[0] = '\0';
    
    Py_Initialize ();
    if (Py_IsInitialized () == 0)
    {
        plugin->print_server (plugin,
                              "Python error: unable to launch global interpreter");
        return PLUGIN_RC_KO;
    }

    PyEval_InitThreads();    
    /* python_mainThreadState = PyThreadState_Swap(NULL); */
    python_mainThreadState = PyEval_SaveThread();
    /* PyEval_ReleaseLock (); */

    if (python_mainThreadState == NULL)
    {
        plugin->print_server (plugin,
                              "Python error: unable to get current interpreter state");
        return PLUGIN_RC_KO;
    }
    
    plugin->cmd_handler_add (plugin, "python",
                             "list/load/unload Python scripts",
                             "[load filename] | [autoload] | [reload] | [unload [script]]",
                             "filename: Python script (file) to load\n"
                             "script: script name to unload\n\n"
                             "Without argument, /python command lists all loaded Python scripts.",
                             "load|autoload|reload|unload %f",
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
	/* PyEval_AcquireLock (); */
        PyThreadState_Swap (python_mainThreadState);
	/* PyEval_ReleaseLock (); */
        python_mainThreadState = NULL;
    }
    
    Py_Finalize ();
    if (Py_IsInitialized () != 0)
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to free interpreter");
    
    python_plugin->print_server (python_plugin,
                                 "Python plugin ended");
}
