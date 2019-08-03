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
#include "src/core/wee-hashtable.h"
#include "src/plugins/irc/irc-message.h"
#include "src/plugins/irc/irc-server.h"
}

#define NICK_256_WITH_SPACE "nick_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxx_64_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxx_128_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx_256 test"

#define WEE_CHECK_PARSE(__tags, __message_without_tags, __nick, __host, \
                        __command, __channel, __arguments, __text,      \
                        __pos_command, __pos_arguments, __pos_channel,  \
                        __pos_text, __server, __message)                \
    tags = NULL;                                                        \
    message_without_tags = NULL;                                        \
    nick = NULL;                                                        \
    host = NULL;                                                        \
    command = NULL;                                                     \
    channel = NULL;                                                     \
    arguments = NULL;                                                   \
    text = NULL;                                                        \
    pos_command = -1;                                                   \
    pos_channel = -1;                                                   \
    pos_text = -1;                                                      \
                                                                        \
    irc_message_parse (__server, __message, &tags,                      \
                       &message_without_tags,                           \
                       &nick, &host, &command, &channel, &arguments,    \
                       &text, &pos_command, &pos_arguments,             \
                       &pos_channel, &pos_text);                        \
                                                                        \
    if (__tags == NULL)                                                 \
    {                                                                   \
        POINTERS_EQUAL(NULL, tags);                                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__tags, tags);                                     \
    }                                                                   \
    if (__message_without_tags == NULL)                                 \
    {                                                                   \
        POINTERS_EQUAL(NULL, message_without_tags);                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__message_without_tags, message_without_tags);     \
    }                                                                   \
    if (__nick == NULL)                                                 \
    {                                                                   \
        POINTERS_EQUAL(NULL, nick);                                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__nick, nick);                                     \
    }                                                                   \
    if (__host == NULL)                                                 \
    {                                                                   \
        POINTERS_EQUAL(NULL, host);                                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__host, host);                                     \
    }                                                                   \
    if (__command == NULL)                                              \
    {                                                                   \
        POINTERS_EQUAL(NULL, command);                                  \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__command, command);                               \
    }                                                                   \
    if (__channel == NULL)                                              \
    {                                                                   \
        POINTERS_EQUAL(NULL, channel);                                  \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__channel, channel);                               \
    }                                                                   \
    if (__arguments == NULL)                                            \
    {                                                                   \
        POINTERS_EQUAL(NULL, arguments);                                \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__arguments, arguments);                           \
    }                                                                   \
    if (__text == NULL)                                                 \
    {                                                                   \
        POINTERS_EQUAL(NULL, text);                                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__text, text);                                     \
    }                                                                   \
    LONGS_EQUAL(__pos_command, pos_command);                            \
    LONGS_EQUAL(__pos_arguments, pos_arguments);                        \
    LONGS_EQUAL(__pos_channel, pos_channel);                            \
    LONGS_EQUAL(__pos_text, pos_text);                                  \
                                                                        \
    if (tags)                                                           \
        free (tags);                                                    \
    if (message_without_tags)                                           \
        free (message_without_tags);                                    \
    if (nick)                                                           \
        free (nick);                                                    \
    if (host)                                                           \
        free (host);                                                    \
    if (command)                                                        \
        free (command);                                                 \
    if (channel)                                                        \
        free (channel);                                                 \
    if (arguments)                                                      \
        free (arguments);                                               \
    if (text)                                                           \
        free (text);

TEST_GROUP(IrcMessage)
{
};

/*
 * Tests functions:
 *   irc_message_parse
 */

TEST(IrcMessage, Parse)
{
    char *tags, *message_without_tags, *nick, *host, *command, *channel;
    char *arguments, *text;
    int pos_command, pos_arguments, pos_channel, pos_text;

    WEE_CHECK_PARSE(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                    -1, -1, -1, -1,
                    NULL, NULL);

    WEE_CHECK_PARSE(NULL, "", NULL, NULL, NULL, NULL, NULL, NULL,
                    -1, -1, -1, -1,
                    NULL, "");

    WEE_CHECK_PARSE(NULL, ":nick!user@host", "nick", "nick!user@host", NULL,
                    NULL, NULL, NULL,
                    -1, -1, -1, -1,
                    NULL, ":nick!user@host");

    /* AWAY */
    WEE_CHECK_PARSE(NULL, ":nick!user@host AWAY", "nick", "nick!user@host",
                    "AWAY", NULL, NULL, NULL,
                    16, -1, -1, -1,
                    NULL, ":nick!user@host AWAY");
    WEE_CHECK_PARSE(NULL, ":nick!user@host AWAY :I am away", "nick",
                    "nick!user@host", "AWAY", NULL, ":I am away", "I am away",
                    16, 21, -1, 22,
                    NULL, ":nick!user@host AWAY :I am away");

    /* CAP */
    WEE_CHECK_PARSE(NULL,
                    ":irc.example.com CAP * LS :identify-msg multi-prefix sasl",
                    "irc.example.com", "irc.example.com", "CAP", "*",
                    "* LS :identify-msg multi-prefix sasl",
                    "LS :identify-msg multi-prefix sasl",
                    17, 21, 21, 23,
                    NULL,
                    ":irc.example.com CAP * LS :identify-msg multi-prefix sasl");

    /* JOIN */
    WEE_CHECK_PARSE(NULL, ":nick!user@host JOIN #channel", "nick",
                    "nick!user@host", "JOIN", "#channel", "#channel", NULL,
                    16, 21, 21, -1,
                    NULL, ":nick!user@host JOIN #channel");

    /* JOIN with colon */
    WEE_CHECK_PARSE(NULL, ":nick!user@host JOIN :#channel", "nick",
                    "nick!user@host", "JOIN", "#channel", ":#channel", NULL,
                    16, 21, 22, -1,
                    NULL, ":nick!user@host JOIN :#channel");

    /* JOIN with extended join capability */
    WEE_CHECK_PARSE(NULL, ":nick!user@host JOIN #channel account :real name",
                    "nick", "nick!user@host", "JOIN", "#channel",
                    "#channel account :real name", "account :real name",
                    16, 21, 21, 30,
                    NULL, ":nick!user@host JOIN #channel account :real name");

    /* KICK */
    WEE_CHECK_PARSE(NULL, ":nick1!user@host KICK #channel nick2 :kick reason",
                    "nick1", "nick1!user@host", "KICK", "#channel",
                    "#channel nick2 :kick reason", "nick2 :kick reason",
                    17, 22, 22, 31,
                    NULL, ":nick1!user@host KICK #channel nick2 :kick reason");

    /* MODE */
    WEE_CHECK_PARSE(NULL, ":nick!user@host MODE #channel +o nick",
                    "nick", "nick!user@host", "MODE", "#channel",
                    "#channel +o nick", "+o nick",
                    16, 21, 21, 30,
                    NULL, ":nick!user@host MODE #channel +o nick");

    /* MODE with colon */
    WEE_CHECK_PARSE(NULL, ":nick!user@host MODE #channel :+o nick",
                    "nick", "nick!user@host", "MODE", "#channel",
                    "#channel :+o nick", "+o nick",
                    16, 21, 21, 31,
                    NULL, ":nick!user@host MODE #channel :+o nick");

    /* NICK */
    WEE_CHECK_PARSE(NULL, ":oldnick!user@host NICK :newnick",
                    "oldnick", "oldnick!user@host", "NICK", NULL,
                    ":newnick", "newnick",
                    19, 24, -1, 25,
                    NULL, ":oldnick!user@host NICK :newnick");

    /* NOTICE */
    WEE_CHECK_PARSE(NULL, "NOTICE AUTH :*** Looking up your hostname...",
                    "AUTH", NULL, "NOTICE", "AUTH",
                    "AUTH :*** Looking up your hostname...",
                    "*** Looking up your hostname...",
                    0, 7, 7, 13,
                    NULL, "NOTICE AUTH :*** Looking up your hostname...");

    /* PING */
    WEE_CHECK_PARSE(NULL, "PING :arguments", NULL, NULL, "PING", NULL,
                    ":arguments", "arguments",
                    0, 5, -1, 6,
                    NULL, "PING :arguments");


    /* PART */
    WEE_CHECK_PARSE(NULL, ":nick!user@host PART #channel", "nick",
                    "nick!user@host", "PART", "#channel", "#channel", NULL,
                    16, 21, 21, -1,
                    NULL, ":nick!user@host PART #channel");

    /* PART with colon */
    WEE_CHECK_PARSE(NULL, ":nick!user@host PART :#channel", "nick",
                    "nick!user@host", "PART", "#channel", ":#channel", NULL,
                    16, 21, 22, -1,
                    NULL, ":nick!user@host PART :#channel");

    /* INVITE */
    WEE_CHECK_PARSE(NULL, ":nick!user@host INVITE nick2 #channel", "nick",
                    "nick!user@host", "INVITE", "#channel", "nick2 #channel",
                    NULL,
                    16, 23, 29, -1,
                    NULL, ":nick!user@host INVITE nick2 #channel");

    /* PRIVMSG */
    WEE_CHECK_PARSE(NULL, ":nick PRIVMSG", "nick", "nick",
                    "PRIVMSG", NULL, NULL, NULL,
                    6, -1, -1, -1,
                    NULL, ":nick PRIVMSG");
    WEE_CHECK_PARSE(NULL, ":nick@host PRIVMSG", "nick", "nick@host",
                    "PRIVMSG", NULL, NULL, NULL,
                    11, -1, -1, -1,
                    NULL, ":nick@host PRIVMSG");
    WEE_CHECK_PARSE(NULL, ":nick!user@host PRIVMSG", "nick", "nick!user@host",
                    "PRIVMSG", NULL, NULL, NULL,
                    16, -1, -1, -1,
                    NULL, ":nick!user@host PRIVMSG");
    WEE_CHECK_PARSE(NULL, ":nick!user@host PRIVMSG #channel", "nick",
                    "nick!user@host", "PRIVMSG", "#channel", "#channel", NULL,
                    16, 24, 24, -1,
                    NULL, ":nick!user@host PRIVMSG #channel");
    WEE_CHECK_PARSE(NULL, ":nick!user@host PRIVMSG #channel :the message",
                    "nick", "nick!user@host", "PRIVMSG", "#channel",
                    "#channel :the message", "the message",
                    16, 24, 24, 34,
                    NULL, ":nick!user@host PRIVMSG #channel :the message");

    /* PRIVMSG with tags */
    WEE_CHECK_PARSE("time=2019-08-03T12:13:00.000Z",
                    ":nick!user@host PRIVMSG #channel :the message",
                    "nick", "nick!user@host", "PRIVMSG", "#channel",
                    "#channel :the message", "the message",
                    47, 55, 55, 65,
                    NULL,
                    "@time=2019-08-03T12:13:00.000Z :nick!user@host PRIVMSG "
                    "#channel :the message");

    /* PRIVMSG with tags and extra spaces*/
    WEE_CHECK_PARSE("time=2019-08-03T12:13:00.000Z",
                    ":nick!user@host  PRIVMSG  #channel  :the message",
                    "nick", "nick!user@host", "PRIVMSG", "#channel",
                    "#channel  :the message", "the message",
                    49, 58, 58, 69,
                    NULL,
                    "@time=2019-08-03T12:13:00.000Z  :nick!user@host  "
                    "PRIVMSG  #channel  :the message");

    /* PRIVMSG to a nick */
    WEE_CHECK_PARSE(NULL, ":nick!user@host PRIVMSG nick2 :the message",
                    "nick", "nick!user@host", "PRIVMSG", "nick2",
                    "nick2 :the message", "the message",
                    16, 24, 24, 31,
                    NULL, ":nick!user@host PRIVMSG nick2 :the message");

    /* 005 */
    WEE_CHECK_PARSE(NULL,
                    ":irc.example.com 005 mynick MODES=4 CHANLIMIT=#:20 "
                    "NICKLEN=16 USERLEN=10 HOSTLEN=63 TOPICLEN=450 "
                    "KICKLEN=450 CHANNELLEN=30 KEYLEN=23 CHANTYPES=# "
                    "PREFIX=(ov)@+ CASEMAPPING=ascii CAPAB IRCD=dancer "
                    ":are available on this server",
                    "irc.example.com", "irc.example.com", "005", "mynick",
                    "mynick MODES=4 CHANLIMIT=#:20 NICKLEN=16 USERLEN=10 "
                    "HOSTLEN=63 TOPICLEN=450 KICKLEN=450 CHANNELLEN=30 "
                    "KEYLEN=23 CHANTYPES=# PREFIX=(ov)@+ CASEMAPPING=ascii "
                    "CAPAB IRCD=dancer :are available on this server",
                    "MODES=4 CHANLIMIT=#:20 NICKLEN=16 USERLEN=10 HOSTLEN=63 "
                    "TOPICLEN=450 KICKLEN=450 CHANNELLEN=30 KEYLEN=23 "
                    "CHANTYPES=# PREFIX=(ov)@+ CASEMAPPING=ascii CAPAB "
                    "IRCD=dancer :are available on this server",
                    17, 21, 21, 28,
                    NULL,
                    ":irc.example.com 005 mynick MODES=4 CHANLIMIT=#:20 "
                    "NICKLEN=16 USERLEN=10 HOSTLEN=63 TOPICLEN=450 "
                    "KICKLEN=450 CHANNELLEN=30 KEYLEN=23 CHANTYPES=# "
                    "PREFIX=(ov)@+ CASEMAPPING=ascii CAPAB IRCD=dancer "
                    ":are available on this server");

    /* 301 */
    WEE_CHECK_PARSE(NULL,
                    ":irc.example.com 301 mynick nick :away message for nick",
                    "irc.example.com", "irc.example.com", "301", "mynick",
                    "mynick nick :away message for nick",
                    "nick :away message for nick",
                    17, 21, 21, 28,
                    NULL,
                    ":irc.example.com 301 mynick nick :away message for nick");

    /* error */
    WEE_CHECK_PARSE(NULL,
                    "404 nick #channel :Cannot send to channel",
                    "nick", NULL, "404", "#channel",
                    "nick #channel :Cannot send to channel",
                    "Cannot send to channel",
                    0, 4, 9, 19,
                    NULL,
                    "404 nick #channel :Cannot send to channel");
    WEE_CHECK_PARSE(NULL,
                    ":irc.example.com 404 nick #channel :Cannot send to channel",
                    "irc.example.com", "irc.example.com", "404", "#channel",
                    "nick #channel :Cannot send to channel",
                    "Cannot send to channel",
                    17, 21, 26, 36,
                    NULL,
                    ":irc.example.com 404 nick #channel :Cannot send to channel");
}

/*
 * Tests functions:
 *   irc_message_parse_to_hashtable
 */

TEST(IrcMessage, ParseToHashtable)
{
    struct t_hashtable *hashtable;

    hashtable = irc_message_parse_to_hashtable (
        NULL,
        "@time=2019-08-03T12:13:00.000Z :nick!user@host PRIVMSG #channel "
        ":the message");
    CHECK(hashtable);

    STRCMP_EQUAL("time=2019-08-03T12:13:00.000Z",
                 (const char *)hashtable_get (hashtable, "tags"));
    STRCMP_EQUAL(":nick!user@host PRIVMSG #channel :the message",
                 (const char *)hashtable_get (hashtable, "message_without_tags"));
    STRCMP_EQUAL("nick",
                 (const char *)hashtable_get (hashtable, "nick"));
    STRCMP_EQUAL("nick!user@host",
                 (const char *)hashtable_get (hashtable, "host"));
    STRCMP_EQUAL("PRIVMSG",
                 (const char *)hashtable_get (hashtable, "command"));
    STRCMP_EQUAL("#channel",
                 (const char *)hashtable_get (hashtable, "channel"));
    STRCMP_EQUAL("#channel :the message",
                 (const char *)hashtable_get (hashtable, "arguments"));
    STRCMP_EQUAL("the message",
                 (const char *)hashtable_get (hashtable, "text"));
    STRCMP_EQUAL("47",
                 (const char *)hashtable_get (hashtable, "pos_command"));
    STRCMP_EQUAL("55",
                 (const char *)hashtable_get (hashtable, "pos_arguments"));
    STRCMP_EQUAL("55",
                 (const char *)hashtable_get (hashtable, "pos_channel"));
    STRCMP_EQUAL("65",
                 (const char *)hashtable_get (hashtable, "pos_text"));

    hashtable_free (hashtable);
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
