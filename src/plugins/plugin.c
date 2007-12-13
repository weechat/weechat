/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* plugin.c: manages WeeChat plugins (dynamic C libraries) */


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

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-util.h"
#include "../gui/gui-chat.h"
#include "plugin.h"
#include "plugin-api.h"
#include "plugin-config.h"
#include "plugin-infolist.h"


struct t_weechat_plugin *weechat_plugins = NULL;
struct t_weechat_plugin *last_weechat_plugin = NULL;


/*
 * plugin_exec_on_files: find files in a directory and execute a
 *                       function on each file
 */

void
plugin_exec_on_files (struct t_weechat_plugin *plugin, char *directory,
                      int (*callback)(struct t_weechat_plugin *, char *))
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

struct t_weechat_plugin *
plugin_search (char *name)
{
    struct t_weechat_plugin *ptr_plugin;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        if (string_strcasecmp (ptr_plugin->name, name) == 0)
            return ptr_plugin;
    }
    
    /* plugin not found */
    return NULL;
}

/*
 * plugin_load: load a WeeChat plugin (a dynamic library)
 *              return: pointer to new WeeChat plugin, NULL if error
 */

struct t_weechat_plugin *
plugin_load (char *filename)
{
    char *full_name;
    void *handle;
    char *name, *description, *version, *charset;
    t_weechat_init_func *init_func;
    struct t_weechat_plugin *new_plugin;
    
    if (!filename)
        return NULL;
    
    full_name = util_search_full_lib_name (filename, "plugins");
    
    if (!full_name)
        return NULL;
    
    handle = dlopen (full_name, RTLD_GLOBAL | RTLD_NOW);
    if (!handle)
    {
        gui_chat_printf (NULL,
                         _("%sError: unable to load plugin \"%s\": %s\n"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         full_name, dlerror());
        free (full_name);
        return NULL;
    }
    
    /* look for plugin name */
    name = dlsym (handle, "plugin_name");
    if (!name)
    {
        dlclose (handle);
        gui_chat_printf (NULL,
                         _("%sError: symbol \"%s\" not found in "
                           "plugin \"%s\", failed to load\n"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "plugin_name",
                         full_name);
        free (full_name);
        return NULL;
    }
    
    /* check for plugin with same name */
    if (plugin_search (name))
    {
        dlclose (handle);
        gui_chat_printf (NULL,
                         _("%sError: unable to load plugin \"%s\": a plugin "
                           "with same name already exists\n"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         full_name);
        free (full_name);
        return NULL;
    }
    
    /* look for plugin description */
    description = dlsym (handle, "plugin_description");
    if (!description)
    {
        dlclose (handle);
        gui_chat_printf (NULL,
                         _("%sError: symbol \"%s\" not found "
                           "in plugin \"%s\", failed to load\n"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "plugin_description",
                         full_name);
        free (full_name);
        return NULL;
    }
    
    /* look for plugin version */
    version = dlsym (handle, "plugin_version");
    if (!version)
    {
        dlclose (handle);
        gui_chat_printf (NULL,
                         _("%sError: symbol \"%s\" not found in "
                           "plugin \"%s\", failed to load\n"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "plugin_version",
                         full_name);
        free (full_name);
        return NULL;
    }
    
    /* look for plugin charset (optional) */
    charset = dlsym (handle, "plugin_charset");
    
    /* look for plugin init function */
    init_func = dlsym (handle, "weechat_plugin_init");
    if (!init_func)
    {
        dlclose (handle);
        gui_chat_printf (NULL,
                         _("%sError: function \"%s\" not "
                           "found in plugin \"%s\", failed to load\n"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_init",
                         full_name);
        free (full_name);
        return NULL;
    }
    
    /* create new plugin */
    new_plugin = (struct t_weechat_plugin *)malloc (sizeof (struct t_weechat_plugin));
    if (new_plugin)
    {
        /* variables */
        new_plugin->filename = strdup (full_name);
        new_plugin->handle = handle;
        new_plugin->name = strdup (name);
        new_plugin->description = strdup (description);
        new_plugin->version = strdup (version);
        new_plugin->charset = (charset) ? strdup (charset) : NULL;
        
        /* functions */
        new_plugin->charset_set = &plugin_api_charset_set;
        new_plugin->iconv_to_internal = &plugin_api_iconv_to_internal;
        new_plugin->iconv_from_internal = &plugin_api_iconv_from_internal;
        new_plugin->gettext = &plugin_api_gettext;
        new_plugin->ngettext = &plugin_api_ngettext;
        new_plugin->strcasecmp = &plugin_api_strcasecmp;
        new_plugin->strncasecmp = &plugin_api_strncasecmp;
        new_plugin->strcasestr = &plugin_api_strcasestr;
        new_plugin->string_replace = &plugin_api_string_replace;
        new_plugin->string_explode = &plugin_api_string_explode;
        new_plugin->string_free_exploded = &plugin_api_string_free_exploded;
        new_plugin->string_split_command = &plugin_api_string_split_command;
        new_plugin->string_free_splitted_command = &plugin_api_string_free_splitted_command;

        new_plugin->utf8_has_8bits = &plugin_api_utf8_has_8bits;
        new_plugin->utf8_is_valid = &plugin_api_utf8_is_valid;
        new_plugin->utf8_normalize = &plugin_api_utf8_normalize;
        new_plugin->utf8_prev_char = &plugin_api_utf8_prev_char;
        new_plugin->utf8_next_char = &plugin_api_utf8_next_char;
        new_plugin->utf8_char_size = &plugin_api_utf8_char_size;
        new_plugin->utf8_strlen = &plugin_api_utf8_strlen;
        new_plugin->utf8_strnlen = &plugin_api_utf8_strnlen;
        new_plugin->utf8_strlen_screen = &plugin_api_utf8_strlen_screen;
        new_plugin->utf8_charcasecmp = &plugin_api_utf8_charcasecmp;
        new_plugin->utf8_char_size_screen = &plugin_api_utf8_char_size_screen;
        new_plugin->utf8_add_offset = &plugin_api_utf8_add_offset;
        new_plugin->utf8_real_pos = &plugin_api_utf8_real_pos;
        new_plugin->utf8_pos = &plugin_api_utf8_pos;
        
        new_plugin->mkdir_home = &plugin_api_mkdir_home;
        new_plugin->mkdir = &plugin_api_mkdir;
        new_plugin->exec_on_files = &plugin_api_exec_on_files;
        
        new_plugin->timeval_diff = &plugin_api_timeval_diff;
        
        new_plugin->list_new = &plugin_api_list_new;
        new_plugin->list_add = &plugin_api_list_add;
        new_plugin->list_search = &plugin_api_list_search;
        new_plugin->list_casesearch = &plugin_api_list_casesearch;
        new_plugin->list_get = &plugin_api_list_get;
        new_plugin->list_next = &plugin_api_list_next;
        new_plugin->list_prev = &plugin_api_list_prev;
        new_plugin->list_string = &plugin_api_list_string;
        new_plugin->list_size = &plugin_api_list_size;
        new_plugin->list_remove = &plugin_api_list_remove;
        new_plugin->list_remove_all = &plugin_api_list_remove_all;
        new_plugin->list_free = &plugin_api_list_free;
        
        new_plugin->config_new = &plugin_api_config_new;
        new_plugin->config_new_section = &plugin_api_config_new_section;
        new_plugin->config_search_section = &plugin_api_config_search_section;
        new_plugin->config_new_option = &plugin_api_config_new_option;
        new_plugin->config_search_option = &plugin_api_config_search_option;
        new_plugin->config_option_set = &plugin_api_config_option_set;
        new_plugin->config_string_to_boolean = &plugin_api_config_string_to_boolean;
        new_plugin->config_boolean = &plugin_api_config_boolean;
        new_plugin->config_integer = &plugin_api_config_integer;
        new_plugin->config_string = &plugin_api_config_string;
        new_plugin->config_color = &plugin_api_config_color;
        new_plugin->config_read = &plugin_api_config_read;
        new_plugin->config_reload = &plugin_api_config_reload;
        new_plugin->config_write = &plugin_api_config_write;
        new_plugin->config_write_line = &plugin_api_config_write_line;
        new_plugin->config_free = &plugin_api_config_free;
        new_plugin->config_get = &plugin_api_config_get;
        new_plugin->plugin_config_get = &plugin_api_plugin_config_get;
        new_plugin->plugin_config_set = &plugin_api_plugin_config_set;

        new_plugin->prefix = &plugin_api_prefix;
        new_plugin->color = &plugin_api_color;
        new_plugin->printf = &plugin_api_printf;
        new_plugin->printf_date = &plugin_api_printf_date;
        new_plugin->log_printf = &plugin_api_log_printf;
        new_plugin->infobar_printf = &plugin_api_infobar_printf;
        new_plugin->infobar_remove = &plugin_api_infobar_remove;
        
        new_plugin->hook_command = &plugin_api_hook_command;
        new_plugin->hook_timer = &plugin_api_hook_timer;
        new_plugin->hook_fd = &plugin_api_hook_fd;
        new_plugin->hook_print = &plugin_api_hook_print;
        new_plugin->hook_signal = &plugin_api_hook_signal;
        new_plugin->hook_signal_send = &plugin_api_hook_signal_send;
        new_plugin->hook_config = &plugin_api_hook_config;
        new_plugin->hook_completion = &plugin_api_hook_completion;
        new_plugin->unhook = &plugin_api_unhook;
        new_plugin->unhook_all = &plugin_api_unhook_all;
        
        new_plugin->buffer_new = &plugin_api_buffer_new;
        new_plugin->buffer_search = &plugin_api_buffer_search;
        new_plugin->buffer_close = &plugin_api_buffer_close;
        new_plugin->buffer_get = &plugin_api_buffer_get;
        new_plugin->buffer_set = &plugin_api_buffer_set;
        new_plugin->buffer_nick_add = &plugin_api_buffer_nick_add;
        new_plugin->buffer_nick_remove = &plugin_api_buffer_nick_remove;
        
        new_plugin->command = &plugin_api_command;
        
        new_plugin->info_get = &plugin_api_info_get;
        
        new_plugin->infolist_get = &plugin_api_infolist_get;
        new_plugin->infolist_next = &plugin_api_infolist_next;
        new_plugin->infolist_prev = &plugin_api_infolist_prev;
        new_plugin->infolist_fields = &plugin_api_infolist_fields;
        new_plugin->infolist_integer = &plugin_api_infolist_integer;
        new_plugin->infolist_string = &plugin_api_infolist_string;
        new_plugin->infolist_pointer = &plugin_api_infolist_pointer;
        new_plugin->infolist_time = &plugin_api_infolist_time;
        new_plugin->infolist_free = &plugin_api_infolist_free;
        
        new_plugin->log = &plugin_api_log;
        
        /* add new plugin to list */
        new_plugin->prev_plugin = last_weechat_plugin;
        new_plugin->next_plugin = NULL;
        if (weechat_plugins)
            last_weechat_plugin->next_plugin = new_plugin;
        else
            weechat_plugins = new_plugin;
        last_weechat_plugin = new_plugin;
        
        /* init plugin */
        if (((t_weechat_init_func *)init_func) (new_plugin) < 0)
        {
            gui_chat_printf (NULL,
                             _("%sError: unable to initialize plugin "
                               "\"%s\"\n"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             full_name);
            plugin_remove (new_plugin);
            free (full_name);
            return NULL;
        }
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sError: unable to load plugin \"%s\" "
                           "(not enough memory)\n"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         full_name);
        free (full_name);
        return NULL;
    }
    
    gui_chat_printf (NULL,
                     _("%sPlugin \"%s\" %s loaded.\n"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                     name, new_plugin->version);
    
    free (full_name);
    
    return new_plugin;
}

/*
 * plugin_auto_load_file: load a file found by plugin_auto_load,
 *                        but only it this is really a dynamic library
 */

int
plugin_auto_load_file (struct t_weechat_plugin *plugin, char *filename)
{
    char *pos;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (CONFIG_STRING(config_plugins_extension)
        && CONFIG_STRING(config_plugins_extension)[0])
    {
        pos = strstr (filename, CONFIG_STRING(config_plugins_extension));
        if (pos)
        {
            if (string_strcasecmp (pos,
                                   CONFIG_STRING(config_plugins_extension)) == 0)
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

void
plugin_auto_load ()
{
    char *ptr_home, *dir_name, *plugins_path, *plugins_path2;
    char *list_plugins, *pos, *pos2;
    
    if (CONFIG_STRING(config_plugins_autoload)
        && CONFIG_STRING(config_plugins_autoload)[0])
    {
        if (string_strcasecmp (CONFIG_STRING(config_plugins_autoload),
                               "*") == 0)
        {
            /* auto-load plugins in WeeChat home dir */
            if (CONFIG_STRING(config_plugins_path)
                && CONFIG_STRING(config_plugins_path)[0])
            {
                ptr_home = getenv ("HOME");
                plugins_path = string_replace (CONFIG_STRING(config_plugins_path),
                                               "~", ptr_home);
                plugins_path2 = string_replace ((plugins_path) ?
                                                plugins_path : CONFIG_STRING(config_plugins_path),
                                                "%h", weechat_home);
                plugin_exec_on_files (NULL,
                                      (plugins_path2) ?
                                      plugins_path2 : ((plugins_path) ?
                                                       plugins_path : CONFIG_STRING(config_plugins_path)),
                                      &plugin_auto_load_file);
                if (plugins_path)
                    free (plugins_path);
                if (plugins_path2)
                    free (plugins_path2);
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
        else
        {
            list_plugins = strdup (CONFIG_STRING(config_plugins_autoload));
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
 * plugin_remove: remove a WeeChat plugin
 */

void
plugin_remove (struct t_weechat_plugin *plugin)
{
    struct t_weechat_plugin *new_weechat_plugins;
    
    /* remove plugin from list */
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
    
    /* remove all config files */
    config_file_free_all_plugin (plugin);
    
    /* remove all hooks */
    unhook_all_plugin (plugin);
    
    /* free data */
    if (plugin->filename)
        free (plugin->filename);
    dlclose (plugin->handle);
    if (plugin->name)
        free (plugin->name);
    if (plugin->description)
        free (plugin->description);
    if (plugin->version)
        free (plugin->version);
    if (plugin->charset)
        free (plugin->charset);
    free (plugin);
    
    weechat_plugins = new_weechat_plugins;
}

/*
 * plugin_unload: unload a WeeChat plugin
 */

void
plugin_unload (struct t_weechat_plugin *plugin)
{
    t_weechat_end_func *end_func;
    char *name;
    
    name = (plugin->name) ? strdup (plugin->name) : strdup ("???");
    
    end_func = dlsym (plugin->handle, "weechat_plugin_end");
    if (end_func)
        (void) (end_func) (plugin);
    plugin_remove (plugin);
    
    gui_chat_printf (NULL,
                     _("%sPlugin \"%s\" unloaded.\n"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                     (name) ? name : "???");
    if (name)
        free (name);
}

/*
 * plugin_unload_name: unload a WeeChat plugin by name
 */

void
plugin_unload_name (char *name)
{
    struct t_weechat_plugin *ptr_plugin;
    
    ptr_plugin = plugin_search (name);
    if (ptr_plugin)
        plugin_unload (ptr_plugin);
    else
    {
        gui_chat_printf (NULL,
                         _("%sError: plugin \"%s\" not found\n"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
    }
}

/*
 * plugin_unload_all: unload all WeeChat plugins
 */

void
plugin_unload_all ()
{
    while (weechat_plugins)
        plugin_unload (last_weechat_plugin);
}

/*
 * plugin_reload_name: reload a WeeChat plugin by name
 */

void
plugin_reload_name (char *name)
{
    struct t_weechat_plugin *ptr_plugin;
    char *filename;
    
    ptr_plugin = plugin_search (name);
    if (ptr_plugin)
    {
        filename = strdup (ptr_plugin->filename);
        if (filename)
        {
            plugin_unload (ptr_plugin);
            gui_chat_printf (NULL,
                             _("%sPlugin \"%s\" unloaded.\n"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_INFO],
                             name);
            plugin_load (filename);
            free (filename);
        }
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sError: plugin \"%s\" not found\n"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
    }
}

/*
 * plugin_init: init plugin support
 */

void
plugin_init (int auto_load)
{
    /* read plugins options on disk */
    plugin_config_init ();
    plugin_config_read ();
    
    /* auto-load plugins if asked */
    if (auto_load)
        plugin_auto_load ();
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

/*
 * plugin_print_log: print plugin infos in log (usually for crash dump)
 */

void
plugin_print_log ()
{
    struct t_weechat_plugin *ptr_plugin;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        log_printf ("");
        log_printf ("[plugin (addr:0x%X)]", ptr_plugin);
        log_printf ("  filename . . . . . . . : '%s'", ptr_plugin->filename);
        log_printf ("  handle . . . . . . . . : 0x%X", ptr_plugin->handle);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_plugin->name);
        log_printf ("  description. . . . . . : '%s'", ptr_plugin->description);
        log_printf ("  version. . . . . . . . : '%s'", ptr_plugin->version);
        log_printf ("  charset. . . . . . . . : '%s'", ptr_plugin->charset);
        log_printf ("  prev_plugin. . . . . . : 0x%X", ptr_plugin->prev_plugin);
        log_printf ("  next_plugin. . . . . . : 0x%X", ptr_plugin->next_plugin);
    }

    plugin_infolist_print_log ();
}
