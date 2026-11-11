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

extern int otr_config_policy_value_valid (const char *value);
extern int otr_config_policy_inherited (struct t_config_file *config_file,
                                        struct t_config_section *section,
                                        const char *option_name);
}

TEST_GROUP(OtrConfig)
{
    struct t_config_option *option (const char *name)
    {
        struct t_config_option *ptr_option;

        ptr_option = NULL;
        config_file_search_with_string (name, NULL, NULL, &ptr_option, NULL);
        return ptr_option;
    }
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

/*
 * Test functions:
 *   otr_config_policy_inherited
 *   otr_config_policy_create_option_cb (value "toggle")
 */

TEST(OtrConfig, PolicyToggle)
{
    struct t_config_option *ptr_option;

    config_file_option_set_with_string ("otr.policy.default.log", "on");

    /* Value inherited from upper levels. */
    LONGS_EQUAL(0, otr_config_policy_inherited (otr_config_file,
                                                otr_config_section_policy,
                                                "log"));
    LONGS_EQUAL(1, otr_config_policy_inherited (otr_config_file,
                                                otr_config_section_policy,
                                                "tst.log"));
    LONGS_EQUAL(1, otr_config_policy_inherited (otr_config_file,
                                                otr_config_section_policy,
                                                "tst.alice.bob.log"));
    config_file_option_set_with_string ("otr.policy.tst.alice.log", "off");
    LONGS_EQUAL(0, otr_config_policy_inherited (otr_config_file,
                                                otr_config_section_policy,
                                                "tst.alice.bob.log"));
    LONGS_EQUAL(1, otr_config_policy_inherited (otr_config_file,
                                                otr_config_section_policy,
                                                "tst.alice2.bob.log"));

    /* Toggle a missing option: opposite of the value inherited. */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_with_string (
                    "otr.policy.tst.alice.bob.log", "toggle"));
    ptr_option = option ("otr.policy.tst.alice.bob.log");
    CHECK(ptr_option);
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_with_string (
                    "otr.policy.tst.alice2.bob.log", "toggle"));
    ptr_option = option ("otr.policy.tst.alice2.bob.log");
    CHECK(ptr_option);
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option));

    /* Toggle an existing option. */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_with_string (
                    "otr.policy.tst.alice2.bob.log", "toggle"));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option));

    config_file_option_unset (option ("otr.policy.tst.alice.bob.log"));
    config_file_option_unset (option ("otr.policy.tst.alice2.bob.log"));
    config_file_option_unset (option ("otr.policy.tst.alice.log"));
    config_file_option_unset (option ("otr.policy.default.log"));
}

/*
 * Test functions:
 *   otr_config_policy_value_valid
 */

TEST(OtrConfig, PolicyValueValid)
{
    LONGS_EQUAL(0, otr_config_policy_value_valid (NULL));
    LONGS_EQUAL(0, otr_config_policy_value_valid (""));
    LONGS_EQUAL(0, otr_config_policy_value_valid ("enabled"));
    LONGS_EQUAL(0, otr_config_policy_value_valid ("ON"));
    LONGS_EQUAL(1, otr_config_policy_value_valid ("on"));
    LONGS_EQUAL(1, otr_config_policy_value_valid ("off"));
    LONGS_EQUAL(1, otr_config_policy_value_valid ("yes"));
    LONGS_EQUAL(1, otr_config_policy_value_valid ("0"));
    LONGS_EQUAL(1, otr_config_policy_value_valid ("toggle"));
}
