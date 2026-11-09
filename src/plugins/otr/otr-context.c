/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Per-peer data attached to libotr contexts */

#include <stdlib.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-context.h"


/*
 * Free per-peer data (called by libotr when the context is freed).
 */

void
otr_context_peer_data_free (void *p)
{
    struct t_otr_peer_data *pd = p;

    if (!pd)
        return;
    free (pd->asm_buf);
    free (pd);
}

/*
 * Get per-peer data attached to the libotr master context of a peer; the
 * context and the data are created if needed.
 *
 * Return pointer to per-peer data, NULL if error.
 */

struct t_otr_peer_data *
otr_context_peer_data_for (const char *account, const char *peer)
{
    ConnContext *m;
    struct t_otr_peer_data *pd;

    m = otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                           OTRL_INSTAG_MASTER, 1, NULL, NULL, NULL);
    if (!m)
        return NULL;
    if (!m->app_data)
    {
        pd = calloc (1, sizeof (*pd));
        if (!pd)
            return NULL;
        pd->prev_log = OTR_LOG_UNTOUCHED;
        m->app_data = pd;
        m->app_data_free = otr_context_peer_data_free;
    }
    return m->app_data;
}

/*
 * Get per-peer data for the account and the peer of a libotr context.
 *
 * Return pointer to per-peer data, NULL if error.
 */

struct t_otr_peer_data *
otr_context_peer_data_ctx (ConnContext *c)
{
    return otr_context_peer_data_for (c->accountname, c->username);
}

/*
 * Search the best libotr context for an account and a peer (the context is not
 * created).
 *
 * Return pointer to context, NULL if not found.
 */

ConnContext *
otr_context_find (const char *account, const char *peer)
{
    return otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                              OTRL_INSTAG_BEST, 0, NULL, NULL, NULL);
}

/*
 * Check if a libotr context is encrypted.
 *
 * Return 1 if the context is encrypted, 0 otherwise.
 */

int
otr_context_encrypted (ConnContext *c)
{
    return c && c->msgstate == OTRL_MSGSTATE_ENCRYPTED;
}

/*
 * Check if a libotr context is encrypted and its active fingerprint is trusted.
 *
 * Return 1 if the context is encrypted and verified, 0 otherwise.
 */

int
otr_context_verified (ConnContext *c)
{
    return otr_context_encrypted (c) && c->active_fingerprint
        && otrl_context_is_fingerprint_trusted (c->active_fingerprint);
}
