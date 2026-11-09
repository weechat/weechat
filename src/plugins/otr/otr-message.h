/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_MESSAGE_H
#define WEECHAT_PLUGIN_OTR_MESSAGE_H

extern int is_ctcp (const char *t);
extern int is_action (const char *t);
extern char *msg_plain_from_irc (const char *t);
extern char *html_escape (const char *in);
extern char *html_filter (const char *in);
extern char *build_in_privmsgs (const char *tags, const char *from,
                                const char *target, const char *msg,
                                const char *line_prefix, int decrypted);

#endif /* WEECHAT_PLUGIN_OTR_MESSAGE_H */
