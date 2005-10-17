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

/* plugins.c: manages WeeChat plugins (dynamic C libraries) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include "../common/weechat.h"
#include "weechat-plugin.h"
#include "plugins.h"
#include "../common/command.h"
#include "../common/weeconfig.h"
#include "../irc/irc.h"
#include "../gui/gui.h"


t_weechat_plugin *weechat_plugins = NULL;
t_weechat_plugin *last_weechat_plugin = NULL;


/*
 * plugin_find_buffer: find a buffer for text display or command execution
 */

t_gui_buffer *
plugin_find_buffer (char *server, char *channel)
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    t_gui_buffer *ptr_buffer;
    
    ptr_server = NULL;
    ptr_channel = NULL;
    ptr_buffer = NULL;
    
    if (server && server[0])
    {
        ptr_server = server_search (server);
        if (!ptr_server)
            return NULL;
    }
    else
    {
        ptr_server = SERVER(gui_current_window->buffer);
        if (!ptr_server)
            ptr_server = SERVER(gui_buffers);
    }
    
    if (channel && channel[0])
    {
        if (ptr_server)
        {
            ptr_channel = channel_search (ptr_server, channel);
            if (ptr_channel)
                ptr_buffer = ptr_channel->buffer;
        }
    }
    else
    {
        if (!channel)
        {
            ptr_buffer = gui_current_window->buffer;
            if (ptr_buffer->dcc)
                ptr_buffer = gui_buffers;
        }
        else
        {
            if (ptr_server)
                ptr_buffer = ptr_server->buffer;
        }
    }
    
    if (!ptr_buffer)
        return NULL;
    
    return (ptr_buffer->dcc) ? NULL : ptr_buffer;
}

/*
 * plugin_exec_on_files: find files in a directory and execute a
 *                       function on each file
 */

void
plugin_exec_on_files (t_weechat_plugin *plugin, char *directory,
                      int (*callback)(t_weechat_plugin *, char *))
{
    char complete_filename[1024];
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    
    dir = opendir (directory);
    if (dir)
    {
        while ((entry = readdir (dir)))
        {
            snprintf (complete_filename, sizeof (complete_filename) - 1,
                      "%s/%s", directory, entry->d_name);
            lstat (complete_filename, &statbuf);
            if (!S_ISDIR(statbuf.st_mode))
            {
                (int) (*callback) (plugin, complete_filename);
            }
        }
        closedir (dir);
    }
}

/*
 * plugin_search: search a plugin by name
 */

t_weechat_plugin *
plugin_search (char *name)
{
    t_weechat_plugin *ptr_plugin;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        if (ascii_strcasecmp (ptr_plugin->name, name) == 0)
            return ptr_plugin;
    }
    
    /* plugin not found */
    return NULL;
}

/*
 * plugin_cmd_handler_search: search a plugin command handler
 *                            return: pointer to handler, NULL if not found
 */

t_plugin_cmd_handler *
plugin_cmd_handler_search (char *command)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_cmd_handler *ptr_plugin_cmd_handler;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_plugin_cmd_handler = ptr_plugin->cmd_handlers;
             ptr_plugin_cmd_handler;
             ptr_plugin_cmd_handler = ptr_plugin_cmd_handler->next_handler)
        {
            if (ascii_strcasecmp (ptr_plugin_cmd_handler->command, command) == 0)
                return ptr_plugin_cmd_handler;
        }
    }
    
    /* command handler not found */
    return NULL;
}

/*
 * plugin_msg_handler_add: add a message handler
 *                         arguments:
 *                           1. the plugin pointer
 *                           2. the IRC command
 *                           3. the handler function
 *                           4. handler args: a string given to
 *                              handler when called (used by scripts)
 *                           5. handler pointer: a pointer given to
 *                              handler when called (used by scripts)
 */

t_plugin_msg_handler *
plugin_msg_handler_add (t_weechat_plugin *plugin, char *irc_command,
                        t_plugin_handler_func *handler_func,
                        char *handler_args, void *handler_pointer)
{
    t_plugin_msg_handler *new_plugin_msg_handler;
    
    new_plugin_msg_handler = (t_plugin_msg_handler *)malloc (sizeof (t_plugin_msg_handler));
    if (new_plugin_msg_handler)
    {
        new_plugin_msg_handler->irc_command = strdup (irc_command);
        new_plugin_msg_handler->msg_handler = handler_func;
        new_plugin_msg_handler->msg_handler_args = (handler_args) ? strdup (handler_args) : NULL;
        new_plugin_msg_handler->msg_handler_pointer = handler_pointer;
        new_plugin_msg_handler->running = 0;
        
        /* add new handler to list */
        new_plugin_msg_handler->prev_handler = plugin->last_msg_handler;
        new_plugin_msg_handler->next_handler = NULL;
        if (plugin->msg_handlers)
            (plugin->last_msg_handler)->next_handler = new_plugin_msg_handler;
        else
            plugin->msg_handlers = new_plugin_msg_handler;
        plugin->last_msg_handler = new_plugin_msg_handler;
    }
    else
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add handler for IRC command \"%s\" (not enough memory)\n"),
                    WEECHAT_ERROR, plugin->name, irc_command);
        return NULL;
    }
    return new_plugin_msg_handler;
}

/*
 * plugin_cmd_handler_add: add a command handler
 *                         arguments:
 *                           1. the plugin pointer
 *                           2. the WeeChat command
 *                           3. command description (for /help)
 *                           4. command arguments (for /help)
 *                           5. command args description (for /help)
 *                           6. the handler function
 *                           7. handler args: a string given to
 *                              handler when called (used by scripts)
 *                           8. handler pointer: a pointer given to
 *                              handler when called (used by scripts)
 */

t_plugin_cmd_handler *
plugin_cmd_handler_add (t_weechat_plugin *plugin, char *command,
                        char *description, char *arguments,
                        char *arguments_description,
                        t_plugin_handler_func *handler_func,
                        char *handler_args, void *handler_pointer)
{
    t_plugin_cmd_handler *new_plugin_cmd_handler;
    
    if (plugin_cmd_handler_search (command))
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add handler for \"%s\" command "
                      "(already exists)\n"),
                    WEECHAT_ERROR, plugin->name, command);
        return NULL;
    }
        
    new_plugin_cmd_handler = (t_plugin_cmd_handler *)malloc (sizeof (t_plugin_cmd_handler));
    if (new_plugin_cmd_handler)
    {
        new_plugin_cmd_handler->command = strdup (command);
        new_plugin_cmd_handler->description = (description) ? strdup (description) : NULL;
        new_plugin_cmd_handler->arguments = (arguments) ? strdup (arguments) : NULL;
        new_plugin_cmd_handler->arguments_description = (arguments_description) ? strdup (arguments_description) : NULL;
        new_plugin_cmd_handler->cmd_handler = handler_func;
        new_plugin_cmd_handler->cmd_handler_args = (handler_args) ? strdup (handler_args) : NULL;
        new_plugin_cmd_handler->cmd_handler_pointer = handler_pointer;
        new_plugin_cmd_handler->running = 0;
        
        /* add new handler to list */
        new_plugin_cmd_handler->prev_handler = plugin->last_cmd_handler;
        new_plugin_cmd_handler->next_handler = NULL;
        if (plugin->cmd_handlers)
            (plugin->last_cmd_handler)->next_handler = new_plugin_cmd_handler;
        else
            plugin->cmd_handlers = new_plugin_cmd_handler;
        plugin->last_cmd_handler = new_plugin_cmd_handler;
        
        /* add command to WeeChat commands list */
        if (!weelist_search (index_commands, command))
            weelist_add (&index_commands, &last_index_command, command);
    }
    else
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add handler for \"%s\" command (not enough memory)\n"),
                    WEECHAT_ERROR, plugin->name, command);
        return NULL;
    }
    return new_plugin_cmd_handler;
}

/*
 * plugin_msg_handler_exec: execute a message handler
 *                          return: number of handlers executed (0 means no handler found)
 */

int
plugin_msg_handler_exec (char *server, char *irc_command, char *irc_message)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_msg_handler *ptr_plugin_msg_handler;
    int count;
    
    count = 0;
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_plugin_msg_handler = ptr_plugin->msg_handlers;
             ptr_plugin_msg_handler;
             ptr_plugin_msg_handler = ptr_plugin_msg_handler->next_handler)
        {
            if (ascii_strcasecmp (ptr_plugin_msg_handler->irc_command, irc_command) == 0)
            {
                if (ptr_plugin_msg_handler->running == 0)
                {
                    ptr_plugin_msg_handler->running = 1;
                    if ((int) (ptr_plugin_msg_handler->msg_handler) (ptr_plugin,
                                                                     server,
                                                                     irc_command,
                                                                     irc_message,
                                                                     ptr_plugin_msg_handler->msg_handler_args,
                                                                     ptr_plugin_msg_handler->msg_handler_pointer))
                        count++;
                    ptr_plugin_msg_handler->running = 0;
                }
            }
        }
    }
    
    return count;
}

/*
 * plugin_cmd_handler_exec: execute a command handler
 *                          return: 1 if handler executed, 0 if no handler found
 */

int
plugin_cmd_handler_exec (char *server, char *command, char *arguments)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_cmd_handler *ptr_plugin_cmd_handler;
    int return_code;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_plugin_cmd_handler = ptr_plugin->cmd_handlers;
             ptr_plugin_cmd_handler;
             ptr_plugin_cmd_handler = ptr_plugin_cmd_handler->next_handler)
        {
            if (ascii_strcasecmp (ptr_plugin_cmd_handler->command, command) == 0)
            {
                if (ptr_plugin_cmd_handler->running == 0)
                {
                    ptr_plugin_cmd_handler->running = 1;
                    return_code = (int) (ptr_plugin_cmd_handler->cmd_handler) (ptr_plugin,
                                                                               server,
                                                                               command,
                                                                               arguments,
                                                                               ptr_plugin_cmd_handler->cmd_handler_args,
                                                                               ptr_plugin_cmd_handler->cmd_handler_pointer);
                    ptr_plugin_cmd_handler->running = 0;
                    return (return_code) ? 1 : 0;
                }
            }
        }
    }
    
    return 0;
}

/*
 * plugin_msg_handler_remove: remove a message handler for a plugin
 */

void
plugin_msg_handler_remove (t_weechat_plugin *plugin,
                           t_plugin_msg_handler *plugin_msg_handler)
{
    t_plugin_msg_handler *new_plugin_msg_handlers;
    
    /* remove handler from list */
    if (plugin->last_msg_handler == plugin_msg_handler)
        plugin->last_msg_handler = plugin_msg_handler->prev_handler;
    if (plugin_msg_handler->prev_handler)
    {
        (plugin_msg_handler->prev_handler)->next_handler = plugin_msg_handler->next_handler;
        new_plugin_msg_handlers = plugin->msg_handlers;
    }
    else
        new_plugin_msg_handlers = plugin_msg_handler->next_handler;
    
    if (plugin_msg_handler->next_handler)
        (plugin_msg_handler->next_handler)->prev_handler = plugin_msg_handler->prev_handler;

    /* free data */
    if (plugin_msg_handler->irc_command)
        free (plugin_msg_handler->irc_command);
    if (plugin_msg_handler->msg_handler_args)
        free (plugin_msg_handler->msg_handler_args);
    plugin->msg_handlers = new_plugin_msg_handlers;
}

/*
 * plugin_cmd_handler_remove: remove a command handler for a plugin
 */

void
plugin_cmd_handler_remove (t_weechat_plugin *plugin,
                           t_plugin_cmd_handler *plugin_cmd_handler)
{
    t_plugin_cmd_handler *new_plugin_cmd_handlers;
    
    /* remove handler from list */
    if (plugin->last_cmd_handler == plugin_cmd_handler)
        plugin->last_cmd_handler = plugin_cmd_handler->prev_handler;
    if (plugin_cmd_handler->prev_handler)
    {
        (plugin_cmd_handler->prev_handler)->next_handler = plugin_cmd_handler->next_handler;
        new_plugin_cmd_handlers = plugin->cmd_handlers;
    }
    else
        new_plugin_cmd_handlers = plugin_cmd_handler->next_handler;
    
    if (plugin_cmd_handler->next_handler)
        (plugin_cmd_handler->next_handler)->prev_handler = plugin_cmd_handler->prev_handler;
    
    /* remove command from WeeChat command list */
    weelist_remove (&index_commands, &last_index_command,
                    weelist_search (index_commands, plugin_cmd_handler->command));
    
    /* free data */
    if (plugin_cmd_handler->command)
        free (plugin_cmd_handler->command);
    if (plugin_cmd_handler->description)
        free (plugin_cmd_handler->description);
    if (plugin_cmd_handler->arguments)
        free (plugin_cmd_handler->arguments);
    if (plugin_cmd_handler->arguments_description)
        free (plugin_cmd_handler->arguments_description);
    if (plugin_cmd_handler->cmd_handler_args)
        free (plugin_cmd_handler->cmd_handler_args);
    plugin->cmd_handlers = new_plugin_cmd_handlers;
}

/*
 * plugin_msg_handler_remove_all: remove all message handlers for a plugin
 */

void
plugin_msg_handler_remove_all (t_weechat_plugin *plugin)
{
    while (plugin->msg_handlers)
        plugin_msg_handler_remove (plugin, plugin->msg_handlers);
}

/*
 * plugin_cmd_handler_remove_all: remove all command handlers for a plugin
 */

void
plugin_cmd_handler_remove_all (t_weechat_plugin *plugin)
{
    while (plugin->cmd_handlers)
        plugin_cmd_handler_remove (plugin, plugin->cmd_handlers);
}

/*
 * plugin_search_full_name: search the full name of a file with a part of name
 *                          and look in WeeChat user's dir, then WeeChat global lib dir
 */

char *
plugin_search_full_name (char *filename)
{
    char *name_with_ext, *final_name;
    int length;
    struct stat st;
    
    /* filename is already a full path */
    if (strchr (filename, '/') || strchr (filename, '\\'))
        return strdup (filename);
    
    length = strlen (filename) + 16;
    if (cfg_plugins_extension && cfg_plugins_extension[0])
        length += strlen (cfg_plugins_extension);
    name_with_ext = (char *)malloc (length);
    if (!name_with_ext)
        return strdup (filename);
    name_with_ext[0] = '\0';
    if (ascii_strncasecmp (filename, "lib", 3) != 0)
        strcat (name_with_ext, "lib");
    strcat (name_with_ext, filename);
    if (!strchr (filename, '.')
        && cfg_plugins_extension && cfg_plugins_extension[0])
        strcat (name_with_ext, cfg_plugins_extension);
    
    /* try WeeChat user's dir */
    length = strlen (weechat_home) + strlen (name_with_ext) + 16;
    final_name = (char *)malloc (length);
    if (!final_name)
    {
        free (name_with_ext);
        return strdup (filename);
    }
    snprintf (final_name, length,
              "%s/plugins/%s", weechat_home, name_with_ext);
    if ((stat (final_name, &st) == 0) && (st.st_size > 0))
    {
        free (name_with_ext);
        return final_name;
    }
    free (final_name);
    
    /* try WeeChat global lib dir */
    length = strlen (WEECHAT_LIBDIR) + strlen (name_with_ext) + 16;
    final_name = (char *)malloc (length);
    if (!final_name)
    {
        free (name_with_ext);
        return strdup (filename);
    }
    snprintf (final_name, length,
              "%s/plugins/%s", WEECHAT_LIBDIR, name_with_ext);
    if ((stat (final_name, &st) == 0) && (st.st_size > 0))
    {
        free (name_with_ext);
        return final_name;
    }
    free (final_name);

    return name_with_ext;
}

/*
 * plugin_load: load a WeeChat plugin (a dynamic library)
 *              return: pointer to new WeeChat plugin, NULL if error
 */

t_weechat_plugin *
plugin_load (char *filename)
{
    char *full_name;
    void *handle;
    char *name, *description, *version;
    t_weechat_init_func *init_func;
    t_weechat_plugin *new_plugin;
    
    if (!filename)
        return NULL;
    
    full_name = plugin_search_full_name (filename);
    
    if (!full_name)
        return NULL;
    
    handle = dlopen (full_name, RTLD_GLOBAL | RTLD_NOW);
    if (!handle)
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s unable to load plugin \"%s\": %s\n"),
                    WEECHAT_ERROR, full_name, dlerror());
        free (full_name);
        return NULL;
    }
    /* look for plugin name */
    name = dlsym (handle, "plugin_name");
    if (!name)
    {
        dlclose (handle);
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s symbol \"plugin_name\" not found in plugin \"%s\", failed to load\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    /* check for plugin with same name */
    if (plugin_search (name))
    {
        dlclose (handle);
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s unable to load plugin \"%s\": a plugin with "
                      "same name already exists\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    /* look for plugin description */
    description = dlsym (handle, "plugin_description");
    if (!description)
    {
        dlclose (handle);
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s symbol \"plugin_description\" not found in plugin \"%s\", failed to load\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    /* look for plugin version */
    version = dlsym (handle, "plugin_version");
    if (!version)
    {
        dlclose (handle);
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s symbol \"plugin_version\" not found in plugin \"%s\", failed to load\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    /* look for plugin init function */
    init_func = dlsym (handle, "weechat_plugin_init");
    if (!init_func)
    {
        dlclose (handle);
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s function \"weechat_plugin_init\" not found in plugin \"%s\", failed to load\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    
    /* create new plugin */
    new_plugin = (t_weechat_plugin *)malloc (sizeof (t_weechat_plugin));
    if (new_plugin)
    {
        /* variables */
        new_plugin->filename = strdup (full_name);
        new_plugin->handle = handle;
        new_plugin->name = strdup (name);
        new_plugin->description = strdup (description);
        new_plugin->version = strdup (version);
        
        /* functions */
        new_plugin->ascii_strcasecmp = &weechat_ascii_strcasecmp;
        new_plugin->explode_string = &weechat_explode_string;
        new_plugin->free_exploded_string = &weechat_free_exploded_string;
        new_plugin->mkdir_home = &weechat_plugin_mkdir_home;
        new_plugin->exec_on_files = &weechat_plugin_exec_on_files;
        new_plugin->msg_handler_add = &weechat_plugin_msg_handler_add;
        new_plugin->msg_handler_remove = &weechat_plugin_msg_handler_remove;
        new_plugin->msg_handler_remove_all = &weechat_plugin_msg_handler_remove_all;
        new_plugin->cmd_handler_add = &weechat_plugin_cmd_handler_add;
        new_plugin->cmd_handler_remove = &weechat_plugin_cmd_handler_remove;
        new_plugin->cmd_handler_remove_all = &weechat_plugin_cmd_handler_remove_all;
        new_plugin->printf = &weechat_plugin_printf;
        new_plugin->printf_server = &weechat_plugin_printf_server;
        new_plugin->infobar_printf = &weechat_plugin_infobar_printf;
        new_plugin->exec_command = &weechat_plugin_exec_command;
        new_plugin->get_info = &weechat_plugin_get_info;
        new_plugin->get_dcc_info = &weechat_plugin_get_dcc_info;
        new_plugin->free_dcc_info = &weechat_plugin_free_dcc_info;
        new_plugin->get_config = &weechat_plugin_get_config;
        
        /* handlers */
        new_plugin->msg_handlers = NULL;
        new_plugin->last_msg_handler = NULL;
        new_plugin->cmd_handlers = NULL;
        new_plugin->last_cmd_handler = NULL;
        
        /* add new plugin to list */
        new_plugin->prev_plugin = last_weechat_plugin;
        new_plugin->next_plugin = NULL;
        if (weechat_plugins)
            last_weechat_plugin->next_plugin = new_plugin;
        else
            weechat_plugins = new_plugin;
        last_weechat_plugin = new_plugin;
        
        irc_display_prefix (NULL, PREFIX_PLUGIN);
        gui_printf (NULL,
                    _("Initializing plugin \"%s\" %s\n"),
                    new_plugin->name, new_plugin->version);
        
        /* init plugin */
        if (!((t_weechat_init_func *)init_func) (new_plugin))
        {
            irc_display_prefix (NULL, PREFIX_ERROR);
            gui_printf (NULL,
                        _("%s unable to initialize plugin \"%s\"\n"),
                        WEECHAT_ERROR, full_name);
            plugin_remove (new_plugin);
            free (full_name);
            return NULL;
        }
    }
    else
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s unable to load plugin \"%s\" (not enough memory)\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    
    irc_display_prefix (NULL, PREFIX_PLUGIN);
    gui_printf (NULL,
                _("Plugin \"%s\" (%s) loaded.\n"),
                name, full_name);
    
    free (full_name);
    
    return new_plugin;
}

/*
 * plugin_auto_load_file: load a file found by plugin_aut_load,
 *                        but only it this is really a dynamic library
 */

int plugin_auto_load_file (t_weechat_plugin *plugin, char *filename)
{
    char *pos;
    
    /* make gcc happy */
    (void) plugin;
    
    if (cfg_plugins_extension && cfg_plugins_extension[0])
    {
        pos = strstr (filename, cfg_plugins_extension);
        if (pos)
        {
            if (ascii_strcasecmp (pos, cfg_plugins_extension) == 0)
                plugin_load (filename);
        }
    }
    else
        plugin_load (filename);
    return 1;
}

/*
 * plugin_auto_load: auto-load WeeChat plugins
 */

void plugin_auto_load ()
{
    char *ptr_home, *dir_name;
    
    /* auto-load plugins in WeeChat home dir */
    if (cfg_plugins_path && cfg_plugins_path[0])
    {
        if (cfg_plugins_path[0] == '~')
        {
            ptr_home = getenv ("HOME");
            dir_name = (char *)malloc (strlen (cfg_plugins_path) + strlen (ptr_home) + 2);
            if (dir_name)
            {
                strcpy (dir_name, ptr_home);
                strcat (dir_name, cfg_plugins_path + 1);
                plugin_exec_on_files (NULL, dir_name, &plugin_auto_load_file);
                free (dir_name);
            }
        }
        else
            plugin_exec_on_files (NULL, cfg_plugins_path, &plugin_auto_load_file);
    }
    
    /* auto-load plugins in WeeChat global lib dir */
    dir_name = (char *)malloc (strlen (WEECHAT_LIBDIR) + 16);
    if (dir_name)
    {
        snprintf (dir_name, strlen (WEECHAT_LIBDIR) + 16,
                  "%s/plugins", WEECHAT_LIBDIR);
        plugin_exec_on_files (NULL, dir_name, &plugin_auto_load_file);
        free (dir_name);
    }
}

/*
 * plugin_remove: remove a WeeChat plugin
 */

void
plugin_remove (t_weechat_plugin *plugin)
{
    t_weechat_plugin *new_weechat_plugins;
    
    /* remove handler from list */
    if (last_weechat_plugin == plugin)
        last_weechat_plugin = plugin->prev_plugin;
    if (plugin->prev_plugin)
    {
        (plugin->prev_plugin)->next_plugin = plugin->next_plugin;
        new_weechat_plugins = weechat_plugins;
    }
    else
        new_weechat_plugins = plugin->next_plugin;
    
    if (plugin->next_plugin)
        (plugin->next_plugin)->prev_plugin = plugin->prev_plugin;
    
    /* free data */
    plugin_msg_handler_remove_all (plugin);
    plugin_cmd_handler_remove_all (plugin);
    if (plugin->filename)
        free (plugin->filename);
    dlclose (plugin->handle);
    if (plugin->name)
        free (plugin->name);
    if (plugin->description)
        free (plugin->description);
    if (plugin->version)
        free (plugin->version);
    free (plugin);
    
    weechat_plugins = new_weechat_plugins;
}

/*
 * plugin_unload: unload a WeeChat plugin
 */

void
plugin_unload (t_weechat_plugin *plugin)
{
    t_weechat_end_func *end_func;
    
    end_func = dlsym (plugin->handle, "weechat_plugin_end");
    if (end_func)
        (void) (end_func) (plugin);
    plugin_remove (plugin);
}

/*
 * plugin_unload_name: unload a WeeChat plugin by name
 */

void
plugin_unload_name (char *name)
{
    t_weechat_plugin *ptr_plugin;
    
    ptr_plugin = plugin_search (name);
    if (ptr_plugin)
    {
        plugin_unload (ptr_plugin);
        irc_display_prefix (NULL, PREFIX_PLUGIN);
        gui_printf (NULL, _("Plugin \"%s\" unloaded.\n"), name);
    }
    else
    {
        irc_display_prefix (NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin \"%s\" not found\n"),
                    WEECHAT_ERROR, name);
    }
}

/*
 * plugin_unload_all: unload all WeeChat plugins
 */

void
plugin_unload_all ()
{
    while (weechat_plugins)
        plugin_unload (weechat_plugins);
}

/*
 * plugin_init: init plugin support
 */

void
plugin_init ()
{
    char *list_plugins, *pos, *pos2;
    
    if (cfg_plugins_autoload && cfg_plugins_autoload[0])
    {
        if (ascii_strcasecmp (cfg_plugins_autoload, "*") == 0)
            plugin_auto_load ();
        else
        {
            list_plugins = strdup (cfg_plugins_autoload);
            if (list_plugins)
            {
                pos = list_plugins;
                while (pos && pos[0])
                {
                    pos2 = strchr (pos, ',');
                    if (pos2)
                        pos2[0] = '\0';
                    plugin_load (pos);
                    if (pos2)
                        pos = pos2 + 1;
                    else
                        pos = NULL;
                }
                free (list_plugins);
            }
        }
    }
}

/*
 * plugin_end: end plugin support
 */

void
plugin_end ()
{
    plugin_unload_all ();
}

/*************************** Public plugin interface **************************/

/*
 * weechat_ascii_strcasecmp: locale and case independent string comparison
 */

int
weechat_ascii_strcasecmp (t_weechat_plugin *plugin,
                          char *string1, char *string2)
{
    /* make gcc happy */
    (void) plugin;
    
    return ascii_strcasecmp (string1, string2);
}

/*
 * weechat_explode_string: explode a string
 */

char **
weechat_explode_string (t_weechat_plugin *plugin, char *string,
                        char *separators, int num_items_max,
                        int *num_items)
{
    /* make gcc happy */
    (void) plugin;
    
    if (!plugin || !string || !separators || !num_items)
        return NULL;
    
    return explode_string (string, separators, num_items_max, num_items);
}

/*
 * weechat_free_exploded_string: free exploded string
 */

void
weechat_free_exploded_string (t_weechat_plugin *plugin, char **exploded_string)
{
    /* make gcc happy */
    (void) plugin;
    
    free_exploded_string (exploded_string);
}

/*
 * weechat_plugin_mkdir_home: create a directory for script in WeeChat home
 */

int
weechat_plugin_mkdir_home (t_weechat_plugin *plugin, char *path)
{
    char *dir_name;
    int dir_length;
    
    /* make gcc happy */
    (void) plugin;
    
    if (!path)
        return 0;
    
    /* build directory, adding WeeChat home */
    dir_length = strlen (weechat_home) + strlen (path) + 2;
    dir_name =
        (char *) malloc (dir_length * sizeof (char));
    if (!dir_name)
        return 0;
    
    snprintf (dir_name, dir_length, "%s/%s", weechat_home, path);
    
    if (mkdir (dir_name, 0755) < 0)
    {
        if (errno != EEXIST)
        {
            free (dir_name);
            return 0;
        }
    }
    
    free (dir_name);
    return 1;
}

/*
 * weechat_plugin_exec_on_files: find files in a directory and execute a
 *                               function on each file
 */

void
weechat_plugin_exec_on_files (t_weechat_plugin *plugin, char *directory,
                              int (*callback)(t_weechat_plugin *, char *))
{
    if (directory && callback)
        plugin_exec_on_files (plugin, directory, callback);
}

/*
 * weechat_plugin_printf: print a message on a server or channel buffer
 */

void
weechat_plugin_printf (t_weechat_plugin *plugin,
                       char *server, char *channel, char *message, ...)
{
    t_gui_buffer *ptr_buffer;
    va_list argptr;
    static char buf[8192];
    
    if (!plugin || !message)
        return;
    
    ptr_buffer = plugin_find_buffer (server, channel);
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    irc_display_prefix (ptr_buffer, PREFIX_PLUGIN);
    gui_printf (ptr_buffer, "%s\n", buf);
}

/*
 * weechat_plugin_printf_server: print a message on server buffer
 */

void
weechat_plugin_printf_server (t_weechat_plugin *plugin, char *message, ...)
{
    va_list argptr;
    static char buf[8192];
    
    if (!plugin || !message)
        return;
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    irc_display_prefix (NULL, PREFIX_PLUGIN);
    gui_printf (NULL, "%s\n", buf);
}

/*
 * weechat_plugin_infobar_printf: print a message in infobar
 */

void
weechat_plugin_infobar_printf (t_weechat_plugin *plugin, int time_displayed, char *message, ...)
{
    va_list argptr;
    static char buf[1024];
    
    if (!plugin || (time_displayed < 0) || !message)
        return;
    
    va_start (argptr, message);
    vsnprintf (buf, sizeof (buf) - 1, message, argptr);
    va_end (argptr);
    gui_infobar_printf (time_displayed, COLOR_WIN_INFOBAR, buf);
}

/*
 * weechat_plugin_msg_handler_add: add a message handler
 */

t_plugin_msg_handler *
weechat_plugin_msg_handler_add (t_weechat_plugin *plugin, char *message,
                                t_plugin_handler_func *handler_func,
                                char *handler_args, void *handler_pointer)
{
    if (plugin && message && handler_func)
        return plugin_msg_handler_add (plugin, message, handler_func,
                                       handler_args, handler_pointer);
    
    return NULL;
}

/*
 * weechat_plugin_msg_handler_remove: remove a WeeChat message handler
 */

void
weechat_plugin_msg_handler_remove (t_weechat_plugin *plugin,
                                   t_plugin_msg_handler *msg_handler)
{
    if (plugin && msg_handler)
        plugin_msg_handler_remove (plugin, msg_handler);
}

/*
 * weechat_plugin_msg_handler_remove_all: remove all WeeChat message handlers
 */

void
weechat_plugin_msg_handler_remove_all (t_weechat_plugin *plugin)
{
    if (plugin)
        plugin_msg_handler_remove_all (plugin);
}

/*
 * weechat_plugin_cmd_handler_add: add a command handler
 */

t_plugin_cmd_handler *
weechat_plugin_cmd_handler_add (t_weechat_plugin *plugin, char *command,
                                char *description, char *arguments,
                                char *arguments_description,
                                t_plugin_handler_func *handler_func,
                                char *handler_args, void *handler_pointer)
{
    if (plugin && command && handler_func)
        return plugin_cmd_handler_add (plugin, command, description, arguments,
                                       arguments_description,
                                       handler_func,
                                       handler_args, handler_pointer);
    
    return NULL;
}

/*
 * weechat_cmd_plugin_handler_remove: remove a WeeChat command handler
 */

void
weechat_plugin_cmd_handler_remove (t_weechat_plugin *plugin,
                                   t_plugin_cmd_handler *cmd_handler)
{
    if (plugin && cmd_handler)
        plugin_cmd_handler_remove (plugin, cmd_handler);
}

/*
 * weechat_plugin_cmd_handler_remove_all: remove all WeeChat command handlers
 */

void
weechat_plugin_cmd_handler_remove_all (t_weechat_plugin *plugin)
{
    if (plugin)
        plugin_cmd_handler_remove_all (plugin);
}

/*
 * weechat_plugin_exec_command: execute a command (simulate user entry)
 */

void
weechat_plugin_exec_command (t_weechat_plugin *plugin,
                             char *server, char *channel, char *command)
{
    t_gui_buffer *ptr_buffer;
    
    if (!plugin || !command)
        return;
    
    ptr_buffer = plugin_find_buffer (server, channel);
    if (ptr_buffer)
        user_command (SERVER(ptr_buffer), ptr_buffer, command);
}

/*
 * weechat_plugin_get_info: get info about WeeChat
 *                          WARNING: caller should free string returned
 *                          by this function after use
 */

char *
weechat_plugin_get_info (t_weechat_plugin *plugin, char *info, char *server, char *channel)
{
    t_gui_buffer *ptr_buffer;
    
    if (!plugin || !info)
        return NULL;
    
    ptr_buffer = plugin_find_buffer (server, channel);
    if (!ptr_buffer)
        return NULL;
    
    if (ascii_strcasecmp (info, "version") == 0)
    {
        return strdup (PACKAGE_VERSION);
    }
    else if (ascii_strcasecmp (info, "nick") == 0)
    {
        if (SERVER(ptr_buffer) && (SERVER(ptr_buffer)->is_connected)
            && (SERVER(ptr_buffer)->nick))
            return strdup (SERVER(ptr_buffer)->nick);
    }
    else if (ascii_strcasecmp (info, "channel") == 0)
    {
        if (BUFFER_IS_CHANNEL(ptr_buffer))
            return strdup (CHANNEL(gui_current_window->buffer)->name);
    }
    else if (ascii_strcasecmp (info, "server") == 0)
    {
        if (SERVER(ptr_buffer) && (SERVER(ptr_buffer)->is_connected)
            && (SERVER(ptr_buffer)->name))
            return strdup (SERVER(ptr_buffer)->name);
    }
    else if (ascii_strcasecmp (info, "away") == 0)
    {
        if (SERVER(ptr_buffer) && (SERVER(ptr_buffer)->is_connected))
        {
            if (SERVER(ptr_buffer)->is_away)
                return strdup ("1");
            else
                return strdup ("0");
        }
    }
    else if (ascii_strcasecmp (info, "weechatdir") == 0)
    {
        /* WARNING: deprecated info, you should use weechat_dir */
        /* will be removed in a future version */
        return strdup (weechat_home);
    }
    else if (ascii_strcasecmp (info, "weechat_dir") == 0)
    {
        return strdup (weechat_home);
    }
    else if (ascii_strcasecmp (info, "weechat_libdir") == 0)
    {
        return strdup (WEECHAT_LIBDIR);
    }
    else if (ascii_strcasecmp (info, "weechat_sharedir") == 0)
    {
        return strdup (WEECHAT_SHAREDIR);
    }
    
    /* info not found */
    return NULL;
}

/*
 * weechat_plugin_get_dcc_info: get list of DCC files/chats info
 */

t_plugin_dcc_info *
weechat_plugin_get_dcc_info (t_weechat_plugin *plugin)
{
    t_plugin_dcc_info *dcc_info, *last_dcc_info, *new_dcc_info;
    t_irc_dcc *ptr_dcc;
    
    if (!plugin)
        return NULL;
    
    if (dcc_list)
    {
        dcc_info = NULL;
        last_dcc_info = NULL;
        for (ptr_dcc = dcc_list; ptr_dcc; ptr_dcc = ptr_dcc->next_dcc)
        {
            new_dcc_info = (t_plugin_dcc_info *)malloc (sizeof (t_plugin_dcc_info));
            if (new_dcc_info)
            {
                new_dcc_info->server = (ptr_dcc->server) ? strdup (ptr_dcc->server->name) : strdup ("");
                new_dcc_info->channel = (ptr_dcc->channel) ? strdup (ptr_dcc->channel->name) : strdup ("");
                new_dcc_info->type = ptr_dcc->type;
                new_dcc_info->status = ptr_dcc->status;
                new_dcc_info->start_time = ptr_dcc->start_time;
                new_dcc_info->start_transfer = ptr_dcc->start_transfer;
                new_dcc_info->addr = ptr_dcc->addr;
                new_dcc_info->port = ptr_dcc->port;
                new_dcc_info->nick = (ptr_dcc->nick) ? strdup (ptr_dcc->nick) : strdup ("");
                new_dcc_info->filename = (ptr_dcc->filename) ? strdup (ptr_dcc->filename) : strdup ("");
                new_dcc_info->local_filename = (ptr_dcc->local_filename) ? strdup (ptr_dcc->local_filename) : strdup ("");
                new_dcc_info->filename_suffix = ptr_dcc->filename_suffix;
                new_dcc_info->size = ptr_dcc->size;
                new_dcc_info->pos = ptr_dcc->pos;
                new_dcc_info->start_resume = ptr_dcc->start_resume;
                new_dcc_info->bytes_per_sec = ptr_dcc->bytes_per_sec;
                
                new_dcc_info->prev_dcc = last_dcc_info;
                new_dcc_info->next_dcc = NULL;
                if (!dcc_info)
                {
                    dcc_info = new_dcc_info;
                    last_dcc_info = new_dcc_info;
                }
                else
                    last_dcc_info->next_dcc = new_dcc_info;
            }
        }
        
        return dcc_info;
    }
    
    return NULL;
}

/*
 * weechat_plugin_free_dcc_info: free dcc info struct list
 */

void
weechat_plugin_free_dcc_info (t_weechat_plugin *plugin, t_plugin_dcc_info *dcc_info)
{
    t_plugin_dcc_info *new_dcc_info;
    
    if (!plugin || !dcc_info)
        return;
    
    while (dcc_info)
    {
        if (dcc_info->server)
            free (dcc_info->server);
        if (dcc_info->channel)
            free (dcc_info->channel);
        if (dcc_info->nick)
            free (dcc_info->nick);
        if (dcc_info->filename)
            free (dcc_info->filename);
        if (dcc_info->local_filename)
            free (dcc_info->local_filename);
        new_dcc_info = dcc_info->next_dcc;
        free (dcc_info);
        dcc_info = new_dcc_info;
    }
}

/*
 * weechat_plugin_get_config_str_value: return string value for any option
 *                                      This function should never be called directly
 *                                      (only used by weechat_get_config)
 */

char *
weechat_plugin_get_config_str_value (t_config_option *option, void *value)
{
    char buf_temp[1024], *color_name;
    
    if (!value)
    {
        if (option->option_type == OPTION_TYPE_STRING)
            value = option->ptr_string;
        else
            value = option->ptr_int;
    }
    
    switch (option->option_type)
    {
        case OPTION_TYPE_BOOLEAN:
            return (*((int *)value)) ?
                strdup ("on") : strdup ("off");
            break;
        case OPTION_TYPE_INT:
            snprintf (buf_temp, sizeof (buf_temp), "%d",
                      *((int *)value));
            return strdup (buf_temp);
            break;
        case OPTION_TYPE_INT_WITH_STRING:
            return option->array_values[*((int *)value)];
            break;
        case OPTION_TYPE_COLOR:
            color_name = gui_get_color_by_value (*((int *)value));
            return (color_name) ? strdup (color_name) : strdup ("");
            break;
        case OPTION_TYPE_STRING:
            return (*((char **)value)) ? strdup (*((char **)value)) : strdup ("");
            break;
    }
    
    /* should never be executed! */
    return NULL;
}

/*
 * weechat_get_config: get value of a config option
 */

char *
weechat_plugin_get_config (t_weechat_plugin *plugin, char *option)
{
    int i, j;
    t_irc_server *ptr_server;
    char option_name[256];
    void *ptr_option_value;
    
    /* make gcc happy */
    (void) plugin;
    
    for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
    {
        if ((i != CONFIG_SECTION_KEYS) && (i != CONFIG_SECTION_ALIAS)
            && (i != CONFIG_SECTION_IGNORE) && (i != CONFIG_SECTION_SERVER))
        {
            for (j = 0; weechat_options[i][j].option_name; j++)
            {
                if ((!option) ||
                    ((option) && (option[0])
                     && (strstr (weechat_options[i][j].option_name, option) != NULL)))
                {
                    return weechat_plugin_get_config_str_value (&weechat_options[i][j], NULL);
                }
            }
        }
    }
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (i = 0; weechat_options[CONFIG_SECTION_SERVER][i].option_name; i++)
        {
            snprintf (option_name, sizeof (option_name), "%s.%s",
                      ptr_server->name, 
                      weechat_options[CONFIG_SECTION_SERVER][i].option_name);
            if ((!option) ||
                ((option) && (option[0])
                 && (strstr (option_name, option) != NULL)))
            {
                ptr_option_value = config_get_server_option_ptr (ptr_server,
                                                                 weechat_options[CONFIG_SECTION_SERVER][i].option_name);
                if (ptr_option_value)
                {
                    return weechat_plugin_get_config_str_value (&weechat_options[CONFIG_SECTION_SERVER][i],
                                                                ptr_option_value);
                }
            }
        }
    }
    
    /* option not found */
    return NULL;
}
