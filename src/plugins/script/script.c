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
 * script.c: scripts manager for WeeChat
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-buffer.h"
#include "script-command.h"
#include "script-completion.h"
#include "script-config.h"
#include "script-info.h"
#include "script-repo.h"


WEECHAT_PLUGIN_NAME(SCRIPT_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Scripts manager"));
WEECHAT_PLUGIN_AUTHOR("Sebastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_script_plugin = NULL;

char *script_language[]  = { "guile", "lua", "perl", "python", "ruby", "tcl", NULL };
char *script_extension[] = { "scm",   "lua", "pl",   "py",     "rb",   "tcl", NULL };

struct t_hashtable *script_loaded = NULL;
struct t_hook *script_timer_refresh = NULL;


/*
 * script_language_search: search language and return index
 *                         return -1 if not found
 */

int
script_language_search (const char *language)
{
    int i;

    for (i = 0; script_language[i]; i++)
    {
        if (strcmp (script_language[i], language) == 0)
            return i;
    }

    /* language not found */
    return -1;
}

/*
 * script_language_search_by_extension: search language by extension and return
 *                                      index
 *                                      return -1 if not found
 */

int
script_language_search_by_extension (const char *extension)
{
    int i;

    for (i = 0; script_extension[i]; i++)
    {
        if (strcmp (script_extension[i], extension) == 0)
            return i;
    }

    /* extension not found */
    return -1;
}

/*
 * script_get_loaded_scripts: get loaded scripts (in hashtable)
 */

void
script_get_loaded_scripts ()
{
    int i;
    char hdata_name[128], *filename, *ptr_base_name;
    const char *ptr_filename;
    struct t_hdata *hdata;
    void *ptr_script;

    if (!script_loaded)
    {
        script_loaded = weechat_hashtable_new (16,
                                               WEECHAT_HASHTABLE_STRING,
                                               WEECHAT_HASHTABLE_STRING,
                                               NULL,
                                               NULL);
    }
    else
        weechat_hashtable_remove_all (script_loaded);

    for (i = 0; script_language[i]; i++)
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
 * script_debug_dump_cb: callback for "debug_dump" signal
 */

int
script_debug_dump_cb (void *data, const char *signal, const char *type_data,
                      void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (weechat_strcasecmp ((char *)signal_data, SCRIPT_PLUGIN_NAME) == 0))
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
 * script_timer_refresh_cb: callback for timer used to refresh list of scripts
 */

int
script_timer_refresh_cb (void *data, int remaining_calls)
{
    /* make C compiler happy */
    (void) data;

    script_get_loaded_scripts ();
    script_repo_update_status_all ();
    script_buffer_refresh (0);

    if (remaining_calls == 0)
        script_timer_refresh = NULL;

    return WEECHAT_RC_OK;
}

/*
 * script_signal_script_cb: callback for signals "xxx_script_yyy"
 *                          (example: "python_script_loaded")
 */

int
script_signal_script_cb (void *data, const char *signal, const char *type_data,
                         void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    if (weechat_script_plugin->debug >= 2)
        weechat_printf (NULL, "signal: %s, data: %s", signal, (char *)signal_data);

    if (!script_timer_refresh)
    {
        script_timer_refresh = weechat_hook_timer (50, 0, 1,
                                                   &script_timer_refresh_cb, NULL);
    }

    return WEECHAT_RC_OK;
}

/*
 * script_focus_chat_cb: callback called when a mouse action occurs in chat area
 */

struct t_hashtable *
script_focus_chat_cb (void *data, struct t_hashtable *info)
{
    const char *buffer;
    int rc;
    long unsigned int value;
    struct t_gui_buffer *ptr_buffer;
    long x;
    char *error, str_date[64];
    struct t_repo_script *ptr_script;
    struct tm *tm;

    /* make C compiler happy */
    (void) data;

    if (!script_buffer)
        return info;

    buffer = weechat_hashtable_get (info, "_buffer");
    if (!buffer)
        return info;

    rc = sscanf (buffer, "%lx", &value);
    if ((rc == EOF) || (rc == 0))
        return info;

    ptr_buffer = (struct t_gui_buffer *)value;

    if (!ptr_buffer || (ptr_buffer != script_buffer))
        return info;

    if (script_buffer_detail_script)
        ptr_script = script_buffer_detail_script;
    else
    {
        error = NULL;
        x = strtol (weechat_hashtable_get (info, "_chat_line_y"), &error, 10);
        if (!error || error[0])
            return info;

        if (x < 0)
            return info;

        ptr_script = script_repo_search_displayed_by_number (x);
        if (!ptr_script)
            return info;
    }

    weechat_hashtable_set (info, "script_name", ptr_script->name);
    weechat_hashtable_set (info, "script_name_with_extension", ptr_script->name_with_extension);
    weechat_hashtable_set (info, "script_language", script_language[ptr_script->language]);
    weechat_hashtable_set (info, "script_author",ptr_script->author);
    weechat_hashtable_set (info, "script_mail", ptr_script->mail);
    weechat_hashtable_set (info, "script_version", ptr_script->version);
    weechat_hashtable_set (info, "script_license", ptr_script->license);
    weechat_hashtable_set (info, "script_description", ptr_script->description);
    weechat_hashtable_set (info, "script_tags", ptr_script->tags);
    weechat_hashtable_set (info, "script_requirements", ptr_script->requirements);
    weechat_hashtable_set (info, "script_min_weechat", ptr_script->min_weechat);
    weechat_hashtable_set (info, "script_max_weechat", ptr_script->max_weechat);
    weechat_hashtable_set (info, "script_md5sum", ptr_script->md5sum);
    weechat_hashtable_set (info, "script_url", ptr_script->url);
    tm = localtime (&ptr_script->date_added);
    strftime (str_date, sizeof (str_date), "%Y-%m-%d %H:%M:%S", tm);
    weechat_hashtable_set (info, "script_date_added", str_date);
    tm = localtime (&ptr_script->date_updated);
    strftime (str_date, sizeof (str_date), "%Y-%m-%d %H:%M:%S", tm);
    weechat_hashtable_set (info, "script_date_updated", str_date);
    weechat_hashtable_set (info, "script_version_loaded", ptr_script->version_loaded);

    return info;
}

/*
 * weechat_plugin_init: initialize script plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    script_buffer_set_callbacks ();

    if (!script_config_init ())
        return WEECHAT_RC_ERROR;

    if (script_config_read () < 0)
        return WEECHAT_RC_ERROR;

    weechat_mkdir_home (SCRIPT_PLUGIN_NAME, 0755);

    script_command_init ();
    script_completion_init ();
    script_info_init ();

    weechat_hook_signal ("debug_dump", &script_debug_dump_cb, NULL);
    weechat_hook_signal ("window_scrolled", &script_buffer_window_scrolled_cb, NULL);
    weechat_hook_signal ("*_script_*", &script_signal_script_cb, NULL);

    weechat_hook_focus ("chat", &script_focus_chat_cb, NULL);

    if (script_repo_file_exists ())
    {
        if (!script_repo_file_is_uptodate ())
            script_repo_file_update (0);
        else
            script_repo_file_read (0);
    }

    if (script_buffer)
        script_buffer_refresh (1);

    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end script plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    script_config_write ();

    script_repo_remove_all ();

    if (script_repo_filter)
        free (script_repo_filter);

    if (script_loaded)
        weechat_hashtable_free (script_loaded);

    script_config_free ();

    return WEECHAT_RC_OK;
}
