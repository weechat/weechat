/*
 * test-irc-server.cpp - test IRC protocol functions
 *
 * Copyright (C) 2020-2021 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdio.h>
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-server.h"

extern char *irc_server_build_autojoin (struct t_irc_server *server);
}

#include "tests/tests.h"

#define IRC_FAKE_SERVER "fake"

TEST_GROUP(IrcServer)
{
};

/*
 * Tests functions:
 *   irc_server_valid
 */

TEST(IrcServer, Valid)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_search
 */

TEST(IrcServer, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_casesearch
 */

TEST(IrcServer, CaseSearch)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_search_option
 */

TEST(IrcServer, SearchOption)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_search_casemapping
 */

TEST(IrcServer, SearchCasemapping)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_search_utf8mapping
 */

TEST(IrcServer, SearchUtf8mapping)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_strcasecmp
 */

TEST(IrcServer, Strcasecmp)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_strncasecmp
 */

TEST(IrcServer, Strncasecmp)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_eval_expression
 */

TEST(IrcServer, EvalExpression)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_EvalFingerprint
 */

TEST(IrcServer, EvalFingerprint)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_sasl_enabled
 */

TEST(IrcServer, SaslEnabled)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_name_without_port
 */

TEST(IrcServer, GetNameWithoutPort)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_addresses
 */

TEST(IrcServer, SetAddresses)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_index_current_address
 */

TEST(IrcServer, SetIndexCurrentAddress)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_nicks
 */

TEST(IrcServer, SetNicks)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_nick
 */

TEST(IrcServer, SetNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_host
 */

TEST(IrcServer, SetHost)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_nick_index
 */

TEST(IrcServer, GetNickIndex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_alternate_nick
 */

TEST(IrcServer, GetAlternateNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_isupport_value
 */

TEST(IrcServer, GetIsupportValue)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_prefix_modes_chars
 */

TEST(IrcServer, SetPrefixModesChars)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_clienttagdeny
 */

TEST(IrcServer, SetClienttagdeny)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("test_clienttagdeny");
    CHECK(server);

    POINTERS_EQUAL(NULL, server->clienttagdeny);
    LONGS_EQUAL(0, server->clienttagdeny_count);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*");
    irc_server_set_clienttagdeny (server, NULL);
    POINTERS_EQUAL(NULL, server->clienttagdeny);
    LONGS_EQUAL(0, server->clienttagdeny_count);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*");
    irc_server_set_clienttagdeny (server, "");
    POINTERS_EQUAL(NULL, server->clienttagdeny);
    LONGS_EQUAL(0, server->clienttagdeny_count);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*");
    STRCMP_EQUAL("*", server->clienttagdeny);
    LONGS_EQUAL(1, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array[1]);
    LONGS_EQUAL(0, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*,-foo");
    STRCMP_EQUAL("*,-foo", server->clienttagdeny);
    LONGS_EQUAL(2, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL("!foo", server->clienttagdeny_array[1]);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array[2]);
    LONGS_EQUAL(0, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*,-foo,-example/bar");
    STRCMP_EQUAL("*,-foo,-example/bar", server->clienttagdeny);
    LONGS_EQUAL(3, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL("!foo", server->clienttagdeny_array[1]);
    STRCMP_EQUAL("!example/bar", server->clienttagdeny_array[2]);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array[3]);
    LONGS_EQUAL(0, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*,-foo,-example/bar,-typing");
    STRCMP_EQUAL("*,-foo,-example/bar,-typing", server->clienttagdeny);
    LONGS_EQUAL(4, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL("!foo", server->clienttagdeny_array[1]);
    STRCMP_EQUAL("!example/bar", server->clienttagdeny_array[2]);
    STRCMP_EQUAL("!typing", server->clienttagdeny_array[3]);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array[4]);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_server_set_lag
 */

TEST(IrcServer, SetLag)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_modes
 */

TEST(IrcServer, GetPrefixModes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_chars
 */

TEST(IrcServer, GetPrefixChars)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_mode_index
 */

TEST(IrcServer, GetPrefixModeIndex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_char_index
 */

TEST(IrcServer, GetPrefixCharIndex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_mode_for_char
 */

TEST(IrcServer, GetPrefixModeForChar)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_char_for_mode
 */

TEST(IrcServer, GetPrefixCharForMode)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_chanmodes
 */

TEST(IrcServer, GetChanmodes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_prefix_char_statusmsg
 */

TEST(IrcServer, PrefixCharStatusmsg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_max_modes
 */

TEST(IrcServer, GetMaxModes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_default_msg
 */

TEST(IrcServer, GetDefaultMsg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_alloc
 */

TEST(IrcServer, Alloc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_alloc_with_url
 */

TEST(IrcServer, AllocWithUrl)
{
    /* TODO: write tests */
}


/*
 * Tests functions:
 *   irc_server_apply_command_line_options
 */

TEST(IrcServer, ApplyCommandLineOptions)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_add
 */

TEST(IrcServer, OutqueueAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_free
 */

TEST(IrcServer, OutqueueFree)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_free_all
 */

TEST(IrcServer, OutqueueFreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_free_data
 */

TEST(IrcServer, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_free
 */

TEST(IrcServer, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_free_all
 */

TEST(IrcServer, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_copy
 */

TEST(IrcServer, Copy)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_rename
 */

TEST(IrcServer, Rename)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_reorder
 */

TEST(IrcServer, Reorder)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_send_signal
 */

TEST(IrcServer, SendSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_send
 */

TEST(IrcServer, Send)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_send_default_tags
 */

TEST(IrcServer, SetSendDefaultTags)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_tags_to_send
 */

TEST(IrcServer, GetTagsToSend)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_send
 */

TEST(IrcServer, OutqueueSend)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_send_one_msg
 */

TEST(IrcServer, SendOneMsg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_sendf
 */

TEST(IrcServer, Sendf)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_msgq_add_msg
 */

TEST(IrcServer, MsgqAddMsg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_msgq_add_unterminated
 */

TEST(IrcServer, MsgqAddUnterminated)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_msgq_add_buffer
 */

TEST(IrcServer, MsgqAddBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_msgq_flush
 */

TEST(IrcServer, MsgqFlush)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_recv_cb
 */

TEST(IrcServer, RecvCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_timer_connection_cb
 */

TEST(IrcServer, TimerConnectionCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_timer_sasl_cb
 */

TEST(IrcServer, TimerSaslCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_check_join_manual_cb
 */

TEST(IrcServer, CheckJoinManualCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_check_join_noswitch_cb
 */

TEST(IrcServer, CheckJoinNoswitchCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_check_join_smart_filtered_cb
 */

TEST(IrcServer, CheckJoinSmartFilteredCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_timer_cb
 */

TEST(IrcServer, TimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_close_connection
 */

TEST(IrcServer, CloseConnection)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_reconnect_schedule
 */

TEST(IrcServer, ReconnectSchedule)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_login
 */

TEST(IrcServer, Login)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_switch_address
 */

TEST(IrcServer, SwitchAddress)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_connect_cb
 */

TEST(IrcServer, ConnectCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_buffer_title
 */

TEST(IrcServer, SetBufferTitle)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_create_buffer
 */

TEST(IrcServer, CreateBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_fingerprint_search_algo_with_size
 */

TEST(IrcServer, FingerprintSearchAlgoWithSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_fingerprint_str_sizes
 */

TEST(IrcServer, FingerprintStrSizes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_compare_fingerprints
 */

TEST(IrcServer, CompareFingerprints)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_check_certificate_fingerprint
 */

TEST(IrcServer, CheckCertificateFingerprint)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_gnutls_callback
 */

TEST(IrcServer, GnutlsCallback)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_connect
 */

TEST(IrcServer, Connect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_reconnect
 */

TEST(IrcServer, Reconnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_auto_connect_timer_cb
 */

TEST(IrcServer, AutoConnectTimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_auto_connect
 */

TEST(IrcServer, AutoConnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_disconnect
 */

TEST(IrcServer, Disconnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_disconnect_all
 */

TEST(IrcServer, DisconnectAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_autojoin_create_buffers
 */

TEST(IrcServer, AutojoinCreateBuffers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_autojoin_channels
 */

TEST(IrcServer, AutojoinChannels)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_channel_count
 */

TEST(IrcServer, GetChannelCount)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_pv_count
 */

TEST(IrcServer, GetPvCount)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_remove_away
 */

TEST(IrcServer, RemoveAway)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_check_away
 */

TEST(IrcServer, CheckAway)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_away
 */

TEST(IrcServer, SetAway)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_xfer_send_ready_cb
 */

TEST(IrcServer, XferSendReadyCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_xfer_resume_ready_cb
 */

TEST(IrcServer, XferResumeReadyCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_xfer_send_accept_resume_cb
 */

TEST(IrcServer, XferSendAcceptResumeCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_hdata_server_cb
 */

TEST(IrcServer, HdataServerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_add_to_infolist
 */

TEST(IrcServer, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_print_log
 */

TEST(IrcServer, PrintLog)
{
    /* TODO: write tests */
}

TEST_GROUP(IrcServerConnected)
{
    struct t_irc_server *ptr_server;

    void server_recv (const char *command)
    {
        char str_command[4096];

        snprintf (str_command, sizeof (str_command),
                  "/command -buffer irc.server." IRC_FAKE_SERVER " irc "
                  "/server fakerecv %s",
                  command);
        run_cmd (str_command);
    }

    void setup ()
    {
        printf ("\n");

        /* create a fake server (no I/O) */
        run_cmd ("/server add " IRC_FAKE_SERVER " fake:127.0.0.1 "
                 "-nicks=nick1,nick2,nick3");

        /* connect to the fake server */
        run_cmd ("/connect " IRC_FAKE_SERVER);

        /* get the server pointer */
        ptr_server = irc_server_search (IRC_FAKE_SERVER);
    }

    void teardown ()
    {
        /* disconnect and delete the fake server */
        run_cmd ("/disconnect " IRC_FAKE_SERVER);
        run_cmd ("/server del " IRC_FAKE_SERVER);
        ptr_server = NULL;
    }
};

/*
 * Tests functions:
 *   irc_server_build_autojoin
 */

TEST(IrcServerConnected, BuildAutojoin)
{
    char *str;

    server_recv (":server 001 alice");

    POINTERS_EQUAL(NULL, irc_server_build_autojoin (ptr_server));

    /* join one channel */
    server_recv (":alice!user@host JOIN #test1");
    WEE_TEST_STR("#test1", irc_server_build_autojoin (ptr_server));

    /* simulate a "parted" channel */
    ptr_server->channels->part = 1;
    POINTERS_EQUAL(NULL, irc_server_build_autojoin (ptr_server));

    /* restore "part" flag */
    ptr_server->channels->part = 0;
    WEE_TEST_STR("#test1", irc_server_build_autojoin (ptr_server));

    /* add a key on channel */
    server_recv (":alice!user@host MODE #test1 +k key1");
    WEE_TEST_STR("#test1 key1",
                 irc_server_build_autojoin (ptr_server));

    /* remove key */
    server_recv (":alice!user@host MODE #test1 -k");
    WEE_TEST_STR("#test1",
                 irc_server_build_autojoin (ptr_server));

    /* join a second channel */
    server_recv (":alice!user@host JOIN #test2");
    WEE_TEST_STR("#test1,#test2", irc_server_build_autojoin (ptr_server));

    /* join a third channel */
    server_recv (":alice!user@host JOIN #test3");
    WEE_TEST_STR("#test1,#test2,#test3",
                 irc_server_build_autojoin (ptr_server));

    /* add key on first channel */
    server_recv (":alice!user@host MODE #test1 +k key1");
    WEE_TEST_STR("#test1,#test2,#test3 key1",
                 irc_server_build_autojoin (ptr_server));

    /* add key on second channel */
    server_recv (":alice!user@host MODE #test2 +k key2");
    WEE_TEST_STR("#test1,#test2,#test3 key1,key2",
                 irc_server_build_autojoin (ptr_server));

    /* add key on third channel */
    server_recv (":alice!user@host MODE #test3 +k key3");
    WEE_TEST_STR("#test1,#test2,#test3 key1,key2,key3",
                 irc_server_build_autojoin (ptr_server));

    /* remove key from first channel */
    server_recv (":alice!user@host MODE #test1 -k");
    WEE_TEST_STR("#test2,#test3,#test1 key2,key3",
                 irc_server_build_autojoin (ptr_server));

    /* remove key from second channel */
    server_recv (":alice!user@host MODE #test2 -k");
    WEE_TEST_STR("#test3,#test1,#test2 key3",
                 irc_server_build_autojoin (ptr_server));

    /* remove key from third channel */
    server_recv (":alice!user@host MODE #test3 -k");
    WEE_TEST_STR("#test1,#test2,#test3",
                 irc_server_build_autojoin (ptr_server));
}
