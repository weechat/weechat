/*
 * test-irc-info.cpp - test IRC info functions
 *
 * Copyright (C) 2024 Sébastien Helleu <flashcode@flashtux.org>
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

#include "tests/tests.h"

extern "C"
{
#include "string.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-ignore.h"
#include "src/plugins/irc/irc-info.h"
#include "src/plugins/irc/irc-nick.h"
#include "src/plugins/irc/irc-notify.h"
#include "src/plugins/irc/irc-server.h"

extern void irc_info_create_string_with_pointer (char **string, void *pointer);
}

TEST_GROUP(IrcInfo)
{
};

/*
 * Tests functions:
 *   irc_info_create_string_with_pointer
 */

TEST(IrcInfo, CreateStringWithPointer)
{
    char *str;

    str = NULL;
    irc_info_create_string_with_pointer (&str, NULL);
    POINTERS_EQUAL(NULL, str);

    str = strdup ("test");
    irc_info_create_string_with_pointer (&str, (void *)0x1234abcd);
    STRCMP_EQUAL("0x1234abcd", str);
    free (str);
}

/*
 * Tests functions:
 *   irc_info_info_irc_is_channel_cb
 */

TEST(IrcInfo, InfoIrcIsChannelCb)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("server");
    CHECK(server);

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_channel", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_channel", ""));

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_channel", "test"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_is_channel", "#test"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_is_channel", "&test"));

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_channel", "server,test"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_is_channel", "server,#test"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_is_channel", "server,&test"));

    if (server->chantypes)
        free (server->chantypes);
    server->chantypes = strdup ("&");

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_channel", "server,test"));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_channel", "server,#test"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_is_channel", "server,&test"));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_info_irc_is_nick_cb
 */

TEST(IrcInfo, InfoIrcIsNickCb)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("server");
    CHECK(server);

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_nick", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_nick", ""));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_nick", "#test"));

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_nick", "&test"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_is_nick", "test"));

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_nick", "server,#test"));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_nick", "server,&test"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_is_nick", "server,test"));

    if (server->chantypes)
        free (server->chantypes);
    server->chantypes = strdup ("&");

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_nick", "server,#test"));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_nick", "server,&test"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_is_nick", "server,test"));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_info_irc_nick_cb
 */

TEST(IrcInfo, InfoIrcNickCb)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("server");
    CHECK(server);

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_nick", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_nick", ""));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_nick", "test"));

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_nick", "server"));

    if (server->nick)
        free (server->nick);
    server->nick = strdup ("alice");

    WEE_TEST_STR("alice", hook_info_get (NULL, "irc_nick", "server"));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_info_irc_nick_from_host_cb
 */

TEST(IrcInfo, InfoIrcNickFromHostCb)
{
    char *str;

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_nick_from_host", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_nick_from_host", ""));

    WEE_TEST_STR("user", hook_info_get (NULL, "irc_nick_from_host", "user"));
    WEE_TEST_STR("user", hook_info_get (NULL, "irc_nick_from_host", "user "));
    WEE_TEST_STR("user", hook_info_get (NULL, "irc_nick_from_host", ":user "));
    WEE_TEST_STR("user", hook_info_get (NULL, "irc_nick_from_host", ":user!host"));
    WEE_TEST_STR("user", hook_info_get (NULL, "irc_nick_from_host", ":user!host"));
}

/*
 * Tests functions:
 *   irc_info_info_irc_nick_color_cb
 *   irc_info_info_irc_nick_color_name_cb
 */

TEST(IrcInfo, InfoIrcNickColorCb)
{
    char *str, str_color[64];

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_nick_color", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_nick_color", ""));

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_nick_color_name", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_nick_color_name", ""));

    snprintf (str_color, sizeof (str_color), "%s", gui_color_get_custom ("186"));
    WEE_TEST_STR(str_color, hook_info_get (NULL, "irc_nick_color", "Nick"));
    WEE_TEST_STR("186", hook_info_get (NULL, "irc_nick_color_name", "Nick"));
}

/*
 * Tests functions:
 *   irc_info_info_irc_buffer_cb
 */

TEST(IrcInfo, InfoIrcBufferCb)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel, *channel_pv;
    struct t_irc_nick *nick;
    char *str, str_pointer[64];

    server = irc_server_alloc ("local");
    CHECK(server);

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    nick = irc_nick_new (server, channel, "bob", "user@host", "@", 0, NULL, NULL);
    CHECK(nick);

    channel_pv = irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob",
                                  1, 0);
    CHECK(channel_pv);

    POINTERS_EQUAL(NULL, hook_info_get (NULL, "irc_buffer", NULL));
    POINTERS_EQUAL(NULL, hook_info_get (NULL, "irc_buffer", ""));
    POINTERS_EQUAL(NULL, hook_info_get (NULL, "irc_buffer", "xxx"));

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_buffer", "local"));

    snprintf (str_pointer, sizeof (str_pointer), "%p", channel->buffer);
    WEE_TEST_STR(str_pointer, hook_info_get (NULL, "irc_buffer", "local,#test"));

    snprintf (str_pointer, sizeof (str_pointer), "%p", channel->buffer);
    WEE_TEST_STR(str_pointer, hook_info_get (NULL, "irc_buffer", "local,#test,bob"));

    irc_nick_free (server, channel, nick);
    if (channel_pv->buffer)
        gui_buffer_close (channel_pv->buffer);
    if (channel->buffer)
        gui_buffer_close (channel->buffer);
    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_info_irc_server_isupport_cb
 *   irc_info_info_irc_server_isupport_value_cb
 */

TEST(IrcInfo, InfoIrcServerIsupportCb)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("server");
    CHECK(server);

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup (
        "BOT=B CALLERID CASEMAPPING=ascii DEAF=D KICKLEN=180 MODES=6 EXCEPTS "
        "INVEX NICKLEN=15 NETWORK=debian MAXLIST=beI:100 MAXTARGETS=4 "
        "CHANTYPES=#");

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_isupport", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_isupport", ""));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_isupport", "server"));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_isupport", "server,XXX"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_server_isupport", "server,NETWORK"));

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_isupport_value", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_isupport_value", ""));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_isupport_value", "server"));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_isupport_value", "server,XXX"));
    WEE_TEST_STR("debian", hook_info_get (NULL, "irc_server_isupport_value", "server,NETWORK"));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_info_irc_server_cap_cb
 *   irc_info_info_irc_server_cap_value_cb
 */

TEST(IrcInfo, InfoIrcServerCapCb)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("server");
    CHECK(server);

    hashtable_set (server->cap_list, "test_cap", "test_value");

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_cap", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_cap", ""));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_cap", "server"));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_cap", "server,xxx"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_server_cap", "server,test_cap"));

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_cap_value", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_cap_value", ""));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_cap_value", "server"));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_server_cap_value", "server,xxx"));
    WEE_TEST_STR("test_value", hook_info_get (NULL, "irc_server_cap_value", "server,test_cap"));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_info_irc_is_message_ignored_cb
 */

TEST(IrcInfo, InfoIrcIsMessageIgnoredCb)
{
    struct t_irc_server *server;
    struct t_irc_ignore *ignore;
    char *str;

    server = irc_server_alloc ("server");
    CHECK(server);

    ignore = irc_ignore_new ("bob", "server", NULL);
    CHECK(ignore);

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_message_ignored", NULL));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_message_ignored", ""));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_message_ignored", "xxx"));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_message_ignored",
                                      "xxx,:alice!user@host PRIVMSG #channel :test"));
    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_message_ignored", "server"));

    WEE_TEST_STR(NULL, hook_info_get (NULL, "irc_is_message_ignored",
                                      "server,:alice!user@host PRIVMSG #channel :test"));
    WEE_TEST_STR("1", hook_info_get (NULL, "irc_is_message_ignored",
                                     "server,:bob!user@host PRIVMSG #channel :test"));

    irc_ignore_free (ignore);
    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_info_hashtable_irc_message_parse_cb
 */

TEST(IrcInfo, InfoHashtableIrcMessageParseCb)
{
    struct t_hashtable *hashtable, *result;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL, NULL);
    CHECK(hashtable);

    POINTERS_EQUAL(NULL, hook_info_get_hashtable (NULL, "irc_message_parse", NULL));
    POINTERS_EQUAL(NULL, hook_info_get_hashtable (NULL, "irc_message_parse", hashtable));

    hashtable_set (hashtable, "message",
                   "@tag1=value1 :alice!user@host PRIVMSG #channel :this is a test");

    result = hook_info_get_hashtable (NULL, "irc_message_parse", hashtable);
    CHECK(result);
    STRCMP_EQUAL("tag1=value1", (const char *)hashtable_get (result, "tags"));
    STRCMP_EQUAL("alice", (const char *)hashtable_get (result, "nick"));
    STRCMP_EQUAL("user", (const char *)hashtable_get (result, "user"));
    STRCMP_EQUAL("alice!user@host", (const char *)hashtable_get (result, "host"));
    STRCMP_EQUAL("PRIVMSG", (const char *)hashtable_get (result, "command"));
    STRCMP_EQUAL("#channel", (const char *)hashtable_get (result, "channel"));
    STRCMP_EQUAL("#channel :this is a test", (const char *)hashtable_get (result, "arguments"));
    STRCMP_EQUAL("this is a test", (const char *)hashtable_get (result, "text"));
    STRCMP_EQUAL("2", (const char *)hashtable_get (result, "num_params"));
    STRCMP_EQUAL("#channel", (const char *)hashtable_get (result, "param1"));
    STRCMP_EQUAL("this is a test", (const char *)hashtable_get (result, "param2"));
    STRCMP_EQUAL("30", (const char *)hashtable_get (result, "pos_command"));
    STRCMP_EQUAL("38", (const char *)hashtable_get (result, "pos_arguments"));
    STRCMP_EQUAL("38", (const char *)hashtable_get (result, "pos_channel"));
    STRCMP_EQUAL("48", (const char *)hashtable_get (result, "pos_text"));
    hashtable_free (result);

    hashtable_free (hashtable);
}

/*
 * Tests functions:
 *   irc_info_info_hashtable_irc_message_split_cb
 */

TEST(IrcInfo, InfoHashtableIrcMessageSplitCb)
{
    struct t_hashtable *hashtable, *result;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL, NULL);
    CHECK(hashtable);

    POINTERS_EQUAL(NULL, hook_info_get_hashtable (NULL, "irc_message_split", NULL));
    POINTERS_EQUAL(NULL, hook_info_get_hashtable (NULL, "irc_message_split", hashtable));

    hashtable_set (hashtable, "message",
                   "@tag1=value1 :alice!user@host PRIVMSG #channel :this is a test");

    result = hook_info_get_hashtable (NULL, "irc_message_split", hashtable);
    CHECK(result);
    STRCMP_EQUAL("@tag1=value1 :alice!user@host PRIVMSG #channel :this is a test",
                 (const char *)hashtable_get (result, "msg1"));
    STRCMP_EQUAL("this is a test", (const char *)hashtable_get (result, "args1"));
    hashtable_free (result);

    hashtable_free (hashtable);

}

/*
 * Tests functions:
 *   irc_info_infolist_irc_server_cb
 */

TEST(IrcInfo, InfolistIrcServerCb)
{
    struct t_irc_server *server1, *server2;
    struct t_infolist *infolist;

    server1 = irc_server_alloc ("server1");
    CHECK(server1);

    server2 = irc_server_alloc ("server2");
    CHECK(server2);

    infolist = hook_infolist_get (NULL, "irc_server", NULL, NULL);
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("server1", infolist_string (infolist, "name"));

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("server2", infolist_string (infolist, "name"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    infolist = hook_infolist_get (NULL, "irc_server", server2, NULL);
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("server2", infolist_string (infolist, "name"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    irc_server_free (server2);
    irc_server_free (server1);
}

/*
 * Tests functions:
 *   irc_info_infolist_irc_channel_cb
 */

TEST(IrcInfo, InfolistIrcChannelCb)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel1, *channel2;
    struct t_infolist *infolist;

    server = irc_server_alloc ("server");
    CHECK(server);

    channel1 = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test1", 0, 0);
    CHECK(channel1);

    channel2 = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test2", 0, 0);
    CHECK(channel2);

    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_channel", NULL, NULL));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_channel", NULL, ""));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_channel", NULL, "xxx"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_channel", NULL, "xxx,yyy"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_channel", NULL, "server,xxx"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_channel", (void *)0x1, "server"));

    infolist = hook_infolist_get (NULL, "irc_channel", NULL, "server");
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("#test1", infolist_string (infolist, "name"));

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("#test2", infolist_string (infolist, "name"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    infolist = hook_infolist_get (NULL, "irc_channel", NULL, "server,#test2");
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("#test2", infolist_string (infolist, "name"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    if (channel2->buffer)
        gui_buffer_close (channel2->buffer);
    if (channel1->buffer)
        gui_buffer_close (channel1->buffer);
    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_infolist_irc_modelist_cb
 *   irc_info_infolist_irc_modelist_item_cb
 */

TEST(IrcInfo, InfolistIrcModelistCb)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_infolist *infolist;

    server = irc_server_alloc ("server");
    CHECK(server);

    if (server->chanmodes)
        free (server->chanmodes);
    server->chanmodes = strdup ("Ibe,k");

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist", NULL, NULL));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist", NULL, ""));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist", NULL, "xxx"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist", NULL, "server"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist", NULL, "xxx,yyy"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist", NULL, "server,xxx"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist", (void *)0x1, "server,#test"));

    infolist = hook_infolist_get (NULL, "irc_modelist", NULL, "server,#test");
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("I", infolist_string (infolist, "type"));
    LONGS_EQUAL(0, infolist_integer (infolist, "state"));

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("b", infolist_string (infolist, "type"));
    LONGS_EQUAL(0, infolist_integer (infolist, "state"));

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("e", infolist_string (infolist, "type"));
    LONGS_EQUAL(0, infolist_integer (infolist, "state"));

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("k", infolist_string (infolist, "type"));
    LONGS_EQUAL(0, infolist_integer (infolist, "state"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    infolist = hook_infolist_get (NULL, "irc_modelist", NULL, "server,#test,k");
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("k", infolist_string (infolist, "type"));
    LONGS_EQUAL(0, infolist_integer (infolist, "state"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist_item", NULL, NULL));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist_item", NULL, ""));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist_item", NULL, "xxx"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist_item", NULL, "server"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist_item", NULL, "server,xxx"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist_item", NULL, "server,#test"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_modelist_item", NULL, "server,#test,Z"));

    infolist = hook_infolist_get (NULL, "irc_modelist_item", NULL, "server,#test,I");
    CHECK(infolist);

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    if (channel->buffer)
        gui_buffer_close (channel->buffer);
    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_infolist_irc_nick_cb
 */

TEST(IrcInfo, InfolistIrcNickCb)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick;
    struct t_infolist *infolist;

    server = irc_server_alloc ("server");
    CHECK(server);

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    nick = irc_nick_new (server, channel, "alice", "user@host", "@", 1, /* away */
                         "account-alice", "realname-alice");
    CHECK(nick);

    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_nick", NULL, NULL));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_nick", NULL, ""));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_nick", NULL, "xxx"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_nick", NULL, "server"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_nick", NULL, "server,#xxx"));
    POINTERS_EQUAL(NULL, hook_infolist_get (NULL, "irc_nick", NULL, "xxx,#test"));

    infolist = hook_infolist_get (NULL, "irc_nick", NULL, "server,#test");
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("alice", infolist_string (infolist, "name"));
    STRCMP_EQUAL("user@host", infolist_string (infolist, "host"));
    STRCMP_EQUAL("@ ", infolist_string (infolist, "prefixes"));
    LONGS_EQUAL(1, infolist_integer (infolist, "away"));
    STRCMP_EQUAL("account-alice", infolist_string (infolist, "account"));
    STRCMP_EQUAL("realname-alice", infolist_string (infolist, "realname"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    infolist = hook_infolist_get (NULL, "irc_nick", NULL, "server,#test,alice");
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("alice", infolist_string (infolist, "name"));
    STRCMP_EQUAL("user@host", infolist_string (infolist, "host"));
    STRCMP_EQUAL("@ ", infolist_string (infolist, "prefixes"));
    LONGS_EQUAL(1, infolist_integer (infolist, "away"));
    STRCMP_EQUAL("account-alice", infolist_string (infolist, "account"));
    STRCMP_EQUAL("realname-alice", infolist_string (infolist, "realname"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    irc_nick_free (server, channel, nick);
    if (channel->buffer)
        gui_buffer_close (channel->buffer);
    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_infolist_irc_ignore_cb
 */

TEST(IrcInfo, InfolistIrcIgnoreCb)
{
    struct t_irc_ignore *ignore1, *ignore2;
    struct t_infolist *infolist;

    ignore1 = irc_ignore_new ("alice", "server", NULL);
    CHECK(ignore1);

    ignore2 = irc_ignore_new ("bob", "server", NULL);
    CHECK(ignore2);

    infolist = hook_infolist_get (NULL, "irc_ignore", NULL, NULL);
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("alice", infolist_string (infolist, "mask"));
    STRCMP_EQUAL("server", infolist_string (infolist, "server"));
    STRCMP_EQUAL("*", infolist_string (infolist, "channel"));

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("bob", infolist_string (infolist, "mask"));
    STRCMP_EQUAL("server", infolist_string (infolist, "server"));
    STRCMP_EQUAL("*", infolist_string (infolist, "channel"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    infolist = hook_infolist_get (NULL, "irc_ignore", ignore2, NULL);
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("bob", infolist_string (infolist, "mask"));
    STRCMP_EQUAL("server", infolist_string (infolist, "server"));
    STRCMP_EQUAL("*", infolist_string (infolist, "channel"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    irc_ignore_free (ignore2);
    irc_ignore_free (ignore1);
}

/*
 * Tests functions:
 *   irc_info_infolist_irc_notify_cb
 */

TEST(IrcInfo, InfolistIrcNotifyCb)
{
    struct t_irc_server *server;
    struct t_irc_notify *notify1, *notify2;
    struct t_infolist *infolist;

    server = irc_server_alloc ("server");
    CHECK(server);

    notify1 = irc_notify_new (server, "bob", 1);
    CHECK(notify1);

    notify2 = irc_notify_new (server, "carol", 1);
    CHECK(notify2);

    infolist = hook_infolist_get (NULL, "irc_notify", NULL, NULL);
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(server, infolist_pointer (infolist, "server"));
    STRCMP_EQUAL("server", infolist_string (infolist, "server_name"));
    STRCMP_EQUAL("bob", infolist_string (infolist, "nick"));
    LONGS_EQUAL(1, infolist_integer (infolist, "check_away"));
    LONGS_EQUAL(-1, infolist_integer (infolist, "is_on_server"));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "away_message"));

    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(server, infolist_pointer (infolist, "server"));
    STRCMP_EQUAL("server", infolist_string (infolist, "server_name"));
    STRCMP_EQUAL("carol", infolist_string (infolist, "nick"));
    LONGS_EQUAL(1, infolist_integer (infolist, "check_away"));
    LONGS_EQUAL(-1, infolist_integer (infolist, "is_on_server"));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "away_message"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    infolist = hook_infolist_get (NULL, "irc_notify", NULL, "serv*");
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(server, infolist_pointer (infolist, "server"));
    STRCMP_EQUAL("server", infolist_string (infolist, "server_name"));
    STRCMP_EQUAL("bob", infolist_string (infolist, "nick"));
    LONGS_EQUAL(1, infolist_integer (infolist, "check_away"));
    LONGS_EQUAL(-1, infolist_integer (infolist, "is_on_server"));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "away_message"));

    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(server, infolist_pointer (infolist, "server"));
    STRCMP_EQUAL("server", infolist_string (infolist, "server_name"));
    STRCMP_EQUAL("carol", infolist_string (infolist, "nick"));
    LONGS_EQUAL(1, infolist_integer (infolist, "check_away"));
    LONGS_EQUAL(-1, infolist_integer (infolist, "is_on_server"));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "away_message"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    infolist = hook_infolist_get (NULL, "irc_notify", NULL, "xxx*");
    CHECK(infolist);

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    infolist = hook_infolist_get (NULL, "irc_notify", notify2, NULL);
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(server, infolist_pointer (infolist, "server"));
    STRCMP_EQUAL("server", infolist_string (infolist, "server_name"));
    STRCMP_EQUAL("carol", infolist_string (infolist, "nick"));
    LONGS_EQUAL(1, infolist_integer (infolist, "check_away"));
    LONGS_EQUAL(-1, infolist_integer (infolist, "is_on_server"));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "away_message"));

    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);

    irc_notify_free (server, notify2, 1);
    irc_notify_free (server, notify1, 1);
    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_info_infolist_irc_color_weechat_cb
 */

TEST(IrcInfo, InfolistIrcColorWeechatCb)
{
    struct t_infolist *infolist;

    infolist = hook_infolist_get (NULL, "irc_color_weechat", NULL, NULL);
    CHECK(infolist);

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("00", infolist_string (infolist, "color_irc"));
    STRCMP_EQUAL("white", infolist_string (infolist, "color_weechat"));

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("01", infolist_string (infolist, "color_irc"));
    STRCMP_EQUAL("black", infolist_string (infolist, "color_weechat"));

    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("02", infolist_string (infolist, "color_irc"));
    STRCMP_EQUAL("blue", infolist_string (infolist, "color_weechat"));

    infolist_free (infolist);
}

/*
 * Tests functions:
 *   irc_info_init
 */

TEST(IrcInfo, Init)
{
    /* TODO: write tests */
}
