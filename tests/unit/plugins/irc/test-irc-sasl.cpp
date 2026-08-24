/*
 * SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Test IRC SASL functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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
    char *str, str_decoded[1024], *error;

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
    free (str);

    /* TODO: complete tests */

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

    /* no key: no content and no error */
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_get_key_content (NULL, &error));
    STRCMP_EQUAL(NULL, error);
    POINTERS_EQUAL(NULL, irc_sasl_get_key_content (NULL, NULL));

    path = string_eval_path_home (sasl_key, NULL, NULL, NULL);
    CHECK(path);
    unlink (path);

    snprintf (str_error, sizeof (str_error),
              "unable to read private key in file \"%s\"", path);

    /* missing file: the error mentions the evaluated path */
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_get_key_content (sasl_key, &error));
    STRCMP_EQUAL(str_error, error);
    free (error);

    /* no crash if sasl_error is NULL */
    POINTERS_EQUAL(NULL, irc_sasl_get_key_content (sasl_key, NULL));

    /* empty file: read as an error */
    file = fopen (path, "w");
    CHECK(file);
    fclose (file);
    error = NULL;
    POINTERS_EQUAL(NULL, irc_sasl_get_key_content (sasl_key, &error));
    STRCMP_EQUAL(str_error, error);
    free (error);

    /* existing file: content is returned and no error is set */
    file = fopen (path, "w");
    CHECK(file);
    fwrite (key, 1, strlen (key), file);
    fclose (file);
    error = NULL;
    content = irc_sasl_get_key_content (sasl_key, &error);
    STRCMP_EQUAL(NULL, error);
    STRCMP_EQUAL(key, content);
    free (content);

    /* same file, reached with the deprecated "%h" prefix */
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

    /* first answer: "alice\0alice" */
    error = NULL;
    str = irc_sasl_mechanism_ecdsa_nist256p_challenge (
        server, "+", "alice", "/does/not/exist.pem", &error);
    STRCMP_EQUAL(NULL, error);
    STRCMP_EQUAL("YWxpY2UAYWxpY2U=", str);
    free (str);

    /*
     * empty challenge: the base64 decoder in this branch is lenient and
     * silently skips invalid chars, so an empty result is the only decode
     * failure that can be detected here
     */
    error = NULL;
    str = irc_sasl_mechanism_ecdsa_nist256p_challenge (
        server, "", "alice", "/does/not/exist.pem", &error);
    STRCMP_EQUAL(NULL, str);
    STRCMP_EQUAL("base64 decode error", error);
    free (error);

    /* valid challenge, but the private key can not be read */
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
