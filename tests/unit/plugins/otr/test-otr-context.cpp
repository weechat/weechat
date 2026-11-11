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
#include <string.h>
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-line.h"
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
 *   otr_context_drop_held_messages
 *   otr_context_end_session (messages held removed)
 */

TEST(OtrContext, HeldMessages)
{
    struct t_otr_peer_data *ptr_peer_data;
    struct t_gui_line *ptr_last_line;

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

    /*
     * Messages held are removed at the end of session (even if no session),
     * silently (for example when the buffer is closed).
     */
    LONGS_EQUAL(1, otr_context_hold_message (ptr_peer_data, "line 1"));
    ptr_last_line = gui_buffers->own_lines->last_line;
    LONGS_EQUAL(0, otr_context_end_session ("alice@srv_otr_ctx_held", "bob",
                                            0, 0));
    POINTERS_EQUAL(NULL, ptr_peer_data->held_messages);
    POINTERS_EQUAL(ptr_last_line, gui_buffers->own_lines->last_line);

    /* With display (/otr finish): number of messages discarded displayed. */
    LONGS_EQUAL(1, otr_context_hold_message (ptr_peer_data, "line 1"));
    LONGS_EQUAL(1, otr_context_hold_message (ptr_peer_data, "line 2"));
    LONGS_EQUAL(0, otr_context_end_session ("alice@srv_otr_ctx_held", "bob",
                                            1, 0));
    POINTERS_EQUAL(NULL, ptr_peer_data->held_messages);
    CHECK(strstr (gui_buffers->own_lines->last_line->data->message,
                  "2 messages held for the OTR session have been discarded."));
}

/*
 * Test functions:
 *   otr_context_move_held_messages
 */

TEST(OtrContext, MoveHeldMessages)
{
    struct t_otr_peer_data *ptr_peer_data1, *ptr_peer_data2;

    otr_context_move_held_messages (NULL, NULL);

    ptr_peer_data1 = otr_context_peer_data_for ("alice@srv_otr_ctx_move",
                                                "bob");
    CHECK(ptr_peer_data1);
    ptr_peer_data2 = otr_context_peer_data_for ("alice2@srv_otr_ctx_move",
                                                "bob");
    CHECK(ptr_peer_data2);

    /* Nothing to move. */
    otr_context_move_held_messages (ptr_peer_data1, ptr_peer_data2);
    POINTERS_EQUAL(NULL, ptr_peer_data2->held_messages);

    /* Move to per-peer data without messages held. */
    otr_context_hold_message (ptr_peer_data1, "line 1");
    otr_context_move_held_messages (ptr_peer_data1, ptr_peer_data1);
    STRCMP_EQUAL("line 1", *(ptr_peer_data1->held_messages));
    otr_context_move_held_messages (ptr_peer_data1, ptr_peer_data2);
    POINTERS_EQUAL(NULL, ptr_peer_data1->held_messages);
    STRCMP_EQUAL("line 1", *(ptr_peer_data2->held_messages));

    /* Move to per-peer data with messages held: added before. */
    otr_context_hold_message (ptr_peer_data1, "line 0");
    otr_context_move_held_messages (ptr_peer_data2, ptr_peer_data1);
    POINTERS_EQUAL(NULL, ptr_peer_data2->held_messages);
    STRCMP_EQUAL("line 1\nline 0", *(ptr_peer_data1->held_messages));

    otr_context_clear_held_messages (ptr_peer_data1);
}
