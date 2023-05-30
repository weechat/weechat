/*
 * plugin-script.c - common functions used by script plugins
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "weechat-plugin.h"
#include "plugin-script.h"
#include "plugin-script-config.h"


/*
 * Displays name and version of interpreter used.
 */

void
plugin_script_display_interpreter (struct t_weechat_plugin *weechat_plugin,
                                   int indent)
{
    const char *ptr_name, *ptr_version;

    ptr_name = weechat_hashtable_get (weechat_plugin->variables,
                                      "interpreter_name");
    ptr_version = weechat_hashtable_get (weechat_plugin->variables,
                                         "interpreter_version");
    if (ptr_name)
    {
        weechat_printf (NULL,
                        "%s%s: %s",
                        (indent) ? "  " : "",
                        ptr_name,
                        (ptr_version && ptr_version[0]) ? ptr_version : "(?)");
    }
}

/*
 * Callback for signal "debug_libs".
 */

int
plugin_script_signal_debug_libs_cb (const void *pointer, void *data,
                                    const char *signal,
                                    const char *type_data,
                                    void *signal_data)
{
    struct t_weechat_plugin *plugin;

    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    plugin = (struct t_weechat_plugin *)pointer;

    plugin_script_display_interpreter (plugin, 1);

    return WEECHAT_RC_OK;
}

/*
 * Callback for info "xxx_interpreter".
 */

char *
plugin_script_info_interpreter_cb (const void *pointer, void *data,
                                   const char *info_name,
                                   const char *arguments)
{
    struct t_weechat_plugin *weechat_plugin;
    const char *ptr_interpreter;

    /* make C compiler happy */
    (void) data;
    (void) info_name;
    (void) arguments;

    weechat_plugin = (struct t_weechat_plugin *)pointer;

    ptr_interpreter = weechat_hashtable_get (weechat_plugin->variables,
                                             "interpreter_name");
    return (ptr_interpreter) ? strdup (ptr_interpreter) : NULL;
}

/*
 * Callback for info "xxx_version".
 */

char *
plugin_script_info_version_cb (const void *pointer, void *data,
                               const char *info_name,
                               const char *arguments)
{
    struct t_weechat_plugin *weechat_plugin;
    const char *ptr_version;

    /* make C compiler happy */
    (void) data;
    (void) info_name;
    (void) arguments;

    weechat_plugin = (struct t_weechat_plugin *)pointer;

    ptr_version = weechat_hashtable_get (weechat_plugin->variables,
                                         "interpreter_version");
    return (ptr_version) ? strdup (ptr_version) : NULL;
}

/*
 * Creates directories for plugin in WeeChat data directory:
 *   - ${weechat_data_dir}/xxx
 *   - ${weechat_data_dir}/xxx/autoload
 */

void
plugin_script_create_dirs (struct t_weechat_plugin *weechat_plugin)
{
    char path[PATH_MAX];

    snprintf (path, sizeof (path),
              "${weechat_data_dir}/%s", weechat_plugin->name);
    weechat_mkdir_home (path, 0755);
    snprintf (path, sizeof (path),
              "${weechat_data_dir}/%s/autoload", weechat_plugin->name);
    weechat_mkdir_home (path, 0755);
}

/*
 * Initializes script plugin:
 *   - reads configuration
 *   - hooks config
 *   - creates directories in WeeChat home
 *   - hooks command, completion, hdata, infolist, signals
 *   - auto-loads scripts.
 */

void
plugin_script_init (struct t_weechat_plugin *weechat_plugin,
                    struct t_plugin_script_data *plugin_data)
{
    char string[512], *completion, *info_auto_load_scripts;
    char *action_signals[] = { "install", "remove", "autoload", NULL };
    int i, auto_load_scripts;

    /* initialize script configuration file (file: "<language>.conf") */
    plugin_script_config_init (weechat_plugin, plugin_data);

    /* read configuration file */
    weechat_config_read (*plugin_data->config_file);

    /* create directories in WeeChat home */
    plugin_script_create_dirs (weechat_plugin);

    /* add command */
    completion = NULL;
    snprintf (string, sizeof (string), "%%(%s_script)", weechat_plugin->name);
    completion = weechat_string_replace ("list %s"
                                         " || listfull %s"
                                         " || load %(filename)"
                                         " || autoload"
                                         " || reload %s"
                                         " || unload %s"
                                         " || eval"
                                         " || version",
                                         "%s",
                                         string);
    weechat_hook_command (
        weechat_plugin->name,
        N_("list/load/unload scripts"),
        N_("list|listfull [<name>]"
           " || load [-q] <filename>"
           " || autoload"
           " || reload|unload [-q] [<name>]"
           " || eval [-o|-oc] <code>"
           " || version"),
        N_("    list: list loaded scripts\n"
           "listfull: list loaded scripts (verbose)\n"
           "    load: load a script\n"
           "autoload: load all scripts in \"autoload\" directory\n"
           "  reload: reload a script (if no name given, unload all scripts, "
           "then load all scripts in \"autoload\" directory)\n"
           "  unload: unload a script (if no name given, unload all scripts)\n"
           "filename: script (file) to load\n"
           "      -q: quiet mode: do not display messages\n"
           "    name: a script name (name used in call to \"register\" "
           "function)\n"
           "    eval: evaluate source code and display result on current "
           "buffer\n"
           "      -o: send evaluation result to the buffer without executing "
           "commands\n"
           "     -oc: send evaluation result to the buffer and execute "
           "commands\n"
           "    code: source code to evaluate\n"
           " version: display the version of interpreter used\n"
           "\n"
           "Without argument, this command lists all loaded scripts."),
        completion,
        plugin_data->callback_command, NULL, NULL);
    if (completion)
        free (completion);

    /* add completion, hdata and infolist */
    snprintf (string, sizeof (string), "%s_script", weechat_plugin->name);
    weechat_hook_completion (string, N_("list of scripts"),
                             plugin_data->callback_completion, NULL, NULL);
    weechat_hook_hdata (string, N_("list of scripts"),
                        plugin_data->callback_hdata, weechat_plugin, NULL);
    weechat_hook_infolist (string, N_("list of scripts"),
                           N_("script pointer (optional)"),
                           N_("script name (wildcard \"*\" is allowed) "
                              "(optional)"),
                           plugin_data->callback_infolist, NULL, NULL);
    snprintf (string, sizeof (string), "%s_eval", weechat_plugin->name);
    weechat_hook_info (string, N_("evaluation of source code"),
                       N_("source code to execute"),
                       plugin_data->callback_info_eval, NULL, NULL);

    /* add signal for "debug_dump" */
    weechat_hook_signal ("debug_dump",
                         plugin_data->callback_signal_debug_dump, NULL, NULL);

    /* add signal for "debug_libs" */
    weechat_hook_signal ("debug_libs",
                         plugin_script_signal_debug_libs_cb,
                         weechat_plugin, NULL);

    /* add signals for script actions (install/remove/autoload) */
    for (i = 0; action_signals[i]; i++)
    {
        snprintf (string, sizeof (string),
                  "%s_script_%s", weechat_plugin->name, action_signals[i]);
        weechat_hook_signal (
            string,
            plugin_data->callback_signal_script_action, NULL, NULL);
    }

    /* add infos */
    snprintf (string, sizeof (string), "%s_interpreter", weechat_plugin->name);
    weechat_hook_info (string, N_("name of the interpreter used"), NULL,
                       &plugin_script_info_interpreter_cb,
                       weechat_plugin, NULL);
    snprintf (string, sizeof (string), "%s_version", weechat_plugin->name);
    weechat_hook_info (string, N_("version of the interpreter used"), NULL,
                       &plugin_script_info_version_cb,
                       weechat_plugin, NULL);

    /* check if auto-load of scripts is enabled */
    info_auto_load_scripts = weechat_info_get ("auto_load_scripts", NULL);
    auto_load_scripts = (info_auto_load_scripts
                         && (strcmp (info_auto_load_scripts, "1") == 0)) ?
        1 : 0;
    if (info_auto_load_scripts)
        free (info_auto_load_scripts);

    /* autoload scripts */
    if (auto_load_scripts)
    {
        plugin_script_auto_load (weechat_plugin,
                                 plugin_data->callback_load_file);
    }
}

/*
 * Checks if a script pointer is valid.
 *
 * Returns:
 *   1: script exists
 *   0: script is not found
 */

int
plugin_script_valid (struct t_plugin_script *scripts,
                     struct t_plugin_script *script)
{
    struct t_plugin_script *ptr_script;

    if (!scripts || !script)
        return 0;

    for (ptr_script = scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (ptr_script == script)
            return 1;
    }

    /* script not found */
    return 0;
}

/*
 * Converts a pointer to a string for usage in a script.
 *
 * Returns string with format "0x12345678".
 */

const char *
plugin_script_ptr2str (void *pointer)
{
    static char str_pointer[32][32];
    static int index_pointer = 0;

    index_pointer = (index_pointer + 1) % 32;
    str_pointer[index_pointer][0] = '\0';

    if (!pointer)
        return str_pointer[index_pointer];

    snprintf (str_pointer[index_pointer], sizeof (str_pointer[index_pointer]),
              "0x%lx", (unsigned long)pointer);

    return str_pointer[index_pointer];
}

/*
 * Converts a string to pointer for usage outside a script.
 *
 * Format of "str_pointer" is "0x12345678".
 */

void *
plugin_script_str2ptr (struct t_weechat_plugin *weechat_plugin,
                       const char *script_name, const char *function_name,
                       const char *str_pointer)
{
    unsigned long value;
    int rc;
    struct t_gui_buffer *ptr_buffer;

    if (!str_pointer || !str_pointer[0])
        return NULL;

    if ((str_pointer[0] != '0') || (str_pointer[1] != 'x'))
        goto invalid;

    rc = sscanf (str_pointer + 2, "%lx", &value);
    if ((rc != EOF) && (rc >= 1))
        return (void *)value;

invalid:
    if ((weechat_plugin->debug >= 1) && script_name && function_name)
    {
        ptr_buffer = weechat_buffer_search_main ();
        if (ptr_buffer)
        {
            weechat_buffer_set (ptr_buffer, "print_hooks_enabled", "0");
            weechat_printf (NULL,
                            _("%s%s: warning, invalid pointer (\"%s\") for "
                              "function \"%s\" (script: %s)"),
                            weechat_prefix ("error"), weechat_plugin->name,
                            str_pointer, function_name, script_name);
            weechat_buffer_set (ptr_buffer, "print_hooks_enabled", "1");
        }
    }
    return NULL;
}

/*
 * Builds concatenated function name and data (both are strings).
 * The result will be sent to callbacks.
 */

char *
plugin_script_build_function_and_data (const char *function, const char *data)
{
    int length_function, length_data, length;
    char *result;

    if (!function || !function[0])
        return NULL;

    length_function = (function) ? strlen (function) : 0;
    length_data = (data) ? strlen (data) : 0;
    length = length_function + 1 + length_data + 1;

    result = malloc (length);
    if (!result)
        return NULL;

    if (function)
        memcpy (result, function, length_function + 1);
    else
        result[0] = '\0';

    if (data)
        memcpy (result + length_function + 1, data, length_data + 1);
    else
        result[length_function + 1] = '\0';

    return result;
}

/*
 * Gets pointer on function name and data from a callback data pointer
 * (which contains 2 strings separated by '\0').
 */

void
plugin_script_get_function_and_data (void *callback_data,
                                     const char **function, const char **data)
{
    const char *string, *ptr_data;

    string = (const char *)callback_data;

    if (string)
    {
        *function = string;
        ptr_data = string + strlen (string) + 1;
        *data = (ptr_data[0]) ? ptr_data : NULL;
    }
    else
    {
        *function = NULL;
        *data = NULL;
    }
}

/*
 * Auto-loads all scripts in a directory.
 */

void
plugin_script_auto_load (struct t_weechat_plugin *weechat_plugin,
                         void (*callback)(void *data,
                                          const char *filename))
{
    char *weechat_data_dir, *dir_name;
    int dir_length;

    /* build directory, adding WeeChat data directory */
    weechat_data_dir = weechat_info_get ("weechat_data_dir", "");
    if (!weechat_data_dir)
        return;
    dir_length = strlen (weechat_data_dir) + strlen (weechat_plugin->name) + 16;
    dir_name = malloc (dir_length);
    if (!dir_name)
    {
        free (weechat_data_dir);
        return;
    }

    snprintf (dir_name, dir_length,
              "%s/%s/autoload", weechat_data_dir, weechat_plugin->name);
    weechat_exec_on_files (dir_name, 0, 0, callback, NULL);

    free (weechat_data_dir);
    free (dir_name);
}

/*
 * Searches for a script by registered name.
 *
 * Returns pointer to script, NULL if not found.
 */

struct t_plugin_script *
plugin_script_search (struct t_plugin_script *scripts, const char *name)
{
    struct t_plugin_script *ptr_script;

    if (!name)
        return NULL;

    for (ptr_script = scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (strcmp (ptr_script->name, name) == 0)
            return ptr_script;
    }

    /* script not found */
    return NULL;
}

/*
 * Searches for a script by full name (example: "go.py").
 *
 * Returns pointer to script, NULL if not found.
 */

struct t_plugin_script *
plugin_script_search_by_full_name (struct t_plugin_script *scripts,
                                   const char *full_name)
{
    char *base_name;
    struct t_plugin_script *ptr_script;

    if (!full_name)
        return NULL;

    for (ptr_script = scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        base_name = basename (ptr_script->filename);
        if (strcmp (base_name, full_name) == 0)
            return ptr_script;
    }

    /* script not found */
    return NULL;
}

/*
 * Searches for path name of a script.
 *
 * Note: result must be freed after use.
 */

char *
plugin_script_search_path (struct t_weechat_plugin *weechat_plugin,
                           const char *filename)
{
    char *final_name, *weechat_data_dir, *dir_system;
    int length;
    struct stat st;

    if (!filename)
        return NULL;

    if (filename[0] == '~')
        return weechat_string_expand_home (filename);

    weechat_data_dir = weechat_info_get ("weechat_data_dir", "");
    if (weechat_data_dir)
    {
        /* try WeeChat user's autoload dir */
        length = strlen (weechat_data_dir) + strlen (weechat_plugin->name) + 8 +
            strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s/autoload/%s",
                      weechat_data_dir, weechat_plugin->name, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
            {
                free (weechat_data_dir);
                return final_name;
            }
            free (final_name);
        }

        /* try WeeChat language user's dir */
        length = strlen (weechat_data_dir) + strlen (weechat_plugin->name) +
            strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s/%s", weechat_data_dir, weechat_plugin->name, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
            {
                free (weechat_data_dir);
                return final_name;
            }
            free (final_name);
        }

        /* try WeeChat user's dir */
        length = strlen (weechat_data_dir) + strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s", weechat_data_dir, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
            {
                free (weechat_data_dir);
                return final_name;
            }
            free (final_name);
        }
        free (weechat_data_dir);
    }

    /* try WeeChat system dir */
    dir_system = weechat_info_get ("weechat_sharedir", "");
    if (dir_system)
    {
        length = strlen (dir_system) + strlen (weechat_plugin->name) +
            strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name,length,
                      "%s/%s/%s", dir_system, weechat_plugin->name, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
            {
                free (dir_system);
                return final_name;
            }
            free (final_name);
        }
        free (dir_system);
    }

    return strdup (filename);
}

/*
 * Searches for position of script in list (to keep list sorted on name).
 */

struct t_plugin_script *
plugin_script_find_pos (struct t_weechat_plugin *weechat_plugin,
                        struct t_plugin_script *scripts,
                        struct t_plugin_script *script)
{
    struct t_plugin_script *ptr_script;

    for (ptr_script = scripts; ptr_script; ptr_script = ptr_script->next_script)
    {
        if (weechat_strcmp (script->name, ptr_script->name) < 0)
            return ptr_script;
    }
    return NULL;
}

/*
 * Inserts a script in list (keeping list sorted on name).
 */

void
plugin_script_insert_sorted (struct t_weechat_plugin *weechat_plugin,
                             struct t_plugin_script **scripts,
                             struct t_plugin_script **last_script,
                             struct t_plugin_script *script)
{
    struct t_plugin_script *pos_script;

    if (*scripts)
    {
        pos_script = plugin_script_find_pos (weechat_plugin, *scripts, script);

        if (pos_script)
        {
            /* insert script into the list (before script found) */
            script->prev_script = pos_script->prev_script;
            script->next_script = pos_script;
            if (pos_script->prev_script)
                (pos_script->prev_script)->next_script = script;
            else
                *scripts = script;
            pos_script->prev_script = script;
        }
        else
        {
            /* add script to the end */
            script->prev_script = *last_script;
            script->next_script = NULL;
            (*last_script)->next_script = script;
            *last_script = script;
        }
    }
    else
    {
        /* first script in list */
        script->prev_script = NULL;
        script->next_script = NULL;
        *scripts = script;
        *last_script = script;
    }
}

/*
 * Allocates a new script.
 *
 * Returns pointer to new script, NULL if error.
 */

struct t_plugin_script *
plugin_script_alloc (const char *filename, const char *name,
                     const char *author, const char *version,
                     const char *license, const char *description,
                     const char *shutdown_func, const char *charset)
{
    struct t_plugin_script *new_script;

    new_script = malloc (sizeof (*new_script));
    if (!new_script)
        return NULL;

    new_script->filename = strdup (filename);
    new_script->interpreter = NULL;
    new_script->name = strdup (name);
    new_script->author = strdup (author);
    new_script->version = strdup (version);
    new_script->license = strdup (license);
    new_script->description = strdup (description);
    new_script->shutdown_func = (shutdown_func) ?
        strdup (shutdown_func) : NULL;
    new_script->charset = (charset) ? strdup (charset) : NULL;
    new_script->unloading = 0;
    new_script->prev_script = NULL;
    new_script->next_script = NULL;

    return new_script;
}

/*
 * Adds a script to list of scripts.
 *
 * Returns pointer to new script, NULL if error.
 */

struct t_plugin_script *
plugin_script_add (struct t_weechat_plugin *weechat_plugin,
                   struct t_plugin_script_data *plugin_data,
                   const char *filename, const char *name, const char *author,
                   const char *version, const char *license,
                   const char *description, const char *shutdown_func,
                   const char *charset)
{
    struct t_plugin_script *new_script;

    if (!name[0] || strchr (name, ' '))
    {
        weechat_printf (NULL,
                        _("%s: error loading script \"%s\" (spaces or empty name "
                          "not allowed)"),
                        weechat_plugin->name, name);
        return NULL;
    }

    if (weechat_config_boolean (*(plugin_data->config_look_check_license))
        && (weechat_strcmp_ignore_chars (weechat_plugin->license, license,
                                         "0123456789-.,/\\()[]{}", 0) != 0))
    {
        weechat_printf (NULL,
                        _("%s%s: warning, license \"%s\" for script \"%s\" "
                          "differs from plugin license (\"%s\")"),
                        weechat_prefix ("error"), weechat_plugin->name,
                        license, name, weechat_plugin->license);
    }

    new_script = plugin_script_alloc (filename, name, author, version, license,
                                      description, shutdown_func, charset);
    if (!new_script)
    {
        weechat_printf (NULL,
                        _("%s: error loading script \"%s\" "
                          "(not enough memory)"),
                        weechat_plugin->name, name);
        return NULL;
    }

    /* add script to the list (except the internal "eval" fake script) */
    if (strcmp (new_script->name, WEECHAT_SCRIPT_EVAL_NAME) != 0)
    {
        plugin_script_insert_sorted (weechat_plugin,
                                     plugin_data->scripts,
                                     plugin_data->last_script,
                                     new_script);
    }

    return new_script;
}

/*
 * Restores buffers callbacks (input and close) for buffers created by script
 * plugin.
 */

void
plugin_script_set_buffer_callbacks (struct t_weechat_plugin *weechat_plugin,
                                    struct t_plugin_script *scripts,
                                    struct t_plugin_script *script,
                                    int (*callback_buffer_input) (const void *pointer,
                                                                  void *data,
                                                                  struct t_gui_buffer *buffer,
                                                                  const char *input_data),
                                    int (*callback_buffer_close) (const void *pointer,
                                                                  void *data,
                                                                  struct t_gui_buffer *buffer))
{
    struct t_infolist *infolist;
    struct t_gui_buffer *ptr_buffer;
    const char *script_name;
    const char *str_script_input_cb, *str_script_input_cb_data;
    const char *str_script_close_cb, *str_script_close_cb_data;
    char *function_and_data;
    struct t_plugin_script *ptr_script;

    infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            if (weechat_infolist_pointer (infolist, "plugin") == weechat_plugin)
            {
                ptr_buffer = weechat_infolist_pointer (infolist, "pointer");
                script_name = weechat_buffer_get_string (ptr_buffer, "localvar_script_name");
                if (script_name && script_name[0])
                {
                    ptr_script = plugin_script_search (scripts, script_name);
                    if (ptr_script && (ptr_script == script))
                    {
                        str_script_input_cb = weechat_buffer_get_string (
                            ptr_buffer, "localvar_script_input_cb");
                        str_script_input_cb_data = weechat_buffer_get_string (
                            ptr_buffer, "localvar_script_input_cb_data");
                        str_script_close_cb = weechat_buffer_get_string (
                            ptr_buffer, "localvar_script_close_cb");
                        str_script_close_cb_data = weechat_buffer_get_string (
                            ptr_buffer, "localvar_script_close_cb_data");

                        function_and_data = plugin_script_build_function_and_data (
                            str_script_input_cb, str_script_input_cb_data);
                        if (function_and_data)
                        {
                            weechat_buffer_set_pointer (
                                ptr_buffer,
                                "input_callback",
                                callback_buffer_input);
                            weechat_buffer_set_pointer (
                                ptr_buffer,
                                "input_callback_pointer",
                                ptr_script);
                            weechat_buffer_set_pointer (
                                ptr_buffer,
                                "input_callback_data",
                                function_and_data);
                        }

                        function_and_data = plugin_script_build_function_and_data (
                            str_script_close_cb, str_script_close_cb_data);
                        if (function_and_data)
                        {
                            weechat_buffer_set_pointer (
                                ptr_buffer,
                                "close_callback",
                                callback_buffer_close);
                            weechat_buffer_set_pointer (
                                ptr_buffer,
                                "close_callback_pointer",
                                ptr_script);
                            weechat_buffer_set_pointer (
                                ptr_buffer,
                                "close_callback_data",
                                function_and_data);
                        }
                    }
                }
            }
        }
        weechat_infolist_free (infolist);
    }
}

/*
 * Closes all buffers created by the script.
 */

void
plugin_script_close_buffers (struct t_weechat_plugin *weechat_plugin,
                             struct t_plugin_script *script)
{
    struct t_hdata *hdata;
    struct t_gui_buffer *ptr_buffer, *ptr_buffer_to_close;
    const char *ptr_script_name;

    hdata = weechat_hdata_get ("buffer");
    while (1)
    {
        ptr_buffer = weechat_hdata_get_list (hdata, "gui_buffers");
        ptr_buffer_to_close = NULL;
        while (ptr_buffer)
        {
            ptr_script_name = weechat_buffer_get_string (
                ptr_buffer, "localvar_script_name");
            if (ptr_script_name
                && (strcmp (ptr_script_name, script->name) == 0))
            {
                ptr_buffer_to_close = ptr_buffer;
                break;
            }
            ptr_buffer = weechat_hdata_move (hdata, ptr_buffer, 1);
        }
        if (ptr_buffer_to_close)
        {
            weechat_buffer_close (ptr_buffer_to_close);
        }
        else
            break;
    }
}

/*
 * Removes all bar items created by the script.
 */

void
plugin_script_remove_bar_items (struct t_weechat_plugin *weechat_plugin,
                                struct t_plugin_script *script)
{
    struct t_hdata *hdata;
    struct t_gui_bar_item *ptr_bar_item, *ptr_next_item;
    void *callback_pointer;

    hdata = weechat_hdata_get ("bar_item");
    ptr_bar_item = weechat_hdata_get_list (hdata, "gui_bar_items");
    while (ptr_bar_item)
    {
        ptr_next_item = weechat_hdata_pointer (hdata, ptr_bar_item,
                                               "next_item");
        callback_pointer = weechat_hdata_pointer (hdata, ptr_bar_item,
                                                  "build_callback_pointer");
        if (callback_pointer == script)
            weechat_bar_item_remove (ptr_bar_item);
        ptr_bar_item = ptr_next_item;
    }
}

/*
 * Removes all configuration files/sections/options created by the script.
 */

void
plugin_script_remove_configs (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *script)
{
    struct t_hdata *hdata_config, *hdata_section, *hdata_option;
    struct t_config_file *ptr_config, *ptr_next_config;
    struct t_config_section *ptr_section, *ptr_next_section;
    struct t_config_option *ptr_option, *ptr_next_option;
    void *callback_pointer;

    hdata_config = weechat_hdata_get ("config_file");
    hdata_section = weechat_hdata_get ("config_section");
    hdata_option = weechat_hdata_get ("config_option");
    ptr_config = weechat_hdata_get_list (hdata_config, "config_files");
    while (ptr_config)
    {
        ptr_next_config = weechat_hdata_pointer (hdata_config, ptr_config,
                                                 "next_config");
        callback_pointer = weechat_hdata_pointer (
            hdata_config, ptr_config, "callback_reload_pointer");
        if (callback_pointer == script)
        {
            if (weechat_config_boolean (weechat_config_get ("weechat.plugin.save_config_on_unload")))
                weechat_config_write (ptr_config);
            weechat_config_free (ptr_config);
        }
        else
        {
            ptr_section = weechat_hdata_pointer (hdata_config, ptr_config,
                                                 "sections");
            while (ptr_section)
            {
                ptr_next_section = weechat_hdata_pointer (hdata_section,
                                                          ptr_section,
                                                          "next_section");
                callback_pointer = weechat_hdata_pointer (
                    hdata_section, ptr_section, "callback_read_pointer");
                if (callback_pointer == script)
                {
                    weechat_config_section_free (ptr_section);
                }
                else
                {
                    ptr_option = weechat_hdata_pointer (hdata_section,
                                                        ptr_section,
                                                        "options");
                    while (ptr_option)
                    {
                        ptr_next_option = weechat_hdata_pointer (hdata_option,
                                                                 ptr_option,
                                                                 "next_option");
                        callback_pointer = weechat_hdata_pointer (
                            hdata_option, ptr_option,
                            "callback_check_value_pointer");
                        if (callback_pointer == script)
                            weechat_config_option_free (ptr_option);
                        ptr_option = ptr_next_option;
                    }
                }
                ptr_section = ptr_next_section;
            }
        }
        ptr_config = ptr_next_config;
    }
}

/*
 * Frees a script.
 */

void
plugin_script_free (struct t_plugin_script *script)
{
    if (script->filename)
        free (script->filename);
    if (script->name)
        free (script->name);
    if (script->author)
        free (script->author);
    if (script->version)
        free (script->version);
    if (script->license)
        free (script->license);
    if (script->description)
        free (script->description);
    if (script->shutdown_func)
        free (script->shutdown_func);
    if (script->charset)
        free (script->charset);

    free (script);
}

/*
 * Removes a script from list of scripts.
 */

void
plugin_script_remove (struct t_weechat_plugin *weechat_plugin,
                      struct t_plugin_script **scripts,
                      struct t_plugin_script **last_script,
                      struct t_plugin_script *script)
{
    script->unloading = 1;

    plugin_script_close_buffers (weechat_plugin, script);

    plugin_script_remove_bar_items (weechat_plugin, script);

    plugin_script_remove_configs (weechat_plugin, script);

    /* remove all hooks created by this script */
    weechat_unhook_all (script->name);

    /* remove script from list */
    if (script->prev_script)
        (script->prev_script)->next_script = script->next_script;
    if (script->next_script)
        (script->next_script)->prev_script = script->prev_script;
    if (*scripts == script)
        *scripts = script->next_script;
    if (*last_script == script)
        *last_script = script->prev_script;

    /* free data and script */
    plugin_script_free (script);
}

/*
 * Adds list of scripts to completion list.
 */

void
plugin_script_completion (struct t_weechat_plugin *weechat_plugin,
                          struct t_gui_completion *completion,
                          struct t_plugin_script *scripts)
{
    struct t_plugin_script *ptr_script;

    for (ptr_script = scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        weechat_completion_list_add (completion, ptr_script->name,
                                     0, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * Adds script name for a plugin action.
 */

void
plugin_script_action_add (char **action_list, const char *name)
{
    int length;
    char *action_list2;

    length = strlen (name);

    if (!(*action_list))
    {
        *action_list = malloc (length + 1);
        if (*action_list)
            strcpy (*action_list, name);
    }
    else
    {
        action_list2 = realloc (*action_list,
                                strlen (*action_list) + 1 + length + 1);
        if (!action_list2)
        {
            free (*action_list);
            *action_list = NULL;
            return;
        }
        *action_list = action_list2;
        strcat (*action_list, ",");
        strcat (*action_list, name);
    }
}

/*
 * Removes script file(s) from disk.
 *
 * Returns:
 *   1: script was removed
 *   0: script was not removed (not found)
 */

int
plugin_script_remove_file (struct t_weechat_plugin *weechat_plugin,
                           const char *name,
                           int quiet,
                           int display_error_if_no_script_removed)
{
    int script_removed, num_found, i;
    char *path_script;

    script_removed = 0;
    num_found = 0;
    i = 0;
    while (i < 2)
    {
        path_script = plugin_script_search_path (weechat_plugin, name);
        /*
         * script not found? (if path_script == name, that means the function
         * above did not find the script)
         */
        if (!path_script || (strcmp (path_script, name) == 0))
        {
            if (path_script)
                free (path_script);
            break;
        }
        num_found++;
        if (unlink (path_script) == 0)
        {
            script_removed = 1;
            if (!quiet)
            {
                weechat_printf (NULL, _("%s: script removed: %s"),
                                weechat_plugin->name,
                                path_script);
            }
        }
        else
        {
            weechat_printf (NULL,
                            _("%s%s: failed to remove script: %s "
                              "(%s)"),
                            weechat_prefix ("error"),
                            weechat_plugin->name,
                            path_script,
                            strerror (errno));
            break;
        }
        free (path_script);
        i++;
    }

    if ((num_found == 0) && display_error_if_no_script_removed)
    {
        weechat_printf (NULL,
                        _("%s: script \"%s\" not found, nothing "
                          "was removed"),
                        weechat_plugin->name,
                        name);
    }

    return script_removed;
}

/*
 * Installs some scripts (using comma separated list).
 *
 * This function does following tasks:
 *   1. unloads script (if script is loaded)
 *   2. removes script file(s)
 *   3. moves script file from "install" dir to language dir
 *   4. makes link in autoload dir (if option "-a" is given)
 *   5. loads script (if it was loaded).
 */

void
plugin_script_action_install (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *scripts,
                              void (*script_unload)(struct t_plugin_script *script),
                              struct t_plugin_script *(*script_load)(const char *filename,
                                                                     const char *code),
                              int *quiet,
                              char **list)
{
    char **argv, *name, *ptr_base_name, *base_name, *new_path, *autoload_path;
    char *symlink_path, str_signal[128], *ptr_list, *weechat_data_dir, *dir_separator;
    int argc, i, length, rc, autoload, existing_script, script_loaded;
    struct t_plugin_script *ptr_script;

    if (!*list)
        return;

    /* create again directories, just in case they have been removed */
    plugin_script_create_dirs (weechat_plugin);

    ptr_list = *list;
    autoload = 0;
    *quiet = 0;

    while ((ptr_list[0] == ' ') || (ptr_list[0] == '-'))
    {
        if (ptr_list[0] == ' ')
            ptr_list++;
        else
        {
            switch (ptr_list[1])
            {
                case 'a': /* autoload */
                    autoload = 1;
                    break;
                case 'q': /* quiet mode */
                    *quiet = 1;
                    break;
            }
            ptr_list += 2;
        }
    }

    argv = weechat_string_split (ptr_list, ",", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    if (argv)
    {
        for (i = 0; i < argc; i++)
        {
            name = strdup (argv[i]);
            if (name)
            {
                ptr_base_name = basename (name);
                base_name = strdup (ptr_base_name);
                if (base_name)
                {
                    /* unload script, if script is loaded */
                    script_loaded = 0;
                    ptr_script = plugin_script_search_by_full_name (scripts,
                                                                    base_name);
                    if (ptr_script)
                    {
                        script_loaded = 1;
                        (*script_unload) (ptr_script);
                    }

                    /* remove script file(s) */
                    existing_script = plugin_script_remove_file (weechat_plugin,
                                                                 base_name,
                                                                 *quiet, 0);

                    /* move file from install dir to language dir */
                    weechat_data_dir = weechat_info_get ("weechat_data_dir", "");
                    length = strlen (weechat_data_dir) + strlen (weechat_plugin->name) +
                        strlen (base_name) + 16;
                    new_path = malloc (length);
                    if (new_path)
                    {
                        snprintf (new_path, length, "%s/%s/%s",
                                  weechat_data_dir, weechat_plugin->name, base_name);
                        if (weechat_file_copy (name, new_path))
                        {
                            /* remove old file */
                            (void) remove (name);

                            /* make link in autoload dir */
                            if (autoload)
                            {
                                length = strlen (weechat_data_dir) +
                                    strlen (weechat_plugin->name) + 8 +
                                    strlen (base_name) + 16;
                                autoload_path = malloc (length);
                                if (autoload_path)
                                {
                                    snprintf (autoload_path, length,
                                              "%s/%s/autoload/%s",
                                              weechat_data_dir, weechat_plugin->name,
                                              base_name);
                                    dir_separator = weechat_info_get ("dir_separator", "");
                                    length = 2 + strlen (dir_separator) +
                                        strlen (base_name) + 1;
                                    symlink_path = malloc (length);
                                    if (symlink_path)
                                    {
                                        snprintf (symlink_path, length, "..%s%s",
                                                  dir_separator, base_name);
                                        rc = symlink (symlink_path, autoload_path);
                                        (void) rc;
                                        free (symlink_path);
                                    }
                                    free (autoload_path);
                                    if (dir_separator)
                                        free (dir_separator);
                                }
                            }

                            /*
                             * load script if one of these conditions is
                             * satisfied:
                             * - new script and autoload is asked
                             * - script was loaded
                             */
                            if ((!existing_script && autoload) || script_loaded)
                                (*script_load) (new_path, NULL);
                        }
                        else
                        {
                            weechat_printf (NULL,
                                            _("%s%s: failed to move script %s "
                                              "to %s (%s)"),
                                            weechat_prefix ("error"),
                                            weechat_plugin->name,
                                            name,
                                            new_path,
                                            strerror (errno));
                        }
                        free (new_path);
                    }
                    free (base_name);
                    if (weechat_data_dir)
                        free (weechat_data_dir);
                }
                free (name);
            }
        }
        weechat_string_free_split (argv);
    }

    *quiet = 0;

    snprintf (str_signal, sizeof (str_signal),
              "%s_script_installed", weechat_plugin->name);
    (void) weechat_hook_signal_send (str_signal, WEECHAT_HOOK_SIGNAL_STRING,
                                     ptr_list);

    free (*list);
    *list = NULL;
}

/*
 * Removes some scripts (using comma separated list).
 *
 * This function does following tasks:
 *   1. unloads script (if script is loaded)
 *   2. removes script file(s).
 */

void
plugin_script_action_remove (struct t_weechat_plugin *weechat_plugin,
                             struct t_plugin_script *scripts,
                             void (*script_unload)(struct t_plugin_script *script),
                             int *quiet,
                             char **list)
{
    char **argv, str_signal[128], *ptr_list;
    int argc, i;
    struct t_plugin_script *ptr_script;

    if (!*list)
        return;

    /* create again directories, just in case they have been removed */
    plugin_script_create_dirs (weechat_plugin);

    ptr_list = *list;
    *quiet = 0;
    if (strncmp (ptr_list, "-q ", 3) == 0)
    {
        *quiet = 1;
        ptr_list += 3;
    }

    argv = weechat_string_split (ptr_list, ",", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    if (argv)
    {
        for (i = 0; i < argc; i++)
        {
            /* unload script, if script is loaded */
            ptr_script = plugin_script_search_by_full_name (scripts, argv[i]);
            if (ptr_script)
                (*script_unload) (ptr_script);

            /* remove script file(s) */
            (void) plugin_script_remove_file (weechat_plugin, argv[i],
                                              *quiet, 1);
        }
        weechat_string_free_split (argv);
    }

    *quiet = 0;

    snprintf (str_signal, sizeof (str_signal),
              "%s_script_removed", weechat_plugin->name);
    (void) weechat_hook_signal_send (str_signal, WEECHAT_HOOK_SIGNAL_STRING,
                                     ptr_list);

    free (*list);
    *list = NULL;
}

/*
 * Enables/disables autoload for some scripts (using comma separated list).
 */

void
plugin_script_action_autoload (struct t_weechat_plugin *weechat_plugin,
                               int *quiet,
                               char **list)
{
    char **argv, *name, *ptr_base_name, *base_name, *autoload_path;
    char *symlink_path, *ptr_list, *weechat_data_dir, *dir_separator;
    int argc, i, length, rc, autoload;

    if (!*list)
        return;

    /* create again directories, just in case they have been removed */
    plugin_script_create_dirs (weechat_plugin);

    ptr_list = *list;
    autoload = 0;
    *quiet = 0;

    while ((ptr_list[0] == ' ') || (ptr_list[0] == '-'))
    {
        if (ptr_list[0] == ' ')
            ptr_list++;
        else
        {
            switch (ptr_list[1])
            {
                case 'a': /* no autoload */
                    autoload = 1;
                    break;
                case 'q': /* quiet mode */
                    *quiet = 1;
                    break;
            }
            ptr_list += 2;
        }
    }

    argv = weechat_string_split (ptr_list, ",", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    if (argv)
    {
        for (i = 0; i < argc; i++)
        {
            name = strdup (argv[i]);
            if (name)
            {
                ptr_base_name = basename (name);
                base_name = strdup (ptr_base_name);
                if (base_name)
                {
                    weechat_data_dir = weechat_info_get ("weechat_data_dir", "");
                    length = strlen (weechat_data_dir) +
                        strlen (weechat_plugin->name) + 8 +
                        strlen (base_name) + 16;
                    autoload_path = malloc (length);
                    if (autoload_path)
                    {
                        snprintf (autoload_path, length,
                                  "%s/%s/autoload/%s",
                                  weechat_data_dir, weechat_plugin->name,
                                  base_name);
                        if (autoload)
                        {
                            dir_separator = weechat_info_get ("dir_separator", "");
                            length = 2 + strlen (dir_separator) +
                                strlen (base_name) + 1;
                            symlink_path = malloc (length);
                            if (symlink_path)
                            {
                                snprintf (symlink_path, length, "..%s%s",
                                          dir_separator, base_name);
                                rc = symlink (symlink_path, autoload_path);
                                (void) rc;
                                free (symlink_path);
                            }
                            if (dir_separator)
                                free (dir_separator);
                        }
                        else
                        {
                            unlink (autoload_path);
                        }
                        free (autoload_path);
                    }
                    free (base_name);
                    if (weechat_data_dir)
                        free (weechat_data_dir);
                }
                free (name);
            }
        }
        weechat_string_free_split (argv);
    }

    *quiet = 0;

    free (*list);
    *list = NULL;
}

/*
 * Displays list of scripts.
 */

void
plugin_script_display_list (struct t_weechat_plugin *weechat_plugin,
                            struct t_plugin_script *scripts,
                            const char *name, int full)
{
    struct t_plugin_script *ptr_script;

    weechat_printf (NULL, "");
    weechat_printf (NULL,
                    /* TRANSLATORS: "%s" is language (for example "perl") */
                    _("%s scripts loaded:"),
                    weechat_plugin->name);
    if (scripts)
    {
        for (ptr_script = scripts; ptr_script;
             ptr_script = ptr_script->next_script)
        {
            if (!name || (weechat_strcasestr (ptr_script->name, name)))
            {
                weechat_printf (NULL,
                                "  %s%s%s v%s - %s",
                                weechat_color ("chat_buffer"),
                                ptr_script->name,
                                weechat_color ("chat"),
                                ptr_script->version,
                                ptr_script->description);
                if (full)
                {
                    weechat_printf (NULL,
                                    _("    file: %s"),
                                    ptr_script->filename);
                    weechat_printf (NULL,
                                    _("    written by \"%s\", license: %s"),
                                    ptr_script->author,
                                    ptr_script->license);
                }
            }
        }
    }
    else
        weechat_printf (NULL, _("  (none)"));
}

/*
 * Displays list of scripts on one line.
 */

void
plugin_script_display_short_list (struct t_weechat_plugin *weechat_plugin,
                                  struct t_plugin_script *scripts)
{
    const char *scripts_loaded;
    char *buf;
    int length;
    struct t_plugin_script *ptr_script;

    if (scripts)
    {
        /* TRANSLATORS: "%s" is language (for example "perl") */
        scripts_loaded = _("%s scripts loaded:");

        length = strlen (scripts_loaded) + strlen (weechat_plugin->name) + 1;

        for (ptr_script = scripts; ptr_script;
             ptr_script = ptr_script->next_script)
        {
            length += strlen (ptr_script->name) + 2;
        }
        length++;

        buf = malloc (length);
        if (buf)
        {
            snprintf (buf, length, scripts_loaded, weechat_plugin->name);
            strcat (buf, " ");
            for (ptr_script = scripts; ptr_script;
                 ptr_script = ptr_script->next_script)
            {
                strcat (buf, ptr_script->name);
                if (ptr_script->next_script)
                    strcat (buf, ", ");
            }
            weechat_printf (NULL, "%s", buf);
            free (buf);
        }
    }
}

/*
 * Gets hdata for script.
 */

struct t_hdata *
plugin_script_hdata_script (struct t_weechat_plugin *weechat_plugin,
                            struct t_plugin_script **scripts,
                            struct t_plugin_script **last_script,
                            const char *hdata_name)
{
    struct t_hdata *hdata;
    char str_hdata_callback[128];

    hdata = weechat_hdata_new (hdata_name, "prev_script", "next_script",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        snprintf (str_hdata_callback, sizeof (str_hdata_callback),
                  "%s_callback", weechat_plugin->name);
        WEECHAT_HDATA_VAR(struct t_plugin_script, filename, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, interpreter, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, author, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, version, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, license, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, description, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, shutdown_func, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, charset, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, unloading, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, prev_script, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_plugin_script, next_script, POINTER, 0, NULL, hdata_name);
        weechat_hdata_new_list (hdata, "scripts", scripts,
                                WEECHAT_HDATA_LIST_CHECK_POINTERS);
        weechat_hdata_new_list (hdata, "last_script", last_script, 0);
    }
    return hdata;
}

/*
 * Adds a script in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
plugin_script_add_to_infolist (struct t_weechat_plugin *weechat_plugin,
                               struct t_infolist *infolist,
                               struct t_plugin_script *script)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !script)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_pointer (ptr_item, "pointer", script))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "filename", script->filename))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "interpreter", script->interpreter))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "name", script->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "author", script->author))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "version", script->version))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "license", script->license))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "description", script->description))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "shutdown_func", script->shutdown_func))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "charset", script->charset))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "unloading", script->unloading))
        return 0;

    return 1;
}

/*
 * Builds infolist with list of scripts.
 */

struct t_infolist *
plugin_script_infolist_list_scripts (struct t_weechat_plugin *weechat_plugin,
                                     struct t_plugin_script *scripts,
                                     void *pointer,
                                     const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_plugin_script *ptr_script;

    if (pointer && !plugin_script_valid (scripts, pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (ptr_infolist)
    {
        if (pointer)
        {
            /* build list with only one script */
            if (!plugin_script_add_to_infolist (weechat_plugin,
                                                ptr_infolist, pointer))
            {
                weechat_infolist_free (ptr_infolist);
                return NULL;
            }
            return ptr_infolist;
        }
        else
        {
            /* build list with all scripts matching arguments */
            for (ptr_script = scripts; ptr_script;
                 ptr_script = ptr_script->next_script)
            {
                if (!arguments || !arguments[0]
                    || weechat_string_match (ptr_script->name, arguments, 1))
                {
                    if (!plugin_script_add_to_infolist (weechat_plugin,
                                                        ptr_infolist, ptr_script))
                    {
                        weechat_infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
            }
            return ptr_infolist;
        }
    }

    return NULL;
}

/*
 * Ends script plugin.
 */

void
plugin_script_end (struct t_weechat_plugin *weechat_plugin,
                   struct t_plugin_script_data *plugin_data)
{
    int scripts_loaded;

    /* unload all scripts */
    scripts_loaded = (*(plugin_data->scripts)) ? 1 : 0;
    (void)(plugin_data->unload_all) ();
    if (scripts_loaded)
    {
        weechat_printf (NULL, _("%s: scripts unloaded"),
                        weechat_plugin->name);
    }

    /* write config file (file: "<language>.conf") */
    weechat_config_write (*(plugin_data->config_file));
    weechat_config_free (*(plugin_data->config_file));
}

/*
 * Prints scripts in WeeChat log file (usually for crash dump).
 */

void
plugin_script_print_log (struct t_weechat_plugin *weechat_plugin,
                         struct t_plugin_script *scripts)
{
    struct t_plugin_script *ptr_script;

    weechat_log_printf ("");
    weechat_log_printf ("***** \"%s\" plugin dump *****",
                        weechat_plugin->name);

    for (ptr_script = scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[script %s (addr:0x%lx)]",      ptr_script->name, ptr_script);
        weechat_log_printf ("  filename. . . . . . : '%s'",  ptr_script->filename);
        weechat_log_printf ("  interpreter . . . . : 0x%lx", ptr_script->interpreter);
        weechat_log_printf ("  name. . . . . . . . : '%s'",  ptr_script->name);
        weechat_log_printf ("  author. . . . . . . : '%s'",  ptr_script->author);
        weechat_log_printf ("  version . . . . . . : '%s'",  ptr_script->version);
        weechat_log_printf ("  license . . . . . . : '%s'",  ptr_script->license);
        weechat_log_printf ("  description . . . . : '%s'",  ptr_script->description);
        weechat_log_printf ("  shutdown_func . . . : '%s'",  ptr_script->shutdown_func);
        weechat_log_printf ("  charset . . . . . . : '%s'",  ptr_script->charset);
        weechat_log_printf ("  unloading . . . . . : %d",    ptr_script->unloading);
        weechat_log_printf ("  prev_script . . . . : 0x%lx", ptr_script->prev_script);
        weechat_log_printf ("  next_script . . . . : 0x%lx", ptr_script->next_script);
    }

    weechat_log_printf ("");
    weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                        weechat_plugin->name);
}
