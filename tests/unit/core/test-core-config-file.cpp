/*
 * test-core-config-file.cpp - test configuration file functions
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

extern "C"
{
#include <string.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-config-file.h"
#include "src/core/core-config.h"
#include "src/core/core-secure-config.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"
#include "src/plugins/plugin-config.h"

extern struct t_config_file *config_file_find_pos (const char *name);
extern char *config_file_option_full_name (struct t_config_option *option);
extern int config_file_string_boolean_is_valid (const char *text);
extern const char *config_file_option_escape (const char *name);
}

struct t_config_option *ptr_option_bool = NULL;
struct t_config_option *ptr_option_bool_child = NULL;
struct t_config_option *ptr_option_int = NULL;
struct t_config_option *ptr_option_int_child = NULL;
struct t_config_option *ptr_option_int_str = NULL;
struct t_config_option *ptr_option_int_str_child = NULL;
struct t_config_option *ptr_option_str = NULL;
struct t_config_option *ptr_option_str_child = NULL;
struct t_config_option *ptr_option_col = NULL;
struct t_config_option *ptr_option_col_child = NULL;
struct t_config_option *ptr_option_enum = NULL;
struct t_config_option *ptr_option_enum_child = NULL;

TEST_GROUP(CoreConfigFile)
{
};

TEST_GROUP(CoreConfigFileWithNewOptions)
{
    static int option_str_check_cb (const void *pointer,
                                    void *data,
                                    struct t_config_option *option,
                                    const char *value)
    {
        (void) pointer;
        (void) data;
        (void) option;

        return ((strcmp (value, "xxx") == 0) || (strcmp (value, "zzz") == 0)) ?
            0 : 1;
    }

    void setup ()
    {
        ptr_option_bool = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_boolean", "boolean", "", NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_bool_child = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_boolean_child << weechat.look.test_boolean",
            "boolean", "", NULL, 0, 0, NULL, NULL, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_int = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_integer", "integer", "", NULL, 0, 123456, "100", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_int_child = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_integer_child << weechat.look.test_integer",
            "integer", "", NULL, 0, 123456, NULL, NULL, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        /* auto-created as enum with WeeChat >= 4.1.0 */
        ptr_option_int_str = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_integer_values", "integer", "", "v1|v2|v3", 0, 0, "v2", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_int_str_child = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_integer_values_child << weechat.look.test_integer_values",
            "integer", "", "v1|v2|v3", 0, 0, NULL, NULL, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_str = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_string", "string", "", NULL, 0, 0, "value", NULL, 0,
            &option_str_check_cb, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_str_child = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_string_child << weechat.look.test_string",
            "string", "", NULL, 0, 0, NULL, NULL, 1,
            &option_str_check_cb, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_col = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "test_color", "color", "", NULL, 0, 0, "blue", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_col_child = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "test_color_child << weechat.color.test_color",
            "color", "", NULL, 0, 0, NULL, NULL, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_enum = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_enum", "enum", "", "v1|v2|v3", 0, 0, "v2", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        ptr_option_enum_child = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "test_enum_child << weechat.look.test_enum",
            "enum", "", "v1|v2|v3", 0, 0, NULL, NULL, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
    }

    void teardown ()
    {
        config_file_option_free (ptr_option_bool, 0);
        ptr_option_bool = NULL;
        config_file_option_free (ptr_option_bool_child, 0);
        ptr_option_bool_child = NULL;
        config_file_option_free (ptr_option_int, 0);
        ptr_option_int = NULL;
        config_file_option_free (ptr_option_int_child, 0);
        ptr_option_int_child = NULL;
        config_file_option_free (ptr_option_int_str, 0);
        ptr_option_int_str = NULL;
        config_file_option_free (ptr_option_int_str_child, 0);
        ptr_option_int_str_child = NULL;
        config_file_option_free (ptr_option_str, 0);
        ptr_option_str = NULL;
        config_file_option_free (ptr_option_str_child, 0);
        ptr_option_str_child = NULL;
        config_file_option_free (ptr_option_col, 0);
        ptr_option_col = NULL;
        config_file_option_free (ptr_option_col_child, 0);
        ptr_option_col_child = NULL;
        config_file_option_free (ptr_option_enum, 0);
        ptr_option_enum = NULL;
        config_file_option_free (ptr_option_enum_child, 0);
        ptr_option_enum_child = NULL;
    }
};

/*
 * Tests functions:
 *   config_file_valid
 */

TEST(CoreConfigFile, Valid)
{
    LONGS_EQUAL(0, config_file_valid (NULL));
    LONGS_EQUAL(0, config_file_valid ((struct t_config_file *)0x1));

    LONGS_EQUAL(1, config_file_valid (config_file_search ("weechat")));
    LONGS_EQUAL(1, config_file_valid (config_file_search ("sec")));
}

/*
 * Tests functions:
 *   config_file_search
 */

TEST(CoreConfigFile, Search)
{
    POINTERS_EQUAL(NULL, config_file_search (NULL));
    POINTERS_EQUAL(NULL, config_file_search (""));
    POINTERS_EQUAL(NULL, config_file_search ("zzz"));

    POINTERS_EQUAL(weechat_config_file, config_file_search ("weechat"));
    POINTERS_EQUAL(secure_config_file, config_file_search ("sec"));
}

/*
 * Tests functions:
 *   config_file_find_pos
 */

TEST(CoreConfigFile, FindPos)
{
    POINTERS_EQUAL(NULL, config_file_find_pos (NULL));
    POINTERS_EQUAL(config_files, config_file_find_pos (""));
    POINTERS_EQUAL(weechat_config_file->next_config, config_file_find_pos ("weechat2"));
    POINTERS_EQUAL(config_files, config_file_find_pos ("WEECHAT2"));
}

/*
 * Tests functions:
 *   config_file_config_insert
 */

TEST(CoreConfigFile, ConfigInsert)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_new
 */

TEST(CoreConfigFile, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_set_version
 */

TEST(CoreConfigFile, SetVersion)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_arraylist_cmp_config_cb
 *   config_file_get_configs_by_priority
 */

TEST(CoreConfigFile, GetConfigsByPriority)
{
    struct t_config_file *ptr_config;
    struct t_arraylist *all_configs;
    int config_count;

    /* count number of configuration files */
    config_count = 0;
    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        config_count++;
    }

    /* get list of configuration files by priority (highest to lowest) */
    all_configs = config_file_get_configs_by_priority ();
    CHECK(all_configs);

    /* ensure we have all files in the list (and not more) */
    LONGS_EQUAL(config_count, arraylist_size (all_configs));

    /* check core configuration files (they have higher priority) */
    POINTERS_EQUAL(secure_config_file, arraylist_get (all_configs, 0));
    POINTERS_EQUAL(weechat_config_file, arraylist_get (all_configs, 1));
    POINTERS_EQUAL(plugin_config_file, arraylist_get (all_configs, 2));

    /* check first plugin configuration file (with highest priority) */
    ptr_config = (struct t_config_file *)arraylist_get (all_configs, 3);
    CHECK(ptr_config);
    STRCMP_EQUAL("charset", ptr_config->name);

    /* check last plugin configuration file (with lowest priority) */
    ptr_config = (struct t_config_file *)arraylist_get (all_configs,
                                                        config_count - 1);
    CHECK(ptr_config);
    STRCMP_EQUAL("fset", ptr_config->name);

    arraylist_free (all_configs);
}

/*
 * Tests functions:
 *   config_file_section_find_pos
 */

TEST(CoreConfigFile, SectionFindPos)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_new_section
 */

TEST(CoreConfigFile, NewSection)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_search_section
 */

TEST(CoreConfigFile, SearchSection)
{
    POINTERS_EQUAL(NULL, config_file_search_section (NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_search_section (weechat_config_file, NULL));
    POINTERS_EQUAL(NULL, config_file_search_section (weechat_config_file, "zzz"));

    POINTERS_EQUAL(weechat_config_section_proxy,
                   config_file_search_section (weechat_config_file, "proxy"));
}

/*
 * Tests functions:
 *   config_file_option_full_name
 */

TEST(CoreConfigFile, OptionFullName)
{
    char *str;

    STRCMP_EQUAL(NULL, config_file_option_full_name (NULL));

    WEE_TEST_STR("weechat.look.buffer_time_format",
                 config_file_option_full_name (config_look_buffer_time_format));
}

/*
 * Tests functions:
 *   config_file_hook_config_exec
 */

TEST(CoreConfigFile, HookConfigExec)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_find_pos
 */

TEST(CoreConfigFile, OptionFindPos)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_insert_in_section
 */

TEST(CoreConfigFile, OptionInsertInSection)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_malloc
 */

TEST(CoreConfigFile, OptionMalloc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_new_option
 */

TEST(CoreConfigFile, NewOption)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_search_option
 */

TEST(CoreConfigFile, SearchOption)
{
    POINTERS_EQUAL(NULL, config_file_search_option (NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, config_file_search_option (weechat_config_file,
                                                    NULL, NULL));
    POINTERS_EQUAL(NULL,
                   config_file_search_option (weechat_config_file,
                                              weechat_config_section_color,
                                              NULL));

    POINTERS_EQUAL(NULL,
                   config_file_search_option (weechat_config_file,
                                              weechat_config_section_color,
                                              "xxx"));
    POINTERS_EQUAL(NULL,
                   config_file_search_option (weechat_config_file,
                                              NULL,
                                              "xxx"));
    POINTERS_EQUAL(NULL,
                   config_file_search_option (NULL,
                                              weechat_config_section_color,
                                              "xxx"));

    POINTERS_EQUAL(config_color_chat_channel,
                   config_file_search_option (weechat_config_file,
                                              weechat_config_section_color,
                                              "chat_channel"));
    POINTERS_EQUAL(config_color_chat_channel,
                   config_file_search_option (weechat_config_file,
                                              NULL,
                                              "chat_channel"));
    POINTERS_EQUAL(config_color_chat_channel,
                   config_file_search_option (NULL,
                                              weechat_config_section_color,
                                              "chat_channel"));
}

/*
 * Tests functions:
 *   config_file_search_section_option
 */

TEST(CoreConfigFile, SearchSectionOption)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (NULL, NULL, NULL,
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (weechat_config_file, NULL, NULL,
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (weechat_config_file,
                                       weechat_config_section_color,
                                       NULL,
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (weechat_config_file,
                                       weechat_config_section_color,
                                       "xxx",
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (weechat_config_file,
                                       weechat_config_section_color,
                                       "chat_channel",
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(weechat_config_section_color, ptr_section);
    POINTERS_EQUAL(config_color_chat_channel, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (weechat_config_file,
                                       NULL,
                                       "chat_channel",
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(weechat_config_section_color, ptr_section);
    POINTERS_EQUAL(config_color_chat_channel, ptr_option);

    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    config_file_search_section_option (NULL,
                                       weechat_config_section_color,
                                       "chat_channel",
                                       &ptr_section, &ptr_option);
    POINTERS_EQUAL(weechat_config_section_color, ptr_section);
    POINTERS_EQUAL(config_color_chat_channel, ptr_option);
}

/*
 * Tests functions:
 *   config_file_search_with_string
 */

TEST(CoreConfigFile, SearchWithString)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char *pos_option_name;

    ptr_config = (struct t_config_file *)0x1;
    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    pos_option_name = (char *)0x1;
    config_file_search_with_string (NULL, NULL, NULL, NULL, NULL);
    POINTERS_EQUAL(0x1, ptr_config);
    POINTERS_EQUAL(0x1, ptr_section);
    POINTERS_EQUAL(0x1, ptr_option);
    POINTERS_EQUAL(0x1, pos_option_name);

    ptr_config = (struct t_config_file *)0x1;
    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    pos_option_name = (char *)0x1;
    config_file_search_with_string (NULL, &ptr_config, &ptr_section,
                                    &ptr_option, &pos_option_name);
    POINTERS_EQUAL(NULL, ptr_config);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);
    POINTERS_EQUAL(NULL, pos_option_name);

    ptr_config = (struct t_config_file *)0x1;
    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    pos_option_name = (char *)0x1;
    config_file_search_with_string ("", &ptr_config, &ptr_section,
                                    &ptr_option, &pos_option_name);
    POINTERS_EQUAL(NULL, ptr_config);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);
    POINTERS_EQUAL(NULL, pos_option_name);

    ptr_config = (struct t_config_file *)0x1;
    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    pos_option_name = (char *)0x1;
    config_file_search_with_string ("zzz", &ptr_config, &ptr_section,
                                    &ptr_option, &pos_option_name);
    POINTERS_EQUAL(NULL, ptr_config);
    POINTERS_EQUAL(NULL, ptr_section);
    POINTERS_EQUAL(NULL, ptr_option);
    POINTERS_EQUAL(NULL, pos_option_name);

    ptr_config = (struct t_config_file *)0x1;
    ptr_section = (struct t_config_section *)0x1;
    ptr_option = (struct t_config_option *)0x1;
    pos_option_name = (char *)0x1;
    config_file_search_with_string ("weechat.color.chat_channel",
                                    &ptr_config, &ptr_section,
                                    &ptr_option, &pos_option_name);
    POINTERS_EQUAL(weechat_config_file, ptr_config);
    POINTERS_EQUAL(weechat_config_section_color, ptr_section);
    POINTERS_EQUAL(config_color_chat_channel, ptr_option);
    STRCMP_EQUAL("chat_channel", pos_option_name);
}

/*
 * Tests functions:
 *   config_file_string_boolean_is_valid
 */

TEST(CoreConfigFile, StringBooleanIsValid)
{
    LONGS_EQUAL(0, config_file_string_boolean_is_valid (NULL));
    LONGS_EQUAL(0, config_file_string_boolean_is_valid (""));
    LONGS_EQUAL(0, config_file_string_boolean_is_valid ("zzz"));

    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("on"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("yes"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("y"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("true"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("t"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("1"));

    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("off"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("no"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("n"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("false"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("f"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("0"));
}

/*
 * Tests functions:
 *   config_file_string_to_boolean
 */

TEST(CoreConfigFile, StringToBoolean)
{
    LONGS_EQUAL(0, config_file_string_to_boolean (NULL));
    LONGS_EQUAL(0, config_file_string_to_boolean (""));
    LONGS_EQUAL(0, config_file_string_to_boolean ("zzz"));

    LONGS_EQUAL(1, config_file_string_to_boolean ("on"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("yes"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("y"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("true"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("t"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("1"));

    LONGS_EQUAL(0, config_file_string_to_boolean ("off"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("no"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("n"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("false"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("f"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("0"));
}

/*
 * Tests functions:
 *   config_file_option_set
 *   config_file_option_reset
 */

TEST(CoreConfigFileWithNewOptions, OptionSetReset)
{
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_reset (NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (NULL, NULL, 1));

    /* boolean */
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_bool, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_bool, "on", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_bool, "toggle", 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_bool, "toggle", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_bool, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));

    /* integer */
    LONGS_EQUAL(100, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_int, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_int, "-500", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_int, "99999999", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_int, "50", 1));
    LONGS_EQUAL(50, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_int, "++15", 1));
    LONGS_EQUAL(65, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_int, "--3", 1));
    LONGS_EQUAL(62, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int, 1));
    LONGS_EQUAL(100, CONFIG_INTEGER(ptr_option_int));

    /* integer with string values (enum with WeeChat >= 4.1.0) */
    LONGS_EQUAL(1, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_int_str, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_int_str, "v3", 1));
    LONGS_EQUAL(2, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int_str, 1));
    LONGS_EQUAL(1, CONFIG_INTEGER(ptr_option_int_str));

    /* string */
    STRCMP_EQUAL("value", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_str, "xxx", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_str, "test", 1));
    STRCMP_EQUAL("test", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_str, 1));
    STRCMP_EQUAL("value", CONFIG_STRING(ptr_option_str));

    /* color */
    LONGS_EQUAL(9, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_col, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "red", 1));
    LONGS_EQUAL(3, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "++5", 1));
    LONGS_EQUAL(8, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "--3", 1));
    LONGS_EQUAL(5, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "%red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_BLINK_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, ".red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_DIM_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "*red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_BOLD_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "!red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_REVERSE_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "/red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_ITALIC_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "_red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_UNDERLINE_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "|red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_KEEPATTR_FLAG, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_col, "%.*!/_|red", 1));
    LONGS_EQUAL(3
                | GUI_COLOR_EXTENDED_BLINK_FLAG
                | GUI_COLOR_EXTENDED_DIM_FLAG
                | GUI_COLOR_EXTENDED_BOLD_FLAG
                | GUI_COLOR_EXTENDED_REVERSE_FLAG
                | GUI_COLOR_EXTENDED_ITALIC_FLAG
                | GUI_COLOR_EXTENDED_UNDERLINE_FLAG
                | GUI_COLOR_EXTENDED_KEEPATTR_FLAG,
                CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_col, 1));
    LONGS_EQUAL(9, CONFIG_COLOR(ptr_option_col));

    /* enum */
    LONGS_EQUAL(1, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (ptr_option_enum, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option_enum, "v3", 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_enum, 1));
    LONGS_EQUAL(1, CONFIG_INTEGER(ptr_option_enum));
}

/*
 * Tests functions:
 *   config_file_option_toggle
 */

TEST(CoreConfigFileWithNewOptions, OptionToggle)
{
    const char *value_boolean_ok[] = { "on", NULL };
    const char *values_boolean_ok[] = { "on", "off", NULL };
    const char *values_boolean_error[] = { "xxx", "zzz", NULL };
    const char *value_integer_ok[] = { "50", NULL };
    const char *values_integer_ok[] = { "75", "92", NULL };
    const char *values_integer_error[] = { "-500", "99999999", NULL };
    const char *value_integer_str_ok[] = { "v3", NULL };
    const char *values_integer_str_ok[] = { "v1", "v3", NULL };
    const char *values_integer_str_error[] = { "xxx", "zzz", NULL };
    const char *value_string_ok[] = { "+", NULL };
    const char *values_string_ok[] = { "$", "*", NULL };
    const char *values_string_error[] = { "xxx", "zzz", NULL };
    const char *value_color_ok[] = { "red", NULL };
    const char *values_color_ok[] = { "green", "cyan", NULL };
    const char *values_color_error[] = { "xxx", "zzz", NULL };

    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (NULL, NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_bool, NULL, -1, 1));

    /* boolean */
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_bool,
                                           values_boolean_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, NULL, 0, 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, NULL, 0, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, value_boolean_ok, 1, 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, value_boolean_ok, 1, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, values_boolean_ok, 2, 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_bool, values_boolean_ok, 2, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_reset (ptr_option_bool, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(ptr_option_bool));

    /* integer */
    LONGS_EQUAL(100, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_int,
                                           values_integer_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_int,
                                           NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int,
                                           value_integer_ok, 1, 1));
    LONGS_EQUAL(50, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int,
                                           value_integer_ok, 1, 1));
    LONGS_EQUAL(100, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int,
                                           values_integer_ok, 2, 1));
    LONGS_EQUAL(75, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int,
                                           values_integer_ok, 2, 1));
    LONGS_EQUAL(92, CONFIG_INTEGER(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int, 1));
    LONGS_EQUAL(100, CONFIG_INTEGER(ptr_option_int));

    /* integer with string values (enum with WeeChat >= 4.1.0) */
    LONGS_EQUAL(1, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_int_str,
                                           values_integer_str_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_int_str,
                                           NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int_str,
                                           value_integer_str_ok, 1, 1));
    LONGS_EQUAL(2, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int_str,
                                           values_integer_str_ok, 2, 1));
    LONGS_EQUAL(0, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_int_str,
                                           values_integer_str_ok, 2, 1));
    LONGS_EQUAL(2, CONFIG_INTEGER(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_int_str, 1));
    LONGS_EQUAL(1, CONFIG_INTEGER(ptr_option_int_str));

    /* string */
    STRCMP_EQUAL("value", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_str,
                                           values_string_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_str,
                                           NULL, 0, 1));
    STRCMP_EQUAL("", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_str,
                                           NULL, 0, 1));
    STRCMP_EQUAL("value", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_str,
                                           value_string_ok, 1, 1));
    STRCMP_EQUAL("+", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_str,
                                           values_string_ok, 2, 1));
    STRCMP_EQUAL("$", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_str,
                                           values_string_ok, 2, 1));
    STRCMP_EQUAL("*", CONFIG_STRING(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_str, 1));
    STRCMP_EQUAL("value", CONFIG_STRING(ptr_option_str));

    /* color */
    LONGS_EQUAL(9, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_col,
                                           values_color_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_col, NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_col,
                                           value_color_ok, 1, 1));
    LONGS_EQUAL(3, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_col,
                                           values_color_ok, 2, 1));
    LONGS_EQUAL(5, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_col,
                                           values_color_ok, 2, 1));
    LONGS_EQUAL(13, CONFIG_COLOR(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_col, 1));
    LONGS_EQUAL(9, CONFIG_COLOR(ptr_option_col));

    /* enum */
    LONGS_EQUAL(1, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_enum,
                                           values_integer_str_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (ptr_option_enum,
                                           NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_enum,
                                           value_integer_str_ok, 1, 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_enum,
                                           values_integer_str_ok, 2, 1));
    LONGS_EQUAL(0, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (ptr_option_enum,
                                           values_integer_str_ok, 2, 1));
    LONGS_EQUAL(2, CONFIG_ENUM(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (ptr_option_enum, 1));
    LONGS_EQUAL(1, CONFIG_ENUM(ptr_option_enum));
}

/*
 * Tests functions:
 *   config_file_option_set_null
 */

TEST(CoreConfigFile, OptionSetNull)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_set_default
 */

TEST(CoreConfigFileWithNewOptions, OptionSetDefault)
{
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (NULL, NULL, 1));

    /* boolean */
    LONGS_EQUAL(0, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_bool, NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_bool, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_bool, "on", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_bool, "toggle", 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_bool, "toggle", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN_DEFAULT(ptr_option_bool));

    /* integer */
    LONGS_EQUAL(100, CONFIG_INTEGER_DEFAULT(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_int, NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_int, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_int, "-500", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_int, "99999999", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_int, "50", 1));
    LONGS_EQUAL(50, CONFIG_INTEGER_DEFAULT(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_int, "++15", 1));
    LONGS_EQUAL(65, CONFIG_INTEGER_DEFAULT(ptr_option_int));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_int, "--3", 1));
    LONGS_EQUAL(62, CONFIG_INTEGER_DEFAULT(ptr_option_int));

    /* integer with string values (enum with WeeChat >= 4.1.0) */
    LONGS_EQUAL(1, CONFIG_INTEGER_DEFAULT(ptr_option_int_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_int_str, NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_int_str, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_int_str, "v3", 1));
    LONGS_EQUAL(2, CONFIG_INTEGER_DEFAULT(ptr_option_int_str));

    /* string */
    STRCMP_EQUAL("value", CONFIG_STRING_DEFAULT(ptr_option_str));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_str, "xxx", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_str, "test", 1));
    STRCMP_EQUAL("test", CONFIG_STRING_DEFAULT(ptr_option_str));

    /* color */
    LONGS_EQUAL(9, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_col, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "red", 1));
    LONGS_EQUAL(3, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "++5", 1));
    LONGS_EQUAL(8, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "--3", 1));
    LONGS_EQUAL(5, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "%red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_BLINK_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, ".red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_DIM_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "*red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_BOLD_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "!red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_REVERSE_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "/red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_ITALIC_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "_red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_UNDERLINE_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "|red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_KEEPATTR_FLAG, CONFIG_COLOR_DEFAULT(ptr_option_col));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_col, "%.*!/_|red", 1));
    LONGS_EQUAL(3
                | GUI_COLOR_EXTENDED_BLINK_FLAG
                | GUI_COLOR_EXTENDED_DIM_FLAG
                | GUI_COLOR_EXTENDED_BOLD_FLAG
                | GUI_COLOR_EXTENDED_REVERSE_FLAG
                | GUI_COLOR_EXTENDED_ITALIC_FLAG
                | GUI_COLOR_EXTENDED_UNDERLINE_FLAG
                | GUI_COLOR_EXTENDED_KEEPATTR_FLAG,
                CONFIG_COLOR_DEFAULT(ptr_option_col));

    /* enum */
    LONGS_EQUAL(1, CONFIG_ENUM_DEFAULT(ptr_option_enum));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_set_default (ptr_option_enum, NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set_default (ptr_option_enum, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set_default (ptr_option_enum, "v3", 1));
    LONGS_EQUAL(2, CONFIG_INTEGER_DEFAULT(ptr_option_enum));
}

/*
 * Tests functions:
 *   config_file_option_unset
 */

TEST(CoreConfigFile, OptionUnset)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_rename
 */

TEST(CoreConfigFile, OptionRename)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_value_to_string
 */

TEST(CoreConfigFile, OptionValueToString)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_get_string
 */

TEST(CoreConfigFile, OptionGetString)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_get_pointer
 */

TEST(CoreConfigFile, OptionGetPointer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_is_null
 */

TEST(CoreConfigFile, OptionIsNull)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_default_is_null
 */

TEST(CoreConfigFile, OptionDefaultIsNull)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_has_changed
 */

TEST(CoreConfigFile, OptionHasChanged)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_set_with_string
 */

TEST(CoreConfigFile, OptionSetWithString)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_boolean
 *   config_file_option_boolean_default
 */

TEST(CoreConfigFileWithNewOptions, OptionBoolean)
{
    LONGS_EQUAL(0, config_file_option_boolean (NULL));
    LONGS_EQUAL(0, config_file_option_boolean_default (NULL));

    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_bool));

    config_file_option_set (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_boolean (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_bool));
    config_file_option_reset (ptr_option_bool, 1);
    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_bool));

    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_int));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_int));
    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_int_str));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_int_str));
    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_str));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_str));
    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_col));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_col));
    LONGS_EQUAL(0, config_file_option_boolean (ptr_option_enum));
    LONGS_EQUAL(0, config_file_option_boolean_default (ptr_option_enum));
}

/*
 * Tests functions:
 *   config_file_option_boolean_inherited
 */

TEST(CoreConfigFileWithNewOptions, OptionBooleanInherited)
{
    LONGS_EQUAL(0, config_file_option_boolean_inherited (NULL));

    LONGS_EQUAL(0, config_file_option_boolean_inherited (ptr_option_bool_child));
    config_file_option_set (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_boolean_inherited (ptr_option_bool_child));
    config_file_option_reset (ptr_option_bool, 1);
    LONGS_EQUAL(0, config_file_option_boolean_inherited (ptr_option_bool_child));
}

/*
 * Tests functions:
 *   config_file_option_integer
 *   config_file_option_integer_default
 */

TEST(CoreConfigFileWithNewOptions, OptionInteger)
{
    LONGS_EQUAL(0, config_file_option_integer (NULL));
    LONGS_EQUAL(0, config_file_option_integer_default (NULL));

    LONGS_EQUAL(100, config_file_option_integer (ptr_option_int));
    LONGS_EQUAL(100, config_file_option_integer_default (ptr_option_int));

    config_file_option_set (ptr_option_int, "123", 1);
    LONGS_EQUAL(123, config_file_option_integer (ptr_option_int));
    LONGS_EQUAL(100, config_file_option_integer_default (ptr_option_int));
    config_file_option_reset (ptr_option_int, 1);
    LONGS_EQUAL(100, config_file_option_integer (ptr_option_int));
    LONGS_EQUAL(100, config_file_option_integer_default (ptr_option_int));

    LONGS_EQUAL(0, config_file_option_integer (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_integer_default (ptr_option_bool));
    config_file_option_set (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_integer (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_integer_default (ptr_option_bool));
    config_file_option_reset (ptr_option_bool, 1);
    LONGS_EQUAL(1, config_file_option_integer (ptr_option_int_str));
    LONGS_EQUAL(1, config_file_option_integer_default (ptr_option_int_str));
    LONGS_EQUAL(0, config_file_option_integer (ptr_option_str));
    LONGS_EQUAL(0, config_file_option_integer_default (ptr_option_str));
    LONGS_EQUAL(9, config_file_option_integer (ptr_option_col));
    LONGS_EQUAL(9, config_file_option_integer_default (ptr_option_col));
    LONGS_EQUAL(1, config_file_option_integer (ptr_option_enum));
    LONGS_EQUAL(1, config_file_option_integer_default (ptr_option_enum));
}

/*
 * Tests functions:
 *   config_file_option_integer_inherited
 */

TEST(CoreConfigFileWithNewOptions, OptionIntegerInherited)
{
    LONGS_EQUAL(0, config_file_option_integer_inherited (NULL));

    LONGS_EQUAL(100, config_file_option_integer_inherited (ptr_option_int_child));
    config_file_option_set (ptr_option_int, "123", 1);
    LONGS_EQUAL(123, config_file_option_integer_inherited (ptr_option_int_child));
    config_file_option_reset (ptr_option_int, 1);
    LONGS_EQUAL(100, config_file_option_integer_inherited (ptr_option_int_child));
}

/*
 * Tests functions:
 *   config_file_option_string
 *   config_file_option_string_default
 */

TEST(CoreConfigFileWithNewOptions, OptionString)
{
    STRCMP_EQUAL(NULL, config_file_option_string (NULL));
    STRCMP_EQUAL(NULL, config_file_option_string_default (NULL));

    STRCMP_EQUAL("v2", config_file_option_string (ptr_option_int_str));
    STRCMP_EQUAL("v2", config_file_option_string_default (ptr_option_int_str));

    STRCMP_EQUAL("value", config_file_option_string (ptr_option_str));
    STRCMP_EQUAL("value", config_file_option_string_default (ptr_option_str));

    config_file_option_set (ptr_option_int_str, "v3", 1);
    STRCMP_EQUAL("v3", config_file_option_string (ptr_option_int_str));
    STRCMP_EQUAL("v2", config_file_option_string_default (ptr_option_int_str));
    config_file_option_reset (ptr_option_int_str, 1);
    STRCMP_EQUAL("v2", config_file_option_string (ptr_option_int_str));
    STRCMP_EQUAL("v2", config_file_option_string_default (ptr_option_int_str));

    config_file_option_set (ptr_option_str, "test", 1);
    STRCMP_EQUAL("test", config_file_option_string (ptr_option_str));
    STRCMP_EQUAL("value", config_file_option_string_default (ptr_option_str));
    config_file_option_reset (ptr_option_str, 1);
    STRCMP_EQUAL("value", config_file_option_string (ptr_option_str));
    STRCMP_EQUAL("value", config_file_option_string_default (ptr_option_str));

    STRCMP_EQUAL("off", config_file_option_string (ptr_option_bool));
    STRCMP_EQUAL("off", config_file_option_string_default (ptr_option_bool));
    STRCMP_EQUAL(NULL, config_file_option_string (ptr_option_int));
    STRCMP_EQUAL(NULL, config_file_option_string_default (ptr_option_int));
    STRCMP_EQUAL("v2", config_file_option_string (ptr_option_int_str));
    STRCMP_EQUAL("v2", config_file_option_string_default (ptr_option_int_str));
    STRCMP_EQUAL("blue", config_file_option_string (ptr_option_col));
    STRCMP_EQUAL("blue", config_file_option_string_default (ptr_option_col));
    STRCMP_EQUAL("v2", config_file_option_string (ptr_option_enum));
    STRCMP_EQUAL("v2", config_file_option_string_default (ptr_option_enum));
}

/*
 * Tests functions:
 *   config_file_option_string_inherited
 */

TEST(CoreConfigFileWithNewOptions, OptionStringInherited)
{
    STRCMP_EQUAL(NULL, config_file_option_string_inherited (NULL));

    STRCMP_EQUAL("v2", config_file_option_string_inherited (ptr_option_int_str_child));
    config_file_option_set (ptr_option_int_str, "v3", 1);
    STRCMP_EQUAL("v3", config_file_option_string_inherited (ptr_option_int_str_child));
    config_file_option_reset (ptr_option_int_str, 1);
    STRCMP_EQUAL("v2", config_file_option_string_inherited (ptr_option_int_str_child));

    STRCMP_EQUAL("value", config_file_option_string_inherited (ptr_option_str_child));
    config_file_option_set (ptr_option_str, "test", 1);
    STRCMP_EQUAL("test", config_file_option_string_inherited (ptr_option_str_child));
    config_file_option_reset (ptr_option_str, 1);
    STRCMP_EQUAL("value", config_file_option_string_inherited (ptr_option_str_child));
}

/*
 * Tests functions:
 *   config_file_option_color
 *   config_file_option_color_default
 */

TEST(CoreConfigFileWithNewOptions, OptionColor)
{
    STRCMP_EQUAL(NULL, config_file_option_color (NULL));
    STRCMP_EQUAL(NULL, config_file_option_color_default (NULL));

    STRCMP_EQUAL("blue", config_file_option_color (ptr_option_col));
    STRCMP_EQUAL("blue", config_file_option_color_default (ptr_option_col));

    config_file_option_set (ptr_option_col, "red", 1);
    STRCMP_EQUAL("red", config_file_option_color (ptr_option_col));
    STRCMP_EQUAL("blue", config_file_option_color_default (ptr_option_col));
    config_file_option_reset (ptr_option_col, 1);
    STRCMP_EQUAL("blue", config_file_option_color (ptr_option_col));
    STRCMP_EQUAL("blue", config_file_option_color_default (ptr_option_col));

    STRCMP_EQUAL(NULL, config_file_option_color (ptr_option_bool));
    STRCMP_EQUAL(NULL, config_file_option_color_default (ptr_option_bool));
    STRCMP_EQUAL(NULL, config_file_option_color (ptr_option_int));
    STRCMP_EQUAL(NULL, config_file_option_color_default (ptr_option_int));
    STRCMP_EQUAL(NULL, config_file_option_color (ptr_option_int_str));
    STRCMP_EQUAL(NULL, config_file_option_color_default (ptr_option_int_str));
    STRCMP_EQUAL(NULL, config_file_option_color (ptr_option_str));
    STRCMP_EQUAL(NULL, config_file_option_color_default (ptr_option_str));
    STRCMP_EQUAL(NULL, config_file_option_color (ptr_option_enum));
    STRCMP_EQUAL(NULL, config_file_option_color_default (ptr_option_enum));
}

/*
 * Tests functions:
 *   config_file_option_color_inherited
 */

TEST(CoreConfigFileWithNewOptions, OptionColorInherited)
{
    STRCMP_EQUAL(NULL, config_file_option_color_inherited (NULL));

    STRCMP_EQUAL("blue", config_file_option_color_inherited (ptr_option_col_child));
    config_file_option_set (ptr_option_col, "red", 1);
    STRCMP_EQUAL("red", config_file_option_color_inherited (ptr_option_col_child));
    config_file_option_reset (ptr_option_col, 1);
    STRCMP_EQUAL("blue", config_file_option_color_inherited (ptr_option_col_child));
}

/*
 * Tests functions:
 *   config_file_option_enum
 *   config_file_option_enum_default
 */

TEST(CoreConfigFileWithNewOptions, OptionEnum)
{
    LONGS_EQUAL(0, config_file_option_enum (NULL));
    LONGS_EQUAL(0, config_file_option_enum_default (NULL));

    LONGS_EQUAL(1, config_file_option_enum (ptr_option_enum));
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_enum));

    config_file_option_set (ptr_option_enum, "v3", 1);
    LONGS_EQUAL(2, config_file_option_enum (ptr_option_enum));
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_enum));
    config_file_option_reset (ptr_option_enum, 1);
    LONGS_EQUAL(1, config_file_option_enum (ptr_option_enum));
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_enum));

    LONGS_EQUAL(0, config_file_option_enum (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_enum_default (ptr_option_bool));
    config_file_option_set (ptr_option_bool, "on", 1);
    LONGS_EQUAL(1, config_file_option_enum (ptr_option_bool));
    LONGS_EQUAL(0, config_file_option_enum_default (ptr_option_bool));
    config_file_option_reset (ptr_option_bool, 1);
    LONGS_EQUAL(100, config_file_option_enum (ptr_option_int));
    LONGS_EQUAL(100, config_file_option_enum_default (ptr_option_int));
    LONGS_EQUAL(1, config_file_option_enum (ptr_option_int_str));
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_int_str));
    LONGS_EQUAL(0, config_file_option_enum (ptr_option_str));
    LONGS_EQUAL(0, config_file_option_enum_default (ptr_option_str));
    LONGS_EQUAL(9, config_file_option_enum (ptr_option_col));
    LONGS_EQUAL(9, config_file_option_enum_default (ptr_option_col));
    LONGS_EQUAL(1, config_file_option_enum (ptr_option_enum));
    LONGS_EQUAL(1, config_file_option_enum_default (ptr_option_enum));
}

/*
 * Tests functions:
 *   config_file_option_enum_inherited
 */

TEST(CoreConfigFileWithNewOptions, OptionEnumInherited)
{
    LONGS_EQUAL(0, config_file_option_enum_inherited (NULL));

    LONGS_EQUAL(1, config_file_option_enum_inherited (ptr_option_enum_child));
    config_file_option_set (ptr_option_enum, "v3", 1);
    LONGS_EQUAL(2, config_file_option_enum_inherited (ptr_option_enum_child));
    config_file_option_reset (ptr_option_enum, 1);
    LONGS_EQUAL(1, config_file_option_enum_inherited (ptr_option_enum_child));
}

/*
 * Tests functions:
 *   config_file_option_escape
 */

TEST(CoreConfigFile, OptionEscape)
{
    STRCMP_EQUAL("\\", config_file_option_escape (NULL));

    STRCMP_EQUAL("", config_file_option_escape (""));
    STRCMP_EQUAL("", config_file_option_escape ("test"));
    STRCMP_EQUAL("", config_file_option_escape ("|test"));
    STRCMP_EQUAL("", config_file_option_escape ("]test"));

    STRCMP_EQUAL("\\", config_file_option_escape ("#test"));
    STRCMP_EQUAL("\\", config_file_option_escape ("[test"));
    STRCMP_EQUAL("\\", config_file_option_escape ("\\test"));
}

/*
 * Tests functions:
 *   config_file_write_option
 */

TEST(CoreConfigFile, WriteOption)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_write_line
 */

TEST(CoreConfigFile, WriteLine)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_write_internal
 */

TEST(CoreConfigFile, WriteInternal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_write
 */

TEST(CoreConfigFile, Write)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_read
 */

TEST(CoreConfigFile, Read)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_reload
 */

TEST(CoreConfigFile, Reload)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_free_data
 */

TEST(CoreConfigFile, OptionFreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_free
 */

TEST(CoreConfigFile, OptionFree)
{
    /* test free of NULL option */
    config_file_option_free (NULL, 1);
}

/*
 * Tests functions:
 *   config_file_section_free_options
 */

TEST(CoreConfigFile, SectionFreeOptions)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_section_free
 */

TEST(CoreConfigFile, SectionFree)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_free
 */

TEST(CoreConfigFile, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_free_all
 */

TEST(CoreConfigFile, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_free_all_plugin
 */

TEST(CoreConfigFile, FreeAllPlugin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_hdata_config_file_cb
 */

TEST(CoreConfigFile, HdataConfigFileCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_hdata_config_section_cb
 */

TEST(CoreConfigFile, HdataConfigSectionCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_hdata_config_option_cb
 */

TEST(CoreConfigFile, HdataConfigOptionCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_add_option_to_infolist
 */

TEST(CoreConfigFile, AddOptionToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_add_to_infolist
 */

TEST(CoreConfigFile, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_print_log
 */

TEST(CoreConfigFile, PrintLog)
{
    /* TODO: write tests */
}
