/*
 * script-mouse.c - mouse actions for script
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
#include <stdio.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-mouse.h"
#include "script-buffer.h"
#include "script-repo.h"


/*
 * Callback called when a mouse action occurs in chat area.
 */

struct t_hashtable *
script_mouse_focus_chat_cb (const void *pointer, void *data,
                            struct t_hashtable *info)
{
    const char *buffer;
    int rc;
    struct t_gui_buffer *ptr_buffer;
    long x;
    char *error, str_date[64];
    struct t_script_repo *ptr_script;
    struct tm *tm;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!script_buffer)
        return info;

    buffer = weechat_hashtable_get (info, "_buffer");
    if (!buffer)
        return info;

    rc = sscanf (buffer, "%p", &ptr_buffer);
    if ((rc == EOF) || (rc == 0))
        return info;

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
    weechat_hashtable_set (info, "script_sha512sum", ptr_script->sha512sum);
    weechat_hashtable_set (info, "script_url", ptr_script->url);
    tm = localtime (&ptr_script->date_added);
    if (strftime (str_date, sizeof (str_date), "%Y-%m-%d %H:%M:%S", tm) == 0)
        str_date[0] = '\0';
    weechat_hashtable_set (info, "script_date_added", str_date);
    tm = localtime (&ptr_script->date_updated);
    if (strftime (str_date, sizeof (str_date), "%Y-%m-%d %H:%M:%S", tm) == 0)
        str_date[0] = '\0';
    weechat_hashtable_set (info, "script_date_updated", str_date);
    weechat_hashtable_set (info, "script_version_loaded", ptr_script->version_loaded);

    return info;
}

/*
 * Initializes mouse.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
script_mouse_init (void)
{
    struct t_hashtable *keys;

    keys = weechat_hashtable_new (4,
                                  WEECHAT_HASHTABLE_STRING,
                                  WEECHAT_HASHTABLE_STRING,
                                  NULL, NULL);
    if (!keys)
        return 0;

    weechat_hook_focus ("chat", &script_mouse_focus_chat_cb, NULL, NULL);

    weechat_hashtable_set (
        keys,
        "@chat(" SCRIPT_PLUGIN_NAME "." SCRIPT_BUFFER_NAME "):button1",
        "/window ${_window_number};/script -go ${_chat_line_y}");
    weechat_hashtable_set (
        keys,
        "@chat(" SCRIPT_PLUGIN_NAME "." SCRIPT_BUFFER_NAME "):button2",
        "/window ${_window_number};"
        "/script -go ${_chat_line_y};"
        "/script installremove -q ${script_name_with_extension}");
    weechat_hashtable_set (
        keys,
        "@chat(" SCRIPT_PLUGIN_NAME "." SCRIPT_BUFFER_NAME "):wheelup",
        "/script -up 5");
    weechat_hashtable_set (
        keys,
        "@chat(" SCRIPT_PLUGIN_NAME "." SCRIPT_BUFFER_NAME "):wheeldown",
        "/script -down 5");
    weechat_hashtable_set (keys, "__quiet", "1");
    weechat_key_bind ("mouse", keys);

    weechat_hashtable_free (keys);

    return 1;
}

/*
 * Ends mouse.
 */

void
script_mouse_end (void)
{
}
