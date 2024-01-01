/*
 * test-irc-channel.cpp - test IRC channel functions
 *
 * Copyright (C) 2019-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-server.h"
}

TEST_GROUP(IrcChannel)
{
};

/*
 * Tests functions:
 *   irc_channel_valid
 */

TEST(IrcChannel, Valid)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_channel_is_channel
 */

TEST(IrcChannel, IsChannel)
{
    struct t_irc_server *server;

    /* no server, default chantypes = "#&+!" */

    /* empty channel */
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, NULL));
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, ""));

    /* not a channel */
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, "abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, "/abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, ":abc"));

    /* valid channel */
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "#abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "##abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "&abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "&&abc"));

    /* server with chantypes = "#" */
    server = irc_server_alloc ("my_ircd");
    CHECK(server);
    if (server->chantypes)
        free (server->chantypes);
    server->chantypes = strdup ("#");

    /* empty channel */
    LONGS_EQUAL(0, irc_channel_is_channel (server, NULL));
    LONGS_EQUAL(0, irc_channel_is_channel (server, ""));

    /* not a channel */
    LONGS_EQUAL(0, irc_channel_is_channel (server, "abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "/abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, ":abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "&abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "+abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "!abc"));

    /* valid channel */
    LONGS_EQUAL(1, irc_channel_is_channel (server, "#abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (server, "##abc"));

    irc_server_free (server);
}
