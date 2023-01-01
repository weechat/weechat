/*
 * relay-auth.c - relay client authentication
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gcrypt.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-auth.h"
#include "relay-client.h"
#include "relay-config.h"


/*
 * this list is sorted from the least secure to the most secure algorithm:
 * "plain" is plain text password, the other values are hash algorithms;
 * during negotiation with the client, the highest value in this list matching
 * the client supported values is used
 */
char *relay_auth_password_hash_algo_name[] =
{ "plain", "sha256", "sha512", "pbkdf2+sha256", "pbkdf2+sha512" };


/*
 * Searches for a password hash algorithm.
 *
 * Returns index in enum t_relay_auth_password_hash_algo,
 * -1 if password hash algorithm is not found.
 */

int
relay_auth_password_hash_algo_search (const char *name)
{
    int i;

    if (!name)
        return -1;

    for (i = 0; i < RELAY_NUM_PASSWORD_HASH_ALGOS; i++)
    {
        if (strcmp (relay_auth_password_hash_algo_name[i], name) == 0)
            return i;
    }

    /* password hash algorithm not found */
    return -1;
}

/*
 * Generates a nonce: a buffer of unpredictable bytes
 *
 * Note: result must be freed after use.
 */

char *
relay_auth_generate_nonce (int size)
{
    char *nonce, *nonce_hexa;

    if (size < 1)
        return NULL;

    nonce = malloc (size);
    if (!nonce)
        return NULL;

    nonce_hexa = malloc ((size * 2) + 1);
    if (!nonce_hexa)
    {
        free (nonce);
        return NULL;
    }

    gcry_create_nonce ((unsigned char *)nonce, size);
    weechat_string_base_encode (16, nonce, size, nonce_hexa);

    free (nonce);

    return nonce_hexa;
}

/*
 * Checks if password received as plain text is valid.
 *
 * Returns:
 *   1: password is valid
 *   0: password is not valid
 */

int
relay_auth_check_password_plain (const char *password,
                                 const char *relay_password)
{
    if (!password || !relay_password)
        return 0;

    return (strcmp (password, relay_password) == 0) ? 1 : 0;
}

/*
 * Authenticates with password (plain text).
 *
 * Returns:
 *   1: authentication OK
 *   0: authentication failed
 */

int
relay_auth_password (struct t_relay_client *client,
                     const char *password, const char *relay_password)
{
    if (client->password_hash_algo != RELAY_AUTH_PASSWORD_HASH_PLAIN)
        return 0;

    return relay_auth_check_password_plain (password, relay_password);

}

/*
 * Parses SHA256 or SHA512 parameters from string with format:
 *
 *   salt:hash
 *
 * where:
 *
 *   salt is the salt in hexadecimal
 *   hash is the hashed password with the parameters above, in hexadecimal
 */

void
relay_auth_parse_sha (const char *parameters,
                      char **salt_hexa, char **salt, int *salt_size,
                      char **hash)
{
    char **argv;
    int argc;

    *salt_hexa = NULL;
    *salt = NULL;
    *salt_size = 0;
    *hash = NULL;

    if (!parameters)
        return;

    argv = weechat_string_split (parameters, ":", NULL, 0, 0, &argc);

    if (!argv || (argc < 2))
    {
        /* not enough parameters */
        if (argv)
            weechat_string_free_split (argv);
        return;
    }

    /* parameter 1: salt */
    *salt = malloc (strlen (argv[0]) + 1);
    if (*salt)
    {
        *salt_size = weechat_string_base_decode (16, argv[0], *salt);
        if (*salt_size > 0)
            *salt_hexa = strdup (argv[0]);
        else
        {
            free (*salt);
            *salt = NULL;
        }
    }

    /* parameter 2: the SHA256 or SHA512 hash */
    *hash = strdup (argv[1]);

    weechat_string_free_split (argv);
}

/*
 * Parses PBKDF2 parameters from string with format:
 *
 *   salt:iterations:hash
 *
 * where:
 *
 *   salt is the salt in hexadecimal
 *   iterations it the number of iterations (≥ 1)
 *   hash is the hashed password with the parameters above, in hexadecimal
 */

void
relay_auth_parse_pbkdf2 (const char *parameters,
                         char **salt_hexa, char **salt, int *salt_size,
                         int *iterations, char **hash)
{
    char **argv, *error;
    int argc;

    *salt_hexa = NULL;
    *salt = NULL;
    *salt_size = 0;
    *iterations = 0;
    *hash = NULL;

    if (!parameters)
        return;

    argv = weechat_string_split (parameters, ":", NULL, 0, 0, &argc);

    if (!argv || (argc < 3))
    {
        /* not enough parameters */
        if (argv)
            weechat_string_free_split (argv);
        return;
    }

    /* parameter 1: salt */
    *salt = malloc (strlen (argv[0]) + 1);
    if (*salt)
    {
        *salt_size = weechat_string_base_decode (16, argv[0], *salt);
        if (*salt_size > 0)
            *salt_hexa = strdup (argv[0]);
        else
        {
            free (*salt);
            *salt = NULL;
        }
    }

    /* parameter 2: iterations */
    *iterations = (int)strtol (argv[1], &error, 10);
    if (!error || error[0])
        *iterations = 0;

    /* parameter 3: the PBKDF2 hash */
    *hash = strdup (argv[2]);

    weechat_string_free_split (argv);
}

/*
 * Checks if the salt received from the client is valid.
 *
 * It is valid if both conditions are true:
 *   1. the salt is longer than the server nonce, so it means it includes a
 *      client nonce
 *   2. the salt begins with the server nonce (client->nonce)
 *
 * Returns:
 *   1: salt is valid
 *   0: salt is not valid
 */

int
relay_auth_check_salt (struct t_relay_client *client, const char *salt_hexa)
{
    return (salt_hexa
            && client->nonce
            && (strlen (salt_hexa) > strlen (client->nonce))
            && (weechat_strncasecmp (
                    salt_hexa, client->nonce,
                    weechat_utf8_strlen (client->nonce)) == 0)) ? 1 : 0;
}

/*
 * Checks if password received as SHA256/SHA512 hash is valid.
 *
 * Returns:
 *   1: password is valid
 *   0: password is not valid
 */

int
relay_auth_check_hash_sha (const char *hash_algo,
                           const char *salt,
                           int salt_size,
                           const char *hash_sha,
                           const char *relay_password)
{
    char *salt_password, hash[512 / 8], hash_hexa[((512 / 8) * 2) + 1];
    int rc, length, hash_size;

    rc = 0;

    if (salt && (salt_size > 0) && hash_sha)
    {
        length = salt_size + strlen (relay_password);
        salt_password = malloc (length);
        if (salt_password)
        {
            memcpy (salt_password, salt, salt_size);
            memcpy (salt_password + salt_size, relay_password,
                    strlen (relay_password));
            if (weechat_crypto_hash (salt_password, length,
                                     hash_algo,
                                     hash, &hash_size))
            {
                weechat_string_base_encode (16, hash, hash_size,
                                            hash_hexa);
                if (weechat_strcasecmp (hash_hexa, hash_sha) == 0)
                    rc = 1;
            }
            free (salt_password);
        }
    }

    return rc;
}

/*
 * Checks if password received as PBKDF2 hash is valid.
 *
 * Returns:
 *   1: password is valid
 *   0: password is not valid
 */

int
relay_auth_check_hash_pbkdf2 (const char *hash_pbkdf2_algo,
                              const char *salt,
                              int salt_size,
                              int iterations,
                              const char *hash_pbkdf2,
                              const char *relay_password)
{
    char hash[512 / 8], hash_hexa[((512 / 8) * 2) + 1];
    int rc, hash_size;

    rc = 0;

    if (hash_pbkdf2_algo && salt && (salt_size > 0) && hash_pbkdf2)
    {
        if (weechat_crypto_hash_pbkdf2 (relay_password,
                                        strlen (relay_password),
                                        hash_pbkdf2_algo,
                                        salt, salt_size,
                                        iterations,
                                        hash, &hash_size))
        {
            weechat_string_base_encode (16, hash, hash_size, hash_hexa);
            if (weechat_strcasecmp (hash_hexa, hash_pbkdf2) == 0)
                rc = 1;
        }
    }

    return rc;
}

/*
 * Authenticates with password hash.
 *
 * Returns:
 *   1: authentication OK
 *   0: authentication failed
 */

int
relay_auth_password_hash (struct t_relay_client *client,
                          const char *hashed_password, const char *relay_password)
{
    const char *pos_hash;
    char *str_hash_algo;
    char *hash_pbkdf2_algo, *salt_hexa, *salt, *hash_sha, *hash_pbkdf2;
    int rc, hash_algo, salt_size, iterations;

    rc = 0;

    str_hash_algo = NULL;

    /* no authentication supported at all? */
    if (client->password_hash_algo < 0)
        goto end;

    if (!hashed_password || !relay_password)
        goto end;

    pos_hash = strchr (hashed_password, ':');
    if (!pos_hash)
        goto end;

    str_hash_algo = weechat_strndup (hashed_password,
                                     pos_hash - hashed_password);
    if (!str_hash_algo)
        goto end;

    pos_hash++;

    hash_algo = relay_auth_password_hash_algo_search (str_hash_algo);

    if (hash_algo != client->password_hash_algo)
        goto end;

    switch (hash_algo)
    {
        case RELAY_AUTH_PASSWORD_HASH_SHA256:
        case RELAY_AUTH_PASSWORD_HASH_SHA512:
            relay_auth_parse_sha (pos_hash, &salt_hexa, &salt, &salt_size,
                                  &hash_sha);
            if (relay_auth_check_salt (client, salt_hexa)
                && relay_auth_check_hash_sha (str_hash_algo, salt, salt_size,
                                              hash_sha, relay_password))
            {
                rc = 1;
            }
            if (salt_hexa)
                free (salt_hexa);
            if (salt)
                free (salt);
            if (hash_sha)
                free (hash_sha);
            break;
        case RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA256:
        case RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA512:
            hash_pbkdf2_algo = strdup (str_hash_algo + 7);
            relay_auth_parse_pbkdf2 (pos_hash, &salt_hexa, &salt, &salt_size,
                                     &iterations, &hash_pbkdf2);
            if ((iterations == client->password_hash_iterations)
                && relay_auth_check_salt (client, salt_hexa)
                && relay_auth_check_hash_pbkdf2 (hash_pbkdf2_algo, salt,
                                                 salt_size, iterations,
                                                 hash_pbkdf2, relay_password))
            {
                rc = 1;
            }
            if (hash_pbkdf2_algo)
                free (hash_pbkdf2_algo);
            if (salt_hexa)
                free (salt_hexa);
            if (salt)
                free (salt);
            if (hash_pbkdf2)
                free (hash_pbkdf2);
            break;
        case RELAY_NUM_PASSWORD_HASH_ALGOS:
            break;
    }

end:
    if (str_hash_algo)
        free (str_hash_algo);

    return rc;
}
