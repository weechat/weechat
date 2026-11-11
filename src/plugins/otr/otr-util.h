/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_UTIL_H
#define WEECHAT_PLUGIN_OTR_UTIL_H

extern char *otr_util_dupstr (const char *string);
extern char *otr_util_irc_sanitize (const char *string);
extern char *otr_util_peer_text (const char *text);

#endif /* WEECHAT_PLUGIN_OTR_UTIL_H */
