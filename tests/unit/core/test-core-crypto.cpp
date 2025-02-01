/*
 * test-core-crypto.cpp - test cryptographic functions
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <gcrypt.h>
#include "src/core/core-crypto.h"
#include "src/core/core-string.h"

/* Hash */
#define DATA_HASH_MSG "this is a test of hash function"
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
#define DATA_HASH_SHA512_224 "5c442c9389a2b72103e16a863e753f4ca98f232ba13e6" \
    "946df97f955"
#define DATA_HASH_SHA512_256 "d9157dc21fffaaea696a868d2c5b29ca7b622c9bcdd7d" \
    "55031589c4a840d43cc"
#define DATA_HASH_SHA3_224 "26432a3a4ea998790be43386b1de417f88be43146a4af98" \
    "2a9627d10"
#define DATA_HASH_SHA3_256 "226e3830306711cf653c1661765c304b37038e7457c35dd" \
    "14fca0f6a8ba1d2e3"
#define DATA_HASH_SHA3_384 "77bc16f89c102efc783ddeccc71862fe919b66e1aaa88bd" \
    "2ba5f0bbe604fcb86c68f0e401d5d553597366cdd400595ba"
#define DATA_HASH_SHA3_512 "31dfb5fc8f30ac7007acddc4fce562d408706833d0d2af2" \
    "e5f61a179099592927ff7d100e278406c7f98d42575001e26e153b135c21f7ef5b00c8" \
    "cef93ca048d"
#define DATA_HASH_BLAKE2B_160 "4b69099962d678140e7c22f3f98edad60432ed3d"
#define DATA_HASH_BLAKE2B_256 "21b3e26905be39894328222c10b009a64633109db228" \
    "df8222d1ff61cf6bd6a8"
#define DATA_HASH_BLAKE2B_384 "a3e35d3ac1b866a4836cefe4c29610792c30c5380dcf" \
    "56fdffa29397b92110fba0d24df470f0aa4563d12f3e31511bab"
#define DATA_HASH_BLAKE2B_512 "ef694e494cf17a4c5e43644d185ee48e2f16ec85e13d" \
    "bd22dfcc415c7eb187baa08befe3422d630de486f07d417551730db8d29944c151bdfe" \
    "d016e84510565c"
#define DATA_HASH_BLAKE2S_128 "b0c4131eab265ea16b7b8b4770ac7b7d"
#define DATA_HASH_BLAKE2S_160 "02d2dde62d0512368041ddbbda348404f3c8d528"
#define DATA_HASH_BLAKE2S_224 "2f25961aff8a79b4ac9a1cfd956d2b590bb900466660" \
    "0d595820acaf"
#define DATA_HASH_BLAKE2S_256 "f0fa555b88a92ec73b25527da818338fcf295449e6c0" \
    "04b8b0ec392e0fc44d7c"

/* Hash PBKDF2 */
#define DATA_HASH_SALT "this is a salt of 32 bytes xxxxx"
#define DATA_HASH_PBKDF2_SHA1_1000 "85ce23c8873830df8f0a96aa82ae7d7635dad12" \
    "7"
#define DATA_HASH_PBKDF2_SHA256_1000 "0eb0a795537a8c37a2d7d7e50a076e07c9a8e" \
    "e9aa281669381af99fad198997c"
#define DATA_HASH_PBKDF2_SHA512_1000 "03d8e9e86f3bbe20b88a600a5aa15f8cfbee0" \
    "a402af301e1714c25467a32489c773c71eddf5aa39f42823ecc54c9e9b015517b5f3c0" \
    "19bae9463a2d8fe527882"

/* HMAC */
#define DATA_HMAC_KEY "secret key"
#define DATA_HMAC_MSG "this is a test of hmac function"
#define DATA_HMAC_CRC32 "3c189d75"
#define DATA_HMAC_MD5 "8148a8e01eb0c6ca42880ea58f50d045"
#define DATA_HMAC_SHA1 "28dea5713c0d48c7638db31050a7ded4308f46fe"
#define DATA_HMAC_SHA224 "f1cf0ccf287a2e35b98414346931396d47ca929c92c48edcc" \
    "e8e0b9e"
#define DATA_HMAC_SHA256 "7be1b4281c0d74d4a3838892b1512efa13a25c7a50d7dce47" \
    "da070c7e7c65dee"
#define DATA_HMAC_SHA384 "8cd5f4afc602e11f6b3032fd65e906da810ac51aeb7d30f4b" \
    "7b495ae3dcc0eede0c5f63d7d2e3688fe658daf4852be67"
#define DATA_HMAC_SHA512 "940e5c280c08cd858f79a6085b4bdc54710ed339dd1008fa2" \
    "1643b7bbeea8a5f61c77f395708505461af62776c9cb7be1c263f39055eb8478190cd8" \
    "0ea5b0850"
#define DATA_HMAC_SHA512_224 "521860f56b6c429a20357055dd1f18ea706543c2e2bab" \
    "be06ff8c610"
#define DATA_HMAC_SHA512_256 "1da19faaa2b3fca54a08c6694123c465e7da76ad8c672" \
    "a5ad323e824e1c3b523"
#define DATA_HMAC_SHA3_224 "a08c7f1598ecc7ea54feeb920ef90b3748d59b3203caa74" \
    "7316eb2d4"
#define DATA_HMAC_SHA3_256 "21aca280bc1ac1fa261b1169a321eb7a49e38a8ddec66a8" \
    "fa2ed9c43d7fae4c5"
#define DATA_HMAC_SHA3_384 "cbf189e8cd31f3c1c5742e2688b13be8e62691952eee374" \
    "9523b48bd7a7d1cdf38812cf9a3e52dbb1d0e32a11e478ce7"
#define DATA_HMAC_SHA3_512 "b1eeb16dd18f66cc8886754ac9cf238deea24d9797ceecb" \
    "9e0582148bfb6b88f7530d594e80a5a5e22e351a079855983da91b0011dff85ea4a895" \
    "e8fde6fd41a"

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

#define WEE_CHECK_HASH_FILE(__result_code, __result_hash,               \
                            __filename, __hash_algo)                    \
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
                weecrypto_hash_file (__filename, __hash_algo,           \
                                     hash, &hash_size));                \
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

#define WEE_CHECK_HMAC(__result_code, __result_hash,                    \
                       __key, __key_size, __message, __message_size,    \
                       __hash_algo)                                     \
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
                weecrypto_hmac (__key, __key_size,                      \
                                __message, __message_size,              \
                                __hash_algo,                            \
                                hash, &hash_size));                     \
    if (__result_hash)                                                  \
    {                                                                   \
        MEMCMP_EQUAL(hash_expected, hash, hash_size);                   \
    }                                                                   \
    LONGS_EQUAL(hash_size_expected, hash_size);

#define WEE_CHECK_TOTP_GENERATE(__result, __secret, __time, __digits)   \
    totp = weecrypto_totp_generate (__secret, __time, __digits);        \
    STRCMP_EQUAL(__result, totp);                                       \
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
#if GCRYPT_VERSION_NUMBER >= 0x010904
    LONGS_EQUAL(GCRY_MD_SHA512_224, weecrypto_get_hash_algo ("sha512-224"));
    LONGS_EQUAL(GCRY_MD_SHA512_256, weecrypto_get_hash_algo ("sha512-256"));
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010700
    LONGS_EQUAL(GCRY_MD_SHA3_224, weecrypto_get_hash_algo ("sha3-224"));
    LONGS_EQUAL(GCRY_MD_SHA3_256, weecrypto_get_hash_algo ("sha3-256"));
    LONGS_EQUAL(GCRY_MD_SHA3_384, weecrypto_get_hash_algo ("sha3-384"));
    LONGS_EQUAL(GCRY_MD_SHA3_512, weecrypto_get_hash_algo ("sha3-512"));
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010800
    LONGS_EQUAL(GCRY_MD_BLAKE2B_160, weecrypto_get_hash_algo ("blake2b-160"));
    LONGS_EQUAL(GCRY_MD_BLAKE2B_256, weecrypto_get_hash_algo ("blake2b-256"));
    LONGS_EQUAL(GCRY_MD_BLAKE2B_384, weecrypto_get_hash_algo ("blake2b-384"));
    LONGS_EQUAL(GCRY_MD_BLAKE2B_512, weecrypto_get_hash_algo ("blake2b-512"));
    LONGS_EQUAL(GCRY_MD_BLAKE2S_128, weecrypto_get_hash_algo ("blake2s-128"));
    LONGS_EQUAL(GCRY_MD_BLAKE2S_160, weecrypto_get_hash_algo ("blake2s-160"));
    LONGS_EQUAL(GCRY_MD_BLAKE2S_224, weecrypto_get_hash_algo ("blake2s-224"));
    LONGS_EQUAL(GCRY_MD_BLAKE2S_256, weecrypto_get_hash_algo ("blake2s-256"));
#endif
}

/*
 * Tests functions:
 *   weecrypto_get_cipher
 */

TEST(CoreCrypto, GetCipher)
{
    LONGS_EQUAL(GCRY_CIPHER_NONE, weecrypto_get_cipher (NULL));
    LONGS_EQUAL(GCRY_CIPHER_NONE, weecrypto_get_cipher (""));
    LONGS_EQUAL(GCRY_CIPHER_NONE, weecrypto_get_cipher ("not_a_cipher"));

    LONGS_EQUAL(GCRY_CIPHER_AES128, weecrypto_get_cipher ("aes128"));
    LONGS_EQUAL(GCRY_CIPHER_AES192, weecrypto_get_cipher ("aes192"));
    LONGS_EQUAL(GCRY_CIPHER_AES256, weecrypto_get_cipher ("aes256"));
}

/*
 * Tests functions:
 *   weecrypto_hash
 */

TEST(CoreCrypto, Hash)
{
    const char *data = DATA_HASH_MSG;
    char hash_expected[4096], hash[4096];
    int data_size, hash_size_expected, hash_size;

    data_size = strlen (data);

    WEE_CHECK_HASH(0, NULL, NULL, 0, GCRY_MD_SHA256);
    WEE_CHECK_HASH(0, NULL, "test", 0, GCRY_MD_SHA256);

    LONGS_EQUAL (0, weecrypto_hash (data, data_size, GCRY_MD_SHA256,
                                    NULL, NULL));

    WEE_CHECK_HASH(1, DATA_HASH_CRC32, data, data_size, GCRY_MD_CRC32);
    WEE_CHECK_HASH(1, DATA_HASH_MD5, data, data_size, GCRY_MD_MD5);
    WEE_CHECK_HASH(1, DATA_HASH_SHA1, data, data_size, GCRY_MD_SHA1);
    WEE_CHECK_HASH(1, DATA_HASH_SHA224, data, data_size, GCRY_MD_SHA224);
    WEE_CHECK_HASH(1, DATA_HASH_SHA256, data, data_size, GCRY_MD_SHA256);
    WEE_CHECK_HASH(1, DATA_HASH_SHA384, data, data_size, GCRY_MD_SHA384);
    WEE_CHECK_HASH(1, DATA_HASH_SHA512, data, data_size, GCRY_MD_SHA512);
#if GCRYPT_VERSION_NUMBER >= 0x010904
    WEE_CHECK_HASH(1, DATA_HASH_SHA512_224, data, data_size, GCRY_MD_SHA512_224);
    WEE_CHECK_HASH(1, DATA_HASH_SHA512_256, data, data_size, GCRY_MD_SHA512_256);
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010700
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_224, data, data_size, GCRY_MD_SHA3_224);
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_256, data, data_size, GCRY_MD_SHA3_256);
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_384, data, data_size, GCRY_MD_SHA3_384);
    WEE_CHECK_HASH(1, DATA_HASH_SHA3_512, data, data_size, GCRY_MD_SHA3_512);
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010800
    WEE_CHECK_HASH(1, DATA_HASH_BLAKE2B_160, data, data_size, GCRY_MD_BLAKE2B_160);
    WEE_CHECK_HASH(1, DATA_HASH_BLAKE2B_256, data, data_size, GCRY_MD_BLAKE2B_256);
    WEE_CHECK_HASH(1, DATA_HASH_BLAKE2B_384, data, data_size, GCRY_MD_BLAKE2B_384);
    WEE_CHECK_HASH(1, DATA_HASH_BLAKE2B_512, data, data_size, GCRY_MD_BLAKE2B_512);
    WEE_CHECK_HASH(1, DATA_HASH_BLAKE2S_128, data, data_size, GCRY_MD_BLAKE2S_128);
    WEE_CHECK_HASH(1, DATA_HASH_BLAKE2S_160, data, data_size, GCRY_MD_BLAKE2S_160);
    WEE_CHECK_HASH(1, DATA_HASH_BLAKE2S_224, data, data_size, GCRY_MD_BLAKE2S_224);
    WEE_CHECK_HASH(1, DATA_HASH_BLAKE2S_256, data, data_size, GCRY_MD_BLAKE2S_256);
#endif
}

/*
 * Tests functions:
 *   weecrypto_hash_file
 */

TEST(CoreCrypto, HashFile)
{
    const char *data = DATA_HASH_MSG;
    char *filename, hash_expected[4096], hash[4096];
    FILE *file;
    int hash_size_expected, hash_size;

    filename = string_eval_path_home ("${weechat_data_dir}/test_file.txt",
                                      NULL, NULL, NULL);
    file = fopen (filename, "w");
    fwrite (data, 1, strlen (data), file);
    fflush (file);
    fclose (file);

    WEE_CHECK_HASH_FILE(0, NULL, NULL, GCRY_MD_SHA256);

    LONGS_EQUAL (0, weecrypto_hash_file (filename, GCRY_MD_SHA256,
                                         NULL, NULL));

    WEE_CHECK_HASH_FILE(1, DATA_HASH_CRC32, filename, GCRY_MD_CRC32);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_MD5, filename, GCRY_MD_MD5);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA1, filename, GCRY_MD_SHA1);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA224, filename, GCRY_MD_SHA224);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA256, filename, GCRY_MD_SHA256);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA384, filename, GCRY_MD_SHA384);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA512, filename, GCRY_MD_SHA512);
#if GCRYPT_VERSION_NUMBER >= 0x010904
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA512_224, filename, GCRY_MD_SHA512_224);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA512_256, filename, GCRY_MD_SHA512_256);
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010700
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA3_224, filename, GCRY_MD_SHA3_224);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA3_256, filename, GCRY_MD_SHA3_256);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA3_384, filename, GCRY_MD_SHA3_384);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_SHA3_512, filename, GCRY_MD_SHA3_512);
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010800
    WEE_CHECK_HASH_FILE(1, DATA_HASH_BLAKE2B_160, filename, GCRY_MD_BLAKE2B_160);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_BLAKE2B_256, filename, GCRY_MD_BLAKE2B_256);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_BLAKE2B_384, filename, GCRY_MD_BLAKE2B_384);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_BLAKE2B_512, filename, GCRY_MD_BLAKE2B_512);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_BLAKE2S_128, filename, GCRY_MD_BLAKE2S_128);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_BLAKE2S_160, filename, GCRY_MD_BLAKE2S_160);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_BLAKE2S_224, filename, GCRY_MD_BLAKE2S_224);
    WEE_CHECK_HASH_FILE(1, DATA_HASH_BLAKE2S_256, filename, GCRY_MD_BLAKE2S_256);
#endif

    unlink (filename);
    free (filename);
}

/*
 * Tests functions:
 *   weecrypto_hash_pbkdf2
 */

TEST(CoreCrypto, HashPbkdf2)
{
    const char *data = DATA_HASH_MSG, *salt = DATA_HASH_SALT;
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
 *   weecrypto_hmac
 */

TEST(CoreCrypto, Hmac)
{
    const char *key = DATA_HMAC_KEY, *msg = DATA_HMAC_MSG;
    char hash_expected[4096], hash[4096];
    int key_size, msg_size, hash_size_expected, hash_size;

    key_size = strlen (key);
    msg_size = strlen (msg);

    WEE_CHECK_HMAC(0, NULL, NULL, 0, NULL, 0, 0);
    WEE_CHECK_HMAC(0, NULL, "key", 0, NULL, 0, 0);
    WEE_CHECK_HMAC(0, NULL, NULL, 0, "msg", 0, 0);
    WEE_CHECK_HMAC(0, NULL, "key", 0, "msg", 0, 0);

    LONGS_EQUAL (0, weecrypto_hmac (key, key_size, msg, msg_size,
                                    GCRY_MD_SHA256, NULL, NULL));

    WEE_CHECK_HMAC(1, DATA_HMAC_CRC32, key, key_size, msg, msg_size, GCRY_MD_CRC32);
    WEE_CHECK_HMAC(1, DATA_HMAC_MD5, key, key_size, msg, msg_size, GCRY_MD_MD5);
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA1, key, key_size, msg, msg_size, GCRY_MD_SHA1);
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA224, key, key_size, msg, msg_size, GCRY_MD_SHA224);
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA256, key, key_size, msg, msg_size, GCRY_MD_SHA256);
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA384, key, key_size, msg, msg_size, GCRY_MD_SHA384);
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA512, key, key_size, msg, msg_size, GCRY_MD_SHA512);
#if GCRYPT_VERSION_NUMBER >= 0x010904
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA512_224, key, key_size, msg, msg_size, GCRY_MD_SHA512_224);
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA512_256, key, key_size, msg, msg_size, GCRY_MD_SHA512_256);
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010700
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA3_224, key, key_size, msg, msg_size, GCRY_MD_SHA3_224);
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA3_256, key, key_size, msg, msg_size, GCRY_MD_SHA3_256);
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA3_384, key, key_size, msg, msg_size, GCRY_MD_SHA3_384);
    WEE_CHECK_HMAC(1, DATA_HMAC_SHA3_512, key, key_size, msg, msg_size, GCRY_MD_SHA3_512);
#endif
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
