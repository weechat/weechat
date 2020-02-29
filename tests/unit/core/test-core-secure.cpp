/*
 * test-core-secure.cpp - test secured data functions
 *
 * Copyright (C) 2018-2020 Sébastien Helleu <flashcode@flashtux.org>
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
#include <gcrypt.h>
#include "src/core/wee-secure.h"
#include "src/core/wee-string.h"

#define DATA_HASH "this is a test of hash function"
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
#define SECURE_PASSPHRASE "this_is_a_secret_passphrase"
#define SECURE_PASSWORD "this_is_a_secret_password"
#define TOTP_SECRET "secretpasswordbase32"

#define WEE_CHECK_HASH_BIN(__result, __buffer, __length, __hash_algo)   \
    if (__result)                                                       \
    {                                                                   \
        result_bin = (char *)malloc (4096);                             \
        length_bin = string_base16_decode (__result,                    \
                                           (char *)result_bin);         \
    }                                                                   \
    else                                                                \
    {                                                                   \
        result_bin = NULL;                                              \
        length_bin = 0;                                                 \
    }                                                                   \
    secure_hash_binary (__buffer, __length, __hash_algo,                \
                        &hash_bin, &length_hash_bin);                   \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, hash_bin);                                 \
    }                                                                   \
    else                                                                \
    {                                                                   \
        MEMCMP_EQUAL(result_bin, hash_bin, length_hash_bin);            \
    }                                                                   \
    LONGS_EQUAL(length_bin, length_hash_bin);                           \
    if (result_bin)                                                     \
        free (result_bin);                                              \
    if (hash_bin)                                                       \
        free (hash_bin);

#define WEE_CHECK_HASH_HEX(__result, __buffer, __length, __hash_algo)   \
    hash = secure_hash (__buffer, __length, __hash_algo);               \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, hash);                                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, hash);                                   \
    }                                                                   \
    if (hash)                                                           \
        free (hash);

#define WEE_CHECK_TOTP_GENERATE(__result, __secret, __time, __digits)   \
    totp = secure_totp_generate (__secret, __time, __digits);           \
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
    LONGS_EQUAL(__result, secure_totp_validate (__secret, __time,       \
                                                __window, __otp));

extern int secure_derive_key (const char *salt, const char *passphrase,
                              unsigned char *key, int length_key);
}

TEST_GROUP(CoreSecure)
{
};

/*
 * Tests functions:
 *   secure_hash_binary
 *   secure_hash
 */

TEST(CoreSecure, Hash)
{
    const char *data = DATA_HASH;
    char *result_bin, *hash_bin, *hash;
    int length, length_bin, length_hash_bin;

    length = strlen (data);

    WEE_CHECK_HASH_BIN(NULL, NULL, 0, 0);
    WEE_CHECK_HASH_HEX(NULL, NULL, 0, 0);

    WEE_CHECK_HASH_BIN(NULL, "test", 0, 0);
    WEE_CHECK_HASH_HEX(NULL, "test", 0, 0);

    WEE_CHECK_HASH_BIN(DATA_HASH_MD5, data, length, GCRY_MD_MD5);
    WEE_CHECK_HASH_HEX(DATA_HASH_MD5, data, length, GCRY_MD_MD5);

    WEE_CHECK_HASH_BIN(DATA_HASH_SHA1, data, length, GCRY_MD_SHA1);
    WEE_CHECK_HASH_HEX(DATA_HASH_SHA1, data, length, GCRY_MD_SHA1);

    WEE_CHECK_HASH_BIN(DATA_HASH_SHA224, data, length, GCRY_MD_SHA224);
    WEE_CHECK_HASH_HEX(DATA_HASH_SHA224, data, length, GCRY_MD_SHA224);

    WEE_CHECK_HASH_BIN(DATA_HASH_SHA256, data, length, GCRY_MD_SHA256);
    WEE_CHECK_HASH_HEX(DATA_HASH_SHA256, data, length, GCRY_MD_SHA256);

    WEE_CHECK_HASH_BIN(DATA_HASH_SHA384, data, length, GCRY_MD_SHA384);
    WEE_CHECK_HASH_HEX(DATA_HASH_SHA384, data, length, GCRY_MD_SHA384);

    WEE_CHECK_HASH_BIN(DATA_HASH_SHA512, data, length, GCRY_MD_SHA512);
    WEE_CHECK_HASH_HEX(DATA_HASH_SHA512, data, length, GCRY_MD_SHA512);

    WEE_CHECK_HASH_BIN(DATA_HASH_SHA3_224, data, length, GCRY_MD_SHA3_224);
    WEE_CHECK_HASH_HEX(DATA_HASH_SHA3_224, data, length, GCRY_MD_SHA3_224);

    WEE_CHECK_HASH_BIN(DATA_HASH_SHA3_256, data, length, GCRY_MD_SHA3_256);
    WEE_CHECK_HASH_HEX(DATA_HASH_SHA3_256, data, length, GCRY_MD_SHA3_256);

    WEE_CHECK_HASH_BIN(DATA_HASH_SHA3_384, data, length, GCRY_MD_SHA3_384);
    WEE_CHECK_HASH_HEX(DATA_HASH_SHA3_384, data, length, GCRY_MD_SHA3_384);

    WEE_CHECK_HASH_BIN(DATA_HASH_SHA3_512, data, length, GCRY_MD_SHA3_512);
    WEE_CHECK_HASH_HEX(DATA_HASH_SHA3_512, data, length, GCRY_MD_SHA3_512);
}

/*
 * Tests functions:
 *   secure_derive_key
 */

TEST(CoreSecure, DeriveKey)
{
    char salt[SECURE_SALT_SIZE], zeroes[32];
    unsigned char *key;
    const char *passphrase = "this is the passphrase";
    const char *sha512 = "a81161a80731aa439adff8dfde94540a258b5d912f3579ec7b4"
        "709968ed0f466e9c63f29d86196aee2c2725f046ef1c074ee790dbabb2ddb09ce85d"
        "4a12bba0e";
    unsigned char sha512_bin[64 + 1];

    memset (salt, 'A', SECURE_SALT_SIZE);
    memset (zeroes, 0, 32);
    key = (unsigned char *)malloc (64);  /* SHA512 */
    string_base16_decode (sha512, (char *)sha512_bin);

    LONGS_EQUAL(0, secure_derive_key (NULL, NULL, NULL, 0));
    LONGS_EQUAL(0, secure_derive_key (salt, NULL, NULL, 0));
    LONGS_EQUAL(0, secure_derive_key (salt, passphrase, NULL, 0));
    LONGS_EQUAL(0, secure_derive_key (salt, passphrase, key, 0));

    /* test with key size == 64 (SHA512) */
    memset (key, 0, 64);
    LONGS_EQUAL(1, secure_derive_key (salt, passphrase, key, 64));
    MEMCMP_EQUAL(sha512_bin, key, 64);

    /* test with key size == 32 (too small for SHA512) */
    memset (key, 0, 64);
    LONGS_EQUAL(1, secure_derive_key (salt, passphrase, key, 32));
    MEMCMP_EQUAL(sha512_bin, key, 32);
    MEMCMP_EQUAL(zeroes, key + 32, 32);

    free (key);
}

/*
 * Tests functions:
 *   secure_encrypt_data
 *   secure_decrypt_data
 */

TEST(CoreSecure, EncryptDecryptData)
{
    const char *password = SECURE_PASSWORD;
    int hash_algo, cipher, rc;
    int length_password, length_encrypted_data, length_decrypted_data;
    char *encrypted_data, *decrypted_data;

    /* compute length of password, including the final \0 */
    length_password = strlen (password) + 1;

    for (hash_algo = 0; secure_hash_algo_string[hash_algo]; hash_algo++)
    {
        for (cipher = 0; secure_cipher_string[cipher]; cipher++)
        {
            /* initialize data */
            encrypted_data = NULL;
            decrypted_data = NULL;
            length_encrypted_data = 0;
            length_decrypted_data = 0;

            /*
             * encrypt the password with a hash algo, cipher and arbitrary
             * passphrase
             */
            rc = secure_encrypt_data (password,
                                      length_password,
                                      secure_hash_algo[hash_algo],
                                      secure_cipher[cipher],
                                      SECURE_PASSPHRASE,
                                      &encrypted_data,
                                      &length_encrypted_data);
            LONGS_EQUAL(0, rc);

            /* decrypt the encrypted password */
            rc = secure_decrypt_data (encrypted_data,
                                      length_encrypted_data,
                                      secure_hash_algo[hash_algo],
                                      secure_cipher[cipher],
                                      SECURE_PASSPHRASE,
                                      &decrypted_data,
                                      &length_decrypted_data);
            LONGS_EQUAL(0, rc);

            /* check decrypted data */
            LONGS_EQUAL(length_password, length_decrypted_data);
            STRCMP_EQUAL(password, decrypted_data);

            /* free encrypted/decrypted data */
            free (encrypted_data);
            free (decrypted_data);
        }
    }
}

/*
 * Tests functions:
 *   secure_totp_generate
 */

TEST(CoreSecure, TotpGenerate)
{
    char *totp;

    /* invalid secret */
    WEE_CHECK_TOTP_GENERATE(NULL, NULL, 0, 6);
    WEE_CHECK_TOTP_GENERATE(NULL, "", 0, 6);
    WEE_CHECK_TOTP_GENERATE(NULL, "not_in_base32_0189", 0, 6);

    /* invalid number of digits (must be between 4 and 10) */
    WEE_CHECK_TOTP_GENERATE(NULL, TOTP_SECRET, 0, 3);
    WEE_CHECK_TOTP_GENERATE(NULL, TOTP_SECRET, 0, 11);

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
 *   secure_totp_validate
 */

TEST(CoreSecure, TotpValidate)
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
