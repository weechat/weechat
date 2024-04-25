/*
 * relay-auth.c - relay client authentication
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <time.h>
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
char *relay_auth_password_hash_algo_name[RELAY_NUM_PASSWORD_HASH_ALGOS] =
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
 * Generates a nonce: a buffer of unpredictable bytes.
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
    weechat_string_base_encode ("16", nonce, size, nonce_hexa);

    free (nonce);

    return nonce_hexa;
}

/*
 * Checks if password received as plain text is valid.
 *
 * Returns:
 *    0: password is valid
 *   -1: (plain-text password ("plain") is not allowed
 *   -2: password is not valid
 */

int
relay_auth_check_password_plain (struct t_relay_client *client,
                                 const char *password,
                                 const char *relay_password)
{
    if (!client || !password || !relay_password)
        return -2;

    if (!weechat_string_match_list (
            "plain",
            (const char **)relay_config_network_password_hash_algo_list,
            1))
    {
        return -1;
    }

    return (strcmp (password, relay_password) == 0) ? 0 : -2;
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
 *
 * If salt_hexa is not NULL, it is set with salt as hexadecimal, and the parsed
 * salt is decoded and put in salt.
 * If salt_hexa is NULL, parsed salt is considered not encoded and is put
 * directly in salt.
 */

void
relay_auth_parse_sha (const char *parameters,
                      char **salt_hexa, char **salt, int *salt_size,
                      char **hash)
{
    char **argv;
    int argc;

    if (salt_hexa)
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
        weechat_string_free_split (argv);
        return;
    }

    /* parameter 1: salt */
    if (salt_hexa)
    {
        *salt = malloc (strlen (argv[0]) + 1);
        if (*salt)
        {
            *salt_size = weechat_string_base_decode ("16", argv[0], *salt);
            if (*salt_size > 0)
                *salt_hexa = strdup (argv[0]);
            else
            {
                free (*salt);
                *salt = NULL;
            }
        }
    }
    else
    {
        *salt = strdup (argv[0]);
        if (*salt)
            *salt_size = strlen (*salt);
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
 *
 * If salt_hexa is not NULL, it is set with salt as hexadecimal, and the parsed
 * salt is decoded and put in salt.
 * If salt_hexa is NULL, parsed salt is considered not encoded and is put
 * directly in salt.
 */

void
relay_auth_parse_pbkdf2 (const char *parameters,
                         char **salt_hexa, char **salt, int *salt_size,
                         int *iterations, char **hash)
{
    char **argv, *error;
    int argc;

    if (salt_hexa)
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
        weechat_string_free_split (argv);
        return;
    }

    /* parameter 1: salt */
    if (salt_hexa)
    {
        *salt = malloc (strlen (argv[0]) + 1);
        if (*salt)
        {
            *salt_size = weechat_string_base_decode ("16", argv[0], *salt);
            if (*salt_size > 0)
                *salt_hexa = strdup (argv[0]);
            else
            {
                free (*salt);
                *salt = NULL;
            }
        }
    }
    else
    {
        *salt = strdup (argv[0]);
        if (*salt)
            *salt_size = strlen (*salt);
    }

    /* parameter 2: iterations */
    error = NULL;
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
 * For "api" protocol, it is valid if both conditions are true:
 *   1. the salt is a valid integer (unix timestamp)
 *   2. the timestamp value is current timestamp (or +/- seconds, according to
 *      the option relay.network.time_window)
 *
 * For other protocols, it is valid if both conditions are true:
 *   1. the salt is longer than the server nonce, so it means it includes a
 *      client nonce
 *   2. the salt begins with the server nonce (client->nonce)
 *
 * Returns:
 *   1: salt is valid
 *   0: salt is not valid
 */

int
relay_auth_check_salt (struct t_relay_client *client,
                       const char *salt_hexa, const char *salt, int salt_size)
{
    long number;
    int time_window;
    char *error;
    time_t time_now;

    if (!client)
        return 0;

    if (client->protocol == RELAY_PROTOCOL_API)
    {
        if (!salt || (salt_size < 1))
            return 0;
        error = NULL;
        number = strtol (salt, &error, 10);
        if (!error || error[0])
            return 0;
        time_now = time (NULL);
        time_window = weechat_config_integer (relay_config_network_time_window);
        return ((number >= time_now - time_window)
                && (number <= time_now + time_window)) ? 1 : 0;
    }

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

    if (!salt || (salt_size <= 0) || !hash_sha)
        return rc;

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
            weechat_string_base_encode ("16", hash, hash_size,
                                        hash_hexa);
            if (weechat_strcasecmp (hash_hexa, hash_sha) == 0)
                rc = 1;
        }
        free (salt_password);
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
            weechat_string_base_encode ("16", hash, hash_size, hash_hexa);
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
 *    0: authentication OK
 *   -1: invalid hash algorithm
 *   -2: invalid salt
 *   -3: invalid number of iterations
 *   -4: invalid password (hash)
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

    /* no authentication supported at all with weechat protocol? */
    if ((client->protocol == RELAY_PROTOCOL_WEECHAT)
        && (client->password_hash_algo < 0))
    {
        rc = -1;
        goto end;
    }

    if (!hashed_password || !relay_password)
    {
        rc = -4;
        goto end;
    }

    pos_hash = strchr (hashed_password, ':');
    if (!pos_hash)
    {
        rc = -4;
        goto end;
    }

    str_hash_algo = weechat_strndup (hashed_password,
                                     pos_hash - hashed_password);
    if (!str_hash_algo)
    {
        rc = -1;
        goto end;
    }

    pos_hash++;

    hash_algo = relay_auth_password_hash_algo_search (str_hash_algo);
    if (hash_algo < 0)
    {
        rc = -1;
        goto end;
    }

    /* only algo negotiated in handshake is allowed for protocol "weechat" */
    if ((client->protocol == RELAY_PROTOCOL_WEECHAT)
        && (hash_algo != client->password_hash_algo))
    {
        rc = -1;
        goto end;
    }

    /* only algos matching allowed algos are allowed for protocol "api" */
    if ((client->protocol == RELAY_PROTOCOL_API)
        && (!weechat_string_match_list (
                relay_auth_password_hash_algo_name[hash_algo],
                (const char **)relay_config_network_password_hash_algo_list,
                1)))
    {
        rc = -1;
        goto end;
    }

    salt_hexa = NULL;

    switch (hash_algo)
    {
        case RELAY_AUTH_PASSWORD_HASH_SHA256:
        case RELAY_AUTH_PASSWORD_HASH_SHA512:
            relay_auth_parse_sha (
                pos_hash,
                (client->protocol == RELAY_PROTOCOL_API) ? NULL : &salt_hexa,
                &salt,
                &salt_size,
                &hash_sha);
            if (!relay_auth_check_salt (client, salt_hexa, salt, salt_size))
            {
                rc = -2;
            }
            else if (!relay_auth_check_hash_sha (str_hash_algo, salt, salt_size,
                                                 hash_sha, relay_password))
            {
                rc = -4;;
            }
            free (salt_hexa);
            free (salt);
            free (hash_sha);
            break;
        case RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA256:
        case RELAY_AUTH_PASSWORD_HASH_PBKDF2_SHA512:
            hash_pbkdf2_algo = strdup (str_hash_algo + 7);
            relay_auth_parse_pbkdf2 (
                pos_hash,
                (client->protocol == RELAY_PROTOCOL_API) ? NULL : &salt_hexa,
                &salt,
                &salt_size,
                &iterations,
                &hash_pbkdf2);
            if (iterations != weechat_config_integer (relay_config_network_password_hash_iterations))
            {
                rc = -3;
            }
            else if (!relay_auth_check_salt (client, salt_hexa, salt, salt_size))
            {
                rc = -2;
            }
            else if (!relay_auth_check_hash_pbkdf2 (hash_pbkdf2_algo, salt,
                                                    salt_size, iterations,
                                                    hash_pbkdf2, relay_password))
            {
                rc = -4;
            }
            free (hash_pbkdf2_algo);
            free (salt_hexa);
            free (salt);
            free (hash_pbkdf2);
            break;
        case RELAY_NUM_PASSWORD_HASH_ALGOS:
            rc = -4;
            break;
    }

end:
    free (str_hash_algo);

    return rc;
}
