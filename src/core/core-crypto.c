/*
 * core-crypto.c - cryptographic functions
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>
#include <gcrypt.h>

#include "weechat.h"
#include "core-crypto.h"
#include "core-config-file.h"
#include "core-hashtable.h"
#include "core-string.h"
#include "../plugins/plugin.h"

char *weecrypto_hash_algo_string[] = {
    "crc32",
    "md5",
    "sha1",
    "sha224",
    "sha256",
    "sha384",
    "sha512",
#if GCRYPT_VERSION_NUMBER >= 0x010700
    "sha3-224",
    "sha3-256",
    "sha3-384",
    "sha3-512",
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010800
    "blake2b-160",
    "blake2b-256",
    "blake2b-384",
    "blake2b-512",
    "blake2s-128",
    "blake2s-160",
    "blake2s-224",
    "blake2s-256",
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010904
    "sha512-224",
    "sha512-256",
#endif
    NULL,
};
int weecrypto_hash_algo[] = {
    GCRY_MD_CRC32,
    GCRY_MD_MD5,
    GCRY_MD_SHA1,
    GCRY_MD_SHA224,
    GCRY_MD_SHA256,
    GCRY_MD_SHA384,
    GCRY_MD_SHA512,
#if GCRYPT_VERSION_NUMBER >= 0x010700
    GCRY_MD_SHA3_224,
    GCRY_MD_SHA3_256,
    GCRY_MD_SHA3_384,
    GCRY_MD_SHA3_512,
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010800
    GCRY_MD_BLAKE2B_160,
    GCRY_MD_BLAKE2B_256,
    GCRY_MD_BLAKE2B_384,
    GCRY_MD_BLAKE2B_512,
    GCRY_MD_BLAKE2S_128,
    GCRY_MD_BLAKE2S_160,
    GCRY_MD_BLAKE2S_224,
    GCRY_MD_BLAKE2S_256,
#endif
#if GCRYPT_VERSION_NUMBER >= 0x010904
    GCRY_MD_SHA512_224,
    GCRY_MD_SHA512_256,
#endif
};

char *weecrypto_cipher_string[] = {
    "aes128",
    "aes192",
    "aes256",
    NULL,
};
int weecrypto_cipher[] = {
    GCRY_CIPHER_AES128,
    GCRY_CIPHER_AES192,
    GCRY_CIPHER_AES256,
};


/*
 * Returns the hash algorithm with the name, or GCRY_MD_NONE if not found.
 */

int
weecrypto_get_hash_algo (const char *hash_algo)
{
    int i;

    if (!hash_algo)
        return GCRY_MD_NONE;

    for (i = 0; weecrypto_hash_algo_string[i]; i++)
    {
        if (strcmp (weecrypto_hash_algo_string[i], hash_algo) == 0)
            return weecrypto_hash_algo[i];
    }

    return GCRY_MD_NONE;
}

/*
 * Returns the cipher with the name, or GCRY_CIPHER_NONE if not found.
 */

int
weecrypto_get_cipher (const char *cipher)
{
    int i;

    if (!cipher)
        return GCRY_CIPHER_NONE;

    for (i = 0; weecrypto_cipher_string[i]; i++)
    {
        if (strcmp (weecrypto_cipher_string[i], cipher) == 0)
            return weecrypto_cipher[i];
    }

    return GCRY_CIPHER_NONE;
}

/*
 * Computes hash of data using the given hash algorithm.
 *
 * The hash size depends on the algorithm, common ones are:
 *
 *   GCRY_MD_CRC32         32 bits ==  4 bytes
 *   GCRY_MD_MD5          128 bits == 16 bytes
 *   GCRY_MD_SHA1         160 bits == 20 bytes
 *   GCRY_MD_SHA224       224 bits == 28 bytes
 *   GCRY_MD_SHA256       256 bits == 32 bytes
 *   GCRY_MD_SHA384       384 bits == 48 bytes
 *   GCRY_MD_SHA512       512 bits == 64 bytes
 *   GCRY_MD_SHA512_256   256 bits == 32 bytes (libgcrypt ≥ 1.9.4)
 *   GCRY_MD_SHA512_224   224 bits == 28 bytes (libgcrypt ≥ 1.9.4)
 *   GCRY_MD_SHA3_224     224 bits == 28 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_SHA3_256     256 bits == 32 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_SHA3_384     384 bits == 48 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_SHA3_512     512 bits == 64 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_BLAKE2B_160  160 bits == 20 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2B_256  256 bits == 32 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2B_384  384 bits == 48 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2B_512  512 bits == 64 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_128  128 bits == 16 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_160  160 bits == 20 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_224  224 bits == 28 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_256  256 bits == 32 bytes (libgcrypt ≥ 1.8.0)
 *
 * The result hash is stored in "hash" (the buffer must be large enough).
 *
 * If hash_size is not NULL, the length of hash is stored in *hash_size
 * (in bytes).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weecrypto_hash (const void *data, int data_size, int hash_algo,
                void *hash, int *hash_size)
{
    gcry_md_hd_t *hd_md;
    int rc, hd_md_opened, algo_size;
    unsigned char *ptr_hash;

    rc = 0;
    hd_md = NULL;
    hd_md_opened = 0;

    if (!hash)
        goto hash_end;

    if (hash_size)
        *hash_size = 0;

    if (!data || (data_size < 1))
        goto hash_end;

    hd_md = malloc (sizeof (gcry_md_hd_t));
    if (!hd_md)
        goto hash_end;

    if (gcry_md_open (hd_md, hash_algo, 0) != 0)
        goto hash_end;

    hd_md_opened = 1;

    gcry_md_write (*hd_md, data, data_size);

    ptr_hash = gcry_md_read (*hd_md, hash_algo);
    if (!ptr_hash)
        goto hash_end;

    algo_size = gcry_md_get_algo_dlen (hash_algo);
    memcpy (hash, ptr_hash, algo_size);
    if (hash_size)
        *hash_size = algo_size;

    rc = 1;

hash_end:
    if (hd_md)
    {
        if (hd_md_opened)
            gcry_md_close (*hd_md);
        free (hd_md);
    }
    return rc;
}

/*
 * Computes hash of file using the given hash algorithm.
 *
 * The hash size depends on the algorithm, common ones are:
 *
 *   GCRY_MD_CRC32         32 bits ==  4 bytes
 *   GCRY_MD_MD5          128 bits == 16 bytes
 *   GCRY_MD_SHA1         160 bits == 20 bytes
 *   GCRY_MD_SHA224       224 bits == 28 bytes
 *   GCRY_MD_SHA256       256 bits == 32 bytes
 *   GCRY_MD_SHA384       384 bits == 48 bytes
 *   GCRY_MD_SHA512       512 bits == 64 bytes
 *   GCRY_MD_SHA512_224   224 bits == 28 bytes (libgcrypt ≥ 1.9.4)
 *   GCRY_MD_SHA512_256   256 bits == 32 bytes (libgcrypt ≥ 1.9.4)
 *   GCRY_MD_SHA3_224     224 bits == 28 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_SHA3_256     256 bits == 32 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_SHA3_384     384 bits == 48 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_SHA3_512     512 bits == 64 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_BLAKE2B_160  160 bits == 20 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2B_256  256 bits == 32 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2B_384  384 bits == 48 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2B_512  512 bits == 64 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_128  128 bits == 16 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_160  160 bits == 20 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_224  224 bits == 28 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_256  256 bits == 32 bytes (libgcrypt ≥ 1.8.0)
 *
 * The result hash is stored in "hash" (the buffer must be large enough).
 *
 * If hash_size is not NULL, the length of hash is stored in *hash_size
 * (in bytes).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weecrypto_hash_file (const char *filename, int hash_algo,
                     void *hash, int *hash_size)
{
    gcry_md_hd_t *hd_md;
    FILE *file;
    size_t num_read;
    int rc, hd_md_opened, algo_size;
    unsigned char *ptr_hash;
    char buffer[4096];

    rc = 0;
    hd_md = NULL;
    hd_md_opened = 0;
    file = NULL;

    if (!hash)
        goto hash_end;

    if (hash_size)
        *hash_size = 0;

    if (!filename || !filename[0] || !hash)
        goto hash_end;

    file = fopen (filename, "r");
    if (!file)
        goto hash_end;

    hd_md = malloc (sizeof (gcry_md_hd_t));
    if (!hd_md)
        goto hash_end;

    if (gcry_md_open (hd_md, hash_algo, 0) != 0)
        goto hash_end;

    hd_md_opened = 1;

    while (!feof (file))
    {
        num_read = fread (buffer, 1, sizeof (buffer), file);
        if (num_read == 0)
            break;
        gcry_md_write (*hd_md, buffer, num_read);
    }

    ptr_hash = gcry_md_read (*hd_md, hash_algo);
    if (!ptr_hash)
        goto hash_end;

    algo_size = gcry_md_get_algo_dlen (hash_algo);
    memcpy (hash, ptr_hash, algo_size);
    if (hash_size)
        *hash_size = algo_size;

    rc = 1;

hash_end:
    if (hd_md)
    {
        if (hd_md_opened)
            gcry_md_close (*hd_md);
        free (hd_md);
    }
    if (file)
        fclose (file);
    return rc;
}
/*
 * Computes PKCS#5 Passphrase Based Key Derivation Function number 2 (PBKDF2)
 * hash of data.
 *
 * The hash size depends on the algorithm, common ones are:
 *
 *   GCRY_MD_SHA1    160 bits == 20 bytes
 *   GCRY_MD_SHA256  256 bits == 32 bytes
 *   GCRY_MD_SHA512  512 bits == 64 bytes
 *
 * The result hash is stored in "hash" (the buffer must be large enough).
 *
 * If hash_size is not NULL, the length of hash is stored in *hash_size
 * (in bytes).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weecrypto_hash_pbkdf2 (const void *data, int data_size, int hash_algo,
                       const void *salt, int salt_size, int iterations,
                       void *hash, int *hash_size)
{
    int rc, algo_size;

    rc = 0;

    if (!hash)
        goto hash_pbkdf2_end;

    if (hash_size)
        *hash_size = 0;

    if (!data || (data_size < 1) || !salt || (salt_size < 1)
        || (iterations < 1))
    {
        goto hash_pbkdf2_end;
    }

    algo_size = gcry_md_get_algo_dlen (hash_algo);
    if (gcry_kdf_derive (data, data_size, GCRY_KDF_PBKDF2, hash_algo,
                         salt, salt_size, iterations,
                         algo_size, hash) != 0)
    {
        goto hash_pbkdf2_end;
    }

    if (hash_size)
        *hash_size = algo_size;

    rc = 1;

hash_pbkdf2_end:
    return rc;
}

/*
 * Computes keyed-hash message authentication code (HMAC).
 *
 * The hash size depends on the algorithm, common ones are:
 *
 *   GCRY_MD_CRC32         32 bits ==  4 bytes
 *   GCRY_MD_MD5          128 bits == 16 bytes
 *   GCRY_MD_SHA1         160 bits == 20 bytes
 *   GCRY_MD_SHA224       224 bits == 28 bytes
 *   GCRY_MD_SHA256       256 bits == 32 bytes
 *   GCRY_MD_SHA384       384 bits == 48 bytes
 *   GCRY_MD_SHA512       512 bits == 64 bytes
 *   GCRY_MD_SHA512_224   224 bits == 28 bytes (libgcrypt ≥ 1.9.4)
 *   GCRY_MD_SHA512_256   256 bits == 32 bytes (libgcrypt ≥ 1.9.4)
 *   GCRY_MD_SHA3_224     224 bits == 28 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_SHA3_256     256 bits == 32 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_SHA3_384     384 bits == 48 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_SHA3_512     512 bits == 64 bytes (libgcrypt ≥ 1.7.0)
 *   GCRY_MD_BLAKE2B_160  160 bits == 20 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2B_256  256 bits == 32 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2B_384  384 bits == 48 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2B_512  512 bits == 64 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_128  128 bits == 16 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_160  160 bits == 20 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_224  224 bits == 28 bytes (libgcrypt ≥ 1.8.0)
 *   GCRY_MD_BLAKE2S_256  256 bits == 32 bytes (libgcrypt ≥ 1.8.0)
 *
 * The result hash is stored in "hash" (the buffer must be large enough).
 *
 * If hash_size is not NULL, the length of hash is stored in *hash_size
 * (in bytes).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weecrypto_hmac (const void *key, int key_size,
                const void *message, int message_size,
                int hash_algo,
                void *hash, int *hash_size)
{
    gcry_md_hd_t *hd_md;
    int rc, hd_md_opened, algo_size;
    unsigned char *ptr_hash;

    rc = 0;
    hd_md = NULL;
    hd_md_opened = 0;

    if (!hash)
        goto hmac_end;

    if (hash_size)
        *hash_size = 0;

    if (!key || (key_size < 1) || !message || (message_size < 1))
        goto hmac_end;

    hd_md = malloc (sizeof (gcry_md_hd_t));
    if (!hd_md)
        goto hmac_end;

    if (gcry_md_open (hd_md, hash_algo, GCRY_MD_FLAG_HMAC) != 0)
        goto hmac_end;

    hd_md_opened = 1;

    if (gcry_md_setkey (*hd_md, key, key_size) != 0)
        goto hmac_end;

    gcry_md_write (*hd_md, message, message_size);

    ptr_hash = gcry_md_read (*hd_md, hash_algo);
    if (!ptr_hash)
        goto hmac_end;

    algo_size = gcry_md_get_algo_dlen (hash_algo);
    memcpy (hash, ptr_hash, algo_size);
    if (hash_size)
        *hash_size = algo_size;

    rc = 1;

hmac_end:
    if (hd_md)
    {
        if (hd_md_opened)
            gcry_md_close (*hd_md);
        free (hd_md);
    }
    return rc;
}

/*
 * Generates a Time-based One-Time Password (TOTP), as described
 * in the RFC 6238.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weecrypto_totp_generate_internal (const char *secret, int length_secret,
                                  uint64_t moving_factor, int digits,
                                  char *result)
{
    uint64_t moving_factor_swapped;
    char hash[20];
    int rc, offset, length;
    unsigned long bin_code;

#if __BYTE_ORDER == __BIG_ENDIAN
    /* Big endian does not need to swap bytes here! */
    moving_factor_swapped = moving_factor;
#else
    moving_factor_swapped = (moving_factor >> 56)
        | ((moving_factor << 40) & 0x00FF000000000000)
        | ((moving_factor << 24) & 0x0000FF0000000000)
        | ((moving_factor << 8) & 0x000000FF00000000)
        | ((moving_factor >> 8) & 0x00000000FF000000)
        | ((moving_factor >> 24) & 0x0000000000FF0000)
        | ((moving_factor >> 40) & 0x000000000000FF00)
        | (moving_factor << 56);
#endif

    rc = weecrypto_hmac (secret, length_secret,
                         &moving_factor_swapped, sizeof (moving_factor_swapped),
                         GCRY_MD_SHA1,
                         hash, NULL);
    if (!rc)
        return 0;

    offset = hash[19] & 0xf;
    bin_code = (hash[offset] & 0x7f) << 24
        | (hash[offset+1] & 0xff) << 16
        | (hash[offset+2] & 0xff) <<  8
        | (hash[offset+3] & 0xff);

    bin_code %= (unsigned long)(pow (10, digits));

    length = snprintf (result, digits + 1, "%.*lu", digits, bin_code);
    if (length != digits)
        return 0;

    return 1;
}

/*
 * Generates a Time-based One-Time Password (TOTP), as described
 * in the RFC 6238.
 *
 * Returns the password as string, NULL if error.
 *
 * Note: result must be freed after use.
 */

char *
weecrypto_totp_generate (const char *secret_base32, time_t totp_time,
                         int digits)
{
    char *result, *secret;
    int length_secret, rc;
    uint64_t moving_factor;

    secret = NULL;
    result = NULL;

    if (!secret_base32 || !secret_base32[0]
        || (digits < WEECRYPTO_TOTP_MIN_DIGITS)
        || (digits > WEECRYPTO_TOTP_MAX_DIGITS))
    {
        goto error;
    }

    secret = malloc ((strlen (secret_base32) * 4) + 16 + 1);
    if (!secret)
        goto error;

    length_secret = string_base32_decode (secret_base32, secret);
    if (length_secret < 0)
        goto error;

    result = malloc (digits + 1);
    if (!result)
        goto error;

    if (totp_time == 0)
        totp_time = time (NULL);

    moving_factor = totp_time / 30;

    rc = weecrypto_totp_generate_internal (secret, length_secret,
                                           moving_factor, digits, result);
    if (!rc)
        goto error;

    free (secret);

    return result;

error:
    free (secret);
    free (result);
    return NULL;
}

/*
 * Validates a Time-based One-Time Password (TOTP).
 *
 * Returns:
 *   1: OTP is OK
 *   0: OTP is invalid
 */

int
weecrypto_totp_validate (const char *secret_base32, time_t totp_time,
                         int window, const char *otp)
{
    char *secret, str_otp[16];
    int length_secret, digits, rc, otp_ok;
    uint64_t i, moving_factor;

    secret = NULL;

    if (!secret_base32 || !secret_base32[0] || (window < 0) || !otp || !otp[0])
        goto error;

    digits = strlen (otp);
    if ((digits < WEECRYPTO_TOTP_MIN_DIGITS)
        || (digits > WEECRYPTO_TOTP_MAX_DIGITS))
    {
        goto error;
    }

    secret = malloc (strlen (secret_base32) + 1);
    if (!secret)
        goto error;

    length_secret = string_base32_decode (secret_base32, secret);
    if (length_secret < 0)
        goto error;

    if (totp_time == 0)
        totp_time = time (NULL);

    moving_factor = totp_time / 30;

    otp_ok = 0;

    for (i = moving_factor - window; i <= moving_factor + window; i++)
    {
        rc = weecrypto_totp_generate_internal (secret, length_secret,
                                               i, digits, str_otp);
        if (rc && (strcmp (str_otp, otp) == 0))
        {
            otp_ok = 1;
            break;
        }
    }

    free (secret);

    return otp_ok;

error:
    free (secret);
    return 0;
}
