/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR command functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-line.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-command.h"
#include "src/plugins/otr/otr-context.h"

extern int otr_command_split_args (const char *string, char **args,
                                   int max_args);
extern const char *otr_command_get_policy_name (const char *name);
extern int otr_command_value_ok (const char *value);
}

TEST_GROUP(OtrCommand)
{
};

/*
 * Test functions:
 *   otr_command_split_args
 */

TEST(OtrCommand, SplitArgs)
{
    char *args[16];
    int i;

    LONGS_EQUAL(0, otr_command_split_args ("", args, 16));
    LONGS_EQUAL(0, otr_command_split_args ("   ", args, 16));

    LONGS_EQUAL(3, otr_command_split_args ("  a b   c ", args, 16));
    STRCMP_EQUAL("a", args[0]);
    STRCMP_EQUAL("b", args[1]);
    STRCMP_EQUAL("c", args[2]);
    for (i = 0; i < 3; i++)
    {
        free (args[i]);
    }

    LONGS_EQUAL(4, otr_command_split_args (
                    "smp ask 'where did we meet?' \"a \\\"b\\\" c\"",
                    args, 16));
    STRCMP_EQUAL("smp", args[0]);
    STRCMP_EQUAL("ask", args[1]);
    STRCMP_EQUAL("where did we meet?", args[2]);
    STRCMP_EQUAL("a \"b\" c", args[3]);
    for (i = 0; i < 4; i++)
    {
        free (args[i]);
    }

    LONGS_EQUAL(1, otr_command_split_args ("a\\ b", args, 16));
    STRCMP_EQUAL("a b", args[0]);
    free (args[0]);

    LONGS_EQUAL(1, otr_command_split_args ("ab'cd ef'gh", args, 16));
    STRCMP_EQUAL("abcd efgh", args[0]);
    free (args[0]);

    /* Errors: unterminated quote, too many arguments. */
    LONGS_EQUAL(-1, otr_command_split_args ("a 'unterminated", args, 16));
    LONGS_EQUAL(-1, otr_command_split_args ("a \"unterminated", args, 16));
    LONGS_EQUAL(-1, otr_command_split_args ("a b c", args, 2));
}

/*
 * Test functions:
 *   otr_command_get_policy_name
 */

TEST(OtrCommand, GetPolicyName)
{
    POINTERS_EQUAL(NULL, otr_command_get_policy_name (""));
    POINTERS_EQUAL(NULL, otr_command_get_policy_name ("unknown"));
    STRCMP_EQUAL("allow_v2", otr_command_get_policy_name ("allow_v2"));
    STRCMP_EQUAL("send_tag", otr_command_get_policy_name ("Send_Tag"));
    STRCMP_EQUAL("html_filter", otr_command_get_policy_name ("HTML_FILTER"));
}

/*
 * Test functions:
 *   otr_command_value_ok
 */

TEST(OtrCommand, ValueOk)
{
    LONGS_EQUAL(0, otr_command_value_ok (""));
    LONGS_EQUAL(0, otr_command_value_ok ("yes"));
    LONGS_EQUAL(0, otr_command_value_ok ("1"));
    LONGS_EQUAL(1, otr_command_value_ok ("on"));
    LONGS_EQUAL(1, otr_command_value_ok ("OFF"));
    LONGS_EQUAL(1, otr_command_value_ok ("Toggle"));
}

/*
 * Test functions:
 *   otr_command_smp
 */

TEST(OtrCommand, Smp)
{
    ConnContext *ptr_context;
    struct t_otr_peer_data *ptr_peer_data;

    ptr_context = otrl_context_find (otr_us, "bob", "alice@srv_otr_cmd_smp",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_context->msgstate = OTRL_MSGSTATE_ENCRYPTED;
    ptr_peer_data = otr_context_peer_data_ctx (ptr_context);
    CHECK(ptr_peer_data);
    LONGS_EQUAL(0, ptr_peer_data->smp_asked);

    /* Respond without SMP request of the peer: refused (no crash in libotr). */
    run_cmd_quiet ("/otr smp respond bob srv_otr_cmd_smp secret");
    CHECK(strstr (gui_buffers->own_lines->last_line->data->message,
                  "There is no SMP request from bob@srv_otr_cmd_smp"));
    LONGS_EQUAL(0, ptr_peer_data->smp_asked);

    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;
}
