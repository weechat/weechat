/*
 * test-utf8.cpp - test UTF-8 string functions
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
#include <wctype.h>
#include "tests/tests.h"
#include "src/core/wee-utf8.h"
}

const char *noel_valid = "no\xc3\xabl";  /* noël */
const char *noel_invalid = "no\xc3l";
const char *noel_invalid2 = "no\xff\xffl";
const char *noel_invalid_norm = "no?l";
const char *noel_invalid2_norm = "no??l";
const char *han_char = "\xf0\xa4\xad\xa2";     /* U+24B62 */
const char *han_char_z = "\xf0\xa4\xad\xa2Z";

TEST_GROUP(Utf8)
{
};

/*
 * Tests functions:
 *   utf8_has_8bits
 *   utf8_is_valid
 */

TEST(Utf8, Validity)
{
    char *error;

    /* check 8 bits */
    LONGS_EQUAL(0, utf8_has_8bits (NULL));
    LONGS_EQUAL(0, utf8_has_8bits (""));
    LONGS_EQUAL(0, utf8_has_8bits ("abc"));
    LONGS_EQUAL(1, utf8_has_8bits ("no\xc3\xabl"));

    /* check validity */
    LONGS_EQUAL(1, utf8_is_valid (NULL, NULL));
    LONGS_EQUAL(1, utf8_is_valid (NULL, &error));
    LONGS_EQUAL(1, utf8_is_valid ("", NULL));
    LONGS_EQUAL(1, utf8_is_valid ("", &error));
    LONGS_EQUAL(1, utf8_is_valid ("abc", &error));
    POINTERS_EQUAL(NULL, error);
    LONGS_EQUAL(1, utf8_is_valid (noel_valid, &error));
    POINTERS_EQUAL(NULL, error);
    LONGS_EQUAL(0, utf8_is_valid (noel_invalid, &error));
    POINTERS_EQUAL(noel_invalid + 2, error);

    /* 2 bytes: code point must be in range U+0080-07FF */
    LONGS_EQUAL(0, utf8_is_valid ("\xc0\x80", NULL));  /* U+0   */
    LONGS_EQUAL(0, utf8_is_valid ("\xc1\xbf", NULL));  /* U+7F  */
    LONGS_EQUAL(1, utf8_is_valid ("\xc2\x80", NULL));  /* U+80  */
    LONGS_EQUAL(1, utf8_is_valid ("\xdf\xbf", NULL));  /* U+7FF */

    /* 3 bytes: code point must be in range: U+0800-FFFF */
    LONGS_EQUAL(0, utf8_is_valid ("\xe0\x80\x80", NULL));  /* U+0    */
    LONGS_EQUAL(0, utf8_is_valid ("\xe0\x9f\xbf", NULL));  /* U+7FF  */
    LONGS_EQUAL(0, utf8_is_valid ("\xed\xa0\x80", NULL));  /* U+D800 */
    LONGS_EQUAL(0, utf8_is_valid ("\xed\xbf\xbf", NULL));  /* U+DFFF */
    LONGS_EQUAL(1, utf8_is_valid ("\xe0\xa0\x80", NULL));  /* U+800  */
    LONGS_EQUAL(1, utf8_is_valid ("\xed\x9f\xbf", NULL));  /* U+D7FF */
    LONGS_EQUAL(1, utf8_is_valid ("\xe7\x80\x80", NULL));  /* U+E000 */
    LONGS_EQUAL(1, utf8_is_valid ("\xef\xbf\xbf", NULL));  /* U+FFFF */

    /* 4 bytes: code point must be in range: U+10000-1FFFFF */
    LONGS_EQUAL(0, utf8_is_valid ("\xf0\x80\x80\x80", NULL));  /* U+0      */
    LONGS_EQUAL(0, utf8_is_valid ("\xf0\x8f\xbf\xbf", NULL));  /* U+FFFF   */
    LONGS_EQUAL(1, utf8_is_valid ("\xf0\x90\x80\x80", NULL));  /* U+10000  */
    LONGS_EQUAL(1, utf8_is_valid ("\xf7\xbf\xbf\xbf", NULL));  /* U+1FFFFF */
}

/*
 * Tests functions:
 *   utf8_normalize
 */

TEST(Utf8, Normalize)
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

TEST(Utf8, Move)
{
    char *ptr;

    /* previous/next char */
    POINTERS_EQUAL(NULL, utf8_prev_char (NULL, NULL));
    POINTERS_EQUAL(NULL, utf8_next_char (NULL));
    ptr = utf8_next_char (noel_valid);
    STRCMP_EQUAL("oël", ptr);
    ptr = utf8_next_char (ptr);
    STRCMP_EQUAL("ël", ptr);
    ptr = utf8_next_char (ptr);
    STRCMP_EQUAL("l", ptr);
    ptr = utf8_prev_char (noel_valid, ptr);
    ptr = utf8_prev_char (noel_valid, ptr);
    ptr = utf8_prev_char (noel_valid, ptr);
    STRCMP_EQUAL("noël", ptr);
    POINTERS_EQUAL(noel_valid, ptr);

    /* add offset */
    ptr = utf8_add_offset (noel_valid, 0);
    STRCMP_EQUAL(noel_valid, ptr);
    ptr = utf8_add_offset (noel_valid, 1);
    STRCMP_EQUAL("oël", ptr);
    ptr = utf8_add_offset (noel_valid, 3);
    STRCMP_EQUAL("l", ptr);

    /* real position */
    LONGS_EQUAL(0, utf8_real_pos (noel_valid, 0));
    LONGS_EQUAL(1, utf8_real_pos (noel_valid, 1));
    LONGS_EQUAL(2, utf8_real_pos (noel_valid, 2));
    LONGS_EQUAL(4, utf8_real_pos (noel_valid, 3));

    /* position */
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

TEST(Utf8, Convert)
{
    char result[5];

    /* get UTF-8 char as integer */
    BYTES_EQUAL(0, utf8_char_int (NULL));
    BYTES_EQUAL(0, utf8_char_int (""));
    BYTES_EQUAL(65, utf8_char_int ("ABC"));
    BYTES_EQUAL(235, utf8_char_int ("ë"));
    BYTES_EQUAL(0x20ac, utf8_char_int ("€"));
    BYTES_EQUAL(0x24b62, utf8_char_int (han_char));

    BYTES_EQUAL(0x0, utf8_char_int ("\xc0\x80"));   /* invalid */
    BYTES_EQUAL(0x7f, utf8_char_int ("\xc1\xbf"));  /* invalid */
    BYTES_EQUAL(0x80, utf8_char_int ("\xc2\x80"));
    BYTES_EQUAL(0x7ff, utf8_char_int ("\xdf\xbf"));

    BYTES_EQUAL(0x0, utf8_char_int ("\xe0\x80\x80"));     /* invalid */
    BYTES_EQUAL(0x7ff, utf8_char_int ("\xe0\x9f\xbf"));   /* invalid */
    LONGS_EQUAL(0xd800, utf8_char_int ("\xed\xa0\x80"));  /* invalid */
    LONGS_EQUAL(0xdfff, utf8_char_int ("\xed\xbf\xbf"));  /* invalid */
    BYTES_EQUAL(0x800, utf8_char_int ("\xe0\xa0\x80"));
    BYTES_EQUAL(0xd7ff, utf8_char_int ("\xed\x9f\xbf"));
    BYTES_EQUAL(0xe000, utf8_char_int ("\xe7\x80\x80"));
    BYTES_EQUAL(0xffff, utf8_char_int ("\xef\xbf\xbf"));

    BYTES_EQUAL(0x0, utf8_char_int ("\xf0\x80\x80\x80"));     /* invalid */
    BYTES_EQUAL(0xffff, utf8_char_int ("\xf0\x8f\xbf\xbf"));  /* invalid */
    BYTES_EQUAL(0x10000, utf8_char_int ("\xf0\x90\x80\x80"));
    BYTES_EQUAL(0x1fffff, utf8_char_int ("\xf7\xbf\xbf\xbf"));

    /* convert unicode char to a string */
    utf8_int_string (0, NULL);
    utf8_int_string (0, result);
    STRCMP_EQUAL("", result);
    utf8_int_string (235, result);
    STRCMP_EQUAL("ë", result);
    utf8_int_string (0x20ac, result);
    STRCMP_EQUAL("€", result);
    utf8_int_string (0x24b62, result);
    STRCMP_EQUAL(han_char, result);

    /* get wide char */
    BYTES_EQUAL(WEOF, utf8_wide_char (NULL));
    BYTES_EQUAL(WEOF, utf8_wide_char (""));
    BYTES_EQUAL(65, utf8_wide_char ("A"));
    BYTES_EQUAL(0xc3ab, utf8_wide_char ("ë"));
    BYTES_EQUAL(0xe282ac, utf8_wide_char ("€"));
    BYTES_EQUAL(0xf0a4ada2, utf8_wide_char (han_char));
}

/*
 * Tests functions:
 *   utf8_char_size
 *   utf8_char_size_screen
 *   utf8_strlen
 *   utf8_strnlen
 *   utf8_strlen_screen
 */

TEST(Utf8, Size)
{
    /* char size */
    LONGS_EQUAL(0, utf8_char_size (NULL));
    LONGS_EQUAL(1, utf8_char_size (""));
    LONGS_EQUAL(1, utf8_char_size ("A"));
    LONGS_EQUAL(2, utf8_char_size ("ë"));
    LONGS_EQUAL(3, utf8_char_size ("€"));
    LONGS_EQUAL(4, utf8_char_size (han_char));

    /* char size on screen */
    LONGS_EQUAL(0, utf8_char_size_screen (NULL));
    LONGS_EQUAL(0, utf8_char_size_screen (""));
    LONGS_EQUAL(1, utf8_char_size_screen ("A"));
    LONGS_EQUAL(1, utf8_char_size_screen ("ë"));
    LONGS_EQUAL(1, utf8_char_size_screen ("€"));
    /* this test does not work on Ubuntu Precise: it returns 2 instead of 1 */
    /*LONGS_EQUAL(1, utf8_char_size_screen (han_char));*/

    /* length of string (in chars) */
    LONGS_EQUAL(0, utf8_strlen (NULL));
    LONGS_EQUAL(0, utf8_strlen (""));
    LONGS_EQUAL(1, utf8_strlen ("A"));
    LONGS_EQUAL(1, utf8_strlen ("ë"));
    LONGS_EQUAL(1, utf8_strlen ("€"));
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
    /* this test does not work on Ubuntu Precise: it returns 2 instead of 1 */
    /*LONGS_EQUAL(1, utf8_strlen_screen (han_char));*/
    LONGS_EQUAL(1, utf8_strlen_screen ("\x7f"));
}

/*
 * Tests functions:
 *   utf8_charcmp
 *   utf8_charcasecmp
 *   utf8_charcasecmp_range
 */

TEST(Utf8, Comparison)
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

TEST(Utf8, Duplicate)
{
    char *str;

    WEE_TEST_STR("", utf8_strndup (noel_valid, 0));
    WEE_TEST_STR("n", utf8_strndup (noel_valid, 1));
    WEE_TEST_STR("no", utf8_strndup (noel_valid, 2));
    WEE_TEST_STR("noë", utf8_strndup (noel_valid, 3));
    WEE_TEST_STR("noël", utf8_strndup (noel_valid, 4));
    WEE_TEST_STR("noël", utf8_strndup (noel_valid, 5));
}
