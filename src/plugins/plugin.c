/*
 * plugin.c - WeeChat plugins management (load/unload dynamic C libraries)
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <libgen.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>

#include "../core/weechat.h"
#include "../core/core-arraylist.h"
#include "../core/core-config.h"
#include "../core/core-dir.h"
#include "../core/core-eval.h"
#include "../core/core-hashtable.h"
#include "../core/core-hdata.h"
#include "../core/core-hook.h"
#include "../core/core-infolist.h"
#include "../core/core-list.h"
#include "../core/core-log.h"
#include "../core/core-network.h"
#include "../core/core-string.h"
#include "../core/core-upgrade-file.h"
#include "../core/core-utf8.h"
#include "../core/core-util.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-key.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "plugin.h"
#include "plugin-api.h"
#include "plugin-config.h"


int plugin_quiet = 0;
struct t_weechat_plugin *weechat_plugins = NULL;
struct t_weechat_plugin *last_weechat_plugin = NULL;

/* structure used to give arguments to callback of ... */
struct t_plugin_args
{
    int argc;
    char **argv;
};

int plugin_autoload_count = 0;         /* number of items in autoload_array */
char **plugin_autoload_array = NULL;   /* autoload array, this is split of  */
                                       /* option "weechat.plugin.autoload"  */


void plugin_remove (struct t_weechat_plugin *plugin);


/*
 * Checks if a plugin pointer is valid.
 *
 * Returns:
 *   1: plugin exists
 *   0: plugin does not exist
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
 * Searches for a plugin by name.
 *
 * Returns pointer to plugin found, NULL if not found.
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
        if (strcmp (ptr_plugin->name, name) == 0)
            return ptr_plugin;
    }

    /* plugin not found */
    return NULL;
}

/*
 * Gets name of a plugin with a pointer.
 */

const char *
plugin_get_name (struct t_weechat_plugin *plugin)
{
    static char *plugin_core = PLUGIN_CORE;

    return (plugin) ? plugin->name : plugin_core;
}

/*
 * Checks if extension of filename is allowed by option
 * "weechat.plugin.extension".
 *
 * Returns:
 *   1: extension allowed
 *   0: extension not allowed
 */

int
plugin_check_extension_allowed (const char *filename)
{
    int i, length, length_ext;

    /* extension allowed if no extension is defined */
    if (!config_plugin_extensions)
        return 1;

    if (!filename)
        return 0;

    length = strlen (filename);
    for (i = 0; i < config_num_plugin_extensions; i++)
    {
        length_ext = strlen (config_plugin_extensions[i]);
        if (length >= length_ext)
        {
            if (strcmp (filename + length - length_ext,
                        config_plugin_extensions[i]) == 0)
            {
                /* extension allowed */
                return 1;
            }
        }
    }

    /* extension not allowed */
    return 0;
}

/*
 * Checks if a plugin can be autoloaded.
 *
 * List of autoloaded plugins is set in option "weechat.plugin.autoload".
 *
 * Returns:
 *   1: plugin can be autoloaded
 *   0: plugin can not be autoloaded
 */

int
plugin_check_autoload (const char *filename)
{
    int i, length, length_ext, match;
    char *full_name, *ptr_base_name, *base_name, *plugin_name;

    /* by default we can auto load all plugins */
    if (!plugin_autoload_array)
        return 1;

    full_name = strdup (filename);
    if (!full_name)
        return 0;

    /* get short name of plugin (filename without extension) */
    plugin_name = NULL;
    ptr_base_name = basename (full_name);
    if (!ptr_base_name)
    {
        free (full_name);
        return 1;
    }

    base_name = strdup (ptr_base_name);
    if (!base_name)
    {
        free (full_name);
        return 1;
    }

    free (full_name);

    if (config_plugin_extensions)
    {
        length = strlen (base_name);
        for (i = 0; i < config_num_plugin_extensions; i++)
        {
            length_ext = strlen (config_plugin_extensions[i]);
            if (length >= length_ext)
            {
                if (strcmp (base_name + length - length_ext,
                            config_plugin_extensions[i]) == 0)
                {
                    plugin_name = string_strndup (base_name, length - length_ext);
                    break;
                }
            }
        }
    }
    else
    {
        plugin_name = strdup (base_name);
    }

    free (base_name);

    if (!plugin_name)
        return 1;

    match = string_match_list (plugin_name,
                               (const char **)plugin_autoload_array,
                               1);

    free (plugin_name);

    return match;
}

/*
 * Returns arguments for plugins (only the relevant arguments for plugins,
 * arguments for WeeChat core not returned).
 *
 * Note: plugin_argv must be freed after use.
 */

void
plugin_get_args (int argc, char **argv,
                 int *plugin_argc, char ***plugin_argv,
                 int *no_connect, int *no_script)
{
    int i, temp_argc;
    char **temp_argv;

    temp_argc = 0;
    temp_argv = NULL;

    *no_connect = 0;
    *no_script = 0;

    if (argc > 0)
    {
        temp_argv = malloc ((argc + 1) * sizeof (*temp_argv));
        if (temp_argv)
        {
            for (i = 0; i < argc; i++)
            {
                if ((strcmp (argv[i], "-a") == 0)
                    || (strcmp (argv[i], "--no-connect") == 0))
                {
                    *no_connect = 1;
                }
                else if ((strcmp (argv[i], "-s") == 0)
                         || (strcmp (argv[i], "--no-script") == 0))
                {
                    *no_script = 1;
                }
                else if (argv[i][0] != '-')
                {
                    temp_argv[temp_argc++] = argv[i];
                }
            }
            if (temp_argc == 0)
            {
                free (temp_argv);
                temp_argv = NULL;
            }
            else
                temp_argv[temp_argc] = NULL;
        }
    }

    *plugin_argc = temp_argc;
    *plugin_argv = temp_argv;
}

/*
 * Initializes a plugin by calling its init() function.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
plugin_call_init (struct t_weechat_plugin *plugin, int argc, char **argv)
{
    t_weechat_init_func *init_func;
    int no_connect, rc, old_auto_connect, no_script, old_auto_load_scripts;
    int plugin_argc;
    char **plugin_argv;

    if (plugin->initialized)
        return 1;

    /* look for plugin init function */
    init_func = dlsym (plugin->handle, "weechat_plugin_init");
    if (!init_func)
        return 0;

    /* get arguments for the plugin */
    plugin_get_args (argc, argv,
                     &plugin_argc, &plugin_argv, &no_connect, &no_script);

    old_auto_connect = weechat_auto_connect;
    weechat_auto_connect = (no_connect) ? 0 : 1;

    old_auto_load_scripts = weechat_auto_load_scripts;
    weechat_auto_load_scripts = (no_script) ? 0 : 1;

    /* init plugin */
    if (weechat_debug_core >= 1)
    {
        gui_chat_printf (NULL,
                         _("Initializing plugin \"%s\" (priority: %d)"),
                         plugin->name,
                         plugin->priority);
    }
    rc = ((t_weechat_init_func *)init_func) (plugin,
                                             plugin_argc, plugin_argv);
    if (rc == WEECHAT_RC_OK)
    {
        plugin->initialized = 1;
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sUnable to initialize plugin \"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         plugin->filename);
    }

    weechat_auto_connect = old_auto_connect;
    weechat_auto_load_scripts = old_auto_load_scripts;

    free (plugin_argv);

    return (rc == WEECHAT_RC_OK) ? 1 : 0;
}

/*
 * Loads a WeeChat plugin (a dynamic library).
 *
 * If init_plugin == 1, then the init() function in plugin is called
 * (with argc/argv), otherwise the plugin is just loaded but not initialized.
 *
 * Returns a pointer to new WeeChat plugin, NULL if error.
 */

struct t_weechat_plugin *
plugin_load (const char *filename, int init_plugin, int argc, char **argv)
{
    void *handle;
    char *name, *api_version, *author, *description, *version;
    char *license, *charset;
    t_weechat_init_func *init_func;
    int *priority;
    struct t_weechat_plugin *new_plugin;
    struct t_config_option *ptr_option;

    if (!filename)
        return NULL;

    /*
     * if plugin must not be autoloaded, then return immediately
     * Note: the "plugin_autoload_array" variable is set only during auto-load,
     * ie when WeeChat is starting or when doing /plugin autoload
     */
    if (plugin_autoload_array && !plugin_check_autoload (filename))
        return NULL;

    handle = dlopen (filename, RTLD_GLOBAL | RTLD_NOW);
    if (!handle)
    {
        gui_chat_printf (NULL,
                         _("%sUnable to load plugin \"%s\": %s"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename, dlerror ());
        gui_chat_printf (NULL,
                         _("%sIf you're trying to load a script and not a C "
                           "plugin, try command to load scripts (/perl, "
                           "/python, ...)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        return NULL;
    }

    /* look for plugin name */
    name = dlsym (handle, "weechat_plugin_name");
    if (!name)
    {
        gui_chat_printf (NULL,
                         _("%sSymbol \"%s\" not found in plugin \"%s\", "
                           "failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_name",
                         filename);
        dlclose (handle);
        return NULL;
    }

    /* look for API version */
    api_version = dlsym (handle, "weechat_plugin_api_version");
    if (!api_version)
    {
        gui_chat_printf (NULL,
                         _("%sSymbol \"%s\" not found in plugin \"%s\", "
                           "failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_api_version",
                         filename);
        gui_chat_printf (NULL,
                         _("%sIf plugin \"%s\" is old/obsolete, you can "
                           "delete this file."),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
        dlclose (handle);
        return NULL;
    }
    if (strcmp (api_version, WEECHAT_PLUGIN_API_VERSION) != 0)
    {
        gui_chat_printf (NULL,
                         _("%sAPI mismatch for plugin \"%s\" (current API: "
                           "\"%s\", plugin API: \"%s\"), failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename,
                         WEECHAT_PLUGIN_API_VERSION,
                         api_version);
        gui_chat_printf (NULL,
                         _("%sIf plugin \"%s\" is old/obsolete, you can "
                           "delete this file."),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
        dlclose (handle);
        return NULL;
    }

    /* check for plugin with same name */
    if (plugin_search (name))
    {
        gui_chat_printf (NULL,
                         _("%sUnable to load plugin \"%s\": a plugin with "
                           "same name already exists"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename);
        dlclose (handle);
        return NULL;
    }

    /* look for plugin description */
    description = dlsym (handle, "weechat_plugin_description");
    if (!description)
    {
        gui_chat_printf (NULL,
                         _("%sSymbol \"%s\" not found in plugin \"%s\", "
                           "failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_description",
                         filename);
        dlclose (handle);
        return NULL;
    }

    /* look for plugin author */
    author = dlsym (handle, "weechat_plugin_author");
    if (!author)
    {
        gui_chat_printf (NULL,
                         _("%sSymbol \"%s\" not found in plugin \"%s\", "
                           "failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_author",
                         filename);
        dlclose (handle);
        return NULL;
    }

    /* look for plugin version */
    version = dlsym (handle, "weechat_plugin_version");
    if (!version)
    {
        gui_chat_printf (NULL,
                         _("%sSymbol \"%s\" not found in plugin \"%s\", "
                           "failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_version",
                         filename);
        dlclose (handle);
        return NULL;
    }

    /* look for plugin license */
    license = dlsym (handle, "weechat_plugin_license");
    if (!license)
    {
        gui_chat_printf (NULL,
                         _("%sSymbol \"%s\" not found in plugin \"%s\", "
                           "failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_license",
                         filename);
        dlclose (handle);
        return NULL;
    }

    /* look for plugin charset (optional, default is UTF-8) */
    charset = dlsym (handle, "weechat_plugin_charset");

    /* look for plugin init function */
    init_func = dlsym (handle, "weechat_plugin_init");
    if (!init_func)
    {
        gui_chat_printf (NULL,
                         _("%sFunction \"%s\" not found in plugin \"%s\", "
                           "failed to load"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         "weechat_plugin_init",
                         filename);
        dlclose (handle);
        return NULL;
    }

    /*
     * look for plugin priority: it is used to initialize plugins in
     * appropriate order: the important plugins that don't depend on other
     * plugins are initialized first
     */
    priority = dlsym (handle, "weechat_plugin_priority");

    /* create new plugin */
    new_plugin = malloc (sizeof (*new_plugin));
    if (new_plugin)
    {
        /* variables */
        new_plugin->filename = strdup (filename);
        new_plugin->handle = handle;
        new_plugin->name = strdup (name);
        new_plugin->description = strdup (description);
        new_plugin->author = strdup (author);
        new_plugin->version = strdup (version);
        new_plugin->license = strdup (license);
        new_plugin->charset = (charset) ? strdup (charset) : NULL;
        new_plugin->priority = (priority) ?
            *priority : PLUGIN_PRIORITY_DEFAULT;
        new_plugin->initialized = 0;
        ptr_option = config_weechat_debug_get (name);
        new_plugin->debug = (ptr_option) ? CONFIG_INTEGER(ptr_option) : 0;
        new_plugin->upgrading = weechat_upgrading;
        new_plugin->variables = hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING, WEECHAT_HASHTABLE_STRING,
            NULL, NULL);

        /* functions */
        new_plugin->plugin_get_name = &plugin_get_name;

        new_plugin->charset_set = &plugin_api_charset_set;
        new_plugin->iconv_to_internal = &string_iconv_to_internal;
        new_plugin->iconv_from_internal = &string_iconv_from_internal;
        new_plugin->gettext = &plugin_api_gettext;
        new_plugin->ngettext = &plugin_api_ngettext;
        new_plugin->asprintf = &string_asprintf;
        new_plugin->strndup = &string_strndup;
        new_plugin->string_cut = &string_cut;
        new_plugin->string_tolower = &string_tolower;
        new_plugin->string_toupper = &string_toupper;
        new_plugin->string_charcmp = &string_charcmp;
        new_plugin->string_charcasecmp = &string_charcasecmp;
        new_plugin->strcmp = &string_strcmp;
        new_plugin->strncmp = &string_strncmp;
        new_plugin->strcasecmp = &string_strcasecmp;
        new_plugin->strcasecmp_range = &string_strcasecmp_range;
        new_plugin->strncasecmp = &string_strncasecmp;
        new_plugin->strncasecmp_range = &string_strncasecmp_range;
        new_plugin->strcmp_ignore_chars = &string_strcmp_ignore_chars;
        new_plugin->strcasestr = &string_strcasestr;
        new_plugin->strlen_screen = &gui_chat_strlen_screen;
        new_plugin->string_match = &string_match;
        new_plugin->string_match_list = &string_match_list;
        new_plugin->string_replace = &string_replace;
        new_plugin->string_expand_home = &string_expand_home;
        new_plugin->string_eval_path_home = &string_eval_path_home;
        new_plugin->string_remove_quotes = &string_remove_quotes;
        new_plugin->string_strip = &string_strip;
        new_plugin->string_convert_escaped_chars = &string_convert_escaped_chars;
        new_plugin->string_mask_to_regex = &string_mask_to_regex;
        new_plugin->string_regex_flags = &string_regex_flags;
        new_plugin->string_regcomp = &string_regcomp;
        new_plugin->string_has_highlight = &string_has_highlight;
        new_plugin->string_has_highlight_regex = &string_has_highlight_regex;
        new_plugin->string_replace_regex = &string_replace_regex;
        new_plugin->string_translate_chars = &string_translate_chars;
        new_plugin->string_split = &string_split;
        new_plugin->string_split_shell = &string_split_shell;
        new_plugin->string_free_split = &string_free_split;
        new_plugin->string_rebuild_split_string = &string_rebuild_split_string;
        new_plugin->string_split_command = &string_split_command;
        new_plugin->string_free_split_command = &string_free_split_command;
        new_plugin->string_format_size = &string_format_size;
        new_plugin->string_parse_size = &string_parse_size;
        new_plugin->string_color_code_size = &gui_color_code_size;
        new_plugin->string_remove_color = &gui_color_decode;
        new_plugin->string_base_encode = &string_base_encode;
        new_plugin->string_base_decode = &string_base_decode;
        new_plugin->string_hex_dump = &string_hex_dump;
        new_plugin->string_is_command_char = &string_is_command_char;
        new_plugin->string_input_for_buffer = &string_input_for_buffer;
        new_plugin->string_eval_expression = &eval_expression;
        new_plugin->string_dyn_alloc = &string_dyn_alloc;
        new_plugin->string_dyn_copy = &string_dyn_copy;
        new_plugin->string_dyn_concat = &string_dyn_concat;
        new_plugin->string_dyn_free = &string_dyn_free;
        new_plugin->string_concat = &string_concat;

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
        new_plugin->utf8_char_size_screen = &utf8_char_size_screen;
        new_plugin->utf8_add_offset = &utf8_add_offset;
        new_plugin->utf8_real_pos = &utf8_real_pos;
        new_plugin->utf8_pos = &utf8_pos;
        new_plugin->utf8_strndup = &utf8_strndup;
        new_plugin->utf8_strncpy = &utf8_strncpy;

        new_plugin->crypto_hash = &plugin_api_crypto_hash;
        new_plugin->crypto_hash_file = &plugin_api_crypto_hash_file;
        new_plugin->crypto_hash_pbkdf2 = &plugin_api_crypto_hash_pbkdf2;
        new_plugin->crypto_hmac = &plugin_api_crypto_hmac;

        new_plugin->mkdir_home = &dir_mkdir_home;
        new_plugin->mkdir = &dir_mkdir;
        new_plugin->mkdir_parents = &dir_mkdir_parents;
        new_plugin->exec_on_files = &dir_exec_on_files;
        new_plugin->file_get_content = &dir_file_get_content;
        new_plugin->file_copy = &dir_file_copy;
        new_plugin->file_compress = &dir_file_compress;

        new_plugin->util_timeval_cmp = &util_timeval_cmp;
        new_plugin->util_timeval_diff = &util_timeval_diff;
        new_plugin->util_timeval_add = &util_timeval_add;
        new_plugin->util_get_time_string = &util_get_time_string;
        new_plugin->util_strftimeval = &util_strftimeval;
        new_plugin->util_parse_time = &util_parse_time;
        new_plugin->util_version_number = &util_version_number;

        new_plugin->list_new = &weelist_new;
        new_plugin->list_add = &weelist_add;
        new_plugin->list_search = &weelist_search;
        new_plugin->list_search_pos = &weelist_search_pos;
        new_plugin->list_casesearch = &weelist_casesearch;
        new_plugin->list_casesearch_pos = &weelist_casesearch_pos;
        new_plugin->list_get = &weelist_get;
        new_plugin->list_set = &weelist_set;
        new_plugin->list_next = &weelist_next;
        new_plugin->list_prev = &weelist_prev;
        new_plugin->list_string = &weelist_string;
        new_plugin->list_user_data = &weelist_user_data;
        new_plugin->list_size = &weelist_size;
        new_plugin->list_remove = &weelist_remove;
        new_plugin->list_remove_all = &weelist_remove_all;
        new_plugin->list_free = &weelist_free;

        new_plugin->arraylist_new = arraylist_new;
        new_plugin->arraylist_size = arraylist_size;
        new_plugin->arraylist_get = arraylist_get;
        new_plugin->arraylist_search = arraylist_search;
        new_plugin->arraylist_insert = arraylist_insert;
        new_plugin->arraylist_add = arraylist_add;
        new_plugin->arraylist_remove = arraylist_remove;
        new_plugin->arraylist_clear = arraylist_clear;
        new_plugin->arraylist_free = arraylist_free;

        new_plugin->hashtable_new = &hashtable_new;
        new_plugin->hashtable_set_with_size = &hashtable_set_with_size;
        new_plugin->hashtable_set = &hashtable_set;
        new_plugin->hashtable_get = &hashtable_get;
        new_plugin->hashtable_has_key = &hashtable_has_key;
        new_plugin->hashtable_map = &hashtable_map;
        new_plugin->hashtable_map_string = &hashtable_map_string;
        new_plugin->hashtable_dup = &hashtable_dup;
        new_plugin->hashtable_get_integer = &hashtable_get_integer;
        new_plugin->hashtable_get_string = &hashtable_get_string;
        new_plugin->hashtable_set_pointer = &hashtable_set_pointer;
        new_plugin->hashtable_add_to_infolist = &hashtable_add_to_infolist;
        new_plugin->hashtable_add_from_infolist = &hashtable_add_from_infolist;
        new_plugin->hashtable_remove = &hashtable_remove;
        new_plugin->hashtable_remove_all = &hashtable_remove_all;
        new_plugin->hashtable_free = &hashtable_free;

        new_plugin->config_new = &config_file_new;
        new_plugin->config_set_version = &config_file_set_version;
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
        new_plugin->config_option_get_string = &config_file_option_get_string;
        new_plugin->config_option_get_pointer = &config_file_option_get_pointer;
        new_plugin->config_option_is_null = &config_file_option_is_null;
        new_plugin->config_option_default_is_null = &config_file_option_default_is_null;
        new_plugin->config_boolean = &config_file_option_boolean;
        new_plugin->config_boolean_inherited = &config_file_option_boolean_inherited;
        new_plugin->config_boolean_default = &config_file_option_boolean_default;
        new_plugin->config_integer = &config_file_option_integer;
        new_plugin->config_integer_inherited = &config_file_option_integer_inherited;
        new_plugin->config_integer_default = &config_file_option_integer_default;
        new_plugin->config_enum = &config_file_option_enum;
        new_plugin->config_enum_inherited = &config_file_option_enum_inherited;
        new_plugin->config_enum_default = &config_file_option_enum_default;
        new_plugin->config_string = &config_file_option_string;
        new_plugin->config_string_inherited = &config_file_option_string_inherited;
        new_plugin->config_string_default = &config_file_option_string_default;
        new_plugin->config_color = &config_file_option_color;
        new_plugin->config_color_inherited = &config_file_option_color_inherited;
        new_plugin->config_color_default = &config_file_option_color_default;
        new_plugin->config_write_option = &config_file_write_option;
        new_plugin->config_write_line = &config_file_write_line;
        new_plugin->config_write = &config_file_write;
        new_plugin->config_read = &config_file_read;
        new_plugin->config_reload = &config_file_reload;
        new_plugin->config_option_free = &plugin_api_config_file_option_free;
        new_plugin->config_section_free_options = &config_file_section_free_options;
        new_plugin->config_section_free = &config_file_section_free;
        new_plugin->config_free = &config_file_free;
        new_plugin->config_get = &plugin_api_config_get;
        new_plugin->config_get_plugin = &plugin_api_config_get_plugin;
        new_plugin->config_is_set_plugin = &plugin_api_config_is_set_plugin;
        new_plugin->config_set_plugin = &plugin_api_config_set_plugin;
        new_plugin->config_set_desc_plugin = &plugin_api_config_set_desc_plugin;
        new_plugin->config_unset_plugin = &plugin_api_config_unset_plugin;

        new_plugin->key_bind = &gui_key_bind_plugin;
        new_plugin->key_unbind = &gui_key_unbind_plugin;

        new_plugin->prefix = &plugin_api_prefix;
        new_plugin->color = &plugin_api_color;
        new_plugin->printf_datetime_tags = &gui_chat_printf_datetime_tags;
        new_plugin->printf_y_datetime_tags = &gui_chat_printf_y_datetime_tags;
        new_plugin->log_printf = &log_printf;

        new_plugin->hook_command = &hook_command;
        new_plugin->hook_command_run = &hook_command_run;
        new_plugin->hook_timer = &hook_timer;
        new_plugin->hook_fd = &hook_fd;
        new_plugin->hook_process = &hook_process;
        new_plugin->hook_process_hashtable = &hook_process_hashtable;
        new_plugin->hook_url = &hook_url;
        new_plugin->hook_connect = &hook_connect;
        new_plugin->hook_line = &hook_line;
        new_plugin->hook_print = &hook_print;
        new_plugin->hook_signal = &hook_signal;
        new_plugin->hook_signal_send = &hook_signal_send;
        new_plugin->hook_hsignal = &hook_hsignal;
        new_plugin->hook_hsignal_send = &hook_hsignal_send;
        new_plugin->hook_config = &hook_config;
        new_plugin->hook_completion = &hook_completion;
        new_plugin->hook_completion_get_string = &gui_completion_get_string;
        new_plugin->hook_completion_list_add = &gui_completion_list_add;
        new_plugin->hook_modifier = &hook_modifier;
        new_plugin->hook_modifier_exec = &hook_modifier_exec;
        new_plugin->hook_info = &hook_info;
        new_plugin->hook_info_hashtable = &hook_info_hashtable;
        new_plugin->hook_infolist = &hook_infolist;
        new_plugin->hook_hdata = &hook_hdata;
        new_plugin->hook_focus = &hook_focus;
        new_plugin->hook_set = &hook_set;
        new_plugin->unhook = &unhook;
        new_plugin->unhook_all = &unhook_all_plugin;

        new_plugin->buffer_new = &gui_buffer_new;
        new_plugin->buffer_new_props = &gui_buffer_new_props;
        new_plugin->buffer_search = &gui_buffer_search;
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
        new_plugin->buffer_match_list = &gui_buffer_match_list;

        new_plugin->window_search_with_buffer = &gui_window_search_with_buffer;
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
        new_plugin->nicklist_get_next_item = &gui_nicklist_get_next_item;
        new_plugin->nicklist_group_get_integer = &gui_nicklist_group_get_integer;
        new_plugin->nicklist_group_get_string = &gui_nicklist_group_get_string;
        new_plugin->nicklist_group_get_pointer = &gui_nicklist_group_get_pointer;
        new_plugin->nicklist_group_set = &gui_nicklist_group_set;
        new_plugin->nicklist_nick_get_integer = &gui_nicklist_nick_get_integer;
        new_plugin->nicklist_nick_get_string = &gui_nicklist_nick_get_string;
        new_plugin->nicklist_nick_get_pointer = &gui_nicklist_nick_get_pointer;
        new_plugin->nicklist_nick_set = &gui_nicklist_nick_set;

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
        new_plugin->command_options = &plugin_api_command_options;

        new_plugin->completion_new = &gui_completion_new;
        new_plugin->completion_search = &gui_completion_search;
        new_plugin->completion_get_string = &gui_completion_get_string;
        new_plugin->completion_list_add = &gui_completion_list_add;
        new_plugin->completion_free = &gui_completion_free;

        new_plugin->network_pass_proxy = &network_pass_proxy;
        new_plugin->network_connect_to = &network_connect_to;

        new_plugin->info_get = &hook_info_get;
        new_plugin->info_get_hashtable = &hook_info_get_hashtable;

        new_plugin->infolist_new = &infolist_new;
        new_plugin->infolist_new_item = &infolist_new_item;
        new_plugin->infolist_new_var_integer = &infolist_new_var_integer;
        new_plugin->infolist_new_var_string = &infolist_new_var_string;
        new_plugin->infolist_new_var_pointer = &infolist_new_var_pointer;
        new_plugin->infolist_new_var_buffer = &infolist_new_var_buffer;
        new_plugin->infolist_new_var_time = &infolist_new_var_time;
        new_plugin->infolist_search_var = &infolist_search_var;
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

        new_plugin->hdata_new = &hdata_new;
        new_plugin->hdata_new_var = &hdata_new_var;
        new_plugin->hdata_new_list = &hdata_new_list;
        new_plugin->hdata_get = &hook_hdata_get;
        new_plugin->hdata_get_var_offset = &hdata_get_var_offset;
        new_plugin->hdata_get_var_type = &hdata_get_var_type;
        new_plugin->hdata_get_var_type_string = &hdata_get_var_type_string;
        new_plugin->hdata_get_var_array_size = &hdata_get_var_array_size;
        new_plugin->hdata_get_var_array_size_string = &hdata_get_var_array_size_string;
        new_plugin->hdata_get_var_hdata = &hdata_get_var_hdata;
        new_plugin->hdata_get_var = &hdata_get_var;
        new_plugin->hdata_get_var_at_offset = &hdata_get_var_at_offset;
        new_plugin->hdata_get_list = &hdata_get_list;
        new_plugin->hdata_check_pointer = &hdata_check_pointer;
        new_plugin->hdata_move = &hdata_move;
        new_plugin->hdata_search = &hdata_search;
        new_plugin->hdata_char = &hdata_char;
        new_plugin->hdata_integer = &hdata_integer;
        new_plugin->hdata_long = &hdata_long;
        new_plugin->hdata_longlong = &hdata_longlong;
        new_plugin->hdata_string = &hdata_string;
        new_plugin->hdata_pointer = &hdata_pointer;
        new_plugin->hdata_time = &hdata_time;
        new_plugin->hdata_hashtable = &hdata_hashtable;
        new_plugin->hdata_compare = &hdata_compare;
        new_plugin->hdata_set = &hdata_set;
        new_plugin->hdata_update = &hdata_update;
        new_plugin->hdata_get_string = &hdata_get_string;

        new_plugin->upgrade_new = &upgrade_file_new;
        new_plugin->upgrade_write_object = &upgrade_file_write_object;
        new_plugin->upgrade_read = &upgrade_file_read;
        new_plugin->upgrade_close = &upgrade_file_close;

        /* add new plugin to list */
        new_plugin->prev_plugin = last_weechat_plugin;
        new_plugin->next_plugin = NULL;
        if (last_weechat_plugin)
            last_weechat_plugin->next_plugin = new_plugin;
        else
            weechat_plugins = new_plugin;
        last_weechat_plugin = new_plugin;

        /*
         * associate orphan buffers with this plugin (if asked during upgrade
         * process)
         */
        gui_buffer_set_plugin_for_upgrade (name, new_plugin);

        if (init_plugin)
        {
            if (!plugin_call_init (new_plugin, argc, argv))
            {
                plugin_remove (new_plugin);
                return NULL;
            }
        }
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sUnable to load plugin \"%s\" (not enough memory)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename);
        dlclose (handle);
        return NULL;
    }

    if ((weechat_debug_core >= 1) || !plugin_quiet)
    {
        gui_chat_printf (NULL,
                         _("Plugin \"%s\" loaded"),
                         name);
    }

    (void) hook_signal_send ("plugin_loaded",
                             WEECHAT_HOOK_SIGNAL_STRING, (char *)filename);

    return new_plugin;
}

/*
 * Loads a file found by function plugin_auto_load, but only if this is really a
 * dynamic library.
 */

void
plugin_auto_load_file (void *data, const char *filename)
{
    struct t_plugin_args *plugin_args;

    plugin_args = (struct t_plugin_args *)data;

    if (plugin_check_extension_allowed (filename))
        plugin_load (filename, 0, plugin_args->argc, plugin_args->argv);
}

/*
 * Callback used to sort plugins arraylist by priority (high priority first).
 */

int
plugin_arraylist_cmp_cb (void *data,
                         struct t_arraylist *arraylist,
                         void *pointer1, void *pointer2)
{
    struct t_weechat_plugin *plugin1, *plugin2;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    plugin1 = (struct t_weechat_plugin *)pointer1;
    plugin2 = (struct t_weechat_plugin *)pointer2;

    return (plugin1->priority > plugin2->priority) ?
        -1 : ((plugin1->priority < plugin2->priority) ? 1 : 0);
}

/*
 * Auto-loads WeeChat plugins, from user and system directories.
 */

void
plugin_auto_load (char *force_plugin_autoload,
                  int load_from_plugin_path,
                  int load_from_extra_lib_dir,
                  int load_from_lib_dir,
                  int argc, char **argv)
{
    char *dir_name, *plugin_path, *extra_libdir;
    const char *ptr_plugin_autoload;
    struct t_weechat_plugin *ptr_plugin;
    struct t_plugin_args plugin_args;
    struct t_arraylist *arraylist;
    struct t_hashtable *options;
    int length, i;

    plugin_args.argc = argc;
    plugin_args.argv = argv;

    plugin_autoload_array = NULL;
    plugin_autoload_count = 0;

    ptr_plugin_autoload = (force_plugin_autoload) ?
        force_plugin_autoload : CONFIG_STRING(config_plugin_autoload);

    if (ptr_plugin_autoload && ptr_plugin_autoload[0])
    {
        plugin_autoload_array = string_split (
            ptr_plugin_autoload,
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &plugin_autoload_count);
    }

    /* auto-load plugins in custom path */
    if (load_from_plugin_path
        && CONFIG_STRING(config_plugin_path)
        && CONFIG_STRING(config_plugin_path)[0])
    {
        options = hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL, NULL);
        if (options)
            hashtable_set (options, "directory", "data");
        plugin_path = string_eval_path_home (CONFIG_STRING(config_plugin_path),
                                             NULL, NULL, options);
        hashtable_free (options);
        if (plugin_path)
        {
            dir_exec_on_files (plugin_path, 1, 0,
                               &plugin_auto_load_file, &plugin_args);
            free (plugin_path);
        }
    }

    /* auto-load plugins in WEECHAT_EXTRA_LIBDIR environment variable */
    if (load_from_extra_lib_dir)
    {
        extra_libdir = getenv (WEECHAT_EXTRA_LIBDIR);
        if (extra_libdir && extra_libdir[0])
        {
            length = strlen (extra_libdir) + 16 + 1;
            dir_name = malloc (length);
            snprintf (dir_name, length, "%s/plugins", extra_libdir);
            dir_exec_on_files (dir_name, 1, 0,
                               &plugin_auto_load_file, &plugin_args);
            free (dir_name);
        }
    }

    /* auto-load plugins in WeeChat global lib dir */
    if (load_from_lib_dir)
    {
        length = strlen (WEECHAT_LIBDIR) + 16 + 1;
        dir_name = malloc (length);
        if (dir_name)
        {
            snprintf (dir_name, length, "%s/plugins", WEECHAT_LIBDIR);
            dir_exec_on_files (dir_name, 1, 0,
                               &plugin_auto_load_file, &plugin_args);
            free (dir_name);
        }
    }

    /* free autoload array */
    if (plugin_autoload_array)
    {
        string_free_split (plugin_autoload_array);
        plugin_autoload_array = NULL;
    }
    plugin_autoload_count = 0;

    /* initialize all uninitialized plugins */
    arraylist = arraylist_new (10, 1, 1,
                               &plugin_arraylist_cmp_cb, NULL, NULL, NULL);
    if (arraylist)
    {
        for (ptr_plugin = weechat_plugins; ptr_plugin;
             ptr_plugin = ptr_plugin->next_plugin)
        {
            arraylist_add (arraylist, ptr_plugin);
        }
        i = 0;
        while (i < arraylist_size (arraylist))
        {
            ptr_plugin = arraylist_get (arraylist, i);
            if (!ptr_plugin->initialized)
            {
                if (!plugin_call_init (ptr_plugin, argc, argv))
                {
                    plugin_remove (ptr_plugin);
                    arraylist_remove (arraylist, i);
                }
                else
                    i++;
            }
            else
                i++;
        }
        arraylist_free (arraylist);
    }
}

/*
 * Removes a WeeChat plugin.
 */

void
plugin_remove (struct t_weechat_plugin *plugin)
{
    struct t_weechat_plugin *new_weechat_plugins;
    struct t_gui_buffer *ptr_buffer, *next_buffer;

    /* remove all completions (only those created by API) */
    gui_completion_free_all_plugin (plugin);

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

    /* remove all configuration files */
    config_file_free_all_plugin (plugin);

    /* remove all hooks */
    unhook_all_plugin (plugin, NULL);

    /* remove all infolists */
    infolist_free_all_plugin (plugin);

    /* remove all hdata */
    hdata_free_all_plugin (plugin);

    /* remove all bar items */
    gui_bar_item_free_all_plugin (plugin);

    /* free data */
    free (plugin->filename);
    if (!weechat_plugin_no_dlclose)
        dlclose (plugin->handle);
    free (plugin->name);
    free (plugin->description);
    free (plugin->author);
    free (plugin->version);
    free (plugin->license);
    free (plugin->charset);
    hashtable_free (plugin->variables);

    free (plugin);

    weechat_plugins = new_weechat_plugins;
}

/*
 * Unloads a WeeChat plugin.
 */

void
plugin_unload (struct t_weechat_plugin *plugin)
{
    t_weechat_end_func *end_func;
    char *name;

    name = (plugin->name) ? strdup (plugin->name) : NULL;

    if (plugin->initialized)
    {
        end_func = dlsym (plugin->handle, "weechat_plugin_end");
        if (end_func)
            (void) (end_func) (plugin);
    }

    plugin_remove (plugin);

    if ((weechat_debug_core >= 1) || !plugin_quiet)
    {
        gui_chat_printf (NULL,
                         _("Plugin \"%s\" unloaded"),
                         (name) ? name : "???");
    }
    (void) hook_signal_send ("plugin_unloaded",
                             WEECHAT_HOOK_SIGNAL_STRING, name);
    free (name);
}

/*
 * Unloads a WeeChat plugin by name.
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
                         _("%sPlugin \"%s\" not found"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
    }
}

/*
 * Unloads all WeeChat plugins.
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
 * Reloads a WeeChat plugin by name.
 */

void
plugin_reload_name (const char *name, int argc, char **argv)
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
            plugin_load (filename, 1, argc, argv);
            free (filename);
        }
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sPlugin \"%s\" not found"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
    }
}

/*
 * Displays list of loaded plugins on one line.
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
 * Initializes plugin support.
 */

void
plugin_init (char *force_plugin_autoload, int argc, char *argv[])
{
    /* read plugins options on disk */
    plugin_config_init ();
    plugin_config_read ();

    /* auto-load plugins */
    plugin_quiet = 1;
    plugin_auto_load (force_plugin_autoload, 1, 1, 1, argc, argv);
    plugin_display_short_list ();
    plugin_quiet = 0;
}

/*
 * Ends plugin support.
 */

void
plugin_end ()
{
    /* write plugins configuration options */
    plugin_config_write ();

    /* unload all plugins */
    plugin_unload_all ();

    /* free all plugin options */
    plugin_config_end ();
}

/*
 * Gets hdata for plugin.
 */

struct t_hdata *
plugin_hdata_plugin_cb (const void *pointer, void *data,
                        const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_plugin", "next_plugin",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_weechat_plugin, filename, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, handle, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, description, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, author, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, version, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, license, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, charset, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, priority, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, initialized, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, debug, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, upgrading, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, variables, HASHTABLE, 0, NULL, NULL);
        HDATA_VAR(struct t_weechat_plugin, prev_plugin, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_weechat_plugin, next_plugin, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(weechat_plugins, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_weechat_plugin, 0);
    }
    return hdata;
}

/*
 * Adds a plugin in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
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
    if (!infolist_new_var_string (ptr_item, "description_nls",
                                  (plugin->description && plugin->description[0]) ?
                                  _(plugin->description) : ""))
        return 0;
    if (!infolist_new_var_string (ptr_item, "author", plugin->author))
        return 0;
    if (!infolist_new_var_string (ptr_item, "version", plugin->version))
        return 0;
    if (!infolist_new_var_string (ptr_item, "license", plugin->license))
        return 0;
    if (!infolist_new_var_string (ptr_item, "charset", plugin->charset))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "priority", plugin->priority))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "initialized", plugin->initialized))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "debug", plugin->debug))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "upgrading", plugin->upgrading))
        return 0;
    if (!hashtable_add_to_infolist (plugin->variables, ptr_item, "var"))
        return 0;

    return 1;
}

/*
 * Prints plugins in WeeChat log file (usually for crash dump).
 */

void
plugin_print_log ()
{
    struct t_weechat_plugin *ptr_plugin;

    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        log_printf ("");
        log_printf ("[plugin (addr:%p)]", ptr_plugin);
        log_printf ("  filename . . . . . . . : '%s'", ptr_plugin->filename);
        log_printf ("  handle . . . . . . . . : %p", ptr_plugin->handle);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_plugin->name);
        log_printf ("  description. . . . . . : '%s'", ptr_plugin->description);
        log_printf ("  author . . . . . . . . : '%s'", ptr_plugin->author);
        log_printf ("  version. . . . . . . . : '%s'", ptr_plugin->version);
        log_printf ("  license. . . . . . . . : '%s'", ptr_plugin->license);
        log_printf ("  charset. . . . . . . . : '%s'", ptr_plugin->charset);
        log_printf ("  priority . . . . . . . : %d", ptr_plugin->priority);
        log_printf ("  initialized. . . . . . : %d", ptr_plugin->initialized);
        log_printf ("  debug. . . . . . . . . : %d", ptr_plugin->debug);
        log_printf ("  upgrading. . . . . . . : %d", ptr_plugin->upgrading);
        hashtable_print_log (ptr_plugin->variables, "variables");
        log_printf ("  prev_plugin. . . . . . : %p", ptr_plugin->prev_plugin);
        log_printf ("  next_plugin. . . . . . : %p", ptr_plugin->next_plugin);
    }
}
