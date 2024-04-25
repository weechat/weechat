/*
 * irc-sasl.c - SASL authentication with IRC server
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
  "ecdsa-nist256p-challenge", "external" };


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

    if (!sasl_username || !sasl_password)
        return NULL;

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
            if (weechat_string_base_encode ("64", string, length - 1,
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
    int i, rc, length, num_attrs, iterations, salt_size, salted_password_size;
    int client_key_size, stored_key_size, client_signature_size;
    int server_key_size, server_signature_size, verifier_size;
    long number;

    if (!server || !hash_algo || !data_base64 || !sasl_username
        || !sasl_password)
    {
        return NULL;
    }

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
    iterations = 0;
    verifier_base64 = NULL;
    verifier = NULL;
    attr_error = NULL;
    auth_no_proof = NULL;
    auth_message = NULL;

    if (strcmp (data_base64, "+") == 0)
    {
        /* send username and nonce with form: "n,,n=username,r=nonce" */
        gcry_create_nonce (nonce_client, sizeof (nonce_client));
        length = weechat_string_base_encode (
            "64",
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
        if (weechat_string_base_decode ("64", data_base64, data) <= 0)
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
                free (nonce_server);
                nonce_server = strdup (attrs[i] + 2);
            }
            else if (strncmp (attrs[i], "s=", 2) == 0)
            {
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
                free (verifier_base64);
                verifier_base64 = strdup (attrs[i] + 2);
            }
            else if (strncmp (attrs[i], "e=", 2) == 0)
            {
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
            verifier_size = weechat_string_base_decode ("64", verifier_base64,
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
            salt_size = weechat_string_base_decode ("64", salt_base64, salt);
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
            rc = snprintf (auth_message, length, "%s,%s,%s",
                           server->sasl_scram_client_first,
                           data,
                           auth_no_proof);
            if ((rc < 0) || (rc >= length))
                goto memory_error;
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
            if (weechat_string_base_encode ("64", client_proof, client_key_size,
                                            client_proof_base64) < 0)
                goto base64_encode_error;
            /* final message: auth_no_proof + "," + proof */
            length = strlen (auth_no_proof) + 3 + strlen (client_proof_base64);
            string = malloc (length + 1);
            rc = snprintf (string, length + 1, "%s,p=%s",
                           auth_no_proof,
                           client_proof_base64);
            if ((rc < 0) || (rc >= length + 1))
                goto memory_error;
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
                if (weechat_string_base_encode ("64", string, length,
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

    free (string);
    free (username);
    free (username2);
    free (data);
    weechat_string_free_split (attrs);
    free (nonce_server);
    free (salt_base64);
    free (salt);
    free (verifier_base64);
    free (verifier);
    free (attr_error);
    free (auth_no_proof);
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
        length_data = weechat_string_base_decode ("64", data_base64, data);

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
                    if (weechat_string_base_encode ("64", pubkey, x.size + 1,
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
            if (weechat_string_base_encode ("64", string, length,
                                            answer_base64) < 0)
            {
                free (answer_base64);
                answer_base64 = NULL;
            }
        }
    }

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
