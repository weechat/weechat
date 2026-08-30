/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Relay contribution to built-in themes */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-theme.h"


/*
 * Relay contribution to the "light" theme: option values tuned for a
 * light-background terminal. Each row is { option_full_name, value };
 * the table is NULL-terminated.
 */

const char *relay_theme_light[][2] =
{
    { "relay.color.status_auth_failed",     "magenta" },
    { "relay.color.status_authenticating",  "202" },
    { "relay.color.status_connecting",      "default" },
    { "relay.color.status_disconnected",    "red" },
    { "relay.color.text_selected",          "default" },
    { NULL,                                 NULL },
};

/*
 * Register relay's contribution to one theme from a NULL-terminated
 * table of {option, value} rows.
 */

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

/*
 * Register all built-in theme contributions from relay.
 */

void
relay_theme_init (void)
{
    relay_theme_register ("light", relay_theme_light);
}
