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
#include "plugins.h"
#include "plugins-config.h"
#include "../common/command.h"
#include "../common/weeconfig.h"
#include "../irc/irc.h"
#include "../gui/gui.h"


t_weechat_plugin *weechat_plugins = NULL;
t_weechat_plugin *last_weechat_plugin = NULL;


/*
 * plugin_find_buffer: find a buffer for text display
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
    
    /* nothing given => print on current buffer */
    if ((!server || !server[0]) && (!channel || !channel[0]))
        ptr_buffer = gui_current_window->buffer;
    else
    {
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
            if (ptr_server)
                ptr_buffer = ptr_server->buffer;
            else
                ptr_buffer = gui_current_window->buffer;
        }
    }
    
    if (!ptr_buffer)
        return NULL;
    
    return (ptr_buffer->dcc) ? gui_buffers : ptr_buffer;
}

/*
 * plugin_find_server_channel: find server/channel for command execution
 */

void
plugin_find_server_channel (char *server, char *channel,
                            t_irc_server **ptr_server,
                            t_irc_channel **ptr_channel)
{
    (*ptr_server) = NULL;
    (*ptr_channel) = NULL;
    
    /* nothing given => return current server/channel */
    if ((!server || !server[0]) && (!channel || !channel[0]))
    {
        (*ptr_server) = SERVER(gui_current_window->buffer);
        (*ptr_channel) = (BUFFER_IS_CHANNEL(gui_current_window->buffer) ||
                          BUFFER_IS_PRIVATE(gui_current_window->buffer)) ?
            CHANNEL(gui_current_window->buffer) : NULL;
    }
    else
    {
        if (server && server[0])
            (*ptr_server) = server_search (server);
        else
        {
            (*ptr_server) = SERVER(gui_current_window->buffer);
            if (!(*ptr_server))
                (*ptr_server) = SERVER(gui_buffers);
        }
        
        if (channel && channel[0])
        {
            if ((*ptr_server))
                (*ptr_channel) = channel_search ((*ptr_server), channel);
        }
    }
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

t_plugin_handler *
plugin_cmd_handler_search (char *command)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == HANDLER_COMMAND)
                && (ascii_strcasecmp (ptr_handler->command, command) == 0))
                return ptr_handler;
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

t_plugin_handler *
plugin_msg_handler_add (t_weechat_plugin *plugin, char *irc_command,
                        t_plugin_handler_func *handler_func,
                        char *handler_args, void *handler_pointer)
{
    t_plugin_handler *new_handler;
    
    new_handler = (t_plugin_handler *)malloc (sizeof (t_plugin_handler));
    if (new_handler)
    {
        new_handler->type = HANDLER_MESSAGE;
        new_handler->irc_command = strdup (irc_command);
        new_handler->command = NULL;
        new_handler->description = NULL;
        new_handler->arguments = NULL;
        new_handler->arguments_description = NULL;
        new_handler->handler = handler_func;
        new_handler->handler_args = (handler_args) ? strdup (handler_args) : NULL;
        new_handler->handler_pointer = handler_pointer;
        new_handler->running = 0;
        
        /* add new handler to list */
        new_handler->prev_handler = plugin->last_handler;
        new_handler->next_handler = NULL;
        if (plugin->handlers)
            (plugin->last_handler)->next_handler = new_handler;
        else
            plugin->handlers = new_handler;
        plugin->last_handler = new_handler;
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add handler for IRC command \"%s\" (not enough memory)\n"),
                    WEECHAT_ERROR, plugin->name, irc_command);
        return NULL;
    }
    return new_handler;
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

t_plugin_handler *
plugin_cmd_handler_add (t_weechat_plugin *plugin, char *command,
                        char *description, char *arguments,
                        char *arguments_description,
                        t_plugin_handler_func *handler_func,
                        char *handler_args, void *handler_pointer)
{
    t_plugin_handler *new_handler;
    
    if (plugin_cmd_handler_search (command))
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add handler for \"%s\" command "
                      "(already exists)\n"),
                    WEECHAT_ERROR, plugin->name, command);
        return NULL;
    }
        
    new_handler = (t_plugin_handler *)malloc (sizeof (t_plugin_handler));
    if (new_handler)
    {
        new_handler->type = HANDLER_COMMAND;
        new_handler->irc_command = NULL;
        new_handler->command = strdup (command);
        new_handler->description = (description) ? strdup (description) : NULL;
        new_handler->arguments = (arguments) ? strdup (arguments) : NULL;
        new_handler->arguments_description = (arguments_description) ? strdup (arguments_description) : NULL;
        new_handler->handler = handler_func;
        new_handler->handler_args = (handler_args) ? strdup (handler_args) : NULL;
        new_handler->handler_pointer = handler_pointer;
        new_handler->running = 0;
        
        /* add new handler to list */
        new_handler->prev_handler = plugin->last_handler;
        new_handler->next_handler = NULL;
        if (plugin->handlers)
            (plugin->last_handler)->next_handler = new_handler;
        else
            plugin->handlers = new_handler;
        plugin->last_handler = new_handler;
        
        /* add command to WeeChat commands list */
        if (!weelist_search (index_commands, command))
            weelist_add (&index_commands, &last_index_command, command);
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s plugin %s: unable to add handler for \"%s\" command (not enough memory)\n"),
                    WEECHAT_ERROR, plugin->name, command);
        return NULL;
    }
    return new_handler;
}

/*
 * plugin_msg_handler_exec: execute a message handler
 *                          return: code for informing WeeChat whether message
 *                          should be ignored or not
 */

int
plugin_msg_handler_exec (char *server, char *irc_command, char *irc_message)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    int return_code, final_return_code;
    
    final_return_code = PLUGIN_RC_OK;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == HANDLER_MESSAGE)
                && (ascii_strcasecmp (ptr_handler->irc_command, irc_command) == 0))
            {
                if (ptr_handler->running == 0)
                {
                    ptr_handler->running = 1;
                    return_code = ((int) (ptr_handler->handler) (ptr_plugin,
                                                                 server,
                                                                 irc_command,
                                                                 irc_message,
                                                                 ptr_handler->handler_args,
                                                                 ptr_handler->handler_pointer));
                    ptr_handler->running = 0;
                    
                    if (return_code >= 0)
                    {
                        if (return_code & PLUGIN_RC_OK_IGNORE_WEECHAT)
                            final_return_code = PLUGIN_RC_OK_IGNORE_WEECHAT;
                        if (return_code & PLUGIN_RC_OK_IGNORE_PLUGINS)
                            return final_return_code;
                    }
                }
            }
        }
    }
    
    return final_return_code;
}

/*
 * plugin_cmd_handler_exec: execute a command handler
 *                          return: 1 if handler executed, 0 if no handler found
 */

int
plugin_cmd_handler_exec (char *server, char *command, char *arguments)
{
    t_weechat_plugin *ptr_plugin;
    t_plugin_handler *ptr_handler;
    int return_code;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        for (ptr_handler = ptr_plugin->handlers;
             ptr_handler; ptr_handler = ptr_handler->next_handler)
        {
            if ((ptr_handler->type == HANDLER_COMMAND)
                && (ascii_strcasecmp (ptr_handler->command, command) == 0))
            {
                if (ptr_handler->running == 0)
                {
                    ptr_handler->running = 1;
                    return_code = (int) (ptr_handler->handler) (ptr_plugin,
                                                                server,
                                                                command,
                                                                arguments,
                                                                ptr_handler->handler_args,
                                                                ptr_handler->handler_pointer);
                    ptr_handler->running = 0;
                    return (return_code == PLUGIN_RC_KO) ? 0 : 1;
                }
            }
        }
    }
    
    return 0;
}

/*
 * plugin_handler_remove: remove a handler for a plugin
 */

void
plugin_handler_remove (t_weechat_plugin *plugin,
                       t_plugin_handler *handler)
{
    t_plugin_handler *new_handlers;
    
    /* remove handler from list */
    if (plugin->last_handler == handler)
        plugin->last_handler = handler->prev_handler;
    if (handler->prev_handler)
    {
        (handler->prev_handler)->next_handler = handler->next_handler;
        new_handlers = plugin->handlers;
    }
    else
        new_handlers = handler->next_handler;
    
    if (handler->next_handler)
        (handler->next_handler)->prev_handler = handler->prev_handler;
    
    /* remove command from WeeChat command list, if command handler */
    if (handler->type == HANDLER_COMMAND)
        weelist_remove (&index_commands, &last_index_command,
                        weelist_search (index_commands, handler->command));
    
    /* free data */
    if (handler->irc_command)
        free (handler->irc_command);
    if (handler->command)
        free (handler->command);
    if (handler->description)
        free (handler->description);
    if (handler->arguments)
        free (handler->arguments);
    if (handler->arguments_description)
        free (handler->arguments_description);
    if (handler->handler_args)
        free (handler->handler_args);
    
    plugin->handlers = new_handlers;
}

/*
 * plugin_handler_remove_all: remove all handlers for a plugin
 */

void
plugin_handler_remove_all (t_weechat_plugin *plugin)
{
    while (plugin->handlers)
        plugin_handler_remove (plugin, plugin->handlers);
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
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s symbol \"plugin_name\" not found in plugin \"%s\", failed to load\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    /* check for plugin with same name */
    if (plugin_search (name))
    {
        dlclose (handle);
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
        new_plugin->cmd_handler_add = &weechat_plugin_cmd_handler_add;
        new_plugin->handler_remove = &weechat_plugin_handler_remove;
        new_plugin->handler_remove_all = &weechat_plugin_handler_remove_all;
        new_plugin->printf = &weechat_plugin_printf;
        new_plugin->printf_server = &weechat_plugin_printf_server;
        new_plugin->infobar_printf = &weechat_plugin_infobar_printf;
        new_plugin->exec_command = &weechat_plugin_exec_command;
        new_plugin->get_info = &weechat_plugin_get_info;
        new_plugin->get_dcc_info = &weechat_plugin_get_dcc_info;
        new_plugin->free_dcc_info = &weechat_plugin_free_dcc_info;
        new_plugin->get_config = &weechat_plugin_get_config;
        new_plugin->set_config = &weechat_plugin_set_config;
        new_plugin->get_plugin_config = &weechat_plugin_get_plugin_config;
        new_plugin->set_plugin_config = &weechat_plugin_set_plugin_config;
        
        /* handlers */
        new_plugin->handlers = NULL;
        new_plugin->last_handler = NULL;
        
        /* add new plugin to list */
        new_plugin->prev_plugin = last_weechat_plugin;
        new_plugin->next_plugin = NULL;
        if (weechat_plugins)
            last_weechat_plugin->next_plugin = new_plugin;
        else
            weechat_plugins = new_plugin;
        last_weechat_plugin = new_plugin;
        
        irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
        gui_printf (NULL,
                    _("Initializing plugin \"%s\" %s\n"),
                    new_plugin->name, new_plugin->version);
        
        /* init plugin */
        if (((t_weechat_init_func *)init_func) (new_plugin) < 0)
        {
            irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL,
                    _("%s unable to load plugin \"%s\" (not enough memory)\n"),
                    WEECHAT_ERROR, full_name);
        free (full_name);
        return NULL;
    }
    
    irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
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
    plugin_handler_remove_all (plugin);
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
        irc_display_prefix (NULL, NULL, PREFIX_PLUGIN);
        gui_printf (NULL, _("Plugin \"%s\" unloaded.\n"), name);
    }
    else
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
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
plugin_init (int auto_load)
{
    char *list_plugins, *pos, *pos2;
    
    /* read plugins options on disk */
    plugin_config_read ();
    
    /* auto-load plugins if asked */
    if (auto_load && cfg_plugins_autoload && cfg_plugins_autoload[0])
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
    /* write plugins config options */
    plugin_config_write ();
    
    /* unload all plugins */
    plugin_unload_all ();
}
