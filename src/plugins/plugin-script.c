/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * plugin-script.c: common functions used by script plugins (perl/python/..)
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
#include "plugin-script-callback.h"


#define SCRIPT_OPTION_CHECK_LICENSE "check_license"

int script_option_check_license = 0;


/*
 * plugin_script_config_read: read script configuration
 */

void
plugin_script_config_read (struct t_weechat_plugin *weechat_plugin)
{
    const char *string;

    string = weechat_config_get_plugin (SCRIPT_OPTION_CHECK_LICENSE);
    if (!string)
    {
        weechat_config_set_plugin (SCRIPT_OPTION_CHECK_LICENSE, "on");
        string = weechat_config_get_plugin (SCRIPT_OPTION_CHECK_LICENSE);
    }
    if (string && (weechat_config_string_to_boolean (string) > 0))
        script_option_check_license = 1;
    else
        script_option_check_license = 0;
}

/*
 * plugin_script_config_cb: callback called when config option is changed
 */

int
plugin_script_config_cb (void *data, const char *option, const char *value)
{
    /* make C compiler happy */
    (void) option;
    (void) value;

    plugin_script_config_read (data);

    return WEECHAT_RC_OK;
}

/*
 * plugin_script_init: initialize script plugin
 */

void
plugin_script_init (struct t_weechat_plugin *weechat_plugin,
                    int argc, char *argv[],
                    struct t_plugin_script_init *init)
{
    char *string, *completion;
    char signal_name[128];
    int length, i, auto_load_scripts;

    /* read script configuration */
    plugin_script_config_read (weechat_plugin);

    /* add hook for config option */
    length = strlen (weechat_plugin->name) + 64;
    string = malloc (length);
    if (string)
    {
        snprintf (string, length, "plugins.var.%s.%s",
                  weechat_plugin->name, SCRIPT_OPTION_CHECK_LICENSE);
        weechat_hook_config (string, &plugin_script_config_cb, weechat_plugin);
        free (string);
    }

    /* create directories in WeeChat home */
    weechat_mkdir_home (weechat_plugin->name, 0755);
    length = strlen (weechat_plugin->name) + strlen ("/autoload") + 1;
    string = malloc (length);
    if (string)
    {
        snprintf (string, length, "%s/autoload", weechat_plugin->name);
        weechat_mkdir_home (string, 0755);
        free (string);
    }

    /* add command */
    completion = NULL;
    length = strlen (weechat_plugin->name) + 16;
    string = malloc (length);
    if (string)
    {
        snprintf (string, length, "%%(%s_script)",
                  weechat_plugin->name);
        completion = weechat_string_replace ("list %s"
                                             " || listfull %s"
                                             " || load %(filename)"
                                             " || autoload"
                                             " || reload %s"
                                             " || unload %s",
                                             "%s",
                                             string);
    }
    weechat_hook_command (weechat_plugin->name,
                          N_("list/load/unload scripts"),
                          N_("list|listfull [<name>]"
                             " || load <filename>"
                             " || autoload"
                             " || reload|unload [<name>]"),
                          N_("    list: list loaded scripts\n"
                             "listfull: list loaded scripts (verbose)\n"
                             "    load: load a script\n"
                             "autoload: load all scripts in \"autoload\" "
                             "directory\n"
                             "  reload: reload a script (if no name given, "
                             "unload all scripts, then load all scripts in "
                             "\"autoload\" directory)\n"
                             "  unload: unload a script (if no name given, "
                             "unload all scripts)\n"
                             "filename: script (file) to load\n"
                             "    name: a script name (name used in call to "
                             "\"register\" function)\n\n"
                             "Without argument, this command "
                             "lists all loaded scripts."),
                          completion,
                          init->callback_command, NULL);
    if (string)
        free (string);
    if (completion)
        free (completion);

    /* add completion, hdata and infolist */
    length = strlen (weechat_plugin->name) + 16;
    string = malloc (length);
    if (string)
    {
        snprintf (string, length, "%s_script", weechat_plugin->name);
        weechat_hook_completion (string, N_("list of scripts"),
                                 init->callback_completion, NULL);
        weechat_hook_hdata (string, N_("list of scripts"),
                            init->callback_hdata, NULL);
        weechat_hook_infolist (string, N_("list of scripts"),
                               N_("script pointer (optional)"),
                               N_("script name (can start or end with \"*\" as wildcard) (optional)"),
                               init->callback_infolist, NULL);
        free (string);
    }

    /* add signal for "debug_dump" */
    weechat_hook_signal ("debug_dump", init->callback_signal_debug_dump, NULL);

    /* add signal for "buffer_closed" */
    weechat_hook_signal ("buffer_closed",
                         init->callback_signal_buffer_closed, NULL);

    /* add signal for a script action (install/remove) */
    snprintf (signal_name, sizeof (signal_name), "%s_script_install",
              weechat_plugin->name);
    weechat_hook_signal (signal_name,
                         init->callback_signal_script_action, NULL);
    snprintf (signal_name, sizeof (signal_name), "%s_script_remove",
              weechat_plugin->name);
    weechat_hook_signal (signal_name,
                         init->callback_signal_script_action, NULL);

    /* parse arguments */
    auto_load_scripts = 1;
    for (i = 0; i < argc; i++)
    {
        if ((strcmp (argv[i], "-s") == 0)
                 || (strcmp (argv[i], "--no-script") == 0))
        {
            auto_load_scripts = 0;
        }
    }

    /* autoload scripts */
    if (auto_load_scripts)
    {
        plugin_script_auto_load (weechat_plugin, init->callback_load_file);
    }
}

/*
 * plugin_script_valid: check if a script pointer exists
 *                      return 1 if script exists
 *                             0 if script is not found
 */

int
plugin_script_valid (struct t_plugin_script *scripts,
                     struct t_plugin_script *script)
{
    struct t_plugin_script *ptr_script;

    if (!script)
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
 * plugin_script_ptr2str: convert pointer to string for usage in a script
 *                        (any language)
 *                        WARNING: result has to be free() after use
 */

char *
plugin_script_ptr2str (void *pointer)
{
    char pointer_str[128];

    if (!pointer)
        return strdup ("");

    snprintf (pointer_str, sizeof (pointer_str),
              "0x%lx", (long unsigned int)pointer);

    return strdup (pointer_str);
}

/*
 * plugin_script_str2ptr: convert string to pointer for usage outside script
 */

void *
plugin_script_str2ptr (struct t_weechat_plugin *weechat_plugin,
                       const char *script_name, const char *function_name,
                       const char *str_pointer)
{
    long unsigned int value;
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
    if (weechat_plugin->debug >= 1)
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
 * plugin_script_auto_load: auto-load all scripts in a directory
 */

void
plugin_script_auto_load (struct t_weechat_plugin *weechat_plugin,
                         void (*callback)(void *data, const char *filename))
{
    const char *dir_home;
    char *dir_name;
    int dir_length;

    /* build directory, adding WeeChat home */
    dir_home = weechat_info_get ("weechat_dir", "");
    if (!dir_home)
        return;
    dir_length = strlen (dir_home) + strlen (weechat_plugin->name) + 16;
    dir_name = malloc (dir_length);
    if (!dir_name)
        return;

    snprintf (dir_name, dir_length,
              "%s/%s/autoload", dir_home, weechat_plugin->name);
    weechat_exec_on_files (dir_name, 0, NULL, callback);

    free (dir_name);
}

/*
 * plugin_script_search: search a script in list (by registered name)
 */

struct t_plugin_script *
plugin_script_search (struct t_weechat_plugin *weechat_plugin,
                      struct t_plugin_script *scripts, const char *name)
{
    struct t_plugin_script *ptr_script;

    for (ptr_script = scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (weechat_strcasecmp (ptr_script->name, name) == 0)
            return ptr_script;
    }

    /* script not found */
    return NULL;
}

/*
 * plugin_script_search_by_full_name: search a script in list (by full name,
 *                                    for example "weeget.py")
 */

struct t_plugin_script *
plugin_script_search_by_full_name (struct t_plugin_script *scripts,
                                   const char *full_name)
{
    char *base_name;
    struct t_plugin_script *ptr_script;

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
 * plugin_script_search_path: search path name of a script
 */

char *
plugin_script_search_path (struct t_weechat_plugin *weechat_plugin,
                           const char *filename)
{
    char *final_name;
    const char *dir_home, *dir_system;
    int length;
    struct stat st;

    if (filename[0] == '~')
        return weechat_string_expand_home (filename);

    dir_home = weechat_info_get ("weechat_dir", "");
    if (dir_home)
    {
        /* try WeeChat user's autoload dir */
        length = strlen (dir_home) + strlen (weechat_plugin->name) + 8 +
            strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s/autoload/%s",
                      dir_home, weechat_plugin->name, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }

        /* try WeeChat language user's dir */
        length = strlen (dir_home) + strlen (weechat_plugin->name) +
            strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s/%s", dir_home, weechat_plugin->name, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }

        /* try WeeChat user's dir */
        length = strlen (dir_home) + strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s", dir_home, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }
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
                return final_name;
            free (final_name);
        }
    }

    return strdup (filename);
}

/*
 * plugin_script_find_pos: find position for a script (for sorting scripts list)
 */

struct t_plugin_script *
plugin_script_find_pos (struct t_weechat_plugin *weechat_plugin,
                        struct t_plugin_script *scripts,
                        struct t_plugin_script *script)
{
    struct t_plugin_script *ptr_script;

    for (ptr_script = scripts; ptr_script; ptr_script = ptr_script->next_script)
    {
        if (weechat_strcasecmp (script->name, ptr_script->name) < 0)
            return ptr_script;
    }
    return NULL;
}

/*
 * plugin_script_insert_sorted: insert a script in list, keeping sort on name
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
 * plugin_script_add: add a script to list of scripts
 */

struct t_plugin_script *
plugin_script_add (struct t_weechat_plugin *weechat_plugin,
                   struct t_plugin_script **scripts,
                   struct t_plugin_script **last_script,
                   const char *filename, const char *name, const char *author,
                   const char *version, const char *license,
                   const char *description, const char *shutdown_func,
                   const char *charset)
{
    struct t_plugin_script *new_script;

    if (strchr (name, ' '))
    {
        weechat_printf (NULL,
                        _("%s: error loading script \"%s\" (bad name, spaces "
                          "are forbidden)"),
                        weechat_plugin->name, name);
        return NULL;
    }

    if (script_option_check_license
        && (weechat_strcmp_ignore_chars (weechat_plugin->license, license,
                                         "0123456789-.,/\\()[]{}", 0) != 0))
    {
        weechat_printf (NULL,
                        _("%s%s: warning, license \"%s\" for script \"%s\" "
                          "differs from plugin license (\"%s\")"),
                        weechat_prefix ("error"), weechat_plugin->name,
                        license, name, weechat_plugin->license);
    }

    new_script = malloc (sizeof (*new_script));
    if (new_script)
    {
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
        new_script->callbacks = NULL;
        new_script->unloading = 0;

        plugin_script_insert_sorted (weechat_plugin, scripts, last_script,
                                     new_script);

        return new_script;
    }

    weechat_printf (NULL,
                    _("%s: error loading script \"%s\" (not enough memory)"),
                    weechat_plugin->name, name);

    return NULL;
}

/*
 * plugin_script_set_buffer_callbacks: restore buffers callbacks (input and
 *                                     close) for buffers created by script
 *                                     plugin
 */

void
plugin_script_set_buffer_callbacks (struct t_weechat_plugin *weechat_plugin,
                                    struct t_plugin_script *scripts,
                                    struct t_plugin_script *script,
                                    int (*callback_buffer_input) (void *data,
                                                                  struct t_gui_buffer *buffer,
                                                                  const char *input_data),
                                    int (*callback_buffer_close) (void *data,
                                                                  struct t_gui_buffer *buffer))
{
    struct t_infolist *infolist;
    struct t_gui_buffer *ptr_buffer;
    const char *script_name, *str_script_input_cb, *str_script_input_cb_data;
    const char *str_script_close_cb, *str_script_close_cb_data;
    struct t_plugin_script *ptr_script;
    struct t_script_callback *script_cb_input;
    struct t_script_callback *script_cb_close;

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
                    ptr_script = plugin_script_search (weechat_plugin, scripts,
                                                       script_name);
                    if (ptr_script && (ptr_script == script))
                    {
                        str_script_input_cb = weechat_buffer_get_string (ptr_buffer,
                                                                         "localvar_script_input_cb");
                        str_script_input_cb_data = weechat_buffer_get_string (ptr_buffer,
                                                                              "localvar_script_input_cb_data");
                        str_script_close_cb = weechat_buffer_get_string (ptr_buffer,
                                                                         "localvar_script_close_cb");
                        str_script_close_cb_data = weechat_buffer_get_string (ptr_buffer,
                                                                              "localvar_script_close_cb_data");

                        if (str_script_input_cb && str_script_input_cb[0])
                        {
                            script_cb_input = plugin_script_callback_add (ptr_script,
                                                                          str_script_input_cb,
                                                                          str_script_input_cb_data);
                            if (script_cb_input)
                            {
                                script_cb_input->buffer = ptr_buffer;
                                weechat_buffer_set_pointer (ptr_buffer,
                                                            "input_callback",
                                                            callback_buffer_input);
                                weechat_buffer_set_pointer (ptr_buffer,
                                                            "input_callback_data",
                                                            script_cb_input);
                            }
                        }
                        if (str_script_close_cb && str_script_close_cb[0])
                        {
                            script_cb_close = plugin_script_callback_add (ptr_script,
                                                                          str_script_close_cb,
                                                                          str_script_close_cb_data);
                            if (script_cb_close)
                            {
                                script_cb_close->buffer = ptr_buffer;
                                weechat_buffer_set_pointer (ptr_buffer,
                                                            "close_callback",
                                                            callback_buffer_close);
                                weechat_buffer_set_pointer (ptr_buffer,
                                                            "close_callback_data",
                                                            script_cb_close);
                            }
                        }
                    }
                }
            }
        }
        weechat_infolist_free (infolist);
    }
}

/*
 * plugin_script_remove_buffer_callbacks: remove callbacks for a buffer (called
 *                                        when a buffer is closed by user)
 */

void
plugin_script_remove_buffer_callbacks (struct t_plugin_script *scripts,
                                       struct t_gui_buffer *buffer)
{
    struct t_plugin_script *ptr_script;
    struct t_script_callback *ptr_script_cb, *next_script_cb;

    for (ptr_script = scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        /*
         * do not remove buffer callbacks if script is being unloaded
         * (because all callbacks will be removed anyway)
         */
        if (!ptr_script->unloading)
        {
            ptr_script_cb = ptr_script->callbacks;
            while (ptr_script_cb)
            {
                next_script_cb = ptr_script_cb->next_callback;

                if (ptr_script_cb->buffer == buffer)
                    plugin_script_callback_remove (ptr_script, ptr_script_cb);

                ptr_script_cb = next_script_cb;
            }
        }
    }
}

/*
 * plugin_script_remove: remove a script from list of scripts
 */

void
plugin_script_remove (struct t_weechat_plugin *weechat_plugin,
                      struct t_plugin_script **scripts,
                      struct t_plugin_script **last_script,
                      struct t_plugin_script *script)
{
    struct t_script_callback *ptr_script_cb, *ptr_script_cb2;

    script->unloading = 1;

    for (ptr_script_cb = script->callbacks; ptr_script_cb;
         ptr_script_cb = ptr_script_cb->next_callback)
    {
        /* free config file */
        if (ptr_script_cb->config_file)
        {
            if (weechat_config_boolean (weechat_config_get ("weechat.plugin.save_config_on_unload")))
                weechat_config_write (ptr_script_cb->config_file);
            weechat_config_free (ptr_script_cb->config_file);
        }

        /* unhook */
        if (ptr_script_cb->hook)
            weechat_unhook (ptr_script_cb->hook);

        /* close buffer */
        if (ptr_script_cb->buffer)
            weechat_buffer_close (ptr_script_cb->buffer);

        /* remove bar item */
        if (ptr_script_cb->bar_item)
            weechat_bar_item_remove (ptr_script_cb->bar_item);

        /*
         * remove same pointers in other callbacks
         * (to not free 2 times with same pointer!)
         */
        for (ptr_script_cb2 = ptr_script_cb->next_callback; ptr_script_cb2;
             ptr_script_cb2 = ptr_script_cb2->next_callback)
        {
            if (ptr_script_cb2->config_file == ptr_script_cb->config_file)
                ptr_script_cb2->config_file = NULL;
            if (ptr_script_cb2->config_section == ptr_script_cb->config_section)
                ptr_script_cb2->config_section = NULL;
            if (ptr_script_cb2->config_option == ptr_script_cb->config_option)
                ptr_script_cb2->config_option = NULL;
            if (ptr_script_cb2->hook == ptr_script_cb->hook)
                ptr_script_cb2->hook = NULL;
            if (ptr_script_cb2->buffer == ptr_script_cb->buffer)
                ptr_script_cb2->buffer = NULL;
            if (ptr_script_cb2->bar_item == ptr_script_cb->bar_item)
                ptr_script_cb2->bar_item = NULL;
            if (ptr_script_cb2->upgrade_file == ptr_script_cb->upgrade_file)
                ptr_script_cb2->upgrade_file = NULL;
        }
    }

    /* remove all callbacks created by this script */
    plugin_script_callback_remove_all (script);

    /* free data */
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

    /* remove script from list */
    if (script->prev_script)
        (script->prev_script)->next_script = script->next_script;
    if (script->next_script)
        (script->next_script)->prev_script = script->prev_script;
    if (*scripts == script)
        *scripts = script->next_script;
    if (*last_script == script)
        *last_script = script->prev_script;

    /* free script */
    free (script);
}

/*
 * plugin_script_completion: complete with list of scripts
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
        weechat_hook_completion_list_add (completion, ptr_script->name,
                                          0, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * plugin_script_action_add: add script name for a plugin action
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
 * plugin_script_remove_file: remove script file(s) from disk
 */

void
plugin_script_remove_file (struct t_weechat_plugin *weechat_plugin,
                           const char *name,
                           int display_error_if_no_script_removed)
{
    int num_found, i;
    char *path_script;

    num_found = 0;
    i = 0;
    while (i < 2)
    {
        path_script = plugin_script_search_path (weechat_plugin, name);
        /* script not found? */
        if (!path_script || (strcmp (path_script, name) == 0))
            break;
        num_found++;
        if (unlink (path_script) == 0)
        {
            weechat_printf (NULL, _("%s: script removed: %s"),
                            weechat_plugin->name,
                            path_script);
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
}

/*
 * plugin_script_action_install: install some scripts (using comma separated
 *                               list)
 *                               this function does following tasks:
 *                                 1. unload script (if script is loaded)
 *                                 2. remove script file(s)
 *                                 3. move script file from "install" dir to
 *                                    language dir
 *                                 4. make link in autoload dir
 *                                 5. load script
 */

void
plugin_script_action_install (struct t_weechat_plugin *weechat_plugin,
                              struct t_plugin_script *scripts,
                              void (*script_unload)(struct t_plugin_script *script),
                              int (*script_load)(const char *filename),
                              char **list)
{
    char **argv, *name, *ptr_base_name, *base_name, *new_path, *autoload_path;
    char *symlink_path;
    const char *dir_home, *dir_separator;
    int argc, i, length, rc;
    struct t_plugin_script *ptr_script;

    if (*list)
    {
        argv = weechat_string_split (*list, ",", 0, 0, &argc);
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
                        ptr_script = plugin_script_search_by_full_name (scripts,
                                                                        base_name);
                        if (ptr_script)
                            (*script_unload) (ptr_script);

                        /* remove script file(s) */
                        plugin_script_remove_file (weechat_plugin, base_name, 0);

                        /* move file from install dir to language dir */
                        dir_home = weechat_info_get ("weechat_dir", "");
                        length = strlen (dir_home) + strlen (weechat_plugin->name) +
                            strlen (base_name) + 16;
                        new_path = malloc (length);
                        if (new_path)
                        {
                            snprintf (new_path, length, "%s/%s/%s",
                                      dir_home, weechat_plugin->name, base_name);
                            if (rename (name, new_path) == 0)
                            {
                                /* make link in autoload dir */
                                length = strlen (dir_home) +
                                    strlen (weechat_plugin->name) + 8 +
                                    strlen (base_name) + 16;
                                autoload_path = malloc (length);
                                if (autoload_path)
                                {
                                    snprintf (autoload_path, length,
                                              "%s/%s/autoload/%s",
                                              dir_home, weechat_plugin->name,
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
                                }

                                /* load script */
                                (*script_load) (new_path);
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
                    }
                    free (name);
                }
            }
            weechat_string_free_split (argv);
        }
        free (*list);
        *list = NULL;
    }
}

/*
 * plugin_script_action_remove: remove some scripts (using comma separated list)
 *                              this function does following tasks:
 *                                1. unload script (if script is loaded)
 *                                2. remove script file(s)
 */

void
plugin_script_action_remove (struct t_weechat_plugin *weechat_plugin,
                             struct t_plugin_script *scripts,
                             void (*script_unload)(struct t_plugin_script *script),
                             char **list)
{
    char **argv;
    int argc, i;
    struct t_plugin_script *ptr_script;

    if (*list)
    {
        argv = weechat_string_split (*list, ",", 0, 0, &argc);
        if (argv)
        {
            for (i = 0; i < argc; i++)
            {
                /* unload script, if script is loaded */
                ptr_script = plugin_script_search_by_full_name (scripts, argv[i]);
                if (ptr_script)
                    (*script_unload) (ptr_script);

                /* remove script file(s) */
                plugin_script_remove_file (weechat_plugin, argv[i], 1);
            }
            weechat_string_free_split (argv);
        }
        free (*list);
        *list = NULL;
    }
}

/*
 * plugin_script_display_list: print list of scripts
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
 * plugin_script_display_short_list: print list of scripts on one line
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
 * plugin_script_hdata_script: return hdata for script
 */

struct t_hdata *
plugin_script_hdata_script (struct t_weechat_plugin *weechat_plugin,
                            struct t_plugin_script **scripts,
                            struct t_plugin_script **last_script,
                            const char *hdata_name)
{
    struct t_hdata *hdata;

    hdata = weechat_hdata_new (hdata_name, "prev_script", "next_script");
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_plugin_script, filename, STRING, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, interpreter, POINTER, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, name, STRING, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, author, STRING, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, version, STRING, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, license, STRING, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, description, STRING, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, shutdown_func, STRING, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, charset, STRING, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, callbacks, POINTER, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, unloading, INTEGER, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_plugin_script, prev_script, POINTER, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_plugin_script, next_script, POINTER, NULL, hdata_name);
        WEECHAT_HDATA_LIST(*scripts);
        WEECHAT_HDATA_LIST(*last_script);
    }
    return hdata;
}

/*
 * plugin_script_add_to_infolist: add a script in an infolist
 *                                return 1 if ok, 0 if error
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
 * plugin_script_infolist_list_scripts: build infolist with list of scripts
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
                    || weechat_string_match (ptr_script->name, arguments, 0))
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
 * plugin_script_end: end script plugin
 */

void
plugin_script_end (struct t_weechat_plugin *weechat_plugin,
                   struct t_plugin_script **scripts,
                   void (*callback_unload_all)())
{
    int scripts_loaded;

    scripts_loaded = (*scripts) ? 1 : 0;

    (void)(callback_unload_all) ();

    if (scripts_loaded)
    {
        weechat_printf (NULL, _("%s: scripts unloaded"),
                        weechat_plugin->name);
    }
}

/*
 * plugin_script_print_log: print script infos in log (usually for crash dump)
 */

void
plugin_script_print_log (struct t_weechat_plugin *weechat_plugin,
                         struct t_plugin_script *scripts)
{
    struct t_plugin_script *ptr_script;
    struct t_script_callback *ptr_script_cb;

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
        weechat_log_printf ("  callbacks . . . . . : 0x%lx", ptr_script->callbacks);
        weechat_log_printf ("  unloading . . . . . : %d",    ptr_script->unloading);
        weechat_log_printf ("  prev_script . . . . : 0x%lx", ptr_script->prev_script);
        weechat_log_printf ("  next_script . . . . : 0x%lx", ptr_script->next_script);

        for (ptr_script_cb = ptr_script->callbacks; ptr_script_cb;
             ptr_script_cb = ptr_script_cb->next_callback)
        {
            plugin_script_callback_print_log (weechat_plugin, ptr_script_cb);
        }
    }

    weechat_log_printf ("");
    weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                        weechat_plugin->name);
}
