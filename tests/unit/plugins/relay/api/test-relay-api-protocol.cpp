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
#include <string.h>
#include <cjson/cJSON.h>
#include "src/core/core-config-file.h"
#include "src/core/core-string.h"
#include "src/core/core-util.h"
#include "src/core/core-version.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-hotlist.h"
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

#define WEE_CHECK_TEXT(__code, __message, __request, __body)            \
    STRCMP_EQUAL("{\"code\":" #__code ","                               \
                 "\"message\":\"" __message "\","                       \
                 "\"request\":\"" __request "\","                       \
                 "\"request_body\":" __body ""                          \
                 "}",                                                   \
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

    void test_client_recv_http (const char *method_path,
                                const char *headers,
                                const char *body)
    {
        char http_request[4096];

        if (body)
        {
            snprintf (http_request, sizeof (http_request),
                      "%s HTTP/1.1\r\n"
                      "%s%s"
                      "Authorization: Basic cGxhaW46c2VjcmV0\r\n"
                      "Content-Length: %d\r\n"
                      "Content-Type: application/x-www-form-urlencoded\r\n"
                      "\r\n"
                      "%s",
                      method_path,
                      (headers && headers[0]) ? headers : "",
                      (headers && headers[0]) ? "\r\n" : "",
                      (int)strlen (body),
                      body);
        }
        else
        {
            snprintf (http_request, sizeof (http_request),
                      "%s HTTP/1.1\r\n"
                      "%s%s"
                      "Authorization: Basic cGxhaW46c2VjcmV0\r\n"
                      "\r\n",
                      method_path,
                      (headers && headers[0]) ? headers : "",
                      (headers && headers[0]) ? "\r\n" : "");
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
 *   relay_api_protocol_cb_options
 */

TEST(RelayApiProtocolWithClient, CbOptions)
{
    test_client_recv_http ("OPTIONS /api/buffers", NULL,
                           "{\"password_hash_algo\": [\"invalid\"]}");
    STRCMP_EQUAL(
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE\r\n"
        "Access-Control-Allow-Headers: origin, content-type, accept, authorization\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: 0\r\n"
        "\r\n",
        data_sent);
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_handshake
 */

TEST(RelayApiProtocolWithClient, CbHandshake)
{
    /* no body */
    test_client_recv_http ("POST /api/handshake", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 200 OK\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 74\r\n"
                 "\r\n"
                 "{\"password_hash_algo\":null,"
                 "\"password_hash_iterations\":100000,"
                 "\"totp\":false}",
                 data_sent);

    /* empty body */
    test_client_recv_http ("POST /api/handshake", NULL, "{}");
    STRCMP_EQUAL("HTTP/1.1 200 OK\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 74\r\n"
                 "\r\n"
                 "{\"password_hash_algo\":null,"
                 "\"password_hash_iterations\":100000,"
                 "\"totp\":false}",
                 data_sent);

    /* unknown password hash algorithm */
    test_client_recv_http ("POST /api/handshake", NULL,
                           "{\"password_hash_algo\": [\"invalid\"]}");
    STRCMP_EQUAL("HTTP/1.1 200 OK\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
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
        NULL,
        "{\"password_hash_algo\": [\"sha256\", \"pbkdf2+sha512\"]}");
    STRCMP_EQUAL("HTTP/1.1 200 OK\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
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

    test_client_recv_http ("GET /api/version", NULL, NULL);
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
    char str_http[256];

    /* error: invalid buffer name */
    test_client_recv_http ("GET /api/buffers/invalid", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 41\r\n"
                 "\r\n"
                 "{\"error\": \"Buffer \\\"invalid\\\" not found\"}",
                 data_sent);

    /* error: invalid buffer id */
    test_client_recv_http ("GET /api/buffers/123", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 37\r\n"
                 "\r\n"
                 "{\"error\": \"Buffer \\\"123\\\" not found\"}",
                 data_sent);

    /* error: invalid sub-resource */
    test_client_recv_http ("GET /api/buffers/core.weechat/invalid", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 59\r\n"
                 "\r\n"
                 "{\"error\": \"Sub-resource of buffers not found: \\\"invalid\\\"\"}",
                 data_sent);

    /* error: too many parameters in path */
    test_client_recv_http ("GET /api/buffers/core.weechat/too/many/parameters",
                           NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* get all buffers */
    test_client_recv_http ("GET /api/buffers", NULL, NULL);
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
    WEE_CHECK_OBJ_STR("", json, "modes");
    WEE_CHECK_OBJ_STR("", json, "input_prompt");
    WEE_CHECK_OBJ_STR("", json, "input");
    WEE_CHECK_OBJ_NUM(0, json, "input_position");
    WEE_CHECK_OBJ_BOOL(0, json, "input_multiline");
    json_var = cJSON_GetObjectItem (json, "local_variables");
    CHECK(json_var);
    CHECK(cJSON_IsObject (json_var));
    WEE_CHECK_OBJ_STR("core", json_var, "plugin");
    WEE_CHECK_OBJ_STR("weechat", json_var, "name");

    /* get one buffer by name */
    gui_buffer_set (gui_buffers, "input_prompt", "test_prompt");
    gui_buffer_set (gui_buffers, "input", "test");
    gui_buffer_set (gui_buffers, "input_pos", "4");
    gui_buffer_set (gui_buffers, "input_multiline", "1");
    test_client_recv_http ("GET /api/buffers/core.weechat", NULL, NULL);
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
    WEE_CHECK_OBJ_STR("", json, "modes");
    WEE_CHECK_OBJ_STR("test_prompt", json, "input_prompt");
    WEE_CHECK_OBJ_STR("test", json, "input");
    WEE_CHECK_OBJ_NUM(4, json, "input_position");
    WEE_CHECK_OBJ_BOOL(1, json, "input_multiline");
    json_var = cJSON_GetObjectItem (json, "local_variables");
    CHECK(json_var);
    CHECK(cJSON_IsObject (json_var));
    WEE_CHECK_OBJ_STR("core", json_var, "plugin");
    WEE_CHECK_OBJ_STR("weechat", json_var, "name");
    gui_buffer_set (gui_buffers, "input_prompt", "");
    gui_buffer_set (gui_buffers, "input", "");
    gui_buffer_set (gui_buffers, "input_multiline", "0");

    /* get one buffer by id */
    snprintf (str_http, sizeof (str_http),
              "GET /api/buffers/%lld", gui_buffers->id);
    test_client_recv_http (str_http, NULL, NULL);
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
    WEE_CHECK_OBJ_STR("", json, "modes");
    WEE_CHECK_OBJ_STR("", json, "input_prompt");
    WEE_CHECK_OBJ_STR("", json, "input");
    WEE_CHECK_OBJ_NUM(0, json, "input_position");
    WEE_CHECK_OBJ_BOOL(0, json, "input_multiline");
    json_var = cJSON_GetObjectItem (json, "local_variables");
    CHECK(json_var);
    CHECK(cJSON_IsObject (json_var));
    WEE_CHECK_OBJ_STR("core", json_var, "plugin");
    WEE_CHECK_OBJ_STR("weechat", json_var, "name");

    /* get the 2 last lines of core buffer */
    gui_chat_printf (NULL, "test line 1");
    gui_chat_printf (NULL, "test line 2");
    test_client_recv_http ("GET /api/buffers/core.weechat/lines?lines=-2", NULL, NULL);
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
    test_client_recv_http ("GET /api/buffers/core.weechat/nicks", NULL, NULL);
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
 *   relay_api_protocol_cb_hotlist
 */

TEST(RelayApiProtocolWithClient, CbHotlist)
{
    cJSON *json, *json_obj, *json_count;

    /* get hotlist (empty) */
    test_client_recv_http ("GET /api/hotlist", NULL, NULL);
    WEE_CHECK_HTTP_CODE(200, "OK");
    CHECK(json_body_sent);
    CHECK(cJSON_IsArray (json_body_sent));
    LONGS_EQUAL(0, cJSON_GetArraySize (json_body_sent));

    gui_hotlist_add (gui_buffers, GUI_HOTLIST_LOW, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_MESSAGE, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_MESSAGE, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_PRIVATE, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_PRIVATE, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_PRIVATE, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_HIGHLIGHT, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_HIGHLIGHT, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_HIGHLIGHT, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_HIGHLIGHT, NULL, 0);

    /* get hotlist (one buffer) */
    test_client_recv_http ("GET /api/hotlist", NULL, NULL);
    WEE_CHECK_HTTP_CODE(200, "OK");
    CHECK(json_body_sent);
    CHECK(cJSON_IsArray (json_body_sent));
    LONGS_EQUAL(1, cJSON_GetArraySize (json_body_sent));
    json = cJSON_GetArrayItem (json_body_sent, 0);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    WEE_CHECK_OBJ_NUM(GUI_HOTLIST_HIGHLIGHT, json, "priority");
    CHECK(cJSON_IsString (cJSON_GetObjectItem (json, "date")));
    WEE_CHECK_OBJ_NUM(gui_buffers->id, json, "buffer_id");
    json_count = cJSON_GetObjectItem (json, "count");
    CHECK(json_count);
    CHECK(cJSON_IsArray (json_count));
    json_obj = cJSON_GetArrayItem (json_count, 0);
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(1 == cJSON_GetNumberValue (json_obj));
    json_obj = cJSON_GetArrayItem (json_count, 1);
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(2 == cJSON_GetNumberValue (json_obj));
    json_obj = cJSON_GetArrayItem (json_count, 2);
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(3 == cJSON_GetNumberValue (json_obj));
    json_obj = cJSON_GetArrayItem (json_count, 3);
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(4 == cJSON_GetNumberValue (json_obj));

    gui_hotlist_remove_buffer (gui_buffers, 1);
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_input
 */

TEST(RelayApiProtocolWithClient, CbInput)
{
    char str_body[1024];
    int old_delay;

    /* error: no body */
    test_client_recv_http ("POST /api/input", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 400 Bad Request\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* error: invalid buffer name */
    test_client_recv_http ("POST /api/input",
                           NULL,
                           "{\"buffer_name\": \"invalid\", "
                           "\"command\": \"/print test\"}");
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
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
                           NULL,
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
                           NULL,
                           "{\"buffer_name\": \"core.weechat\", "
                           "\"command\": \"/print test from relay 2\"}");
    relay_api_protocol_command_delay = old_delay;
    record_stop ();
    WEE_CHECK_HTTP_CODE(204, "No Content");
    CHECK(record_search ("core.weechat", "", "test from relay 2", NULL));

    /* on core buffer, with buffer id */
    record_start ();
    old_delay = relay_api_protocol_command_delay;
    relay_api_protocol_command_delay = 0;
    snprintf (str_body, sizeof (str_body),
              "{\"buffer_id\": %lld, "
              "\"command\": \"/print test from relay 3\"}",
              gui_buffers->id);
    test_client_recv_http ("POST /api/input", NULL, str_body);
    relay_api_protocol_command_delay = old_delay;
    record_stop ();
    WEE_CHECK_HTTP_CODE(204, "No Content");
    CHECK(record_search ("core.weechat", "", "test from relay 3", NULL));
}

/*
 * Tests functions:
 *   relay_api_protocol_cb_ping
 */

TEST(RelayApiProtocolWithClient, CbPing)
{
    cJSON *json, *json_obj;

    /* ping without body */
    test_client_recv_http ("POST /api/ping", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 204 No Content\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* ping with a body */
    test_client_recv_http ("POST /api/ping", NULL, "{\"data\": \"abcdef\"}");
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
    test_client_recv_http ("POST /api/sync", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 403 Forbidden\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
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
    WEE_CHECK_TEXT(204, "No Content", "POST /api/sync", "null");

    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_enabled));
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_nicks));
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, RELAY_API_DATA(ptr_relay_client, sync_colors));

    test_client_recv_text ("{\"request\": \"POST /api/sync\", "
                           "\"body\": {\"sync\": false}}");
    WEE_CHECK_TEXT(204, "No Content", "POST /api/sync", "{\"sync\":false}");

    LONGS_EQUAL(0, RELAY_API_DATA(ptr_relay_client, sync_enabled));
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_nicks));
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, RELAY_API_DATA(ptr_relay_client, sync_colors));

    test_client_recv_text ("{\"request\": \"POST /api/sync\", "
                           "\"body\": {\"sync\": true, \"nicks\": false}}");
    WEE_CHECK_TEXT(204, "No Content", "POST /api/sync", "{\"sync\":true,\"nicks\":false}");

    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_enabled));
    LONGS_EQUAL(0, RELAY_API_DATA(ptr_relay_client, sync_nicks));
    LONGS_EQUAL(RELAY_API_COLORS_ANSI, RELAY_API_DATA(ptr_relay_client, sync_colors));

    test_client_recv_text ("{\"request\": \"POST /api/sync\", "
                           "\"body\": {\"sync\": true, \"nicks\": true, \"colors\": \"weechat\"}}");
    WEE_CHECK_TEXT(204, "No Content", "POST /api/sync", "{\"sync\":true,\"nicks\":true,\"colors\":\"weechat\"}");

    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_enabled));
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_relay_client, sync_nicks));
    LONGS_EQUAL(RELAY_API_COLORS_WEECHAT, RELAY_API_DATA(ptr_relay_client, sync_colors));

    test_client_recv_text ("{\"request\": \"POST /api/sync\", "
                           "\"body\": {\"sync\": true, \"nicks\": true, \"colors\": \"strip\"}}");
    WEE_CHECK_TEXT(204, "No Content", "POST /api/sync", "{\"sync\":true,\"nicks\":true,\"colors\":\"strip\"}");

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
    WEE_CHECK_TEXT(400, "Bad Request", "", "null");

    /* error: empty body */
    test_client_recv_text ("{}");
    WEE_CHECK_TEXT(400, "Bad Request", "", "null");

    /* error: empty request */
    test_client_recv_text ("{\"request\": \"\"}");
    WEE_CHECK_TEXT(400, "Bad Request", "", "null");

    /* error: invalid request (number) */
    test_client_recv_text ("{\"request\": 123}");
    WEE_CHECK_TEXT(400, "Bad Request", "", "null");

    /* error: invalid request (string, not a valid request) */
    test_client_recv_text ("{\"request\": \"abc\"}");
    WEE_CHECK_TEXT(400, "Bad Request", "", "null");

    /* error: invalid request (string, resource not found) */
    test_client_recv_text ("{\"request\": \"GET /api/unknown\"}");
    WEE_CHECK_TEXT(404, "Not Found", "GET /api/unknown", "null");

    /* error: invalid request (string, resource not found) */
    test_client_recv_text ("{\"request\": \"GET /api/unknown\", \"body\": {\"test\": 123}}");
    WEE_CHECK_TEXT(404, "Not Found", "GET /api/unknown", "{\"test\":123}");
}

/*
 * Tests functions:
 *   relay_api_protocol_recv_http (error 404)
 */

TEST(RelayApiProtocolWithClient, RecvHttp404)
{
    /* resource not found: error 404 */
    test_client_recv_http ("GET / HTTP/1.1", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* resource not found: error 404 */
    test_client_recv_http ("GET /unknown HTTP/1.1", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* resource not found: error 404 */
    test_client_recv_http ("GET /unknown/abc HTTP/1.1", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* resource not found: error 404 */
    test_client_recv_http ("GET /api HTTP/1.1", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 data_sent);

    /* resource not found: error 404 */
    test_client_recv_http ("GET /api/unknown HTTP/1.1", NULL, NULL);
    STRCMP_EQUAL("HTTP/1.1 404 Not Found\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
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
                 "Access-Control-Allow-Origin: *\r\n"
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
                 "Access-Control-Allow-Origin: *\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: 29\r\n"
                 "\r\n"
                 "{\"error\": \"Invalid password\"}",
                 data_sent);
}
