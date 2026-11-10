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
    char *assembly;                    /* reassembly of OTR messages split  */
                                       /* over several IRC lines            */
    size_t assembly_length;            /* length of assembly                */
    int in_smp;                        /* 1 if SMP authentication running   */
    int smp_question;                  /* 1 if peer asked an SMP question   */
    int smp_asked;                     /* 1 if peer asked SMP verification, */
                                       /* waiting for our response          */
    int prev_log;                      /* logger level before OTR session:  */
                                       /* OTR_LOG_UNTOUCHED,                */
                                       /* OTR_LOG_DEFAULT or level          */
};

extern struct t_otr_peer_data *otr_context_peer_data_for (const char *account,
                                                          const char *peer);
extern struct t_otr_peer_data *otr_context_peer_data_ctx (ConnContext *context);
extern ConnContext *otr_context_find (const char *account, const char *peer);
extern int otr_context_encrypted (ConnContext *context);
extern int otr_context_verified (ConnContext *context);
extern int otr_context_smp_asked (ConnContext *context);
extern void otr_context_smp_reset (struct t_otr_peer_data *peer_data);
extern int otr_context_end_session (const char *account, const char *peer,
                                    int display, int buffer_closing);

#endif /* WEECHAT_PLUGIN_OTR_CONTEXT_H */
