/*
 * test-irc-sasl.cpp - test IRC SASL functions
 *
 * Copyright (C) 2021-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

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
}

TEST_GROUP(IrcSasl)
{
};

/*
 * Tests functions:
 *   irc_sasl_mechanism_plain
 */

TEST(IrcSasl, MechanismPlain)
{
    STRCMP_EQUAL(NULL, irc_sasl_mechanism_plain (NULL, NULL));
    STRCMP_EQUAL(NULL, irc_sasl_mechanism_plain (NULL, ""));
    STRCMP_EQUAL(NULL, irc_sasl_mechanism_plain ("", NULL));

    STRCMP_EQUAL("AAA=", irc_sasl_mechanism_plain ("", ""));

    /* "alice\0alice\0" */
    STRCMP_EQUAL("YWxpY2UAYWxpY2UA",
                 irc_sasl_mechanism_plain ("alice", ""));

    /* "alice\0alice\0secret" */
    STRCMP_EQUAL("YWxpY2UAYWxpY2UAc2VjcmV0",
                 irc_sasl_mechanism_plain ("alice", "secret"));

    /* "\0\0secret" */
    STRCMP_EQUAL("AABzZWNyZXQ=",
                 irc_sasl_mechanism_plain ("", "secret"));
}

/*
 * Tests functions:
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

    /* decoded returned value is like: n,,n=user,r=rOprNGfwEbeRWgbNEkqO */
    error = NULL;
    str = irc_sasl_mechanism_scram (server, "sha256", "+",
                                    "user1", "secret", &error);
    STRCMP_EQUAL(NULL, error);
    CHECK(string_base64_decode (0, str, str_decoded) > 0);
    CHECK(strncmp (str_decoded, "n,,n=user1,r=", 13) == 0);
    snprintf (nonce_client, sizeof (nonce_client), "%s", str_decoded + 13);
    free (str);

    /* Server-first message with an invalid server nonce. */
    snprintf (msg, sizeof (msg), "r=invalid,s=c2FsdA==,i=4096");
    string_base64_encode (0, msg, strlen (msg), msg_base64);
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_scram (server, "sha256",
                                                   msg_base64, "user1",
                                                   "secret", &error));
    STRCMP_EQUAL("invalid server nonce", error);
    free (error);
    snprintf (msg, sizeof (msg), "r=%.10s,s=c2FsdA==,i=4096", nonce_client);
    string_base64_encode (0, msg, strlen (msg), msg_base64);
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_scram (server, "sha256",
                                                   msg_base64, "user1",
                                                   "secret", &error));
    STRCMP_EQUAL("invalid server nonce", error);
    free (error);
    snprintf (msg, sizeof (msg), "r=%s,s=c2FsdA==,i=4096", nonce_client);
    string_base64_encode (0, msg, strlen (msg), msg_base64);
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_scram (server, "sha256",
                                                   msg_base64, "user1",
                                                   "secret", &error));
    STRCMP_EQUAL("invalid server nonce", error);
    free (error);

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
 * Tests functions:
 *   irc_sasl_get_key_content
 */

TEST(IrcSasl, GetKeyContent)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_sasl_mechanism_ecdsa_nist256p_challenge
 */

TEST(IrcSasl, MechanismEcdsaNist256pChallenge)
{
    /* TODO: write tests */
}
