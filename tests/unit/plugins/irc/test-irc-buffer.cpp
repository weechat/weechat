/*
 * SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC buffer functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include "src/plugins/irc/irc-buffer.h"
}

#define CHANNEL_300 "#this_channel_name_has_300_chars_xxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx_END"

TEST_GROUP(IrcBuffer)
{
};

/*
 * Test functions:
 *   irc_buffer_get_server_and_channel
 */

TEST(IrcBuffer, GetServerAndChannel)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_buffer_build_name
 */

TEST(IrcBuffer, BuildName)
{
    char *str;

    WEE_TEST_STR("", irc_buffer_build_name (NULL, NULL));
    WEE_TEST_STR(".", irc_buffer_build_name ("", ""));

    /* Only server */
    WEE_TEST_STR("server.libera", irc_buffer_build_name ("libera", NULL));

    /* Only channel */
    WEE_TEST_STR("#chan1", irc_buffer_build_name (NULL, "#chan1"));

    /* Server and channel */
    WEE_TEST_STR("libera.#chan1", irc_buffer_build_name ("libera", "#chan1"));
    WEE_TEST_STR("libera." CHANNEL_300,
                 irc_buffer_build_name ("libera", CHANNEL_300));
}

/*
 * Test functions:
 *   irc_buffer_close_server_channels
 */

TEST(IrcBuffer, CloseServerChannels)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_buffer_search_server_lowest_number
 */

TEST(IrcBuffer, SearchServerLowestNumber)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_buffer_search_private_lowest_number
 */

TEST(IrcBuffer, SearchPrivateLowestNumber)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_buffer_move_near_server
 */

TEST(IrcBuffer, IrcBufferMoveNearServer)
{
    /* TODO: write tests */
}
