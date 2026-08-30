/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Xfer contribution to built-in themes */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-theme.h"


/*
 * Xfer contribution to the "light" theme: option values tuned for a
 * light-background terminal. Each row is { option_full_name, value };
 * the table is NULL-terminated.
 */

const char *xfer_theme_light[][2] =
{
    { "xfer.color.status_aborted",    "red" },
    { "xfer.color.status_active",     "blue" },
    { "xfer.color.status_connecting", "202" },
    { "xfer.color.status_done",       "green" },
    { "xfer.color.status_failed",     "red" },
    { "xfer.color.status_waiting",    "cyan" },
    { "xfer.color.text_selected",     "default" },
    { NULL,                           NULL },
};

/*
 * Register xfer's contribution to one theme from a NULL-terminated
 * table of {option, value} rows.
 */

void
xfer_theme_register (const char *name, const char *entries[][2])
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
 * Register all built-in theme contributions from xfer.
 */

void
xfer_theme_init (void)
{
    xfer_theme_register ("light", xfer_theme_light);
}
