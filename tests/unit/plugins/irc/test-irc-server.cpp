/*
 * test-irc-server.cpp - test IRC protocol functions
 *
 * Copyright (C) 2020-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/core-config-file.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-server.h"

extern int irc_server_fingerprint_search_algo_with_size (int size);
extern char *irc_server_eval_fingerprint (struct t_irc_server *server);
extern char *irc_server_build_autojoin (struct t_irc_server *server);
}

#define IRC_FAKE_SERVER "fake"

TEST_GROUP(IrcServer)
{
};

/*
 * Tests functions:
 *   irc_server_valid
 */

TEST(IrcServer, Valid)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");

    LONGS_EQUAL(0, irc_server_valid (NULL));
    LONGS_EQUAL(0, irc_server_valid ((struct t_irc_server *)0x1));
    LONGS_EQUAL(0, irc_server_valid (server + 1));

    LONGS_EQUAL(1, irc_server_valid (server));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_server_search
 */

TEST(IrcServer, Search)
{
    struct t_irc_server *server1, *server2;

    server1 = irc_server_alloc ("srv1");
    server2 = irc_server_alloc ("abc");

    POINTERS_EQUAL(NULL, irc_server_search (NULL));
    POINTERS_EQUAL(NULL, irc_server_search ("does_not_exist"));
    POINTERS_EQUAL(NULL, irc_server_search ("SRV1"));
    POINTERS_EQUAL(NULL, irc_server_search ("ABC"));

    POINTERS_EQUAL(server1, irc_server_search ("srv1"));
    POINTERS_EQUAL(server2, irc_server_search ("abc"));

    irc_server_free (server1);
    irc_server_free (server2);
}

/*
 * Tests functions:
 *   irc_server_search_option
 */

TEST(IrcServer, SearchOption)
{
    LONGS_EQUAL(-1, irc_server_search_option (NULL));
    LONGS_EQUAL(-1, irc_server_search_option (""));
    LONGS_EQUAL(-1, irc_server_search_option ("does_not_exist"));

    CHECK(irc_server_search_option ("addresses") >= 0);
    CHECK(irc_server_search_option ("ADDRESSES") >= 0);
    CHECK(irc_server_search_option ("autojoin") >= 0);
    CHECK(irc_server_search_option ("AUTOJOIN") >= 0);
}

/*
 * Tests functions:
 *   irc_server_search_casemapping
 */

TEST(IrcServer, SearchCasemapping)
{
    LONGS_EQUAL(-1, irc_server_search_casemapping (NULL));
    LONGS_EQUAL(-1, irc_server_search_casemapping (""));
    LONGS_EQUAL(-1, irc_server_search_casemapping ("does_not_exist"));

    CHECK(irc_server_search_casemapping ("rfc1459") >= 0);
    CHECK(irc_server_search_casemapping ("RFC1459") >= 0);
    CHECK(irc_server_search_casemapping ("strict-rfc1459") >= 0);
    CHECK(irc_server_search_casemapping ("STRICT-RFC1459") >= 0);
}

/*
 * Tests functions:
 *   irc_server_search_utf8mapping
 */

TEST(IrcServer, SearchUtf8mapping)
{
    LONGS_EQUAL(-1, irc_server_search_utf8mapping (NULL));
    LONGS_EQUAL(-1, irc_server_search_utf8mapping (""));
    LONGS_EQUAL(-1, irc_server_search_utf8mapping ("does_not_exist"));

    CHECK(irc_server_search_utf8mapping ("none") >= 0);
    CHECK(irc_server_search_utf8mapping ("NONE") >= 0);
    CHECK(irc_server_search_utf8mapping ("rfc8265") >= 0);
    CHECK(irc_server_search_utf8mapping ("RFC8265") >= 0);
}

/*
 * Tests functions:
 *   irc_server_strcasecmp
 *   irc_server_strncasecmp
 */

TEST(IrcServer, Strcasecmp)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, NULL, NULL));
    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, "", ""));

    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, NULL, NULL, 0));
    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "", "", 0));
    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, NULL, NULL, 1));
    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "", "", 1));

    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, "abc", "abc"));
    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, "abc", "ABC"));

    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "abc", "abc", 1));
    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "abc", "ABC", 1));

    LONGS_EQUAL(-19, irc_server_strcasecmp (NULL, "abc", "test"));
    LONGS_EQUAL(19, irc_server_strcasecmp (NULL, "test", "abc"));

    LONGS_EQUAL(-19, irc_server_strncasecmp (NULL, "abc", "test", 1));
    LONGS_EQUAL(19, irc_server_strncasecmp (NULL, "test", "abc", 1));

    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "atest", "abc", 1));

    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, "nick[a]", "nick{a}"));
    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, "nick^a", "nick~a"));

    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "nick[a]", "nick{a}", 10));
    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "nick^a", "nick~a", 10));

    LONGS_EQUAL(0, irc_server_strcasecmp (server, "nick[a]", "nick{a}"));
    LONGS_EQUAL(0, irc_server_strcasecmp (server, "nick^a", "nick~a"));

    LONGS_EQUAL(0, irc_server_strncasecmp (server, "nick[a]", "nick{a}", 10));
    LONGS_EQUAL(0, irc_server_strncasecmp (server, "nick^a", "nick~a", 10));

    server->casemapping = IRC_SERVER_CASEMAPPING_STRICT_RFC1459;

    LONGS_EQUAL(0, irc_server_strcasecmp (server, "nick[a]", "nick{a}"));
    LONGS_EQUAL(-32, irc_server_strcasecmp (server, "nick^a", "nick~a"));
    LONGS_EQUAL(32, irc_server_strcasecmp (server, "nick~a", "nick^a"));

    LONGS_EQUAL(0, irc_server_strncasecmp (server, "nick[a]", "nick{a}", 10));
    LONGS_EQUAL(-32, irc_server_strncasecmp (server, "nick^a", "nick~a", 10));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick~a", "nick^a", 10));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick_ô", "nick_Ô", 10));

    server->casemapping = IRC_SERVER_CASEMAPPING_ASCII;

    LONGS_EQUAL(-32, irc_server_strcasecmp (server, "nick[a]", "nick{a}"));
    LONGS_EQUAL(32, irc_server_strcasecmp (server, "nick{a}", "nick[a]"));
    LONGS_EQUAL(-32, irc_server_strcasecmp (server, "nick^a", "nick~a"));
    LONGS_EQUAL(32, irc_server_strcasecmp (server, "nick~a", "nick^a"));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick_ô", "nick_Ô", 10));

    LONGS_EQUAL(-32, irc_server_strncasecmp (server, "nick[a]", "nick{a}", 10));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick{a}", "nick[a]", 10));
    LONGS_EQUAL(-32, irc_server_strncasecmp (server, "nick^a", "nick~a", 10));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick~a", "nick^a", 10));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick_ô", "nick_Ô", 10));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_server_eval_expression
 */

TEST(IrcServer, EvalExpression)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("server1");
    CHECK(server);

    WEE_TEST_STR("", irc_server_eval_expression (NULL, "${server}"));
    WEE_TEST_STR("", irc_server_eval_expression (NULL, "${irc_server}"));
    WEE_TEST_STR("", irc_server_eval_expression (NULL, "${irc_server.name}"));

    WEE_TEST_STR("server1", irc_server_eval_expression (server, "${server}"));
    WEE_TEST_STR("server1", irc_server_eval_expression (server, "${irc_server.name}"));

    str = irc_server_eval_expression (server, "${irc_server}");
    STRNCMP_EQUAL("0x", str, 2);
    free (str);

    str = irc_server_eval_expression (server, "${username}");
    CHECK(str && str[0]);
    free (str);

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_server_fingerprint_search_algo_with_size
 */

TEST(IrcServer, FingerprintSearchAlgoWithSize)
{
    LONGS_EQUAL(-1, irc_server_fingerprint_search_algo_with_size (-1));
    LONGS_EQUAL(-1, irc_server_fingerprint_search_algo_with_size (0));
    LONGS_EQUAL(-1, irc_server_fingerprint_search_algo_with_size (-1));
    LONGS_EQUAL(-1, irc_server_fingerprint_search_algo_with_size (1024));

    LONGS_EQUAL(IRC_FINGERPRINT_ALGO_SHA1,
                irc_server_fingerprint_search_algo_with_size (160));
    LONGS_EQUAL(IRC_FINGERPRINT_ALGO_SHA256,
                irc_server_fingerprint_search_algo_with_size (256));
    LONGS_EQUAL(IRC_FINGERPRINT_ALGO_SHA512,
                irc_server_fingerprint_search_algo_with_size (512));
}

/*
 * Tests functions:
 *   irc_server_eval_fingerprint
 */

TEST(IrcServer, EvalFingerprint)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("server1");
    CHECK(server);

    STRCMP_EQUAL(NULL, irc_server_eval_fingerprint (NULL));

    WEE_TEST_STR("", irc_server_eval_fingerprint (server));

    /* invalid: evaluated to empty string */
    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
                            "${empty_value}", 1);
    STRCMP_EQUAL(NULL, irc_server_eval_fingerprint (server));

    /* invalid fingerprint value */
    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
                            "invalid", 1);
    STRCMP_EQUAL(NULL, irc_server_eval_fingerprint (server));

    /* invalid fingerprint value (same length as SHA-1) */
    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
                            "zzzz0aeb5ebce80ad5c201ebc358d616904czzzz", 1);
    STRCMP_EQUAL(NULL, irc_server_eval_fingerprint (server));

    /* valid SHA-1 fingerprint */
    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
                            "340b0aeb5ebce80ad5c201ebc358d616904ca84e", 1);
    WEE_TEST_STR("340b0aeb5ebce80ad5c201ebc358d616904ca84e",
                 irc_server_eval_fingerprint (server));

    /* valid SHA-256 fingerprint */
    config_file_option_set (
        server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
        "6a52951b8c2541c82bf11c83534631447dbae36b6576fe79fa6a5d3467eb3af9", 1);
    WEE_TEST_STR("6a52951b8c2541c82bf11c83534631447dbae36b6576fe79fa6a5d3467eb3af9",
                 irc_server_eval_fingerprint (server));

    /* valid SHA-256 fingerprint */
    config_file_option_set (
        server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
        "738c7bb821afe25b6be60386883bd8edb3e972bee442f7d75c01aa65155b5887"
        "c3512362e1008eb71cdd343449440b0ea0559b1e11743009ddf62ab1d3618ace", 1);
    WEE_TEST_STR(
        "738c7bb821afe25b6be60386883bd8edb3e972bee442f7d75c01aa65155b5887"
        "c3512362e1008eb71cdd343449440b0ea0559b1e11743009ddf62ab1d3618ace",
        irc_server_eval_fingerprint (server));

    /* valid SHA-1 + SHA-256 fingerprints */
    config_file_option_set (
        server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
        "340b0aeb5ebce80ad5c201ebc358d616904ca84e,"
        "6a52951b8c2541c82bf11c83534631447dbae36b6576fe79fa6a5d3467eb3af9", 1);
    WEE_TEST_STR(
        "340b0aeb5ebce80ad5c201ebc358d616904ca84e,"
        "6a52951b8c2541c82bf11c83534631447dbae36b6576fe79fa6a5d3467eb3af9",
        irc_server_eval_fingerprint (server));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_server_sasl_get_creds
 */

TEST(IrcServer, SaslGetCreds)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_sasl_enabled
 */

TEST(IrcServer, SaslEnabled)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_name_without_port
 */

TEST(IrcServer, GetNameWithoutPort)
{
    char *str;

    STRCMP_EQUAL(NULL, irc_server_get_name_without_port (NULL));

    WEE_TEST_STR("", irc_server_get_name_without_port (""));
    WEE_TEST_STR("example.com", irc_server_get_name_without_port ("example.com"));
    WEE_TEST_STR("example.com", irc_server_get_name_without_port ("example.com/6697"));
}

/*
 * Tests functions:
 *   irc_server_get_short_description
 *   irc_server_set_addresses
 */

TEST(IrcServer, SetAddresses)
{
    struct t_irc_server *server;
    char *str;

    STRCMP_EQUAL(NULL, irc_server_get_short_description (NULL));

    server = irc_server_alloc ("server1");

    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS], "off", 1);

    LONGS_EQUAL(0, irc_server_set_addresses (NULL, NULL, 0));
    LONGS_EQUAL(0, irc_server_set_addresses (NULL, "irc.example.org", 0));

    LONGS_EQUAL(1, irc_server_set_addresses (server, "fake:irc.fake.org", 0));
    LONGS_EQUAL(1, server->fake_server);
    STRCMP_EQUAL("irc.fake.org", server->addresses_eval);
    LONGS_EQUAL(1, server->addresses_count);
    STRCMP_EQUAL("irc.fake.org", server->addresses_array[0]);
    LONGS_EQUAL(6667, server->ports_array[0]);
    LONGS_EQUAL(0, server->retry_array[0]);

    WEE_TEST_STR("irc.fake.org/6667 (fake, TLS: disabled)",
                 irc_server_get_short_description (server));

    LONGS_EQUAL(1, irc_server_set_addresses (server, "irc.example.org", 0));
    LONGS_EQUAL(0, server->fake_server);
    STRCMP_EQUAL("irc.example.org", server->addresses_eval);
    LONGS_EQUAL(1, server->addresses_count);
    STRCMP_EQUAL("irc.example.org", server->addresses_array[0]);
    LONGS_EQUAL(6667, server->ports_array[0]);
    LONGS_EQUAL(0, server->retry_array[0]);

    WEE_TEST_STR("irc.example.org/6667 (TLS: disabled)",
                 irc_server_get_short_description (server));

    LONGS_EQUAL(1,
                irc_server_set_addresses (
                    server, "irc.example.org,irc2.example.org/6666", 0));
    LONGS_EQUAL(0, server->fake_server);
    STRCMP_EQUAL("irc.example.org,irc2.example.org/6666", server->addresses_eval);
    LONGS_EQUAL(2, server->addresses_count);
    STRCMP_EQUAL("irc.example.org", server->addresses_array[0]);
    STRCMP_EQUAL("irc2.example.org", server->addresses_array[1]);
    LONGS_EQUAL(6667, server->ports_array[0]);
    LONGS_EQUAL(6666, server->ports_array[1]);
    LONGS_EQUAL(0, server->retry_array[0]);
    LONGS_EQUAL(0, server->retry_array[1]);

    WEE_TEST_STR("irc.example.org/6667, irc2.example.org/6666 (TLS: disabled)",
                 irc_server_get_short_description (server));

    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS], "on", 1);

    LONGS_EQUAL(1,
                irc_server_set_addresses (
                    server, "irc.example.org,irc2.example.org/7000", 1));
    LONGS_EQUAL(0, server->fake_server);
    STRCMP_EQUAL("irc.example.org,irc2.example.org/7000", server->addresses_eval);
    LONGS_EQUAL(2, server->addresses_count);
    STRCMP_EQUAL("irc.example.org", server->addresses_array[0]);
    STRCMP_EQUAL("irc2.example.org", server->addresses_array[1]);
    LONGS_EQUAL(6697, server->ports_array[0]);
    LONGS_EQUAL(7000, server->ports_array[1]);
    LONGS_EQUAL(0, server->retry_array[0]);
    LONGS_EQUAL(0, server->retry_array[1]);

    WEE_TEST_STR("irc.example.org/6697, irc2.example.org/7000 (TLS: enabled)",
                 irc_server_get_short_description (server));

    LONGS_EQUAL(0,
                irc_server_set_addresses (
                    server, "irc.example.org,irc2.example.org/7000", 1));
    LONGS_EQUAL(0, server->fake_server);
    STRCMP_EQUAL("irc.example.org,irc2.example.org/7000", server->addresses_eval);
    LONGS_EQUAL(2, server->addresses_count);
    STRCMP_EQUAL("irc.example.org", server->addresses_array[0]);
    STRCMP_EQUAL("irc2.example.org", server->addresses_array[1]);
    LONGS_EQUAL(6697, server->ports_array[0]);
    LONGS_EQUAL(7000, server->ports_array[1]);
    LONGS_EQUAL(0, server->retry_array[0]);
    LONGS_EQUAL(0, server->retry_array[1]);

    WEE_TEST_STR("irc.example.org/6697, irc2.example.org/7000 (TLS: enabled)",
                 irc_server_get_short_description (server));

    server->temp_server = 1;

    WEE_TEST_STR("irc.example.org/6697, irc2.example.org/7000 "
                 "(temporary, TLS: enabled)",
                 irc_server_get_short_description (server));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_server_set_index_current_address
 */

TEST(IrcServer, SetIndexCurrentAddress)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_nicks
 */

TEST(IrcServer, SetNicks)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_nick
 */

TEST(IrcServer, SetNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_host
 */

TEST(IrcServer, SetHost)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_nick_index
 */

TEST(IrcServer, GetNickIndex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_alternate_nick
 */

TEST(IrcServer, GetAlternateNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_isupport_value
 */

TEST(IrcServer, GetIsupportValue)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("test_clienttagdeny");
    CHECK(server);

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup ("");

    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, NULL));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, ""));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TEST"));

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup ("AWAYLEN=307 BOT=B CASEMAPPING=ascii "
                               "CHANLIMIT=#:10 EMPTY= INVEX KICKLEN=307 WHOX");

    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, NULL));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, ""));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "xxx"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "AWAYLE"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "WHO"));

    STRCMP_EQUAL("307", irc_server_get_isupport_value (server, "AWAYLEN"));
    STRCMP_EQUAL("B", irc_server_get_isupport_value (server, "BOT"));
    STRCMP_EQUAL("ascii", irc_server_get_isupport_value (server, "CASEMAPPING"));
    STRCMP_EQUAL("#:10", irc_server_get_isupport_value (server, "CHANLIMIT"));
    STRCMP_EQUAL("", irc_server_get_isupport_value (server, "EMPTY"));
    STRCMP_EQUAL("", irc_server_get_isupport_value (server, "INVEX"));
    STRCMP_EQUAL("307", irc_server_get_isupport_value (server, "KICKLEN"));
    STRCMP_EQUAL("", irc_server_get_isupport_value (server, "WHOX"));

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup ("TEST SECOND");

    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "T"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TES"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "EST"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TESTT"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "SEC"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "COND"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "SECONDD"));

    STRCMP_EQUAL("", irc_server_get_isupport_value (server, "TEST"));
    STRCMP_EQUAL("", irc_server_get_isupport_value (server, "SECOND"));

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup ("TEST=abc");

    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "T"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TES"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "EST"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TESTT"));

    STRCMP_EQUAL("abc", irc_server_get_isupport_value (server, "TEST"));

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup ("  TEST=abc  ");

    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "T"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TES"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "EST"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TESTT"));

    STRCMP_EQUAL("abc", irc_server_get_isupport_value (server, "TEST"));

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_server_set_prefix_modes_chars
 */

TEST(IrcServer, SetPrefixModesChars)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_clienttagdeny
 */

TEST(IrcServer, SetClienttagdeny)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("test_clienttagdeny");
    CHECK(server);

    STRCMP_EQUAL(NULL, server->clienttagdeny);
    LONGS_EQUAL(0, server->clienttagdeny_count);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*");
    irc_server_set_clienttagdeny (server, NULL);
    STRCMP_EQUAL(NULL, server->clienttagdeny);
    LONGS_EQUAL(0, server->clienttagdeny_count);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*");
    irc_server_set_clienttagdeny (server, "");
    STRCMP_EQUAL(NULL, server->clienttagdeny);
    LONGS_EQUAL(0, server->clienttagdeny_count);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*");
    STRCMP_EQUAL("*", server->clienttagdeny);
    LONGS_EQUAL(1, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL(NULL, server->clienttagdeny_array[1]);
    LONGS_EQUAL(0, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*,-foo");
    STRCMP_EQUAL("*,-foo", server->clienttagdeny);
    LONGS_EQUAL(2, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL("!foo", server->clienttagdeny_array[1]);
    STRCMP_EQUAL(NULL, server->clienttagdeny_array[2]);
    LONGS_EQUAL(0, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*,-foo,-example/bar");
    STRCMP_EQUAL("*,-foo,-example/bar", server->clienttagdeny);
    LONGS_EQUAL(3, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL("!foo", server->clienttagdeny_array[1]);
    STRCMP_EQUAL("!example/bar", server->clienttagdeny_array[2]);
    STRCMP_EQUAL(NULL, server->clienttagdeny_array[3]);
    LONGS_EQUAL(0, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*,-foo,-example/bar,-typing");
    STRCMP_EQUAL("*,-foo,-example/bar,-typing", server->clienttagdeny);
    LONGS_EQUAL(4, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL("!foo", server->clienttagdeny_array[1]);
    STRCMP_EQUAL("!example/bar", server->clienttagdeny_array[2]);
    STRCMP_EQUAL("!typing", server->clienttagdeny_array[3]);
    STRCMP_EQUAL(NULL, server->clienttagdeny_array[4]);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_server_set_lag
 */

TEST(IrcServer, SetLag)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_modes
 */

TEST(IrcServer, GetPrefixModes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_chars
 */

TEST(IrcServer, GetPrefixChars)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_mode_index
 */

TEST(IrcServer, GetPrefixModeIndex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_char_index
 */

TEST(IrcServer, GetPrefixCharIndex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_mode_for_char
 */

TEST(IrcServer, GetPrefixModeForChar)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_prefix_char_for_mode
 */

TEST(IrcServer, GetPrefixCharForMode)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_chanmodes
 */

TEST(IrcServer, GetChanmodes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_prefix_char_statusmsg
 */

TEST(IrcServer, PrefixCharStatusmsg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_max_modes
 */

TEST(IrcServer, GetMaxModes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_default_msg
 */

TEST(IrcServer, GetDefaultMsg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_has_channels
 */

TEST(IrcServer, HasChannels)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_alloc
 */

TEST(IrcServer, Alloc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_alloc_with_url
 */

TEST(IrcServer, AllocWithUrl)
{
    struct t_irc_server *server;

    POINTERS_EQUAL(NULL, irc_server_alloc_with_url (NULL));
    POINTERS_EQUAL(NULL, irc_server_alloc_with_url (""));
    POINTERS_EQUAL(NULL, irc_server_alloc_with_url ("test"));
    POINTERS_EQUAL(NULL, irc_server_alloc_with_url ("test://irc.example.org"));

    /* address */
    server = irc_server_alloc_with_url ("irc://irc.example.org");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6667",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* address/port */
    server = irc_server_alloc_with_url ("irc://irc.example.org:7000");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/7000",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* address/port, IPv6 */
    server = irc_server_alloc_with_url ("irc6://irc.example.org:7000");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/7000",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* address, TLS */
    server = irc_server_alloc_with_url ("ircs://irc.example.org");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6697",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* address, IPv6, TLS */
    server = irc_server_alloc_with_url ("irc6s://irc.example.org");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6697",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* address/port, TLS */
    server = irc_server_alloc_with_url ("ircs://irc.example.org:7000");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/7000",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* address, nick */
    server = irc_server_alloc_with_url ("irc://alice@irc.example.org");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6667",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL("alice,alice2,alice3,alice4,alice5",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* address, nick, password */
    server = irc_server_alloc_with_url ("irc://alice:secret@irc.example.org");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6667",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL("secret",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL("alice,alice2,alice3,alice4,alice5",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* address, nick, password, channels */
    server = irc_server_alloc_with_url (
        "irc://alice:secret@irc.example.org/#test1,#test2");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6667",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL("secret",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL("alice,alice2,alice3,alice4,alice5",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL("#test1,#test2",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);
}


/*
 * Tests functions:
 *   irc_server_apply_command_line_options
 */

TEST(IrcServer, ApplyCommandLineOptions)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_add
 */

TEST(IrcServer, OutqueueAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_free
 */

TEST(IrcServer, OutqueueFree)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_free_all
 */

TEST(IrcServer, OutqueueFreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_free_data
 */

TEST(IrcServer, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_free
 */

TEST(IrcServer, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_free_all
 */

TEST(IrcServer, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_copy
 */

TEST(IrcServer, Copy)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_rename
 */

TEST(IrcServer, Rename)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_reorder
 */

TEST(IrcServer, Reorder)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_send_signal
 */

TEST(IrcServer, SendSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_send
 */

TEST(IrcServer, Send)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_send_default_tags
 */

TEST(IrcServer, SetSendDefaultTags)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_tags_to_send
 */

TEST(IrcServer, GetTagsToSend)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_all_empty
 */

TEST(IrcServer, OutqueueAllEmpty)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_timer_cb
 */

TEST(IrcServer, OutqueueTimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_timer_remove
 */

TEST(IrcServer, OutqueueTimerRemove)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_timer_add
 */

TEST(IrcServer, OutqueueTimerAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_send_one_msg
 */

TEST(IrcServer, OutqueueSendOneMsg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_outqueue_send
 */

TEST(IrcServer, OutqueueSend)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_send_one_msg
 */

TEST(IrcServer, SendOneMsg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_sendf
 */

TEST(IrcServer, Sendf)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_msgq_add_msg
 */

TEST(IrcServer, MsgqAddMsg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_msgq_add_unterminated
 */

TEST(IrcServer, MsgqAddUnterminated)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_msgq_add_buffer
 */

TEST(IrcServer, MsgqAddBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_msgq_flush
 */

TEST(IrcServer, MsgqFlush)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_recv_cb
 */

TEST(IrcServer, RecvCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_timer_connection_cb
 */

TEST(IrcServer, TimerConnectionCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_timer_sasl_cb
 */

TEST(IrcServer, TimerSaslCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_check_join_manual_cb
 */

TEST(IrcServer, CheckJoinManualCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_check_join_noswitch_cb
 */

TEST(IrcServer, CheckJoinNoswitchCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_check_join_smart_filtered_cb
 */

TEST(IrcServer, CheckJoinSmartFilteredCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_timer_cb
 */

TEST(IrcServer, TimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_close_connection
 */

TEST(IrcServer, CloseConnection)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_reconnect_schedule
 */

TEST(IrcServer, ReconnectSchedule)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_login
 */

TEST(IrcServer, Login)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_switch_address
 */

TEST(IrcServer, SwitchAddress)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_connect_cb
 */

TEST(IrcServer, ConnectCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_buffer_title
 */

TEST(IrcServer, SetBufferTitle)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_create_buffer
 */

TEST(IrcServer, CreateBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_fingerprint_str_sizes
 */

TEST(IrcServer, FingerprintStrSizes)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_compare_fingerprints
 */

TEST(IrcServer, CompareFingerprints)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_check_certificate_fingerprint
 */

TEST(IrcServer, CheckCertificateFingerprint)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_gnutls_callback
 */

TEST(IrcServer, GnutlsCallback)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_connect
 */

TEST(IrcServer, Connect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_reconnect
 */

TEST(IrcServer, Reconnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_auto_connect_timer_cb
 */

TEST(IrcServer, AutoConnectTimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_auto_connect
 */

TEST(IrcServer, AutoConnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_disconnect
 */

TEST(IrcServer, Disconnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_disconnect_all
 */

TEST(IrcServer, DisconnectAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_execute_command
 */

TEST(IrcServer, ExecuteCommand)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_autojoin_create_buffers
 */

TEST(IrcServer, AutojoinCreateBuffers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_autojoin_channels
 */

TEST(IrcServer, AutojoinChannels)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_channel_count
 */

TEST(IrcServer, GetChannelCount)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_get_pv_count
 */

TEST(IrcServer, GetPvCount)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_remove_away
 */

TEST(IrcServer, RemoveAway)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_check_away
 */

TEST(IrcServer, CheckAway)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_set_away
 */

TEST(IrcServer, SetAway)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_xfer_send_ready_cb
 */

TEST(IrcServer, XferSendReadyCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_xfer_resume_ready_cb
 */

TEST(IrcServer, XferResumeReadyCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_xfer_send_accept_resume_cb
 */

TEST(IrcServer, XferSendAcceptResumeCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_hdata_server_cb
 */

TEST(IrcServer, HdataServerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_add_to_infolist
 */

TEST(IrcServer, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_server_print_log
 */

TEST(IrcServer, PrintLog)
{
    /* TODO: write tests */
}

TEST_GROUP(IrcServerConnected)
{
    struct t_irc_server *ptr_server;

    void server_recv (const char *command)
    {
        char str_command[4096];

        snprintf (str_command, sizeof (str_command),
                  "/command -buffer irc.server." IRC_FAKE_SERVER " irc "
                  "/server fakerecv \"%s\"",
                  command);
        run_cmd_quiet (str_command);
    }

    void setup ()
    {
        /* create a fake server (no I/O) */
        run_cmd_quiet ("/mute /server add " IRC_FAKE_SERVER " fake:127.0.0.1 "
                       "-nicks=nick1,nick2,nick3");

        /* connect to the fake server */
        run_cmd_quiet ("/mute /connect " IRC_FAKE_SERVER);

        /* get the server pointer */
        ptr_server = irc_server_search (IRC_FAKE_SERVER);
    }

    void teardown ()
    {
        /* disconnect and delete the fake server */
        run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
        run_cmd_quiet ("/mute /server del " IRC_FAKE_SERVER);
        ptr_server = NULL;
    }
};

/*
 * Tests functions:
 *   irc_server_build_autojoin
 */

TEST(IrcServerConnected, BuildAutojoin)
{
    char *str;

    server_recv (":server 001 alice");

    STRCMP_EQUAL(NULL, irc_server_build_autojoin (ptr_server));

    /* join one channel */
    server_recv (":alice!user@host JOIN #test1");
    WEE_TEST_STR("#test1", irc_server_build_autojoin (ptr_server));

    /* simulate a "parted" channel */
    ptr_server->channels->part = 1;
    STRCMP_EQUAL(NULL, irc_server_build_autojoin (ptr_server));

    /* restore "part" flag */
    ptr_server->channels->part = 0;
    WEE_TEST_STR("#test1", irc_server_build_autojoin (ptr_server));

    /* add a key on channel */
    server_recv (":alice!user@host MODE #test1 +k key1");
    WEE_TEST_STR("#test1 key1",
                 irc_server_build_autojoin (ptr_server));

    /* remove key */
    server_recv (":alice!user@host MODE #test1 -k");
    WEE_TEST_STR("#test1",
                 irc_server_build_autojoin (ptr_server));

    /* join a second channel */
    server_recv (":alice!user@host JOIN #test2");
    WEE_TEST_STR("#test1,#test2", irc_server_build_autojoin (ptr_server));

    /* join a third channel */
    server_recv (":alice!user@host JOIN #test3");
    WEE_TEST_STR("#test1,#test2,#test3",
                 irc_server_build_autojoin (ptr_server));

    /* add key on first channel */
    server_recv (":alice!user@host MODE #test1 +k key1");
    WEE_TEST_STR("#test1,#test2,#test3 key1",
                 irc_server_build_autojoin (ptr_server));

    /* add key on second channel */
    server_recv (":alice!user@host MODE #test2 +k key2");
    WEE_TEST_STR("#test1,#test2,#test3 key1,key2",
                 irc_server_build_autojoin (ptr_server));

    /* add key on third channel */
    server_recv (":alice!user@host MODE #test3 +k key3");
    WEE_TEST_STR("#test1,#test2,#test3 key1,key2,key3",
                 irc_server_build_autojoin (ptr_server));

    /* remove key from first channel */
    server_recv (":alice!user@host MODE #test1 -k");
    WEE_TEST_STR("#test2,#test3,#test1 key2,key3",
                 irc_server_build_autojoin (ptr_server));

    /* remove key from second channel */
    server_recv (":alice!user@host MODE #test2 -k");
    WEE_TEST_STR("#test3,#test1,#test2 key3",
                 irc_server_build_autojoin (ptr_server));

    /* remove key from third channel */
    server_recv (":alice!user@host MODE #test3 -k");
    WEE_TEST_STR("#test1,#test2,#test3",
                 irc_server_build_autojoin (ptr_server));
}
