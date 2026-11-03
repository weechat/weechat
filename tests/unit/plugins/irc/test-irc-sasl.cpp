/*
 * SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC SASL functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "src/core/core-crypto.h"
#include "src/core/core-string.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-sasl.h"
#include "src/plugins/irc/irc-server.h"

extern char *irc_sasl_get_key_content (const char *sasl_key,
                                       char **sasl_error);
}

TEST_GROUP(IrcSasl)
{
};

/*
 * Test functions:
 *   irc_sasl_mechanism_plain
 */

TEST(IrcSasl, MechanismPlain)
{
    char *str;

    STRCMP_EQUAL(NULL, irc_sasl_mechanism_plain (NULL, NULL));
    STRCMP_EQUAL(NULL, irc_sasl_mechanism_plain (NULL, ""));
    STRCMP_EQUAL(NULL, irc_sasl_mechanism_plain ("", NULL));

    STRCMP_EQUAL("AAA=", irc_sasl_mechanism_plain ("", ""));

    /* "alice\0alice\0" */
    WEE_TEST_STR("YWxpY2UAYWxpY2UA",
                 irc_sasl_mechanism_plain ("alice", ""));

    /* "alice\0alice\0secret" */
    WEE_TEST_STR("YWxpY2UAYWxpY2UAc2VjcmV0",
                 irc_sasl_mechanism_plain ("alice", "secret"));

    /* "\0\0secret" */
    WEE_TEST_STR("AABzZWNyZXQ=",
                 irc_sasl_mechanism_plain ("", "secret"));
}

/*
 * Test functions:
 *   irc_sasl_mechanism_scram
 */

TEST(IrcSasl, MechanismScram)
{
    struct t_irc_server *server;
    char *str, str_decoded[1024], *error, nonce_client[256], msg[1024];
    char msg_base64[2048], server_final_base64[2048];
    char server_key[512 / 8], server_signature[512 / 8];
    char server_signature_base64[((512 / 8) * 4) + 1];
    int server_key_size, server_signature_size;

    STRCMP_EQUAL(NULL, irc_sasl_mechanism_scram (NULL, NULL, NULL, NULL,
                                                 NULL, NULL));

    server = irc_server_alloc ("my_ircd");

    /* Decoded returned value is like: "n,,n=user,r=rOprNGfwEbeRWgbNEkqO". */
    error = NULL;
    str = irc_sasl_mechanism_scram (server, "sha256", "+",
                                    "user1", "secret", &error);
    STRCMP_EQUAL(NULL, error);
    CHECK(string_base64_decode (0, str, str_decoded) > 0);
    CHECK(strncmp (str_decoded, "n,,n=user1,r=", 13) == 0);
    snprintf (nonce_client, sizeof (nonce_client), "%s", str_decoded + 13);
    free (str);

    /* Server-first message with an iteration count too low. */
    snprintf (msg, sizeof (msg), "r=%sserver,s=c2FsdA==,i=1", nonce_client);
    string_base64_encode (0, msg, strlen (msg), msg_base64);
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_scram (server, "sha256",
                                                   msg_base64, "user1",
                                                   "secret", &error));
    STRCMP_EQUAL("invalid iteration count: 1 (must be between 4096 "
                 "and 1000000)", error);
    free (error);
    snprintf (msg, sizeof (msg), "r=%sserver,s=c2FsdA==,i=4095",
              nonce_client);
    string_base64_encode (0, msg, strlen (msg), msg_base64);
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_scram (server, "sha256",
                                                   msg_base64, "user1",
                                                   "secret", &error));
    STRCMP_EQUAL("invalid iteration count: 4095 (must be between 4096 "
                 "and 1000000)", error);
    free (error);

    /* Server-first message with an iteration count too high. */
    snprintf (msg, sizeof (msg), "r=%sserver,s=c2FsdA==,i=1000001",
              nonce_client);
    string_base64_encode (0, msg, strlen (msg), msg_base64);
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_scram (server, "sha256",
                                                   msg_base64, "user1",
                                                   "secret", &error));
    STRCMP_EQUAL("invalid iteration count: 1000001 (must be between 4096 "
                 "and 1000000)", error);
    free (error);
    snprintf (msg, sizeof (msg), "r=%sserver,s=c2FsdA==,i=2147483647",
              nonce_client);
    string_base64_encode (0, msg, strlen (msg), msg_base64);
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_scram (server, "sha256",
                                                   msg_base64, "user1",
                                                   "secret", &error));
    STRCMP_EQUAL("invalid iteration count: 2147483647 (must be between 4096 "
                 "and 1000000)", error);
    free (error);
    POINTERS_EQUAL(NULL, server->sasl_scram_salted_pwd);
    POINTERS_EQUAL(NULL, server->sasl_scram_auth_message);

    /* Server-first message with a valid iteration count. */
    snprintf (msg, sizeof (msg), "r=%sserver,s=c2FsdA==,i=4096",
              nonce_client);
    string_base64_encode (0, msg, strlen (msg), msg_base64);
    error = NULL;
    str = irc_sasl_mechanism_scram (server, "sha256", msg_base64,
                                    "user1", "secret", &error);
    STRCMP_EQUAL(NULL, error);
    CHECK(str);
    CHECK(string_base64_decode (0, str, str_decoded) > 0);
    snprintf (msg, sizeof (msg), "c=biws,r=%sserver,p=", nonce_client);
    CHECK(strncmp (str_decoded, msg, strlen (msg)) == 0);
    free (str);
    CHECK(server->sasl_scram_salted_pwd);
    LONGS_EQUAL(256 / 8, server->sasl_scram_salted_pwd_size);
    CHECK(server->sasl_scram_auth_message);

    /* Server-final message with an invalid server signature. */
    snprintf (msg, sizeof (msg),
              "v=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");
    string_base64_encode (0, msg, strlen (msg), msg_base64);
    error = NULL;
    WEE_TEST_STR("*", irc_sasl_mechanism_scram (server, "sha256",
                                                msg_base64, "user1",
                                                "secret", &error));
    STRCMP_EQUAL("unable to validate server signature", error);
    free (error);

    /* Server-final message with a valid server signature. */
    CHECK(weecrypto_hmac (server->sasl_scram_salted_pwd,
                          server->sasl_scram_salted_pwd_size,
                          "Server Key", 10,
                          weecrypto_get_hash_algo ("sha256"),
                          server_key, &server_key_size));
    CHECK(weecrypto_hmac (server_key, server_key_size,
                          server->sasl_scram_auth_message,
                          strlen (server->sasl_scram_auth_message),
                          weecrypto_get_hash_algo ("sha256"),
                          server_signature, &server_signature_size));
    string_base64_encode (0, server_signature, server_signature_size,
                          server_signature_base64);
    snprintf (msg, sizeof (msg), "v=%s", server_signature_base64);
    string_base64_encode (0, msg, strlen (msg), server_final_base64);
    error = NULL;
    WEE_TEST_STR("+", irc_sasl_mechanism_scram (server, "sha256",
                                                server_final_base64, "user1",
                                                "secret", &error));
    STRCMP_EQUAL(NULL, error);

    /* Error sent by the server. */
    snprintf (msg, sizeof (msg), "e=invalid-proof");
    string_base64_encode (0, msg, strlen (msg), msg_base64);
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_scram (server, "sha256",
                                                   msg_base64, "user1",
                                                   "secret", &error));
    STRCMP_EQUAL("invalid-proof", error);
    free (error);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_sasl_get_key_content
 */

TEST(IrcSasl, GetKeyContent)
{
    const char *key = "-----BEGIN EC PRIVATE KEY-----\n"
        "not a real key\n"
        "-----END EC PRIVATE KEY-----\n";
    const char *sasl_key = "${weechat_config_dir}/test_sasl_key.pem";
    char *path, *content, *error, str_error[4096];
    FILE *file;

    /* No key: no content and no error */
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_get_key_content (NULL, &error));
    STRCMP_EQUAL(NULL, error);
    POINTERS_EQUAL(NULL, irc_sasl_get_key_content (NULL, NULL));

    path = string_eval_path_home (sasl_key, NULL, NULL, NULL);
    CHECK(path);
    unlink (path);

    snprintf (str_error, sizeof (str_error),
              "unable to read private key in file \"%s\"", path);

    /* Missing file: the error mentions the evaluated path. */
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_get_key_content (sasl_key, &error));
    STRCMP_EQUAL(str_error, error);
    free (error);

    /* No crash if sasl_error is NULL. */
    POINTERS_EQUAL(NULL, irc_sasl_get_key_content (sasl_key, NULL));

    /* Empty file: read as an error. */
    file = fopen (path, "w");
    CHECK(file);
    fclose (file);
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_get_key_content (sasl_key, &error));
    STRCMP_EQUAL(str_error, error);
    free (error);

    /* Existing file: content is returned and no error is set. */
    file = fopen (path, "w");
    CHECK(file);
    fwrite (key, 1, strlen (key), file);
    fclose (file);
    error = NULL;
    content = irc_sasl_get_key_content (sasl_key, &error);
    STRCMP_EQUAL(NULL, error);
    STRCMP_EQUAL(key, content);
    free (content);

    /* Same file, reached with the deprecated "%h" prefix. */
    error = NULL;
    content = irc_sasl_get_key_content ("%h/test_sasl_key.pem", &error);
    STRCMP_EQUAL(NULL, error);
    STRCMP_EQUAL(key, content);
    free (content);

    unlink (path);
    free (path);
}

/*
 * Test functions:
 *   irc_sasl_mechanism_ecdsa_nist256p_challenge
 */

TEST(IrcSasl, MechanismEcdsaNist256pChallenge)
{
    struct t_irc_server *server;
    char *str, *error;

    server = irc_server_alloc ("test_ecdsa");

    /* First answer: "alice\0alice" */
    error = NULL;
    str = irc_sasl_mechanism_ecdsa_nist256p_challenge (
        server, "+", "alice", "/does/not/exist.pem", &error);
    STRCMP_EQUAL(NULL, error);
    STRCMP_EQUAL("YWxpY2UAYWxpY2U=", str);
    free (str);

    /* Empty challenge */
    error = NULL;
    str = irc_sasl_mechanism_ecdsa_nist256p_challenge (
        server, "", "alice", "/does/not/exist.pem", &error);
    STRCMP_EQUAL(NULL, str);
    STRCMP_EQUAL("base64 decode error", error);
    free (error);

    /* Challenge with an invalid base64 char */
    error = NULL;
    str = irc_sasl_mechanism_ecdsa_nist256p_challenge (
        server, "abcd!efgh", "alice", "/does/not/exist.pem", &error);
    STRCMP_EQUAL(NULL, str);
    STRCMP_EQUAL("base64 decode error", error);
    free (error);

    /* Challenge with a truncated group of base64 chars */
    error = NULL;
    str = irc_sasl_mechanism_ecdsa_nist256p_challenge (
        server, "abcde", "alice", "/does/not/exist.pem", &error);
    STRCMP_EQUAL(NULL, str);
    STRCMP_EQUAL("base64 decode error", error);
    free (error);

    /* Valid challenge, but the private key can not be read */
    error = NULL;
    str = irc_sasl_mechanism_ecdsa_nist256p_challenge (
        server, "YWJjZA==", "alice", "/does/not/exist.pem", &error);
    STRCMP_EQUAL(NULL, str);
    STRCMP_EQUAL("unable to read private key in file \"/does/not/exist.pem\"",
                 error);
    free (error);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_sasl_mechanism_external
 */

TEST(IrcSasl, MechanismExternal)
{
    char *str;

    WEE_TEST_STR("+", irc_sasl_mechanism_external (NULL));
    WEE_TEST_STR("+", irc_sasl_mechanism_external (""));

    WEE_TEST_STR("YWxpY2U=", irc_sasl_mechanism_external ("alice"));
}
