/*
 * script-action.c - actions on scripts
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-action.h"
#include "script-buffer.h"
#include "script-config.h"
#include "script-repo.h"


char **script_actions = NULL;


void script_action_run_install (int quiet);


/*
 * Lists loaded scripts (all languages).
 */

void
script_action_run_list (void)
{
    int i, scripts_loaded;
    char hdata_name[128];
    const char *ptr_name;
    struct t_hdata *hdata;
    void *ptr_script;

    weechat_printf (NULL, "");
    weechat_printf (NULL, _("Scripts loaded:"));

    scripts_loaded = 0;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        snprintf (hdata_name, sizeof (hdata_name),
                  "%s_script", script_language[i]);
        hdata = weechat_hdata_get (hdata_name);
        ptr_script = weechat_hdata_get_list (hdata, "scripts");
        while (ptr_script)
        {
            ptr_name = weechat_hdata_string (hdata, ptr_script, "name");
            weechat_printf (NULL, " %s %s%s%s.%s %s%s %s(%s%s%s)",
                            script_repo_get_status_for_display (script_repo_search_by_name (ptr_name),
                                                                "*?iaHN", 0),
                            weechat_color (weechat_config_string (script_config_color_text_name)),
                            ptr_name,
                            weechat_color (weechat_config_string (script_config_color_text_extension)),
                            script_extension[i],
                            weechat_color (weechat_config_string (script_config_color_text_version)),
                            weechat_hdata_string (hdata, ptr_script, "version"),
                            weechat_color ("chat_delimiters"),
                            weechat_color (weechat_config_string (script_config_color_text_description)),
                            weechat_hdata_string (hdata, ptr_script, "description"),
                            weechat_color ("chat_delimiters"));
            scripts_loaded++;
            ptr_script = weechat_hdata_move (hdata, ptr_script, 1);
        }
    }

    if (scripts_loaded == 0)
    {
        weechat_printf (NULL, _("  (none)"));
    }
}

/*
 * Lists loaded scripts (all languages) in input.
 *
 * Sends input to buffer if send_to_buffer == 1.
 * String is translated if translated == 1 (otherwise it's English).
 */

void
script_action_run_list_input (struct t_gui_buffer *buffer,
                              int send_to_buffer, int translated)
{
    int i, length;
    char hdata_name[128], **output, str_pos[16];
    struct t_hdata *hdata;
    void *ptr_script;

    if (!buffer)
        return;

    output = weechat_string_dyn_alloc (256);
    if (!output)
        return;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        snprintf (hdata_name, sizeof (hdata_name),
                  "%s_script", script_language[i]);
        hdata = weechat_hdata_get (hdata_name);
        ptr_script = weechat_hdata_get_list (hdata, "scripts");
        while (ptr_script)
        {
            if ((*output)[0])
            {
                weechat_string_dyn_concat (output, ", ", -1);
            }
            else
            {
                weechat_string_dyn_concat (
                    output,
                    (translated) ? _("Scripts loaded:") : "Scripts loaded:",
                    -1);
                weechat_string_dyn_concat (output, " ", -1);
            }
            weechat_string_dyn_concat (output,
                                       weechat_hdata_string (hdata,
                                                             ptr_script,
                                                             "name"),
                                       -1);
            weechat_string_dyn_concat (output, ".", -1);
            weechat_string_dyn_concat (output, script_extension[i], -1);
            weechat_string_dyn_concat (output, " ", -1);
            weechat_string_dyn_concat (output,
                                       weechat_hdata_string (hdata,
                                                             ptr_script,
                                                             "version"),
                                       -1);
            ptr_script = weechat_hdata_move (hdata, ptr_script, 1);
        }
    }

    if (!(*output)[0])
    {
        weechat_string_dyn_concat (
            output,
            (translated) ? _("No scripts loaded") : "No scripts loaded",
            -1);
    }

    if (send_to_buffer)
    {
        weechat_command (buffer, *output);
    }
    else
    {
        weechat_buffer_set (buffer, "input", *output);
        length = weechat_utf8_strlen (*output);
        snprintf (str_pos, sizeof (str_pos), "%d", length);
        weechat_buffer_set (buffer, "input_pos", str_pos);
    }

    weechat_string_dyn_free (output, 1);
}

/*
 * Loads a script.
 */

void
script_action_run_load (const char *name, int quiet)
{
    char *pos, str_command[1024];
    int language;

    language = -1;
    pos = strrchr (name, '.');
    if (pos)
        language = script_language_search_by_extension (pos + 1);
    if (language < 0)
    {
        if (!quiet)
        {
            weechat_printf (NULL,
                            _("%s: unknown language for script \"%s\""),
                            SCRIPT_PLUGIN_NAME, name);
        }
        return;
    }

    /* check that plugin for this language is loaded */
    if (!script_plugin_loaded[language])
    {
        weechat_printf (NULL,
                        _("%s: plugin \"%s\" is not loaded"),
                        SCRIPT_PLUGIN_NAME, script_language[language]);
        return;
    }

    /* execute command (for example: "/python load go.py") */
    snprintf (str_command, sizeof (str_command),
              "/%s load %s%s",
              script_language[language],
              (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
              name);
    weechat_command (NULL, str_command);
}

/*
 * Unloads a script.
 */

void
script_action_run_unload (const char *name, int quiet)
{
    char *pos, hdata_name[128], *filename, *ptr_base_name, str_command[1024];
    const char *ptr_filename, *ptr_registered_name;
    int language, found, i;
    struct t_hdata *hdata;
    void *ptr_script;

    pos = strrchr (name, '.');
    if (pos)
    {
        /* unload script by using name + extension (example: "go.py") */
        language = script_language_search_by_extension (pos + 1);
        if (language < 0)
        {
            if (!quiet)
            {
                weechat_printf (NULL,
                                _("%s: unknown language for script \"%s\""),
                                SCRIPT_PLUGIN_NAME, name);
            }
            return;
        }
        /*
         * search registered name of script using name with extension,
         * for example with "go.py" we should find "go"
         */
        snprintf (hdata_name, sizeof (hdata_name),
                  "%s_script", script_language[language]);
        hdata = weechat_hdata_get (hdata_name);
        ptr_script = weechat_hdata_get_list (hdata, "scripts");
        while (ptr_script)
        {
            found = 0;
            ptr_filename = weechat_hdata_string (hdata, ptr_script, "filename");
            if (ptr_filename)
            {
                filename = strdup (ptr_filename);
                if (filename)
                {
                    ptr_base_name = basename (filename);
                    if (strcmp (ptr_base_name, name) == 0)
                        found = 1;
                    free (filename);
                }
            }
            if (found)
            {
                ptr_registered_name = weechat_hdata_string (hdata, ptr_script,
                                                            "name");
                if (ptr_registered_name)
                {
                    snprintf (str_command, sizeof (str_command),
                              "/%s unload %s%s",
                              script_language[language],
                              (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
                              ptr_registered_name);
                    weechat_command (NULL, str_command);
                }
                return;
            }
            ptr_script = weechat_hdata_move (hdata, ptr_script, 1);
        }
    }
    else
    {
        /* unload script by using name (example: "go") */
        for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
        {
            snprintf (hdata_name, sizeof (hdata_name),
                      "%s_script", script_language[i]);
            hdata = weechat_hdata_get (hdata_name);
            ptr_script = weechat_hdata_get_list (hdata, "scripts");
            while (ptr_script)
            {
                ptr_registered_name = weechat_hdata_string (hdata, ptr_script,
                                                            "name");
                if (strcmp (ptr_registered_name, name) == 0)
                {
                    snprintf (str_command, sizeof (str_command),
                              "/%s unload %s%s",
                              script_language[i],
                              (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
                              name);
                    weechat_command (NULL, str_command);
                    return;
                }
                ptr_script = weechat_hdata_move (hdata, ptr_script, 1);
            }
        }
    }

    if (!quiet)
    {
        weechat_printf (NULL,
                        _("%s: script \"%s\" is not loaded"),
                        SCRIPT_PLUGIN_NAME, name);
    }
}

/*
 * Reloads a script.
 */

void
script_action_run_reload (const char *name, int quiet)
{
    char *pos, hdata_name[128], *filename, *ptr_base_name, str_command[1024];
    const char *ptr_filename, *ptr_registered_name;
    int language, found, i;
    struct t_hdata *hdata;
    void *ptr_script;

    pos = strrchr (name, '.');
    if (pos)
    {
        /* reload script by using name + extension (example: "go.py") */
        language = script_language_search_by_extension (pos + 1);
        if (language < 0)
        {
            if (!quiet)
            {
                weechat_printf (NULL,
                                _("%s: unknown language for script \"%s\""),
                                SCRIPT_PLUGIN_NAME, name);
            }
            return;
        }
        /*
         * search registered name of script using name with extension,
         * for example with "go.py" we should find "go"
         */
        snprintf (hdata_name, sizeof (hdata_name),
                  "%s_script", script_language[language]);
        hdata = weechat_hdata_get (hdata_name);
        ptr_script = weechat_hdata_get_list (hdata, "scripts");
        while (ptr_script)
        {
            found = 0;
            ptr_filename = weechat_hdata_string (hdata, ptr_script, "filename");
            if (ptr_filename)
            {
                filename = strdup (ptr_filename);
                if (filename)
                {
                    ptr_base_name = basename (filename);
                    if (strcmp (ptr_base_name, name) == 0)
                        found = 1;
                    free (filename);
                }
            }
            if (found)
            {
                ptr_registered_name = weechat_hdata_string (hdata, ptr_script,
                                                            "name");
                if (ptr_registered_name)
                {
                    snprintf (str_command, sizeof (str_command),
                              "/%s reload %s%s",
                              script_language[language],
                              (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
                              ptr_registered_name);
                    weechat_command (NULL, str_command);
                }
                return;
            }
            ptr_script = weechat_hdata_move (hdata, ptr_script, 1);
        }
    }
    else
    {
        /* reload script by using name (example: "go") */
        for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
        {
            snprintf (hdata_name, sizeof (hdata_name),
                      "%s_script", script_language[i]);
            hdata = weechat_hdata_get (hdata_name);
            ptr_script = weechat_hdata_get_list (hdata, "scripts");
            while (ptr_script)
            {
                ptr_registered_name = weechat_hdata_string (hdata, ptr_script,
                                                            "name");
                if (strcmp (ptr_registered_name, name) == 0)
                {
                    snprintf (str_command, sizeof (str_command),
                              "/%s reload %s%s",
                              script_language[i],
                              (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
                              name);
                    weechat_command (NULL, str_command);
                    return;
                }
                ptr_script = weechat_hdata_move (hdata, ptr_script, 1);
            }
        }
    }

    if (!quiet)
    {
        weechat_printf (NULL,
                        _("%s: script \"%s\" is not loaded"),
                        SCRIPT_PLUGIN_NAME, name);
    }
}

/*
 * Enables/disables autoload for a script.
 *
 * Argument "autoload" can be:
 *    0: disables autoload
 *    1: enables autoload
 *   -1: toggles autoload
 */

void
script_action_run_autoload (const char *name, int quiet, int autoload)
{
    struct t_script_repo *ptr_script;
    char *pos, str_signal[256], *weechat_data_dir, *filename;
    int language, script_found, script_autoloaded;
    struct stat st;

    /* find script language */
    language = -1;
    pos = strrchr (name, '.');
    if (pos)
        language = script_language_search_by_extension (pos + 1);
    if (language < 0)
    {
        if (!quiet)
        {
            weechat_printf (NULL,
                            _("%s: unknown language for script \"%s\""),
                            SCRIPT_PLUGIN_NAME, name);
        }
        return;
    }

    /* check if script exists and if it's currently autoloaded */
    script_found = 0;
    script_autoloaded = 0;
    weechat_data_dir = weechat_info_get ("weechat_data_dir", NULL);

    if (weechat_asprintf (&filename,
                          "%s/%s/%s",
                          weechat_data_dir,
                          script_language[language],
                          name) >= 0)
    {
        /* check if script exists */
        if (stat (filename, &st) == 0)
            script_found = 1;

        free (filename);

        if (weechat_asprintf (&filename,
                              "%s/%s/autoload/%s",
                              weechat_data_dir,
                              script_language[language],
                              name) >= 0)
        {
            /* check if script is autoloaded */
            if (stat (filename, &st) == 0)
                script_autoloaded = 1;
            free (filename);
        }
    }

    free (weechat_data_dir);

    if (!script_found)
    {
        if (!quiet)
        {
            weechat_printf (NULL,
                            _("%s: script \"%s\" not found"),
                            SCRIPT_PLUGIN_NAME, name);
        }
        return;
    }

    /* toggle autoload if value is -1 */
    if (autoload < 0)
        autoload = (script_autoloaded) ? 0 : 1;

    /* ask plugin to autoload (or not) script */
    if (weechat_asprintf (
            &filename,
            "%s%s%s",
            (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
            (autoload) ? "-a " : "",
            name) >= 0)
    {
        snprintf (str_signal, sizeof (str_signal),
                  "%s_script_autoload",
                  script_language[language]);
        (void) weechat_hook_signal_send (str_signal,
                                         WEECHAT_HOOK_SIGNAL_STRING,
                                         filename);
        free (filename);
    }

    if (!quiet)
    {
        weechat_printf (NULL,
                        (autoload) ?
                        _("%s: autoload enabled for script \"%s\"") :
                        _("%s: autoload disabled for script \"%s\""),
                        SCRIPT_PLUGIN_NAME, name);
    }

    ptr_script = script_repo_search_by_name_ext (name);
    if (ptr_script)
        script_repo_update_status (ptr_script);
}

/*
 * Installs next script.
 */

int
script_action_installnext_timer_cb (const void *pointer, void *data,
                                    int remaining_calls)
{
    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    script_action_run_install ((pointer) ? 1 : 0);

    return WEECHAT_RC_OK;
}

/*
 * Installs script (after download of script).
 */

int
script_action_install_url_cb (const void *pointer, void *data,
                              const char *url,
                              struct t_hashtable *options,
                              struct t_hashtable *output)
{
    const char *pos_name, *ptr_error;
    char *filename, *filename2, str_signal[256];
    int quiet, auto_load;
    struct t_script_repo *ptr_script;

    /* make C compiler happy */
    (void) data;
    (void) options;

    quiet = (pointer) ? 1 : 0;

    pos_name = strrchr (url, '/');
    if (pos_name)
        pos_name++;

    ptr_error = weechat_hashtable_get (output, "error");
    if (ptr_error && ptr_error[0])
    {
        weechat_printf (NULL,
                        _("%s%s: error downloading script \"%s\": %s"),
                        weechat_prefix ("error"),
                        SCRIPT_PLUGIN_NAME,
                        (pos_name) ? pos_name : "?",
                        ptr_error);
        return WEECHAT_RC_OK;
    }

    if (!pos_name)
    {
        /* silently ignore malformed URL (it should never happen) */
        return WEECHAT_RC_OK;
    }

    ptr_script = script_repo_search_by_name_ext (pos_name);
    if (!ptr_script)
        return WEECHAT_RC_OK;

    filename = script_config_get_script_download_filename (ptr_script, NULL);
    if (!filename)
        return WEECHAT_RC_OK;

    if (ptr_script->status & SCRIPT_STATUS_INSTALLED)
        auto_load = (ptr_script->status & SCRIPT_STATUS_AUTOLOADED) ? 1 : 0;
    else
        auto_load = weechat_config_boolean (script_config_scripts_autoload);

    if (weechat_asprintf (
            &filename2,
            "%s%s%s",
            (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
            (auto_load) ? "-a " : "",
            filename) < 0)
    {
        free (filename);
        return WEECHAT_RC_OK;
    }

    snprintf (str_signal, sizeof (str_signal),
              "%s_script_install",
              script_language[ptr_script->language]);
    (void) weechat_hook_signal_send (str_signal,
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     filename2);
    free (filename);
    free (filename2);

    /* schedule install of next script */
    weechat_hook_timer (10, 0, 1,
                        &script_action_installnext_timer_cb,
                        (quiet) ? (void *)1 : (void *)0,
                        NULL);

    return WEECHAT_RC_OK;
}

/*
 * Gets next script to install according to "install_order" in scripts.
 */

struct t_script_repo *
script_action_get_next_script_to_install (void)
{
    struct t_script_repo *ptr_script, *ptr_script_to_install;

    ptr_script_to_install = NULL;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (ptr_script->install_order > 0)
        {
            if (ptr_script->install_order == 1)
                ptr_script_to_install = ptr_script;
            ptr_script->install_order--;
        }
    }

    return ptr_script_to_install;
}

/*
 * Installs scrip(s) marked for install.
 */

void
script_action_run_install (int quiet)
{
    struct t_script_repo *ptr_script_to_install;
    char *filename;
    struct t_hashtable *options;

    if (!script_download_enabled (1))
        return;

    while (1)
    {
        ptr_script_to_install = script_action_get_next_script_to_install ();

        /* no more script to install? just exit function */
        if (!ptr_script_to_install)
            return;

        /*
         * script to install and plugin is loaded: exit loop and go on with
         * install
         */
        if (script_plugin_loaded[ptr_script_to_install->language])
            break;

        /* plugin not loaded for language of script: display error */
        weechat_printf (NULL,
                        _("%s: script \"%s\" cannot be installed because "
                          "plugin \"%s\" is not loaded"),
                        SCRIPT_PLUGIN_NAME,
                        ptr_script_to_install->name_with_extension,
                        script_language[ptr_script_to_install->language]);
    }

    if (!ptr_script_to_install->url || !ptr_script_to_install->url[0])
        return;

    filename = script_config_get_script_download_filename (ptr_script_to_install,
                                                           NULL);
    if (!filename)
        return;

    options = weechat_hashtable_new (32,
                                     WEECHAT_HASHTABLE_STRING,
                                     WEECHAT_HASHTABLE_STRING,
                                     NULL, NULL);
    if (!options)
    {
        free (filename);
        return;
    }

    if (!weechat_config_boolean (script_config_look_quiet_actions))
    {
        weechat_printf (NULL,
                        _("%s: downloading script \"%s\"..."),
                        SCRIPT_PLUGIN_NAME,
                        ptr_script_to_install->name_with_extension);
    }

    weechat_hashtable_set (options, "file_out", filename);
    weechat_hook_url (
        ptr_script_to_install->url,
        options,
        weechat_config_integer (script_config_scripts_download_timeout) * 1000,
        &script_action_install_url_cb,
        (quiet) ? (void *)1 : (void *)0,
        NULL);
    weechat_hashtable_free (options);
    free (filename);
}

/*
 * Removes a script.
 */

void
script_action_run_remove (const char *name, int quiet)
{
    struct t_script_repo *ptr_script;
    char str_signal[256], *filename;

    ptr_script = script_repo_search_by_name_ext (name);
    if (!ptr_script)
    {
        if (!quiet)
        {
            weechat_printf (NULL,
                            _("%s: script \"%s\" not found"),
                            SCRIPT_PLUGIN_NAME, name);
        }
        return;
    }

    /* check that script is installed */
    if (!(ptr_script->status & SCRIPT_STATUS_INSTALLED))
    {
        if (!quiet)
        {
            weechat_printf (NULL,
                            _("%s: script \"%s\" is not installed"),
                            SCRIPT_PLUGIN_NAME, name);
        }
        return;
    }

    /* check that script is not held */
    if (ptr_script->status & SCRIPT_STATUS_HELD)
    {
        if (!quiet)
        {
            weechat_printf (NULL,
                            _("%s: script \"%s\" is held"),
                            SCRIPT_PLUGIN_NAME, name);
        }
        return;
    }

    /* check that plugin for this language is loaded */
    if (!script_plugin_loaded[ptr_script->language])
    {
        weechat_printf (NULL,
                        _("%s: script \"%s\" cannot be removed "
                          "because plugin \"%s\" is not loaded"),
                        SCRIPT_PLUGIN_NAME,
                        ptr_script->name_with_extension,
                        script_language[ptr_script->language]);
        return;
    }

    /* ask plugin to remove script */
    if (weechat_asprintf (
            &filename,
            "%s%s",
            (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
            ptr_script->name_with_extension) >= 0)
    {
        snprintf (str_signal, sizeof (str_signal),
                  "%s_script_remove",
                  script_language[ptr_script->language]);
        (void) weechat_hook_signal_send (str_signal,
                                         WEECHAT_HOOK_SIGNAL_STRING,
                                         filename);
        free (filename);
    }
}

/*
 * Un(hold)s a script.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
script_action_run_hold (const char *name, int quiet)
{
    struct t_script_repo *ptr_script;

    ptr_script = script_repo_search_by_name_ext (name);
    if (ptr_script)
    {
        if (ptr_script->status & SCRIPT_STATUS_HELD)
        {
            script_config_unhold (ptr_script->name_with_extension);
            if (!quiet)
            {
                weechat_printf (NULL,
                                _("%s: script \"%s\" is not "
                                  "held anymore"),
                                SCRIPT_PLUGIN_NAME, name);
            }
        }
        else
        {
            script_config_hold (ptr_script->name_with_extension);
            if (!quiet)
            {
                weechat_printf (NULL,
                                _("%s: script \"%s\" is held"),
                                SCRIPT_PLUGIN_NAME, name);
            }
        }
        script_repo_update_status (ptr_script);
        return 1;
    }
    else
    {
        if (!quiet)
        {
            weechat_printf (NULL,
                            _("%s: script \"%s\" not found"),
                            SCRIPT_PLUGIN_NAME, name);
        }
    }

    return 0;
}

/*
 * Shows a diff between script installed and script in repository (after
 * download of script).
 */

int
script_action_show_diff_process_cb (const void *pointer, void *data,
                                    const char *command,
                                    int return_code, const char *out,
                                    const char *err)
{
    char **lines, *filename;
    const char *color;
    int num_lines, i, diff_color;

    /* make C compiler happy */
    (void) data;
    (void) command;

    if (script_buffer && script_buffer_detail_script
        && ((return_code == WEECHAT_HOOK_PROCESS_RUNNING) || (return_code >= 0)))
    {
        if (out)
        {
            lines = weechat_string_split (out, "\n", NULL,
                                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                          0, &num_lines);
            if (lines)
            {
                diff_color = weechat_config_boolean (script_config_look_diff_color);
                for (i = 0; i < num_lines; i++)
                {
                    color = NULL;
                    if (diff_color)
                    {
                        switch (lines[i][0])
                        {
                            case '-':
                            case '<':
                                color = weechat_color ("red");
                                break;
                            case '+':
                            case '>':
                                color = weechat_color ("green");
                                break;
                            case '@':
                                color = weechat_color ("cyan");
                                break;
                        }
                    }
                    weechat_printf_y (script_buffer,
                                      script_buffer_detail_script_last_line++,
                                      "%s%s",
                                      (color) ? color : "",
                                      lines[i]);
                }
                weechat_string_free_split (lines);
            }
        }
        else if (err)
        {
            lines = weechat_string_split (err, "\n", NULL,
                                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                          0, &num_lines);
            if (lines)
            {
                for (i = 0; i < num_lines; i++)
                {
                    weechat_printf_y (script_buffer,
                                      script_buffer_detail_script_last_line++,
                                      "%s",
                                      lines[i]);
                }
                weechat_string_free_split (lines);
            }
        }
        if (return_code >= 0)
        {
            weechat_printf_y (script_buffer,
                              script_buffer_detail_script_last_line++,
                              "%s----------------------------------------"
                              "----------------------------------------",
                              weechat_color ("magenta"));
        }
    }

    if ((return_code == WEECHAT_HOOK_PROCESS_ERROR) || (return_code >= 0))
    {
        /* last call to this callback: delete temporary file */
        filename = (char *)pointer;
        unlink (filename);
        free (filename);
    }

    return WEECHAT_RC_OK;
}

/*
 * Shows source code of script (after download of script).
 */

int
script_action_show_source_url_cb (const void *pointer, void *data,
                                  const char *url,
                                  struct t_hashtable *options,
                                  struct t_hashtable *output)
{
    const char *pos_name, *ptr_error, *ptr_diff_command;
    char *filename, *filename_loaded, line[4096], *ptr_line;
    char *diff_command;
    struct t_script_repo *ptr_script;
    FILE *file;
    int length, diff_made;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) options;

    pos_name = strrchr (url, '/');
    if (pos_name)
        pos_name++;

    ptr_error = weechat_hashtable_get (output, "error");
    if (ptr_error && ptr_error[0])
    {
        weechat_printf (NULL,
                        _("%s%s: error downloading script \"%s\": %s"),
                        weechat_prefix ("error"),
                        SCRIPT_PLUGIN_NAME,
                        (pos_name) ? pos_name : "?",
                        ptr_error);
        return WEECHAT_RC_OK;
    }

    if (!pos_name)
    {
        /* silently ignore malformed URL (it should never happen) */
        return WEECHAT_RC_OK;
    }

    ptr_script = script_repo_search_by_name_ext (pos_name);
    if (!ptr_script)
        return WEECHAT_RC_OK;

    filename = script_config_get_script_download_filename (ptr_script,
                                                           ".repository");
    if (!filename)
        return WEECHAT_RC_OK;

    /*
     * read file and display content on script buffer
     * (only if script buffer is still displaying detail of
     * this script)
     */
    if (script_buffer && script_buffer_detail_script
        && (script_buffer_detail_script == ptr_script))
    {
        file = fopen (filename, "r");
        if (file)
        {
            while (!feof (file))
            {
                ptr_line = fgets (line, sizeof (line) - 1, file);
                if (ptr_line)
                {
                    /* remove trailing '\r' and '\n' */
                    length = strlen (line) - 1;
                    while ((length >= 0)
                           && ((line[length] == '\n')
                               || (line[length] == '\r')))
                    {
                        line[length] = '\0';
                        length--;
                    }
                    weechat_printf_y (script_buffer,
                                      script_buffer_detail_script_last_line++,
                                      "%s", ptr_line);
                }
            }
            fclose (file);
        }
        else
        {
            weechat_printf_y (script_buffer,
                              script_buffer_detail_script_last_line++,
                              _("Error: file not found"));
        }
        weechat_printf_y (script_buffer,
                          script_buffer_detail_script_last_line++,
                          "%s----------------------------------------"
                          "----------------------------------------",
                          weechat_color ("lightcyan"));
    }
    diff_made = 0;
    ptr_diff_command = script_config_get_diff_command ();
    if (ptr_diff_command && ptr_diff_command[0]
        && (ptr_script->status & SCRIPT_STATUS_NEW_VERSION))
    {
        /*
         * diff command set => get the diff with a new process,
         * file will be deleted later (in callback of this new
         * process)
         */
        filename_loaded = script_repo_get_filename_loaded (ptr_script);
        if (filename_loaded)
        {
            if (weechat_asprintf (&diff_command,
                                  "%s %s %s",
                                  ptr_diff_command,
                                  filename_loaded,
                                  filename) >= 0)
            {
                script_buffer_detail_script_last_line++;
                script_buffer_detail_script_line_diff = script_buffer_detail_script_last_line;
                weechat_printf_y (script_buffer,
                                  script_buffer_detail_script_last_line++,
                                  "%s", diff_command);
                weechat_printf_y (script_buffer,
                                  script_buffer_detail_script_last_line++,
                                  "%s----------------------------------------"
                                  "----------------------------------------",
                                  weechat_color ("magenta"));
                weechat_hook_process (diff_command, 10000,
                                      &script_action_show_diff_process_cb,
                                      filename, NULL);
                diff_made = 1;
                free (diff_command);
            }
            free (filename_loaded);
        }
    }
    if (!diff_made)
    {
        /* no diff made: delete temporary file now */
        unlink (filename);
        free (filename);
    }

    return WEECHAT_RC_OK;
}

/*
 * Shows detailed info on a script.
 */

void
script_action_run_show (const char *name, int quiet)
{
    struct t_script_repo *ptr_script;
    char *filename;
    struct t_hashtable *options;

    if (!script_download_enabled (1))
        return;

    if (!name)
    {
        script_buffer_show_detail_script (NULL);
        return;
    }

    ptr_script = script_repo_search_by_name_ext (name);
    if (!ptr_script)
    {
        if (!quiet)
        {
            weechat_printf (NULL,
                            _("%s: script \"%s\" not found"),
                            SCRIPT_PLUGIN_NAME, name);
        }
        return;
    }

    script_buffer_show_detail_script (ptr_script);

    if (!weechat_config_boolean (script_config_look_display_source))
        return;

    if (!ptr_script->url)
        return;

    weechat_printf_y (script_buffer,
                      script_buffer_detail_script_last_line++,
                      _("Source code:"));
    weechat_printf_y (script_buffer,
                      script_buffer_detail_script_last_line++,
                      "%s----------------------------------------"
                      "----------------------------------------",
                      weechat_color ("lightcyan"));
    weechat_printf_y (script_buffer,
                      script_buffer_detail_script_last_line,
                      _("Downloading script..."));
    weechat_printf_y (script_buffer,
                      script_buffer_detail_script_last_line + 1,
                      "%s----------------------------------------"
                      "----------------------------------------",
                      weechat_color ("lightcyan"));
    filename = script_config_get_script_download_filename (ptr_script,
                                                           ".repository");
    if (filename)
    {
        options = weechat_hashtable_new (32,
                                         WEECHAT_HASHTABLE_STRING,
                                         WEECHAT_HASHTABLE_STRING,
                                         NULL, NULL);
        if (options)
        {
            weechat_hashtable_set (options, "file_out", filename);
            weechat_hook_url (
                ptr_script->url,
                options,
                weechat_config_integer (script_config_scripts_download_timeout) * 1000,
                &script_action_show_source_url_cb,
                NULL, NULL);
            weechat_hashtable_free (options);
        }
        free (filename);
    }
}

/*
 * Jumps to diff on buffer with detail of script.
 */

void
script_action_run_showdiff (void)
{
    char str_command[64];
    struct t_gui_window *window;
    int diff, start_line_y, chat_height;

    if (script_buffer && script_buffer_detail_script
        && (script_buffer_detail_script_line_diff >= 0))
    {
        /* check if we are already on diff */
        diff = 0;
        window = weechat_window_search_with_buffer (script_buffer);
        if (window)
        {
            script_buffer_get_window_info (window, &start_line_y, &chat_height);
            diff = (start_line_y == script_buffer_detail_script_line_diff);
        }

        /* scroll to top of window */
        weechat_command (script_buffer, "/window scroll_top");

        /* if not currently on diff, jump to it */
        if (!diff)
        {
            snprintf (str_command, sizeof (str_command),
                      "/window scroll %d",
                      script_buffer_detail_script_line_diff);
            weechat_command (script_buffer, str_command);
        }
    }
}

/*
 * Adds an action to list of actions.
 */

void
script_action_add (struct t_gui_buffer *buffer, const char *action)
{
    if (!action)
        return;

    if (!script_actions)
    {
        script_actions = weechat_string_dyn_alloc (256);
        if (!script_actions)
            return;
    }

    if ((*script_actions)[0])
        weechat_string_dyn_concat (script_actions, "\n", -1);

    weechat_string_dyn_concat (
        script_actions, weechat_buffer_get_string (buffer, "full_name"), -1);
    weechat_string_dyn_concat (script_actions, "\r", -1);
    weechat_string_dyn_concat (script_actions, action, -1);
}

/*
 * Clears list of actions.
 */

void
script_action_clear (void)
{
    if (script_actions)
        weechat_string_dyn_copy (script_actions, NULL);
}

/*
 * Runs planned actions.
 *
 * Returns:
 *   1: at least an action was executed
 *   0: no action executed
 */

int
script_action_run_all (void)
{
    char **actions, **argv, **argv_eol, *ptr_action;
    int num_actions, argc, i, j, quiet, script_found;
    struct t_script_repo *ptr_script;
    struct t_gui_buffer *ptr_buffer;

    if (!script_actions || !script_actions[0])
        return 0;

    script_get_loaded_plugins ();

    actions = weechat_string_split (*script_actions, "\n", NULL,
                                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                    0, &num_actions);

    script_action_clear ();

    if (actions)
    {
        for (i = 0; i < num_actions; i++)
        {
            quiet = 0;
            ptr_action = strchr (actions[i], '\r');
            if (!ptr_action)
                continue;
            ptr_action[0] = '\0';
            ptr_action++;
            ptr_buffer = weechat_buffer_search ("==", actions[i]);
            if (!ptr_buffer)
                ptr_buffer = weechat_current_buffer ();
            if (ptr_action[0] == '-')
            {
                /*
                 * if action starts with options (like "-q"),
                 * read and skip them
                 */
                ptr_action++;
                while (ptr_action[0] && (ptr_action[0] != ' '))
                {
                    switch (ptr_action[0])
                    {
                        case 'q': /* quiet */
                            quiet = 1;
                            break;
                    }
                    ptr_action++;
                }
                while (ptr_action[0] == ' ')
                {
                    ptr_action++;
                }
            }
            argv = weechat_string_split (
                ptr_action,
                " ",
                NULL,
                WEECHAT_STRING_SPLIT_STRIP_LEFT
                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                0,
                &argc);
            argv_eol = weechat_string_split (
                ptr_action,
                " ",
                NULL,
                WEECHAT_STRING_SPLIT_STRIP_LEFT
                | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
                | WEECHAT_STRING_SPLIT_KEEP_EOL,
                0,
                &argc);
            if (argv && argv_eol)
            {
                if (weechat_strcmp (argv[0], "buffer") == 0)
                {
                    /* open buffer with list of scripts */
                    if (!script_buffer)
                    {
                        script_buffer_open ();
                        script_buffer_refresh (1);
                    }
                    if (script_buffer_detail_script)
                    {
                        /* back to list of scripts */
                        script_buffer_show_detail_script (NULL);
                    }
                    weechat_buffer_set (script_buffer, "display", "1");
                }
                else if (weechat_strcmp (argv[0], "list") == 0)
                {
                    if (argc > 1)
                    {
                        if (weechat_strcmp (argv[1], "-i") == 0)
                            script_action_run_list_input (ptr_buffer, 0, 0);
                        else if (weechat_strcmp (argv[1], "-il") == 0)
                            script_action_run_list_input (ptr_buffer, 0, 1);
                        else if (weechat_strcmp (argv[1], "-o") == 0)
                            script_action_run_list_input (ptr_buffer, 1, 0);
                        else if (weechat_strcmp (argv[1], "-ol") == 0)
                            script_action_run_list_input (ptr_buffer, 1, 1);
                        else
                            script_action_run_list ();
                    }
                    else
                        script_action_run_list ();
                }
                else if (weechat_strcmp (argv[0], "load") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_run_load (argv[j], quiet);
                    }
                }
                else if (weechat_strcmp (argv[0], "unload") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_run_unload (argv[j], quiet);
                    }
                }
                else if (weechat_strcmp (argv[0], "reload") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_run_reload (argv[j], quiet);
                    }
                }
                else if (weechat_strcmp (argv[0], "autoload") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_run_autoload (argv[j], quiet, 1);
                    }
                }
                else if (weechat_strcmp (argv[0], "noautoload") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_run_autoload (argv[j], quiet, 0);
                    }
                }
                else if (weechat_strcmp (argv[0], "toggleautoload") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_run_autoload (argv[j], quiet, -1);
                    }
                }
                else if (weechat_strcmp (argv[0], "install") == 0)
                {
                    script_found = 0;
                    for (j = 1; j < argc; j++)
                    {
                        ptr_script = script_repo_search_by_name_ext (argv[j]);
                        if (ptr_script)
                        {
                            if (ptr_script->status & SCRIPT_STATUS_HELD)
                            {
                                weechat_printf (NULL,
                                                _("%s: script \"%s\" is held"),
                                                SCRIPT_PLUGIN_NAME, argv[j]);
                            }
                            else if ((ptr_script->status & SCRIPT_STATUS_INSTALLED)
                                     && !(ptr_script->status & SCRIPT_STATUS_NEW_VERSION))
                            {
                                weechat_printf (NULL,
                                                _("%s: script \"%s\" is already "
                                                  "installed and up-to-date"),
                                                SCRIPT_PLUGIN_NAME, argv[j]);
                            }
                            else
                            {
                                script_found++;
                                ptr_script->install_order = script_found;
                            }
                        }
                        else
                        {
                            weechat_printf (NULL,
                                            _("%s: script \"%s\" not found"),
                                            SCRIPT_PLUGIN_NAME, argv[j]);
                        }
                    }
                    if (script_found)
                        script_action_run_install (quiet);
                }
                else if (weechat_strcmp (argv[0], "remove") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_run_remove (argv[j], quiet);
                    }
                }
                else if (weechat_strcmp (argv[0], "installremove") == 0)
                {
                    script_found = 0;
                    for (j = 1; j < argc; j++)
                    {
                        ptr_script = script_repo_search_by_name_ext (argv[j]);
                        if (ptr_script)
                        {
                            if (ptr_script->status & SCRIPT_STATUS_HELD)
                            {
                                weechat_printf (NULL,
                                                _("%s: script \"%s\" is held"),
                                                SCRIPT_PLUGIN_NAME, argv[j]);
                            }
                            else if (ptr_script->status & SCRIPT_STATUS_INSTALLED)
                            {
                                script_action_run_remove (argv[j], quiet);
                            }
                            else
                            {
                                script_found++;
                                ptr_script->install_order = script_found;
                            }
                        }
                        else
                        {
                            weechat_printf (NULL,
                                            _("%s: script \"%s\" not found"),
                                            SCRIPT_PLUGIN_NAME, argv[j]);
                        }
                    }
                    if (script_found)
                        script_action_run_install (quiet);
                }
                else if (weechat_strcmp (argv[0], "hold") == 0)
                {
                    script_found = 0;
                    for (j = 1; j < argc; j++)
                    {
                        if (script_action_run_hold (argv[j], quiet))
                            script_found = 1;
                    }
                    if (script_found)
                        script_buffer_refresh (0);
                }
                else if (weechat_strcmp (argv[0], "show") == 0)
                {
                    if (!script_buffer)
                        script_buffer_open ();
                    script_action_run_show ((argc >= 2) ? argv[1] : NULL,
                                            quiet);
                    weechat_buffer_set (script_buffer, "display", "1");
                }
                else if (weechat_strcmp (argv[0], "showdiff") == 0)
                {
                    script_action_run_showdiff ();
                }
                else if (weechat_strcmp (argv[0], "upgrade") == 0)
                {
                    script_found = 0;
                    for (ptr_script = scripts_repo; ptr_script;
                         ptr_script = ptr_script->next_script)
                    {
                        /*
                         * if script is installed, with new version available,
                         * and not held, then upgrade it
                         */
                        if ((ptr_script->status & SCRIPT_STATUS_INSTALLED)
                            && (ptr_script->status & SCRIPT_STATUS_NEW_VERSION)
                            && !(ptr_script->status & SCRIPT_STATUS_HELD))
                        {
                            script_found++;
                            ptr_script->install_order = script_found;
                        }
                    }
                    if (script_found)
                        script_action_run_install (quiet);
                    else
                    {
                        weechat_printf (NULL,
                                        _("%s: all scripts are up-to-date"),
                                        SCRIPT_PLUGIN_NAME);
                    }
                }
            }
            weechat_string_free_split (argv);
            weechat_string_free_split (argv_eol);
        }
        weechat_string_free_split (actions);
    }

    return 1;
}

/*
 * Schedules an action.
 *
 * If "need_repository" is 1:
 *   - if repository is up-to-date: action is executed
 *   - if "error_repository" is 0: action is executed otherwise the action is
 *     ignored
 * else action is executed.
 */

void
script_action_schedule (struct t_gui_buffer *buffer,
                        const char *action,
                        int need_repository, int error_repository,
                        int quiet)
{
    /* create again "script" directory, just in case it has been removed */
    if (!weechat_mkdir_home ("${weechat_cache_dir}/" SCRIPT_PLUGIN_NAME, 0755))
        return;

    script_action_add (buffer, action);

    if (need_repository)
    {
        if (script_repo_file_is_uptodate ())
        {
            if (!scripts_repo)
                script_repo_file_read (quiet);
            script_action_run_all ();
        }
        else
        {
            if (!error_repository && !script_download_enabled (0))
            {
                /*
                 * the action can be executed even without repository
                 * (example: /script list)
                 */
                script_action_run_all ();
            }
            else if (!script_repo_file_update (quiet))
            {
                script_action_clear ();
            }
        }
    }
    else
        script_action_run_all ();
}

/*
 * Ends script action.
 */

void
script_action_end (void)
{
    if (script_actions)
    {
        weechat_string_dyn_free (script_actions, 1);
        script_actions = NULL;
    }
}
