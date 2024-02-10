/*
 * test-relay-api.cpp - test relay API protocol (general functions)
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
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/api/relay-api.h"
}

TEST_GROUP(RelayApi)
{
};

/*
 * Tests functions:
 *   relay_api_search_colors
 */

TEST(RelayApi, SearchColors)
{
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, relay_api_search_colors (NULL));
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, relay_api_search_colors (""));
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, relay_api_search_colors ("xxx"));
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, relay_api_search_colors ("WEECHAT"));
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, relay_api_search_colors ("STRIP"));

    LONGS_EQUAL(RELAY_API_COLORS_WEECHAT, relay_api_search_colors ("weechat"));
    LONGS_EQUAL(RELAY_API_COLORS_STRIP, relay_api_search_colors ("strip"));
}

/*
 * Tests functions:
 *   relay_api_hook_signals
 */

TEST(RelayApi, HookSignals)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_unhook_signals
 */

TEST(RelayApi, UnhookSignals)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_recv_http
 */

TEST(RelayApi, RecvHttp)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_recv_json
 */

TEST(RelayApi, RecvJson)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_close_connection
 */

TEST(RelayApi, CloseConnection)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_alloc
 */

TEST(RelayApi, Alloc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_alloc_with_infolist
 */

TEST(RelayApi, AllocWithInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_get_initial_status
 */

TEST(RelayApi, GetInitialStatus)
{
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, relay_api_get_initial_status (NULL));
}

/*
 * Tests functions:
 *   relay_api_free
 */

TEST(RelayApi, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_add_to_infolist
 */

TEST(RelayApi, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_print_log
 */

TEST(RelayApi, PrintLog)
{
    /* TODO: write tests */
}
