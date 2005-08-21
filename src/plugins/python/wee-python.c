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
 * weechat.register: startup function for all WeeChat Python scripts
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
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Python", "register");
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
                    _("%s error: unable to register \"%s\" script (another script "
                    "already exists with this name)\n"),
                    "Python", name);
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
            
            wee_log_printf (_("Registered %s script: \"%s\", version %s (%s)\n"),
                            "Python", name, version, description);
        }
        else
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s error: unable to load script \"%s\" (not enough memory)\n"),
                        "Python", name);
        }
    }
    
    Py_INCREF (Py_None);
    return Py_None;
}

/*
 * weechat.print: print message into a buffer (current or specified one)
 */

static PyObject *
wee_python_print (PyObject *self, PyObject *args)
{
    char *message, *channel_name, *server_name;
    t_gui_buffer *ptr_buffer;
    
    /* make gcc happy */
    (void) self;
    
    message = NULL;
    channel_name = NULL;
    server_name = NULL;
    
    if (!PyArg_ParseTuple (args, "s|ss", &message, &channel_name, &server_name))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Python", "prnt");
        return NULL;
    }
    
    ptr_buffer = plugin_find_buffer (server_name, channel_name);
    if (ptr_buffer)
    {
        irc_display_prefix (ptr_buffer, PREFIX_PLUGIN);
        gui_printf (ptr_buffer, "%s\n", message);
        return Py_BuildValue ("i", 1);
    }
    
    /* buffer not found */
    return Py_BuildValue ("i", 0);
}

/*
 * weechat.print_infobar: print message to infobar
 */

static PyObject *
wee_python_print_infobar (PyObject *self, PyObject *args)
{
    int delay;
    char *message;
    
    /* make gcc happy */
    (void) self;
    
    delay = 1;
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "is", &delay, &message))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Python", "print_infobar");
        return NULL;
    }
    
    gui_infobar_printf (delay, COLOR_WIN_INFOBAR, message);
    
    Py_INCREF (Py_None);
    return Py_None;
}

/*
 * weechat.command: send command to server
 */

static PyObject *
wee_python_command (PyObject *self, PyObject *args)
{
    char *command, *channel_name, *server_name;
    t_gui_buffer *ptr_buffer;
    
    /* make gcc happy */
    (void) self;
    
    command = NULL;
    channel_name = NULL;
    server_name = NULL;
    
    if (!PyArg_ParseTuple (args, "s|ss", &command, &channel_name, &server_name))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Python", "command");
        return NULL;
    }
    
    ptr_buffer = plugin_find_buffer (server_name, channel_name);
    if (ptr_buffer)
    {
        user_command (SERVER(ptr_buffer), ptr_buffer, command);
        return Py_BuildValue ("i", 1);
    }
    
    /* buffer not found */
    return Py_BuildValue ("i", 0);
}

/*
 * weechat.add_message_handler: add handler for messages
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
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Python", "add_message_handler");
        return NULL;
    }
    
    plugin_handler_add (&plugin_msg_handlers, &last_plugin_msg_handler,
                        PLUGIN_TYPE_PYTHON, message, function);
    
    Py_INCREF (Py_None);
    return Py_None;
}

/*
 * weechat.add_command_handler: define/redefines commands
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
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Python", "add_command_handler");
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
 * weechat.get_info: get various infos
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
                    _("%s error: wrong parameters for \"%s\" function\n"),
                    "Python", "get_info");
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
                        _("%s error: server not found for \"%s\" function\n"),
                        "Python", "get_info");
            return NULL;
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
	else if ( (strcasecmp (arg, "100") == 0) || (strcasecmp (arg, "dccs") == 0) )
        {
	  t_irc_dcc *p = dcc_list;
	  int nbdccs = 0;
	  
	  for(; p; p = p->next_dcc)
	    nbdccs++;
	  
	  if (nbdccs == 0)
	    return Py_None;
	  
	  PyObject *list = PyList_New(nbdccs);

	  if (!list)
	    return Py_None;
	  
	  PyObject *listvalue;
	  
	  int pos = 0;
	  for(; p; p = p->next_dcc)
            {
	      listvalue = Py_BuildValue("{s:k,s:k,s:s,s:s,s:s,s:i,s:k,s:k,s:i,s:i}",
					 "address32", p->addr,
					 "cps", p->bytes_per_sec,
					 "remote_file", p->filename,
					 "local_file", p->local_filename,
					 "nick", p->nick,
					 "port", p->port, 
					 "pos", p->pos,
					 "size", p->size,
					 "status", p->status,
					 "type", p->type);
	      if (listvalue) 
		{
		  if (PyList_SetItem(list, pos, listvalue) != 0)
		    {
		      PyMem_Free(listvalue);
		      PyMem_Free(list);
		      return Py_None;
		    }
		}
	      else
		return Py_None;
	      pos++;
            }
	  
	  return list;
	}
        
        if (info)
            return Py_BuildValue ("s", info);
        else
            return Py_BuildValue ("s", "");
    }
    
    return Py_BuildValue ("i", 1);
}

/*
 * Python subroutines
 */

static
PyMethodDef weechat_funcs[] = {
    { "register", wee_python_register, METH_VARARGS, "" },
    { "prnt", wee_python_print, METH_VARARGS, "" },
    { "print_infobar", wee_python_print_infobar, METH_VARARGS, "" },
    { "command", wee_python_command, METH_VARARGS, "" },
    { "add_message_handler", wee_python_add_message_handler, METH_VARARGS, "" },
    { "add_command_handler", wee_python_add_command_handler, METH_VARARGS, "" },
    { "get_info", wee_python_get_info, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL }
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
        gui_printf (NULL, _("%s error: error while launching interpreter\n"),
                    "Python");
    }
    else
    {
        wee_log_printf (_("Loading %s module \"weechat\"\n"), "Python");
        Py_InitModule ("weechat", weechat_funcs);

	if (PyRun_SimpleString (
				"import weechat, sys, string\n"

				"class weechatStdout:\n"
				"\tdef write(self, str):\n"
				"\t\tstr = string.strip(str)\n"
				"\t\tif str != \"\":\n"
				"\t\t\tweechat.prnt(\"Python stdout : \" + str, \"\")\n"

				"class weechatStderr:\n"
				"\tdef write(self, str):\n"
				"\t\tstr = string.strip(str)\n"
				"\t\tif str != \"\":\n"
				"\t\t\tweechat.prnt(\"Python stderr : \" + str, \"\")\n"

				"sys.stdout = weechatStdout()\n"
				"sys.stderr = weechatStderr()\n"
				) != 0)
	  {
	    irc_display_prefix (NULL, PREFIX_PLUGIN);
            gui_printf (NULL,
                        _("%s error: error while redirecting stdout and stderr\n"),
                        "Python");
	  }
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
                    _("%s error: unable to run function \"%s\" in script (not enough memory)\n"),
                    "Python", function);
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
                    _("%s error: unable to run function \"%s\" in script (not enough memory)\n"),
                    "Python", function);
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
                        _("%s error: error while running function \"%s\"\n"),
                        "Python", function);
            return_code = 0;
        }
        free (runstring);
    }
    else
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: unable to run function \"%s\" in script (not enough memory)\n"),
                    "Python", function);
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
    wee_log_printf (_("Loading %s script \"%s\"\n"), "Python", filename);
    irc_display_prefix (NULL, PREFIX_PLUGIN);
    gui_printf (NULL, _("Loading %s script \"%s\"\n"), "Python", filename);
    
    if ((fp = fopen (filename, "r")) == NULL)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: error while opening file \"%s\"\n"),
                    "Python", filename);
        return 1;
    }
    
    if (PyRun_SimpleFile (fp, filename) != 0)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s error: error while parsing file \"%s\"\n"),
                    "Python", filename);
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
        wee_log_printf (_("Unloading %s script \"%s\"\n"),
                        "Python", ptr_python_script->name);
        
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
    wee_log_printf (_("Unloading all %s scripts...\n"), "Python");
    while (python_scripts)
        wee_python_unload (python_scripts);
    
    irc_display_prefix (NULL, PREFIX_PLUGIN);
    gui_printf (NULL, _("%s scripts unloaded\n"), "Python");
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

    /* free Python interpreter */
    Py_Finalize ();
    if (Py_IsInitialized () != 0)
    {
        irc_display_prefix (NULL, PREFIX_PLUGIN);
        gui_printf (NULL, _("%s error: error while freeing interpreter\n"),
                    "Python");
    }
}
