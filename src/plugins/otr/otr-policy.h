/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_POLICY_H
#define WEECHAT_PLUGIN_OTR_POLICY_H

extern int otr_policy_for (const char *account, const char *peer,
                           const char *name);
extern int otr_policy_ctx (ConnContext *c, const char *name);
extern int otr_policy_no_send_tag (const char *peer);
extern char *otr_policy_format_default (void);
extern char *otr_policy_format_peer (const char *account, const char *peer);

#endif /* WEECHAT_PLUGIN_OTR_POLICY_H */
