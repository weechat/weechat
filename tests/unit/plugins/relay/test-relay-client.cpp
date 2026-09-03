/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay client functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-server.h"
#include "src/plugins/weechat-plugin.h"

extern void relay_client_recv_text (struct t_relay_client *client,
                                    const char *data);
extern int relay_client_outqueue_add (struct t_relay_client *client,
                                     const char *data, int data_size,
                                     enum t_relay_msg_type raw_msg_type[2],
                                     int raw_flags[2],
                                     const char *raw_message[2],
                                     int raw_size[2]);
extern void relay_client_outqueue_free_all (struct t_relay_client *client);
}

#define WEE_NEW_CHUNK(__size)                                           \
    free (chunk);                                                       \
    chunk = (char *)malloc (__size + 1);                                \
    CHECK(chunk);                                                       \
    memset (chunk, 'a', __size);                                        \
    chunk[__size] = '\0';

TEST_GROUP(RelayClient)
{
    struct t_relay_server *server;
    struct t_relay_client *client;
    char *chunk;

    void setup ()
    {
        /* Disable auto-open of relay buffer (it would pollute other tests). */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);

        server = relay_server_new ("weechat", RELAY_PROTOCOL_WEECHAT, NULL,
                                   9000,
                                   NULL,  /* path */
                                   1,  /* ipv4 */
                                   0,  /* ipv6 */
                                   0,  /* tls */
                                   0);  /* unix_socket */
        client = (server) ? relay_client_new (-1, "test", server) : NULL;
        chunk = NULL;
    }

    void teardown ()
    {
        free (chunk);
        chunk = NULL;

        relay_client_free (client);
        client = NULL;

        relay_server_free (server);
        server = NULL;

        /* Restore auto-open of relay buffer. */
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);
    }
};

/*
 * Test functions:
 *   relay_client_recv_text
 */

TEST(RelayClient, RecvText)
{
    CHECK(server);
    CHECK(client);
    LONGS_EQUAL(RELAY_CLIENT_DATA_TEXT_LINE, client->recv_data_type);

    /* Data without any end-of-line is kept as partial message. */
    relay_client_recv_text (client, "unknown1");
    STRCMP_EQUAL("unknown1", client->partial_message);

    /* Complete lines are consumed, the remaining data is kept. */
    relay_client_recv_text (client, " arg\nunknown2 arg\npartial");
    STRCMP_EQUAL("partial", client->partial_message);

    /* Data ending with end-of-line: nothing is kept. */
    relay_client_recv_text (client, " end\n");
    POINTERS_EQUAL(NULL, client->partial_message);
}

/*
 * Test functions:
 *   relay_client_recv_text (the partial message accumulated is bounded)
 *
 * Check that data received without any end-of-line does not grow the partial
 * message without limit.
 */

TEST(RelayClient, RecvTextLimit)
{
    size_t chunk_size, length;
    int i;

    CHECK(server);
    CHECK(client);

    chunk_size = 1024 * 1024;
    WEE_NEW_CHUNK(chunk_size);

    /* Feed 16 MB with no end-of-line. */
    for (i = 0; i < 16; i++)
    {
        relay_client_recv_text (client, chunk);
    }
    CHECK(client->partial_message);
    length = strlen (client->partial_message);

    /* The partial message must be bounded (not ~16 MB). */
    LONGS_EQUAL(RELAY_CLIENT_PARTIAL_MESSAGE_MAX_LENGTH, length);

    /* Feeding more data must not grow it any further. */
    for (i = 0; i < 16; i++)
    {
        relay_client_recv_text (client, chunk);
    }
    LONGS_EQUAL(length, strlen (client->partial_message));
}

/*
 * Test functions:
 *   relay_client_recv_text (data not fitting in the partial message is
 *   ignored)
 */

TEST(RelayClient, RecvTextLimitIgnoreData)
{
    size_t chunk_size;

    CHECK(server);
    CHECK(client);

    chunk_size = 3 * 1024 * 1024;
    WEE_NEW_CHUNK(chunk_size);

    /* 3 MB, then 6 MB: below the limit, data is accumulated. */
    relay_client_recv_text (client, chunk);
    LONGS_EQUAL(chunk_size, strlen (client->partial_message));
    relay_client_recv_text (client, chunk);
    LONGS_EQUAL(2 * chunk_size, strlen (client->partial_message));

    /* 9 MB would exceed the limit: all the data received is ignored. */
    relay_client_recv_text (client, chunk);
    LONGS_EQUAL(2 * chunk_size, strlen (client->partial_message));

    /* Smaller data still fitting in the partial message is accumulated. */
    chunk_size = 1024 * 1024;
    WEE_NEW_CHUNK(chunk_size);
    relay_client_recv_text (client, chunk);
    LONGS_EQUAL(7 * 1024 * 1024, strlen (client->partial_message));
}

/*
 * Test functions:
 *   relay_client_outqueue_add
 *   relay_client_outqueue_free_all
 */

TEST(RelayClient, OutqueueAdd)
{
    CHECK(server);
    CHECK(client);

    /* The out queue is empty on a new client. */
    POINTERS_EQUAL(NULL, client->outqueue);
    LONGS_EQUAL(0, client->outqueue_size);

    /* Nothing is queued with invalid arguments. */
    LONGS_EQUAL(0, relay_client_outqueue_add (NULL, "test", 4,
                                              NULL, NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_client_outqueue_add (client, NULL, 4,
                                              NULL, NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_client_outqueue_add (client, "test", 0,
                                              NULL, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, client->outqueue);
    LONGS_EQUAL(0, client->outqueue_size);

    /* The size of the messages queued is accumulated. */
    LONGS_EQUAL(1, relay_client_outqueue_add (client, "test", 4,
                                              NULL, NULL, NULL, NULL));
    CHECK(client->outqueue);
    LONGS_EQUAL(4, client->outqueue_size);
    LONGS_EQUAL(1, relay_client_outqueue_add (client, "message", 7,
                                              NULL, NULL, NULL, NULL));
    LONGS_EQUAL(11, client->outqueue_size);

    /* The size is back to 0 when the out queue is freed. */
    relay_client_outqueue_free_all (client);
    POINTERS_EQUAL(NULL, client->outqueue);
    LONGS_EQUAL(0, client->outqueue_size);
}

/*
 * Test functions:
 *   relay_client_outqueue_add (the data queued is bounded)
 *
 * Check that a client which does not read data from the socket is
 * disconnected once the data queued exceeds the limit.
 */

TEST(RelayClient, OutqueueAddLimit)
{
    int chunk_size, i;

    CHECK(server);
    CHECK(client);
    CHECK(!RELAY_STATUS_HAS_ENDED(client->status));

    chunk_size = 1024 * 1024;
    WEE_NEW_CHUNK(chunk_size);

    /* Queue 9 MB: the limit is not exceeded before the last message. */
    for (i = 0; i < 9; i++)
    {
        LONGS_EQUAL(1, relay_client_outqueue_add (client, chunk, chunk_size,
                                                  NULL, NULL, NULL, NULL));
    }
    LONGS_EQUAL(9 * 1024 * 1024, client->outqueue_size);
    CHECK(!RELAY_STATUS_HAS_ENDED(client->status));

    /*
     * The data queued now exceeds the limit: the next message is not queued,
     * the client is disconnected and its out queue is freed.
     */
    LONGS_EQUAL(0, relay_client_outqueue_add (client, "test", 4,
                                              NULL, NULL, NULL, NULL));
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client->status);
    POINTERS_EQUAL(NULL, client->outqueue);
    LONGS_EQUAL(0, client->outqueue_size);
}

/*
 * Test functions:
 *   relay_client_outqueue_add (a single large message is queued)
 *
 * Check that a message larger than the limit is queued, so that a client
 * reading data from the socket is not disconnected because of a single large
 * message.
 */

TEST(RelayClient, OutqueueAddLimitSingleMessage)
{
    int chunk_size;

    CHECK(server);
    CHECK(client);
    CHECK(!RELAY_STATUS_HAS_ENDED(client->status));

    chunk_size = 16 * 1024 * 1024;
    WEE_NEW_CHUNK(chunk_size);

    /* The message is queued although it is larger than the limit. */
    LONGS_EQUAL(1, relay_client_outqueue_add (client, chunk, chunk_size,
                                              NULL, NULL, NULL, NULL));
    LONGS_EQUAL(16 * 1024 * 1024, client->outqueue_size);
    CHECK(!RELAY_STATUS_HAS_ENDED(client->status));

    /* The next message is not queued and the client is disconnected. */
    LONGS_EQUAL(0, relay_client_outqueue_add (client, "test", 4,
                                              NULL, NULL, NULL, NULL));
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client->status);
    LONGS_EQUAL(0, client->outqueue_size);
}
