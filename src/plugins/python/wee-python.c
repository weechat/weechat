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

/* wee-python.c: Python plugin support for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Python.h>
#include <stdlib.h>
#include <string.h>
#undef _
#include "../../common/weechat.h"
#include "../plugins.h"
#include "wee-python.h"
#include "../../common/command.h"
#include "../../irc/irc.h"
#include "../../gui/gui.h"


t_plugin_script *python_scripts = NULL;
t_plugin_script *last_python_script = NULL;


/*
 * weechat.register(nom, version, fonction, description): 
 *   startup function for all WeeChat Python scripts
 */

static PyObject *
wee_python_register (PyObject *self, PyObject *args)
{
    char *name, *version, *shutdown_func, *description;
    t_plugin_script *ptr_python_script, *python_script_found, *new_python_script;

    /* make gcc happy */
    (void) self;
  
    if (!PyArg_ParseTuple (args, "ssss", &name, &version, &shutdown_func, &description))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: wrong parameters for 'print_with_channel' Python function\n"));
        return NULL;
    } 
    
    python_script_found = NULL;
    for (ptr_python_script = python_scripts; ptr_python_script;
         ptr_python_script = ptr_python_script->next_script)
    {
        if (strcasecmp (ptr_python_script->name, name) == 0)
        {
            python_script_found = ptr_python_script;
            break;
        }
    }
    
    if (python_script_found)
    {
        /* error: another scripts already exists with this name! */
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: unable to register Python script \"%s\" (another script "
                      "already exists with this name)\n"),
                    name);
    }
    else
    {
        /* registering script */
        new_python_script = (t_plugin_script *)malloc (sizeof (t_plugin_script));
        if (new_python_script)
        {
            new_python_script->name = strdup (name);
            new_python_script->version = strdup (version);
            new_python_script->shutdown_func = strdup (shutdown_func);
            new_python_script->description = strdup (description);
            
            /* add new script to list */
            new_python_script->prev_script = last_python_script;
            new_python_script->next_script = NULL;
            if (python_scripts)
                last_python_script->next_script = new_python_script;
            else
                python_scripts = new_python_script;
            last_python_script = new_python_script;
            
            wee_log_printf (_("registered Python script: \"%s\", version %s (%s)\n"),
                            name, version, description);
        }
        else
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s unable to load Python script \"%s\" (not enough memory)\n"),
                        WEECHAT_ERROR, name);
        }
    }
    
    Py_INCREF (Py_None);
    return Py_None;
}

/*
 * weechat.print(message): print message to current buffer
 */

static PyObject *
wee_python_print (PyObject *self, PyObject *args)
{
    char *message;
    
    /* make gcc happy */
    (void) self;
    
    if (!PyArg_ParseTuple (args, "s", &message))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: wrong parameters for 'print' Python function\n"));
        return NULL;
    }
    
    irc_display_prefix (gui_current_window->buffer, PREFIX_PLUGIN);
    gui_printf (gui_current_window->buffer, "%s\n", message);
    
    Py_INCREF (Py_None);
    return Py_None;
}

/*
 * weechat.print_with_channel(message, channel, server=None): 
 *       print message to a specific channel/server 
 *         (server is optional) 
 */

static PyObject *
wee_python_print_with_channel (PyObject *self, PyObject *args)
{
    char *message, *channel, *server = NULL;
    int ret = 0;
    t_gui_buffer *ptr_buffer;
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    
    /* make gcc happy */
    (void) self;
    
    if (!PyArg_ParseTuple (args, "ss|s", &message, &channel, &server))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: wrong parameters for 'print_with_channel' Python function\n"));
        return NULL;
    }
    
    ptr_buffer = NULL;
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (!server || (strcasecmp (ptr_server->name, server)) == 0)
        {
            for (ptr_channel = ptr_server->channels; ptr_channel;
                 ptr_channel = ptr_channel->next_channel)
            {
                if (strcasecmp (ptr_channel->name, channel) == 0)
                {
                    ptr_buffer = ptr_channel->buffer;
                    break;
                }
            }
        }
        if (ptr_buffer)
            break;
    }
  
    /* buffer found => display message & return 1 ~= True */
    if (ptr_buffer)
    {
        irc_display_prefix (ptr_buffer, PREFIX_PLUGIN);
        gui_printf (ptr_buffer, "%s", message);
        ret = 1;
    }
    
    return Py_BuildValue ("i", ret);
}

/*
 * weechat.print_infobar(delay, message): print message to infobar
 */

static PyObject *
wee_python_print_infobar (PyObject *self, PyObject *args)
{
    int delay;
    char *message;
    
    /* make gcc happy */
    (void) self;
    
    if (!PyArg_ParseTuple (args, "is", &delay, &message))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: wrong parameters for 'print_infobar' Python function\n"));
        return NULL;
    }
    
    gui_infobar_printf (delay, COLOR_WIN_INFOBAR, message);
    
    Py_INCREF (Py_None);
    return Py_None;
}

/*
 * weechat.command(command, server=None): send command to server
 */

static PyObject *
wee_python_command (PyObject *self, PyObject *args)
{
    char *server = NULL, *command, *command2;
    t_irc_server *ptr_server;
    
    /* make gcc happy */
    (void) self;
    
    if (!PyArg_ParseTuple (args, "s|s", &command, &server))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: wrong parameters for 'command' Python function\n"));
        return NULL;
    }
    
    if (server == NULL)
    {
        ptr_server = SERVER(gui_current_window->buffer);
    }
    else
    {
        for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
        {
            if (strcasecmp (ptr_server->name, server) == 0)
                break;
        }
        if (!ptr_server)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("Python error: server not found for 'command' Python function\n"));
        }
    }
    
    if (ptr_server)
    {
        command2 = (char *) malloc (strlen (command) + 8);
        strcpy (command2, command);
        if (!strstr (command2, "\r\n"))
            strcat (command2, "\r\n");
        server_sendf (ptr_server, command2);
        free (command2);
    }
    
    Py_INCREF (Py_None);
    return Py_None;
}

/*
 * weechat.add_message_handler(message, function):
 *   add handler for messages
 */

static PyObject *
wee_python_add_message_handler (PyObject *self, PyObject *args)
{
    char *message, *function;
    
    /* make gcc happy */
    (void) self;
    
    if (!PyArg_ParseTuple (args, "ss", &message, &function))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: wrong parameters for 'add_message_handler' Python function\n"));
        return NULL;
    }
    
    plugin_handler_add (&plugin_msg_handlers, &last_plugin_msg_handler,
                        PLUGIN_TYPE_PYTHON, message, function);
    
    Py_INCREF (Py_None);
    return Py_None;
}

/*
 * weechat.add_command_handler(name, function):
 *   define/redefines commands
 */

static PyObject *
wee_python_add_command_handler(PyObject *self, PyObject *args)
{
    char *name, *function;
    t_plugin_handler *ptr_plugin_handler;
    
    /* make gcc happy */
    (void) self;
    
    if (!PyArg_ParseTuple (args, "ss", &name, &function))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: wrong parameters for 'add_command_handler' Python function\n"));
        return NULL; 
    }
    
    if (!weelist_search (index_commands, name))
        weelist_add (&index_commands, &last_index_command, name);
    
    ptr_plugin_handler = plugin_handler_search (plugin_cmd_handlers, name);
    if (ptr_plugin_handler)
    {
        free (ptr_plugin_handler->function_name);
        ptr_plugin_handler->function_name = strdup (function);
    }
    else
        plugin_handler_add (&plugin_cmd_handlers, &last_plugin_cmd_handler,
                            PLUGIN_TYPE_PYTHON, name, function);
    
    Py_INCREF (Py_None);
    return Py_None;
}

/*
 * weechat.get_info(info, server=None):
 *   get various infos
 */

static PyObject *
wee_python_get_info (PyObject *self, PyObject *args)
{
    char *arg, *info = NULL, *server = NULL;
    t_irc_server *ptr_server;
    
    /* make gcc happy */
    (void) self;
    
    if (!PyArg_ParseTuple (args, "s|s", &arg, &server))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: wrong parameters for 'get_info' Python function\n"));
        return NULL; 
    }
    
    if (server == NULL)
    {
        ptr_server = SERVER(gui_current_window->buffer);
    }
    else
    {
        for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
        {
            if (strcasecmp (ptr_server->name, server) == 0)
                break;
        }
        if (!ptr_server)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("Python error: server not found for 'get_info' Python function\n"));
        }
    }
    
    if (ptr_server && arg)
    {
        if ( (strcasecmp (arg, "0") == 0) || (strcasecmp (arg, "version") == 0) )
        {
            info = PACKAGE_STRING;
        }
        else if ( (strcasecmp (arg, "1") == 0) || (strcasecmp (arg, "nick") == 0) )
        {
            if (ptr_server->nick)
                info = ptr_server->nick;
        }
        else if ( (strcasecmp (arg, "2") == 0) || (strcasecmp (arg, "channel") == 0) )
        {
            if (BUFFER_IS_CHANNEL (gui_current_window->buffer))
                info = CHANNEL (gui_current_window->buffer)->name;
        }
        else if ( (strcasecmp (arg, "3") == 0) || (strcasecmp (arg, "server") == 0) )
        {
            if (ptr_server->name)
                info = ptr_server->name;
        }
        else if ( (strcasecmp (arg, "4") == 0) || (strcasecmp (arg, "weechatdir") == 0) )
        {
            info = weechat_home;
        }
        else if ( (strcasecmp (arg, "5") == 0) || (strcasecmp (arg, "away") == 0) )
        {	 
            return Py_BuildValue ("i", SERVER(gui_current_window->buffer)->is_away);
        }
        
        if (info)
            return Py_BuildValue ("s", info);
        else
            return Py_BuildValue ("s", "");
    }
    
    return Py_BuildValue ("i", 1);
}

/*
 * initialize subroutines
 */

static
PyMethodDef weechat_funcs[] = {
    {"register", wee_python_register, METH_VARARGS, ""},
    {"prnt", wee_python_print, METH_VARARGS, ""},
    {"print_with_channel", wee_python_print_with_channel, METH_VARARGS, ""},
    {"print_infobar", wee_python_print_infobar, METH_VARARGS, ""},
    {"command", wee_python_command, METH_VARARGS, ""},
    {"add_message_handler", wee_python_add_message_handler, METH_VARARGS, ""},
    {"add_command_handler", wee_python_add_command_handler, METH_VARARGS, ""},
    {"get_info", wee_python_get_info, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}
};

/*
 * wee_python_init: initialize Python interface for WeeChat
 */

void
wee_python_init ()
{

    Py_Initialize ();
    if (Py_IsInitialized () == 0)
    {
        irc_display_prefix (NULL, PREFIX_PLUGIN);
        gui_printf (NULL, _("Python error: error while launching Python interpreter\n"));
    }
    else
    {
        Py_InitModule ("weechat", weechat_funcs);
        irc_display_prefix (NULL, PREFIX_PLUGIN);
        gui_printf (NULL, _("Loading Python module \"weechat\"\n"));
    }
}

/*
 * wee_python_search: search a (loaded) Python script by name
 */

t_plugin_script *
wee_python_search (char *name)
{
    t_plugin_script *ptr_python_script;
    
    for (ptr_python_script = python_scripts; ptr_python_script;
         ptr_python_script = ptr_python_script->next_script)
    {
        if (strcmp (ptr_python_script->name, name) == 0)
            return ptr_python_script;
    }
    
    /* script not found */
    return NULL;
}

/*
 * wee_python_exec: execute a Python script
 */

int
wee_python_exec (char *function, char *server, char *arguments)
{  
    int len, return_code, i, j, alen;
    char *srv, *args, *runstring;
    
    return_code = 1;
    
    if (arguments == NULL)
    {
        alen = 0;
        args = strdup ("");
    }
    else 
    {
        alen = (int) strlen (arguments);
        args = (char *) malloc ( 2 * alen * sizeof (*args));
    }
    
    if (!args)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: unable to run function \"%s\"in Python script (not enough memory)\n"),
                    function);
        return 0;
    }
    
    // copy arguments in args to escape double quotes
    j = 0;
    for (i=0; i < alen; ++i)
    {
        if (arguments[i] == '"' || arguments[i] == '\\')
        {
            args[j] = '\\';
            ++j;
        }
        args[j] = arguments[i];
        ++j;
    }
    args[j] = 0;
    
    if (server == NULL)
        srv = strdup ("");
    else 
        srv = strdup (server);
    
    if (!srv)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: unable to run function \"%s\"in Python script (not enough memory)\n"),
                    function);
        free (args);
        return 0;
    }
    
    len = (int) strlen (function);
    len += (int) strlen ("(\"\",\"\")");
    len += (int) strlen (srv);
    len += (int) strlen (args);
    len += 1;
    
    runstring = (char *) malloc ( len * sizeof (*runstring));
    
    if (runstring)
    {
        sprintf (runstring, "%s(\"%s\",\"%s\")", function, srv, args);
      
        if (PyRun_SimpleString (runstring) != 0)
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("Python error: error while running function \"%s\"\n"), function);
            return_code = 0;
        }
        free (runstring);
    }
    else
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: unable to run function \"%s\"in Python script (not enough memory)\n"),
                    function);
        return_code = 0;
    }
    
    free (args);
    free (srv);
    
    return return_code;
}

/*
 * wee_python_load: load a Python script
 */

int
wee_python_load (char *filename)
{
    FILE *fp;
    
    /* execute Python script */
    wee_log_printf (_("loading Python script \"%s\"\n"), filename);
    irc_display_prefix (NULL, PREFIX_PLUGIN);
    gui_printf (NULL, _("Loading Python script \"%s\"\n"), filename);
    
    if ((fp = fopen (filename, "r")) == NULL)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: error while opening file \"%s\"\n"), filename);
        return 1;
    }
    
    if (PyRun_SimpleFile (fp, filename) != 0)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("Python error: error while parsing file \"%s\"\n"), filename);
        return 1;
    }
    
    fclose (fp);
    return 0;
}

/*
 * wee_python_script_free: free a Python script
 */

void
wee_python_script_free (t_plugin_script *ptr_python_script)
{
    t_plugin_script *new_python_scripts;

    /* remove script from list */
    if (last_python_script == ptr_python_script)
        last_python_script = ptr_python_script->prev_script;
    if (ptr_python_script->prev_script)
    {
        (ptr_python_script->prev_script)->next_script = ptr_python_script->next_script;
        new_python_scripts = python_scripts;
    }
    else
        new_python_scripts = ptr_python_script->next_script;
    
    if (ptr_python_script->next_script)
        (ptr_python_script->next_script)->prev_script = ptr_python_script->prev_script;

    /* free data */
    if (ptr_python_script->name)
        free (ptr_python_script->name);
    if (ptr_python_script->version)
        free (ptr_python_script->version);
    if (ptr_python_script->shutdown_func)
        free (ptr_python_script->shutdown_func);
    if (ptr_python_script->description)
        free (ptr_python_script->description);
    free (ptr_python_script);
    python_scripts = new_python_scripts;
}

/*
 * wee_python_unload: unload a Python script
 */

void
wee_python_unload (t_plugin_script *ptr_python_script)
{
    if (ptr_python_script)
    {
        wee_log_printf (_("unloading Python script \"%s\"\n"),
                        ptr_python_script->name);
        
        /* call shutdown callback function */
        if (ptr_python_script->shutdown_func[0])
            wee_python_exec (ptr_python_script->shutdown_func, "", "");
        wee_python_script_free (ptr_python_script);
    }
}

/*
 * wee_python_unload_all: unload all Python scripts
 */

void
wee_python_unload_all ()
{
    wee_log_printf (_("unloading all Python scripts...\n"));
    while (python_scripts)
        wee_python_unload (python_scripts);
}

/*
 * wee_python_end: shutdown Python interface
 */

void
wee_python_end ()
{
    /* unload all scripts */
    wee_python_unload_all ();
    
    /* free all handlers */
    plugin_handler_free_all_type (&plugin_msg_handlers,
                                  &last_plugin_msg_handler,
                                  PLUGIN_TYPE_PYTHON);
    plugin_handler_free_all_type (&plugin_cmd_handlers,
                                  &last_plugin_cmd_handler,
                                  PLUGIN_TYPE_PYTHON);

    Py_Finalize ();
    if (Py_IsInitialized () != 0) {
      irc_display_prefix (NULL, PREFIX_PLUGIN);
      gui_printf (NULL, _("Python error: error while freeing Python interpreter\n"));
    }
}
