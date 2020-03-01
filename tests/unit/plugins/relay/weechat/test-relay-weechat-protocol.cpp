/*
 * test-relay-weechat-protocol.cpp - test relay weechat protocol
 *
 * Copyright (C) 2020 Sébastien Helleu <flashcode@flashtux.org>
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

extern void relay_weechat_protocol_parse_pbkdf2 (const char *parameters,
                                                 char **algorithm,
                                                 char **salt,
                                                 int *salt_size,
                                                 int *iterations,
                                                 char **hash_pbkdf2);
extern int relay_weechat_protocol_check_hash (const char *hashed_password,
                                              const char *password);
}

#define WEE_CHECK_PARSE_PBKDF2(__parameters)                            \
    algorithm = (char *)0x1;                                            \
    salt = (char *)0x1;                                                 \
    salt_size = -1;                                                     \
    iterations = -1;                                                    \
    hash_pbkdf2 = (char *)0x1;                                          \
    relay_weechat_protocol_parse_pbkdf2 (                               \
        __parameters,                                                   \
        &algorithm,                                                     \
        &salt,                                                          \
        &salt_size,                                                     \
        &iterations,                                                    \
        &hash_pbkdf2);

TEST_GROUP(RelayWeechatProtocol)
{
};

/*
 * Tests functions:
 *   relay_weechat_protocol_parse_pbkdf2
 */

TEST(RelayWeechatProtocol, ParsePbkdf2)
{
    char *algorithm, *salt, *hash_pbkdf2;
    const char salt_expected[4] = { 0x41, 0x42, 0x43, 0x44 };  /* "ABCD" */
    int salt_size, iterations;

    /* NULL string */
    WEE_CHECK_PARSE_PBKDF2(NULL);
    POINTERS_EQUAL(NULL, algorithm);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    POINTERS_EQUAL(NULL, hash_pbkdf2);

    /* not enough parameters: 0 (expected: 4) */
    WEE_CHECK_PARSE_PBKDF2("");
    POINTERS_EQUAL(NULL, algorithm);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    POINTERS_EQUAL(NULL, hash_pbkdf2);

    /* not enough parameters: 1 (expected: 4) */
    WEE_CHECK_PARSE_PBKDF2("sha256");
    POINTERS_EQUAL(NULL, algorithm);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    POINTERS_EQUAL(NULL, hash_pbkdf2);

    /* not enough parameters: 2 (expected: 4) */
    WEE_CHECK_PARSE_PBKDF2("sha256:41424344");
    POINTERS_EQUAL(NULL, algorithm);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    POINTERS_EQUAL(NULL, hash_pbkdf2);

    /* not enough parameters: 3 (expected: 4) */
    WEE_CHECK_PARSE_PBKDF2("sha256:41424344:100000");
    POINTERS_EQUAL(NULL, algorithm);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    POINTERS_EQUAL(NULL, hash_pbkdf2);

    /* good parameters */
    WEE_CHECK_PARSE_PBKDF2("sha256:41424344:100000:"
                           "01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7"
                           "f9ac09afc945a1c");
    STRCMP_EQUAL("sha256", algorithm);
    MEMCMP_EQUAL(salt_expected, salt, 4);
    LONGS_EQUAL(4, salt_size);
    LONGS_EQUAL(100000, iterations);
    STRCMP_EQUAL("01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7f9ac09afc9"
                 "45a1c",
                 hash_pbkdf2);
    free (algorithm);
    free (salt);
    free (hash_pbkdf2);

    /* wrong algorithm */
    WEE_CHECK_PARSE_PBKDF2("not_an_algo:41424344:100000:"
                           "01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7"
                           "f9ac09afc945a1c");
    POINTERS_EQUAL(NULL, algorithm);
    MEMCMP_EQUAL(salt_expected, salt, 4);
    LONGS_EQUAL(4, salt_size);
    LONGS_EQUAL(100000, iterations);
    STRCMP_EQUAL("01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7f9ac09afc9"
                 "45a1c",
                 hash_pbkdf2);
    free (salt);
    free (hash_pbkdf2);

    /* wrong salt */
    WEE_CHECK_PARSE_PBKDF2("sha256:Z:100000:"
                           "01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7"
                           "f9ac09afc945a1c");
    STRCMP_EQUAL("sha256", algorithm);
    CHECK(salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(100000, iterations);
    STRCMP_EQUAL("01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7f9ac09afc9"
                 "45a1c",
                 hash_pbkdf2);
    free (algorithm);
    free (salt);
    free (hash_pbkdf2);

    /* wrong iterations */
    WEE_CHECK_PARSE_PBKDF2("sha256:41424344:abcd:"
                           "01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7"
                           "f9ac09afc945a1c");
    STRCMP_EQUAL("sha256", algorithm);
    MEMCMP_EQUAL(salt_expected, salt, 4);
    LONGS_EQUAL(4, salt_size);
    LONGS_EQUAL(0, iterations);
    STRCMP_EQUAL("01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7f9ac09afc9"
                 "45a1c",
                 hash_pbkdf2);
    free (algorithm);
    free (salt);
    free (hash_pbkdf2);
}

/*
 * Tests functions:
 *   relay_weechat_protocol_check_hash
 */

TEST(RelayWeechatProtocol, CheckHash)
{
    LONGS_EQUAL(0, relay_weechat_protocol_check_hash (NULL, NULL));
    LONGS_EQUAL(0, relay_weechat_protocol_check_hash ("", ""));
    LONGS_EQUAL(0, relay_weechat_protocol_check_hash ("abcd", NULL));
    LONGS_EQUAL(0, relay_weechat_protocol_check_hash (NULL, "password"));
    LONGS_EQUAL(0, relay_weechat_protocol_check_hash ("invalid", "password"));

    /* SHA256: hash is for password: "wrong" */
    LONGS_EQUAL(0,
                relay_weechat_protocol_check_hash (
                    "sha256:8810ad581e59f2bc3928b261707a71308f7e139eb04820366"
                    "dc4d5c18d980225",
                    "password"));
    /* SHA256: hash is for password: "password" */
    LONGS_EQUAL(1,
                relay_weechat_protocol_check_hash (
                    "sha256:5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62"
                    "a11ef721d1542d8",
                    "password"));

    /* SHA512: hash is for password: "wrong" */
    LONGS_EQUAL(0,
                relay_weechat_protocol_check_hash (
                    "sha512:4a80cdd4a4c8230ec1acd2ce3b6139819e914f4db4dc46ec6"
                    "21d0add88d5e3054b438359bac599fc1e101da39e9d2fe23b9fdd562"
                    "5893f6a79f982127034622a",
                    "password"));
    /* SHA512: hash is for password: "password" */
    LONGS_EQUAL(1,
                relay_weechat_protocol_check_hash (
                    "sha512:b109f3bbbc244eb82441917ed06d618b9008dd09b3befd1b5"
                    "e07394c706a8bb980b1d7785e5976ec049b46df5f1326af5a2ea6d10"
                    "3fd07c95385ffab0cacbc86",
                    "password"));

    /* PBKDF2 (SHA256): hash is for password "wrong" */
    LONGS_EQUAL(0,
                relay_weechat_protocol_check_hash (
                    "pbkdf2:sha256:4142434445464748494a4b4c4d4e4f50:100000:"
                    "e8f92a75f5956e9dc3499775221e9ef121bf4d09bdca4391b69aa62c"
                    "50c2bb6b",
                    "password"));
    /* PBKDF2 (SHA256): hash is for password "password" */
    LONGS_EQUAL(1,
                relay_weechat_protocol_check_hash (
                    "pbkdf2:sha256:4142434445464748494a4b4c4d4e4f50:100000:"
                    "323d29f1762dcb5917bc8320c4eb9ea05900fc28e53cbc3e1b7f0980"
                    "2e35e2d0",
                    "password"));

    /* PBKDF2 (SHA512): hash is for password "wrong" */
    LONGS_EQUAL(0,
                relay_weechat_protocol_check_hash (
                    "pbkdf2:sha512:4142434445464748494a4b4c4d4e4f50:100000:"
                    "e682a3815a4d1de8d13a223932b6b0467b7d775111aae3794afb9a84"
                    "ee62bd50755fde725262f75d1211e8497a35c8dca8a6333bcc9f7b53"
                    "244f6ff567d25cfc",
                    "password"));
    /* PBKDF2 (SHA512): hash is for password "password" */
    LONGS_EQUAL(1,
                relay_weechat_protocol_check_hash (
                    "pbkdf2:sha512:4142434445464748494a4b4c4d4e4f50:100000:"
                    "db166999c1f415a40570a4bbd3a26d461f87e495da215c75135b77bf"
                    "910a261d3749f28264d24b546fc898908d4209704700020b8dd2bca6"
                    "e4698208dd5aa5f2",
                    "password"));
}
