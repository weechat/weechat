/*
 * test-string.cpp - test string functions
 *
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "../src/core/wee-string.h"
}

#define ONE_KB 1000ULL
#define ONE_MB (ONE_KB * 1000ULL)
#define ONE_GB (ONE_MB * 1000ULL)
#define ONE_TB (ONE_GB * 1000ULL)

#define WEE_HAS_HL_STR(__result, __str, __words)                        \
    LONGS_EQUAL(__result, string_has_highlight (__str, __words));

#define WEE_HAS_HL_REGEX(__result_regex, __result_hl, __str, __regex)   \
    LONGS_EQUAL(__result_hl,                                            \
                string_has_highlight_regex (__str, __regex));           \
    LONGS_EQUAL(__result_regex,                                         \
                string_regcomp (&regex, __regex, REG_ICASE));           \
    if (__result_regex == 0)                                            \
    {                                                                   \
        LONGS_EQUAL(__result_hl,                                        \
                    string_has_highlight_regex_compiled (__str,         \
                                                         &regex));      \
        regfree(&regex);                                                \
    }

#define WEE_FORMAT_SIZE(__result, __size)                               \
    str = string_format_size (__size);                                  \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

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

    str = string_strndup (NULL, 0);

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

    result = string_expand_home("~/abc.txt");
    CHECK(strncmp (result, home, length_home) == 0);
    LONGS_EQUAL(length_home + 8, strlen (result));
    STRCMP_EQUAL(result + length_home, "/abc.txt");
    free (result);
}

/*
 * Tests functions:
 *   string_remove_quotes
 */

TEST(String, RemoveQuotes)
{
    POINTERS_EQUAL(NULL, string_remove_quotes (NULL, NULL));
    POINTERS_EQUAL(NULL, string_remove_quotes (NULL, "abc"));
    POINTERS_EQUAL(NULL, string_remove_quotes ("abc", NULL));
    STRCMP_EQUAL("", string_remove_quotes("", ""));
    STRCMP_EQUAL("", string_remove_quotes("", "\"'"));
    STRCMP_EQUAL("abc", string_remove_quotes("abc", "\"'"));
    STRCMP_EQUAL(" abc ", string_remove_quotes(" abc ", "\"'"));
    STRCMP_EQUAL("abc", string_remove_quotes("'abc'", "\"'"));
    STRCMP_EQUAL("abc", string_remove_quotes(" 'abc' ", "\"'"));
    STRCMP_EQUAL("'abc'", string_remove_quotes("\"'abc'\"", "\"'"));
    STRCMP_EQUAL("'abc'", string_remove_quotes(" \"'abc'\" ", "\"'"));
    STRCMP_EQUAL("'a'b'c'", string_remove_quotes("\"'a'b'c'\"", "\"'"));
    STRCMP_EQUAL("'a'b'c'", string_remove_quotes(" \"'a'b'c'\" ", "\"'"));
}

/*
 * Tests functions:
 *   string_strip
 */

TEST(String, Strip)
{
    POINTERS_EQUAL(NULL, string_strip (NULL, 1, 1, NULL));
    POINTERS_EQUAL(NULL, string_strip (NULL, 1, 1, ".;"));
    STRCMP_EQUAL("test", string_strip ("test", 1, 1, NULL));
    STRCMP_EQUAL("test", string_strip ("test", 1, 1, ".;"));
    STRCMP_EQUAL(".-test.-", string_strip (".-test.-", 0, 0, ".-"));
    STRCMP_EQUAL("test", string_strip (".-test.-", 1, 1, ".-"));
    STRCMP_EQUAL("test.-", string_strip (".-test.-", 1, 0, ".-"));
    STRCMP_EQUAL(".-test", string_strip (".-test.-", 0, 1, ".-"));
}

/*
 * Tests functions:
 *   string_convert_escaped_chars
 */

TEST(String, ConvertEscapedChars)
{
    POINTERS_EQUAL(NULL, string_convert_escaped_chars (NULL));
    STRCMP_EQUAL("", string_convert_escaped_chars (""));
    STRCMP_EQUAL("\"", string_convert_escaped_chars ("\\\""));
    STRCMP_EQUAL("\\", string_convert_escaped_chars ("\\\\"));
    STRCMP_EQUAL("\a", string_convert_escaped_chars ("\\a"));
    STRCMP_EQUAL("\a", string_convert_escaped_chars ("\\a"));
    STRCMP_EQUAL("\b", string_convert_escaped_chars ("\\b"));
    STRCMP_EQUAL("\e", string_convert_escaped_chars ("\\e"));
    STRCMP_EQUAL("\f", string_convert_escaped_chars ("\\f"));
    STRCMP_EQUAL("\n", string_convert_escaped_chars ("\\n"));
    STRCMP_EQUAL("\r", string_convert_escaped_chars ("\\r"));
    STRCMP_EQUAL("\t", string_convert_escaped_chars ("\\t"));
    STRCMP_EQUAL("\v", string_convert_escaped_chars ("\\v"));
    STRCMP_EQUAL("\123", string_convert_escaped_chars ("\\0123"));
    STRCMP_EQUAL("\123",
                 string_convert_escaped_chars ("\\0123"));  /* invalid */
    STRCMP_EQUAL("\x41", string_convert_escaped_chars ("\\x41"));
    STRCMP_EQUAL("\x04z", string_convert_escaped_chars ("\\x4z"));
    STRCMP_EQUAL("\u0012zz", string_convert_escaped_chars ("\\u12zz"));
    STRCMP_EQUAL("\U00123456", string_convert_escaped_chars ("\\U00123456"));
    STRCMP_EQUAL("\U00000123zzz",
                 string_convert_escaped_chars ("\\U00123zzz"));
    STRCMP_EQUAL("",
                 string_convert_escaped_chars ("\\U12345678")); /* invalid */
}

/*
 * Tests functions:
 *   string_is_word_char
 */

TEST(String, IsWordChar)
{
    LONGS_EQUAL(0, string_is_word_char (NULL));
    LONGS_EQUAL(0, string_is_word_char (""));
    LONGS_EQUAL(0, string_is_word_char ("&abc"));
    LONGS_EQUAL(0, string_is_word_char ("+abc"));
    LONGS_EQUAL(0, string_is_word_char ("$abc"));
    LONGS_EQUAL(0, string_is_word_char ("*abc"));
    LONGS_EQUAL(0, string_is_word_char ("/abc"));

    LONGS_EQUAL(1, string_is_word_char ("abc"));
    LONGS_EQUAL(1, string_is_word_char ("-abc"));
    LONGS_EQUAL(1, string_is_word_char ("_abc"));
    LONGS_EQUAL(1, string_is_word_char ("|abc"));
}

/*
 * Tests functions:
 *   string_mask_to_regex
 */

TEST(String, MaskToRegex)
{
    POINTERS_EQUAL(NULL, string_mask_to_regex (NULL));
    STRCMP_EQUAL("", string_mask_to_regex (""));
    STRCMP_EQUAL("test", string_mask_to_regex ("test"));
    STRCMP_EQUAL("test.*", string_mask_to_regex ("test*"));
    STRCMP_EQUAL(".*test.*", string_mask_to_regex ("*test*"));
    STRCMP_EQUAL(".*te.*st.*", string_mask_to_regex ("*te*st*"));
    STRCMP_EQUAL("test\\.\\[\\]\\{\\}\\(\\)\\?\\+\\|\\^\\$\\\\",
                 string_mask_to_regex ("test.[]{}()?+|^$\\"));
}

/*
 * Tests functions:
 *   string_regex_flags
 *   string_regcomp
 */

TEST(String, Regex)
{
    int flags, rc;
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

    ptr = string_regex_flags ("test", REG_EXTENDED, &flags);
    LONGS_EQUAL(REG_EXTENDED, flags);
    STRCMP_EQUAL("test", ptr);

    string_regex_flags ("(?e)test", 0, &flags);
    LONGS_EQUAL(REG_EXTENDED, flags);
    STRCMP_EQUAL("test", ptr);

    string_regex_flags ("(?ei)test", 0, &flags);
    LONGS_EQUAL(REG_EXTENDED | REG_ICASE, flags);
    STRCMP_EQUAL("test", ptr);

    string_regex_flags ("(?eins)test", 0, &flags);
    LONGS_EQUAL(REG_EXTENDED | REG_ICASE | REG_NEWLINE | REG_NOSUB, flags);
    STRCMP_EQUAL("test", ptr);

    string_regex_flags ("(?ins)test", REG_EXTENDED, &flags);
    LONGS_EQUAL(REG_EXTENDED | REG_ICASE | REG_NEWLINE | REG_NOSUB, flags);
    STRCMP_EQUAL("test", ptr);

    string_regex_flags ("(?ins-e)test", REG_EXTENDED, &flags);
    LONGS_EQUAL(REG_ICASE | REG_NEWLINE | REG_NOSUB, flags);
    STRCMP_EQUAL("test", ptr);

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
    WEE_HAS_HL_STR(1, "test", "test");
    WEE_HAS_HL_STR(1, "this is a test", "test");
    WEE_HAS_HL_STR(1, "test here", "test");
    WEE_HAS_HL_STR(1, "this is a test here", "test");
    WEE_HAS_HL_STR(0, "this is a test here", "abc,def");
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
 * Tests functions:
 *    string_replace
 *    string_replace_regex
 *    string_replace_with_callback
 */

TEST(String, Replace)
{
    POINTERS_EQUAL(NULL, string_replace (NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, string_replace ("string", NULL, NULL));
    POINTERS_EQUAL(NULL, string_replace (NULL, "search", NULL));
    POINTERS_EQUAL(NULL, string_replace (NULL, NULL, "replace"));
    POINTERS_EQUAL(NULL, string_replace ("string", "search", NULL));
    POINTERS_EQUAL(NULL, string_replace ("string", NULL, "replace"));
    POINTERS_EQUAL(NULL, string_replace (NULL, "search", "replace"));

    STRCMP_EQUAL("test abc def", string_replace("test abc def", "xyz", "xxx"));
    STRCMP_EQUAL("test xxx def", string_replace("test abc def", "abc", "xxx"));
    STRCMP_EQUAL("xxx test xxx def xxx",
                 string_replace("abc test abc def abc", "abc", "xxx"));
}

/*
 * Tests functions:
 *    string_split
 *    string_split_shared
 *    string_split_shell
 *    string_free_split
 *    string_free_split_shared
 *    string_build_with_split_string
 *    string_split_command
 *    string_free_split_command
 */

TEST(String, Split)
{
    char **argv, *str;
    int argc;

    POINTERS_EQUAL(NULL, string_split (NULL, NULL, 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split (NULL, "", 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split ("", NULL, 0, 0, NULL));
    POINTERS_EQUAL(NULL, string_split ("", "", 0, 0, NULL));

    argc = 1;
    POINTERS_EQUAL(NULL, string_split (NULL, NULL, 0, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = 1;
    POINTERS_EQUAL(NULL, string_split (NULL, "", 0, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = 1;
    POINTERS_EQUAL(NULL, string_split ("", NULL, 0, 0, &argc));
    LONGS_EQUAL(0, argc);
    argc = 1;
    POINTERS_EQUAL(NULL, string_split ("", "", 0, 0, &argc));
    LONGS_EQUAL(0, argc);

    /* free split with NULL */
    string_free_split (NULL);
    string_free_split_shared (NULL);
    string_free_split_command (NULL);

    /* standard split */
    argv = string_split (" abc de  fghi ", " ", 0, 0, &argc);
    LONGS_EQUAL(3, argc);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* max 2 items */
    argv = string_split (" abc de  fghi ", " ", 0, 2, &argc);
    LONGS_EQUAL(2, argc);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    POINTERS_EQUAL(NULL, argv[2]);
    string_free_split (argv);

    /* keep eol */
    argv = string_split (" abc de  fghi ", " ", 1, 0, &argc);
    LONGS_EQUAL(3, argc);
    STRCMP_EQUAL("abc de  fghi", argv[0]);
    STRCMP_EQUAL("de  fghi", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);
    string_free_split (argv);

    /* keep eol and max 2 items */
    argv = string_split (" abc de  fghi ", " ", 1, 2, &argc);
    LONGS_EQUAL(2, argc);
    STRCMP_EQUAL("abc de  fghi", argv[0]);
    STRCMP_EQUAL("de  fghi", argv[1]);
    POINTERS_EQUAL(NULL, argv[2]);
    string_free_split (argv);

    /* split with shared strings */
    argv = string_split_shared (" abc de  abc ", " ", 0, 0, &argc);
    LONGS_EQUAL(3, argc);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("abc", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);
    /* same content == same pointer for shared strings */
    POINTERS_EQUAL(argv[0], argv[2]);
    string_free_split_shared (argv);

    /* build string with split string */
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

    /* split command */
    POINTERS_EQUAL(NULL, string_split_command (NULL, ';'));
    POINTERS_EQUAL(NULL, string_split_command ("", ';'));
    argv = string_split_command ("abc;de;fghi", ';');
    LONGS_EQUAL(3, argc);
    STRCMP_EQUAL("abc", argv[0]);
    STRCMP_EQUAL("de", argv[1]);
    STRCMP_EQUAL("fghi", argv[2]);
    POINTERS_EQUAL(NULL, argv[3]);
    string_free_split_command (argv);
}

/*
 * Tests functions:
 *    string_iconv
 *    string_iconv_to_internal
 *    string_iconv_from_internal
 *    string_iconv_fprintf
 */

TEST(String, Iconv)
{
    const char *noel_utf8 = "no\xc3\xabl";  /* noël */
    const char *noel_iso = "no\xebl";
    char *str;
    FILE *f;

    /* string_iconv */
    POINTERS_EQUAL(NULL, string_iconv (0, NULL, NULL, NULL));
    STRCMP_EQUAL("", string_iconv (0, NULL, NULL, ""));
    STRCMP_EQUAL("abc", string_iconv (0, NULL, NULL, "abc"));
    STRCMP_EQUAL("abc", string_iconv (1, "UTF-8", "ISO-8859-15", "abc"));
    STRCMP_EQUAL(noel_iso,
                 string_iconv (1, "UTF-8", "ISO-8859-15", noel_utf8));
    STRCMP_EQUAL(noel_utf8,
                 string_iconv (0, "ISO-8859-15", "UTF-8", noel_iso));

    /* string_iconv_to_internal */
    POINTERS_EQUAL(NULL, string_iconv_to_internal (NULL, NULL));
    STRCMP_EQUAL("", string_iconv_to_internal (NULL, ""));
    STRCMP_EQUAL("abc", string_iconv_to_internal (NULL, "abc"));
    STRCMP_EQUAL(noel_utf8,
                 string_iconv_to_internal ("ISO-8859-15", noel_iso));

    /* string_iconv_from_internal */
    POINTERS_EQUAL(NULL, string_iconv_from_internal (NULL, NULL));
    STRCMP_EQUAL("", string_iconv_from_internal (NULL, ""));
    STRCMP_EQUAL("abc", string_iconv_from_internal (NULL, "abc"));
    STRCMP_EQUAL(noel_iso,
                 string_iconv_from_internal ("ISO-8859-15", noel_utf8));

    /* string_iconv_fprintf */
    f = fopen ("/dev/null", "w");
    LONGS_EQUAL(0, string_iconv_fprintf (f, NULL));
    LONGS_EQUAL(1, string_iconv_fprintf (f, "abc"));
    LONGS_EQUAL(1, string_iconv_fprintf (f, noel_utf8));
    LONGS_EQUAL(1, string_iconv_fprintf (f, noel_iso));
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
    /* TODO: write tests */
}

/*
 * Tests functions:
 *    string_is_command_char
 *    string_input_for_buffer
 */

TEST(String, Input)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *    string_is_command_char
 */

TEST(String, CommandChar)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *    string_shared_get
 *    string_shared_free
 */

TEST(String, Shared)
{
    /* TODO: write tests */
}
