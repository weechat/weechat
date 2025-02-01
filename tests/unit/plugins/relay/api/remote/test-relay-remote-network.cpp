/*
 * test-relay-remote-network.cpp - test network functions for relay remote
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
 * Tests functions:
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
 * Tests functions:
 *   relay_remote_network_close_connection
 */

TEST(RelayRemoteNetwork, CloseConnection)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_disconnect
 */

TEST(RelayRemoteNetwork, Disconnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_check_auth
 */

TEST(RelayRemoteNetwork, CheckAuth)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_send_data
 */

TEST(RelayRemoteNetwork, SendData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_send
 */

TEST(RelayRemoteNetwork, Send)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_send_json
 */

TEST(RelayRemoteNetwork, SendJson)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_recv_text
 */

TEST(RelayRemoteNetwork, RecvText)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_read_websocket_frames
 */

TEST(RelayRemoteNetwork, ReadWebsocketFrames)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_recv_buffer
 */

TEST(RelayRemoteNetwork, RecvBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_recv_cb
 */

TEST(RelayRemoteNetwork, RecvCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_connect_ws_auth
 */

TEST(RelayRemoteNetwork, ConnectWsAuth)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_connect_cb
 */

TEST(RelayRemoteNetwork, ConnectCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_network_url_handshake_cb
 */

TEST(RelayRemoteNetwork, UrlHandshakeCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
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
 * Tests functions:
 *   relay_remote_network_connect
 */

TEST(RelayRemoteNetwork, Connect)
{
    /* TODO: write tests */
}
