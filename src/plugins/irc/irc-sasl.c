/*
 * irc-sasl.c - SASL authentication with IRC server
 *
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <gcrypt.h>

#include <gnutls/gnutls.h>
#if LIBGNUTLS_VERSION_NUMBER >= 0x020a01 /* 2.10.1 */
#include <gnutls/abstract.h>
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x020a01 */

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-sasl.h"
#include "irc-server.h"


/*
 * these names are sent to the IRC server (as upper case), so they must be
 * valid values for the AUTHENTICATE command (example: "AUTHENTICATE PLAIN")
 */
char *irc_sasl_mechanism_string[IRC_NUM_SASL_MECHANISMS] =
{ "plain", "scram-sha-1", "scram-sha-256", "scram-sha-512",
  "ecdsa-nist256p-challenge", "external", "dh-blowfish", "dh-aes" };


/*
 * Builds answer for SASL authentication, using mechanism "PLAIN".
 *
 * Note: result must be freed after use.
 */

char *
irc_sasl_mechanism_plain (const char *sasl_username, const char *sasl_password)
{
    char *answer_base64, *string;
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
        {
            if (weechat_string_base_encode (64, string, length - 1,
                                            answer_base64) < 0)
            {
                free (answer_base64);
                answer_base64 = NULL;
            }
        }

        free (string);
    }

    return answer_base64;
}

/*
 * Builds answer for SASL authentication, using mechanism
 * "SCRAM-SHA-1", "SCRAM-SHA-256" or "SCRAM-SHA-512".
 *
 * If an error occurs or is received from the server, and if sasl_error is
 * not NULL, *sasl_error is set to the error and must be freed after use.
 *
 * Note: result must be freed after use.
 */

char *
irc_sasl_mechanism_scram (struct t_irc_server *server,
                          const char *hash_algo,
                          const char *data_base64,
                          const char *sasl_username,
                          const char *sasl_password,
                          char **sasl_error)
{
    char *answer_base64, *string, *username, *username2, *data, **attrs, *error;
    char nonce_client[18], nonce_client_base64[24 + 1], *nonce_server;
    char *salt_base64, *salt, *verifier_base64, *verifier, *attr_error;
    char *auth_no_proof, *auth_message;
    char salted_password[512 / 8], client_key[512 / 8], stored_key[512 / 8];
    char client_signature[512 / 8], client_proof[512 / 8];
    char client_proof_base64[((512 / 8) * 4) + 1], server_key[512 / 8];
    char server_signature[512 / 8];
    int i, length, num_attrs, iterations, salt_size, salted_password_size;
    int client_key_size, stored_key_size, client_signature_size;
    int server_key_size, server_signature_size, verifier_size;
    long number;

    answer_base64 = NULL;
    string = NULL;
    length = 0;
    username = NULL;
    username2 = NULL;
    data = NULL;
    attrs = NULL;
    nonce_server = NULL;
    salt_base64 = NULL;
    salt = NULL;
    salt_size = 0;
    iterations = 0;
    verifier_base64 = NULL;
    verifier = NULL;
    verifier_size = 0;
    attr_error = NULL;
    auth_no_proof = NULL;
    auth_message = NULL;

    if (strcmp (data_base64, "+") == 0)
    {
        /* send username and nonce with form: "n,,n=username,r=nonce" */
        gcry_create_nonce (nonce_client, sizeof (nonce_client));
        length = weechat_string_base_encode (
            64,
            nonce_client, sizeof (nonce_client),
            nonce_client_base64);
        if (length != sizeof (nonce_client_base64) - 1)
            goto base64_encode_error;
        username = weechat_string_replace (sasl_username, "=", "=3D");
        if (!username)
            goto memory_error;
        username2 = weechat_string_replace (username, ",", "=2C");
        if (!username2)
            goto memory_error;
        length = 5 + strlen (username2) + 3 + sizeof (nonce_client_base64) - 1;
        string = malloc (length + 1);
        if (string)
        {
            snprintf (string, length + 1, "n,,n=%s,r=%s",
                      username2, nonce_client_base64);
            if (server->sasl_scram_client_first)
                free (server->sasl_scram_client_first);
            server->sasl_scram_client_first = strdup (string + 3);
        }
    }
    else
    {
        /* decode SCRAM attributes sent by the server */
        data = malloc (strlen (data_base64) + 1);
        if (!data)
            goto memory_error;
        if (weechat_string_base_decode (64, data_base64, data) <= 0)
            goto base64_decode_error;

        /* split attributes */
        attrs = weechat_string_split (data, ",", NULL,
                                      WEECHAT_STRING_SPLIT_STRIP_LEFT
                                      | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                      | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                      0, &num_attrs);
        if (!attrs)
            goto proto_error;
        for (i = 0; i < num_attrs; i++)
        {
            if (strncmp (attrs[i], "r=", 2) == 0)
            {
                if (nonce_server)
                    free (nonce_server);
                nonce_server = strdup (attrs[i] + 2);
            }
            else if (strncmp (attrs[i], "s=", 2) == 0)
            {
                if (salt_base64)
                    free (salt_base64);
                salt_base64 = strdup (attrs[i] + 2);
            }
            else if (strncmp (attrs[i], "i=", 2) == 0)
            {
                error = NULL;
                number = strtol (attrs[i] + 2, &error, 10);
                if (error && !error[0])
                    iterations = (int)number;
            }
            else if (strncmp (attrs[i], "v=", 2) == 0)
            {
                if (verifier_base64)
                    free (verifier_base64);
                verifier_base64 = strdup (attrs[i] + 2);
            }
            else if (strncmp (attrs[i], "e=", 2) == 0)
            {
                if (attr_error)
                    free (attr_error);
                attr_error = strdup (attrs[i] + 2);
            }
        }
        if (attr_error)
        {
            if (sasl_error)
                *sasl_error = strdup (attr_error);
            goto end;
        }
        else if (verifier_base64)
        {
            /* last exchange: we verify the server signature */
            if (!server->sasl_scram_salted_pwd
                || (server->sasl_scram_salted_pwd_size <= 0)
                || !server->sasl_scram_auth_message)
            {
                goto proto_error;
            }
            verifier = malloc (strlen (verifier_base64) + 1);
            if (!verifier)
                goto memory_error;
            verifier_size = weechat_string_base_decode (64, verifier_base64,
                                                        verifier);
            if (verifier_size <= 0)
                goto base64_decode_error;
            /* RFC: ServerKey := HMAC(SaltedPassword, "Server Key") */
            if (!weechat_crypto_hmac (server->sasl_scram_salted_pwd,
                                      server->sasl_scram_salted_pwd_size,
                                      IRC_SASL_SCRAM_SERVER_KEY,
                                      strlen (IRC_SASL_SCRAM_SERVER_KEY),
                                      hash_algo,
                                      server_key,
                                      &server_key_size))
                goto crypto_error;
            /* RFC: ServerSignature := HMAC(ServerKey, AuthMessage) */
            if (!weechat_crypto_hmac (server_key,
                                      server_key_size,
                                      server->sasl_scram_auth_message,
                                      strlen (server->sasl_scram_auth_message),
                                      hash_algo,
                                      server_signature,
                                      &server_signature_size))
                goto crypto_error;
            if (verifier_size != server_signature_size)
                goto crypto_error;
            if (memcmp (verifier, server_signature, verifier_size) != 0)
            {
                if (sasl_error)
                {
                    *sasl_error = strdup (
                        _("unable to validate server signature"));
                }
                string = strdup ("*");
                if (!string)
                    goto memory_error;
                length = strlen (string);
                goto end;
            }
            string = strdup ("+");
            if (!string)
                goto memory_error;
            length = strlen (string);
        }
        else
        {
            if (!server->sasl_scram_client_first || !nonce_server
                || !salt_base64 || (iterations <= 0))
            {
                goto proto_error;
            }
            /* decode salt */
            salt = malloc (strlen (salt_base64) + 1);
            if (!salt)
                goto memory_error;
            salt_size = weechat_string_base_decode (64, salt_base64, salt);
            if (salt_size <= 0)
                goto base64_decode_error;
            /* RFC: SaltedPassword := Hi(Normalize(password), salt, i) */
            if (!weechat_crypto_hash_pbkdf2 (sasl_password,
                                             strlen (sasl_password),
                                             hash_algo,
                                             salt, salt_size,
                                             iterations,
                                             salted_password,
                                             &salted_password_size))
                goto crypto_error;
            if (server->sasl_scram_salted_pwd)
                free (server->sasl_scram_salted_pwd);
            server->sasl_scram_salted_pwd = malloc (salted_password_size);
            if (!server->sasl_scram_salted_pwd)
                goto memory_error;
            memcpy (server->sasl_scram_salted_pwd, salted_password,
                    salted_password_size);
            server->sasl_scram_salted_pwd_size = salted_password_size;
            /* RFC: ClientKey := HMAC(SaltedPassword, "Client Key") */
            if (!weechat_crypto_hmac (salted_password,
                                      salted_password_size,
                                      IRC_SASL_SCRAM_CLIENT_KEY,
                                      strlen (IRC_SASL_SCRAM_CLIENT_KEY),
                                      hash_algo,
                                      client_key,
                                      &client_key_size))
                goto crypto_error;
            /* RFC: StoredKey := H(ClientKey) */
            if (!weechat_crypto_hash (client_key, client_key_size,
                                      hash_algo,
                                      stored_key,
                                      &stored_key_size))
                goto crypto_error;
            /*
             * RFC: AuthMessage := client-first-message-bare + "," +
             *                     server-first-message + "," +
             *                     client-final-message-without-proof
             */
            length = strlen (nonce_server) + 64 + 1;
            auth_no_proof = malloc (length);
            if (!auth_no_proof)
                goto memory_error;
            /* "biws" is "n,," encoded to base64 */
            snprintf (auth_no_proof, length, "c=biws,r=%s",
                      nonce_server);
            length = strlen (server->sasl_scram_client_first) + 1
                + strlen (data) + 1 + strlen (auth_no_proof) + 1;
            auth_message = malloc (length);
            if (!auth_message)
                goto memory_error;
            snprintf (auth_message, length, "%s,%s,%s",
                      server->sasl_scram_client_first,
                      data,
                      auth_no_proof);
            if (server->sasl_scram_auth_message)
                free (server->sasl_scram_auth_message);
            server->sasl_scram_auth_message = strdup (auth_message);
            /* RFC: ClientSignature := HMAC(StoredKey, AuthMessage) */
            if (!weechat_crypto_hmac (stored_key,
                                      stored_key_size,
                                      auth_message,
                                      strlen (auth_message),
                                      hash_algo,
                                      client_signature,
                                      &client_signature_size))
                goto crypto_error;
            if (client_key_size != client_signature_size)
                goto crypto_error;
            /* RFC: ClientProof := ClientKey XOR ClientSignature */
            for (i = 0; i < client_key_size; i++)
            {
                client_proof[i] = ((unsigned char)client_key[i] ^
                                   (unsigned char)client_signature[i]);
            }
            if (weechat_string_base_encode (64, client_proof, client_key_size,
                                            client_proof_base64) < 0)
                goto base64_encode_error;
            /* final message: auth_no_proof + "," + proof */
            length = strlen (auth_no_proof) + 3 + strlen (client_proof_base64);
            string = malloc (length + 1);
            snprintf (string, length + 1, "%s,p=%s",
                      auth_no_proof,
                      client_proof_base64);
        }
    }
    goto end;

memory_error:
    if (sasl_error)
        *sasl_error = strdup (_("memory error"));
    goto end;

base64_decode_error:
    if (sasl_error)
        *sasl_error = strdup (_("base64 decode error"));
    goto end;

base64_encode_error:
    if (sasl_error)
        *sasl_error = strdup (_("base64 encode error"));
    goto end;

crypto_error:
    if (sasl_error)
        *sasl_error = strdup (_("cryptography error"));
    goto end;

proto_error:
    if (sasl_error)
        *sasl_error = strdup (_("protocol error"));
    goto end;

end:
    if (string && (length > 0))
    {
        if ((strcmp (string, "+") == 0) || (strcmp (string, "*") == 0))
        {
            answer_base64 = strdup (string);
        }
        else
        {
            answer_base64 = malloc ((length + 1) * 4);
            if (answer_base64)
            {
                if (weechat_string_base_encode (64, string, length,
                                                answer_base64) < 0)
                {
                    free (answer_base64);
                    answer_base64 = NULL;
                    if (sasl_error)
                        *sasl_error = strdup (_("base64 encode error"));
                }
            }
        }
    }

    if (string)
        free (string);
    if (username)
        free (username);
    if (username2)
        free (username2);
    if (data)
        free (data);
    if (attrs)
        weechat_string_free_split (attrs);
    if (nonce_server)
        free (nonce_server);
    if (salt_base64)
        free (salt_base64);
    if (salt)
        free (salt);
    if (verifier_base64)
        free (verifier_base64);
    if (verifier)
        free (verifier);
    if (attr_error)
        free (attr_error);
    if (auth_no_proof)
        free (auth_no_proof);
    if (auth_message)
        free (auth_message);

    return answer_base64;
}

/*
 * Returns the content of file with SASL key.
 *
 * If the file is not found and sasl_error is not NULL, *sasl_error is set to
 * the error and must be freed after use.
 *
 * Note: result must be freed after use.
 */

char *
irc_sasl_get_key_content (const char *sasl_key, char **sasl_error)
{
    char *key_path, *content, str_error[4096];
    struct t_hashtable *options;

    if (!sasl_key)
        return NULL;

    content = NULL;

    options = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (options)
        weechat_hashtable_set (options, "directory", "config");
    key_path = weechat_string_eval_path_home (sasl_key, NULL, NULL, options);
    if (options)
        weechat_hashtable_free (options);

    if (key_path)
        content = weechat_file_get_content (key_path);

    if (key_path && !content && sasl_error)
    {
        snprintf (str_error, sizeof (str_error),
                  _("unable to read private key in file \"%s\""),
                  key_path);
        *sasl_error = strdup (str_error);
    }

    if (key_path)
        free (key_path);

    return content;
}

/*
 * Builds answer for SASL authentication, using mechanism
 * "ECDSA-NIST256P-CHALLENGE".
 *
 * If an error occurs and if sasl_error is not NULL, *sasl_error is set to the
 * error and must be freed after use.
 *
 * Note: result must be freed after use.
 */

char *
irc_sasl_mechanism_ecdsa_nist256p_challenge (struct t_irc_server *server,
                                             const char *data_base64,
                                             const char *sasl_username,
                                             const char *sasl_key,
                                             char **sasl_error)
{
#if LIBGNUTLS_VERSION_NUMBER >= 0x030015 /* 3.0.21 */
    char *answer_base64, *string, *data, str_error[4096];
    int length_data, length_username, length, ret;
    char *str_privkey;
    gnutls_x509_privkey_t x509_privkey;
    gnutls_privkey_t privkey;
    gnutls_datum_t filedatum, decoded_data, signature;
#if LIBGNUTLS_VERSION_NUMBER >= 0x030300 /* 3.3.0 */
    gnutls_ecc_curve_t curve;
    gnutls_datum_t x, y, k;
    char *pubkey, *pubkey_base64;
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x030300 */

    answer_base64 = NULL;
    string = NULL;
    length = 0;

    if (strcmp (data_base64, "+") == 0)
    {
        /* send "username" + '\0' + "username" */
        length_username = strlen (sasl_username);
        length = length_username + 1 + length_username;
        string = malloc (length + 1);
        if (string)
        {
            snprintf (string, length + 1, "%s|%s", sasl_username, sasl_username);
            string[length_username] = '\0';
        }
    }
    else
    {
        /* sign the challenge with the private key and return the result */

        /* decode the challenge */
        data = malloc (strlen (data_base64) + 1);
        if (!data)
            return NULL;
        length_data = weechat_string_base_decode (64, data_base64, data);

        /* read file with private key */
        str_privkey = irc_sasl_get_key_content (sasl_key, sasl_error);
        if (!str_privkey)
        {
            free (data);
            return NULL;
        }

        /* import key */
        gnutls_x509_privkey_init (&x509_privkey);
        gnutls_privkey_init (&privkey);
        filedatum.data = (unsigned char *)str_privkey;
        filedatum.size = strlen (str_privkey);
        ret = gnutls_x509_privkey_import (x509_privkey, &filedatum,
                                          GNUTLS_X509_FMT_PEM);
        free (str_privkey);
        if (ret != GNUTLS_E_SUCCESS)
        {
            if (sasl_error)
            {
                snprintf (str_error, sizeof (str_error),
                          _("invalid private key file: error %d %s"),
                          ret,
                          gnutls_strerror (ret));
                *sasl_error = strdup (str_error);
            }
            gnutls_x509_privkey_deinit (x509_privkey);
            gnutls_privkey_deinit (privkey);
            free (data);
            return NULL;
        }

#if LIBGNUTLS_VERSION_NUMBER >= 0x030300 /* 3.3.0 */
        /* read raw values in key, to display public key */
        ret = gnutls_x509_privkey_export_ecc_raw (x509_privkey,
                                                  &curve, &x, &y, &k);
        if (ret == GNUTLS_E_SUCCESS)
        {
            pubkey = malloc (x.size + 1);
            if (pubkey)
            {
                pubkey[0] = (y.data[y.size - 1] & 1) ? 0x03 : 0x02;
                memcpy (pubkey + 1, x.data, x.size);
                pubkey_base64 = malloc ((x.size + 1 + 1) * 4);
                if (pubkey_base64)
                {
                    if (weechat_string_base_encode (64, pubkey, x.size + 1,
                                                    pubkey_base64) >= 0)
                    {
                        weechat_printf (
                            server->buffer,
                            _("%s%s: signing the challenge with ECC public "
                              "key: %s"),
                            weechat_prefix ("network"),
                            IRC_PLUGIN_NAME,
                            pubkey_base64);
                    }
                    free (pubkey_base64);
                }
                free (pubkey);
            }
            gnutls_free (x.data);
            gnutls_free (y.data);
            gnutls_free (k.data);
        }
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x030300 */

        /* import private key in an abstract key structure */
        ret = gnutls_privkey_import_x509 (privkey, x509_privkey, 0); /* gnutls >= 2.11.0 */
        if (ret != GNUTLS_E_SUCCESS)
        {
            if (sasl_error)
            {
                snprintf (str_error, sizeof (str_error),
                          _("unable to import the private key: error %d %s"),
                          ret,
                          gnutls_strerror (ret));
                *sasl_error = strdup (str_error);
            }
            gnutls_x509_privkey_deinit (x509_privkey);
            gnutls_privkey_deinit (privkey);
            free (data);
            return NULL;
        }

        decoded_data.data = (unsigned char *)data;
        decoded_data.size = length_data;
        ret = gnutls_privkey_sign_hash (privkey, GNUTLS_DIG_SHA256, 0, /* gnutls >= 2.11.0 */
                                        &decoded_data, &signature);
        if (ret != GNUTLS_E_SUCCESS)
        {
            if (sasl_error)
            {
                snprintf (str_error, sizeof (str_error),
                          _("unable to sign the hashed data: error %d %s"),
                          ret,
                          gnutls_strerror (ret));
                *sasl_error = strdup (str_error);
            }
            gnutls_x509_privkey_deinit (x509_privkey);
            gnutls_privkey_deinit (privkey);
            free (data);
            return NULL;
        }

        gnutls_x509_privkey_deinit (x509_privkey);
        gnutls_privkey_deinit (privkey);

        string = malloc (signature.size);
        if (string)
            memcpy (string, signature.data, signature.size);
        length = signature.size;

        gnutls_free (signature.data);

        free (data);
    }

    if (string && (length > 0))
    {
        answer_base64 = malloc ((length + 1) * 4);
        if (answer_base64)
        {
            if (weechat_string_base_encode (64, string, length,
                                            answer_base64) < 0)
            {
                free (answer_base64);
                answer_base64 = NULL;
            }
        }
    }

    if (string)
        free (string);

    return answer_base64;

#else /* GnuTLS < 3.0.21 */

    /* make C compiler happy */
    (void) data_base64;
    (void) sasl_username;
    (void) sasl_key;

    if (sasl_error)
    {
        *sasl_error = strdup (
            _("%sgnutls: version >= 3.0.21 is required for SASL "
              "\"ecdsa-nist256p-challenge\""));
    }

    return NULL;
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x030015 */
}

/*
 * Reads key sent by server (Diffie-Hellman key exchange).
 *
 * If an error occurs and if sasl_error is not NULL, *sasl_error is set to the
 * error and must be freed after use.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_sasl_dh (const char *data_base64,
             unsigned char **public_bin, unsigned char **secret_bin,
             int *length_key, char **sasl_error)
{
    char *data;
    unsigned char *ptr_data;
    int length_data, size, num_bits_prime_number, rc;
    size_t num_written;
    gcry_mpi_t data_prime_number, data_generator_number, data_server_pub_key;
    gcry_mpi_t pub_key, priv_key, secret_mpi;

    rc = 0;

    data = NULL;
    data_prime_number = NULL;
    data_generator_number = NULL;
    data_server_pub_key = NULL;
    pub_key = NULL;
    priv_key = NULL;
    secret_mpi = NULL;

    /* decode data */
    data = malloc (strlen (data_base64) + 1);
    if (!data)
	goto memory_error;
    length_data = weechat_string_base_decode (64, data_base64, data);
    ptr_data = (unsigned char *)data;

    /* extract prime number */
    size = ntohs ((((unsigned int)ptr_data[1]) << 8) | ptr_data[0]);
    ptr_data += 2;
    length_data -= 2;
    if (size > length_data)
        goto crypto_error;
    data_prime_number = gcry_mpi_new (size * 8);
    gcry_mpi_scan (&data_prime_number, GCRYMPI_FMT_USG, ptr_data, size, NULL);
    num_bits_prime_number = gcry_mpi_get_nbits (data_prime_number);
    if (num_bits_prime_number == 0 || INT_MAX - 7 < num_bits_prime_number)
	goto crypto_error;
    ptr_data += size;
    length_data -= size;

    /* extract generator number */
    size = ntohs ((((unsigned int)ptr_data[1]) << 8) | ptr_data[0]);
    ptr_data += 2;
    length_data -= 2;
    if (size > length_data)
        goto crypto_error;
    data_generator_number = gcry_mpi_new (size * 8);
    gcry_mpi_scan (&data_generator_number, GCRYMPI_FMT_USG, ptr_data, size, NULL);
    ptr_data += size;
    length_data -= size;

    /* extract server-generated public key */
    size = ntohs ((((unsigned int)ptr_data[1]) << 8) | ptr_data[0]);
    ptr_data += 2;
    length_data -= 2;
    if (size > length_data)
        goto crypto_error;
    data_server_pub_key = gcry_mpi_new (size * 8);
    gcry_mpi_scan (&data_server_pub_key, GCRYMPI_FMT_USG, ptr_data, size, NULL);

    /* generate keys */
    pub_key = gcry_mpi_new (num_bits_prime_number);
    priv_key = gcry_mpi_new (num_bits_prime_number);
    gcry_mpi_randomize (priv_key, num_bits_prime_number, GCRY_STRONG_RANDOM);
    /* pub_key = (g ^ priv_key) % p */
    gcry_mpi_powm (pub_key, data_generator_number, priv_key, data_prime_number);

    /* compute secret_bin */
    *length_key = (num_bits_prime_number + 7) / 8;
    *secret_bin = malloc (*length_key);
    secret_mpi = gcry_mpi_new (num_bits_prime_number);
    /* secret_mpi = (y ^ priv_key) % p */
    gcry_mpi_powm (secret_mpi, data_server_pub_key, priv_key, data_prime_number);
    gcry_mpi_print (GCRYMPI_FMT_USG, *secret_bin, *length_key,
                    &num_written, secret_mpi);

    /* create public_bin */
    *public_bin = malloc (*length_key);
    gcry_mpi_print (GCRYMPI_FMT_USG, *public_bin, *length_key,
                    &num_written, pub_key);
    rc = 1;

    goto end;

memory_error:
    if (sasl_error)
        *sasl_error = strdup (_("memory error"));
    goto end;

crypto_error:
    if (sasl_error)
        *sasl_error = strdup (_("cryptography error"));
    goto end;

end:
    if (data)
        free (data);
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

    return rc;
}

/*
 * Builds answer for SASL authentication, using mechanism "DH-BLOWFISH".
 *
 * Argument data_base64 is a concatenation of 3 strings, each string is composed
 * of 2 bytes (length of string), followed by content of string:
 *   1. a prime number
 *   2. a generator number
 *   3. server-generated public key
 *
 * If an error occurs and if sasl_error is not NULL, *sasl_error is set to the
 * error and must be freed after use.
 *
 * Note: result must be freed after use.
 */

char *
irc_sasl_mechanism_dh_blowfish (const char *data_base64,
                                const char *sasl_username,
                                const char *sasl_password,
                                char **sasl_error)
{
    char *answer, *ptr_answer, *answer_base64;
    unsigned char *password_clear, *password_crypted;
    int length_key, length_username, length_password, length_answer;
    unsigned char *public_bin, *secret_bin;
    gcry_cipher_hd_t gcrypt_handle;

    password_clear = NULL;
    password_crypted = NULL;
    answer = NULL;
    answer_base64 = NULL;
    secret_bin = NULL;
    public_bin = NULL;

    if (!irc_sasl_dh (data_base64, &public_bin, &secret_bin, &length_key,
                      sasl_error))
    {
        goto end;
    }

    /* create password buffers (clear and crypted) */
    length_password = strlen (sasl_password) +
        ((8 - (strlen (sasl_password) % 8)) % 8);
    password_clear = calloc (1, length_password);
    password_crypted = calloc (1, length_password);
    memcpy (password_clear, sasl_password, strlen (sasl_password));

    /* crypt password using blowfish */
    if (gcry_cipher_open (&gcrypt_handle, GCRY_CIPHER_BLOWFISH,
                          GCRY_CIPHER_MODE_ECB, 0) != 0)
        goto crypto_error;
    if (gcry_cipher_setkey (gcrypt_handle, secret_bin, length_key) != 0)
        goto crypto_error;
    if (gcry_cipher_encrypt (gcrypt_handle,
                             password_crypted, length_password,
                             password_clear, length_password) != 0)
        goto crypto_error;

    gcry_cipher_close (gcrypt_handle);

    /*
     * build answer for server, it is concatenation of:
     *   1. key length (2 bytes)
     *   2. public key ('length_key' bytes)
     *   3. sasl_username ('length_username'+1 bytes)
     *   4. encrypted password ('length_password' bytes)
     */
    length_username = strlen (sasl_username) + 1;
    length_answer = 2 + length_key + length_username + length_password;
    answer = malloc (length_answer);
    ptr_answer = answer;
    *((unsigned int *)ptr_answer) = htons (length_key);
    ptr_answer += 2;
    memcpy (ptr_answer, public_bin, length_key);
    ptr_answer += length_key;
    memcpy (ptr_answer, sasl_username, length_username);
    ptr_answer += length_username;
    memcpy (ptr_answer, password_crypted, length_password);

    /* encode answer to base64 */
    answer_base64 = malloc ((length_answer + 1) * 4);
    if (answer_base64)
    {
        if (weechat_string_base_encode (64, answer, length_answer,
                                        answer_base64) < 0)
        {
            free (answer_base64);
            answer_base64 = NULL;
        }
    }

    goto end;

crypto_error:
    if (sasl_error)
        *sasl_error = strdup (_("cryptography error"));
    goto end;

end:
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

    return answer_base64;
}

/*
 * Builds answer for SASL authentication, using mechanism "DH-AES".
 *
 * Argument data_base64 is a concatenation of 3 strings, each string is composed
 * of 2 bytes (length of string), followed by content of string:
 *   1. a prime number
 *   2. a generator number
 *   3. server-generated public key
 *
 * If an error occurs and if sasl_error is not NULL, *sasl_error is set to the
 * error and must be freed after use.
 *
 * Note: result must be freed after use.
 */

char *
irc_sasl_mechanism_dh_aes (const char *data_base64,
                           const char *sasl_username,
                           const char *sasl_password,
                           char **sasl_error)
{
    char *answer, *ptr_answer, *answer_base64;
    unsigned char *ptr_userpass, *userpass_clear, *userpass_crypted;
    int length_key, length_answer;
    int length_username, length_password, length_userpass;
    unsigned char *public_bin, *secret_bin;
    char iv[16];
    int cipher_algo;
    gcry_cipher_hd_t gcrypt_handle;

    userpass_clear = NULL;
    userpass_crypted = NULL;
    answer = NULL;
    answer_base64 = NULL;
    secret_bin = NULL;
    public_bin = NULL;

    if (!irc_sasl_dh (data_base64, &public_bin, &secret_bin, &length_key,
                      sasl_error))
    {
        goto end;
    }

    /* Select cipher algorithm: key length * 8 = cipher bit size */
    switch (length_key)
    {
        case 32:
            cipher_algo = GCRY_CIPHER_AES256;
            break;
        case 24:
            cipher_algo = GCRY_CIPHER_AES192;
            break;
        case 16:
            cipher_algo = GCRY_CIPHER_AES128;
            break;
        default:
            /* Invalid bit length */
            goto end;
    }

    /* Generate the IV */
    gcry_randomize (iv, sizeof (iv), GCRY_STRONG_RANDOM);

    /* create user/pass buffers (clear and crypted) */
    length_username = strlen (sasl_username) + 1;
    length_password = strlen (sasl_password) + 1;
    length_userpass = length_username + length_password +
        ((16 - ((length_username + length_password) % 16)) % 16);
    userpass_clear = calloc (1, length_userpass);
    ptr_userpass = userpass_clear;
    userpass_crypted = calloc (1, length_userpass);
    memcpy (ptr_userpass, sasl_username, length_username);
    ptr_userpass += length_username;
    memcpy (ptr_userpass, sasl_password, length_password);

    /* crypt password using AES in CBC mode */
    if (gcry_cipher_open (&gcrypt_handle, cipher_algo,
                          GCRY_CIPHER_MODE_CBC, 0) != 0)
        goto crypto_error;
    if (gcry_cipher_setkey (gcrypt_handle, secret_bin, length_key) != 0)
        goto crypto_error;
    if (gcry_cipher_setiv (gcrypt_handle, iv, sizeof (iv)) != 0)
        goto crypto_error;
    if (gcry_cipher_encrypt (gcrypt_handle,
                             userpass_crypted, length_userpass,
                             userpass_clear, length_userpass) != 0)
        goto crypto_error;

    gcry_cipher_close (gcrypt_handle);

    /*
     * build answer for server, it is concatenation of:
     *   1. key length (2 bytes)
     *   2. public key ('length_key' bytes)
     *   3. IV (sizeof (iv) bytes)
     *   4. encrypted password ('length_userpass' bytes)
     */
    length_answer = 2 + length_key + sizeof (iv) + length_userpass;
    answer = malloc (length_answer);
    ptr_answer = answer;
    *((unsigned int *)ptr_answer) = htons (length_key);
    ptr_answer += 2;
    memcpy (ptr_answer, public_bin, length_key);
    ptr_answer += length_key;
    memcpy (ptr_answer, iv, sizeof (iv));
    ptr_answer += sizeof (iv);
    memcpy (ptr_answer, userpass_crypted, length_userpass);

    /* encode answer to base64 */
    answer_base64 = malloc ((length_answer + 1) * 4);
    if (answer_base64)
    {
        if (weechat_string_base_encode (64, answer, length_answer,
                                        answer_base64) < 0)
        {
            free (answer_base64);
            answer_base64 = NULL;
        }
    }

    goto end;

crypto_error:
    if (sasl_error)
        *sasl_error = strdup (_("cryptography error"));
    goto end;

end:
    if (secret_bin)
        free (secret_bin);
    if (public_bin)
        free (public_bin);
    if (userpass_clear)
        free (userpass_clear);
    if (userpass_crypted)
        free (userpass_crypted);
    if (answer)
        free (answer);

    return answer_base64;
}
