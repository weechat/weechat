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

/* Built-in theme registrations (core contribution only). */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>

#include "weechat.h"
#include "core-hashtable.h"
#include "core-theme.h"
#include "../plugins/weechat-plugin.h"


/*
 * Core overrides for the "light" theme: option values tuned for a
 * light-background terminal. Order is by full option name to keep diffs
 * stable; the list ends with a NULL sentinel.
 */

struct t_theme_builtin_entry
{
    const char *option;
    const char *value;
};

struct t_theme_builtin_entry theme_builtin_light_core[] =
{
    { "weechat.bar.status.color_bg",           "254"        },
    { "weechat.bar.status.color_bg_inactive",  "default"    },
    { "weechat.bar.title.color_bg",            "254"        },
    { "weechat.bar.title.color_bg_inactive",   "default"    },
    { "weechat.color.bar_more",                "magenta"    },
    { "weechat.color.chat_buffer",             "default"    },
    { "weechat.color.chat_channel",            "default"    },
    { "weechat.color.chat_nick",               "cyan"       },
    { "weechat.color.chat_nick_colors",
      "red,green,brown,blue,magenta,cyan,lightred,lightblue,lightmagenta,"
      "20,28,52,57,58,61,63,88,94,128,166,202" },
    { "weechat.color.chat_nick_self",          "default"    },
    { "weechat.color.chat_prefix_action",      "default"    },
    { "weechat.color.chat_prefix_error",       "94"         },
    { "weechat.color.chat_prefix_join",        "green"      },
    { "weechat.color.chat_prefix_more",        "magenta"    },
    { "weechat.color.chat_prefix_quit",        "red"        },
    { "weechat.color.chat_prefix_suffix",      "251"        },
    { "weechat.color.chat_server",             "94"         },
    { "weechat.color.chat_text_found_bg",      "magenta"    },
    { "weechat.color.chat_time_delimiters",    "94"         },
    { "weechat.color.eval_syntax_colors",
      "green,red,blue,magenta,94,cyan" },
    { "weechat.color.input_actions",           "28"         },
    { "weechat.color.item_away",               "brown"      },
    { "weechat.color.separator",               "251"        },
    { "weechat.color.status_count_msg",        "94"         },
    { "weechat.color.status_data_highlight",   "93"         },
    { "weechat.color.status_data_msg",         "94"         },
    { "weechat.color.status_data_private",     "green"      },
    { "weechat.color.status_more",             "94"         },
    { "weechat.color.status_mouse",            "green"      },
    { "weechat.color.status_name",             "default"    },
    { "weechat.color.status_name_insecure",    "202"        },
    { "weechat.color.status_name_tls",         "default"    },
    { "weechat.color.status_number",           "28"         },
    { NULL,                                    NULL         },
};

/*
 * Builds a hashtable of overrides from a NULL-terminated table and
 * registers it under the given theme name.
 */

void
theme_builtin_register_entries (const char *name,
                                const struct t_theme_builtin_entry *entries)
{
    struct t_hashtable *overrides;
    int i;

    if (!name || !entries)
        return;

    overrides = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL, NULL);
    if (!overrides)
        return;

    for (i = 0; entries[i].option; i++)
        hashtable_set (overrides, entries[i].option, entries[i].value);

    theme_register (NULL, NULL, name, overrides);

    hashtable_free (overrides);
}

/*
 * Registers all built-in themes contributed by core. Called once from
 * theme_init; plugins/scripts add their own contributions later via
 * weechat_theme_register.
 */

void
theme_builtin_init (void)
{
    theme_builtin_register_entries ("light", theme_builtin_light_core);
}
