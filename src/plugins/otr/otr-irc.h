/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_IRC_H
#define WEECHAT_PLUGIN_OTR_IRC_H

extern struct t_gui_buffer *otr_irc_buffer (const char *server,
                                            const char *nick);
extern int otr_irc_target_is_channel (const char *server, const char *target);
extern char *otr_irc_make_account (const char *server);
extern const char *otr_irc_account_server (const char *account);
extern void otr_irc_account_nick (const char *account, char *nick,
                                  size_t size);
extern void otr_irc_send_raw (const char *server, const char *nick,
                              const char *text);

#endif /* WEECHAT_PLUGIN_OTR_IRC_H */
