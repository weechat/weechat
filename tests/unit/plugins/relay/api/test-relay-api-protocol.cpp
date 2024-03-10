/*
 * test-relay-api-protocol.cpp - test relay API protocol (protocol)
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

#include "tests/tests-record.h"

extern "C"
{
#include <unistd.h>
#include <cjson/cJSON.h>
#include "src/core/wee-config-file.h"
#include "src/core/wee-string.h"
#include "src/core/wee-util.h"
#include "src/core/wee-version.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-line.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-server.h"
#include "src/plugins/relay/api/relay-api.h"
#include "src/plugins/relay/api/relay-api-protocol.h"

extern void relay_client_recv_text (struct t_relay_client *client,
                                    const char *data);
extern int relay_api_protocol_command_delay;
}

#define WEE_CHECK_HTTP_CODE(__code, __message)                          \
    STRNCMP_EQUAL("HTTP/1.1 " #__code " " __message "\r\n",             \
                  data_sent,                                            \
                  strlen ("HTTP/1.1 " #__code " " __message "\r\n"));

#define WEE_CHECK_TEXT(__code, __message)                               \
    STRCMP_EQUAL("{\"code\":" #__code ","                               \
                 "\"message\":\"" __message "\"}",                      \
                 data_sent);

#define WEE_CHECK_OBJ_STR(__expected, __json, __name)                   \
    json_obj = cJSON_GetObjectItem (__json, __name);                    \
    CHECK(json_obj);                                                    \
    CHECK(cJSON_IsString (json_obj));                                   \
    STRCMP_EQUAL(__expected, cJSON_GetStringValue (json_obj));

#define WEE_CHECK_OBJ_STRN(__expected, __length, __json, __name)        \
    json_obj = cJSON_GetObjectItem (__json, __name);                    \
    CHECK(json_obj);                                                    \
    CHECK(cJSON_IsString (json_obj));                                   \
    STRNCMP_EQUAL(__expected, cJSON_GetStringValue (json_obj),          \
                  __length);

#define WEE_CHECK_OBJ_NUM(__expected, __json, __name)                   \
    json_obj = cJSON_GetObjectItem (__json, __name);                    \
    CHECK(json_obj);                                                    \
    CHECK(cJSON_IsNumber (json_obj));                                   \
    CHECK(__expected == cJSON_GetNumberValue (json_obj));

#define WEE_CHECK_OBJ_BOOL(__expected, __json, __name)                  \
    json_obj = cJSON_GetObjectItem (__json, __name);                    \
    CHECK(json_obj);                                                    \
    CHECK(cJSON_IsBool (json_obj));                                     \
    LONGS_EQUAL(__expected, cJSON_IsTrue (json_obj) ? 1 : 0);

struct t_relay_server *ptr_relay_server = NULL;
struct t_relay_client *ptr_relay_client = NULL;
char *data_sent = NULL;
int data_sent_size = 0;
cJSON *json_body_sent = NULL;

TEST_GROUP(RelayApiProtocol)
{
};

TEST_GROUP(RelayApiProtocolWithClient)
{
    void free_data_sent ()
    {
        if (data_sent)
        {
            free (data_sent);
            data_sent = NULL;
        }
        data_sent_size = 0;
        if (json_body_sent)
        {
            cJSON_Delete (json_body_sent);
            json_body_sent = NULL;
        }
    }

    void test_client_recv_http_raw (const char *http_request)
    {
        free_data_sent ();
        relay_client_recv_buffer (ptr_relay_client,
                                  http_request, strlen (http_request));
    }

    void test_client_recv_http (const char *method_path, const char *body)
    {
        char http_request[4096];

        if (body)
        {
            snprintf (http_request, sizeof (http_request),
                      "%s HTTP/1.1\r\n"
                      "Authorization: Basic cGxhaW46c2VjcmV0\r\n"
                      "Content-Length: %d\r\n"
                      "Content-Type: application/x-www-form-urlencoded\r\n"
                      "\r\n"
                      "%s",
                      method_path,
                      (int)strlen (body),
                      body);
        }
        else
        {
            snprintf (http_request, sizeof (http_request),
                      "%s HTTP/1.1\r\n"
                      "Authorization: Basic cGxhaW46c2VjcmV0\r\n"
                      "\r\n",
                      method_path);
        }
        test_client_recv_http_raw (http_request);
    }

    void test_client_recv_text (const char *data)
    {
        free_data_sent ();
        relay_client_recv_text (ptr_relay_client, data);
    }

    static void fake_send_func (void *client, const char *data, int data_size)
    {
        char *pos_body;

        (void) client;

        if (data_sent)
        {
            free (data_sent);
            data_sent = NULL;
        }
        data_sent_size = 0;
        if (json_body_sent)
        {
            cJSON_Delete (json_body_sent);
            json_body_sent = NULL;
        }

        data_sent = (char *)malloc (data_size + 1);
        memcpy (data_sent, data, data_size);
        data_sent[data_size] = '\0';
        data_sent_size = data_size;

        pos_body = strstr (data_sent, "\r\n\r\n");
        if (pos_body)
            json_body_sent = cJSON_Parse(pos_body + 4);
    }

    void setup ()
    {
        /* disable auto-open of relay buffer */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);

        /* set relay password */
        config_file_option_set (relay_config_network_password, "secret", 1);

        /* create a relay server */
        ptr_relay_server = relay_server_new (
            "api",
            RELAY_PROTOCOL_API,
            "test",
            9000,
            NULL,  /* path */
            1,  /* ipv4 */
            0,  /* ipv6 */
            0,  /* tls */
            0);  /* unix_socket */

        /* create a relay client */
        ptr_relay_client = relay_client_new (-1, "test", ptr_relay_server);
        ptr_relay_client->fake_send_func = &fake_send_func;

        data_sent = NULL;
        data_sent_size = 0;
        json_body_sent = NULL;
    }

    void teardown ()
    {
        relay_client_free (ptr_relay_client);
        ptr_relay_client = NULL;

        relay_server_free (ptr_relay_server);
        ptr_relay_server = NULL;

        free_data_sent ();

        /* restore auto-open of relay buffer */
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);

        /* restore relay password */
        config_file_option_reset (relay_config_network_password, 1);
    }
};

/*
 * Tests functions:
 *   relay_api_protocol_signal_buffer_cb
 */

TEST(RelayApiProtocol, SignalBufferCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_hsignal_nicklist_cb
 */

TEST(RelayApiProtocol, HsignalNicklistCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_signal_upgrade_cb
 */

TEST(RelayApiProtocol, SignalUpgradeCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_handshake
 */

TEST(RelayApiProtocolWithClient, CbHandshake)
{
    /* no body */
    test_client_recv_http ("POST /api/handshake", NULL);
    STRCMP_EQUAL("HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 74\r\n"
                 "\r\n"
                 "{\"password_hash_algo\":null,"
                 "\"password_hash_iterations\":100000,"
                 "\"totp\":false}",
                 data_sent);

    /* empty body */
    test_client_recv_http ("POST /api/handshake", "{}");
    STRCMP_EQUAL("HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 74\r\n"
                 "\r\n"
                 "{\"password_hash_algo\":null,"
                 "\"password_hash_iterations\":100000,"
                 "\"totp\":false}",
                 data_sent);

    /* unknown password hash algorithm */
    test_client_recv_http ("POST /api/handshake",
                           "{\"password_hash_algo\": [\"invalid\"]}");
    STRCMP_EQUAL("HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 74\r\n"
                 "\r\n"
                 "{\"password_hash_algo\":null,"
                 "\"password_hash_iterations\":100000,"
                 "\"totp\":false}",
                 data_sent);

    /* two supported hash algorithms */
    test_client_recv_http (
        "POST /api/handshake",
        "{\"password_hash_algo\": [\"sha256\", \"pbkdf2+sha512\"]}");
    STRCMP_EQUAL("HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 85\r\n"
                 "\r\n"
                 "{\"password_hash_algo\":\"pbkdf2+sha512\","
                 "\"password_hash_iterations\":100000,"
                 "\"totp\":false}",
                 data_sent);
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_version
 */

TEST(RelayApiProtocolWithClient, CbVersion)
{
    cJSON *json, *json_obj;

    test_client_recv_http ("GET /api/version", NULL);
    WEE_CHECK_HTTP_CODE(200, "OK");
    json = json_body_sent;
    WEE_CHECK_OBJ_STR(version_get_version (), json, "weechat_version");
    WEE_CHECK_OBJ_STR(version_get_git (), json, "weechat_version_git");
    WEE_CHECK_OBJ_NUM(util_version_number (version_get_version ()),
                      json, "weechat_version_number");
    WEE_CHECK_OBJ_STR(RELAY_API_VERSION_STR, json, "relay_api_version");
    WEE_CHECK_OBJ_NUM(RELAY_API_VERSION_NUMBER, json, "relay_api_version_number");
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_buffers
 */

TEST(RelayApiProtocolWithClient, CbBuffers)
{
    cJSON *json, *json_obj, *json_var, *json_groups;

    /* error: invalid buffer name */
    test_client_recv_http ("GET /api/buffers/invalid", NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 41\r\n"
                 "\r\n"
                 "{\"error\": \"Buffer \\\"invalid\\\" not found\"}",
                 data_sent);

    /* error: invalid sub-resource */
    test_client_recv_http ("GET /api/buffers/core.weechat/invalid", NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 59\r\n"
                 "\r\n"
                 "{\"error\": \"Sub-resource of buffers not found: \\\"invalid\\\"\"}",
                 data_sent);

    /* error: too many parameters in path */
    test_client_recv_http ("GET /api/buffers/core.weechat/too/many/parameters", NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* get all buffers */
    test_client_recv_http ("GET /api/buffers", NULL);
    WEE_CHECK_HTTP_CODE(200, "OK");
    CHECK(json_body_sent);
    CHECK(cJSON_IsArray (json_body_sent));
    json = cJSON_GetArrayItem (json_body_sent, 0);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    WEE_CHECK_OBJ_NUM(gui_buffers->id, json, "id");
    WEE_CHECK_OBJ_STR("core.weechat", json, "name");
    WEE_CHECK_OBJ_STR("weechat", json, "short_name");
    WEE_CHECK_OBJ_NUM(1, json, "number");
    WEE_CHECK_OBJ_STR("formatted", json, "type");
    WEE_CHECK_OBJ_STRN("WeeChat", 7, json, "title");
    json_var = cJSON_GetObjectItem (json, "local_variables");
    CHECK(json_var);
    CHECK(cJSON_IsObject (json_var));
    WEE_CHECK_OBJ_STR("core", json_var, "plugin");
    WEE_CHECK_OBJ_STR("weechat", json_var, "name");

    /* get one buffer */
    test_client_recv_http ("GET /api/buffers/core.weechat", NULL);
    WEE_CHECK_HTTP_CODE(200, "OK");
    CHECK(json_body_sent);
    CHECK(cJSON_IsObject (json_body_sent));
    json = json_body_sent;
    WEE_CHECK_OBJ_NUM(gui_buffers->id, json, "id");
    WEE_CHECK_OBJ_STR("core.weechat", json, "name");
    WEE_CHECK_OBJ_STR("weechat", json, "short_name");
    WEE_CHECK_OBJ_NUM(1, json, "number");
    WEE_CHECK_OBJ_STR("formatted", json, "type");
    WEE_CHECK_OBJ_STRN("WeeChat", 7, json, "title");
    json_var = cJSON_GetObjectItem (json, "local_variables");
    CHECK(json_var);
    CHECK(cJSON_IsObject (json_var));
    WEE_CHECK_OBJ_STR("core", json_var, "plugin");
    WEE_CHECK_OBJ_STR("weechat", json_var, "name");

    /* get the 2 last lines of core buffer */
    gui_chat_printf (NULL, "test line 1");
    gui_chat_printf (NULL, "test line 2");
    test_client_recv_http ("GET /api/buffers/core.weechat/lines?lines=-2", NULL);
    WEE_CHECK_HTTP_CODE(200, "OK");
    CHECK(json_body_sent);
    CHECK(cJSON_IsArray (json_body_sent));
    json = cJSON_GetArrayItem (json_body_sent, 0);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->prev_line->data->id, json, "id");
    WEE_CHECK_OBJ_NUM(-1, json, "y");
    CHECK(cJSON_IsString (cJSON_GetObjectItem (json, "date")));
    CHECK(cJSON_IsString (cJSON_GetObjectItem (json, "date_printed")));
    WEE_CHECK_OBJ_BOOL(0, json, "highlight");
    WEE_CHECK_OBJ_STR("", json, "prefix");
    WEE_CHECK_OBJ_STR("test line 1", json, "message");
    json = cJSON_GetArrayItem (json_body_sent, 1);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->data->id, json, "id");
    WEE_CHECK_OBJ_NUM(-1, json, "y");
    CHECK(cJSON_IsString (cJSON_GetObjectItem (json, "date")));
    CHECK(cJSON_IsString (cJSON_GetObjectItem (json, "date_printed")));
    WEE_CHECK_OBJ_BOOL(0, json, "highlight");
    WEE_CHECK_OBJ_STR("", json, "prefix");
    WEE_CHECK_OBJ_STR("test line 2", json, "message");

    /* get nicks */
    test_client_recv_http ("GET /api/buffers/core.weechat/nicks", NULL);
    WEE_CHECK_HTTP_CODE(200, "OK");
    CHECK(json_body_sent);
    CHECK(cJSON_IsObject (json_body_sent));
    json = json_body_sent;
    WEE_CHECK_OBJ_STR("root", json, "name");
    WEE_CHECK_OBJ_STR("", json, "color");
    json_groups = cJSON_GetObjectItem (json, "groups");
    CHECK(json_groups);
    CHECK(cJSON_IsArray (json_groups));
    LONGS_EQUAL(0, cJSON_GetArraySize (json_groups));
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_input
 */

TEST(RelayApiProtocolWithClient, CbInput)
{
    int old_delay;

    /* error: no body */
    test_client_recv_http ("POST /api/input", NULL);
    STRCMP_EQUAL("HTTP/1.1 400 Bad Request\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* error: invalid buffer name */
    test_client_recv_http ("POST /api/input",
                           "{\"buffer\": \"invalid\", "
                           "\"command\": \"/print test\"}");
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 41\r\n"
                 "\r\n"
                 "{\"error\": \"Buffer \\\"invalid\\\" not found\"}",
                 data_sent);

    /* on core buffer, without buffer name */
    record_start ();
    old_delay = relay_api_protocol_command_delay;
    relay_api_protocol_command_delay = 0;
    test_client_recv_http ("POST /api/input",
                           "{\"command\": \"/print test from relay 1\"}");
    relay_api_protocol_command_delay = old_delay;
    record_stop ();
    WEE_CHECK_HTTP_CODE(204, "No Content");
    CHECK(record_search ("core.weechat", "", "test from relay 1", NULL));

    /* on core buffer, with buffer name */
    record_start ();
    old_delay = relay_api_protocol_command_delay;
    relay_api_protocol_command_delay = 0;
    test_client_recv_http ("POST /api/input",
                           "{\"buffer\": \"core.weechat\", "
                           "\"command\": \"/print test from relay 2\"}");
    relay_api_protocol_command_delay = old_delay;
    record_stop ();
    WEE_CHECK_HTTP_CODE(204, "No Content");
    CHECK(record_search ("core.weechat", "", "test from relay 2", NULL));
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_ping
 */

TEST(RelayApiProtocolWithClient, CbPing)
{
    cJSON *json, *json_obj;

    /* ping without body */
    test_client_recv_http ("POST /api/ping", NULL);
    STRCMP_EQUAL("HTTP/1.1 204 No Content\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* ping with a body */
    test_client_recv_http ("POST /api/ping", "{\"data\": \"abcdef\"}");
    WEE_CHECK_HTTP_CODE(200, "OK");
    json = json_body_sent;
    WEE_CHECK_OBJ_STR("abcdef", json, "data");
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_sync
 */

TEST(RelayApiProtocolWithClient, CbSync)
{
    test_client_recv_http ("POST /api/sync", NULL);
    STRCMP_EQUAL("HTTP/1.1 403 Forbidden\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 72\r\n"
                 "\r\n"
                 "{\"error\": \"Sync resource is available only with "
                 "a websocket connection\"}",
                 data_sent);
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_sync (websocket)
 */

TEST(RelayApiProtocolWithClient, CbSyncWebsocket)
{
    test_client_recv_http_raw (
        "GET /api HTTP/1.1\r\n"
        "Authorization: Basic cGxhaW46c2VjcmV0\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Key: dbKbsCX3CxFBmQo09ah1OQ==\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "Host: 127.0.0.1:9000\r\n"
        "\r\n");
    STRCMP_EQUAL("HTTP/1.1 101 Switching Protocols\r\n"
                 "Upgrade: websocket\r\n"
                 "Connection: Upgrade\r\n"
                 "Sec-WebSocket-Accept: Z5uTZwvwYNDm9w4HFGk26ijp/p0=\r\n"
                 "\r\n",
                 data_sent);

    test_client_recv_text ("{\"request\": \"POST /api/sync\"}");
    WEE_CHECK_TEXT(204, "No Content");

    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_enabled));
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_nicks));
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, RELAY_API_DATA(ptr_relay_client, sync_colors));

    test_client_recv_text ("{\"request\": \"POST /api/sync\", "
                           "\"body\": {\"sync\": false}}");
    WEE_CHECK_TEXT(204, "No Content");

    LONGS_EQUAL(0, RELAY_API_DATA(ptr_relay_client, sync_enabled));
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_nicks));
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, RELAY_API_DATA(ptr_relay_client, sync_colors));

    test_client_recv_text ("{\"request\": \"POST /api/sync\", "
                           "\"body\": {\"sync\": true, \"nicks\": false}}");
    WEE_CHECK_TEXT(204, "No Content");

    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_enabled));
    LONGS_EQUAL(0, RELAY_API_DATA(ptr_relay_client, sync_nicks));
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, RELAY_API_DATA(ptr_relay_client, sync_colors));

    test_client_recv_text ("{\"request\": \"POST /api/sync\", "
                           "\"body\": {\"sync\": true, \"nicks\": true, \"colors\": \"weechat\"}}");
    WEE_CHECK_TEXT(204, "No Content");

    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_enabled));
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_nicks));
    LONGS_EQUAL(RELAY_API_COLORS_WEECHAT, RELAY_API_DATA(ptr_relay_client, sync_colors));

    test_client_recv_text ("{\"request\": \"POST /api/sync\", "
                           "\"body\": {\"sync\": true, \"nicks\": true, \"colors\": \"strip\"}}");
    WEE_CHECK_TEXT(204, "No Content");

    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_enabled));
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_nicks));
    LONGS_EQUAL(RELAY_API_COLORS_STRIP, RELAY_API_DATA(ptr_relay_client, sync_colors));
}

/*
 * Tests functions:
 *   relay_api_protocol_recv_json
 */

TEST(RelayApiProtocolWithClient, RecvJson)
{
    test_client_recv_http_raw (
        "GET /api HTTP/1.1\r\n"
        "Authorization: Basic cGxhaW46c2VjcmV0\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Key: dbKbsCX3CxFBmQo09ah1OQ==\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "Host: 127.0.0.1:9000\r\n"
        "\r\n");
    STRCMP_EQUAL("HTTP/1.1 101 Switching Protocols\r\n"
                 "Upgrade: websocket\r\n"
                 "Connection: Upgrade\r\n"
                 "Sec-WebSocket-Accept: Z5uTZwvwYNDm9w4HFGk26ijp/p0=\r\n"
                 "\r\n",
                 data_sent);

    /* error: empty string */
    test_client_recv_text ("");
    WEE_CHECK_TEXT(400, "Bad Request");

    /* error: empty body */
    test_client_recv_text ("{}");
    WEE_CHECK_TEXT(400, "Bad Request");

    /* error: empty request */
    test_client_recv_text ("{\"request\": \"\"}");
    WEE_CHECK_TEXT(400, "Bad Request");

    /* error: invalid request (number) */
    test_client_recv_text ("{\"request\": 123}");
    WEE_CHECK_TEXT(400, "Bad Request");

    /* error: invalid request (string, not a valid request) */
    test_client_recv_text ("{\"request\": \"abc\"}");
    WEE_CHECK_TEXT(400, "Bad Request");

    /* error: invalid request (string, resource not found) */
    test_client_recv_text ("{\"request\": \"GET /api/unknown\"}");
    WEE_CHECK_TEXT(404, "Not Found");
}

/*
 * Tests functions:
 *   relay_api_protocol_recv_http (error 404)
 */

TEST(RelayApiProtocolWithClient, RecvHttp404)
{
    /* resource not found: error 404 */
    test_client_recv_http ("GET / HTTP/1.1", NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* resource not found: error 404 */
    test_client_recv_http ("GET /unknown HTTP/1.1", NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* resource not found: error 404 */
    test_client_recv_http ("GET /unknown/abc HTTP/1.1", NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* resource not found: error 404 */
    test_client_recv_http ("GET /api HTTP/1.1", NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* resource not found: error 404 */
    test_client_recv_http ("GET /api/unknown HTTP/1.1", NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);
}

/*
 * Tests functions:
 *   relay_api_protocol_recv_http (missing password)
 */

TEST(RelayApiProtocolWithClient, RecvHttpMissingPassword)
{
    /* unauthorized: missing password */
    test_client_recv_http_raw ("GET /api/version HTTP/1.1\r\n"
                               "\r\n");
    STRCMP_EQUAL("HTTP/1.1 401 Unauthorized\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 29\r\n"
                 "\r\n"
                 "{\"error\": \"Missing password\"}",
                 data_sent);
}

/*
 * Tests functions:
 *   relay_api_protocol_recv_http (invalid password)
 */

TEST(RelayApiProtocolWithClient, RecvHttpInvalidPassword)
{
    /* unauthorized: invalid password: "plain:invalid" */
    test_client_recv_http_raw ("GET /api/version HTTP/1.1\r\n"
                               "Authorization: Basic cGxhaW46aW52YWxpZA==\r\n"
                               "\r\n");
    STRCMP_EQUAL("HTTP/1.1 401 Unauthorized\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 29\r\n"
                 "\r\n"
                 "{\"error\": \"Invalid password\"}",
                 data_sent);
}
