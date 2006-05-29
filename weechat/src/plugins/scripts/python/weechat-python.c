/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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


#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
                     char *function, char *arg1, char *arg2, char *arg3)
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
        plugin->print_server (plugin,
                              "Python error: unable to run function \"%s\"",
                              function);
        return PLUGIN_RC_KO;
    }

    ret = -1;
    
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
        rc = PyObject_CallFunction (evFunc, "");

    
    if (rc == Py_None)
    {
	python_plugin->print_server (python_plugin, "Python error: function \"%s\" must return a valid value", function);
	return PLUGIN_RC_OK;
    }

    if (rc)
    {
        ret = (int) PyInt_AsLong(rc);
        Py_XDECREF(rc);
    }
    
    if (PyErr_Occurred ()) PyErr_Print ();
    
    if (ret < 0)
        return PLUGIN_RC_OK;
    else
        return ret;
}

/*
 * weechat_python_cmd_msg_handler: general command/message handler for Python
 */

int
weechat_python_cmd_msg_handler (t_weechat_plugin *plugin,
                                int argc, char **argv,
                                char *handler_args, void *handler_pointer)
{
    if (argc >= 3)
        return weechat_python_exec (plugin, (t_plugin_script *)handler_pointer,
                                    handler_args, argv[0], argv[2], NULL);
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
    
    return weechat_python_exec (plugin, (t_plugin_script *)handler_pointer,
                                handler_args, NULL, NULL, NULL);
}

/*
 * weechat_python_keyboard_handler: general keyboard handler for Python
 */

int
weechat_python_keyboard_handler (t_weechat_plugin *plugin,
                                 int argc, char **argv,
                                 char *handler_args, void *handler_pointer)
{
    if (argc >= 3)
        return weechat_python_exec (plugin, (t_plugin_script *)handler_pointer,
                                    handler_args, argv[0], argv[1], argv[2]);
    else
        return PLUGIN_RC_KO;
}

/*
 * weechat_python_register: startup function for all WeeChat Python scripts
 */

static PyObject *
weechat_python_register (PyObject *self, PyObject *args)
{
    char *name, *version, *shutdown_func, *description;
    
    /* make gcc happy */
    (void) self;

    python_current_script = NULL;
    
    name = NULL;
    version = NULL;
    shutdown_func = NULL;
    description = NULL;
    
    if (!PyArg_ParseTuple (args, "ssss", &name, &version, &shutdown_func, &description))
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
                                                description);
    if (python_current_script)
    {
        python_plugin->print_server (python_plugin,
                                     "Python: registered script \"%s\", "
                                     "version %s (%s)",
                                     name, version, description);
    }
    else
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to load script "
                                     "\"%s\" (not enough memory)",
                                     name);
        return Py_BuildValue ("i", 0);
    }
    
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
        return Py_None;
    }

    dcc_list = PyList_New (0);    
    if (!dcc_list)
	return Py_None;
    
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
	    
	    
            if (PyList_Append(dcc_list, dcc_list_member) != 0)
            {
		Py_DECREF(dcc_list_member);
		Py_DECREF(dcc_list);
                python_plugin->free_dcc_info (python_plugin, dcc_info);
                return Py_None;
            }
        }
        else
        {
	    Py_DECREF(dcc_list);
            python_plugin->free_dcc_info (python_plugin, dcc_info);
            return Py_None;
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
        return Py_BuildValue ("i", 0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        python_plugin->print_server (python_plugin,
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
        return Py_BuildValue ("i", 0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        python_plugin->print_server (python_plugin,
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
        return Py_BuildValue ("i", 0);
    }
    
    server_hash = PyDict_New ();
    if (!server_hash)
	return Py_None;

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
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "charset_decode_iso"),
			   Py_BuildValue("s", ptr_server->charset_decode_iso));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "charset_decode_utf"),
			   Py_BuildValue("s", ptr_server->charset_decode_utf));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "charset_encode"),
			   Py_BuildValue("s", ptr_server->charset_encode));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "is_connected"),
			   Py_BuildValue("i", ptr_server->is_connected));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "ssl_connected"),
			   Py_BuildValue("i", ptr_server->ssl_connected));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "nick"),
			   Py_BuildValue("s", ptr_server->nick));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "away_time"),
			   Py_BuildValue("s", timebuffer));
	    PyDict_SetItem(server_hash_member, Py_BuildValue("s", "lag"),
			   Py_BuildValue("i", ptr_server->lag));
	    
	    PyDict_SetItem(server_hash, Py_BuildValue("s", ptr_server->name), server_hash_member);
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
        return Py_BuildValue ("i", 0);
    }
    
    server = NULL;
    if (!PyArg_ParseTuple (args, "s", &server))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"get_channel_info\" function");
        return Py_BuildValue ("i", 0);
    }
    
    channel_hash = PyDict_New ();
    if (!channel_hash)
	return Py_None;

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
        return Py_BuildValue ("i", 0);
    }
    
    server = NULL;
    channel = NULL;
    if (!PyArg_ParseTuple (args, "ss", &server, &channel))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: wrong parameters for "
                                     "\"get_nick_info\" function");
        return Py_BuildValue ("i", 0);
    }
    
    nick_hash = PyDict_New ();
    if (!nick_hash)
	return Py_None;

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
	}
    }    

    python_plugin->free_nick_info(python_plugin, nick_info);
    
    return nick_hash;
}

/*
 * Python subroutines
 */

static
PyMethodDef weechat_python_funcs[] = {
    { "register", weechat_python_register, METH_VARARGS, "" },
    { "prnt", weechat_python_print, METH_VARARGS, "" },
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
    { "get_info", weechat_python_get_info, METH_VARARGS, "" },
    { "get_dcc_info", weechat_python_get_dcc_info, METH_VARARGS, "" },
    { "get_config", weechat_python_get_config, METH_VARARGS, "" },
    { "set_config", weechat_python_set_config, METH_VARARGS, "" },
    { "get_plugin_config", weechat_python_get_plugin_config, METH_VARARGS, "" },
    { "set_plugin_config", weechat_python_set_plugin_config, METH_VARARGS, "" },
    { "get_server_info", weechat_python_get_server_info, METH_VARARGS, "" },
    { "get_channel_info", weechat_python_get_channel_info, METH_VARARGS, "" },
    { "get_nick_info", weechat_python_get_nick_info, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL }
};

/*
 * weechat_python_output : redirection for stdout and stderr
 */

static PyObject *
weechat_python_output (PyObject *self, PyObject *args)
{
    char *msg, *p;
    /* make gcc happy */
    (void) self;
    
    msg = NULL;

    if (!PyArg_ParseTuple (args, "s", &msg))
    {
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to get "
                                     "stdout/stderr message(s)");
        return NULL; 
    }
    
    while ((p = strrchr(msg, '\n')) != NULL)
	*p = '\0';
    
    if (strlen(msg) > 0)
	python_plugin->print_server (python_plugin,
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
    
    plugin->print_server (plugin, "Loading Python script \"%s\"", filename);
    
    if ((fp = fopen (filename, "r")) == NULL)
    {
        plugin->print_server (plugin,
                              "Python error: unable to open file \"%s\"",
                              filename);
        return 0;
    }

    python_current_script = NULL;
    
    python_current_interpreter = Py_NewInterpreter ();
    PySys_SetArgv(1, argv);

    if (python_current_interpreter == NULL)
    {
        plugin->print_server (plugin,
                              "Python error: unable to create new sub-interpreter");
        fclose (fp);
        return 0;
    }

    PyThreadState_Swap (python_current_interpreter);
    
    weechat_module = Py_InitModule ("weechat", weechat_python_funcs);

    if ( weechat_module == NULL)
    {
        plugin->print_server (plugin,
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
	Py_EndInterpreter (python_current_interpreter);
        fclose (fp);
	/* if script was registered, removing from list */
	if (python_current_script != NULL)
	    weechat_script_remove (plugin, &python_scripts, python_current_script);
        return 0;
    }

    if (PyErr_Occurred ())
	PyErr_Print ();

    fclose (fp);
    free (python_current_script_filename);
    
    if (python_current_script == NULL)
    {
        plugin->print_server (plugin,
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
    plugin->print_server (plugin,
                          "Unloading Python script \"%s\"",
                          script->name);
    
    if (script->shutdown_func[0])
        weechat_python_exec (plugin, script, script->shutdown_func, NULL, NULL, NULL);

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
    int argc, handler_found;
    char **argv, *path_script;
    t_plugin_script *ptr_script;
    t_plugin_handler *ptr_handler;
    
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
                if ((ptr_handler->type == HANDLER_MESSAGE)
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
                if ((ptr_handler->type == HANDLER_COMMAND)
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
                if ((ptr_handler->type == HANDLER_TIMER)
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
                if ((ptr_handler->type == HANDLER_KEYBOARD)
                    && (ptr_handler->handler_args))
                {
                    handler_found = 1;
                    plugin->print_server (plugin, "  Python(%s)",
                                          ptr_handler->handler_args);
                }
            }
            if (!handler_found)
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
    
    Py_Initialize ();
    if (Py_IsInitialized () == 0)
    {
        plugin->print_server (plugin,
                              "Python error: unable to launch global interpreter");
        return PLUGIN_RC_KO;
    }

    PyEval_InitThreads();
    
    python_mainThreadState = PyThreadState_Get();
    
    if (python_mainThreadState == NULL)
    {
        plugin->print_server (plugin,
                              "Python error: unable to get current interpreter state");
        return PLUGIN_RC_KO;
    }
    
    plugin->cmd_handler_add (plugin, "python",
                             "list/load/unload Python scripts",
                             "[load filename] | [autoload] | [reload] | [unload]",
                             "filename: Python script (file) to load\n\n"
                             "Without argument, /python command lists all loaded Python scripts.",
                             "load|autoload|reload|unload",
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
        python_plugin->print_server (python_plugin,
                                     "Python error: unable to free interpreter");
    
    python_plugin->print_server (python_plugin,
                                 "Python plugin ended");
}
