/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_CONTEXT_H
#define WEECHAT_PLUGIN_OTR_CONTEXT_H

struct t_otr_peer_data
{
    char *asm_buf;     /* reassembly of OTR messages split over IRC lines */
    size_t asm_len;
    int in_smp;
    int smp_question;
    int prev_log;      /* OTR_LOG_UNTOUCHED / OTR_LOG_DEFAULT / level */
};

extern struct t_otr_peer_data *otr_context_peer_data_for (const char *account,
                                                          const char *peer);
extern struct t_otr_peer_data *otr_context_peer_data_ctx (ConnContext *c);
extern ConnContext *otr_context_find (const char *account, const char *peer);
extern int otr_context_encrypted (ConnContext *c);
extern int otr_context_verified (ConnContext *c);

#endif /* WEECHAT_PLUGIN_OTR_CONTEXT_H */
