/*
 * test-relay-http.cpp - test HTTP functions
 *
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <ctype.h>
#include <time.h>
#include <gcrypt.h>
#include "src/core/core-config-file.h"
#include "src/core/core-crypto.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-http.h"
#include "src/plugins/relay/relay-websocket.h"
#include "src/plugins/weechat-plugin.h"

extern char *relay_http_url_decode (const char *url);
extern void relay_http_parse_path (const char *url,
                                   char ***paths, int *num_paths,
                                   struct t_hashtable *params);
extern int relay_http_parse_header (struct t_relay_http_request *request,
                                    const char *header);
extern void relay_http_add_to_body (struct t_relay_http_request *request,
                                    char **partial_message);
extern int relay_http_get_auth_status (struct t_relay_client *client);
extern char *relay_http_compress (struct t_relay_http_request *request,
                                  const char *data, int data_size,
                                  int *compressed_size,
                                  char *http_content_encoding,
                                  int http_content_encoding_size);
extern int relay_http_parse_response_code (struct t_relay_http_response *response,
                                           const char *response_code);
extern int relay_http_parse_response_header (struct t_relay_http_response *response,
                                             const char *header);
}

#define WEE_PARSE_PATH(__path)                                  \
    paths = (char **)0x1;                                       \
    num_paths = -1;                                             \
    hashtable_remove_all (params);                              \
    relay_http_parse_path (__path, &paths, &num_paths, params);

TEST_GROUP(RelayHttp)
{
};

/*
 * Tests functions:
 *   relay_http_request_alloc
 *   relay_http_request_reinit
 *   relay_http_request_free
 */

TEST(RelayHttp, RequestAllocReinitFree)
{
    struct t_relay_http_request *request;

    request = relay_http_request_alloc ();
    CHECK(request);

    LONGS_EQUAL(RELAY_HTTP_METHOD, request->status);
    CHECK(request->raw);
    STRCMP_EQUAL("", *(request->raw));
    POINTERS_EQUAL(NULL, request->method);
    POINTERS_EQUAL(NULL, request->path);
    POINTERS_EQUAL(NULL, request->path_items);
    LONGS_EQUAL(0, request->num_path_items);
    CHECK(request->params);
    LONGS_EQUAL(0, request->params->items_count);
    POINTERS_EQUAL(NULL, request->http_version);
    CHECK(request->headers);
    LONGS_EQUAL(0, request->headers->items_count);
    CHECK(request->accept_encoding);
    LONGS_EQUAL(0, request->accept_encoding->items_count);
    CHECK(request->ws_deflate);
    LONGS_EQUAL(0, request->ws_deflate->enabled);
    LONGS_EQUAL(0, request->ws_deflate->server_context_takeover);
    LONGS_EQUAL(0, request->ws_deflate->server_context_takeover);
    LONGS_EQUAL(0, request->ws_deflate->window_bits_deflate);
    LONGS_EQUAL(0, request->ws_deflate->window_bits_inflate);
    POINTERS_EQUAL(NULL, request->ws_deflate->strm_deflate);
    POINTERS_EQUAL(NULL, request->ws_deflate->strm_inflate);
    LONGS_EQUAL(0, request->content_length);
    LONGS_EQUAL(0, request->body_size);
    POINTERS_EQUAL(NULL, request->body);

    request->status = RELAY_HTTP_HEADERS;
    string_dyn_concat (request->raw, "test", -1);
    request->method = strdup ("test");
    request->path = strdup ("test");
    request->path_items = string_split ("test,1,2,3", ",", NULL, 0, 0, &request->num_path_items);
    hashtable_set (request->params, "test", "value");
    request->http_version = strdup ("HTTP/1.1");
    hashtable_set (request->headers, "x-test", "value");
    hashtable_set (request->accept_encoding, "gzip", "");
    request->ws_deflate->enabled = 1;
    request->content_length = 100;
    request->body_size = 16;
    request->body = (char *)malloc (16);
    memset (request->body, 0, 16);

    relay_http_request_reinit (request);

    LONGS_EQUAL(RELAY_HTTP_METHOD, request->status);
    CHECK(request->raw);
    STRCMP_EQUAL("", *(request->raw));
    POINTERS_EQUAL(NULL, request->method);
    POINTERS_EQUAL(NULL, request->path);
    POINTERS_EQUAL(NULL, request->path_items);
    LONGS_EQUAL(0, request->num_path_items);
    CHECK(request->params);
    LONGS_EQUAL(0, request->params->items_count);
    POINTERS_EQUAL(NULL, request->http_version);
    CHECK(request->headers);
    LONGS_EQUAL(0, request->headers->items_count);
    CHECK(request->accept_encoding);
    LONGS_EQUAL(0, request->accept_encoding->items_count);
    CHECK(request->ws_deflate);
    LONGS_EQUAL(0, request->ws_deflate->enabled);
    LONGS_EQUAL(0, request->ws_deflate->server_context_takeover);
    LONGS_EQUAL(0, request->ws_deflate->server_context_takeover);
    LONGS_EQUAL(0, request->ws_deflate->window_bits_deflate);
    LONGS_EQUAL(0, request->ws_deflate->window_bits_inflate);
    POINTERS_EQUAL(NULL, request->ws_deflate->strm_deflate);
    POINTERS_EQUAL(NULL, request->ws_deflate->strm_inflate);
    LONGS_EQUAL(0, request->content_length);
    LONGS_EQUAL(0, request->body_size);
    POINTERS_EQUAL(NULL, request->body);

    relay_http_request_free (request);
}

/*
 * Tests functions:
 *   relay_http_url_decode
 */

TEST(RelayHttp, UrlDecode)
{
    char *str;

    POINTERS_EQUAL(NULL, relay_http_url_decode (NULL));
    WEE_TEST_STR("", relay_http_url_decode (""));
    WEE_TEST_STR("test", relay_http_url_decode ("test"));
    WEE_TEST_STR("%", relay_http_url_decode ("%"));
    WEE_TEST_STR("%%", relay_http_url_decode ("%%"));
    WEE_TEST_STR("%test", relay_http_url_decode ("%test"));
    WEE_TEST_STR("#test", relay_http_url_decode ("#test"));
    WEE_TEST_STR("#test", relay_http_url_decode ("%23test"));
    WEE_TEST_STR("%#test", relay_http_url_decode ("%%23test"));
    WEE_TEST_STR("%#test hello", relay_http_url_decode ("%%23test%20hello"));
    WEE_TEST_STR("test+z", relay_http_url_decode ("test%2bz"));
    WEE_TEST_STR("test+z", relay_http_url_decode ("test%2Bz"));
    WEE_TEST_STR("test*", relay_http_url_decode ("test%2a"));
}

/*
 * Tests functions:
 *   relay_http_get_param_boolean
 */

TEST(RelayHttp, GetParamBoolean)
{
    struct t_relay_http_request *request;

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api/test?key1=true&key2=1&key3=off");
    LONGS_EQUAL(1, relay_http_get_param_boolean (request, "key1", 0));
    LONGS_EQUAL(1, relay_http_get_param_boolean (request, "key1", 1));
    LONGS_EQUAL(1, relay_http_get_param_boolean (request, "key2", 0));
    LONGS_EQUAL(1, relay_http_get_param_boolean (request, "key2", 1));
    LONGS_EQUAL(0, relay_http_get_param_boolean (request, "key3", 0));
    LONGS_EQUAL(0, relay_http_get_param_boolean (request, "key3", 1));
    LONGS_EQUAL(0, relay_http_get_param_boolean (request, "xxx", 0));
    LONGS_EQUAL(1, relay_http_get_param_boolean (request, "xxx", 1));
    relay_http_request_free (request);
}

/*
 * Tests functions:
 *   relay_http_get_param_long
 */

TEST(RelayHttp, GetParamLong)
{
    struct t_relay_http_request *request;

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api/test?key1=123&key2=-4&key3=abc");
    LONGS_EQUAL(123, relay_http_get_param_long (request, "key1", 8));
    LONGS_EQUAL(-4, relay_http_get_param_long (request, "key2", 8));
    LONGS_EQUAL(8, relay_http_get_param_long (request, "key3", 8));
    LONGS_EQUAL(99, relay_http_get_param_long (request, "xxx", 99));
    relay_http_request_free (request);
}

/*
 * Tests functions:
 *   relay_http_parse_path
 */

TEST(RelayHttp, ParsePath)
{
    char **paths;
    int num_paths;
    struct t_hashtable *params;

    params = hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);

    WEE_PARSE_PATH(NULL);
    POINTERS_EQUAL(NULL, paths);
    LONGS_EQUAL(0, num_paths);
    LONGS_EQUAL(0, params->items_count);

    WEE_PARSE_PATH("");
    POINTERS_EQUAL(NULL, paths);
    LONGS_EQUAL(0, num_paths);
    LONGS_EQUAL(0, params->items_count);

    WEE_PARSE_PATH("api");
    CHECK(paths);
    LONGS_EQUAL(1, num_paths);
    STRCMP_EQUAL("api", paths[0]);
    POINTERS_EQUAL(NULL, paths[1]);
    LONGS_EQUAL(0, params->items_count);

    WEE_PARSE_PATH("/api");
    CHECK(paths);
    LONGS_EQUAL(1, num_paths);
    STRCMP_EQUAL("api", paths[0]);
    POINTERS_EQUAL(NULL, paths[1]);
    LONGS_EQUAL(0, params->items_count);

    WEE_PARSE_PATH("/api/");
    CHECK(paths);
    LONGS_EQUAL(1, num_paths);
    STRCMP_EQUAL("api", paths[0]);
    POINTERS_EQUAL(NULL, paths[1]);
    LONGS_EQUAL(0, params->items_count);

    WEE_PARSE_PATH("/api/buffers");
    CHECK(paths);
    LONGS_EQUAL(2, num_paths);
    STRCMP_EQUAL("api", paths[0]);
    STRCMP_EQUAL("buffers", paths[1]);
    POINTERS_EQUAL(NULL, paths[2]);
    LONGS_EQUAL(0, params->items_count);

    WEE_PARSE_PATH("/api/buffers?");
    CHECK(paths);
    LONGS_EQUAL(2, num_paths);
    STRCMP_EQUAL("api", paths[0]);
    STRCMP_EQUAL("buffers", paths[1]);
    POINTERS_EQUAL(NULL, paths[2]);
    LONGS_EQUAL(0, params->items_count);

    WEE_PARSE_PATH("/api/buffers?param");
    CHECK(paths);
    LONGS_EQUAL(2, num_paths);
    STRCMP_EQUAL("api", paths[0]);
    STRCMP_EQUAL("buffers", paths[1]);
    POINTERS_EQUAL(NULL, paths[2]);
    LONGS_EQUAL(1, params->items_count);
    STRCMP_EQUAL("", (const char *)hashtable_get (params, "param"));

    WEE_PARSE_PATH("/api/buffers?param=");
    CHECK(paths);
    LONGS_EQUAL(2, num_paths);
    STRCMP_EQUAL("api", paths[0]);
    STRCMP_EQUAL("buffers", paths[1]);
    POINTERS_EQUAL(NULL, paths[2]);
    LONGS_EQUAL(1, params->items_count);
    STRCMP_EQUAL("", (const char *)hashtable_get (params, "param"));

    WEE_PARSE_PATH("/api/buffers?param=off");
    CHECK(paths);
    LONGS_EQUAL(2, num_paths);
    STRCMP_EQUAL("api", paths[0]);
    STRCMP_EQUAL("buffers", paths[1]);
    POINTERS_EQUAL(NULL, paths[2]);
    LONGS_EQUAL(1, params->items_count);
    STRCMP_EQUAL("off", (const char *)hashtable_get (params, "param"));

    WEE_PARSE_PATH("/api/buffers?param=off&test=value2");
    CHECK(paths);
    LONGS_EQUAL(2, num_paths);
    STRCMP_EQUAL("api", paths[0]);
    STRCMP_EQUAL("buffers", paths[1]);
    POINTERS_EQUAL(NULL, paths[2]);
    LONGS_EQUAL(2, params->items_count);
    STRCMP_EQUAL("off", (const char *)hashtable_get (params, "param"));
    STRCMP_EQUAL("value2", (const char *)hashtable_get (params, "test"));

    WEE_PARSE_PATH("/api/buffers/irc.libera.%23weechat?param=off&test=value%202");
    CHECK(paths);
    LONGS_EQUAL(3, num_paths);
    STRCMP_EQUAL("api", paths[0]);
    STRCMP_EQUAL("buffers", paths[1]);
    STRCMP_EQUAL("irc.libera.#weechat", paths[2]);
    POINTERS_EQUAL(NULL, paths[3]);
    LONGS_EQUAL(2, params->items_count);
    STRCMP_EQUAL("off", (const char *)hashtable_get (params, "param"));
    STRCMP_EQUAL("value 2", (const char *)hashtable_get (params, "test"));

    hashtable_free (params);
}

/*
 * Tests functions:
 *   relay_http_parse_method_path
 */

TEST(RelayHttp, ParseMethodPath)
{
    struct t_relay_http_request *request;

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (NULL, NULL);
    relay_http_parse_method_path (request, NULL);
    LONGS_EQUAL(RELAY_HTTP_METHOD, request->status);
    STRCMP_EQUAL("", *(request->raw));
    POINTERS_EQUAL(NULL, request->path);
    POINTERS_EQUAL(NULL, request->path_items);
    LONGS_EQUAL(0, request->num_path_items);
    LONGS_EQUAL(0, request->params->items_count);
    POINTERS_EQUAL(NULL, request->http_version);
    LONGS_EQUAL(0, request->headers->items_count);
    LONGS_EQUAL(0, request->accept_encoding->items_count);
    LONGS_EQUAL(0, request->content_length);
    LONGS_EQUAL(0, request->body_size);
    POINTERS_EQUAL(NULL, request->body);
    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "");
    LONGS_EQUAL(RELAY_HTTP_METHOD, request->status);
    STRCMP_EQUAL("", *(request->raw));
    POINTERS_EQUAL(NULL, request->path);
    POINTERS_EQUAL(NULL, request->path_items);
    LONGS_EQUAL(0, request->num_path_items);
    LONGS_EQUAL(0, request->params->items_count);
    POINTERS_EQUAL(NULL, request->http_version);
    LONGS_EQUAL(0, request->headers->items_count);
    LONGS_EQUAL(0, request->accept_encoding->items_count);
    LONGS_EQUAL(0, request->content_length);
    LONGS_EQUAL(0, request->body_size);
    POINTERS_EQUAL(NULL, request->body);
    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET");
    LONGS_EQUAL(RELAY_HTTP_END, request->status);
    STRCMP_EQUAL("GET\n", *(request->raw));
    POINTERS_EQUAL(NULL, request->path);
    POINTERS_EQUAL(NULL, request->path_items);
    LONGS_EQUAL(0, request->num_path_items);
    LONGS_EQUAL(0, request->params->items_count);
    POINTERS_EQUAL(NULL, request->http_version);
    LONGS_EQUAL(0, request->headers->items_count);
    LONGS_EQUAL(0, request->accept_encoding->items_count);
    LONGS_EQUAL(0, request->content_length);
    LONGS_EQUAL(0, request->body_size);
    POINTERS_EQUAL(NULL, request->body);
    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, request->status);
    STRCMP_EQUAL("GET /api\n", *(request->raw));
    STRCMP_EQUAL("/api", request->path);
    CHECK(request->path_items);
    STRCMP_EQUAL("api", request->path_items[0]);
    POINTERS_EQUAL(NULL, request->path_items[1]);
    LONGS_EQUAL(1, request->num_path_items);
    LONGS_EQUAL(0, request->params->items_count);
    POINTERS_EQUAL(NULL, request->http_version);
    LONGS_EQUAL(0, request->headers->items_count);
    LONGS_EQUAL(0, request->accept_encoding->items_count);
    LONGS_EQUAL(0, request->content_length);
    LONGS_EQUAL(0, request->body_size);
    POINTERS_EQUAL(NULL, request->body);
    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api/buffers HTTP/1.1");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, request->status);
    STRCMP_EQUAL("GET /api/buffers HTTP/1.1\n", *(request->raw));
    STRCMP_EQUAL("/api/buffers", request->path);
    CHECK(request->path_items);
    STRCMP_EQUAL("api", request->path_items[0]);
    STRCMP_EQUAL("buffers", request->path_items[1]);
    POINTERS_EQUAL(NULL, request->path_items[2]);
    LONGS_EQUAL(2, request->num_path_items);
    LONGS_EQUAL(0, request->params->items_count);
    STRCMP_EQUAL("HTTP/1.1", request->http_version);
    LONGS_EQUAL(0, request->headers->items_count);
    LONGS_EQUAL(0, request->accept_encoding->items_count);
    LONGS_EQUAL(0, request->content_length);
    LONGS_EQUAL(0, request->body_size);
    POINTERS_EQUAL(NULL, request->body);
    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api/buffers?test=1&var=abc HTTP/1.1");
    /* do it 2 times, to be sure it has no side effect */
    relay_http_parse_method_path (request, "GET /api/buffers?test=1&var=abc HTTP/1.1");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, request->status);
    STRCMP_EQUAL("GET /api/buffers?test=1&var=abc HTTP/1.1\n"
                 "GET /api/buffers?test=1&var=abc HTTP/1.1\n",
                 *(request->raw));
    STRCMP_EQUAL("/api/buffers?test=1&var=abc", request->path);
    CHECK(request->path_items);
    STRCMP_EQUAL("api", request->path_items[0]);
    STRCMP_EQUAL("buffers", request->path_items[1]);
    POINTERS_EQUAL(NULL, request->path_items[2]);
    LONGS_EQUAL(2, request->num_path_items);
    LONGS_EQUAL(2, request->params->items_count);
    STRCMP_EQUAL("1", (const char *)hashtable_get (request->params, "test"));
    STRCMP_EQUAL("abc", (const char *)hashtable_get (request->params, "var"));
    STRCMP_EQUAL("HTTP/1.1", request->http_version);
    LONGS_EQUAL(0, request->headers->items_count);
    LONGS_EQUAL(0, request->accept_encoding->items_count);
    LONGS_EQUAL(0, request->content_length);
    LONGS_EQUAL(0, request->body_size);
    POINTERS_EQUAL(NULL, request->body);
    relay_http_request_free (request);
}

/*
 * Tests functions:
 *   relay_http_parse_header
 */

TEST(RelayHttp, ParseHeader)
{
    struct t_relay_http_request *request;

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api/version");
    relay_http_parse_header (request, NULL);
    LONGS_EQUAL(RELAY_HTTP_END, request->status);
    STRCMP_EQUAL("GET /api/version\n"
                 "\n",
                 *(request->raw));
    LONGS_EQUAL(0, request->headers->items_count);
    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api/version");
    relay_http_parse_header (request, "");
    LONGS_EQUAL(RELAY_HTTP_END, request->status);
    STRCMP_EQUAL("GET /api/version\n"
                 "\n",
                 *(request->raw));
    LONGS_EQUAL(0, request->headers->items_count);
    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api/version");
    relay_http_parse_header (request, "Test");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, request->status);
    STRCMP_EQUAL("GET /api/version\n"
                 "Test\n",
                 *(request->raw));
    LONGS_EQUAL(0, request->headers->items_count);
    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api/version");
    relay_http_parse_header (request, "X-Test: value");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, request->status);
    STRCMP_EQUAL("GET /api/version\n"
                 "X-Test: value\n",
                 *(request->raw));
    LONGS_EQUAL(1, request->headers->items_count);
    STRCMP_EQUAL("value", (const char *)hashtable_get (request->headers, "x-test"));
    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api/version");
    relay_http_parse_header (request, "Accept-Encoding: gzip, zstd, br");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, request->status);
    STRCMP_EQUAL("GET /api/version\n"
                 "Accept-Encoding: gzip, zstd, br\n",
                 *(request->raw));
    LONGS_EQUAL(1, request->headers->items_count);
    STRCMP_EQUAL("gzip, zstd, br",
                 (const char *)hashtable_get (request->headers, "accept-encoding"));
    LONGS_EQUAL(3, request->accept_encoding->items_count);
    CHECK(hashtable_has_key (request->accept_encoding, "gzip"));
    CHECK(hashtable_has_key (request->accept_encoding, "zstd"));
    CHECK(hashtable_has_key (request->accept_encoding, "br"));
    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api/version");
    relay_http_parse_header (request, "Content-Length: 123");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, request->status);
    STRCMP_EQUAL("GET /api/version\n"
                 "Content-Length: 123\n",
                 *(request->raw));
    LONGS_EQUAL(1, request->headers->items_count);
    LONGS_EQUAL(123, request->content_length);
    relay_http_request_free (request);

    /* websocket request */
    request = relay_http_request_alloc ();
    CHECK(request);
    relay_http_parse_method_path (request, "GET /api HTTP/1.1");
    relay_http_parse_header (
        request,
        "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, request->status);
    STRCMP_EQUAL(
        "GET /api HTTP/1.1\n"
        "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\n",
        *(request->raw));
    LONGS_EQUAL(1, request->headers->items_count);
    CHECK(request->ws_deflate);
    LONGS_EQUAL(1, request->ws_deflate->enabled);
    LONGS_EQUAL(1, request->ws_deflate->server_context_takeover);
    LONGS_EQUAL(1, request->ws_deflate->server_context_takeover);
    LONGS_EQUAL(15, request->ws_deflate->window_bits_deflate);
    LONGS_EQUAL(15, request->ws_deflate->window_bits_inflate);
    POINTERS_EQUAL(NULL, request->ws_deflate->strm_deflate);
    POINTERS_EQUAL(NULL, request->ws_deflate->strm_inflate);
    relay_http_request_free (request);
}

/*
 * Tests functions:
 *   relay_http_add_to_body
 */

TEST(RelayHttp, AddToBody)
{
    struct t_relay_http_request *request;
    const char *body_part1 = "abc", *body_part2 = "defghij";
    char *partial_message;

    request = relay_http_request_alloc ();
    CHECK(request);
    LONGS_EQUAL(RELAY_HTTP_METHOD, request->status);
    relay_http_parse_method_path (request, "GET /api/version");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, request->status);
    relay_http_parse_header (request, "Content-Length: 10");
    relay_http_parse_header (request, "");
    LONGS_EQUAL(RELAY_HTTP_BODY, request->status);
    LONGS_EQUAL(10, request->content_length);
    LONGS_EQUAL(0, request->body_size);

    partial_message = strdup (body_part1);
    relay_http_add_to_body (request, &partial_message);
    LONGS_EQUAL(RELAY_HTTP_BODY, request->status);
    LONGS_EQUAL(3, request->body_size);
    MEMCMP_EQUAL("abc", request->body, 3);
    POINTERS_EQUAL(NULL, partial_message);

    partial_message = strdup (body_part2);
    relay_http_add_to_body (request, &partial_message);
    LONGS_EQUAL(RELAY_HTTP_END, request->status);
    LONGS_EQUAL(10, request->body_size);
    MEMCMP_EQUAL("abcdefghij", request->body, 10);
    POINTERS_EQUAL(NULL, partial_message);

    relay_http_request_free (request);

    request = relay_http_request_alloc ();
    CHECK(request);
    LONGS_EQUAL(RELAY_HTTP_METHOD, request->status);
    relay_http_parse_method_path (request, "GET /api/version");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, request->status);
    relay_http_parse_header (request, "Content-Length: 5");
    relay_http_parse_header (request, "");
    LONGS_EQUAL(RELAY_HTTP_BODY, request->status);
    LONGS_EQUAL(5, request->content_length);
    LONGS_EQUAL(0, request->body_size);

    partial_message = strdup (body_part1);
    relay_http_add_to_body (request, &partial_message);
    LONGS_EQUAL(RELAY_HTTP_BODY, request->status);
    LONGS_EQUAL(3, request->body_size);
    MEMCMP_EQUAL("abc", request->body, 3);
    POINTERS_EQUAL(NULL, partial_message);

    partial_message = strdup (body_part2);
    relay_http_add_to_body (request, &partial_message);
    LONGS_EQUAL(RELAY_HTTP_END, request->status);
    LONGS_EQUAL(5, request->body_size);
    MEMCMP_EQUAL("abcde", request->body, 5);
    STRCMP_EQUAL("fghij", partial_message);
    free (partial_message);

    relay_http_request_free (request);
}

/*
 * Tests functions:
 *   relay_http_get_auth_status
 */

TEST(RelayHttp, GetAuthStatus)
{
    struct t_relay_client *client;
    const char *good_pwd = "secret_password";
    const char *bad_pwd = "test";
    char *totp, *totp2, salt[1024], salt_pass[1024], hash[1024], hash_hexa[2048];
    char auth[4096], auth_base64[4096], auth_header[8192];
    time_t time_now;
    int hash_size;

    config_file_option_set (relay_config_network_password, good_pwd, 1);

    client = (struct t_relay_client *)calloc (1, sizeof (*client));
    CHECK(client);

    client->protocol = RELAY_PROTOCOL_API;

    client->http_req = relay_http_request_alloc ();

    LONGS_EQUAL(-1, relay_http_get_auth_status (client));
    hashtable_set (client->http_req->headers, "authorization", "Basic    ");
    LONGS_EQUAL(-2, relay_http_get_auth_status (client));
    hashtable_set (client->http_req->headers, "authorization", "Basic \u26c4");
    LONGS_EQUAL(-2, relay_http_get_auth_status (client));

    /* test invalid plain-text password ("test") */
    hashtable_set (client->http_req->headers, "authorization", "Basic cGxhaW46dGVzdA==");
    LONGS_EQUAL(-2, relay_http_get_auth_status (client));

    /* test valid plain-text password ("secret_password") */
    hashtable_set (client->http_req->headers,
                   "authorization",
                   "Basic  cGxhaW46c2VjcmV0X3Bhc3N3b3Jk");
    LONGS_EQUAL(0, relay_http_get_auth_status (client));

    /* test invalid hash: "SHA128" */
    time_now = time (NULL);
    snprintf (salt_pass, sizeof (salt_pass),
              "%ld%s", time_now, bad_pwd);
    LONGS_EQUAL(1, weecrypto_hash (salt_pass, strlen (salt_pass),
                                   GCRY_MD_SHA256, hash, &hash_size));
    LONGS_EQUAL(64, string_base_encode ("16", hash, hash_size, hash_hexa));
    snprintf (auth, sizeof (auth),
              "hash:sha128:%ld:%s",
              time_now,
              hash_hexa);
    string_base_encode ("64", auth, strlen (auth), auth_base64);
    snprintf (auth_header, sizeof (auth_header), "Basic %s", auth_base64);
    hashtable_set (client->http_req->headers, "authorization", auth_header);
    LONGS_EQUAL(-5, relay_http_get_auth_status (client));

    /* test invalid password hashed with SHA256: "test" */
    time_now = time (NULL);
    snprintf (salt_pass, sizeof (salt_pass),
              "%ld%s", time_now, bad_pwd);
    LONGS_EQUAL(1, weecrypto_hash (salt_pass, strlen (salt_pass),
                                   GCRY_MD_SHA256, hash, &hash_size));
    LONGS_EQUAL(64, string_base_encode ("16", hash, hash_size, hash_hexa));
    snprintf (auth, sizeof (auth),
              "hash:sha256:%ld:%s",
              time_now,
              hash_hexa);
    string_base_encode ("64", auth, strlen (auth), auth_base64);
    snprintf (auth_header, sizeof (auth_header), "Basic %s", auth_base64);
    hashtable_set (client->http_req->headers, "authorization", auth_header);
    LONGS_EQUAL(-2, relay_http_get_auth_status (client));

    /* test invalid password hashed with SHA512: "test" */
    time_now = time (NULL);
    snprintf (salt_pass, sizeof (salt_pass),
              "%ld%s", time_now, bad_pwd);
    LONGS_EQUAL(1, weecrypto_hash (salt_pass, strlen (salt_pass),
                                   GCRY_MD_SHA512, hash, &hash_size));
    LONGS_EQUAL(128, string_base_encode ("16", hash, hash_size, hash_hexa));
    snprintf (auth, sizeof (auth),
              "hash:sha512:%ld:%s",
              time_now,
              hash_hexa);
    string_base_encode ("64", auth, strlen (auth), auth_base64);
    snprintf (auth_header, sizeof (auth_header), "Basic %s", auth_base64);
    hashtable_set (client->http_req->headers, "authorization", auth_header);
    LONGS_EQUAL(-2, relay_http_get_auth_status (client));

    /* test valid password hashed with SHA256: "secret_password" but too old time (salt) */
    time_now = time (NULL) - 10;
    snprintf (salt_pass, sizeof (salt_pass),
              "%ld%s", time_now, good_pwd);
    LONGS_EQUAL(1, weecrypto_hash (salt_pass, strlen (salt_pass),
                                   GCRY_MD_SHA256, hash, &hash_size));
    LONGS_EQUAL(64, string_base_encode ("16", hash, hash_size, hash_hexa));
    snprintf (auth, sizeof (auth),
              "hash:sha256:%ld:%s",
              time_now,
              hash_hexa);
    string_base_encode ("64", auth, strlen (auth), auth_base64);
    snprintf (auth_header, sizeof (auth_header), "Basic %s", auth_base64);
    hashtable_set (client->http_req->headers, "authorization", auth_header);
    LONGS_EQUAL(-6, relay_http_get_auth_status (client));

    /* test valid password hashed with SHA256: "secret_password" */
    time_now = time (NULL);
    snprintf (salt_pass, sizeof (salt_pass),
              "%ld%s", time_now, good_pwd);
    LONGS_EQUAL(1, weecrypto_hash (salt_pass, strlen (salt_pass),
                                   GCRY_MD_SHA256, hash, &hash_size));
    LONGS_EQUAL(64, string_base_encode ("16", hash, hash_size, hash_hexa));
    snprintf (auth, sizeof (auth),
              "hash:sha256:%ld:%s",
              time_now,
              hash_hexa);
    string_base_encode ("64", auth, strlen (auth), auth_base64);
    snprintf (auth_header, sizeof (auth_header), "Basic %s", auth_base64);
    hashtable_set (client->http_req->headers, "authorization", auth_header);
    LONGS_EQUAL(0, relay_http_get_auth_status (client));

    /* test valid password hashed with SHA512: "secret_password" */
    time_now = time (NULL);
    snprintf (salt_pass, sizeof (salt_pass),
              "%ld%s", time_now, good_pwd);
    LONGS_EQUAL(1, weecrypto_hash (salt_pass, strlen (salt_pass),
                                   GCRY_MD_SHA512, hash, &hash_size));
    LONGS_EQUAL(128, string_base_encode ("16", hash, hash_size, hash_hexa));
    snprintf (auth, sizeof (auth),
              "hash:sha512:%ld:%s",
              time_now,
              hash_hexa);
    string_base_encode ("64", auth, strlen (auth), auth_base64);
    snprintf (auth_header, sizeof (auth_header), "Basic %s", auth_base64);
    hashtable_set (client->http_req->headers, "authorization", auth_header);
    LONGS_EQUAL(0, relay_http_get_auth_status (client));

    /* test invalid number of iterations */
    time_now = time (NULL);
    snprintf (salt, sizeof (salt), "%ld", time_now);
    LONGS_EQUAL(1, weecrypto_hash_pbkdf2 (good_pwd, strlen (good_pwd),
                                          GCRY_MD_SHA256,
                                          salt, strlen (salt),
                                          123,
                                          hash, &hash_size));
    LONGS_EQUAL(64, string_base_encode ("16", hash, hash_size, hash_hexa));
    snprintf (auth, sizeof (auth),
              "hash:pbkdf2+sha256:%ld:123:%s",
              time_now,
              hash_hexa);
    string_base_encode ("64", auth, strlen (auth), auth_base64);
    snprintf (auth_header, sizeof (auth_header), "Basic %s", auth_base64);
    hashtable_set (client->http_req->headers, "authorization", auth_header);
    LONGS_EQUAL(-7, relay_http_get_auth_status (client));

    /* test invalid password hashed with PBKDF2+SHA256: "test" */
    time_now = time (NULL);
    snprintf (salt, sizeof (salt), "%ld", time_now);
    LONGS_EQUAL(1, weecrypto_hash_pbkdf2 (bad_pwd, strlen (bad_pwd),
                                          GCRY_MD_SHA256,
                                          salt, strlen (salt),
                                          100000,
                                          hash, &hash_size));
    LONGS_EQUAL(64, string_base_encode ("16", hash, hash_size, hash_hexa));
    snprintf (auth, sizeof (auth),
              "hash:pbkdf2+sha256:%ld:100000:%s",
              time_now,
              hash_hexa);
    string_base_encode ("64", auth, strlen (auth), auth_base64);
    snprintf (auth_header, sizeof (auth_header), "Basic %s", auth_base64);
    hashtable_set (client->http_req->headers, "authorization", auth_header);
    LONGS_EQUAL(-2, relay_http_get_auth_status (client));

    /* test valid password hashed with PBKDF2+SHA256: "secret_password" */
    time_now = time (NULL);
    snprintf (salt, sizeof (salt), "%ld", time_now);
    LONGS_EQUAL(1, weecrypto_hash_pbkdf2 (good_pwd, strlen (good_pwd),
                                          GCRY_MD_SHA256,
                                          salt, strlen (salt),
                                          100000,
                                          hash, &hash_size));
    LONGS_EQUAL(64, string_base_encode ("16", hash, hash_size, hash_hexa));
    snprintf (auth, sizeof (auth),
              "hash:pbkdf2+sha256:%ld:100000:%s",
              time_now,
              hash_hexa);
    string_base_encode ("64", auth, strlen (auth), auth_base64);
    snprintf (auth_header, sizeof (auth_header), "Basic %s", auth_base64);
    hashtable_set (client->http_req->headers, "authorization", auth_header);
    LONGS_EQUAL(0, relay_http_get_auth_status (client));

    /* test valid password hashed with PBKDF2+SHA512: "secret_password" */
    time_now = time (NULL);
    snprintf (salt, sizeof (salt), "%ld", time_now);
    LONGS_EQUAL(1, weecrypto_hash_pbkdf2 (good_pwd, strlen (good_pwd),
                                          GCRY_MD_SHA512,
                                          salt, strlen (salt),
                                          100000,
                                          hash, &hash_size));
    LONGS_EQUAL(128, string_base_encode ("16", hash, hash_size, hash_hexa));
    snprintf (auth, sizeof (auth),
              "hash:pbkdf2+sha512:%ld:100000:%s",
              time_now,
              hash_hexa);
    string_base_encode ("64", auth, strlen (auth), auth_base64);
    snprintf (auth_header, sizeof (auth_header), "Basic %s", auth_base64);
    hashtable_set (client->http_req->headers, "authorization", auth_header);
    LONGS_EQUAL(0, relay_http_get_auth_status (client));

    /* test missing/invalid TOTP */
    config_file_option_set (relay_config_network_totp_secret, "secretbase32", 1);
    config_file_option_set (relay_config_network_totp_window, "1", 1);
    LONGS_EQUAL(-3, relay_http_get_auth_status (client));
    totp = hook_info_get (NULL, "totp_generate", "secretbase32");
    CHECK(totp);
    totp2 = strdup (totp);
    totp2[0] = (totp2[0] == '1') ? '2' : '1';
    hashtable_set (client->http_req->headers, "x-weechat-totp", totp2);
    LONGS_EQUAL(-4, relay_http_get_auth_status (client));
    hashtable_set (client->http_req->headers, "x-weechat-totp", totp);
    LONGS_EQUAL(0, relay_http_get_auth_status (client));
    free (totp);
    free (totp2);
    config_file_option_reset (relay_config_network_totp_secret, 1);
    config_file_option_reset (relay_config_network_totp_window, 1);

    config_file_option_reset (relay_config_network_password, 1);

    relay_http_request_free (client->http_req);
    free (client);
}

/*
 * Tests functions:
 *   relay_http_check_auth
 */

TEST(RelayHttp, CheckAuth)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_http_process_websocket
 */

TEST(RelayHttp, ProcessWebsocket)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_http_process_request
 */

TEST(RelayHttp, ProcessRequest)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_http_recv
 */

TEST(RelayHttp, Recv)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_http_compress
 */

TEST(RelayHttp, Compress)
{
    struct t_relay_http_request *request;
    char body[256], *buffer, encoding[256];
    int i, size;

    request = relay_http_request_alloc ();

    for (i = 0; i < (int)sizeof (body); i++)
    {
        body[i] = i % 64;
    }

    hashtable_remove_all (request->accept_encoding);
    size = -1;
    buffer = relay_http_compress (request, NULL, 0, &size, NULL, 0);
    POINTERS_EQUAL(NULL, buffer);
    LONGS_EQUAL(0, size);

    hashtable_remove_all (request->accept_encoding);
    size = -1;
    snprintf (encoding, sizeof (encoding), "test");
    buffer = relay_http_compress (request, NULL, 0, &size,
                                  encoding, sizeof (encoding));
    POINTERS_EQUAL(NULL, buffer);
    LONGS_EQUAL(0, size);
    STRCMP_EQUAL("", encoding);

    /* no "Accept-Encoding" header was received => no compression */
    hashtable_remove_all (request->accept_encoding);
    size = -1;
    snprintf (encoding, sizeof (encoding), "test");
    buffer = relay_http_compress (request, body, sizeof (body), &size,
                                  encoding, sizeof (encoding));
    POINTERS_EQUAL(NULL, buffer);
    LONGS_EQUAL(0, size);
    STRCMP_EQUAL("", encoding);

    /* "Accept-Encoding: gzip" => gzip compression */
    hashtable_remove_all (request->accept_encoding);
    hashtable_set (request->accept_encoding, "gzip", "");
    size = -1;
    snprintf (encoding, sizeof (encoding), "test");
    buffer = relay_http_compress (request, body, sizeof (body), &size,
                                  encoding, sizeof (encoding));
    CHECK(buffer);
    CHECK((size > 0) && (size < (int)sizeof (body)));
    STRCMP_EQUAL("Content-Encoding: gzip\r\n", encoding);
    free (buffer);

#ifdef HAVE_ZSTD
    /* "Accept-Encoding: gzip, zstd" => zstd compression */
    hashtable_remove_all (request->accept_encoding);
    hashtable_set (request->accept_encoding, "gzip", "");
    hashtable_set (request->accept_encoding, "zstd", "");
    size = -1;
    snprintf (encoding, sizeof (encoding), "test");
    buffer = relay_http_compress (request, body, sizeof (body), &size,
                                  encoding, sizeof (encoding));
    CHECK(buffer);
    CHECK((size > 0) && (size < (int)sizeof (body)));
    STRCMP_EQUAL("Content-Encoding: zstd\r\n", encoding);
    free (buffer);
#endif
}

/*
 * Tests functions:
 *   relay_http_send
 */

TEST(RelayHttp, Send)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_http_send_json
 */

TEST(RelayHttp, SendJson)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_http_send_error_json
 */

TEST(RelayHttp, SendErrorJson)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_http_response_alloc
 *   relay_http_response_free
 */

TEST(RelayHttp, ResponseAllocFree)
{
    struct t_relay_http_response *response;

    response = relay_http_response_alloc ();
    CHECK(response);

    LONGS_EQUAL(RELAY_HTTP_METHOD, response->status);
    POINTERS_EQUAL(NULL, response->http_version);
    LONGS_EQUAL(0, response->return_code);
    POINTERS_EQUAL(NULL, response->message);
    CHECK(response->headers);
    LONGS_EQUAL(0, response->headers->items_count);
    LONGS_EQUAL(0, response->content_length);
    LONGS_EQUAL(0, response->body_size);
    POINTERS_EQUAL(NULL, response->body);

    relay_http_response_free (response);

    /* test free of NULL response */
    relay_http_response_free (NULL);
}

/*
 * Tests functions:
 *   relay_http_parse_response_code
 */

TEST(RelayHttp, ParseResponseCode)
{
    struct t_relay_http_response *response;

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (NULL, NULL);
    LONGS_EQUAL(RELAY_HTTP_METHOD, response->status);
    POINTERS_EQUAL(NULL, response->http_version);
    LONGS_EQUAL(0, response->return_code);
    POINTERS_EQUAL(NULL, response->message);
    CHECK(response->headers);
    LONGS_EQUAL(0, response->headers->items_count);
    LONGS_EQUAL(0, response->content_length);
    LONGS_EQUAL(0, response->body_size);
    POINTERS_EQUAL(NULL, response->body);
    relay_http_response_free (response);

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, NULL);
    relay_http_parse_response_code (response, "");
    LONGS_EQUAL(RELAY_HTTP_END, response->status);
    POINTERS_EQUAL(NULL, response->http_version);
    LONGS_EQUAL(0, response->return_code);
    POINTERS_EQUAL(NULL, response->message);
    CHECK(response->headers);
    LONGS_EQUAL(0, response->headers->items_count);
    LONGS_EQUAL(0, response->content_length);
    LONGS_EQUAL(0, response->body_size);
    POINTERS_EQUAL(NULL, response->body);
    relay_http_response_free (response);

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, "HTTP/1.1");
    LONGS_EQUAL(RELAY_HTTP_END, response->status);
    POINTERS_EQUAL(NULL, response->http_version);
    LONGS_EQUAL(0, response->return_code);
    POINTERS_EQUAL(NULL, response->message);
    CHECK(response->headers);
    LONGS_EQUAL(0, response->headers->items_count);
    LONGS_EQUAL(0, response->content_length);
    LONGS_EQUAL(0, response->body_size);
    POINTERS_EQUAL(NULL, response->body);
    relay_http_response_free (response);

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, "HTTP/1.1 200");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, response->status);
    STRCMP_EQUAL("HTTP/1.1", response->http_version);
    LONGS_EQUAL(200, response->return_code);
    POINTERS_EQUAL(NULL, response->message);
    CHECK(response->headers);
    LONGS_EQUAL(0, response->headers->items_count);
    LONGS_EQUAL(0, response->content_length);
    LONGS_EQUAL(0, response->body_size);
    POINTERS_EQUAL(NULL, response->body);
    relay_http_response_free (response);

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, "HTTP/1.1 200 OK");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, response->status);
    STRCMP_EQUAL("HTTP/1.1", response->http_version);
    LONGS_EQUAL(200, response->return_code);
    STRCMP_EQUAL("OK", response->message);
    CHECK(response->headers);
    LONGS_EQUAL(0, response->headers->items_count);
    LONGS_EQUAL(0, response->content_length);
    LONGS_EQUAL(0, response->body_size);
    POINTERS_EQUAL(NULL, response->body);
    relay_http_response_free (response);

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, "HTTP/1.1 101 Switching Protocols");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, response->status);
    STRCMP_EQUAL("HTTP/1.1", response->http_version);
    LONGS_EQUAL(101, response->return_code);
    STRCMP_EQUAL("Switching Protocols", response->message);
    CHECK(response->headers);
    LONGS_EQUAL(0, response->headers->items_count);
    LONGS_EQUAL(0, response->content_length);
    LONGS_EQUAL(0, response->body_size);
    POINTERS_EQUAL(NULL, response->body);
    relay_http_response_free (response);


    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, "HTTP/1.1 401 Unauthorized");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, response->status);
    STRCMP_EQUAL("HTTP/1.1", response->http_version);
    LONGS_EQUAL(401, response->return_code);
    STRCMP_EQUAL("Unauthorized", response->message);
    CHECK(response->headers);
    LONGS_EQUAL(0, response->headers->items_count);
    LONGS_EQUAL(0, response->content_length);
    LONGS_EQUAL(0, response->body_size);
    POINTERS_EQUAL(NULL, response->body);
    relay_http_response_free (response);
}

/*
 * Tests functions:
 *   relay_http_parse_response_header
 */

TEST(RelayHttp, ParseResponseHeader)
{
    struct t_relay_http_response *response;

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, "HTTP/1.1 200 OK");
    relay_http_parse_response_header (response, NULL);
    LONGS_EQUAL(RELAY_HTTP_END, response->status);
    STRCMP_EQUAL("HTTP/1.1", response->http_version);
    LONGS_EQUAL(200, response->return_code);
    STRCMP_EQUAL("OK", response->message);
    LONGS_EQUAL(0, response->headers->items_count);
    relay_http_response_free (response);

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, "HTTP/1.1 200 OK");
    relay_http_parse_response_header (response, "");
    LONGS_EQUAL(RELAY_HTTP_END, response->status);
    STRCMP_EQUAL("HTTP/1.1", response->http_version);
    LONGS_EQUAL(200, response->return_code);
    STRCMP_EQUAL("OK", response->message);
    LONGS_EQUAL(0, response->headers->items_count);
    relay_http_response_free (response);

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, "HTTP/1.1 200 OK");
    relay_http_parse_response_header (response, "Test");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, response->status);
    STRCMP_EQUAL("HTTP/1.1", response->http_version);
    LONGS_EQUAL(200, response->return_code);
    STRCMP_EQUAL("OK", response->message);
    LONGS_EQUAL(0, response->headers->items_count);
    relay_http_response_free (response);

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, "HTTP/1.1 200 OK");
    relay_http_parse_response_header (response, "X-Test: value");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, response->status);
    STRCMP_EQUAL("HTTP/1.1", response->http_version);
    LONGS_EQUAL(200, response->return_code);
    STRCMP_EQUAL("OK", response->message);
    LONGS_EQUAL(1, response->headers->items_count);
    STRCMP_EQUAL("value", (const char *)hashtable_get (response->headers, "x-test"));
    relay_http_response_free (response);

    response = relay_http_response_alloc ();
    CHECK(response);
    relay_http_parse_response_code (response, "HTTP/1.1 200 OK");
    relay_http_parse_response_header (response, "Content-Length: 123");
    LONGS_EQUAL(RELAY_HTTP_HEADERS, response->status);
    STRCMP_EQUAL("HTTP/1.1", response->http_version);
    LONGS_EQUAL(200, response->return_code);
    STRCMP_EQUAL("OK", response->message);
    LONGS_EQUAL(1, response->headers->items_count);
    LONGS_EQUAL(123, response->content_length);
    relay_http_response_free (response);
}

/*
 * Tests functions:
 *   relay_http_parse_response
 */

TEST(RelayHttp, ParseResponse)
{
    struct t_relay_http_response *response;

    POINTERS_EQUAL(NULL, relay_http_parse_response (NULL));
    POINTERS_EQUAL(NULL, relay_http_parse_response (""));

    response = relay_http_parse_response ("invalid");
    CHECK(response);
    LONGS_EQUAL(RELAY_HTTP_METHOD, response->status);
    POINTERS_EQUAL(NULL, response->http_version);
    LONGS_EQUAL(0, response->return_code);
    POINTERS_EQUAL(NULL, response->message);
    CHECK(response->headers);
    LONGS_EQUAL(0, response->headers->items_count);
    LONGS_EQUAL(0, response->content_length);
    LONGS_EQUAL(0, response->body_size);
    POINTERS_EQUAL(NULL, response->body);
    relay_http_response_free (response);

    response = relay_http_parse_response ("invalid\r\n");
    CHECK(response);
    LONGS_EQUAL(RELAY_HTTP_END, response->status);
    POINTERS_EQUAL(NULL, response->http_version);
    LONGS_EQUAL(0, response->return_code);
    POINTERS_EQUAL(NULL, response->message);
    CHECK(response->headers);
    LONGS_EQUAL(0, response->headers->items_count);
    LONGS_EQUAL(0, response->content_length);
    LONGS_EQUAL(0, response->body_size);
    POINTERS_EQUAL(NULL, response->body);
    relay_http_response_free (response);

    response = relay_http_parse_response ("HTTP/1.1 200 OK\r\n"
                                          "Content-Type: application/json\r\n"
                                          "Content-Length: 13\r\n"
                                          "\r\n"
                                          "{\"test\": 123}");
    CHECK(response);
    LONGS_EQUAL(RELAY_HTTP_END, response->status);
    STRCMP_EQUAL("HTTP/1.1", response->http_version);
    LONGS_EQUAL(200, response->return_code);
    STRCMP_EQUAL("OK", response->message);
    CHECK(response->headers);
    LONGS_EQUAL(2, response->headers->items_count);
    STRCMP_EQUAL("application/json", (const char *)hashtable_get (response->headers,
                                                                  "content-type"));
    STRCMP_EQUAL("13", (const char *)hashtable_get (response->headers,
                                                    "content-length"));
    LONGS_EQUAL(13, response->content_length);
    LONGS_EQUAL(13, response->body_size);
    STRCMP_EQUAL("{\"test\": 123}", response->body);
    relay_http_response_free (response);
}

/*
 * Tests functions:
 *   relay_http_print_log_request
 */

TEST(RelayHttp, PrintLogRequest)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_http_print_log_response
 */

TEST(RelayHttp, PrintLogResponse)
{
    /* TODO: write tests */
}
