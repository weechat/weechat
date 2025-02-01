/*
 * test-plugin-config.cpp - tests plugins config functions
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

extern "C"
{
#include "src/core/core-config-file.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/plugin-config.h"
}

TEST_GROUP(PluginConfig)
{
};

/*
 * Tests functions:
 *   plugin_config_search
 *   plugin_config_set_internal
 *   plugin_config_set
 *   plugin_config_desc_changed_cb
 *   plugin_config_set_desc_internal
 *   plugin_config_set_desc
 */

TEST(PluginConfig, Set)
{
    struct t_config_option *ptr_option, *ptr_option_desc;

    POINTERS_EQUAL(NULL, plugin_config_search (NULL, NULL));
    POINTERS_EQUAL(NULL, plugin_config_search ("python", NULL));
    POINTERS_EQUAL(NULL, plugin_config_search ("python", "test"));

    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                plugin_config_set ("python", "test", "the old value"));
    ptr_option = plugin_config_search ("python", "test");
    STRCMP_EQUAL("the old value", CONFIG_STRING(ptr_option));

    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                plugin_config_set ("python", "test", "the new value"));
    ptr_option = plugin_config_search ("python", "test");
    STRCMP_EQUAL("the new value", CONFIG_STRING(ptr_option));

    config_file_search_with_string ("plugins.desc.python.test",
                                    NULL, NULL, &ptr_option_desc, NULL);
    POINTERS_EQUAL(NULL, ptr_option_desc);

    plugin_config_set_desc ("python", "test", "the old description");
    config_file_search_with_string ("plugins.desc.python.test",
                                    NULL, NULL, &ptr_option_desc, NULL);
    CHECK(ptr_option_desc);
    STRCMP_EQUAL("the old description", CONFIG_STRING(ptr_option_desc));

    plugin_config_set_desc ("python", "test", "the new description");
    config_file_search_with_string ("plugins.desc.python.test",
                                    NULL, NULL, &ptr_option_desc, NULL);
    CHECK(ptr_option_desc);
    STRCMP_EQUAL("the new description", CONFIG_STRING(ptr_option_desc));

    config_file_option_free (ptr_option_desc, 1);
    config_file_search_with_string ("plugins.desc.python.test",
                                    NULL, NULL, &ptr_option_desc, NULL);
    POINTERS_EQUAL(NULL, ptr_option_desc);

    config_file_option_free (ptr_option, 1);
    config_file_search_with_string ("plugins.var.python.test",
                                    NULL, NULL, &ptr_option, NULL);
    POINTERS_EQUAL(NULL, ptr_option);
}

/*
 * Tests functions:
 *   plugin_config_reload
 */

TEST(PluginConfig, Reload)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   plugin_config_create_option
 */

TEST(PluginConfig, CreateOption)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   plugin_config_create_desc
 */

TEST(PluginConfig, CreateDesc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   plugin_config_delete_desc
 */

TEST(PluginConfig, DeleteDesc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   plugin_config_init
 */

TEST(PluginConfig, Init)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   plugin_config_read
 */

TEST(PluginConfig, Read)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   plugin_config_write
 */

TEST(PluginConfig, Write)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   plugin_config_end
 */

TEST(PluginConfig, End)
{
    /* TODO: write tests */
}
