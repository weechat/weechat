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

/* fset contribution to built-in themes. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "../weechat-plugin.h"
#include "fset.h"
#include "fset-theme.h"


/*
 * fset contribution to the "light" theme: option values tuned for a
 * light-background terminal. Each row is { option_full_name, value };
 * the table is NULL-terminated.
 */

const char *fset_theme_light[][2] =
{
    { "fset.color.allowed_values_selected",  "default" },
    { "fset.color.color_name",               "darkgray" },
    { "fset.color.color_name_selected",      "darkgray" },
    { "fset.color.default_value_selected",   "default" },
    { "fset.color.description_selected",     "242" },
    { "fset.color.file_changed",             "94" },
    { "fset.color.file_changed_selected",    "94" },
    { "fset.color.file_selected",            "default" },
    { "fset.color.help_default_value",       "default" },
    { "fset.color.help_name",                "default" },
    { "fset.color.index",                    "24" },
    { "fset.color.index_selected",           "24" },
    { "fset.color.line_marked_bg1",          "193" },
    { "fset.color.line_marked_bg2",          "193" },
    { "fset.color.line_selected_bg1",        "117" },
    { "fset.color.line_selected_bg2",        "117" },
    { "fset.color.marked",                   "94" },
    { "fset.color.marked_selected",          "94" },
    { "fset.color.max_selected",             "default" },
    { "fset.color.min_selected",             "default" },
    { "fset.color.name_changed",             "red" },
    { "fset.color.name_changed_selected",    "red" },
    { "fset.color.name_selected",            "default" },
    { "fset.color.option_changed",           "94" },
    { "fset.color.option_changed_selected",  "94" },
    { "fset.color.option_selected",          "default" },
    { "fset.color.parent_name_selected",     "default" },
    { "fset.color.parent_value",             "24" },
    { "fset.color.parent_value_selected",    "24" },
    { "fset.color.quotes_changed_selected",  "default" },
    { "fset.color.section_changed",          "94" },
    { "fset.color.section_changed_selected", "94" },
    { "fset.color.section_selected",         "default" },
    { "fset.color.string_values_selected",   "default" },
    { "fset.color.title_count_options",      "30" },
    { "fset.color.title_current_option",     "30" },
    { "fset.color.title_filter",             "18" },
    { "fset.color.title_marked_options",     "94" },
    { "fset.color.title_sort",               "darkgray" },
    { "fset.color.type",                     "58" },
    { "fset.color.type_selected",            "58" },
    { "fset.color.unmarked_selected",        "default" },
    { "fset.color.value",                    "20" },
    { "fset.color.value_changed",            "red" },
    { "fset.color.value_changed_selected",   "red" },
    { "fset.color.value_selected",           "20" },
    { "fset.color.value_undef_selected",     "magenta" },
    { NULL,                                  NULL },
};

/*
 * Registers fset's contribution to one theme from a NULL-terminated
 * table of {option, value} rows.
 */

void
fset_theme_register (const char *name, const char *entries[][2])
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
 * Registers all built-in theme contributions from fset.
 */

void
fset_theme_init (void)
{
    fset_theme_register ("light", fset_theme_light);
}
