/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay "weechat" protocol (protocol) */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <gcrypt.h>
#include "src/core/core-config-file.h"
#include "src/core/core-crypto.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-auth.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-server.h"
#include "src/plugins/relay/weechat/relay-weechat-protocol.h"
}

TEST_GROUP(RelayWeechatProtocol)
{
    struct t_relay_server *server;

    void setup ()
    {
        /* disable auto-open of relay buffer (it would pollute other tests) */
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
    /* plain text password */
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, "init password=secret"));
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, "init password=wrong"));

    /* hashed password */
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                init (RELAY_AUTH_PASSWORD_HASH_SHA256,
                      "init password_hash={good_hash}"));
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_SHA256,
                      "init password_hash={bad_hash}"));

    /* only the first plain text password is checked */
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN,
                      "init password=secret,password=wrong"));
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN,
                      "init password=wrong,password=secret"));

    /* only the first hashed password is checked */
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

    /* only the first password is checked, plain text or hashed */
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

    /* missing TOTP */
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, "init password=secret"));

    /* valid TOTP */
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s", totp);
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));

    /* invalid TOTP */
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s", totp2);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));

    /* last 4 or 5 digits of the valid TOTP are rejected */
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s", totp + 2);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s", totp + 1);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));

    /* extra arguments for info "totp_validate" (escaped commas) are rejected */
    snprintf (command, sizeof (command),
              "init password=secret,totp=%s\\,0\\,1\\,4", totp + 2);
    LONGS_EQUAL(RELAY_STATUS_AUTH_FAILED,
                init (RELAY_AUTH_PASSWORD_HASH_PLAIN, command));

    /* only the first TOTP is checked */
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
