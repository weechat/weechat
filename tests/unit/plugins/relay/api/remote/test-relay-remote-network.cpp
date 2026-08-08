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
#include <cjson/cJSON.h>
#include "src/core/core-config-file.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-auth.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-remote.h"
#include "src/plugins/relay/api/remote/relay-remote-network.h"

extern char *relay_remote_network_get_url_resource (struct t_relay_remote *remote,
                                                    const char *resource);
extern char *relay_remote_network_get_handshake_request ();
}

struct t_relay_remote *ptr_relay_remote = NULL;
struct t_relay_remote *ptr_relay_remote2 = NULL;

TEST_GROUP(RelayRemoteNetwork)
{
};

TEST_GROUP(RelayRemoteNetworkWithRemote)
{
    void setup ()
    {
        /* disable auto-open of relay buffer */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);

        /* create two relay remotes */
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

        /* restore auto-open of relay buffer */
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
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_disconnect
 */

TEST(RelayRemoteNetwork, Disconnect)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_check_auth
 */

TEST(RelayRemoteNetwork, CheckAuth)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_send_data
 */

TEST(RelayRemoteNetwork, SendData)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_send
 */

TEST(RelayRemoteNetwork, Send)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_send_json
 */

TEST(RelayRemoteNetwork, SendJson)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_recv_text
 */

TEST(RelayRemoteNetwork, RecvText)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_read_websocket_frames
 */

TEST(RelayRemoteNetwork, ReadWebsocketFrames)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_recv_buffer
 */

TEST(RelayRemoteNetwork, RecvBuffer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_recv_cb
 */

TEST(RelayRemoteNetwork, RecvCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_connect_ws_auth
 */

TEST(RelayRemoteNetwork, ConnectWsAuth)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_connect_cb
 */

TEST(RelayRemoteNetwork, ConnectCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_network_url_handshake_cb
 */

TEST(RelayRemoteNetwork, UrlHandshakeCb)
{
    /* TODO: write tests */
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

/*
 * Test functions:
 *   relay_remote_network_connect
 */

TEST(RelayRemoteNetwork, Connect)
{
    /* TODO: write tests */
}
