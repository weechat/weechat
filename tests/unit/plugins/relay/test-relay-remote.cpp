/*
 * test-relay-remote.cpp - test relay remote functions
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
#include <stdio.h>
#include <string.h>
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-remote.h"

extern int relay_remote_parse_url (const char *url,
                                   int *tls, char **address, int *port);
}

#define WEE_CHECK_PARSE_URL(__result, __result_tls, __result_address,   \
                            __result_port, __url)                       \
    tls = -1;                                                           \
    address = NULL;                                                     \
    port = -1;                                                          \
    relay_remote_parse_url (__url, &tls, &address, &port);              \
    LONGS_EQUAL(__result_tls, tls);                                     \
    STRCMP_EQUAL(__result_address, address);                            \
    LONGS_EQUAL(__result_port, port);                                   \
    free (address);

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
 *   relay_remote_parse_url
 */

TEST(RelayRemote, ParseUrl)
{
    int tls, port;
    char *address;

    LONGS_EQUAL(0, relay_remote_parse_url (NULL, NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_remote_parse_url ("", NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_remote_parse_url ("zzz", NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_remote_parse_url ("http://[::1", NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_remote_parse_url ("https://[::1", NULL, NULL, NULL));

    LONGS_EQUAL(1, relay_remote_parse_url ("http://example.com", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com/", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com?option=1", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com/?option=1", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com:9876", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com:9876/", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com:9876?option=1", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com:9876/?option=1", NULL, NULL, NULL));

    WEE_CHECK_PARSE_URL(1, 0, "", RELAY_REMOTE_DEFAULT_PORT, "http://");
    WEE_CHECK_PARSE_URL(1, 1, "", RELAY_REMOTE_DEFAULT_PORT, "https://");

    WEE_CHECK_PARSE_URL(1, 0, "localhost", RELAY_REMOTE_DEFAULT_PORT, "http://localhost");
    WEE_CHECK_PARSE_URL(1, 1, "localhost", RELAY_REMOTE_DEFAULT_PORT, "https://localhost");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", RELAY_REMOTE_DEFAULT_PORT, "https://example.com");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", RELAY_REMOTE_DEFAULT_PORT, "https://example.com/");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", RELAY_REMOTE_DEFAULT_PORT, "https://example.com?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", RELAY_REMOTE_DEFAULT_PORT, "https://example.com/?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", 9876, "https://example.com:9876");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", 9876, "https://example.com:9876/");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", 9876, "https://example.com:9876?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", 9876, "https://example.com:9876/?option=1");

    WEE_CHECK_PARSE_URL(1, 0, "::1", RELAY_REMOTE_DEFAULT_PORT, "http://[::1]");
    WEE_CHECK_PARSE_URL(1, 1, "::1", RELAY_REMOTE_DEFAULT_PORT, "https://[::1]");
    WEE_CHECK_PARSE_URL(1, 1, "::1", RELAY_REMOTE_DEFAULT_PORT, "https://[::1]/");
    WEE_CHECK_PARSE_URL(1, 1, "::1", RELAY_REMOTE_DEFAULT_PORT, "https://[::1]?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "::1", RELAY_REMOTE_DEFAULT_PORT, "https://[::1]/?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "::1", 9876, "https://[::1]:9876");
    WEE_CHECK_PARSE_URL(1, 1, "::1", 9876, "https://[::1]:9876/");
    WEE_CHECK_PARSE_URL(1, 1, "::1", 9876, "https://[::1]:9876?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "::1", 9876, "https://[::1]:9876/?option=1");
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
 *   relay_remote_disconnect
 */

TEST(RelayRemote, Disconnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_reconnect_schedule
 */

TEST(RelayRemote, ReconnectSchedule)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_reconnect
 */

TEST(RelayRemote, Reconnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_timer
 */

TEST(RelayRemote, Timer)
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
