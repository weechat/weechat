/*
 * test-irc-buffer.cpp - test IRC buffer functions
 *
 * Copyright (C) 2021-2023 Sébastien Helleu <flashcode@flashtux.org>
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
 * Tests functions:
 *   irc_buffer_get_server_and_channel
 */

TEST(IrcBuffer, GetServerAndChannel)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_buffer_build_name
 */

TEST(IrcBuffer, BuildName)
{
    char *str;

    WEE_TEST_STR("", irc_buffer_build_name (NULL, NULL));
    WEE_TEST_STR(".", irc_buffer_build_name ("", ""));

    /* only server */
    WEE_TEST_STR("server.libera", irc_buffer_build_name ("libera", NULL));

    /* only channel */
    WEE_TEST_STR("#chan1", irc_buffer_build_name (NULL, "#chan1"));

    /* server and channel */
    WEE_TEST_STR("libera.#chan1", irc_buffer_build_name ("libera", "#chan1"));
    WEE_TEST_STR("libera." CHANNEL_300,
                 irc_buffer_build_name ("libera", CHANNEL_300));
}

/*
 * Tests functions:
 *   irc_buffer_close_server_channels
 */

TEST(IrcBuffer, CloseServerChannels)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_buffer_search_server_lowest_number
 */

TEST(IrcBuffer, SearchServerLowestNumber)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_buffer_search_private_lowest_number
 */

TEST(IrcBuffer, SearchPrivateLowestNumber)
{
    /* TODO: write tests */
}
