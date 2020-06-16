/*
 * test-irc-protocol.cpp - test IRC protocol functions
 *
 * Copyright (C) 2019-2020 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/wee-config-file.h"
#include "src/core/wee-hashtable.h"
#include "src/core/wee-hook.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-protocol.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-nick.h"
#include "src/plugins/irc/irc-server.h"

extern int irc_protocol_is_numeric_command (const char *str);
extern int irc_protocol_log_level_for_command (const char *command);
extern const char *irc_protocol_nick_address (struct t_irc_server *server,
                                              int server_message,
                                              struct t_irc_nick *nick,
                                              const char *nickname,
                                              const char *address);
extern struct t_hashtable *irc_protocol_get_message_tags (const char *tags);
}

#include "tests/tests.h"

#define IRC_FAKE_SERVER "fake"
#define IRC_MSG_005 "PREFIX=(ohv)@%+ MAXLIST=bqeI:100 MODES=4 "         \
    "NETWORK=StaticBox STATUSMSG=@+ CALLERID=g "                        \
    "CASEMAPPING=strict-rfc1459 NICKLEN=30 MAXNICKLEN=31 "              \
    "USERLEN=16 HOSTLEN=32 CHANNELLEN=50 TOPICLEN=390 DEAF=D "          \
    "CHANTYPES=# CHANMODES=eIbq,k,flj,CFLMPQScgimnprstuz "              \
    "MONITOR=100"

struct t_irc_server *ptr_server;

TEST_GROUP(IrcProtocol)
{
};

/*
 * Tests functions:
 *   irc_protocol_is_numeric_command
 */

TEST(IrcProtocol, IsNumericCommand)
{
    LONGS_EQUAL(0, irc_protocol_is_numeric_command (NULL));
    LONGS_EQUAL(0, irc_protocol_is_numeric_command (""));
    LONGS_EQUAL(0, irc_protocol_is_numeric_command ("abc"));

    LONGS_EQUAL(1, irc_protocol_is_numeric_command ("0"));
    LONGS_EQUAL(1, irc_protocol_is_numeric_command ("1"));
    LONGS_EQUAL(1, irc_protocol_is_numeric_command ("12"));
    LONGS_EQUAL(1, irc_protocol_is_numeric_command ("123"));
}

/*
 * Tests functions:
 *   irc_protocol_log_level_for_command
 */

TEST(IrcProtocol, LogLevelForCommand)
{
    LONGS_EQUAL(0, irc_protocol_log_level_for_command (NULL));
    LONGS_EQUAL(0, irc_protocol_log_level_for_command (""));

    LONGS_EQUAL(1, irc_protocol_log_level_for_command ("privmsg"));
    LONGS_EQUAL(1, irc_protocol_log_level_for_command ("notice"));

    LONGS_EQUAL(2, irc_protocol_log_level_for_command ("nick"));

    LONGS_EQUAL(4, irc_protocol_log_level_for_command ("join"));
    LONGS_EQUAL(4, irc_protocol_log_level_for_command ("part"));
    LONGS_EQUAL(4, irc_protocol_log_level_for_command ("quit"));
    LONGS_EQUAL(4, irc_protocol_log_level_for_command ("nick_back"));

    LONGS_EQUAL(3, irc_protocol_log_level_for_command ("001"));
    LONGS_EQUAL(3, irc_protocol_log_level_for_command ("away"));
    LONGS_EQUAL(3, irc_protocol_log_level_for_command ("kick"));
    LONGS_EQUAL(3, irc_protocol_log_level_for_command ("topic"));
}

/*
 * Tests functions:
 *   irc_protocol_tags
 */

TEST(IrcProtocol, Tags)
{
    POINTERS_EQUAL(NULL, irc_protocol_tags (NULL, NULL, NULL, NULL));

    /* command */
    STRCMP_EQUAL("irc_privmsg,log1",
                 irc_protocol_tags ("privmsg", NULL, NULL, NULL));
    STRCMP_EQUAL("irc_join,log4",
                 irc_protocol_tags ("join", NULL, NULL, NULL));

    /* command and empty tags */
    STRCMP_EQUAL("irc_privmsg,log1",
                 irc_protocol_tags ("privmsg", "", NULL, NULL));
    STRCMP_EQUAL("irc_join,log4",
                 irc_protocol_tags ("join", "", NULL, NULL));

    /* command and tags */
    STRCMP_EQUAL("irc_privmsg,tag1,tag2,log1",
                 irc_protocol_tags ("privmsg", "tag1,tag2", NULL, NULL));
    STRCMP_EQUAL("irc_join,tag1,tag2,log4",
                 irc_protocol_tags ("join", "tag1,tag2", NULL, NULL));

    /* command, tags and empty nick */
    STRCMP_EQUAL("irc_privmsg,tag1,tag2,log1",
                 irc_protocol_tags ("privmsg", "tag1,tag2", "", NULL));
    STRCMP_EQUAL("irc_join,tag1,tag2,log4",
                 irc_protocol_tags ("join", "tag1,tag2", "", NULL));

    /* command, tags and nick */
    STRCMP_EQUAL("irc_privmsg,tag1,tag2,nick_alice,log1",
                 irc_protocol_tags ("privmsg", "tag1,tag2", "alice", NULL));
    STRCMP_EQUAL("irc_join,tag1,tag2,nick_bob,log4",
                 irc_protocol_tags ("join", "tag1,tag2", "bob", NULL));

    /* command, tags, nick and empty address */
    STRCMP_EQUAL("irc_privmsg,tag1,tag2,nick_alice,log1",
                 irc_protocol_tags ("privmsg", "tag1,tag2", "alice", ""));
    STRCMP_EQUAL("irc_join,tag1,tag2,nick_bob,log4",
                 irc_protocol_tags ("join", "tag1,tag2", "bob", ""));

    /* command, tags, nick and address */
    STRCMP_EQUAL("irc_privmsg,tag1,tag2,nick_alice,host_example.com,log1",
                 irc_protocol_tags ("privmsg", "tag1,tag2", "alice",
                                    "example.com"));
    STRCMP_EQUAL("irc_join,tag1,tag2,nick_bob,host_example.com,log4",
                 irc_protocol_tags ("join", "tag1,tag2", "bob",
                                    "example.com"));
}

/*
 * Tests functions:
 *   irc_protocol_get_message_tags
 */

TEST(IrcProtocol, GetMessageTags)
{
    struct t_hashtable *hashtable;

    POINTERS_EQUAL(NULL, irc_protocol_get_message_tags (NULL));
    POINTERS_EQUAL(NULL, irc_protocol_get_message_tags (""));

    hashtable = irc_protocol_get_message_tags ("abc");
    CHECK(hashtable);
    LONGS_EQUAL(1, hashtable->items_count);
    POINTERS_EQUAL(NULL, (const char *)hashtable_get (hashtable, "abc"));
    hashtable_free (hashtable);

    hashtable = irc_protocol_get_message_tags ("abc=def");
    CHECK(hashtable);
    LONGS_EQUAL(1, hashtable->items_count);
    STRCMP_EQUAL("def", (const char *)hashtable_get (hashtable, "abc"));
    hashtable_free (hashtable);

    hashtable = irc_protocol_get_message_tags ("aaa=bbb;ccc;example.com/ddd=eee");
    CHECK(hashtable);
    LONGS_EQUAL(3, hashtable->items_count);
    STRCMP_EQUAL("bbb", (const char *)hashtable_get (hashtable, "aaa"));
    POINTERS_EQUAL(NULL, (const char *)hashtable_get (hashtable, "ccc"));
    STRCMP_EQUAL("eee", (const char *)hashtable_get (hashtable, "example.com/ddd"));
    hashtable_free (hashtable);
}

/*
 * Tests functions:
 *   irc_protocol_parse_time
 */

TEST(IrcProtocol, ParseTime)
{
    /* invalid time formats */
    LONGS_EQUAL(0, irc_protocol_parse_time (NULL));
    LONGS_EQUAL(0, irc_protocol_parse_time (""));
    LONGS_EQUAL(0, irc_protocol_parse_time ("invalid"));

    /* incomplete time formats */
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01"));
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01-13"));
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01-13T14"));
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01-13T14:37"));

    /* valid time with ISO 8601 format*/
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("2019-01-13T13:38:19.123Z"));
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("2019-01-13T13:38:19.123"));
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("2019-01-13T13:38:19"));

    /* valid time as timestamp */
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("1547386699.123"));
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("1547386699"));
}

TEST_GROUP(IrcProtocolWithServer)
{
    void server_recv (const char *command)
    {
        char str_command[4096];

        snprintf (str_command, sizeof (str_command),
                  "/command -buffer irc.server." IRC_FAKE_SERVER " irc "
                  "/server fakerecv %s",
                  command);
        run_cmd (str_command);
    }

    static int signal_cb (const void *pointer, void *data, const char *signal,
                          const char *type_data, void *signal_data)
    {
        /* make C++ compiler happy */
        (void) pointer;
        (void) signal;
        (void) type_data;

        snprintf ((char *)data, 1024, "%s", (const char *)signal_data);

        return WEECHAT_RC_OK;
    }

    void server_recv_check_response (const char *command,
                                     const char *expected_response)
    {
        char *data, response_sent[1024], str_error[4096];
        struct t_hook *ptr_hook;

        data = (char *)malloc (1024);
        data[0] = '\0';

        ptr_hook = hook_signal (NULL, IRC_FAKE_SERVER ",irc_out1_*",
                                &signal_cb, expected_response, data);

        server_recv (command);

        snprintf (response_sent, sizeof (response_sent), "%s", data);

        unhook (ptr_hook);

        if (expected_response && !response_sent[0])
        {
            snprintf (str_error, sizeof (str_error),
                      "Message received: \"%s\", expected response was "
                      "\"%s\", but it has not been sent to the IRC server",
                      command,
                      expected_response);
            FAIL(str_error);
        }

        if (!expected_response && response_sent[0])
        {
            snprintf (str_error, sizeof (str_error),
                      "Message received: \"%s\", expected no response, but "
                      "an unexpected response was sent to the IRC server: "
                      "\"%s\"",
                      command,
                      response_sent);
            FAIL(str_error);
        }

        if (expected_response)
        {
            STRCMP_EQUAL(expected_response, response_sent);
        }
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
 *   irc_protocol_nick_address
 */

TEST(IrcProtocolWithServer, NickAddress)
{
    struct t_irc_nick *ptr_nick;
    char result[1024];

    server_recv (":server 001 alice");
    server_recv (":alice!user@host JOIN #test");

    ptr_nick = ptr_server->channels->nicks;

    STRCMP_EQUAL("", irc_protocol_nick_address (NULL, 0, NULL,
                                                NULL, NULL));
    STRCMP_EQUAL("", irc_protocol_nick_address (ptr_server, 0, NULL,
                                                NULL, NULL));
    STRCMP_EQUAL("", irc_protocol_nick_address (ptr_server, 0, ptr_nick,
                                                NULL, NULL));
    STRCMP_EQUAL("", irc_protocol_nick_address (ptr_server, 0, ptr_nick,
                                                NULL, NULL));

    snprintf (result, sizeof (result),
              "%s%s%s",
              ptr_nick->color,
              "alice",
              gui_color_get_custom ("reset"));
    STRCMP_EQUAL(result, irc_protocol_nick_address (ptr_server, 0, ptr_nick,
                                                    "alice", NULL));

    snprintf (result, sizeof (result),
              "%s%s %s(%s%s%s)%s",
              ptr_nick->color,
              "alice",
              gui_color_search_config ("chat_delimiters"),
              gui_color_search_config ("chat_host"),
              "example.com",
              gui_color_search_config ("chat_delimiters"),
              gui_color_get_custom ("reset"));
    STRCMP_EQUAL(result, irc_protocol_nick_address (ptr_server, 0, ptr_nick,
                                                    "alice", "example.com"));

    config_file_option_set (irc_config_look_color_nicks_in_server_messages,
                            "off", 1);
    snprintf (result, sizeof (result),
              "%s%s %s(%s%s%s)%s",
              ptr_nick->color,
              "alice",
              gui_color_search_config ("chat_delimiters"),
              gui_color_search_config ("chat_host"),
              "example.com",
              gui_color_search_config ("chat_delimiters"),
              gui_color_get_custom ("reset"));
    STRCMP_EQUAL(result, irc_protocol_nick_address (ptr_server, 0, ptr_nick,
                                                    "alice", "example.com"));
    snprintf (result, sizeof (result),
              "%s%s %s(%s%s%s)%s",
              gui_color_search_config ("chat_nick"),
              "alice",
              gui_color_search_config ("chat_delimiters"),
              gui_color_search_config ("chat_host"),
              "example.com",
              gui_color_search_config ("chat_delimiters"),
              gui_color_get_custom ("reset"));
    STRCMP_EQUAL(result, irc_protocol_nick_address (ptr_server, 1, ptr_nick,
                                                    "alice", "example.com"));
    config_file_option_reset (irc_config_look_color_nicks_in_server_messages, 0);
}

/*
 * Tests functions:
 *   irc_protocol_recv_command (command not found)
 */

TEST(IrcProtocolWithServer, recv_command_not_found)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host XYZ");
    server_recv (":alice!user@host XYZ abc def");

    server_recv (":alice!user@host 099");
    server_recv (":alice!user@host 099 abc def");
}

/*
 * Tests functions:
 *   irc_protocol_recv_command (invalid message)
 */

TEST(IrcProtocolWithServer, recv_command_invalid_message)
{
    server_recv (":server 001 alice");

    server_recv (":");
    server_recv ("abc");
    server_recv (":");
    server_recv (":alice!user@host");
    server_recv ("@");
    server_recv ("@test");
    server_recv ("@test :");
    server_recv ("@test :abc");
}

/*
 * Tests functions:
 *   irc_protocol_cb_account (without account-notify capability)
 */

TEST(IrcProtocolWithServer, account_without_account_notify_cap)
{
    struct t_irc_nick *ptr_nick;

    server_recv (":server 001 alice");
    server_recv (":alice!user@host JOIN #test");

    ptr_nick = ptr_server->channels->nicks;

    POINTERS_EQUAL(NULL, ptr_nick->account);

    /* not enough arguments */
    server_recv (":alice!user@host ACCOUNT");
    POINTERS_EQUAL(NULL, ptr_nick->account);

    server_recv (":alice!user@host ACCOUNT *");
    POINTERS_EQUAL(NULL, ptr_nick->account);

    server_recv (":alice!user@host ACCOUNT :*");
    POINTERS_EQUAL(NULL, ptr_nick->account);

    server_recv (":alice!user@host ACCOUNT new_account");
    POINTERS_EQUAL(NULL, ptr_nick->account);

    server_recv (":alice!user@host ACCOUNT :new_account");
    POINTERS_EQUAL(NULL, ptr_nick->account);
}

/*
 * Tests functions:
 *   irc_protocol_cb_account (with account-notify capability)
 */

TEST(IrcProtocolWithServer, account_with_account_notify_cap)
{
    struct t_irc_nick *ptr_nick;

    /* assume "account-notify" capability is enabled in server */
    hashtable_set (ptr_server->cap_list, "account-notify", NULL);

    server_recv (":server 001 alice");
    server_recv (":alice!user@host JOIN #test");

    ptr_nick = ptr_server->channels->nicks;

    POINTERS_EQUAL(NULL, ptr_nick->account);

    server_recv (":alice!user@host ACCOUNT new_account");
    STRCMP_EQUAL("new_account", ptr_nick->account);

    server_recv (":alice!user@host ACCOUNT :new_account2");
    STRCMP_EQUAL("new_account2", ptr_nick->account);

    server_recv (":alice!user@host ACCOUNT *");
    POINTERS_EQUAL(NULL, ptr_nick->account);

    server_recv (":alice!user@host ACCOUNT :new_account3");
    STRCMP_EQUAL("new_account3", ptr_nick->account);

    server_recv (":alice!user@host ACCOUNT :*");
    POINTERS_EQUAL(NULL, ptr_nick->account);
}

/*
 * Tests functions:
 *   irc_protocol_cb_authenticate
 */

TEST(IrcProtocolWithServer, authenticate)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv ("AUTHENTICATE");

    server_recv ("AUTHENTICATE "
                 "QQDaUzXAmVffxuzFy77XWBGwABBQAgdinelBrKZaR3wE7nsIETuTVY=");
}

/*
 * Tests functions:
 *   irc_protocol_cb_away
 */

TEST(IrcProtocolWithServer, away)
{
    struct t_irc_nick *ptr_nick;

    server_recv (":server 001 alice");
    server_recv (":alice!user@host JOIN #test");

    ptr_nick = ptr_server->channels->nicks;

    LONGS_EQUAL(0, ptr_nick->away);

    server_recv (":alice!user@host AWAY Bye");
    LONGS_EQUAL(1, ptr_nick->away);

    server_recv (":alice!user@host AWAY :Holidays now!");
    LONGS_EQUAL(1, ptr_nick->away);

    server_recv (":alice!user@host AWAY");
    LONGS_EQUAL(0, ptr_nick->away);
}

/*
 * Tests functions:
 *   irc_protocol_cb_cap
 */

TEST(IrcProtocolWithServer, cap)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server CAP");
    server_recv (":server CAP *");

    server_recv (":server CAP * LS :identify-msg multi-prefix sasl");
    server_recv (":server CAP * LS * :identify-msg multi-prefix sasl");
    server_recv (":server CAP * LIST :identify-msg multi-prefix sasl");
    server_recv (":server CAP * LIST * :identify-msg multi-prefix sasl");
    server_recv (":server CAP * NEW :identify-msg multi-prefix sasl");
    server_recv (":server CAP * DEL :identify-msg multi-prefix sasl");
    server_recv (":server CAP * ACK :sasl");
    server_recv (":server CAP * NAK :sasl");
}

/*
 * Tests functions:
 *   irc_protocol_cb_chghost
 */

TEST(IrcProtocolWithServer, chghost)
{
    struct t_irc_nick *ptr_nick, *ptr_nick2;

    server_recv (":server 001 alice");
    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host JOIN #test");

    ptr_nick = ptr_server->channels->nicks;
    ptr_nick2 = ptr_server->channels->last_nick;

    STRCMP_EQUAL("user@host", ptr_nick->host);

    /* not enough arguments */
    server_recv (":alice!user@host CHGHOST");
    server_recv (":alice!user@host CHGHOST user2");
    STRCMP_EQUAL("user@host", ptr_nick->host);

    /* self nick */
    server_recv (":alice!user@host CHGHOST user2 host2");
    STRCMP_EQUAL("user2@host2", ptr_nick->host);

    server_recv (":alice!user@host CHGHOST user2 host2");
    STRCMP_EQUAL("user2@host2", ptr_nick->host);

    server_recv (":alice!user2@host2 CHGHOST user3 :host3");
    STRCMP_EQUAL("user3@host3", ptr_nick->host);

    /* another nick */
    server_recv (":bob!user@host CHGHOST user_bob_2 host_bob_2");
    STRCMP_EQUAL("user_bob_2@host_bob_2", ptr_nick2->host);
}

/*
 * Tests functions:
 *   irc_protocol_cb_error
 */

TEST(IrcProtocolWithServer, error)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv ("ERROR");

    server_recv ("ERROR test");
    server_recv ("ERROR :Closing Link: irc.server.org (Bad Password)");
}

/*
 * Tests functions:
 *   irc_protocol_cb_invite
 */

TEST(IrcProtocolWithServer, invite)
{
    server_recv (":server 001 alice");

    server_recv (":bob!user@host INVITE alice #channel");
    server_recv (":bob!user@host INVITE xxx #channel");
}

/*
 * Tests functions:
 *   irc_protocol_cb_join
 */

TEST(IrcProtocolWithServer, join)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    server_recv (":server 001 alice");

    POINTERS_EQUAL(NULL, ptr_server->channels);

    /* not enough arguments */
    server_recv (":alice!user@host JOIN");
    POINTERS_EQUAL(NULL, ptr_server->channels);

    /* join of a user while the channel does not yet exist in local */
    server_recv (":bob!user@host JOIN #test");

    server_recv (":alice!user@host JOIN #test");

    ptr_channel = ptr_server->channels;

    CHECK(ptr_channel);
    CHECK(ptr_channel == ptr_server->last_channel);

    LONGS_EQUAL(IRC_CHANNEL_TYPE_CHANNEL, ptr_channel->type);
    STRCMP_EQUAL("#test", ptr_channel->name);
    POINTERS_EQUAL(NULL, ptr_channel->topic);
    POINTERS_EQUAL(NULL, ptr_channel->modes);
    LONGS_EQUAL(0, ptr_channel->limit);
    POINTERS_EQUAL(NULL, ptr_channel->key);
    LONGS_EQUAL(0, ptr_channel->checking_whox);
    POINTERS_EQUAL(NULL, ptr_channel->away_message);
    LONGS_EQUAL(0, ptr_channel->has_quit_server);
    LONGS_EQUAL(0, ptr_channel->cycle);
    LONGS_EQUAL(0, ptr_channel->part);
    LONGS_EQUAL(0, ptr_channel->part);
    POINTERS_EQUAL(NULL, ptr_channel->pv_remote_nick_color);
    POINTERS_EQUAL(NULL, ptr_channel->hook_autorejoin);

    ptr_nick = ptr_channel->nicks;

    LONGS_EQUAL(1, ptr_channel->nicks_count);
    CHECK(ptr_nick);
    CHECK(ptr_nick == ptr_channel->last_nick);
    STRCMP_EQUAL("alice", ptr_nick->name);
    STRCMP_EQUAL("user@host", ptr_nick->host);
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);
    LONGS_EQUAL(0, ptr_nick->away);
    POINTERS_EQUAL(NULL, ptr_nick->account);
    POINTERS_EQUAL(NULL, ptr_nick->realname);
    CHECK(ptr_nick->color);

    CHECK(ptr_channel->buffer);

    server_recv (":bob!user@host JOIN #test * :Bob Name");

    ptr_nick = ptr_channel->last_nick;

    LONGS_EQUAL(2, ptr_channel->nicks_count);
    CHECK(ptr_nick);
    STRCMP_EQUAL("bob", ptr_nick->name);
    STRCMP_EQUAL("user@host", ptr_nick->host);
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);
    LONGS_EQUAL(0, ptr_nick->away);
    POINTERS_EQUAL(NULL, ptr_nick->account);
    STRCMP_EQUAL("Bob Name", ptr_nick->realname);
    CHECK(ptr_nick->color);

    server_recv (":carol!user@host JOIN #test carol_account :Carol Name");

    ptr_nick = ptr_channel->last_nick;

    LONGS_EQUAL(3, ptr_channel->nicks_count);
    CHECK(ptr_nick);
    STRCMP_EQUAL("carol", ptr_nick->name);
    STRCMP_EQUAL("user@host", ptr_nick->host);
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);
    LONGS_EQUAL(0, ptr_nick->away);
    STRCMP_EQUAL("carol_account", ptr_nick->account);
    STRCMP_EQUAL("Carol Name", ptr_nick->realname);
    CHECK(ptr_nick->color);
}

/*
 * Tests functions:
 *   irc_protocol_cb_kick
 */

TEST(IrcProtocolWithServer, kick)
{
    struct t_irc_channel *ptr_channel;

    server_recv (":server 001 alice");

    POINTERS_EQUAL(NULL, ptr_server->channels);

    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host JOIN #test");

    ptr_channel = ptr_server->channels;

    CHECK(ptr_channel);
    CHECK(ptr_channel->nicks);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    CHECK(ptr_channel->nicks->next_nick);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);

    /* not enough arguments */
    server_recv (":alice!user@host KICK");
    server_recv (":alice!user@host KICK #test");
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);

    /* channel not found */
    server_recv (":alice!user@host KICK #xyz");

    /* without kick reason */
    server_recv (":alice!user@host KICK #test bob");
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    server_recv (":bob!user@host JOIN #test");

    /* with kick reason */
    server_recv (":alice!user@host KICK #test bob :no spam here!");
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    server_recv (":bob!user@host JOIN #test");

    /* kick of self nick */
    server_recv (":bob!user@host KICK #test alice :no spam here!");
    POINTERS_EQUAL(NULL, ptr_channel->nicks);
}

/*
 * Tests functions:
 *   irc_protocol_cb_kill
 */

TEST(IrcProtocolWithServer, kill)
{
    struct t_irc_channel *ptr_channel;

    server_recv (":server 001 alice");

    POINTERS_EQUAL(NULL, ptr_server->channels);

    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host JOIN #test");

    ptr_channel = ptr_server->channels;

    CHECK(ptr_channel);
    CHECK(ptr_channel->nicks);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    CHECK(ptr_channel->nicks->next_nick);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);

    /* not enough arguments */
    server_recv (":alice!user@host KILL");
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);

    /* without kill reason */
    server_recv (":alice!user@host KILL bob");
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    server_recv (":bob!user@host JOIN #test");

    /* with kill reason */
    server_recv (":alice!user@host KILL bob :killed by admin");
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    server_recv (":bob!user@host JOIN #test");

    /* kill of self nick */
    server_recv (":bob!user@host KILL alice :killed by admin");
    POINTERS_EQUAL(NULL, ptr_channel->nicks);
}

/*
 * Tests functions:
 *   irc_protocol_cb_mode
 */

TEST(IrcProtocolWithServer, mode)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    server_recv (":server 001 alice");

    POINTERS_EQUAL(NULL, ptr_server->channels);

    server_recv (":alice!user@host JOIN #test");

    ptr_channel = ptr_server->channels;
    CHECK(ptr_channel);
    POINTERS_EQUAL(NULL, ptr_channel->modes);
    ptr_nick = ptr_channel->nicks;
    CHECK(ptr_nick);
    STRCMP_EQUAL("alice", ptr_nick->name);
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);

    /* not enough arguments */
    server_recv (":admin MODE");
    server_recv (":admin MODE #test");
    POINTERS_EQUAL(NULL, ptr_channel->modes);

    /* channel mode */
    server_recv (":admin MODE #test +nt");
    STRCMP_EQUAL("+tn", ptr_channel->modes);

    /* channel mode removed */
    server_recv (":admin MODE #test -n");
    STRCMP_EQUAL("+t", ptr_channel->modes);

    /* channel mode removed */
    server_recv (":admin MODE #test -t");
    POINTERS_EQUAL(NULL, ptr_channel->modes);

    /* nick mode '@' on channel #test */
    server_recv (":admin MODE #test +o alice");
    STRCMP_EQUAL("@ ", ptr_nick->prefixes);
    STRCMP_EQUAL("@", ptr_nick->prefix);

    /* another nick mode '+' on channel #test */
    server_recv (":admin MODE #test +v alice");
    STRCMP_EQUAL("@+", ptr_nick->prefixes);
    STRCMP_EQUAL("@", ptr_nick->prefix);

    /* nick mode '@' removed on channel #test */
    server_recv (":admin MODE #test -o alice");
    STRCMP_EQUAL(" +", ptr_nick->prefixes);
    STRCMP_EQUAL("+", ptr_nick->prefix);

    /* nick mode '+' removed on channel #test */
    server_recv (":admin MODE #test -v alice");
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);

    /* nick mode 'i' */
    POINTERS_EQUAL(NULL, ptr_server->nick_modes);
    server_recv (":admin MODE alice +i");
    STRCMP_EQUAL("i", ptr_server->nick_modes);

    /* nick mode 'R' */
    server_recv (":admin MODE alice +R");
    STRCMP_EQUAL("iR", ptr_server->nick_modes);

    /* remove nick mode 'i' */
    server_recv (":admin MODE alice -i");
    STRCMP_EQUAL("R", ptr_server->nick_modes);
}

/*
 * Tests functions:
 *   irc_protocol_cb_nick
 */

TEST(IrcProtocolWithServer, nick)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick1, *ptr_nick2;

    server_recv (":server 001 alice");

    POINTERS_EQUAL(NULL, ptr_server->channels);

    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host JOIN #test");

    server_recv (":bob!user@host PRIVMSG alice :hi Alice!");

    ptr_channel = ptr_server->channels;
    CHECK(ptr_channel);
    ptr_nick1 = ptr_channel->nicks;
    CHECK(ptr_nick1);
    ptr_nick2 = ptr_nick1->next_nick;
    CHECK(ptr_nick2);
    STRCMP_EQUAL("alice", ptr_nick1->name);
    STRCMP_EQUAL("bob", ptr_nick2->name);

    /* not enough arguments */
    server_recv (":alice!user@host NICK");
    STRCMP_EQUAL("alice", ptr_nick1->name);
    STRCMP_EQUAL("bob", ptr_nick2->name);

    /* new nick for alice */
    server_recv (":alice!user@host NICK alice_away");
    STRCMP_EQUAL("alice_away", ptr_nick1->name);

    /* new nick for alice_away (with ":") */
    server_recv (":alice_away!user@host NICK :alice2");
    STRCMP_EQUAL("alice2", ptr_nick1->name);

    /* new nick for bob */
    server_recv (":bob!user@host NICK bob_away");
    STRCMP_EQUAL("bob_away", ptr_nick2->name);

    /* new nick for bob_away (with ":") */
    server_recv (":bob_away!user@host NICK :bob2");
    STRCMP_EQUAL("bob2", ptr_nick2->name);

    STRCMP_EQUAL("bob2", ptr_server->last_channel->name);
}

/*
 * Tests functions:
 *   irc_protocol_cb_notice
 */

TEST(IrcProtocolWithServer, notice)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv ("NOTICE");
    server_recv ("NOTICE AUTH");
    server_recv (":bob!user@host NOTICE");
    server_recv (":bob!user@host NOTICE #test");
    server_recv (":bob!user@host NOTICE alice");

    /* notice from server */
    server_recv ("NOTICE AUTH :*** Looking up your hostname...");

    /* notice to channel/user */
    server_recv (":bob!user@host NOTICE #test :this is the notice");
    server_recv (":bob!user@host NOTICE alice :this is the notice");

    /* notice to ops of channel */
    server_recv (":bob!user@host NOTICE @#test :this is the notice");

    /* notice from self nick (case of bouncer) */
    server_recv (":alice!user@host NOTICE alice :this is the notice");

    /* notice with channel name at beginning */
    server_recv (":bob!user@host NOTICE alice :[#test] this is the notice");
    server_recv (":bob!user@host NOTICE alice :(#test) this is the notice");
    server_recv (":bob!user@host NOTICE alice :{#test} this is the notice");
    server_recv (":bob!user@host NOTICE alice :<#test> this is the notice");

    /* broken CTCP to channel */
    server_recv (":bob!user@host NOTICE #test :\01");
    server_recv (":bob!user@host NOTICE #test :\01TEST");
    server_recv (":bob!user@host NOTICE #test :\01ACTION");
    server_recv (":bob!user@host NOTICE #test :\01ACTION is testing");
    server_recv (":bob!user@host NOTICE #test :\01VERSION");
    server_recv (":bob!user@host NOTICE #test :\01DCC");
    server_recv (":bob!user@host NOTICE #test :\01DCC SEND");
    server_recv (":bob!user@host NOTICE #test :\01DCC SEND file.txt");
    server_recv (":bob!user@host NOTICE #test :\01DCC SEND file.txt 1 2 3");

    /* broken CTCP to user */
    server_recv (":bob!user@host NOTICE alice :\01");
    server_recv (":bob!user@host NOTICE alice :\01TEST");
    server_recv (":bob!user@host NOTICE alice :\01ACTION");
    server_recv (":bob!user@host NOTICE alice :\01ACTION is testing");
    server_recv (":bob!user@host NOTICE alice :\01VERSION");
    server_recv (":bob!user@host NOTICE alice :\01DCC");
    server_recv (":bob!user@host NOTICE alice :\01DCC SEND");
    server_recv (":bob!user@host NOTICE alice :\01DCC SEND file.txt");
    server_recv (":bob!user@host NOTICE alice :\01DCC SEND file.txt 1 2 3");

    /* valid CTCP to channel */
    server_recv (":bob!user@host NOTICE #test :\01TEST\01");
    server_recv (":bob!user@host NOTICE #test :\01ACTION\01");
    server_recv (":bob!user@host NOTICE #test :\01ACTION is testing\01");
    server_recv (":bob!user@host NOTICE #test :\01VERSION\01");
    server_recv (":bob!user@host NOTICE #test :\01DCC SEND file.txt 1 2 3\01");

    /* valid CTCP to user */
    server_recv (":bob!user@host NOTICE alice :\01TEST\01");
    server_recv (":bob!user@host NOTICE alice :\01ACTION\01");
    server_recv (":bob!user@host NOTICE alice :\01ACTION is testing\01");
    server_recv (":bob!user@host NOTICE alice :\01VERSION\01");
    server_recv (":bob!user@host NOTICE alice :\01DCC SEND file.txt 1 2 3\01");
}

/*
 * Tests functions:
 *   irc_protocol_cb_part
 */

TEST(IrcProtocolWithServer, part)
{
    server_recv (":server 001 alice");

    POINTERS_EQUAL(NULL, ptr_server->channels);

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":alice!user@host PART");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    CHECK(ptr_server->channels->nicks);
    LONGS_EQUAL(0, ptr_server->channels->part);

    /* channel not found */
    server_recv (":alice!user@host PART #xyz");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    CHECK(ptr_server->channels->nicks);
    LONGS_EQUAL(0, ptr_server->channels->part);

    server_recv (":alice!user@host PART #test");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    POINTERS_EQUAL(NULL, ptr_server->channels->nicks);
    LONGS_EQUAL(1, ptr_server->channels->part);

    server_recv (":alice!user@host JOIN #test");

    server_recv (":alice!user@host PART #test :part message");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    POINTERS_EQUAL(NULL, ptr_server->channels->nicks);
    LONGS_EQUAL(1, ptr_server->channels->part);

    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host JOIN #test");

    /* part from another user */
    server_recv (":bob!user@host PART #test :part message");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    CHECK(ptr_server->channels->nicks == ptr_server->channels->last_nick);
    LONGS_EQUAL(0, ptr_server->channels->part);
}

/*
 * Tests functions:
 *   irc_protocol_cb_ping
 */

TEST(IrcProtocolWithServer, ping)
{
    server_recv (":server 001 alice");

    /* not enough arguments, no response */
    server_recv_check_response ("PING", NULL);

    server_recv_check_response ("PING :123456789", "PONG :123456789");
}

/*
 * Tests functions:
 *   irc_protocol_cb_pong
 */

TEST(IrcProtocolWithServer, pong)
{
    server_recv (":server 001 alice");

    server_recv (":server PONG");
    server_recv (":server PONG server");
    server_recv (":server PONG server :server");
}

/*
 * Tests functions:
 *   irc_protocol_cb_privmsg
 */

TEST(IrcProtocolWithServer, privmsg)
{
    char *info, message[1024];

    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":bob!user@host PRIVMSG");
    server_recv (":bob!user@host PRIVMSG #test");
    server_recv (":bob!user@host PRIVMSG alice");

    /* message to channel/user */
    server_recv (":bob!user@host PRIVMSG #test :this is the message");
    server_recv (":bob!user@host PRIVMSG alice :this is the message");

    /* message with tags to channel/user */
    server_recv ("@tag1=value1;tag2=value2 :bob!user@host PRIVMSG #test "
                 ":this is the message");
    server_recv ("@tag1=value1;tag2=value2 :bob!user@host PRIVMSG alice "
                 ":this is the message");

    /* message to ops of channel */
    server_recv (":bob!user@host PRIVMSG @#test :this is the message");

    /* message from self nick (case of bouncer) */
    server_recv (":alice!user@host PRIVMSG alice :this is the message");

    /* broken CTCP to channel */
    server_recv (":bob!user@host PRIVMSG #test :\01");
    server_recv (":bob!user@host PRIVMSG #test :\01TEST");
    server_recv (":bob!user@host PRIVMSG #test :\01ACTION");
    server_recv (":bob!user@host PRIVMSG #test :\01ACTION is testing");
    server_recv (":bob!user@host PRIVMSG #test :\01VERSION");
    server_recv (":bob!user@host PRIVMSG #test :\01DCC");
    server_recv (":bob!user@host PRIVMSG #test :\01DCC SEND");
    server_recv (":bob!user@host PRIVMSG #test :\01DCC SEND file.txt");
    server_recv (":bob!user@host PRIVMSG #test :\01DCC SEND file.txt 1 2 3");

    /* broken CTCP to user */
    server_recv (":bob!user@host PRIVMSG alice :\01");
    server_recv (":bob!user@host PRIVMSG alice :\01TEST");
    server_recv (":bob!user@host PRIVMSG alice :\01ACTION");
    server_recv (":bob!user@host PRIVMSG alice :\01ACTION is testing");
    server_recv (":bob!user@host PRIVMSG alice :\01VERSION");
    server_recv (":bob!user@host PRIVMSG alice :\01DCC");
    server_recv (":bob!user@host PRIVMSG alice :\01DCC SEND");
    server_recv (":bob!user@host PRIVMSG alice :\01DCC SEND file.txt");
    server_recv (":bob!user@host PRIVMSG alice :\01DCC SEND file.txt 1 2 3");

    /* valid CTCP to channel */
    server_recv (":bob!user@host PRIVMSG #test :\01TEST\01");
    server_recv (":bob!user@host PRIVMSG #test :\01ACTION\01");
    server_recv (":bob!user@host PRIVMSG #test :\01ACTION is testing\01");
    server_recv (":bob!user@host PRIVMSG #test :\01VERSION\01");
    server_recv (":bob!user@host PRIVMSG #test :\01DCC SEND file.txt 1 2 3\01");

    /* valid CTCP to user */
    server_recv_check_response (
        ":bob!user@host PRIVMSG alice :\01TEST\01", NULL);
    server_recv_check_response (
        ":bob!user@host PRIVMSG alice :\01ACTION\01", NULL);
    server_recv_check_response (
        ":bob!user@host PRIVMSG alice :\01ACTION is testing\01", NULL);
    server_recv (":bob!user@host PRIVMSG alice :\01VERSION\01");
    info = hook_info_get (NULL, "weechat_site_download", "");
    snprintf (message, sizeof (message),
              "NOTICE bob :\01SOURCE %s\01", info);
    server_recv_check_response (
        ":bob!user@host PRIVMSG alice :\01SOURCE\01", message);
    free (info);
    server_recv_check_response (
        ":bob!user@host PRIVMSG alice :\01DCC SEND file.txt 1 2 3\01", NULL);
}

/*
 * Tests functions:
 *   irc_protocol_cb_quit
 */

TEST(IrcProtocolWithServer, quit)
{
    struct t_irc_channel *ptr_channel;

    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host PRIVMSG alice :hi Alice!");

    ptr_channel = ptr_server->channels;

    server_recv (":bob!user@host JOIN #test");
    LONGS_EQUAL(2, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    STRCMP_EQUAL("bob", ptr_channel->last_nick->name);

    /* without quit message */
    server_recv (":bob!user@host QUIT");
    LONGS_EQUAL(1, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    server_recv (":bob!user@host JOIN #test");
    LONGS_EQUAL(2, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    STRCMP_EQUAL("bob", ptr_channel->last_nick->name);

    /* with quit message */
    server_recv (":bob!user@host QUIT :quit message");
    LONGS_EQUAL(1, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);
}

/*
 * Tests functions:
 *   irc_protocol_cb_topic
 */

TEST(IrcProtocolWithServer, topic)
{
    struct t_irc_channel *ptr_channel;

    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    ptr_channel = ptr_server->channels;
    POINTERS_EQUAL(NULL, ptr_channel->topic);

    /* not enough arguments */
    server_recv (":alice!user@host TOPIC");
    POINTERS_EQUAL(NULL, ptr_channel->topic);

    /* not a channel */
    server_recv (":alice!user@host TOPIC bob");

    /* empty topic */
    server_recv (":alice!user@host TOPIC #test");
    POINTERS_EQUAL(NULL, ptr_channel->topic);

    /* new topic */
    server_recv (":alice!user@host TOPIC #test :new topic");
    STRCMP_EQUAL("new topic", ptr_channel->topic);

    /* another new topic */
    server_recv (":alice!user@host TOPIC #test :another new topic");
    STRCMP_EQUAL("another new topic", ptr_channel->topic);

    /* empty topic */
    server_recv (":alice!user@host TOPIC #test");
    POINTERS_EQUAL(NULL, ptr_channel->topic);
}

/*
 * Tests functions:
 *   irc_protocol_cb_wallops
 */

TEST(IrcProtocolWithServer, wallops)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":alice!user@host WALLOPS");

    server_recv (":alice!user@host WALLOPS :message from admin");
}

/*
 * Tests functions:
 *   irc_protocol_cb_001 (connected to IRC server, empty)
 */

TEST(IrcProtocolWithServer, 001_empty)
{
    LONGS_EQUAL(0, ptr_server->is_connected);
    STRCMP_EQUAL("nick1", ptr_server->nick);

    server_recv (":server 001 alice");

    LONGS_EQUAL(1, ptr_server->is_connected);
    STRCMP_EQUAL("alice", ptr_server->nick);
}

/*
 * Tests functions:
 *   irc_protocol_cb_001 (connected to IRC server, welcome message)
 */

TEST(IrcProtocolWithServer, 001_welcome)
{
    run_cmd ("/set irc.server." IRC_FAKE_SERVER ".autojoin \"#autojoin1\"");
    run_cmd ("/set irc.server." IRC_FAKE_SERVER ".command "
             "\"/join #test1;/join #test2;/query remote_nick\"");
    LONGS_EQUAL(0, ptr_server->is_connected);
    STRCMP_EQUAL("nick1", ptr_server->nick);

    server_recv (":server 001 alice :Welcome on this server!");

    LONGS_EQUAL(1, ptr_server->is_connected);
    STRCMP_EQUAL("alice", ptr_server->nick);
    CHECK(ptr_server->channels);
    STRCMP_EQUAL("remote_nick", ptr_server->channels->name);
}

/*
 * Tests functions:
 *   irc_protocol_cb_005 (infos from server, empty)
 */

TEST(IrcProtocolWithServer, 005_empty)
{
    server_recv (":server 001 alice");

    POINTERS_EQUAL(NULL, ptr_server->prefix_modes);
    POINTERS_EQUAL(NULL, ptr_server->prefix_chars);

    server_recv (":server 005 alice TEST=A");

    POINTERS_EQUAL(NULL, ptr_server->prefix_modes);
    POINTERS_EQUAL(NULL, ptr_server->prefix_chars);
}

/*
 * Tests functions:
 *   irc_protocol_cb_005 (infos from server, full)
 */

TEST(IrcProtocolWithServer, 005_full)
{
    server_recv (":server 001 alice");

    POINTERS_EQUAL(NULL, ptr_server->prefix_modes);
    POINTERS_EQUAL(NULL, ptr_server->prefix_chars);
    LONGS_EQUAL(0, ptr_server->nick_max_length);
    LONGS_EQUAL(0, ptr_server->user_max_length);
    LONGS_EQUAL(0, ptr_server->host_max_length);
    LONGS_EQUAL(0, ptr_server->casemapping);
    POINTERS_EQUAL(NULL, ptr_server->chantypes);
    POINTERS_EQUAL(NULL, ptr_server->chanmodes);
    LONGS_EQUAL(0, ptr_server->monitor);
    POINTERS_EQUAL(NULL, ptr_server->isupport);

    server_recv (":server 005 alice " IRC_MSG_005 " :are supported");

    STRCMP_EQUAL("ohv", ptr_server->prefix_modes);
    STRCMP_EQUAL("@%+", ptr_server->prefix_chars);
    LONGS_EQUAL(30, ptr_server->nick_max_length);
    LONGS_EQUAL(16, ptr_server->user_max_length);
    LONGS_EQUAL(32, ptr_server->host_max_length);
    LONGS_EQUAL(1, ptr_server->casemapping);
    STRCMP_EQUAL("#", ptr_server->chantypes);
    STRCMP_EQUAL("eIbq,k,flj,CFLMPQScgimnprstuz", ptr_server->chanmodes);
    LONGS_EQUAL(100, ptr_server->monitor);
    CHECK(ptr_server->isupport[0] == ' ');
    STRCMP_EQUAL(IRC_MSG_005, ptr_server->isupport + 1);

    /* check that realloc of info is OK if we receive the message again */
    server_recv (":server 005 alice " IRC_MSG_005 " :are supported");
}

/*
 * Tests functions:
 *   irc_protocol_cb_005 (infos from server, multiple messages)
 */

TEST(IrcProtocolWithServer, 005_multiple_messages)
{
    server_recv (":server 001 alice");

    POINTERS_EQUAL(NULL, ptr_server->prefix_modes);
    POINTERS_EQUAL(NULL, ptr_server->prefix_chars);
    LONGS_EQUAL(0, ptr_server->host_max_length);
    POINTERS_EQUAL(NULL, ptr_server->isupport);

    server_recv (":server 005 alice PREFIX=(ohv)@%+ :are supported");
    server_recv (":server 005 alice HOSTLEN=24 :are supported");

    STRCMP_EQUAL("ohv", ptr_server->prefix_modes);
    STRCMP_EQUAL("@%+", ptr_server->prefix_chars);
    LONGS_EQUAL(24, ptr_server->host_max_length);
    STRCMP_EQUAL(" PREFIX=(ohv)@%+ HOSTLEN=24", ptr_server->isupport);
}

/*
 * Tests functions:
 *   irc_protocol_cb_008 (server notice mask)
 */

TEST(IrcProtocolWithServer, 008)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 008");
    server_recv (":server 008 alice");

    server_recv (":server 008 alice +Zbfkrsuy :Server notice mask");
}

/*
 * Tests functions:
 *   irc_protocol_cb_221 (user mode string)
 */

TEST(IrcProtocolWithServer, 221)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 221");
    server_recv (":server 221 alice");

    POINTERS_EQUAL(NULL, ptr_server->nick_modes);

    server_recv (":server 221 alice :+abc");
    STRCMP_EQUAL("abc", ptr_server->nick_modes);

    server_recv (":server 221 alice :-abc");
    POINTERS_EQUAL(NULL, ptr_server->nick_modes);
}

/*
 * Tests functions:
 *   irc_protocol_cb_whois_nick_msg
 *
 * Messages:
 *   223: whois (charset is)
 *   264: whois (is using encrypted connection)
 *   275: whois (secure connection)
 *   276: whois (has client certificate fingerprint)
 *   307: whois (registered nick)
 *   310: whois (help mode)
 *   313: whois (operator)
 *   318: whois (end)
 *   319: whois (channels)
 *   320: whois (identified user)
 *   326: whois (has oper privs)
 *   335: is a bot on
 *   378: whois (connecting from)
 *   379: whois (using modes)
 *   671: whois (secure connection)
 */

TEST(IrcProtocolWithServer, whois_nick_msg)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 223");
    server_recv (":server 223 alice");
    server_recv (":server 223 alice bob");

    server_recv (":server 223 alice bob UTF-8");
    server_recv (":server 223 alice bob :UTF-8");
    server_recv (":server 264 alice bob :is using encrypted connection");
    server_recv (":server 275 alice bob :is using secure connection");
    server_recv (":server 276 alice bob :has client certificate fingerprint");
    server_recv (":server 307 alice bob :registered nick");
    server_recv (":server 310 alice bob :help mode");
    server_recv (":server 313 alice bob :operator");
    server_recv (":server 318 alice bob :end");
    server_recv (":server 319 alice bob :channels");
    server_recv (":server 320 alice bob :identified user");
    server_recv (":server 326 alice bob :has oper privs");
    server_recv (":server 335 alice bob :is a bot");
    server_recv (":server 378 alice bob :connecting from");
    server_recv (":server 379 alice bob :using modes");
    server_recv (":server 671 alice bob :secure connection");
}

/*
 * Tests functions:
 *   irc_protocol_cb_whowas_nick_msg
 *
 * Messages:
 *   369: whowas (end)
 */

TEST(IrcProtocolWithServer, whowas_nick_msg)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 369");
    server_recv (":server 369 alice");
    server_recv (":server 369 alice bob");

    server_recv (":server 369 alice bob end");
    server_recv (":server 369 alice bob :end");
}

/*
 * Tests functions:
 *   irc_protocol_cb_301 (away message)
 */

TEST(IrcProtocolWithServer, 301)
{
    server_recv (":server 001 alice");

    server_recv (":bob!user@host PRIVMSG alice :hi Alice!");

    /* not enough arguments */
    server_recv (":server 301");

    POINTERS_EQUAL(NULL, ptr_server->channels->away_message);

    server_recv (":server 301 alice bob");
    POINTERS_EQUAL(NULL, ptr_server->channels->away_message);

    server_recv (":server 301 alice bob :I am away");
    STRCMP_EQUAL("I am away", ptr_server->channels->away_message);
}

/*
 * Tests functions:
 *   irc_protocol_cb_303 (ison)
 */

TEST(IrcProtocolWithServer, 303)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 303");
    server_recv (":server 303 alice");

    server_recv (":server 303 alice :nick1 nick2");
}

/*
 * Tests functions:
 *   irc_protocol_cb_305 (unaway)
 *   irc_protocol_cb_306 (away)
 */

TEST(IrcProtocolWithServer, 305_306)
{
    server_recv (":server 001 alice");

    server_recv (":bob!user@host PRIVMSG alice :hi Alice!");

    /* not enough arguments */
    server_recv (":server 305");
    server_recv (":server 306");

    POINTERS_EQUAL(NULL, ptr_server->channels->away_message);

    server_recv (":server 306 alice");  /* now away */
    LONGS_EQUAL(1, ptr_server->is_away);

    server_recv (":server 305 alice");
    LONGS_EQUAL(0, ptr_server->is_away);

    server_recv (":server 306 alice :We'll miss you");  /* now away */
    LONGS_EQUAL(1, ptr_server->is_away);

    server_recv (":server 305 alice :Does this mean you're really back?");
    LONGS_EQUAL(0, ptr_server->is_away);
}

/*
 * Tests functions:
 *   irc_protocol_cb_311 (whois, user)
 */

TEST(IrcProtocolWithServer, 311)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 311");
    server_recv (":server 311 alice");
    server_recv (":server 311 alice bob");
    server_recv (":server 311 alice bob user");
    server_recv (":server 311 alice bob user host");
    server_recv (":server 311 alice bob user host *");

    server_recv (":server 311 alice bob user host * :real name");
}

/*
 * Tests functions:
 *   irc_protocol_cb_312 (whois, server)
 */

TEST(IrcProtocolWithServer, 312)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 312");
    server_recv (":server 312 alice");
    server_recv (":server 312 alice bob");
    server_recv (":server 312 alice bob server");

    server_recv (":server 312 alice bob server :https://example.com/");
}

/*
 * Tests functions:
 *   irc_protocol_cb_314 (whowas)
 */

TEST(IrcProtocolWithServer, 314)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 314");
    server_recv (":server 314 alice");
    server_recv (":server 314 alice bob");
    server_recv (":server 314 alice bob user");
    server_recv (":server 314 alice bob user host");
    server_recv (":server 314 alice bob user host *");

    server_recv (":server 314 alice bob user host * :real name");
}

/*
 * Tests functions:
 *   irc_protocol_cb_315 (end of /who)
 */

TEST(IrcProtocolWithServer, 315)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 315");
    server_recv (":server 315 alice");
    server_recv (":server 315 alice #test");

    server_recv (":server 315 alice #test End of /WHO list.");
}

/*
 * Tests functions:
 *   irc_protocol_cb_317 (whois, idle)
 */

TEST(IrcProtocolWithServer, 317)
{
    time_t time;
    char message[1024];

    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 317");
    server_recv (":server 317 alice");
    server_recv (":server 317 alice bob");
    server_recv (":server 317 alice bob 122877");

    /* signon at 03/12/2008 @ 1:18pm (UTC) */
    server_recv (":server 317 alice bob 122877 1205327880");
    server_recv (":server 317 alice bob 122877 1205327880 "
                 ":seconds idle, signon time");

    /* signon 2 minutes ago */
    time = time_t (NULL);
    time -= 120;
    snprintf (message, sizeof (message),
              ":server 317 alice bob 30 %lld :seconds idle, signon time",
              (long long)time);
    server_recv (message);
}

/*
 * Tests functions:
 *   irc_protocol_cb_321 (/list start)
 */

TEST(IrcProtocolWithServer, 321)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 321");
    server_recv (":server 321 alice");

    server_recv (":server 321 alice #test");
    server_recv (":server 321 alice #test Users");
    server_recv (":server 321 alice #test :Users  Name");
}

/*
 * Tests functions:
 *   irc_protocol_cb_322 (channel for /list)
 */

TEST(IrcProtocolWithServer, 322)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 322");
    server_recv (":server 322 alice");
    server_recv (":server 322 alice #test");

    server_recv (":server 322 alice #test 3");
    server_recv (":server 322 alice #test 3 :topic of channel");

    run_cmd ("/list -server " IRC_FAKE_SERVER " -re #test.*");

    server_recv (":server 322 alice #test 3");
    server_recv (":server 322 alice #test 3 :topic of channel");

    server_recv (":server 322 alice #xyz 3");
    server_recv (":server 322 alice #xyz 3 :topic of channel");
}

/*
 * Tests functions:
 *   irc_protocol_cb_323 (end of /list)
 */

TEST(IrcProtocolWithServer, 323)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 323");

    server_recv (":server 323 alice");
    server_recv (":server 323 alice end");
    server_recv (":server 323 alice :End of /LIST");
}

/*
 * Tests functions:
 *   irc_protocol_cb_324 (channel mode)
 */

TEST(IrcProtocolWithServer, 324)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    POINTERS_EQUAL(NULL, ptr_server->channels->modes);

    /* not enough arguments */
    server_recv (":server 324");
    server_recv (":server 324 alice");

    server_recv (":server 324 alice #test +nt");
    STRCMP_EQUAL("+nt", ptr_server->channels->modes);

    server_recv (":server 324 alice #test");
    POINTERS_EQUAL(NULL, ptr_server->channels->modes);
}

/*
 * Tests functions:
 *   irc_protocol_cb_327 (whois, host)
 */

TEST(IrcProtocolWithServer, 327)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 327");
    server_recv (":server 327 alice");
    server_recv (":server 327 alice bob");
    server_recv (":server 327 alice bob host");

    server_recv (":server 327 alice bob host 1.2.3.4");
    server_recv (":server 327 alice bob host 1.2.3.4 :real name");
}

/*
 * Tests functions:
 *   irc_protocol_cb_328 (channel URL)
 */

TEST(IrcProtocolWithServer, 328)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 328");
    server_recv (":server 328 alice");
    server_recv (":server 328 alice #test");

    server_recv (":server 328 alice #test :https://example.com/");
}

/*
 * Tests functions:
 *   irc_protocol_cb_329 (channel creation date)
 */

TEST(IrcProtocolWithServer, 329)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 329");
    server_recv (":server 329 alice");
    server_recv (":server 329 alice #test");

    server_recv (":server 329 alice #test 1205327894");
    server_recv (":server 329 alice #test :1205327894");

    /* channel not found */
    server_recv (":server 329 alice #xyz 1205327894");
    server_recv (":server 329 alice #xyz :1205327894");
}

/*
 * Tests functions:
 *   irc_protocol_cb_330 (whois, is logged in as)
 *   irc_protocol_cb_343 (whois, is opered as)
 */

TEST(IrcProtocolWithServer, 330_343)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 330");
    server_recv (":server 330 alice");
    server_recv (":server 330 alice bob");

    /* not enough arguments */
    server_recv (":server 343");
    server_recv (":server 343 alice");
    server_recv (":server 343 alice bob");

    server_recv (":server 330 alice bob bob2");
    server_recv (":server 330 alice bob bob2 :is logged in as");

    server_recv (":server 343 alice bob bob2");
    server_recv (":server 343 alice bob bob2 :is opered in as");
}

/*
 * Tests functions:
 *   irc_protocol_cb_331 (no topic for channel)
 */

TEST(IrcProtocolWithServer, 331)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 331");
    server_recv (":server 331 alice");

    server_recv (":server 331 alice #test");

    /* channel not found */
    server_recv (":server 331 alice #xyz");
}

/*
 * Tests functions:
 *   irc_protocol_cb_332 (topic of channel)
 */

TEST(IrcProtocolWithServer, 332)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 332");
    server_recv (":server 332 alice");
    server_recv (":server 332 alice #test");

    POINTERS_EQUAL(NULL, ptr_server->channels->topic);

    server_recv (":server 332 alice #test :the new topic");
    STRCMP_EQUAL("the new topic", ptr_server->channels->topic);
}

/*
 * Tests functions:
 *   irc_protocol_cb_333 (infos about topic (nick / date))
 */

TEST(IrcProtocolWithServer, 333)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 333");
    server_recv (":server 333 alice");
    server_recv (":server 333 alice #test");

    server_recv (":server 333 alice #test nick!user@host");
    server_recv (":server 333 alice #test nick!user@host 1205428096");
    server_recv (":server 333 alice #test 1205428096");

    /* channel not found */
    server_recv (":server 333 alice #xyz nick!user@host");
    server_recv (":server 333 alice #xyz nick!user@host 1205428096");
    server_recv (":server 333 alice #xyz 1205428096");
}

/*
 * Tests functions:
 *   irc_protocol_cb_338 (whois, host)
 */

TEST(IrcProtocolWithServer, 338)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 338");
    server_recv (":server 338 alice");
    server_recv (":server 338 alice bob");
    server_recv (":server 338 alice bob host");

    server_recv (":server 338 alice bob host :actually using host");
}

/*
 * Tests functions:
 *   irc_protocol_cb_341 (inviting)
 */

TEST(IrcProtocolWithServer, 341)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 341");
    server_recv (":server 341 alice");
    server_recv (":server 341 alice bob");

    server_recv (":server 341 alice bob #test");
}

/*
 * Tests functions:
 *   irc_protocol_cb_344 (channel reop)
 */

TEST(IrcProtocolWithServer, 344)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 344");
    server_recv (":server 344 alice");
    server_recv (":server 344 alice #test");

    server_recv (":server 344 alice #test nick!user@host");

    /* channel not found */
    server_recv (":server 344 alice #xyz nick!user@host");
}

/*
 * Tests functions:
 *   irc_protocol_cb_345 (end of channel reop)
 */

TEST(IrcProtocolWithServer, 345)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 345");
    server_recv (":server 345 alice");
    server_recv (":server 345 alice #test");

    server_recv (":server 345 alice #test end");
    server_recv (":server 345 alice #test :End of Channel Reop List");

    /* channel not found */
    server_recv (":server 345 alice #xyz end");
    server_recv (":server 345 alice #xyz :End of Channel Reop List");
}

/*
 * Tests functions:
 *   irc_protocol_cb_346 (channel invite list)
 */

TEST(IrcProtocolWithServer, 346)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 346");
    server_recv (":server 346 alice");
    server_recv (":server 346 alice #test");

    server_recv (":server 346 alice #test invitemask");
    server_recv (":server 346 alice #test invitemask nick!user@host");
    server_recv (":server 346 alice #test invitemask nick!user@host 1205590879");

    /* channel not found */
    server_recv (":server 346 alice #xyz invitemask");
    server_recv (":server 346 alice #xyz invitemask nick!user@host");
    server_recv (":server 346 alice #xyz invitemask nick!user@host 1205590879");
}

/*
 * Tests functions:
 *   irc_protocol_cb_347 (end of channel invite list)
 */

TEST(IrcProtocolWithServer, 347)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 347");
    server_recv (":server 347 alice");

    server_recv (":server 347 alice #test");
    server_recv (":server 347 alice #test end");
    server_recv (":server 347 alice #test :End of Channel Invite List");

    /* channel not found */
    server_recv (":server 347 alice #xyz");
    server_recv (":server 347 alice #xyz end");
    server_recv (":server 347 alice #xyz :End of Channel Invite List");
}

/*
 * Tests functions:
 *   irc_protocol_cb_348 (channel exception list)
 */

TEST(IrcProtocolWithServer, 348)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 348");
    server_recv (":server 348 alice");
    server_recv (":server 348 alice #test");

    server_recv (":server 348 alice #test nick1!user1@host1");
    server_recv (":server 348 alice #test nick1!user1@host1 nick2!user2@host2");
    server_recv (":server 348 alice #test nick1!user1@host1 nick2!user2@host2 "
                 "1205585109");

    /* channel not found */
    server_recv (":server 348 alice #xyz nick1!user1@host1");
    server_recv (":server 348 alice #xyz nick1!user1@host1 nick2!user2@host2");
    server_recv (":server 348 alice #xyz nick1!user1@host1 nick2!user2@host2 "
                 "1205585109");
}

/*
 * Tests functions:
 *   irc_protocol_cb_349 (end of channel exception list)
 */

TEST(IrcProtocolWithServer, 349)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 349");
    server_recv (":server 349 alice");

    server_recv (":server 349 alice #test");
    server_recv (":server 349 alice #test end");
    server_recv (":server 349 alice #test :End of Channel Exception List");

    /* channel not found */
    server_recv (":server 349 alice #xyz");
    server_recv (":server 349 alice #xyz end");
    server_recv (":server 349 alice #xyz :End of Channel Exception List");
}

/*
 * Tests functions:
 *   irc_protocol_cb_351 (server version)
 */

TEST(IrcProtocolWithServer, 351)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 351");
    server_recv (":server 351 alice");
    server_recv (":server 351 alice dancer-ircd-1.0");

    server_recv (":server 351 alice dancer-ircd-1.0 server");
    server_recv (":server 351 alice dancer-ircd-1.0 server :iMZ dncrTS/v4");
}

/*
 * Tests functions:
 *   irc_protocol_cb_352 (who)
 */

TEST(IrcProtocolWithServer, 352)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 352");
    server_recv (":server 352 alice");
    server_recv (":server 352 alice #test");

    server_recv (":server 352 alice #test user");
    server_recv (":server 352 alice #test user host");
    server_recv (":server 352 alice #test user host server");
    server_recv (":server 352 alice #test user host server bob");
    server_recv (":server 352 alice #test user host server bob *");
    server_recv (":server 352 alice #test user host server bob * :0 nick");
    server_recv (":server 352 alice #test user host server bob H :0 nick");
    server_recv (":server 352 alice #test user host server bob G :0 nick");

    /* channel not found */
    server_recv (":server 352 alice #xyz user");
    server_recv (":server 352 alice #xyz user host");
    server_recv (":server 352 alice #xyz user host server");
    server_recv (":server 352 alice #xyz user host server bob");
    server_recv (":server 352 alice #xyz user host server bob *");
    server_recv (":server 352 alice #xyz user host server bob * :0 nick");
    server_recv (":server 352 alice #xyz user host server bob H :0 nick");
    server_recv (":server 352 alice #xyz user host server bob G :0 nick");
}

/*
 * Tests functions:
 *   irc_protocol_cb_353 (list of users on a channel)
 */

TEST(IrcProtocolWithServer, 353)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 353");
    server_recv (":server 353 alice");
    server_recv (":server 353 alice #test");
    server_recv (":server 353 alice =");

    server_recv (":server 353 alice #test");
    server_recv (":server 353 alice #test :alice");
    server_recv (":server 353 alice #test :alice bob @carol +dan!user@host");

    server_recv (":server 353 alice = #test");
    server_recv (":server 353 alice = #test :alice");
    server_recv (":server 353 alice = #test :alice bob @carol +dan!user@host");

    /* with option irc.look.color_nicks_in_names enabled */
    config_file_option_set (irc_config_look_color_nicks_in_names, "on", 1);
    server_recv (":server 353 alice = #test :alice bob @carol +dan!user@host");
    config_file_option_unset (irc_config_look_color_nicks_in_names);

    /* channel not found */
    server_recv (":server 353 alice #xyz");
    server_recv (":server 353 alice #xyz :alice");
    server_recv (":server 353 alice #xyz :alice bob @carol +dan!user@host");

    /* channel not found */
    server_recv (":server 353 alice = #xyz");
    server_recv (":server 353 alice = #xyz :alice");
    server_recv (":server 353 alice = #xyz :alice bob @carol +dan!user@host");
}

/*
 * Tests functions:
 *   irc_protocol_cb_354 (WHOX output)
 */

TEST(IrcProtocolWithServer, 354)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 354");
    server_recv (":server 354 alice");

    server_recv (":server 354 alice #test");
    server_recv (":server 354 alice #test user2");
    server_recv (":server 354 alice #test user2 host2");
    server_recv (":server 354 alice #test user2 host2 server");
    server_recv (":server 354 alice #test user2 host2 server bob");
    server_recv (":server 354 alice #test user2 host2 server bob status");
    server_recv (":server 354 alice #test user2 host2 server bob status "
                 "hopcount");
    server_recv (":server 354 alice #test user2 host2 server bob status "
                 "hopcount account");
    server_recv (":server 354 alice #test user2 host2 server bob status "
                 "hopcount account :real name");

    /* channel not found */
    server_recv (":server 354 alice #xyz");
    server_recv (":server 354 alice #xyz user2");
    server_recv (":server 354 alice #xyz user2 host2");
    server_recv (":server 354 alice #xyz user2 host2 server");
    server_recv (":server 354 alice #xyz user2 host2 server bob");
    server_recv (":server 354 alice #xyz user2 host2 server bob status");
    server_recv (":server 354 alice #xyz user2 host2 server bob status "
                 "hopcount");
    server_recv (":server 354 alice #xyz user2 host2 server bob status "
                 "hopcount account");
    server_recv (":server 354 alice #xyz user2 host2 server bob status "
                 "hopcount account :real name");
}

/*
 * Tests functions:
 *   irc_protocol_cb_366 (end of /names list)
 */

TEST(IrcProtocolWithServer, 366)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");
    server_recv (":bob!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 366");
    server_recv (":server 366 alice");
    server_recv (":server 366 alice #test");

    server_recv (":server 366 alice #test end");
    server_recv (":server 366 alice #test :End of /NAMES list");

    /* channel not found */
    server_recv (":server 366 alice #xyz end");
    server_recv (":server 366 alice #xyz :End of /NAMES list");
}

/*
 * Tests functions:
 *   irc_protocol_cb_367 (banlist)
 */

TEST(IrcProtocolWithServer, 367)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 367");
    server_recv (":server 367 alice");
    server_recv (":server 367 alice #test");

    server_recv (":server 367 alice #test nick1!user1@host1");
    server_recv (":server 367 alice #test nick1!user1@host1 nick2!user2@host2");
    server_recv (":server 367 alice #test nick1!user1@host1 nick2!user2@host2 "
                 "1205585109");

    /* channel not found */
    server_recv (":server 367 alice #xyz nick1!user1@host1");
    server_recv (":server 367 alice #xyz nick1!user1@host1 nick2!user2@host2");
    server_recv (":server 367 alice #xyz nick1!user1@host1 nick2!user2@host2 "
                 "1205585109");
}

/*
 * Tests functions:
 *   irc_protocol_cb_368 (end of banlist)
 */

TEST(IrcProtocolWithServer, 368)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 368");
    server_recv (":server 368 alice");

    server_recv (":server 368 alice #test");
    server_recv (":server 368 alice #test end");
    server_recv (":server 368 alice #test :End of Channel Ban List");

    /* channel not found */
    server_recv (":server 368 alice #xyz");
    server_recv (":server 368 alice #xyz end");
    server_recv (":server 368 alice #xyz :End of Channel Ban List");
}

/*
 * Tests functions:
 *   irc_protocol_cb_432 (erroneous nickname, not connected)
 */

TEST(IrcProtocolWithServer, 432_not_connected)
{
    server_recv (":server 432 * alice error");
    server_recv (":server 432 * :alice error");
    server_recv (":server 432 * alice :Erroneous Nickname");
    server_recv (":server 432 * alice1 :Erroneous Nickname");
}

/*
 * Tests functions:
 *   irc_protocol_cb_432 (erroneous nickname, connected)
 */

TEST(IrcProtocolWithServer, 432_connected)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 432");
    server_recv (":server 432 alice");

    server_recv (":server 432 * alice");
    server_recv (":server 432 * alice error");
    server_recv (":server 432 * alice :Erroneous Nickname");
}

/*
 * Tests functions:
 *   irc_protocol_cb_433 (nickname already in use, not connected)
 */

TEST(IrcProtocolWithServer, 433_not_connected)
{
    server_recv (":server 433 * alice error");
    server_recv (":server 433 * alice :Nickname is already in use.");
    server_recv (":server 433 * alice1 :Nickname is already in use.");
}

/*
 * Tests functions:
 *   irc_protocol_cb_433 (nickname already in use, connected)
 */

TEST(IrcProtocolWithServer, 433_connected)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 433");
    server_recv (":server 433 alice");

    server_recv (":server 433 * alice");
    server_recv (":server 433 * alice error");
    server_recv (":server 433 * alice :Nickname is already in use.");
}

/*
 * Tests functions:
 *   irc_protocol_cb_437 (nick/channel temporarily unavailable, not connected)
 */

TEST(IrcProtocolWithServer, 437_not_connected)
{
    server_recv (":server 437 * alice error");
    server_recv (":server 437 * alice :Nick/channel is temporarily unavailable");
    server_recv (":server 437 * alice1 :Nick/channel is temporarily unavailable");
}

/*
 * Tests functions:
 *   irc_protocol_cb_437 (nick/channel temporarily unavailable, connected)
 */

TEST(IrcProtocolWithServer, 437_connected)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 437");
    server_recv (":server 437 alice");

    server_recv (":server 437 * alice");
    server_recv (":server 437 * alice error");
    server_recv (":server 437 * alice :Nick/channel is temporarily unavailable");
}

/*
 * Tests functions:
 *   irc_protocol_cb_438 (not authorized to change nickname)
 */

TEST(IrcProtocolWithServer, 438)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 438");
    server_recv (":server 438 alice");

    server_recv (":server 438 alice alice2");
    server_recv (":server 438 alice alice2 error");
    server_recv (":server 438 alice alice2 :Nick change too fast. Please wait 30 seconds.");
}

/*
 * Tests functions:
 *   irc_protocol_cb_470 (forwarding to another channel)
 */

TEST(IrcProtocolWithServer, 470)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 470");
    server_recv (":server 470 alice");
    server_recv (":server 470 alice #test");

    server_recv (":server 470 alice #test #test2");
    server_recv (":server 470 alice #test #test2 forwarding");
    server_recv (":server 470 alice #test #test2 :Forwarding to another channel");

    server_recv (":server 438 alice alice2");
    server_recv (":server 438 alice alice2 error");
    server_recv (":server 438 alice alice2 :Nick change too fast. Please wait 30 seconds.");
}

/*
 * Tests functions:
 *   irc_protocol_cb_728 (quietlist)
 */

TEST(IrcProtocolWithServer, 728)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 728");
    server_recv (":server 728 alice");
    server_recv (":server 728 alice #test q");

    server_recv (":server 728 alice #test q nick1!user1@host1");
    server_recv (":server 728 alice #test q nick1!user1@host1 alice!user@host");
    server_recv (":server 728 alice #test q nick1!user1@host1 alice!user@host "
                 "1351350090");

    /* channel not found */
    server_recv (":server 728 alice #xyz q nick1!user1@host1");
    server_recv (":server 728 alice #xyz q nick1!user1@host1 alice!user@host");
    server_recv (":server 728 alice #xyz q nick1!user1@host1 alice!user@host "
                 "1351350090");
}

/*
 * Tests functions:
 *   irc_protocol_cb_729 (end of quietlist)
 */

TEST(IrcProtocolWithServer, 729)
{
    server_recv (":server 001 alice");

    server_recv (":alice!user@host JOIN #test");

    /* not enough arguments */
    server_recv (":server 729");
    server_recv (":server 729 alice");
    server_recv (":server 729 alice #test");

    server_recv (":server 729 alice #test q");
    server_recv (":server 729 alice #test q end");
    server_recv (":server 729 alice #test q :End of Channel Quiet List");

    /* channel not found */
    server_recv (":server 729 alice #xyz q");
    server_recv (":server 729 alice #xyz q end");
    server_recv (":server 729 alice #xyz q :End of Channel Quiet List");
}

/*
 * Tests functions:
 *   irc_protocol_cb_730 (monitored nicks are online (RPL_MONONLINE))
 */

TEST(IrcProtocolWithServer, 730)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 730");
    server_recv (":server 730 alice");

    server_recv (":server 730 alice :nick1!user1@host1,nick2!user2@host2");
}

/*
 * Tests functions:
 *   irc_protocol_cb_731 (monitored nicks are offline (RPL_MONOFFLINE))
 */

TEST(IrcProtocolWithServer, 731)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 731");
    server_recv (":server 731 alice");

    server_recv (":server 731 alice :nick1!user1@host1,nick2!user2@host2");
}

/*
 * Tests functions:
 *   irc_protocol_cb_732 (list of monitored nicks (RPL_MONLIST))
 */

TEST(IrcProtocolWithServer, 732)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 732");

    server_recv (":server 732 alice");
    server_recv (":server 732 alice :nick1!user1@host1,nick2!user2@host2");
}

/*
 * Tests functions:
 *   irc_protocol_cb_733 (end of a monitor list (RPL_ENDOFMONLIST))
 */

TEST(IrcProtocolWithServer, 733)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 733");

    server_recv (":server 733 alice");
    server_recv (":server 733 alice end");
    server_recv (":server 733 alice :End of MONITOR list");
}

/*
 * Tests functions:
 *   irc_protocol_cb_734 (monitor list is full (ERR_MONLISTFULL))
 */

TEST(IrcProtocolWithServer, 734)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 734");
    server_recv (":server 734 alice");
    server_recv (":server 734 alice 10");

    server_recv (":server 734 alice 10 nick1,nick2");
    server_recv (":server 734 alice 10 nick1,nick2 full");
    server_recv (":server 734 alice 10 nick1,nick2 :Monitor list is full");
}

/*
 * Tests functions:
 *   irc_protocol_cb_900 (logged in as (SASL))
 */

TEST(IrcProtocolWithServer, 900)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 900");
    server_recv (":server 900 alice");
    server_recv (":server 900 alice alice!user@host");
    server_recv (":server 900 alice alice!user@host alice");

    server_recv (":server 900 alice alice!user@host alice logged");
    server_recv (":server 900 alice alice!user@host alice "
                 ":You are now logged in as mynick");
}

/*
 * Tests functions:
 *   irc_protocol_cb_901 (you are now logged in)
 */

TEST(IrcProtocolWithServer, 901)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 901");
    server_recv (":server 901 alice");
    server_recv (":server 901 alice user");
    server_recv (":server 901 alice user host");

    server_recv (":server 901 alice user host logged");
    server_recv (":server 901 alice user host "
                 ":You are now logged in as mynick");
}

/*
 * Tests functions:
 *   irc_protocol_cb_903 (SASL OK)
 *   irc_protocol_cb_907 (SASL OK)
 */

TEST(IrcProtocolWithServer, 903_907)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 903");

    /* not enough arguments */
    server_recv (":server 907");

    server_recv (":server 903 alice ok");
    server_recv (":server 903 alice :SASL authentication successful");

    server_recv (":server 907 alice ok");
    server_recv (":server 907 alice :SASL authentication successful");
}

/*
 * Tests functions:
 *   irc_protocol_cb_902 (SASL failed)
 *   irc_protocol_cb_904 (SASL failed)
 *   irc_protocol_cb_905 (SASL failed)
 *   irc_protocol_cb_906 (SASL failed)
 */

TEST(IrcProtocolWithServer, 902_904_905_906)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 902");

    /* not enough arguments */
    server_recv (":server 904");

    /* not enough arguments */
    server_recv (":server 905");

    /* not enough arguments */
    server_recv (":server 906");

    server_recv (":server 902 alice error");
    server_recv (":server 902 alice :SASL authentication failed");

    server_recv (":server 904 alice error");
    server_recv (":server 904 alice :SASL authentication failed");

    server_recv (":server 905 alice error");
    server_recv (":server 905 alice :SASL authentication failed");

    server_recv (":server 906 alice error");
    server_recv (":server 906 alice :SASL authentication failed");
}

/*
 * Tests functions:
 *   irc_protocol_cb_server_mode_reason
 *
 * Messages:
 *   973: whois (secure connection)
 *   974: whois (secure connection)
 *   975: whois (secure connection)
 */

TEST(IrcProtocolWithServer, server_mode_reason)
{
    server_recv (":server 001 alice");

    /* not enough arguments */
    server_recv (":server 973");

    server_recv (":server 973 alice");
    server_recv (":server 973 alice mode");
    server_recv (":server 973 alice mode test");
    server_recv (":server 973 alice mode :test");

    server_recv (":server 974 alice");
    server_recv (":server 974 alice mode");
    server_recv (":server 974 alice mode test");
    server_recv (":server 974 alice mode :test");

    server_recv (":server 975 alice");
    server_recv (":server 975 alice mode");
    server_recv (":server 975 alice mode test");
    server_recv (":server 975 alice mode :test");

    server_recv (":server 973 bob");
    server_recv (":server 973 bob mode");
    server_recv (":server 973 bob mode test");
    server_recv (":server 973 bob mode :test");

    server_recv (":server 974 bob");
    server_recv (":server 974 bob mode");
    server_recv (":server 974 bob mode test");
    server_recv (":server 974 bob mode :test");

    server_recv (":server 975 bob");
    server_recv (":server 975 bob mode");
    server_recv (":server 975 bob mode test");
    server_recv (":server 975 bob mode :test");
}
