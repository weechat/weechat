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

/* trigger contribution to built-in themes. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-theme.h"


const char *trigger_theme_light[][2] =
{
    { "trigger.color.flag_command",     "green" },
    { "trigger.color.flag_conditions",  "94" },
    { "trigger.color.flag_post_action", "blue" },
    { "trigger.color.flag_regex",       "cyan" },
    { "trigger.color.flag_return_code", "magenta" },
    { "trigger.color.regex",            "default" },
    { NULL,                             NULL },
};

void
trigger_theme_register (const char *name, const char *entries[][2])
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

void
trigger_theme_init (void)
{
    trigger_theme_register ("light", trigger_theme_light);
}
