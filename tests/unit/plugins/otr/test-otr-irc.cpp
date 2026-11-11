/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR IRC functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-line.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-context.h"
#include "src/plugins/otr/otr-irc.h"
}

TEST_GROUP(OtrIrc)
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
 *   otr_irc_target_is_channel
 */

TEST(OtrIrc, TargetIsChannel)
{
    LONGS_EQUAL(0, otr_irc_target_is_channel ("test", NULL));
    LONGS_EQUAL(0, otr_irc_target_is_channel ("test", ""));
    LONGS_EQUAL(0, otr_irc_target_is_channel ("test", "bob"));
    LONGS_EQUAL(1, otr_irc_target_is_channel ("test", "#chan"));
}

/*
 * Test functions:
 *   otr_irc_make_account
 *   otr_irc_peer_account
 */

TEST(OtrIrc, MakeAccount)
{
    /* Not connected to the server. */
    POINTERS_EQUAL(NULL, otr_irc_make_account ("unknown_server"));
    POINTERS_EQUAL(NULL, otr_irc_peer_account ("unknown_server", "bob"));
}

/*
 * Test functions:
 *   otr_irc_account_server
 *   otr_irc_account_nick
 */

TEST(OtrIrc, AccountServerNick)
{
    char nick[128];

    STRCMP_EQUAL("", otr_irc_account_server (""));
    STRCMP_EQUAL("", otr_irc_account_server ("alice"));
    STRCMP_EQUAL("libera", otr_irc_account_server ("alice@libera"));

    otr_irc_account_nick ("", nick, sizeof (nick));
    STRCMP_EQUAL("", nick);
    otr_irc_account_nick ("alice", nick, sizeof (nick));
    STRCMP_EQUAL("alice", nick);
    otr_irc_account_nick ("alice@libera", nick, sizeof (nick));
    STRCMP_EQUAL("alice", nick);
    otr_irc_account_nick ("alice@libera", nick, 3);
    STRCMP_EQUAL("al", nick);
}

/*
 * Test functions:
 *   otr_irc_nick_cb
 */

TEST(OtrIrc, NickCb)
{
    struct t_irc_server *server;
    struct t_gui_line *ptr_last_line;
    struct t_otr_peer_data *ptr_peer_data, *ptr_new_peer_data;
    ConnContext *ptr_context;

    run_cmd_quiet ("/mute /server add test_otr_nick fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_nick");
    server = irc_server_search ("test_otr_nick");
    CHECK(server);
    irc_server_set_nick (server, "alice3");

    /* Session started with our first nick "alice". */
    ptr_context = otrl_context_find (otr_us, "bob", "alice@test_otr_nick",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_context->msgstate = OTRL_MSGSTATE_ENCRYPTED;

    /* Nick change of another user: no OTR query. */
    ptr_last_line = gui_buffers->own_lines->last_line;
    otr_irc_nick_cb (NULL, NULL, "test_otr_nick,irc_in2_nick", NULL,
                     (void *)":carol!u@h NICK :carol2");
    LONGS_EQUAL(0, core_line_displayed (ptr_last_line, "Sending OTR query"));

    /* Our second nick change ("alice2" -> "alice3"): OTR query sent to bob. */
    ptr_last_line = gui_buffers->own_lines->last_line;
    otr_irc_nick_cb (NULL, NULL, "test_otr_nick,irc_in2_nick", NULL,
                     (void *)":alice2!u@h NICK :alice3");
    LONGS_EQUAL(1, core_line_displayed (ptr_last_line, "Sending OTR query"));

    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;

    /*
     * Messages held for dave (no OTR session) with our nick "alice3", then
     * nick change ("alice3" -> "alice4"): messages moved to the new account
     * and OTR query sent with the new nick.
     */
    ptr_peer_data = otr_context_peer_data_for ("alice3@test_otr_nick", "dave");
    CHECK(ptr_peer_data);
    LONGS_EQUAL(1, otr_context_hold_message (ptr_peer_data, "line 1"));
    LONGS_EQUAL(1, otr_context_hold_message (ptr_peer_data, "line 2"));
    irc_server_set_nick (server, "alice4");
    ptr_last_line = gui_buffers->own_lines->last_line;
    otr_irc_nick_cb (NULL, NULL, "test_otr_nick,irc_in2_nick", NULL,
                     (void *)":alice3!u@h NICK :alice4");
    LONGS_EQUAL(1, core_line_displayed (ptr_last_line, "Sending OTR query"));
    POINTERS_EQUAL(NULL, ptr_peer_data->held_messages);
    ptr_new_peer_data = otr_context_peer_data_for ("alice4@test_otr_nick",
                                                   "dave");
    CHECK(ptr_new_peer_data);
    CHECK(ptr_new_peer_data->held_messages);
    STRCMP_EQUAL("line 1\nline 2", *(ptr_new_peer_data->held_messages));
    otr_context_clear_held_messages (ptr_new_peer_data);

    run_cmd_quiet ("/mute /disconnect test_otr_nick");
    run_cmd_quiet ("/mute /server del test_otr_nick");
}

/*
 * Test functions:
 *   otr_irc_nick_tolower
 */

TEST(OtrIrc, NickTolower)
{
    struct t_irc_server *server;
    char *str;

    POINTERS_EQUAL(NULL, otr_irc_nick_tolower ("test", NULL));

    /* Unknown server: casemapping "rfc1459". */
    WEE_TEST_STR("", otr_irc_nick_tolower ("unknown_server", ""));
    WEE_TEST_STR("nick{a}|~_", otr_irc_nick_tolower ("unknown_server",
                                                     "NICK[A]\\^_"));
    WEE_TEST_STR("\xc3\x89lan", otr_irc_nick_tolower ("unknown_server",
                                                     "\xc3\x89lan"));

    run_cmd_quiet ("/mute /server add test_otr_cm fake:127.0.0.1");
    run_cmd_quiet ("/connect test_otr_cm");
    server = irc_server_search ("test_otr_cm");
    CHECK(server);

    /* No casemapping received from server: "rfc1459". */
    WEE_TEST_STR("nick{a}|~", otr_irc_nick_tolower ("test_otr_cm",
                                                    "Nick[A]\\^"));

    server->casemapping = IRC_SERVER_CASEMAPPING_RFC1459;
    WEE_TEST_STR("nick{a}|~", otr_irc_nick_tolower ("test_otr_cm",
                                                    "Nick[A]\\^"));

    server->casemapping = IRC_SERVER_CASEMAPPING_STRICT_RFC1459;
    WEE_TEST_STR("nick{a}|^", otr_irc_nick_tolower ("test_otr_cm",
                                                    "Nick[A]\\^"));

    server->casemapping = IRC_SERVER_CASEMAPPING_ASCII;
    WEE_TEST_STR("nick[a]\\^", otr_irc_nick_tolower ("test_otr_cm",
                                                     "Nick[A]\\^"));

    run_cmd_quiet ("/mute /disconnect test_otr_cm");
    run_cmd_quiet ("/mute /server del test_otr_cm");
}
