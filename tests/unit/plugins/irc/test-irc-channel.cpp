/*
 * SPDX-FileCopyrightText: 2019-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC channel functions */

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
 * Test functions:
 *   irc_channel_valid
 */

TEST(IrcChannel, Valid)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_channel_is_channel
 */

TEST(IrcChannel, IsChannel)
{
    struct t_irc_server *server;

    /* No server, default chantypes = "#&+!" */

    /* Empty channel */
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, NULL));
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, ""));

    /* Not a channel */
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, "abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, "/abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (NULL, ":abc"));

    /* Valid channel */
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "#abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "##abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "&abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (NULL, "&&abc"));

    /* Server with chantypes = "#" */
    server = irc_server_alloc ("my_ircd");
    CHECK(server);
    if (server->chantypes)
        free (server->chantypes);
    server->chantypes = strdup ("#");

    /* Empty channel */
    LONGS_EQUAL(0, irc_channel_is_channel (server, NULL));
    LONGS_EQUAL(0, irc_channel_is_channel (server, ""));

    /* Not a channel */
    LONGS_EQUAL(0, irc_channel_is_channel (server, "abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "/abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, ":abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "&abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "+abc"));
    LONGS_EQUAL(0, irc_channel_is_channel (server, "!abc"));

    /* Valid channel */
    LONGS_EQUAL(1, irc_channel_is_channel (server, "#abc"));
    LONGS_EQUAL(1, irc_channel_is_channel (server, "##abc"));

    irc_server_free (server);
}
