/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR policy functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/plugins/plugin.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-config.h"
#include "src/plugins/otr/otr-policy.h"
}

TEST_GROUP(OtrPolicy)
{
};

/*
 * Test functions:
 *   otr_policy_for
 */

TEST(OtrPolicy, PolicyFor)
{
    struct t_config_option *ptr_option;
    const char *options[] = {
        "otr.policy.test.allow_v2",
        "otr.policy.test.alice.allow_v2",
        "otr.policy.test.alice.bob.allow_v2",
        NULL,
    };
    int i;

    /* Default policies. */
    LONGS_EQUAL(1, otr_policy_for ("alice@test", "bob", "allow_v2"));
    LONGS_EQUAL(1, otr_policy_for ("alice@test", "bob", "html_filter"));
    LONGS_EQUAL(0, otr_policy_for ("alice@test", "bob", "send_tag"));
    LONGS_EQUAL(0, otr_policy_for ("alice@test", "bob", "unknown"));

    /* The most specific option wins: peer, nick, server, default. */
    config_file_option_set_with_string ("otr.policy.test.allow_v2", "off");
    LONGS_EQUAL(0, otr_policy_for ("alice@test", "bob", "allow_v2"));
    LONGS_EQUAL(1, otr_policy_for ("alice@other", "bob", "allow_v2"));
    config_file_option_set_with_string ("otr.policy.test.alice.allow_v2",
                                        "on");
    LONGS_EQUAL(1, otr_policy_for ("alice@test", "bob", "allow_v2"));
    LONGS_EQUAL(0, otr_policy_for ("carol@test", "bob", "allow_v2"));
    config_file_option_set_with_string ("otr.policy.test.alice.bob.allow_v2",
                                        "off");
    LONGS_EQUAL(0, otr_policy_for ("alice@test", "bob", "allow_v2"));
    LONGS_EQUAL(1, otr_policy_for ("alice@test", "dave", "allow_v2"));

    for (i = 0; options[i]; i++)
    {
        config_file_search_with_string (options[i], NULL, NULL, &ptr_option,
                                        NULL);
        CHECK(ptr_option);
        config_file_option_unset (ptr_option);
    }
    LONGS_EQUAL(1, otr_policy_for ("alice@test", "bob", "allow_v2"));
}

/*
 * Test functions:
 *   otr_policy_query
 */

TEST(OtrPolicy, Query)
{
    STRCMP_EQUAL("?OTRv23?", otr_policy_query ("alice@test", "bob"));

    config_file_option_set_with_string ("otr.policy.default.allow_v2", "off");
    STRCMP_EQUAL("?OTRv3?", otr_policy_query ("alice@test", "bob"));

    config_file_option_set_with_string ("otr.policy.default.allow_v3", "off");
    POINTERS_EQUAL(NULL, otr_policy_query ("alice@test", "bob"));

    config_file_option_set_with_string ("otr.policy.default.allow_v2", "on");
    STRCMP_EQUAL("?OTRv2?", otr_policy_query ("alice@test", "bob"));

    config_file_option_set_with_string ("otr.policy.default.allow_v3", "on");
    STRCMP_EQUAL("?OTRv23?", otr_policy_query ("alice@test", "bob"));
}

/*
 * Test functions:
 *   otr_policy_no_send_tag
 */

TEST(OtrPolicy, NoSendTag)
{
    /* Default regex: "^(alis|chanfix|global|.+serv|\*.+)$" */
    LONGS_EQUAL(0, otr_policy_no_send_tag ("bob"));
    LONGS_EQUAL(0, otr_policy_no_send_tag ("serv"));
    LONGS_EQUAL(1, otr_policy_no_send_tag ("alis"));
    LONGS_EQUAL(1, otr_policy_no_send_tag ("NickServ"));
    LONGS_EQUAL(1, otr_policy_no_send_tag ("chanserv"));
    LONGS_EQUAL(1, otr_policy_no_send_tag ("*status"));

    config_file_option_set (otr_config_general_no_send_tag_regex, "", 1);
    LONGS_EQUAL(0, otr_policy_no_send_tag ("nickserv"));

    config_file_option_reset (otr_config_general_no_send_tag_regex, 1);
    LONGS_EQUAL(1, otr_policy_no_send_tag ("nickserv"));
}

/*
 * Test functions:
 *   otr_policy_format_default
 *   otr_policy_format_peer
 */

TEST(OtrPolicy, Format)
{
    char *str;

    str = otr_policy_format_default ();
    CHECK(str);
    CHECK(strstr (str, "  allow_v2 ("));
    CHECK(strstr (str, ") : on\n"));
    CHECK(strstr (str, "/otr policy default NAME on|off"));
    free (str);

    str = otr_policy_format_peer ("alice@test", "bob");
    CHECK(str);
    CHECK(strstr (str, "bob@test"));
    CHECK(strstr (str, "  send_tag ("));
    CHECK(strstr (str, ") : off\n"));
    free (str);
}
