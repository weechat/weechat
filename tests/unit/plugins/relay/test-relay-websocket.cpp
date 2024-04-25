/*
 * test-relay-websocket.cpp - test websocket functions
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

#include "tests/tests.h"

extern "C"
{
#include <string.h>
#include <zlib.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-http.h"
#include "src/plugins/relay/relay-websocket.h"

extern void relay_websocket_deflate_free_stream_deflate (struct t_relay_websocket_deflate *ws_deflate);
extern void relay_websocket_deflate_free_stream_inflate (struct t_relay_websocket_deflate *ws_deflate);
extern char *relay_websocket_deflate (const void *data, size_t size, z_stream *strm,
                                      size_t *size_compressed);
extern char *relay_websocket_inflate (const void *data, size_t size, z_stream *strm,
                                      size_t *size_decompressed);
}

TEST_GROUP(RelayWebsocket)
{
};

/*
 * Tests functions:
 *   relay_websocket_deflate_alloc
 *   relay_websocket_deflate_init_stream_deflate
 *   relay_websocket_deflate_free_stream_deflate
 *   relay_websocket_deflate_init_stream_inflate
 *   relay_websocket_deflate_free_stream_inflate
 *   relay_websocket_deflate_free
 */

TEST(RelayWebsocket, DeflateAllocFree)
{
    struct t_relay_websocket_deflate *ws_deflate;

    ws_deflate = relay_websocket_deflate_alloc ();
    LONGS_EQUAL(0, ws_deflate->enabled);
    LONGS_EQUAL(0, ws_deflate->server_context_takeover);
    LONGS_EQUAL(0, ws_deflate->server_context_takeover);
    LONGS_EQUAL(0, ws_deflate->window_bits_deflate);
    LONGS_EQUAL(0, ws_deflate->window_bits_inflate);
    POINTERS_EQUAL(NULL, ws_deflate->strm_deflate);
    POINTERS_EQUAL(NULL, ws_deflate->strm_inflate);

    ws_deflate->window_bits_deflate = 15;
    ws_deflate->window_bits_inflate = 15;

    ws_deflate->strm_deflate = (z_stream *)calloc (1, sizeof (*ws_deflate->strm_deflate));
    CHECK(ws_deflate->strm_deflate);
    relay_websocket_deflate_init_stream_deflate (ws_deflate);
    relay_websocket_deflate_free_stream_deflate (ws_deflate);
    POINTERS_EQUAL(NULL, ws_deflate->strm_deflate);

    ws_deflate->strm_inflate = (z_stream *)calloc (1, sizeof (*ws_deflate->strm_inflate));
    CHECK(ws_deflate->strm_inflate);
    relay_websocket_deflate_init_stream_inflate (ws_deflate);
    relay_websocket_deflate_free_stream_inflate (ws_deflate);
    POINTERS_EQUAL(NULL, ws_deflate->strm_inflate);

    relay_websocket_deflate_free (ws_deflate);

    /* test free of NULL websocket deflate */
    relay_websocket_deflate_free (NULL);
}

/*
 * Tests functions:
 *   relay_websocket_is_valid_http_get
 */

TEST(RelayWebsocket, IsValidHttpGet)
{
    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_WEECHAT, NULL));
    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_WEECHAT, ""));
    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_WEECHAT, "xxx"));
    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_WEECHAT, "GET /api\r\n"));
    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_WEECHAT, "GET /api test\r\n"));
    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_WEECHAT, "GET /api HTTP/1.1\r\n"));
    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_API, "GET /weechat\r\n"));
    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_API, "GET /weechat test\r\n"));
    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_API, "GET /weechat HTTP/1.1\r\n"));

    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_WEECHAT, "GET /weechat test\r\n"));
    LONGS_EQUAL(0, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_API, "GET /api test\r\n"));

    LONGS_EQUAL(1, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_WEECHAT, "GET /weechat\r\n"));
    LONGS_EQUAL(1, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_WEECHAT, "GET /weechat HTTP/1.1\r\n"));
    LONGS_EQUAL(1, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_API, "GET /api\r\n"));
    LONGS_EQUAL(1, relay_websocket_is_valid_http_get (RELAY_PROTOCOL_API, "GET /api HTTP/1.1\r\n"));
}

/*
 * Tests functions:
 *   relay_websocket_client_handshake_valid
 *   relay_websocket_build_handshake
 */

TEST(RelayWebsocket, ClientHandshakeValid)
{
    struct t_relay_http_request *request;
    char *str;

    LONGS_EQUAL(-1, relay_websocket_client_handshake_valid (NULL));

    request = relay_http_request_alloc ();
    CHECK(request);

    LONGS_EQUAL(-1, relay_websocket_client_handshake_valid (NULL));

    LONGS_EQUAL(-1, relay_websocket_client_handshake_valid (request));
    hashtable_set (request->headers, "upgrade", NULL);
    LONGS_EQUAL(-1, relay_websocket_client_handshake_valid (request));
    hashtable_set (request->headers, "upgrade", "test");
    LONGS_EQUAL(-1, relay_websocket_client_handshake_valid (request));
    hashtable_set (request->headers, "upgrade", "websocket");
    LONGS_EQUAL(-1, relay_websocket_client_handshake_valid (request));
    hashtable_set (request->headers, "sec-websocket-key", NULL);
    LONGS_EQUAL(-1, relay_websocket_client_handshake_valid (request));
    hashtable_set (request->headers, "sec-websocket-key", "CI1sXhf/u2o34BfWK7NeIg==");
    LONGS_EQUAL(0, relay_websocket_client_handshake_valid (request));

    POINTERS_EQUAL(NULL, relay_websocket_build_handshake (NULL));

    WEE_TEST_STR(
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: fhLJYtv//ugX2vQXpifQgByRZ5Y=\r\n"
        "\r\n",
        relay_websocket_build_handshake (request));

    config_file_option_set (relay_config_network_websocket_allowed_origins, "example.com", 1);
    LONGS_EQUAL(-2, relay_websocket_client_handshake_valid (request));
    hashtable_set (request->headers, "origin", NULL);
    LONGS_EQUAL(-2, relay_websocket_client_handshake_valid (request));
    hashtable_set (request->headers, "origin", "weechat.org");
    LONGS_EQUAL(-2, relay_websocket_client_handshake_valid (request));
    hashtable_set (request->headers, "origin", "example.com");
    LONGS_EQUAL(0, relay_websocket_client_handshake_valid (request));

    relay_websocket_parse_extensions (
        "permessage-deflate; client_max_window_bits",
        request->ws_deflate);
    WEE_TEST_STR(
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: fhLJYtv//ugX2vQXpifQgByRZ5Y=\r\n"
        "Sec-WebSocket-Extensions: permessage-deflate; server_max_window_bits=15; client_max_window_bits=15\r\n"
        "\r\n",
        relay_websocket_build_handshake (request));

    relay_websocket_parse_extensions (
        "permessage-deflate; client_max_window_bits = 12; server_no_context_takeover",
        request->ws_deflate);
    WEE_TEST_STR(
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: fhLJYtv//ugX2vQXpifQgByRZ5Y=\r\n"
        "Sec-WebSocket-Extensions: permessage-deflate; server_no_context_takeover; server_max_window_bits=15; client_max_window_bits=12\r\n"
        "\r\n",
        relay_websocket_build_handshake (request));

    relay_websocket_parse_extensions (
        "permessage-deflate; client_max_window_bits = 12; server_max_window_bits=8; client_no_context_takeover; server_no_context_takeover",
        request->ws_deflate);
    WEE_TEST_STR(
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: fhLJYtv//ugX2vQXpifQgByRZ5Y=\r\n"
        "Sec-WebSocket-Extensions: permessage-deflate; server_no_context_takeover; client_no_context_takeover; server_max_window_bits=8; client_max_window_bits=12\r\n"
        "\r\n",
        relay_websocket_build_handshake (request));

    relay_http_request_free (request);
}

/*
 * Tests functions:
 *   relay_websocket_parse_extensions
 */

TEST(RelayWebsocket, ParseExtensions)
{
    struct t_relay_websocket_deflate ws_deflate;

    relay_websocket_parse_extensions (NULL, NULL);
    relay_websocket_parse_extensions ("test", NULL);
    relay_websocket_parse_extensions (NULL, &ws_deflate);

    memset (&ws_deflate, 0, sizeof (ws_deflate));
    relay_websocket_parse_extensions ("test", &ws_deflate);
    LONGS_EQUAL(0, ws_deflate.enabled);

    memset (&ws_deflate, 0, sizeof (ws_deflate));
    relay_websocket_parse_extensions ("permessage-deflate", &ws_deflate);
    LONGS_EQUAL(1, ws_deflate.enabled);
    LONGS_EQUAL(1, ws_deflate.server_context_takeover);
    LONGS_EQUAL(1, ws_deflate.client_context_takeover);
    LONGS_EQUAL(15, ws_deflate.window_bits_deflate);
    LONGS_EQUAL(15, ws_deflate.window_bits_inflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_deflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_inflate);

    memset (&ws_deflate, 0, sizeof (ws_deflate));
    relay_websocket_parse_extensions ("permessage-deflate; client_max_window_bits",
                                      &ws_deflate);
    LONGS_EQUAL(1, ws_deflate.enabled);
    LONGS_EQUAL(1, ws_deflate.server_context_takeover);
    LONGS_EQUAL(1, ws_deflate.client_context_takeover);
    LONGS_EQUAL(15, ws_deflate.window_bits_deflate);
    LONGS_EQUAL(15, ws_deflate.window_bits_inflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_deflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_inflate);

    /* client_max_window_bits < 8 (min value) */
    memset (&ws_deflate, 0, sizeof (ws_deflate));
    relay_websocket_parse_extensions (
        "permessage-deflate; client_max_window_bits=4",
        &ws_deflate);
    LONGS_EQUAL(1, ws_deflate.enabled);
    LONGS_EQUAL(1, ws_deflate.server_context_takeover);
    LONGS_EQUAL(1, ws_deflate.client_context_takeover);
    LONGS_EQUAL(15, ws_deflate.window_bits_deflate);
    LONGS_EQUAL(8, ws_deflate.window_bits_inflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_deflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_inflate);

    /* client_max_window_bits > 15 (max value) */
    memset (&ws_deflate, 0, sizeof (ws_deflate));
    relay_websocket_parse_extensions (
        "permessage-deflate; client_max_window_bits=30",
        &ws_deflate);
    LONGS_EQUAL(1, ws_deflate.enabled);
    LONGS_EQUAL(1, ws_deflate.server_context_takeover);
    LONGS_EQUAL(1, ws_deflate.client_context_takeover);
    LONGS_EQUAL(15, ws_deflate.window_bits_deflate);
    LONGS_EQUAL(15, ws_deflate.window_bits_inflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_deflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_inflate);

    /* invalid value for client_max_window_bits */
    memset (&ws_deflate, 0, sizeof (ws_deflate));
    relay_websocket_parse_extensions (
        "permessage-deflate; client_max_window_bits=test",
        &ws_deflate);
    LONGS_EQUAL(1, ws_deflate.enabled);
    LONGS_EQUAL(1, ws_deflate.server_context_takeover);
    LONGS_EQUAL(1, ws_deflate.client_context_takeover);
    LONGS_EQUAL(15, ws_deflate.window_bits_deflate);
    LONGS_EQUAL(15, ws_deflate.window_bits_inflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_deflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_inflate);

    memset (&ws_deflate, 0, sizeof (ws_deflate));
    relay_websocket_parse_extensions (
        "permessage-deflate; client_max_window_bits=9",
        &ws_deflate);
    LONGS_EQUAL(1, ws_deflate.enabled);
    LONGS_EQUAL(1, ws_deflate.server_context_takeover);
    LONGS_EQUAL(1, ws_deflate.client_context_takeover);
    LONGS_EQUAL(15, ws_deflate.window_bits_deflate);
    LONGS_EQUAL(9, ws_deflate.window_bits_inflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_deflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_inflate);

    memset (&ws_deflate, 0, sizeof (ws_deflate));
    relay_websocket_parse_extensions (
        "permessage-deflate; client_max_window_bits=9; server_max_window_bits=10",
        &ws_deflate);
    LONGS_EQUAL(1, ws_deflate.enabled);
    LONGS_EQUAL(1, ws_deflate.server_context_takeover);
    LONGS_EQUAL(1, ws_deflate.client_context_takeover);
    LONGS_EQUAL(10, ws_deflate.window_bits_deflate);
    LONGS_EQUAL(9, ws_deflate.window_bits_inflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_deflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_inflate);

    memset (&ws_deflate, 0, sizeof (ws_deflate));
    relay_websocket_parse_extensions (
        "permessage-deflate; client_max_window_bits=9; server_max_window_bits=10; "
        "server_no_context_takeover",
        &ws_deflate);
    LONGS_EQUAL(1, ws_deflate.enabled);
    LONGS_EQUAL(0, ws_deflate.server_context_takeover);
    LONGS_EQUAL(1, ws_deflate.client_context_takeover);
    LONGS_EQUAL(10, ws_deflate.window_bits_deflate);
    LONGS_EQUAL(9, ws_deflate.window_bits_inflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_deflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_inflate);

    memset (&ws_deflate, 0, sizeof (ws_deflate));
    relay_websocket_parse_extensions (
        "permessage-deflate; client_max_window_bits=9; server_max_window_bits=10; "
        "server_no_context_takeover; client_no_context_takeover",
        &ws_deflate);
    LONGS_EQUAL(1, ws_deflate.enabled);
    LONGS_EQUAL(0, ws_deflate.server_context_takeover);
    LONGS_EQUAL(0, ws_deflate.client_context_takeover);
    LONGS_EQUAL(10, ws_deflate.window_bits_deflate);
    LONGS_EQUAL(9, ws_deflate.window_bits_inflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_deflate);
    POINTERS_EQUAL(NULL, ws_deflate.strm_inflate);
}

/*
 * Tests functions:
 *   relay_websocket_deflate
 *   relay_websocket_inflate
 */

TEST(RelayWebsocket, Inflate)
{
    struct t_relay_websocket_deflate *ws_deflate;
    char payload[256], *payload_comp, *payload_decomp;
    size_t size_comp, size_decomp;
    int i;

    ws_deflate = relay_websocket_deflate_alloc ();
    CHECK(ws_deflate);

    ws_deflate->window_bits_deflate = 15;
    ws_deflate->window_bits_inflate = 15;

    ws_deflate->strm_deflate = (z_stream *)calloc (1, sizeof (*ws_deflate->strm_deflate));
    CHECK(ws_deflate->strm_deflate);
    LONGS_EQUAL(1, relay_websocket_deflate_init_stream_deflate (ws_deflate));

    ws_deflate->strm_inflate = (z_stream *)calloc (1, sizeof (*ws_deflate->strm_inflate));
    CHECK(ws_deflate->strm_inflate);
    LONGS_EQUAL(1, relay_websocket_deflate_init_stream_inflate (ws_deflate));

    for (i = 0; i < (int)sizeof (payload); i++)
    {
        payload[i] = i % 64;
    }

    POINTERS_EQUAL(NULL, relay_websocket_deflate (NULL, 0, NULL, NULL));
    POINTERS_EQUAL(NULL, relay_websocket_deflate (payload, 0, NULL, NULL));
    POINTERS_EQUAL(NULL, relay_websocket_deflate (payload, sizeof (payload),
                                                  NULL, NULL));
    POINTERS_EQUAL(NULL, relay_websocket_deflate (payload, sizeof (payload),
                                                  ws_deflate->strm_deflate, NULL));

    payload_comp = (char *)relay_websocket_deflate (payload, sizeof (payload),
                                                    ws_deflate->strm_deflate, &size_comp);
    CHECK(payload_comp);
    CHECK((size_comp > 0) && (size_comp < (int)sizeof (payload)));

    payload_decomp = (char *)relay_websocket_inflate (payload_comp, size_comp,
                                                      ws_deflate->strm_inflate, &size_decomp);
    CHECK(payload_decomp);
    LONGS_EQUAL(sizeof (payload), size_decomp);
    MEMCMP_EQUAL(payload, payload_decomp, sizeof (payload));

    free (payload_decomp);
    free (payload_comp);

    relay_websocket_deflate_free (ws_deflate);
}

/*
 * Tests functions:
 *   relay_websocket_decode_frame
 */

TEST(RelayWebsocket, DecodeFrame)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_websocket_encode_frame
 */

TEST(RelayWebsocket, EncodeFrame)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_websocket_deflate_print_log
 */

TEST(RelayWebsocket, DeflatePrintLog)
{
    /* TODO: write tests */
}
