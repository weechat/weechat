/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay API protocol (general functions) */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include "src/core/core-infolist.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/api/relay-api.h"
}

TEST_GROUP(RelayApi)
{
};

/*
 * Test functions:
 *   relay_api_search_colors
 */

TEST(RelayApi, SearchColors)
{
    LONGS_EQUAL(-1, relay_api_search_colors (NULL));
    LONGS_EQUAL(-1, relay_api_search_colors (""));
    LONGS_EQUAL(-1, relay_api_search_colors ("xxx"));
    LONGS_EQUAL(-1, relay_api_search_colors ("ANSI"));
    LONGS_EQUAL(-1, relay_api_search_colors ("WEECHAT"));
    LONGS_EQUAL(-1, relay_api_search_colors ("STRIP"));

    LONGS_EQUAL(RELAY_API_COLORS_ANSI, relay_api_search_colors ("ansi"));
    LONGS_EQUAL(RELAY_API_COLORS_WEECHAT, relay_api_search_colors ("weechat"));
    LONGS_EQUAL(RELAY_API_COLORS_STRIP, relay_api_search_colors ("strip"));
}

/*
 * Test functions:
 *   relay_api_search_filtered
 */

TEST(RelayApi, SearchFiltered)
{
    LONGS_EQUAL(-1, relay_api_search_filtered (NULL));
    LONGS_EQUAL(-1, relay_api_search_filtered (""));
    LONGS_EQUAL(-1, relay_api_search_filtered ("xxx"));
    LONGS_EQUAL(-1, relay_api_search_filtered ("INCLUDE"));
    LONGS_EQUAL(-1, relay_api_search_filtered ("BETWEEN"));
    LONGS_EQUAL(-1, relay_api_search_filtered ("EXCLUDE"));

    LONGS_EQUAL(RELAY_API_FILTERED_INCLUDE, relay_api_search_filtered ("include"));
    LONGS_EQUAL(RELAY_API_FILTERED_BETWEEN, relay_api_search_filtered ("between"));
    LONGS_EQUAL(RELAY_API_FILTERED_EXCLUDE, relay_api_search_filtered ("exclude"));
}

/*
 * Test functions:
 *   relay_api_hook_signals
 */

TEST(RelayApi, HookSignals)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_unhook_signals
 */

TEST(RelayApi, UnhookSignals)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_recv_http
 */

TEST(RelayApi, RecvHttp)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_recv_json
 */

TEST(RelayApi, RecvJson)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_close_connection
 */

TEST(RelayApi, CloseConnection)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_alloc
 */

TEST(RelayApi, Alloc)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_alloc_with_infolist
 *   relay_api_add_to_infolist
 */

TEST(RelayApi, AllocWithInfolist)
{
    struct t_relay_client client, client2, *ptr_client, *ptr_client2;
    struct t_infolist *infolist;
    struct t_infolist_item *item;

    memset (&client, 0, sizeof (client));
    memset (&client2, 0, sizeof (client2));
    ptr_client = &client;
    ptr_client2 = &client2;

    relay_api_alloc (&client);
    CHECK(client.protocol_data);
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_client, sync_filtered_lines));

    /* Save and restore sync data. */
    RELAY_API_DATA(ptr_client, sync_enabled) = 1;
    RELAY_API_DATA(ptr_client, sync_notify_only) = 1;
    RELAY_API_DATA(ptr_client, sync_filtered_lines) = 0;
    infolist = infolist_new (NULL);
    CHECK(infolist);
    item = infolist_new_item (infolist);
    CHECK(item);
    LONGS_EQUAL(1, relay_api_add_to_infolist (item, &client, 0));
    CHECK(infolist_next (infolist));
    relay_api_alloc_with_infolist (&client2, infolist);
    CHECK(client2.protocol_data);
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_client2, sync_enabled));
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_client2, sync_notify_only));
    LONGS_EQUAL(0, RELAY_API_DATA(ptr_client2, sync_filtered_lines));
    relay_api_free (&client2);
    infolist_free (infolist);

    /* Restore data saved by an older version (without "sync_filtered_lines"). */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    item = infolist_new_item (infolist);
    CHECK(item);
    CHECK(infolist_new_var_integer (item, "sync_enabled", 1));
    CHECK(infolist_next (infolist));
    relay_api_alloc_with_infolist (&client2, infolist);
    CHECK(client2.protocol_data);
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_client2, sync_enabled));
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_client2, sync_filtered_lines));
    relay_api_free (&client2);
    infolist_free (infolist);

    relay_api_free (&client);
}

/*
 * Test functions:
 *   relay_api_get_initial_status
 */

TEST(RelayApi, GetInitialStatus)
{
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, relay_api_get_initial_status (NULL));
}

/*
 * Test functions:
 *   relay_api_free
 */

TEST(RelayApi, Free)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_add_to_infolist
 */

TEST(RelayApi, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_print_log
 */

TEST(RelayApi, PrintLog)
{
    /* TODO: write tests */
}
