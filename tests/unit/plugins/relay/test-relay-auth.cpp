/*
 * test-relay-auth.cpp - test client authentication functions
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
#include "src/plugins/relay/relay-auth.h"
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
 *   relay_auth_parse_sha
 */

TEST(RelayAuth, ParseSha)
{
    char *salt_hexa, *salt, *hash_sha;
    const char salt_expected[4] = { 0x41, 0x42, 0x43, 0x44 };  /* "ABCD" */
    int salt_size;

    /* NULL string */
    WEE_CHECK_PARSE_SHA(NULL);
    POINTERS_EQUAL(NULL, salt_hexa);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    POINTERS_EQUAL(NULL, hash_sha);

    /* not enough parameters: 0 (expected: 2) */
    WEE_CHECK_PARSE_SHA("");
    POINTERS_EQUAL(NULL, salt_hexa);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    POINTERS_EQUAL(NULL, hash_sha);

    /* not enough parameters: 1 (expected: 2) */
    WEE_CHECK_PARSE_SHA("41424344");
    POINTERS_EQUAL(NULL, salt_hexa);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    POINTERS_EQUAL(NULL, hash_sha);

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
    POINTERS_EQUAL(NULL, salt_hexa);
    POINTERS_EQUAL(NULL, salt);
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
    POINTERS_EQUAL(NULL, salt_hexa);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    POINTERS_EQUAL(NULL, hash_pbkdf2);

    /* not enough parameters: 0 (expected: 3) */
    WEE_CHECK_PARSE_PBKDF2("");
    POINTERS_EQUAL(NULL, salt_hexa);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    POINTERS_EQUAL(NULL, hash_pbkdf2);

    /* not enough parameters: 1 (expected: 3) */
    WEE_CHECK_PARSE_PBKDF2("41424344");
    POINTERS_EQUAL(NULL, salt_hexa);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    POINTERS_EQUAL(NULL, hash_pbkdf2);

    /* not enough parameters: 2 (expected: 3) */
    WEE_CHECK_PARSE_PBKDF2("41424344:100000");
    POINTERS_EQUAL(NULL, salt_hexa);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(0, iterations);
    POINTERS_EQUAL(NULL, hash_pbkdf2);

    /* good parameters */
    WEE_CHECK_PARSE_PBKDF2("41424344:100000:01757d53157ca14a1419e3a8cc1563536"
                           "520a60b76d2d48e7f9ac09afc945a1c");
    STRCMP_EQUAL("41424344", salt_hexa);
    MEMCMP_EQUAL(salt_expected, salt, 4);
    LONGS_EQUAL(4, salt_size);
    LONGS_EQUAL(100000, iterations);
    STRCMP_EQUAL("01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7f9ac09afc9"
                 "45a1c",
                 hash_pbkdf2);
    free (salt_hexa);
    free (salt);
    free (hash_pbkdf2);

    /* wrong salt */
    WEE_CHECK_PARSE_PBKDF2("Z:100000:01757d53157ca14a1419e3a8cc1563536520a60b"
                           "76d2d48e7f9ac09afc945a1c");
    POINTERS_EQUAL(NULL, salt_hexa);
    POINTERS_EQUAL(NULL, salt);
    LONGS_EQUAL(0, salt_size);
    LONGS_EQUAL(100000, iterations);
    STRCMP_EQUAL("01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7f9ac09afc9"
                 "45a1c",
                 hash_pbkdf2);
    free (hash_pbkdf2);

    /* wrong iterations */
    WEE_CHECK_PARSE_PBKDF2("41424344:abcd:01757d53157ca14a1419e3a8cc156353652"
                           "0a60b76d2d48e7f9ac09afc945a1c");
    STRCMP_EQUAL("41424344", salt_hexa);
    MEMCMP_EQUAL(salt_expected, salt, 4);
    LONGS_EQUAL(4, salt_size);
    LONGS_EQUAL(0, iterations);
    STRCMP_EQUAL("01757d53157ca14a1419e3a8cc1563536520a60b76d2d48e7f9ac09afc9"
                 "45a1c",
                 hash_pbkdf2);
    free (salt_hexa);
    free (salt);
    free (hash_pbkdf2);
}

/*
 * Tests functions:
 *   relay_auth_check_password_plain
 */

TEST(RelayAuth, CheckPasswordPlain)
{
    /* invalid arguments */
    LONGS_EQUAL(0, relay_auth_check_password_plain (NULL, NULL));
    LONGS_EQUAL(0, relay_auth_check_password_plain ("abcd", NULL));
    LONGS_EQUAL(0, relay_auth_check_password_plain (NULL, "password"));

    /* wrong password */
    LONGS_EQUAL(0, relay_auth_check_password_plain ("test", "password"));
    LONGS_EQUAL(0, relay_auth_check_password_plain ("Password", "password"));

    /* good password */
    LONGS_EQUAL(1, relay_auth_check_password_plain ("", ""));
    LONGS_EQUAL(1, relay_auth_check_password_plain ("password", "password"));
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
                    "sha256", salt, sizeof (salt), 100000, NULL, NULL));
    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    "sha256", salt, sizeof (salt), 100000, "", ""));

    /* PBKDF2 (SHA256): hash is for password "wrong" */
    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    "sha256",
                    salt,
                    sizeof (salt),
                    100000,
                    "e8f92a75f5956e9dc3499775221e9ef121bf4d09bdca4391b69aa62c"
                    "50c2bb6b",
                    "password"));
    /* PBKDF2 (SHA256): hash is for password "password" */
    LONGS_EQUAL(1,
                relay_auth_check_hash_pbkdf2 (
                    "sha256",
                    salt,
                    sizeof (salt),
                    100000,
                    "323d29f1762dcb5917bc8320c4eb9ea05900fc28e53cbc3e1b7f0980"
                    "2e35e2d0",
                    "password"));

    /* PBKDF2 (SHA512): hash is for password "wrong" */
    LONGS_EQUAL(0,
                relay_auth_check_hash_pbkdf2 (
                    "sha512",
                    salt,
                    sizeof (salt),
                    100000,
                    "e682a3815a4d1de8d13a223932b6b0467b7d775111aae3794afb9a84"
                    "ee62bd50755fde725262f75d1211e8497a35c8dca8a6333bcc9f7b53"
                    "244f6ff567d25cfc",
                    "password"));
    /* PBKDF2 (SHA512): hash is for password "password" */
    LONGS_EQUAL(1,
                relay_auth_check_hash_pbkdf2 (
                    "sha512",
                    salt,
                    sizeof (salt),
                    100000,
                    "db166999c1f415a40570a4bbd3a26d461f87e495da215c75135b77bf"
                    "910a261d3749f28264d24b546fc898908d4209704700020b8dd2bca6"
                    "e4698208dd5aa5f2",
                    "password"));
}
