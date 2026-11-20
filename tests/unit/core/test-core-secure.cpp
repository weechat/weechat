/*
 * SPDX-FileCopyrightText: 2018-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test secured data functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include <gcrypt.h>
#include "src/core/core-config-file.h"
#include "src/core/core-crypto.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-secure.h"
#include "src/core/core-secure-config.h"
#include "src/core/core-string.h"

#define SECURE_PASSPHRASE "this_is_a_secret_passphrase"
#define SECURE_PASSWORD "this_is_a_secret_password"

extern int secure_derive_key (const char *salt, const char *passphrase,
                              unsigned char *key, int length_key);
}

/*
 * Encrypt data with a passphrase and return it encoded in base16, as stored
 * in file sec.conf.
 *
 * Note: result must be freed after use.
 */

char *
test_secure_encrypt_base16 (const char *passphrase, const char *data)
{
    char *buffer, *buffer_base16;
    int length_buffer, rc;

    buffer = NULL;
    length_buffer = 0;
    rc = secure_encrypt_data (
        data, strlen (data) + 1,
        weecrypto_get_hash_algo (
            config_file_option_string (secure_config_crypt_hash_algo)),
        weecrypto_get_cipher (
            config_file_option_string (secure_config_crypt_cipher)),
        passphrase,
        &buffer,
        &length_buffer);
    if ((rc != 0) || !buffer)
        return NULL;

    buffer_base16 = (char *)malloc ((length_buffer * 2) + 1);
    if (buffer_base16)
        string_base16_encode (buffer, length_buffer, buffer_base16);
    free (buffer);

    return buffer_base16;
}

TEST_GROUP(CoreSecure)
{
};

/*
 * Test functions:
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

    /* Test with key size == 64 (SHA512) */
    memset (key, 0, 64);
    LONGS_EQUAL(1, secure_derive_key (salt, passphrase, key, 64));
    MEMCMP_EQUAL(sha512_bin, key, 64);

    /* Test with key size == 32 (too small for SHA512) */
    memset (key, 0, 64);
    LONGS_EQUAL(1, secure_derive_key (salt, passphrase, key, 32));
    MEMCMP_EQUAL(sha512_bin, key, 32);
    MEMCMP_EQUAL(zeroes, key + 32, 32);

    free (key);
}

/*
 * Test functions:
 *   secure_encrypt_data
 *   secure_decrypt_data
 */

TEST(CoreSecure, EncryptDecryptData)
{
    const char *password = SECURE_PASSWORD;
    int i, j, hash_algo, cipher, rc;
    int length_password, length_encrypted_data, length_decrypted_data;
    char *encrypted_data, *decrypted_data;

    /* Compute length of password, including the final \0. */
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

            /* Initialize data. */
            encrypted_data = NULL;
            decrypted_data = NULL;
            length_encrypted_data = 0;
            length_decrypted_data = 0;

            /*
             * Encrypt the password with a hash algo, cipher and arbitrary
             * passphrase.
             */
            rc = secure_encrypt_data (password,
                                      length_password,
                                      hash_algo,
                                      cipher,
                                      SECURE_PASSPHRASE,
                                      &encrypted_data,
                                      &length_encrypted_data);
            LONGS_EQUAL(0, rc);

            /* Decrypt the encrypted password. */
            rc = secure_decrypt_data (encrypted_data,
                                      length_encrypted_data,
                                      hash_algo,
                                      cipher,
                                      SECURE_PASSPHRASE,
                                      &decrypted_data,
                                      &length_decrypted_data);
            LONGS_EQUAL(0, rc);

            /* Check decrypted data. */
            LONGS_EQUAL(length_password, length_decrypted_data);
            STRCMP_EQUAL(password, decrypted_data);

            /* Free encrypted/decrypted data. */
            free (encrypted_data);
            free (decrypted_data);
        }
    }
}

/*
 * Test functions:
 *   secure_decrypt_data_not_decrypted
 *   secure_decrypt_data_not_decrypted_map_cb
 */

TEST(CoreSecure, DecryptDataNotDecrypted)
{
    char *value1, *value2, *value3;

    hashtable_remove_all (secure_hashtable_data);
    hashtable_remove_all (secure_hashtable_data_encrypted);

    value1 = test_secure_encrypt_base16 (SECURE_PASSPHRASE, "value1");
    CHECK(value1);
    value2 = test_secure_encrypt_base16 (SECURE_PASSPHRASE, "value2");
    CHECK(value2);
    value3 = test_secure_encrypt_base16 ("other_passphrase", "value3");
    CHECK(value3);

    /* No passphrase. */
    LONGS_EQUAL(-1, secure_decrypt_data_not_decrypted (NULL));
    LONGS_EQUAL(-1, secure_decrypt_data_not_decrypted (""));

    /* No encrypted data. */
    LONGS_EQUAL(0, secure_decrypt_data_not_decrypted (SECURE_PASSPHRASE));

    /* Names with a comma, data encrypted with another passphrase. */
    hashtable_set (secure_hashtable_data_encrypted, "name1", value1);
    hashtable_set (secure_hashtable_data_encrypted, "name,2", value2);
    hashtable_set (secure_hashtable_data_encrypted, "name3", value3);
    hashtable_set (secure_hashtable_data_encrypted, "name4", "");

    LONGS_EQUAL(0, secure_decrypt_data_not_decrypted ("wrong"));
    LONGS_EQUAL(0, secure_hashtable_data->items_count);
    LONGS_EQUAL(4, secure_hashtable_data_encrypted->items_count);

    LONGS_EQUAL(2, secure_decrypt_data_not_decrypted (SECURE_PASSPHRASE));
    LONGS_EQUAL(2, secure_hashtable_data->items_count);
    STRCMP_EQUAL("value1",
                 (const char *)hashtable_get (secure_hashtable_data, "name1"));
    STRCMP_EQUAL("value2",
                 (const char *)hashtable_get (secure_hashtable_data, "name,2"));
    LONGS_EQUAL(2, secure_hashtable_data_encrypted->items_count);
    STRCMP_EQUAL(value3,
                 (const char *)hashtable_get (secure_hashtable_data_encrypted,
                                              "name3"));
    CHECK(hashtable_has_key (secure_hashtable_data_encrypted, "name4"));

    LONGS_EQUAL(1, secure_decrypt_data_not_decrypted ("other_passphrase"));
    STRCMP_EQUAL("value3",
                 (const char *)hashtable_get (secure_hashtable_data, "name3"));
    LONGS_EQUAL(1, secure_hashtable_data_encrypted->items_count);

    hashtable_remove_all (secure_hashtable_data);
    hashtable_remove_all (secure_hashtable_data_encrypted);

    free (value1);
    free (value2);
    free (value3);
}
