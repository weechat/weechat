/*
 * test-irc-join.cpp - test IRC join functions
 *
 * Copyright (C) 2022-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-secure.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-join.h"
#include "src/plugins/irc/irc-server.h"

extern int irc_join_compare_join_channel (struct t_irc_server *server,
                                          struct t_irc_join_channel *join_channel1,
                                          struct t_irc_join_channel *join_channel2);

}

#define WEE_CHECK_ADD_CHANNEL(__result, __join, __channel, __key)       \
    str = irc_join_add_channel (NULL, __join, __channel, __key);        \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, str);                                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, str);                                    \
    }                                                                   \
    if (str)                                                            \
        free (str);

#define WEE_CHECK_ADD_CHANNELS(__result, __join, __join2)               \
    str = irc_join_add_channels (NULL, __join, __join2);                \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, str);                                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, str);                                    \
    }                                                                   \
    if (str)                                                            \
        free (str);

#define WEE_CHECK_REMOVE_CHANNEL(__result, __join, __channel)           \
    str = irc_join_remove_channel (NULL, __join, __channel);            \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, str);                                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, str);                                    \
    }                                                                   \
    if (str)                                                            \
        free (str);

#define WEE_CHECK_RENAME_CHANNEL(__result, __join, __channel,           \
                                 __new_channel)                         \
    str = irc_join_rename_channel (NULL, __join, __channel,             \
                                   __new_channel);                      \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, str);                                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, str);                                    \
    }                                                                   \
    if (str)                                                            \
        free (str);

#define WEE_CHECK_SORT_CHANNELS(__result, __join)                       \
    str = irc_join_sort_channels (NULL, __join, IRC_JOIN_SORT_ALPHA);   \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, str);                                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, str);                                    \
    }                                                                   \
    if (str)                                                            \
        free (str);

TEST_GROUP(IrcJoin)
{
};

/*
 * Tests functions:
 *   irc_join_compare_join_channel
 */

TEST(IrcJoin, CompareJoinChannel)
{
    struct t_irc_join_channel join_chan1, join_chan2;

    join_chan1.name = NULL;
    join_chan1.key = NULL;
    join_chan2.name = NULL;
    join_chan2.key = NULL;

    LONGS_EQUAL(0, irc_join_compare_join_channel (NULL, &join_chan1, &join_chan2));

    join_chan1.name = strdup ("#abc");
    CHECK(irc_join_compare_join_channel (NULL, &join_chan1, &join_chan2) > 0);

    join_chan2.name = strdup ("#abc");
    LONGS_EQUAL(0, irc_join_compare_join_channel (NULL, &join_chan1, &join_chan2));

    join_chan1.key = strdup ("key_abc");
    CHECK(irc_join_compare_join_channel (NULL, &join_chan1, &join_chan2) < 0);

    join_chan2.key = strdup ("key_abc");
    LONGS_EQUAL(0, irc_join_compare_join_channel (NULL, &join_chan1, &join_chan2));

    free (join_chan2.key);
    join_chan2.key = strdup ("key2_abc");
    CHECK(irc_join_compare_join_channel (NULL, &join_chan1, &join_chan2) > 0);

    free (join_chan2.name);
    join_chan2.name = strdup ("#def");
    CHECK(irc_join_compare_join_channel (NULL, &join_chan1, &join_chan2) < 0);

    free (join_chan1.name);
    free (join_chan1.key);
    free (join_chan2.name);
    free (join_chan2.key);
}

/*
 * Tests functions:
 *   irc_join_compare_cb
 *   irc_join_free_join_channel
 *   irc_join_free_cb
 *   irc_join_split
 *   irc_join_build_string
 */

TEST(IrcJoin, SplitBuildString)
{
    struct t_arraylist *arraylist;
    struct t_irc_join_channel **channels;
    struct t_irc_server *server;
    char *autojoin;

    arraylist = irc_join_split (NULL, NULL, IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(0, arraylist->size);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("", autojoin);
    free (autojoin);
    arraylist_free (arraylist);

    autojoin = irc_join_build_string (NULL);
    STRCMP_EQUAL("", autojoin);
    free (autojoin);

    arraylist = irc_join_split (NULL, "", IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(0, arraylist->size);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("", autojoin);
    free (autojoin);
    arraylist_free (arraylist);

    /* 1 channel, 2 keys (the second is ignored) */
    arraylist = irc_join_split (NULL, "#xyz key_xyz,key_abc",
                                IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(1, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#xyz", channels[0]->name);
    STRCMP_EQUAL("key_xyz", channels[0]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#xyz key_xyz", autojoin);
    free (autojoin);
    arraylist_free (arraylist);

    /* 1 channel */
    arraylist = irc_join_split (NULL, "#xyz", IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(1, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#xyz", channels[0]->name);
    POINTERS_EQUAL(NULL, channels[0]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#xyz", autojoin);
    free (autojoin);
    arraylist_free (arraylist);

    /* 2 channels */
    arraylist = irc_join_split (NULL, "#xyz,#abc", IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(2, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#xyz", channels[0]->name);
    POINTERS_EQUAL(NULL, channels[0]->key);
    STRCMP_EQUAL("#abc", channels[1]->name);
    POINTERS_EQUAL(NULL, channels[1]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#xyz,#abc", autojoin);
    free (autojoin);
    arraylist_free (arraylist);

    /* 2 channels, 2 keys */
    arraylist = irc_join_split (NULL, "#xyz,#abc key_xyz,key_abc",
                                IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(2, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#xyz", channels[0]->name);
    STRCMP_EQUAL("key_xyz", channels[0]->key);
    STRCMP_EQUAL("#abc", channels[1]->name);
    STRCMP_EQUAL("key_abc", channels[1]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#xyz,#abc key_xyz,key_abc", autojoin);
    free (autojoin);
    arraylist_free (arraylist);

    /* 3 channels, 2 keys */
    arraylist = irc_join_split (NULL, "#xyz,#abc,#def key_xyz,key_abc",
                                IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(3, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#xyz", channels[0]->name);
    STRCMP_EQUAL("key_xyz", channels[0]->key);
    STRCMP_EQUAL("#abc", channels[1]->name);
    STRCMP_EQUAL("key_abc", channels[1]->key);
    STRCMP_EQUAL("#def", channels[2]->name);
    POINTERS_EQUAL(NULL, channels[2]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#xyz,#abc,#def key_xyz,key_abc", autojoin);
    free (autojoin);
    arraylist_free (arraylist);

    /* duplicated channel */
    arraylist = irc_join_split (NULL, "#xyz,#XYZ", IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(1, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#XYZ", channels[0]->name);
    POINTERS_EQUAL(NULL, channels[0]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#XYZ", autojoin);
    free (autojoin);
    arraylist_free (arraylist);

    /* duplicated channel */
    arraylist = irc_join_split (NULL, "#abc,#def,#abc", IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(2, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#abc", channels[0]->name);
    POINTERS_EQUAL(NULL, channels[0]->key);
    CHECK(channels[1]);
    STRCMP_EQUAL("#def", channels[1]->name);
    POINTERS_EQUAL(NULL, channels[1]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#abc,#def", autojoin);
    free (autojoin);
    arraylist_free (arraylist);

    /* server with casemapping RFC1459 */
    server = irc_server_alloc ("my_ircd");
    CHECK(server);
    server->casemapping = IRC_SERVER_CASEMAPPING_RFC1459;
    arraylist = irc_join_split (server, "#chan[a]^,#CHAN{A}~",
                                IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(1, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#CHAN{A}~", channels[0]->name);
    POINTERS_EQUAL(NULL, channels[0]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#CHAN{A}~", autojoin);
    free (autojoin);
    arraylist_free (arraylist);
    irc_server_free (server);

    /* server with casemapping STRICT_RFC1459 */
    server = irc_server_alloc ("my_ircd");
    CHECK(server);
    server->casemapping = IRC_SERVER_CASEMAPPING_STRICT_RFC1459;
    arraylist = irc_join_split (server, "#chan[a]^,#CHAN{A}~",
                                IRC_JOIN_SORT_DISABLED);
    CHECK(arraylist);
    LONGS_EQUAL(2, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#chan[a]^", channels[0]->name);
    POINTERS_EQUAL(NULL, channels[0]->key);
    STRCMP_EQUAL("#CHAN{A}~", channels[1]->name);
    POINTERS_EQUAL(NULL, channels[1]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#chan[a]^,#CHAN{A}~", autojoin);
    free (autojoin);
    arraylist_free (arraylist);
    irc_server_free (server);

    /* server with casemapping RFC1459, sort channels alphabetically */
    server = irc_server_alloc ("my_ircd");
    CHECK(server);
    server->casemapping = IRC_SERVER_CASEMAPPING_RFC1459;
    arraylist = irc_join_split (
        server, "#xyz,#def,#abc,#chan[a]^,#CHAN{A}~ key_xyz",
        IRC_JOIN_SORT_ALPHA);
    CHECK(arraylist);
    LONGS_EQUAL(4, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#xyz", channels[0]->name);
    STRCMP_EQUAL("key_xyz", channels[0]->key);
    STRCMP_EQUAL("#abc", channels[1]->name);
    POINTERS_EQUAL(NULL, channels[1]->key);
    STRCMP_EQUAL("#CHAN{A}~", channels[2]->name);
    POINTERS_EQUAL(NULL, channels[2]->key);
    STRCMP_EQUAL("#def", channels[3]->name);
    POINTERS_EQUAL(NULL, channels[3]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#xyz,#abc,#CHAN{A}~,#def key_xyz", autojoin);
    free (autojoin);
    arraylist_free (arraylist);
    irc_server_free (server);

    /* server with casemapping RFC1459, sort channels by buffer number */
    server = irc_server_alloc ("my_ircd");
    CHECK(server);
    irc_server_create_buffer (server);
    server->casemapping = IRC_SERVER_CASEMAPPING_RFC1459;
    irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#CHAN{A}~", 0, 0);
    irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#def", 0, 0);
    irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#abc", 0, 0);
    irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#xyz", 0, 0);
    arraylist = irc_join_split (
        server, "#xyz,#abc,#def,#chan[a]^,#zzz,#CHAN{A}~ key_xyz",
        IRC_JOIN_SORT_BUFFER);
    CHECK(arraylist);
    LONGS_EQUAL(5, arraylist->size);
    channels = (struct t_irc_join_channel **)arraylist->data;
    CHECK(channels[0]);
    STRCMP_EQUAL("#xyz", channels[0]->name);
    STRCMP_EQUAL("key_xyz", channels[0]->key);
    STRCMP_EQUAL("#CHAN{A}~", channels[1]->name);
    POINTERS_EQUAL(NULL, channels[1]->key);
    STRCMP_EQUAL("#def", channels[2]->name);
    POINTERS_EQUAL(NULL, channels[2]->key);
    STRCMP_EQUAL("#abc", channels[3]->name);
    POINTERS_EQUAL(NULL, channels[3]->key);
    STRCMP_EQUAL("#zzz", channels[4]->name);
    POINTERS_EQUAL(NULL, channels[4]->key);
    autojoin = irc_join_build_string (arraylist);
    STRCMP_EQUAL("#xyz,#CHAN{A}~,#def,#abc,#zzz key_xyz", autojoin);
    free (autojoin);
    arraylist_free (arraylist);
    gui_buffer_close (server->buffer);
    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_join_has_channel
 */

TEST(IrcJoin, HasChannel)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("my_ircd");
    CHECK(server);

    LONGS_EQUAL(0, irc_join_has_channel (NULL, NULL, NULL));
    LONGS_EQUAL(0, irc_join_has_channel (server, NULL, NULL));
    LONGS_EQUAL(0, irc_join_has_channel (server, NULL, ""));
    LONGS_EQUAL(0, irc_join_has_channel (server, "#abc,#def key_abc", NULL));
    LONGS_EQUAL(0, irc_join_has_channel (server, "#abc,#def key_abc", ""));
    LONGS_EQUAL(0, irc_join_has_channel (server, "#abc,#def key_abc", "#zzz"));

    LONGS_EQUAL(1, irc_join_has_channel (NULL, "#abc,#def key_abc", "#abc"));
    LONGS_EQUAL(1, irc_join_has_channel (NULL, "#abc,#def key_abc", "#ABC"));
    LONGS_EQUAL(1, irc_join_has_channel (NULL, "#abc,#def key_abc", "#def"));
    LONGS_EQUAL(1, irc_join_has_channel (NULL, "#abc,#def key_abc", "#DEF"));

    LONGS_EQUAL(1, irc_join_has_channel (server, "#abc,#def key_abc", "#abc"));
    LONGS_EQUAL(1, irc_join_has_channel (server, "#abc,#def key_abc", "#ABC"));
    LONGS_EQUAL(1, irc_join_has_channel (server, "#abc,#def key_abc", "#def"));
    LONGS_EQUAL(1, irc_join_has_channel (server, "#abc,#def key_abc", "#DEF"));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_join_add_channel
 */

TEST(IrcJoin, AddChannel)
{
    char *str;

    WEE_CHECK_ADD_CHANNEL(NULL, NULL, NULL, NULL);
    WEE_CHECK_ADD_CHANNEL(NULL, "", NULL, NULL);
    WEE_CHECK_ADD_CHANNEL("", "", "", NULL);
    WEE_CHECK_ADD_CHANNEL("", NULL, "", NULL);

    WEE_CHECK_ADD_CHANNEL("#abc", NULL, "#abc", NULL);
    WEE_CHECK_ADD_CHANNEL("#abc", "", "#abc", NULL);
    WEE_CHECK_ADD_CHANNEL("#abc key_abc", NULL, "#abc", "key_abc");
    WEE_CHECK_ADD_CHANNEL("#ABC key_ABC", NULL, "#ABC", "key_ABC");

    WEE_CHECK_ADD_CHANNEL("#xyz,#abc", "#xyz", "#abc", NULL);
    WEE_CHECK_ADD_CHANNEL("#abc,#xyz key_abc", "#xyz", "#abc", "key_abc");

    WEE_CHECK_ADD_CHANNEL("#abc,#xyz,#def key_abc", "#xyz,#def", "#abc", "key_abc");

    /* duplicated channel */
    WEE_CHECK_ADD_CHANNEL("#abc,#def", "#abc,#def", "#abc", NULL);
    WEE_CHECK_ADD_CHANNEL("#ABC,#def", "#abc,#def", "#ABC", NULL);
    WEE_CHECK_ADD_CHANNEL("#abc,#def", "#abc,#def", "#def", NULL);
    WEE_CHECK_ADD_CHANNEL("#abc,#DEF", "#abc,#def", "#DEF", NULL);
}

/*
 * Tests functions:
 *   irc_join_add_channels
 */

TEST(IrcJoin, AddChannels)
{
    char *str;

    WEE_CHECK_ADD_CHANNELS("", NULL, NULL);
    WEE_CHECK_ADD_CHANNELS("", "", NULL);
    WEE_CHECK_ADD_CHANNELS("", "", "");
    WEE_CHECK_ADD_CHANNELS("", NULL, "");

    WEE_CHECK_ADD_CHANNELS("#abc", NULL, "#abc");
    WEE_CHECK_ADD_CHANNELS("#abc", "", "#abc");
    WEE_CHECK_ADD_CHANNELS("#abc key_abc", NULL, "#abc key_abc");
    WEE_CHECK_ADD_CHANNELS("#ABC key_ABC", NULL, "#ABC key_ABC");

    WEE_CHECK_ADD_CHANNELS("#xyz,#abc", "#xyz", "#abc");
    WEE_CHECK_ADD_CHANNELS("#abc,#xyz key_abc", "#xyz", "#abc key_abc");

    WEE_CHECK_ADD_CHANNELS("#abc,#xyz,#def key_abc", "#xyz,#def", "#abc key_abc");

    WEE_CHECK_ADD_CHANNELS("#abc,#chan1,#chan2,#xyz,#chan3 key_abc,key1,key2",
                           "#abc,#xyz,#chan2 key_abc", "#chan1,#chan2,#chan3 key1,key2");

    /* duplicated channel */
    WEE_CHECK_ADD_CHANNELS("#abc,#def", "#abc,#def", "#abc");
    WEE_CHECK_ADD_CHANNELS("#ABC,#def", "#abc,#def", "#ABC");
    WEE_CHECK_ADD_CHANNELS("#abc,#def", "#abc,#def", "#def");
    WEE_CHECK_ADD_CHANNELS("#abc,#DEF", "#abc,#def", "#DEF");
}

/*
 * Tests functions:
 *   irc_join_remove_channel
 */

TEST(IrcJoin, RemoveChannel)
{
    char *str;

    WEE_CHECK_REMOVE_CHANNEL(NULL, NULL, NULL);
    WEE_CHECK_REMOVE_CHANNEL(NULL, "", NULL);
    WEE_CHECK_REMOVE_CHANNEL("", "", "");
    WEE_CHECK_REMOVE_CHANNEL("", NULL, "");

    WEE_CHECK_REMOVE_CHANNEL("", NULL, "#abc");
    WEE_CHECK_REMOVE_CHANNEL("", "", "#abc");

    WEE_CHECK_REMOVE_CHANNEL("#xyz", "#xyz", "#abc");
    WEE_CHECK_REMOVE_CHANNEL("", "#xyz", "#xyz");
    WEE_CHECK_REMOVE_CHANNEL("", "#xyz", "#XYZ");
    WEE_CHECK_REMOVE_CHANNEL("#xyz", "#abc,#xyz key_abc", "#abc");
    WEE_CHECK_REMOVE_CHANNEL("#abc key_abc", "#abc,#xyz key_abc", "#xyz");
    WEE_CHECK_REMOVE_CHANNEL("#abc key_abc", "#abc,#xyz key_abc", "#XYZ");

    WEE_CHECK_REMOVE_CHANNEL("#def,#ghi key_def",
                             "#abc,#def,#ghi key_abc,key_def", "#abc");
    WEE_CHECK_REMOVE_CHANNEL("#def,#ghi key_def",
                             "#abc,#def,#ghi key_abc,key_def", "#ABC");

    WEE_CHECK_REMOVE_CHANNEL("#abc,#ghi key_abc",
                             "#abc,#def,#ghi key_abc,key_def", "#def");

    WEE_CHECK_REMOVE_CHANNEL("#abc,#def key_abc,key_def",
                             "#abc,#def,#ghi key_abc,key_def", "#ghi");
}

/*
 * Tests functions:
 *   irc_join_rename_channel
 */

TEST(IrcJoin, RenameChannel)
{
    char *str;

    WEE_CHECK_RENAME_CHANNEL(NULL, NULL, NULL, NULL);
    WEE_CHECK_RENAME_CHANNEL(NULL, "", NULL, NULL);
    WEE_CHECK_RENAME_CHANNEL(NULL, "", NULL, NULL);
    WEE_CHECK_RENAME_CHANNEL(NULL, "", NULL, "");
    WEE_CHECK_RENAME_CHANNEL(NULL, "", "", NULL);
    WEE_CHECK_RENAME_CHANNEL(NULL, NULL, "", "");
    WEE_CHECK_RENAME_CHANNEL("", NULL, "", "#xyz");
    WEE_CHECK_RENAME_CHANNEL("", NULL, "xyz","");

    WEE_CHECK_RENAME_CHANNEL("", NULL, "#abc", "#xyz");
    WEE_CHECK_RENAME_CHANNEL("", "", "#abc", "#xyz");

    WEE_CHECK_RENAME_CHANNEL("#abc", "#abc", "#xyz", "#xyz");
    WEE_CHECK_RENAME_CHANNEL("#xyz", "#abc", "#abc", "#xyz");
    WEE_CHECK_RENAME_CHANNEL("#xyz,#def", "#abc,#def", "#abc", "#xyz");

    WEE_CHECK_RENAME_CHANNEL("#xyz,#def key_abc",
                             "#abc,#def key_abc", "#abc", "#xyz");
    WEE_CHECK_RENAME_CHANNEL("#xyz,#def key_abc,key_def",
                             "#abc,#def key_abc,key_def", "#abc", "#xyz");

    /* channel not found */
    WEE_CHECK_RENAME_CHANNEL("#abc,#def key_abc,key_def",
                             "#abc,#def key_abc,key_def", "#xxx", "#yyy");

    /* same name for the new channel */
    WEE_CHECK_RENAME_CHANNEL("#abc,#def key_abc,key_def",
                             "#abc,#def key_abc,key_def", "#abc", "#abc");

    /* new name already exists */
    WEE_CHECK_RENAME_CHANNEL("#def key_def",
                             "#abc,#def key_abc,key_def", "#abc", "#def");
}

/*
 * Tests functions:
 *   irc_join_sort_channels
 */

TEST(IrcJoin, SortChannels)
{
    char *str;

    WEE_CHECK_SORT_CHANNELS("", NULL);
    WEE_CHECK_SORT_CHANNELS("", "");

    WEE_CHECK_SORT_CHANNELS("#abc", "#abc");
    WEE_CHECK_SORT_CHANNELS("#ABC,#def,#GHI", "#GHI,#def,#ABC");
    WEE_CHECK_SORT_CHANNELS("#xyz,#abc key_xyz", "#xyz,#abc key_xyz");
    WEE_CHECK_SORT_CHANNELS("#xyz,#zzz,#ABC,#def,#ghi key_xyz,key_zzz",
                            "#zzz,#xyz,#ghi,#def,#ABC key_zzz,key_xyz");

    /* duplicated channel */
    WEE_CHECK_SORT_CHANNELS("#abc,#def", "#abc,#def,#abc");
    WEE_CHECK_SORT_CHANNELS("#abc,#DEF", "#abc,#def,#abc,#def,#DEF");
}

/*
 * Tests functions:
 *   irc_join_set_autojoin_option
 *   irc_join_add_channel_to_autojoin
 *   irc_join_add_channels_to_autojoin
 *   irc_join_remove_channel_from_autojoin
 *   irc_join_rename_channel_in_autojoin
 */

TEST(IrcJoin, AddRemoveChannelsAutojoin)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("my_ircd");
    CHECK(server);

    irc_join_remove_channel_from_autojoin (server, "#xyz");
    STRCMP_EQUAL(
        "",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_add_channel_to_autojoin (server, "#xyz", NULL);
    STRCMP_EQUAL(
        "#xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_add_channel_to_autojoin (server, NULL, NULL);
    STRCMP_EQUAL(
        "#xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_add_channel_to_autojoin (server, "#abc", "key_abc");
    STRCMP_EQUAL(
        "#abc,#xyz key_abc",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_add_channel_to_autojoin (server, "#def", "key_def");
    STRCMP_EQUAL(
        "#abc,#def,#xyz key_abc,key_def",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_add_channel_to_autojoin (server, "#ghi", NULL);
    STRCMP_EQUAL(
        "#abc,#def,#xyz,#ghi key_abc,key_def",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_add_channel_to_autojoin (server, "#jkl", "");
    STRCMP_EQUAL(
        "#abc,#def,#xyz,#ghi,#jkl key_abc,key_def",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_remove_channel_from_autojoin (server, "#def");
    STRCMP_EQUAL(
        "#abc,#xyz,#ghi,#jkl key_abc",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_remove_channel_from_autojoin (server, "#ghi");
    STRCMP_EQUAL(
        "#abc,#xyz,#jkl key_abc",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_remove_channel_from_autojoin (server, "#abc");
    STRCMP_EQUAL(
        "#xyz,#jkl",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_remove_channel_from_autojoin (server, "#jkl");
    STRCMP_EQUAL(
        "#xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_remove_channel_from_autojoin (server, "#xyz");
    STRCMP_EQUAL(
        "",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_remove_channel_from_autojoin (server, NULL);
    STRCMP_EQUAL(
        "",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_add_channels_to_autojoin (server, "#abc,#def key_abc");
    STRCMP_EQUAL(
        "#abc,#def key_abc",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_add_channels_to_autojoin (server, "#xyz,#ghi key_xyz");
    STRCMP_EQUAL(
        "#abc,#xyz,#def,#ghi key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* duplicated channel */
    irc_join_add_channels_to_autojoin (server, "#abc key_abc");
    STRCMP_EQUAL(
        "#abc,#xyz,#def,#ghi key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* duplicated channel */
    irc_join_add_channels_to_autojoin (server, "#xyz key_xyz");
    STRCMP_EQUAL(
        "#abc,#xyz,#def,#ghi key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* duplicated channel */
    irc_join_add_channels_to_autojoin (server, "#def");
    STRCMP_EQUAL(
        "#abc,#xyz,#def,#ghi key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* duplicated channel */
    irc_join_add_channels_to_autojoin (server, "#DEF");
    STRCMP_EQUAL(
        "#abc,#xyz,#DEF,#ghi key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* duplicated channel */
    irc_join_add_channels_to_autojoin (server, "#ghi");
    STRCMP_EQUAL(
        "#abc,#xyz,#DEF,#ghi key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* duplicated channel */
    irc_join_add_channels_to_autojoin (server, "#GHI");
    STRCMP_EQUAL(
        "#abc,#xyz,#DEF,#GHI key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* rename channel (not found) */
    irc_join_rename_channel_in_autojoin (server, "#yyy", "#zzz");
    STRCMP_EQUAL(
        "#abc,#xyz,#DEF,#GHI key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* rename channel */
    irc_join_rename_channel_in_autojoin (server, "#abc", "#aabbcc");
    STRCMP_EQUAL(
        "#aabbcc,#xyz,#DEF,#GHI key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* rename channel */
    irc_join_rename_channel_in_autojoin (server, "#aabbcc", "#abc");
    STRCMP_EQUAL(
        "#abc,#xyz,#DEF,#GHI key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* rename channel */
    irc_join_rename_channel_in_autojoin (server, "#DEF", "#def");
    STRCMP_EQUAL(
        "#abc,#xyz,#DEF,#GHI key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* rename channel (new channel already exists) */
    irc_join_rename_channel_in_autojoin (server, "#GHI", "#jkl");
    STRCMP_EQUAL(
        "#abc,#xyz,#DEF,#jkl key_abc,key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* rename channel (new channel already exists) */
    irc_join_rename_channel_in_autojoin (server, "#abc", "#def");
    STRCMP_EQUAL(
        "#xyz,#DEF,#jkl key_xyz",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* use of secure data in autojoin option */
    hashtable_set (secure_hashtable_data, "autojoin", "#abc");
    config_file_option_set (server->options[IRC_SERVER_OPTION_AUTOJOIN],
                            "${sec.data.autojoin}", 1);
    irc_join_add_channels_to_autojoin (server, "#def key_def");
    STRCMP_EQUAL(
        "${sec.data.autojoin}",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    STRCMP_EQUAL(
        "#def,#abc key_def",
        (const char *)hashtable_get (secure_hashtable_data, "autojoin"));
    irc_join_rename_channel_in_autojoin (server, "#abc", "#zzz");
    STRCMP_EQUAL(
        "${sec.data.autojoin}",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    STRCMP_EQUAL(
        "#def,#zzz key_def",
        (const char *)hashtable_get (secure_hashtable_data, "autojoin"));
    irc_join_remove_channel_from_autojoin (server, "#def");
    STRCMP_EQUAL(
        "${sec.data.autojoin}",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    STRCMP_EQUAL(
        "#zzz",
        (const char *)hashtable_get (secure_hashtable_data, "autojoin"));
    hashtable_remove (secure_hashtable_data, "autojoin");

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_join_save_channels_to_autojoin
 */

TEST(IrcJoin, SaveChannelsToAutojoin)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel1, *channel2;

    server = irc_server_alloc ("my_ircd");
    CHECK(server);

    channel1 = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL,
                                "#test1", 0, 0);
    channel2 = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL,
                                "#test2", 0, 0);
    channel2->key = strdup ("key2");

    irc_join_save_channels_to_autojoin (server);
    STRCMP_EQUAL(
        "#test2,#test1 key2",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* use of secure data in autojoin option */
    hashtable_set (secure_hashtable_data, "autojoin", "#abc");
    config_file_option_set (server->options[IRC_SERVER_OPTION_AUTOJOIN],
                            "${sec.data.autojoin}", 1);
    irc_join_save_channels_to_autojoin (server);
    STRCMP_EQUAL(
        "${sec.data.autojoin}",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    STRCMP_EQUAL(
        "#test2,#test1 key2",
        (const char *)hashtable_get (secure_hashtable_data, "autojoin"));
    hashtable_remove (secure_hashtable_data, "autojoin");

    gui_buffer_close (channel1->buffer);
    gui_buffer_close (channel2->buffer);

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_join_sort_autojoin_channels
 */

TEST(IrcJoin, SortAutojoinChannels)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("my_ircd");
    CHECK(server);

    irc_join_add_channel_to_autojoin (server, "#zzz", "key_zzz");
    irc_join_add_channel_to_autojoin (server, "#xyz", "key_xyz");
    irc_join_add_channel_to_autojoin (server, "#ghi", NULL);
    irc_join_add_channel_to_autojoin (server, "#def", NULL);
    irc_join_add_channel_to_autojoin (server, "#ABC", NULL);
    STRCMP_EQUAL("#zzz,#xyz,#ghi,#def,#ABC key_zzz,key_xyz",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    irc_join_sort_autojoin (server, IRC_JOIN_SORT_ALPHA);
    STRCMP_EQUAL("#xyz,#zzz,#ABC,#def,#ghi key_xyz,key_zzz",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));

    /* use of secure data in autojoin option */
    hashtable_set (secure_hashtable_data,
                   "autojoin", "#zzz,#xyz,#ghi,#def,#ABC key_zzz,key_xyz");
    config_file_option_set (server->options[IRC_SERVER_OPTION_AUTOJOIN],
                            "${sec.data.autojoin}", 1);
    irc_join_sort_autojoin (server, IRC_JOIN_SORT_ALPHA);
    STRCMP_EQUAL(
        "${sec.data.autojoin}",
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    STRCMP_EQUAL(
        "#xyz,#zzz,#ABC,#def,#ghi key_xyz,key_zzz",
        (const char *)hashtable_get (secure_hashtable_data, "autojoin"));
    hashtable_remove (secure_hashtable_data, "autojoin");

    irc_server_free (server);
}
