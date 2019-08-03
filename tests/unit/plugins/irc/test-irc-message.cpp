/*
 * test-irc-message.cpp - test IRC message functions
 *
 * Copyright (C) 2019 Sébastien Helleu <flashcode@flashtux.org>
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
#include "tests/tests.h"
#include "src/plugins/irc/irc-message.h"
#include "src/plugins/irc/irc-server.h"
}

#define NICK_256_WITH_SPACE "nick_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxx_64_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxx_128_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx_256 test"

TEST_GROUP(IrcMessage)
{
};

/*
 * Tests functions:
 *   irc_message_parse
 *   irc_message_parse_to_hashtable
 */

TEST(IrcMessage, Parse)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_message_convert_charset
 */

TEST(IrcMessage, ConvertCharset)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_message_get_nick_from_host
 */

TEST(IrcMessage, GetNickFromHost)
{
    POINTERS_EQUAL(NULL, irc_message_get_nick_from_host (NULL));
    STRCMP_EQUAL("", irc_message_get_nick_from_host (""));
    STRCMP_EQUAL("nick", irc_message_get_nick_from_host ("nick"));
    STRCMP_EQUAL("nick", irc_message_get_nick_from_host ("nick "));
    STRCMP_EQUAL("nick", irc_message_get_nick_from_host ("nick test"));
    STRCMP_EQUAL("nick", irc_message_get_nick_from_host (":nick "));
    STRCMP_EQUAL("nick", irc_message_get_nick_from_host (":nick!host"));
    STRCMP_EQUAL("nick", irc_message_get_nick_from_host (":nick!user@host"));
    STRCMP_EQUAL("nick_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                 "x_64_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                 "xxxx_12",
                 irc_message_get_nick_from_host (NICK_256_WITH_SPACE));
}

/*
 * Tests functions:
 *   irc_message_get_address_from_host
 */

TEST(IrcMessage, GetAddressFromHost)
{
    POINTERS_EQUAL(NULL, irc_message_get_address_from_host (NULL));
    STRCMP_EQUAL("", irc_message_get_address_from_host (""));
    STRCMP_EQUAL("host", irc_message_get_address_from_host ("host"));
    STRCMP_EQUAL("host", irc_message_get_address_from_host ("host "));
    STRCMP_EQUAL("host", irc_message_get_address_from_host ("host test"));
    STRCMP_EQUAL("host", irc_message_get_address_from_host (":host "));
    STRCMP_EQUAL("host", irc_message_get_address_from_host (":nick!host"));
    STRCMP_EQUAL("user@host",
                 irc_message_get_address_from_host (":nick!user@host"));
    STRCMP_EQUAL("nick_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                 "x_64_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                 "xxxx_128_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                 "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                 "xxxxxxxxxxxx_25",
                 irc_message_get_address_from_host (NICK_256_WITH_SPACE));
}

/*
 * Tests functions:
 *   irc_message_replace_vars
 */

TEST(IrcMessage, ReplaceVars)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("my_ircd");
    CHECK(server);

    WEE_TEST_STR(NULL, irc_message_replace_vars (NULL, NULL, NULL));
    WEE_TEST_STR(NULL, irc_message_replace_vars (server, NULL, NULL));
    WEE_TEST_STR(NULL, irc_message_replace_vars (NULL, "#test", NULL));
    WEE_TEST_STR(NULL, irc_message_replace_vars (server, "#test", NULL));
    WEE_TEST_STR("", irc_message_replace_vars (NULL, NULL, ""));
    WEE_TEST_STR("", irc_message_replace_vars (server, NULL, ""));
    WEE_TEST_STR("", irc_message_replace_vars (NULL, "#test", ""));
    WEE_TEST_STR("", irc_message_replace_vars (server, "#test", ""));

    /* empty nick, empty channel, empty server */
    WEE_TEST_STR("nick '', channel '', server ''",
                 irc_message_replace_vars (
                     NULL, NULL,
                     "nick '$nick', channel '$channel', server '$server'"));

    irc_server_set_nick (server, "my_nick");

    /* nick, empty channel, server */
    WEE_TEST_STR("nick 'my_nick', channel '', server 'my_ircd'",
                 irc_message_replace_vars (
                     server, NULL,
                     "nick '$nick', channel '$channel', server '$server'"));

    /* nick, channel, server */
    WEE_TEST_STR("nick 'my_nick', channel '#test', server 'my_ircd'",
                 irc_message_replace_vars (
                     server, "#test",
                     "nick '$nick', channel '$channel', server '$server'"));

    /* nick, channel, server (2 vars for each) */
    WEE_TEST_STR("nick 'my_nick', channel '#test', server 'my_ircd', "
                 "nick 'my_nick', channel '#test', server 'my_ircd'",
                 irc_message_replace_vars (
                     server, "#test",
                     "nick '$nick', channel '$channel', server '$server', "
                     "nick '$nick', channel '$channel', server '$server'"));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_message_split_add
 *   irc_message_split_string
 *   irc_message_split_join
 *   irc_message_split_privmsg_notice
 *   irc_message_split_005
 *   irc_message_split
 */

TEST(IrcMessage, Split)
{
    /* TODO: write tests */
}
