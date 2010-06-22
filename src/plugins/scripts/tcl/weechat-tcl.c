/*
 * Copyright (C) 2008-2010 Dmitry Kobylin <fnfal@academ.tsc.ru>
 * Copyright (C) 2008-2010 Sebastien Helleu <flashcode@flashtux.org>
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
 * weechat-tcl.c: tcl plugin for WeeChat
 */

#undef _

#include <tcl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "weechat-tcl.h"
#include "weechat-tcl-api.h"


WEECHAT_PLUGIN_NAME(TCL_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("Tcl plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("Dmitry Kobylin <fnfal@academ.tsc.ru>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_tcl_plugin = NULL;

int tcl_quiet = 0;
struct t_plugin_script *tcl_scripts = NULL;
struct t_plugin_script *last_tcl_script = NULL;
struct t_plugin_script *tcl_current_script = NULL;
struct t_plugin_script *tcl_registered_script = NULL;
const char *tcl_current_script_filename = NULL;

/*
 * string used to execute action "install":
 * when signal "tcl_install_script" is received, name of string
 * is added to this string, to be installed later by a timer (when nothing is
 * running in script)
 */
char *tcl_action_install_list = NULL;

/*
 * string used to execute action "remove":
 * when signal "tcl_remove_script" is received, name of string
 * is added to this string, to be removed later by a timer (when nothing is
 * running in script)
 */
char *tcl_action_remove_list = NULL;

Tcl_Interp* cinterp;


/*
 * weechat_tcl_exec: execute a Tcl script
 */

void *
weechat_tcl_exec (struct t_plugin_script *script,
                  int ret_type, const char *function, char **argv)
{
    int i, llength;
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
        cmdlist = Tcl_NewListObj(0,NULL);
        Tcl_IncrRefCount(cmdlist); /* +1 */
        Tcl_ListObjAppendElement(interp,cmdlist,Tcl_NewStringObj(function,-1));
    }
    else
    {
        tcl_current_script = old_tcl_script;
        return NULL;
    }
    
    if (argv)
    {
        for (i = 0; argv[i]; i++)
        {
            Tcl_ListObjAppendElement(interp,cmdlist,Tcl_NewStringObj(argv[i],-1));
        }
    }

    if (Tcl_ListObjLength(interp,cmdlist,&llength) != TCL_OK)
        llength = 0;

    if (Tcl_EvalObjEx (interp, cmdlist, TCL_EVAL_DIRECT) == TCL_OK)
    {
        Tcl_ListObjReplace(interp,cmdlist,0,llength,0,NULL); /* remove elements, decrement their ref count */
        Tcl_DecrRefCount (cmdlist); /* -1 */
        ret_val = NULL;
        if (ret_type == WEECHAT_SCRIPT_EXEC_STRING)
        {
            ret_cv = Tcl_GetStringFromObj (Tcl_GetObjResult (interp), &i);
            if (ret_cv)
                ret_val = (void *)strdup (ret_cv);
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
        
        tcl_current_script = old_tcl_script;
        if (ret_val)
            return ret_val;
        
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"%s\" must return a "
                                         "valid value"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, function);
        return NULL;
    }
    
    Tcl_ListObjReplace(interp,cmdlist,0,llength,0,NULL); /* remove elements, decrement their ref count */
    Tcl_DecrRefCount(cmdlist); /* -1 */
    weechat_printf (NULL,
                    weechat_gettext ("%s%s: unable to run function \"%s\": %s"),
                    weechat_prefix ("error"), TCL_PLUGIN_NAME, function,
                    Tcl_GetStringFromObj (Tcl_GetObjResult (interp), &i));
    tcl_current_script = old_tcl_script;
    
    return NULL;
}

/*
 * weechat_tcl_load: load a Tcl script
 */

int
weechat_tcl_load (const char *filename)
{
    int i;
    Tcl_Interp *interp;
    struct stat buf;
  
    if (stat (filename, &buf) != 0)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not found"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, filename);
        return 0;
    }
    
    if ((weechat_tcl_plugin->debug >= 1) || !tcl_quiet)
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
        return 0;
    }
    tcl_current_script_filename = filename;
    
    weechat_tcl_api_init (interp);

    if (Tcl_EvalFile (interp, filename) != TCL_OK)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: error occured while "
                                         "parsing file \"%s\": %s"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, filename,
                        Tcl_GetStringFromObj (Tcl_GetObjResult (interp), &i));
        /* this ok, maybe "register" was called, so not return */
        /* return 0; */
    }

    if (!tcl_registered_script)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: function \"register\" not "
                                         "found (or failed) in file \"%s\""),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, filename);
        Tcl_DeleteInterp (interp);
        return 0;
    }
    tcl_current_script = tcl_registered_script;
    
    return 1;
}

/*
 * weechat_tcl_load_cb: callback for weechat_script_auto_load() function
 */

void
weechat_tcl_load_cb (void *data, const char *filename)
{
    /* make C compiler happy */
    (void) data;
    
    weechat_tcl_load (filename);
}

/*
 * weechat_tcl_unload: unload a Tcl script
 */

void
weechat_tcl_unload (struct t_plugin_script *script)
{
    Tcl_Interp* interp;
    void *pointer;
    
    if ((weechat_tcl_plugin->debug >= 1) || !tcl_quiet)
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s: unloading script \"%s\""),
                        TCL_PLUGIN_NAME, script->name);
    }
    
    if (script->shutdown_func && script->shutdown_func[0])
    {
        pointer = weechat_tcl_exec (script,
                                    WEECHAT_SCRIPT_EXEC_INT,
                                    script->shutdown_func,
                                    NULL);
        if (pointer)
            free (pointer);
    }
    
    interp = (Tcl_Interp*)script->interpreter;
    
    if (tcl_current_script == script)
        tcl_current_script = (tcl_current_script->prev_script) ?
            tcl_current_script->prev_script : tcl_current_script->next_script;
    
    script_remove (weechat_tcl_plugin, &tcl_scripts, &last_tcl_script, script);
    
    Tcl_DeleteInterp(interp);
}
 
/*
 * weechat_tcl_unload_name: unload a Tcl script by name
 */
 
void
weechat_tcl_unload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    
    ptr_script = script_search (weechat_tcl_plugin, tcl_scripts, name);
    if (ptr_script)
    {
        weechat_tcl_unload (ptr_script);
        weechat_printf (NULL,
                        weechat_gettext ("%s: script \"%s\" unloaded"),
                        TCL_PLUGIN_NAME, name);
    }
    else
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: script \"%s\" not loaded"),
                        weechat_prefix ("error"), TCL_PLUGIN_NAME, name);
    }
}

/*
 * weechat_tcl_unload_all: unload all Tcl scripts
 */

void
weechat_tcl_unload_all ()
{
    while (tcl_scripts)
    {
        weechat_tcl_unload (tcl_scripts);
    }
}

/*
 * weechat_tcl_reload_name: reload a Tcl script by name
 */

void
weechat_tcl_reload_name (const char *name)
{
    struct t_plugin_script *ptr_script;
    char *filename;
    
    ptr_script = script_search (weechat_tcl_plugin, tcl_scripts, name);
    if (ptr_script)
    {
        filename = strdup (ptr_script->filename);
        if (filename)
        {
            weechat_tcl_unload (ptr_script);
            weechat_printf (NULL,
                            weechat_gettext ("%s: script \"%s\" unloaded"),
                            TCL_PLUGIN_NAME, name);
            weechat_tcl_load (filename);
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
 * weechat_tcl_command_cb: callback for "/tcl" command
 */

int
weechat_tcl_command_cb (void *data, struct t_gui_buffer *buffer,
                         int argc, char **argv, char **argv_eol)
{
    char *path_script;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    if (argc == 1)
    {
        script_display_list (weechat_tcl_plugin, tcl_scripts,
                             NULL, 0);
    }
    else if (argc == 2)
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            script_display_list (weechat_tcl_plugin, tcl_scripts,
                                 NULL, 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            script_display_list (weechat_tcl_plugin, tcl_scripts,
                                 NULL, 1);
        }
        else if (weechat_strcasecmp (argv[1], "autoload") == 0)
        {
            script_auto_load (weechat_tcl_plugin, &weechat_tcl_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            weechat_tcl_unload_all ();
            script_auto_load (weechat_tcl_plugin, &weechat_tcl_load_cb);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            weechat_tcl_unload_all ();
        }
    }
    else
    {
        if (weechat_strcasecmp (argv[1], "list") == 0)
        {
            script_display_list (weechat_tcl_plugin, tcl_scripts,
                                 argv_eol[2], 0);
        }
        else if (weechat_strcasecmp (argv[1], "listfull") == 0)
        {
            script_display_list (weechat_tcl_plugin, tcl_scripts,
                                 argv_eol[2], 1);
        }
        else if (weechat_strcasecmp (argv[1], "load") == 0)
        {
            /* load Tcl script */
            path_script = script_search_path (weechat_tcl_plugin,
                                              argv_eol[2]);
            weechat_tcl_load ((path_script) ? path_script : argv_eol[2]);
            if (path_script)
                free (path_script);
        }
        else if (weechat_strcasecmp (argv[1], "reload") == 0)
        {
            /* reload one Tcl script */
            weechat_tcl_reload_name (argv_eol[2]);
        }
        else if (weechat_strcasecmp (argv[1], "unload") == 0)
        {
            /* unload Tcl script */
            weechat_tcl_unload_name (argv_eol[2]);
        }
        else
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s%s: unknown option for "
                                             "command \"%s\""),
                            weechat_prefix ("error"), TCL_PLUGIN_NAME, "tcl");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_tcl_completion_cb: callback for script completion
 */

int
weechat_tcl_completion_cb (void *data, const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    script_completion (weechat_tcl_plugin, completion, tcl_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_tcl_infolist_cb: callback for infolist
 */

struct t_infolist *
weechat_tcl_infolist_cb (void *data, const char *infolist_name,
                         void *pointer, const char *arguments)
{
    /* make C compiler happy */
    (void) data;
    
    if (!infolist_name || !infolist_name[0])
        return NULL;
    
    if (weechat_strcasecmp (infolist_name, "tcl_script") == 0)
    {
        return script_infolist_list_scripts (weechat_tcl_plugin,
                                             tcl_scripts, pointer,
                                             arguments);
    }
    
    return NULL;
}

/*
 * weechat_tcl_signal_debug_dump_cb: dump Tcl plugin data in WeeChat log file
 */

int
weechat_tcl_signal_debug_dump_cb (void *data, const char *signal,
                                  const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    script_print_log (weechat_tcl_plugin, tcl_scripts);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_tcl_signal_buffer_closed_cb: callback called when a buffer is closed
 */

int
weechat_tcl_signal_buffer_closed_cb (void *data, const char *signal,
                                     const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    if (signal_data)
        script_remove_buffer_callbacks (tcl_scripts, signal_data);
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_tcl_timer_action_cb: timer for executing actions
 */

int
weechat_tcl_timer_action_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    if (data)
    {
        if (data == &tcl_action_install_list)
        {
            script_action_install (weechat_tcl_plugin,
                                   tcl_scripts,
                                   &weechat_tcl_unload,
                                   &weechat_tcl_load,
                                   &tcl_action_install_list);
        }
        else if (data == &tcl_action_remove_list)
        {
            script_action_remove (weechat_tcl_plugin,
                                  tcl_scripts,
                                  &weechat_tcl_unload,
                                  &tcl_action_remove_list);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_tcl_signal_script_action_cb: callback called when a script action
 *                                      is asked (install/remove a script)
 */

int
weechat_tcl_signal_script_action_cb (void *data, const char *signal,
                                     const char *type_data,
                                     void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (strcmp (signal, "tcl_script_install") == 0)
        {
            script_action_add (&tcl_action_install_list,
                               (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_tcl_timer_action_cb,
                                &tcl_action_install_list);
        }
        else if (strcmp (signal, "tcl_script_remove") == 0)
        {
            script_action_add (&tcl_action_remove_list,
                               (const char *)signal_data);
            weechat_hook_timer (1, 0, 1,
                                &weechat_tcl_timer_action_cb,
                                &tcl_action_remove_list);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: initialize Tcl plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    weechat_tcl_plugin = plugin;
    
    tcl_quiet = 1;
    script_init (weechat_tcl_plugin,
                 argc,
                 argv,
                 &tcl_scripts,
                 &weechat_tcl_command_cb,
                 &weechat_tcl_completion_cb,
                 &weechat_tcl_infolist_cb,
                 &weechat_tcl_signal_debug_dump_cb,
                 &weechat_tcl_signal_buffer_closed_cb,
                 &weechat_tcl_signal_script_action_cb,
                 &weechat_tcl_load_cb,
                 &weechat_tcl_api_buffer_input_data_cb,
                 &weechat_tcl_api_buffer_close_cb);
    tcl_quiet = 0;
    
    script_display_short_list (weechat_tcl_plugin,
                               tcl_scripts);
    
    /* init ok */
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end Tcl plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* unload all scripts */
    tcl_quiet = 1;
    script_end (plugin, &tcl_scripts, &weechat_tcl_unload_all);
    tcl_quiet = 0;
    
    return WEECHAT_RC_OK;
}
