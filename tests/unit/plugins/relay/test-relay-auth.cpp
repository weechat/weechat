/*
 * test-relay-auth.cpp - test client authentication functions
 *
 * Copyright (C) 2020-2025 Sébastien Helleu <flashcode@flashtux.org>
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

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "src/core/core-config-file.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-auth.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/relay-config.h"

extern int relay_auth_check_salt (struct t_relay_client *client,
                                  const char *salt_hexa,
                                  const char *salt, int salt_size);
}

#define WEE_CHECK_PARSE_SHA(__parameters)                               \
    salt = (char *)0x1;                                                 \
    salt_size = -1;                                                     \
    hash_sha = (char *)0x1;                                             \
    relay_auth_parse_sha (                                              \
        __parameters,                                                   \
        &salt_hexa,                                                     \
        &salt,                                                          \
        &salt_size,                                                     \
        &hash_sha);

#define WEE_CHECK_PARSE_PBKDF2(__parameters)                            \
    salt = (char *)0x1;                                                 \
    salt_size = -1;                                                     \
    iterations = -1;                                                    \
    hash_pbkdf2 = (char *)0x1;                                          \
    relay_auth_parse_pbkdf2 (                                           \
        __parameters,                                                   \
        &salt_hexa,                                                     \
        &salt,                                                          \
        &salt_size,                                                     \
        &iterations,                                                    \
        &hash_pbkdf2);

TEST_GROUP(RelayAuth)
{
};

/*
 * Tests functions:
 *   relay_auth_password_hash_algo_search
 */

TEST(RelayAuth, PasswordHashAlgoSearch)
{
    LONGS_EQUAL(-1, relay_auth_password_hash_algo_search (NULL));
    LONGS_EQUAL(-1, relay_auth_password_hash_algo_search (""));
    LONGS_EQUAL(-1, relay_auth_password_hash_algo_search ("zzz"));

    LONGS_EQUAL(0, relay_auth_password_hash_algo_search ("plain"));
}

/*
 * Tests functions:
 *   relay_auth_generate_nonce
 */

TEST(RelayAuth, GenerateNonce)
{
    char *nonce;

    STRCMP_EQUAL(NULL, relay_auth_generate_nonce (-1));
    STRCMP_EQUAL(NULL, relay_auth_generate_nonce (0));

    nonce = relay_auth_generate_nonce (1);
    LONGS_EQUAL(2, strlen (nonce));
    CHECK(isxdigit ((int)nonce[0]));
    CHECK(isxdigit ((int)nonce[1]));
    free (nonce);

    nonce = relay_auth_generate_nonce (2);
    LONGS_EQUAL(4, strlen (nonce));
    CHECK(isxdigit ((int)nonce[0]));
    CHECK(isxdigit ((int)nonce[1]));
    CHECK(isxdigit ((int)nonce[2]));
    CHECK(isxdigit ((int)nonce[3]));
    free (nonce);
}

/*
 * Tests functions:
 *   relay_auth_check_password_plain
 */

TEST(RelayAuth, CheckPasswordPlain)
{
    struct t_relay_client *client;

    client = (struct t_relay_client *)calloc (1, sizeof (*client));
    CHECK(client);
    client->protocol = RELAY_PROTOCOL_API;

    /* invalid arguments */
    LONGS_EQUAL(-2, relay_auth_check_password_plain (client, NULL, NULL));
    LONGS_EQUAL(-2, relay_auth_check_password_plain (client, "abcd", NULL));
    LONGS_EQUAL(-2, relay_auth_check_password_plain (client, NULL, "password"));

    /* wrong password */
    LONGS_EQUAL(-2, relay_auth_check_password_plain (client, "test", "password"));
    LONGS_EQUAL(-2, relay_auth_check_password_plain (client, "Password", "password"));

    /* good password */
    LONGS_EQUAL(0, relay_auth_check_password_plain (client, "", ""));
    LONGS_EQUAL(0, relay_auth_check_password_plain (client, "password", "password"));

    /* test with "plain" disabled */
    config_file_option_set (relay_config_network_password_hash_algo, "*,!plain", 1);
    LONGS_EQUAL(-1, relay_auth_check_password_plain (client, "password", "password"));
    config_file_option_reset (relay_config_network_password_hash_algo, 1);
}

/*
 * Tests functions:
 *   relay_auth_parse_sha
 */

TEST(RelayAuth, ParseSha)
{
    char *salt_hexa, *salt, *hash_sha;
    const char salt_expected[4] = { 0x41, 0x42, 0x43, 0x44 };  /* "ABCD" */
    int salt_size;

    /* NULL string */
    WEE_CHECK_PARSE_SHA(NULL);
    STRCMP_EQUAL(NULL, salt_hexa);
    STRCMP_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    STRCMP_EQUAL(NULL, hash_sha);

    /* not enough parameters: 0 (expected: 2) */
    WEE_CHECK_PARSE_SHA("");
    STRCMP_EQUAL(NULL, salt_hexa);
    STRCMP_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    STRCMP_EQUAL(NULL, hash_sha);

    /* not enough parameters: 1 (expected: 2) */
    WEE_CHECK_PARSE_SHA("41424344");
    STRCMP_EQUAL(NULL, salt_hexa);
    STRCMP_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    STRCMP_EQUAL(NULL, hash_sha);

    /* good parameters */
    WEE_CHECK_PARSE_SHA("41424344:5e884898da28047151d0e56f8dc6292773603d0d6aa"
                        "bbdd62a11ef721d1542d8");
    STRCMP_EQUAL("41424344", salt_hexa);
    MEMCMP_EQUAL(salt_expected, salt, 4);
    LONGS_EQUAL(4, salt_size);
    STRCMP_EQUAL("5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1"
                 "542d8",
                 hash_sha);
    free (salt_hexa);
    free (salt);
    free (hash_sha);

    /* wrong salt */
    WEE_CHECK_PARSE_SHA("Z:5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a"
                        "11ef721d1542d8");
    STRCMP_EQUAL(NULL, salt_hexa);
    STRCMP_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    STRCMP_EQUAL("5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1"
                 "542d8",
                 hash_sha);
    free (hash_sha);
}

/*
 * Tests functions:
 *   relay_auth_parse_pbkdf2
 */

TEST(RelayAuth, ParsePbkdf2)
{
    char *salt_hexa, *salt, *hash_pbkdf2;
    const char salt_expected[4] = { 0x41, 0x42, 0x43, 0x44 };  /* "ABCD" */
    int salt_size, iterations;

    /* NULL string */
    WEE_CHECK_PARSE_PBKDF2(NULL);
    STRCMP_EQUAL(NULL, salt_hexa);
    STRCMP_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    STRCMP_EQUAL(NULL, hash_pbkdf2);

    /* not enough parameters: 0 (expected: 3) */
    WEE_CHECK_PARSE_PBKDF2("");
    STRCMP_EQUAL(NULL, salt_hexa);
    STRCMP_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    STRCMP_EQUAL(NULL, hash_pbkdf2);

    /* not enough parameters: 1 (expected: 3) */
    WEE_CHECK_PARSE_PBKDF2("41424344");
    STRCMP_EQUAL(NULL, salt_hexa);
    STRCMP_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    STRCMP_EQUAL(NULL, hash_pbkdf2);

    /* not enough parameters: 2 (expected: 3) */
    WEE_CHECK_PARSE_PBKDF2("41424344:1000");
    STRCMP_EQUAL(NULL, salt_hexa);
    STRCMP_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    STRCMP_EQUAL(NULL, hash_pbkdf2);

    /* good parameters */
    WEE_CHECK_PARSE_PBKDF2("41424344:1000:8765936466387f2cfcc47d2617423386684"
                           "a218d64a57f8213e42b0fe60d8849");
    STRCMP_EQUAL("41424344", salt_hexa);
    MEMCMP_EQUAL(salt_expected, salt, 4);
    LONGS_EQUAL(4, salt_size);
    LONGS_EQUAL(1000, iterations);
    STRCMP_EQUAL("8765936466387f2cfcc47d2617423386684a218d64a57f8213e42b0fe60"
                 "d8849",
                 hash_pbkdf2);
    free (salt_hexa);
    free (salt);
    free (hash_pbkdf2);

    /* wrong salt */
    WEE_CHECK_PARSE_PBKDF2("Z:1000:8765936466387f2cfcc47d2617423386684a218d64"
                           "a57f8213e42b0fe60d8849");
    STRCMP_EQUAL(NULL, salt_hexa);
    STRCMP_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(1000, iterations);
    STRCMP_EQUAL("8765936466387f2cfcc47d2617423386684a218d64a57f8213e42b0fe60"
                 "d8849",
                 hash_pbkdf2);
    free (hash_pbkdf2);

    /* wrong iterations */
    WEE_CHECK_PARSE_PBKDF2("41424344:abcd:8765936466387f2cfcc47d2617423386684"
                           "a218d64a57f8213e42b0fe60d8849");
    STRCMP_EQUAL("41424344", salt_hexa);
    MEMCMP_EQUAL(salt_expected, salt, 4);
    LONGS_EQUAL(4, salt_size);
    LONGS_EQUAL(0, iterations);
    STRCMP_EQUAL("8765936466387f2cfcc47d2617423386684a218d64a57f8213e42b0fe60"
                 "d8849",
                 hash_pbkdf2);
    free (salt_hexa);
    free (salt);
    free (hash_pbkdf2);
}

/*
 * Tests functions:
 *   relay_auth_check_salt
 */

TEST(RelayAuth, CheckSalt)
{
    struct t_relay_client *client;
    time_t time_now;
    char salt[128];

    client = (struct t_relay_client *)calloc (1, sizeof (*client));
    CHECK(client);
    client->nonce = strdup ("01aa03bb");

    client->protocol = RELAY_PROTOCOL_API;

    LONGS_EQUAL(0, relay_auth_check_salt (NULL, NULL, NULL, 0));
    LONGS_EQUAL(0, relay_auth_check_salt (client, NULL, NULL, 0));
    LONGS_EQUAL(0, relay_auth_check_salt (client, NULL, "test", 4));
    LONGS_EQUAL(0, relay_auth_check_salt (client, NULL, "1234", 4));

    time_now = time (NULL);
    snprintf (salt, sizeof (salt), "%ld", time_now);
    LONGS_EQUAL(1, relay_auth_check_salt (client, NULL, salt, strlen (salt)));
    time_now = time (NULL) - 2;
    snprintf (salt, sizeof (salt), "%ld", time_now);
    LONGS_EQUAL(1, relay_auth_check_salt (client, NULL, salt, strlen (salt)));
    time_now = time (NULL) + 2;
    snprintf (salt, sizeof (salt), "%ld", time_now);
    LONGS_EQUAL(1, relay_auth_check_salt (client, NULL, salt, strlen (salt)));
    time_now = time (NULL) - 10;
    snprintf (salt, sizeof (salt), "%ld", time_now);
    LONGS_EQUAL(0, relay_auth_check_salt (client, NULL, salt, strlen (salt)));

    client->protocol = RELAY_PROTOCOL_WEECHAT;

    LONGS_EQUAL(0, relay_auth_check_salt (NULL, NULL, NULL, 0));
    LONGS_EQUAL(0, relay_auth_check_salt (client, NULL, NULL, 0));
    LONGS_EQUAL(0, relay_auth_check_salt (client, NULL, "test", 4));
    LONGS_EQUAL(0, relay_auth_check_salt (client, NULL, "1234", 4));

    LONGS_EQUAL(0, relay_auth_check_salt (client, "01aa", "\x01\xaa", 2));
    LONGS_EQUAL(0, relay_auth_check_salt (client, "01aa03bb", "\x01\xaa\x03\xbb", 4));
    LONGS_EQUAL(1, relay_auth_check_salt (client, "01aa03bbcc", "\x01\xaa\x03\xbb\xcc", 5));
    LONGS_EQUAL(1, relay_auth_check_salt (client, "01AA03BBCC", "\x01\xaa\x03\xbb\xcc", 5));

    free (client->nonce);
    free (client);
}

/*
 * Tests functions:
 *   relay_auth_check_hash_sha
 */

TEST(RelayAuth, CheckHashSha)
{
    /* "ABCDEFGHIJKLMNOP" */
    const char salt[16] = { 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                            0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50 };

    LONGS_EQUAL(0,
                relay_auth_check_hash_sha (
                    NULL, NULL, 0, NULL, NULL));
    LONGS_EQUAL(0,
                relay_auth_check_hash_sha (
                    "", "", 0, "", ""));
    LONGS_EQUAL(0,
                relay_auth_check_hash_sha (
                    "sha256", NULL, 0, NULL, NULL));
    LONGS_EQUAL(0,
                relay_auth_check_hash_sha (
                    "sha256", salt, sizeof (salt), NULL, NULL));
    LONGS_EQUAL(0,
                relay_auth_check_hash_sha (
                    "sha256", salt, sizeof (salt), "", ""));

    /* SHA256: hash is for password: "wrong" */
    LONGS_EQUAL(0,
                relay_auth_check_hash_sha (
                    "sha256",
                    salt,
                    sizeof (salt),
                    "5d21c7a7d34f47623195ff4750bd65c34bb5f1ba131bf0086a498b2a"
                    "6a4edfcb",
                    "password"));
    /* SHA256: hash is for password: "password" */
    LONGS_EQUAL(1,
                relay_auth_check_hash_sha (
                    "sha256",
                    salt,
                    sizeof (salt),
                    "6b1550cb48b6cd66b7152f96804b816b5ae861e4ae52ff5c7a56b7a4"
                    "f2fdb772",
                    "password"));

    /* SHA512: hash is for password: "wrong" */
    LONGS_EQUAL(0,
                relay_auth_check_hash_sha (
                    "sha512",
                    salt,
                    sizeof (salt),
                    "527d147327d77aceeb862848b404d462ce2a11e4502eda82ce0b1be1"
                    "958422491ca14f3fe8b94a66c61d54639d9fbed0979025ae1073ccaa"
                    "a66a2d2de9416221",
                    "password"));
    /* SHA512: hash is for password: "password" */
    LONGS_EQUAL(1,
                relay_auth_check_hash_sha (
                    "sha512",
                    salt,
                    sizeof (salt),
                    "49d2c9a7f7cf630b32c0cc79b331db4eec6215e2c90bcc6c43db93f8"
                    "847cfdf885a4a8d36b440cb47fed79e97b35380d086a5722c3a26018"
                    "fdc633fe56949938",
                    "password"));
}

/*
 * Tests functions:
 *   relay_auth_check_hash_pbkdf2
 */

TEST(RelayAuth, CheckHashPbkdf2)
{
    /* "ABCDEFGHIJKLMNOP" */
    const char salt[16] = { 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                            0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50 };


    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    NULL, NULL, 0, 0, NULL, NULL));
    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    "", "", 0, 0, "", ""));
    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    "sha256", NULL, 0, 0, NULL, NULL));
    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    "sha256", salt, sizeof (salt), 0, NULL, NULL));
    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    "sha256", salt, sizeof (salt), 1000, NULL, NULL));
    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    "sha256", salt, sizeof (salt), 1000, "", ""));

    /* PBKDF2 (SHA256): hash is for password "wrong" */
    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    "sha256",
                    salt,
                    sizeof (salt),
                    1000,
                    "59f69895354b82a76d0b3030745c54f961de9da4a80b697b3010d749"
                    "58f452a1",
                    "password"));
    /* PBKDF2 (SHA256): hash is for password "password" */
    LONGS_EQUAL(1,
                relay_auth_check_hash_pbkdf2 (
                    "sha256",
                    salt,
                    sizeof (salt),
                    1000,
                    "1351b6c26ade0de7dc9422e09a0cd44aae9c1e5e9147ad7e91fb117f"
                    "2f27852d",
                    "password"));

    /* PBKDF2 (SHA512): hash is for password "wrong" */
    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    "sha512",
                    salt,
                    sizeof (salt),
                    1000,
                    "4a7cd751fe20abaf52a92daeb13e571aed2453425a17258b3fa4a536"
                    "e8b66228f5f44570347aca462ae280de7951b9e90d2ee3d7c3dd455f"
                    "678e9ec80768d30e",
                    "password"));
    /* PBKDF2 (SHA512): hash is for password "password" */
    LONGS_EQUAL(1,
                relay_auth_check_hash_pbkdf2 (
                    "sha512",
                    salt,
                    sizeof (salt),
                    1000,
                    "7b7eca3ea0c75d9218dc5d31cd7a80f752112dc7de86501973ba8723"
                    "b635d9b1e461273c3a8ad179cb5285b32f0c5ed0360e37b31713977e"
                    "f53326c3729ffd12",
                    "password"));
}

/*
 * Tests functions:
 *   relay_auth_password_hash
 */

TEST(RelayAuth, PasswordHash)
{
    /* TODO: write tests */
}
