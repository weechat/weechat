/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_IRC_H
#define WEECHAT_PLUGIN_OTR_IRC_H

extern struct t_gui_buffer *irc_buffer (const char *server, const char *nick);
extern int target_is_channel (const char *server, const char *target);
extern char *make_account (const char *server);
extern const char *account_server (const char *account);
extern void account_nick (const char *account, char *dst, size_t n);
extern void send_raw (const char *server, const char *nick, const char *text);

#endif /* WEECHAT_PLUGIN_OTR_IRC_H */
