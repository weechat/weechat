/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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
#include "../core/wee-list.h"
#include "../core/wee-log.h"
#include "../core/wee-network.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../core/wee-util.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-nicklist.h"
#include "plugin.h"
#include "plugin-api.h"
#include "plugin-config.h"
#include "plugin-infolist.h"


struct t_weechat_plugin *weechat_plugins = NULL;
struct t_weechat_plugin *last_weechat_plugin = NULL;

int plugin_argc;                       /* command line arguments (used only */
char **plugin_argv;                    /* first time loading plugin)        */


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
    char *ptr_home, *full_name, *full_name2;
    void *handle;
    char *name, *author, *description, *version, *weechat_version, *license;
    char *charset;
    t_weechat_init_func *init_func;
    int rc, i, argc;
    char **argv;
    struct t_weechat_plugin *new_plugin;
    
    if (!filename)
        return NULL;
    
    full_name = util_search_full_lib_name (filename, "plugins");
    
    if (!full_name)
        return NULL;
    
    ptr_home = getenv ("HOME");
    full_name2 = string_replace (full_name, "~", ptr_home);
    
    if (full_name2)
    {
        free (full_name);
        full_name = full_name2;
    }
    
    handle = dlopen (full_name, RTLD_GLOBAL | RTLD_NOW);
    if (!handle)
    {
        gui_chat_printf (NULL,
                         _("%sError: unable to load plugin \"%s\": %s"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         full_name, dlerror());
        free (full_name);
        return NULL;
    }
    
    /* look for plugin name */
    name = dlsym (handle, "weechat_plugin_name");
    if (!name)
    {
        gui_chat_printf (NULL,
                         _("%sError: symbol \"%s\" not found in "
                           "plugin \"%s\", failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_name",
                         full_name);
        dlclose (handle);
        free (full_name);
        return NULL;
    }
    
    /* check for plugin with same name */
    if (plugin_search (name))
    {
        gui_chat_printf (NULL,
                         _("%sError: unable to load plugin \"%s\": a plugin "
                           "with same name already exists"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         full_name);
        dlclose (handle);
        free (full_name);
        return NULL;
    }
    
    /* look for plugin description */
    description = dlsym (handle, "weechat_plugin_description");
    if (!description)
    {
        gui_chat_printf (NULL,
                         _("%sError: symbol \"%s\" not found "
                           "in plugin \"%s\", failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_description",
                         full_name);
        dlclose (handle);
        free (full_name);
        return NULL;
    }
    
    /* look for plugin author */
    author = dlsym (handle, "weechat_plugin_author");
    if (!author)
    {
        gui_chat_printf (NULL,
                         _("%sError: symbol \"%s\" not found "
                           "in plugin \"%s\", failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_author",
                         full_name);
        dlclose (handle);
        free (full_name);
        return NULL;
    }
    
    /* look for plugin version */
    version = dlsym (handle, "weechat_plugin_version");
    if (!version)
    {
        gui_chat_printf (NULL,
                         _("%sError: symbol \"%s\" not found in "
                           "plugin \"%s\", failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_version",
                         full_name);
        dlclose (handle);
        free (full_name);
        return NULL;
    }
    
    /* look for WeeChat version required for plugin */
    weechat_version = dlsym (handle, "weechat_plugin_weechat_version");
    if (!weechat_version)
    {
        gui_chat_printf (NULL,
                         _("%sError: symbol \"%s\" not found in "
                           "plugin \"%s\", failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_weechat_version",
                         full_name);
        dlclose (handle);
        free (full_name);
        return NULL;
    }

    if (util_weechat_version_cmp (PACKAGE_VERSION, weechat_version) != 0)
    {
        gui_chat_printf (NULL,
                         _("%sError: plugin \"%s\" is compiled for WeeChat "
                           "%s and you are running version %s, failed to "
                           "load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         full_name,
                         weechat_version, PACKAGE_VERSION);
        dlclose (handle);
        free (full_name);
        return NULL;
    }
    
    /* look for plugin license */
    license = dlsym (handle, "weechat_plugin_license");
    if (!license)
    {
        gui_chat_printf (NULL,
                         _("%sError: symbol \"%s\" not found in "
                           "plugin \"%s\", failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_license",
                         full_name);
        dlclose (handle);
        free (full_name);
        return NULL;
    }
    
    /* look for plugin charset (optional, default is UTF-8) */
    charset = dlsym (handle, "weechat_plugin_charset");
    
    /* look for plugin init function */
    init_func = dlsym (handle, "weechat_plugin_init");
    if (!init_func)
    {
        gui_chat_printf (NULL,
                         _("%sError: function \"%s\" not "
                           "found in plugin \"%s\", failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_init",
                         full_name);
        dlclose (handle);
        free (full_name);
        return NULL;
    }
    
    /* create new plugin */
    new_plugin = malloc (sizeof (*new_plugin));
    if (new_plugin)
    {
        /* variables */
        new_plugin->filename = strdup (full_name);
        new_plugin->handle = handle;
        new_plugin->name = strdup (name);
        new_plugin->description = strdup (description);
        new_plugin->author = strdup (author);
        new_plugin->version = strdup (version);
        new_plugin->weechat_version = strdup (weechat_version);
        new_plugin->license = strdup (license);
        new_plugin->charset = (charset) ? strdup (charset) : NULL;
        
        /* functions */
        new_plugin->charset_set = &plugin_api_charset_set;
        new_plugin->iconv_to_internal = &string_iconv_to_internal;
        new_plugin->iconv_from_internal = &string_iconv_from_internal;
        new_plugin->gettext = &plugin_api_gettext;
        new_plugin->ngettext = &plugin_api_ngettext;
        new_plugin->strndup = &string_strndup;
        new_plugin->string_tolower = &string_tolower;
        new_plugin->string_toupper = &string_toupper;
        new_plugin->strcasecmp = &string_strcasecmp;
        new_plugin->strncasecmp = &string_strncasecmp;
        new_plugin->strcmp_ignore_chars = &string_strcmp_ignore_chars;
        new_plugin->strcasestr = &string_strcasestr;
        new_plugin->string_match = &string_match;
        new_plugin->string_replace = &string_replace;
        new_plugin->string_remove_quotes = &string_remove_quotes;
        new_plugin->string_strip = &string_strip;
        new_plugin->string_explode = &string_explode;
        new_plugin->string_free_exploded = &string_free_exploded;
        new_plugin->string_split_command = &string_split_command;
        new_plugin->string_free_splitted_command = &string_free_splitted_command;
        
        new_plugin->utf8_has_8bits = &utf8_has_8bits;
        new_plugin->utf8_is_valid = &utf8_is_valid;
        new_plugin->utf8_normalize = &utf8_normalize;
        new_plugin->utf8_prev_char = &utf8_prev_char;
        new_plugin->utf8_next_char = &utf8_next_char;
        new_plugin->utf8_char_size = &utf8_char_size;
        new_plugin->utf8_strlen = &utf8_strlen;
        new_plugin->utf8_strnlen = &utf8_strnlen;
        new_plugin->utf8_strlen_screen = &utf8_strlen_screen;
        new_plugin->utf8_charcasecmp = &utf8_charcasecmp;
        new_plugin->utf8_char_size_screen = &utf8_char_size_screen;
        new_plugin->utf8_add_offset = &utf8_add_offset;
        new_plugin->utf8_real_pos = &utf8_real_pos;
        new_plugin->utf8_pos = &utf8_pos;
        
        new_plugin->mkdir_home = &plugin_api_mkdir_home;
        new_plugin->mkdir = &plugin_api_mkdir;
        new_plugin->exec_on_files = &util_exec_on_files;
        
        new_plugin->timeval_cmp = &util_timeval_cmp;
        new_plugin->timeval_diff = &util_timeval_diff;
        new_plugin->timeval_add = &util_timeval_add;
        
        new_plugin->list_new = &weelist_new;
        new_plugin->list_add = &weelist_add;
        new_plugin->list_search = &weelist_search;
        new_plugin->list_casesearch = &weelist_casesearch;
        new_plugin->list_get = &weelist_get;
        new_plugin->list_set = &weelist_set;
        new_plugin->list_next = &weelist_next;
        new_plugin->list_prev = &weelist_prev;
        new_plugin->list_string = &weelist_string;
        new_plugin->list_size = &weelist_size;
        new_plugin->list_remove = &weelist_remove;
        new_plugin->list_remove_all = &weelist_remove_all;
        new_plugin->list_free = &weelist_free;
        
        new_plugin->config_new = &config_file_new;
        new_plugin->config_new_section = &config_file_new_section;
        new_plugin->config_search_section = &config_file_search_section;
        new_plugin->config_new_option = &config_file_new_option;
        new_plugin->config_search_option = &config_file_search_option;
        new_plugin->config_search_section_option = &config_file_search_section_option;
        new_plugin->config_search_with_string = &config_file_search_with_string;
        new_plugin->config_string_to_boolean = &config_file_string_to_boolean;
        new_plugin->config_option_reset = &config_file_option_reset;
        new_plugin->config_option_set = &config_file_option_set;
        new_plugin->config_option_rename = &config_file_option_rename;
        new_plugin->config_option_get_pointer = &config_file_option_get_pointer;
        new_plugin->config_boolean = &config_file_option_boolean;
        new_plugin->config_integer = &config_file_option_integer;
        new_plugin->config_string = &config_file_option_string;
        new_plugin->config_color = &config_file_option_color;
        new_plugin->config_write_line = &config_file_write_line;
        new_plugin->config_write = &config_file_write;
        new_plugin->config_read = &config_file_read;
        new_plugin->config_reload = &config_file_reload;
        new_plugin->config_option_free = &config_file_option_free;
        new_plugin->config_section_free_options = &config_file_section_free_options;
        new_plugin->config_section_free = &config_file_section_free;
        new_plugin->config_free = &config_file_free;
        new_plugin->config_get = &plugin_api_config_get;
        new_plugin->config_get_plugin = &plugin_api_config_get_plugin;
        new_plugin->config_set_plugin = &plugin_api_config_set_plugin;
        
        new_plugin->prefix = &plugin_api_prefix;
        new_plugin->color = &plugin_api_color;
        new_plugin->printf_date_tags = &gui_chat_printf_date_tags;
        new_plugin->printf_y = &gui_chat_printf_y;
        new_plugin->infobar_printf = &plugin_api_infobar_printf;
        new_plugin->infobar_remove = &plugin_api_infobar_remove;
        new_plugin->log_printf = &log_printf;
        
        new_plugin->hook_command = &hook_command;
        new_plugin->hook_timer = &hook_timer;
        new_plugin->hook_fd = &hook_fd;
        new_plugin->hook_connect = &hook_connect;
        new_plugin->hook_print = &hook_print;
        new_plugin->hook_signal = &hook_signal;
        new_plugin->hook_signal_send = &hook_signal_send;
        new_plugin->hook_config = &hook_config;
        new_plugin->hook_completion = &hook_completion;
        new_plugin->hook_modifier = &hook_modifier;
        new_plugin->hook_modifier_exec = &hook_modifier_exec;
        new_plugin->unhook = &unhook;
        new_plugin->unhook_all = &unhook_all_plugin;
        
        new_plugin->buffer_new = &gui_buffer_new;
        new_plugin->buffer_search = &gui_buffer_search_by_category_name;
        new_plugin->buffer_clear = &gui_buffer_clear;
        new_plugin->buffer_close = &gui_buffer_close;
        new_plugin->buffer_get_string = &gui_buffer_get_string;
        new_plugin->buffer_get_pointer = &gui_buffer_get_pointer;
        new_plugin->buffer_set = &gui_buffer_set;

        new_plugin->nicklist_add_group = &gui_nicklist_add_group;
        new_plugin->nicklist_search_group = &gui_nicklist_search_group;
        new_plugin->nicklist_add_nick = &gui_nicklist_add_nick;
        new_plugin->nicklist_search_nick = &gui_nicklist_search_nick;
        new_plugin->nicklist_remove_group = &gui_nicklist_remove_group;
        new_plugin->nicklist_remove_nick = &gui_nicklist_remove_nick;
        new_plugin->nicklist_remove_all = &gui_nicklist_remove_all;
        
        new_plugin->bar_item_search = &gui_bar_item_search;
        new_plugin->bar_item_new = &gui_bar_item_new;
        new_plugin->bar_item_update = &gui_bar_item_update;
        new_plugin->bar_item_remove = &gui_bar_item_free;
        new_plugin->bar_search = &gui_bar_search;
        new_plugin->bar_new = &gui_bar_new;
        new_plugin->bar_set = &gui_bar_set;
        new_plugin->bar_update = &gui_bar_update;
        new_plugin->bar_remove = &gui_bar_free;
        
        new_plugin->command = &plugin_api_command;
        
        new_plugin->network_pass_proxy = &network_pass_proxy;
        new_plugin->network_connect_to = &network_connect_to;
        
        new_plugin->info_get = &plugin_api_info_get;

        new_plugin->infolist_new = &plugin_infolist_new;
        new_plugin->infolist_new_item = &plugin_infolist_new_item;
        new_plugin->infolist_new_var_integer = &plugin_infolist_new_var_integer;
        new_plugin->infolist_new_var_string = &plugin_infolist_new_var_string;
        new_plugin->infolist_new_var_pointer = &plugin_infolist_new_var_pointer;
        new_plugin->infolist_new_var_time = &plugin_infolist_new_var_time;
        new_plugin->infolist_get = &plugin_api_infolist_get;
        new_plugin->infolist_next = &plugin_api_infolist_next;
        new_plugin->infolist_prev = &plugin_api_infolist_prev;
        new_plugin->infolist_fields = &plugin_api_infolist_fields;
        new_plugin->infolist_integer = &plugin_api_infolist_integer;
        new_plugin->infolist_string = &plugin_api_infolist_string;
        new_plugin->infolist_pointer = &plugin_api_infolist_pointer;
        new_plugin->infolist_time = &plugin_api_infolist_time;
        new_plugin->infolist_free = &plugin_api_infolist_free;
        
        /* add new plugin to list */
        new_plugin->prev_plugin = last_weechat_plugin;
        new_plugin->next_plugin = NULL;
        if (weechat_plugins)
            last_weechat_plugin->next_plugin = new_plugin;
        else
            weechat_plugins = new_plugin;
        last_weechat_plugin = new_plugin;

        /* build arguments for plugin */
        argc = 0;
        argv = NULL;
        if (plugin_argc > 0)
        {
            argv = malloc ((plugin_argc + 1) * sizeof (*argv));
            if (argv)
            {
                argc = 0;
                for (i = 0; i < plugin_argc; i++)
                {
                    if ((string_strcasecmp (plugin_argv[i], "-a") == 0)
                        || (string_strcasecmp (plugin_argv[i], "--no-connect") == 0)
                        || (string_strncasecmp (plugin_argv[i], name, strlen (name)) == 0))
                    {
                        argv[argc] = plugin_argv[i];
                        argc++;
                    }
                }
                if (argc == 0)
                {
                    free (argv);
                    argv = NULL;
                }
                else
                    argv[argc] = NULL;
            }
        }
        
        /* init plugin */
        rc = ((t_weechat_init_func *)init_func) (new_plugin, argc, argv);
        
        if (argv)
            free (argv);
        
        if (rc != WEECHAT_RC_OK)
        {
            gui_chat_printf (NULL,
                             _("%sError: unable to initialize plugin "
                               "\"%s\""),
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
                           "(not enough memory)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         full_name);
        dlclose (handle);
        free (full_name);
        return NULL;
    }
    
    gui_chat_printf (NULL,
                     _("Plugin \"%s\" loaded"),
                     name);
    
    free (full_name);
    
    return new_plugin;
}

/*
 * plugin_auto_load_file: load a file found by plugin_auto_load,
 *                        but only it this is really a dynamic library
 */

int
plugin_auto_load_file (void *plugin, char *filename)
{
    char *pos;
    
    /* make C compiler happy */
    (void) plugin;
    
    if (CONFIG_STRING(config_plugin_extension)
        && CONFIG_STRING(config_plugin_extension)[0])
    {
        pos = strstr (filename, CONFIG_STRING(config_plugin_extension));
        if (pos)
        {
            if (string_strcasecmp (pos,
                                   CONFIG_STRING(config_plugin_extension)) == 0)
            {
                plugin_load (filename);
            }
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
    char *ptr_home, *dir_name, *plugin_path, *plugin_path2;
    char *list_plugins, *pos, *pos2;
    
    if (CONFIG_STRING(config_plugin_autoload)
        && CONFIG_STRING(config_plugin_autoload)[0])
    {
        if (string_strcasecmp (CONFIG_STRING(config_plugin_autoload),
                               "*") == 0)
        {
            /* auto-load plugins in WeeChat home dir */
            if (CONFIG_STRING(config_plugin_path)
                && CONFIG_STRING(config_plugin_path)[0])
            {
                ptr_home = getenv ("HOME");
                plugin_path = string_replace (CONFIG_STRING(config_plugin_path),
                                              "~", ptr_home);
                plugin_path2 = string_replace ((plugin_path) ?
                                               plugin_path : CONFIG_STRING(config_plugin_path),
                                               "%h", weechat_home);
                util_exec_on_files ((plugin_path2) ?
                                    plugin_path2 : ((plugin_path) ?
                                                    plugin_path : CONFIG_STRING(config_plugin_path)),
                                    NULL,
                                    &plugin_auto_load_file);
                if (plugin_path)
                    free (plugin_path);
                if (plugin_path2)
                    free (plugin_path2);
            }
    
            /* auto-load plugins in WeeChat global lib dir */
            dir_name = malloc (strlen (WEECHAT_LIBDIR) + 16);
            if (dir_name)
            {
                snprintf (dir_name, strlen (WEECHAT_LIBDIR) + 16,
                          "%s/plugins", WEECHAT_LIBDIR);
                util_exec_on_files (dir_name, NULL, &plugin_auto_load_file);
                free (dir_name);
            }
        }
        else
        {
            list_plugins = strdup (CONFIG_STRING(config_plugin_autoload));
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
    struct t_gui_buffer *ptr_buffer, *next_buffer;
    
    /* close buffers created by this plugin */
    ptr_buffer = gui_buffers;
    while (ptr_buffer)
    {
        next_buffer = ptr_buffer->next_buffer;
        
        if (ptr_buffer->plugin == plugin)
        {
            ptr_buffer->close_callback = NULL;
            ptr_buffer->close_callback_data = NULL;
            gui_buffer_close (ptr_buffer, 1);
        }
        
        ptr_buffer = next_buffer;
    }
    
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
    
    /* remove all bar items */
    gui_bar_item_free_all_plugin (plugin);
    
    /* free data */
    if (plugin->filename)
        free (plugin->filename);
    dlclose (plugin->handle);
    if (plugin->name)
        free (plugin->name);
    if (plugin->description)
        free (plugin->description);
    if (plugin->author)
        free (plugin->author);
    if (plugin->version)
        free (plugin->version);
    if (plugin->weechat_version)
        free (plugin->weechat_version);
    if (plugin->license)
        free (plugin->license);
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
    
    name = (plugin->name) ? strdup (plugin->name) : NULL;
    
    end_func = dlsym (plugin->handle, "weechat_plugin_end");
    if (end_func)
        (void) (end_func) (plugin);
    
    plugin_remove (plugin);
    
    gui_chat_printf (NULL,
                     _("Plugin \"%s\" unloaded"),
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
                         _("%sError: plugin \"%s\" not found"),
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
    {
        plugin_unload (last_weechat_plugin);
    }
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
                             _("Plugin \"%s\" unloaded"),
                             name);
            plugin_load (filename);
            free (filename);
        }
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sError: plugin \"%s\" not found"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
    }
}

/*
 * plugin_init: init plugin support
 */

void
plugin_init (int auto_load, int argc, char *argv[])
{
    plugin_argc = argc;
    plugin_argv = argv;
    
    /* read plugins options on disk */
    plugin_config_init ();
    plugin_config_read ();
    
    /* auto-load plugins if asked */
    if (auto_load)
        plugin_auto_load ();

    /* discard command arguments for future plugins */
    plugin_argc = 0;
    plugin_argv = NULL;
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
    
    /* free all plugin options */
    plugin_config_end ();
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
        log_printf ("[plugin (addr:0x%x)]", ptr_plugin);
        log_printf ("  filename . . . . . . . : '%s'", ptr_plugin->filename);
        log_printf ("  handle . . . . . . . . : 0x%x", ptr_plugin->handle);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_plugin->name);
        log_printf ("  description. . . . . . : '%s'", ptr_plugin->description);
        log_printf ("  version. . . . . . . . : '%s'", ptr_plugin->version);
        log_printf ("  charset. . . . . . . . : '%s'", ptr_plugin->charset);
        log_printf ("  prev_plugin. . . . . . : 0x%x", ptr_plugin->prev_plugin);
        log_printf ("  next_plugin. . . . . . : 0x%x", ptr_plugin->next_plugin);
    }

    plugin_infolist_print_log ();
}
