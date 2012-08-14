/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * irc-sasl.c: SASL authentication with IRC server,
 *             using mechanisms PLAIN or DH-BLOWFISH
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <gcrypt.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-sasl.h"


char *irc_sasl_mechanism_string[IRC_NUM_SASL_MECHANISMS] =
{ "plain", "dh-blowfish", "external" };


/*
 * irc_sasl_mechanism_plain: build answer for SASL authentication, using
 *                           mechanism "PLAIN"
 *                           Note: result must be freed after use
 */

char *
irc_sasl_mechanism_plain (const char *sasl_username, const char *sasl_password)
{
    char *string, *answer_base64;
    int length_username, length;

    answer_base64 = NULL;
    length_username = strlen (sasl_username);
    length = ((length_username + 1) * 2) + strlen (sasl_password) + 1;
    string = malloc (length);
    if (string)
    {
        snprintf (string, length, "%s|%s|%s",
                  sasl_username, sasl_username, sasl_password);
        string[length_username] = '\0';
        string[(length_username * 2) + 1] = '\0';

        answer_base64 = malloc (length * 4);
        if (answer_base64)
            weechat_string_encode_base64 (string, length - 1, answer_base64);

        free (string);
    }

    return answer_base64;
}

/*
 * irc_sasl_mechanism_dh_blowfish: build answer for SASL authentication, using
 *                                 mechanism "DH-BLOWFISH"
 *                                 Note: result must be freed after use
 *
 *                                 data_base64 is a concatenation of 3 strings,
 *                                 each string is composed of 2 bytes (length
 *                                 of string), followed by content of string:
 *                                   1. a prime number
 *                                   2. a generator number
 *                                   3. server-generated public key
 */

char *
irc_sasl_mechanism_dh_blowfish (const char *data_base64,
                                const char *sasl_username,
                                const char *sasl_password)
{
    char *data, *answer, *ptr_answer, *answer_base64;
    unsigned char *ptr_data, *secret_bin, *public_bin;
    unsigned char *password_clear, *password_crypted;
    int length_data, size, num_bits_prime_number, length_key;
    int length_username, length_password, length_answer;
    size_t num_written;
    gcry_mpi_t data_prime_number, data_generator_number, data_server_pub_key;
    gcry_mpi_t pub_key, priv_key, secret_mpi;
    gcry_cipher_hd_t gcrypt_handle;

    data = NULL;
    secret_bin = NULL;
    public_bin = NULL;
    password_clear = NULL;
    password_crypted = NULL;
    answer = NULL;
    answer_base64 = NULL;
    data_prime_number = NULL;
    data_generator_number = NULL;
    data_server_pub_key = NULL;
    pub_key = NULL;
    priv_key = NULL;
    secret_mpi = NULL;

    /* decode data */
    data = malloc (strlen (data_base64) + 1);
    length_data = weechat_string_decode_base64 (data_base64, data);
    ptr_data = (unsigned char *)data;

    /* extract prime number */
    size = ntohs ((((unsigned int)ptr_data[1]) << 8) | ptr_data[0]);
    ptr_data += 2;
    length_data -= 2;
    if (size > length_data)
        goto end;
    data_prime_number = gcry_mpi_new (size * 8);
    gcry_mpi_scan (&data_prime_number, GCRYMPI_FMT_USG, ptr_data, size, NULL);
    num_bits_prime_number = gcry_mpi_get_nbits (data_prime_number);
    ptr_data += size;
    length_data -= size;

    /* extract generator number */
    size = ntohs ((((unsigned int)ptr_data[1]) << 8) | ptr_data[0]);
    ptr_data += 2;
    length_data -= 2;
    if (size > length_data)
        goto end;
    data_generator_number = gcry_mpi_new (size * 8);
    gcry_mpi_scan (&data_generator_number, GCRYMPI_FMT_USG, ptr_data, size, NULL);
    ptr_data += size;
    length_data -= size;

    /* extract server-generated public key */
    size = ntohs ((((unsigned int)ptr_data[1]) << 8) | ptr_data[0]);
    ptr_data += 2;
    length_data -= 2;
    if (size > length_data)
        goto end;
    data_server_pub_key = gcry_mpi_new (size * 8);
    gcry_mpi_scan (&data_server_pub_key, GCRYMPI_FMT_USG, ptr_data, size, NULL);

    /* generate keys */
    pub_key = gcry_mpi_new (num_bits_prime_number);
    priv_key = gcry_mpi_new (num_bits_prime_number);
    gcry_mpi_randomize (priv_key, num_bits_prime_number, GCRY_STRONG_RANDOM);
    /* pub_key = (g ^ priv_key) % p */
    gcry_mpi_powm (pub_key, data_generator_number, priv_key, data_prime_number);

    /* compute secret_bin */
    length_key = num_bits_prime_number / 8;
    secret_bin = malloc (length_key);
    secret_mpi = gcry_mpi_new (num_bits_prime_number);
    /* secret_mpi = (y ^ priv_key) % p */
    gcry_mpi_powm (secret_mpi, data_server_pub_key, priv_key, data_prime_number);
    gcry_mpi_print (GCRYMPI_FMT_USG, secret_bin, length_key,
                    &num_written, secret_mpi);

    /* create public_bin */
    public_bin = malloc (length_key);
    gcry_mpi_print (GCRYMPI_FMT_USG, public_bin, length_key,
                    &num_written, pub_key);

    /* create password buffers (clear and crypted) */
    length_password = strlen (sasl_password) +
        ((8 - (strlen (sasl_password) % 8)) % 8);
    password_clear = malloc (length_password);
    password_crypted = malloc (length_password);
    memset (password_clear, 0, length_password);
    memset (password_crypted, 0, length_password);
    memcpy (password_clear, sasl_password, strlen (sasl_password));

    /* crypt password using blowfish */
    if (gcry_cipher_open (&gcrypt_handle, GCRY_CIPHER_BLOWFISH,
                          GCRY_CIPHER_MODE_ECB, 0) != 0)
        goto end;
    if (gcry_cipher_setkey (gcrypt_handle, secret_bin, length_key) != 0)
        goto end;
    if (gcry_cipher_encrypt (gcrypt_handle,
                             password_crypted, length_password,
                             password_clear, length_password) != 0)
        goto end;

    /*
     * build answer for server, it is concatenation of:
     *   1. key length (2 bytes)
     *   2. public key ('length_key' bytes)
     *   3. sasl_username ('length_username'+1 bytes)
     *   4. encrypted password ('length_password' bytes)
     */
    length_username = strlen (sasl_username);
    length_answer = 2 + length_key + length_username + 1 + length_password;
    answer = malloc (length_answer);
    ptr_answer = answer;
    *((unsigned int *)ptr_answer) = htons(length_key);
    ptr_answer += 2;
    memcpy (ptr_answer, public_bin, length_key);
    ptr_answer += length_key;
    memcpy (ptr_answer, sasl_username, length_username + 1);
    ptr_answer += length_username + 1;
    memcpy (ptr_answer, password_crypted, length_password);

    /* encode answer to base64 */
    answer_base64 = malloc (length_answer * 4);
    if (answer_base64)
        weechat_string_encode_base64 (answer, length_answer, answer_base64);

end:
    if (data)
        free (data);
    if (secret_bin)
        free (secret_bin);
    if (public_bin)
        free (public_bin);
    if (password_clear)
        free (password_clear);
    if (password_crypted)
        free (password_crypted);
    if (answer)
        free (answer);
    if (data_prime_number)
        gcry_mpi_release (data_prime_number);
    if (data_generator_number)
        gcry_mpi_release (data_generator_number);
    if (data_server_pub_key)
        gcry_mpi_release (data_server_pub_key);
    if (pub_key)
        gcry_mpi_release (pub_key);
    if (priv_key)
        gcry_mpi_release (priv_key);
    if (secret_mpi)
        gcry_mpi_release (secret_mpi);

    return answer_base64;
}
