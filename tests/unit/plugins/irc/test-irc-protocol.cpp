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
#include "src/core/wee-config-file.h"
#include "src/core/wee-hashtable.h"
#include "src/gui/gui-color.h"
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

    server_recv (":alice!user@host ACCOUNT new_account");
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

    server_recv (":alice!user@host ACCOUNT new_account2");
    STRCMP_EQUAL("new_account2", ptr_nick->account);

    server_recv (":alice!user@host ACCOUNT *");
    POINTERS_EQUAL(NULL, ptr_nick->account);
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

    server_recv (":alice!user@host AWAY :Holidays!");

    LONGS_EQUAL(1, ptr_nick->away);
}

/*
 * Tests functions:
 *   irc_protocol_cb_chghost
 */

TEST(IrcProtocolWithServer, chghost)
{
    struct t_irc_nick *ptr_nick;

    server_recv (":server 001 alice");
    server_recv (":alice!user@host JOIN #test");

    ptr_nick = ptr_server->channels->nicks;

    STRCMP_EQUAL("user@host", ptr_nick->host);

    /* not enough arguments */
    server_recv (":alice!user@host CHGHOST");
    server_recv (":alice!user@host CHGHOST user2");
    STRCMP_EQUAL("user@host", ptr_nick->host);

    server_recv (":alice!user@host CHGHOST user2 host2");
    STRCMP_EQUAL("user2@host2", ptr_nick->host);

    server_recv (":alice!user2@host2 CHGHOST user3 :host3");
    STRCMP_EQUAL("user3@host3", ptr_nick->host);
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

    server_recv (":alice!user@host KICK #test bob :no spam here!");

    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);
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

    server_recv (":alice!user@host KILL bob :killed by admin");

    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);
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

    /* nick mode '@' */
    server_recv (":admin MODE #test +o alice");
    STRCMP_EQUAL("@ ", ptr_nick->prefixes);
    STRCMP_EQUAL("@", ptr_nick->prefix);

    /* another nick mode '+' */
    server_recv (":admin MODE #test +v alice");
    STRCMP_EQUAL("@+", ptr_nick->prefixes);
    STRCMP_EQUAL("@", ptr_nick->prefix);

    /* nick mode '@' removed */
    server_recv (":admin MODE #test -o alice");
    STRCMP_EQUAL(" +", ptr_nick->prefixes);
    STRCMP_EQUAL("+", ptr_nick->prefix);

    /* nick mode '+' removed */
    server_recv (":admin MODE #test -v alice");
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);
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
}

/*
 * Tests functions:
 *   irc_protocol_cb_001 (empty)
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
 *   irc_protocol_cb_001 (welcome)
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
 *   irc_protocol_cb_005 (empty)
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
 *   irc_protocol_cb_005 (full)
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
}

/*
 * Tests functions:
 *   irc_protocol_cb_005 (multiple messages)
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
