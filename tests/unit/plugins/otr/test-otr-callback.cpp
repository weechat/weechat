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
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-line.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-callback.h"
#include "src/plugins/otr/otr-context.h"

extern void otr_callback_inject_message (void *opdata,
                                         const char *accountname,
                                         const char *protocol,
                                         const char *recipient,
                                         const char *message);
extern void otr_callback_handle_msg_event (void *opdata,
                                           OtrlMessageEvent event,
                                           ConnContext *context,
                                           const char *message,
                                           gcry_error_t error);
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
    otr_callback_handle_smp_event (
        NULL, OTRL_SMPEVENT_ASK_FOR_ANSWER, ptr_context, 25,
        (char *)"question?\nSMP verification succeeded.");
    /* The question of the peer can not add a line (fake OTR message). */
    CHECK(strstr (gui_buffers->own_lines->last_line->prev_line->data->message,
                  "verification: question? SMP verification succeeded."));
    LONGS_EQUAL(1, ptr_peer_data->in_smp);
    LONGS_EQUAL(1, ptr_peer_data->smp_question);
    LONGS_EQUAL(1, ptr_peer_data->smp_asked);
    otr_callback_handle_smp_event (NULL, OTRL_SMPEVENT_SUCCESS,
                                   ptr_context, 100, NULL);
    LONGS_EQUAL(0, ptr_peer_data->in_smp);
    LONGS_EQUAL(0, ptr_peer_data->smp_question);
    LONGS_EQUAL(0, ptr_peer_data->smp_asked);
    LONGS_EQUAL(0, otrl_context_is_fingerprint_trusted (ptr_fingerprint));

    /* Request of the peer, then aborted by the peer. */
    otr_callback_handle_smp_event (NULL, OTRL_SMPEVENT_ASK_FOR_SECRET,
                                   ptr_context, 25, NULL);
    LONGS_EQUAL(1, ptr_peer_data->in_smp);
    LONGS_EQUAL(0, ptr_peer_data->smp_question);
    LONGS_EQUAL(1, ptr_peer_data->smp_asked);
    otr_callback_handle_smp_event (NULL, OTRL_SMPEVENT_ABORT,
                                   ptr_context, 0, NULL);
    LONGS_EQUAL(0, ptr_peer_data->in_smp);
    LONGS_EQUAL(0, ptr_peer_data->smp_asked);

    ptr_context->active_fingerprint = NULL;
    otrl_context_forget_fingerprint (ptr_fingerprint, 0);
}

/*
 * Test functions:
 *   otr_callback_inject_message
 *   otr_callback_handle_msg_event
 */

TEST(OtrCallback, CallbackData)
{
    ConnContext *ptr_context;
    struct t_otr_callback_data callback_data;

    callback_data.unencrypted = NULL;
    callback_data.last_injected = NULL;

    /* Messages injected (server not found: not sent). */
    otr_callback_inject_message (NULL, "alice@srv_otr_cb_data", OTR_PROTOCOL,
                                 "bob", "?OTR:test0.");
    otr_callback_inject_message (&callback_data, "alice@srv_otr_cb_data",
                                 OTR_PROTOCOL, "bob", "?OTR:test1.");
    STRCMP_EQUAL("?OTR:test1.", callback_data.last_injected);
    otr_callback_inject_message (&callback_data, "alice@srv_otr_cb_data",
                                 OTR_PROTOCOL, "bob", "?OTR:test2.");
    STRCMP_EQUAL("?OTR:test2.", callback_data.last_injected);

    /* Plaintext received while the session is encrypted. */
    ptr_context = otrl_context_find (otr_us, "bob", "alice@srv_otr_cb_data",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    otr_callback_handle_msg_event (NULL, OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED,
                                   ptr_context, "hello", 0);
    POINTERS_EQUAL(NULL, callback_data.unencrypted);
    otr_callback_handle_msg_event (&callback_data,
                                   OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED,
                                   ptr_context, "hello", 0);
    STRCMP_EQUAL("hello", callback_data.unencrypted);
    STRCMP_EQUAL("?OTR:test2.", callback_data.last_injected);

    free (callback_data.unencrypted);
    free (callback_data.last_injected);
}
