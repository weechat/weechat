/*
 * test-relay-remote.cpp - test relay remote functions
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

extern "C"
{
#include <stdio.h>
#include <string.h>
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-remote.h"

extern char *relay_remote_get_address (const char *url);
}

TEST_GROUP(RelayRemote)
{
};

/*
 * Tests functions:
 *   relay_remote_search_option
 */

TEST(RelayRemote, SearchOption)
{
    LONGS_EQUAL(-1, relay_remote_search_option (NULL));
    LONGS_EQUAL(-1, relay_remote_search_option (""));
    LONGS_EQUAL(-1, relay_remote_search_option ("zzz"));

    LONGS_EQUAL(0, relay_remote_search_option ("url"));
}

/*
 * Tests functions:
 *   relay_remote_valid
 */

TEST(RelayRemote, Valid)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_search
 */

TEST(RelayRemote, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_search_by_number
 */

TEST(RelayRemote, SearchByNumber)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_name_valid
 */

TEST(RelayRemote, NameValid)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_url_valid
 */

TEST(RelayRemote, UrlValid)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_send_signal
 */

TEST(RelayRemote, SendSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_get_address
 */

TEST(RelayRemote, GetAddress)
{
    POINTERS_EQUAL(NULL, relay_remote_get_address (NULL));
    POINTERS_EQUAL(NULL, relay_remote_get_address (""));
    POINTERS_EQUAL(NULL, relay_remote_get_address ("zzz"));

    STRCMP_EQUAL("", relay_remote_get_address ("http://"));
    STRCMP_EQUAL("", relay_remote_get_address ("https://"));

    STRCMP_EQUAL("localhost", relay_remote_get_address ("https://localhost"));
    STRCMP_EQUAL("example.com", relay_remote_get_address ("https://example.com"));
    STRCMP_EQUAL("example.com", relay_remote_get_address ("https://example.com:8000"));
    STRCMP_EQUAL("example.com", relay_remote_get_address ("https://example.com:8000/"));
    STRCMP_EQUAL("example.com", relay_remote_get_address ("https://example.com:8000/?option=1"));
    STRCMP_EQUAL("example.com", relay_remote_get_address ("https://example.com?option=1"));
}

/*
 * Tests functions:
 *   relay_remote_get_port
 */

TEST(RelayRemote, GetPort)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_alloc
 */

TEST(RelayRemote, Alloc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_find_pos
 */

TEST(RelayRemote, FindPos)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_add
 */

TEST(RelayRemote, Add)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_new_with_options
 */

TEST(RelayRemote, NewWithOptions)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_new
 */

TEST(RelayRemote, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_new_with_infolist
 */

TEST(RelayRemote, NewWithInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_set_status
 */

TEST(RelayRemote, SetStatus)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_connect
 */

TEST(RelayRemote, Connect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_rename
 */

TEST(RelayRemote, Rename)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_free
 */

TEST(RelayRemote, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_free_all
 */

TEST(RelayRemote, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_disconnect
 */

TEST(RelayRemote, Disconnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_disconnect_all
 */

TEST(RelayRemote, DisconnectAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_add_to_infolist
 */

TEST(RelayRemote, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_print_log
 */

TEST(RelayRemote, PrintLog)
{
    /* TODO: write tests */
}
