/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR configuration options */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include "src/core/core-config-file.h"
#include "src/plugins/plugin.h"
#include "src/plugins/otr/otr-config.h"
}

TEST_GROUP(OtrConfig)
{
};

/*
 * Test functions:
 *   otr_config_policy_create_option_cb
 *   otr_config_policy_delete_option_cb
 */

TEST(OtrConfig, PolicyCreateDelete)
{
    struct t_config_option *ptr_option;

    /* Default policy is reset (not deleted) by /unset. */
    config_file_search_with_string ("otr.policy.default.allow_v2", NULL, NULL,
                                    &ptr_option, NULL);
    CHECK(ptr_option);
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option, "off", 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_UNSET_OK_RESET,
                config_file_option_unset (ptr_option));
    config_file_search_with_string ("otr.policy.default.allow_v2", NULL, NULL,
                                    &ptr_option, NULL);
    CHECK(ptr_option);
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET,
                config_file_option_unset (ptr_option));

    /* Other policy is created then deleted by /unset. */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_with_string (
                    "otr.policy.test.alice.bob.send_tag", "on"));
    config_file_search_with_string ("otr.policy.test.alice.bob.send_tag",
                                    NULL, NULL, &ptr_option, NULL);
    CHECK(ptr_option);
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED,
                config_file_option_unset (ptr_option));
    config_file_search_with_string ("otr.policy.test.alice.bob.send_tag",
                                    NULL, NULL, &ptr_option, NULL);
    POINTERS_EQUAL(NULL, ptr_option);

    /* Other policy with an invalid value is not created. */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_with_string (
                    "otr.policy.test.alice.bob.send_tag", "invalid"));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_with_string (
                    "otr.policy.test.alice.bob.send_tag", "ON"));
    config_file_search_with_string ("otr.policy.test.alice.bob.send_tag",
                                    NULL, NULL, &ptr_option, NULL);
    POINTERS_EQUAL(NULL, ptr_option);
}

/*
 * Test functions:
 *   otr_config_compile_regex_no_send_tag
 *   otr_config_free_regex_no_send_tag
 *   otr_config_change_no_send_tag_regex_cb
 */

TEST(OtrConfig, RegexNoSendTag)
{
    /* Default regex is compiled. */
    CHECK(otr_config_regex_no_send_tag);

    /* Empty regex: no restriction. */
    config_file_option_set (otr_config_general_no_send_tag_regex, "", 1);
    POINTERS_EQUAL(NULL, otr_config_regex_no_send_tag);

    /* Invalid regex: no restriction. */
    config_file_option_set (otr_config_general_no_send_tag_regex, "(", 1);
    POINTERS_EQUAL(NULL, otr_config_regex_no_send_tag);

    config_file_option_set (otr_config_general_no_send_tag_regex, "^bob$", 1);
    CHECK(otr_config_regex_no_send_tag);

    config_file_option_reset (otr_config_general_no_send_tag_regex, 1);
    CHECK(otr_config_regex_no_send_tag);
}
