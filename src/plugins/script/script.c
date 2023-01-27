/*
 * script.c - script manager for WeeChat
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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-action.h"
#include "script-buffer.h"
#include "script-command.h"
#include "script-completion.h"
#include "script-config.h"
#include "script-info.h"
#include "script-mouse.h"
#include "script-repo.h"


WEECHAT_PLUGIN_NAME(SCRIPT_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Script manager"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(SCRIPT_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_script_plugin = NULL;

char *script_language[SCRIPT_NUM_LANGUAGES] =
{ "guile", "lua", "perl", "python", "ruby", "tcl", "javascript", "php" };
char *script_extension[SCRIPT_NUM_LANGUAGES] =
{ "scm",   "lua", "pl",   "py",     "rb",   "tcl", "js", "php" };

int script_plugin_loaded[SCRIPT_NUM_LANGUAGES];
struct t_hashtable *script_loaded = NULL;
struct t_hook *script_timer_refresh = NULL;


/*
 * Searches for a language.
 *
 * Returns index of language, -1 if not found.
 */

int
script_language_search (const char *language)
{
    int i;

    if (!language)
        return -1;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        if (strcmp (script_language[i], language) == 0)
            return i;
    }

    /* language not found */
    return -1;
}

/*
 * Searches for a language by extension.
 *
 * Returns index of language, -1 if not found.
 */

int
script_language_search_by_extension (const char *extension)
{
    int i;

    if (!extension)
        return -1;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        if (strcmp (script_extension[i], extension) == 0)
            return i;
    }

    /* extension not found */
    return -1;
}

/*
 * Checks if download of scripts is enabled.
 *
 * Returns:
 *   0: download NOT enabled (an error is displayed if display_error is 1)
 *   1: download enabled
 */

int
script_download_enabled (int display_error)
{
    if (weechat_config_boolean (script_config_scripts_download_enabled))
        return 1;

    if (display_error)
    {
        /* download not enabled: display an error */
        weechat_printf (NULL,
                        _("%s%s: download of scripts is disabled by default; "
                          "see /help script.scripts.download_enabled"),
                        weechat_prefix ("error"),
                        SCRIPT_PLUGIN_NAME);
    }
    return 0;
}

/*
 * Builds download URL (to use with hook_process or hook_process_hashtable).
 *
 * Note: result must be freed after use.
 */

char *
script_build_download_url (const char *url)
{
    char *result;
    int length;

    if (!url || !url[0])
        return NULL;

    /* length of url + "url:" */
    length = 4 + strlen (url) + 1;
    result = malloc (length);
    if (!result)
        return NULL;

    snprintf (result, length, "url:%s", url);

    return result;
}

/*
 * Gets loaded plugins (in array of integers).
 */

void
script_get_loaded_plugins ()
{
    int i, language;
    struct t_hdata *hdata;
    void *ptr_plugin;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        script_plugin_loaded[i] = 0;
    }
    hdata = weechat_hdata_get ("plugin");
    ptr_plugin = weechat_hdata_get_list (hdata, "weechat_plugins");
    while (ptr_plugin)
    {
        language = script_language_search (weechat_hdata_string (hdata,
                                                                 ptr_plugin,
                                                                 "name"));
        if (language >= 0)
            script_plugin_loaded[language] = 1;
        ptr_plugin = weechat_hdata_move (hdata, ptr_plugin, 1);
    }
}

/*
 * Gets scripts (in hashtable).
 */

void
script_get_scripts ()
{
    int i;
    char hdata_name[128], *filename, *ptr_base_name;
    const char *ptr_filename;
    struct t_hdata *hdata;
    void *ptr_script;

    if (!script_loaded)
    {
        script_loaded = weechat_hashtable_new (32,
                                               WEECHAT_HASHTABLE_STRING,
                                               WEECHAT_HASHTABLE_STRING,
                                               NULL, NULL);
    }
    else
        weechat_hashtable_remove_all (script_loaded);

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        snprintf (hdata_name, sizeof (hdata_name),
                  "%s_script", script_language[i]);
        hdata = weechat_hdata_get (hdata_name);
        ptr_script = weechat_hdata_get_list (hdata, "scripts");
        while (ptr_script)
        {
            ptr_filename = weechat_hdata_string (hdata, ptr_script, "filename");
            if (ptr_filename)
            {
                filename = strdup (ptr_filename);
                if (filename)
                {
                    ptr_base_name = basename (filename);
                    weechat_hashtable_set (script_loaded,
                                           ptr_base_name,
                                           weechat_hdata_string (hdata, ptr_script,
                                                                 "version"));
                    free (filename);
                }
            }
            ptr_script = weechat_hdata_move (hdata, ptr_script, 1);
        }
    }
}

/*
 * Callback for signal "debug_dump".
 */

int
script_debug_dump_cb (const void *pointer, void *data,
                      const char *signal, const char *type_data,
                      void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, SCRIPT_PLUGIN_NAME) == 0))
    {
        weechat_log_printf ("");
        weechat_log_printf ("***** \"%s\" plugin dump *****",
                            weechat_plugin->name);

        script_repo_print_log ();

        weechat_log_printf ("");
        weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                            weechat_plugin->name);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for timer to refresh list of scripts.
 */

int
script_timer_refresh_cb (const void *pointer, void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    script_get_loaded_plugins ();
    script_get_scripts ();
    script_repo_update_status_all ();
    script_buffer_refresh (0);

    if (remaining_calls == 0)
        script_timer_refresh = NULL;

    return WEECHAT_RC_OK;
}

/*
 * Callback for signals "plugin_loaded" and "plugin_unloaded".
 */

int
script_signal_plugin_cb (const void *pointer, void *data,
                         const char *signal, const char *type_data,
                         void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) type_data;

    if (weechat_script_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: signal: %s, data: %s",
                        SCRIPT_PLUGIN_NAME,
                        signal, (char *)signal_data);
    }

    if (!script_timer_refresh)
    {
        script_timer_refresh = weechat_hook_timer (50, 0, 1,
                                                   &script_timer_refresh_cb,
                                                   NULL, NULL);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for signals "xxx_script_yyy" (example: "python_script_loaded").
 */

int
script_signal_script_cb (const void *pointer, void *data,
                         const char *signal, const char *type_data,
                         void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) type_data;

    if (weechat_script_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: signal: %s, data: %s",
                        SCRIPT_PLUGIN_NAME,
                        signal, (char *)signal_data);
    }

    if (!script_timer_refresh)
    {
        script_timer_refresh = weechat_hook_timer (50, 0, 1,
                                                   &script_timer_refresh_cb,
                                                   NULL, NULL);
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes script plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int i;

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    for (i = 0; i < SCRIPT_NUM_LANGUAGES; i++)
    {
        script_plugin_loaded[i] = 0;
    }

    script_buffer_set_callbacks ();

    if (!script_config_init ())
        return WEECHAT_RC_ERROR;

    script_config_read ();

    weechat_mkdir_home ("${weechat_cache_dir}/" SCRIPT_PLUGIN_NAME, 0755);

    script_command_init ();
    script_completion_init ();
    script_info_init ();

    weechat_hook_signal ("debug_dump",
                         &script_debug_dump_cb, NULL, NULL);
    weechat_hook_signal ("window_scrolled",
                         &script_buffer_window_scrolled_cb, NULL, NULL);
    weechat_hook_signal ("plugin_*",
                         &script_signal_plugin_cb, NULL, NULL);
    weechat_hook_signal ("*_script_*",
                         &script_signal_script_cb, NULL, NULL);

    script_mouse_init ();

    if (script_repo_file_exists ())
        script_repo_file_read (0);

    if (script_buffer)
        script_buffer_refresh (1);

    return WEECHAT_RC_OK;
}

/*
 * Ends script plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    script_mouse_end ();

    script_config_write ();

    script_repo_remove_all ();

    if (script_repo_filter)
        free (script_repo_filter);

    if (script_loaded)
        weechat_hashtable_free (script_loaded);

    script_config_free ();

    script_action_end ();

    return WEECHAT_RC_OK;
}
