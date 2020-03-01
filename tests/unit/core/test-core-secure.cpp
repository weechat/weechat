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
#include "tests/unit/core/test-core.h"
#include "src/core/wee-secure.h"
#include "src/core/wee-string.h"

#define SECURE_PASSPHRASE "this_is_a_secret_passphrase"
#define SECURE_PASSWORD "this_is_a_secret_password"
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
                secure_hash (__data, __data_size, __hash_algo,          \
                             hash, &hash_size));                        \
    if (__result_hash)                                                  \
    {                                                                   \
        MEMCMP_EQUAL(hash_expected, hash, hash_size);                   \
    }                                                                   \
    LONGS_EQUAL(hash_size_expected, hash_size);

#define WEE_CHECK_HASH_PBKDF2(__result_code, __result_hash,             \
                              __data, __data_size,                      \
                              __hash_subalgo, __salt, __salt_size,      \
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
                secure_hash_pbkdf2 (__data, __data_size,                \
                                    __hash_subalgo,                     \
                                    __salt, __salt_size,                \
                                    __iterations,                       \
                                    hash, &hash_size));                 \
    if (__result_hash)                                                  \
    {                                                                   \
        MEMCMP_EQUAL(hash_expected, hash, hash_size);                   \
    }                                                                   \
    LONGS_EQUAL(hash_size_expected, hash_size);

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
 *   secure_hash
 */

TEST(CoreSecure, Hash)
{
    const char *data = DATA_HASH;
    char hash_expected[4096], hash[4096];
    int data_size, hash_size_expected, hash_size;

    data_size = strlen (data);

    WEE_CHECK_HASH(0, NULL, NULL, 0, 0);
    WEE_CHECK_HASH(0, NULL, "test", 0, 0);

    WEE_CHECK_HASH(1, DATA_HASH_CRC32, data, data_size, GCRY_MD_CRC32);
    WEE_CHECK_HASH(1, DATA_HASH_MD5, data, data_size, GCRY_MD_MD5);
    WEE_CHECK_HASH(1, DATA_HASH_SHA1, data, data_size, GCRY_MD_SHA1);
    WEE_CHECK_HASH(1, DATA_HASH_SHA224, data, data_size, GCRY_MD_SHA224);
    WEE_CHECK_HASH(1, DATA_HASH_SHA256, data, data_size, GCRY_MD_SHA256);
    WEE_CHECK_HASH(1, DATA_HASH_SHA384, data, data_size, GCRY_MD_SHA384);
    WEE_CHECK_HASH(1, DATA_HASH_SHA512, data, data_size, GCRY_MD_SHA512);
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_224, data, data_size, GCRY_MD_SHA3_224);
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_256, data, data_size, GCRY_MD_SHA3_256);
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_384, data, data_size, GCRY_MD_SHA3_384);
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_512, data, data_size, GCRY_MD_SHA3_512);
}

/*
 * Tests functions:
 *   secure_hash_pbkdf2
 */

TEST(CoreSecure, HashPbkdf2)
{
    const char *data = DATA_HASH, *salt = DATA_HASH_SALT;
    char hash_expected[4096], hash[4096];
    int data_size, salt_size, hash_size_expected, hash_size;

    data_size = strlen (data);
    salt_size = strlen (salt);

    WEE_CHECK_HASH_PBKDF2(0, NULL, NULL, 0, 0, NULL, 0, 0);
    WEE_CHECK_HASH_PBKDF2(0, NULL, "test", 0, 0, NULL, 0, 0);
    WEE_CHECK_HASH_PBKDF2(0, NULL, "test", 4, GCRY_MD_SHA1, "salt", 4, 0);

    /* SHA1 */
    WEE_CHECK_HASH_PBKDF2(1, DATA_HASH_PBKDF2_SHA1_1000,
                          data, data_size,
                          GCRY_MD_SHA1,
                          DATA_HASH_SALT, salt_size,
                          1000);
    WEE_CHECK_HASH_PBKDF2(1, DATA_HASH_PBKDF2_SHA1_100000,
                          data, data_size,
                          GCRY_MD_SHA1,
                          DATA_HASH_SALT, salt_size,
                          100000);

    /* SHA256 */
    WEE_CHECK_HASH_PBKDF2(1, DATA_HASH_PBKDF2_SHA256_1000,
                          data, data_size,
                          GCRY_MD_SHA256,
                          DATA_HASH_SALT, salt_size,
                          1000);
    WEE_CHECK_HASH_PBKDF2(1, DATA_HASH_PBKDF2_SHA256_100000,
                          data, data_size,
                          GCRY_MD_SHA256,
                          DATA_HASH_SALT, salt_size,
                          100000);

    /* SHA512 */
    WEE_CHECK_HASH_PBKDF2(1, DATA_HASH_PBKDF2_SHA512_1000,
                          data, data_size,
                          GCRY_MD_SHA512,
                          DATA_HASH_SALT, salt_size,
                          1000);
    WEE_CHECK_HASH_PBKDF2(1, DATA_HASH_PBKDF2_SHA512_100000,
                          data, data_size,
                          GCRY_MD_SHA512,
                          DATA_HASH_SALT, salt_size,
                          100000);
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
