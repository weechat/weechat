/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR context functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-context.h"
}

TEST_GROUP(OtrContext)
{
};

/*
 * Test functions:
 *   otr_context_smp_asked
 *   otr_context_smp_reset
 */

TEST(OtrContext, Smp)
{
    ConnContext *ptr_context;
    OtrlSMState *ptr_smstate;
    struct t_otr_peer_data peer_data;

    LONGS_EQUAL(0, otr_context_smp_asked (NULL));

    ptr_context = otrl_context_find (otr_us, "bob", "alice@srv_otr_ctx_smp",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_smstate = ptr_context->smstate;
    CHECK(ptr_smstate);

    /* No SMP started (or state reset at the end of session). */
    LONGS_EQUAL(0, otr_context_smp_asked (ptr_context));

    /* SMP state initialized, without our half of the generators. */
    otrl_sm_state_init (ptr_smstate);
    LONGS_EQUAL(0, otr_context_smp_asked (ptr_context));

    /* Request of the peer received (as done by otrl_sm_step2a). */
    ptr_smstate->x2 = gcry_mpi_new (0);
    ptr_smstate->x3 = gcry_mpi_new (0);
    LONGS_EQUAL(1, otr_context_smp_asked (ptr_context));

    /* Response already sent. */
    ptr_smstate->nextExpected = OTRL_SMP_EXPECT3;
    LONGS_EQUAL(0, otr_context_smp_asked (ptr_context));
    ptr_smstate->nextExpected = OTRL_SMP_EXPECT1;

    /* SMP completed. */
    ptr_smstate->sm_prog_state = OTRL_SMP_PROG_SUCCEEDED;
    LONGS_EQUAL(0, otr_context_smp_asked (ptr_context));
    ptr_smstate->sm_prog_state = OTRL_SMP_PROG_OK;

    /* End of session: state reset by libotr. */
    otrl_sm_state_free (ptr_smstate);
    LONGS_EQUAL(0, otr_context_smp_asked (ptr_context));

    /* Reset of SMP state in per-peer data. */
    otr_context_smp_reset (NULL);
    peer_data.in_smp = 1;
    peer_data.smp_question = 1;
    peer_data.smp_asked = 1;
    otr_context_smp_reset (&peer_data);
    LONGS_EQUAL(0, peer_data.in_smp);
    LONGS_EQUAL(0, peer_data.smp_question);
    LONGS_EQUAL(0, peer_data.smp_asked);
}

/*
 * Test functions:
 *   otr_context_hold_message
 *   otr_context_clear_held_messages
 *   otr_context_end_session (messages held removed)
 */

TEST(OtrContext, HeldMessages)
{
    struct t_otr_peer_data *ptr_peer_data;

    LONGS_EQUAL(0, otr_context_hold_message (NULL, "test"));
    otr_context_clear_held_messages (NULL);

    ptr_peer_data = otr_context_peer_data_for ("alice@srv_otr_ctx_held", "bob");
    CHECK(ptr_peer_data);
    POINTERS_EQUAL(NULL, ptr_peer_data->held_messages);
    LONGS_EQUAL(0, otr_context_hold_message (ptr_peer_data, NULL));

    LONGS_EQUAL(1, otr_context_hold_message (ptr_peer_data, "line 1"));
    STRCMP_EQUAL("line 1", *(ptr_peer_data->held_messages));
    LONGS_EQUAL(1, otr_context_hold_message (ptr_peer_data, "line 2"));
    STRCMP_EQUAL("line 1\nline 2", *(ptr_peer_data->held_messages));

    otr_context_clear_held_messages (ptr_peer_data);
    POINTERS_EQUAL(NULL, ptr_peer_data->held_messages);

    /* Messages held are removed at the end of session (even if no session). */
    LONGS_EQUAL(1, otr_context_hold_message (ptr_peer_data, "line 1"));
    LONGS_EQUAL(0, otr_context_end_session ("alice@srv_otr_ctx_held", "bob",
                                            0, 0));
    POINTERS_EQUAL(NULL, ptr_peer_data->held_messages);
}
