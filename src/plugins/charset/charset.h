/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_CHARSET_H
#define WEECHAT_PLUGIN_CHARSET_H

#define weechat_plugin weechat_charset_plugin
#define CHARSET_PLUGIN_NAME "charset"
#define CHARSET_PLUGIN_PRIORITY 16000

#define CHARSET_CONFIG_NAME "charset"
#define CHARSET_CONFIG_PRIO_NAME (TO_STR(CHARSET_PLUGIN_PRIORITY) "|" CHARSET_CONFIG_NAME)

extern struct t_weechat_plugin *weechat_charset_plugin;

#endif /* WEECHAT_PLUGIN_CHARSET_H */
