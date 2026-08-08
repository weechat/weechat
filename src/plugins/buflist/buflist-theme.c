/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* buflist contribution to built-in themes. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-theme.h"


/*
 * buflist contribution to the "light" theme: format strings tuned for
 * a light-background terminal. Each row is { option_full_name, value };
 * the table is NULL-terminated.
 */

const char *buflist_theme_light[][2] =
{
    { "buflist.format.buffer_current",
      "${color:,117}${format_buffer}" },
    { "buflist.format.hotlist_low",
      "${color:default}" },
    { "buflist.format.hotlist_message",
      "${color:94}" },
    { "buflist.format.lag",
      " ${color:green}[${color:94}${lag}${color:green}]" },
    { "buflist.format.number",
      "${color:28}${number}${if:${number_displayed}?.: }" },
    { NULL, NULL },
};

/*
 * Register buflist's contribution to one theme from a NULL-terminated
 * table of {option, value} rows.
 */

void
buflist_theme_register (const char *name, const char *entries[][2])
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
 * Register all built-in theme contributions from buflist.
 */

void
buflist_theme_init (void)
{
    buflist_theme_register ("light", buflist_theme_light);
}
