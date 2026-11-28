/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_MESSAGE_H
#define WEECHAT_PLUGIN_OTR_MESSAGE_H

extern int otr_message_is_ctcp (const char *message);
extern int otr_message_is_action (const char *message);
extern char *otr_message_plain_from_irc (const char *message);
extern char *otr_message_html_escape (const char *string);
extern char *otr_message_html_filter (const char *html);
extern char *otr_message_get_tags (const char *message);
extern char *otr_message_build_in_privmsgs (const char *tags, const char *from,
                                            const char *target,
                                            const char *message,
                                            const char *line_prefix,
                                            int decrypted);

#endif /* WEECHAT_PLUGIN_OTR_MESSAGE_H */
