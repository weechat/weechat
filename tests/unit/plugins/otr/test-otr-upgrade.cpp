/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR upgrade functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/core/weechat.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-context.h"
#include "src/plugins/otr/otr-key.h"
#include "src/plugins/otr/otr-upgrade.h"

extern int otr_upgrade_save (int save_sessions);
extern int otr_upgrade_query_pending (const char *account, const char *peer);
extern int otr_upgrade_timer_cb (const void *pointer, void *data,
                                 int remaining_calls);
extern int otr_upgrade_server_connected_cb (const void *pointer, void *data,
                                            const char *signal,
                                            const char *type_data,
                                            void *signal_data);
}

#define TEST_ACCOUNT "alice@srv_otr_upg"
#define TEST_ACCOUNT_NEW_NICK "alice2@srv_otr_upg"

TEST_GROUP(OtrUpgrade)
{
    /*
     * Get master context of a peer.
     */
    ConnContext *master (const char *account, const char *peer)
    {
        return otrl_context_find (otr_us, peer, account, OTR_PROTOCOL,
                                  OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);
    }

    /*
     * Reset per-peer data and OTR session of a peer.
     */
    void reset_peer (const char *account, const char *peer)
    {
        struct t_otr_peer_data *ptr_peer_data;
        ConnContext *ptr_context;

        ptr_peer_data = otr_context_peer_data_for (account, peer);
        CHECK(ptr_peer_data);
        ptr_peer_data->prev_log = OTR_LOG_UNTOUCHED;
        free (ptr_peer_data->log_option);
        ptr_peer_data->log_option = NULL;
        otr_context_clear_held_messages (ptr_peer_data);
        ptr_context = master (account, peer);
        CHECK(ptr_context);
        ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;
    }

    /*
     * Run the timer callback (the timer hook is removed before).
     */
    void run_timer ()
    {
        struct t_hook *ptr_timer;

        ptr_timer = otr_upgrade_timer;
        CHECK(ptr_timer);
        unhook (ptr_timer);
        LONGS_EQUAL(WEECHAT_RC_OK, otr_upgrade_timer_cb (NULL, NULL, 0));
        POINTERS_EQUAL(NULL, otr_upgrade_timer);
    }

    /*
     * Remove the upgrade file.
     */
    void remove_upgrade_file ()
    {
        char *filename;

        string_asprintf (&filename, "%s/%s.upgrade",
                         weechat_data_dir, OTR_UPGRADE_FILENAME);
        if (filename)
        {
            unlink (filename);
            free (filename);
        }
    }
};

/*
 * Test functions:
 *   otr_upgrade_query_pending
 *   otr_upgrade_peer_has_session
 *   otr_upgrade_save_peers
 *   otr_upgrade_save
 *   otr_upgrade_add_query
 *   otr_upgrade_restore_peer
 *   otr_upgrade_read_cb
 *   otr_upgrade_restore_log
 *   otr_upgrade_cancel_query
 *   otr_upgrade_move_peer_data
 *   otr_upgrade_send_query
 *   otr_upgrade_send_queries
 *   otr_upgrade_timer_cb
 *   otr_upgrade_server_connected_cb
 *   otr_upgrade_load
 *   otr_upgrade_end
 */

TEST(OtrUpgrade, SaveLoad)
{
    struct t_otr_peer_data *ptr_peer_data;
    struct t_otr_key_generation *ptr_generation;
    struct t_irc_server *ptr_server;

    /* Peer "bob": OTR session, logger level saved, messages held. */
    ptr_peer_data = otr_context_peer_data_for (TEST_ACCOUNT, "bob");
    CHECK(ptr_peer_data);
    ptr_peer_data->prev_log = 5;
    ptr_peer_data->log_option = strdup ("logger.level.irc.srv_otr_upg.bob");
    CHECK(otr_context_hold_message (ptr_peer_data, "message 1"));
    CHECK(otr_context_hold_message (ptr_peer_data, "message 2"));
    master (TEST_ACCOUNT, "bob")->msgstate = OTRL_MSGSTATE_ENCRYPTED;

    /* Peer "carol": session ended by the peer, logger level saved. */
    ptr_peer_data = otr_context_peer_data_for (TEST_ACCOUNT, "carol");
    CHECK(ptr_peer_data);
    ptr_peer_data->prev_log = OTR_LOG_DEFAULT;
    master (TEST_ACCOUNT, "carol")->msgstate = OTRL_MSGSTATE_FINISHED;

    /* Peer "dave": nothing to save. */
    CHECK(otr_context_peer_data_for (TEST_ACCOUNT, "dave"));

    /* Save without sessions (upgrade with quit): no peer restored. */
    LONGS_EQUAL(1, otr_upgrade_save (0));
    LONGS_EQUAL(1, otr_upgrade_load ());
    POINTERS_EQUAL(NULL, otr_upgrade_queries);
    POINTERS_EQUAL(NULL, otr_upgrade_timer);
    POINTERS_EQUAL(NULL, otr_upgrade_hook_connected);

    /* Save with sessions (upgrade without quit). */
    LONGS_EQUAL(1, otr_upgrade_save (1));
    reset_peer (TEST_ACCOUNT, "bob");
    reset_peer (TEST_ACCOUNT, "carol");

    /* Load: peers restored, query scheduled for "bob" only. */
    LONGS_EQUAL(1, otr_upgrade_load ());
    ptr_peer_data = otr_context_peer_data_for (TEST_ACCOUNT, "bob");
    LONGS_EQUAL(5, ptr_peer_data->prev_log);
    STRCMP_EQUAL("logger.level.irc.srv_otr_upg.bob",
                 ptr_peer_data->log_option);
    CHECK(ptr_peer_data->held_messages);
    STRCMP_EQUAL("message 1\nmessage 2", *(ptr_peer_data->held_messages));
    LONGS_EQUAL(OTR_LOG_DEFAULT,
                otr_context_peer_data_for (TEST_ACCOUNT, "carol")->prev_log);
    LONGS_EQUAL(OTR_LOG_UNTOUCHED,
                otr_context_peer_data_for (TEST_ACCOUNT, "dave")->prev_log);
    CHECK(otr_upgrade_queries);
    STRCMP_EQUAL(TEST_ACCOUNT " bob 1\n" TEST_ACCOUNT " carol 0",
                 *otr_upgrade_queries);
    CHECK(otr_upgrade_hook_connected);

    /*
     * Timer: logger level restored for "carol" (no session), server not
     * connected: query kept for "bob".
     */
    run_timer ();
    LONGS_EQUAL(OTR_LOG_UNTOUCHED,
                otr_context_peer_data_for (TEST_ACCOUNT, "carol")->prev_log);
    CHECK(otr_upgrade_queries);
    STRCMP_EQUAL(TEST_ACCOUNT " bob 1", *otr_upgrade_queries);
    LONGS_EQUAL(1, otr_upgrade_query_pending (TEST_ACCOUNT, "bob"));
    LONGS_EQUAL(0, otr_upgrade_query_pending (TEST_ACCOUNT, "carol"));
    CHECK(otr_upgrade_hook_connected);

    /* Upgrade again before the server is connected: query still pending. */
    LONGS_EQUAL(1, otr_upgrade_save (1));
    otr_upgrade_end ();
    LONGS_EQUAL(1, otr_upgrade_load ());
    CHECK(otr_upgrade_queries);
    STRCMP_EQUAL(TEST_ACCOUNT " bob 1", *otr_upgrade_queries);
    run_timer ();

    /* Query cancelled (for example private buffer closed), then restored. */
    LONGS_EQUAL(0, otr_upgrade_cancel_query (NULL, "bob", 0));
    LONGS_EQUAL(0, otr_upgrade_cancel_query ("srv_otr_upg", NULL, 0));
    LONGS_EQUAL(0, otr_upgrade_cancel_query ("srv_otr_upg2", "bob", 0));
    LONGS_EQUAL(0, otr_upgrade_cancel_query ("srv_otr_upg", "carol", 0));
    LONGS_EQUAL(1, otr_upgrade_cancel_query ("srv_otr_upg", "bob", 0));
    POINTERS_EQUAL(NULL, otr_upgrade_queries);
    POINTERS_EQUAL(NULL, otr_upgrade_hook_connected);
    ptr_peer_data = otr_context_peer_data_for (TEST_ACCOUNT, "bob");
    LONGS_EQUAL(OTR_LOG_UNTOUCHED, ptr_peer_data->prev_log);
    POINTERS_EQUAL(NULL, ptr_peer_data->held_messages);
    LONGS_EQUAL(0, otr_upgrade_cancel_query ("srv_otr_upg", "bob", 0));
    otr_upgrade_end ();
    LONGS_EQUAL(1, otr_upgrade_load ());
    STRCMP_EQUAL(TEST_ACCOUNT " bob 1", *otr_upgrade_queries);
    run_timer ();

    /* Another server connected: query kept. */
    run_cmd_quiet ("/mute /server add srv_otr_upg2 fake:127.0.0.1");
    run_cmd_quiet ("/connect srv_otr_upg2");
    ptr_server = irc_server_search ("srv_otr_upg2");
    CHECK(ptr_server);
    ptr_server->is_connected = 1;
    LONGS_EQUAL(WEECHAT_RC_OK,
                otr_upgrade_server_connected_cb (
                    NULL, NULL, "irc_server_connected",
                    WEECHAT_HOOK_SIGNAL_STRING, (void *)"srv_otr_upg2"));
    CHECK(otr_upgrade_queries);
    STRCMP_EQUAL(TEST_ACCOUNT " bob 1", *otr_upgrade_queries);

    /*
     * Server connected with another nick: data moved to the new account, no
     * private key for this account: key generated (query sent when the key
     * is generated), query removed, signal removed.
     */
    run_cmd_quiet ("/mute /server add srv_otr_upg fake:127.0.0.1");
    run_cmd_quiet ("/connect srv_otr_upg");
    ptr_server = irc_server_search ("srv_otr_upg");
    CHECK(ptr_server);
    irc_server_set_nick (ptr_server, "alice2");
    ptr_server->is_connected = 1;
    LONGS_EQUAL(WEECHAT_RC_OK,
                otr_upgrade_server_connected_cb (
                    NULL, NULL, "irc_server_connected",
                    WEECHAT_HOOK_SIGNAL_STRING, (void *)"srv_otr_upg"));
    POINTERS_EQUAL(NULL, otr_upgrade_queries);
    POINTERS_EQUAL(NULL, otr_upgrade_hook_connected);
    ptr_peer_data = otr_context_peer_data_for (TEST_ACCOUNT, "bob");
    LONGS_EQUAL(OTR_LOG_UNTOUCHED, ptr_peer_data->prev_log);
    POINTERS_EQUAL(NULL, ptr_peer_data->held_messages);
    ptr_peer_data = otr_context_peer_data_for (TEST_ACCOUNT_NEW_NICK, "bob");
    LONGS_EQUAL(5, ptr_peer_data->prev_log);
    STRCMP_EQUAL("logger.level.irc.srv_otr_upg.bob",
                 ptr_peer_data->log_option);
    CHECK(ptr_peer_data->held_messages);
    STRCMP_EQUAL("message 1\nmessage 2", *(ptr_peer_data->held_messages));
    ptr_generation = otr_key_generation_search (TEST_ACCOUNT_NEW_NICK,
                                                OTR_PROTOCOL);
    CHECK(ptr_generation);
    STRCMP_EQUAL("bob", *(ptr_generation->peers));

    otr_key_generation_free_all ();
    run_cmd_quiet ("/mute /disconnect srv_otr_upg");
    run_cmd_quiet ("/mute /disconnect srv_otr_upg2");
    run_cmd_quiet ("/mute /server del srv_otr_upg");
    run_cmd_quiet ("/mute /server del srv_otr_upg2");

    otr_upgrade_end ();
    POINTERS_EQUAL(NULL, otr_upgrade_queries);
    POINTERS_EQUAL(NULL, otr_upgrade_timer);
    POINTERS_EQUAL(NULL, otr_upgrade_hook_connected);

    reset_peer (TEST_ACCOUNT, "bob");
    reset_peer (TEST_ACCOUNT, "carol");
    reset_peer (TEST_ACCOUNT_NEW_NICK, "bob");
    remove_upgrade_file ();
}

/*
 * Test functions:
 *   otr_upgrade_signal_cb
 */

TEST(OtrUpgrade, SignalCb)
{
    otr_upgrade_keep_sessions = 0;

    /* Only save session: sessions are not kept on unload. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                otr_upgrade_signal_cb (NULL, NULL, "upgrade",
                                       WEECHAT_HOOK_SIGNAL_STRING,
                                       (void *)"save"));
    LONGS_EQUAL(0, otr_upgrade_keep_sessions);

    /* Upgrade: sessions kept. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                otr_upgrade_signal_cb (NULL, NULL, "upgrade",
                                       WEECHAT_HOOK_SIGNAL_STRING, NULL));
    LONGS_EQUAL(1, otr_upgrade_keep_sessions);

    /* Upgrade with quit: sessions ended. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                otr_upgrade_signal_cb (NULL, NULL, "upgrade",
                                       WEECHAT_HOOK_SIGNAL_STRING,
                                       (void *)"quit"));
    LONGS_EQUAL(0, otr_upgrade_keep_sessions);

    otr_upgrade_keep_sessions = 0;
    remove_upgrade_file ();
}
