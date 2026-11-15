/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay upgrade functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "src/core/weechat.h"
#include "src/core/core-config-file.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/core/core-upgrade-file.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/plugin.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-buffer.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-raw.h"
#include "src/plugins/relay/relay-server.h"
#include "src/plugins/relay/relay-upgrade.h"
#include "src/plugins/relay/relay-websocket.h"
#include "src/plugins/relay/api/relay-api.h"
#include "src/plugins/relay/weechat/relay-weechat.h"

extern int relay_upgrade_save_all_data (struct t_upgrade_file *upgrade_file,
                                        int force_disconnected_state);
extern void relay_upgrade_set_buffer_callbacks (void);
extern int relay_upgrade_read_cb (const void *pointer, void *data,
                                  struct t_upgrade_file *upgrade_file,
                                  int object_id,
                                  struct t_infolist *infolist);
}

#define TEST_DATE 1734000000

TEST_GROUP(RelayUpgrade)
{
    struct t_relay_server *server_weechat = NULL;
    struct t_relay_server *server_api = NULL;

    void setup ()
    {
        /* Disable auto-open of relay buffer (it would pollute other tests). */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);

        /* UNIX sockets: no TCP port is used on the local system. */
        server_weechat = relay_server_new (
            "unix.weechat", RELAY_PROTOCOL_WEECHAT, NULL,
            -1,  /* port */
            "%h/test_relay_upgrade_weechat.sock",  /* path */
            0,  /* ipv4 */
            0,  /* ipv6 */
            0,  /* tls */
            1);  /* unix_socket */
        CHECK(server_weechat);
        server_api = relay_server_new (
            "unix.api", RELAY_PROTOCOL_API, NULL,
            -1,  /* port */
            "%h/test_relay_upgrade_api.sock",  /* path */
            0,  /* ipv4 */
            0,  /* ipv6 */
            0,  /* tls */
            1);  /* unix_socket */
        CHECK(server_api);

        relay_raw_message_free_all ();
    }

    void teardown ()
    {
        relay_client_free_all ();
        relay_raw_message_free_all ();
        relay_server_free (server_weechat);
        server_weechat = NULL;
        relay_server_free (server_api);
        server_api = NULL;
        test_relay_upgrade_remove_file ();
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);
    }

    /* Remove the relay upgrade file. */

    void test_relay_upgrade_remove_file ()
    {
        char *path;

        if (string_asprintf (&path, "%s/%s.upgrade",
                             weechat_data_dir, RELAY_UPGRADE_FILENAME) >= 0)
        {
            unlink (path);
            free (path);
        }
    }
};

/*
 * Test functions:
 *   relay_upgrade_save
 *   relay_upgrade_save_all_data
 *   relay_upgrade_load
 *   relay_upgrade_read_cb
 */

TEST(RelayUpgrade, SaveLoad)
{
    struct t_relay_client *client_weechat, *client_api, *ptr_client;
    char *desc_weechat, *desc_api, *nonce_weechat;
    int id_first, id_last, id_weechat, id_api, status_weechat, count;
    z_stream *ptr_strm;
    Bytef data_compressed[256], data_decompressed[256];
    Bytef dict_deflate[32768], dict_inflate[32768], dict[32768];
    uInt dict_size_deflate, dict_size_inflate, dict_size;
    int size_compressed;

    client_weechat = relay_client_new (-1, "1.2.3.4", server_weechat);
    CHECK(client_weechat);
    client_api = relay_client_new (-1, "5.6.7.8", server_api);
    CHECK(client_api);
    count = relay_client_count;

    client_weechat->bytes_recv = 123;
    client_weechat->bytes_sent = 456;
    client_weechat->partial_message = strdup ("partial");
    client_weechat->password_hash_algo = 2;
    RELAY_WEECHAT_DATA(client_weechat, compression) =
        RELAY_WEECHAT_COMPRESSION_ZLIB;
    RELAY_API_DATA(client_api, sync_enabled) = 1;
    RELAY_API_DATA(client_api, sync_nicks) = 1;
    relay_client_set_status (client_api, RELAY_STATUS_CONNECTED);

    /* Websocket with deflate streams (dictionaries are saved). */
    client_api->websocket = RELAY_CLIENT_WEBSOCKET_READY;
    client_api->ws_deflate->enabled = 1;
    client_api->ws_deflate->window_bits_deflate = 15;
    client_api->ws_deflate->window_bits_inflate = 15;
    client_api->ws_deflate->strm_deflate = (z_stream *)calloc (
        1, sizeof (*client_api->ws_deflate->strm_deflate));
    CHECK(client_api->ws_deflate->strm_deflate);
    LONGS_EQUAL(1, relay_websocket_deflate_init_stream_deflate (
                    client_api->ws_deflate));
    client_api->ws_deflate->strm_inflate = (z_stream *)calloc (
        1, sizeof (*client_api->ws_deflate->strm_inflate));
    CHECK(client_api->ws_deflate->strm_inflate);
    LONGS_EQUAL(1, relay_websocket_deflate_init_stream_inflate (
                    client_api->ws_deflate));
    /* Compress and decompress data, so that dictionaries are not empty. */
    ptr_strm = client_api->ws_deflate->strm_deflate;
    ptr_strm->next_in = (Bytef *)"hello world, hello world";
    ptr_strm->avail_in = strlen ("hello world, hello world");
    ptr_strm->next_out = data_compressed;
    ptr_strm->avail_out = sizeof (data_compressed);
    LONGS_EQUAL(Z_OK, deflate (ptr_strm, Z_SYNC_FLUSH));
    size_compressed = sizeof (data_compressed) - ptr_strm->avail_out;
    ptr_strm = client_api->ws_deflate->strm_inflate;
    ptr_strm->next_in = data_compressed;
    ptr_strm->avail_in = size_compressed;
    ptr_strm->next_out = data_decompressed;
    ptr_strm->avail_out = sizeof (data_decompressed);
    LONGS_EQUAL(Z_OK, inflate (ptr_strm, Z_SYNC_FLUSH));
    dict_size_deflate = sizeof (dict_deflate);
    LONGS_EQUAL(Z_OK,
                deflateGetDictionary (client_api->ws_deflate->strm_deflate,
                                      dict_deflate, &dict_size_deflate));
    CHECK(dict_size_deflate > 0);
    dict_size_inflate = sizeof (dict_inflate);
    LONGS_EQUAL(Z_OK,
                inflateGetDictionary (client_api->ws_deflate->strm_inflate,
                                      dict_inflate, &dict_size_inflate));
    CHECK(dict_size_inflate > 0);

    server_weechat->last_client_disconnect = TEST_DATE;

    relay_raw_message_add_to_list (TEST_DATE, 123456, "prefix1", "message1");
    relay_raw_message_add_to_list (TEST_DATE + 1, 0, "prefix2", "message2");

    id_first = relay_clients->id;
    id_last = last_relay_client->id;
    id_weechat = client_weechat->id;
    status_weechat = client_weechat->status;
    id_api = client_api->id;
    desc_weechat = strdup (client_weechat->desc);
    desc_api = strdup (client_api->desc);
    nonce_weechat = strdup (client_weechat->nonce);

    LONGS_EQUAL(1, relay_upgrade_save (0));

    /* Simulate an upgrade: free clients and raw messages. */
    relay_client_free_all ();
    relay_raw_message_free_all ();
    LONGS_EQUAL(0, relay_client_count);
    POINTERS_EQUAL(NULL, relay_raw_messages);
    server_weechat->last_client_disconnect = 0;

    LONGS_EQUAL(1, relay_upgrade_load ());

    LONGS_EQUAL(count, relay_client_count);
    LONGS_EQUAL(TEST_DATE, server_weechat->last_client_disconnect);

    /* Clients are restored in the same order. */
    LONGS_EQUAL(id_first, relay_clients->id);
    LONGS_EQUAL(id_last, last_relay_client->id);

    ptr_client = relay_client_search_by_id (id_weechat);
    CHECK(ptr_client);
    STRCMP_EQUAL(desc_weechat, ptr_client->desc);
    LONGS_EQUAL(-1, ptr_client->sock);
    POINTERS_EQUAL(NULL, ptr_client->hook_fd);
    LONGS_EQUAL(server_weechat->port, ptr_client->server_port);
    STRCMP_EQUAL("1.2.3.4", ptr_client->address);
    LONGS_EQUAL(RELAY_PROTOCOL_WEECHAT, ptr_client->protocol);
    STRCMP_EQUAL("weechat", ptr_client->protocol_string);
    LONGS_EQUAL(status_weechat, ptr_client->status);
    STRCMP_EQUAL(nonce_weechat, ptr_client->nonce);
    LONGS_EQUAL(2, ptr_client->password_hash_algo);
    CHECK(ptr_client->bytes_recv == 123);
    CHECK(ptr_client->bytes_sent == 456);
    STRCMP_EQUAL("partial", ptr_client->partial_message);
    CHECK(ptr_client->protocol_data);
    LONGS_EQUAL(RELAY_WEECHAT_COMPRESSION_ZLIB,
                RELAY_WEECHAT_DATA(ptr_client, compression));

    ptr_client = relay_client_search_by_id (id_api);
    CHECK(ptr_client);
    STRCMP_EQUAL(desc_api, ptr_client->desc);
    LONGS_EQUAL(server_api->port, ptr_client->server_port);
    STRCMP_EQUAL("5.6.7.8", ptr_client->address);
    LONGS_EQUAL(RELAY_PROTOCOL_API, ptr_client->protocol);
    STRCMP_EQUAL("api", ptr_client->protocol_string);
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, ptr_client->status);
    CHECK(ptr_client->protocol_data);
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_client, sync_enabled));
    LONGS_EQUAL(1, RELAY_API_DATA(ptr_client, sync_nicks));
    LONGS_EQUAL(RELAY_CLIENT_WEBSOCKET_READY, ptr_client->websocket);
    LONGS_EQUAL(1, ptr_client->ws_deflate->enabled);
    LONGS_EQUAL(15, ptr_client->ws_deflate->window_bits_deflate);
    LONGS_EQUAL(15, ptr_client->ws_deflate->window_bits_inflate);
    CHECK(ptr_client->ws_deflate->strm_deflate);
    CHECK(ptr_client->ws_deflate->strm_inflate);
    dict_size = sizeof (dict);
    LONGS_EQUAL(Z_OK,
                deflateGetDictionary (ptr_client->ws_deflate->strm_deflate,
                                      dict, &dict_size));
    LONGS_EQUAL(dict_size_deflate, dict_size);
    CHECK(memcmp (dict_deflate, dict, dict_size) == 0);
    dict_size = sizeof (dict);
    LONGS_EQUAL(Z_OK,
                inflateGetDictionary (ptr_client->ws_deflate->strm_inflate,
                                      dict, &dict_size));
    LONGS_EQUAL(dict_size_inflate, dict_size);
    CHECK(memcmp (dict_inflate, dict, dict_size) == 0);

    /* Raw messages are restored in the same order. */
    CHECK(relay_raw_messages);
    LONGS_EQUAL(TEST_DATE, relay_raw_messages->date);
    LONGS_EQUAL(123456, relay_raw_messages->date_usec);
    STRCMP_EQUAL("prefix1", relay_raw_messages->prefix);
    STRCMP_EQUAL("message1", relay_raw_messages->message);
    CHECK(relay_raw_messages->next_message);
    STRCMP_EQUAL("prefix2", relay_raw_messages->next_message->prefix);
    STRCMP_EQUAL("message2", relay_raw_messages->next_message->message);
    POINTERS_EQUAL(NULL, relay_raw_messages->next_message->next_message);

    free (desc_weechat);
    free (desc_api);
    free (nonce_weechat);
}

/*
 * Test functions:
 *   relay_upgrade_save (with force of disconnected state)
 *   relay_upgrade_load
 */

TEST(RelayUpgrade, SaveLoadForceDisconnected)
{
    struct t_relay_client *client;
    int id;

    client = relay_client_new (-1, "1.2.3.4", server_api);
    CHECK(client);
    relay_client_set_status (client, RELAY_STATUS_CONNECTED);
    id = client->id;

    LONGS_EQUAL(1, relay_upgrade_save (1));

    relay_client_free_all ();
    LONGS_EQUAL(1, relay_upgrade_load ());

    client = relay_client_search_by_id (id);
    CHECK(client);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client->status);
    CHECK(client->end_time > 0);
}

/*
 * Test functions:
 *   relay_upgrade_load (no upgrade file)
 */

TEST(RelayUpgrade, LoadNoFile)
{
    test_relay_upgrade_remove_file ();
    LONGS_EQUAL(0, relay_upgrade_load ());
    LONGS_EQUAL(0, relay_client_count);
}

/*
 * Test functions:
 *   relay_upgrade_read_cb (data saved by old versions, unknown server)
 */

TEST(RelayUpgrade, ReadCbOldVersion)
{
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    struct t_relay_client *client;

    infolist = infolist_new (NULL);
    CHECK(infolist);

    /*
     * Client saved by an old version: "ssl" instead of "tls" (< 4.0.0),
     * no "nonce" and "password_hash_algo" (< 2.9), no "desc".
     */
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_integer (ptr_item, "id", 42));
    CHECK(infolist_new_var_integer (ptr_item, "sock", -1));
    CHECK(infolist_new_var_integer (ptr_item, "server_port", 9000));
    CHECK(infolist_new_var_integer (ptr_item, "ssl", 0));
    CHECK(infolist_new_var_string (ptr_item, "address", "1.2.3.4"));
    CHECK(infolist_new_var_string (ptr_item, "real_ip", "5.6.7.8"));
    CHECK(infolist_new_var_integer (ptr_item, "status",
                                    RELAY_STATUS_DISCONNECTED));
    CHECK(infolist_new_var_integer (ptr_item, "protocol",
                                    RELAY_PROTOCOL_WEECHAT));
    CHECK(infolist_new_var_string (ptr_item, "protocol_string",
                                   "unix.weechat"));
    CHECK(infolist_new_var_string (ptr_item, "bytes_recv", "10"));
    CHECK(infolist_new_var_string (ptr_item, "bytes_sent", "20"));
    CHECK(infolist_new_var_buffer (ptr_item, "partial_ws_frame",
                                   (void *)"abc", 3));

    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_upgrade_read_cb (NULL, NULL, NULL,
                                       RELAY_UPGRADE_TYPE_CLIENT, infolist));
    infolist_free (infolist);

    client = relay_client_search_by_id (42);
    CHECK(client);
    POINTERS_EQUAL(client, relay_clients);
    STRCMP_EQUAL("42/weechat/1.2.3.4(5.6.7.8)", client->desc);
    LONGS_EQUAL(0, client->tls);
    STRCMP_EQUAL("5.6.7.8", client->real_ip);
    CHECK(client->nonce);
    CHECK(client->nonce[0]);
    LONGS_EQUAL(0, client->password_hash_algo);
    CHECK(client->bytes_recv == 10);
    CHECK(client->bytes_sent == 20);
    LONGS_EQUAL(3, client->partial_ws_frame_size);
    CHECK(memcmp (client->partial_ws_frame, "abc", 3) == 0);
    LONGS_EQUAL(0, RELAY_WEECHAT_DATA(client, handshake_done));
    LONGS_EQUAL(1, RELAY_WEECHAT_DATA(client, totp_ok));

    /* Server: unknown protocol is ignored. */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "protocol_string", "xxx"));
    CHECK(infolist_new_var_time (ptr_item, "last_client_disconnect",
                                 TEST_DATE));
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_time (ptr_item, "last_client_disconnect",
                                 TEST_DATE));
    ptr_item = infolist_new_item (infolist);
    CHECK(ptr_item);
    CHECK(infolist_new_var_string (ptr_item, "protocol_string", "unix.api"));
    CHECK(infolist_new_var_time (ptr_item, "last_client_disconnect",
                                 TEST_DATE));
    server_weechat->last_client_disconnect = 0;
    server_api->last_client_disconnect = 0;
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_upgrade_read_cb (NULL, NULL, NULL,
                                       RELAY_UPGRADE_TYPE_SERVER, infolist));
    infolist_free (infolist);
    LONGS_EQUAL(0, server_weechat->last_client_disconnect);
    LONGS_EQUAL(TEST_DATE, server_api->last_client_disconnect);

    /* Unknown object (ignored). */
    infolist = infolist_new (NULL);
    CHECK(infolist);
    CHECK(infolist_new_item (infolist));
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_upgrade_read_cb (NULL, NULL, NULL, 999, infolist));
    infolist_free (infolist);
}

/*
 * Test functions:
 *   relay_upgrade_set_buffer_callbacks
 */

TEST(RelayUpgrade, SetBufferCallbacks)
{
    struct t_gui_buffer *buffer, *raw_buffer;

    relay_buffer_open ();
    relay_raw_open (0);
    buffer = relay_buffer;
    raw_buffer = relay_raw_buffer;
    CHECK(buffer);
    CHECK(raw_buffer);

    /* Simulate an upgrade: pointers and callbacks are lost. */
    relay_buffer = NULL;
    relay_raw_buffer = NULL;
    buffer->close_callback = NULL;
    buffer->input_callback = NULL;
    raw_buffer->close_callback = NULL;
    raw_buffer->input_callback = NULL;

    relay_upgrade_set_buffer_callbacks ();

    POINTERS_EQUAL(buffer, relay_buffer);
    POINTERS_EQUAL(raw_buffer, relay_raw_buffer);
    POINTERS_EQUAL(&relay_buffer_close_cb, buffer->close_callback);
    POINTERS_EQUAL(&relay_buffer_input_cb, buffer->input_callback);
    POINTERS_EQUAL(&relay_buffer_close_cb, raw_buffer->close_callback);
    POINTERS_EQUAL(&relay_buffer_input_cb, raw_buffer->input_callback);

    gui_buffer_close (buffer);
    gui_buffer_close (raw_buffer);
    POINTERS_EQUAL(NULL, relay_buffer);
    POINTERS_EQUAL(NULL, relay_raw_buffer);
}
