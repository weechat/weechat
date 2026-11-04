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
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
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
extern void relay_client_set_desc (struct t_relay_client *client);
extern int relay_client_recv_cb (const void *pointer, void *data, int fd);
extern void relay_client_send_outqueue (struct t_relay_client *client);
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

int test_relay_client_signal_count = 0;
char test_relay_client_signal[128];

TEST_GROUP(RelayClientWithSocket)
{
    struct t_relay_server *server;
    struct t_relay_client *client;
    int sock_peer;
    struct t_hook *hook_signal_client;

    /*
     * Callback for signals "relay_client_*".
     */

    static int
    signal_cb (const void *pointer, void *data, const char *signal,
               const char *type_data, void *signal_data)
    {
        (void) pointer;
        (void) data;
        (void) type_data;
        (void) signal_data;

        test_relay_client_signal_count++;
        snprintf (test_relay_client_signal, sizeof (test_relay_client_signal),
                  "%s", signal);

        return WEECHAT_RC_OK;
    }

    /*
     * Create a client connected with a socket pair: the other end of the
     * socket (the remote client) is in "sock_peer".
     */

    struct t_relay_client *new_client ()
    {
        struct t_relay_client *new_client;
        int sv[2];

        LONGS_EQUAL(0, socketpair (AF_UNIX, SOCK_STREAM, 0, sv));
        fcntl (sv[0], F_SETFL, fcntl (sv[0], F_GETFL) | O_NONBLOCK);
        fcntl (sv[1], F_SETFL, fcntl (sv[1], F_GETFL) | O_NONBLOCK);
        new_client = relay_client_new (sv[0], "test", server);
        CHECK(new_client);
        if (sock_peer >= 0)
            close (sock_peer);
        sock_peer = sv[1];
        return new_client;
    }

    /*
     * Free a client, closing its socket.
     */

    void free_client (struct t_relay_client *ptr_client)
    {
        if (!ptr_client)
            return;
        if (ptr_client->sock >= 0)
            close (ptr_client->sock);
        relay_client_free (ptr_client);
    }

    /*
     * Send data from the remote client, then read it in the relay client.
     */

    void peer_send (const void *data, int size)
    {
        LONGS_EQUAL(size, send (sock_peer, data, size, 0));
        relay_client_recv_cb (client, NULL, client->sock);
    }

    /*
     * Read all data received by the remote client.
     *
     * Return number of bytes read.
     */

    int peer_recv (char *buffer, int size)
    {
        int num_read, total;

        total = 0;
        while (total < size)
        {
            num_read = recv (sock_peer, buffer + total, size - total, 0);
            if (num_read <= 0)
                break;
            total += num_read;
        }
        return total;
    }

    /*
     * Build a websocket frame sent by a client (masked, payload < 126 bytes).
     *
     * Return size of frame.
     */

    int ws_frame (unsigned char *frame, int opcode, const char *payload)
    {
        const unsigned char mask[4] = { 0x12, 0x34, 0x56, 0x78 };
        int i, length;

        length = strlen (payload);
        frame[0] = 0x80 | opcode;
        frame[1] = 0x80 | length;
        memcpy (frame + 2, mask, 4);
        for (i = 0; i < length; i++)
        {
            frame[6 + i] = payload[i] ^ mask[i % 4];
        }
        return 6 + length;
    }

    void setup ()
    {
        /* Disable auto-open of relay buffer (it would pollute other tests). */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);
        /* Disable compression, so that messages are sent as-is. */
        config_file_option_set (relay_config_network_compression, "0", 1);

        server = relay_server_new ("weechat", RELAY_PROTOCOL_WEECHAT, NULL,
                                   9000,
                                   NULL,  /* path */
                                   1,  /* ipv4 */
                                   0,  /* ipv6 */
                                   0,  /* tls */
                                   0);  /* unix_socket */
        CHECK(server);
        sock_peer = -1;
        client = new_client ();

        test_relay_client_signal_count = 0;
        test_relay_client_signal[0] = '\0';
        hook_signal_client = hook_signal (NULL, "relay_client_*", &signal_cb,
                                   NULL, NULL);
    }

    void teardown ()
    {
        unhook (hook_signal_client);
        free_client (client);
        client = NULL;
        if (sock_peer >= 0)
            close (sock_peer);
        relay_server_free (server);
        config_file_option_reset (relay_config_network_auth_timeout, 1);
        config_file_option_reset (relay_config_network_clients_purge_delay, 1);
        config_file_option_reset (relay_config_network_compression, 1);
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);
    }
};

/*
 * Test functions:
 *   relay_client_valid
 *   relay_client_search_by_number
 *   relay_client_search_by_id
 *   relay_client_count_active_by_port
 */

TEST(RelayClientWithSocket, Search)
{
    struct t_relay_client *client2, *client3, client_fake;

    client2 = relay_client_new (-1, "test2", server);
    CHECK(client2);
    client3 = relay_client_new (-1, "test3", server);
    CHECK(client3);

    /* New clients are added at the beginning of the list, with a new id. */
    LONGS_EQUAL(client->id + 1, client2->id);
    LONGS_EQUAL(client->id + 2, client3->id);

    LONGS_EQUAL(0, relay_client_valid (NULL));
    LONGS_EQUAL(0, relay_client_valid (&client_fake));
    LONGS_EQUAL(1, relay_client_valid (client));
    LONGS_EQUAL(1, relay_client_valid (client3));

    POINTERS_EQUAL(client3, relay_client_search_by_number (0));
    POINTERS_EQUAL(client2, relay_client_search_by_number (1));
    POINTERS_EQUAL(client, relay_client_search_by_number (2));
    POINTERS_EQUAL(NULL, relay_client_search_by_number (3));
    POINTERS_EQUAL(NULL, relay_client_search_by_number (-1));

    POINTERS_EQUAL(client, relay_client_search_by_id (client->id));
    POINTERS_EQUAL(client3, relay_client_search_by_id (client3->id));
    POINTERS_EQUAL(NULL, relay_client_search_by_id (client3->id + 1));
    POINTERS_EQUAL(NULL, relay_client_search_by_id (-1));

    /* Only clients not ended are counted. */
    LONGS_EQUAL(3, relay_client_count_active_by_port (9000));
    LONGS_EQUAL(0, relay_client_count_active_by_port (9001));
    relay_client_set_status (client2, RELAY_STATUS_DISCONNECTED);
    LONGS_EQUAL(2, relay_client_count_active_by_port (9000));
    relay_client_set_status (client3, RELAY_STATUS_AUTH_FAILED);
    LONGS_EQUAL(1, relay_client_count_active_by_port (9000));

    relay_client_free (client3);
    LONGS_EQUAL(0, relay_client_valid (client3));
    POINTERS_EQUAL(client2, relay_client_search_by_number (0));
    relay_client_free (client2);
    POINTERS_EQUAL(client, relay_client_search_by_number (0));
}

/*
 * Test functions:
 *   relay_client_set_desc
 */

TEST(RelayClientWithSocket, SetDesc)
{
    char expected[256];

    snprintf (expected, sizeof (expected), "%d/weechat/test", client->id);
    STRCMP_EQUAL(expected, client->desc);

    client->protocol_args = strdup ("args");
    client->real_ip = strdup ("192.168.1.2");
    client->tls = 1;
    relay_client_set_desc (client);
    snprintf (expected, sizeof (expected),
              "%d/tls.weechat.args/test(192.168.1.2)", client->id);
    STRCMP_EQUAL(expected, client->desc);
    client->tls = 0;
}

/*
 * Test functions:
 *   relay_client_recv_cb
 *   relay_client_recv_buffer
 */

TEST(RelayClientWithSocket, RecvCb)
{
    char buffer[4096];
    int sock;

    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client->status);

    /* Handshake: reply is sent to the client. */
    peer_send ("handshake\n", 10);
    LONGS_EQUAL(10, client->bytes_recv);
    CHECK(client->bytes_sent > 0);
    LONGS_EQUAL(client->bytes_sent, peer_recv (buffer, sizeof (buffer)));

    /* Authentication (no password). */
    peer_send ("init\n", 5);
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, client->status);
    STRCMP_EQUAL("relay_client_connected", test_relay_client_signal);

    /* Message received in two parts. */
    peer_send ("ping te", 7);
    STRCMP_EQUAL("ping te", client->partial_message);
    LONGS_EQUAL(0, peer_recv (buffer, sizeof (buffer)));
    peer_send ("st\r\n", 4);
    POINTERS_EQUAL(NULL, client->partial_message);
    CHECK(peer_recv (buffer, sizeof (buffer)) > 0);
    LONGS_EQUAL(26, client->bytes_recv);

    /* No data available: nothing done. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_client_recv_cb (client, NULL, client->sock));
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, client->status);

    /* Data is ignored if client is not authenticating or connected. */
    client->status = RELAY_STATUS_CONNECTING;
    peer_send ("ping\n", 5);
    LONGS_EQUAL(26, client->bytes_recv);
    client->status = RELAY_STATUS_CONNECTED;
    relay_client_recv_cb (client, NULL, client->sock);
    LONGS_EQUAL(31, client->bytes_recv);
    peer_recv (buffer, sizeof (buffer));

    /* Connection closed by the remote client: client is disconnected. */
    close (sock_peer);
    sock_peer = -1;
    sock = client->sock;
    relay_client_recv_cb (client, NULL, sock);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client->status);
    LONGS_EQUAL(-1, client->sock);
    POINTERS_EQUAL(NULL, client->hook_fd);
    STRCMP_EQUAL("relay_client_disconnected", test_relay_client_signal);

    /* Socket closed: nothing done. */
    LONGS_EQUAL(WEECHAT_RC_OK, relay_client_recv_cb (client, NULL, sock));
}

/*
 * Test functions:
 *   relay_client_recv_buffer (detection of websocket)
 */

TEST(RelayClientWithSocket, RecvBufferWebsocketDetection)
{
    const char *http_get = "GET /weechat HTTP/1.1\r\n";

    /* HTTP GET as first message: websocket is initializing. */
    relay_client_recv_buffer (client, http_get, strlen (http_get));
    LONGS_EQUAL(RELAY_CLIENT_WEBSOCKET_INITIALIZING, client->websocket);

    /* HTTP GET after another message: not a websocket. */
    free_client (client);
    client = new_client ();
    relay_client_recv_buffer (client, "test\n", 5);
    relay_client_recv_buffer (client, http_get, strlen (http_get));
    LONGS_EQUAL(RELAY_CLIENT_WEBSOCKET_NOT_USED, client->websocket);

    /* Invalid HTTP GET: not a websocket. */
    free_client (client);
    client = new_client ();
    relay_client_recv_buffer (client, "GET /api HTTP/1.1\r\n", 19);
    LONGS_EQUAL(RELAY_CLIENT_WEBSOCKET_NOT_USED, client->websocket);
}

/*
 * Test functions:
 *   relay_client_recv_buffer (websocket frames)
 *   relay_client_read_websocket_frames
 */

TEST(RelayClientWithSocket, RecvBufferWebsocketFrames)
{
    unsigned char frame[256], frame2[512];
    char buffer[4096];
    int size, size2;

    client->websocket = RELAY_CLIENT_WEBSOCKET_READY;
    client->bytes_recv = 1;

    /* Text frame: command is executed, reply is a binary frame. */
    size = ws_frame (frame, 0x1, "handshake\n");
    relay_client_recv_buffer (client, (char *)frame, size);
    size = peer_recv (buffer, sizeof (buffer));
    CHECK(size > 2);
    BYTES_EQUAL(0x82, buffer[0]);

    /* Empty frame: ignored. */
    size = ws_frame (frame, 0x1, "");
    relay_client_recv_buffer (client, (char *)frame, size);
    LONGS_EQUAL(0, peer_recv (buffer, sizeof (buffer)));

    /* Ping frame: answer with a pong frame. */
    size = ws_frame (frame, 0x9, "abc");
    relay_client_recv_buffer (client, (char *)frame, size);
    LONGS_EQUAL(5, peer_recv (buffer, sizeof (buffer)));
    MEMCMP_EQUAL("\x8a\x03" "abc", buffer, 5);

    /* Ping frame received in two parts. */
    size = ws_frame (frame, 0x9, "defgh");
    relay_client_recv_buffer (client, (char *)frame, 3);
    CHECK(client->partial_ws_frame);
    LONGS_EQUAL(3, client->partial_ws_frame_size);
    LONGS_EQUAL(0, peer_recv (buffer, sizeof (buffer)));
    relay_client_recv_buffer (client, (char *)frame + 3, size - 3);
    POINTERS_EQUAL(NULL, client->partial_ws_frame);
    LONGS_EQUAL(7, peer_recv (buffer, sizeof (buffer)));
    MEMCMP_EQUAL("\x8a\x05" "defgh", buffer, 7);

    /* Two ping frames in the same buffer. */
    size = ws_frame (frame2, 0x9, "1");
    size2 = ws_frame (frame2 + size, 0x9, "2");
    relay_client_recv_buffer (client, (char *)frame2, size + size2);
    LONGS_EQUAL(6, peer_recv (buffer, sizeof (buffer)));
    MEMCMP_EQUAL("\x8a\x01" "1" "\x8a\x01" "2", buffer, 6);

    /* Close frame: answer with a close frame, frames after are ignored. */
    size = ws_frame (frame2, 0x8, "bye");
    size2 = ws_frame (frame2 + size, 0x9, "ignored");
    relay_client_recv_buffer (client, (char *)frame2, size + size2);
    LONGS_EQUAL(5, peer_recv (buffer, sizeof (buffer)));
    MEMCMP_EQUAL("\x88\x03" "bye", buffer, 5);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client->status);
}

/*
 * Test functions:
 *   relay_client_recv_buffer (invalid websocket frame)
 */

TEST(RelayClientWithSocket, RecvBufferWebsocketInvalidFrame)
{
    client->websocket = RELAY_CLIENT_WEBSOCKET_READY;
    client->bytes_recv = 1;

    /* Frame not masked: client is disconnected. */
    relay_client_recv_buffer (client, "\x89\x03" "abc", 5);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client->status);
}

/*
 * Test functions:
 *   relay_client_send
 *   relay_client_send_data
 *   relay_client_send_outqueue
 *   relay_client_timer_send_cb
 */

TEST(RelayClientWithSocket, Send)
{
    char *data, *buffer;
    int i, chunk_size, total_sent, total_read, size_read, sndbuf;

    client->send_data_type = RELAY_CLIENT_DATA_TEXT_LINE;

    /* Data sent immediately. */
    LONGS_EQUAL(4, relay_client_send (client, RELAY_MSG_STANDARD,
                                      "abc\n", 4, NULL));
    LONGS_EQUAL(4, client->bytes_sent);
    POINTERS_EQUAL(NULL, client->outqueue);
    POINTERS_EQUAL(NULL, client->hook_timer_send);

    chunk_size = 64 * 1024;
    buffer = (char *)malloc (64 * 1024 * 1024);
    CHECK(buffer);
    LONGS_EQUAL(4, peer_recv (buffer, chunk_size));
    MEMCMP_EQUAL("abc\n", buffer, 4);

    /* Fill the socket buffer: data is queued, then sent by a timer. */
    sndbuf = 4096;
    setsockopt (client->sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof (sndbuf));
    data = (char *)malloc (chunk_size);
    CHECK(data);
    total_sent = 0;
    for (i = 0; (i < 256) && !client->outqueue; i++)
    {
        memset (data, 'a' + (i % 26), chunk_size);
        relay_client_send (client, RELAY_MSG_STANDARD, data, chunk_size, NULL);
        total_sent += chunk_size;
    }
    CHECK(client->outqueue);
    CHECK(client->hook_timer_send);
    CHECK(client->outqueue_size > 0);

    /* Next messages are queued, to keep order of messages. */
    LONGS_EQUAL(-1, relay_client_send (client, RELAY_MSG_STANDARD,
                                       "end\n", 4, NULL));
    total_sent += 4;
    CHECK(client->outqueue->next_outqueue);
    STRNCMP_EQUAL("end\n", client->last_outqueue->data, 4);

    /* Read data in the remote client, and send the out queue. */
    total_read = 0;
    for (i = 0; (i < 100000) && client->outqueue; i++)
    {
        size_read = peer_recv (buffer + total_read,
                               (64 * 1024 * 1024) - total_read);
        total_read += size_read;
        relay_client_send_outqueue (client);
    }
    total_read += peer_recv (buffer + total_read,
                             (64 * 1024 * 1024) - total_read);
    POINTERS_EQUAL(NULL, client->outqueue);
    POINTERS_EQUAL(NULL, client->hook_timer_send);
    LONGS_EQUAL(0, client->outqueue_size);
    LONGS_EQUAL(total_sent, total_read);
    LONGS_EQUAL(total_sent + 4, (int)client->bytes_sent);
    MEMCMP_EQUAL("end\n", buffer + total_read - 4, 4);

    /* Data is received in the order it was sent. */
    for (i = 0; i < (total_read - 4) / chunk_size; i++)
    {
        BYTES_EQUAL('a' + (i % 26), buffer[i * chunk_size]);
        BYTES_EQUAL('a' + (i % 26), buffer[((i + 1) * chunk_size) - 1]);
    }

    free (data);
    free (buffer);
}

/*
 * Test functions:
 *   relay_client_send (error)
 */

TEST(RelayClientWithSocket, SendError)
{
    /* Remote client has closed the connection: client is disconnected. */
    close (sock_peer);
    sock_peer = -1;
    LONGS_EQUAL(-1, relay_client_send (client, RELAY_MSG_STANDARD,
                                       "abc\n", 4, NULL));
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client->status);
    LONGS_EQUAL(-1, client->sock);
}

/*
 * Test functions:
 *   relay_client_set_status
 */

TEST(RelayClientWithSocket, SetStatus)
{
    server->last_client_disconnect = 0;

    relay_client_set_status (client, RELAY_STATUS_CONNECTED);
    LONGS_EQUAL(1, test_relay_client_signal_count);
    STRCMP_EQUAL("relay_client_connected", test_relay_client_signal);
    CHECK(client->hook_fd);

    LONGS_EQUAL(1, relay_client_outqueue_add (client, "test", 4,
                                              NULL, NULL, NULL, NULL));
    CHECK(client->hook_timer_send);

    /* Disconnected: socket closed, out queue freed, hooks removed. */
    relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
    LONGS_EQUAL(2, test_relay_client_signal_count);
    STRCMP_EQUAL("relay_client_disconnected", test_relay_client_signal);
    LONGS_EQUAL(-1, client->sock);
    POINTERS_EQUAL(NULL, client->hook_fd);
    POINTERS_EQUAL(NULL, client->hook_timer_send);
    POINTERS_EQUAL(NULL, client->outqueue);
    CHECK(client->end_time > 0);
    LONGS_EQUAL(client->end_time, server->last_client_disconnect);

    /* Authentication failed: disconnect time not updated on server. */
    free_client (client);
    client = new_client ();
    server->last_client_disconnect = 0;
    relay_client_set_status (client, RELAY_STATUS_AUTH_FAILED);
    STRCMP_EQUAL("relay_client_auth_failed", test_relay_client_signal);
    LONGS_EQUAL(-1, client->sock);
    LONGS_EQUAL(0, server->last_client_disconnect);
}

/*
 * Test functions:
 *   relay_client_disconnect
 *   relay_client_disconnect_all
 */

TEST(RelayClientWithSocket, Disconnect)
{
    struct t_relay_client *client2, *client3;

    client2 = relay_client_new (-1, "test2", server);
    CHECK(client2);

    /* No socket: client is not disconnected. */
    relay_client_disconnect (client2);
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client2->status);

    relay_client_disconnect (client);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client->status);

    free_client (client);
    client = new_client ();
    client3 = client;
    client = new_client ();
    relay_client_disconnect_all ();
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client->status);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client3->status);
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client2->status);

    relay_client_free (client3);
    relay_client_free (client2);
}

/*
 * Test functions:
 *   relay_client_timer
 */

TEST(RelayClientWithSocket, Timer)
{
    struct t_relay_client *client2;

    /* Never purge clients ended (tested below). */
    config_file_option_set (relay_config_network_clients_purge_delay,
                            "-1", 1);

    /* Client not authenticated in time: authentication failed. */
    config_file_option_set (relay_config_network_auth_timeout, "60", 1);
    client->start_time = time (NULL) - 30;
    relay_client_timer ();
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client->status);
    client->start_time = time (NULL) - 120;
    relay_client_timer ();
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED, client->status);

    /* No timeout. */
    config_file_option_set (relay_config_network_auth_timeout, "0", 1);
    client2 = relay_client_new (-1, "test2", server);
    CHECK(client2);
    client2->start_time = time (NULL) - 100000;
    relay_client_timer ();
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client2->status);
    relay_client_free (client2);

    /* Client ended: never purged. */
    client->end_time = time (NULL) - 100000;
    relay_client_timer ();
    LONGS_EQUAL(1, relay_client_valid (client));

    /* Client ended: purged after delay (in minutes). */
    config_file_option_set (relay_config_network_clients_purge_delay,
                            "10", 1);
    client->end_time = time (NULL) - 60;
    relay_client_timer ();
    LONGS_EQUAL(1, relay_client_valid (client));
    client->end_time = time (NULL) - 601;
    relay_client_timer ();
    LONGS_EQUAL(0, relay_client_valid (client));
    client = NULL;
}

/*
 * Test functions:
 *   relay_client_timer (send out queue)
 */

TEST(RelayClientWithSocket, TimerSendOutqueue)
{
    char buffer[64];

    LONGS_EQUAL(1, relay_client_outqueue_add (client, "test", 4,
                                              NULL, NULL, NULL, NULL));
    relay_client_timer ();
    POINTERS_EQUAL(NULL, client->outqueue);
    LONGS_EQUAL(4, peer_recv (buffer, sizeof (buffer)));
    MEMCMP_EQUAL("test", buffer, 4);
}

/*
 * Test functions:
 *   relay_client_add_to_infolist
 *   relay_client_new_with_infolist
 */

TEST(RelayClientWithSocket, Infolist)
{
    struct t_infolist *infolist;
    struct t_relay_client *client2;

    client->real_ip = strdup ("192.168.1.2");
    relay_client_set_desc (client);
    client->bytes_recv = 123;
    client->bytes_sent = 4567890123ULL;
    client->partial_message = strdup ("partial");
    client->partial_ws_frame = (char *)malloc (3);
    memcpy (client->partial_ws_frame, "abc", 3);
    client->partial_ws_frame_size = 3;

    LONGS_EQUAL(0, relay_client_add_to_infolist (NULL, client, 0));
    infolist = infolist_new (NULL);
    CHECK(infolist);
    LONGS_EQUAL(0, relay_client_add_to_infolist (infolist, NULL, 0));

    /*
     * Save client in its current state, then restore it (the hook on the
     * socket is removed first, as the socket is used by the restored client).
     */
    LONGS_EQUAL(1, relay_client_add_to_infolist (infolist, client, 0));
    unhook (client->hook_fd);
    client->hook_fd = NULL;
    CHECK(infolist_next (infolist));
    LONGS_EQUAL(client->sock, infolist_integer (infolist, "sock"));
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING,
                infolist_integer (infolist, "status"));
    client2 = relay_client_new_with_infolist (infolist);
    CHECK(client2);
    POINTERS_EQUAL(client2, relay_clients);
    LONGS_EQUAL(client->id, client2->id);
    STRCMP_EQUAL(client->desc, client2->desc);
    LONGS_EQUAL(client->sock, client2->sock);
    CHECK(client2->hook_fd);
    LONGS_EQUAL(client->server_port, client2->server_port);
    LONGS_EQUAL(client->tls, client2->tls);
    STRCMP_EQUAL("test", client2->address);
    STRCMP_EQUAL("192.168.1.2", client2->real_ip);
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client2->status);
    LONGS_EQUAL(RELAY_PROTOCOL_WEECHAT, client2->protocol);
    STRCMP_EQUAL("weechat", client2->protocol_string);
    POINTERS_EQUAL(NULL, client2->protocol_args);
    STRCMP_EQUAL(client->nonce, client2->nonce);
    LONGS_EQUAL(client->password_hash_algo, client2->password_hash_algo);
    LONGS_EQUAL(client->start_time, client2->start_time);
    LONGS_EQUAL(123, client2->bytes_recv);
    CHECK(4567890123ULL == client2->bytes_sent);
    LONGS_EQUAL(client->recv_data_type, client2->recv_data_type);
    LONGS_EQUAL(client->send_data_type, client2->send_data_type);
    STRCMP_EQUAL("partial", client2->partial_message);
    LONGS_EQUAL(3, client2->partial_ws_frame_size);
    MEMCMP_EQUAL("abc", client2->partial_ws_frame, 3);
    CHECK(client2->protocol_data);
    POINTERS_EQUAL(NULL, client2->outqueue);
    relay_client_free (client2);
    infolist_free (infolist);

    /* Save client in a disconnected state: client is unchanged. */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    LONGS_EQUAL(1, relay_client_add_to_infolist (infolist, client, 1));
    CHECK(infolist_next (infolist));
    LONGS_EQUAL(-1, infolist_integer (infolist, "sock"));
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED,
                infolist_integer (infolist, "status"));
    CHECK(infolist_time (infolist, "end_time") > 0);
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client->status);
    CHECK(client->sock >= 0);
    client2 = relay_client_new_with_infolist (infolist);
    CHECK(client2);
    LONGS_EQUAL(-1, client2->sock);
    POINTERS_EQUAL(NULL, client2->hook_fd);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client2->status);
    relay_client_free (client2);
    infolist_free (infolist);

    /* Client saved without description: description is computed. */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    free (client->desc);
    client->desc = NULL;
    LONGS_EQUAL(1, relay_client_add_to_infolist (infolist, client, 1));
    CHECK(infolist_next (infolist));
    POINTERS_EQUAL(NULL, infolist_string (infolist, "desc"));
    client2 = relay_client_new_with_infolist (infolist);
    CHECK(client2);
    relay_client_set_desc (client);
    STRCMP_EQUAL(client->desc, client2->desc);
    relay_client_free (client2);
    infolist_free (infolist);
}
