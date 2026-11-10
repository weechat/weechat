/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR callbacks for libotr */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-callback.h"
#include "src/plugins/otr/otr-context.h"

extern void otr_callback_handle_smp_event (void *opdata, OtrlSMPEvent event,
                                           ConnContext *context,
                                           unsigned short progress,
                                           char *question);
}

TEST_GROUP(OtrCallback)
{
};

/*
 * Test functions:
 *   otr_callback_handle_smp_event
 */

TEST(OtrCallback, HandleSmpEvent)
{
    ConnContext *ptr_context;
    Fingerprint *ptr_fingerprint;
    struct t_otr_peer_data *ptr_peer_data;
    unsigned char fingerprint[20];

    ptr_context = otrl_context_find (otr_us, "bob", "alice@srv_otr_smp",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    memset (fingerprint, 0xAB, sizeof (fingerprint));
    ptr_fingerprint = otrl_context_find_fingerprint (ptr_context, fingerprint,
                                                     1, NULL);
    CHECK(ptr_fingerprint);
    ptr_context->active_fingerprint = ptr_fingerprint;
    ptr_peer_data = otr_context_peer_data_ctx (ptr_context);
    CHECK(ptr_peer_data);

    /*
     * We answered a question of the peer: SMP succeeded, but the peer is not
     * authenticated (the trust is set by libotr only when appropriate).
     */
    otr_callback_handle_smp_event (NULL, OTRL_SMPEVENT_ASK_FOR_ANSWER,
                                   ptr_context, 25, (char *)"question?");
    LONGS_EQUAL(1, ptr_peer_data->in_smp);
    LONGS_EQUAL(1, ptr_peer_data->smp_question);
    otr_callback_handle_smp_event (NULL, OTRL_SMPEVENT_SUCCESS,
                                   ptr_context, 100, NULL);
    LONGS_EQUAL(0, ptr_peer_data->in_smp);
    LONGS_EQUAL(0, ptr_peer_data->smp_question);
    LONGS_EQUAL(0, otrl_context_is_fingerprint_trusted (ptr_fingerprint));

    ptr_context->active_fingerprint = NULL;
    otrl_context_forget_fingerprint (ptr_fingerprint, 0);
}
