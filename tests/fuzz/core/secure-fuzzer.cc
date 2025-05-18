/*
 * SPDX-FileCopyrightText: 2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* Fuzz testing on WeeChat core secured data functions */

extern "C"
{
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <gcrypt.h>

#include "src/core/core-config.h"
#include "src/core/core-secure.h"
#include "src/core/core-secure-config.h"
#include "src/core/core-string.h"

extern int secure_derive_key (const char *salt, const char *passphrase,
                              unsigned char *key, int length_key);
}

extern "C" int
LLVMFuzzerInitialize (int *argc, char ***argv)
{
    /* make C++ compiler happy */
    (void) argc;
    (void) argv;

    string_init ();
    secure_init ();
    secure_config_init ();
    config_weechat_init ();

    return 0;
}

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
    char *str, hash[1024], *encrypted, *decrypted;
    int length_encrypted, length_decrypted;

    /* ignore empty or huge data */
    if ((size == 0) || (size > 65536))
        return 0;

    str = (char *)malloc (size + 1);
    memcpy (str, data, size);
    str[size] = '\0';

    if (size >= 8)
        secure_derive_key (str, str, (unsigned char *)hash, sizeof (hash));

    encrypted = NULL;
    decrypted = NULL;
    config_file_option_set (secure_config_crypt_salt, "on", 1);
    secure_encrypt_data (str, size, GCRY_MD_SHA512, GCRY_CIPHER_AES256, "test", &encrypted, &length_encrypted);
    secure_decrypt_data (encrypted, length_encrypted, GCRY_MD_SHA512, GCRY_CIPHER_AES256, "test", &decrypted, &length_decrypted);
    assert ((size_t)length_decrypted == size);
    assert (memcmp (decrypted, str, length_decrypted) == 0);
    free (encrypted);
    free (decrypted);
    config_file_option_set (secure_config_crypt_salt, "off", 1);
    encrypted = NULL;
    secure_encrypt_data (str, size, GCRY_MD_SHA512, GCRY_CIPHER_AES256, "test", &encrypted, &length_encrypted);
    free (encrypted);
    config_file_option_reset (secure_config_crypt_salt, 1);

    free (str);

    return 0;
}
