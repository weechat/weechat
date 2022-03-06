/*
 * test-core-utf8.cpp - test UTF-8 string functions
 *
 * Copyright (C) 2014-2022 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>
#include <wctype.h>
#include "src/core/wee-utf8.h"
#include "src/core/wee-config.h"
}

const char *empty_string = "";
const char *utf_4bytes_invalid = "\xf0\x03\x02\x01";
const char *utf_2bytes_truncated_1 = "\xc0";
const char *utf_3bytes_truncated_1 = "\xe2";
const char *utf_3bytes_truncated_2 = "\xe2\xbb";
const char *utf_4bytes_truncated_1 = "\xf0";
const char *utf_4bytes_truncated_2 = "\xf0\xa4";
const char *utf_4bytes_truncated_3 = "\xf0\xa4\xad";
const char *noel_valid = "no\xc3\xabl";        /* noël */
const char *noel_invalid = "no\xc3l";
const char *noel_invalid2 = "no\xff\xffl";
const char *noel_invalid_norm = "no?l";
const char *noel_invalid2_norm = "no??l";
const char *cjk_yellow = "\xe2\xbb\xa9";       /* U+2EE9 */
const char *han_char = "\xf0\xa4\xad\xa2";     /* U+24B62 */
const char *han_char_z = "\xf0\xa4\xad\xa2Z";

TEST_GROUP(CoreUtf8)
{
};

/*
 * Tests functions:
 *   utf8_has_8bits
 *   utf8_is_valid
 */

TEST(CoreUtf8, Validity)
{
    char *error;

    /* check 8 bits */
    LONGS_EQUAL(0, utf8_has_8bits (NULL));
    LONGS_EQUAL(0, utf8_has_8bits (""));
    LONGS_EQUAL(0, utf8_has_8bits ("abc"));
    LONGS_EQUAL(1, utf8_has_8bits ("no\xc3\xabl"));

    /* check validity */
    LONGS_EQUAL(1, utf8_is_valid (NULL, -1, NULL));
    LONGS_EQUAL(1, utf8_is_valid (NULL, 0, NULL));
    LONGS_EQUAL(1, utf8_is_valid (NULL, 1, NULL));
    LONGS_EQUAL(1, utf8_is_valid (NULL, -1, &error));
    LONGS_EQUAL(1, utf8_is_valid (NULL, 0, &error));
    LONGS_EQUAL(1, utf8_is_valid (NULL, 1, &error));
    LONGS_EQUAL(1, utf8_is_valid ("", -1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("", 0, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("", 1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("", -1, &error));
    LONGS_EQUAL(1, utf8_is_valid ("", 0, &error));
    LONGS_EQUAL(1, utf8_is_valid ("", 1, &error));
    LONGS_EQUAL(1, utf8_is_valid ("abc", -1, &error));
    POINTERS_EQUAL(NULL, error);
    LONGS_EQUAL(1, utf8_is_valid ("abc", 0, &error));
    POINTERS_EQUAL(NULL, error);
    LONGS_EQUAL(1, utf8_is_valid ("abc", 1, &error));
    POINTERS_EQUAL(NULL, error);
    LONGS_EQUAL(1, utf8_is_valid (noel_valid, -1, &error));
    POINTERS_EQUAL(NULL, error);
    LONGS_EQUAL(1, utf8_is_valid (noel_valid, 0, &error));
    POINTERS_EQUAL(NULL, error);
    LONGS_EQUAL(1, utf8_is_valid (noel_valid, 1, &error));
    POINTERS_EQUAL(NULL, error);
    LONGS_EQUAL(0, utf8_is_valid (utf_4bytes_invalid, -1, &error));
    POINTERS_EQUAL(utf_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (utf_4bytes_invalid, 0, &error));
    POINTERS_EQUAL(utf_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (utf_4bytes_invalid, 1, &error));
    POINTERS_EQUAL(utf_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (utf_4bytes_invalid, 2, &error));
    POINTERS_EQUAL(utf_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (utf_4bytes_invalid, 3, &error));
    POINTERS_EQUAL(utf_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (utf_4bytes_invalid, 4, &error));
    POINTERS_EQUAL(utf_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (noel_invalid, -1, &error));
    POINTERS_EQUAL(noel_invalid + 2, error);
    LONGS_EQUAL(0, utf8_is_valid (noel_invalid, 0, &error));
    POINTERS_EQUAL(noel_invalid + 2, error);
    LONGS_EQUAL(1, utf8_is_valid (noel_invalid, 1, &error));
    POINTERS_EQUAL(NULL, error);
    LONGS_EQUAL(1, utf8_is_valid (noel_invalid, 2, &error));
    POINTERS_EQUAL(NULL, error);
    LONGS_EQUAL(0, utf8_is_valid (noel_invalid, 3, &error));
    POINTERS_EQUAL(noel_invalid + 2, error);
    LONGS_EQUAL(0, utf8_is_valid (noel_invalid, 4, &error));
    POINTERS_EQUAL(noel_invalid + 2, error);
    LONGS_EQUAL(0, utf8_is_valid (noel_invalid, 5, &error));
    POINTERS_EQUAL(noel_invalid + 2, error);

    /* 2 bytes: code point must be in range U+0080-07FF */

    /* U+0 */
    LONGS_EQUAL(0, utf8_is_valid ("\xc0\x80", -1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xc0\x80", 0, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xc0\x80", 1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xc0\x80", 2, NULL));

    /* U+7F */
    LONGS_EQUAL(0, utf8_is_valid ("\xc1\xbf", -1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xc1\xbf", 0, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xc1\xbf", 1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xc1\xbf", 2, NULL));

    /* U+80 */
    LONGS_EQUAL(1, utf8_is_valid ("\xc2\x80", -1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xc2\x80", 0, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xc2\x80", 1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xc2\x80", 2, NULL));

    /* U+7FF */
    LONGS_EQUAL(1, utf8_is_valid ("\xdf\xbf", -1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xdf\xbf", 0, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xdf\xbf", 1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xdf\xbf", 2, NULL));

    /* 3 bytes: code point must be in range: U+0800-FFFF */

    /* U+0 */
    LONGS_EQUAL(0, utf8_is_valid ("\xe0\x80\x80", -1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xe0\x80\x80", 0, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xe0\x80\x80", 1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xe0\x80\x80", 2, NULL));

    /* U+7FF  */
    LONGS_EQUAL(0, utf8_is_valid ("\xe0\x9f\xbf", -1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xe0\x9f\xbf", 0, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xe0\x9f\xbf", 1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xe0\x9f\xbf", 2, NULL));

    /* U+D800 */
    LONGS_EQUAL(0, utf8_is_valid ("\xed\xa0\x80", -1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xed\xa0\x80", 0, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xed\xa0\x80", 1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xed\xa0\x80", 2, NULL));

    /* U+DFFF */
    LONGS_EQUAL(0, utf8_is_valid ("\xed\xbf\xbf", -1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xed\xbf\xbf", 0, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xed\xbf\xbf", 1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xed\xbf\xbf", 2, NULL));

    /* U+800  */
    LONGS_EQUAL(1, utf8_is_valid ("\xe0\xa0\x80", -1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xe0\xa0\x80", 0, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xe0\xa0\x80", 1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xe0\xa0\x80", 2, NULL));

    /* U+D7FF */
    LONGS_EQUAL(1, utf8_is_valid ("\xed\x9f\xbf", -1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xed\x9f\xbf", 0, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xed\x9f\xbf", 1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xed\x9f\xbf", 2, NULL));

    /* U+E000 */
    LONGS_EQUAL(1, utf8_is_valid ("\xe7\x80\x80", -1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xe7\x80\x80", 0, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xe7\x80\x80", 1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xe7\x80\x80", 2, NULL));

    /* U+FFFF */
    LONGS_EQUAL(1, utf8_is_valid ("\xef\xbf\xbf", -1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xef\xbf\xbf", 0, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xef\xbf\xbf", 1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xef\xbf\xbf", 2, NULL));

    /* 4 bytes: code point must be in range: U+10000-1FFFFF */

    /* U+0 */
    LONGS_EQUAL(0, utf8_is_valid ("\xf0\x80\x80\x80", -1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xf0\x80\x80\x80", 0, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xf0\x80\x80\x80", 1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xf0\x80\x80\x80", 2, NULL));

    /* U+FFFF   */
    LONGS_EQUAL(0, utf8_is_valid ("\xf0\x8f\xbf\xbf", -1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xf0\x8f\xbf\xbf", 0, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xf0\x8f\xbf\xbf", 1, NULL));
    LONGS_EQUAL(0, utf8_is_valid ("\xf0\x8f\xbf\xbf", 2, NULL));

    /* U+10000  */
    LONGS_EQUAL(1, utf8_is_valid ("\xf0\x90\x80\x80", -1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xf0\x90\x80\x80", 0, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xf0\x90\x80\x80", 1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xf0\x90\x80\x80", 2, NULL));

    /* U+1FFFFF */
    LONGS_EQUAL(1, utf8_is_valid ("\xf7\xbf\xbf\xbf", -1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xf7\xbf\xbf\xbf", 0, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xf7\xbf\xbf\xbf", 1, NULL));
    LONGS_EQUAL(1, utf8_is_valid ("\xf7\xbf\xbf\xbf", 2, NULL));
}

/*
 * Tests functions:
 *   utf8_normalize
 */

TEST(CoreUtf8, Normalize)
{
    char *str;

    str = strdup (noel_invalid);
    utf8_normalize (NULL, '?');
    utf8_normalize (str, '?');
    STRCMP_EQUAL(noel_invalid_norm, str);
    free (str);

    str = strdup (noel_invalid2);
    utf8_normalize (str, '?');
    STRCMP_EQUAL(noel_invalid2_norm, str);
    free (str);
}

/*
 * Tests functions:
 *   utf8_prev_char
 *   utf8_next_char
 *   utf8_add_offset
 *   utf8_real_pos
 *   utf8_pos
 */

TEST(CoreUtf8, Move)
{
    const char *ptr;

    /* previous/next char */
    POINTERS_EQUAL(NULL, utf8_prev_char (NULL, NULL));
    POINTERS_EQUAL(NULL, utf8_next_char (NULL));
    POINTERS_EQUAL(NULL, utf8_prev_char (empty_string, empty_string));
    POINTERS_EQUAL(empty_string + 1, utf8_next_char (empty_string));
    POINTERS_EQUAL(NULL, utf8_prev_char (noel_valid + 1, noel_valid));
    ptr = utf8_next_char (noel_valid);
    STRCMP_EQUAL("oël", ptr);
    ptr = utf8_next_char (ptr);
    STRCMP_EQUAL("ël", ptr);
    ptr = utf8_next_char (ptr);
    STRCMP_EQUAL("l", ptr);
    ptr = utf8_prev_char (noel_valid, ptr);
    STRCMP_EQUAL("ël", ptr);
    ptr = utf8_prev_char (noel_valid, ptr);
    STRCMP_EQUAL("oël", ptr);
    ptr = utf8_prev_char (noel_valid, ptr);
    STRCMP_EQUAL("noël", ptr);
    POINTERS_EQUAL(noel_valid, ptr);
    ptr = utf8_prev_char (han_char, han_char + 4);
    POINTERS_EQUAL(han_char, ptr);
    ptr = utf8_prev_char (noel_valid + 3, noel_valid + 4);
    POINTERS_EQUAL(noel_valid + 3, ptr);
    POINTERS_EQUAL(utf_2bytes_truncated_1 + 1,
                   utf8_next_char (utf_2bytes_truncated_1));
    POINTERS_EQUAL(utf_3bytes_truncated_1 + 1,
                   utf8_next_char (utf_3bytes_truncated_1));
    POINTERS_EQUAL(utf_3bytes_truncated_2 + 2,
                   utf8_next_char (utf_3bytes_truncated_2));
    POINTERS_EQUAL(utf_4bytes_truncated_1 + 1,
                   utf8_next_char (utf_4bytes_truncated_1));
    POINTERS_EQUAL(utf_4bytes_truncated_2 + 2,
                   utf8_next_char (utf_4bytes_truncated_2));
    POINTERS_EQUAL(utf_4bytes_truncated_3 + 3,
                   utf8_next_char (utf_4bytes_truncated_3));

    /* add offset */
    POINTERS_EQUAL(NULL, utf8_add_offset (NULL, 0));
    ptr = utf8_add_offset (noel_valid, 0);
    STRCMP_EQUAL(noel_valid, ptr);
    ptr = utf8_add_offset (noel_valid, 1);
    STRCMP_EQUAL("oël", ptr);
    ptr = utf8_add_offset (noel_valid, 3);
    STRCMP_EQUAL("l", ptr);

    /* real position */
    LONGS_EQUAL(-1, utf8_real_pos (NULL, -1));
    LONGS_EQUAL(0, utf8_real_pos (NULL, 0));
    LONGS_EQUAL(0, utf8_real_pos (noel_valid, -1));
    LONGS_EQUAL(0, utf8_real_pos (noel_valid, 0));
    LONGS_EQUAL(1, utf8_real_pos (noel_valid, 1));
    LONGS_EQUAL(2, utf8_real_pos (noel_valid, 2));
    LONGS_EQUAL(4, utf8_real_pos (noel_valid, 3));

    /* position */
    LONGS_EQUAL(-1, utf8_pos (NULL, -1));
    LONGS_EQUAL(0, utf8_pos (NULL, 0));
    LONGS_EQUAL(0, utf8_pos (noel_valid, -1));
    LONGS_EQUAL(0, utf8_pos (noel_valid, 0));
    LONGS_EQUAL(1, utf8_pos (noel_valid, 1));
    LONGS_EQUAL(2, utf8_pos (noel_valid, 2));
    LONGS_EQUAL(3, utf8_pos (noel_valid, 4));
}

/*
 * Tests functions:
 *   utf8_char_int
 *   utf8_int_string
 *   utf8_wide_char
 */

TEST(CoreUtf8, Convert)
{
    char result[5];

    /* get UTF-8 char as integer */
    LONGS_EQUAL(0, utf8_char_int (NULL));
    LONGS_EQUAL(0, utf8_char_int (""));
    LONGS_EQUAL(65, utf8_char_int ("ABC"));
    LONGS_EQUAL(235, utf8_char_int ("ë"));
    LONGS_EQUAL(0x20ac, utf8_char_int ("€"));
    LONGS_EQUAL(0x2ee9, utf8_char_int (cjk_yellow));
    LONGS_EQUAL(0x24b62, utf8_char_int (han_char));

    LONGS_EQUAL(0x0, utf8_char_int ("\xc0\x80"));   /* invalid */
    LONGS_EQUAL(0x7f, utf8_char_int ("\xc1\xbf"));  /* invalid */
    LONGS_EQUAL(0x80, utf8_char_int ("\xc2\x80"));
    LONGS_EQUAL(0x7ff, utf8_char_int ("\xdf\xbf"));

    LONGS_EQUAL(0x0, utf8_char_int ("\xe0\x80\x80"));     /* invalid */
    LONGS_EQUAL(0x7ff, utf8_char_int ("\xe0\x9f\xbf"));   /* invalid */
    LONGS_EQUAL(0xd800, utf8_char_int ("\xed\xa0\x80"));  /* invalid */
    LONGS_EQUAL(0xdfff, utf8_char_int ("\xed\xbf\xbf"));  /* invalid */
    LONGS_EQUAL(0x800, utf8_char_int ("\xe0\xa0\x80"));
    LONGS_EQUAL(0xd7ff, utf8_char_int ("\xed\x9f\xbf"));
    LONGS_EQUAL(0x7000, utf8_char_int ("\xe7\x80\x80"));
    LONGS_EQUAL(0xffff, utf8_char_int ("\xef\xbf\xbf"));

    LONGS_EQUAL(0x0, utf8_char_int ("\xf0\x80\x80\x80"));     /* invalid */
    LONGS_EQUAL(0xffff, utf8_char_int ("\xf0\x8f\xbf\xbf"));  /* invalid */
    LONGS_EQUAL(0x10000, utf8_char_int ("\xf0\x90\x80\x80"));
    LONGS_EQUAL(0x1fffff, utf8_char_int ("\xf7\xbf\xbf\xbf"));

    LONGS_EQUAL(0x0, utf8_char_int (utf_2bytes_truncated_1));
    LONGS_EQUAL(0x02, utf8_char_int (utf_3bytes_truncated_1));
    LONGS_EQUAL(0xbb, utf8_char_int (utf_3bytes_truncated_2));
    LONGS_EQUAL(0x0, utf8_char_int (utf_4bytes_truncated_1));
    LONGS_EQUAL(0x24, utf8_char_int (utf_4bytes_truncated_2));
    LONGS_EQUAL(0x92d, utf8_char_int (utf_4bytes_truncated_3));

    /* convert unicode char to a string */
    utf8_int_string (0, NULL);
    utf8_int_string (0, result);
    STRCMP_EQUAL("", result);
    utf8_int_string (235, result);
    STRCMP_EQUAL("ë", result);
    utf8_int_string (0x20ac, result);
    STRCMP_EQUAL("€", result);
    utf8_int_string (0x2ee9, result);
    STRCMP_EQUAL(cjk_yellow, result);
    utf8_int_string (0x24b62, result);
    STRCMP_EQUAL(han_char, result);

    /* get wide char */
    LONGS_EQUAL(WEOF, utf8_wide_char (NULL));
    LONGS_EQUAL(WEOF, utf8_wide_char (""));
    LONGS_EQUAL(65, utf8_wide_char ("A"));
    LONGS_EQUAL(0xc3ab, utf8_wide_char ("ë"));
    LONGS_EQUAL(0xe282ac, utf8_wide_char ("€"));
    LONGS_EQUAL(0xe2bba9, utf8_wide_char (cjk_yellow));
    LONGS_EQUAL(0xf0a4ada2, utf8_wide_char (han_char));
}

/*
 * Tests functions:
 *   utf8_char_size
 *   utf8_char_size_screen
 *   utf8_strlen
 *   utf8_strnlen
 *   utf8_strlen_screen
 */

TEST(CoreUtf8, Size)
{
    /* char size */
    LONGS_EQUAL(0, utf8_char_size (NULL));
    LONGS_EQUAL(1, utf8_char_size (""));
    LONGS_EQUAL(1, utf8_char_size ("A"));
    LONGS_EQUAL(2, utf8_char_size ("ë"));
    LONGS_EQUAL(3, utf8_char_size ("€"));
    LONGS_EQUAL(3, utf8_char_size (cjk_yellow));
    LONGS_EQUAL(4, utf8_char_size (han_char));
    /* ë as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(1, utf8_char_size ("\xeb"));
    /* ël as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(2, utf8_char_size ("\xebl"));
    /* ëlm as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(3, utf8_char_size ("\xeblm"));
    /* ëlmn as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(3, utf8_char_size ("\xeblmn"));

    /* char size on screen */
    LONGS_EQUAL(0, utf8_char_size_screen (NULL));
    LONGS_EQUAL(0, utf8_char_size_screen (""));
    LONGS_EQUAL(1, utf8_char_size_screen ("A"));
    LONGS_EQUAL(1, utf8_char_size_screen ("ë"));
    LONGS_EQUAL(1, utf8_char_size_screen ("€"));
    LONGS_EQUAL(2, utf8_char_size_screen (cjk_yellow));
    /* ë as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(1, utf8_char_size_screen ("\xeb"));
    /* ël as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(1, utf8_char_size_screen ("\xebl"));
    /* ëlm as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(1, utf8_char_size_screen ("\xeblm"));
    /* ëlmn as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(1, utf8_char_size_screen ("\xeblmn"));

    /* length of string (in chars) */
    LONGS_EQUAL(0, utf8_strlen (NULL));
    LONGS_EQUAL(0, utf8_strlen (""));
    LONGS_EQUAL(1, utf8_strlen ("A"));
    LONGS_EQUAL(1, utf8_strlen ("ë"));
    LONGS_EQUAL(1, utf8_strlen ("€"));
    LONGS_EQUAL(1, utf8_strlen (cjk_yellow));
    LONGS_EQUAL(1, utf8_strlen (han_char));

    /* length of string (in chars, for max N bytes) */
    LONGS_EQUAL(0, utf8_strnlen (NULL, 0));
    LONGS_EQUAL(0, utf8_strnlen ("", 0));
    LONGS_EQUAL(1, utf8_strnlen ("AZ", 1));
    LONGS_EQUAL(1, utf8_strnlen ("ëZ", 2));
    LONGS_EQUAL(1, utf8_strnlen ("€Z", 3));
    LONGS_EQUAL(1, utf8_strnlen (han_char_z, 4));

    /* length of string on screen (in chars) */
    LONGS_EQUAL(0, utf8_strlen_screen (NULL));
    LONGS_EQUAL(0, utf8_strlen_screen (""));
    LONGS_EQUAL(1, utf8_strlen_screen ("A"));
    LONGS_EQUAL(1, utf8_strlen_screen ("ë"));
    LONGS_EQUAL(1, utf8_strlen_screen ("€"));
    LONGS_EQUAL(1, utf8_strlen_screen ("\x7f"));
    LONGS_EQUAL(2, utf8_strlen_screen (cjk_yellow));

    /* length of Tabulation */
    LONGS_EQUAL(1, utf8_strlen_screen ("\t"));
    config_file_option_set (config_look_tab_width, "4", 1);
    LONGS_EQUAL(4, utf8_strlen_screen ("\t"));
    config_file_option_set (config_look_tab_width, "8", 1);
    LONGS_EQUAL(8, utf8_strlen_screen ("\t"));
    config_file_option_reset (config_look_tab_width, 1);
}

/*
 * Tests functions:
 *   utf8_charcmp
 *   utf8_charcasecmp
 *   utf8_charcasecmp_range
 */

TEST(CoreUtf8, Comparison)
{
    /* case-sensitive comparison */
    LONGS_EQUAL(0, utf8_charcmp (NULL, NULL));
    LONGS_EQUAL(-1, utf8_charcmp (NULL, "abc"));
    LONGS_EQUAL(1, utf8_charcmp ("abc", NULL));
    LONGS_EQUAL(0, utf8_charcmp ("axx", "azz"));
    LONGS_EQUAL(-1, utf8_charcmp ("A", "Z"));
    LONGS_EQUAL(1, utf8_charcmp ("Z", "A"));
    LONGS_EQUAL(-1, utf8_charcmp ("A", "a"));
    LONGS_EQUAL(-1, utf8_charcmp ("ë", "€"));
    LONGS_EQUAL(1, utf8_charcmp ("ë", ""));
    LONGS_EQUAL(-1, utf8_charcmp ("", "ë"));

    /* case-insensitive comparison */
    LONGS_EQUAL(0, utf8_charcasecmp (NULL, NULL));
    LONGS_EQUAL(-1, utf8_charcasecmp (NULL, "abc"));
    LONGS_EQUAL(1, utf8_charcasecmp ("abc", NULL));
    LONGS_EQUAL(0, utf8_charcasecmp ("axx", "azz"));
    LONGS_EQUAL(-1, utf8_charcasecmp ("A", "Z"));
    LONGS_EQUAL(1, utf8_charcasecmp ("Z", "A"));
    LONGS_EQUAL(0, utf8_charcasecmp ("A", "a"));
    LONGS_EQUAL(-1, utf8_charcasecmp ("ë", "€"));

    /* case-insensitive comparison with a range */
    LONGS_EQUAL(0, utf8_charcasecmp_range (NULL, NULL, 30));
    LONGS_EQUAL(-1, utf8_charcasecmp_range (NULL, "abc", 30));
    LONGS_EQUAL(1, utf8_charcasecmp_range ("abc", NULL, 30));
    LONGS_EQUAL(0, utf8_charcasecmp_range ("axx", "azz", 30));
    LONGS_EQUAL(-1, utf8_charcasecmp_range ("A", "Z", 30));
    LONGS_EQUAL(1, utf8_charcasecmp_range ("Z", "A", 30));
    LONGS_EQUAL(0, utf8_charcasecmp_range ("A", "a", 30));
    LONGS_EQUAL(-1, utf8_charcasecmp_range ("ë", "€", 30));
    LONGS_EQUAL(0, utf8_charcasecmp_range ("[", "{", 30));
    LONGS_EQUAL(0, utf8_charcasecmp_range ("]", "}", 30));
    LONGS_EQUAL(0, utf8_charcasecmp_range ("\\", "|", 30));
    LONGS_EQUAL(0, utf8_charcasecmp_range ("^", "~", 30));
    LONGS_EQUAL(-1, utf8_charcasecmp_range ("[", "{", 26));
    LONGS_EQUAL(-1, utf8_charcasecmp_range ("]", "}", 26));
    LONGS_EQUAL(-1, utf8_charcasecmp_range ("\\", "|", 26));
    LONGS_EQUAL(-1, utf8_charcasecmp_range ("^", "~", 26));
}

/*
 * Tests functions:
 *   utf8_strndup
 */

TEST(CoreUtf8, Duplicate)
{
    char *str;

    WEE_TEST_STR(NULL, utf8_strndup (NULL, 0));
    WEE_TEST_STR("", utf8_strndup (noel_valid, 0));
    WEE_TEST_STR("n", utf8_strndup (noel_valid, 1));
    WEE_TEST_STR("no", utf8_strndup (noel_valid, 2));
    WEE_TEST_STR("noë", utf8_strndup (noel_valid, 3));
    WEE_TEST_STR("noël", utf8_strndup (noel_valid, 4));
    WEE_TEST_STR("noël", utf8_strndup (noel_valid, 5));
}
