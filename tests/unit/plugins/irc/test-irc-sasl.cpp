/*
 * test-irc-sasl.cpp - test IRC SASL functions
 *
 * Copyright (C) 2021-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * Tests functions:
 *   irc_sasl_mechanism_plain
 */

TEST(IrcSasl, MechanismPlain)
{
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_plain (NULL, NULL));
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_plain (NULL, ""));
    POINTERS_EQUAL(NULL, irc_sasl_mechanism_plain ("", NULL));

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
    char *str, str_decoded[1024], *error;

    POINTERS_EQUAL(NULL, irc_sasl_mechanism_scram (NULL, NULL, NULL, NULL,
                                                   NULL, NULL));

    server = irc_server_alloc ("my_ircd");

    /* decoded returned value is like: n,,n=user,r=rOprNGfwEbeRWgbNEkqO */
    error = NULL;
    str = irc_sasl_mechanism_scram (server, "sha256", "+",
                                    "user1", "secret", &error);
    POINTERS_EQUAL(NULL, error);
    CHECK(string_base64_decode (0, str, str_decoded) > 0);
    CHECK(strncmp (str_decoded, "n,,n=user1,r=", 13) == 0);
    free (str);

    /* TODO: complete tests */

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
