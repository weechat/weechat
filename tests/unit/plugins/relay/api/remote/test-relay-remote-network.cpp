/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test network functions for relay remote */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/core/hook/hook-url.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-auth.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-json.h"
#include "src/plugins/relay/relay-remote.h"
#include "src/plugins/relay/relay-websocket.h"
#include "src/plugins/relay/api/relay-api.h"
#include "src/plugins/relay/api/remote/relay-remote-network.h"

extern char *relay_remote_network_get_url_resource (struct t_relay_remote *remote,
                                                    const char *resource);
extern char *relay_remote_network_get_handshake_request ();
extern void relay_remote_network_close_connection (struct t_relay_remote *remote);
extern int relay_remote_network_check_auth (struct t_relay_remote *remote,
                                            const char *buffer);
extern int relay_remote_network_send_data (struct t_relay_remote *remote,
                                           const char *data, int data_size);
extern void relay_remote_network_recv_text (struct t_relay_remote *remote,
                                            const char *buffer,
                                            int buffer_size);
extern void relay_remote_network_recv_buffer (struct t_relay_remote *remote,
                                              const char *buffer,
                                              int buffer_size);
extern int relay_remote_network_recv_cb (const void *pointer, void *data,
                                         int fd);
extern void relay_remote_network_connect_ws_auth (struct t_relay_remote *remote);
extern int relay_remote_network_connect_cb (const void *pointer, void *data,
                                            int status, int gnutls_rc,
                                            int sock, const char *error,
                                            const char *ip_address);
extern int relay_remote_network_url_handshake_cb (const void *pointer,
                                                  void *data,
                                                  const char *url,
                                                  struct t_hashtable *options,
                                                  struct t_hashtable *output);
}

struct t_relay_remote *ptr_relay_remote = NULL;
struct t_relay_remote *ptr_relay_remote2 = NULL;

/*
 * Read a websocket frame sent by the remote (client side: masked frame,
 * payload < 126 bytes) and unmask its payload.
 *
 * Return the opcode (-1 if error), the payload is stored in "payload".
 */

int
test_remote_read_frame (const unsigned char *frame, int size,
                        char *payload, int payload_size)
{
    int i, length;

    payload[0] = '\0';
    if (size < 6)
        return -1;
    if (!(frame[1] & 0x80))
        return -1;
    length = frame[1] & 0x7F;
    if ((length >= 126) || (6 + length > size) || (length >= payload_size))
        return -1;
    for (i = 0; i < length; i++)
    {
        payload[i] = frame[6 + i] ^ frame[2 + (i % 4)];
    }
    payload[length] = '\0';
    return frame[0] & 0x0F;
}

/*
 * Build a websocket frame sent by a remote relay server (not masked,
 * payload < 126 bytes).
 *
 * Return size of frame.
 */

int
test_remote_build_frame (unsigned char *frame, int opcode,
                         const char *payload)
{
    int length;

    length = strlen (payload);
    frame[0] = 0x80 | opcode;
    frame[1] = length;
    memcpy (frame + 2, payload, length);
    return 2 + length;
}

TEST_GROUP(RelayRemoteNetwork)
{
};

TEST_GROUP(RelayRemoteNetworkWithRemote)
{
    void setup ()
    {
        /* Disable auto-open of relay buffer. */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);

        /* Create two relay remotes. */
        ptr_relay_remote = relay_remote_new ("remote",
                                             "http://localhost:9000",
                                             NULL, "off", NULL, "on",
                                             "secret", "secretbase32");
        ptr_relay_remote2 = relay_remote_new ("remote2",
                                              "https://localhost:9001/",
                                              "30", "off", "my_proxy", "off",
                                              "secret", "secretbase32");
    }

    void teardown ()
    {
        relay_remote_free (ptr_relay_remote);
        ptr_relay_remote = NULL;
        relay_remote_free (ptr_relay_remote2);
        ptr_relay_remote2 = NULL;

        /* Restore auto-open of relay buffer. */
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);
    }
};

/*
 * Test functions:
 *   relay_remote_network_get_url_resource
 */

TEST(RelayRemoteNetworkWithRemote, GetUrlResource)
{
    char *str;

    WEE_TEST_STR(NULL, relay_remote_network_get_url_resource (NULL, NULL));
    WEE_TEST_STR(NULL, relay_remote_network_get_url_resource (NULL, ""));
    WEE_TEST_STR(NULL, relay_remote_network_get_url_resource (NULL, "/api/buffers"));

    WEE_TEST_STR(
        "http://localhost:9000/api/buffers",
        relay_remote_network_get_url_resource (ptr_relay_remote, "buffers"));
    WEE_TEST_STR(
        "https://localhost:9001/api/buffers",
        relay_remote_network_get_url_resource (ptr_relay_remote2, "buffers"));
}

/*
 * Test functions:
 *   relay_remote_network_close_connection
 */

TEST(RelayRemoteNetwork, CloseConnection)
{
    struct t_relay_remote *remote;
    int sv[2];

    relay_remote_network_close_connection (NULL);

    remote = relay_remote_new ("remoteclose", "http://localhost:9000",
                               NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(remote);

    LONGS_EQUAL(0, socketpair (AF_UNIX, SOCK_STREAM, 0, sv));
    remote->sock = sv[0];
    remote->version_ok = 1;
    remote->synced = 1;
    remote->partial_ws_frame = (char *)malloc (3);
    memcpy (remote->partial_ws_frame, "abc", 3);
    remote->partial_ws_frame_size = 3;

    relay_remote_network_close_connection (remote);
    LONGS_EQUAL(-1, remote->sock);
    POINTERS_EQUAL(NULL, remote->hook_fd);
    LONGS_EQUAL(0, remote->version_ok);
    LONGS_EQUAL(0, remote->synced);
    POINTERS_EQUAL(NULL, remote->partial_ws_frame);
    LONGS_EQUAL(0, remote->partial_ws_frame_size);

    /* The other end of the socket sees the connection closed. */
    LONGS_EQUAL(0, recv (sv[1], sv, 1, 0));
    close (sv[1]);

    /* Already closed: nothing done. */
    relay_remote_network_close_connection (remote);
    LONGS_EQUAL(-1, remote->sock);

    relay_remote_free (remote);
}

/*
 * Test functions:
 *   relay_remote_network_disconnect
 */

TEST(RelayRemoteNetwork, Disconnect)
{
    struct t_relay_remote *remote;
    int sv[2];

    relay_remote_network_disconnect (NULL);

    remote = relay_remote_new ("remotedisc", "http://localhost:9000",
                               "off", "10", NULL, NULL, NULL, NULL);
    CHECK(remote);

    LONGS_EQUAL(0, socketpair (AF_UNIX, SOCK_STREAM, 0, sv));
    remote->sock = sv[0];
    relay_remote_set_status (remote, RELAY_STATUS_CONNECTED);

    /* Connection closed and reconnection scheduled. */
    relay_remote_network_disconnect (remote);
    LONGS_EQUAL(-1, remote->sock);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(10, remote->reconnect_delay);
    CHECK(remote->reconnect_start > 0);
    close (sv[1]);

    /* No reconnection if delay is 0. */
    config_file_option_set (
        remote->options[RELAY_REMOTE_OPTION_AUTORECONNECT_DELAY], "0", 1);
    relay_remote_network_disconnect (remote);
    LONGS_EQUAL(0, remote->reconnect_delay);
    LONGS_EQUAL(0, remote->reconnect_start);

    relay_remote_free (remote);
}

/*
 * Test functions:
 *   relay_remote_network_check_auth
 */

TEST(RelayRemoteNetwork, CheckAuth)
{
    struct t_relay_remote *remote;

    remote = relay_remote_new ("remoteauth", "http://localhost:9000",
                               NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(remote);

    /* Key from RFC 6455, section 1.3. */
    remote->websocket_key = strdup ("dGhlIHNhbXBsZSBub25jZQ==");

    /* Invalid response. */
    LONGS_EQUAL(0, relay_remote_network_check_auth (remote, ""));
    LONGS_EQUAL(0, relay_remote_network_check_auth (remote, "test"));

    /* Authentication failed (with error message in body). */
    LONGS_EQUAL(
        0,
        relay_remote_network_check_auth (
            remote,
            "HTTP/1.1 401 Unauthorized\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 31\r\n"
            "\r\n"
            "{\"error\": \"Invalid password\"}\r\n"));

    /* Other error. */
    LONGS_EQUAL(
        0,
        relay_remote_network_check_auth (
            remote, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n"));
    LONGS_EQUAL(
        0,
        relay_remote_network_check_auth (
            remote, "HTTP/1.1 101 Other message\r\n\r\n"));

    /* Missing or invalid "Sec-WebSocket-Accept". */
    LONGS_EQUAL(
        0,
        relay_remote_network_check_auth (
            remote,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "\r\n"));
    LONGS_EQUAL(
        0,
        relay_remote_network_check_auth (
            remote,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: invalid\r\n"
            "\r\n"));

    /* Valid "Sec-WebSocket-Accept" (case insensitive message). */
    LONGS_EQUAL(
        1,
        relay_remote_network_check_auth (
            remote,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
            "\r\n"));
    LONGS_EQUAL(0, remote->ws_deflate->enabled);
    LONGS_EQUAL(
        1,
        relay_remote_network_check_auth (
            remote,
            "HTTP/1.1 101 switching protocols\r\n"
            "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
            "\r\n"));

    /* Extension "permessage-deflate" accepted by the remote. */
    LONGS_EQUAL(
        1,
        relay_remote_network_check_auth (
            remote,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
            "Sec-WebSocket-Extensions: permessage-deflate\r\n"
            "\r\n"));
    LONGS_EQUAL(1, remote->ws_deflate->enabled);

    /* No key sent: invalid response. */
    free (remote->websocket_key);
    remote->websocket_key = NULL;
    LONGS_EQUAL(
        0,
        relay_remote_network_check_auth (
            remote,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
            "\r\n"));

    relay_remote_free (remote);
}

/*
 * Test functions:
 *   relay_remote_network_send_data
 */

TEST(RelayRemoteNetwork, SendData)
{
    LONGS_EQUAL(0, relay_remote_network_send_data (NULL, "test", 4));
}

/*
 * Test functions:
 *   relay_remote_network_send
 */

TEST(RelayRemoteNetwork, Send)
{
    LONGS_EQUAL(0, relay_remote_network_send (NULL, RELAY_MSG_STANDARD,
                                              "test", 4));
}

/*
 * Test functions:
 *   relay_remote_network_send_json
 */

TEST(RelayRemoteNetwork, SendJson)
{
    struct t_relay_json *json;

    json = relay_json_new_object ();
    CHECK(json);
    LONGS_EQUAL(0, relay_remote_network_send_json (NULL, json));
    LONGS_EQUAL(0, relay_remote_network_send_json (ptr_relay_remote, NULL));
    relay_json_free (json);
}

/*
 * Test functions:
 *   relay_remote_network_get_handshake_request
 */

TEST(RelayRemoteNetwork, GetHandshakeRequest)
{
    char *str;

    WEE_TEST_STR("{\"password_hash_algo\":["
                 "\"plain\","
                 "\"sha256\","
                 "\"sha512\","
                 "\"pbkdf2+sha256\","
                 "\"pbkdf2+sha512\""
                 "]}",
                 relay_remote_network_get_handshake_request ());
}

TEST_GROUP(RelayRemoteNetworkWithSocket)
{
    struct t_relay_remote *remote;
    int sock_peer;
    char buffer[65536];

    /*
     * Read all data sent by the remote (on the other end of the socket).
     *
     * Return number of bytes read.
     */

    int peer_recv ()
    {
        int num_read, total;

        total = 0;
        while (total < (int)sizeof (buffer) - 1)
        {
            num_read = recv (sock_peer, buffer + total,
                             sizeof (buffer) - 1 - total, 0);
            if (num_read <= 0)
                break;
            total += num_read;
        }
        buffer[total] = '\0';
        return total;
    }

    /*
     * Read a websocket frame sent by the remote, return its opcode.
     */

    int peer_recv_frame (char *payload, int payload_size)
    {
        int size;

        size = peer_recv ();
        return test_remote_read_frame ((unsigned char *)buffer, size,
                                       payload, payload_size);
    }

    void setup ()
    {
        int sv[2];

        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);
        config_file_option_set (
            relay_config_network_websocket_permessage_deflate, "off", 1);

        remote = relay_remote_new ("remotesock",
                                   /* nothing listening on port 1 */
                                   "http://localhost:1",
                                   "off",  /* autoconnect */
                                   "0",  /* autoreconnect_delay */
                                   "",  /* proxy */
                                   "off",  /* tls_verify */
                                   "secret",  /* password */
                                   "");  /* totp_secret */
        CHECK(remote);

        LONGS_EQUAL(0, socketpair (AF_UNIX, SOCK_STREAM, 0, sv));
        fcntl (sv[0], F_SETFL, fcntl (sv[0], F_GETFL) | O_NONBLOCK);
        fcntl (sv[1], F_SETFL, fcntl (sv[1], F_GETFL) | O_NONBLOCK);
        remote->sock = sv[0];
        sock_peer = sv[1];
    }

    void teardown ()
    {
        relay_remote_network_close_connection (remote);
        relay_remote_free (remote);
        if (sock_peer >= 0)
            close (sock_peer);
        config_file_option_reset (
            relay_config_network_websocket_permessage_deflate, 1);
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);
    }
};

/*
 * Test functions:
 *   relay_remote_network_send_data
 */

TEST(RelayRemoteNetworkWithSocket, SendData)
{
    /* Data sent on the socket. */
    LONGS_EQUAL(4, relay_remote_network_send_data (remote, "test", 4));
    LONGS_EQUAL(4, peer_recv ());
    STRCMP_EQUAL("test", buffer);

    /* No socket: data considered as sent. */
    close (remote->sock);
    remote->sock = -1;
    LONGS_EQUAL(4, relay_remote_network_send_data (remote, "test", 4));
}

/*
 * Test functions:
 *   relay_remote_network_send
 */

TEST(RelayRemoteNetworkWithSocket, Send)
{
    char payload[256];

    /* Not connected: data sent as-is (HTTP request). */
    LONGS_EQUAL(4, relay_remote_network_send (remote, RELAY_MSG_STANDARD,
                                              "test", 4));
    LONGS_EQUAL(4, peer_recv ());
    STRCMP_EQUAL("test", buffer);

    /* Connected: data sent in a masked websocket frame. */
    relay_remote_set_status (remote, RELAY_STATUS_CONNECTED);
    LONGS_EQUAL(10, relay_remote_network_send (remote, RELAY_MSG_STANDARD,
                                               "test", 4));
    LONGS_EQUAL(0x1, peer_recv_frame (payload, sizeof (payload)));
    STRCMP_EQUAL("test", payload);

    LONGS_EQUAL(9, relay_remote_network_send (remote, RELAY_MSG_PING,
                                              "abc", 3));
    LONGS_EQUAL(0x9, peer_recv_frame (payload, sizeof (payload)));
    STRCMP_EQUAL("abc", payload);

    relay_remote_network_send (remote, RELAY_MSG_PONG, "abc", 3);
    LONGS_EQUAL(0xA, peer_recv_frame (payload, sizeof (payload)));

    relay_remote_network_send (remote, RELAY_MSG_CLOSE, "bye", 3);
    LONGS_EQUAL(0x8, peer_recv_frame (payload, sizeof (payload)));
    STRCMP_EQUAL("bye", payload);
}

/*
 * Test functions:
 *   relay_remote_network_send_json
 */

TEST(RelayRemoteNetworkWithSocket, SendJson)
{
    struct t_relay_json *json;
    char payload[256];

    json = relay_json_new_object ();
    CHECK(json);
    relay_json_object_add (json, "request",
                           relay_json_new_string ("GET /api/version"));

    relay_remote_set_status (remote, RELAY_STATUS_CONNECTED);
    CHECK(relay_remote_network_send_json (remote, json) > 0);
    LONGS_EQUAL(0x1, peer_recv_frame (payload, sizeof (payload)));
    STRCMP_EQUAL("{\"request\":\"GET /api/version\"}", payload);

    relay_json_free (json);
}

/*
 * Test functions:
 *   relay_remote_network_recv_text
 */

TEST(RelayRemoteNetworkWithSocket, RecvText)
{
    remote->websocket_key = strdup ("dGhlIHNhbXBsZSBub25jZQ==");

    /* Authentication failed: disconnected. */
    relay_remote_set_status (remote, RELAY_STATUS_AUTHENTICATING);
    relay_remote_network_recv_text (remote, "HTTP/1.1 401 Unauthorized\r\n\r\n",
                                    29);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(-1, remote->sock);
}

/*
 * Test functions:
 *   relay_remote_network_recv_text (authentication OK)
 */

TEST(RelayRemoteNetworkWithSocket, RecvTextAuthOk)
{
    char event_version[512];
    const char *response_ok =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
        "\r\n";
    char payload[256];

    remote->websocket_key = strdup ("dGhlIHNhbXBsZSBub25jZQ==");
    remote->reconnect_delay = 30;
    remote->reconnect_start = 123;

    /* Authentication OK: connected, request "GET /api/version" sent. */
    relay_remote_set_status (remote, RELAY_STATUS_AUTHENTICATING);
    relay_remote_network_recv_text (remote, response_ok, strlen (response_ok));
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, remote->status);
    LONGS_EQUAL(0, remote->reconnect_delay);
    LONGS_EQUAL(0, remote->reconnect_start);
    LONGS_EQUAL(0x1, peer_recv_frame (payload, sizeof (payload)));
    STRCMP_EQUAL("{\"request\": \"GET /api/version\"}", payload);

    /* Connected: event received from remote. */
    snprintf (event_version, sizeof (event_version),
              "{\"code\": 200, \"body_type\": \"version\", "
              "\"body\": {\"weechat_version\": \"5.0.0\", "
              "\"weechat_version_git\": \"\", "
              "\"relay_api_version\": \"%s\"}}",
              RELAY_API_VERSION_STR);
    relay_remote_network_recv_text (remote, event_version,
                                    strlen (event_version));
    LONGS_EQUAL(1, remote->version_ok);
}

/*
 * Test functions:
 *   relay_remote_network_recv_buffer
 *   relay_remote_network_read_websocket_frames
 */

TEST(RelayRemoteNetworkWithSocket, RecvBufferWebsocketFrames)
{
    unsigned char frame[512];
    char payload[256];
    int size;

    /* Status not authenticating/connected: data ignored. */
    relay_remote_set_status (remote, RELAY_STATUS_CONNECTING);
    size = test_remote_build_frame (frame, 0x9, "abc");
    relay_remote_network_recv_buffer (remote, (char *)frame, size);
    LONGS_EQUAL(0, peer_recv ());

    relay_remote_set_status (remote, RELAY_STATUS_CONNECTED);

    /* Ping: answer with a pong. */
    relay_remote_network_recv_buffer (remote, (char *)frame, size);
    LONGS_EQUAL(0xA, peer_recv_frame (payload, sizeof (payload)));
    STRCMP_EQUAL("abc", payload);

    /* Empty frame (pong): ignored. */
    size = test_remote_build_frame (frame, 0xA, "");
    relay_remote_network_recv_buffer (remote, (char *)frame, size);
    LONGS_EQUAL(0, peer_recv ());

    /* Frame received in two parts. */
    size = test_remote_build_frame (frame, 0x9, "defgh");
    relay_remote_network_recv_buffer (remote, (char *)frame, 3);
    CHECK(remote->partial_ws_frame);
    LONGS_EQUAL(3, remote->partial_ws_frame_size);
    LONGS_EQUAL(0, peer_recv ());
    relay_remote_network_recv_buffer (remote, (char *)frame + 3, size - 3);
    POINTERS_EQUAL(NULL, remote->partial_ws_frame);
    LONGS_EQUAL(0xA, peer_recv_frame (payload, sizeof (payload)));
    STRCMP_EQUAL("defgh", payload);

    /* Text frame: event received from remote. */
    size = test_remote_build_frame (
        frame, 0x1,
        "{\"code\": 200, \"body_type\": \"version\", "
        "\"body\": {\"relay_api_version\": \"0.0.0\"}}");
    relay_remote_network_recv_buffer (remote, (char *)frame, size);
    /* API version mismatch: disconnected. */
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
}

/*
 * Test functions:
 *   relay_remote_network_recv_buffer (close frame)
 *   relay_remote_network_read_websocket_frames
 */

TEST(RelayRemoteNetworkWithSocket, RecvBufferWebsocketClose)
{
    unsigned char frame[512];
    char payload[256];
    int size, size2;

    relay_remote_set_status (remote, RELAY_STATUS_CONNECTED);

    /* Close: answer with a close, frames after are ignored. */
    size = test_remote_build_frame (frame, 0x8, "bye");
    size2 = test_remote_build_frame (frame + size, 0x9, "ignored");
    relay_remote_network_recv_buffer (remote, (char *)frame, size + size2);
    LONGS_EQUAL(0x8, peer_recv_frame (payload, sizeof (payload)));
    STRCMP_EQUAL("bye", payload);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(-1, remote->sock);
}

/*
 * Test functions:
 *   relay_remote_network_recv_buffer (invalid frame)
 */

TEST(RelayRemoteNetworkWithSocket, RecvBufferWebsocketInvalid)
{
    unsigned char frame[16];

    relay_remote_set_status (remote, RELAY_STATUS_CONNECTED);

    /* Fragmented control frame (ping without FIN bit): disconnected. */
    frame[0] = 0x09;
    frame[1] = 0x01;
    frame[2] = 'a';
    relay_remote_network_recv_buffer (remote, (char *)frame, 3);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
}

/*
 * Test functions:
 *   relay_remote_network_recv_cb
 */

TEST(RelayRemoteNetworkWithSocket, RecvCb)
{
    unsigned char frame[64];
    char payload[256];
    int size, sock;

    LONGS_EQUAL(WEECHAT_RC_ERROR,
                relay_remote_network_recv_cb (NULL, NULL, 0));

    relay_remote_set_status (remote, RELAY_STATUS_CONNECTED);

    /* No data available: nothing done. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_remote_network_recv_cb (remote, NULL, remote->sock));
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, remote->status);

    /* Data received on the socket. */
    size = test_remote_build_frame (frame, 0x9, "abc");
    LONGS_EQUAL(size, send (sock_peer, frame, size, 0));
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_remote_network_recv_cb (remote, NULL, remote->sock));
    LONGS_EQUAL(0xA, peer_recv_frame (payload, sizeof (payload)));

    /* Connection closed by peer: disconnected. */
    close (sock_peer);
    sock_peer = -1;
    sock = remote->sock;
    LONGS_EQUAL(WEECHAT_RC_OK, relay_remote_network_recv_cb (remote, NULL, sock));
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(-1, remote->sock);

    /* No socket: nothing done. */
    LONGS_EQUAL(WEECHAT_RC_OK, relay_remote_network_recv_cb (remote, NULL, -1));
}

/*
 * Test functions:
 *   relay_remote_network_connect_ws_auth
 */

TEST(RelayRemoteNetworkWithSocket, ConnectWsAuth)
{
    char auth_expected[512], str_auth[256], *pos, *pos2, key[128];

    /* Plain text password. */
    remote->password_hash_algo = RELAY_AUTH_PASSWORD_HASH_PLAIN;
    relay_remote_network_connect_ws_auth (remote);
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, remote->status);
    CHECK(remote->websocket_key);
    LONGS_EQUAL(24, strlen (remote->websocket_key));
    CHECK(peer_recv () > 0);
    snprintf (auth_expected, sizeof (auth_expected),
              "GET /api HTTP/1.1\r\n"
              "Authorization: Basic cGxhaW46c2VjcmV0\r\n"
              "Sec-WebSocket-Version: 13\r\n"
              "Sec-WebSocket-Key: %s\r\n"
              "Connection: Upgrade\r\n"
              "Upgrade: websocket\r\n"
              "Host: localhost:1\r\n"
              "\r\n",
              remote->websocket_key);
    STRCMP_EQUAL(auth_expected, buffer);

    /* A new websocket key is generated for each connection. */
    snprintf (key, sizeof (key), "%s", remote->websocket_key);
    relay_remote_network_connect_ws_auth (remote);
    CHECK(strcmp (key, remote->websocket_key) != 0);
    peer_recv ();

    /* Hashed password (SHA-256), with TOTP and permessage-deflate. */
    config_file_option_set (remote->options[RELAY_REMOTE_OPTION_TOTP_SECRET],
                            "secretbase32", 1);
    config_file_option_set (relay_config_network_websocket_permessage_deflate,
                            "on", 1);
    remote->password_hash_algo = RELAY_AUTH_PASSWORD_HASH_SHA256;
    relay_remote_network_connect_ws_auth (remote);
    CHECK(peer_recv () > 0);
    pos = strstr (buffer, "Authorization: Basic ");
    CHECK(pos);
    pos += 21;
    pos2 = strstr (pos, "\r\n");
    CHECK(pos2);
    pos2[0] = '\0';
    CHECK(string_base_decode ("64", pos, str_auth) > 0);
    pos2[0] = '\r';
    STRNCMP_EQUAL("hash:sha256:", str_auth, 12);
    CHECK(strstr (buffer, "\r\nx-weechat-totp: "));
    CHECK(strstr (buffer, "\r\nSec-WebSocket-Extensions: permessage-deflate; "
                  "client_max_window_bits\r\n"));

    /* PBKDF2. */
    remote->password_hash_algo = RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA512;
    remote->password_hash_iterations = 1000;
    relay_remote_network_connect_ws_auth (remote);
    CHECK(peer_recv () > 0);
    pos = strstr (buffer, "Authorization: Basic ") + 21;
    pos2 = strstr (pos, "\r\n");
    pos2[0] = '\0';
    CHECK(string_base_decode ("64", pos, str_auth) > 0);
    STRNCMP_EQUAL("hash:pbkdf2+sha512:", str_auth, 19);
    CHECK(strstr (str_auth, ":1000:"));

    /* No password: no authorization header. */
    config_file_option_set (remote->options[RELAY_REMOTE_OPTION_PASSWORD],
                            "", 1);
    config_file_option_set (remote->options[RELAY_REMOTE_OPTION_TOTP_SECRET],
                            "", 1);
    relay_remote_network_connect_ws_auth (remote);
    CHECK(peer_recv () > 0);
    POINTERS_EQUAL(NULL, strstr (buffer, "Authorization:"));
    POINTERS_EQUAL(NULL, strstr (buffer, "x-weechat-totp:"));
}

/*
 * Test functions:
 *   relay_remote_network_connect_cb
 */

TEST(RelayRemoteNetworkWithSocket, ConnectCb)
{
    int i, sock;
    int status_errors[] = {
        WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND,
        WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND,
        WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED,
        WEECHAT_HOOK_CONNECT_PROXY_ERROR,
        WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR,
        WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR,
        WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR,
        WEECHAT_HOOK_CONNECT_MEMORY_ERROR,
        WEECHAT_HOOK_CONNECT_TIMEOUT,
        WEECHAT_HOOK_CONNECT_SOCKET_ERROR,
        -1,
    };

    /* Connection OK: authentication sent. */
    sock = remote->sock;
    remote->sock = -1;
    remote->password_hash_algo = RELAY_AUTH_PASSWORD_HASH_PLAIN;
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_remote_network_connect_cb (
                    remote, NULL, WEECHAT_HOOK_CONNECT_OK, 0, sock, NULL,
                    "127.0.0.1"));
    LONGS_EQUAL(sock, remote->sock);
    CHECK(remote->hook_fd);
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, remote->status);
    CHECK(peer_recv () > 0);
    STRNCMP_EQUAL("GET /api HTTP/1.1\r\n", buffer, 19);

    /* Errors: disconnected. */
    for (i = 0; status_errors[i] >= 0; i++)
    {
        relay_remote_set_status (remote, RELAY_STATUS_CONNECTING);
        LONGS_EQUAL(WEECHAT_RC_OK,
                    relay_remote_network_connect_cb (
                        remote, NULL, status_errors[i], 0, -1, "error",
                        NULL));
        LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
        LONGS_EQUAL(-1, remote->sock);
        POINTERS_EQUAL(NULL, remote->hook_fd);
    }
}

/*
 * Test functions:
 *   relay_remote_network_url_handshake_cb
 */

TEST(RelayRemoteNetworkWithSocket, UrlHandshakeCb)
{
    struct t_hashtable *output;

    output = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                            WEECHAT_HASHTABLE_STRING, NULL, NULL);
    CHECK(output);

#define WEE_CHECK_HANDSHAKE_ERROR(__code, __error, __output)            \
    hashtable_remove_all (output);                                      \
    if (__code)                                                         \
        hashtable_set (output, "response_code", __code);                \
    if (__error)                                                        \
        hashtable_set (output, "error", __error);                       \
    if (__output)                                                       \
        hashtable_set (output, "output", __output);                     \
    relay_remote_set_status (remote, RELAY_STATUS_CONNECTING);          \
    remote->password_hash_algo = -1;                                    \
    remote->password_hash_iterations = -1;                              \
    remote->totp = -1;                                                  \
    LONGS_EQUAL(WEECHAT_RC_OK,                                          \
                relay_remote_network_url_handshake_cb (                 \
                    remote, NULL, "url", NULL, output));                \
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);             \
    POINTERS_EQUAL(NULL, remote->hook_connect);

    /* HTTP error or error from URL transfer. */
    WEE_CHECK_HANDSHAKE_ERROR("404", NULL, NULL);
    WEE_CHECK_HANDSHAKE_ERROR(NULL, "connection refused", NULL);

    /* Invalid or incomplete response. */
    WEE_CHECK_HANDSHAKE_ERROR("200", NULL, NULL);
    WEE_CHECK_HANDSHAKE_ERROR("200", NULL, "invalid json");
    WEE_CHECK_HANDSHAKE_ERROR("200", NULL, "{}");
    WEE_CHECK_HANDSHAKE_ERROR(
        "200", NULL,
        "{\"password_hash_algo\": \"unknown\", "
        "\"password_hash_iterations\": 100000, \"totp\": false}");
    WEE_CHECK_HANDSHAKE_ERROR(
        "200", NULL,
        "{\"password_hash_algo\": \"sha256\", \"totp\": false}");

    /*
     * Number of iterations out of range (1 to 1000000): refused (values that
     * do not fit in an integer must not be truncated).
     */
    WEE_CHECK_HANDSHAKE_ERROR(
        "200", NULL,
        "{\"password_hash_algo\": \"pbkdf2+sha512\", "
        "\"password_hash_iterations\": 0, \"totp\": false}");
    WEE_CHECK_HANDSHAKE_ERROR(
        "200", NULL,
        "{\"password_hash_algo\": \"pbkdf2+sha512\", "
        "\"password_hash_iterations\": -1, \"totp\": false}");
    WEE_CHECK_HANDSHAKE_ERROR(
        "200", NULL,
        "{\"password_hash_algo\": \"pbkdf2+sha512\", "
        "\"password_hash_iterations\": 1000001, \"totp\": false}");
    WEE_CHECK_HANDSHAKE_ERROR(
        "200", NULL,
        "{\"password_hash_algo\": \"pbkdf2+sha512\", "
        "\"password_hash_iterations\": 2147483647, \"totp\": false}");
    WEE_CHECK_HANDSHAKE_ERROR(
        "200", NULL,
        "{\"password_hash_algo\": \"pbkdf2+sha512\", "
        "\"password_hash_iterations\": 4294967396, \"totp\": false}");
    WEE_CHECK_HANDSHAKE_ERROR(
        "200", NULL,
        "{\"password_hash_algo\": \"sha256\", "
        "\"password_hash_iterations\": 99999999999, \"totp\": false}");
    WEE_CHECK_HANDSHAKE_ERROR(
        "200", NULL,
        "{\"password_hash_algo\": \"sha256\", "
        "\"password_hash_iterations\": 100000, \"totp\": \"no\"}");

    /* Proxy not found. */
    config_file_option_set (remote->options[RELAY_REMOTE_OPTION_PROXY],
                            "unknown_proxy", 1);
    WEE_CHECK_HANDSHAKE_ERROR(
        "200", NULL,
        "{\"password_hash_algo\": \"pbkdf2+sha256\", "
        "\"password_hash_iterations\": 100000, \"totp\": true}");
    LONGS_EQUAL(RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA256,
                remote->password_hash_algo);
    LONGS_EQUAL(100000, remote->password_hash_iterations);
    LONGS_EQUAL(1, remote->totp);
    config_file_option_set (remote->options[RELAY_REMOTE_OPTION_PROXY],
                            "", 1);

#undef WEE_CHECK_HANDSHAKE_ERROR

    /* Successful handshake: connection to remote. */
#define WEE_CHECK_HANDSHAKE_OK(__algo, __iterations, __output)          \
    hashtable_remove_all (output);                                      \
    hashtable_set (output, "response_code", "200");                     \
    hashtable_set (output, "output", __output);                         \
    relay_remote_network_close_connection (remote);                     \
    relay_remote_set_status (remote, RELAY_STATUS_CONNECTING);          \
    LONGS_EQUAL(WEECHAT_RC_OK,                                          \
                relay_remote_network_url_handshake_cb (                 \
                    remote, NULL, "url", NULL, output));                \
    LONGS_EQUAL(RELAY_STATUS_CONNECTING, remote->status);               \
    LONGS_EQUAL(__algo, remote->password_hash_algo);                    \
    LONGS_EQUAL(__iterations, remote->password_hash_iterations);        \
    LONGS_EQUAL(0, remote->totp);                                       \
    CHECK(remote->hook_connect);

    WEE_CHECK_HANDSHAKE_OK(
        RELAY_AUTH_PASSWORD_HASH_SHA512, 100000,
        "{\"password_hash_algo\": \"sha512\", "
        "\"password_hash_iterations\": 100000, \"totp\": false}");

    /* Min and max number of iterations. */
    WEE_CHECK_HANDSHAKE_OK(
        RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA512, 1,
        "{\"password_hash_algo\": \"pbkdf2+sha512\", "
        "\"password_hash_iterations\": 1, \"totp\": false}");
    WEE_CHECK_HANDSHAKE_OK(
        RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA512, 1000000,
        "{\"password_hash_algo\": \"pbkdf2+sha512\", "
        "\"password_hash_iterations\": 1000000, \"totp\": false}");

#undef WEE_CHECK_HANDSHAKE_OK

    hashtable_free (output);
}

/*
 * Test functions:
 *   relay_remote_network_connect
 */

TEST(RelayRemoteNetworkWithSocket, Connect)
{
    int sock;

    LONGS_EQUAL(0, relay_remote_network_connect (NULL));

    /* Already connected. */
    LONGS_EQUAL(0, relay_remote_network_connect (remote));
    POINTERS_EQUAL(NULL, remote->hook_url_handshake);

    /* Handshake started with the URL. */
    sock = remote->sock;
    remote->sock = -1;
    LONGS_EQUAL(1, relay_remote_network_connect (remote));
    LONGS_EQUAL(RELAY_STATUS_CONNECTING, remote->status);
    CHECK(remote->hook_url_handshake);
    STRCMP_EQUAL("http://localhost:1/api/handshake",
                 HOOK_URL(remote->hook_url_handshake, url));
    remote->sock = sock;
}
