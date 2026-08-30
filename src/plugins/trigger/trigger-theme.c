/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Trigger contribution to built-in themes */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "../weechat-plugin.h"
#include "trigger.h"
#include "trigger-theme.h"


/*
 * Trigger contribution to the "light" theme: option values tuned for a
 * light-background terminal. Each row is { option_full_name, value };
 * the table is NULL-terminated.
 */

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

/*
 * Register trigger's contribution to one theme from a NULL-terminated
 * table of {option, value} rows.
 */

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

/*
 * Register all built-in theme contributions from trigger.
 */

void
trigger_theme_init (void)
{
    trigger_theme_register ("light", trigger_theme_light);
}
