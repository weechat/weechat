/*
 * test-core-config-file.cpp - test configuration file functions
 *
 * Copyright (C) 2021-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/wee-config-file.h"
#include "src/core/wee-config.h"
#include "src/core/wee-secure-config.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"

extern char *config_file_option_full_name (struct t_config_option *option);
extern int config_file_string_boolean_is_valid (const char *text);
extern const char *config_file_option_escape (const char *name);
}

TEST_GROUP(CoreConfigFile)
{
};

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
 *   config_file_config_find_pos
 */

TEST(CoreConfigFile, FindPos)
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

    POINTERS_EQUAL(NULL, config_file_option_full_name (NULL));

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
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("ON"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("yes"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("Yes"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("y"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("true"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("t"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("1"));

    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("off"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("OFF"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("no"));
    LONGS_EQUAL(1, config_file_string_boolean_is_valid ("No"));
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
    LONGS_EQUAL(1, config_file_string_to_boolean ("ON"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("yes"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("Yes"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("y"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("true"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("t"));
    LONGS_EQUAL(1, config_file_string_to_boolean ("1"));

    LONGS_EQUAL(0, config_file_string_to_boolean ("off"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("OFF"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("no"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("No"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("n"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("false"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("f"));
    LONGS_EQUAL(0, config_file_string_to_boolean ("0"));
}

/*
 * Tests functions:
 *   config_file_option_reset
 *   config_file_option_set
 */

TEST(CoreConfigFile, OptionReset)
{
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_reset (NULL, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (NULL, NULL, 1));

    /* boolean */
    LONGS_EQUAL(0, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (config_look_confirm_quit, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_look_confirm_quit, "on", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_look_confirm_quit, "toggle", 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_look_confirm_quit, "toggle", 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (config_look_confirm_quit, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(config_look_confirm_quit));

    /* integer */
    LONGS_EQUAL(100, CONFIG_INTEGER(config_look_mouse_timer_delay));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (config_look_mouse_timer_delay, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (config_look_mouse_timer_delay, "-500", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (config_look_mouse_timer_delay, "99999999", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_look_mouse_timer_delay, "50", 1));
    LONGS_EQUAL(50, CONFIG_INTEGER(config_look_mouse_timer_delay));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_look_mouse_timer_delay, "++15", 1));
    LONGS_EQUAL(65, CONFIG_INTEGER(config_look_mouse_timer_delay));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_look_mouse_timer_delay, "--3", 1));
    LONGS_EQUAL(62, CONFIG_INTEGER(config_look_mouse_timer_delay));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (config_look_mouse_timer_delay, 1));
    LONGS_EQUAL(100, CONFIG_INTEGER(config_look_mouse_timer_delay));

    /* integer with string values */
    LONGS_EQUAL(CONFIG_LOOK_ALIGN_END_OF_LINES_MESSAGE,
                CONFIG_INTEGER(config_look_align_end_of_lines));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (config_look_align_end_of_lines, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_look_align_end_of_lines, "time", 1));
    LONGS_EQUAL(CONFIG_LOOK_ALIGN_END_OF_LINES_TIME,
                CONFIG_INTEGER(config_look_align_end_of_lines));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (config_look_align_end_of_lines, 1));
    LONGS_EQUAL(CONFIG_LOOK_ALIGN_END_OF_LINES_MESSAGE,
                CONFIG_INTEGER(config_look_align_end_of_lines));

    /* string */
    STRCMP_EQUAL("-", CONFIG_STRING(config_look_separator_horizontal));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (config_look_separator_horizontal, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_look_separator_horizontal, "+", 1));
    STRCMP_EQUAL("+", CONFIG_STRING(config_look_separator_horizontal));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (config_look_separator_horizontal, 1));
    STRCMP_EQUAL("-", CONFIG_STRING(config_look_separator_horizontal));

    /* color */
    LONGS_EQUAL(0, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_set (config_color_chat, "zzz", 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, "red", 1));
    LONGS_EQUAL(3, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, "++5", 1));
    LONGS_EQUAL(8, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, "--3", 1));
    LONGS_EQUAL(5, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, "%red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_BLINK_FLAG, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, ".red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_DIM_FLAG, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, "*red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_BOLD_FLAG, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, "!red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_REVERSE_FLAG, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, "/red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_ITALIC_FLAG, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, "_red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_UNDERLINE_FLAG, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, "|red", 1));
    LONGS_EQUAL(3 | GUI_COLOR_EXTENDED_KEEPATTR_FLAG, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (config_color_chat, "%.*!/_|red", 1));
    LONGS_EQUAL(3
                | GUI_COLOR_EXTENDED_BLINK_FLAG
                | GUI_COLOR_EXTENDED_DIM_FLAG
                | GUI_COLOR_EXTENDED_BOLD_FLAG
                | GUI_COLOR_EXTENDED_REVERSE_FLAG
                | GUI_COLOR_EXTENDED_ITALIC_FLAG
                | GUI_COLOR_EXTENDED_UNDERLINE_FLAG
                | GUI_COLOR_EXTENDED_KEEPATTR_FLAG,
                CONFIG_COLOR(config_color_chat));

    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (config_color_chat, 1));
    LONGS_EQUAL(0, CONFIG_COLOR(config_color_chat));
}

/*
 * Tests functions:
 *   config_file_option_toggle
 */

TEST(CoreConfigFile, OptionToggle)
{
    const char *value_boolean_ok[] = { "on", NULL };
    const char *values_boolean_ok[] = { "on", "off", NULL };
    const char *values_boolean_error[] = { "xxx", "zzz", NULL };
    const char *value_integer_ok[] = { "50", NULL };
    const char *values_integer_ok[] = { "75", "92", NULL };
    const char *values_integer_error[] = { "-500", "99999999", NULL };
    const char *value_integer_str_ok[] = { "time", NULL };
    const char *values_integer_str_ok[] = { "prefix", "suffix", NULL };
    const char *values_integer_str_error[] = { "xxx", "zzz", NULL };
    const char *value_string_ok[] = { "+", NULL };
    const char *values_string_ok[] = { "$", "*", NULL };
    const char *values_string_error[] = { "xxx", "zzz", NULL };
    const char *value_color_ok[] = { "red", NULL };
    const char *values_color_ok[] = { "green", "blue", NULL };
    const char *values_color_error[] = { "xxx", "zzz", NULL };

    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (NULL, NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (config_look_confirm_quit, NULL, -1, 1));

    /* boolean */
    LONGS_EQUAL(0, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (config_look_confirm_quit,
                                           values_boolean_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_confirm_quit, NULL, 0, 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_confirm_quit, NULL, 0, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_confirm_quit, value_boolean_ok, 1, 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_confirm_quit, value_boolean_ok, 1, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_confirm_quit, values_boolean_ok, 2, 1));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_confirm_quit, values_boolean_ok, 2, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(config_look_confirm_quit));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                config_file_option_reset (config_look_confirm_quit, 1));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(config_look_confirm_quit));

    /* integer */
    LONGS_EQUAL(100, CONFIG_INTEGER(config_look_mouse_timer_delay));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (config_look_mouse_timer_delay,
                                           values_integer_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (config_look_mouse_timer_delay,
                                           NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_mouse_timer_delay,
                                           value_integer_ok, 1, 1));
    LONGS_EQUAL(50, CONFIG_INTEGER(config_look_mouse_timer_delay));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_mouse_timer_delay,
                                           value_integer_ok, 1, 1));
    LONGS_EQUAL(100, CONFIG_INTEGER(config_look_mouse_timer_delay));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_mouse_timer_delay,
                                           values_integer_ok, 2, 1));
    LONGS_EQUAL(75, CONFIG_INTEGER(config_look_mouse_timer_delay));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_mouse_timer_delay,
                                           values_integer_ok, 2, 1));
    LONGS_EQUAL(92, CONFIG_INTEGER(config_look_mouse_timer_delay));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (config_look_mouse_timer_delay, 1));
    LONGS_EQUAL(100, CONFIG_INTEGER(config_look_mouse_timer_delay));

    /* integer with string values */
    LONGS_EQUAL(CONFIG_LOOK_ALIGN_END_OF_LINES_MESSAGE,
                CONFIG_INTEGER(config_look_align_end_of_lines));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (config_look_align_end_of_lines,
                                           values_integer_str_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (config_look_align_end_of_lines,
                                           NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_align_end_of_lines,
                                           value_integer_str_ok, 1, 1));
    LONGS_EQUAL(CONFIG_LOOK_ALIGN_END_OF_LINES_TIME,
                CONFIG_INTEGER(config_look_align_end_of_lines));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_align_end_of_lines,
                                           values_integer_str_ok, 2, 1));
    LONGS_EQUAL(CONFIG_LOOK_ALIGN_END_OF_LINES_PREFIX,
                CONFIG_INTEGER(config_look_align_end_of_lines));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_align_end_of_lines,
                                           values_integer_str_ok, 2, 1));
    LONGS_EQUAL(CONFIG_LOOK_ALIGN_END_OF_LINES_SUFFIX,
                CONFIG_INTEGER(config_look_align_end_of_lines));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (config_look_align_end_of_lines, 1));
    LONGS_EQUAL(CONFIG_LOOK_ALIGN_END_OF_LINES_MESSAGE,
                CONFIG_INTEGER(config_look_align_end_of_lines));

    /* string */
    STRCMP_EQUAL("-", CONFIG_STRING(config_look_separator_horizontal));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (config_look_separator_horizontal,
                                           values_string_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_separator_horizontal,
                                           NULL, 0, 1));
    STRCMP_EQUAL("", CONFIG_STRING(config_look_separator_horizontal));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_separator_horizontal,
                                           NULL, 0, 1));
    STRCMP_EQUAL("-", CONFIG_STRING(config_look_separator_horizontal));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_separator_horizontal,
                                           value_string_ok, 1, 1));
    STRCMP_EQUAL("+", CONFIG_STRING(config_look_separator_horizontal));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_separator_horizontal,
                                           values_string_ok, 2, 1));
    STRCMP_EQUAL("$", CONFIG_STRING(config_look_separator_horizontal));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_look_separator_horizontal,
                                           values_string_ok, 2, 1));
    STRCMP_EQUAL("*", CONFIG_STRING(config_look_separator_horizontal));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (config_look_separator_horizontal, 1));
    STRCMP_EQUAL("-", CONFIG_STRING(config_look_separator_horizontal));

    /* color */
    LONGS_EQUAL(0, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (config_color_chat,
                                           values_color_error, 2, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                config_file_option_toggle (config_color_chat, NULL, 0, 1));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_color_chat,
                                           value_color_ok, 1, 1));
    LONGS_EQUAL(3, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_color_chat,
                                           values_color_ok, 2, 1));
    LONGS_EQUAL(5, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_toggle (config_color_chat,
                                           values_color_ok, 2, 1));
    LONGS_EQUAL(9, CONFIG_COLOR(config_color_chat));
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_reset (config_color_chat, 1));
    LONGS_EQUAL(0, CONFIG_COLOR(config_color_chat));
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
 */

TEST(CoreConfigFile, OptionBoolean)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_boolean_default
 */

TEST(CoreConfigFile, OptionBooleanDefault)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_integer
 */

TEST(CoreConfigFile, OptionInteger)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_integer_default
 */

TEST(CoreConfigFile, OptionIntegerDefault)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_string
 */

TEST(CoreConfigFile, OptionString)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_string_default
 */

TEST(CoreConfigFile, OptionStringDefault)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_color
 */

TEST(CoreConfigFile, OptionColor)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   config_file_option_color_default
 */

TEST(CoreConfigFile, OptionColorDefault)
{
    /* TODO: write tests */
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
    /* TODO: write tests */
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
