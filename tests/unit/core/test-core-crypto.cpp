/*
 * test-core-crypto.cpp - test cryptographic functions
 *
 * Copyright (C) 2020-2021 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include <ctype.h>
#include <gcrypt.h>
#include "src/core/wee-crypto.h"
#include "src/core/wee-string.h"

#define DATA_HASH "this is a test of hash function"
#define DATA_HASH_CRC32 "ef26fe3e"
#define DATA_HASH_MD5 "1197d121af621ac6a63cb8ef6b5dfa30"
#define DATA_HASH_SHA1 "799d818061175b400dc5aaeb14b8d32cdef32ff0"
#define DATA_HASH_SHA224 "637d21f3ba3f4e9fa9fb889dc990b31a658cb37b4aefb5144" \
    "70b016d"
#define DATA_HASH_SHA256 "b9a4c3393dfac4330736684510378851e581c68add8eca841" \
    "10c31a33e694676"
#define DATA_HASH_SHA384 "42853280be9b8409eed265f272bd580e2fbd448b7c7e236c7" \
    "f37dafec7906d51d982dc84ec70a4733eca49d86ac19455"
#define DATA_HASH_SHA512 "4469190d4e0d1fdc0afb6f408d9873c89b8ce89cc4db79fe0" \
    "58255c55ad6821fa5e9bb068f9e578c8ae7cc825d85ff99c439d59e439bc589d95620a" \
    "1e6b8ae6e"
#define DATA_HASH_SHA3_224 "26432a3a4ea998790be43386b1de417f88be43146a4af98" \
    "2a9627d10"
#define DATA_HASH_SHA3_256 "226e3830306711cf653c1661765c304b37038e7457c35dd" \
    "14fca0f6a8ba1d2e3"
#define DATA_HASH_SHA3_384 "77bc16f89c102efc783ddeccc71862fe919b66e1aaa88bd" \
    "2ba5f0bbe604fcb86c68f0e401d5d553597366cdd400595ba"
#define DATA_HASH_SHA3_512 "31dfb5fc8f30ac7007acddc4fce562d408706833d0d2af2" \
    "e5f61a179099592927ff7d100e278406c7f98d42575001e26e153b135c21f7ef5b00c8" \
    "cef93ca048d"
#define DATA_HASH_SALT "this is a salt of 32 bytes xxxxx"
#define DATA_HASH_PBKDF2_SHA1_1000 "85ce23c8873830df8f0a96aa82ae7d7635dad12" \
    "7"
#define DATA_HASH_PBKDF2_SHA256_1000 "0eb0a795537a8c37a2d7d7e50a076e07c9a8e" \
    "e9aa281669381af99fad198997c"
#define DATA_HASH_PBKDF2_SHA512_1000 "03d8e9e86f3bbe20b88a600a5aa15f8cfbee0" \
    "a402af301e1714c25467a32489c773c71eddf5aa39f42823ecc54c9e9b015517b5f3c0" \
    "19bae9463a2d8fe527882"

#define TOTP_SECRET "secretpasswordbase32"

#define WEE_CHECK_HASH(__result_code, __result_hash,                    \
                       __data, __data_size, __hash_algo)                \
    if (__result_hash)                                                  \
    {                                                                   \
        hash_size_expected = string_base16_decode (__result_hash,       \
                                                   hash_expected);      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        hash_size_expected = 0;                                         \
    }                                                                   \
    hash_size = -1;                                                     \
    LONGS_EQUAL(__result_code,                                          \
                weecrypto_hash (__data, __data_size, __hash_algo,       \
                                hash, &hash_size));                     \
    if (__result_hash)                                                  \
    {                                                                   \
        MEMCMP_EQUAL(hash_expected, hash, hash_size);                   \
    }                                                                   \
    LONGS_EQUAL(hash_size_expected, hash_size);

#define WEE_CHECK_HASH_PBKDF2(__result_code, __result_hash,             \
                              __data, __data_size,                      \
                              __hash_algo, __salt, __salt_size,         \
                              __iterations)                             \
    if (__result_hash)                                                  \
    {                                                                   \
        hash_size_expected = string_base16_decode (__result_hash,       \
                                                   hash_expected);      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        hash_size_expected = 0;                                         \
    }                                                                   \
    hash_size = -1;                                                     \
    LONGS_EQUAL(__result_code,                                          \
                weecrypto_hash_pbkdf2 (__data, __data_size,             \
                                       __hash_algo,                     \
                                       __salt, __salt_size,             \
                                       __iterations,                    \
                                       hash, &hash_size));              \
    if (__result_hash)                                                  \
    {                                                                   \
        MEMCMP_EQUAL(hash_expected, hash, hash_size);                   \
    }                                                                   \
    LONGS_EQUAL(hash_size_expected, hash_size);

#define WEE_CHECK_TOTP_GENERATE(__result, __secret, __time, __digits)   \
    totp = weecrypto_totp_generate (__secret, __time, __digits);        \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, totp);                                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, totp);                                   \
    }                                                                   \
    if (totp)                                                           \
        free (totp);

#define WEE_CHECK_TOTP_VALIDATE(__result, __secret, __time, __window,   \
                                __otp)                                  \
    LONGS_EQUAL(__result, weecrypto_totp_validate (__secret, __time,    \
                                                   __window, __otp));
}

TEST_GROUP(CoreCrypto)
{
};

/*
 * Tests functions:
 *   weecrypto_get_hash_algo
 */

TEST(CoreCrypto, GetHashAlgo)
{
    LONGS_EQUAL(GCRY_MD_NONE, weecrypto_get_hash_algo (NULL));
    LONGS_EQUAL(GCRY_MD_NONE, weecrypto_get_hash_algo (""));
    LONGS_EQUAL(GCRY_MD_NONE, weecrypto_get_hash_algo ("not_an_algorithm"));

    LONGS_EQUAL(GCRY_MD_CRC32, weecrypto_get_hash_algo ("crc32"));
    LONGS_EQUAL(GCRY_MD_MD5, weecrypto_get_hash_algo ("md5"));
    LONGS_EQUAL(GCRY_MD_SHA1, weecrypto_get_hash_algo ("sha1"));
    LONGS_EQUAL(GCRY_MD_SHA224, weecrypto_get_hash_algo ("sha224"));
    LONGS_EQUAL(GCRY_MD_SHA256, weecrypto_get_hash_algo ("sha256"));
    LONGS_EQUAL(GCRY_MD_SHA384, weecrypto_get_hash_algo ("sha384"));
    LONGS_EQUAL(GCRY_MD_SHA512, weecrypto_get_hash_algo ("sha512"));
#if GCRYPT_VERSION_NUMBER >= 0x010700
    LONGS_EQUAL(GCRY_MD_SHA3_224, weecrypto_get_hash_algo ("sha3-224"));
    LONGS_EQUAL(GCRY_MD_SHA3_256, weecrypto_get_hash_algo ("sha3-256"));
    LONGS_EQUAL(GCRY_MD_SHA3_384, weecrypto_get_hash_algo ("sha3-384"));
    LONGS_EQUAL(GCRY_MD_SHA3_512, weecrypto_get_hash_algo ("sha3-512"));
#endif
}

/*
 * Tests functions:
 *   weecrypto_hash
 */

TEST(CoreCrypto, Hash)
{
    const char *data = DATA_HASH;
    char hash_expected[4096], hash[4096];
    int data_size, hash_size_expected, hash_size;

    data_size = strlen (data);

    WEE_CHECK_HASH(0, NULL, NULL, 0, 0);
    WEE_CHECK_HASH(0, NULL, "test", 0, 0);

    LONGS_EQUAL (0, weecrypto_hash (data, data_size, GCRY_MD_SHA256,
                                    NULL, NULL));

    WEE_CHECK_HASH(1, DATA_HASH_CRC32, data, data_size, GCRY_MD_CRC32);
    WEE_CHECK_HASH(1, DATA_HASH_MD5, data, data_size, GCRY_MD_MD5);
    WEE_CHECK_HASH(1, DATA_HASH_SHA1, data, data_size, GCRY_MD_SHA1);
    WEE_CHECK_HASH(1, DATA_HASH_SHA224, data, data_size, GCRY_MD_SHA224);
    WEE_CHECK_HASH(1, DATA_HASH_SHA256, data, data_size, GCRY_MD_SHA256);
    WEE_CHECK_HASH(1, DATA_HASH_SHA384, data, data_size, GCRY_MD_SHA384);
    WEE_CHECK_HASH(1, DATA_HASH_SHA512, data, data_size, GCRY_MD_SHA512);
#if GCRYPT_VERSION_NUMBER >= 0x010700
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_224, data, data_size, GCRY_MD_SHA3_224);
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_256, data, data_size, GCRY_MD_SHA3_256);
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_384, data, data_size, GCRY_MD_SHA3_384);
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_512, data, data_size, GCRY_MD_SHA3_512);
#endif
}

/*
 * Tests functions:
 *   weecrypto_hash_pbkdf2
 */

TEST(CoreCrypto, HashPbkdf2)
{
    const char *data = DATA_HASH, *salt = DATA_HASH_SALT;
    char hash_expected[4096], hash[4096];
    int data_size, salt_size, hash_size_expected, hash_size;

    data_size = strlen (data);
    salt_size = strlen (salt);

    WEE_CHECK_HASH_PBKDF2(0, NULL, NULL, 0, 0, NULL, 0, 0);
    WEE_CHECK_HASH_PBKDF2(0, NULL, "test", 0, 0, NULL, 0, 0);
    WEE_CHECK_HASH_PBKDF2(0, NULL, "test", 4, GCRY_MD_SHA1, "salt", 4, 0);

    LONGS_EQUAL (0, weecrypto_hash_pbkdf2 (data, data_size, GCRY_MD_SHA256,
                                           salt, salt_size, 1000,
                                           NULL, NULL));

    /* SHA1 */
    WEE_CHECK_HASH_PBKDF2(1, DATA_HASH_PBKDF2_SHA1_1000,
                          data, data_size,
                          GCRY_MD_SHA1,
                          DATA_HASH_SALT, salt_size,
                          1000);

    /* SHA256 */
    WEE_CHECK_HASH_PBKDF2(1, DATA_HASH_PBKDF2_SHA256_1000,
                          data, data_size,
                          GCRY_MD_SHA256,
                          DATA_HASH_SALT, salt_size,
                          1000);

    /* SHA512 */
    WEE_CHECK_HASH_PBKDF2(1, DATA_HASH_PBKDF2_SHA512_1000,
                          data, data_size,
                          GCRY_MD_SHA512,
                          DATA_HASH_SALT, salt_size,
                          1000);
}

/*
 * Tests functions:
 *   weecrypto_totp_generate
 */

TEST(CoreCrypto, TotpGenerate)
{
    char *totp;

    /* invalid secret */
    WEE_CHECK_TOTP_GENERATE(NULL, NULL, 0, 6);
    WEE_CHECK_TOTP_GENERATE(NULL, "", 0, 6);
    WEE_CHECK_TOTP_GENERATE(NULL, "not_in_base32_0189", 0, 6);

    /* invalid number of digits (must be between 4 and 10) */
    WEE_CHECK_TOTP_GENERATE(NULL, TOTP_SECRET, 0, 3);
    WEE_CHECK_TOTP_GENERATE(NULL, TOTP_SECRET, 0, 11);

    /* current time */
    totp = weecrypto_totp_generate (TOTP_SECRET, 0, 6);
    CHECK(totp);
    CHECK(isdigit (totp[0]) && isdigit (totp[1]) && isdigit (totp[2])
          && isdigit (totp[3]) && isdigit (totp[4]) && isdigit (totp[5]));
    LONGS_EQUAL(6, strlen (totp));
    free (totp);

    /* TOTP with 6 digits */
    WEE_CHECK_TOTP_GENERATE("065486", TOTP_SECRET, 1540624066, 6);
    WEE_CHECK_TOTP_GENERATE("640073", TOTP_SECRET, 1540624085, 6);
    WEE_CHECK_TOTP_GENERATE("725645", TOTP_SECRET, 1540624110, 6);

    /* TOTP with 7 digits */
    WEE_CHECK_TOTP_GENERATE("0065486", TOTP_SECRET, 1540624066, 7);
    WEE_CHECK_TOTP_GENERATE("6640073", TOTP_SECRET, 1540624085, 7);
    WEE_CHECK_TOTP_GENERATE("4725645", TOTP_SECRET, 1540624110, 7);

    /* TOTP with 8 digits */
    WEE_CHECK_TOTP_GENERATE("40065486", TOTP_SECRET, 1540624066, 8);
    WEE_CHECK_TOTP_GENERATE("16640073", TOTP_SECRET, 1540624085, 8);
    WEE_CHECK_TOTP_GENERATE("94725645", TOTP_SECRET, 1540624110, 8);
}

/*
 * Tests functions:
 *   weecrypto_totp_validate
 */

TEST(CoreCrypto, TotpValidate)
{
    /* invalid secret */
    WEE_CHECK_TOTP_VALIDATE(0, NULL, 0, 0, "123456");
    WEE_CHECK_TOTP_VALIDATE(0, "", 0, 0, "123456");
    WEE_CHECK_TOTP_VALIDATE(0, "not_in_base32_0189", 0, 0, "123456");

    /* invalid window (must be ≥ 0) */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 0, -1, "123456");

    /* invalid OTP */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 0, 0, NULL);
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 0, 0, "");

    /* not enough digits in OTP (min is 4) */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1234567890, 0, "1");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1234567890, 0, "12");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1234567890, 0, "123");

    /* too many digits (max is 10) */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1234567890, 0, "12345678901");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1234567890, 0, "123456789012");

    /* current time */
    weecrypto_totp_validate (TOTP_SECRET, 0, 0, "123456");

    /* validation error (wrong OTP) */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 0, "065486");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 1, "065486");

    /* TOTP with 6 digits */
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624066, 0, "065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624085, 0, "640073");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 0, "725645");

    /* TOTP with 7 digits */
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624066, 0, "0065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624085, 0, "6640073");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 0, "4725645");

    /* TOTP with 7 digits */
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624066, 0, "40065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624085, 0, "16640073");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 0, "94725645");

    /* TOTP with 6 digits, using window */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 0, "065486");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 1, "065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 2, "065486");

    /* TOTP with 7 digits, using window */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 0, "0065486");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 1, "0065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 2, "0065486");

    /* TOTP with 8 digits, using window */
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 0, "40065486");
    WEE_CHECK_TOTP_VALIDATE(0, TOTP_SECRET, 1540624110, 1, "40065486");
    WEE_CHECK_TOTP_VALIDATE(1, TOTP_SECRET, 1540624110, 2, "40065486");
}
