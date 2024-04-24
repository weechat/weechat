/*
 * core-secure.c - secured data
 *
 * Copyright (C) 2013-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <gcrypt.h>

#include "weechat.h"
#include "core-config-file.h"
#include "core-crypto.h"
#include "core-hashtable.h"
#include "core-hook.h"
#include "core-secure.h"
#include "core-secure-config.h"
#include "core-string.h"
#include "../plugins/plugin.h"

/* the passphrase used to encrypt/decrypt data */
char *secure_passphrase = NULL;

/* decrypted data */
struct t_hashtable *secure_hashtable_data = NULL;

/* data still encrypted (if passphrase not set) */
struct t_hashtable *secure_hashtable_data_encrypted = NULL;

char *secure_decrypt_error[] = { "memory", "buffer", "key", "cipher", "setkey",
                                 "decrypt", "hash", "hash mismatch" };

/* used only when reading sec.conf: 1 if flag __passphrase__ is enabled */
int secure_data_encrypted = 0;


/*
 * Derives a key from salt + passphrase (using a hash).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
secure_derive_key (const char *salt, const char *passphrase,
                   unsigned char *key, int length_key)
{
    char *buffer, hash[512 / 8];
    int length, length_hash;

    if (!salt || !passphrase || !key || (length_key < 1))
        return 0;

    memset (key, 0, length_key);

    length = SECURE_SALT_SIZE + strlen (passphrase);
    buffer = malloc (length);
    if (!buffer)
        return 0;

    /* build a buffer with salt + passphrase */
    memcpy (buffer, salt, SECURE_SALT_SIZE);
    memcpy (buffer + SECURE_SALT_SIZE, passphrase, strlen (passphrase));

    /* compute hash of buffer */
    if (!weecrypto_hash (buffer, length, GCRY_MD_SHA512, hash, &length_hash))
    {
        free (buffer);
        return 0;
    }

    /* copy beginning of hash (or full hash) in the key */
    memcpy (key, hash,
            (length_hash > length_key) ? length_key : length_hash);

    free (buffer);

    return 1;
}

/*
 * Encrypts data using a hash algorithm + cipher + passphrase.
 *
 * Following actions are performed:
 *   1. derive a key from the passphrase (with optional salt)
 *   2. compute hash of data
 *   3. store hash + data in a buffer
 *   4. encrypt the buffer (hash + data), using the key
 *   5. return salt + encrypted hash/data
 *
 * Output buffer has following content:
 * - salt (8 bytes, used to derive a key from the passphrase)
 * - encrypted hash(data) + data
 *
 * So it looks like:
 *
 * +----------+------------+------------------------------+
 * |   salt   |    hash    |             data             |
 * +----------+------------+------------------------------+
 * \_ _ _ _ _/\_ _ _ _ _ _ /\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _/
 *   8 bytes     N bytes         variable length
 *            \_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _/
 *                          encrypted data
 *
 * Returns:
 *     0: OK
 *    -1: not enough memory
 *    -2: key derive error
 *    -3: compute hash error
 *    -4: cipher open error
 *    -5: setkey error
 *    -6: encrypt error
 */

int
secure_encrypt_data (const char *data, int length_data,
                     int hash_algo, int cipher, const char *passphrase,
                     char **encrypted, int *length_encrypted)
{
    int rc, length_salt, length_hash, length_hash_data, length_key;
    int hd_md_opened, hd_cipher_opened;
    gcry_md_hd_t *hd_md;
    gcry_cipher_hd_t *hd_cipher;
    char salt[SECURE_SALT_SIZE];
    unsigned char *ptr_hash, *key, *hash_and_data;

    rc = -1;

    hd_md = NULL;
    hd_md_opened = 0;
    hd_cipher = NULL;
    hd_cipher_opened = 0;
    key = NULL;
    hash_and_data = NULL;

    hd_md = malloc (sizeof (gcry_md_hd_t));
    if (!hd_md)
        return -1;
    hd_cipher = malloc (sizeof (gcry_cipher_hd_t));
    if (!hd_cipher)
    {
        free (hd_md);
        return -1;
    }

    /* derive a key from the passphrase */
    length_key = gcry_cipher_get_algo_keylen (cipher);
    key = malloc (length_key);
    if (!key)
        goto encrypt_end;
    if (CONFIG_BOOLEAN(secure_config_crypt_salt))
        gcry_randomize (salt, SECURE_SALT_SIZE, GCRY_STRONG_RANDOM);
    else
    {
        length_salt = strlen (SECURE_SALT_DEFAULT);
        if (length_salt < SECURE_SALT_SIZE)
            memset (salt, 0, SECURE_SALT_SIZE);
        memcpy (salt, SECURE_SALT_DEFAULT,
                (length_salt <= SECURE_SALT_SIZE) ?
                length_salt : SECURE_SALT_SIZE);
    }
    if (!secure_derive_key (salt, passphrase, key, length_key))
    {
        rc = -2;
        goto encrypt_end;
    }

    /* compute hash of data */
    if (gcry_md_open (hd_md, hash_algo, 0) != 0)
    {
        rc = -3;
        goto encrypt_end;
    }
    hd_md_opened = 1;
    length_hash = gcry_md_get_algo_dlen (hash_algo);
    gcry_md_write (*hd_md, data, length_data);
    ptr_hash = gcry_md_read (*hd_md, hash_algo);
    if (!ptr_hash)
    {
        rc = -3;
        goto encrypt_end;
    }

    /* build a buffer with hash + data */
    length_hash_data = length_hash + length_data;
    hash_and_data = malloc (length_hash_data);
    if (!hash_and_data)
        goto encrypt_end;
    memcpy (hash_and_data, ptr_hash, length_hash);
    memcpy (hash_and_data + length_hash, data, length_data);

    /* encrypt hash + data */
    if (gcry_cipher_open (hd_cipher, cipher, GCRY_CIPHER_MODE_CFB, 0) != 0)
    {
        rc = -4;
        goto encrypt_end;
    }
    hd_cipher_opened = 1;
    if (gcry_cipher_setkey (*hd_cipher, key, length_key) != 0)
    {
        rc = -5;
        goto encrypt_end;
    }
    if (gcry_cipher_encrypt (*hd_cipher, hash_and_data, length_hash_data,
                             NULL, 0) != 0)
    {
        rc = -6;
        goto encrypt_end;
    }

    /* create buffer and copy salt + encrypted hash/data into this buffer*/
    *length_encrypted = SECURE_SALT_SIZE + length_hash_data;
    *encrypted = malloc (*length_encrypted);
    if (!*encrypted)
        goto encrypt_end;
    memcpy (*encrypted, salt, SECURE_SALT_SIZE);
    memcpy (*encrypted + SECURE_SALT_SIZE, hash_and_data, length_hash_data);

    rc = 0;

encrypt_end:
    if (hd_md)
    {
        if (hd_md_opened)
            gcry_md_close (*hd_md);
        free (hd_md);
    }
    if (hd_cipher)
    {
        if (hd_cipher_opened)
            gcry_cipher_close (*hd_cipher);
        free (hd_cipher);
    }
    free (key);
    free (hash_and_data);

    return rc;
}

/*
 * Decrypts data using a hash algorithm + cipher + passphrase.
 *
 * The buffer must contain:
 * - salt (8 bytes, used to derive a key from the passphrase)
 * - encrypted hash(data) + data
 *
 * Following actions are performed:
 *   1. check length of buffer (it must have at least salt + hash + some data)
 *   2. derive a key from the passphrase using salt (at beginning of buffer)
 *   3. decrypt hash + data in a buffer
 *   4. compute hash of decrypted data
 *   5. check that decrypted hash is equal to hash of data
 *   6. return decrypted data
 *
 * Returns:
 *    0: OK
 *   -1: not enough memory
 *   -2: buffer is not long enough
 *   -3: key derive error
 *   -4: cipher open error
 *   -5: setkey error
 *   -6: decrypt error
 *   -7: compute hash error
 *   -8: hash does not match the decrypted data
 *
 * Note: when adding a return code, change the array "secure_decrypt_error"
 * accordingly.
 */

int
secure_decrypt_data (const char *buffer, int length_buffer,
                     int hash_algo, int cipher, const char *passphrase,
                     char **decrypted, int *length_decrypted)
{
    int rc, length_hash, length_key, hd_md_opened, hd_cipher_opened;
    gcry_md_hd_t *hd_md;
    gcry_cipher_hd_t *hd_cipher;
    unsigned char *ptr_hash, *key, *decrypted_hash_data;

    rc = -1;

    /* check length of buffer */
    length_hash = gcry_md_get_algo_dlen (hash_algo);
    if (length_buffer <= SECURE_SALT_SIZE + length_hash)
        return -2;

    hd_md = NULL;
    hd_md_opened = 0;
    hd_cipher = NULL;
    hd_cipher_opened = 0;
    key = NULL;
    decrypted_hash_data = NULL;

    hd_md = malloc (sizeof (gcry_md_hd_t));
    if (!hd_md)
        return rc;
    hd_cipher = malloc (sizeof (gcry_cipher_hd_t));
    if (!hd_cipher)
    {
        free (hd_md);
        return rc;
    }

    /* derive a key from the passphrase */
    length_key = gcry_cipher_get_algo_keylen (cipher);
    key = malloc (length_key);
    if (!key)
        goto decrypt_end;
    if (!secure_derive_key (buffer, passphrase, key, length_key))
    {
        rc = -3;
        goto decrypt_end;
    }

    /* decrypt hash + data */
    decrypted_hash_data = malloc (length_buffer - SECURE_SALT_SIZE);
    if (!decrypted_hash_data)
        goto decrypt_end;
    if (gcry_cipher_open (hd_cipher, cipher, GCRY_CIPHER_MODE_CFB, 0) != 0)
    {
        rc = -4;
        goto decrypt_end;
    }
    hd_cipher_opened = 1;
    if (gcry_cipher_setkey (*hd_cipher, key, length_key) != 0)
    {
        rc = -5;
        goto decrypt_end;
    }
    if (gcry_cipher_decrypt (*hd_cipher,
                             decrypted_hash_data,
                             length_buffer - SECURE_SALT_SIZE,
                             buffer + SECURE_SALT_SIZE,
                             length_buffer - SECURE_SALT_SIZE) != 0)
    {
        rc = -6;
        goto decrypt_end;
    }

    /* check if hash is OK for decrypted data */
    if (gcry_md_open (hd_md, hash_algo, 0) != 0)
    {
        rc = -7;
        goto decrypt_end;
    }
    hd_md_opened = 1;
    gcry_md_write (*hd_md, decrypted_hash_data + length_hash,
                   length_buffer - SECURE_SALT_SIZE - length_hash);
    ptr_hash = gcry_md_read (*hd_md, hash_algo);
    if (!ptr_hash)
    {
        rc = -7;
        goto decrypt_end;
    }
    if (memcmp (ptr_hash, decrypted_hash_data, length_hash) != 0)
    {
        rc = -8;
        goto decrypt_end;
    }

    /* return the decrypted data */
    *length_decrypted = length_buffer - SECURE_SALT_SIZE - length_hash;
    *decrypted = malloc (*length_decrypted);
    if (!*decrypted)
        goto decrypt_end;

    memcpy (*decrypted, decrypted_hash_data + length_hash, *length_decrypted);

    rc = 0;

decrypt_end:
    if (hd_md)
    {
        if (hd_md_opened)
            gcry_md_close (*hd_md);
        free (hd_md);
    }
    if (hd_cipher)
    {
        if (hd_cipher_opened)
            gcry_cipher_close (*hd_cipher);
        free (hd_cipher);
    }
    free (key);
    free (decrypted_hash_data);

    return rc;
}

/*
 * Decrypts data still encrypted (data that could not be decrypted when reading
 * secured data configuration file (because no passphrase was given).
 *
 * Returns:
 *   >= 0: number of decrypted data
 *     -1: error decrypting data (bad passphrase)
 *     -2: unsupported hash algorithm
 *     -3: unsupported cipher
 */

int
secure_decrypt_data_not_decrypted (const char *passphrase)
{
    char **keys, *buffer, *decrypted;
    const char *value;
    int num_ok, num_keys, i, hash_algo, cipher, rc;
    int length_buffer, length_decrypted;

    /* we need a passphrase to decrypt data! */
    if (!passphrase || !passphrase[0])
        return -1;

    hash_algo = weecrypto_get_hash_algo (
        config_file_option_string (secure_config_crypt_hash_algo));
    if (hash_algo == GCRY_MD_NONE)
        return -2;

    cipher = weecrypto_get_cipher (
        config_file_option_string (secure_config_crypt_cipher));
    if (cipher == GCRY_CIPHER_NONE)
        return -3;

    num_ok = 0;

    keys = string_split (hashtable_get_string (secure_hashtable_data_encrypted,
                                               "keys"),
                         ",",
                         NULL,
                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                         0,
                         &num_keys);
    if (keys)
    {
        for (i = 0; i < num_keys; i++)
        {
            value = hashtable_get (secure_hashtable_data_encrypted, keys[i]);
            if (value && value[0])
            {
                buffer = malloc (strlen (value) + 1);
                if (buffer)
                {
                    length_buffer = string_base16_decode (value, buffer);
                    decrypted = NULL;
                    length_decrypted = 0;
                    rc = secure_decrypt_data (
                        buffer,
                        length_buffer,
                        hash_algo,
                        cipher,
                        passphrase,
                        &decrypted,
                        &length_decrypted);
                    if ((rc == 0) && decrypted)
                    {
                        hashtable_set (secure_hashtable_data, keys[i],
                                       decrypted);
                        hashtable_remove (secure_hashtable_data_encrypted,
                                          keys[i]);
                        num_ok++;
                    }
                    free (decrypted);
                    free (buffer);
                }
            }
        }
        string_free_split (keys);
    }

    return num_ok;
}

/*
 * Initializes secured data.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
secure_init ()
{
    char *ptr_phrase;

    /* try to read passphrase (if not set) from env var "WEECHAT_PASSPHRASE" */
    if (!secure_passphrase)
    {
        ptr_phrase = getenv (SECURE_ENV_PASSPHRASE);
        if (ptr_phrase)
        {
            if (ptr_phrase[0])
                secure_passphrase = strdup (ptr_phrase);
            unsetenv (SECURE_ENV_PASSPHRASE);
        }
    }

    secure_hashtable_data = hashtable_new (32,
                                           WEECHAT_HASHTABLE_STRING,
                                           WEECHAT_HASHTABLE_STRING,
                                           NULL, NULL);
    if (!secure_hashtable_data)
        return 0;

    secure_hashtable_data_encrypted = hashtable_new (32,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     NULL, NULL);
    if (!secure_hashtable_data_encrypted)
    {
        hashtable_free (secure_hashtable_data);
        return 0;
    }

    return 1;
}

/*
 * Frees all allocated data.
 */

void
secure_end ()
{
    if (secure_passphrase)
    {
        free (secure_passphrase);
        secure_passphrase = NULL;
    }
    if (secure_hashtable_data)
    {
        hashtable_free (secure_hashtable_data);
        secure_hashtable_data = NULL;
    }
    if (secure_hashtable_data_encrypted)
    {
        hashtable_free (secure_hashtable_data_encrypted);
        secure_hashtable_data_encrypted = NULL;
    }
}
