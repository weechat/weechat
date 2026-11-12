/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay "weechat" protocol (protocol) */

#include <string>
#include <vector>
#include <algorithm>

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <gcrypt.h>
#include "src/core/core-config.h"
#include "src/core/core-config-file.h"
#include "src/core/core-crypto.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/core/hook/hook-timer.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-nicklist.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-auth.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-server.h"
#include "src/plugins/relay/weechat/relay-weechat.h"
#include "src/plugins/relay/weechat/relay-weechat-protocol.h"

extern struct t_gui_buffer *relay_weechat_protocol_get_buffer (const char *arg);
extern int relay_weechat_protocol_sync_flag (const char *flag);
extern int relay_weechat_protocol_is_sync (struct t_relay_client *ptr_client,
                                           struct t_gui_buffer *buffer,
                                           int flags);
}

#define TEST_MAX_MSGS 16

#define CHECK_PREFIX(__prefix, __string)                                \
    STRNCMP_EQUAL(__prefix, (__string).c_str (), strlen (__prefix));

/* Messages sent to the client (captured by the fake send function). */
int test_msgs_count = 0;
char *test_msgs[TEST_MAX_MSGS];
int test_msgs_size[TEST_MAX_MSGS];

/*
 * Reader of a binary message sent with "weechat" protocol.
 */

struct t_test_msg_reader
{
    const unsigned char *data;         /* message data                      */
    int size;                          /* message size                      */
    int pos;                           /* current position in data          */
    int error;                         /* 1 if an error occurred            */
    int mask_pointers;                 /* 1 to display "0x?" for pointers   */
};

/*
 * Read bytes in a message.
 *
 * Return:
 *   1: OK
 *   0: error (not enough data)
 */

int
test_msg_read_bytes (struct t_test_msg_reader *reader, void *buffer, int length)
{
    if (reader->error || (length < 0)
        || (reader->pos + length > reader->size))
    {
        reader->error = 1;
        return 0;
    }
    memcpy (buffer, reader->data + reader->pos, length);
    reader->pos += length;
    return 1;
}

/*
 * Read an integer (4 bytes, big endian) in a message.
 */

int
test_msg_read_int (struct t_test_msg_reader *reader)
{
    uint32_t value32;

    if (!test_msg_read_bytes (reader, &value32, 4))
        return 0;
    return (int)ntohl (value32);
}

/*
 * Read a string (length on 4 bytes + content) in a message.
 *
 * Return "null" for a NULL string, otherwise the string, quoted if "quote"
 * is true.
 */

std::string
test_msg_read_str (struct t_test_msg_reader *reader, bool quote)
{
    int length;
    std::string str;

    length = test_msg_read_int (reader);
    if (reader->error)
        return "";
    if (length < 0)
        return "null";
    if (reader->pos + length > reader->size)
    {
        reader->error = 1;
        return "";
    }
    str.assign ((const char *)reader->data + reader->pos, length);
    reader->pos += length;
    return (quote) ? "\"" + str + "\"" : str;
}

/*
 * Read a short string (length on 1 byte + content) in a message.
 */

std::string
test_msg_read_short_str (struct t_test_msg_reader *reader)
{
    unsigned char length;
    char buffer[256];

    if (!test_msg_read_bytes (reader, &length, 1))
        return "";
    if (!test_msg_read_bytes (reader, buffer, length))
        return "";
    return std::string (buffer, length);
}

/*
 * Read an object type (3 chars) in a message.
 */

std::string
test_msg_read_type (struct t_test_msg_reader *reader)
{
    char type[3];

    if (!test_msg_read_bytes (reader, type, 3))
        return "";
    return std::string (type, 3);
}

/*
 * Read a value of the given type in a message and return it as a string.
 */

std::string
test_msg_read_value (struct t_test_msg_reader *reader, const std::string &type)
{
    std::string str, str_ptr, type_keys, type_values, path, keys;
    std::vector<std::string> items, list_keys, list_types;
    char chr;
    int i, j, count, count_vars, num_path;
    size_t pos, pos2;

    if (type == "chr")
    {
        if (!test_msg_read_bytes (reader, &chr, 1))
            return "";
        return std::to_string ((int)chr);
    }
    if (type == "int")
        return std::to_string (test_msg_read_int (reader));
    if ((type == "lon") || (type == "tim"))
        return test_msg_read_short_str (reader);
    if ((type == "str") || (type == "buf"))
        return test_msg_read_str (reader, true);
    if (type == "ptr")
    {
        str_ptr = test_msg_read_short_str (reader);
        if (reader->mask_pointers && (str_ptr != "0"))
            return "0x?";
        return "0x" + str_ptr;
    }
    if (type == "htb")
    {
        /* Hashtable: entries are sorted by key, to get a stable output. */
        type_keys = test_msg_read_type (reader);
        type_values = test_msg_read_type (reader);
        count = test_msg_read_int (reader);
        for (i = 0; (i < count) && !reader->error; i++)
        {
            str = test_msg_read_value (reader, type_keys);
            str += ":";
            str += test_msg_read_value (reader, type_values);
            items.push_back (str);
        }
        std::sort (items.begin (), items.end ());
        str = "{";
        for (i = 0; i < (int)items.size (); i++)
        {
            if (i > 0)
                str += ",";
            str += items[i];
        }
        return str + "}";
    }
    if (type == "arr")
    {
        type_values = test_msg_read_type (reader);
        count = test_msg_read_int (reader);
        str = type_values + "[";
        for (i = 0; (i < count) && !reader->error; i++)
        {
            if (i > 0)
                str += ",";
            str += test_msg_read_value (reader, type_values);
        }
        return str + "]";
    }
    if (type == "inf")
    {
        str = test_msg_read_str (reader, false);
        return str + "=" + test_msg_read_str (reader, true);
    }
    if (type == "inl")
    {
        /* Infolist: name, then items with "name:type=value" variables. */
        str = test_msg_read_str (reader, false);
        count = test_msg_read_int (reader);
        for (i = 0; (i < count) && !reader->error; i++)
        {
            str += "{";
            count_vars = test_msg_read_int (reader);
            for (j = 0; (j < count_vars) && !reader->error; j++)
            {
                if (j > 0)
                    str += ",";
                str += test_msg_read_str (reader, false);
                type_values = test_msg_read_type (reader);
                str += ":" + type_values + "=";
                str += test_msg_read_value (reader, type_values);
            }
            str += "}";
        }
        return str;
    }
    if (type == "hda")
    {
        /*
         * Hdata: path and keys, then items: "{ptr1/ptr2|key=value,...}"
         * (one pointer per element in path).
         */
        path = test_msg_read_str (reader, false);
        keys = test_msg_read_str (reader, false);
        count = test_msg_read_int (reader);
        str = path + "[" + keys + "]";
        if (reader->error || (path == "null") || (keys == "null"))
            return str;
        num_path = 1 + (int)std::count (path.begin (), path.end (), '/');
        pos = 0;
        while (pos < keys.size ())
        {
            pos2 = keys.find (',', pos);
            if (pos2 == std::string::npos)
                pos2 = keys.size ();
            str_ptr = keys.substr (pos, pos2 - pos);
            list_keys.push_back (str_ptr.substr (0, str_ptr.find (':')));
            list_types.push_back (str_ptr.substr (str_ptr.find (':') + 1));
            pos = pos2 + 1;
        }
        for (i = 0; (i < count) && !reader->error; i++)
        {
            str += "{";
            for (j = 0; j < num_path; j++)
            {
                if (j > 0)
                    str += "/";
                str += test_msg_read_value (reader, "ptr");
            }
            str += "|";
            for (j = 0; j < (int)list_keys.size (); j++)
            {
                if (j > 0)
                    str += ",";
                str += list_keys[j] + "=";
                str += test_msg_read_value (reader, list_types[j]);
            }
            str += "}";
        }
        return str;
    }

    reader->error = 1;
    return "";
}

/*
 * Decode a binary message sent with "weechat" protocol and return it as a
 * string, for example: "(id) str:"value" int:123".
 *
 * If "mask_pointers" is 1, non-NULL pointers are displayed as "0x?".
 */

std::string
test_msg_decode (const char *data, int size, int mask_pointers)
{
    struct t_test_msg_reader reader;
    std::string str, type;
    char compression;

    reader.data = (const unsigned char *)data;
    reader.size = size;
    reader.pos = 0;
    reader.error = 0;
    reader.mask_pointers = mask_pointers;

    /* Header: size (4 bytes) and compression (1 byte). */
    if (test_msg_read_int (&reader) != size)
        return "error: invalid size";
    if (!test_msg_read_bytes (&reader, &compression, 1))
        return "error: no compression";
    if (compression != RELAY_WEECHAT_COMPRESSION_OFF)
        return "error: compressed message";

    str = "(" + test_msg_read_str (&reader, false) + ")";

    while (!reader.error && (reader.pos < reader.size))
    {
        type = test_msg_read_type (&reader);
        str += " " + type + ":" + test_msg_read_value (&reader, type);
    }

    if (reader.error)
        str += " error";

    return str;
}

TEST_GROUP(RelayWeechatProtocol)
{
    struct t_relay_server *server;

    void setup ()
    {
        /* Disable auto-open of relay buffer (it would pollute other tests). */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);
        config_file_option_set (relay_config_network_password, "secret", 1);

        server = relay_server_new ("weechat", RELAY_PROTOCOL_WEECHAT, NULL,
                                   9000,
                                   NULL,  /* path */
                                   1,  /* ipv4 */
                                   0,  /* ipv6 */
                                   0,  /* tls */
                                   0);  /* unix_socket */
        CHECK(server);
    }

    void teardown ()
    {
        relay_server_free (server);
        config_file_option_reset (relay_config_network_totp_secret, 1);
        config_file_option_reset (relay_config_network_totp_window, 1);
        config_file_option_reset (relay_config_network_password, 1);
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);
    }

    /*
     * Build a hashed password for a client with algo "sha256": the salt is
     * the server nonce followed by a client nonce ("1234").
     */

    void build_password_hash (struct t_relay_client *client,
                              const char *password,
                              char *password_hash, int size)
    {
        char salt_hexa[1024], salt[1024], salt_pass[2048];
        char hash[1024], hash_hexa[2048];
        int salt_size, hash_size;

        snprintf (salt_hexa, sizeof (salt_hexa), "%s1234", client->nonce);
        salt_size = string_base_decode ("16", salt_hexa, salt);
        CHECK(salt_size > 0);
        memcpy (salt_pass, salt, salt_size);
        memcpy (salt_pass + salt_size, password, strlen (password));
        LONGS_EQUAL(1, weecrypto_hash (salt_pass,
                                       salt_size + strlen (password),
                                       GCRY_MD_SHA256, hash, &hash_size));
        string_base_encode ("16", hash, hash_size, hash_hexa);
        snprintf (password_hash, size, "sha256:%s:%s", salt_hexa, hash_hexa);
    }

    /*
     * Send command "init" from a new client, with password hash algo
     * negotiated, then return the client status.
     *
     * In the command, "{good_hash}" and "{bad_hash}" are replaced by the
     * password hashed with "sha256", for "secret" and "wrong".
     */

    int init (int password_hash_algo, const char *options)
    {
        struct t_relay_client *client;
        char good_hash[4096], bad_hash[4096], *command, *command2;
        int status;

        client = relay_client_new (-1, "test", server);
        CHECK(client);
        client->password_hash_algo = password_hash_algo;

        build_password_hash (client, "secret", good_hash, sizeof (good_hash));
        build_password_hash (client, "wrong", bad_hash, sizeof (bad_hash));
        command = string_replace (options, "{good_hash}", good_hash);
        command2 = string_replace (command, "{bad_hash}", bad_hash);
        free (command);

        relay_weechat_protocol_recv (client, command2);
        free (command2);

        status = client->status;
        relay_client_free (client);

        return status;
    }
};

/*
 * Test functions:
 *   relay_weechat_protocol_cb_init
 */

TEST(RelayWeechatProtocol, CbInitPassword)
{
    /* Plain text password */
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, "init password=secret"));
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, "init password=wrong"));

    /* Hashed password */
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                init (RELAY_AUTH_PASSWORD_HASH_SHA256,
                      "init password_hash={good_hash}"));
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_SHA256,
                      "init password_hash={bad_hash}"));

    /* Only the first plain text password is checked */
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN,
                      "init password=secret,password=wrong"));
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN,
                      "init password=wrong,password=secret"));

    /* Only the first hashed password is checked */
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                init (RELAY_AUTH_PASSWORD_HASH_SHA256,
                      "init password_hash={good_hash},"
                      "password_hash={bad_hash}"));
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_SHA256,
                      "init password_hash={bad_hash},"
                      "password_hash={good_hash}"));
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_SHA256,
                      "init password_hash={bad_hash},"
                      "password_hash={bad_hash},"
                      "password_hash={bad_hash},"
                      "password_hash={good_hash}"));

    /* Only the first password is checked, plain text or hashed */
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_SHA256,
                      "init password=secret,password_hash={good_hash}"));
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN,
                      "init password_hash={good_hash},password=secret"));
}

/*
 * Test functions:
 *   relay_weechat_protocol_cb_init
 */

TEST(RelayWeechatProtocol, CbInitTotp)
{
    char command[256], *totp, *totp2;

    config_file_option_set (relay_config_network_totp_secret,
                            "secretbase32", 1);
    config_file_option_set (relay_config_network_totp_window, "1", 1);

    totp = hook_info_get (NULL, "totp_generate", "secretbase32");
    CHECK(totp);
    totp2 = strdup (totp);
    CHECK(totp2);
    totp2[0] = (totp2[0] == '1') ? '2' : '1';

    /* Missing TOTP */
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, "init password=secret"));

    /* Valid TOTP */
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s", totp);
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));

    /* Invalid TOTP */
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s", totp2);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));

    /* Last 4 or 5 digits of the valid TOTP are rejected */
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s", totp + 2);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s", totp + 1);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));

    /* Extra arguments for info "totp_validate" (escaped commas) are rejected */
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s\\,0\\,1\\,4", totp + 2);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));

    /* Only the first TOTP is checked */
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s,totp=%s", totp, totp2);
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s,totp=%s", totp2, totp);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));

    free (totp);
    free (totp2);
}

TEST_GROUP(RelayWeechatProtocolWithClient)
{
    struct t_relay_server *server;
    struct t_relay_client *client;

    static void
    fake_send_func (void *client, const char *data, int data_size)
    {
        (void) client;

        if (test_msgs_count >= TEST_MAX_MSGS)
            return;
        test_msgs[test_msgs_count] = (char *)malloc (data_size);
        memcpy (test_msgs[test_msgs_count], data, data_size);
        test_msgs_size[test_msgs_count] = data_size;
        test_msgs_count++;
    }

    void free_msgs ()
    {
        int i;

        for (i = 0; i < test_msgs_count; i++)
        {
            free (test_msgs[i]);
            test_msgs[i] = NULL;
        }
        test_msgs_count = 0;
    }

    /*
     * Create a new client, not authenticated.
     */

    struct t_relay_client *new_client ()
    {
        struct t_relay_client *new_client;

        new_client = relay_client_new (-1, "test", server);
        CHECK(new_client);
        new_client->fake_send_func = &fake_send_func;
        return new_client;
    }

    /*
     * Send a command from the client, after removing messages already sent.
     */

    void recv (struct t_relay_client *ptr_client, const char *data)
    {
        free_msgs ();
        relay_weechat_protocol_recv (ptr_client, data);
    }

    /*
     * Return a message sent to the client, decoded.
     */

    std::string msg (int index, int mask_pointers)
    {
        if (index >= test_msgs_count)
            return "error: no message";
        return test_msg_decode (test_msgs[index], test_msgs_size[index],
                                mask_pointers);
    }

    void setup ()
    {
        /* Disable auto-open of relay buffer (it would pollute other tests). */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);

        /* Disable compression, so that messages can be decoded. */
        config_file_option_set (relay_config_network_compression, "0", 1);

        test_msgs_count = 0;

        server = relay_server_new ("weechat", RELAY_PROTOCOL_WEECHAT, NULL,
                                   9000,
                                   NULL,  /* path */
                                   1,  /* ipv4 */
                                   0,  /* ipv6 */
                                   0,  /* tls */
                                   0);  /* unix_socket */
        CHECK(server);

        /* Authenticated client. */
        client = new_client ();
        RELAY_WEECHAT_DATA(client, handshake_done) = 1;
        RELAY_WEECHAT_DATA(client, password_ok) = 1;
        RELAY_WEECHAT_DATA(client, totp_ok) = 1;
        client->status = RELAY_STATUS_CONNECTED;
    }

    void teardown ()
    {
        relay_client_free (client);
        relay_server_free (server);
        free_msgs ();
        config_file_option_reset (relay_config_network_compression, 1);
        config_file_option_reset (relay_config_network_password_hash_algo, 1);
        config_file_option_reset (relay_config_network_password_hash_iterations, 1);
        config_file_option_reset (relay_config_network_totp_secret, 1);
        config_file_option_reset (relay_config_network_commands, 1);
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);
    }
};

/*
 * Test functions:
 *   relay_weechat_protocol_get_buffer
 */

TEST(RelayWeechatProtocolWithClient, GetBuffer)
{
    struct t_gui_buffer *buffer;
    char str_pointer[64];

    POINTERS_EQUAL(NULL, relay_weechat_protocol_get_buffer ("unknown"));
    POINTERS_EQUAL(NULL, relay_weechat_protocol_get_buffer ("core.unknown"));
    POINTERS_EQUAL(NULL, relay_weechat_protocol_get_buffer ("0x"));
    POINTERS_EQUAL(NULL, relay_weechat_protocol_get_buffer ("0x0"));
    POINTERS_EQUAL(NULL, relay_weechat_protocol_get_buffer ("0xzzz"));
    POINTERS_EQUAL(NULL, relay_weechat_protocol_get_buffer ("0x1234"));

    buffer = gui_buffer_search_main ();
    CHECK(buffer);

    POINTERS_EQUAL(buffer, relay_weechat_protocol_get_buffer ("core.weechat"));

    snprintf (str_pointer, sizeof (str_pointer),
              "0x%lx", (unsigned long)buffer);
    POINTERS_EQUAL(buffer, relay_weechat_protocol_get_buffer (str_pointer));
}

/*
 * Test functions:
 *   relay_weechat_protocol_sync_flag
 */

TEST(RelayWeechatProtocolWithClient, SyncFlag)
{
    LONGS_EQUAL(0, relay_weechat_protocol_sync_flag (""));
    LONGS_EQUAL(0, relay_weechat_protocol_sync_flag ("unknown"));
    LONGS_EQUAL(0, relay_weechat_protocol_sync_flag ("BUFFER"));

    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER,
                relay_weechat_protocol_sync_flag ("buffer"));
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST,
                relay_weechat_protocol_sync_flag ("nicklist"));
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS,
                relay_weechat_protocol_sync_flag ("buffers"));
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_UPGRADE,
                relay_weechat_protocol_sync_flag ("upgrade"));
}

/*
 * Test functions:
 *   relay_weechat_protocol_recv
 */

TEST(RelayWeechatProtocolWithClient, Recv)
{
    struct t_relay_client *client2;

    /* Empty data or unknown command: nothing sent. */
    recv (client, NULL);
    LONGS_EQUAL(0, test_msgs_count);
    recv (client, "");
    LONGS_EQUAL(0, test_msgs_count);
    recv (client, "unknown");
    LONGS_EQUAL(0, test_msgs_count);
    recv (client, "(id) unknown arg");
    LONGS_EQUAL(0, test_msgs_count);
    recv (client, "PING");
    LONGS_EQUAL(0, test_msgs_count);
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, client->status);

    /* Message id. */
    recv (client, "(my_id) info version_number");
    LONGS_EQUAL(1, test_msgs_count);
    CHECK_PREFIX("(my_id) inf:version_number=\"", msg (0, 0));
    recv (client, "(my_id)   info version_number");
    LONGS_EQUAL(1, test_msgs_count);
    CHECK_PREFIX("(my_id) inf:version_number=\"", msg (0, 0));
    recv (client, "() info version_number");
    LONGS_EQUAL(1, test_msgs_count);
    CHECK_PREFIX("() inf:version_number=\"", msg (0, 0));
    recv (client, "info version_number");
    LONGS_EQUAL(1, test_msgs_count);
    CHECK_PREFIX("(null) inf:version_number=\"", msg (0, 0));

    /* Unterminated id: "(my_id" is the command. */
    recv (client, "(my_id info version_number");
    LONGS_EQUAL(0, test_msgs_count);

    /* Too few arguments: command ignored. */
    recv (client, "info");
    LONGS_EQUAL(0, test_msgs_count);
    recv (client, "info   ");
    LONGS_EQUAL(0, test_msgs_count);

    /* Escaped chars are interpreted only if enabled in handshake. */
    recv (client, "ping a\\tb");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(_pong) str:\"a\\tb\"", msg (0, 0).c_str ());
    RELAY_WEECHAT_DATA(client, escape_commands) = 1;
    recv (client, "ping a\\tb");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(_pong) str:\"a\tb\"", msg (0, 0).c_str ());
    recv (client, "(\\x41) ping");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(_pong) str:\"\"", msg (0, 0).c_str ());
    RELAY_WEECHAT_DATA(client, escape_commands) = 0;

    /* Client not authenticated: only "handshake" and "init" are allowed. */
    client2 = new_client ();
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client2->status);
    recv (client2, "ping");
    LONGS_EQUAL(0, test_msgs_count);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED, client2->status);
    relay_client_free (client2);

    /* Unknown command is ignored even if client is not authenticated. */
    client2 = new_client ();
    recv (client2, "unknown");
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client2->status);
    relay_client_free (client2);

    /* Client disconnected: commands are ignored. */
    client->status = RELAY_STATUS_DISCONNECTED;
    recv (client, "ping");
    LONGS_EQUAL(0, test_msgs_count);
    client->status = RELAY_STATUS_CONNECTED;
}

/*
 * Test functions:
 *   relay_weechat_protocol_handshake_reply
 *   relay_weechat_protocol_cb_handshake
 */

TEST(RelayWeechatProtocolWithClient, CbHandshake)
{
    struct t_relay_client *client2;
    char expected[1024];

    config_file_option_set (relay_config_network_password_hash_iterations,
                            "1234", 1);

    /* No options: plain text password. */
    client2 = new_client ();
    recv (client2, "(hs) handshake");
    LONGS_EQUAL(1, test_msgs_count);
    snprintf (expected, sizeof (expected),
              "(hs) htb:{\"compression\":\"off\","
              "\"escape_commands\":\"off\","
              "\"nonce\":\"%s\","
              "\"password_hash_algo\":\"plain\","
              "\"password_hash_iterations\":\"1234\","
              "\"rgb_colors\":\"off\","
              "\"totp\":\"off\"}",
              client2->nonce);
    STRCMP_EQUAL(expected, msg (0, 0).c_str ());
    LONGS_EQUAL(RELAY_AUTH_PASSWORD_HASH_PLAIN, client2->password_hash_algo);
    LONGS_EQUAL(1, RELAY_WEECHAT_DATA(client2, handshake_done));
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client2->status);

    /* Only one handshake is allowed. */
    recv (client2, "handshake");
    LONGS_EQUAL(0, test_msgs_count);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED, client2->status);
    relay_client_free (client2);

    /* Strongest algorithm is used, first supported compression. */
    client2 = new_client ();
    recv (client2,
          "handshake password_hash_algo=plain:pbkdf2+sha256:sha512:unknown,"
          "compression=unknown:zlib:off,escape_commands=on,unknown=value,"
          "rgb_colors=on,invalid");
    LONGS_EQUAL(1, test_msgs_count);
    snprintf (expected, sizeof (expected),
              "(null) htb:{\"compression\":\"zlib\","
              "\"escape_commands\":\"on\","
              "\"nonce\":\"%s\","
              "\"password_hash_algo\":\"pbkdf2+sha256\","
              "\"password_hash_iterations\":\"1234\","
              "\"rgb_colors\":\"on\","
              "\"totp\":\"off\"}",
              client2->nonce);
    STRCMP_EQUAL(expected, msg (0, 0).c_str ());
    LONGS_EQUAL(RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA256,
                client2->password_hash_algo);
    LONGS_EQUAL(RELAY_WEECHAT_COMPRESSION_ZLIB,
                RELAY_WEECHAT_DATA(client2, compression));
    LONGS_EQUAL(1, RELAY_WEECHAT_DATA(client2, escape_commands));
    LONGS_EQUAL(1, RELAY_WEECHAT_DATA(client2, rgb_colors));
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client2->status);
    relay_client_free (client2);

    /* Escape commands: only "on" enables it. */
    client2 = new_client ();
    recv (client2, "handshake escape_commands=ON");
    LONGS_EQUAL(0, RELAY_WEECHAT_DATA(client2, escape_commands));
    relay_client_free (client2);

    /* RGB colors: only "on" enables it. */
    client2 = new_client ();
    recv (client2, "handshake rgb_colors=ON");
    LONGS_EQUAL(0, RELAY_WEECHAT_DATA(client2, rgb_colors));
    relay_client_free (client2);

    /* No supported algorithm: authentication failed. */
    client2 = new_client ();
    recv (client2, "handshake password_hash_algo=unknown");
    LONGS_EQUAL(1, test_msgs_count);
    CHECK(strstr (msg (0, 0).c_str (), "\"password_hash_algo\":\"\""));
    LONGS_EQUAL(-1, client2->password_hash_algo);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED, client2->status);
    relay_client_free (client2);

    /* Algorithms restricted by option. */
    config_file_option_set (relay_config_network_password_hash_algo,
                            "*,!plain,!pbkdf2*", 1);
    client2 = new_client ();
    recv (client2, "handshake password_hash_algo=plain:sha256:pbkdf2+sha512");
    LONGS_EQUAL(RELAY_AUTH_PASSWORD_HASH_SHA256, client2->password_hash_algo);
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING, client2->status);
    relay_client_free (client2);
    client2 = new_client ();
    recv (client2, "handshake password_hash_algo=plain:pbkdf2+sha512");
    LONGS_EQUAL(-1, client2->password_hash_algo);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED, client2->status);
    relay_client_free (client2);

    /* No algorithm received and plain text not allowed. */
    client2 = new_client ();
    recv (client2, "handshake compression=zlib");
    LONGS_EQUAL(-1, client2->password_hash_algo);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED, client2->status);
    relay_client_free (client2);
    config_file_option_reset (relay_config_network_password_hash_algo, 1);

    /* TOTP enabled. */
    config_file_option_set (relay_config_network_totp_secret,
                            "secretbase32", 1);
    client2 = new_client ();
    recv (client2, "handshake");
    CHECK(strstr (msg (0, 0).c_str (), "\"totp\":\"on\""));
    relay_client_free (client2);
    config_file_option_reset (relay_config_network_totp_secret, 1);

    /* Handshake is ignored if client is already authenticated. */
    RELAY_WEECHAT_DATA(client, handshake_done) = 0;
    recv (client, "handshake");
    LONGS_EQUAL(0, test_msgs_count);
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, client->status);
}

/*
 * Test functions:
 *   relay_weechat_protocol_cb_hdata
 */

TEST(RelayWeechatProtocolWithClient, CbHdata)
{
    /* Too few arguments: nothing sent. */
    recv (client, "hdata");
    LONGS_EQUAL(0, test_msgs_count);

    /* Invalid hdata: empty hdata. */
    recv (client, "(h) hdata unknown:gui_buffers(*)");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(h) hda:null[null]", msg (0, 1).c_str ());
    recv (client, "(h) hdata buffer:unknown_list(*)");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(h) hda:null[null]", msg (0, 1).c_str ());

    /* Core buffer (first buffer). */
    recv (client, "(h) hdata buffer:gui_buffers(1) number,full_name");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(h) hda:buffer[number:int,full_name:str]"
                 "{0x?|number=1,full_name=\"core.weechat\"}",
                 msg (0, 1).c_str ());

    /* Unknown keys are ignored. */
    recv (client, "(h) hdata buffer:gui_buffers(1) number,unknown");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(h) hda:buffer[number:int]{0x?|number=1}",
                 msg (0, 1).c_str ());

    /* Path with sub-hdata. */
    recv (client, "(h) hdata buffer:gui_buffers(1)/own_lines lines_count");
    LONGS_EQUAL(1, test_msgs_count);
    CHECK_PREFIX("(h) hda:buffer/lines[lines_count:int]{0x?/0x?|lines_count=",
                 msg (0, 1));
}

/*
 * Test functions:
 *   relay_weechat_protocol_cb_info
 */

TEST(RelayWeechatProtocolWithClient, CbInfo)
{
    char *version, expected[256];

    /* Too few arguments: nothing sent. */
    recv (client, "info");
    LONGS_EQUAL(0, test_msgs_count);

    recv (client, "(i) info unknown_info");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(i) inf:unknown_info=null", msg (0, 0).c_str ());

    version = hook_info_get (NULL, "version", NULL);
    CHECK(version);
    snprintf (expected, sizeof (expected),
              "(i) inf:version=\"%s\"", version);
    free (version);
    recv (client, "(i) info version");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL(expected, msg (0, 0).c_str ());

    /* Info with arguments. */
    recv (client, "(i) info nick_color_name alice");
    LONGS_EQUAL(1, test_msgs_count);
    CHECK_PREFIX("(i) inf:nick_color_name=\"", msg (0, 0));

    /* RGB color of nick: replaced if client does not support RGB colors. */
    config_file_option_set (config_color_chat_nick_colors, "#ff8800", 1);
    recv (client, "(i) info nick_color_name alice");
    LONGS_EQUAL(1, test_msgs_count);
    snprintf (expected, sizeof (expected), "(i) inf:nick_color_name=\"%d\"",
              gui_color_convert_rgb_to_term_display (0xff8800, 256));
    STRCMP_EQUAL(expected, msg (0, 0).c_str ());
    recv (client, "(i) info nick_color_name_ignore_case alice;30");
    LONGS_EQUAL(1, test_msgs_count);
    snprintf (expected, sizeof (expected),
              "(i) inf:nick_color_name_ignore_case=\"%d\"",
              gui_color_convert_rgb_to_term_display (0xff8800, 256));
    STRCMP_EQUAL(expected, msg (0, 0).c_str ());
    config_file_option_reset (config_color_chat_nick_colors, 1);
}

/*
 * Test functions:
 *   relay_weechat_protocol_cb_infolist
 */

TEST(RelayWeechatProtocolWithClient, CbInfolist)
{
    struct t_gui_buffer *buffer;
    char command[256];
    std::string str;

    /* Too few arguments: nothing sent. */
    recv (client, "infolist");
    LONGS_EQUAL(0, test_msgs_count);

    /* Unknown infolist: message without objects. */
    recv (client, "(l) infolist unknown");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(l)", msg (0, 0).c_str ());

    /* Infolist with a pointer. */
    buffer = gui_buffer_search_main ();
    snprintf (command, sizeof (command),
              "(l) infolist buffer 0x%lx", (unsigned long)buffer);
    recv (client, command);
    LONGS_EQUAL(1, test_msgs_count);
    str = msg (0, 1);
    CHECK_PREFIX("(l) inl:buffer{pointer:ptr=0x?,current_buffer:int=1,"
                 "id:lon=", str);
    CHECK(str.find ("full_name:str=\"core.weechat\"") != std::string::npos);
    CHECK(str.find ("}{") == std::string::npos);
    CHECK(str.find ("error") == std::string::npos);

    /* Invalid pointer: all buffers. */
    recv (client, "(l) infolist buffer zzz");
    LONGS_EQUAL(1, test_msgs_count);
    str = msg (0, 1);
    CHECK_PREFIX("(l) inl:buffer{", str);
    CHECK(str.find ("full_name:str=\"core.weechat\"") != std::string::npos);

    /* Infolist with a pointer and arguments. */
    recv (client, "(l) infolist option 0x0 weechat.look.buffer_time_format");
    LONGS_EQUAL(1, test_msgs_count);
    str = msg (0, 1);
    CHECK_PREFIX("(l) inl:option{", str);
    CHECK(str.find ("full_name:str=\"weechat.look.buffer_time_format\"")
          != std::string::npos);
    CHECK(str.find ("}{") == std::string::npos);
}

/*
 * Test functions:
 *   relay_weechat_protocol_cb_nicklist
 */

TEST(RelayWeechatProtocolWithClient, CbNicklist)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    char command[256];

    buffer = gui_buffer_new (NULL, "test", NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer);
    gui_buffer_set (buffer, "nicklist", "1");
    group = gui_nicklist_add_group (buffer, NULL, "010|ops", "green", 1);
    CHECK(group);
    CHECK(gui_nicklist_add_nick (buffer, group, "alice", "cyan",
                                 "@", "lightgreen", 1));
    CHECK(gui_nicklist_add_nick (buffer, NULL, "bob", "red",
                                 NULL, NULL, 0));

    /* Invalid buffer: nothing sent. */
    recv (client, "nicklist core.unknown");
    LONGS_EQUAL(0, test_msgs_count);
    recv (client, "nicklist 0x1234");
    LONGS_EQUAL(0, test_msgs_count);

    /*
     * Nicklist of a buffer: groups and nicks, sub-groups (with their nicks)
     * before nicks of the parent group.
     */
    recv (client, "(n) nicklist core.test");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL(
        "(n) hda:buffer/nicklist_item"
        "[group:chr,visible:chr,level:int,name:str,color:str,"
        "prefix:str,prefix_color:str]"
        "{0x?/0x?|group=1,visible=0,level=0,name=\"root\",color=null,"
        "prefix=null,prefix_color=null}"
        "{0x?/0x?|group=1,visible=1,level=1,name=\"010|ops\","
        "color=\"green\",prefix=null,prefix_color=null}"
        "{0x?/0x?|group=0,visible=1,level=0,name=\"alice\",color=\"cyan\","
        "prefix=\"@\",prefix_color=\"lightgreen\"}"
        "{0x?/0x?|group=0,visible=0,level=0,name=\"bob\",color=\"red\","
        "prefix=null,prefix_color=null}",
        msg (0, 1).c_str ());

    /* Same nicklist, buffer given by pointer. */
    snprintf (command, sizeof (command),
              "(n) nicklist 0x%lx", (unsigned long)buffer);
    recv (client, command);
    LONGS_EQUAL(1, test_msgs_count);
    CHECK_PREFIX("(n) hda:buffer/nicklist_item[", msg (0, 1));
    CHECK(msg (0, 1).find ("name=\"alice\"") != std::string::npos);

    /* All buffers. */
    recv (client, "(n) nicklist");
    LONGS_EQUAL(1, test_msgs_count);
    CHECK(msg (0, 1).find ("name=\"alice\"") != std::string::npos);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   relay_weechat_protocol_cb_input
 */

TEST(RelayWeechatProtocolWithClient, CbInput)
{
    struct t_gui_buffer *buffer;

    buffer = gui_buffer_new (NULL, "test", NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer);
    gui_buffer_set (buffer, "title", "title1");

    /* Too few arguments, invalid buffer or no text: nothing done. */
    recv (client, "input");
    recv (client, "input core.unknown /buffer set title title2");
    recv (client, "input core.test");
    recv (client, "input core.test ");
    usleep (10000);
    hook_timer_exec ();
    STRCMP_EQUAL("title1", buffer->title);

    /* Command is delayed: executed by a timer. */
    recv (client, "input core.test /buffer set title title2");
    LONGS_EQUAL(0, test_msgs_count);
    STRCMP_EQUAL("title1", buffer->title);
    usleep (10000);
    hook_timer_exec ();
    STRCMP_EQUAL("title2", buffer->title);

    /* Command not allowed. */
    config_file_option_set (relay_config_network_commands, "*,!buffer", 1);
    recv (client, "input core.test /buffer set title title3");
    usleep (10000);
    hook_timer_exec ();
    STRCMP_EQUAL("title2", buffer->title);
    config_file_option_reset (relay_config_network_commands, 1);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   relay_weechat_protocol_cb_completion
 */

TEST(RelayWeechatProtocolWithClient, CbCompletion)
{
    const char *empty_hdata = "(c) hda:completion[null]";

    /* Errors: empty hdata. */
    recv (client, "(c) completion");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL(empty_hdata, msg (0, 0).c_str ());
    recv (client, "(c) completion core.weechat");
    STRCMP_EQUAL(empty_hdata, msg (0, 0).c_str ());
    recv (client, "(c) completion core.unknown -1 /help");
    STRCMP_EQUAL(empty_hdata, msg (0, 0).c_str ());
    recv (client, "(c) completion core.weechat abc /help");
    STRCMP_EQUAL(empty_hdata, msg (0, 0).c_str ());

    /* Completion of a command. */
    recv (client, "(c) completion core.weechat -1 /hel");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(c) hda:completion"
                 "[context:str,base_word:str,pos_start:int,pos_end:int,"
                 "add_space:int,list:arr]"
                 "{0x?|context=\"command\",base_word=\"hel\",pos_start=1,"
                 "pos_end=3,add_space=1,list=str[\"help\"]}",
                 msg (0, 1).c_str ());

    /* Position greater than length of data: end of data. */
    recv (client, "(c) completion core.weechat 100 /hel");
    STRCMP_EQUAL("(c) hda:completion"
                 "[context:str,base_word:str,pos_start:int,pos_end:int,"
                 "add_space:int,list:arr]"
                 "{0x?|context=\"command\",base_word=\"hel\",pos_start=1,"
                 "pos_end=3,add_space=1,list=str[\"help\"]}",
                 msg (0, 1).c_str ());

    /* Completion at a given position, in the middle of a word. */
    recv (client, "(c) completion core.weechat 3 /hel");
    STRCMP_EQUAL("(c) hda:completion"
                 "[context:str,base_word:str,pos_start:int,pos_end:int,"
                 "add_space:int,list:arr]"
                 "{0x?|context=\"command\",base_word=\"he\",pos_start=1,"
                 "pos_end=2,add_space=1,list=str[\"help\"]}",
                 msg (0, 1).c_str ());

    /* Completion of a command argument. */
    recv (client, "(c) completion core.weechat -1 /help filte");
    STRCMP_EQUAL("(c) hda:completion"
                 "[context:str,base_word:str,pos_start:int,pos_end:int,"
                 "add_space:int,list:arr]"
                 "{0x?|context=\"command_arg\",base_word=\"filte\","
                 "pos_start=6,pos_end=10,add_space=1,list=str[\"filter\"]}",
                 msg (0, 1).c_str ());

    /* No completion found: empty list. */
    recv (client, "(c) completion core.weechat -1 /zzz_unknown");
    STRCMP_EQUAL("(c) hda:completion"
                 "[context:str,base_word:str,pos_start:int,pos_end:int,"
                 "add_space:int,list:arr]"
                 "{0x?|context=\"command\",base_word=\"zzz_unknown\","
                 "pos_start=1,pos_end=11,add_space=1,list=str[]}",
                 msg (0, 1).c_str ());
}

/*
 * Test functions:
 *   relay_weechat_protocol_is_sync
 *   relay_weechat_protocol_cb_sync
 *   relay_weechat_protocol_cb_desync
 */

TEST(RelayWeechatProtocolWithClient, CbSyncDesync)
{
    struct t_gui_buffer *buffer;
    struct t_hashtable *buffers_sync;
    char command[256];

    buffers_sync = RELAY_WEECHAT_DATA(client, buffers_sync);
    buffer = gui_buffer_search_main ();
    CHECK(buffer);

    LONGS_EQUAL(0, buffers_sync->items_count);
    LONGS_EQUAL(0, relay_weechat_protocol_is_sync (
                    client, NULL, RELAY_WEECHAT_PROTOCOL_SYNC_ALL));
    LONGS_EQUAL(0, relay_weechat_protocol_is_sync (
                    client, buffer, RELAY_WEECHAT_PROTOCOL_SYNC_ALL));

    /* Sync all buffers with all flags. */
    recv (client, "sync");
    LONGS_EQUAL(0, test_msgs_count);
    LONGS_EQUAL(1, buffers_sync->items_count);
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_ALL,
                *((int *)hashtable_get (buffers_sync, "*")));
    LONGS_EQUAL(1, relay_weechat_protocol_is_sync (
                    client, NULL, RELAY_WEECHAT_PROTOCOL_SYNC_UPGRADE));
    LONGS_EQUAL(1, relay_weechat_protocol_is_sync (
                    client, buffer, RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST));

    /* Desync all buffers with all flags. */
    recv (client, "desync");
    LONGS_EQUAL(0, test_msgs_count);
    LONGS_EQUAL(0, buffers_sync->items_count);

    /* Sync all buffers with some flags; unknown flags are ignored. */
    recv (client, "sync * buffers,unknown,upgrade");
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS
                | RELAY_WEECHAT_PROTOCOL_SYNC_UPGRADE,
                *((int *)hashtable_get (buffers_sync, "*")));
    recv (client, "sync * nicklist");
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS
                | RELAY_WEECHAT_PROTOCOL_SYNC_UPGRADE
                | RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST,
                *((int *)hashtable_get (buffers_sync, "*")));
    LONGS_EQUAL(0, relay_weechat_protocol_is_sync (
                    client, buffer, RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER));
    LONGS_EQUAL(1, relay_weechat_protocol_is_sync (
                    client, buffer, RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST));

    /* Only unknown flags: nothing changed. */
    recv (client, "sync core.weechat unknown");
    LONGS_EQUAL(1, buffers_sync->items_count);

    /* Unknown buffer: ignored. */
    recv (client, "sync core.unknown,0x1234 buffer");
    LONGS_EQUAL(1, buffers_sync->items_count);

    /* Sync a buffer: only flags "buffer" and "nicklist" are allowed. */
    recv (client, "sync core.weechat buffers,upgrade");
    LONGS_EQUAL(1, buffers_sync->items_count);
    recv (client, "sync core.weechat buffer");
    LONGS_EQUAL(2, buffers_sync->items_count);
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER,
                *((int *)hashtable_get (buffers_sync, "core.weechat")));

    /* Buffer flags have precedence over "*". */
    LONGS_EQUAL(1, relay_weechat_protocol_is_sync (
                    client, buffer, RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER));
    LONGS_EQUAL(0, relay_weechat_protocol_is_sync (
                    client, buffer, RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST));

    /* Sync a buffer by pointer. */
    snprintf (command, sizeof (command),
              "sync 0x%lx nicklist", (unsigned long)buffer);
    recv (client, command);
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_FOR_BUFFER,
                *((int *)hashtable_get (buffers_sync, "core.weechat")));

    /* Desync a buffer, flag by flag. */
    recv (client, "desync core.weechat unknown");
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_FOR_BUFFER,
                *((int *)hashtable_get (buffers_sync, "core.weechat")));
    recv (client, "desync core.weechat buffer");
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST,
                *((int *)hashtable_get (buffers_sync, "core.weechat")));
    recv (client, "desync core.unknown nicklist");
    LONGS_EQUAL(2, buffers_sync->items_count);
    recv (client, "desync core.weechat nicklist");
    LONGS_EQUAL(1, buffers_sync->items_count);
    POINTERS_EQUAL(NULL, hashtable_get (buffers_sync, "core.weechat"));

    /* Desync of a buffer not synchronized. */
    recv (client, "desync core.weechat");
    LONGS_EQUAL(1, buffers_sync->items_count);

    /* Sync and desync of multiple buffers. */
    recv (client, "sync core.weechat,*");
    LONGS_EQUAL(2, buffers_sync->items_count);
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_FOR_BUFFER,
                *((int *)hashtable_get (buffers_sync, "core.weechat")));
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_ALL,
                *((int *)hashtable_get (buffers_sync, "*")));
    recv (client, "desync * upgrade");
    LONGS_EQUAL(RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER
                | RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST
                | RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS,
                *((int *)hashtable_get (buffers_sync, "*")));
    recv (client, "desync *,core.weechat");
    LONGS_EQUAL(0, buffers_sync->items_count);
}

/*
 * Test functions:
 *   relay_weechat_protocol_cb_test
 */

TEST(RelayWeechatProtocolWithClient, CbTest)
{
    recv (client, "(t) test");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(t) chr:65 int:123456 int:-123456 "
                 "lon:1234567890 lon:-1234567890 "
                 "str:\"a string\" str:\"\" str:null "
                 "buf:\"buffer\" buf:null "
                 "ptr:0x1234abcd ptr:0x0 "
                 "tim:1321993456 "
                 "arr:str[\"abc\",\"de\"] "
                 "arr:int[123,456,789]",
                 msg (0, 0).c_str ());
}

/*
 * Test functions:
 *   relay_weechat_protocol_cb_ping
 */

TEST(RelayWeechatProtocolWithClient, CbPing)
{
    recv (client, "ping");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(_pong) str:\"\"", msg (0, 0).c_str ());

    /* Id is ignored: the reply always has id "_pong". */
    recv (client, "(p) ping 1370802127000");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(_pong) str:\"1370802127000\"", msg (0, 0).c_str ());

    recv (client, "ping  a  b ");
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(_pong) str:\"a  b \"", msg (0, 0).c_str ());
}

/*
 * Test functions:
 *   relay_weechat_protocol_cb_quit
 */

TEST(RelayWeechatProtocolWithClient, CbQuit)
{
    recv (client, "quit");
    LONGS_EQUAL(0, test_msgs_count);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, client->status);

    /* Commands are ignored once client is disconnected. */
    recv (client, "ping");
    LONGS_EQUAL(0, test_msgs_count);
}

/*
 * Test functions:
 *   relay_weechat_protocol_signal_upgrade_cb
 */

TEST(RelayWeechatProtocolWithClient, SignalUpgradeCb)
{
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_weechat_protocol_signal_upgrade_cb (
                    NULL, NULL, "upgrade", NULL, NULL));
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_weechat_protocol_signal_upgrade_cb (
                    (void *)0x1, NULL, "upgrade", NULL, NULL));

    /* Client not synchronized with flag "upgrade": nothing sent. */
    free_msgs ();
    relay_weechat_protocol_signal_upgrade_cb (client, NULL, "upgrade",
                                              NULL, NULL);
    LONGS_EQUAL(0, test_msgs_count);
    recv (client, "sync * buffer");
    relay_weechat_protocol_signal_upgrade_cb (client, NULL, "upgrade",
                                              NULL, NULL);
    LONGS_EQUAL(0, test_msgs_count);

    /* Client synchronized with flag "upgrade". */
    recv (client, "sync * upgrade");
    relay_weechat_protocol_signal_upgrade_cb (client, NULL, "upgrade",
                                              NULL, NULL);
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(_upgrade)", msg (0, 0).c_str ());
    free_msgs ();
    relay_weechat_protocol_signal_upgrade_cb (client, NULL, "upgrade_ended",
                                              NULL, NULL);
    LONGS_EQUAL(1, test_msgs_count);
    STRCMP_EQUAL("(_upgrade_ended)", msg (0, 0).c_str ());

    /* Other signal: nothing sent. */
    free_msgs ();
    relay_weechat_protocol_signal_upgrade_cb (client, NULL, "upgrade_other",
                                              NULL, NULL);
    LONGS_EQUAL(0, test_msgs_count);
}
