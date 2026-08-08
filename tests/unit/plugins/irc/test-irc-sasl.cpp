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
#include <string.h>
#include "src/core/core-string.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-sasl.h"
#include "src/plugins/irc/irc-server.h"
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
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_sasl_mechanism_ecdsa_nist256p_challenge
 */

TEST(IrcSasl, MechanismEcdsaNist256pChallenge)
{
    /* TODO: write tests */
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
