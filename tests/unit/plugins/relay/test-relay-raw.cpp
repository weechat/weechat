/*
 * test-relay-raw.cpp - test raw messages functions
 *
 * Copyright (C) 2024-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-raw.h"
}


TEST_GROUP(RelayRaw)
{
};

/*
 * Tests functions:
 *   relay_raw_message_print
 */

TEST(RelayRaw, MessagePrint)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_open
 */

TEST(RelayRaw, Open)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_message_free
 */

TEST(RelayRaw, MessageFree)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_message_free_all
 */

TEST(RelayRaw, MessageFreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_message_remove_old
 */

TEST(RelayRaw, MessageRemoveOld)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_message_add_to_list
 */

TEST(RelayRaw, MessageAddToList)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_convert_binary_message
 */

TEST(RelayRaw, ConvertBinaryMessage)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_convert_text_message
 */

TEST(RelayRaw, ConvertTextMessage)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_message_add
 */

TEST(RelayRaw, MessageAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_print_client
 */

TEST(RelayRaw, PrintClient)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_print_remote
 */

TEST(RelayRaw, PrintRemote)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_raw_add_to_infolist
 */

TEST(RelayRaw, AddToInfolist)
{
    /* TODO: write tests */
}
