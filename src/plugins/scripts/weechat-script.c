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
    dir_home = plugin->get_info (plugin, "weechat_dir", NULL, NULL);
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
    t_plugin_msg_handler *ptr_msg_handler, *next_msg_handler;
    t_plugin_cmd_handler *ptr_cmd_handler, *next_cmd_handler;

    /* make gcc happy */
    (void) plugin;
    
    /* remove message handlers */
    ptr_msg_handler = plugin->msg_handlers;
    while (ptr_msg_handler)
    {
        if ((t_plugin_script *)ptr_msg_handler->msg_handler_pointer == script)
        {
            next_msg_handler = ptr_msg_handler->next_handler;
            plugin->msg_handler_remove (plugin, ptr_msg_handler);
            ptr_msg_handler = next_msg_handler;
        }
        else
            ptr_msg_handler = ptr_msg_handler->next_handler;
    }
    
    /* remove command handlers */
    ptr_cmd_handler = plugin->cmd_handlers;
    while (ptr_cmd_handler)
    {
        if ((t_plugin_script *)ptr_cmd_handler->cmd_handler_pointer == script)
        {
            next_cmd_handler = ptr_cmd_handler->next_handler;
            plugin->cmd_handler_remove (plugin, ptr_cmd_handler);
            ptr_cmd_handler = next_cmd_handler;
        }
        else
            ptr_cmd_handler = ptr_cmd_handler->next_handler;
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
