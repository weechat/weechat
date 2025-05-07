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

/* Fuzz testing on WeeChat core crypto functions */

extern "C"
{
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <gcrypt.h>

#include "src/core/core-crypto.h"

extern char *weecrypto_hash_algo_string[];
extern int weecrypto_hash_algo[];
}

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
    char *str, hash[1024], *result;
    int i, hash_size, key_size, salt_size;

    str = (char *)malloc (size + 1);
    memcpy (str, data, size);
    str[size] = '\0';

    key_size = (size > 8) ? 8 : size;
    for (i = 0; weecrypto_hash_algo_string[i]; i++)
    {
        weecrypto_hash (data, size, weecrypto_hash_algo[i], hash, &hash_size);
        weecrypto_hmac (data, key_size, data, size, weecrypto_hash_algo[i], hash, &hash_size);
    }

    salt_size = (size > 8) ? 8 : size;
    weecrypto_hash_pbkdf2 (data, size, GCRY_MD_SHA1, data, salt_size, 100, hash, &hash_size);
    weecrypto_hash_pbkdf2 (data, size, GCRY_MD_SHA256, data, salt_size, 100, hash, &hash_size);
    weecrypto_hash_pbkdf2 (data, size, GCRY_MD_SHA512, data, salt_size, 100, hash, &hash_size);

    result = weecrypto_totp_generate (str, 1746358623, 6);
    if (result && result[0])
        assert (weecrypto_totp_validate (str, 1746358623, 0, result));
    free (result);
    result = weecrypto_totp_generate (str, 1746358623, 12);
    if (result && result[0])
        assert (weecrypto_totp_validate (str, 1746358623, 0, result));
    free (result);

    free (str);

    return 0;
}
