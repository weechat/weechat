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
#include "string.h"
#include "tests/tests.h"
#include "src/core/wee-config-file.h"
#include "src/core/wee-hashtable.h"
#include "src/core/wee-hook.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-message.h"
#include "src/plugins/irc/irc-server.h"
}

#define NICK_256_WITH_SPACE "nick_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxx_64_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxx_128_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx_256 test"

#define LOREM_IPSUM_512 "Lorem ipsum dolor sit amet, consectetur adipiscing " \
    "elit. Fusce auctor ac leo ut maximus. Curabitur vestibulum facilisis ne" \
    "que, vitae sodales elit pulvinar ac. Mauris suscipit pharetra metus eu " \
    "hendrerit. Proin viverra ligula ut nibh malesuada, vel vehicula leo pul" \
    "vinar. Nullam tellus dolor, posuere sed orci in, pretium fermentum ante" \
    ". Donec a quam vulputate, fermentum nisi nec, convallis sapien. Vestibu" \
    "lum malesuada dui eget iaculis sagittis. Praesent egestas non ex quis b" \
    "landit. Maecenas quis leo nunc. In."

#define LOREM_IPSUM_1024 "Lorem ipsum dolor sit amet, consectetur adipiscing" \
    " elit. Fusce auctor ac leo ut maximus. Curabitur vestibulum facilisis n" \
    "eque, vitae sodales elit pulvinar ac. Mauris suscipit pharetra metus eu" \
    " hendrerit. Proin viverra ligula ut nibh malesuada, vel vehicula leo pu" \
    "lvinar. Nullam tellus dolor, posuere sed orci in, pretium fermentum ant" \
    "e. Donec a quam vulputate, fermentum nisi nec, convallis sapien. Vestib" \
    "ulum malesuada dui eget iaculis sagittis. Praesent egestas non ex quis " \
    "blandit. Maecenas quis leo nunc. Integer eget tincidunt sapien, id lobo" \
    "rtis libero. Aliquam posuere turpis in libero luctus pharetra. Vestibul" \
    "um dui augue, volutpat ultricies laoreet in, varius sodales ante. Ut ne" \
    "c urna non lacus bibendum scelerisque. Nullam convallis aliquet lectus " \
    "interdum volutpat. Phasellus lacus tortor, elementum hendrerit lobortis" \
    " ac, commodo id augue. Morbi imperdiet interdum consequat. Mauris purus" \
    " lectus, ultrices sed velit et, pretium rhoncus erat. Pellentesque pell" \
    "entesque efficitur nisl quis sodales. Nam hendreri."

#define CHANNELS_512 "#channel01,#channel02,#channel03,#channel04,#channel05" \
    ",#channel06,#channel07,#channel08,#channel09,#channel10,#channel11,#cha" \
    "nnel12,#channel13,#channel14,#channel15,#channel16,#channel17,#channel1" \
    "8,#channel19,#channel20,#channel21,#channel22,#channel23,#channel24,#ch" \
    "annel25,#channel26,#channel27,#channel28,#channel29,#channel30,#channel" \
    "31,#channel32,#channel33,#channel34,#channel35,#channel36,#channel37,#c" \
    "hannel38,#channel39,#channel40,#channel41,#channel42,#channel43,#channe" \
    "l44,#channel45,#channel46,#cha47"

#define NICKS_512_SPACE "nick01 nick02 nick03 nick04 nick05 nick06 nick07 ni" \
    "ck08 nick09 nick10 nick11 nick12 nick13 nick14 nick15 nick16 nick17 nic" \
    "k18 nick19 nick20 nick21 nick22 nick23 nick24 nick25 nick26 nick27 nick" \
    "28 nick29 nick30 nick31 nick32 nick33 nick34 nick35 nick36 nick37 nick3" \
    "8 nick39 nick40 nick41 nick42 nick43 nick44 nick45 nick46 nick47 nick48" \
    " nick49 nick50 nick51 nick52 nick53 nick54 nick55 nick56 nick57 nick58 " \
    "nick59 nick60 nick61 nick62 nick63 nick64 nick65 nick66 nick67 nick68 n" \
    "ick69 nick70 nick71 nick72 nick__73"

#define NICKS_512_COMMA "nick01,nick02,nick03,nick04,nick05,nick06,nick07,ni" \
    "ck08,nick09,nick10,nick11,nick12,nick13,nick14,nick15,nick16,nick17,nic" \
    "k18,nick19,nick20,nick21,nick22,nick23,nick24,nick25,nick26,nick27,nick" \
    "28,nick29,nick30,nick31,nick32,nick33,nick34,nick35,nick36,nick37,nick3" \
    "8,nick39,nick40,nick41,nick42,nick43,nick44,nick45,nick46,nick47,nick48" \
    ",nick49,nick50,nick51,nick52,nick53,nick54,nick55,nick56,nick57,nick58," \
    "nick59,nick60,nick61,nick62,nick63,nick64,nick65,nick66,nick67,nick68,n" \
    "ick69,nick70,nick71,nick72,nick__73"

#define MSG_005 "CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFLMPQScgimn" \
    "prstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES=4 NETWORK=fr" \
    "eenode STATUSMSG=@+ CALLERID=g CASEMAPPING=rfc1459 CHARSET=ascii NICKLE" \
    "N=16 CHANNELLEN=50 TOPICLEN=390 DEAF=D FNC TARGMAX=NAMES:1,LIST:1,KICK:" \
    "1,WHOIS:1,PRIVMSG:4,NOTICE:4,ACCEPT:,MONITOR: EXTBAN=$,ajrxz CLIENTVER=" \
    "3.0 SAFELIST ELIST=CTU CPRIVMSG :are supported by this server"

#define MSG_LONG_005 "CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFLMPQS" \
    "cgimnprstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES=4 NETWO" \
    "RK=freenode STATUSMSG=@+ CALLERID=g CASEMAPPING=rfc1459 CHARSET=ascii N" \
    "ICKLEN=16 CHANNELLEN=50 TOPICLEN=390 DEAF=D FNC TARGMAX=NAMES:1,LIST:1," \
    "KICK:1,WHOIS:1,PRIVMSG:4,NOTICE:4,ACCEPT:,MONITOR: EXTBAN=$,ajrxz CLIEN" \
    "TVER=3.0 SAFELIST ELIST=CTU CPRIVMSG TEST1:abc TEST2:dev TEST3:ghi TEST" \
    "4:jkl TEST5:mno TEST6:pqr TEST7:stu TEST8:vwx TEST9:yz ABC:1 DEF:2 GHI:" \
    "3 JKL:4 MNO:5 PQR:6 STU:7 VWX:8 YT:9 :are supported by this server"

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

char *
convert_irc_charset_cb (const void *pointer, void *data,
                        const char *modifier, const char *modifier_data,
                        const char *string)
{
    char *new_string;
    int length;

    /* make C++ compiler happy */
    (void) pointer;
    (void) data;
    (void) modifier;
    (void) modifier_data;

    if (!string)
        return NULL;

    length = strlen (string) + 32 + 1;
    new_string = (char *)malloc (length);
    if (!new_string)
        return NULL;

    snprintf (new_string, length, "%s MODIFIED", string);

    return new_string;
}

/*
 * Tests functions:
 *   irc_message_convert_charset
 */

TEST(IrcMessage, ConvertCharset)
{
    struct t_hook *hook;
    char *str;

    hook = hook_modifier (NULL, "convert_irc_charset",
                          &convert_irc_charset_cb, NULL, NULL);

    POINTERS_EQUAL(NULL,
                   irc_message_convert_charset (NULL, 0,
                                                "convert_irc_charset", NULL));

    str = irc_message_convert_charset ("PRIVMSG #channel :this is a test", 18,
                                       "convert_irc_charset", NULL);
    STRCMP_EQUAL("PRIVMSG #channel :this is a test MODIFIED", str);
    free (str);

    unhook (hook);
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
    struct t_irc_server *server;
    struct t_hashtable *hashtable;

    server = irc_server_alloc ("test_split_msg");
    CHECK(server);

    /* NULL server, NULL message */
    hashtable = irc_message_split (NULL, NULL);
    CHECK(hashtable);
    LONGS_EQUAL(1, hashtable->items_count);
    STRCMP_EQUAL("0",
                 (const char *)hashtable_get (hashtable, "count"));
    hashtable_free (hashtable);

    /* NULL message */
    hashtable = irc_message_split (server, NULL);
    CHECK(hashtable);
    LONGS_EQUAL(1, hashtable->items_count);
    STRCMP_EQUAL("0",
                 (const char *)hashtable_get (hashtable, "count"));
    hashtable_free (hashtable);

    /* empty message: no split */
    hashtable = irc_message_split (server, "");
    CHECK(hashtable);
    LONGS_EQUAL(2, hashtable->items_count);
    STRCMP_EQUAL("1",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("",
                 (const char *)hashtable_get (hashtable, "msg1"));
    hashtable_free (hashtable);

    /* ISON with small content: no split */
    hashtable = irc_message_split (server, "ISON :nick1 nick2");
    CHECK(hashtable);
    LONGS_EQUAL(3, hashtable->items_count);
    STRCMP_EQUAL("1",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("ISON :nick1 nick2",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("nick1 nick2",
                 (const char *)hashtable_get (hashtable, "args1"));
    hashtable_free (hashtable);

    /* ISON with 512 bytes of content: 1 split */
    hashtable = irc_message_split (server, "ISON :" NICKS_512_SPACE);
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("ISON :nick01 nick02 nick03 nick04 nick05 nick06 nick07 nick"
                 "08 nick09 nick10 nick11 nick12 nick13 nick14 nick15 nick16 "
                 "nick17 nick18 nick19 nick20 nick21 nick22 nick23 nick24 nic"
                 "k25 nick26 nick27 nick28 nick29 nick30 nick31 nick32 nick33"
                 " nick34 nick35 nick36 nick37 nick38 nick39 nick40 nick41 ni"
                 "ck42 nick43 nick44 nick45 nick46 nick47 nick48 nick49 nick5"
                 "0 nick51 nick52 nick53 nick54 nick55 nick56 nick57 nick58",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("nick01 nick02 nick03 nick04 nick05 nick06 nick07 nick08 nic"
                 "k09 nick10 nick11 nick12 nick13 nick14 nick15 nick16 nick17"
                 " nick18 nick19 nick20 nick21 nick22 nick23 nick24 nick25 ni"
                 "ck26 nick27 nick28 nick29 nick30 nick31 nick32 nick33 nick3"
                 "4 nick35 nick36 nick37 nick38 nick39 nick40 nick41 nick42 n"
                 "ick43 nick44 nick45 nick46 nick47 nick48 nick49 nick50 nick"
                 "51 nick52 nick53 nick54 nick55 nick56 nick57 nick58",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL("ISON :nick59 nick60 nick61 nick62 nick63 nick64 nick65 nick"
                 "66 nick67 nick68 nick69 nick70 nick71 nick72 nick__73",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("nick59 nick60 nick61 nick62 nick63 nick64 nick65 nick66 nic"
                 "k67 nick68 nick69 nick70 nick71 nick72 nick__73",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* JOIN with small content: no split */
    hashtable = irc_message_split (server, "JOIN #channel1,#channel2");
    CHECK(hashtable);
    LONGS_EQUAL(3, hashtable->items_count);
    STRCMP_EQUAL("1",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("JOIN #channel1,#channel2",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("#channel1,#channel2",
                 (const char *)hashtable_get (hashtable, "args1"));
    hashtable_free (hashtable);

    /* JOIN with 512 bytes of content: 1 split */
    hashtable = irc_message_split (server, "JOIN " CHANNELS_512);
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("JOIN #channel01,#channel02,#channel03,#channel04,#channel05"
                 ",#channel06,#channel07,#channel08,#channel09,#channel10,#ch"
                 "annel11,#channel12,#channel13,#channel14,#channel15,#channe"
                 "l16,#channel17,#channel18,#channel19,#channel20,#channel21,"
                 "#channel22,#channel23,#channel24,#channel25,#channel26,#cha"
                 "nnel27,#channel28,#channel29,#channel30,#channel31,#channel"
                 "32,#channel33,#channel34,#channel35,#channel36,#channel37,#"
                 "channel38,#channel39,#channel40,#channel41,#channel42,#chan"
                 "nel43,#channel44,#channel45",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("#channel01,#channel02,#channel03,#channel04,#channel05,#cha"
                 "nnel06,#channel07,#channel08,#channel09,#channel10,#channel"
                 "11,#channel12,#channel13,#channel14,#channel15,#channel16,#"
                 "channel17,#channel18,#channel19,#channel20,#channel21,#chan"
                 "nel22,#channel23,#channel24,#channel25,#channel26,#channel2"
                 "7,#channel28,#channel29,#channel30,#channel31,#channel32,#c"
                 "hannel33,#channel34,#channel35,#channel36,#channel37,#chann"
                 "el38,#channel39,#channel40,#channel41,#channel42,#channel43"
                 ",#channel44,#channel45",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL("JOIN #channel46,#cha47",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("#channel46,#cha47",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* JOIN with 512 bytes of content and 3 keys: 1 split */
    hashtable = irc_message_split (server,
                                   "JOIN "
                                   CHANNELS_512
                                   " key1,key2,key3");
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("JOIN #channel01,#channel02,#channel03,#channel04,#channel05"
                 ",#channel06,#channel07,#channel08,#channel09,#channel10,#ch"
                 "annel11,#channel12,#channel13,#channel14,#channel15,#channe"
                 "l16,#channel17,#channel18,#channel19,#channel20,#channel21,"
                 "#channel22,#channel23,#channel24,#channel25,#channel26,#cha"
                 "nnel27,#channel28,#channel29,#channel30,#channel31,#channel"
                 "32,#channel33,#channel34,#channel35,#channel36,#channel37,#"
                 "channel38,#channel39,#channel40,#channel41,#channel42,#chan"
                 "nel43,#channel44 key1,key2,key3",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("#channel01,#channel02,#channel03,#channel04,#channel05,#cha"
                 "nnel06,#channel07,#channel08,#channel09,#channel10,#channel"
                 "11,#channel12,#channel13,#channel14,#channel15,#channel16,#"
                 "channel17,#channel18,#channel19,#channel20,#channel21,#chan"
                 "nel22,#channel23,#channel24,#channel25,#channel26,#channel2"
                 "7,#channel28,#channel29,#channel30,#channel31,#channel32,#c"
                 "hannel33,#channel34,#channel35,#channel36,#channel37,#chann"
                 "el38,#channel39,#channel40,#channel41,#channel42,#channel43"
                 ",#channel44 key1,key2,key3",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL("JOIN #channel45,#channel46,#cha47",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("#channel45,#channel46,#cha47",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* MONITOR with small content: no split */
    hashtable = irc_message_split (server, "MONITOR + nick1,nick2");
    CHECK(hashtable);
    LONGS_EQUAL(3, hashtable->items_count);
    STRCMP_EQUAL("1",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("MONITOR + nick1,nick2",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("nick1,nick2",
                 (const char *)hashtable_get (hashtable, "args1"));
    hashtable_free (hashtable);

    /* MONITOR with 512 bytes of content: 1 split */
    hashtable = irc_message_split (server, "MONITOR + " NICKS_512_COMMA);
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("MONITOR + nick01,nick02,nick03,nick04,nick05,nick06,nick07,"
                 "nick08,nick09,nick10,nick11,nick12,nick13,nick14,nick15,nic"
                 "k16,nick17,nick18,nick19,nick20,nick21,nick22,nick23,nick24"
                 ",nick25,nick26,nick27,nick28,nick29,nick30,nick31,nick32,ni"
                 "ck33,nick34,nick35,nick36,nick37,nick38,nick39,nick40,nick4"
                 "1,nick42,nick43,nick44,nick45,nick46,nick47,nick48,nick49,n"
                 "ick50,nick51,nick52,nick53,nick54,nick55,nick56,nick57,nick"
                 "58",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("nick01,nick02,nick03,nick04,nick05,nick06,nick07,nick08,nic"
                 "k09,nick10,nick11,nick12,nick13,nick14,nick15,nick16,nick17"
                 ",nick18,nick19,nick20,nick21,nick22,nick23,nick24,nick25,ni"
                 "ck26,nick27,nick28,nick29,nick30,nick31,nick32,nick33,nick3"
                 "4,nick35,nick36,nick37,nick38,nick39,nick40,nick41,nick42,n"
                 "ick43,nick44,nick45,nick46,nick47,nick48,nick49,nick50,nick"
                 "51,nick52,nick53,nick54,nick55,nick56,nick57,nick58",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL("MONITOR + nick59,nick60,nick61,nick62,nick63,nick64,nick65,"
                 "nick66,nick67,nick68,nick69,nick70,nick71,nick72,nick__73",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("nick59,nick60,nick61,nick62,nick63,nick64,nick65,nick66,nic"
                 "k67,nick68,nick69,nick70,nick71,nick72,nick__73",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* MONITOR with 512 bytes of content (invalid, no action): 1 split */
    hashtable = irc_message_split (server, "MONITOR :" NICKS_512_COMMA);
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("MONITOR :nick01,nick02,nick03,nick04,nick05,nick06,nick07,n"
                 "ick08,nick09,nick10,nick11,nick12,nick13,nick14,nick15,nick"
                 "16,nick17,nick18,nick19,nick20,nick21,nick22,nick23,nick24,"
                 "nick25,nick26,nick27,nick28,nick29,nick30,nick31,nick32,nic"
                 "k33,nick34,nick35,nick36,nick37,nick38,nick39,nick40,nick41"
                 ",nick42,nick43,nick44,nick45,nick46,nick47,nick48,nick49,ni"
                 "ck50,nick51,nick52,nick53,nick54,nick55,nick56,nick57,nick5"
                 "8",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("nick01,nick02,nick03,nick04,nick05,nick06,nick07,nick08,nic"
                 "k09,nick10,nick11,nick12,nick13,nick14,nick15,nick16,nick17"
                 ",nick18,nick19,nick20,nick21,nick22,nick23,nick24,nick25,ni"
                 "ck26,nick27,nick28,nick29,nick30,nick31,nick32,nick33,nick3"
                 "4,nick35,nick36,nick37,nick38,nick39,nick40,nick41,nick42,n"
                 "ick43,nick44,nick45,nick46,nick47,nick48,nick49,nick50,nick"
                 "51,nick52,nick53,nick54,nick55,nick56,nick57,nick58",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL("MONITOR :nick59,nick60,nick61,nick62,nick63,nick64,nick65,n"
                 "ick66,nick67,nick68,nick69,nick70,nick71,nick72,nick__73",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("nick59,nick60,nick61,nick62,nick63,nick64,nick65,nick66,nic"
                 "k67,nick68,nick69,nick70,nick71,nick72,nick__73",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* PONG: no split */
    hashtable = irc_message_split (server, "PONG");
    CHECK(hashtable);
    LONGS_EQUAL(2, hashtable->items_count);
    STRCMP_EQUAL("1",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("PONG",
                 (const char *)hashtable_get (hashtable, "msg1"));
    hashtable_free (hashtable);

    /* PRIVMSG with small content: no split */
    hashtable = irc_message_split (server, "PRIVMSG #channel :test");
    CHECK(hashtable);
    LONGS_EQUAL(3, hashtable->items_count);
    STRCMP_EQUAL("1",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("PRIVMSG #channel :test",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("test",
                 (const char *)hashtable_get (hashtable, "args1"));
    hashtable_free (hashtable);

    /* PRIVMSG with 512 bytes of content: 1 split */
    hashtable = irc_message_split (server,
                                   "PRIVMSG #channel :" LOREM_IPSUM_512);
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("PRIVMSG #channel :Lorem ipsum dolor sit amet, consectetur a"
                 "dipiscing elit. Fusce auctor ac leo ut maximus. Curabitur v"
                 "estibulum facilisis neque, vitae sodales elit pulvinar ac. "
                 "Mauris suscipit pharetra metus eu hendrerit. Proin viverra "
                 "ligula ut nibh malesuada, vel vehicula leo pulvinar. Nullam"
                 " tellus dolor, posuere sed orci in, pretium fermentum ante."
                 " Donec a quam vulputate, fermentum nisi nec, convallis sapi"
                 "en.",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fu"
                 "sce auctor ac leo ut maximus. Curabitur vestibulum facilisi"
                 "s neque, vitae sodales elit pulvinar ac. Mauris suscipit ph"
                 "aretra metus eu hendrerit. Proin viverra ligula ut nibh mal"
                 "esuada, vel vehicula leo pulvinar. Nullam tellus dolor, pos"
                 "uere sed orci in, pretium fermentum ante. Donec a quam vulp"
                 "utate, fermentum nisi nec, convallis sapien.",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL("PRIVMSG #channel :Vestibulum malesuada dui eget iaculis sag"
                 "ittis. Praesent egestas non ex quis blandit. Maecenas quis "
                 "leo nunc. In.",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("Vestibulum malesuada dui eget iaculis sagittis. Praesent eg"
                 "estas non ex quis blandit. Maecenas quis leo nunc. In.",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* PRIVMSG with tags and host and 512 bytes of content: 1 split */
    hashtable = irc_message_split (server,
                                   "@tag1=value1;tag2=value2;tag3=value3 "
                                   ":nick!user@host "
                                   "PRIVMSG #channel :" LOREM_IPSUM_512);
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("@tag1=value1;tag2=value2;tag3=value3 :nick!user@host PRIVMS"
                 "G #channel :Lorem ipsum dolor sit amet, consectetur adipisc"
                 "ing elit. Fusce auctor ac leo ut maximus. Curabitur vestibu"
                 "lum facilisis neque, vitae sodales elit pulvinar ac. Mauris"
                 " suscipit pharetra metus eu hendrerit. Proin viverra ligula"
                 " ut nibh malesuada, vel vehicula leo pulvinar. Nullam tellu"
                 "s dolor, posuere sed orci in, pretium fermentum ante. Donec"
                 " a quam vulputate, fermentum nisi nec, convallis sapien.",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fu"
                 "sce auctor ac leo ut maximus. Curabitur vestibulum facilisi"
                 "s neque, vitae sodales elit pulvinar ac. Mauris suscipit ph"
                 "aretra metus eu hendrerit. Proin viverra ligula ut nibh mal"
                 "esuada, vel vehicula leo pulvinar. Nullam tellus dolor, pos"
                 "uere sed orci in, pretium fermentum ante. Donec a quam vulp"
                 "utate, fermentum nisi nec, convallis sapien.",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL("@tag1=value1;tag2=value2;tag3=value3 :nick!user@host PRIVMS"
                 "G #channel :Vestibulum malesuada dui eget iaculis sagittis."
                 " Praesent egestas non ex quis blandit. Maecenas quis leo nu"
                 "nc. In.",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("Vestibulum malesuada dui eget iaculis sagittis. Praesent eg"
                 "estas non ex quis blandit. Maecenas quis leo nunc. In.",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* PRIVMSG with "\x01ACTION " + 512 bytes + "\x01": 1 split */
    hashtable = irc_message_split (server,
                                   "PRIVMSG #channel :"
                                   "\x01" "ACTION "
                                   LOREM_IPSUM_512
                                   "\x01");
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("PRIVMSG #channel :" "\x01" "ACTION " "Lorem ipsum dolor sit"
                 " amet, consectetur adipiscing elit. Fusce auctor ac leo ut "
                 "maximus. Curabitur vestibulum facilisis neque, vitae sodale"
                 "s elit pulvinar ac. Mauris suscipit pharetra metus eu hendr"
                 "erit. Proin viverra ligula ut nibh malesuada, vel vehicula "
                 "leo pulvinar. Nullam tellus dolor, posuere sed orci in, pre"
                 "tium fermentum ante. Donec a quam vulputate, fermentum nisi"
                 " nec, convallis" "\x01",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fu"
                 "sce auctor ac leo ut maximus. Curabitur vestibulum facilisi"
                 "s neque, vitae sodales elit pulvinar ac. Mauris suscipit ph"
                 "aretra metus eu hendrerit. Proin viverra ligula ut nibh mal"
                 "esuada, vel vehicula leo pulvinar. Nullam tellus dolor, pos"
                 "uere sed orci in, pretium fermentum ante. Donec a quam vulp"
                 "utate, fermentum nisi nec, convallis",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL("PRIVMSG #channel :" "\x01" "ACTION " "sapien. Vestibulum ma"
                 "lesuada dui eget iaculis sagittis. Praesent egestas non ex "
                 "quis blandit. Maecenas quis leo nunc. In." "\x01",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("sapien. Vestibulum malesuada dui eget iaculis sagittis. Pra"
                 "esent egestas non ex quis blandit. Maecenas quis leo nunc. "
                 "In.",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* PRIVMSG with 1024 bytes of content: 2 splits */
    hashtable = irc_message_split (server,
                                   "PRIVMSG #channel :" LOREM_IPSUM_1024);
    CHECK(hashtable);
    LONGS_EQUAL(7, hashtable->items_count);
    STRCMP_EQUAL("3",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("PRIVMSG #channel :Lorem ipsum dolor sit amet, consectetur a"
                 "dipiscing elit. Fusce auctor ac leo ut maximus. Curabitur v"
                 "estibulum facilisis neque, vitae sodales elit pulvinar ac. "
                 "Mauris suscipit pharetra metus eu hendrerit. Proin viverra "
                 "ligula ut nibh malesuada, vel vehicula leo pulvinar. Nullam"
                 " tellus dolor, posuere sed orci in, pretium fermentum ante."
                 " Donec a quam vulputate, fermentum nisi nec, convallis sapi"
                 "en.",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fu"
                 "sce auctor ac leo ut maximus. Curabitur vestibulum facilisi"
                 "s neque, vitae sodales elit pulvinar ac. Mauris suscipit ph"
                 "aretra metus eu hendrerit. Proin viverra ligula ut nibh mal"
                 "esuada, vel vehicula leo pulvinar. Nullam tellus dolor, pos"
                 "uere sed orci in, pretium fermentum ante. Donec a quam vulp"
                 "utate, fermentum nisi nec, convallis sapien.",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL("PRIVMSG #channel :Vestibulum malesuada dui eget iaculis sag"
                 "ittis. Praesent egestas non ex quis blandit. Maecenas quis "
                 "leo nunc. Integer eget tincidunt sapien, id lobortis libero"
                 ". Aliquam posuere turpis in libero luctus pharetra. Vestibu"
                 "lum dui augue, volutpat ultricies laoreet in, varius sodale"
                 "s ante. Ut nec urna non lacus bibendum scelerisque. Nullam "
                 "convallis aliquet lectus interdum volutpat. Phasellus lacus",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("Vestibulum malesuada dui eget iaculis sagittis. Praesent eg"
                 "estas non ex quis blandit. Maecenas quis leo nunc. Integer "
                 "eget tincidunt sapien, id lobortis libero. Aliquam posuere "
                 "turpis in libero luctus pharetra. Vestibulum dui augue, vol"
                 "utpat ultricies laoreet in, varius sodales ante. Ut nec urn"
                 "a non lacus bibendum scelerisque. Nullam convallis aliquet "
                 "lectus interdum volutpat. Phasellus lacus",
                 (const char *)hashtable_get (hashtable, "args2"));
    STRCMP_EQUAL("PRIVMSG #channel :tortor, elementum hendrerit lobortis ac, "
                 "commodo id augue. Morbi imperdiet interdum consequat. Mauri"
                 "s purus lectus, ultrices sed velit et, pretium rhoncus erat"
                 ". Pellentesque pellentesque efficitur nisl quis sodales. Na"
                 "m hendreri.",
                 (const char *)hashtable_get (hashtable, "msg3"));
    STRCMP_EQUAL("tortor, elementum hendrerit lobortis ac, commodo id augue. "
                 "Morbi imperdiet interdum consequat. Mauris purus lectus, ul"
                 "trices sed velit et, pretium rhoncus erat. Pellentesque pel"
                 "lentesque efficitur nisl quis sodales. Nam hendreri.",
                 (const char *)hashtable_get (hashtable, "args3"));
    hashtable_free (hashtable);

    /* 005: no split */
    hashtable = irc_message_split (server, "005 nick " MSG_005);
    CHECK(hashtable);
    LONGS_EQUAL(3, hashtable->items_count);
    STRCMP_EQUAL("1",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("005 nick CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFL"
                 "MPQScgimnprstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:1"
                 "00 MODES=4 NETWORK=freenode STATUSMSG=@+ CALLERID=g CASEMAP"
                 "PING=rfc1459 CHARSET=ascii NICKLEN=16 CHANNELLEN=50 TOPICLE"
                 "N=390 DEAF=D FNC TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIV"
                 "MSG:4,NOTICE:4,ACCEPT:,MONITOR: EXTBAN=$,ajrxz CLIENTVER=3."
                 "0 SAFELIST ELIST=CTU CPRIVMSG :are supported by this server",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFLMPQScgimn"
                 "prstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES="
                 "4 NETWORK=freenode STATUSMSG=@+ CALLERID=g CASEMAPPING=rfc1"
                 "459 CHARSET=ascii NICKLEN=16 CHANNELLEN=50 TOPICLEN=390 DEA"
                 "F=D FNC TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIVMSG:4,NOT"
                 "ICE:4,ACCEPT:,MONITOR: EXTBAN=$,ajrxz CLIENTVER=3.0 SAFELIS"
                 "T ELIST=CTU CPRIVMSG",
                 (const char *)hashtable_get (hashtable, "args1"));
    hashtable_free (hashtable);

    /* long 005: 1 split */
    hashtable = irc_message_split (server, "005 nick " MSG_LONG_005);
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("005 nick CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFL"
                 "MPQScgimnprstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:1"
                 "00 MODES=4 NETWORK=freenode STATUSMSG=@+ CALLERID=g CASEMAP"
                 "PING=rfc1459 CHARSET=ascii NICKLEN=16 CHANNELLEN=50 TOPICLE"
                 "N=390 DEAF=D FNC TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIV"
                 "MSG:4,NOTICE:4,ACCEPT:,MONITOR: EXTBAN=$,ajrxz CLIENTVER=3."
                 "0 SAFELIST ELIST=CTU CPRIVMSG TEST1:abc TEST2:dev TEST3:ghi"
                 " TEST4:jkl TEST5:mno TEST6:pqr TEST7:stu TEST8:vwx TEST9:yz"
                 " ABC:1 :are supported by this server",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("CHANTYPES=# EXCEPTS INVEX CHANMODES=eIbq,k,flj,CFLMPQScgimn"
                 "prstz CHANLIMIT=#:120 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES="
                 "4 NETWORK=freenode STATUSMSG=@+ CALLERID=g CASEMAPPING=rfc1"
                 "459 CHARSET=ascii NICKLEN=16 CHANNELLEN=50 TOPICLEN=390 DEA"
                 "F=D FNC TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIVMSG:4,NOT"
                 "ICE:4,ACCEPT:,MONITOR: EXTBAN=$,ajrxz CLIENTVER=3.0 SAFELIS"
                 "T ELIST=CTU CPRIVMSG TEST1:abc TEST2:dev TEST3:ghi TEST4:jk"
                 "l TEST5:mno TEST6:pqr TEST7:stu TEST8:vwx TEST9:yz ABC:1",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL("005 nick DEF:2 GHI:3 JKL:4 MNO:5 PQR:6 STU:7 VWX:8 YT:9 :ar"
                 "e supported by this server",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("DEF:2 GHI:3 JKL:4 MNO:5 PQR:6 STU:7 VWX:8 YT:9",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* 353 with small of content: no split */
    hashtable = irc_message_split (server,
                                   ":irc.example.org 353 mynick = #channel "
                                   ":nick1 nick2");
    CHECK(hashtable);
    LONGS_EQUAL(3, hashtable->items_count);
    STRCMP_EQUAL("1",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL(":irc.example.org 353 mynick = #channel :nick1 nick2",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("nick1 nick2",
                 (const char *)hashtable_get (hashtable, "args1"));
    hashtable_free (hashtable);

    /* 353 with 512 bytes of content: 1 split */
    hashtable = irc_message_split (server,
                                   ":irc.example.org 353 mynick = #channel "
                                   ":" NICKS_512_SPACE);
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL(":irc.example.org 353 mynick = #channel :nick01 nick02 nick0"
                 "3 nick04 nick05 nick06 nick07 nick08 nick09 nick10 nick11 n"
                 "ick12 nick13 nick14 nick15 nick16 nick17 nick18 nick19 nick"
                 "20 nick21 nick22 nick23 nick24 nick25 nick26 nick27 nick28 "
                 "nick29 nick30 nick31 nick32 nick33 nick34 nick35 nick36 nic"
                 "k37 nick38 nick39 nick40 nick41 nick42 nick43 nick44 nick45"
                 " nick46 nick47 nick48 nick49 nick50 nick51 nick52 nick53 ni"
                 "ck54 nick55 nick56 nick57 nick58 nick59 nick60 nick61 nick6"
                 "2 nick63 nick64 nick65 nick66 nick67",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("nick01 nick02 nick03 nick04 nick05 nick06 nick07 nick08 nic"
                 "k09 nick10 nick11 nick12 nick13 nick14 nick15 nick16 nick17"
                 " nick18 nick19 nick20 nick21 nick22 nick23 nick24 nick25 ni"
                 "ck26 nick27 nick28 nick29 nick30 nick31 nick32 nick33 nick3"
                 "4 nick35 nick36 nick37 nick38 nick39 nick40 nick41 nick42 n"
                 "ick43 nick44 nick45 nick46 nick47 nick48 nick49 nick50 nick"
                 "51 nick52 nick53 nick54 nick55 nick56 nick57 nick58 nick59 "
                 "nick60 nick61 nick62 nick63 nick64 nick65 nick66 nick67",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL(":irc.example.org 353 mynick = #channel :nick68 nick69 nick7"
                 "0 nick71 nick72 nick__73",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("nick68 nick69 nick70 nick71 nick72 nick__73",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* 353 with 512 bytes of content but no "=": 1 split */
    hashtable = irc_message_split (server,
                                   ":irc.example.org 353 mynick #channel "
                                   ":" NICKS_512_SPACE);
    CHECK(hashtable);
    LONGS_EQUAL(5, hashtable->items_count);
    STRCMP_EQUAL("2",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL(":irc.example.org 353 mynick #channel :nick01 nick02 nick03 "
                 "nick04 nick05 nick06 nick07 nick08 nick09 nick10 nick11 nic"
                 "k12 nick13 nick14 nick15 nick16 nick17 nick18 nick19 nick20"
                 " nick21 nick22 nick23 nick24 nick25 nick26 nick27 nick28 ni"
                 "ck29 nick30 nick31 nick32 nick33 nick34 nick35 nick36 nick3"
                 "7 nick38 nick39 nick40 nick41 nick42 nick43 nick44 nick45 n"
                 "ick46 nick47 nick48 nick49 nick50 nick51 nick52 nick53 nick"
                 "54 nick55 nick56 nick57 nick58 nick59 nick60 nick61 nick62 "
                 "nick63 nick64 nick65 nick66 nick67",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("nick01 nick02 nick03 nick04 nick05 nick06 nick07 nick08 nic"
                 "k09 nick10 nick11 nick12 nick13 nick14 nick15 nick16 nick17"
                 " nick18 nick19 nick20 nick21 nick22 nick23 nick24 nick25 ni"
                 "ck26 nick27 nick28 nick29 nick30 nick31 nick32 nick33 nick3"
                 "4 nick35 nick36 nick37 nick38 nick39 nick40 nick41 nick42 n"
                 "ick43 nick44 nick45 nick46 nick47 nick48 nick49 nick50 nick"
                 "51 nick52 nick53 nick54 nick55 nick56 nick57 nick58 nick59 "
                 "nick60 nick61 nick62 nick63 nick64 nick65 nick66 nick67",
                 (const char *)hashtable_get (hashtable, "args1"));
    STRCMP_EQUAL(":irc.example.org 353 mynick #channel :nick68 nick69 nick70 "
                 "nick71 nick72 nick__73",
                 (const char *)hashtable_get (hashtable, "msg2"));
    STRCMP_EQUAL("nick68 nick69 nick70 nick71 nick72 nick__73",
                 (const char *)hashtable_get (hashtable, "args2"));
    hashtable_free (hashtable);

    /* PRIVMSG with 512 bytes and split_msg_max_length == 0: no split */
    config_file_option_set (
        irc_config_server_default[IRC_SERVER_OPTION_SPLIT_MSG_MAX_LENGTH],
        "0", 0);
    hashtable = irc_message_split (server,
                                   "PRIVMSG #channel :" LOREM_IPSUM_512);
    CHECK(hashtable);
    LONGS_EQUAL(3, hashtable->items_count);
    STRCMP_EQUAL("1",
                 (const char *)hashtable_get (hashtable, "count"));
    STRCMP_EQUAL("PRIVMSG #channel :Lorem ipsum dolor sit amet, consectetur a"
                 "dipiscing elit. Fusce auctor ac leo ut maximus. Curabitur v"
                 "estibulum facilisis neque, vitae sodales elit pulvinar ac. "
                 "Mauris suscipit pharetra metus eu hendrerit. Proin viverra "
                 "ligula ut nibh malesuada, vel vehicula leo pulvinar. Nullam"
                 " tellus dolor, posuere sed orci in, pretium fermentum ante."
                 " Donec a quam vulputate, fermentum nisi nec, convallis sapi"
                 "en. Vestibulum malesuada dui eget iaculis sagittis. Praesen"
                 "t egestas non ex quis blandit. Maecenas quis leo nunc. In.",
                 (const char *)hashtable_get (hashtable, "msg1"));
    STRCMP_EQUAL("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fu"
                 "sce auctor ac leo ut maximus. Curabitur vestibulum facilisi"
                 "s neque, vitae sodales elit pulvinar ac. Mauris suscipit ph"
                 "aretra metus eu hendrerit. Proin viverra ligula ut nibh mal"
                 "esuada, vel vehicula leo pulvinar. Nullam tellus dolor, pos"
                 "uere sed orci in, pretium fermentum ante. Donec a quam vulp"
                 "utate, fermentum nisi nec, convallis sapien. Vestibulum mal"
                 "esuada dui eget iaculis sagittis. Praesent egestas non ex q"
                 "uis blandit. Maecenas quis leo nunc. In.",
                 (const char *)hashtable_get (hashtable, "args1"));
    hashtable_free (hashtable);
    config_file_option_unset (
        irc_config_server_default[IRC_SERVER_OPTION_SPLIT_MSG_MAX_LENGTH]);

    irc_server_free (server);
}
