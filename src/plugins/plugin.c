/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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

/*
 * plugin.c: WeeChat plugins management (load/unload dynamic C libraries)
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-list.h"
#include "../core/wee-log.h"
#include "../core/wee-network.h"
#include "../core/wee-string.h"
#include "../core/wee-upgrade-file.h"
#include "../core/wee-utf8.h"
#include "../core/wee-util.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "plugin.h"
#include "plugin-api.h"
#include "plugin-config.h"


int plugin_quiet = 0;
struct t_weechat_plugin *weechat_plugins = NULL;
struct t_weechat_plugin *last_weechat_plugin = NULL;

int plugin_argc;                       /* command line arguments (used only */
char **plugin_argv;                    /* first time loading plugin)        */

int plugin_autoload_count = 0;         /* number of items in autoload_array */
char **plugin_autoload_array = NULL;   /* autoload array, this is split of  */
                                       /* option "weechat.plugin.autoload"  */


void plugin_remove (struct t_weechat_plugin *plugin);


/*
 * plugin_valid: check if a plugin pointer exists
 *               return 1 if plugin exists
 *                      0 if plugin is not found
 */

int
plugin_valid (struct t_weechat_plugin *plugin)
{
    struct t_weechat_plugin *ptr_plugin;
    
    if (!plugin)
        return 0;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        if (ptr_plugin == plugin)
            return 1;
    }
    
    /* plugin not found */
    return 0;
}

/*
 * plugin_search: search a plugin by name
 */

struct t_weechat_plugin *
plugin_search (const char *name)
{
    struct t_weechat_plugin *ptr_plugin;
    
    if (!name)
        return NULL;
    
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
 * plugin_get_name: get name of plugin with a pointer
 */

const char *
plugin_get_name (struct t_weechat_plugin *plugin)
{
    static char *plugin_core = PLUGIN_CORE;
    
    return (plugin) ? plugin->name : plugin_core;
}

/*
 * plugin_check_autoload: check if a plugin can be autoloaded or not
 *                        return 1 if plugin can be autoloaded
 *                               0 if plugin can NOT be autoloaded
 *                        list of autoloaded plugins is set in option
 *                        "weechat.plugin.autoload"
 */

int
plugin_check_autoload (char *plugin_full_name)
{
    int i, plugin_authorized, plugin_blacklisted;
    char *ptr_base_name, *base_name, *plugin_name, *pos;
    
    /* by default we can auto load all plugins */
    if (!plugin_autoload_array)
        return 1;
    
    /* get short name of plugin (filename without extension) */
    plugin_name = NULL;
    ptr_base_name = basename (plugin_full_name);
    if (!ptr_base_name)
        return 1;
    
    base_name = strdup (ptr_base_name);
    if (!base_name)
        return 1;
    
    if (CONFIG_STRING(config_plugin_extension)
        && CONFIG_STRING(config_plugin_extension)[0])
    {
        pos = strstr (base_name,
                      CONFIG_STRING(config_plugin_extension));
        plugin_name = (pos) ?
            string_strndup (base_name, pos - base_name) :
            strdup (base_name);
    }
    else
        plugin_name = strdup (base_name);
    
    free (base_name);
    
    if (!plugin_name)
        return 1;
    
    /* browse array and check if plugin is "authorized" or "blacklisted" */
    plugin_authorized = 0;
    plugin_blacklisted = 0;
    for (i = 0; i < plugin_autoload_count; i++)
    {
        if (plugin_autoload_array[i][0] == '!')
        {
            /*
             * negative value: it is used to "blacklist" a plugin
             * for example with "*,!perl", all plugins are loaded, but not perl
             */
            if (string_match (plugin_name, plugin_autoload_array[i] + 1, 0))
                plugin_blacklisted = 1;
        }
        else
        {
            if (string_match (plugin_name, plugin_autoload_array[i], 0))
                plugin_authorized = 1;
        }
    }
    
    free (plugin_name);
    
    if (plugin_blacklisted)
        return 0;
    
    return plugin_authorized;
}

/*
 * plugin_find_pos: find position for a plugin (for sorting plugins list)
 */

struct t_weechat_plugin *
plugin_find_pos (struct t_weechat_plugin *plugin)
{
    struct t_weechat_plugin *ptr_plugin;

    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        if (string_strcasecmp (plugin->name, ptr_plugin->name) < 0)
            return ptr_plugin;
    }
    return NULL;
}

/*
 * plugin_load: load a WeeChat plugin (a dynamic library)
 *              return: pointer to new WeeChat plugin, NULL if error
 */

struct t_weechat_plugin *
plugin_load (const char *filename)
{
    char *ptr_home, *full_name, *full_name2;
    void *handle;
    char *name, *api_version, *author, *description, *version;
    char *license, *charset;
    t_weechat_init_func *init_func;
    int rc, i, argc;
    char **argv;
    struct t_weechat_plugin *new_plugin;
    struct t_config_option *ptr_option;
    
    if (!filename)
        return NULL;
    
    full_name = util_search_full_lib_name (filename, "plugins");
    
    if (!full_name)
        return NULL;
    
    /*
     * if plugin must not be autoloaded, then return immediately
     * Note: the "plugin_autoload_array" variable is set only during auto-load,
     * ie when WeeChat is starting or when doing /plugin autoload
     */
    if (plugin_autoload_array && !plugin_check_autoload (full_name))
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
        gui_chat_printf (NULL,
                         _("%sIf you're trying to load a script and not a C "
                           "plugin, try command to load scripts (/perl, "
                           "/python, ...)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
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
    
    /* look for API version */
    api_version = dlsym (handle, "weechat_plugin_api_version");
    if (!api_version)
    {
        gui_chat_printf (NULL,
                         _("%sError: symbol \"%s\" not found in "
                           "plugin \"%s\", failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_api_version",
                         full_name);
        gui_chat_printf (NULL,
                         _("%sIf plugin \"%s\" is old/obsolete, you can "
                           "delete this file."),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
        dlclose (handle);
        free (full_name);
        return NULL;
    }
    if (strcmp (api_version, WEECHAT_PLUGIN_API_VERSION) != 0)
    {
        gui_chat_printf (NULL,
                         _("%sError: API mismatch for plugin \"%s\" (current "
                           "API: \"%s\", plugin API: \"%s\"), failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         full_name,
                         WEECHAT_PLUGIN_API_VERSION,
                         api_version);
        gui_chat_printf (NULL,
                         _("%sIf plugin \"%s\" is old/obsolete, you can "
                           "delete this file."),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
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
        new_plugin->license = strdup (license);
        new_plugin->charset = (charset) ? strdup (charset) : NULL;
        ptr_option = config_weechat_debug_get (name);
        new_plugin->debug = (ptr_option) ? CONFIG_INTEGER(ptr_option) : 0;
        
        /* functions */
        new_plugin->plugin_get_name = &plugin_get_name;
        
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
        new_plugin->string_has_highlight = &string_has_highlight;
        new_plugin->string_mask_to_regex = &string_mask_to_regex;
        new_plugin->string_split = &string_split;
        new_plugin->string_free_split = &string_free_split;
        new_plugin->string_build_with_split_string = &string_build_with_split_string;
        new_plugin->string_split_command = &string_split_command;
        new_plugin->string_free_split_command = &string_free_split_command;
        new_plugin->string_format_size = &string_format_size;
        new_plugin->string_remove_color = &gui_color_decode;
        new_plugin->string_encode_base64 = &string_encode_base64;
        new_plugin->string_decode_base64 = &string_decode_base64;
        new_plugin->string_is_command_char = &string_is_command_char;
        new_plugin->string_input_for_buffer = &string_input_for_buffer;
        
        new_plugin->utf8_has_8bits = &utf8_has_8bits;
        new_plugin->utf8_is_valid = &utf8_is_valid;
        new_plugin->utf8_normalize = &utf8_normalize;
        new_plugin->utf8_prev_char = &utf8_prev_char;
        new_plugin->utf8_next_char = &utf8_next_char;
        new_plugin->utf8_char_int = &utf8_char_int;
        new_plugin->utf8_char_size = &utf8_char_size;
        new_plugin->utf8_strlen = &utf8_strlen;
        new_plugin->utf8_strnlen = &utf8_strnlen;
        new_plugin->utf8_strlen_screen = &utf8_strlen_screen;
        new_plugin->utf8_charcmp = &utf8_charcmp;
        new_plugin->utf8_charcasecmp = &utf8_charcasecmp;
        new_plugin->utf8_char_size_screen = &utf8_char_size_screen;
        new_plugin->utf8_add_offset = &utf8_add_offset;
        new_plugin->utf8_real_pos = &utf8_real_pos;
        new_plugin->utf8_pos = &utf8_pos;
        new_plugin->utf8_strndup = &utf8_strndup;
        
        new_plugin->mkdir_home = &util_mkdir_home;
        new_plugin->mkdir = &util_mkdir;
        new_plugin->mkdir_parents = &util_mkdir_parents;
        new_plugin->exec_on_files = &util_exec_on_files;
        new_plugin->file_get_content = &util_file_get_content;
        
        new_plugin->util_timeval_cmp = &util_timeval_cmp;
        new_plugin->util_timeval_diff = &util_timeval_diff;
        new_plugin->util_timeval_add = &util_timeval_add;
        new_plugin->util_get_time_string = &util_get_time_string;
        
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
        new_plugin->config_option_set_null = &config_file_option_set_null;
        new_plugin->config_option_unset = &config_file_option_unset;
        new_plugin->config_option_rename = &config_file_option_rename;
        new_plugin->config_option_get_pointer = &config_file_option_get_pointer;
        new_plugin->config_option_is_null = &config_file_option_is_null;
        new_plugin->config_option_default_is_null = &config_file_option_default_is_null;
        new_plugin->config_boolean = &config_file_option_boolean;
        new_plugin->config_boolean_default = &config_file_option_boolean_default;
        new_plugin->config_integer = &config_file_option_integer;
        new_plugin->config_integer_default = &config_file_option_integer_default;
        new_plugin->config_string = &config_file_option_string;
        new_plugin->config_string_default = &config_file_option_string_default;
        new_plugin->config_color = &config_file_option_color;
        new_plugin->config_color_default = &config_file_option_color_default;
        new_plugin->config_write_option = &config_file_write_option;
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
        new_plugin->config_is_set_plugin = &plugin_api_config_is_set_plugin;
        new_plugin->config_set_plugin = &plugin_api_config_set_plugin;
        new_plugin->config_unset_plugin = &plugin_api_config_unset_plugin;
        
        new_plugin->prefix = &plugin_api_prefix;
        new_plugin->color = &plugin_api_color;
        new_plugin->printf_date_tags = &gui_chat_printf_date_tags;
        new_plugin->printf_y = &gui_chat_printf_y;
        new_plugin->log_printf = &log_printf;
        
        new_plugin->hook_command = &hook_command;
        new_plugin->hook_command_run = &hook_command_run;
        new_plugin->hook_timer = &hook_timer;
        new_plugin->hook_fd = &hook_fd;
        new_plugin->hook_process = &hook_process;
        new_plugin->hook_connect = &hook_connect;
        new_plugin->hook_print = &hook_print;
        new_plugin->hook_signal = &hook_signal;
        new_plugin->hook_signal_send = &hook_signal_send;
        new_plugin->hook_config = &hook_config;
        new_plugin->hook_completion = &hook_completion;
        new_plugin->hook_completion_list_add = &hook_completion_list_add;
        new_plugin->hook_modifier = &hook_modifier;
        new_plugin->hook_modifier_exec = &hook_modifier_exec;
        new_plugin->hook_info = &hook_info;
        new_plugin->hook_infolist = &hook_infolist;
        new_plugin->unhook = &unhook;
        new_plugin->unhook_all = &unhook_all_plugin;
        
        new_plugin->buffer_new = &gui_buffer_new;
        new_plugin->buffer_search = &gui_buffer_search_by_name;
        new_plugin->buffer_search_main = &gui_buffer_search_main;
        new_plugin->buffer_clear = &gui_buffer_clear;
        new_plugin->buffer_close = &gui_buffer_close;
        new_plugin->buffer_merge = &gui_buffer_merge;
        new_plugin->buffer_unmerge = &gui_buffer_unmerge;
        new_plugin->buffer_get_integer = &gui_buffer_get_integer;
        new_plugin->buffer_get_string = &gui_buffer_get_string;
        new_plugin->buffer_get_pointer = &gui_buffer_get_pointer;
        new_plugin->buffer_set = &gui_buffer_set;
        new_plugin->buffer_set_pointer = &gui_buffer_set_pointer;
        new_plugin->buffer_string_replace_local_var = &gui_buffer_string_replace_local_var;
        
        new_plugin->window_get_integer = &gui_window_get_integer;
        new_plugin->window_get_string = &gui_window_get_string;
        new_plugin->window_get_pointer = &gui_window_get_pointer;
        new_plugin->window_set_title = &gui_window_set_title;
        
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
        
        new_plugin->info_get = &hook_info_get;
        
        new_plugin->infolist_new = &infolist_new;
        new_plugin->infolist_new_item = &infolist_new_item;
        new_plugin->infolist_new_var_integer = &infolist_new_var_integer;
        new_plugin->infolist_new_var_string = &infolist_new_var_string;
        new_plugin->infolist_new_var_pointer = &infolist_new_var_pointer;
        new_plugin->infolist_new_var_buffer = &infolist_new_var_buffer;
        new_plugin->infolist_new_var_time = &infolist_new_var_time;
        new_plugin->infolist_get = &hook_infolist_get;
        new_plugin->infolist_next = &plugin_api_infolist_next;
        new_plugin->infolist_prev = &plugin_api_infolist_prev;
        new_plugin->infolist_reset_item_cursor = &plugin_api_infolist_reset_item_cursor;
        new_plugin->infolist_fields = &plugin_api_infolist_fields;
        new_plugin->infolist_integer = &plugin_api_infolist_integer;
        new_plugin->infolist_string = &plugin_api_infolist_string;
        new_plugin->infolist_pointer = &plugin_api_infolist_pointer;
        new_plugin->infolist_buffer = &plugin_api_infolist_buffer;
        new_plugin->infolist_time = &plugin_api_infolist_time;
        new_plugin->infolist_free = &plugin_api_infolist_free;
        
        new_plugin->upgrade_new = &upgrade_file_new;
        new_plugin->upgrade_write_object = &upgrade_file_write_object;
        new_plugin->upgrade_read = &upgrade_file_read;
        new_plugin->upgrade_close = &upgrade_file_close;
        
        /* add new plugin to list */
        new_plugin->prev_plugin = last_weechat_plugin;
        new_plugin->next_plugin = NULL;
        if (weechat_plugins)
            last_weechat_plugin->next_plugin = new_plugin;
        else
            weechat_plugins = new_plugin;
        last_weechat_plugin = new_plugin;
        
        /*
         * associate orphan buffers with this plugin (if asked during upgrade
         * process)
         */
        gui_buffer_set_plugin_for_upgrade (name, new_plugin);
        
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
                    if ((strcmp (plugin_argv[i], "-a") == 0)
                        || (strcmp (plugin_argv[i], "--no-connect") == 0)
                        || (strcmp (plugin_argv[i], "-s") == 0)
                        || (strcmp (plugin_argv[i], "--no-script") == 0)
                        || (strcmp (plugin_argv[i], "--upgrade") == 0)
                        || (strncmp (plugin_argv[i], name, strlen (name)) == 0))
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
    
    if ((weechat_debug_core >= 1) || !plugin_quiet)
    {
        gui_chat_printf (NULL,
                         _("Plugin \"%s\" loaded"),
                         name);
    }
    
    free (full_name);
    
    return new_plugin;
}

/*
 * plugin_auto_load_file: load a file found by plugin_auto_load,
 *                        but only it this is really a dynamic library
 */

void
plugin_auto_load_file (void *plugin, const char *filename)
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
}

/*
 * plugin_auto_load: auto-load WeeChat plugins, from user and system
 *                   directories
 */

void
plugin_auto_load ()
{
    char *ptr_home, *dir_name, *plugin_path, *plugin_path2;
    
    plugin_autoload_array = NULL;
    plugin_autoload_count = 0;
    
    if (CONFIG_STRING(config_plugin_autoload)
        && CONFIG_STRING(config_plugin_autoload)[0])
    {
        plugin_autoload_array = string_split (CONFIG_STRING(config_plugin_autoload),
                                              ",", 0, 0,
                                              &plugin_autoload_count);
    }
    
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
                            0,
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
        util_exec_on_files (dir_name, 0, NULL, &plugin_auto_load_file);
        free (dir_name);
    }
    
    /* free autoload array */
    if (plugin_autoload_array)
    {
        string_free_split (plugin_autoload_array);
        plugin_autoload_array = NULL;
    }
    plugin_autoload_count = 0;
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
            gui_buffer_close (ptr_buffer);
        
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
    if (!weechat_plugin_no_dlclose)
        dlclose (plugin->handle);
    if (plugin->name)
        free (plugin->name);
    if (plugin->description)
        free (plugin->description);
    if (plugin->author)
        free (plugin->author);
    if (plugin->version)
        free (plugin->version);
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
    
    if ((weechat_debug_core >= 1) || !plugin_quiet)
    {
        gui_chat_printf (NULL,
                         _("Plugin \"%s\" unloaded"),
                         (name) ? name : "???");
    }
    if (name)
        free (name);
}

/*
 * plugin_unload_name: unload a WeeChat plugin by name
 */

void
plugin_unload_name (const char *name)
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
    int plugins_loaded;
    
    plugins_loaded = (weechat_plugins) ? 1 : 0;
    
    plugin_quiet = 1;
    while (weechat_plugins)
    {
        plugin_unload (last_weechat_plugin);
    }
    plugin_quiet = 0;
    
    if (plugins_loaded)
    {
        gui_chat_printf (NULL, _("Plugins unloaded"));
    }
}

/*
 * plugin_reload_name: reload a WeeChat plugin by name
 */

void
plugin_reload_name (const char *name)
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
 * plugin_display_short_list: print list of plugins on one line
 */

void
plugin_display_short_list ()
{
    const char *plugins_loaded;
    char *buf;
    int length;
    struct t_weechat_plugin *ptr_plugin;
    struct t_weelist *list;
    struct t_weelist_item *ptr_item;
    
    if (weechat_plugins)
    {
        list = weelist_new ();
        if (list)
        {
            plugins_loaded = _("Plugins loaded:");
            
            length = strlen (plugins_loaded) + 1;
            
            for (ptr_plugin = weechat_plugins; ptr_plugin;
                 ptr_plugin = ptr_plugin->next_plugin)
            {
                length += strlen (ptr_plugin->name) + 2;
                weelist_add (list, ptr_plugin->name, WEECHAT_LIST_POS_SORT, NULL);
            }
            length++;
            
            buf = malloc (length);
            if (buf)
            {
                strcpy (buf, plugins_loaded);
                strcat (buf, " ");
                for (ptr_item = list->items; ptr_item;
                     ptr_item = ptr_item->next_item)
                {
                    strcat (buf, ptr_item->data);
                    if (ptr_item->next_item)
                        strcat (buf, ", ");
                }
                gui_chat_printf (NULL, "%s", buf);
                free (buf);
            }
            weelist_free (list);
        }
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

    /* init plugin API (create some hooks) */
    plugin_api_init ();
    
    /* read plugins options on disk */
    plugin_config_init ();
    plugin_config_read ();
    
    /* auto-load plugins if asked */
    if (auto_load)
    {
        plugin_quiet = 1;
        plugin_auto_load ();
        plugin_display_short_list ();
        plugin_quiet = 0;
    }

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
 * plugin_add_to_infolist: add a plugin in an infolist
 *                         return 1 if ok, 0 if error
 */

int
plugin_add_to_infolist (struct t_infolist *infolist,
                        struct t_weechat_plugin *plugin)
{
    struct t_infolist_item *ptr_item;
    
    if (!infolist || !plugin)
        return 0;
    
    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!infolist_new_var_pointer (ptr_item, "pointer", plugin))
        return 0;
    if (!infolist_new_var_string (ptr_item, "filename", plugin->filename))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "handle", plugin->handle))
        return 0;
    if (!infolist_new_var_string (ptr_item, "name", plugin->name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "description", plugin->description))
        return 0;
    if (!infolist_new_var_string (ptr_item, "author", plugin->author))
        return 0;
    if (!infolist_new_var_string (ptr_item, "version", plugin->version))
        return 0;
    if (!infolist_new_var_string (ptr_item, "license", plugin->license))
        return 0;
    if (!infolist_new_var_string (ptr_item, "charset", plugin->charset))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "debug", plugin->debug))
        return 0;
    
    return 1;
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
        log_printf ("[plugin (addr:0x%lx)]", ptr_plugin);
        log_printf ("  filename . . . . . . . : '%s'",  ptr_plugin->filename);
        log_printf ("  handle . . . . . . . . : 0x%lx", ptr_plugin->handle);
        log_printf ("  name . . . . . . . . . : '%s'",  ptr_plugin->name);
        log_printf ("  description. . . . . . : '%s'",  ptr_plugin->description);
        log_printf ("  version. . . . . . . . : '%s'",  ptr_plugin->version);
        log_printf ("  charset. . . . . . . . : '%s'",  ptr_plugin->charset);
        log_printf ("  debug. . . . . . . . . : %d",    ptr_plugin->debug);
        log_printf ("  prev_plugin. . . . . . : 0x%lx", ptr_plugin->prev_plugin);
        log_printf ("  next_plugin. . . . . . : 0x%lx", ptr_plugin->next_plugin);
    }
}
