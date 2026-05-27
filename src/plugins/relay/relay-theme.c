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

/* relay contribution to built-in themes. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-theme.h"


const char *relay_theme_light[][2] =
{
    { "relay.color.status_auth_failed",     "magenta" },
    { "relay.color.status_authenticating",  "202" },
    { "relay.color.status_connecting",      "default" },
    { "relay.color.status_disconnected",    "red" },
    { "relay.color.text_selected",          "default" },
    { NULL,                                 NULL },
};

void
relay_theme_register (const char *name, const char *entries[][2])
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
relay_theme_init (void)
{
    relay_theme_register ("light", relay_theme_light);
}
