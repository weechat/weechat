/*
 * test-core-secure.cpp - test secured data functions
 *
 * Copyright (C) 2018-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/core-config-file.h"
#include "src/core/core-crypto.h"
#include "src/core/core-secure.h"
#include "src/core/core-secure-config.h"
#include "src/core/core-string.h"

#define SECURE_PASSPHRASE "this_is_a_secret_passphrase"
#define SECURE_PASSWORD "this_is_a_secret_password"

extern int secure_derive_key (const char *salt, const char *passphrase,
                              unsigned char *key, int length_key);
}

TEST_GROUP(CoreSecure)
{
};

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
    int i, j, hash_algo, cipher, rc;
    int length_password, length_encrypted_data, length_decrypted_data;
    char *encrypted_data, *decrypted_data;

    /* compute length of password, including the final \0 */
    length_password = strlen (password) + 1;

    for (i = 0; i <= secure_config_crypt_hash_algo->max; i++)
    {
        hash_algo = weecrypto_get_hash_algo (
            secure_config_crypt_hash_algo->string_values[i]);
        if (hash_algo == GCRY_MD_NONE)
            continue;
        for (j = 0; j <= secure_config_crypt_cipher->max; j++)
        {
            cipher = weecrypto_get_cipher (
                secure_config_crypt_cipher->string_values[j]);
            if (cipher == GCRY_CIPHER_NONE)
                continue;

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
                                      hash_algo,
                                      cipher,
                                      SECURE_PASSPHRASE,
                                      &encrypted_data,
                                      &length_encrypted_data);
            LONGS_EQUAL(0, rc);

            /* decrypt the encrypted password */
            rc = secure_decrypt_data (encrypted_data,
                                      length_encrypted_data,
                                      hash_algo,
                                      cipher,
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
