/*
 * test-string.cpp - test string functions
 *
 * Copyright (C) 2014-2018 Sébastien Helleu <flashcode@flashtux.org>
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
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_PCRE
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#include "tests/tests.h"
#include "src/core/weechat.h"
#include "src/core/wee-string.h"
#include "src/core/wee-hashtable.h"
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
    if (__result_replace == NULL)                                       \
    {                                                                   \
        POINTERS_EQUAL(NULL, result);                                   \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result_replace, result);                         \
        free (result);                                                  \
    }                                                                   \
    if (__result_regex == 0)                                            \
        regfree(&regex);

#define WEE_REPLACE_CB(__result_replace, __result_errors,               \
                       __str, __prefix, __suffix,                       \
                       __list_prefix_no_replace,                        \
                       __callback, __callback_data, __errors)           \
    errors = -1;                                                        \
    result = string_replace_with_callback (                             \
        __str, __prefix, __suffix, __list_prefix_no_replace,            \
        __callback, __callback_data, __errors);                         \
    if (__result_replace == NULL)                                       \
    {                                                                   \
        POINTERS_EQUAL(NULL, result);                                   \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result_replace, result);                         \
        free (result);                                                  \
    }                                                                   \
    if (__result_errors >= 0)                                           \
    {                                                                   \
        LONGS_EQUAL(__result_errors, errors);                           \
    }

#define WEE_FORMAT_SIZE(__result, __size)                               \
    str = string_format_size (__size);                                  \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

extern struct t_hashtable *string_hashtable_shared;

TEST_GROUP(String)
{
};

/*
 * Tests functions:
 *   string_strndup
 */

TEST(String, Duplicate)
{
    const char *str_test = "test";
    char *str;

    POINTERS_EQUAL(NULL, string_strndup (NULL, 0));

    POINTERS_EQUAL(NULL, string_strndup (str_test, -1));

    str = string_strndup (str_test, 0);
    CHECK(str);
    CHECK(str != str_test);
    STRCMP_EQUAL(str, "");
    free (str);

    str = string_strndup (str_test, 2);
    CHECK(str);
    CHECK(str != str_test);
    STRCMP_EQUAL(str, "te");
    free (str);

    str = string_strndup (str_test, 500);
    CHECK(str);
    CHECK(str != str_test);
    STRCMP_EQUAL(str, str_test);
    free (str);
}

/*
 * Tests functions:
 *   string_tolower
 *   string_toupper
 */

TEST(String, Case)
{
    char *str;

    str = strdup ("ABC");

    string_tolower (str);
    STRCMP_EQUAL("abc", str);
    string_toupper (str);
    STRCMP_EQUAL("ABC", str);

    free (str);
}

/*
 * Tests functions:
 *   string_strcasecmp
 *   string_strncasecmp
 *   string_strcasecmp_range
 *   string_strncasecmp_range
 *   string_strcmp_ignore_chars
 */

TEST(String, Comparison)
{
    /* case-insensitive comparison */
    LONGS_EQUAL(0, string_strcasecmp (NULL, NULL));
    LONGS_EQUAL(-1, string_strcasecmp (NULL, "abc"));
    LONGS_EQUAL(1, string_strcasecmp ("abc", NULL));
    LONGS_EQUAL(0, string_strcasecmp ("abc", "abc"));
    LONGS_EQUAL(0, string_strcasecmp ("abc", "ABC"));
    LONGS_EQUAL(0, string_strcasecmp ("ABC", "ABC"));
    LONGS_EQUAL(-1, string_strcasecmp ("abc", "def"));
    LONGS_EQUAL(-1, string_strcasecmp ("abc", "DEF"));
    LONGS_EQUAL(-1, string_strcasecmp ("ABC", "def"));
    LONGS_EQUAL(-1, string_strcasecmp ("ABC", "DEF"));
    LONGS_EQUAL(1, string_strcasecmp ("def", "abc"));
    LONGS_EQUAL(1, string_strcasecmp ("def", "ABC"));
    LONGS_EQUAL(1, string_strcasecmp ("DEF", "abc"));
    LONGS_EQUAL(1, string_strcasecmp ("DEF", "ABC"));

    /* case-insensitive comparison with max length */
    LONGS_EQUAL(0, string_strncasecmp (NULL, NULL, 3));
    LONGS_EQUAL(-1, string_strncasecmp (NULL, "abc", 3));
    LONGS_EQUAL(1, string_strncasecmp ("abc", NULL, 3));
    LONGS_EQUAL(0, string_strncasecmp ("abc", "abc", 3));
    LONGS_EQUAL(0, string_strncasecmp ("abcabc", "abcdef", 3));
    LONGS_EQUAL(-1, string_strncasecmp ("abcabc", "abcdef", 6));
    LONGS_EQUAL(0, string_strncasecmp ("abc", "ABC", 3));
    LONGS_EQUAL(0, string_strncasecmp ("abcabc", "ABCDEF", 3));
    LONGS_EQUAL(-1, string_strncasecmp ("abcabc", "ABCDEF", 6));
    LONGS_EQUAL(0, string_strncasecmp ("ABC", "ABC", 3));
    LONGS_EQUAL(0, string_strncasecmp ("ABCABC", "ABCDEF", 3));
    LONGS_EQUAL(-1, string_strncasecmp ("ABCABC", "ABCDEF", 6));
    LONGS_EQUAL(-1, string_strncasecmp ("abc", "def", 3));
    LONGS_EQUAL(-1, string_strncasecmp ("abc", "DEF", 3));
    LONGS_EQUAL(-1, string_strncasecmp ("ABC", "def", 3));
    LONGS_EQUAL(-1, string_strncasecmp ("ABC", "DEF", 3));
    LONGS_EQUAL(1, string_strncasecmp ("def", "abc", 3));
    LONGS_EQUAL(1, string_strncasecmp ("def", "ABC", 3));
    LONGS_EQUAL(1, string_strncasecmp ("DEF", "abc", 3));
    LONGS_EQUAL(1, string_strncasecmp ("DEF", "ABC", 3));

    /* case-insensitive comparison with a range */
    LONGS_EQUAL(0, string_strcasecmp_range (NULL, NULL, 30));
    LONGS_EQUAL(-1, string_strcasecmp_range (NULL, "abc", 30));
    LONGS_EQUAL(1, string_strcasecmp_range ("abc", NULL, 30));
    LONGS_EQUAL(-1, string_strcasecmp_range ("A", "Z", 30));
    LONGS_EQUAL(1, string_strcasecmp_range ("Z", "A", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("A", "a", 30));
    LONGS_EQUAL(-1, string_strcasecmp_range ("ë", "€", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("[", "{", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("]", "}", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("\\", "|", 30));
    LONGS_EQUAL(0, string_strcasecmp_range ("^", "~", 30));
    LONGS_EQUAL(-1, string_strcasecmp_range ("[", "{", 26));
    LONGS_EQUAL(-1, string_strcasecmp_range ("]", "}", 26));
    LONGS_EQUAL(-1, string_strcasecmp_range ("\\", "|", 26));
    LONGS_EQUAL(-1, string_strcasecmp_range ("^", "~", 26));

    /* case-insensitive comparison with max length and a range */
    LONGS_EQUAL(0, string_strncasecmp_range (NULL, NULL, 3, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range (NULL, "abc", 3, 30));
    LONGS_EQUAL(1, string_strncasecmp_range ("abc", NULL, 3, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range ("ABC", "ZZZ", 3, 30));
    LONGS_EQUAL(1, string_strncasecmp_range ("ZZZ", "ABC", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("ABC", "abc", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("ABCABC", "abcdef", 3, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range ("ABCABC", "abcdef", 6, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range ("ëëë", "€€€", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("[[[", "{{{", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("[[[abc", "{{{def", 3, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range ("[[[abc", "{{{def", 6, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("]]]", "}}}", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("]]]abc", "}}}def", 3, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range ("]]]abc", "}}}def", 6, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("\\\\\\", "|||", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("\\\\\\abc", "|||def", 3, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range ("\\\\\\abc", "|||def", 6, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("^^^", "~~~", 3, 30));
    LONGS_EQUAL(0, string_strncasecmp_range ("^^^abc", "~~~def", 3, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range ("^^^abc", "~~~def", 6, 30));
    LONGS_EQUAL(-1, string_strncasecmp_range ("[[[", "{{{", 3, 26));
    LONGS_EQUAL(-1, string_strncasecmp_range ("]]]", "}}}", 3, 26));
    LONGS_EQUAL(-1, string_strncasecmp_range ("\\\\\\", "|||", 3, 26));
    LONGS_EQUAL(-1, string_strncasecmp_range ("^^^", "~~~", 3, 26));

    /* comparison with chars ignored */
    LONGS_EQUAL(0, string_strcmp_ignore_chars (NULL, NULL, "", 0));
    LONGS_EQUAL(-1, string_strcmp_ignore_chars (NULL, "abc", "", 0));
    LONGS_EQUAL(1, string_strcmp_ignore_chars ("abc", NULL, "", 0));
    LONGS_EQUAL(-1, string_strcmp_ignore_chars ("ABC", "ZZZ", "", 0));
    LONGS_EQUAL(1, string_strcmp_ignore_chars ("ZZZ", "ABC", "", 0));
    LONGS_EQUAL(0, string_strcmp_ignore_chars ("ABC", "abc", "", 0));
    LONGS_EQUAL(-1, string_strcmp_ignore_chars ("ABC", "abc", "", 1));
    LONGS_EQUAL(0, string_strcmp_ignore_chars ("abc..abc", "abcabc", ".", 0));
    LONGS_EQUAL(1, string_strcmp_ignore_chars ("abc..abc", "ABCABC", ".", 1));
    LONGS_EQUAL(0, string_strcmp_ignore_chars ("abc..abc", "abc-.-.abc",
                                               ".-", 0));
    LONGS_EQUAL(1, string_strcmp_ignore_chars ("abc..abc", "ABC-.-.ABC",
                                               ".-", 1));
}

/*
 * Tests functions:
 *   string_strcasestr
 */

TEST(String, Search)
{
    const char *str = "test";

    /* case-insensitive search of string in a string */
    POINTERS_EQUAL(NULL, string_strcasestr (NULL, NULL));
    POINTERS_EQUAL(NULL, string_strcasestr (NULL, str));
    POINTERS_EQUAL(NULL, string_strcasestr (str, NULL));
    POINTERS_EQUAL(NULL, string_strcasestr (str, ""));
    POINTERS_EQUAL(NULL, string_strcasestr (str, "zz"));
    POINTERS_EQUAL(str + 1, string_strcasestr (str, "est"));
    POINTERS_EQUAL(str + 1, string_strcasestr (str, "EST"));
}

/*
 * Tests functions:
 *   string_match
 */

TEST(String, Match)
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
}

/*
 * Tests functions:
 *   string_expand_home
 */

TEST(String, ExpandHome)
{
    char *home, *result;
    int length_home;

    home = getenv ("HOME");
    length_home = strlen (home);

    POINTERS_EQUAL(NULL, string_expand_home (NULL));

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

TEST(String, EvalPathHome)
{
    char *home, *result;
    int length_home, length_weechat_home;
    struct t_hashtable *extra_vars;

    home = getenv ("HOME");
    length_home = strlen (home);

    length_weechat_home = strlen (weechat_home);

    POINTERS_EQUAL(NULL, string_eval_path_home (NULL, NULL, NULL, NULL));

    result = string_eval_path_home ("/tmp/test", NULL, NULL, NULL);
    STRCMP_EQUAL(result, "/tmp/test");
    free (result);

    result = string_eval_path_home ("~/test", NULL, NULL, NULL);
    CHECK(strncmp (result, home, length_home) == 0);
    LONGS_EQUAL(length_home + 5, strlen (result));
    STRCMP_EQUAL(result + length_home, "/test");
    free (result);

    result = string_eval_path_home ("%h/test", NULL, NULL, NULL);
    CHECK(strncmp (result, weechat_home, length_weechat_home) == 0);
    LONGS_EQUAL(length_weechat_home + 5, strlen (result));
    STRCMP_EQUAL(result + length_weechat_home, "/test");
    free (result);

    setenv ("WEECHAT_TEST_PATH", "path1", 1);

    result = string_eval_path_home ("%h/${env:WEECHAT_TEST_PATH}/path2",
                                    NULL, NULL, NULL);
    CHECK(strncmp (result, weechat_home, length_weechat_home) == 0);
    LONGS_EQUAL(length_weechat_home + 12, strlen (result));
    STRCMP_EQUAL(result + length_weechat_home, "/path1/path2");
    free (result);

    extra_vars = hashtable_new (32,
                                WEECHAT_HASHTABLE_STRING,
                                WEECHAT_HASHTABLE_STRING,
                                NULL, NULL);
    CHECK(extra_vars);
    hashtable_set (extra_vars, "path2", "value");

    result = string_eval_path_home ("%h/${env:WEECHAT_TEST_PATH}/${path2}",
                                    NULL, extra_vars, NULL);
    CHECK(strncmp (result, weechat_home, length_weechat_home) == 0);
    LONGS_EQUAL(length_weechat_home + 12, strlen (result));
    STRCMP_EQUAL(result + length_weechat_home, "/path1/value");
    free (result);

    hashtable_free (extra_vars);
}

/*
 * Tests functions:
 *   string_remove_quotes
 */

TEST(String, RemoveQuotes)
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

TEST(String, Strip)
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

TEST(String, ConvertEscapedChars)
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
    WEE_TEST_STR(" zz", string_convert_escaped_chars ("\\u20zz"));
    WEE_TEST_STR("\U00012345", string_convert_escaped_chars ("\\U00012345"));
    WEE_TEST_STR("\U00000123zzz",
                 string_convert_escaped_chars ("\\U00123zzz"));
    WEE_TEST_STR("",
                 string_convert_escaped_chars ("\\U12345678")); /* invalid */
}

/*
 * Tests functions:
 *   string_is_word_char_highlight
 *   string_is_word_char_input
 */

TEST(String, IsWordChar)
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

TEST(String, MaskToRegex)
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

TEST(String, Regex)
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

TEST(String, Highlight)
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
 * It replaces "abc" by "def", "xxx" by empty string, and for any other value
 * it returns NULL (so the value is kept as-is).
 */

char *
test_replace_cb (void *data, const char *text)
{
    /* make C++ compiler happy */
    (void) data;

    if (strcmp (text, "abc") == 0)
        return strdup ("def");

    if (strcmp (text, "xxx") == 0)
        return strdup ("");

    if (strncmp (text, "no_replace:", 11) == 0)
        return strdup (text);

    return NULL;
}

/*
 * Tests functions:
 *   string_replace
 */

TEST(String, Replace)
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

TEST(String, ReplaceRegex)
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
 *   string_replace_with_callback
 */

TEST(String, ReplaceWithCallback)
{
    char *result;
    const char *list_prefix_no_replace[] = { "no_replace:", NULL };
    int errors;

    /* tests with invalid arguments */
    WEE_REPLACE_CB(NULL, -1, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, "", NULL, NULL, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, NULL, "", NULL, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, NULL, NULL, "", NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, NULL, NULL, NULL, NULL, &test_replace_cb, NULL, NULL);
    WEE_REPLACE_CB(NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, &errors);
    WEE_REPLACE_CB(NULL, -1, "test", NULL, NULL, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, "test", "${", NULL, NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, "test", NULL, "}", NULL, NULL, NULL, NULL);
    WEE_REPLACE_CB(NULL, -1, "test", NULL, NULL, NULL, &test_replace_cb, NULL, NULL);
    WEE_REPLACE_CB(NULL, 0, "test", NULL, NULL, NULL, NULL, NULL, &errors);
    WEE_REPLACE_CB(NULL, -1, "test", "${", "}", NULL, NULL, NULL, NULL);

    /* valid arguments */
    WEE_REPLACE_CB("test", -1, "test", "${", "}", NULL,
                   &test_replace_cb, NULL, NULL);
    WEE_REPLACE_CB("test", 0, "test", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test def", 0, "test ${abc}", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ", 0, "test ${xxx}", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ${aaa}", 1, "test ${aaa}", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test def  ${aaa}", 1, "test ${abc} ${xxx} ${aaa}",
                   "${", "}", NULL, &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ", 1, "test ${abc", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test abc}", 0, "test abc}", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ${}", 1, "test ${}", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("test ${ }", 1, "test ${ }", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("def", 0, "${abc}", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("", 0, "${xxx}", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("${aaa}", 1, "${aaa}", "${", "}", NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("no_replace:def", 0, "${no_replace:${abc}}", "${", "}",
                   NULL,
                   &test_replace_cb, NULL, &errors);
    WEE_REPLACE_CB("no_replace:${abc}", 0, "${no_replace:${abc}}", "${", "}",
                   list_prefix_no_replace,
                   &test_replace_cb, NULL, &errors);
}

/*
 * Tests functions:
 *    string_split
 *    string_free_split
 */

TEST(String, Split)
{
    char **argv;
    int argc;

    POINTERS_EQUAL(NULL, string_split (NULL, NULL, 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split (NULL, "", 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split ("", NULL, 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split ("", "", 0, 0, NULL));

    argc = -1;
    POINTERS_EQUAL(NULL, string_split (NULL, NULL, 0, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = -1;
    POINTERS_EQUAL(NULL, string_split (NULL, "", 0, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = -1;
    POINTERS_EQUAL(NULL, string_split ("", NULL, 0, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = -1;
    POINTERS_EQUAL(NULL, string_split ("", "", 0, 0, &argc));
    LONGS_EQUAL(0, argc);

    /* free split with NULL */
    string_free_split (NULL);

    /* standard split */
    argv = string_split (" abc de  fghi ", " ", 0, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* max 2 items */
    argv = string_split (" abc de  fghi ", " ", 0, 2, &argc);
    LONGS_EQUAL(2, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    POINTERS_EQUAL(NULL, argv[2]);
    string_free_split (argv);

    /* keep eol == 1 */
    argv = string_split (" abc de  fghi ", " ", 1, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc de  fghi", argv[0]);
    STRCMP_EQUAL("de  fghi", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* keep eol == 1 and max 2 items */
    argv = string_split (" abc de  fghi ", " ", 1, 2, &argc);
    LONGS_EQUAL(2, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc de  fghi", argv[0]);
    STRCMP_EQUAL("de  fghi", argv[1]);
    POINTERS_EQUAL(NULL, argv[2]);
    string_free_split (argv);

    /* keep eol == 2 */
    argv = string_split (" abc de  fghi ", " ", 2, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc de  fghi ", argv[0]);
    STRCMP_EQUAL("de  fghi ", argv[1]);
    STRCMP_EQUAL("fghi ", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* keep eol == 2 and max 2 items */
    argv = string_split (" abc de  fghi ", " ", 2, 2, &argc);
    LONGS_EQUAL(2, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc de  fghi ", argv[0]);
    STRCMP_EQUAL("de  fghi ", argv[1]);
    POINTERS_EQUAL(NULL, argv[2]);
    string_free_split (argv);
}

/*
 * Tests functions:
 *    string_split_shared
 *    string_free_split_shared
 */

TEST(String, SplitShared)
{
    char **argv;
    int argc;

    POINTERS_EQUAL(NULL, string_split_shared (NULL, NULL, 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split_shared (NULL, "", 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split_shared ("", NULL, 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split_shared ("", "", 0, 0, NULL));

    argv = string_split_shared (" abc de  abc ", " ", 0, 0, &argc);
    LONGS_EQUAL(3, argc);
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("abc", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);

    /* same content == same pointer for shared strings */
    POINTERS_EQUAL(argv[0], argv[2]);

    string_free_split_shared (argv);

    /* free split with NULL */
    string_free_split_shared (NULL);
}

/*
 * Tests functions:
 *    string_split_shell
 *    string_free_split
 */

TEST(String, SplitShell)
{
    char **argv;
    int argc;

    POINTERS_EQUAL(NULL, string_split_shell (NULL, NULL));

    /* test with an empty string */
    argc = -1;
    argv = string_split_shell ("", &argc);
    LONGS_EQUAL(0, argc);
    CHECK(argv);
    POINTERS_EQUAL(NULL, argv[0]);
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
    POINTERS_EQUAL(NULL, argv[4]);
    string_free_split (argv);

    /* free split with NULL */
    string_free_split_shared (NULL);
}

/*
 * Tests functions:
 *    string_split_command
 *    string_free_split_command
 */

TEST(String, SplitCommand)
{
    char **argv;

    /* test with a NULL/empty string */
    POINTERS_EQUAL(NULL, string_split_command (NULL, ';'));
    POINTERS_EQUAL(NULL, string_split_command ("", ';'));

    /* string with one command */
    argv = string_split_command ("abc", ';');
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    POINTERS_EQUAL(NULL, argv[1]);
    string_free_split_command (argv);

    /* string with 3 commands */
    argv = string_split_command ("abc;de;fghi", ';');
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);
    string_free_split_command (argv);

    /* string with 3 commands (containing spaces) */
    argv = string_split_command ("  abc ; de ; fghi  ", ';');
    CHECK(argv);
    STRCMP_EQUAL("abc ", argv[0]);
    STRCMP_EQUAL("de ", argv[1]);
    STRCMP_EQUAL("fghi  ", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);
    string_free_split_command (argv);

    /* separator other than ';' */
    argv = string_split_command ("abc,de,fghi", ',');
    CHECK(argv);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);
    string_free_split_command (argv);

    /* free split with NULL */
    string_free_split_command (NULL);
}

/*
 * Tests functions:
 *    string_build_with_split_string
 */

TEST(String, SplitBuildWithSplitString)
{
    char **argv, *str;
    int argc;

    str = string_build_with_split_string (NULL, NULL);
    POINTERS_EQUAL(NULL, str);

    argv = string_split (" abc de  fghi ", " ", 0, 0, &argc);

    str = string_build_with_split_string ((const char **)argv, NULL);
    STRCMP_EQUAL("abcdefghi", str);
    free (str);

    str = string_build_with_split_string ((const char **)argv, "");
    STRCMP_EQUAL("abcdefghi", str);
    free (str);

    str = string_build_with_split_string ((const char **)argv, ";;");
    STRCMP_EQUAL("abc;;de;;fghi", str);
    free (str);

    string_free_split (argv);
}

/*
 * Tests functions:
 *    string_iconv
 *    string_iconv_to_internal
 *    string_iconv_from_internal
 *    string_fprintf
 */

TEST(String, Iconv)
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
 *    string_format_size
 */

TEST(String, FormatSize)
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
 *    string_encode_base16
 *    string_decode_base16
 *    string_encode_base64
 *    string_decode_base64
 */

TEST(String, BaseN)
{
    char str[1024];
    const char *str_base64[][2] =
        { { "", "" },
          { "A", "QQ==" },
          { "B", "Qg==" },
          { "C", "Qw==" },
          { "D", "RA==" },
          { "abc", "YWJj" },
          { "This is a test.", "VGhpcyBpcyBhIHRlc3Qu" },
          { "This is a test..", "VGhpcyBpcyBhIHRlc3QuLg==" },
          { "This is a test...", "VGhpcyBpcyBhIHRlc3QuLi4=" },
          { "This is a test....", "VGhpcyBpcyBhIHRlc3QuLi4u" },
          { "This is a long long long sentence here...",
            "VGhpcyBpcyBhIGxvbmcgbG9uZyBsb25nIHNlbnRlbmNlIGhlcmUuLi4=" },
          { "Another example for base64",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQ=" },
          { "Another example for base64.",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQu" },
          { "Another example for base64..",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQuLg==" },
          { "Another example for base64...",
            "QW5vdGhlciBleGFtcGxlIGZvciBiYXNlNjQuLi4=" },
          { NULL, NULL } };
    int i, length;

    /* string_encode_base16 */
    string_encode_base16 (NULL, 0, NULL);
    string_encode_base16 (NULL, 0, str);
    string_encode_base16 ("", 0, NULL);
    str[0] = 0xAA;
    string_encode_base16 ("", -1, str);
    BYTES_EQUAL(0x0, str[0]);
    str[0] = 0xAA;
    string_encode_base16 ("", 0, str);
    BYTES_EQUAL(0x0, str[0]);
    string_encode_base16 ("abc", 3, str);
    STRCMP_EQUAL("616263", str);

    /* string_decode_base16 */
    LONGS_EQUAL(0, string_decode_base16 (NULL, NULL));
    LONGS_EQUAL(0, string_decode_base16 (NULL, str));
    LONGS_EQUAL(0, string_decode_base16 ("", NULL));
    LONGS_EQUAL(0, string_decode_base16 ("", str));
    LONGS_EQUAL(3, string_decode_base16 ("616263", str));
    STRCMP_EQUAL("abc", str);

    /* string_encode_base64 */
    string_encode_base64 (NULL, 0, NULL);
    string_encode_base64 (NULL, 0, str);
    string_encode_base64 ("", 0, NULL);
    str[0] = 0xAA;
    string_encode_base64 ("", -1, str);
    BYTES_EQUAL(0x0, str[0]);
    str[0] = 0xAA;
    string_encode_base64 ("", 0, str);
    BYTES_EQUAL(0x0, str[0]);
    for (i = 0; str_base64[i][0]; i++)
    {
        string_encode_base64 (str_base64[i][0], strlen (str_base64[i][0]),
                              str);
        STRCMP_EQUAL(str_base64[i][1], str);
    }

    /* string_decode_base64 */
    LONGS_EQUAL(0, string_decode_base64 (NULL, NULL));
    LONGS_EQUAL(0, string_decode_base64 (NULL, str));
    LONGS_EQUAL(0, string_decode_base64 ("", NULL));
    LONGS_EQUAL(0, string_decode_base64 ("", str));
    for (i = 0; str_base64[i][0]; i++)
    {
        length = string_decode_base64 (str_base64[i][1], str);
        STRCMP_EQUAL(str_base64[i][0], str);
        LONGS_EQUAL(strlen (str_base64[i][0]), length);
    }
}

/*
 * Tests functions:
 *    string_hex_dump
 */

TEST(String, Hex_dump)
{
    const char *noel_utf8 = "no\xc3\xabl";  /* noël */
    const char *noel_iso = "no\xebl";
    char *str;

    POINTERS_EQUAL(NULL, string_hex_dump (NULL, 0, 0, NULL, NULL));
    POINTERS_EQUAL(NULL, string_hex_dump ("abc", 0, 0, NULL, NULL));
    POINTERS_EQUAL(NULL, string_hex_dump ("abc", 3, 0, NULL, NULL));
    POINTERS_EQUAL(NULL, string_hex_dump ("abc", 0, 5, NULL, NULL));

    str = string_hex_dump ("abc", 3, 3, NULL, NULL);
    STRCMP_EQUAL("61 62 63   a b c ", str);

    str = string_hex_dump ("abc", 3, 3, "", "");
    STRCMP_EQUAL("61 62 63   a b c ", str);

    str = string_hex_dump ("abc", 3, 3, "(( ", NULL);
    STRCMP_EQUAL("(( 61 62 63   a b c ", str);

    str = string_hex_dump ("abc", 3, 3, NULL, " ))");
    STRCMP_EQUAL("61 62 63   a b c  ))", str);

    str = string_hex_dump ("abc", 3, 3, "(( ", " ))");
    STRCMP_EQUAL("(( 61 62 63   a b c  ))", str);

    str = string_hex_dump ("abc", 3, 5, NULL, NULL);
    STRCMP_EQUAL("61 62 63         a b c     ", str);

    str = string_hex_dump ("abc", 3, 10, NULL, NULL);
    STRCMP_EQUAL("61 62 63                        a b c               ", str);

    str = string_hex_dump ("abc", 3, 2, NULL, NULL);
    STRCMP_EQUAL("61 62   a b \n"
                 "63      c   ",
                 str);

    str = string_hex_dump (noel_utf8, strlen (noel_utf8), 5, NULL, NULL);
    STRCMP_EQUAL("6E 6F C3 AB 6C   n o . . l ", str);

    str = string_hex_dump (noel_utf8, strlen (noel_utf8), 2, NULL, NULL);
    STRCMP_EQUAL("6E 6F   n o \n"
                 "C3 AB   . . \n"
                 "6C      l   ",
                 str);
    str = string_hex_dump (noel_utf8, strlen (noel_utf8), 2, "( ", NULL);
    STRCMP_EQUAL("( 6E 6F   n o \n"
                 "( C3 AB   . . \n"
                 "( 6C      l   ",
                 str);
    str = string_hex_dump (noel_utf8, strlen (noel_utf8), 2, "( ", " )");
    STRCMP_EQUAL("( 6E 6F   n o  )\n"
                 "( C3 AB   . .  )\n"
                 "( 6C      l    )",
                 str);

    str = string_hex_dump (noel_iso, strlen (noel_iso), 5, NULL, NULL);
    STRCMP_EQUAL("6E 6F EB 6C      n o . l   ", str);

    str = string_hex_dump (noel_iso, strlen (noel_iso), 2, NULL, NULL);
    STRCMP_EQUAL("6E 6F   n o \n"
                 "EB 6C   . l ",
                 str);
}

/*
 * Tests functions:
 *    string_is_command_char
 *    string_input_for_buffer
 */

TEST(String, Input)
{
    char *str;

    /* string_is_command_char */
    LONGS_EQUAL(0, string_is_command_char (NULL));
    LONGS_EQUAL(0, string_is_command_char (""));
    LONGS_EQUAL(0, string_is_command_char ("abc"));
    LONGS_EQUAL(1, string_is_command_char ("/"));
    LONGS_EQUAL(1, string_is_command_char ("/abc"));
    LONGS_EQUAL(1, string_is_command_char ("//abc"));

    /* string_input_for_buffer */
    POINTERS_EQUAL(NULL, string_input_for_buffer (NULL));
    POINTERS_EQUAL(NULL, string_input_for_buffer ("/abc"));
    str = strdup ("");
    STRCMP_EQUAL(str, string_input_for_buffer (str));
    free (str);
    str = strdup ("/");
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
}

/*
 * Tests functions:
 *    string_shared_get
 *    string_shared_free
 */

TEST(String, Shared)
{
    const char *str1, *str2, *str3;
    int count;

    count = string_hashtable_shared->items_count;

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
}

/*
 * Tests functions:
 *    string_dyn_alloc
 *    string_dyn_copy
 *    string_dyn_concat
 *    string_dyn_free
 */

TEST(String, Dyn)
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
    LONGS_EQUAL(1, string_dyn_concat (str, NULL));
    LONGS_EQUAL(1, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(1, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("", ptr_string_dyn->string);
    STRCMP_EQUAL("", *str);

    /* check concat with an empty string */
    LONGS_EQUAL(1, string_dyn_concat (str, ""));
    LONGS_EQUAL(1, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(1, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("", ptr_string_dyn->string);
    STRCMP_EQUAL("", *str);

    /* check concat with some strings */
    LONGS_EQUAL(1, string_dyn_concat (str, "a"));
    LONGS_EQUAL(2, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(2, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("a", ptr_string_dyn->string);
    STRCMP_EQUAL("a", *str);

    LONGS_EQUAL(1, string_dyn_concat (str, "bcd"));
    LONGS_EQUAL(5, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(5, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcd", ptr_string_dyn->string);
    STRCMP_EQUAL("abcd", *str);

    LONGS_EQUAL(1, string_dyn_concat (str, "e"));
    LONGS_EQUAL(7, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(6, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcde", ptr_string_dyn->string);
    STRCMP_EQUAL("abcde", *str);

    LONGS_EQUAL(1, string_dyn_concat (str, "fg"));
    LONGS_EQUAL(10, ptr_string_dyn->size_alloc);
    LONGS_EQUAL(8, ptr_string_dyn->size);
    POINTERS_EQUAL(ptr_string_dyn->string, *str);
    STRCMP_EQUAL("abcdefg", ptr_string_dyn->string);
    STRCMP_EQUAL("abcdefg", *str);

    str_ptr = *str;
    string_dyn_free (str, 0);

    STRCMP_EQUAL("abcdefg", str_ptr);

    free (str_ptr);
}
