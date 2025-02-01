/*
 * test-core-string.cpp - test string functions
 *
 * Copyright (C) 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "src/core/weechat.h"
#include "src/core/core-config.h"
#include "src/core/core-string.h"
#include "src/core/core-hashtable.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"
}

#define ONE_KB 1000ULL
#define ONE_MB (ONE_KB * 1000ULL)
#define ONE_GB (ONE_MB * 1000ULL)
#define ONE_TB (ONE_GB * 1000ULL)

#define WEE_IS_WORD_CHAR(__result, __str)                               \
    LONGS_EQUAL(__result, string_is_word_char_highlight (__str));       \
    LONGS_EQUAL(__result, string_is_word_char_input (__str));
#define WEE_HAS_HL_STR(__result, __str, __words)                        \
    LONGS_EQUAL(__result, string_has_highlight (__str, __words));

#define WEE_HAS_HL_REGEX(__result_regex, __result_hl, __str, __regex)   \
    LONGS_EQUAL(__result_hl,                                            \
                string_has_highlight_regex (__str, __regex));           \
    LONGS_EQUAL(__result_regex,                                         \
                string_regcomp (&regex, __regex, REG_ICASE));           \
    LONGS_EQUAL(__result_hl,                                            \
                string_has_highlight_regex_compiled (__str,             \
                                                     &regex));          \
    if (__result_regex == 0)                                            \
        regfree(&regex);

#define WEE_REPLACE_REGEX(__result_regex, __result_replace, __str,      \
                          __regex, __replace, __ref_char, __callback)   \
    LONGS_EQUAL(__result_regex,                                         \
                string_regcomp (&regex, __regex,                        \
                                REG_EXTENDED | REG_ICASE));             \
    result = string_replace_regex (__str, &regex, __replace,            \
                                   __ref_char, __callback, NULL);       \
    STRCMP_EQUAL(__result_replace, result);                             \
    free (result);                                                      \
    if (__result_regex == 0)                                            \
        regfree(&regex);

#define WEE_REPLACE_CB(__result_replace, __result_errors,               \
                       __str, __prefix, __suffix, __allow_escape,       \
                       __list_prefix_no_replace,                        \
                       __callback, __callback_data, __errors)           \
    errors = -1;                                                        \
    result = string_replace_with_callback (                             \
        __str, __prefix, __suffix, __allow_escape,                      \
        __list_prefix_no_replace, __callback, __callback_data,          \
        __errors);                                                      \
    STRCMP_EQUAL(__result_replace, result);                             \
    free (result);                                                      \
    if (__result_errors >= 0)                                           \
    {                                                                   \
        LONGS_EQUAL(__result_errors, errors);                           \
    }

#define WEE_FORMAT_SIZE(__result, __size)                               \
    str = string_format_size (__size);                                  \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

#define WEE_HEX_DUMP(__result, __data, __size, __bytes_per_line,      \
                     __prefix, __suffix)                                \
    str = string_hex_dump (__data, __size, __bytes_per_line,            \
                           __prefix, __suffix);                         \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

extern struct t_hashtable *string_hashtable_shared;

TEST_GROUP(CoreString)
{
};

/*
 * Tests functions:
 *   string_asprintf
 */

TEST(CoreString, Asprintf)
{
    char *test;

    test = (char *)0x1;
    LONGS_EQUAL(-1, string_asprintf (NULL, NULL));
    POINTERS_EQUAL(0x1, test);

    test = (char *)0x1;
    LONGS_EQUAL(-1, string_asprintf (NULL, ""));
    POINTERS_EQUAL(0x1, test);

    test = (char *)0x1;
    LONGS_EQUAL(-1, string_asprintf (&test, NULL));
    STRCMP_EQUAL(NULL, test);

    test = (char *)0x1;
    LONGS_EQUAL(0, string_asprintf (&test, ""));
    STRCMP_EQUAL("", test);
    free (test);

    test = (char *)0x1;
    LONGS_EQUAL(4, string_asprintf (&test, "test"));
    STRCMP_EQUAL("test", test);
    free (test);

    test = (char *)0x1;
    LONGS_EQUAL(16, string_asprintf (&test, "test, %s, %d", "string", 42));
    STRCMP_EQUAL("test, string, 42", test);
    free (test);
}

/*
 * Tests functions:
 *   string_strndup
 */

TEST(CoreString, Strndup)
{
    const char *str_test = "test";
    char *str;

    STRCMP_EQUAL(NULL, string_strndup (NULL, 0));

    STRCMP_EQUAL(NULL, string_strndup (str_test, -1));

    str = string_strndup (str_test, 0);
    CHECK(str);
    CHECK(str != str_test);
    STRCMP_EQUAL("", str);
    free (str);

    str = string_strndup (str_test, 1);
    CHECK(str);
    CHECK(str != str_test);
    STRCMP_EQUAL("t", str);
    free (str);

    str = string_strndup (str_test, 2);
    CHECK(str);
    CHECK(str != str_test);
    STRCMP_EQUAL("te", str);
    free (str);

    str = string_strndup (str_test, 3);
    CHECK(str);
    CHECK(str != str_test);
    STRCMP_EQUAL("tes", str);
    free (str);

    str = string_strndup (str_test, 4);
    CHECK(str);
    CHECK(str != str_test);
    STRCMP_EQUAL("test", str);
    free (str);

    str = string_strndup (str_test, 5);
    CHECK(str);
    CHECK(str != str_test);
    STRCMP_EQUAL("test", str);
    free (str);

    str = string_strndup (str_test, 500);
    CHECK(str);
    CHECK(str != str_test);
    STRCMP_EQUAL(str_test, str);
    free (str);
}

/*
 * Tests functions:
 *   string_tolower
 */

TEST(CoreString, ToLower)
{
    char *str;

    WEE_TEST_STR(NULL, string_tolower (NULL));

    WEE_TEST_STR("", string_tolower (""));
    WEE_TEST_STR("abcd_é", string_tolower ("ABCD_É"));
    WEE_TEST_STR("àáâãäåæçèéêëìíîïðñòóôõöøœšùúûüýÿ",
                 string_tolower ("ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØŒŠÙÚÛÜÝŸ"));
    WEE_TEST_STR("€", string_tolower ("€"));
    WEE_TEST_STR("[⛄]", string_tolower ("[⛄]"));
}

/*
 * Tests functions:
 *   string_toupper
 */

TEST(CoreString, ToUpper)
{
    char *str;

    WEE_TEST_STR(NULL, string_tolower (NULL));

    WEE_TEST_STR("", string_toupper (""));
    WEE_TEST_STR("ABCD_É", string_toupper ("abcd_é"));
    WEE_TEST_STR("ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØŒŠÙÚÛÜÝŸ",
                 string_toupper ("àáâãäåæçèéêëìíîïðñòóôõöøœšùúûüýÿ"));
    WEE_TEST_STR("€", string_toupper ("€"));
    WEE_TEST_STR("[⛄]", string_toupper ("[⛄]"));
}

/*
 * Tests functions:
 *   string_tolower_range
 */

TEST(CoreString, ToLowerRange)
{
    char *str;

    WEE_TEST_STR(NULL, string_tolower_range (NULL, 0));
    WEE_TEST_STR(NULL, string_tolower_range (NULL, 30));

    WEE_TEST_STR("", string_tolower_range ("", 0));
    WEE_TEST_STR("", string_tolower_range ("", 30));
    WEE_TEST_STR("^[a]ô", string_tolower_range ("^[A]Ô", 0));
    WEE_TEST_STR("~{a}Ô", string_tolower_range ("^[A]Ô", 30));
    WEE_TEST_STR("^{a}Ô", string_tolower_range ("^[A]Ô", 29));
    WEE_TEST_STR("^[a]Ô", string_tolower_range ("^[A]Ô", 26));
}

/*
 * Tests functions:
 *   string_toupper_range
 */

TEST(CoreString, ToUpperRange)
{
    char *str;

    WEE_TEST_STR(NULL, string_toupper_range (NULL, 0));
    WEE_TEST_STR(NULL, string_toupper_range (NULL, 30));

    WEE_TEST_STR("", string_toupper_range ("", 0));
    WEE_TEST_STR("", string_toupper_range ("", 30));
    WEE_TEST_STR("~{A}Ô", string_toupper_range ("~{a}ô", 0));
    WEE_TEST_STR("^[A]ô", string_toupper_range ("~{a}ô", 30));
    WEE_TEST_STR("~[A]ô", string_toupper_range ("~{a}ô", 29));
    WEE_TEST_STR("~{A}ô", string_toupper_range ("~{a}ô", 26));
}

/*
 * Tests functions:
 *   string_cut
 */

TEST(CoreString, Cut)
{
    char suffix[128], string[128];

    STRCMP_EQUAL(NULL, string_cut (NULL, 0, 0, 0, NULL));
    STRCMP_EQUAL("", string_cut ("", 0, 0, 0, NULL));

    /* cut with length == 0 */
    STRCMP_EQUAL("", string_cut ("noël", 0, 0, 0, NULL));
    STRCMP_EQUAL("+", string_cut ("noël", 0, 0, 0, "+"));
    STRCMP_EQUAL("…", string_cut ("noël", 0, 0, 0, "…"));
    STRCMP_EQUAL("", string_cut ("noël", 0, 1, 0, NULL));
    STRCMP_EQUAL("", string_cut ("noël", 0, 1, 0, "+"));
    STRCMP_EQUAL("", string_cut ("noël", 0, 1, 0, "…"));

    /* cut with length == 1 */
    STRCMP_EQUAL("n", string_cut ("noël", 1, 0, 0, NULL));
    STRCMP_EQUAL("n+", string_cut ("noël", 1, 0, 0, "+"));
    STRCMP_EQUAL("n…", string_cut ("noël", 1, 0, 0, "…"));
    STRCMP_EQUAL("n", string_cut ("noël", 1, 1, 0, NULL));
    STRCMP_EQUAL("+", string_cut ("noël", 1, 1, 0, "+"));
    STRCMP_EQUAL("…", string_cut ("noël", 1, 1, 0, "…"));

    /* cut with length == 2 */
    STRCMP_EQUAL("no", string_cut ("noël", 2, 0, 0, NULL));
    STRCMP_EQUAL("no+", string_cut ("noël", 2, 0, 0, "+"));
    STRCMP_EQUAL("no…", string_cut ("noël", 2, 0, 0, "…"));
    STRCMP_EQUAL("no", string_cut ("noël", 2, 1, 0, NULL));
    STRCMP_EQUAL("n+", string_cut ("noël", 2, 1, 0, "+"));
    STRCMP_EQUAL("n…", string_cut ("noël", 2, 1, 0, "…"));

    /* cut with length == 3 */
    STRCMP_EQUAL("noë", string_cut ("noël", 3, 0, 0, NULL));
    STRCMP_EQUAL("noë+", string_cut ("noël", 3, 0, 0, "+"));
    STRCMP_EQUAL("noë…", string_cut ("noël", 3, 0, 0, "…"));
    STRCMP_EQUAL("noë", string_cut ("noël", 3, 1, 0, NULL));
    STRCMP_EQUAL("no+", string_cut ("noël", 3, 1, 0, "+"));
    STRCMP_EQUAL("no…", string_cut ("noël", 3, 1, 0, "…"));

    /* cut with length == 4 */
    STRCMP_EQUAL("noël", string_cut ("noël", 4, 0, 0, NULL));
    STRCMP_EQUAL("noël", string_cut ("noël", 4, 0, 0, "+"));
    STRCMP_EQUAL("noël", string_cut ("noël", 4, 0, 0, "…"));
    STRCMP_EQUAL("noël", string_cut ("noël", 4, 1, 0, NULL));
    STRCMP_EQUAL("noël", string_cut ("noël", 4, 1, 0, "+"));
    STRCMP_EQUAL("noël", string_cut ("noël", 4, 1, 0, "…"));

    /* cut with length == 5 */
    STRCMP_EQUAL("noël", string_cut ("noël", 5, 0, 0, NULL));
    STRCMP_EQUAL("noël", string_cut ("noël", 5, 0, 0, "+"));
    STRCMP_EQUAL("noël", string_cut ("noël", 5, 0, 0, "…"));
    STRCMP_EQUAL("noël", string_cut ("noël", 5, 1, 0, NULL));
    STRCMP_EQUAL("noël", string_cut ("noël", 5, 1, 0, "+"));
    STRCMP_EQUAL("noël", string_cut ("noël", 5, 1, 0, "…"));

    /* cut with length == 1, screen == 0 then 1 */
    STRCMP_EQUAL("こ", string_cut ("こんにちは世界", 1, 0, 0, NULL));
    STRCMP_EQUAL("こ+", string_cut ("こんにちは世界", 1, 0, 0, "+"));
    STRCMP_EQUAL("こ…", string_cut ("こんにちは世界", 1, 0, 0, "…"));
    STRCMP_EQUAL("こ", string_cut ("こんにちは世界", 1, 1, 0, NULL));
    STRCMP_EQUAL("+", string_cut ("こんにちは世界", 1, 1, 0, "+"));
    STRCMP_EQUAL("…", string_cut ("こんにちは世界", 1, 1, 0, "…"));

    STRCMP_EQUAL("", string_cut ("こんにちは世界", 1, 0, 1, NULL));
    STRCMP_EQUAL("+", string_cut ("こんにちは世界", 1, 0, 1, "+"));
    STRCMP_EQUAL("…", string_cut ("こんにちは世界", 1, 0, 1, "…"));
    STRCMP_EQUAL("", string_cut ("こんにちは世界", 1, 1, 1, NULL));
    STRCMP_EQUAL("+", string_cut ("こんにちは世界", 1, 1, 1, "+"));
    STRCMP_EQUAL("…", string_cut ("こんにちは世界", 1, 1, 1, "…"));

    /* cut with length == 2, screen == 0 then 1 */
    STRCMP_EQUAL("こん", string_cut ("こんにちは世界", 2, 0, 0, NULL));
    STRCMP_EQUAL("こん+", string_cut ("こんにちは世界", 2, 0, 0, "+"));
    STRCMP_EQUAL("こん…", string_cut ("こんにちは世界", 2, 0, 0, "…"));
    STRCMP_EQUAL("こん", string_cut ("こんにちは世界", 2, 1, 0, NULL));
    STRCMP_EQUAL("こ+", string_cut ("こんにちは世界", 2, 1, 0, "+"));
    STRCMP_EQUAL("こ…", string_cut ("こんにちは世界", 2, 1, 0, "…"));

    STRCMP_EQUAL("こ", string_cut ("こんにちは世界", 2, 0, 1, NULL));
    STRCMP_EQUAL("こ+", string_cut ("こんにちは世界", 2, 0, 1, "+"));
    STRCMP_EQUAL("こ…", string_cut ("こんにちは世界", 2, 0, 1, "…"));
    STRCMP_EQUAL("こ", string_cut ("こんにちは世界", 2, 1, 1, NULL));
    STRCMP_EQUAL("+", string_cut ("こんにちは世界", 2, 1, 1, "+"));
    STRCMP_EQUAL("…", string_cut ("こんにちは世界", 2, 1, 1, "…"));

    /* cut with length == 3, screen == 0 then 1 */
    STRCMP_EQUAL("こんに", string_cut ("こんにちは世界", 3, 0, 0, NULL));
    STRCMP_EQUAL("こんに+", string_cut ("こんにちは世界", 3, 0, 0, "+"));
    STRCMP_EQUAL("こんに…", string_cut ("こんにちは世界", 3, 0, 0, "…"));
    STRCMP_EQUAL("こんに", string_cut ("こんにちは世界", 3, 1, 0, NULL));
    STRCMP_EQUAL("こん+", string_cut ("こんにちは世界", 3, 1, 0, "+"));
    STRCMP_EQUAL("こん…", string_cut ("こんにちは世界", 3, 1, 0, "…"));

    STRCMP_EQUAL("こ", string_cut ("こんにちは世界", 3, 0, 1, NULL));
    STRCMP_EQUAL("こ+", string_cut ("こんにちは世界", 3, 0, 1, "+"));
    STRCMP_EQUAL("こ…", string_cut ("こんにちは世界", 3, 0, 1, "…"));
    STRCMP_EQUAL("こ", string_cut ("こんにちは世界", 3, 1, 1, NULL));
    STRCMP_EQUAL("こ+", string_cut ("こんにちは世界", 3, 1, 1, "+"));
    STRCMP_EQUAL("こ…", string_cut ("こんにちは世界", 3, 1, 1, "…"));

    /* cut suffix using color and 1 char */
    snprintf (suffix, sizeof (suffix), "%s+", gui_color_get_custom ("red"));
    snprintf (string, sizeof (string), "te%s+", gui_color_get_custom ("red"));
    STRCMP_EQUAL(string, string_cut ("test", 3, 1, 1, suffix));

    /* cut suffix using color and 2 chars */
    snprintf (suffix, sizeof (suffix), "%s++", gui_color_get_custom ("red"));
    snprintf (string, sizeof (string), "t%s++", gui_color_get_custom ("red"));
    STRCMP_EQUAL(string, string_cut ("test", 3, 1, 1, suffix));

    /* cut suffix using color and 3 chars */
    snprintf (suffix, sizeof (suffix), "%s+++", gui_color_get_custom ("red"));
    snprintf (string, sizeof (string), "%s+++", gui_color_get_custom ("red"));
    STRCMP_EQUAL(string, string_cut ("test", 3, 1, 1, suffix));

    /* cut suffix using color and 4 chars */
    snprintf (suffix, sizeof (suffix), "%s++++", gui_color_get_custom ("red"));
    STRCMP_EQUAL("", string_cut ("test", 3, 1, 1, suffix));
}

/*
 * Tests functions:
 *   string_reverse
 */

TEST(CoreString, Reverse)
{
    char string[128];

    STRCMP_EQUAL(NULL, string_reverse (NULL));
    STRCMP_EQUAL("", string_reverse (""));

    /* reverse of UTF-8 string */
    STRCMP_EQUAL("n", string_reverse ("n"));
    STRCMP_EQUAL("on", string_reverse ("no"));
    STRCMP_EQUAL("ëon", string_reverse ("noë"));
    STRCMP_EQUAL("lëon", string_reverse ("noël"));
    STRCMP_EQUAL("界世はちにんこ", string_reverse ("こんにちは世界"));

    /*
     * reverse of ISO-8859-15 string: the result may not be what you expect:
     * the function string_reverse accepts only an UTF-8 string as input
     */
    STRCMP_EQUAL("\xeblon", string_reverse ("no\xebl"));

    /* reverse of string with color codes */
    snprintf (string, sizeof (string),
              "%s",
              gui_color_get_custom ("red"));
    STRCMP_EQUAL("30F\x19", string_reverse (string));

    snprintf (string, sizeof (string),
              "%s red",
              gui_color_get_custom ("red"));
    STRCMP_EQUAL("der 30F\x19", string_reverse (string));

    snprintf (string, sizeof (string),
              "red %s",
              gui_color_get_custom ("red"));
    STRCMP_EQUAL("30F\x19 der", string_reverse (string));
}

/*
 * Tests functions:
 *   string_reverse_screen
 */

TEST(CoreString, ReverseScreen)
{
    char string[128], result[128];

    STRCMP_EQUAL(NULL, string_reverse_screen (NULL));
    STRCMP_EQUAL("", string_reverse_screen (""));

    /* reverse of UTF-8 string */
    STRCMP_EQUAL("n", string_reverse_screen ("n"));
    STRCMP_EQUAL("on", string_reverse_screen ("no"));
    STRCMP_EQUAL("ëon", string_reverse_screen ("noë"));
    STRCMP_EQUAL("lëon", string_reverse_screen ("noël"));
    STRCMP_EQUAL("界世はちにんこ", string_reverse_screen ("こんにちは世界"));

    /*
     * reverse of ISO-8859-15 string: the result may not be what you expect:
     * the function string_reverse_screen accepts only an UTF-8 string as input
     */
    STRCMP_EQUAL("\xeblon", string_reverse_screen ("no\xebl"));

    /* reverse of string with color codes */
    snprintf (string, sizeof (string),
              "%s",
              gui_color_get_custom ("red"));
    snprintf (result, sizeof (result),
              "%s",
              gui_color_get_custom ("red"));
    STRCMP_EQUAL(result, string_reverse_screen (string));

    snprintf (string, sizeof (string),
              "%s red",
              gui_color_get_custom ("red"));
    snprintf (result, sizeof (result),
              "der %s",
              gui_color_get_custom ("red"));
    STRCMP_EQUAL(result, string_reverse_screen (string));

    snprintf (string, sizeof (string),
              "red %s",
              gui_color_get_custom ("red"));
    snprintf (result, sizeof (result),
              "%s der",
              gui_color_get_custom ("red"));
    STRCMP_EQUAL(result, string_reverse_screen (string));
}

/*
 * Tests functions:
 *   string_repeat
 */

TEST(CoreString, Repeat)
{
    STRCMP_EQUAL(NULL, string_repeat (NULL, 1));
    STRCMP_EQUAL(NULL, string_repeat ("----", INT_MAX / 4));

    STRCMP_EQUAL("", string_repeat ("", 1));

    STRCMP_EQUAL("", string_repeat ("x", -1));
    STRCMP_EQUAL("", string_repeat ("x", 0));
    STRCMP_EQUAL("x", string_repeat ("x", 1));
    STRCMP_EQUAL("xxx", string_repeat ("x", 3));
    STRCMP_EQUAL("abcabc", string_repeat ("abc", 2));
    STRCMP_EQUAL("noëlnoël", string_repeat ("noël", 2));
}

/*
 * Tests functions:
 *   string_charcmp
 *   string_charcasecmp
 *   string_charcasecmp_range
 */

TEST(CoreString, CharComparison)
{
    /* case-sensitive comparison */
    LONGS_EQUAL(0, string_charcmp (NULL, NULL));
    LONGS_EQUAL(-97, string_charcmp (NULL, "abc"));
    LONGS_EQUAL(97, string_charcmp ("abc", NULL));
    LONGS_EQUAL(0, string_charcmp ("axx", "azz"));
    LONGS_EQUAL(-2, string_charcmp ("A", "C"));
    LONGS_EQUAL(2, string_charcmp ("C", "A"));
    LONGS_EQUAL(-32, string_charcmp ("A", "a"));
    LONGS_EQUAL(-8129, string_charcmp ("ë", "€"));
    LONGS_EQUAL(235, string_charcmp ("ë", ""));
    LONGS_EQUAL(-235, string_charcmp ("", "ë"));

    /* case-insensitive comparison */
    LONGS_EQUAL(0, string_charcasecmp (NULL, NULL));
    LONGS_EQUAL(-97, string_charcasecmp (NULL, "abc"));
    LONGS_EQUAL(97, string_charcasecmp ("abc", NULL));
    LONGS_EQUAL(0, string_charcasecmp ("axx", "azz"));
    LONGS_EQUAL(-2, string_charcasecmp ("A", "C"));
    LONGS_EQUAL(2, string_charcasecmp ("C", "A"));
    LONGS_EQUAL(0, string_charcasecmp ("A", "a"));
    LONGS_EQUAL(-8129, string_charcasecmp ("ë", "€"));

    /* case-insensitive comparison with a range */
    LONGS_EQUAL(0, string_charcasecmp_range (NULL, NULL, 30));
    LONGS_EQUAL(-97, string_charcasecmp_range (NULL, "abc", 30));
    LONGS_EQUAL(97, string_charcasecmp_range ("abc", NULL, 30));
    LONGS_EQUAL(0, string_charcasecmp_range ("axx", "azz", 30));
    LONGS_EQUAL(-2, string_charcasecmp_range ("A", "C", 30));
    LONGS_EQUAL(2, string_charcasecmp_range ("C", "A", 30));
    LONGS_EQUAL(0, string_charcasecmp_range ("A", "a", 30));
    LONGS_EQUAL(-8129, string_charcasecmp_range ("ë", "€", 30));
    LONGS_EQUAL(0, string_charcasecmp_range ("[", "{", 30));
    LONGS_EQUAL(0, string_charcasecmp_range ("]", "}", 30));
    LONGS_EQUAL(0, string_charcasecmp_range ("\\", "|", 30));
    LONGS_EQUAL(0, string_charcasecmp_range ("^", "~", 30));
    LONGS_EQUAL(0, string_charcasecmp_range ("[", "{", 29));
    LONGS_EQUAL(0, string_charcasecmp_range ("]", "}", 29));
    LONGS_EQUAL(0, string_charcasecmp_range ("\\", "|", 29));
    LONGS_EQUAL(-32, string_charcasecmp_range ("^", "~", 29));
    LONGS_EQUAL(32, string_charcasecmp_range ("~", "^", 29));
    LONGS_EQUAL(-32, string_charcasecmp_range ("[", "{", 26));
    LONGS_EQUAL(32, string_charcasecmp_range ("{", "[", 26));
    LONGS_EQUAL(-32, string_charcasecmp_range ("]", "}", 26));
    LONGS_EQUAL(32, string_charcasecmp_range ("}", "]", 26));
    LONGS_EQUAL(-32, string_charcasecmp_range ("\\", "|", 26));
    LONGS_EQUAL(32, string_charcasecmp_range ("|", "\\", 26));
    LONGS_EQUAL(-32, string_charcasecmp_range ("^", "~", 26));
    LONGS_EQUAL(32, string_charcasecmp_range ("~", "^", 26));
}

/*
 * Tests functions:
 *   string_strcmp
 *   string_strncmp
 *   string_strcasecmp
 *   string_strncasecmp
 *   string_strcasecmp_range
 *   string_strncasecmp_range
 *   string_strcmp_ignore_chars
 */

TEST(CoreString, StringComparison)
{
    /* case-sensitive comparison */
    LONGS_EQUAL(0, string_strcmp (NULL, NULL));
    LONGS_EQUAL(-1, string_strcmp (NULL, ""));
    LONGS_EQUAL(1, string_strcmp ("", NULL));
    LONGS_EQUAL(-1, string_strcmp (NULL, "abc"));
    LONGS_EQUAL(1, string_strcmp ("abc", NULL));
    LONGS_EQUAL(-97, string_strcmp ("", "abc"));
    LONGS_EQUAL(97, string_strcmp ("abc", ""));
    LONGS_EQUAL(-98, string_strcmp ("", "b"));
    LONGS_EQUAL(98, string_strcmp ("b", ""));
    LONGS_EQUAL(0, string_strcmp ("abc", "abc"));
    LONGS_EQUAL(32, string_strcmp ("abc", "ABC"));
    LONGS_EQUAL(0, string_strcmp ("ABC", "ABC"));
    LONGS_EQUAL(-3, string_strcmp ("abc", "def"));
    LONGS_EQUAL(29, string_strcmp ("abc", "DEF"));
    LONGS_EQUAL(-35, string_strcmp ("ABC", "def"));
    LONGS_EQUAL(-3, string_strcmp ("ABC", "DEF"));
    LONGS_EQUAL(3, string_strcmp ("def", "abc"));
    LONGS_EQUAL(35, string_strcmp ("def", "ABC"));
    LONGS_EQUAL(-29, string_strcmp ("DEF", "abc"));
    LONGS_EQUAL(3, string_strcmp ("DEF", "ABC"));
    LONGS_EQUAL(-9, string_strcmp ("à", "é"));
    LONGS_EQUAL(32, string_strcmp ("ê", "Ê"));

    /* case-sensitive comparison with max length */
    LONGS_EQUAL(0, string_strncmp (NULL, NULL, 3));
    LONGS_EQUAL(-1, string_strncmp (NULL, "", 3));
    LONGS_EQUAL(1, string_strncmp ("", NULL, 3));
    LONGS_EQUAL(-1, string_strncmp (NULL, "abc", 3));
    LONGS_EQUAL(1, string_strncmp ("abc", NULL, 3));
    LONGS_EQUAL(-97, string_strncmp ("", "abc", 3));
    LONGS_EQUAL(97, string_strncmp ("abc", "", 3));
    LONGS_EQUAL(-98, string_strncmp ("", "b", 3));
    LONGS_EQUAL(98, string_strncmp ("b", "", 3));
    LONGS_EQUAL(0, string_strncmp ("abc", "abc", 3));
    LONGS_EQUAL(0, string_strncmp ("abcabc", "abcdef", 3));
    LONGS_EQUAL(-3, string_strncmp ("abcabc", "abcdef", 6));
    LONGS_EQUAL(32, string_strncmp ("abc", "ABC", 3));
    LONGS_EQUAL(32, string_strncmp ("abcabc", "ABCDEF", 3));
    LONGS_EQUAL(32, string_strncmp ("abcabc", "ABCDEF", 6));
    LONGS_EQUAL(0, string_strncmp ("ABC", "ABC", 3));
    LONGS_EQUAL(0, string_strncmp ("ABCABC", "ABCDEF", 3));
    LONGS_EQUAL(-3, string_strncmp ("ABCABC", "ABCDEF", 6));
    LONGS_EQUAL(-3, string_strncmp ("abc", "def", 3));
    LONGS_EQUAL(29, string_strncmp ("abc", "DEF", 3));
    LONGS_EQUAL(-35, string_strncmp ("ABC", "def", 3));
    LONGS_EQUAL(-3, string_strncmp ("ABC", "DEF", 3));
    LONGS_EQUAL(3, string_strncmp ("def", "abc", 3));
    LONGS_EQUAL(35, string_strncmp ("def", "ABC", 3));
    LONGS_EQUAL(-29, string_strncmp ("DEF", "abc", 3));
    LONGS_EQUAL(3, string_strncmp ("DEF", "ABC", 3));
    LONGS_EQUAL(-9, string_strncmp ("à", "é", 1));
    LONGS_EQUAL(32, string_strncmp ("ê", "Ê", 1));

    /* case-insensitive comparison */
    LONGS_EQUAL(0, string_strcasecmp (NULL, NULL));
    LONGS_EQUAL(-1, string_strcasecmp (NULL, ""));
    LONGS_EQUAL(1, string_strcasecmp ("", NULL));
    LONGS_EQUAL(-1, string_strcasecmp (NULL, "abc"));
    LONGS_EQUAL(1, string_strcasecmp ("abc", NULL));
    LONGS_EQUAL(-97, string_strcasecmp ("", "abc"));
    LONGS_EQUAL(97, string_strcasecmp ("abc", ""));
    LONGS_EQUAL(-98, string_strcasecmp ("", "b"));
    LONGS_EQUAL(98, string_strcasecmp ("b", ""));
    LONGS_EQUAL(0, string_strcasecmp ("abc", "abc"));
    LONGS_EQUAL(0, string_strcasecmp ("abc", "ABC"));
    LONGS_EQUAL(0, string_strcasecmp ("ABC", "ABC"));
    LONGS_EQUAL(-3, string_strcasecmp ("abc", "def"));
    LONGS_EQUAL(-3, string_strcasecmp ("abc", "DEF"));
    LONGS_EQUAL(-3, string_strcasecmp ("ABC", "def"));
    LONGS_EQUAL(-3, string_strcasecmp ("ABC", "DEF"));
    LONGS_EQUAL(3, string_strcasecmp ("def", "abc"));
    LONGS_EQUAL(3, string_strcasecmp ("def", "ABC"));
    LONGS_EQUAL(3, string_strcasecmp ("DEF", "abc"));
    LONGS_EQUAL(3, string_strcasecmp ("DEF", "ABC"));
    LONGS_EQUAL(-9, string_strcasecmp ("à", "é"));
    LONGS_EQUAL(0, string_strcasecmp ("ê", "Ê"));

    /* case-insensitive comparison with max length */
    LONGS_EQUAL(0, string_strncasecmp (NULL, NULL, 3));
    LONGS_EQUAL(-1, string_strncasecmp (NULL, "", 3));
    LONGS_EQUAL(1, string_strncasecmp ("", NULL, 3));
    LONGS_EQUAL(-1, string_strncasecmp (NULL, "abc", 3));
    LONGS_EQUAL(1, string_strncasecmp ("abc", NULL, 3));
    LONGS_EQUAL(-97, string_strncasecmp ("", "abc", 3));
    LONGS_EQUAL(97, string_strncasecmp ("abc", "", 3));
    LONGS_EQUAL(-98, string_strncasecmp ("", "b", 3));
    LONGS_EQUAL(98, string_strncasecmp ("b", "", 3));
    LONGS_EQUAL(0, string_strncasecmp ("abc", "abc", 3));
    LONGS_EQUAL(0, string_strncasecmp ("abcabc", "abcdef", 3));
    LONGS_EQUAL(-3, string_strncasecmp ("abcabc", "abcdef", 6));
    LONGS_EQUAL(0, string_strncasecmp ("abc", "ABC", 3));
    LONGS_EQUAL(0, string_strncasecmp ("abcabc", "ABCDEF", 3));
    LONGS_EQUAL(-3, string_strncasecmp ("abcabc", "ABCDEF", 6));
    LONGS_EQUAL(0, string_strncasecmp ("ABC", "ABC", 3));
    LONGS_EQUAL(0, string_strncasecmp ("ABCABC", "ABCDEF", 3));
    LONGS_EQUAL(-3, string_strncasecmp ("ABCABC", "ABCDEF", 6));
    LONGS_EQUAL(-3, string_strncasecmp ("abc", "def", 3));
    LONGS_EQUAL(-3, string_strncasecmp ("abc", "DEF", 3));
    LONGS_EQUAL(-3, string_strncasecmp ("ABC", "def", 3));
    LONGS_EQUAL(-3, string_strncasecmp ("ABC", "DEF", 3));
    LONGS_EQUAL(3, string_strncasecmp ("def", "abc", 3));
    LONGS_EQUAL(3, string_strncasecmp ("def", "ABC", 3));
    LONGS_EQUAL(3, string_strncasecmp ("DEF", "abc", 3));
    LONGS_EQUAL(3, string_strncasecmp ("DEF", "ABC", 3));
    LONGS_EQUAL(-9, string_strncasecmp ("à", "é", 1));
    LONGS_EQUAL(0, string_strncasecmp ("ê", "Ê", 1));

    /* case-insensitive comparison with a range */
    LONGS_EQUAL(0, string_strcasecmp_range (NULL, NULL, 30));
    LONGS_EQUAL(-1, string_strcasecmp_range (NULL, "", 30));
    LONGS_EQUAL(1, string_strcasecmp_range ("", NULL, 30));
    LONGS_EQUAL(-1, string_strcasecmp_range (NULL, "abc", 30));
    LONGS_EQUAL(1, string_strcasecmp_range ("abc", NULL, 30));
    LONGS_EQUAL(-97, string_strcasecmp_range ("", "abc", 30));
    LONGS_EQUAL(97, string_strcasecmp_range ("abc", "", 30));
    LONGS_EQUAL(-98, string_strcasecmp_range ("", "b", 30));
    LONGS_EQUAL(98, string_strcasecmp_range ("b", "", 30));
    LONGS_EQUAL(-2, string_strcasecmp_range ("A", "C", 30));
    LONGS_EQUAL(2, string_strcasecmp_range ("C", "A", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("A", "a", 30));
    LONGS_EQUAL(-8129, string_strcasecmp_range ("ë", "€", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("[", "{", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("]", "}", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("\\", "|", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("^", "~", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("[", "{", 29));
    LONGS_EQUAL(0, string_strcasecmp_range ("]", "}", 29));
    LONGS_EQUAL(0, string_strcasecmp_range ("\\", "|", 29));
    LONGS_EQUAL(-32, string_strcasecmp_range ("^", "~", 29));
    LONGS_EQUAL(32, string_strcasecmp_range ("~", "^", 29));
    LONGS_EQUAL(-32, string_strcasecmp_range ("[", "{", 26));
    LONGS_EQUAL(32, string_strcasecmp_range ("{", "[", 26));
    LONGS_EQUAL(-32, string_strcasecmp_range ("]", "}", 26));
    LONGS_EQUAL(32, string_strcasecmp_range ("}", "]", 26));
    LONGS_EQUAL(-32, string_strcasecmp_range ("\\", "|", 26));
    LONGS_EQUAL(32, string_strcasecmp_range ("|", "\\", 26));
    LONGS_EQUAL(-32, string_strcasecmp_range ("^", "~", 26));
    LONGS_EQUAL(32, string_strcasecmp_range ("~", "^", 26));

    /* case-insensitive comparison with max length and a range */
    LONGS_EQUAL(0, string_strncasecmp_range (NULL, NULL, 3, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range (NULL, "", 3, 30));
    LONGS_EQUAL(1, string_strncasecmp_range ("", NULL, 3, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range (NULL, "abc", 3, 30));
    LONGS_EQUAL(1, string_strncasecmp_range ("abc", NULL, 3, 30));
    LONGS_EQUAL(-97, string_strncasecmp_range ("", "abc", 3, 30));
    LONGS_EQUAL(97, string_strncasecmp_range ("abc", "", 3, 30));
    LONGS_EQUAL(-98, string_strncasecmp_range ("", "b", 3, 30));
    LONGS_EQUAL(98, string_strncasecmp_range ("b", "", 3, 30));
    LONGS_EQUAL(-2, string_strncasecmp_range ("ABC", "CCC", 3, 30));
    LONGS_EQUAL(2, string_strncasecmp_range ("CCC", "ABC", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("ABC", "abc", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("ABCABC", "abcdef", 3, 30));
    LONGS_EQUAL(-3, string_strncasecmp_range ("ABCABC", "abcdef", 6, 30));
    LONGS_EQUAL(-8129, string_strncasecmp_range ("ëëë", "€€€", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("[[[", "{{{", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("[[[abc", "{{{def", 3, 30));
    LONGS_EQUAL(-3, string_strncasecmp_range ("[[[abc", "{{{def", 6, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("]]]", "}}}", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("]]]abc", "}}}def", 3, 30));
    LONGS_EQUAL(-3, string_strncasecmp_range ("]]]abc", "}}}def", 6, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("\\\\\\", "|||", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("\\\\\\abc", "|||def", 3, 30));
    LONGS_EQUAL(-3, string_strncasecmp_range ("\\\\\\abc", "|||def", 6, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("^^^", "~~~", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("^^^abc", "~~~def", 3, 30));
    LONGS_EQUAL(-3, string_strncasecmp_range ("^^^abc", "~~~def", 6, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("[[[", "{{{", 3, 29));
    LONGS_EQUAL(0, string_strncasecmp_range ("]]]", "}}}", 3, 29));
    LONGS_EQUAL(0, string_strncasecmp_range ("\\\\\\", "|||", 3, 29));
    LONGS_EQUAL(-32, string_strncasecmp_range ("^^^", "~~~", 3, 29));
    LONGS_EQUAL(32, string_strncasecmp_range ("~~~", "^^^", 3, 29));
    LONGS_EQUAL(-32, string_strncasecmp_range ("[[[", "{{{", 3, 26));
    LONGS_EQUAL(-32, string_strncasecmp_range ("]]]", "}}}", 3, 26));
    LONGS_EQUAL(-32, string_strncasecmp_range ("\\\\\\", "|||", 3, 26));
    LONGS_EQUAL(-32, string_strncasecmp_range ("^^^", "~~~", 3, 26));

    /* comparison with chars ignored */
    LONGS_EQUAL(0, string_strcmp_ignore_chars (NULL, NULL, "", 0));
    LONGS_EQUAL(-1, string_strcmp_ignore_chars (NULL, "", "", 0));
    LONGS_EQUAL(1, string_strcmp_ignore_chars ("", NULL, "", 0));
    LONGS_EQUAL(-1, string_strcmp_ignore_chars (NULL, "abc", "", 0));
    LONGS_EQUAL(1, string_strcmp_ignore_chars ("abc", NULL, "", 0));
    LONGS_EQUAL(-97, string_strcmp_ignore_chars ("", "abc", "", 0));
    LONGS_EQUAL(97, string_strcmp_ignore_chars ("abc", "", "", 0));
    LONGS_EQUAL(-98, string_strcmp_ignore_chars ("", "b", "", 0));
    LONGS_EQUAL(98, string_strcmp_ignore_chars ("b", "", "", 0));
    LONGS_EQUAL(-2, string_strcmp_ignore_chars ("ABC", "CCC", "", 0));
    LONGS_EQUAL(2, string_strcmp_ignore_chars ("CCC", "ABC", "", 0));
    LONGS_EQUAL(0, string_strcmp_ignore_chars ("ABC", "abc", "", 0));
    LONGS_EQUAL(-32, string_strcmp_ignore_chars ("ABC", "abc", "", 1));
    LONGS_EQUAL(0, string_strcmp_ignore_chars ("abc..abc", "abcabc", ".", 0));
    LONGS_EQUAL(32, string_strcmp_ignore_chars ("abc..abc", "ABCABC", ".", 1));
    LONGS_EQUAL(0, string_strcmp_ignore_chars ("abc..abc", "abc-.-.abc",
                                               ".-", 0));
    LONGS_EQUAL(32, string_strcmp_ignore_chars ("abc..abc", "ABC-.-.ABC",
                                                ".-", 1));
    LONGS_EQUAL(0, string_strcmp_ignore_chars (".abc..abc", "..abcabc", ".", 0));
    LONGS_EQUAL(97, string_strcmp_ignore_chars (".abc..abc", "..", ".", 0));
    LONGS_EQUAL(-97, string_strcmp_ignore_chars (".", "..abcabc", ".", 0));
    LONGS_EQUAL(0, string_strcmp_ignore_chars (".", ".", ".", 0));
    LONGS_EQUAL(-2, string_strcmp_ignore_chars ("è", "ê", "", 0));
    LONGS_EQUAL(-2, string_strcmp_ignore_chars ("è", "Ê", "", 0));
    LONGS_EQUAL(-2, string_strcmp_ignore_chars ("è", "ê", "", 1));
}

/*
 * Tests functions:
 *   string_strcasestr
 */

TEST(CoreString, Search)
{
    const char *str = "test";

    /* case-insensitive search of string in a string */
    STRCMP_EQUAL(NULL, string_strcasestr (NULL, NULL));
    STRCMP_EQUAL(NULL, string_strcasestr (NULL, str));
    STRCMP_EQUAL(NULL, string_strcasestr (str, NULL));
    STRCMP_EQUAL(NULL, string_strcasestr (str, ""));
    STRCMP_EQUAL(NULL, string_strcasestr (str, "zz"));
    STRCMP_EQUAL(str + 1, string_strcasestr (str, "est"));
    STRCMP_EQUAL(str + 1, string_strcasestr (str, "EST"));
}

/*
 * Tests functions:
 *   string_match
 */

TEST(CoreString, Match)
{
    LONGS_EQUAL(0, string_match (NULL, NULL, 0));
    LONGS_EQUAL(0, string_match (NULL, "test", 0));
    LONGS_EQUAL(0, string_match ("test", NULL, 0));
    LONGS_EQUAL(0, string_match ("", "", 0));
    LONGS_EQUAL(0, string_match ("", "test", 0));
    LONGS_EQUAL(0, string_match ("test", "", 0));
    LONGS_EQUAL(0, string_match ("test", "def", 0));
    LONGS_EQUAL(0, string_match ("test", "def", 1));
    LONGS_EQUAL(0, string_match ("test", "def*", 0));
    LONGS_EQUAL(0, string_match ("test", "def*", 1));
    LONGS_EQUAL(0, string_match ("test", "*def", 0));
    LONGS_EQUAL(0, string_match ("test", "*def", 1));
    LONGS_EQUAL(0, string_match ("test", "*def*", 0));
    LONGS_EQUAL(0, string_match ("test", "*def*", 1));
    LONGS_EQUAL(0, string_match ("test", "def", 0));
    LONGS_EQUAL(0, string_match ("test", "def", 1));
    LONGS_EQUAL(0, string_match ("test", "es", 0));
    LONGS_EQUAL(0, string_match ("test", "es", 1));
    LONGS_EQUAL(0, string_match ("test", "es*", 0));
    LONGS_EQUAL(0, string_match ("test", "es*", 1));
    LONGS_EQUAL(0, string_match ("test", "*es", 0));
    LONGS_EQUAL(0, string_match ("test", "*es", 1));
    LONGS_EQUAL(1, string_match ("test", "*es*", 0));
    LONGS_EQUAL(1, string_match ("test", "**es**", 0));
    LONGS_EQUAL(1, string_match ("test", "*es*", 1));
    LONGS_EQUAL(1, string_match ("test", "*ES*", 0));
    LONGS_EQUAL(0, string_match ("test", "*ES*", 1));
    LONGS_EQUAL(1, string_match ("TEST", "*es*", 0));
    LONGS_EQUAL(0, string_match ("TEST", "*es*", 1));
    LONGS_EQUAL(0, string_match ("aaba", "*aa", 0));
    LONGS_EQUAL(0, string_match ("aaba", "*aa", 1));
    LONGS_EQUAL(1, string_match ("abaa", "*aa", 0));
    LONGS_EQUAL(1, string_match ("abaa", "*aa", 1));
    LONGS_EQUAL(1, string_match ("aabaa", "*aa", 0));
    LONGS_EQUAL(1, string_match ("aabaa", "*aa", 1));
    LONGS_EQUAL(1, string_match ("aabaabaabaa", "*aa", 0));
    LONGS_EQUAL(1, string_match ("aabaabaabaa", "*aa", 1));
    LONGS_EQUAL(0, string_match ("abaa", "aa*", 0));
    LONGS_EQUAL(0, string_match ("abaa", "aa*", 1));
    LONGS_EQUAL(1, string_match ("aaba", "aa*", 0));
    LONGS_EQUAL(1, string_match ("aaba", "aa*", 1));
    LONGS_EQUAL(1, string_match ("aabaa", "aa*", 0));
    LONGS_EQUAL(1, string_match ("aabaa", "aa*", 1));
    LONGS_EQUAL(1, string_match ("aabaabaabaa", "aa*", 0));
    LONGS_EQUAL(1, string_match ("aabaabaabaa", "aa*", 1));
    LONGS_EQUAL(1, string_match ("script.color.description", "*script.color*", 0));
    LONGS_EQUAL(1, string_match ("script.color.description", "*script.color*", 1));
    LONGS_EQUAL(1, string_match ("script.color.description", "*script.COLOR*", 0));
    LONGS_EQUAL(0, string_match ("script.color.description", "*script.COLOR*", 1));
    LONGS_EQUAL(1, string_match ("script.color.description", "*script*color*", 0));
    LONGS_EQUAL(1, string_match ("script.color.description", "*script*color*", 1));
    LONGS_EQUAL(1, string_match ("script.color.description", "*script*COLOR*", 0));
    LONGS_EQUAL(0, string_match ("script.color.description", "*script*COLOR*", 1));
    LONGS_EQUAL(1, string_match ("script.script.script", "scr*scr*scr*", 0));
    LONGS_EQUAL(1, string_match ("script.script.script", "SCR*SCR*SCR*", 0));
    LONGS_EQUAL(0, string_match ("script.script.script", "SCR*SCR*SCR*", 1));
    LONGS_EQUAL(0, string_match ("script.script.script", "scr*scr*scr*scr*", 0));
}

/*
 * Tests functions:
 *   string_match_list
 */

TEST(CoreString, MatchList)
{
    const char *masks_none[1] = { NULL };
    const char *masks_one_empty[2] = { "", NULL };
    const char *masks_one[2] = { "toto", NULL };
    const char *masks_two[3] = { "toto", "abc", NULL };
    const char *masks_negative[3] = { "*", "!abc", NULL };
    const char *masks_negative_star[3] = { "*", "!abc*", NULL };

    LONGS_EQUAL(0, string_match_list (NULL, NULL, 0));
    LONGS_EQUAL(0, string_match_list (NULL, masks_one, 0));

    LONGS_EQUAL(0, string_match_list ("", NULL, 0));
    LONGS_EQUAL(0, string_match_list ("", masks_none, 0));
    LONGS_EQUAL(0, string_match_list ("", masks_one_empty, 0));
    LONGS_EQUAL(0, string_match_list ("", masks_none, 0));
    LONGS_EQUAL(0, string_match_list ("", masks_one_empty, 0));

    LONGS_EQUAL(0, string_match_list ("toto", NULL, 0));
    LONGS_EQUAL(0, string_match_list ("toto", masks_none, 0));
    LONGS_EQUAL(0, string_match_list ("toto", masks_one_empty, 0));
    LONGS_EQUAL(0, string_match_list ("toto", masks_none, 0));
    LONGS_EQUAL(0, string_match_list ("toto", masks_one_empty, 0));

    LONGS_EQUAL(0, string_match_list ("test", masks_one, 0));
    LONGS_EQUAL(0, string_match_list ("to", masks_one, 0));
    LONGS_EQUAL(1, string_match_list ("toto", masks_one, 0));
    LONGS_EQUAL(1, string_match_list ("TOTO", masks_one, 0));
    LONGS_EQUAL(0, string_match_list ("TOTO", masks_one, 1));

    LONGS_EQUAL(0, string_match_list ("test", masks_two, 0));
    LONGS_EQUAL(1, string_match_list ("toto", masks_two, 0));
    LONGS_EQUAL(1, string_match_list ("abc", masks_two, 0));
    LONGS_EQUAL(0, string_match_list ("def", masks_two, 0));

    LONGS_EQUAL(1, string_match_list ("test", masks_negative, 0));
    LONGS_EQUAL(1, string_match_list ("toto", masks_negative, 0));
    LONGS_EQUAL(0, string_match_list ("abc", masks_negative, 0));
    LONGS_EQUAL(0, string_match_list ("ABC", masks_negative, 0));
    LONGS_EQUAL(1, string_match_list ("ABC", masks_negative, 1));
    LONGS_EQUAL(1, string_match_list ("abcdef", masks_negative, 0));
    LONGS_EQUAL(1, string_match_list ("ABCDEF", masks_negative, 0));
    LONGS_EQUAL(1, string_match_list ("ABCDEF", masks_negative, 1));
    LONGS_EQUAL(1, string_match_list ("def", masks_negative, 0));

    LONGS_EQUAL(1, string_match_list ("test", masks_negative_star, 0));
    LONGS_EQUAL(1, string_match_list ("toto", masks_negative_star, 0));
    LONGS_EQUAL(0, string_match_list ("abc", masks_negative_star, 0));
    LONGS_EQUAL(0, string_match_list ("ABC", masks_negative_star, 0));
    LONGS_EQUAL(1, string_match_list ("ABC", masks_negative_star, 1));
    LONGS_EQUAL(0, string_match_list ("abcdef", masks_negative_star, 0));
    LONGS_EQUAL(0, string_match_list ("ABCDEF", masks_negative_star, 0));
    LONGS_EQUAL(1, string_match_list ("ABCDEF", masks_negative_star, 1));
    LONGS_EQUAL(1, string_match_list ("def", masks_negative_star, 0));
}

/*
 * Tests functions:
 *   string_expand_home
 */

TEST(CoreString, ExpandHome)
{
    char *home, *result;
    int length_home;

    home = getenv ("HOME");
    CHECK(home);
    length_home = strlen (home);

    STRCMP_EQUAL(NULL, string_expand_home (NULL));

    result = string_expand_home ("~/abc.txt");
    CHECK(strncmp (result, home, length_home) == 0);
    LONGS_EQUAL(length_home + 8, strlen (result));
    STRCMP_EQUAL(result + length_home, "/abc.txt");
    free (result);
}

/*
 * Tests functions:
 *   string_eval_path_home
 */

TEST(CoreString, EvalPathHome)
{
    char *home, *result;
    int length_home, length_weechat_config_dir, length_weechat_data_dir;
    int length_weechat_state_dir, length_weechat_cache_dir;
    int length_weechat_runtime_dir;
    struct t_hashtable *extra_vars, *options;

    home = getenv ("HOME");
    CHECK(home);
    length_home = strlen (home);

    length_weechat_config_dir = strlen (weechat_config_dir);
    length_weechat_data_dir = strlen (weechat_data_dir);
    length_weechat_state_dir = strlen (weechat_state_dir);
    length_weechat_cache_dir = strlen (weechat_cache_dir);
    length_weechat_runtime_dir = strlen (weechat_runtime_dir);

    STRCMP_EQUAL(NULL, string_eval_path_home (NULL, NULL, NULL, NULL));

    result = string_eval_path_home ("/tmp/test", NULL, NULL, NULL);
    STRCMP_EQUAL(result, "/tmp/test");
    free (result);

    result = string_eval_path_home ("~/test", NULL, NULL, NULL);
    CHECK(strncmp (result, home, length_home) == 0);
    LONGS_EQUAL(length_home + 5, strlen (result));
    STRCMP_EQUAL(result + length_home, "/test");
    free (result);

    /* "%h" is weechat_data_dir by default */
    result = string_eval_path_home ("%h/test", NULL, NULL, NULL);
    CHECK(strncmp (result, weechat_data_dir, length_weechat_data_dir) == 0);
    LONGS_EQUAL(length_weechat_data_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_data_dir, "/test");
    free (result);

    options = hashtable_new (32,
                             WEECHAT_HASHTABLE_STRING,
                             WEECHAT_HASHTABLE_STRING,
                             NULL, NULL);

    /* "%h" with forced config dir */
    hashtable_set (options, "directory", "config");
    result = string_eval_path_home ("%h/test", NULL, NULL, options);
    CHECK(strncmp (result, weechat_config_dir, length_weechat_config_dir) == 0);
    LONGS_EQUAL(length_weechat_config_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_config_dir, "/test");
    free (result);

    /* "%h" with forced data dir */
    hashtable_set (options, "directory", "data");
    result = string_eval_path_home ("%h/test", NULL, NULL, options);
    CHECK(strncmp (result, weechat_data_dir, length_weechat_data_dir) == 0);
    LONGS_EQUAL(length_weechat_data_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_data_dir, "/test");
    free (result);

    /* "%h" with forced state dir */
    hashtable_set (options, "directory", "state");
    result = string_eval_path_home ("%h/test", NULL, NULL, options);
    CHECK(strncmp (result, weechat_state_dir, length_weechat_state_dir) == 0);
    LONGS_EQUAL(length_weechat_state_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_state_dir, "/test");
    free (result);

    /* "%h" with forced cache dir */
    hashtable_set (options, "directory", "cache");
    result = string_eval_path_home ("%h/test", NULL, NULL, options);
    CHECK(strncmp (result, weechat_cache_dir, length_weechat_cache_dir) == 0);
    LONGS_EQUAL(length_weechat_cache_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_cache_dir, "/test");
    free (result);

    /* "%h" with forced runtime dir */
    hashtable_set (options, "directory", "runtime");
    result = string_eval_path_home ("%h/test", NULL, NULL, options);
    CHECK(strncmp (result, weechat_runtime_dir, length_weechat_runtime_dir) == 0);
    LONGS_EQUAL(length_weechat_runtime_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_runtime_dir, "/test");
    free (result);

    hashtable_free (options);

    /* config dir */
    result = string_eval_path_home ("${weechat_config_dir}/path",
                                    NULL, NULL, NULL);
    CHECK(strncmp (result, weechat_config_dir, length_weechat_config_dir) == 0);
    LONGS_EQUAL(length_weechat_config_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_config_dir, "/path");
    free (result);

    /* data dir */
    result = string_eval_path_home ("${weechat_data_dir}/path",
                                    NULL, NULL, NULL);
    CHECK(strncmp (result, weechat_data_dir, length_weechat_data_dir) == 0);
    LONGS_EQUAL(length_weechat_data_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_data_dir, "/path");
    free (result);

    /* state dir */
    result = string_eval_path_home ("${weechat_state_dir}/path",
                                    NULL, NULL, NULL);
    CHECK(strncmp (result, weechat_state_dir, length_weechat_state_dir) == 0);
    LONGS_EQUAL(length_weechat_state_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_state_dir, "/path");
    free (result);

    /* cache dir */
    result = string_eval_path_home ("${weechat_cache_dir}/path",
                                    NULL, NULL, NULL);
    CHECK(strncmp (result, weechat_cache_dir, length_weechat_cache_dir) == 0);
    LONGS_EQUAL(length_weechat_cache_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_cache_dir, "/path");
    free (result);

    /* runtime dir */
    result = string_eval_path_home ("${weechat_runtime_dir}/path",
                                    NULL, NULL, NULL);
    CHECK(strncmp (result, weechat_runtime_dir, length_weechat_runtime_dir) == 0);
    LONGS_EQUAL(length_weechat_runtime_dir + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_runtime_dir, "/path");
    free (result);

    setenv ("WEECHAT_TEST_PATH", "path1", 1);

    result = string_eval_path_home ("%h/${env:WEECHAT_TEST_PATH}/path2",
                                    NULL, NULL, NULL);
    CHECK(strncmp (result, weechat_data_dir, length_weechat_data_dir) == 0);
    LONGS_EQUAL(length_weechat_data_dir + 12, strlen (result));
    STRCMP_EQUAL(result + length_weechat_data_dir, "/path1/path2");
    free (result);

    extra_vars = hashtable_new (32,
                                WEECHAT_HASHTABLE_STRING,
                                WEECHAT_HASHTABLE_STRING,
                                NULL, NULL);
    CHECK(extra_vars);
    hashtable_set (extra_vars, "path2", "value");

    result = string_eval_path_home ("%h/${env:WEECHAT_TEST_PATH}/${path2}",
                                    NULL, extra_vars, NULL);
    CHECK(strncmp (result, weechat_data_dir, length_weechat_data_dir) == 0);
    LONGS_EQUAL(length_weechat_data_dir + 12, strlen (result));
    STRCMP_EQUAL(result + length_weechat_data_dir, "/path1/value");
    free (result);

    hashtable_free (extra_vars);
}

/*
 * Tests functions:
 *   string_remove_quotes
 */

TEST(CoreString, RemoveQuotes)
{
    char *str;

    WEE_TEST_STR(NULL, string_remove_quotes (NULL, NULL));
    WEE_TEST_STR(NULL, string_remove_quotes (NULL, "abc"));
    WEE_TEST_STR(NULL, string_remove_quotes ("abc", NULL));
    WEE_TEST_STR("", string_remove_quotes("", ""));
    WEE_TEST_STR("", string_remove_quotes("", "\"'"));
    WEE_TEST_STR("abc", string_remove_quotes("abc", "\"'"));
    WEE_TEST_STR(" abc ", string_remove_quotes(" abc ", "\"'"));
    WEE_TEST_STR("abc", string_remove_quotes("'abc'", "\"'"));
    WEE_TEST_STR("abc", string_remove_quotes(" 'abc' ", "\"'"));
    WEE_TEST_STR("'abc'", string_remove_quotes("\"'abc'\"", "\"'"));
    WEE_TEST_STR("'abc'", string_remove_quotes(" \"'abc'\" ", "\"'"));
    WEE_TEST_STR("'a'b'c'", string_remove_quotes("\"'a'b'c'\"", "\"'"));
    WEE_TEST_STR("'a'b'c'", string_remove_quotes(" \"'a'b'c'\" ", "\"'"));
}

/*
 * Tests functions:
 *   string_strip
 */

TEST(CoreString, Strip)
{
    char *str;

    WEE_TEST_STR(NULL, string_strip (NULL, 1, 1, NULL));
    WEE_TEST_STR(NULL, string_strip (NULL, 1, 1, ".;"));
    WEE_TEST_STR("test", string_strip ("test", 1, 1, NULL));
    WEE_TEST_STR("test", string_strip ("test", 1, 1, ".;"));
    WEE_TEST_STR(".-test.-", string_strip (".-test.-", 0, 0, ".-"));
    WEE_TEST_STR("test", string_strip (".-test.-", 1, 1, ".-"));
    WEE_TEST_STR("test.-", string_strip (".-test.-", 1, 0, ".-"));
    WEE_TEST_STR(".-test", string_strip (".-test.-", 0, 1, ".-"));
}

/*
 * Tests functions:
 *   string_convert_escaped_chars
 */

TEST(CoreString, ConvertEscapedChars)
{
    char *str;

    WEE_TEST_STR(NULL, string_convert_escaped_chars (NULL));
    WEE_TEST_STR("", string_convert_escaped_chars (""));
    WEE_TEST_STR("", string_convert_escaped_chars ("\\"));
    WEE_TEST_STR("\"", string_convert_escaped_chars ("\\\""));
    WEE_TEST_STR("\\", string_convert_escaped_chars ("\\\\"));
    WEE_TEST_STR("\a", string_convert_escaped_chars ("\\a"));
    WEE_TEST_STR("\a", string_convert_escaped_chars ("\\a"));
    WEE_TEST_STR("\b", string_convert_escaped_chars ("\\b"));
    WEE_TEST_STR("\e", string_convert_escaped_chars ("\\e"));
    WEE_TEST_STR("\f", string_convert_escaped_chars ("\\f"));
    WEE_TEST_STR("\n", string_convert_escaped_chars ("\\n"));
    WEE_TEST_STR("\r", string_convert_escaped_chars ("\\r"));
    WEE_TEST_STR("\t", string_convert_escaped_chars ("\\t"));
    WEE_TEST_STR("\v", string_convert_escaped_chars ("\\v"));
    WEE_TEST_STR("\123", string_convert_escaped_chars ("\\0123"));
    WEE_TEST_STR("\123",
                 string_convert_escaped_chars ("\\0123"));  /* invalid */
    WEE_TEST_STR("\x41", string_convert_escaped_chars ("\\x41"));
    WEE_TEST_STR("\x04z", string_convert_escaped_chars ("\\x4z"));
    WEE_TEST_STR("xzzy", string_convert_escaped_chars ("\\xzzy"));
    WEE_TEST_STR(" zz", string_convert_escaped_chars ("\\u20zz"));
    WEE_TEST_STR("\U00012345", string_convert_escaped_chars ("\\U00012345"));
    WEE_TEST_STR("\U00000123zzz",
                 string_convert_escaped_chars ("\\U00123zzz"));
    WEE_TEST_STR("",
                 string_convert_escaped_chars ("\\U12345678")); /* invalid */
    WEE_TEST_STR("Uzzy", string_convert_escaped_chars ("\\Uzzy"));
    WEE_TEST_STR("\\~zzy", string_convert_escaped_chars ("\\~zzy"));
}

/*
 * Tests functions:
 *   string_is_whitespace_char
 */

TEST(CoreString, IsWhitespaceChar)
{
    LONGS_EQUAL(0, string_is_whitespace_char (NULL));
    LONGS_EQUAL(0, string_is_whitespace_char (""));
    LONGS_EQUAL(0, string_is_whitespace_char ("abc def"));

    LONGS_EQUAL(1, string_is_whitespace_char (" abc def"));
    LONGS_EQUAL(1, string_is_whitespace_char ("\tabc def"));
    LONGS_EQUAL(1, string_is_whitespace_char ("\nabc def"));
    LONGS_EQUAL(1, string_is_whitespace_char ("\rabc def"));
}

/*
 * Tests functions:
 *   string_is_word_char_highlight
 *   string_is_word_char_input
 */

TEST(CoreString, IsWordChar)
{
    WEE_IS_WORD_CHAR(0, NULL);
    WEE_IS_WORD_CHAR(0, "");
    WEE_IS_WORD_CHAR(0, " abc");       /* space */
    WEE_IS_WORD_CHAR(0, "\u00A0abc");  /* unbreakable space */
    WEE_IS_WORD_CHAR(0, "&abc");
    WEE_IS_WORD_CHAR(0, "+abc");
    WEE_IS_WORD_CHAR(0, "$abc");
    WEE_IS_WORD_CHAR(0, "*abc");
    WEE_IS_WORD_CHAR(0, "/abc");
    WEE_IS_WORD_CHAR(0, "\\abc");

    WEE_IS_WORD_CHAR(1, "abc");
    WEE_IS_WORD_CHAR(1, "1abc");
    WEE_IS_WORD_CHAR(1, "-abc");
    WEE_IS_WORD_CHAR(1, "_abc");
    WEE_IS_WORD_CHAR(1, "|abc");
}

/*
 * Tests functions:
 *   string_mask_to_regex
 */

TEST(CoreString, MaskToRegex)
{
    char *str;

    WEE_TEST_STR(NULL, string_mask_to_regex (NULL));
    WEE_TEST_STR("", string_mask_to_regex (""));
    WEE_TEST_STR("test", string_mask_to_regex ("test"));
    WEE_TEST_STR("test.*", string_mask_to_regex ("test*"));
    WEE_TEST_STR(".*test.*", string_mask_to_regex ("*test*"));
    WEE_TEST_STR(".*te.*st.*", string_mask_to_regex ("*te*st*"));
    WEE_TEST_STR("test\\.\\[\\]\\{\\}\\(\\)\\?\\+\\|\\^\\$\\\\",
                 string_mask_to_regex ("test.[]{}()?+|^$\\"));
}

/*
 * Tests functions:
 *   string_regex_flags
 *   string_regcomp
 */

TEST(CoreString, Regex)
{
    int flags;
    const char *ptr;
    regex_t regex;

    string_regex_flags (NULL, 0, NULL);
    string_regex_flags ("", 0, NULL);

    string_regex_flags (NULL, 0, &flags);
    LONGS_EQUAL(0, flags);
    string_regex_flags ("", 0, &flags);
    LONGS_EQUAL(0, flags);
    string_regex_flags (NULL, REG_EXTENDED, &flags);
    LONGS_EQUAL(REG_EXTENDED, flags);
    string_regex_flags ("", REG_EXTENDED, &flags);
    LONGS_EQUAL(REG_EXTENDED, flags);

    ptr = string_regex_flags ("test1", REG_EXTENDED, &flags);
    LONGS_EQUAL(REG_EXTENDED, flags);
    STRCMP_EQUAL("test1", ptr);

    ptr = string_regex_flags ("(?e)test2", 0, &flags);
    LONGS_EQUAL(REG_EXTENDED, flags);
    STRCMP_EQUAL("test2", ptr);

    ptr = string_regex_flags ("(?ei)test3", 0, &flags);
    LONGS_EQUAL(REG_EXTENDED | REG_ICASE, flags);
    STRCMP_EQUAL("test3", ptr);

    ptr = string_regex_flags ("(?eins)test4", 0, &flags);
    LONGS_EQUAL(REG_EXTENDED | REG_ICASE | REG_NEWLINE | REG_NOSUB, flags);
    STRCMP_EQUAL("test4", ptr);

    ptr = string_regex_flags ("(?ins)test5", REG_EXTENDED, &flags);
    LONGS_EQUAL(REG_EXTENDED | REG_ICASE | REG_NEWLINE | REG_NOSUB, flags);
    STRCMP_EQUAL("test5", ptr);

    ptr = string_regex_flags ("(?ins-e)test6", REG_EXTENDED, &flags);
    LONGS_EQUAL(REG_ICASE | REG_NEWLINE | REG_NOSUB, flags);
    STRCMP_EQUAL("test6", ptr);

    /* compile regular expression */
    LONGS_EQUAL(-1, string_regcomp (&regex, NULL, 0));
    LONGS_EQUAL(0, string_regcomp (&regex, "", 0));
    regfree (&regex);
    LONGS_EQUAL(0, string_regcomp (&regex, "test", 0));
    regfree (&regex);
    LONGS_EQUAL(0, string_regcomp (&regex, "test", REG_EXTENDED));
    regfree (&regex);
    LONGS_EQUAL(0, string_regcomp (&regex, "(?ins)test", REG_EXTENDED));
    regfree (&regex);
}

/*
 * Tests functions:
 *   string_has_highlight
 *   string_has_highlight_regex_compiled
 *   string_has_highlight_regex
 */

TEST(CoreString, Highlight)
{
    regex_t regex;

    /* check highlight with a string */
    WEE_HAS_HL_STR(0, NULL, NULL);
    WEE_HAS_HL_STR(0, NULL, "");
    WEE_HAS_HL_STR(0, "", NULL);
    WEE_HAS_HL_STR(0, "", "");
    WEE_HAS_HL_STR(0, "test", "");
    WEE_HAS_HL_STR(0, "", "test");
    WEE_HAS_HL_STR(0, "test-here", "test");
    WEE_HAS_HL_STR(0, "this is a test here", "abc,def");
    WEE_HAS_HL_STR(1, "test", "test");
    WEE_HAS_HL_STR(1, "this is a test", "test");
    WEE_HAS_HL_STR(1, "test here", "test");
    WEE_HAS_HL_STR(1, "test: here", "test");
    WEE_HAS_HL_STR(1, "test : here", "test");
    WEE_HAS_HL_STR(1, "test\u00A0here", "test");   /* unbreakable space */
    WEE_HAS_HL_STR(1, "test\u00A0:here", "test");  /* unbreakable space */
    WEE_HAS_HL_STR(1, "this is a test here", "test");
    WEE_HAS_HL_STR(1, "this is a test here", "abc,test");

    /*
     * check highlight with a regex, each call of macro
     * checks with a regex as string, and then a compiled regex
     */
    WEE_HAS_HL_REGEX(-1, 0, NULL, NULL);
    WEE_HAS_HL_REGEX(0, 0, NULL, "");
    WEE_HAS_HL_REGEX(-1, 0,  "", NULL);
    WEE_HAS_HL_REGEX(0, 0, "", "");
    WEE_HAS_HL_REGEX(0, 0, "test", "");
    WEE_HAS_HL_REGEX(0, 0, "", "test");
    WEE_HAS_HL_REGEX(0, 1, "test", "test");
    WEE_HAS_HL_REGEX(0, 1, "this is a test", "test");
    WEE_HAS_HL_REGEX(0, 1, "abc tested", "test.*");
    WEE_HAS_HL_REGEX(0, 1, "abc tested here", "test.*");
    WEE_HAS_HL_REGEX(0, 1, "tested here", "test.*");
    WEE_HAS_HL_REGEX(0, 0, "this is a test", "teste.*");
    WEE_HAS_HL_REGEX(0, 0, "test here", "teste.*");
}

/*
 * Test callback for function string_replace_with_callback.
 *
 * It replaces "abc" by "def", "empty" by empty string, and for any other value
 * it returns NULL (so the value is kept as-is).
 */

char *
test_replace_cb (void *data,
                 const char *prefix, const char *text, const char *suffix)
{
    /* make C++ compiler happy */
    (void) data;
    (void) prefix;
    (void) suffix;

    if (strcmp (text, "abc") == 0)
        return strdup ("def");

    if (strcmp (text, "empty") == 0)
        return strdup ("");

    if (strncmp (text, "no_replace:", 11) == 0)
        return strdup (text);

    return NULL;
}

/*
 * Tests functions:
 *   string_replace
 */

TEST(CoreString, Replace)
{
    char *str;

    WEE_TEST_STR(NULL, string_replace (NULL, NULL, NULL));
    WEE_TEST_STR(NULL, string_replace ("string", NULL, NULL));
    WEE_TEST_STR(NULL, string_replace (NULL, "search", NULL));
    WEE_TEST_STR(NULL, string_replace (NULL, NULL, "replace"));
    WEE_TEST_STR(NULL, string_replace ("string", "search", NULL));
    WEE_TEST_STR(NULL, string_replace ("string", NULL, "replace"));
    WEE_TEST_STR(NULL, string_replace (NULL, "search", "replace"));

    WEE_TEST_STR("test abc def", string_replace("test abc def", "xyz", "xxx"));
    WEE_TEST_STR("test xxx def", string_replace("test abc def", "abc", "xxx"));
    WEE_TEST_STR("xxx test xxx def xxx",
                 string_replace("abc test abc def abc", "abc", "xxx"));
}

/*
 * Tests functions:
 *   string_replace_regex
 */

TEST(CoreString, ReplaceRegex)
{
    regex_t regex;
    char *result;

    WEE_REPLACE_REGEX(-1, NULL, NULL, NULL, NULL, '$', NULL);
    WEE_REPLACE_REGEX(0, NULL, NULL, "", NULL, '$', NULL);
    WEE_REPLACE_REGEX(0, "string", "string", "", NULL, '$', NULL);
    WEE_REPLACE_REGEX(0, "test abc def", "test abc def",
                      "xyz", "xxx", '$', NULL);
    WEE_REPLACE_REGEX(0, "test xxx def", "test abc def",
                      "abc", "xxx", '$', NULL);
    WEE_REPLACE_REGEX(0, "foo", "test foo", "^(test +)(.*)", "$2", '$', NULL);
    WEE_REPLACE_REGEX(0, "test / ***", "test foo",
                      "^(test +)(.*)", "$1/ $.*2", '$', NULL);
    WEE_REPLACE_REGEX(0, "%%%", "test foo",
                      "^(test +)(.*)", "$.%+", '$', NULL);
}

/*
 * Tests functions:
 *   string_translate_chars
 */

TEST(CoreString, TranslateChars)
{
    char *str;

    STRCMP_EQUAL(NULL, string_translate_chars (NULL, NULL, NULL));
    STRCMP_EQUAL(NULL, string_translate_chars (NULL, "abc", NULL));
    STRCMP_EQUAL(NULL, string_translate_chars (NULL, "abc", "ABC"));
    STRCMP_EQUAL("", string_translate_chars ("", "abc", "ABCDEF"));
    STRCMP_EQUAL("test", string_translate_chars ("test", "abc", "ABCDEF"));
    WEE_TEST_STR("", string_translate_chars ("", "abc", "ABC"));

    WEE_TEST_STR("tEst", string_translate_chars ("test", "abcdef", "ABCDEF"));

    WEE_TEST_STR("CleAn the BoAt",
                 string_translate_chars ("clean the boat", "abc", "ABC"));

    WEE_TEST_STR("↑", string_translate_chars ("←", "←↑→↓", "↑→↓←"));
    WEE_TEST_STR("→", string_translate_chars ("↑", "←↑→↓", "↑→↓←"));
    WEE_TEST_STR("↓", string_translate_chars ("→", "←↑→↓", "↑→↓←"));
    WEE_TEST_STR("←", string_translate_chars ("↓", "←↑→↓", "↑→↓←"));

    WEE_TEST_STR("uijt jt b uftu",
                 string_translate_chars ("this is a test",
                                         "abcdefghijklmnopqrstuvwxyz",
                                         "bcdefghijklmnopqrstuvwxyza"));

    WEE_TEST_STR("Uijt jt b uftu",
                 string_translate_chars ("This is a test",
                                         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
                                         "bcdefghijklmnopqrstuvwxyzaBCDEFGHIJKLMNOPQRSTUVWXYZA"));
}

/*
 * Tests functions:
 *   string_replace_with_callback
 */

TEST(CoreString, ReplaceWithCallback)
{
    char *result;
    const char *list_prefix_no_replace[] = { "no_replace:", NULL };
    int errors;

    /* tests with invalid arguments */
    WEE_REPLACE_CB(NULL, -1, NULL, NULL, NULL, 1, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, "", NULL, NULL, 1, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, NULL, "", NULL, 1, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, NULL, NULL, "", 1, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, NULL, NULL, NULL, 1, NULL, &test_replace_cb, NULL, NULL);
    WEE_REPLACE_CB(NULL, 0, NULL, NULL, NULL, 1, NULL, NULL, NULL, &errors);
    WEE_REPLACE_CB(NULL, -1, "test", NULL, NULL, 1, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, "test", "${", NULL, 1, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, "test", NULL, "}", 1, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, "test", NULL, NULL, 1, NULL, &test_replace_cb, NULL, NULL);
    WEE_REPLACE_CB(NULL, 0, "test", NULL, NULL, 1, NULL, NULL, NULL, &errors);
    WEE_REPLACE_CB(NULL, -1, "test", "${", "}", 1, NULL, NULL, NULL, NULL);

    /* valid arguments */
    WEE_REPLACE_CB("test", -1, "test", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, NULL);
    WEE_REPLACE_CB("test", 0, "test", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test def", 0, "test ${abc}", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ", 0, "test ${empty}", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ${aaa", 1, "test ${aaa", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ", 0, "test ${empty", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ${empty", 0, "test \\${empty", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test \\${empty", 0, "test \\${empty", "${", "}", 0, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ${aaa}", 1, "test ${aaa}", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test def  ${aaa}", 1, "test ${abc} ${empty} ${aaa}",
                   "${", "}", 1, NULL, &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test def", 0, "test ${abc", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test abc}", 0, "test abc}", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ${}", 1, "test ${}", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ${ }", 1, "test ${ }", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("def", 0, "${abc}", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("", 0, "${empty}", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("${aaa}", 1, "${aaa}", "${", "}", 1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("no_replace:def", 0, "${no_replace:${abc}}", "${", "}",
                   1, NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("no_replace:${abc}", 0, "${no_replace:${abc}}", "${", "}",
                   1, list_prefix_no_replace,
                   &test_replace_cb, NULL, &errors);
}

/*
 * Tests functions:
 *   string_split
 *   string_free_split
 */

TEST(CoreString, Split)
{
    char **argv;
    int argc, flags;

    POINTERS_EQUAL(NULL, string_split (NULL, NULL, NULL, 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split (NULL, "", NULL, 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split ("", NULL, NULL, 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split ("", "", NULL, 0, 0, NULL));

    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;

    argc = -1;
    POINTERS_EQUAL(NULL, string_split (NULL, NULL, NULL, flags, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = -1;
    POINTERS_EQUAL(NULL, string_split (NULL, "", NULL, flags, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = -1;
    POINTERS_EQUAL(NULL, string_split ("", NULL, NULL, flags, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = -1;
    POINTERS_EQUAL(NULL, string_split ("", "", NULL, flags, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = -1;
    POINTERS_EQUAL(NULL, string_split ("", ",", NULL, flags, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = -1;
    POINTERS_EQUAL(NULL, string_split ("   ", " ", NULL, flags, 0, &argc));
    LONGS_EQUAL(0, argc);

    /* free split with NULL */
    string_free_split (NULL);

    /* standard split */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argc = -1;
    argv = string_split ("abc de  fghi", " ", NULL, flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* standard split */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argc = -1;
    argv = string_split (" abc de  fghi ", " ", NULL, flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* max 2 items */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argc = -1;
    argv = string_split (" abc de  fghi ", " ", NULL, flags, 2, &argc);
    LONGS_EQUAL(2, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL(NULL, argv[2]);
    string_free_split (argv);

    /* strip left/right, keep eol for each value */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
        | WEECHAT_STRING_SPLIT_KEEP_EOL;
    argc = -1;
    argv = string_split (" abc de  fghi ", " ", NULL, flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc de  fghi", argv[0]);
    STRCMP_EQUAL("de  fghi", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* strip left/right, keep eol for each value, max 2 items */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
        | WEECHAT_STRING_SPLIT_KEEP_EOL;
    argc = -1;
    argv = string_split (" abc de  fghi ", " ", NULL, flags, 2, &argc);
    LONGS_EQUAL(2, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc de  fghi", argv[0]);
    STRCMP_EQUAL("de  fghi", argv[1]);
    STRCMP_EQUAL(NULL, argv[2]);
    string_free_split (argv);

    /* strip left, keep eol for each value */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
        | WEECHAT_STRING_SPLIT_KEEP_EOL;
    argc = -1;
    argv = string_split (" abc de  fghi ", " ", NULL, flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc de  fghi ", argv[0]);
    STRCMP_EQUAL("de  fghi ", argv[1]);
    STRCMP_EQUAL("fghi ", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* strip left, keep eol for each value, max 2 items */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
        | WEECHAT_STRING_SPLIT_KEEP_EOL;
    argc = -1;
    argv = string_split (" abc de  fghi ", " ", NULL, flags, 2, &argc);
    LONGS_EQUAL(2, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc de  fghi ", argv[0]);
    STRCMP_EQUAL("de  fghi ", argv[1]);
    STRCMP_EQUAL(NULL, argv[2]);
    string_free_split (argv);

    /* standard split with comma separator */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argc = -1;
    argv = string_split ("abc,de,fghi", ",", NULL, flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /*
     * standard split with comma separator,
     * strip_items set to empty string (ignored)
     */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argc = -1;
    argv = string_split (" abc ,, de ,fghi ,,", ",", "", flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL(" abc ", argv[0]);
    STRCMP_EQUAL(" de ", argv[1]);
    STRCMP_EQUAL("fghi ", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /*
     * standard split with comma separator,
     * strip spaces in items (left/right)
     */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argc = -1;
    argv = string_split (" abc ,, de ,fghi ,,", ",", " ", flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /*
     * standard split with comma separator,
     * strip spaces and parentheses in items (left/right)
     */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argc = -1;
    argv = string_split (" abc ,, (de) ,(f(g)hi) ,,", ",", " ()",
                         flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("f(g)hi", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* standard split with comma separator and empty item (ignore this item) */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argc = -1;
    argv = string_split ("abc,,fghi", ",", NULL, flags, 0, &argc);
    LONGS_EQUAL(2, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("fghi", argv[1]);
    STRCMP_EQUAL(NULL, argv[2]);
    string_free_split (argv);

    /* standard split with comma separator and empty item (keep this item) */
    flags = 0;
    argc = -1;
    argv = string_split ("abc,,fghi", ",", NULL, flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* standard split with comma separator and empty items (keep them) */
    flags = 0;
    argc = -1;
    argv = string_split (",abc,,fghi,", ",", NULL, flags, 0, &argc);
    LONGS_EQUAL(5, argc);
    CHECK(argv);
    STRCMP_EQUAL("", argv[0]);
    STRCMP_EQUAL("abc", argv[1]);
    STRCMP_EQUAL("", argv[2]);
    STRCMP_EQUAL("fghi", argv[3]);
    STRCMP_EQUAL("", argv[4]);
    STRCMP_EQUAL(NULL, argv[5]);
    string_free_split (argv);

    /*
     * standard split with comma separator and empty items (keep them),
     * max 2 items
     */
    flags = 0;
    argc = -1;
    argv = string_split (",abc,,fghi,", ",", NULL, flags, 2, &argc);
    LONGS_EQUAL(2, argc);
    CHECK(argv);
    STRCMP_EQUAL("", argv[0]);
    STRCMP_EQUAL("abc", argv[1]);
    STRCMP_EQUAL(NULL, argv[2]);
    string_free_split (argv);

    /*
     * standard split with comma separator and empty items (keep them),
     * max 3 items
     */
    flags = 0;
    argc = -1;
    argv = string_split (",abc,,fghi,", ",", NULL, flags, 3, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("", argv[0]);
    STRCMP_EQUAL("abc", argv[1]);
    STRCMP_EQUAL("", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /*
     * standard split with comma separator and empty items (keep them),
     * max 4 items
     */
    flags = 0;
    argc = -1;
    argv = string_split (",abc,,fghi,", ",", NULL, flags, 4, &argc);
    LONGS_EQUAL(4, argc);
    CHECK(argv);
    STRCMP_EQUAL("", argv[0]);
    STRCMP_EQUAL("abc", argv[1]);
    STRCMP_EQUAL("", argv[2]);
    STRCMP_EQUAL("fghi", argv[3]);
    STRCMP_EQUAL(NULL, argv[4]);
    string_free_split (argv);

    /* standard split with only separators in string */
    flags = 0;
    argc = -1;
    argv = string_split (",,", ",", NULL, flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("", argv[0]);
    STRCMP_EQUAL("", argv[1]);
    STRCMP_EQUAL("", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* standard split with only separators in string and strip separators */
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT;
    argc = -1;
    POINTERS_EQUAL(NULL, string_split (",,", ",", NULL, flags, 0, &argc));
    LONGS_EQUAL(0, argc);
}

/*
 * Tests functions:
 *   string_split_shared
 *   string_free_split_shared
 */

TEST(CoreString, SplitShared)
{
    char **argv;
    int argc, flags;

    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;

    POINTERS_EQUAL(NULL, string_split_shared (NULL, NULL, NULL,
                                              flags, 0, NULL));
    POINTERS_EQUAL(NULL, string_split_shared (NULL, "", NULL,
                                              flags, 0, NULL));
    POINTERS_EQUAL(NULL, string_split_shared ("", NULL, NULL,
                                              flags, 0, NULL));
    POINTERS_EQUAL(NULL, string_split_shared ("", "", NULL,
                                              flags, 0, NULL));

    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argv = string_split_shared (" abc de  abc ", " ", NULL, flags, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("abc", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);

    /* same content == same pointer for shared strings */
    POINTERS_EQUAL(argv[0], argv[2]);

    string_free_split_shared (argv);

    /* free split with NULL */
    string_free_split_shared (NULL);
}

/*
 * Tests functions:
 *   string_split_shell
 *   string_free_split
 */

TEST(CoreString, SplitShell)
{
    char **argv;
    int argc;

    POINTERS_EQUAL(NULL, string_split_shell (NULL, NULL));

    /* test with an empty string */
    argc = -1;
    argv = string_split_shell ("", &argc);
    LONGS_EQUAL(0, argc);
    CHECK(argv);
    STRCMP_EQUAL(NULL, argv[0]);
    string_free_split (argv);

    /* test with a real string (command + arguments) */
    argv = string_split_shell ("/path/to/bin arg1 \"arg2 here\" 'arg3 here'",
                               &argc);
    LONGS_EQUAL(4, argc);
    CHECK(argv);
    STRCMP_EQUAL("/path/to/bin", argv[0]);
    STRCMP_EQUAL("arg1", argv[1]);
    STRCMP_EQUAL("arg2 here", argv[2]);
    STRCMP_EQUAL("arg3 here", argv[3]);
    STRCMP_EQUAL(NULL, argv[4]);
    string_free_split (argv);

    /* test with quote characters inside words: they are stripped */
    argv = string_split_shell ("test\"single\"word", &argc);
    LONGS_EQUAL(1, argc);
    CHECK(argv);
    STRCMP_EQUAL("testsingleword", argv[0]);
    STRCMP_EQUAL(NULL, argv[1]);
    string_free_split (argv);

    /* test with enclosing characters in quotes */
    argv = string_split_shell ("test \"'\"", &argc);
    LONGS_EQUAL(2, argc);
    CHECK(argv);
    STRCMP_EQUAL("test", argv[0]);
    STRCMP_EQUAL("'", argv[1]);
    STRCMP_EQUAL(NULL, argv[2]);
    string_free_split (argv);

    /* test with quoted empty strings */
    argv = string_split_shell ("test '' \"\"", &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("test", argv[0]);
    STRCMP_EQUAL("", argv[1]);
    STRCMP_EQUAL("", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* test with many quotes */
    argv = string_split_shell ("test '''' \"\"\"\"", &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("test", argv[0]);
    STRCMP_EQUAL("", argv[1]);
    STRCMP_EQUAL("", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* test with escaped chars in and outside quotes */
    argv = string_split_shell ("test \\n \"\\n\" '\\n'", &argc);
    LONGS_EQUAL(4, argc);
    CHECK(argv);
    STRCMP_EQUAL("test", argv[0]);
    STRCMP_EQUAL("n", argv[1]);
    STRCMP_EQUAL("\\n", argv[2]);
    STRCMP_EQUAL("\\n", argv[3]);
    STRCMP_EQUAL(NULL, argv[4]);
    string_free_split (argv);

    /* test with escaped quotes */
    argv = string_split_shell (" test \\\"  4  arguments\\\" ", &argc);
    LONGS_EQUAL(4, argc);
    CHECK(argv);
    STRCMP_EQUAL("test", argv[0]);
    STRCMP_EQUAL("\"", argv[1]);
    STRCMP_EQUAL("4", argv[2]);
    STRCMP_EQUAL("arguments\"", argv[3]);
    STRCMP_EQUAL(NULL, argv[4]);
    string_free_split (argv);

    /* free split with NULL */
    string_free_split_shared (NULL);
}

/*
 * Tests functions:
 *   string_split_command
 *   string_free_split_command
 */

TEST(CoreString, SplitCommand)
{
    char **argv;

    /* test with a NULL/empty string */
    POINTERS_EQUAL(NULL, string_split_command (NULL, ';'));
    POINTERS_EQUAL(NULL, string_split_command ("", ';'));

    /* string with one command */
    argv = string_split_command ("abc", ';');
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL(NULL, argv[1]);
    string_free_split_command (argv);

    /* string with 3 commands */
    argv = string_split_command ("abc;de;fghi", ';');
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split_command (argv);

    /* string with 3 commands (containing spaces) */
    argv = string_split_command ("  abc ; de ; fghi  ", ';');
    CHECK(argv);
    STRCMP_EQUAL("abc ", argv[0]);
    STRCMP_EQUAL("de ", argv[1]);
    STRCMP_EQUAL("fghi  ", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split_command (argv);

    /* separator other than ';' */
    argv = string_split_command ("abc,de,fghi", ',');
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    STRCMP_EQUAL(NULL, argv[3]);
    string_free_split_command (argv);

    /* free split with NULL */
    string_free_split_command (NULL);
}

/*
 * Tests functions:
 *   string_split_tags
 *   string_free_split_tags
 */

TEST(CoreString, SplitTags)
{
    char ***tags;
    int num_tags;

    /* test with a NULL/empty string */
    POINTERS_EQUAL(NULL, string_split_tags (NULL, NULL));
    POINTERS_EQUAL(NULL, string_split_tags ("", NULL));
    num_tags = -1;
    POINTERS_EQUAL(NULL, string_split_tags (NULL, &num_tags));
    LONGS_EQUAL(0, num_tags);
    num_tags = -1;
    POINTERS_EQUAL(NULL, string_split_tags ("", &num_tags));
    LONGS_EQUAL(0, num_tags);

    /* string with one tag */
    num_tags = -1;
    tags = string_split_tags ("irc_join", &num_tags);
    CHECK(tags);
    LONGS_EQUAL(1, num_tags);
    STRCMP_EQUAL("irc_join", tags[0][0]);
    STRCMP_EQUAL(NULL, tags[0][1]);
    string_free_split_tags (tags);

    /* string with OR on 2 tags */
    num_tags = -1;
    tags = string_split_tags ("irc_join,irc_quit", &num_tags);
    CHECK(tags);
    LONGS_EQUAL(2, num_tags);
    STRCMP_EQUAL("irc_join", tags[0][0]);
    STRCMP_EQUAL(NULL, tags[0][1]);
    STRCMP_EQUAL("irc_quit", tags[1][0]);
    STRCMP_EQUAL(NULL, tags[1][1]);
    string_free_split_tags (tags);

    /*
     * string with OR on:
     * - 1 tag
     * - AND on 2 tags
     */
    num_tags = -1;
    tags = string_split_tags ("irc_join,irc_quit+nick_test", &num_tags);
    CHECK(tags);
    LONGS_EQUAL(2, num_tags);
    STRCMP_EQUAL("irc_join", tags[0][0]);
    STRCMP_EQUAL(NULL, tags[0][1]);
    STRCMP_EQUAL("irc_quit", tags[1][0]);
    STRCMP_EQUAL("nick_test", tags[1][1]);
    STRCMP_EQUAL(NULL, tags[1][2]);
    string_free_split_tags (tags);

    /* free split with NULL */
    string_free_split_tags (NULL);
}

/*
 * Tests functions:
 *   string_rebuild_split_string
 */

TEST(CoreString, RebuildSplitString)
{
    char **argv, *str;
    int argc, flags;

    str = string_rebuild_split_string (NULL, NULL, 0, -1);
    STRCMP_EQUAL(NULL, str);

    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argv = string_split (" abc de  fghi ", " ", NULL, flags, 0, &argc);
    /* => ["abc", "de", "fghi"] */

    /* invalid index_end, which is < index_start */
    str = string_rebuild_split_string ((const char **)argv, NULL, 1, 0);
    STRCMP_EQUAL(NULL, str);
    str = string_rebuild_split_string ((const char **)argv, NULL, 2, 1);
    STRCMP_EQUAL(NULL, str);

    str = string_rebuild_split_string ((const char **)argv, NULL, 0, -1);
    STRCMP_EQUAL("abcdefghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, NULL, 0, 0);
    STRCMP_EQUAL("abc", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, NULL, 0, 1);
    STRCMP_EQUAL("abcde", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, NULL, 0, 2);
    STRCMP_EQUAL("abcdefghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, NULL, 0, 3);
    STRCMP_EQUAL("abcdefghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, NULL, 1, 1);
    STRCMP_EQUAL("de", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, NULL, 1, 2);
    STRCMP_EQUAL("defghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, NULL, 1, 3);
    STRCMP_EQUAL("defghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, NULL, 2, 2);
    STRCMP_EQUAL("fghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, NULL, 2, 3);
    STRCMP_EQUAL("fghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, "", 0, -1);
    STRCMP_EQUAL("abcdefghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, ";;", 0, -1);
    STRCMP_EQUAL("abc;;de;;fghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, ";;", 0, 0);
    STRCMP_EQUAL("abc", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, ";;", 0, 1);
    STRCMP_EQUAL("abc;;de", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, ";;", 0, 2);
    STRCMP_EQUAL("abc;;de;;fghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, ";;", 0, 3);
    STRCMP_EQUAL("abc;;de;;fghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, ";;", 1, 1);
    STRCMP_EQUAL("de", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, ";;", 1, 2);
    STRCMP_EQUAL("de;;fghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, ";;", 1, 3);
    STRCMP_EQUAL("de;;fghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, ";;", 2, 2);
    STRCMP_EQUAL("fghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, ";;", 2, 3);
    STRCMP_EQUAL("fghi", str);
    free (str);

    string_free_split (argv);

    /* test with empty items */
    argv = string_split (",abc,de,,fghi,", ",", NULL, 0, 0, &argc);
    /* => ["", "abc", "de", "", "fghi", ""] */

    str = string_rebuild_split_string ((const char **)argv, "/", 0, -1);
    STRCMP_EQUAL("/abc/de//fghi/", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, "/", 0, 0);
    STRCMP_EQUAL("", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, "/", 0, 1);
    STRCMP_EQUAL("/abc", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, "/", 0, 2);
    STRCMP_EQUAL("/abc/de", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, "/", 0, 3);
    STRCMP_EQUAL("/abc/de/", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, "/", 0, 4);
    STRCMP_EQUAL("/abc/de//fghi", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, "/", 0, 5);
    STRCMP_EQUAL("/abc/de//fghi/", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, "/", 0, 6);
    STRCMP_EQUAL("/abc/de//fghi/", str);
    free (str);

    str = string_rebuild_split_string ((const char **)argv, "/", 2, 4);
    STRCMP_EQUAL("de//fghi", str);
    free (str);

    string_free_split (argv);
}

/*
 * Tests functions:
 *   string_iconv
 *   string_iconv_to_internal
 *   string_iconv_from_internal
 *   string_fprintf
 */

TEST(CoreString, Iconv)
{
    const char *noel_utf8 = "no\xc3\xabl";  /* noël */
    const char *noel_iso = "no\xebl";
    char *str;
    FILE *f;

    /* string_iconv */
    WEE_TEST_STR(NULL, string_iconv (0, NULL, NULL, NULL));
    WEE_TEST_STR("", string_iconv (0, NULL, NULL, ""));
    WEE_TEST_STR("abc", string_iconv (0, NULL, NULL, "abc"));
    WEE_TEST_STR("abc", string_iconv (1, "UTF-8", "ISO-8859-15", "abc"));
    WEE_TEST_STR(noel_iso, string_iconv (1, "UTF-8", "ISO-8859-15", noel_utf8));
    WEE_TEST_STR(noel_utf8, string_iconv (0, "ISO-8859-15", "UTF-8", noel_iso));

    /* string_iconv_to_internal */
    WEE_TEST_STR(NULL, string_iconv_to_internal (NULL, NULL));
    WEE_TEST_STR("", string_iconv_to_internal (NULL, ""));
    WEE_TEST_STR("abc", string_iconv_to_internal (NULL, "abc"));
    WEE_TEST_STR(noel_utf8, string_iconv_to_internal ("ISO-8859-15", noel_iso));

    /* string_iconv_from_internal */
    WEE_TEST_STR(NULL, string_iconv_from_internal (NULL, NULL));
    WEE_TEST_STR("", string_iconv_from_internal (NULL, ""));
    WEE_TEST_STR("abc", string_iconv_from_internal (NULL, "abc"));
    WEE_TEST_STR(noel_iso, string_iconv_from_internal ("ISO-8859-15", noel_utf8));

    /* string_fprintf */
    f = fopen ("/dev/null", "w");
    LONGS_EQUAL(0, string_fprintf (f, NULL));
    LONGS_EQUAL(1, string_fprintf (f, "abc"));
    LONGS_EQUAL(1, string_fprintf (f, noel_utf8));
    LONGS_EQUAL(1, string_fprintf (f, noel_iso));
    fclose (f);
}

/*
 * Tests functions:
 *   string_format_size
 */

TEST(CoreString, FormatSize)
{
    char *str;

    WEE_FORMAT_SIZE("0 bytes", 0);
    WEE_FORMAT_SIZE("1 byte", 1);
    WEE_FORMAT_SIZE("2 bytes", 2);
    WEE_FORMAT_SIZE("42 bytes", 42);
    WEE_FORMAT_SIZE("999 bytes", ONE_KB - 1);
    WEE_FORMAT_SIZE("1000 bytes", ONE_KB);
    WEE_FORMAT_SIZE("9999 bytes", (10 * ONE_KB) - 1);

    WEE_FORMAT_SIZE("10.0 KB", 10 * ONE_KB);
    WEE_FORMAT_SIZE("10.1 KB", (10 * ONE_KB) + (ONE_KB / 10));
    WEE_FORMAT_SIZE("42.0 KB", 42 * ONE_KB);
    WEE_FORMAT_SIZE("1000.0 KB", ONE_MB - 1);

    WEE_FORMAT_SIZE("1.00 MB", ONE_MB);
    WEE_FORMAT_SIZE("1.10 MB", ONE_MB + (ONE_MB / 10));
    WEE_FORMAT_SIZE("42.00 MB", 42 * ONE_MB);
    WEE_FORMAT_SIZE("1000.00 MB", ONE_GB - 1);

    WEE_FORMAT_SIZE("1.00 GB", ONE_GB);
    WEE_FORMAT_SIZE("1.10 GB", ONE_GB + (ONE_GB / 10));
    WEE_FORMAT_SIZE("42.00 GB", 42 * ONE_GB);
    WEE_FORMAT_SIZE("1000.00 GB", ONE_TB - 1);

    WEE_FORMAT_SIZE("1.00 TB", ONE_TB);
    WEE_FORMAT_SIZE("1.10 TB", ONE_TB + (ONE_TB / 10));
    WEE_FORMAT_SIZE("42.00 TB", 42 * ONE_TB);
}

/*
 * Tests functions:
 *   string_parse_size
 */

TEST(CoreString, ParseSize)
{
    CHECK(string_parse_size (NULL) == 0ULL);

    CHECK(string_parse_size ("") == 0ULL);
    CHECK(string_parse_size ("*") == 0ULL);
    CHECK(string_parse_size ("b") == 0ULL);
    CHECK(string_parse_size ("k") == 0ULL);
    CHECK(string_parse_size ("m") == 0ULL);
    CHECK(string_parse_size ("g") == 0ULL);
    CHECK(string_parse_size ("t") == 0ULL);
    CHECK(string_parse_size ("z") == 0ULL);
    CHECK(string_parse_size ("0z") == 0ULL);

    CHECK(string_parse_size ("0") == 0ULL);
    CHECK(string_parse_size ("0b") == 0ULL);
    CHECK(string_parse_size ("0B") == 0ULL);

    CHECK(string_parse_size ("1") == 1ULL);
    CHECK(string_parse_size ("1b") == 1ULL);
    CHECK(string_parse_size ("1B") == 1ULL);
    CHECK(string_parse_size ("1 b") == 1ULL);
    CHECK(string_parse_size ("1 B") == 1ULL);

    CHECK(string_parse_size ("2") == 2ULL);
    CHECK(string_parse_size ("2b") == 2ULL);
    CHECK(string_parse_size ("2B") == 2ULL);

    CHECK(string_parse_size ("42") == 42ULL);
    CHECK(string_parse_size ("42b") == 42ULL);
    CHECK(string_parse_size ("42B") == 42ULL);

    CHECK(string_parse_size ("999") == 999ULL);
    CHECK(string_parse_size ("999b") == 999ULL);
    CHECK(string_parse_size ("999B") == 999ULL);

    CHECK(string_parse_size ("1200") == 1200ULL);
    CHECK(string_parse_size ("1200b") == 1200ULL);
    CHECK(string_parse_size ("1200B") == 1200ULL);

    CHECK(string_parse_size ("1k") == 1000ULL);
    CHECK(string_parse_size ("1K") == 1000ULL);

    CHECK(string_parse_size ("12k") == 12000ULL);
    CHECK(string_parse_size ("12K") == 12000ULL);

    CHECK(string_parse_size ("1m") == 1000000ULL);
    CHECK(string_parse_size ("1M") == 1000000ULL);

    CHECK(string_parse_size ("30m") == 30000000ULL);
    CHECK(string_parse_size ("30M") == 30000000ULL);

    CHECK(string_parse_size ("1g") == 1000000000ULL);
    CHECK(string_parse_size ("1G") == 1000000000ULL);

    CHECK(string_parse_size ("1234m") == 1234000000ULL);
    CHECK(string_parse_size ("1234m") == 1234000000ULL);

    CHECK(string_parse_size ("15g") == 15000000000ULL);
    CHECK(string_parse_size ("15G") == 15000000000ULL);

    CHECK(string_parse_size ("8t") == 8000000000000ULL);
    CHECK(string_parse_size ("8T") == 8000000000000ULL);
}

/*
 * Tests functions:
 *   string_base16_encode
 *   string_base16_decode
 */

TEST(CoreString, Base16)
{
    int i, length;
    char str[1024];
    const char *str_base16[][2] =
        { { "", "" },
          { "abcdefgh", "6162636465666768" },
          { "this is a *test*", "746869732069732061202A746573742A" },
          { "this is a *test*\xAA", "746869732069732061202A746573742AAA" },
          { NULL, NULL } };


    /* string_base16_encode */
    LONGS_EQUAL(-1, string_base16_encode (NULL, 0, NULL));
    LONGS_EQUAL(-1, string_base16_encode (NULL, 0, str));
    LONGS_EQUAL(-1, string_base16_encode ("", 0, NULL));
    str[0] = 0xAA;
    LONGS_EQUAL(0, string_base16_encode ("", -1, str));
    BYTES_EQUAL(0x0, str[0]);
    str[0] = 0xAA;
    LONGS_EQUAL(0, string_base16_encode ("", 0, str));
    BYTES_EQUAL(0x0, str[0]);
    for (i = 0; str_base16[i][0]; i++)
    {
        length = strlen (str_base16[i][1]);
        LONGS_EQUAL(length, string_base16_encode (str_base16[i][0],
                                                  strlen (str_base16[i][0]),
                                                  str));
        STRCMP_EQUAL(str_base16[i][1], str);
    }

    /* string_base16_decode */
    LONGS_EQUAL(-1, string_base16_decode (NULL, NULL));
    LONGS_EQUAL(-1, string_base16_decode (NULL, str));
    LONGS_EQUAL(-1, string_base16_decode ("", NULL));
    LONGS_EQUAL(0, string_base16_decode ("", str));
    for (i = 0; str_base16[i][0]; i++)
    {
        length = strlen (str_base16[i][0]);
        LONGS_EQUAL(length, string_base16_decode (str_base16[i][1], str));
        STRCMP_EQUAL(str_base16[i][0], str);
    }
}

/*
 * Tests functions:
 *   string_base32_encode
 *   string_base32_decode
 */

TEST(CoreString, Base32)
{
    int i, length;
    char str[1024];
    const char *str_base32[][2] =
        { { "", "" },
          { "A", "IE======" },
          { "B", "II======" },
          { "C", "IM======" },
          { "D", "IQ======" },
          { "abcdefgh", "MFRGGZDFMZTWQ===" },
          { "This is a test.", "KRUGS4ZANFZSAYJAORSXG5BO" },
          { "This is a test..", "KRUGS4ZANFZSAYJAORSXG5BOFY======" },
          { "This is a test...", "KRUGS4ZANFZSAYJAORSXG5BOFYXA====" },
          { "This is a test....", "KRUGS4ZANFZSAYJAORSXG5BOFYXC4===" },
          { "This is a long long long sentence here...",
            "KRUGS4ZANFZSAYJANRXW4ZZANRXW4ZZANRXW4ZZAONSW45DFNZRWKIDIMVZGKLRO"
            "FY======" },
          { NULL, NULL } };

    /* string_base32_encode */
    LONGS_EQUAL(-1, string_base32_encode (NULL, 0, NULL));
    LONGS_EQUAL(-1, string_base32_encode (NULL, 0, str));
    LONGS_EQUAL(-1, string_base32_encode ("", 0, NULL));
    str[0] = 0xAA;
    LONGS_EQUAL(0, string_base32_encode ("", -1, str));
    BYTES_EQUAL(0x0, str[0]);
    str[0] = 0xAA;
    LONGS_EQUAL(0, string_base32_encode ("", 0, str));
    BYTES_EQUAL(0x0, str[0]);
    for (i = 0; str_base32[i][0]; i++)
    {
        length = strlen (str_base32[i][1]);
        LONGS_EQUAL(length, string_base32_encode (str_base32[i][0],
                                                  strlen (str_base32[i][0]),
                                                  str));
        STRCMP_EQUAL(str_base32[i][1], str);
    }

    /* string_base32_decode */
    LONGS_EQUAL(-1, string_base32_decode (NULL, NULL));
    LONGS_EQUAL(-1, string_base32_decode (NULL, str));
    LONGS_EQUAL(-1, string_base32_decode ("", NULL));
    LONGS_EQUAL(0, string_base32_decode ("", str));
    for (i = 0; str_base32[i][0]; i++)
    {
        length = strlen (str_base32[i][0]);
        LONGS_EQUAL(length, string_base32_decode (str_base32[i][1], str));
        STRCMP_EQUAL(str_base32[i][0], str);
    }
}

/*
 * Tests functions:
 *   string_base64_encode
 *   string_base64_decode
 */

TEST(CoreString, Base64)
{
    int i, length;
    char str[1024];
    const char *str_base64[][3] =
        { { "", "", "" },
          { "A", "QQ==", "QQ" },
          { "B", "Qg==", "Qg" },
          { "C", "Qw==", "Qw" },
          { "D", "RA==", "RA" },
          { "abc", "YWJj", "YWJj" },
          { "<<?!!>>", "PDw/ISE+Pg==", "PDw_ISE-Pg" },
          { "This is a test.",
            "VGhpcyBpcyBhIHRlc3Qu", "VGhpcyBpcyBhIHRlc3Qu" },
          { "This is a test..",
            "VGhpcyBpcyBhIHRlc3QuLg==", "VGhpcyBpcyBhIHRlc3QuLg" },
          { "This is a test...",
            "VGhpcyBpcyBhIHRlc3QuLi4=", "VGhpcyBpcyBhIHRlc3QuLi4" },
          { "This is a test....",
            "VGhpcyBpcyBhIHRlc3QuLi4u", "VGhpcyBpcyBhIHRlc3QuLi4u" },
          { "This is a long long long sentence here...",
            "VGhpcyBpcyBhIGxvbmcgbG9uZyBsb25nIHNlbnRlbmNlIGhlcmUuLi4=",
            "VGhpcyBpcyBhIGxvbmcgbG9uZyBsb25nIHNlbnRlbmNlIGhlcmUuLi4" },
          { "Another example for base64",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQ=",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQ" },
          { "Another example for base64.",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQu",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQu" },
          { "Another example for base64..",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQuLg==",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQuLg" },
          { "Another example for base64...",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQuLi4=",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQuLi4" },
          { NULL, NULL } };

    /* string_base64_encode */
    LONGS_EQUAL(-1, string_base64_encode (0, NULL, 0, NULL));
    LONGS_EQUAL(-1, string_base64_encode (0, NULL, 0, str));
    LONGS_EQUAL(-1, string_base64_encode (0, "", 0, NULL));
    str[0] = 0xAA;
    LONGS_EQUAL(0, string_base64_encode (0, "", -1, str));
    BYTES_EQUAL(0x0, str[0]);
    str[0] = 0xAA;
    LONGS_EQUAL(0, string_base64_encode (0, "", 0, str));
    BYTES_EQUAL(0x0, str[0]);
    for (i = 0; str_base64[i][0]; i++)
    {
        length = strlen (str_base64[i][1]);
        LONGS_EQUAL(length, string_base64_encode (0,
                                                  str_base64[i][0],
                                                  strlen (str_base64[i][0]),
                                                  str));
        STRCMP_EQUAL(str_base64[i][1], str);
        length = strlen (str_base64[i][2]);
        LONGS_EQUAL(length, string_base64_encode (1,
                                                  str_base64[i][0],
                                                  strlen (str_base64[i][0]),
                                                  str));
        STRCMP_EQUAL(str_base64[i][2], str);
    }
    /* test with a \0 in string */
    LONGS_EQUAL(20, string_base64_encode (0, "This is\0a test.", 15, str));
    STRCMP_EQUAL("VGhpcyBpcwBhIHRlc3Qu", str);

    /* string_base64_decode */
    LONGS_EQUAL(-1, string_base64_decode (0, NULL, NULL));
    LONGS_EQUAL(-1, string_base64_decode (0, NULL, str));
    LONGS_EQUAL(-1, string_base64_decode (0, "", NULL));
    LONGS_EQUAL(0, string_base64_decode (0, "", str));
    for (i = 0; str_base64[i][0]; i++)
    {
        length = strlen (str_base64[i][0]);
        LONGS_EQUAL(length, string_base64_decode (0, str_base64[i][1], str));
        STRCMP_EQUAL(str_base64[i][0], str);
        LONGS_EQUAL(length, string_base64_decode (1, str_base64[i][2], str));
        STRCMP_EQUAL(str_base64[i][0], str);
    }
    /* test with a \0 in string */
    LONGS_EQUAL(15, string_base64_decode (0, "VGhpcyBpcwBhIHRlc3Qu", str));
    MEMCMP_EQUAL("This is\0a test.", str, 15);

    /* invalid base64 string, missing two "=" at the end */
    LONGS_EQUAL(4, string_base64_decode (0, "dGVzdA", str));
    STRCMP_EQUAL("test", str);
}

/*
 * Tests functions:
 *   string_base_encode
 */

TEST(CoreString, BaseEncode)
{
    char str[1024];

    LONGS_EQUAL(-1, string_base_encode ("0", NULL, 0, NULL));
    LONGS_EQUAL(-1, string_base_encode ("0", "", 0, str));
    LONGS_EQUAL(-1, string_base_encode ("16", NULL, 0, str));
    LONGS_EQUAL(-1, string_base_encode ("32", NULL, 0, str));
    LONGS_EQUAL(-1, string_base_encode ("64", NULL, 0, str));

    str[0] = 0xAA;
    LONGS_EQUAL(16, string_base_encode ("16", "abcdefgh", 8, str));
    STRCMP_EQUAL("6162636465666768", str);

    str[0] = 0xAA;
    LONGS_EQUAL(16, string_base_encode ("32", "abcdefgh", 8, str));
    STRCMP_EQUAL("MFRGGZDFMZTWQ===", str);

    str[0] = 0xAA;
    LONGS_EQUAL(20, string_base_encode ("64", "This is a test.", 15, str));
    STRCMP_EQUAL("VGhpcyBpcyBhIHRlc3Qu", str);

    str[0] = 0xAA;
    LONGS_EQUAL(12, string_base_encode ("64", "<<" "???" ">>", 7, str));
    STRCMP_EQUAL("PDw/Pz8+Pg==", str);

    str[0] = 0xAA;
    LONGS_EQUAL(10, string_base_encode ("64url", "<<" "???" ">>", 7, str));
    STRCMP_EQUAL("PDw_Pz8-Pg", str);
}

/*
 * Tests functions:
 *   string_base_decode
 */

TEST(CoreString, BaseDecode)
{
    char str[1024];

    LONGS_EQUAL(-1, string_base_decode ("0", NULL, NULL));
    LONGS_EQUAL(-1, string_base_decode ("0", "", str));
    LONGS_EQUAL(-1, string_base_decode ("16", NULL, str));
    LONGS_EQUAL(-1, string_base_decode ("32", NULL, str));
    LONGS_EQUAL(-1, string_base_decode ("64", NULL, str));

    str[0] = 0xAA;
    LONGS_EQUAL(8, string_base_decode ("16", "6162636465666768", str));
    STRCMP_EQUAL("abcdefgh", str);

    str[0] = 0xAA;
    LONGS_EQUAL(8, string_base_decode ("32", "MFRGGZDFMZTWQ===", str));
    STRCMP_EQUAL("abcdefgh", str);

    str[0] = 0xAA;
    LONGS_EQUAL(15, string_base_decode ("64", "VGhpcyBpcyBhIHRlc3Qu", str));
    STRCMP_EQUAL("This is a test.", str);

    str[0] = 0xAA;
    LONGS_EQUAL(7, string_base_decode ("64", "PDw/Pz8+Pg==", str));
    STRCMP_EQUAL("<<" "???" ">>", str);

    str[0] = 0xAA;
    LONGS_EQUAL(7, string_base_decode ("64url", "PDw_Pz8-Pg", str));
    STRCMP_EQUAL("<<" "???" ">>", str);
}

/*
 * Tests functions:
 *   string_hex_dump
 */

TEST(CoreString, Hex_dump)
{
    const char *noel_utf8 = "no\xc3\xabl";  /* noël */
    const char *noel_iso = "no\xebl";
    char *str;

    STRCMP_EQUAL(NULL, string_hex_dump (NULL, 0, 0, NULL, NULL));
    STRCMP_EQUAL(NULL, string_hex_dump ("abc", 0, 0, NULL, NULL));
    STRCMP_EQUAL(NULL, string_hex_dump ("abc", 3, 0, NULL, NULL));
    STRCMP_EQUAL(NULL, string_hex_dump ("abc", 0, 5, NULL, NULL));

    WEE_HEX_DUMP("61 62 63   a b c ", "abc", 3, 3, NULL, NULL);
    WEE_HEX_DUMP("61 62 63   a b c ", "abc", 3, 3, "", "");
    WEE_HEX_DUMP("(( 61 62 63   a b c ", "abc", 3, 3, "(( ", NULL);
    WEE_HEX_DUMP("61 62 63   a b c  ))", "abc", 3, 3, NULL, " ))");
    WEE_HEX_DUMP("(( 61 62 63   a b c  ))", "abc", 3, 3, "(( ", " ))");
    WEE_HEX_DUMP("61 62 63         a b c     ", "abc", 3, 5, NULL, NULL);
    WEE_HEX_DUMP("61 62 63                        a b c               ",
                 "abc", 3, 10, NULL, NULL);
    WEE_HEX_DUMP("61 62   a b \n"
                 "63      c   ",
                 "abc", 3, 2, NULL, NULL);
    WEE_HEX_DUMP("6E 6F C3 AB 6C   n o . . l ",
                 noel_utf8, strlen (noel_utf8), 5, NULL, NULL);
    WEE_HEX_DUMP("6E 6F   n o \n"
                 "C3 AB   . . \n"
                 "6C      l   ",
                 noel_utf8, strlen (noel_utf8), 2, NULL, NULL);
    WEE_HEX_DUMP("( 6E 6F   n o \n"
                 "( C3 AB   . . \n"
                 "( 6C      l   ",
                 noel_utf8, strlen (noel_utf8), 2, "( ", NULL);
    WEE_HEX_DUMP("( 6E 6F   n o  )\n"
                 "( C3 AB   . .  )\n"
                 "( 6C      l    )",
                 noel_utf8, strlen (noel_utf8), 2, "( ", " )");
    WEE_HEX_DUMP("6E 6F EB 6C      n o . l   ",
                 noel_iso, strlen (noel_iso), 5, NULL, NULL);
    WEE_HEX_DUMP("6E 6F   n o \n"
                 "EB 6C   . l ",
                 noel_iso, strlen (noel_iso), 2, NULL, NULL);
}

/*
 * Tests functions:
 *   string_is_command_char
 */

TEST(CoreString, IsCommandChar)
{
    LONGS_EQUAL(0, string_is_command_char (NULL));
    LONGS_EQUAL(0, string_is_command_char (""));
    LONGS_EQUAL(0, string_is_command_char ("abc"));
    LONGS_EQUAL(1, string_is_command_char ("/"));
    LONGS_EQUAL(1, string_is_command_char ("/abc"));
    LONGS_EQUAL(1, string_is_command_char ("//abc"));

    /* test with custom command chars */
    config_file_option_set (config_look_command_chars, "öï", 1);

    LONGS_EQUAL(0, string_is_command_char ("abc"));
    LONGS_EQUAL(0, string_is_command_char ("o_abc"));
    LONGS_EQUAL(0, string_is_command_char ("i_abc"));
    LONGS_EQUAL(0, string_is_command_char ("é_abc"));
    LONGS_EQUAL(1, string_is_command_char ("ö"));
    LONGS_EQUAL(1, string_is_command_char ("ö_abc"));
    LONGS_EQUAL(1, string_is_command_char ("ö_öabc"));
    LONGS_EQUAL(1, string_is_command_char ("ï"));
    LONGS_EQUAL(1, string_is_command_char ("ï_abc"));
    LONGS_EQUAL(1, string_is_command_char ("ï_öabc"));
    LONGS_EQUAL(1, string_is_command_char ("/abc"));

    config_file_option_reset (config_look_command_chars, 1);
}

/*
 * Tests functions:
 *   string_input_for_buffer
 */

TEST(CoreString, InputForBuffer)
{
    char *str;

    STRCMP_EQUAL(NULL, string_input_for_buffer (NULL));
    STRCMP_EQUAL(NULL, string_input_for_buffer ("/"));
    STRCMP_EQUAL(NULL, string_input_for_buffer ("/abc"));

    /* not commands */
    str = strdup ("");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("/ ");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("/ abc");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("/ /");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("/*");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("abc");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("//abc");
    STRCMP_EQUAL(str + 1, string_input_for_buffer (str));
    free (str);
    str = strdup ("/abc/def /ghi");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("/abc/def /ghi");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);

    /* commands */
    STRCMP_EQUAL(NULL, string_input_for_buffer (NULL));
    str = strdup ("/");
    STRCMP_EQUAL(NULL, string_input_for_buffer (str));
    free (str);
    str = strdup ("/abc");
    STRCMP_EQUAL(NULL, string_input_for_buffer (str));
    free (str);
    str = strdup ("/abc /def");
    STRCMP_EQUAL(NULL, string_input_for_buffer (str));
    free (str);
    str = strdup ("/abc\n/def");
    STRCMP_EQUAL(NULL, string_input_for_buffer (str));
    free (str);

    /* test with custom command chars */
    config_file_option_set (config_look_command_chars, "öï", 1);

    str = strdup ("o_abc");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("ö_abc");
    STRCMP_EQUAL(NULL, string_input_for_buffer (str));
    free (str);
    str = strdup ("ö abc");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("öö_abc");
    STRCMP_EQUAL(str + 2, string_input_for_buffer (str));
    free (str);
    str = strdup ("ï_abc");
    STRCMP_EQUAL(NULL, string_input_for_buffer (str));
    free (str);
    str = strdup ("ï abc");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("ïï_abc");
    STRCMP_EQUAL(str + 2, string_input_for_buffer (str));
    free (str);

    config_file_option_reset (config_look_command_chars, 1);
}

/*
 * Tests functions:
 *   string_get_common_bytes_count
 */

TEST(CoreString, GetCommonBytesCount)
{
    LONGS_EQUAL(0, string_get_common_bytes_count (NULL, NULL));
    LONGS_EQUAL(0, string_get_common_bytes_count ("", NULL));
    LONGS_EQUAL(0, string_get_common_bytes_count (NULL, ""));
    LONGS_EQUAL(0, string_get_common_bytes_count ("", ""));

    LONGS_EQUAL(1, string_get_common_bytes_count ("a", "a"));
    LONGS_EQUAL(0, string_get_common_bytes_count ("a", "b"));

    LONGS_EQUAL(3, string_get_common_bytes_count ("abc", "abc"));

    LONGS_EQUAL(3, string_get_common_bytes_count ("abcdef", "fac"));

    LONGS_EQUAL(4, string_get_common_bytes_count ("noël", "noïl"));
}

/*
 * Tests functions:
 *   string_levenshtein
 */

TEST(CoreString, Levenshtein)
{
    LONGS_EQUAL(0, string_levenshtein (NULL, NULL, 1));
    LONGS_EQUAL(0, string_levenshtein ("", "", 1));
    LONGS_EQUAL(3, string_levenshtein (NULL, "abc", 1));
    LONGS_EQUAL(3, string_levenshtein ("abc", NULL, 1));
    LONGS_EQUAL(3, string_levenshtein ("", "abc", 1));
    LONGS_EQUAL(3, string_levenshtein ("abc", "", 1));

    LONGS_EQUAL(0, string_levenshtein ("abc", "abc", 1));
    LONGS_EQUAL(1, string_levenshtein ("abc", "ab", 1));
    LONGS_EQUAL(1, string_levenshtein ("ab", "abc", 1));
    LONGS_EQUAL(2, string_levenshtein ("abc", "a", 1));
    LONGS_EQUAL(2, string_levenshtein ("a", "abc", 1));
    LONGS_EQUAL(3, string_levenshtein ("abc", "", 1));
    LONGS_EQUAL(3, string_levenshtein ("", "abc", 1));

    LONGS_EQUAL(3, string_levenshtein ("abc", "ABC", 1));
    LONGS_EQUAL(3, string_levenshtein ("abc", "AB", 1));
    LONGS_EQUAL(3, string_levenshtein ("ab", "ABC", 1));
    LONGS_EQUAL(3, string_levenshtein ("abc", "A", 1));
    LONGS_EQUAL(3, string_levenshtein ("a", "ABC", 1));
    LONGS_EQUAL(3, string_levenshtein ("abc", "", 1));
    LONGS_EQUAL(3, string_levenshtein ("", "ABC", 1));

    LONGS_EQUAL(0, string_levenshtein ("abc", "ABC", 0));
    LONGS_EQUAL(1, string_levenshtein ("abc", "AB", 0));
    LONGS_EQUAL(1, string_levenshtein ("ab", "ABC", 0));
    LONGS_EQUAL(2, string_levenshtein ("abc", "A", 0));
    LONGS_EQUAL(2, string_levenshtein ("a", "ABC", 0));
    LONGS_EQUAL(3, string_levenshtein ("abc", "", 0));
    LONGS_EQUAL(3, string_levenshtein ("", "ABC", 0));

    LONGS_EQUAL(2, string_levenshtein ("response", "respond", 1));
    LONGS_EQUAL(4, string_levenshtein ("response", "resist", 1));

    LONGS_EQUAL(2, string_levenshtein ("response", "responsive", 1));

    /* with UTF-8 chars */
    LONGS_EQUAL(1, string_levenshtein ("é", "É", 1));
    LONGS_EQUAL(0, string_levenshtein ("é", "É", 0));
    LONGS_EQUAL(1, string_levenshtein ("é", "à", 1));
    LONGS_EQUAL(1, string_levenshtein ("é", "à", 0));
    LONGS_EQUAL(1, string_levenshtein ("té", "to", 1));
    LONGS_EQUAL(1, string_levenshtein ("noël", "noel", 1));
    LONGS_EQUAL(2, string_levenshtein ("bôô", "boo", 1));
    LONGS_EQUAL(2, string_levenshtein ("界世", "こん", 1));
}

/*
 * Tests functions:
 *   string_get_priority_and_name
 */

TEST(CoreString, GetPriorityAndName)
{
    const char *empty = "";
    const char *delimiter = "|";
    const char *name = "test";
    const char *name_prio_empty = "|test";
    const char *name_prio = "1234|test";
    const char *ptr_name;
    int priority;

    string_get_priority_and_name (NULL, NULL, NULL, 0);
    string_get_priority_and_name ("test", NULL, NULL, 0);

    /* NULL => (default_priority, NULL) */
    priority = -1;
    ptr_name = NULL;
    string_get_priority_and_name (NULL, &priority, &ptr_name, 500);
    LONGS_EQUAL(500, priority);
    STRCMP_EQUAL(NULL, ptr_name);

    /* "" => (default_priority, "") */
    priority = -1;
    ptr_name = NULL;
    string_get_priority_and_name (empty, &priority, &ptr_name, 500);
    LONGS_EQUAL(500, priority);
    STRCMP_EQUAL("", ptr_name);

    /* "|" => (0, "") */
    priority = -1;
    ptr_name = NULL;
    string_get_priority_and_name (delimiter, &priority, &ptr_name, 500);
    LONGS_EQUAL(0, priority);
    STRCMP_EQUAL("", ptr_name);

    /* "test" => (default_priority, "test") */
    priority = -1;
    ptr_name = NULL;
    string_get_priority_and_name (name, &priority, &ptr_name, 500);
    LONGS_EQUAL(500, priority);
    STRCMP_EQUAL("test", ptr_name);

    /* "|test" => (0, "test") */
    priority = -1;
    ptr_name = NULL;
    string_get_priority_and_name (name_prio_empty, &priority, &ptr_name, 500);
    LONGS_EQUAL(0, priority);
    STRCMP_EQUAL("test", ptr_name);

    /* "1234|test" => (1234, "test") */
    priority = -1;
    ptr_name = NULL;
    string_get_priority_and_name (name_prio, &priority, &ptr_name, 500);
    LONGS_EQUAL(1234, priority);
    STRCMP_EQUAL("test", ptr_name);
}

/*
 * Tests functions:
 *   string_shared_get
 *   string_shared_free
 */

TEST(CoreString, Shared)
{
    const char *str1, *str2, *str3;
    int count;

    count = (string_hashtable_shared) ?
        string_hashtable_shared->items_count : 0;

    STRCMP_EQUAL(NULL, string_shared_get (NULL));

    str1 = string_shared_get ("this is a test");
    CHECK(str1);

    LONGS_EQUAL(count + 1, string_hashtable_shared->items_count);

    str2 = string_shared_get ("this is a test");
    CHECK(str2);
    POINTERS_EQUAL(str1, str2);

    LONGS_EQUAL(count + 1, string_hashtable_shared->items_count);

    str3 = string_shared_get ("this is another test");
    CHECK(str3);
    CHECK(str1 != str3);
    CHECK(str2 != str3);

    LONGS_EQUAL(count + 2, string_hashtable_shared->items_count);

    string_shared_free (str1);
    LONGS_EQUAL(count + 2, string_hashtable_shared->items_count);

    string_shared_free (str2);
    LONGS_EQUAL(count + 1, string_hashtable_shared->items_count);

    string_shared_free (str3);
    LONGS_EQUAL(count + 0, string_hashtable_shared->items_count);

    /* test free of NULL */
    string_shared_free (NULL);
}

/*
 * Tests functions:
 *   string_dyn_alloc
 *   string_dyn_copy
 *   string_dyn_concat
 *   string_dyn_free
 */

TEST(CoreString, Dyn)
{
    char **str, *str_ptr;
    struct t_string_dyn *ptr_string_dyn;

    POINTERS_EQUAL(NULL, string_dyn_alloc (-1));
    POINTERS_EQUAL(NULL, string_dyn_alloc (0));

    str = string_dyn_alloc (2);
    ptr_string_dyn = (struct t_string_dyn *)str;
    CHECK(str);
    CHECK(*str);
    STRCMP_EQUAL("", *str);

    /* check internal structure content */
    LONGS_EQUAL(2, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(1, ptr_string_dyn->size);
    STRCMP_EQUAL("", ptr_string_dyn->string);

    /* check copy with NULL */
    LONGS_EQUAL(1, string_dyn_copy (str, NULL));
    LONGS_EQUAL(2, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(1, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("", ptr_string_dyn->string);
    STRCMP_EQUAL("", *str);

    /* check copy with an empty string */
    LONGS_EQUAL(1, string_dyn_copy (str, ""));
    LONGS_EQUAL(2, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(1, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("", ptr_string_dyn->string);
    STRCMP_EQUAL("", *str);

    /* check copy with some strings */
    LONGS_EQUAL(1, string_dyn_copy (str, "a"));
    LONGS_EQUAL(2, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(2, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("a", ptr_string_dyn->string);
    STRCMP_EQUAL("a", *str);

    LONGS_EQUAL(1, string_dyn_copy (str, "abcd"));
    LONGS_EQUAL(5, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(5, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcd", ptr_string_dyn->string);
    STRCMP_EQUAL("abcd", *str);

    string_dyn_free (str, 1);

    str = string_dyn_alloc (1);
    ptr_string_dyn = (struct t_string_dyn *)str;

    /* check concat with NULL */
    LONGS_EQUAL(1, string_dyn_concat (str, NULL, -1));
    LONGS_EQUAL(1, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(1, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("", ptr_string_dyn->string);
    STRCMP_EQUAL("", *str);

    /* check concat with an empty string */
    LONGS_EQUAL(1, string_dyn_concat (str, "", -1));
    LONGS_EQUAL(1, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(1, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("", ptr_string_dyn->string);
    STRCMP_EQUAL("", *str);

    /* check concat with some strings and automatic length */
    LONGS_EQUAL(1, string_dyn_concat (str, "a", -1));
    LONGS_EQUAL(2, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(2, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("a", ptr_string_dyn->string);
    STRCMP_EQUAL("a", *str);

    LONGS_EQUAL(1, string_dyn_concat (str, "bcd", -1));
    LONGS_EQUAL(5, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(5, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcd", ptr_string_dyn->string);
    STRCMP_EQUAL("abcd", *str);

    LONGS_EQUAL(1, string_dyn_concat (str, "e", -1));
    LONGS_EQUAL(7, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(6, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcde", ptr_string_dyn->string);
    STRCMP_EQUAL("abcde", *str);

    LONGS_EQUAL(1, string_dyn_concat (str, "fg", -1));
    LONGS_EQUAL(10, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(8, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcdefg", ptr_string_dyn->string);
    STRCMP_EQUAL("abcdefg", *str);

    string_dyn_free (str, 1);

    str = string_dyn_alloc (1);
    ptr_string_dyn = (struct t_string_dyn *)str;

    /* check concat with some strings and fixed length */
    LONGS_EQUAL(1, string_dyn_copy (str, "abcd"));
    LONGS_EQUAL(1, string_dyn_concat (str, "xyz", 0));
    LONGS_EQUAL(5, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(5, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcd", ptr_string_dyn->string);
    STRCMP_EQUAL("abcd", *str);

    LONGS_EQUAL(1, string_dyn_concat (str, "xyz", 1));
    LONGS_EQUAL(7, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(6, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcdx", ptr_string_dyn->string);
    STRCMP_EQUAL("abcdx", *str);

    LONGS_EQUAL(1, string_dyn_concat (str, "xyz", 2));
    LONGS_EQUAL(10, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(8, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcdxxy", ptr_string_dyn->string);
    STRCMP_EQUAL("abcdxxy", *str);

    LONGS_EQUAL(1, string_dyn_concat (str, "xyz", 3));
    LONGS_EQUAL(15, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(11, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcdxxyxyz", ptr_string_dyn->string);
    STRCMP_EQUAL("abcdxxyxyz", *str);

    str_ptr = *str;
    string_dyn_free (str, 0);

    STRCMP_EQUAL("abcdxxyxyz", str_ptr);

    free (str_ptr);

    str_ptr = NULL;
    str = &str_ptr;

    /* test copy to NULL */
    LONGS_EQUAL(0, string_dyn_copy (NULL, NULL));
    LONGS_EQUAL(0, string_dyn_copy (NULL, "a"));
    LONGS_EQUAL(0, string_dyn_copy (str, NULL));
    LONGS_EQUAL(0, string_dyn_copy (str, "a"));

    /* test concat to NULL */
    LONGS_EQUAL(0, string_dyn_concat (NULL, NULL, 1));
    LONGS_EQUAL(0, string_dyn_concat (NULL, "a", 1));
    LONGS_EQUAL(0, string_dyn_concat (str, NULL, 1));
    LONGS_EQUAL(0, string_dyn_concat (str, "a", 1));

    /* test free of NULL */
    string_dyn_free (NULL, 0);
    string_dyn_free (str, 0);
}

/*
 * Tests functions:
 *   string_concat
 */

TEST(CoreString, Concat)
{
    STRCMP_EQUAL("", string_concat (NULL, NULL));
    STRCMP_EQUAL("", string_concat (NULL, "", NULL));
    STRCMP_EQUAL("", string_concat ("", "", NULL));
    STRCMP_EQUAL("", string_concat (",", "", NULL));

    STRCMP_EQUAL("abc", string_concat (NULL, "abc", NULL));
    STRCMP_EQUAL("abcdef", string_concat (NULL, "abc", "def", NULL));
    STRCMP_EQUAL("abcdefghi", string_concat (NULL, "abc", "def", "ghi", NULL));

    STRCMP_EQUAL("abc", string_concat ("", "abc", NULL));
    STRCMP_EQUAL("abcdef", string_concat ("", "abc", "def", NULL));
    STRCMP_EQUAL("abcdefghi", string_concat ("", "abc", "def", "ghi", NULL));

    STRCMP_EQUAL("abc", string_concat (",", "abc", NULL));
    STRCMP_EQUAL("abc,def", string_concat (",", "abc", "def", NULL));
    STRCMP_EQUAL("abc,def,ghi", string_concat (",", "abc", "def", "ghi", NULL));

    STRCMP_EQUAL("abc", string_concat (" / ", "abc", NULL));
    STRCMP_EQUAL("abc / def", string_concat (" / ", "abc", "def", NULL));
    STRCMP_EQUAL("abc / def / ghi", string_concat (" / ", "abc", "def", "ghi", NULL));
}
