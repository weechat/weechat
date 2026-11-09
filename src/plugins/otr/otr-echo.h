/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_ECHO_H
#define WEECHAT_PLUGIN_OTR_ECHO_H

extern void otr_echo_pending_clear_all (void);
extern void otr_echo_pending_add (const char *server, const char *peer,
                                  const char *cipher, const char *plain);
extern char *otr_echo_replace (const char *server, const char *string,
                               const char *host, const char *target,
                               const char *text);

#endif /* WEECHAT_PLUGIN_OTR_ECHO_H */
