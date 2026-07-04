/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* script contribution to built-in themes. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-theme.h"


/*
 * script contribution to the "light" theme: option values tuned for a
 * light-background terminal. Each row is { option_full_name, value };
 * the table is NULL-terminated.
 */

const char *script_theme_light[][2] =
{
    { "script.color.status_autoloaded",         "default" },
    { "script.color.status_held",               "202" },
    { "script.color.status_installed",          "21" },
    { "script.color.status_obsolete",           "magenta" },
    { "script.color.status_popular",            "94" },
    { "script.color.status_running",            "23" },
    { "script.color.status_unknown",            "red" },
    { "script.color.text_bg_selected",          "117" },
    { "script.color.text_date",                 "darkgray" },
    { "script.color.text_date_selected",        "darkgray" },
    { "script.color.text_delimiters",           "251" },
    { "script.color.text_description",          "default" },
    { "script.color.text_description_selected", "default" },
    { "script.color.text_extension",            "lightblue" },
    { "script.color.text_extension_selected",   "lightblue" },
    { "script.color.text_name",                 "18" },
    { "script.color.text_name_selected",        "18" },
    { "script.color.text_selected",             "default" },
    { "script.color.text_tags",                 "94" },
    { "script.color.text_tags_selected",        "94" },
    { "script.color.text_version",              "58" },
    { "script.color.text_version_loaded",       "default" },
    { "script.color.text_version_loaded_selected", "default" },
    { "script.color.text_version_selected",     "58" },
    { NULL,                                     NULL },
};

/*
 * Register script's contribution to one theme from a NULL-terminated
 * table of {option, value} rows.
 */

void
script_theme_register (const char *name, const char *entries[][2])
{
    struct t_hashtable *overrides;
    int i;

    if (!name || !entries)
        return;

    overrides = weechat_hashtable_new (32,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL, NULL);
    if (!overrides)
        return;

    for (i = 0; entries[i][0]; i++)
        weechat_hashtable_set (overrides, entries[i][0], entries[i][1]);

    weechat_theme_register (name, overrides);

    weechat_hashtable_free (overrides);
}

/*
 * Register all built-in theme contributions from script.
 */

void
script_theme_init (void)
{
    script_theme_register ("light", script_theme_light);
}
