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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-history.h"
#include "src/gui/gui-line.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-command.h"
#include "src/plugins/otr/otr-context.h"
#include "src/plugins/otr/otr-key.h"

extern int otr_command_split_args (const char *string, char **args,
                                   int max_args);
extern const char *otr_command_get_policy_name (const char *name);
extern const char *otr_command_get_policy_value (const char *value);
extern void otr_command_show_peer_fingerprints (const char *search);
extern int otr_command_has_smp_secret (const char *string);
extern int otr_command_start (const struct t_otr_peer_ref *peer_ref);
extern int otr_command_log (struct t_gui_buffer *buffer, char **args,
                            int num_args);
}

TEST_GROUP(OtrCommand)
{
    /*
     * Check if a line containing a string has been displayed in core buffer
     * after a given line.
     */
    int core_line_displayed (struct t_gui_line *after_line, const char *string)
    {
        struct t_gui_line *ptr_line;

        for (ptr_line = gui_buffers->own_lines->last_line;
             ptr_line && (ptr_line != after_line);
             ptr_line = ptr_line->prev_line)
        {
            if (ptr_line->data->message
                && strstr (ptr_line->data->message, string))
            {
                return 1;
            }
        }
        return 0;
    }
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
 *   otr_command_has_smp_secret
 */

TEST(OtrCommand, HasSmpSecret)
{
    LONGS_EQUAL(0, otr_command_has_smp_secret (NULL));
    LONGS_EQUAL(0, otr_command_has_smp_secret (""));
    LONGS_EQUAL(0, otr_command_has_smp_secret ("hello"));
    LONGS_EQUAL(0, otr_command_has_smp_secret ("otr smp ask secret"));
    LONGS_EQUAL(0, otr_command_has_smp_secret ("//otr smp ask secret"));
    LONGS_EQUAL(0, otr_command_has_smp_secret ("/otr"));
    LONGS_EQUAL(0, otr_command_has_smp_secret ("/otr start"));
    LONGS_EQUAL(0, otr_command_has_smp_secret ("/otr smp"));
    LONGS_EQUAL(0, otr_command_has_smp_secret ("/otr smp abort"));
    LONGS_EQUAL(0, otr_command_has_smp_secret ("/otrx smp ask secret"));
    LONGS_EQUAL(0, otr_command_has_smp_secret ("/msg bob /otr smp ask secret"));

    LONGS_EQUAL(1, otr_command_has_smp_secret ("/otr smp ask"));
    LONGS_EQUAL(1, otr_command_has_smp_secret ("/otr smp ask secret"));
    LONGS_EQUAL(1, otr_command_has_smp_secret ("/otr  smp  ask  \"a b\""));
    LONGS_EQUAL(1, otr_command_has_smp_secret ("/otr smp ask bob libera "
                                               "\"Question?\" \"secret\""));
    LONGS_EQUAL(1, otr_command_has_smp_secret ("/otr smp respond secret"));
    LONGS_EQUAL(1, otr_command_has_smp_secret ("/otr \"smp\" 'respond' x"));

    /* Invalid arguments: dropped, they may contain a secret. */
    LONGS_EQUAL(1, otr_command_has_smp_secret ("/otr smp ask \"secret"));
}

/*
 * Test functions:
 *   otr_command_history_add_cb
 */

TEST(OtrCommand, HistoryAdd)
{
    gui_history_add (gui_buffers, "/otr smp abort");
    STRCMP_EQUAL("/otr smp abort", gui_history->text);

    gui_history_add (gui_buffers, "/otr smp ask \"swordfish\"");
    STRCMP_EQUAL("/otr smp abort", gui_history->text);
    STRCMP_EQUAL("/otr smp abort", gui_buffers->history->text);

    gui_history_add (gui_buffers, "/otr smp respond Berlin");
    STRCMP_EQUAL("/otr smp abort", gui_history->text);
    STRCMP_EQUAL("/otr smp abort", gui_buffers->history->text);
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
 *   otr_command_get_policy_value
 */

TEST(OtrCommand, GetPolicyValue)
{
    POINTERS_EQUAL(NULL, otr_command_get_policy_value (""));
    POINTERS_EQUAL(NULL, otr_command_get_policy_value ("yes"));
    POINTERS_EQUAL(NULL, otr_command_get_policy_value ("1"));
    STRCMP_EQUAL("on", otr_command_get_policy_value ("on"));
    STRCMP_EQUAL("on", otr_command_get_policy_value ("ON"));
    STRCMP_EQUAL("off", otr_command_get_policy_value ("OFF"));
    STRCMP_EQUAL("toggle", otr_command_get_policy_value ("Toggle"));
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

    /*
     * Request of the peer, but no SMP state in libotr (for example after the
     * end of the session): refused (no crash in libotr).
     */
    ptr_peer_data->smp_asked = 1;
    run_cmd_quiet ("/otr smp respond bob srv_otr_cmd_smp secret");
    CHECK(strstr (gui_buffers->own_lines->last_line->data->message,
                  "There is no SMP request from bob@srv_otr_cmd_smp"));
    ptr_peer_data->smp_asked = 0;

    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;
}

/*
 * Test functions:
 *   otr_command_show_peer_fingerprints
 */

TEST(OtrCommand, ShowPeerFingerprints)
{
    ConnContext *ptr_context;
    Fingerprint *ptr_fingerprint;
    struct t_gui_line *ptr_last_line;
    unsigned char fingerprint[20];

    ptr_context = otrl_context_find (otr_us, "bobfp", "alice@srv_otr_fp",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    memset (fingerprint, 0xCD, sizeof (fingerprint));
    ptr_fingerprint = otrl_context_find_fingerprint (ptr_context, fingerprint,
                                                     1, NULL);
    CHECK(ptr_fingerprint);

    ptr_last_line = gui_buffers->own_lines->last_line;
    otr_command_show_peer_fingerprints ("bobfp");
    LONGS_EQUAL(1, core_line_displayed (ptr_last_line, "alice@srv_otr_fp"));

    /* Search is case insensitive. */
    ptr_last_line = gui_buffers->own_lines->last_line;
    otr_command_show_peer_fingerprints ("BobFP");
    LONGS_EQUAL(1, core_line_displayed (ptr_last_line, "alice@srv_otr_fp"));

    ptr_last_line = gui_buffers->own_lines->last_line;
    otr_command_show_peer_fingerprints ("carol");
    LONGS_EQUAL(0, core_line_displayed (ptr_last_line, "alice@srv_otr_fp"));

    otrl_context_forget_fingerprint (ptr_fingerprint, 0);
}

/*
 * Test functions:
 *   otr_command_peer_args
 */

TEST(OtrCommand, PeerArgs)
{
    struct t_otr_peer_ref peer_ref;
    char *args[2], long_nick[129], long_server[129];

    /* No arguments, not in a private buffer. */
    LONGS_EQUAL(0, otr_command_peer_args (gui_buffers, NULL, 0, &peer_ref));

    /* Nick and server. */
    args[0] = (char *)"bob";
    args[1] = (char *)"libera";
    LONGS_EQUAL(1, otr_command_peer_args (NULL, args, 2, &peer_ref));
    STRCMP_EQUAL("bob", peer_ref.nick);
    STRCMP_EQUAL("libera", peer_ref.server);

    /* Not a nick: channel or list of targets. */
    args[0] = (char *)"#chan";
    LONGS_EQUAL(0, otr_command_peer_args (NULL, args, 2, &peer_ref));
    args[0] = (char *)"bob,carol";
    LONGS_EQUAL(0, otr_command_peer_args (NULL, args, 2, &peer_ref));
    args[0] = (char *)"bob carol";
    LONGS_EQUAL(0, otr_command_peer_args (NULL, args, 2, &peer_ref));

    /* Max length of nick and server (127 chars): OK. */
    memset (long_nick, 'a', sizeof (long_nick) - 1);
    long_nick[127] = '\0';
    memset (long_server, 's', sizeof (long_server) - 1);
    long_server[127] = '\0';
    args[0] = long_nick;
    args[1] = long_server;
    LONGS_EQUAL(1, otr_command_peer_args (NULL, args, 2, &peer_ref));
    STRCMP_EQUAL(long_nick, peer_ref.nick);
    STRCMP_EQUAL(long_server, peer_ref.server);

    /* Nick or server too long: refused (not truncated). */
    long_nick[127] = 'a';
    long_nick[128] = '\0';
    LONGS_EQUAL(0, otr_command_peer_args (NULL, args, 2, &peer_ref));
    long_nick[127] = '\0';
    long_server[127] = 's';
    long_server[128] = '\0';
    LONGS_EQUAL(0, otr_command_peer_args (NULL, args, 2, &peer_ref));
}

/*
 * Test functions:
 *   otr_command_otr (error already displayed)
 */

TEST(OtrCommand, ErrorDisplayedOnce)
{
    struct t_gui_line *ptr_last_line;
    const char *commands[] = {
        "/otr start bob srv_otr_cmd_unknown",
        "/otr finish bob srv_otr_cmd_unknown",
        "/otr status bob srv_otr_cmd_unknown",
        "/otr trust bob srv_otr_cmd_unknown",
        "/otr smp abort bob srv_otr_cmd_unknown",
        NULL,
    };
    int i;

    for (i = 0; commands[i]; i++)
    {
        ptr_last_line = gui_buffers->own_lines->last_line;
        run_cmd_quiet (commands[i]);
        LONGS_EQUAL(1, core_line_displayed (ptr_last_line,
                                            "not connected to server "
                                            "\"srv_otr_cmd_unknown\""));
        LONGS_EQUAL(0, core_line_displayed (ptr_last_line,
                                            "invalid arguments"));
    }

    /* Invalid arguments: error displayed. */
    ptr_last_line = gui_buffers->own_lines->last_line;
    run_cmd_quiet ("/otr status bob");
    LONGS_EQUAL(1, core_line_displayed (ptr_last_line, "invalid arguments"));
}

/*
 * Test functions:
 *   otr_command_start (OTR disabled by policy)
 */

TEST(OtrCommand, StartDisabledByPolicy)
{
    struct t_irc_server *server;
    struct t_otr_peer_ref peer_ref;
    struct t_gui_line *ptr_last_line;

    run_cmd_quiet ("/mute /server add test_otr_start_pol fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_start_pol");
    server = irc_server_search ("test_otr_start_pol");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    config_file_option_set_with_string (
        "otr.policy.test_otr_start_pol.alice.bob.allow_v2", "off");
    config_file_option_set_with_string (
        "otr.policy.test_otr_start_pol.alice.bob.allow_v3", "off");

    snprintf (peer_ref.nick, sizeof (peer_ref.nick), "bob");
    snprintf (peer_ref.server, sizeof (peer_ref.server), "test_otr_start_pol");
    LONGS_EQUAL(OTR_COMMAND_RC_ERROR_DISPLAYED, otr_command_start (&peer_ref));

    /* Error displayed once by the command. */
    ptr_last_line = gui_buffers->own_lines->last_line;
    run_cmd_quiet ("/otr start bob test_otr_start_pol");
    LONGS_EQUAL(0, core_line_displayed (ptr_last_line, "invalid arguments"));

    run_cmd_quiet ("/mute /unset otr.policy.test_otr_start_pol.*");
    run_cmd_quiet ("/mute /disconnect test_otr_start_pol");
    run_cmd_quiet ("/mute /server del test_otr_start_pol");
}

/*
 * Test functions:
 *   otr_command_log
 */

TEST(OtrCommand, Log)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    ConnContext *ptr_context;
    char *args[2];

    run_cmd_quiet ("/mute /server add test_otr_cmd_log fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_cmd_log");
    server = irc_server_search ("test_otr_cmd_log");
    CHECK(server);
    irc_server_set_nick (server, "alice");
    run_cmd_quiet ("/mute /set logger.file.auto_log off");
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0);
    CHECK(channel);
    ptr_context = otrl_context_find (otr_us, "bob", "alice@test_otr_cmd_log",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_context->msgstate = OTRL_MSGSTATE_ENCRYPTED;

    args[0] = (char *)"log";

    /* Invalid argument: error. */
    args[1] = (char *)"foo";
    LONGS_EQUAL(WEECHAT_RC_ERROR, otr_command_log (channel->buffer, args, 2));

    /* Without argument: state displayed. */
    LONGS_EQUAL(WEECHAT_RC_OK, otr_command_log (channel->buffer, args, 1));
    CHECK(strstr (channel->buffer->own_lines->last_line->data->message,
                  "This conversation is currently NOT being logged."));

    /* Nothing to change: state displayed. */
    args[1] = (char *)"stop";
    gui_buffer_clear (channel->buffer);
    LONGS_EQUAL(WEECHAT_RC_OK, otr_command_log (channel->buffer, args, 2));
    CHECK(channel->buffer->own_lines->last_line);
    CHECK(strstr (channel->buffer->own_lines->last_line->data->message,
                  "This conversation is currently NOT being logged."));

    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;
    gui_buffer_close (channel->buffer);
    run_cmd_quiet ("/mute /unset logger.file.auto_log");
    run_cmd_quiet ("/mute /disconnect test_otr_cmd_log");
    run_cmd_quiet ("/mute /server del test_otr_cmd_log");
}

/*
 * Test functions:
 *   otr_command_finish (messages held)
 */

TEST(OtrCommand, FinishHeldMessages)
{
    struct t_irc_server *server;
    struct t_otr_peer_data *ptr_peer_data;
    struct t_gui_line *ptr_last_line;

    run_cmd_quiet ("/mute /server add test_otr_finish fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_finish");
    server = irc_server_search ("test_otr_finish");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    /* No session, no messages held. */
    ptr_last_line = gui_buffers->own_lines->last_line;
    run_cmd_quiet ("/otr finish bob test_otr_finish");
    LONGS_EQUAL(1, core_line_displayed (ptr_last_line,
                                        "no OTR session to finish"));

    /* No session, messages held: only the messages discarded are displayed. */
    ptr_peer_data = otr_context_peer_data_for ("alice@test_otr_finish", "bob");
    CHECK(ptr_peer_data);
    LONGS_EQUAL(1, otr_context_hold_message (ptr_peer_data, "line 1"));
    ptr_last_line = gui_buffers->own_lines->last_line;
    run_cmd_quiet ("/otr finish bob test_otr_finish");
    LONGS_EQUAL(1, core_line_displayed (ptr_last_line,
                                        "1 message held for the OTR session "
                                        "has been discarded."));
    LONGS_EQUAL(0, core_line_displayed (ptr_last_line,
                                        "no OTR session to finish"));
    POINTERS_EQUAL(NULL, ptr_peer_data->held_messages);

    run_cmd_quiet ("/mute /disconnect test_otr_finish");
    run_cmd_quiet ("/mute /server del test_otr_finish");
}

/*
 * Test functions:
 *   otr_command_start (no private key)
 */

TEST(OtrCommand, StartWithoutKey)
{
    struct t_irc_server *server;
    struct t_otr_key_generation *ptr_generation;

    run_cmd_quiet ("/mute /server add test_otr_start fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_start");
    server = irc_server_search ("test_otr_start");
    CHECK(server);
    irc_server_set_nick (server, "alice");

    POINTERS_EQUAL(NULL, otrl_privkey_find (otr_us, "alice@test_otr_start",
                                            OTR_PROTOCOL));

    /* No key: key generated, query sent later to the peer (in lower case). */
    run_cmd_quiet ("/otr start Bob test_otr_start");
    ptr_generation = otr_key_generation_search ("alice@test_otr_start",
                                                OTR_PROTOCOL);
    CHECK(ptr_generation);
    STRCMP_EQUAL("bob", *(ptr_generation->peers));

    /* Generation already running: only the peer is added. */
    run_cmd_quiet ("/otr start carol test_otr_start");
    POINTERS_EQUAL(ptr_generation,
                   otr_key_generation_search ("alice@test_otr_start",
                                              OTR_PROTOCOL));
    STRCMP_EQUAL("bob\ncarol", *(ptr_generation->peers));

    /* End before the key is generated: no query sent later to the peer. */
    run_cmd_quiet ("/otr end bob test_otr_start");
    STRCMP_EQUAL("carol", *(ptr_generation->peers));

    otr_key_generation_free_all ();

    run_cmd_quiet ("/mute /disconnect test_otr_start");
    run_cmd_quiet ("/mute /server del test_otr_start");
}
