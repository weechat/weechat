/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 * SPDX-FileCopyrightText: 2006 Emmanuel Bouthenot <kolter@openics.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_IRC_H
#define WEECHAT_PLUGIN_IRC_H

#define weechat_plugin weechat_irc_plugin
#define IRC_PLUGIN_NAME "irc"
#define IRC_PLUGIN_PRIORITY 6000

extern struct t_weechat_plugin *weechat_irc_plugin;

extern int irc_signal_quit_received;

#endif /* WEECHAT_PLUGIN_IRC_H */
