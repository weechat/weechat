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
 * script-action.c: actions on scripts (load/unload, install/remove, hold, ...)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-action.h"
#include "script-buffer.h"
#include "script-config.h"
#include "script-repo.h"


char *script_actions = NULL;


void script_action_install (int quiet);


/*
 * script_action_list: list loaded scripts (all languages)
 */

void
script_action_list ()
{
    int i, scripts_loaded;
    char hdata_name[128];
    const char *ptr_name;
    struct t_hdata *hdata;
    void *ptr_script;

    weechat_printf (NULL, "");
    weechat_printf (NULL, _("Scripts loaded:"));

    scripts_loaded = 0;

    for (i = 0; script_language[i]; i++)
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
 * script_action_list_input: list loaded scripts (all languages) in input
 *                           (send it to buffer if send_to_buffer == 1)
 */

void
script_action_list_input (int send_to_buffer)
{
    int i, length;
    char hdata_name[128], *buf, str_pos[16];
    struct t_hdata *hdata;
    void *ptr_script;

    buf = malloc (16384);
    if (!buf)
        return;

    buf[0] = '\0';
    length = 0;

    for (i = 0; script_language[i]; i++)
    {
        snprintf (hdata_name, sizeof (hdata_name),
                  "%s_script", script_language[i]);
        hdata = weechat_hdata_get (hdata_name);
        ptr_script = weechat_hdata_get_list (hdata, "scripts");
        while (ptr_script)
        {
            if (buf[0])
                strcat (buf, ", ");
            strcat (buf, weechat_hdata_string (hdata, ptr_script, "name"));
            strcat (buf, ".");
            strcat (buf, script_extension[i]);
            strcat (buf, " ");
            strcat (buf, weechat_hdata_string (hdata, ptr_script, "version"));
            length = strlen (buf);
            if (length > 16384 - 64)
            {
                strcat (buf, "...");
                length += 3;
                break;
            }
            ptr_script = weechat_hdata_move (hdata, ptr_script, 1);
        }
    }

    if (buf[0])
    {
        if (send_to_buffer)
            weechat_command (weechat_current_buffer (), buf);
        else
        {
            weechat_buffer_set (weechat_current_buffer (), "input", buf);
            snprintf (str_pos, sizeof (str_pos), "%d", length);
            weechat_buffer_set (weechat_current_buffer (), "input_pos", str_pos);
        }
    }
}

/*
 * script_action_load: load a script
 */

void
script_action_load (const char *name, int quiet)
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

    /* execute command (for example: "/perl load iset.pl") */
    snprintf (str_command, sizeof (str_command),
              "/%s load %s%s",
              script_language[language],
              (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
              name);
    weechat_command (NULL, str_command);
}

/*
 * script_action_unload: unload a script
 */

void
script_action_unload (const char *name, int quiet)
{
    char *pos, hdata_name[128], *filename, *ptr_base_name, str_command[1024];
    const char *ptr_filename, *ptr_registered_name;
    int language, found, i;
    struct t_hdata *hdata;
    void *ptr_script;

    language = -1;
    pos = strrchr (name, '.');
    if (pos)
    {
        /* unload script by using name + extension (example: "iset.pl") */
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
         * for example with "iset.pl" we should find "iset"
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
        /* unload script by using name (example: "iset") */
        for (i = 0; script_language[i]; i++)
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
 * script_action_reload: reload a script
 */

void
script_action_reload (const char *name, int quiet)
{
    char *pos, hdata_name[128], *filename, *ptr_base_name, str_command[1024];
    const char *ptr_filename, *ptr_registered_name;
    int language, found, i;
    struct t_hdata *hdata;
    void *ptr_script;

    language = -1;
    pos = strrchr (name, '.');
    if (pos)
    {
        /* reload script by using name + extension (example: "iset.pl") */
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
         * for example with "iset.pl" we should find "iset"
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
        /* reload script by using name (example: "iset") */
        for (i = 0; script_language[i]; i++)
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
 * script_action_installnext_timer_cb: callback called to install next script
 */

int
script_action_installnext_timer_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) remaining_calls;

    script_action_install ((data) ? 1 : 0);

    return WEECHAT_RC_OK;
}

/*
 * script_action_install_process_cb: callback called when script is downloaded
 */

int
script_action_install_process_cb (void *data, const char *command,
                                  int return_code, const char *out,
                                  const char *err)
{
    char *pos, *filename, *filename2, str_signal[256];
    int quiet, length;
    struct t_repo_script *ptr_script;

    quiet = (data) ? 1 : 0;

    if (return_code >= 0)
    {
        pos = strrchr (command, '/');

        if ((err && err[0]) || (out && (strncmp (out, "error:", 6) == 0)))
        {
            weechat_printf (NULL,
                            _("%s%s: error downloading script \"%s\": %s"),
                            weechat_prefix ("error"),
                            SCRIPT_PLUGIN_NAME,
                            (pos) ? pos + 1 : "?",
                            (err && err[0]) ? err : out + 6);
            return WEECHAT_RC_OK;
        }

        if (pos)
        {
            ptr_script = script_repo_search_by_name_ext (pos + 1);
            if (ptr_script)
            {
                filename = script_config_get_script_download_filename (ptr_script);
                if (filename)
                {
                    length = 3 + strlen (filename) + 1;
                    filename2 = malloc (length);
                    if (filename2)
                    {
                        snprintf (filename2, length,
                                  "%s%s",
                                  (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
                                  filename);
                        snprintf (str_signal, sizeof (str_signal),
                                  "%s_script_install",
                                  script_language[ptr_script->language]);
                        weechat_hook_signal_send (str_signal,
                                                  WEECHAT_HOOK_SIGNAL_STRING,
                                                  filename2);
                        free (filename2);
                    }
                    free (filename);
                }

                /* schedule install of next script */
                weechat_hook_timer (10, 0, 1,
                                    &script_action_installnext_timer_cb,
                                    (quiet) ? (void *)1 : (void *)0);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * script_action_install: install script(s) marked for install
 */

void
script_action_install (int quiet)
{
    struct t_repo_script *ptr_script, *ptr_script_to_install;
    char *filename, *url;
    int length;
    struct t_hashtable *options;

    ptr_script_to_install = NULL;

    for (ptr_script = repo_scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (ptr_script->install_order > 0)
        {
            if (ptr_script->install_order == 1)
                ptr_script_to_install = ptr_script;
            ptr_script->install_order--;
        }
    }

    if (ptr_script_to_install)
    {
        filename = script_config_get_script_download_filename (ptr_script_to_install);
        if (filename)
        {
            options = weechat_hashtable_new (8,
                                             WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_STRING,
                                             NULL,
                                             NULL);
            if (options)
            {
                length = 4 + strlen (ptr_script_to_install->url) + 1;
                url = malloc (length);
                if (url)
                {
                    if (!weechat_config_boolean (script_config_look_quiet_actions))
                    {
                        weechat_printf (NULL,
                                        _("%s: downloading script \"%s\"..."),
                                        SCRIPT_PLUGIN_NAME,
                                        ptr_script_to_install->name_with_extension);
                    }

                    snprintf (url, length, "url:%s",
                              ptr_script_to_install->url);
                    weechat_hashtable_set (options, "file_out", filename);
                    weechat_hook_process_hashtable (url, options, 30000,
                                                    &script_action_install_process_cb,
                                                    (quiet) ? (void *)1 : (void *)0);
                    free (url);
                }
                weechat_hashtable_free (options);
            }
            free (filename);
        }
    }
}

/*
 * script_action_remove: remove a script
 */

void
script_action_remove (const char *name, int quiet)
{
    struct t_repo_script *ptr_script;
    char str_signal[256], *filename;
    int length;

    ptr_script = script_repo_search_by_name_ext (name);
    if (ptr_script)
    {
        if (!(ptr_script->status & SCRIPT_STATUS_INSTALLED))
        {
            if (!quiet)
            {
                weechat_printf (NULL,
                                _("%s: script \"%s\" is not installed"),
                                SCRIPT_PLUGIN_NAME, name);
            }
        }
        else if (ptr_script->status & SCRIPT_STATUS_HELD)
        {
            if (!quiet)
            {
                weechat_printf (NULL,
                                _("%s: script \"%s\" is held"),
                                SCRIPT_PLUGIN_NAME, name);
            }
        }
        else
        {
            length = 3 + strlen (ptr_script->name_with_extension) + 1;
            filename = malloc (length);
            if (filename)
            {
                snprintf (filename, length,
                          "%s%s",
                          (quiet && weechat_config_boolean (script_config_look_quiet_actions)) ? "-q " : "",
                          ptr_script->name_with_extension);
                snprintf (str_signal, sizeof (str_signal),
                          "%s_script_remove",
                          script_language[ptr_script->language]);
                weechat_hook_signal_send (str_signal,
                                          WEECHAT_HOOK_SIGNAL_STRING,
                                          filename);
                free (filename);
            }
        }
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
}

/*
 * script_action_hold: (un)hold a script
 *                     return 1 if ok, 0 if error
 */

int
script_action_hold (const char *name, int quiet)
{
    struct t_repo_script *ptr_script;

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
                                  "held any more"),
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
 * script_action_show: show detailed info on a script
 */

void
script_action_show (const char *name, int quiet)
{
    struct t_repo_script *ptr_script;

    if (name)
    {
        ptr_script = script_repo_search_by_name_ext (name);
        if (ptr_script)
        {
            script_buffer_show_detail_script (ptr_script);
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
    }
    else
        script_buffer_show_detail_script (NULL);
}

/*
 * script_action_run: run planned actions
 *                    return 1 if an action was executed, otherwise 0
 */

int
script_action_run ()
{
    char **actions, **argv, **argv_eol, *ptr_action;
    int num_actions, argc, i, j, quiet, script_found;
    struct t_repo_script *ptr_script;

    if (!script_actions)
        return 0;

    actions = weechat_string_split (script_actions, "\n", 0, 0, &num_actions);
    if (actions)
    {
        for (i = 0; i < num_actions; i++)
        {
            quiet = 0;
            ptr_action = actions[i];
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
            argv = weechat_string_split (ptr_action, " ", 0, 0, &argc);
            argv_eol = weechat_string_split (ptr_action, " ", 1, 0, &argc);
            if (argv && argv_eol)
            {
                if (weechat_strcasecmp (argv[0], "buffer") == 0)
                {
                    /* open buffer with list of scripts */
                    if (!script_buffer)
                    {
                        script_buffer_open ();
                        script_buffer_refresh (1);
                    }
                    weechat_buffer_set (script_buffer, "display", "1");
                }
                else if (weechat_strcasecmp (argv[0], "list") == 0)
                {
                    if (argc > 1)
                    {
                        if (weechat_strcasecmp (argv[1], "-i") == 0)
                            script_action_list_input (0);
                        else if (weechat_strcasecmp (argv[1], "-o") == 0)
                            script_action_list_input (1);
                        else
                            script_action_list ();
                    }
                    else
                        script_action_list ();
                }
                else if (weechat_strcasecmp (argv[0], "load") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_load (argv[j], quiet);
                    }
                }
                else if (weechat_strcasecmp (argv[0], "unload") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_unload (argv[j], quiet);
                    }
                }
                else if (weechat_strcasecmp (argv[0], "reload") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_reload (argv[j], quiet);
                    }
                }
                else if (weechat_strcasecmp (argv[0], "install") == 0)
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
                        script_action_install (quiet);
                }
                else if (weechat_strcasecmp (argv[0], "remove") == 0)
                {
                    for (j = 1; j < argc; j++)
                    {
                        script_action_remove (argv[j], quiet);
                    }
                }
                else if (weechat_strcasecmp (argv[0], "installremove") == 0)
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
                                script_action_remove (argv[j], quiet);
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
                        script_action_install (quiet);
                }
                else if (weechat_strcasecmp (argv[0], "hold") == 0)
                {
                    script_found = 0;
                    for (j = 1; j < argc; j++)
                    {
                        if (script_action_hold (argv[j], quiet))
                            script_found = 1;
                    }
                    if (script_found)
                        script_buffer_refresh (0);
                }
                else if (weechat_strcasecmp (argv[0], "show") == 0)
                {
                    if (!script_buffer)
                        script_buffer_open ();
                    script_action_show ((argc >= 2) ? argv[1] : NULL,
                                        quiet);
                    weechat_buffer_set (script_buffer, "display", "1");
                }
                else if (weechat_strcasecmp (argv[0], "upgrade") == 0)
                {
                    script_found = 0;
                    for (ptr_script = repo_scripts; ptr_script;
                         ptr_script = ptr_script->next_script)
                    {
                        /*
                         * if script is intalled, with new version available,
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
                        script_action_install (quiet);
                    else
                    {
                        weechat_printf (NULL,
                                        _("%s: all scripts are up-to-date"),
                                        SCRIPT_PLUGIN_NAME);
                    }
                }
            }
            if (argv)
                weechat_string_free_split (argv);
            if (argv_eol)
                weechat_string_free_split (argv_eol);
        }
        weechat_string_free_split (actions);
    }

    free (script_actions);
    script_actions = NULL;

    return 1;
}


/*
 * script_action_add: add an action to list of actions
 */

void
script_action_add (const char *action)
{
    char *new_actions;

    if (!action)
        return;

    if (script_actions)
    {
        new_actions = realloc (script_actions,
                               strlen (script_actions) + 1 + strlen (action) + 1);
        if (!new_actions)
            return;
        script_actions = new_actions;
        strcat (script_actions, "\n");
        strcat (script_actions, action);
    }
    else
    {
        script_actions = strdup (action);
    }
}

/*
 * script_action_schedule: schedule action
 *                         if "need_repository" is 1, then the action will be
 *                         executed only when the repository file is up-to-date
 */

void
script_action_schedule (const char *action, int need_repository, int quiet)
{
    script_action_add (action);

    if (need_repository)
    {
        if (script_repo_file_is_uptodate ())
        {
            if (!repo_scripts)
                script_repo_file_read (quiet);
            script_action_run ();
        }
        else
            script_repo_file_update (quiet);
    }
    else
        script_action_run ();
}
