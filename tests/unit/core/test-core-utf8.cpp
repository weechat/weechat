/*
 * test-core-utf8.cpp - test UTF-8 string functions
 *
 * Copyright (C) 2014-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/core-utf8.h"
#include "src/core/core-config.h"
}

#define TEST_STRNCPY(__result, __dest, __string, __length)              \
    if (__dest != NULL)                                                 \
    {                                                                   \
        dest[0] = '\x01';                                               \
        dest[1] = '\0';                                                 \
    }                                                                   \
    utf8_strncpy (__dest, __string, __length);                          \
    if (__dest != NULL)                                                 \
    {                                                                   \
        STRCMP_EQUAL(__result, __dest);                                 \
    }

/*
 * delete:
 *   []
 *   U+007F (127)
 *   UTF-8: 1 byte = 0x7F
 */
#define UNICODE_DELETE "\u007f"

/*
 * next line:
 *   []
 *   U+0085 (133)
 *   UTF-8: 2 bytes = 0xC2 0x85
 */
#define UNICODE_NEXT_LINE "\u0085"

/*
 * private use two:
 *   []
 *   U+0092 (146)
 *   UTF-8: 2 bytes = 0xC2 0X92
 */
#define UNICODE_PRIVATE_USE_TWO "\u0092"

/*
 * soft hyphen:
 *   [­]
 *   U+00AD (173)
 *   UTF-8: 2 bytes = 0xC2 0xAD
 */
#define UNICODE_SOFT_HYPHEN "\u00ad"

/* zero width space:
 *   [​]
 *   U+200B (8203)
 *   UTF-8: 3 bytes = 0xE2 0x80 0x8B
 */
#define UNICODE_ZERO_WIDTH_SPACE "\u200b"

/* snowman without snow:
 *   [⛄]
 *   U+26C4 (9924)
 *   UTF-8: 3 bytes = 0xE2 0x9B 0x84
 */
#define UNICODE_SNOWMAN "\u26c4"

/* cjk yellow:
 *   [⻩]
 *   U+2EE9 (12009)
 *   UTF-8: 3 bytes = 0xE2 0xBB 0xA9
 */
#define UNICODE_CJK_YELLOW "\u2ee9"

/* han char:
 *  [𤭢]
 *  U+24B62 (150370)
 *  UTF-8: 4 bytes = 0xF0 0xA4 0xAD 0xA2
 */
#define UNICODE_HAN_CHAR "\U00024b62"

/* various invalid or incomplete UTF-8 sequences */
#define UTF8_4BYTES_INVALID      "\xf0\x03\x02\x01"
#define UTF8_2BYTES_TRUNCATED_1  "\xc0"
#define UTF8_3BYTES_TRUNCATED_1  "\xe2"
#define UTF8_3BYTES_TRUNCATED_2  "\xe2\xbb"
#define UTF8_4BYTES_TRUNCATED_1  "\xf0"
#define UTF8_4BYTES_TRUNCATED_2  "\xf0\xa4"
#define UTF8_4BYTES_TRUNCATED_3  "\xf0\xa4\xad"

/* "noël" */
#define UTF8_NOEL_VALID            "no\xc3\xabl"
#define UTF8_NOEL_VALID_MULTILINE  "no\xc3\xabl\nno\xc3\xabl"
#define UTF8_NOEL_INVALID          "no\xc3l"
#define UTF8_NOEL_INVALID2         "no\xff\xffl"
#define UTF8_NOEL_INVALID_NORM     "no?l"
#define UTF8_NOEL_INVALID2_NORM    "no??l"

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
    const char *noel_valid = UTF8_NOEL_VALID;
    const char *noel_invalid = UTF8_NOEL_INVALID;
    const char *utf8_4bytes_invalid = UTF8_4BYTES_INVALID;
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
    LONGS_EQUAL(0, utf8_is_valid (utf8_4bytes_invalid, -1, &error));
    POINTERS_EQUAL(utf8_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (utf8_4bytes_invalid, 0, &error));
    POINTERS_EQUAL(utf8_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (utf8_4bytes_invalid, 1, &error));
    POINTERS_EQUAL(utf8_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (utf8_4bytes_invalid, 2, &error));
    POINTERS_EQUAL(utf8_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (utf8_4bytes_invalid, 3, &error));
    POINTERS_EQUAL(utf8_4bytes_invalid, error);
    LONGS_EQUAL(0, utf8_is_valid (utf8_4bytes_invalid, 4, &error));
    POINTERS_EQUAL(utf8_4bytes_invalid, error);
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

    str = strdup (UTF8_NOEL_INVALID);
    utf8_normalize (NULL, '?');
    utf8_normalize (str, '?');
    STRCMP_EQUAL(UTF8_NOEL_INVALID_NORM, str);
    free (str);

    str = strdup (UTF8_NOEL_INVALID2);
    utf8_normalize (str, '?');
    STRCMP_EQUAL(UTF8_NOEL_INVALID2_NORM, str);
    free (str);
}

/*
 * Tests functions:
 *   utf8_prev_char
 *   utf8_next_char
 *   utf8_beginning_of_line
 *   utf8_end_of_line
 *   utf8_add_offset
 *   utf8_real_pos
 *   utf8_pos
 */

TEST(CoreUtf8, Move)
{
    const char *ptr;
    const char *empty_string = "";
    const char *noel_valid = UTF8_NOEL_VALID;
    const char *noel_valid_multiline = UTF8_NOEL_VALID_MULTILINE;
    const char *utf8_2bytes_truncated_1 = UTF8_2BYTES_TRUNCATED_1;
    const char *utf8_3bytes_truncated_1 = UTF8_3BYTES_TRUNCATED_1;
    const char *utf8_3bytes_truncated_2 = UTF8_3BYTES_TRUNCATED_2;
    const char *utf8_4bytes_truncated_1 = UTF8_4BYTES_TRUNCATED_1;
    const char *utf8_4bytes_truncated_2 = UTF8_4BYTES_TRUNCATED_2;
    const char *utf8_4bytes_truncated_3 = UTF8_4BYTES_TRUNCATED_3;
    const char *han_char = UNICODE_HAN_CHAR;

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
    POINTERS_EQUAL(utf8_2bytes_truncated_1 + 1,
                   utf8_next_char (utf8_2bytes_truncated_1));
    POINTERS_EQUAL(utf8_3bytes_truncated_1 + 1,
                   utf8_next_char (utf8_3bytes_truncated_1));
    POINTERS_EQUAL(utf8_3bytes_truncated_2 + 2,
                   utf8_next_char (utf8_3bytes_truncated_2));
    POINTERS_EQUAL(utf8_4bytes_truncated_1 + 1,
                   utf8_next_char (utf8_4bytes_truncated_1));
    POINTERS_EQUAL(utf8_4bytes_truncated_2 + 2,
                   utf8_next_char (utf8_4bytes_truncated_2));
    POINTERS_EQUAL(utf8_4bytes_truncated_3 + 3,
                   utf8_next_char (utf8_4bytes_truncated_3));

    /* beginning/end of line */
    POINTERS_EQUAL(NULL, utf8_beginning_of_line (NULL, NULL));
    POINTERS_EQUAL(NULL, utf8_end_of_line (NULL));
    ptr = utf8_end_of_line (noel_valid_multiline);
    STRCMP_EQUAL("\nnoël", ptr);
    ptr = utf8_end_of_line (ptr);
    STRCMP_EQUAL("\nnoël", ptr);
    ptr = utf8_next_char (ptr);
    ptr = utf8_end_of_line (ptr);
    STRCMP_EQUAL("", ptr);
    ptr = utf8_end_of_line (ptr);
    STRCMP_EQUAL("", ptr);
    ptr = utf8_beginning_of_line (noel_valid_multiline, ptr);
    STRCMP_EQUAL(noel_valid, ptr);
    ptr = utf8_beginning_of_line (noel_valid_multiline, ptr);
    STRCMP_EQUAL(noel_valid, ptr);
    ptr = utf8_prev_char (noel_valid_multiline, ptr);
    ptr = utf8_beginning_of_line (noel_valid_multiline, ptr);
    STRCMP_EQUAL(noel_valid_multiline, ptr);
    ptr = utf8_beginning_of_line (noel_valid_multiline, ptr);
    STRCMP_EQUAL(noel_valid_multiline, ptr);

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
    const char *utf8_2bytes_truncated_1 = UTF8_2BYTES_TRUNCATED_1;
    const char *utf8_3bytes_truncated_1 = UTF8_3BYTES_TRUNCATED_1;
    const char *utf8_3bytes_truncated_2 = UTF8_3BYTES_TRUNCATED_2;
    const char *utf8_4bytes_truncated_1 = UTF8_4BYTES_TRUNCATED_1;
    const char *utf8_4bytes_truncated_2 = UTF8_4BYTES_TRUNCATED_2;
    const char *utf8_4bytes_truncated_3 = UTF8_4BYTES_TRUNCATED_3;
    char result[5];

    /* get UTF-8 char as integer */
    LONGS_EQUAL(0, utf8_char_int (NULL));
    LONGS_EQUAL(0, utf8_char_int (""));
    LONGS_EQUAL(65, utf8_char_int ("ABC"));
    LONGS_EQUAL(235, utf8_char_int ("ë"));
    LONGS_EQUAL(0x20ac, utf8_char_int ("€"));
    LONGS_EQUAL(0x2ee9, utf8_char_int (UNICODE_CJK_YELLOW));
    LONGS_EQUAL(0x24b62, utf8_char_int (UNICODE_HAN_CHAR));

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

    LONGS_EQUAL(0x0, utf8_char_int (utf8_2bytes_truncated_1));
    LONGS_EQUAL(0x02, utf8_char_int (utf8_3bytes_truncated_1));
    LONGS_EQUAL(0xbb, utf8_char_int (utf8_3bytes_truncated_2));
    LONGS_EQUAL(0x0, utf8_char_int (utf8_4bytes_truncated_1));
    LONGS_EQUAL(0x24, utf8_char_int (utf8_4bytes_truncated_2));
    LONGS_EQUAL(0x92d, utf8_char_int (utf8_4bytes_truncated_3));

    /* convert unicode char to a string */
    LONGS_EQUAL(0, utf8_int_string (0, NULL));
    LONGS_EQUAL(0, utf8_int_string (0, result));
    STRCMP_EQUAL("", result);
    LONGS_EQUAL(2, utf8_int_string (L'ë', result));
    STRCMP_EQUAL("ë", result);
    LONGS_EQUAL(3, utf8_int_string (L'€', result));
    STRCMP_EQUAL("€", result);
    LONGS_EQUAL(3, utf8_int_string (0x2ee9, result));
    STRCMP_EQUAL(UNICODE_CJK_YELLOW, result);
    LONGS_EQUAL(4, utf8_int_string (0x24b62, result));
    STRCMP_EQUAL(UNICODE_HAN_CHAR, result);
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
    /* char size (in bytes) */
    LONGS_EQUAL(0, utf8_char_size (NULL));
    LONGS_EQUAL(1, utf8_char_size (""));
    LONGS_EQUAL(1, utf8_char_size ("A"));
    LONGS_EQUAL(2, utf8_char_size ("ë"));
    LONGS_EQUAL(3, utf8_char_size ("€"));
    LONGS_EQUAL(1, utf8_char_size ("\x01"));
    LONGS_EQUAL(1, utf8_char_size (UNICODE_DELETE));
    LONGS_EQUAL(2, utf8_char_size (UNICODE_NEXT_LINE));
    LONGS_EQUAL(2, utf8_char_size (UNICODE_PRIVATE_USE_TWO));
    LONGS_EQUAL(2, utf8_char_size (UNICODE_SOFT_HYPHEN));
    LONGS_EQUAL(3, utf8_char_size (UNICODE_ZERO_WIDTH_SPACE));
    LONGS_EQUAL(3, utf8_char_size (UNICODE_SNOWMAN));
    LONGS_EQUAL(3, utf8_char_size (UNICODE_CJK_YELLOW));
    LONGS_EQUAL(4, utf8_char_size (UNICODE_HAN_CHAR));
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
    LONGS_EQUAL(1, utf8_char_size_screen ("\x01"));
    LONGS_EQUAL(-1, utf8_char_size_screen (UNICODE_DELETE));
    LONGS_EQUAL(-1, utf8_char_size_screen (UNICODE_NEXT_LINE));
    LONGS_EQUAL(-1, utf8_char_size_screen (UNICODE_PRIVATE_USE_TWO));
    LONGS_EQUAL(-1, utf8_char_size_screen (UNICODE_SOFT_HYPHEN));
    LONGS_EQUAL(-1, utf8_char_size_screen (UNICODE_ZERO_WIDTH_SPACE));
    LONGS_EQUAL(2, utf8_char_size_screen (UNICODE_SNOWMAN));
    LONGS_EQUAL(2, utf8_char_size_screen (UNICODE_CJK_YELLOW));
    LONGS_EQUAL(2, utf8_char_size_screen (UNICODE_HAN_CHAR));
    /* ë as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(-1, utf8_char_size_screen ("\xeb"));
    /* ël as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(1, utf8_char_size_screen ("\xebl"));
    /* ëlm as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(2, utf8_char_size_screen ("\xeblm"));
    /* ëlmn as iso-8859-15: invalid UTF-8 */
    LONGS_EQUAL(2, utf8_char_size_screen ("\xeblmn"));

    /* length of string (in chars) */
    LONGS_EQUAL(0, utf8_strlen (NULL));
    LONGS_EQUAL(0, utf8_strlen (""));
    LONGS_EQUAL(1, utf8_strlen ("A"));
    LONGS_EQUAL(1, utf8_strlen ("ë"));
    LONGS_EQUAL(1, utf8_strlen ("€"));
    LONGS_EQUAL(1, utf8_strlen ("\x01"));
    LONGS_EQUAL(4, utf8_strlen (UTF8_NOEL_VALID));
    LONGS_EQUAL(1, utf8_strlen (UNICODE_DELETE));
    LONGS_EQUAL(1, utf8_strlen (UNICODE_NEXT_LINE));
    LONGS_EQUAL(1, utf8_strlen (UNICODE_PRIVATE_USE_TWO));
    LONGS_EQUAL(1, utf8_strlen (UNICODE_SOFT_HYPHEN));
    LONGS_EQUAL(1, utf8_strlen (UNICODE_ZERO_WIDTH_SPACE));
    LONGS_EQUAL(1, utf8_strlen (UNICODE_SNOWMAN));
    LONGS_EQUAL(1, utf8_strlen (UNICODE_CJK_YELLOW));
    LONGS_EQUAL(1, utf8_strlen (UNICODE_HAN_CHAR));

    /* length of string (in chars, for max N bytes) */
    LONGS_EQUAL(0, utf8_strnlen (NULL, 0));
    LONGS_EQUAL(0, utf8_strnlen ("", 0));
    LONGS_EQUAL(1, utf8_strnlen ("AZ", 1));
    LONGS_EQUAL(1, utf8_strnlen ("ëZ", 2));
    LONGS_EQUAL(1, utf8_strnlen ("€Z", 3));
    LONGS_EQUAL(1, utf8_strnlen (UNICODE_HAN_CHAR "Z", 4));

    /* length of string on screen (in chars) */
    LONGS_EQUAL(0, utf8_strlen_screen (NULL));
    LONGS_EQUAL(0, utf8_strlen_screen (""));
    LONGS_EQUAL(1, utf8_strlen_screen ("A"));
    LONGS_EQUAL(1, utf8_strlen_screen ("ë"));
    LONGS_EQUAL(1, utf8_strlen_screen ("€"));
    LONGS_EQUAL(1, utf8_strlen_screen ("\x01"));
    LONGS_EQUAL(4, utf8_strlen_screen (UTF8_NOEL_VALID));
    LONGS_EQUAL(4, utf8_strlen_screen ("abc\x01"));
    LONGS_EQUAL(8, utf8_strlen_screen ("a" "\x01" UTF8_NOEL_VALID "\x02" "b"));
    LONGS_EQUAL(0, utf8_strlen_screen (UNICODE_DELETE));
    LONGS_EQUAL(4, utf8_strlen_screen ("a" "\x01" UNICODE_DELETE "\x02" "b"));
    LONGS_EQUAL(0, utf8_strlen_screen (UNICODE_NEXT_LINE));
    LONGS_EQUAL(4, utf8_strlen_screen ("a" "\x01" UNICODE_NEXT_LINE "\x02" "b"));
    LONGS_EQUAL(0, utf8_strlen_screen (UNICODE_PRIVATE_USE_TWO));
    LONGS_EQUAL(4, utf8_strlen_screen ("a" "\x01" UNICODE_PRIVATE_USE_TWO "\x02" "b"));
    LONGS_EQUAL(0, utf8_strlen_screen (UNICODE_SOFT_HYPHEN));
    LONGS_EQUAL(4, utf8_strlen_screen ("a" "\x01" UNICODE_SOFT_HYPHEN "\x02" "b"));
    LONGS_EQUAL(0, utf8_strlen_screen (UNICODE_ZERO_WIDTH_SPACE));
    LONGS_EQUAL(4, utf8_strlen_screen ("a" "\x01" UNICODE_ZERO_WIDTH_SPACE "\x02" "b"));
    LONGS_EQUAL(2, utf8_strlen_screen (UNICODE_SNOWMAN));
    LONGS_EQUAL(6, utf8_strlen_screen ("a" "\x01" UNICODE_SNOWMAN "\x02" "b"));
    LONGS_EQUAL(2, utf8_strlen_screen (UNICODE_CJK_YELLOW));
    LONGS_EQUAL(6, utf8_strlen_screen ("a" "\x01" UNICODE_CJK_YELLOW "\x02" "b"));
    LONGS_EQUAL(2, utf8_strlen_screen (UNICODE_HAN_CHAR));
    LONGS_EQUAL(6, utf8_strlen_screen ("a" "\x01" UNICODE_HAN_CHAR "\x02" "b"));

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
 *   utf8_strndup
 */

TEST(CoreUtf8, Duplicate)
{
    char *str;

    WEE_TEST_STR(NULL, utf8_strndup (NULL, 0));
    WEE_TEST_STR("", utf8_strndup (UTF8_NOEL_VALID, 0));
    WEE_TEST_STR("n", utf8_strndup (UTF8_NOEL_VALID, 1));
    WEE_TEST_STR("no", utf8_strndup (UTF8_NOEL_VALID, 2));
    WEE_TEST_STR("noë", utf8_strndup (UTF8_NOEL_VALID, 3));
    WEE_TEST_STR("noël", utf8_strndup (UTF8_NOEL_VALID, 4));
    WEE_TEST_STR("noël", utf8_strndup (UTF8_NOEL_VALID, 5));
}

/*
 * Tests functions:
 *   utf8_strncpy
 */

TEST(CoreUtf8, Copy)
{
    char dest[256];

    /* invalid parameters */
    TEST_STRNCPY("", NULL, NULL, -1);
    TEST_STRNCPY("", dest, NULL, -1);
    TEST_STRNCPY("", dest, "abc", -1);

    TEST_STRNCPY("", dest, "abc", 0);
    TEST_STRNCPY("a", dest, "abc", 1);
    TEST_STRNCPY("ab", dest, "abc", 2);
    TEST_STRNCPY("abc", dest, "abc", 3);
    TEST_STRNCPY("abc", dest, "abc", 4);

    TEST_STRNCPY("", dest, UTF8_NOEL_VALID, 0);
    TEST_STRNCPY("n", dest, UTF8_NOEL_VALID, 1);
    TEST_STRNCPY("no", dest, UTF8_NOEL_VALID, 2);
    TEST_STRNCPY("noë", dest, UTF8_NOEL_VALID, 3);
    TEST_STRNCPY("noël", dest, UTF8_NOEL_VALID, 4);
    TEST_STRNCPY("noël", dest, UTF8_NOEL_VALID, 5);
}
