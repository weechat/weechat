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

/* scripts.c: script interface for WeeChat plugins */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../weechat-plugin.h"
#include "weechat-script.h"


/*
 * weechat_script_auto_load: auto-load all scripts in a directory
 */

void
weechat_script_auto_load (t_weechat_plugin *plugin, char *language,
                          int (*callback)(t_weechat_plugin *, char *))
{
    char *dir_home, *dir_name;
    int dir_length;
    
    /* build directory, adding WeeChat home */
    dir_home = plugin->get_info (plugin, "weechat_dir", NULL);
    if (!dir_home)
        return;
    dir_length = strlen (dir_home) + strlen (language) + 16;
    dir_name =
        (char *) malloc (dir_length * sizeof (char));
    if (!dir_name)
    {
        free (dir_home);
        return;
    }
    snprintf (dir_name, dir_length, "%s/%s/autoload", dir_home, language);
    
    plugin->exec_on_files (plugin, dir_name, callback);
    
    free (dir_name);
    free (dir_home);
}

/*
 * weechat_script_search: search a script in list
 */

t_plugin_script *
weechat_script_search (t_weechat_plugin *plugin,
                       t_plugin_script **list, char *name)
{
    t_plugin_script *ptr_script;
    
    for (ptr_script = *list; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (plugin->ascii_strcasecmp (plugin, ptr_script->name, name) == 0)
            return ptr_script;
    }
    
    /* script not found */
    return NULL;
}

/*
 * weechat_script_add: add a script to list of scripts
 */

t_plugin_script *
weechat_script_add (t_weechat_plugin *plugin,
                    t_plugin_script **script_list,
                    char *filename,
                    char *name, char *version,
                    char *shutdown_func, char *description)
{
    t_plugin_script *new_script;
    
    /* make gcc happy */
    (void) plugin;
    
    new_script = (t_plugin_script *)malloc (sizeof (t_plugin_script));
    if (new_script)
    {
        new_script->filename = strdup (filename);
        new_script->interpreter = NULL;
        new_script->name = strdup (name);
        new_script->version = strdup (version);
        new_script->shutdown_func = strdup (shutdown_func);
        new_script->description = strdup (description);
        
        /* add new script to list */
        if ((*script_list))
            (*script_list)->prev_script = new_script;
        new_script->prev_script = NULL;
        new_script->next_script = (*script_list);
        (*script_list) = new_script;
        return new_script;
    }
    
    return NULL;
}

/*
 * weechat_script_remove: remove a script from list of scripts
 */

void
weechat_script_remove (t_weechat_plugin *plugin,
                       t_plugin_script **script_list, t_plugin_script *script)
{
    t_plugin_handler *ptr_handler, *next_handler;
    
    /* remove all handlers pointing to script */
    ptr_handler = plugin->handlers;
    while (ptr_handler)
    {
        if ((t_plugin_script *)ptr_handler->handler_pointer == script)
        {
            next_handler = ptr_handler->next_handler;
            plugin->handler_remove (plugin, ptr_handler);
            ptr_handler = next_handler;
        }
        else
            ptr_handler = ptr_handler->next_handler;
    }
    
    /* free data */
    if (script->filename)
        free (script->filename);
    if (script->name)
        free (script->name);
    if (script->description)
        free (script->description);
    if (script->version)
        free (script->version);
    if (script->shutdown_func)
        free (script->shutdown_func);
    
    /* remove script from list */
    if (script->prev_script)
        (script->prev_script)->next_script = script->next_script;
    else
        (*script_list) = script->next_script;
    if (script->next_script)
        (script->next_script)->prev_script = script->prev_script;
    
    /* free script */
    free (script);
}

/*
 * weechat_script_remove_handler: remove a handler for a script
 *                                for a msg handler, arg1=irc command, arg2=function
 *                                for a cmd handler, arg1=command, arg2=function
 */

void
weechat_script_remove_handler (t_weechat_plugin *plugin,
                               t_plugin_script *script,
                               char *arg1, char *arg2)
{
    t_plugin_handler *ptr_handler, *next_handler;
    char *ptr_arg1;
    
    /* search and remove message handlers */
    ptr_handler = plugin->handlers;
    while (ptr_handler)
    {
        ptr_arg1 = NULL;
        if (ptr_handler->type == HANDLER_MESSAGE)
            ptr_arg1 = ptr_handler->irc_command;
        else if (ptr_handler->type == HANDLER_COMMAND)
            ptr_arg1 = ptr_handler->command;
        
        if ((ptr_arg1)
            && ((t_plugin_script *)ptr_handler->handler_pointer == script)
            && (plugin->ascii_strcasecmp (plugin, ptr_arg1, arg1) == 0)
            && (plugin->ascii_strcasecmp (plugin, ptr_handler->handler_args, arg2) == 0))
        {
            next_handler = ptr_handler->next_handler;
            plugin->handler_remove (plugin, ptr_handler);
            ptr_handler = next_handler;
        }
        else
            ptr_handler = ptr_handler->next_handler;
    }
}

/*
 * weechat_script_get_plugin_config: get a value of a script option
 *                                   format in file is: plugin.script.option=value
 */

char *
weechat_script_get_plugin_config (t_weechat_plugin *plugin,
                                  t_plugin_script *script,
                                  char *option)
{
    char *option_fullname, *return_value;
    
    option_fullname = (char *)malloc (strlen (script->name) +
                                      strlen (option) + 2);
    if (!option_fullname)
        return NULL;
    
    strcpy (option_fullname, script->name);
    strcat (option_fullname, ".");
    strcat (option_fullname, option);
    
    return_value = plugin->get_plugin_config (plugin, option_fullname);
    free (option_fullname);
    
    return return_value;
}

/*
 * weechat_script_set_plugin_config: set value of a script config option
 *                                   format in file is: plugin.script.option=value
 */

int
weechat_script_set_plugin_config (t_weechat_plugin *plugin,
                                  t_plugin_script *script,
                                  char *option, char *value)
{
    char *option_fullname;
    int return_code;
    
    option_fullname = (char *)malloc (strlen (script->name) +
                                      strlen (option) + 2);
    if (!option_fullname)
        return 0;
    
    strcpy (option_fullname, script->name);
    strcat (option_fullname, ".");
    strcat (option_fullname, option);
    
    return_code = plugin->set_plugin_config (plugin, option_fullname, value);
    free (option_fullname);
    
    return return_code;
}
