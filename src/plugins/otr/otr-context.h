/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_CONTEXT_H
#define WEECHAT_PLUGIN_OTR_CONTEXT_H

struct peer_data
{
    char *asm_buf;     /* reassembly of OTR messages split over IRC lines */
    size_t asm_len;
    int in_smp;
    int smp_question;
    int prev_log;      /* LOG_UNTOUCHED / LOG_DEFAULT / level */
};

extern struct peer_data *peer_data_for (const char *account, const char *peer);
extern struct peer_data *peer_data_ctx (ConnContext *c);
extern ConnContext *find_ctx (const char *account, const char *peer);
extern int ctx_encrypted (ConnContext *c);
extern int ctx_verified (ConnContext *c);

#endif /* WEECHAT_PLUGIN_OTR_CONTEXT_H */
