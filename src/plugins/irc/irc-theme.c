/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* IRC plugin contribution to built-in themes */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-theme.h"


/*
 * IRC contribution to the "light" theme: option values tuned for a
 * light-background terminal. Each row is { option_full_name, value };
 * the table is NULL-terminated.
 */

const char *irc_theme_light[][2] =
{
    { "irc.color.input_nick",                  "cyan" },
    { "irc.color.item_lag_finished",           "94" },
    { "irc.color.item_tls_version_deprecated", "202" },
    { "irc.color.list_buffer_line_selected",   "default" },
    { "irc.color.message_chghost",             "94" },
    { "irc.color.message_setname",             "94" },
    { "irc.color.nick_prefixes",
      "y:red;q:red;a:cyan;o:green;h:magenta;v:94;*:blue" },
    { "irc.color.topic_new",                   "28" },
    { "irc.color.topic_old",                   "darkgray" },
    { NULL,                                    NULL },
};

/*
 * Register IRC's contribution to one theme from a NULL-terminated
 * table of {option, value} rows.
 */

void
irc_theme_register (const char *name, const char *entries[][2])
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
 * Register all built-in theme contributions from IRC.
 */

void
irc_theme_init (void)
{
    irc_theme_register ("light", irc_theme_light);
}
