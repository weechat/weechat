/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR modifier functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-context.h"
#include "src/plugins/otr/otr-modifier.h"

extern int otr_modifier_is_base64_part (const char *line);
extern int otr_modifier_assemble (struct t_otr_peer_data *peer_data,
                                  const char *line, char **message);
extern int otr_modifier_parse_out_privmsg (const char *message, char **tags,
                                           char **target, const char **text);
extern int otr_modifier_all_targets_channels (const char *server,
                                              const char *targets);
extern int otr_modifier_must_hold (const char *account, const char *peer);
extern int otr_modifier_remove_batch (const char *server,
                                      const char *message);
extern char *otr_modifier_remove_batch_tag (const char *server,
                                            const char *tags);
}

TEST_GROUP(OtrModifier)
{
};

/*
 * Test functions:
 *   otr_modifier_is_base64_part
 */

TEST(OtrModifier, IsBase64Part)
{
    LONGS_EQUAL(0, otr_modifier_is_base64_part (""));
    LONGS_EQUAL(0, otr_modifier_is_base64_part ("."));
    LONGS_EQUAL(0, otr_modifier_is_base64_part ("a.b"));
    LONGS_EQUAL(0, otr_modifier_is_base64_part ("abc.."));
    LONGS_EQUAL(0, otr_modifier_is_base64_part ("hello world"));
    LONGS_EQUAL(0, otr_modifier_is_base64_part ("abc,"));
    LONGS_EQUAL(1, otr_modifier_is_base64_part ("abc"));
    LONGS_EQUAL(1, otr_modifier_is_base64_part ("abc."));
    LONGS_EQUAL(1, otr_modifier_is_base64_part ("AZaz09+/=="));
}

/*
 * Test functions:
 *   otr_modifier_assemble
 */

TEST(OtrModifier, Assemble)
{
    struct t_otr_peer_data peer_data;
    char *message, *line;

    memset (&peer_data, 0, sizeof (peer_data));

    /* Lines returned as-is. */
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "hello", &message));
    STRCMP_EQUAL("hello", message);
    free (message);
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "?OTRv23?", &message));
    STRCMP_EQUAL("?OTRv23?", message);
    free (message);
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data,
                                          "?OTR|1|2,00001,00002,abc,",
                                          &message));
    STRCMP_EQUAL("?OTR|1|2,00001,00002,abc,", message);
    free (message);
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "?OTR:AAMDabc.",
                                          &message));
    STRCMP_EQUAL("?OTR:AAMDabc.", message);
    free (message);
    POINTERS_EQUAL(NULL, peer_data.assembly);

    /* Plain text starting with "?OTR" must not hide next lines. */
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "?OTR is that you",
                                          &message));
    STRCMP_EQUAL("?OTR is that you", message);
    free (message);
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "hello", &message));
    STRCMP_EQUAL("hello", message);
    free (message);
    POINTERS_EQUAL(NULL, peer_data.assembly);

    /* OTR message split over 3 lines. */
    message = NULL;
    LONGS_EQUAL(0, otr_modifier_assemble (&peer_data, "?OTR:AAMDabc",
                                          &message));
    POINTERS_EQUAL(NULL, message);
    STRCMP_EQUAL("?OTR:AAMDabc", peer_data.assembly);
    LONGS_EQUAL(0, otr_modifier_assemble (&peer_data, "def", &message));
    POINTERS_EQUAL(NULL, message);
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "ghi.", &message));
    STRCMP_EQUAL("?OTR:AAMDabcdefghi.", message);
    free (message);
    POINTERS_EQUAL(NULL, peer_data.assembly);
    LONGS_EQUAL(0, peer_data.assembly_length);

    /* Incomplete OTR message dropped by a line which is not base64. */
    message = NULL;
    LONGS_EQUAL(0, otr_modifier_assemble (&peer_data, "?OTR:AAMDabc",
                                          &message));
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "hello world",
                                          &message));
    STRCMP_EQUAL("hello world", message);
    free (message);
    POINTERS_EQUAL(NULL, peer_data.assembly);

    /* Incomplete OTR message dropped when the max size is reached. */
    line = (char *)malloc (OTR_MAX_ASSEMBLY);
    CHECK(line);
    memset (line, 'A', OTR_MAX_ASSEMBLY - 1);
    line[OTR_MAX_ASSEMBLY - 1] = '\0';
    message = NULL;
    LONGS_EQUAL(0, otr_modifier_assemble (&peer_data, "?OTR:AAMDabc",
                                          &message));
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, line, &message));
    STRCMP_EQUAL(line, message);
    free (message);
    POINTERS_EQUAL(NULL, peer_data.assembly);
    free (line);
}

/*
 * Test functions:
 *   otr_modifier_parse_out_privmsg
 */

TEST(OtrModifier, ParseOutPrivmsg)
{
    char *tags, *target;
    const char *text;

    tags = NULL;
    target = NULL;
    text = NULL;
    /* Not a PRIVMSG. */
    LONGS_EQUAL(0, otr_modifier_parse_out_privmsg ("", &tags, &target, &text));
    LONGS_EQUAL(0, otr_modifier_parse_out_privmsg ("NOTICE bob :hello",
                                                   &tags, &target, &text));
    LONGS_EQUAL(0, otr_modifier_parse_out_privmsg ("@batch=abc",
                                                   &tags, &target, &text));
    LONGS_EQUAL(0, otr_modifier_parse_out_privmsg ("@batch=abc NOTICE bob :hi",
                                                   &tags, &target, &text));
    LONGS_EQUAL(0, otr_modifier_parse_out_privmsg (
                    "BATCH +abc draft/multiline #chan", &tags, &target, &text));
    LONGS_EQUAL(0, otr_modifier_parse_out_privmsg ("BATCH -abc",
                                                   &tags, &target, &text));
    POINTERS_EQUAL(NULL, tags);
    POINTERS_EQUAL(NULL, target);

    /* Invalid PRIVMSG. */
    LONGS_EQUAL(-1, otr_modifier_parse_out_privmsg ("PRIVMSG bob",
                                                    &tags, &target, &text));
    LONGS_EQUAL(-1, otr_modifier_parse_out_privmsg ("@batch=abc PRIVMSG bob",
                                                    &tags, &target, &text));
    POINTERS_EQUAL(NULL, tags);
    POINTERS_EQUAL(NULL, target);

    LONGS_EQUAL(1, otr_modifier_parse_out_privmsg ("PRIVMSG bob :hello world",
                                                   &tags, &target, &text));
    POINTERS_EQUAL(NULL, tags);
    STRCMP_EQUAL("bob", target);
    STRCMP_EQUAL("hello world", text);
    free (target);

    LONGS_EQUAL(1, otr_modifier_parse_out_privmsg ("  privmsg  bob hello",
                                                   &tags, &target, &text));
    POINTERS_EQUAL(NULL, tags);
    STRCMP_EQUAL("bob", target);
    STRCMP_EQUAL("hello", text);
    free (target);

    /* Message with IRC tags (for example a multiline batch). */
    LONGS_EQUAL(1, otr_modifier_parse_out_privmsg (
                    "@batch=abc;+draft/reply=1 PRIVMSG #chan :line 1",
                    &tags, &target, &text));
    STRCMP_EQUAL("@batch=abc;+draft/reply=1 ", tags);
    STRCMP_EQUAL("#chan", target);
    STRCMP_EQUAL("line 1", text);
    free (tags);
    free (target);
}

/*
 * Test functions:
 *   otr_modifier_all_targets_channels
 */

TEST(OtrModifier, AllTargetsChannels)
{
    LONGS_EQUAL(0, otr_modifier_all_targets_channels ("test", "bob"));
    LONGS_EQUAL(0, otr_modifier_all_targets_channels ("test", "#chan,bob"));
    LONGS_EQUAL(1, otr_modifier_all_targets_channels ("test", "#chan"));
    LONGS_EQUAL(1, otr_modifier_all_targets_channels ("test", "#a,#b"));
    LONGS_EQUAL(1, otr_modifier_all_targets_channels ("test", ",#a,,#b,"));
}

/*
 * Test functions:
 *   otr_modifier_in_cb
 */

TEST(OtrModifier, InCb)
{
    ConnContext *ptr_context;
    char *str;

    ptr_context = otrl_context_find (otr_us, "bob", "alice@srv_otr",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);

    /* Plaintext without OTR session: unchanged. */
    POINTERS_EQUAL(NULL,
                   otr_modifier_in_cb (NULL, NULL, "irc_in_privmsg", "srv_otr",
                                       ":bob!u@h PRIVMSG alice :hello"));

    /* Plaintext during an encrypted session: displayed with a prefix. */
    ptr_context->msgstate = OTRL_MSGSTATE_ENCRYPTED;
    WEE_TEST_STR(":bob!u@h PRIVMSG alice :Unencrypted message received: hello",
                 otr_modifier_in_cb (NULL, NULL, "irc_in_privmsg", "srv_otr",
                                     ":bob!u@h PRIVMSG alice :hello"));
    WEE_TEST_STR("@time=2026-01-01T00:00:00.000Z :bob!u@h PRIVMSG alice "
                 ":Unencrypted message received: hello",
                 otr_modifier_in_cb (NULL, NULL, "irc_in_privmsg", "srv_otr",
                                     "@time=2026-01-01T00:00:00.000Z "
                                     ":bob!u@h PRIVMSG alice :hello"));
    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;
}

/*
 * Test functions:
 *   otr_modifier_out_cb
 */

TEST(OtrModifier, OutCb)
{
    ConnContext *ptr_context;
    char *str;

    ptr_context = otrl_context_find (otr_us, "bob", "alice@srv_otr_out",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_context->msgstate = OTRL_MSGSTATE_ENCRYPTED;

    /* Not a PRIVMSG, or channel: unchanged. */
    POINTERS_EQUAL(NULL,
                   otr_modifier_out_cb (NULL, NULL, "irc_out_privmsg",
                                        "srv_otr_out", "BATCH -abc"));
    POINTERS_EQUAL(NULL,
                   otr_modifier_out_cb (NULL, NULL, "irc_out_privmsg",
                                        "srv_otr_out",
                                        "PRIVMSG #chan :hello"));

    /* CTCP other than ACTION: sent unencrypted. */
    POINTERS_EQUAL(NULL,
                   otr_modifier_out_cb (NULL, NULL, "irc_out_privmsg",
                                        "srv_otr_out",
                                        "PRIVMSG bob :\x01VERSION\x01"));
    POINTERS_EQUAL(NULL,
                   otr_modifier_out_cb (NULL, NULL, "irc_out_privmsg",
                                        "srv_otr_out",
                                        "@batch=abc PRIVMSG bob :\x01" "DCC "
                                        "SEND file 2130706433 5000 1234\x01"));

    /* OTR disabled by policies during the session: not sent in plain text. */
    config_file_option_set_with_string (
        "otr.policy.srv_otr_out.alice.bob.allow_v2", "off");
    config_file_option_set_with_string (
        "otr.policy.srv_otr_out.alice.bob.allow_v3", "off");
    WEE_TEST_STR("",
                 otr_modifier_out_cb (NULL, NULL, "irc_out_privmsg",
                                      "srv_otr_out", "PRIVMSG bob :secret"));
    ptr_context->msgstate = OTRL_MSGSTATE_FINISHED;
    WEE_TEST_STR("",
                 otr_modifier_out_cb (NULL, NULL, "irc_out_privmsg",
                                      "srv_otr_out", "PRIVMSG bob :secret"));
    run_cmd_quiet ("/mute /unset otr.policy.srv_otr_out.alice.bob.allow_v2");
    run_cmd_quiet ("/mute /unset otr.policy.srv_otr_out.alice.bob.allow_v3");

    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;
}

/*
 * Test functions:
 *   otr_modifier_remove_batch
 *   otr_modifier_remove_batch_tag
 *   otr_modifier_free_removed_batches
 */

TEST(OtrModifier, RemoveBatch)
{
    ConnContext *ptr_context;
    char *str;

    ptr_context = otrl_context_find (otr_us, "bob", "alice@srv_otr_batch",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);

    /* Not a batch, or batch not removed. */
    LONGS_EQUAL(0, otr_modifier_remove_batch ("srv_otr_batch", "PING x"));
    LONGS_EQUAL(0, otr_modifier_remove_batch ("srv_otr_batch", "BATCH"));
    LONGS_EQUAL(0, otr_modifier_remove_batch ("srv_otr_batch", "BATCH +"));
    LONGS_EQUAL(0, otr_modifier_remove_batch ("srv_otr_batch", "BATCH -abc"));
    LONGS_EQUAL(0, otr_modifier_remove_batch (
                    "srv_otr_batch", "BATCH +abc draft/multiline bob"));
    LONGS_EQUAL(0, otr_modifier_remove_batch ("srv_otr_batch", "BATCH -abc"));
    WEE_TEST_STR("@batch=abc;+draft/reply=1 ",
                 otr_modifier_remove_batch_tag ("srv_otr_batch",
                                                "@batch=abc;+draft/reply=1 "));

    ptr_context->msgstate = OTRL_MSGSTATE_ENCRYPTED;

    /* Batch to a channel: not removed. */
    LONGS_EQUAL(0, otr_modifier_remove_batch (
                    "srv_otr_batch", "BATCH +abc draft/multiline #chan"));
    LONGS_EQUAL(0, otr_modifier_remove_batch (
                    "srv_otr_batch", "BATCH +abc other/type bob"));

    /* Batch to a nick with an encrypted session: removed. */
    LONGS_EQUAL(1, otr_modifier_remove_batch (
                    "srv_otr_batch", "BATCH +abc draft/multiline Bob"));
    WEE_TEST_STR(NULL,
                 otr_modifier_remove_batch_tag ("srv_otr_batch",
                                                "@batch=abc "));
    WEE_TEST_STR("@+draft/reply=1 ",
                 otr_modifier_remove_batch_tag ("srv_otr_batch",
                                                "@batch=abc;+draft/reply=1 "));
    WEE_TEST_STR("@a=1;b ",
                 otr_modifier_remove_batch_tag ("srv_otr_batch",
                                                "@a=1;batch=abc;b "));
    WEE_TEST_STR("@batch=other ",
                 otr_modifier_remove_batch_tag ("srv_otr_batch",
                                                "@batch=other "));
    WEE_TEST_STR("@batch=abc ",
                 otr_modifier_remove_batch_tag ("other_server",
                                                "@batch=abc "));

    /* Line of the batch removed and sent unencrypted: batch tag removed. */
    WEE_TEST_STR("PRIVMSG bob :\x01VERSION\x01",
                 otr_modifier_out_cb (NULL, NULL, "irc_out_privmsg",
                                      "srv_otr_batch",
                                      "@batch=abc PRIVMSG bob "
                                      ":\x01VERSION\x01"));

    /* End of batch removed. */
    LONGS_EQUAL(1, otr_modifier_remove_batch ("srv_otr_batch", "BATCH -abc"));
    LONGS_EQUAL(0, otr_modifier_remove_batch ("srv_otr_batch", "BATCH -abc"));

    /* Free batches removed (end of batch never sent). */
    LONGS_EQUAL(1, otr_modifier_remove_batch (
                    "srv_otr_batch", "BATCH +def draft/multiline bob"));
    CHECK(otr_modifier_removed_batches);
    otr_modifier_free_removed_batches ();
    POINTERS_EQUAL(NULL, otr_modifier_removed_batches);

    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;
}

/*
 * Test functions:
 *   otr_modifier_must_hold
 *   otr_modifier_hold_message
 */

TEST(OtrModifier, HoldMessages)
{
    struct t_irc_server *server;
    struct t_otr_peer_data *ptr_peer_data;
    ConnContext *ptr_context;
    char *str;

    run_cmd_quiet ("/mute /server add srv_otr_hold fake:127.0.0.1");
    run_cmd_quiet ("/connect srv_otr_hold");
    server = irc_server_search ("srv_otr_hold");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    ptr_peer_data = otr_context_peer_data_for ("alice@srv_otr_hold", "bob");
    CHECK(ptr_peer_data);
    ptr_context = otrl_context_find (otr_us, "bob", "alice@srv_otr_hold",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 0,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);

    /* Encryption not required: messages not held. */
    LONGS_EQUAL(0, otr_modifier_must_hold ("alice@srv_otr_hold", "bob"));

    config_file_option_set_with_string (
        "otr.policy.srv_otr_hold.alice.bob.require_encryption", "on");

    /* Encryption required and no session: messages held. */
    LONGS_EQUAL(1, otr_modifier_must_hold ("alice@srv_otr_hold", "bob"));
    WEE_TEST_STR("",
                 otr_modifier_out_cb (NULL, NULL, "irc_out_privmsg",
                                      "srv_otr_hold", "PRIVMSG bob :line 1"));
    WEE_TEST_STR("",
                 otr_modifier_out_cb (NULL, NULL, "irc_out_privmsg",
                                      "srv_otr_hold",
                                      "@a=1 PRIVMSG bob :line 2"));
    CHECK(ptr_peer_data->held_messages);
    STRCMP_EQUAL("line 1\nline 2", *(ptr_peer_data->held_messages));

    /* Multiline batch removed: its lines are held. */
    LONGS_EQUAL(1, otr_modifier_remove_batch (
                    "srv_otr_hold", "BATCH +ref draft/multiline bob"));
    WEE_TEST_STR("",
                 otr_modifier_out_cb (NULL, NULL, "irc_out_privmsg",
                                      "srv_otr_hold",
                                      "@batch=ref PRIVMSG bob :line 3"));
    LONGS_EQUAL(1, otr_modifier_remove_batch ("srv_otr_hold", "BATCH -ref"));
    STRCMP_EQUAL("line 1\nline 2\nline 3", *(ptr_peer_data->held_messages));

    /* Session encrypted: messages not held. */
    ptr_context->msgstate = OTRL_MSGSTATE_ENCRYPTED;
    LONGS_EQUAL(0, otr_modifier_must_hold ("alice@srv_otr_hold", "bob"));
    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;

    otr_context_clear_held_messages (ptr_peer_data);
    run_cmd_quiet ("/mute /unset "
                   "otr.policy.srv_otr_hold.alice.bob.require_encryption");

    run_cmd_quiet ("/mute /disconnect srv_otr_hold");
    run_cmd_quiet ("/mute /server del srv_otr_hold");
}

/*
 * Test functions:
 *   otr_modifier_in_cb (CTCP ACTION received in plain text)
 */

TEST(OtrModifier, InCbAction)
{
    struct t_irc_server *server;
    ConnContext *ptr_context;
    char *str;

    run_cmd_quiet ("/mute /server add srv_otr_action fake:127.0.0.1");
    run_cmd_quiet ("/connect srv_otr_action");
    server = irc_server_search ("srv_otr_action");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    ptr_context = otrl_context_find (otr_us, "bob", "alice@srv_otr_action",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);

    /* No OTR session: unchanged. */
    POINTERS_EQUAL(NULL,
                   otr_modifier_in_cb (NULL, NULL, "irc_in_privmsg",
                                       "srv_otr_action",
                                       ":bob!u@h PRIVMSG alice "
                                       ":\x01" "ACTION waves\x01"));

    /* Other CTCP: unchanged. */
    ptr_context->msgstate = OTRL_MSGSTATE_ENCRYPTED;
    POINTERS_EQUAL(NULL,
                   otr_modifier_in_cb (NULL, NULL, "irc_in_privmsg",
                                       "srv_otr_action",
                                       ":bob!u@h PRIVMSG alice "
                                       ":\x01VERSION\x01"));

    /* Encrypted or finished session: flagged. */
    WEE_TEST_STR(":bob!u@h PRIVMSG alice "
                 ":Unencrypted message received: /me waves",
                 otr_modifier_in_cb (NULL, NULL, "irc_in_privmsg",
                                     "srv_otr_action",
                                     ":bob!u@h PRIVMSG alice "
                                     ":\x01" "ACTION waves\x01"));
    ptr_context->msgstate = OTRL_MSGSTATE_FINISHED;
    WEE_TEST_STR(":bob!u@h PRIVMSG alice "
                 ":Unencrypted message received: /me waves",
                 otr_modifier_in_cb (NULL, NULL, "irc_in_privmsg",
                                     "srv_otr_action",
                                     ":bob!u@h PRIVMSG alice "
                                     ":\x01" "ACTION waves\x01"));

    /* No OTR session, but encryption required: flagged. */
    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;
    config_file_option_set_with_string (
        "otr.policy.srv_otr_action.alice.bob.require_encryption", "on");
    WEE_TEST_STR(":bob!u@h PRIVMSG alice "
                 ":Unencrypted message received: /me waves",
                 otr_modifier_in_cb (NULL, NULL, "irc_in_privmsg",
                                     "srv_otr_action",
                                     ":bob!u@h PRIVMSG alice "
                                     ":\x01" "ACTION waves\x01"));
    run_cmd_quiet ("/mute /unset "
                   "otr.policy.srv_otr_action.alice.bob.require_encryption");

    run_cmd_quiet ("/mute /disconnect srv_otr_action");
    run_cmd_quiet ("/mute /server del srv_otr_action");
}
