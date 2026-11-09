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


static void
peer_data_free (void *p)
{
    struct peer_data *pd = p;

    if (!pd)
        return;
    free (pd->asm_buf);
    free (pd);
}

struct peer_data *
peer_data_for (const char *account, const char *peer)
{
    ConnContext *m;
    struct peer_data *pd;

    m = otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                           OTRL_INSTAG_MASTER, 1, NULL, NULL, NULL);
    if (!m)
        return NULL;
    if (!m->app_data)
    {
        pd = calloc (1, sizeof (*pd));
        if (!pd)
            return NULL;
        pd->prev_log = LOG_UNTOUCHED;
        m->app_data = pd;
        m->app_data_free = peer_data_free;
    }
    return m->app_data;
}

struct peer_data *
peer_data_ctx (ConnContext *c)
{
    return peer_data_for (c->accountname, c->username);
}

ConnContext *
find_ctx (const char *account, const char *peer)
{
    return otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                              OTRL_INSTAG_BEST, 0, NULL, NULL, NULL);
}

int
ctx_encrypted (ConnContext *c)
{
    return c && c->msgstate == OTRL_MSGSTATE_ENCRYPTED;
}

int
ctx_verified (ConnContext *c)
{
    return ctx_encrypted (c) && c->active_fingerprint
        && otrl_context_is_fingerprint_trusted (c->active_fingerprint);
}
