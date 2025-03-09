/*
 * core-string.c - string functions
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <regex.h>
#include <stdint.h>
#include <gcrypt.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#ifndef ICONV_CONST
  #ifdef ICONV_2ARG_IS_CONST
    #define ICONV_CONST const
  #else
    #define ICONV_CONST
  #endif
#endif /* ICONV_CONST */

#include "weechat.h"
#include "core-string.h"
#include "core-config.h"
#include "core-eval.h"
#include "core-hashtable.h"
#include "core-utf8.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../plugins/plugin.h"


#define IS_OCTAL_DIGIT(c) ((c >= '0') && (c <= '7'))
#define HEX2DEC(c) (((c >= 'a') && (c <= 'f')) ? c - 'a' + 10 :         \
                    ((c >= 'A') && (c <= 'F')) ? c - 'A' + 10 :         \
                    c - '0')
#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

struct t_hashtable *string_hashtable_shared = NULL;
int string_concat_index = 0;
char **string_concat_buffer[STRING_NUM_CONCAT_BUFFERS];


/*
 * Formats a message in a string allocated by the function.
 *
 * This function is defined for systems where the GNU function `asprintf()`
 * is not available.
 * The behavior is almost the same except that `*result` is set to NULL on error.
 *
 * Returns the number of bytes in the resulting string, negative value in case
 * of error.
 *
 * Value of `*result` is allocated with the result string (NULL if error),
 * it must be freed after use.
 */

int
string_asprintf (char **result, const char *fmt, ...)
{
    va_list argptr;
    int num_bytes;
    size_t size;

    if (!result)
        return -1;

    *result = NULL;

    if (!fmt)
        return -1;

    /* determine required size */
    va_start (argptr, fmt);
    num_bytes = vsnprintf (NULL, 0, fmt, argptr);
    va_end (argptr);

    if (num_bytes < 0)
        return num_bytes;

    size = (size_t)num_bytes + 1;
    *result = malloc (size);
    if (!*result)
        return -1;

    va_start (argptr, fmt);
    num_bytes = vsnprintf (*result, size, fmt, argptr);
    va_end (argptr);

    if (num_bytes < 0)
    {
        free (*result);
        *result = NULL;
    }

    return num_bytes;
}

/*
 * Defines a "strndup" function for systems where this function does not exist
 * (FreeBSD and maybe others).
 *
 * Note: result must be freed after use.
 */

char *
string_strndup (const char *string, int bytes)
{
    char *result;

    if (!string || (bytes < 0))
        return NULL;

    if ((int)strlen (string) <= bytes)
        return strdup (string);

    result = malloc (bytes + 1);
    if (!result)
        return NULL;

    memcpy (result, string, bytes);
    result[bytes] = '\0';

    return result;
}

/*
 * Cuts a string after max "length" chars, adds an optional suffix
 * after the string if it is cut.
 *
 * If count_suffix == 1, the length of suffix is counted in the max length.
 *
 * If screen == 1, the cut is based on width of chars displayed.
 *
 * Note: result must be freed after use.
 */

char *
string_cut (const char *string, int length, int count_suffix, int screen,
            const char *cut_suffix)
{
    int length_result, length_cut_suffix;
    char *result;
    const char *ptr_string;

    if (!string)
        return NULL;

    if (screen)
        ptr_string = gui_chat_string_add_offset_screen (string, length);
    else
        ptr_string = gui_chat_string_add_offset (string, length);

    if (!ptr_string || !ptr_string[0])
    {
        /* no cut */
        return strdup (string);
    }

    if (cut_suffix && cut_suffix[0])
    {
        length_cut_suffix = strlen (cut_suffix);
        if (count_suffix)
        {
            if (screen)
                length -= gui_chat_strlen_screen (cut_suffix);
            else
                length -= utf8_strlen (cut_suffix);
            if (length < 0)
                return strdup ("");
            if (screen)
                ptr_string = gui_chat_string_add_offset_screen (string, length);
            else
                ptr_string = gui_chat_string_add_offset (string, length);
            if (!ptr_string || !ptr_string[0])
            {
                /* no cut */
                return strdup (string);
            }
        }
        length_result = (ptr_string - string) + length_cut_suffix + 1;
        result = malloc (length_result);
        if (!result)
            return NULL;
        memcpy (result, string, ptr_string - string);
        memcpy (result + (ptr_string - string), cut_suffix,
                length_cut_suffix + 1);
        return result;
    }
    else
    {
        return string_strndup (string, ptr_string - string);
    }
}

/*
 * Reverses a string.
 *
 * Note: result must be freed after use.
 */

char *
string_reverse (const char *string)
{
    int length, char_size;
    const char *ptr_string;
    char *result, *ptr_result;

    if (!string)
        return NULL;

    if (!string[0])
        return strdup (string);

    length = strlen (string);
    result = malloc (length + 1);
    if (!result)
        return NULL;

    ptr_string = string;
    ptr_result = result + length;
    ptr_result[0] = '\0';

    while (ptr_string && ptr_string[0])
    {
        char_size = utf8_char_size (ptr_string);

        ptr_result -= char_size;
        memcpy (ptr_result, ptr_string, char_size);
        ptr_string += char_size;
    }

    return result;
}

/*
 * Reverses a string for screen: color codes are not reversed.
 * For example: reverse of "<red>test" is "test<red>" where the color code
 * "<red>" is kept as-is, so it is still valid (this is not the case with
 * function string_reverse).
 *
 * Note: result must be freed after use.
 */

char *
string_reverse_screen (const char *string)
{
    int length, color_size, char_size;
    const char *ptr_string, *ptr_next;
    char *result, *ptr_result;

    if (!string)
        return NULL;

    if (!string[0])
        return strdup (string);

    length = strlen (string);
    result = malloc (length + 1);
    if (!result)
        return NULL;

    ptr_string = string;
    ptr_result = result + length;
    ptr_result[0] = '\0';

    while (ptr_string && ptr_string[0])
    {
        ptr_next = gui_chat_string_next_char (NULL, NULL,
                                              (const unsigned char *)ptr_string,
                                              0, 0, 0);
        if (!ptr_next)
            ptr_next = ptr_string + strlen (ptr_string);
        color_size = ptr_next - ptr_string;
        if (color_size > 0)
        {
            /* add the color code as-is */
            ptr_result -= color_size;
            memcpy (ptr_result, ptr_string, color_size);
            ptr_string += color_size;
        }

        if (ptr_string[0])
        {
            char_size = utf8_char_size (ptr_string);

            ptr_result -= char_size;
            memcpy (ptr_result, ptr_string, char_size);
            ptr_string += char_size;
        }
    }

    return result;
}

/*
 * Repeats a string a given number of times.
 *
 * Note: result must be freed after use.
 */

char *
string_repeat (const char *string, int count)
{
    int length_string, length_result, i;
    char *result;

    if (!string)
        return NULL;

    if (!string[0] || (count <= 0))
        return strdup ("");

    if (count == 1)
        return strdup (string);

    length_string = strlen (string);

    if (count >= INT_MAX / length_string)
        return NULL;

    length_result = (length_string * count) + 1;
    result = malloc (length_result);
    if (!result)
        return NULL;

    i = 0;
    while (count > 0)
    {
        memcpy (result + i, string, length_string);
        count--;
        i += length_string;
    }
    result[length_result - 1] = '\0';

    return result;
}

/*
 * Converts string to lowercase (locale dependent).
 */

char *
string_tolower (const char *string)
{
    char **result, utf_char[5];

    if (!string)
        return NULL;

    result = string_dyn_alloc (strlen (string) + 1);
    if (!result)
        return NULL;

    while (string && string[0])
    {
        if (!((unsigned char)(string[0]) & 0x80))
        {
            /*
             * optimization for single-byte char: only letters A-Z must be
             * converted to lowercase; this is faster than calling `towlower`
             */
            if ((string[0] >= 'A') && (string[0] <= 'Z'))
                utf_char[0] = string[0] + ('a' - 'A');
            else
                utf_char[0] = string[0];
            utf_char[1] = '\0';
            string_dyn_concat (result, utf_char, -1);
            string++;
        }
        else
        {
            /* char ≥ 2 bytes, use `towlower` */
            utf8_int_string (towlower (utf8_char_int (string)), utf_char);
            string_dyn_concat (result, utf_char, -1);
            string = (char *)utf8_next_char (string);
        }
    }
    return string_dyn_free (result, 0);
}

/*
 * Converts string to uppercase (locale dependent).
 */

char *
string_toupper (const char *string)
{
    char **result, utf_char[5];

    if (!string)
        return NULL;

    result = string_dyn_alloc (strlen (string) + 1);
    if (!result)
        return NULL;

    while (string && string[0])
    {
        if (!((unsigned char)(string[0]) & 0x80))
        {
            /*
             * optimization for single-byte char: only letters a-z must be
             * converted to uppercase; this is faster than calling `towupper`
             */
            if ((string[0] >= 'a') && (string[0] <= 'z'))
                utf_char[0] = string[0] - ('a' - 'A');
            else
                utf_char[0] = string[0];
            utf_char[1] = '\0';
            string_dyn_concat (result, utf_char, -1);
            string++;
        }
        else
        {
            /* char ≥ 2 bytes, use `towupper` */
            utf8_int_string (towupper (utf8_char_int (string)), utf_char);
            string_dyn_concat (result, utf_char, -1);
            string = (char *)utf8_next_char (string);
        }
    }
    return string_dyn_free (result, 0);
}

/*
 * Converts string to lower case (using a range of chars).
 *
 * Note: result must be freed after use.
 */

char *
string_tolower_range (const char *string, int range)
{
    char *result, *ptr_result;

    if (!string)
        return NULL;

    if (range <= 0)
        return string_tolower (string);

    result = strdup (string);
    if (!result)
        return NULL;

    ptr_result = result;
    while (ptr_result && ptr_result[0])
    {
        if ((ptr_result[0] >= 'A') && (ptr_result[0] < 'A' + range))
            ptr_result[0] += ('a' - 'A');
        ptr_result = (char *)utf8_next_char (ptr_result);
    }

    return result;
}

/*
 * Converts string to upper case (using a range of char).
 *
 * Note: result must be freed after use.
 */

char *
string_toupper_range (const char *string, int range)
{
    char *result, *ptr_result;

    if (!string)
        return NULL;

    if (range <= 0)
        return string_toupper (string);

    result = strdup (string);
    if (!result)
        return NULL;

    ptr_result = result;
    while (ptr_result && ptr_result[0])
    {
        if ((ptr_result[0] >= 'a') && (ptr_result[0] < 'a' + range))
            ptr_result[0] -= ('a' - 'A');
        ptr_result = (char *)utf8_next_char (ptr_result);
    }

    return result;
}

/*
 * Compares two chars (case-sensitive).
 *
 * Returns: arithmetic result of subtracting the first UTF-8 char in string2
 * from the first UTF-8 char in string1:
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
string_charcmp (const char *string1, const char *string2)
{
    return utf8_char_int (string1) - utf8_char_int (string2);
}

/*
 * Compares two chars (case-insensitive).
 *
 * Returns: arithmetic result of subtracting the first UTF-8 char in string2
 * (converted to lowercase) from the first UTF-8 char in string1 (converted
 * to lowercase):
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
string_charcasecmp (const char *string1, const char *string2)
{
    wint_t wchar1, wchar2;

    /*
     * optimization for single-byte chars: only letters A-Z must be converted
     * to lowercase; this is faster than calling `towlower`
     */
    if (string1 && !((unsigned char)(string1[0]) & 0x80)
        && string2 && !((unsigned char)(string2[0]) & 0x80))
    {
        wchar1 = string1[0];
        if ((wchar1 >= 'A') && (wchar1 <= 'Z'))
            wchar1 += ('a' - 'A');
        wchar2 = string2[0];
        if ((wchar2 >= 'A') && (wchar2 <= 'Z'))
            wchar2 += ('a' - 'A');
    }
    else
    {
        wchar1 = towlower (utf8_char_int (string1));
        wchar2 = towlower (utf8_char_int (string2));
    }

    return wchar1 - wchar2;
}

/*
 * Compares two chars (case-insensitive using a range).
 *
 * The range is the number of chars which can be converted from upper to lower
 * case. For example 26 = all letters of alphabet, 29 = all letters + 3 chars.
 *
 * Examples:
 *   - range = 26: A-Z         ==> a-z
 *   - range = 29: A-Z [ \ ]   ==> a-z { | }
 *   - range = 30: A-Z [ \ ] ^ ==> a-z { | } ~
 *   (ranges 29 and 30 are used by some protocols like IRC)
 *
 * Returns: arithmetic result of subtracting the last compared UTF-8 char in
 * string2 (converted to lowercase) from the last compared UTF-8 char in
 * string1 (converted to lowercase):
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
string_charcasecmp_range (const char *string1, const char *string2, int range)
{
    wchar_t wchar1, wchar2;

    wchar1 = utf8_char_int (string1);
    if ((wchar1 >= (wchar_t)'A') && (wchar1 < (wchar_t)('A' + range)))
        wchar1 += ('a' - 'A');

    wchar2 = utf8_char_int (string2);
    if ((wchar2 >= (wchar_t)'A') && (wchar2 < (wchar_t)('A' + range)))
        wchar2 += ('a' - 'A');

    return wchar1 - wchar2;
}

/*
 * Compares two strings (case-sensitive).
 *
 * Returns: arithmetic result of subtracting the last compared UTF-8 char in
 * string2 from the last compared UTF-8 char in string1:
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
string_strcmp (const char *string1, const char *string2)
{
    int diff;

    if (!string1 && string2)
        return -1;

    if (string1 && !string2)
        return 1;

    while (string1 && string1[0] && string2 && string2[0])
    {
        diff = string_charcmp (string1, string2);
        if (diff != 0)
            return diff;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
    }

    return string_charcmp (string1, string2);
}

/*
 * Compares two strings with max length (case-sensitive).
 *
 * Returns: arithmetic result of subtracting the last compared UTF-8 char in
 * string2 from the last compared UTF-8 char in string1:
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
string_strncmp (const char *string1, const char *string2, int max)
{
    int count, diff;

    if (!string1 && string2)
        return -1;

    if (string1 && !string2)
        return 1;

    count = 0;
    while ((count < max) && string1 && string1[0] && string2 && string2[0])
    {
        diff = string_charcmp (string1, string2);
        if (diff != 0)
            return diff;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
        count++;
    }

    if (count >= max)
        return 0;
    else
        return string_charcmp (string1, string2);
}

/*
 * Compares two strings (case-insensitive).
 *
 * Returns: arithmetic result of subtracting the last compared UTF-8 char in
 * string2 (converted to lowercase) from the last compared UTF-8 char in
 * string1 (converted to lowercase):
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
string_strcasecmp (const char *string1, const char *string2)
{
    int diff;

    if (!string1 && string2)
        return -1;

    if (string1 && !string2)
        return 1;

    while (string1 && string1[0] && string2 && string2[0])
    {
        diff = string_charcasecmp (string1, string2);
        if (diff != 0)
            return diff;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
    }

    return string_charcasecmp (string1, string2);
}

/*
 * Compares two strings (case-insensitive using a range).
 *
 * The range is the number of chars which can be converted from upper to lower
 * case. For example 26 = all letters of alphabet, 29 = all letters + 3 chars.
 *
 * Examples:
 *   - range = 26: A-Z         ==> a-z
 *   - range = 29: A-Z [ \ ]   ==> a-z { | }
 *   - range = 30: A-Z [ \ ] ^ ==> a-z { | } ~
 *   (ranges 29 and 30 are used by some protocols like IRC)
 *
 * Returns: arithmetic result of subtracting the last compared UTF-8 char in
 * string2 (converted to lowercase) from the last compared UTF-8 char in
 * string1 (converted to lowercase):
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
string_strcasecmp_range (const char *string1, const char *string2, int range)
{
    int diff;

    if (!string1 && string2)
        return -1;

    if (string1 && !string2)
        return 1;

    while (string1 && string1[0] && string2 && string2[0])
    {
        diff = string_charcasecmp_range (string1, string2, range);
        if (diff != 0)
            return diff;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
    }

    return string_charcasecmp_range (string1, string2, range);
}

/*
 * Compares two strings with max length (case-insensitive).
 *
 * Returns: arithmetic result of subtracting the last compared UTF-8 char in
 * string2 (converted to lowercase) from the last compared UTF-8 char in
 * string1 (converted to lowercase):
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
string_strncasecmp (const char *string1, const char *string2, int max)
{
    int count, diff;

    if (!string1 && string2)
        return -1;

    if (string1 && !string2)
        return 1;

    count = 0;
    while ((count < max) && string1 && string1[0] && string2 && string2[0])
    {
        diff = string_charcasecmp (string1, string2);
        if (diff != 0)
            return diff;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
        count++;
    }

    if (count >= max)
        return 0;
    else
        return string_charcasecmp (string1, string2);
}

/*
 * Compares two strings with max length (case-insensitive using a range).
 *
 * The range is the number of chars which can be converted from upper to lower
 * case. For example 26 = all letters of alphabet, 29 = all letters + 3 chars.
 *
 * Examples:
 *   - range = 26: A-Z         ==> a-z
 *   - range = 29: A-Z [ \ ]   ==> a-z { | }
 *   - range = 30: A-Z [ \ ] ^ ==> a-z { | } ~
 *   (ranges 29 and 30 are used by some protocols like IRC)
 *
 * Returns: arithmetic result of subtracting the last compared UTF-8 char in
 * string2 (converted to lowercase) from the last compared UTF-8 char in
 * string1 (converted to lowercase):
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
string_strncasecmp_range (const char *string1, const char *string2, int max,
                          int range)
{
    int count, diff;

    if (!string1 && string2)
        return -1;

    if (string1 && !string2)
        return 1;

    count = 0;
    while ((count < max) && string1 && string1[0] && string2 && string2[0])
    {
        diff = string_charcasecmp_range (string1, string2, range);
        if (diff != 0)
            return diff;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
        count++;
    }

    if (count >= max)
        return 0;
    else
        return string_charcasecmp_range (string1, string2, range);
}

/*
 * Compares two strings, ignoring some chars.
 *
 * Returns: arithmetic result of subtracting the last compared UTF-8 char in
 * string2 (converted to lowercase if case_sensitive is set to 0) from the last
 * compared UTF-8 char in string1 (converted to lowercase if case_sensitive is
 * set to 0):
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
string_strcmp_ignore_chars (const char *string1, const char *string2,
                            const char *chars_ignored, int case_sensitive)
{
    int diff;

    if (!string1 && string2)
        return -1;

    if (string1 && !string2)
        return 1;

    while (string1 && string1[0] && string2 && string2[0])
    {
        /* skip ignored chars */
        while (string1 && string1[0] && strchr (chars_ignored, string1[0]))
        {
            string1 = utf8_next_char (string1);
        }
        while (string2 && string2[0] && strchr (chars_ignored, string2[0]))
        {
            string2 = utf8_next_char (string2);
        }

        /* end of one (or both) string(s) ? */
        if (!string1 || !string1[0] || !string2 || !string2[0])
        {
            return (case_sensitive) ?
                string_charcmp (string1, string2) :
                string_charcasecmp (string1, string2);
        }

        /* look at diff */
        diff = (case_sensitive) ?
            string_charcmp (string1, string2) :
            string_charcasecmp (string1, string2);
        if (diff != 0)
            return diff;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);

        /* skip ignored chars */
        while (string1 && string1[0] && strchr (chars_ignored, string1[0]))
        {
            string1 = utf8_next_char (string1);
        }
        while (string2 && string2[0] && strchr (chars_ignored, string2[0]))
        {
            string2 = utf8_next_char (string2);
        }
    }
    return (case_sensitive) ?
        string_charcmp (string1, string2) :
        string_charcasecmp (string1, string2);
}

/*
 * Searches for a string in another string (locale and case independent).
 *
 * Returns pointer to string found, or NULL if not found.
 */

const char *
string_strcasestr (const char *string, const char *search)
{
    int length_search;

    length_search = utf8_strlen (search);

    if (!string || !search || (length_search == 0))
        return NULL;

    while (string[0])
    {
        if (string_strncasecmp (string, search, length_search) == 0)
            return (char *)string;

        string = utf8_next_char (string);
    }

    return NULL;
}

/*
 * Checks if a string matches a mask.
 *
 * The mask can contain wildcards ("*"), each wildcard matches 0 or more chars
 * in the string.
 *
 * Returns:
 *   1: string matches mask
 *   0: string does not match mask
 */

int
string_match (const char *string, const char *mask, int case_sensitive)
{
    const char *ptr_string, *ptr_mask, *pos_word, *pos_word2, *pos_end;
    char *word;
    int wildcard, length_word;

    if (!string || !mask || !mask[0])
        return 0;

    ptr_string = string;
    ptr_mask = mask;

    while (ptr_mask[0])
    {
        wildcard = 0;

        /* if we are on a wildcard, set the wildcard flag and skip it */
        if (ptr_mask[0] == '*')
        {
            wildcard = 1;
            ptr_mask++;
            while (ptr_mask[0] == '*')
            {
                ptr_mask++;
            }
            if (!ptr_mask[0])
                return 1;
        }

        /* no match if some mask without string */
        if (!string[0])
            return 0;

        /* search the next wildcard (after the word) */
        pos_end = strchr (ptr_mask, '*');

        /* extract the word before the wildcard (or the end of mask) */
        if (pos_end)
        {
            length_word = pos_end - ptr_mask;
        }
        else
        {
            length_word = strlen (ptr_mask);
            pos_end = ptr_mask + length_word;
        }
        word = string_strndup (ptr_mask, length_word);
        if (!word)
            return 0;

        /* check if the word is matching */
        if (wildcard)
        {
            /*
             * search the word anywhere in the string (from current position),
             * multiple times if needed
             */
            pos_word = (case_sensitive) ?
                strstr (ptr_string, word) : string_strcasestr (ptr_string, word);
            if (!pos_word)
            {
                free (word);
                return 0;
            }
            if ((!pos_word[length_word] && !pos_end[0])
                || string_match (pos_word + length_word, pos_end,
                                 case_sensitive))
            {
                free (word);
                return 1;
            }
            while (1)
            {
                pos_word2 = (case_sensitive) ?
                    strstr (pos_word + length_word, word) :
                    string_strcasestr (pos_word + length_word, word);
                if (!pos_word2)
                    break;
                pos_word = pos_word2;
                if ((!pos_word[length_word] && !pos_end[0])
                    || string_match (pos_word + length_word, pos_end,
                                     case_sensitive))
                {
                    free (word);
                    return 1;
                }
            }
            free (word);
            return 0;
        }
        else
        {
            /* check if word is at beginning of string */
            if ((case_sensitive
                 && (strncmp (ptr_string, word, length_word) != 0))
                || (!case_sensitive
                    && (string_strncasecmp (ptr_string, word,
                                            utf8_strlen (word)) != 0)))
            {
                free (word);
                return 0;
            }
            ptr_string += length_word;
        }

        free (word);

        ptr_mask = pos_end;
    }

    /* match if no more string/mask */
    if (!ptr_string[0] && !ptr_mask[0])
        return 1;

    /* no match in other cases */
    return 0;
}

/*
 * Checks if a string matches a list of masks. Negative masks are allowed
 * with "!mask" to exclude this mask and have higher priority than standard
 * masks.
 *
 * Each mask is compared with the function string_match.
 *
 * Example of masks to allow anything by default, but "toto" and "abc" are
 * forbidden:
 *   "*", "!toto", "!abc"
 *
 * Returns:
 *   1: string matches list of masks
 *   0: string does not match list of masks
 */

int
string_match_list (const char *string, const char **masks, int case_sensitive)
{
    int match;
    const char **ptr_mask, *ptr_mask2;

    if (!string || !masks)
        return 0;

    match = 0;

    for (ptr_mask = masks; *ptr_mask; ptr_mask++)
    {
        ptr_mask2 = ((*ptr_mask)[0] == '!') ? *ptr_mask + 1 : *ptr_mask;
        if (string_match (string, ptr_mask2, case_sensitive))
        {
            if ((*ptr_mask)[0] == '!')
                return 0;
            else
                match = 1;
        }
    }

    return match;
}

/*
 * Expands home in a path.
 *
 * Example: "~/file.txt" => "/home/user/file.txt"
 *
 * Note: result must be freed after use.
 */

char *
string_expand_home (const char *path)
{
    char *ptr_home, *str;

    if (!path)
        return NULL;

    if (!path[0] || (path[0] != '~')
        || ((path[1] && path[1] != DIR_SEPARATOR_CHAR)))
    {
        return strdup (path);
    }

    ptr_home = getenv ("HOME");
    if (!ptr_home)
        return NULL;

    string_asprintf (&str, "%s%s", ptr_home, path + 1);

    return str;
}

/*
 * Evaluate a path by replacing (in this order):
 *   1. "%h" (at beginning of string) by WeeChat home directory (deprecated)
 *   2. "~" by user home directory (call to string_expand_home)
 *   3. evaluated variables (see /help eval)
 *
 * Returns the evaluated path, NULL if error.
 *
 * Note: result must be freed after use.
 */

char *
string_eval_path_home (const char *path,
                       struct t_hashtable *pointers,
                       struct t_hashtable *extra_vars,
                       struct t_hashtable *options)
{
    char *path1, *path2, *path3;
    const char *ptr_option_directory, *ptr_directory;

    if (!path)
        return NULL;

    path1 = NULL;
    path2 = NULL;
    path3 = NULL;

    /*
     * replace "%h" by WeeChat home
     * (deprecated: "%h" should not be used anymore with WeeChat ≥ 3.2)
     */
    if (strncmp (path, "%h", 2) == 0)
    {
        ptr_directory = weechat_data_dir;
        ptr_option_directory = hashtable_get (options, "directory");
        if (ptr_option_directory)
        {
            if (strcmp (ptr_option_directory, "config") == 0)
                ptr_directory = weechat_config_dir;
            else if (strcmp (ptr_option_directory, "data") == 0)
                ptr_directory = weechat_data_dir;
            else if (strcmp (ptr_option_directory, "state") == 0)
                ptr_directory = weechat_state_dir;
            else if (strcmp (ptr_option_directory, "cache") == 0)
                ptr_directory = weechat_cache_dir;
            else if (strcmp (ptr_option_directory, "runtime") == 0)
                ptr_directory = weechat_runtime_dir;
        }
        string_asprintf (&path1, "%s%s", ptr_directory, path + 2);
    }
    else
    {
        path1 = strdup (path);
    }
    if (!path1)
        goto end;

    /* replace "~" by user home */
    path2 = string_expand_home (path1);
    if (!path2)
        goto end;

    /* evaluate content of path */
    path3 = eval_expression (path2, pointers, extra_vars, options);

end:
    free (path1);
    free (path2);

    return path3;
}

/*
 * Removes quotes at beginning/end of string (ignores spaces if there are before
 * first quote or after last quote).
 *
 * Note: result must be freed after use.
 */

char *
string_remove_quotes (const char *string, const char *quotes)
{
    int length;
    const char *pos_start, *pos_end;

    if (!string || !quotes)
        return NULL;

    if (!string[0])
        return strdup (string);

    pos_start = string;
    while (pos_start[0] == ' ')
    {
        pos_start++;
    }
    length = strlen (string);
    pos_end = string + length - 1;
    while ((pos_end[0] == ' ') && (pos_end > pos_start))
    {
        pos_end--;
    }
    if (!pos_start[0] || !pos_end[0] || (pos_end <= pos_start))
        return strdup (string);

    if (strchr (quotes, pos_start[0]) && (pos_end[0] == pos_start[0]))
    {
        if (pos_end == (pos_start + 1))
            return strdup ("");
        return string_strndup (pos_start + 1, pos_end - pos_start - 1);
    }

    return strdup (string);
}

/*
 * Strips chars at beginning/end of string.
 *
 * Note: result must be freed after use.
 */

char *
string_strip (const char *string, int left, int right, const char *chars)
{
    const char *ptr_start, *ptr_end;

    if (!string)
        return NULL;

    if (!string[0] || !chars)
        return strdup (string);

    ptr_start = string;
    ptr_end = string + strlen (string) - 1;

    if (left)
    {
        while (ptr_start[0] && strchr (chars, ptr_start[0]))
        {
            ptr_start++;
        }
        if (!ptr_start[0])
            return strdup (ptr_start);
    }

    if (right)
    {
        while ((ptr_end >= ptr_start) && strchr (chars, ptr_end[0]))
        {
            ptr_end--;
        }
        if (ptr_end < ptr_start)
            return strdup ("");
    }

    return string_strndup (ptr_start, ptr_end - ptr_start + 1);
}

/*
 * Converts escaped chars to their values.
 *
 * Following escaped chars are supported:
 *   \"         double quote
 *   \\         backslash
 *   \a         alert (BEL)
 *   \b         backspace
 *   \e         escape
 *   \f         form feed
 *   \n         new line
 *   \r         carriage return
 *   \t         horizontal tab
 *   \v         vertical tab
 *   \0ooo      char as octal value (ooo is 0 to 3 digits)
 *   \xhh       char as hexadecimal value (hh is 1 to 2 digits)
 *   \uhhhh     unicode char as hexadecimal value (hhhh is 1 to 4 digits)
 *   \Uhhhhhhhh unicode char as hexadecimal value (hhhhhhhh is 1 to 8 digits)
 *
 * Note: result must be freed after use.
 */

char *
string_convert_escaped_chars (const char *string)
{
    const unsigned char *ptr_string;
    char *output, utf_char[16];
    int pos_output, i, length;
    unsigned int value;

    if (!string)
        return NULL;

    /* the output length is always <= to string length */
    output = malloc (strlen (string) + 1);
    if (!output)
        return NULL;

    pos_output = 0;
    ptr_string = (const unsigned char *)string;
    while (ptr_string && ptr_string[0])
    {
        if (ptr_string[0] == '\\')
        {
            ptr_string++;
            switch (ptr_string[0])
            {
                case '"':  /* double quote */
                    output[pos_output++] = '"';
                    ptr_string++;
                    break;
                case '\\':  /* backslash */
                    output[pos_output++] = '\\';
                    ptr_string++;
                    break;
                case 'a':  /* alert */
                    output[pos_output++] = 7;
                    ptr_string++;
                    break;
                case 'b':  /* backspace */
                    output[pos_output++] = 8;
                    ptr_string++;
                    break;
                case 'e':  /* escape */
                    output[pos_output++] = 27;
                    ptr_string++;
                    break;
                case 'f':  /* form feed */
                    output[pos_output++] = 12;
                    ptr_string++;
                    break;
                case 'n':  /* new line */
                    output[pos_output++] = 10;
                    ptr_string++;
                    break;
                case 'r':  /* carriage return */
                    output[pos_output++] = 13;
                    ptr_string++;
                    break;
                case 't':  /* horizontal tab */
                    output[pos_output++] = 9;
                    ptr_string++;
                    break;
                case 'v':  /* vertical tab */
                    output[pos_output++] = 11;
                    ptr_string++;
                    break;
                case '0':  /* char as octal value (0 to 3 digits) */
                    value = 0;
                    for (i = 0; (i < 3) && IS_OCTAL_DIGIT(ptr_string[i + 1]); i++)
                    {
                        value = (value * 8) + (ptr_string[i + 1] - '0');
                    }
                    output[pos_output++] = value;
                    ptr_string += 1 + i;
                    break;
                case 'x':  /* char as hexadecimal value (1 to 2 digits) */
                case 'X':
                    if (isxdigit (ptr_string[1]))
                    {
                        value = 0;
                        for (i = 0; (i < 2) && isxdigit (ptr_string[i + 1]); i++)
                        {
                            value = (value * 16) + HEX2DEC(ptr_string[i + 1]);
                        }
                        output[pos_output++] = value;
                        ptr_string += 1 + i;
                    }
                    else
                    {
                        output[pos_output++] = ptr_string[0];
                        ptr_string++;
                    }
                    break;
                case 'u':  /* unicode char as hexadecimal (1 to 4 digits) */
                case 'U':  /* unicode char as hexadecimal (1 to 8 digits) */
                    if (isxdigit (ptr_string[1]))
                    {
                        value = 0;
                        for (i = 0;
                             (i < ((ptr_string[0] == 'u') ? 4 : 8))
                                 && isxdigit (ptr_string[i + 1]);
                             i++)
                        {
                            value = (value * 16) + HEX2DEC(ptr_string[i + 1]);
                        }
                        length = utf8_int_string (value, utf_char);
                        if (utf_char[0])
                        {
                            memcpy (output + pos_output, utf_char, length);
                            pos_output += length;
                        }
                        ptr_string += 1 + i;
                    }
                    else
                    {
                        output[pos_output++] = ptr_string[0];
                        ptr_string++;
                    }
                    break;
                default:
                    if (ptr_string[0])
                    {
                        output[pos_output++] = '\\';
                        output[pos_output++] = ptr_string[0];
                        ptr_string++;
                    }
                    break;
            }
        }
        else
        {
            output[pos_output++] = ptr_string[0];
            ptr_string++;
        }
    }
    output[pos_output] = '\0';

    return output;
}

/*
 * Checks if first char of string is a whitespace (space, tab, newline or carriage return).
 *
 * Returns:
 *   1: first char is whitespace
 *   0: first char is not whitespace
 */

int
string_is_whitespace_char (const char *string)
{
    return (string && (
            (string[0] == ' ')
            || (string[0] == '\t')
            || (string[0] == '\n')
            || (string[0] == '\r'))) ? 1 : 0;
}

/*
 * Checks if first char of string is a "word char".
 *
 * The word chars are customizable with options "weechat.look.word_chars_*".
 *
 * Returns:
 *   1: first char is a word char
 *   0: first char is not a word char
 */

int
string_is_word_char (const char *string,
                     struct t_config_look_word_char_item *word_chars,
                     int word_chars_count)
{
    wint_t c;
    int i, match;

    if (!string || !string[0])
        return 0;

    c = utf8_char_int (string);

    for (i = 0; i < word_chars_count; i++)
    {
        if (word_chars[i].wc_class != (wctype_t)0)
        {
            match = iswctype (c, word_chars[i].wc_class);
        }
        else
        {
            if ((word_chars[i].char1 == 0)
                && (word_chars[i].char2 == 0))
            {
                match = 1;
            }
            else
            {
                match = ((c >= word_chars[i].char1) &&
                         (c <= word_chars[i].char2));
            }
        }
        if (match)
            return (word_chars[i].exclude) ? 0 : 1;
    }

    /* not a word char */
    return 0;
}

/*
 * Checks if first char of string is a "word char" (for highlight).
 *
 * The word chars for highlights are customizable with option
 * "weechat.look.word_chars_highlight".
 *
 * Returns:
 *   1: first char is a word char
 *   0: first char is not a word char
 */

int
string_is_word_char_highlight (const char *string)
{
    return string_is_word_char (string,
                                config_word_chars_highlight,
                                config_word_chars_highlight_count);
}

/*
 * Checks if first char of string is a "word char" (for input).
 *
 * The word chars for input are customizable with option
 * "weechat.look.word_chars_input".
 *
 * Returns:
 *   1: first char is a word char
 *   0: first char is not a word char
 */

int
string_is_word_char_input (const char *string)
{
    return string_is_word_char (string,
                                config_word_chars_input,
                                config_word_chars_input_count);
}

/*
 * Converts a mask (string with only "*" as wildcard) to a regex, paying
 * attention to special chars in a regex.
 *
 * Note: result must be freed after use.
 */

char *
string_mask_to_regex (const char *mask)
{
    char *result;
    const char *ptr_mask;
    int index_result;
    char *regex_special_char = ".[]{}()?+|^$\\";

    if (!mask)
        return NULL;

    result = malloc ((strlen (mask) * 2) + 1);
    if (!result)
        return NULL;

    result[0] = '\0';
    index_result = 0;
    ptr_mask = mask;
    while (ptr_mask[0])
    {
        /* '*' in string ? then replace by '.*' */
        if (ptr_mask[0] == '*')
        {
            result[index_result++] = '.';
            result[index_result++] = '*';
        }
        /* special regex char in string ? escape it with '\' */
        else if (strchr (regex_special_char, ptr_mask[0]))
        {
            result[index_result++] = '\\';
            result[index_result++] = ptr_mask[0];
        }
        /* standard char, just copy it */
        else
            result[index_result++] = ptr_mask[0];

        ptr_mask++;
    }

    /* add final '\0' */
    result[index_result] = '\0';

    return result;
}

/*
 * Extracts flags and regex from a string.
 *
 * Format of flags is: (?eins-eins)string
 * Flags are:
 *   e: POSIX extended regex (REG_EXTENDED)
 *   i: case-insensitive (REG_ICASE)
 *   n: match-any-character operators don't match a newline (REG_NEWLINE)
 *   s: support for substring addressing of matches is not required (REG_NOSUB)
 *
 * Examples (with default_flags = REG_EXTENDED):
 *   "(?i)toto"  : regex "toto", flags = REG_EXTENDED | REG_ICASE
 *   "(?i)toto"  : regex "toto", flags = REG_EXTENDED | REG_ICASE
 *   "(?i-e)toto": regex "toto", flags = REG_ICASE
 */

const char *
string_regex_flags (const char *regex, int default_flags, int *flags)
{
    const char *ptr_regex, *ptr_flags;
    int set_flag, flag;
    char *pos;

    if (flags)
        *flags = default_flags;

    if (!regex)
        return NULL;

    ptr_regex = regex;
    while (strncmp (ptr_regex, "(?", 2) == 0)
    {
        pos = strchr (ptr_regex, ')');
        if (!pos)
            break;
        if (!isalpha ((unsigned char)ptr_regex[2]) && (ptr_regex[2] != '-'))
            break;
        if (flags)
        {
            set_flag = 1;
            for (ptr_flags = ptr_regex + 2; ptr_flags < pos; ptr_flags++)
            {
                flag = 0;
                switch (ptr_flags[0])
                {
                    case '-':
                        set_flag = 0;
                        break;
                    case 'e':
                        flag = REG_EXTENDED;
                        break;
                    case 'i':
                        flag = REG_ICASE;
                        break;
                    case 'n':
                        flag = REG_NEWLINE;
                        break;
                    case 's':
                        flag = REG_NOSUB;
                        break;
                }
                if (flag > 0)
                {
                    if (set_flag)
                        *flags |= flag;
                    else
                        *flags &= ~flag;
                }
            }
        }
        ptr_regex = pos + 1;
    }

    return ptr_regex;
}

/*
 * Compiles a regex using optional flags at beginning of string (for format of
 * flags in regex, see string_regex_flags()).
 *
 * Returns:
 *   0: successful compilation
 *   other value: compilation failed
 *
 * Note: regex must be freed with regfree after use.
 */

int
string_regcomp (void *preg, const char *regex, int default_flags)
{
    const char *ptr_regex;
    int flags;

    if (!regex)
        return -1;

    ptr_regex = string_regex_flags (regex, default_flags, &flags);

    return regcomp ((regex_t *)preg,
                    (ptr_regex && ptr_regex[0]) ? ptr_regex : "^",
                    flags);
}

/*
 * Checks if a string has a highlight (using list of words to highlight).
 *
 * Returns:
 *   1: string has a highlight
 *   0: string has no highlight
 */

int
string_has_highlight (const char *string, const char *highlight_words)
{
    const char *match, *match_pre, *match_post, *msg_pos;
    char *msg, *highlight, *pos, *pos_end;
    int end, length, startswith, endswith, wildcard_start, wildcard_end, flags;

    if (!string || !string[0] || !highlight_words || !highlight_words[0])
        return 0;

    msg = strdup (string);
    if (!msg)
        return 0;

    highlight = strdup (highlight_words);
    if (!highlight)
    {
        free (msg);
        return 0;
    }

    pos = highlight;
    end = 0;
    while (!end)
    {
        flags = 0;
        pos = (char *)string_regex_flags (pos, REG_ICASE, &flags);

        pos_end = strchr (pos, ',');
        if (!pos_end)
        {
            pos_end = strchr (pos, '\0');
            end = 1;
        }
        /* error parsing string! */
        if (!pos_end)
        {
            free (msg);
            free (highlight);
            return 0;
        }

        length = pos_end - pos;
        pos_end[0] = '\0';
        if (length > 0)
        {
            if ((wildcard_start = (pos[0] == '*')))
            {
                pos++;
                length--;
            }
            if ((wildcard_end = (*(pos_end - 1) == '*')))
            {
                *(pos_end - 1) = '\0';
                length--;
            }
        }

        if (length > 0)
        {
            msg_pos = msg;
            while (1)
            {
                match = (flags & REG_ICASE) ?
                    string_strcasestr (msg_pos, pos) : strstr (msg_pos, pos);
                if (!match)
                    break;
                match_pre = utf8_prev_char (msg, match);
                if (!match_pre)
                    match_pre = match - 1;
                match_post = match + length;
                startswith = ((match == msg) || (!string_is_word_char_highlight (match_pre)));
                endswith = ((!match_post[0]) || (!string_is_word_char_highlight (match_post)));
                if ((wildcard_start && wildcard_end) ||
                    (!wildcard_start && !wildcard_end &&
                     startswith && endswith) ||
                    (wildcard_start && endswith) ||
                    (wildcard_end && startswith))
                {
                    /* highlight found! */
                    free (msg);
                    free (highlight);
                    return 1;
                }
                msg_pos = match_post;
            }
        }

        if (!end)
            pos = pos_end + 1;
    }

    free (msg);
    free (highlight);

    /* no highlight found */
    return 0;
}

/*
 * Checks if a string has a highlight using a compiled regular expression (any
 * match in string must be surrounded by delimiters).
 */

int
string_has_highlight_regex_compiled (const char *string, regex_t *regex)
{
    int rc, startswith, endswith;
    regmatch_t regex_match;
    const char *match_pre;

    if (!string || !regex)
        return 0;

    while (string && string[0])
    {
        rc = regexec (regex, string,  1, &regex_match, 0);

        /*
         * no match found: exit the loop (if rm_eo == 0, it is an empty match
         * at beginning of string: we consider there is no match, to prevent an
         * infinite loop)
         */
        if ((rc != 0) || (regex_match.rm_so < 0) || (regex_match.rm_eo <= 0))
            break;

        startswith = (regex_match.rm_so == 0);
        if (!startswith)
        {
            match_pre = utf8_prev_char (string, string + regex_match.rm_so);
            startswith = !string_is_word_char_highlight (match_pre);
        }
        endswith = 0;
        if (startswith)
        {
            endswith = ((regex_match.rm_eo == (int)strlen (string))
                        || !string_is_word_char_highlight (string + regex_match.rm_eo));
        }
        if (startswith && endswith)
            return 1;

        string += regex_match.rm_eo;
    }

    /* no highlight found */
    return 0;
}

/*
 * Checks if a string has a highlight using a regular expression (any match in
 * string must be surrounded by delimiters).
 */

int
string_has_highlight_regex (const char *string, const char *regex)
{
    regex_t reg;
    int rc;

    if (!string || !regex || !regex[0])
        return 0;

    if (string_regcomp (&reg, regex, REG_EXTENDED | REG_ICASE) != 0)
        return 0;

    rc = string_has_highlight_regex_compiled (string, &reg);

    regfree (&reg);

    return rc;
}

/*
 * Replaces a string by new one in a string.
 *
 * Note: result must be freed after use.
 */

char *
string_replace (const char *string, const char *search, const char *replace)
{
    const char *pos;
    char *new_string;
    int length1, length2, length_new, count;

    if (!string || !search || !replace)
        return NULL;

    length1 = strlen (search);
    length2 = strlen (replace);

    /* count number of strings to replace */
    count = 0;
    pos = string;
    while (pos && pos[0] && (pos = strstr (pos, search)))
    {
        count++;
        pos += length1;
    }

    /* easy: no string to replace! */
    if (count == 0)
        return strdup (string);

    /* compute needed memory for new string */
    length_new = strlen (string) - (count * length1) + (count * length2) + 1;

    /* allocate new string */
    new_string = malloc (length_new);
    if (!new_string)
        return strdup (string);

    /* replace all occurrences */
    new_string[0] = '\0';
    while (string && string[0])
    {
        pos = strstr (string, search);
        if (pos)
        {
            strncat (new_string, string, pos - string);
            strcat (new_string, replace);
            pos += length1;
        }
        else
            strcat (new_string, string);
        string = pos;
    }
    return new_string;
}

/*
 * Get replacement string for a regex, using array of "match"
 * (for more info, see function "string_replace_regex").
 *
 * Note: result must be freed after use.
 */

char *
string_replace_regex_get_replace (const char *string, regmatch_t *regex_match,
                                  int last_match, const char *replace,
                                  const char reference_char,
                                  char *(*callback)(void *data, const char *text),
                                  void *callback_data)
{
    int length, length_current, length_add, match;
    const char *ptr_replace, *ptr_add;
    char *result, *result2, *modified_replace, *temp, char_replace;

    /* default length is length*2, it will grow later if needed */
    length = (strlen (string) * 2);
    result = malloc (length + 1);
    if (!result)
        return NULL;

    result[0] = '\0';
    length_current = 0;
    ptr_replace = replace;
    while (ptr_replace && ptr_replace[0])
    {
        ptr_add = NULL;
        length_add = 0;
        modified_replace = NULL;

        if ((ptr_replace[0] == '\\') && (ptr_replace[1] == reference_char))
        {
            /* escaped reference char */
            ptr_add = ptr_replace + 1;
            length_add = 1;
            ptr_replace += 2;
        }
        else if (ptr_replace[0] == reference_char)
        {
            if ((ptr_replace[1] == '+') || isdigit ((unsigned char)ptr_replace[1]))
            {
                if (ptr_replace[1] == '+')
                {
                    /* reference to last match */
                    match = last_match;
                    ptr_replace += 2;
                }
                else
                {
                    /* reference to match 0 .. 99 */
                    if (isdigit ((unsigned char)ptr_replace[2]))
                    {
                        match = ((ptr_replace[1] - '0') * 10) + (ptr_replace[2] - '0');
                        ptr_replace += 3;
                    }
                    else
                    {
                        match = ptr_replace[1] - '0';
                        ptr_replace += 2;
                    }
                }
                if (regex_match[match].rm_so >= 0)
                {
                    if (callback)
                    {
                        temp = string_strndup (string + regex_match[match].rm_so,
                                               regex_match[match].rm_eo - regex_match[match].rm_so);
                        if (temp)
                        {
                            modified_replace = (*callback) (callback_data, temp);
                            if (modified_replace)
                            {
                                ptr_add = modified_replace;
                                length_add = strlen (modified_replace);
                            }
                            free (temp);
                        }
                    }
                    if (!ptr_add)
                    {
                        ptr_add = string + regex_match[match].rm_so;
                        length_add = regex_match[match].rm_eo - regex_match[match].rm_so;
                    }
                }
            }
            else if ((ptr_replace[1] == '.')
                     && (ptr_replace[2] >= 32) && (ptr_replace[2] <= 126)
                     && ((ptr_replace[3] == '+') || isdigit ((unsigned char)ptr_replace[3])))
            {
                char_replace = ptr_replace[2];
                if (ptr_replace[3] == '+')
                {
                    /* reference to last match */
                    match = last_match;
                    ptr_replace += 4;
                }
                else
                {
                    /* reference to match 0 .. 99 */
                    if (isdigit ((unsigned char)ptr_replace[4]))
                    {
                        match = ((ptr_replace[3] - '0') * 10) + (ptr_replace[4] - '0');
                        ptr_replace += 5;
                    }
                    else
                    {
                        match = ptr_replace[3] - '0';
                        ptr_replace += 4;
                    }
                }
                if (regex_match[match].rm_so >= 0)
                {
                    temp = string_strndup (string + regex_match[match].rm_so,
                                           regex_match[match].rm_eo - regex_match[match].rm_so);
                    if (temp)
                    {
                        length_add = utf8_strlen (temp);
                        modified_replace = malloc (length_add + 1);
                        if (modified_replace)
                        {
                            memset (modified_replace, char_replace, length_add);
                            modified_replace[length_add] = '\0';
                            ptr_add = modified_replace;
                        }
                        free (temp);
                    }
                }
            }
            else
            {
                /* just ignore the reference char */
                ptr_replace++;
            }
        }
        else
        {
            ptr_add = ptr_replace;
            length_add = utf8_char_size (ptr_replace);
            ptr_replace += length_add;
        }

        if (ptr_add)
        {
            if (length_current + length_add > length)
            {
                length = (length * 2 >= length_current + length_add) ?
                    length * 2 : length_current + length_add;
                result2 = realloc (result, length + 1);
                if (!result2)
                {
                    free (modified_replace);
                    free (result);
                    return NULL;
                }
                result = result2;
            }
            memcpy (result + length_current, ptr_add, length_add);
            length_current += length_add;
            result[length_current] = '\0';
        }
        free (modified_replace);
    }

    return result;
}

/*
 * Replaces text in a string using a regular expression and replacement text.
 *
 * The argument "regex" is a pointer to a regex compiled with WeeChat function
 * string_regcomp (or function regcomp).
 *
 * The argument "replace" can contain references to matches:
 *   $0 .. $99  match 0 to 99 (0 is whole match, 1 .. 99 are groups captured)
 *   $+         the last match (with highest number)
 *   $.*N       match N (can be '+' or 0 to 99), with all chars replaced by '*'
 *              (the char '*' can be replaced by any char between space (32)
 *              and '~' (126))
 *
 * If the callback is not NULL, it is called for every reference to a match
 * (except for matches replaced by a char).
 * If not NULL, the string returned by the callback (which must have been newly
 * allocated) is used and freed after use.
 *
 * Examples:
 *
 *    string   | regex         | replace  | result
 *   ----------+---------------+----------+-------------
 *    test foo | test          | Z        | Z foo
 *    test foo | ^(test +)(.*) | $2       | foo
 *    test foo | ^(test +)(.*) | $1/ $.*2 | test / ***
 *    test foo | ^(test +)(.*) | $.%+     | %%%
 *
 * Note: result must be freed after use.
 */

char *
string_replace_regex (const char *string, void *regex, const char *replace,
                      const char reference_char,
                      char *(*callback)(void *data, const char *text),
                      void *callback_data)
{
    char *result, *result2, *str_replace;
    int length, length_replace, start_offset, i, rc, end, last_match;
    regmatch_t regex_match[100];

    if (!string || !regex)
        return NULL;

    result = strdup (string);

    start_offset = 0;
    while (result && result[start_offset])
    {
        for (i = 0; i < 100; i++)
        {
            regex_match[i].rm_so = -1;
        }

        rc = regexec ((regex_t *)regex, result + start_offset, 100, regex_match,
                      0);
        /*
         * no match found: exit the loop (if rm_eo == 0, it is an empty match
         * at beginning of string: we consider there is no match, to prevent an
         * infinite loop)
         */
        if ((rc != 0)
            || (regex_match[0].rm_so < 0) || (regex_match[0].rm_eo <= 0))
        {
            break;
        }

        /* adjust the start/end offsets */
        last_match = 0;
        for (i = 0; i < 100; i++)
        {
            if (regex_match[i].rm_so >= 0)
            {
                last_match = i;
                regex_match[i].rm_so += start_offset;
                regex_match[i].rm_eo += start_offset;
            }
        }

        /* check if the regex matched the end of string */
        end = !result[regex_match[0].rm_eo];

        str_replace = string_replace_regex_get_replace (result,
                                                        regex_match,
                                                        last_match,
                                                        replace,
                                                        reference_char,
                                                        callback,
                                                        callback_data);
        length_replace = (str_replace) ? strlen (str_replace) : 0;

        length = regex_match[0].rm_so + length_replace +
            strlen (result + regex_match[0].rm_eo) + 1;
        result2 = malloc (length);
        if (!result2)
        {
            free (result);
            return NULL;
        }
        result2[0] = '\0';
        if (regex_match[0].rm_so > 0)
        {
            memcpy (result2, result, regex_match[0].rm_so);
            result2[regex_match[0].rm_so] = '\0';
        }
        if (str_replace)
            strcat (result2, str_replace);
        strcat (result2, result + regex_match[0].rm_eo);

        free (result);
        result = result2;

        free (str_replace);

        if (end)
            break;

        start_offset = regex_match[0].rm_so + length_replace;
    }

    return result;
}

/*
 * Translates chars by other ones in a string.
 *
 * Note: result must be freed after use.
 */

char *
string_translate_chars (const char *string,
                        const char *chars1, const char *chars2)
{
    int length, length2, translated;
    const char *ptr_string, *ptr_chars1, *ptr_chars2;
    char **result;

    if (!string)
        return NULL;

    length = (chars1) ? utf8_strlen (chars1) : 0;
    length2 = (chars2) ? utf8_strlen (chars2) : 0;

    if (!chars1 || !chars2 || (length != length2))
        return strdup (string);

    result = string_dyn_alloc (strlen (string) + 1);
    if (!result)
        return strdup (string);

    ptr_string = string;
    while (ptr_string && ptr_string[0])
    {
        translated = 0;
        ptr_chars1 = chars1;
        ptr_chars2 = chars2;
        while (ptr_chars1 && ptr_chars1[0] && ptr_chars2 && ptr_chars2[0])
        {
            if (string_charcmp (ptr_chars1, ptr_string) == 0)
            {
                string_dyn_concat (result, ptr_chars2, utf8_char_size (ptr_chars2));
                translated = 1;
                break;
            }
            ptr_chars1 = utf8_next_char (ptr_chars1);
            ptr_chars2 = utf8_next_char (ptr_chars2);
        }
        if (!translated)
            string_dyn_concat (result, ptr_string, utf8_char_size (ptr_string));
        ptr_string = utf8_next_char (ptr_string);
    }

    return string_dyn_free (result, 0);
}

/*
 * Splits a string according to separators.
 *
 * This function must not be called directly (call string_split or
 * string_split_shared instead).
 *
 * Arguments:
 *   string: the string to split
 *   separators: the separators to split on (commonly just one char like " "
 *               or ",")
 *   strip_items: chars to strip from extracted items (left/right),
 *                for example " " when "separators" does not contain a space;
 *                this argument can be NULL (nothing is stripped)
 *   flags: combination of flags (see below)
 *   num_items_max: the max number of items to return (0 = no limit)
 *   num_items: if not NULL, the variable is set with the number of items
 *              returned
 *   shared: 1 if the strings are "shared strings" (created with the function
 *           string_shared_get), otherwise 0 for allocated strings
 *
 * The flags is a combination of flags:
 *   - WEECHAT_STRING_SPLIT_STRIP_LEFT: strip separators on the left
 *     (beginning of string)
 *   - WEECHAT_STRING_SPLIT_STRIP_RIGHT: strip separators on the right
 *     (end of string)
 *   - WEECHAT_STRING_SPLIT_COLLAPSE_SEPS: collapse multiple consecutive
 *     separators into a single one
 *   - WEECHAT_STRING_SPLIT_KEEP_EOL: keep end of line for each value
 *
 * Examples:
 *
 *   string_split ("abc de  fghi ", " ", NULL, 0, 0, &argc)
 *     ==> array[0] == "abc"
 *         array[1] == "de"
 *         array[2] == ""
 *         array[3] == "fghi"
 *         array[4] == ""
 *         array[5] == NULL
 *         argc == 5
 *
 *   string_split ("abc de  fghi ", " ", NULL,
 *                 WEECHAT_STRING_SPLIT_STRIP_LEFT
 *                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
 *                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
 *                 0, &argc)
 *     ==> array[0] == "abc"
 *         array[1] == "de"
 *         array[2] == "fghi"
 *         array[3] == NULL
 *         argc == 3
 *
 *   string_split ("abc de  fghi ", " ", NULL,
 *                 WEECHAT_STRING_SPLIT_STRIP_LEFT
 *                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
 *                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
 *                 | WEECHAT_STRING_SPLIT_KEEP_EOL,
 *                 0, &argc)
 *     ==> array[0] == "abc de  fghi"
 *         array[1] == "de  fghi"
 *         array[2] == "fghi"
 *         array[3] == NULL
 *         argc == 3
 *
 *   string_split ("abc de  fghi ", " ", NULL,
 *                 WEECHAT_STRING_SPLIT_STRIP_LEFT
 *                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
 *                 | WEECHAT_STRING_SPLIT_KEEP_EOL,
 *                 0, &argc)
 *     ==> array[0] == "abc de  fghi "
 *         array[1] == "de  fghi "
 *         array[2] == "fghi "
 *         array[3] == NULL
 *         argc == 3
 *
 *   string_split (",abc , de , fghi,", ",", NULL,
 *                 WEECHAT_STRING_SPLIT_STRIP_LEFT
 *                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
 *                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
 *                 | WEECHAT_STRING_SPLIT_KEEP_EOL,
 *                 0, &argc)
 *     ==> array[0] == "abc "
 *         array[1] == " de "
 *         array[2] == " fghi "
 *         array[3] == NULL
 *         argc == 3
 *
 *   string_split (",abc ,, de , fghi,", ",", " ",
 *                 WEECHAT_STRING_SPLIT_STRIP_LEFT
 *                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
 *                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
 *                 0, &argc)
 *     ==> array[0] == "abc"
 *         array[1] == "de"
 *         array[2] == "fghi"
 *         array[3] == NULL
 *         argc == 3
 */

char **
string_split_internal (const char *string, const char *separators,
                       const char *strip_items, int flags,
                       int num_items_max, int *num_items, int shared)
{
    int i, j, count_items;
    char *string2, **array, *temp_str, *ptr, *ptr1, *ptr2;
    const char *str_shared;

    if (num_items)
        *num_items = 0;

    if (!string || !string[0] || !separators || !separators[0])
        return NULL;

    string2 = string_strip (
        string,
        (flags & WEECHAT_STRING_SPLIT_STRIP_LEFT) ? 1 : 0,
        (flags & WEECHAT_STRING_SPLIT_STRIP_RIGHT) ? 1 : 0,
        separators);
    if (!string2)
        return NULL;
    if (!string2[0])
    {
        free (string2);
        return NULL;
    }

    /* calculate number of items */
    ptr = string2;
    i = 1;
    while ((ptr = strpbrk (ptr, separators)))
    {
        if (flags & WEECHAT_STRING_SPLIT_COLLAPSE_SEPS)
        {
            while (ptr[0] && strchr (separators, ptr[0]))
            {
                ptr++;
            }
            if (ptr[0])
                i++;
        }
        else
        {
            ptr++;
            i++;
        }
    }
    count_items = i;

    if ((num_items_max != 0) && (count_items > num_items_max))
        count_items = num_items_max;

    array = malloc ((count_items + 1) * sizeof (array[0]));
    if (!array)
    {
        free (string2);
        return NULL;
    }
    for (i = 0; i < count_items + 1; i++)
    {
        array[i] = NULL;
    }

    ptr1 = string2;

    for (i = 0; i < count_items; i++)
    {
        if (flags & WEECHAT_STRING_SPLIT_COLLAPSE_SEPS)
        {
            /* skip separators to find the beginning of item */
            while (ptr1[0] && strchr (separators, ptr1[0]))
            {
                ptr1++;
            }
        }
        /* search the end of item */
        if (i == (count_items - 1))
        {
            ptr2 = strpbrk (ptr1, separators);
            if (!ptr2)
                ptr2 = strchr (ptr1, '\0');
        }
        else
        {
            if ((ptr2 = strpbrk (ptr1, separators)) == NULL)
            {
                if ((ptr2 = strchr (ptr1, '\r')) == NULL)
                {
                    if ((ptr2 = strchr (ptr1, '\n')) == NULL)
                    {
                        ptr2 = strchr (ptr1, '\0');
                    }
                }
            }
        }

        if (!ptr1 || !ptr2)
        {
            array[i] = NULL;
        }
        else
        {
            if (ptr2 > ptr1)
            {
                if (flags & WEECHAT_STRING_SPLIT_KEEP_EOL)
                {
                    if (shared)
                    {
                        if (strip_items && strip_items[0])
                        {
                            temp_str = string_strip (ptr1, 1, 1, strip_items);
                            if (!temp_str)
                                goto error;
                            array[i] = (char *)string_shared_get (temp_str);
                            free (temp_str);
                        }
                        else
                        {
                            array[i] = (char *)string_shared_get (ptr1);
                        }
                    }
                    else
                    {
                        array[i] = (strip_items && strip_items[0]) ?
                            string_strip (ptr1, 1, 1, strip_items) :
                            strdup (ptr1);
                    }
                    if (!array[i])
                        goto error;
                }
                else
                {
                    array[i] = malloc (ptr2 - ptr1 + 1);
                    if (!array[i])
                        goto error;
                    strncpy (array[i], ptr1, ptr2 - ptr1);
                    array[i][ptr2 - ptr1] = '\0';
                    if (strip_items && strip_items[0])
                    {
                        temp_str = string_strip (array[i], 1, 1, strip_items);
                        if (!temp_str)
                            goto error;
                        free (array[i]);
                        array[i] = temp_str;
                    }
                    if (shared)
                    {
                        str_shared = string_shared_get (array[i]);
                        if (!str_shared)
                            goto error;
                        free (array[i]);
                        array[i] = (char *)str_shared;
                    }
                }
                if (!(flags & WEECHAT_STRING_SPLIT_COLLAPSE_SEPS)
                    && strchr (separators, ptr2[0]))
                {
                    ptr2++;
                }
                ptr1 = ptr2;
            }
            else
            {
                array[i] = (shared) ? (char *)string_shared_get ("") : strdup ("");
                if (ptr1[0] != '\0')
                    ptr1++;
            }
        }
    }

    array[i] = NULL;
    if (num_items)
        *num_items = i;

    free (string2);

    return array;

error:
    for (j = 0; j < count_items; j++)
    {
        if (array[j])
        {
            if (shared)
                string_shared_free (array[j]);
            else
                free (array[j]);
        }
    }
    free (array);
    free (string2);
    return NULL;
}

/*
 * Splits a string according to separators.
 *
 * For full description, see function string_split_internal.
 */

char **
string_split (const char *string, const char *separators,
              const char *strip_items, int flags,
              int num_items_max, int *num_items)
{
    return string_split_internal (string, separators, strip_items, flags,
                                  num_items_max, num_items, 0);
}

/*
 * Splits a string according to separators, and use shared strings for the
 * strings in the array returned.
 *
 * For full description, see function string_split_internal.
 */

char **
string_split_shared (const char *string, const char *separators,
                     const char *strip_items, int flags,
                     int num_items_max, int *num_items)
{
    return string_split_internal (string, separators, strip_items, flags,
                                  num_items_max, num_items, 1);
}

/*
 * Splits a string like the shell does for a command with arguments.
 *
 * This function is a C conversion of Python class "shlex"
 * (file: Lib/shlex.py in Python repository)
 * Doc: https://docs.python.org/3/library/shlex.html
 *
 * Copyrights in shlex.py:
 *   Module and documentation by Eric S. Raymond, 21 Dec 1998
 *   Input stacking and error message cleanup added by ESR, March 2000
 *   push_source() and pop_source() made explicit by ESR, January 2001.
 *   Posix compliance, split(), string arguments, and
 *   iterator interface by Gustavo Niemeyer, April 2003.
 *
 * Note: result must be freed after use with function string_free_split().
 */

char **
string_split_shell (const char *string, int *num_items)
{
    int temp_len, num_args, add_char_to_temp, add_temp_to_args, quoted;
    char *string2, *temp, **args, **args2, state, escapedstate;
    char *ptr_string, *ptr_next, saved_char;

    if (num_items)
        *num_items = 0;

    if (!string)
        return NULL;

    string2 = strdup (string);
    if (!string2)
        return NULL;

    /*
     * prepare "args" with one pointer to NULL, the "args" will be reallocated
     * later, each time a new argument is added
     */
    num_args = 0;
    args = malloc ((num_args + 1) * sizeof (args[0]));
    if (!args)
    {
        free (string2);
        return NULL;
    }
    args[0] = NULL;

    /* prepare a temp string for working (adding chars one by one) */
    temp = malloc ((2 * strlen (string)) + 1);
    if (!temp)
    {
        free (string2);
        free (args);
        return NULL;
    }
    temp[0] = '\0';
    temp_len = 0;

    state = ' ';
    escapedstate = ' ';
    quoted = 0;
    ptr_string = string2;
    while (ptr_string[0])
    {
        add_char_to_temp = 0;
        add_temp_to_args = 0;
        ptr_next = (char *)utf8_next_char (ptr_string);
        saved_char = ptr_next[0];
        ptr_next[0] = '\0';
        if (state == ' ')
        {
            if ((ptr_string[0] == ' ') || (ptr_string[0] == '\t')
                || (ptr_string[0] == '\r') || (ptr_string[0] == '\n'))
            {
                if (temp[0] || quoted)
                    add_temp_to_args = 1;
            }
            else if (ptr_string[0] == '\\')
            {
                escapedstate = 'a';
                state = ptr_string[0];
            }
            else if ((ptr_string[0] == '\'') || (ptr_string[0] == '"'))
            {
                state = ptr_string[0];
            }
            else
            {
                add_char_to_temp = 1;
                state = 'a';
            }
        }
        else if ((state == '\'') || (state == '"'))
        {
            quoted = 1;
            if (ptr_string[0] == state)
            {
                state = 'a';
            }
            else if ((state == '"') && (ptr_string[0] == '\\'))
            {
                escapedstate = state;
                state = ptr_string[0];
            }
            else
            {
                add_char_to_temp = 1;
            }
        }
        else if (state == '\\')
        {
            if (((escapedstate == '\'') || (escapedstate == '"'))
                && (ptr_string[0] != state) && (ptr_string[0] != escapedstate))
            {
                temp[temp_len] = state;
                temp_len++;
                temp[temp_len] = '\0';
            }
            add_char_to_temp = 1;
            state = escapedstate;
        }
        else if (state == 'a')
        {
            if ((ptr_string[0] == ' ') || (ptr_string[0] == '\t')
                || (ptr_string[0] == '\r') || (ptr_string[0] == '\n'))
            {
                state = ' ';
                if (temp[0] || quoted)
                    add_temp_to_args = 1;
            }
            else if (ptr_string[0] == '\\')
            {
                escapedstate = 'a';
                state = ptr_string[0];
            }
            else if ((ptr_string[0] == '\'') || (ptr_string[0] == '"'))
            {
                state = ptr_string[0];
            }
            else
            {
                add_char_to_temp = 1;
            }
        }
        if (add_char_to_temp)
        {
            memcpy (temp + temp_len, ptr_string, ptr_next - ptr_string);
            temp_len += (ptr_next - ptr_string);
            temp[temp_len] = '\0';
        }
        if (add_temp_to_args)
        {
            num_args++;
            args2 = realloc (args, (num_args + 1) * sizeof (args[0]));
            if (!args2)
            {
                free (string2);
                free (temp);
                return args;
            }
            args = args2;
            args[num_args - 1] = strdup (temp);
            args[num_args] = NULL;
            temp[0] = '\0';
            temp_len = 0;
            escapedstate = ' ';
            quoted = 0;
        }
        ptr_next[0] = saved_char;
        ptr_string = ptr_next;
    }

    if (temp[0] || (state != ' '))
    {
        num_args++;
        args2 = realloc (args, (num_args + 1) * sizeof (args[0]));
        if (!args2)
        {
            free (string2);
            free (temp);
            return args;
        }
        args = args2;
        args[num_args - 1] = strdup (temp);
        args[num_args] = NULL;
        temp[0] = '\0';
        /*temp_len = 0;*/
    }

    free (string2);
    free (temp);

    if (num_items)
        *num_items = num_args;

    return args;
}

/*
 * Frees a split string.
 */

void
string_free_split (char **split_string)
{
    char **ptr;

    if (split_string)
    {
        for (ptr = split_string; *ptr; ptr++)
            free (*ptr);
        free (split_string);
    }
}

/*
 * Frees a split string (using shared strings).
 */

void
string_free_split_shared (char **split_string)
{
    char **ptr;

    if (split_string)
    {
        for (ptr = split_string; *ptr; ptr++)
            string_shared_free (*ptr);
        free (split_string);
    }
}

/*
 * Rebuilds a split string using a delimiter and optional index of start/end
 * string.
 *
 * If index_end < 0, then all arguments are used until NULL is found.
 * If NULL is found before index_end, then the build stops there (at NULL).
 *
 * Note: result must be freed after use.
 */

char *
string_rebuild_split_string (const char **split_string,
                             const char *separator,
                             int index_start, int index_end)
{
    const char **ptr_string;
    char **result;
    int i;

    if (!split_string || (index_start < 0)
        || ((index_end >= 0) && (index_end < index_start)))
    {
        return NULL;
    }

    result = string_dyn_alloc (256);

    for (ptr_string = split_string, i = 0; *ptr_string; ptr_string++, i++)
    {
        if ((index_end >= 0) && (i > index_end))
            break;
        if (i >= index_start)
        {
            if (i > index_start)
                string_dyn_concat (result, separator, -1);
            string_dyn_concat (result, *ptr_string, -1);
        }
        if (i == INT_MAX)
            break;
    }

    return string_dyn_free (result, 0);
}

/*
 * Splits a list of commands separated by 'separator' and escaped with '\'.
 * Empty commands are removed, spaces on the left of each commands are stripped.
 *
 * Note: result must be freed after use with function
 * string_free_split_command().
 */

char **
string_split_command (const char *command, char separator)
{
    int nb_substr, arr_idx, str_idx, type;
    char **array, **array2;
    char *buffer, *p;
    const char *ptr;

    if (!command || !command[0])
        return NULL;

    nb_substr = 1;
    ptr = command;
    while ((p = strchr (ptr, separator)) != NULL)
    {
        nb_substr++;
        ptr = ++p;
    }

    array = malloc ((nb_substr + 1) * sizeof (array[0]));
    if (!array)
        return NULL;

    buffer = malloc (strlen (command) + 1);
    if (!buffer)
    {
        free (array);
        return NULL;
    }

    ptr = command;
    str_idx = 0;
    arr_idx = 0;
    while (*ptr != '\0')
    {
        type = 0;
        if (*ptr == separator)
        {
            if (ptr == command)
                type = 1;
            else if ( *(ptr-1) != '\\')
                type = 1;
            else if ( *(ptr-1) == '\\')
                type = 2;
        }
        if (type == 1)
        {
            buffer[str_idx] = '\0';
            str_idx = -1;
            p = buffer;
            /* strip white spaces at the beginning of the line */
            while (p[0] == ' ') p++;
            if (p[0])
                array[arr_idx++] = strdup (p);
        }
        else if (type == 2)
            buffer[--str_idx] = *ptr;
        else
            buffer[str_idx] = *ptr;
        str_idx++;
        ptr++;
    }

    buffer[str_idx] = '\0';
    p = buffer;
    while (p[0] == ' ') p++;
    if (p[0])
        array[arr_idx++] = strdup (p);

    array[arr_idx] = NULL;

    free (buffer);

    array2 = realloc (array, (arr_idx + 1) * sizeof (array[0]));
    if (!array2)
    {
        free (array);
        return NULL;
    }

    return array2;
}

/*
 * Frees a command split.
 */

void
string_free_split_command (char **split_command)
{
    char **ptr;

    if (split_command)
    {
        for (ptr = split_command; *ptr; ptr++)
            free (*ptr);
        free (split_command);
    }
}

/*
 * Splits tags in an array of tags.
 *
 * The format of tags is a list of tags separated by commas (logical OR),
 * and for each item, multiple tags can be separated by "+" (logical AND).
 *
 * For example:
 *   irc_join
 *   irc_join,irc_quit
 *   irc_join+nick_toto,irc_quit
 */

char ***
string_split_tags (const char *tags, int *num_tags)
{
    char ***tags_array, **tags_array_temp;
    int i, tags_count;

    tags_array = NULL;
    tags_count = 0;

    if (tags)
    {
        tags_array_temp = string_split (tags, ",", NULL,
                                        WEECHAT_STRING_SPLIT_STRIP_LEFT
                                        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                        0, &tags_count);
        if (tags_array_temp && (tags_count > 0))
        {
            tags_array = malloc ((tags_count + 1) * sizeof (*tags_array));
            if (tags_array)
            {
                for (i = 0; i < tags_count; i++)
                {
                    tags_array[i] = string_split_shared (tags_array_temp[i],
                                                         "+", NULL,
                                                         0, 0,
                                                         NULL);
                }
                tags_array[tags_count] = NULL;
            }
        }
        string_free_split (tags_array_temp);
    }

    if (num_tags)
        *num_tags = tags_count;

    return tags_array;
}

/*
 * Frees tags split.
 */

void
string_free_split_tags (char ***split_tags)
{
    char ***ptr;

    if (split_tags)
    {
        for (ptr = split_tags; *ptr; ptr++)
            string_free_split_shared (*ptr);
        free (split_tags);
    }
}

/*
 * Converts a string to another charset.
 *
 * Note: result must be freed after use.
 */

char *
string_iconv (int from_utf8, const char *from_code, const char *to_code,
              const char *string)
{
    char *outbuf;

#ifdef HAVE_ICONV
    iconv_t cd;
    char *inbuf, *ptr_outbuf;
    const char *ptr_inbuf, *ptr_inbuf_shift, *next_char;
    int done;
    size_t err, inbytesleft, outbytesleft;
#endif /* HAVE_ICONV */

    if (!string)
        return NULL;

#ifdef HAVE_ICONV
    if (from_code && from_code[0] && to_code && to_code[0]
        && (string_strcasecmp (from_code, to_code) != 0))
    {
        cd = iconv_open (to_code, from_code);
        if (cd == (iconv_t)(-1))
            outbuf = strdup (string);
        else
        {
            inbuf = strdup (string);
            if (!inbuf)
                return NULL;
            ptr_inbuf = inbuf;
            inbytesleft = strlen (inbuf);
            outbytesleft = inbytesleft * 4;
            outbuf = malloc (outbytesleft + 2);
            if (!outbuf)
                return inbuf;
            ptr_outbuf = outbuf;
            ptr_inbuf_shift = NULL;
            done = 0;
            while (!done)
            {
                err = iconv (cd, (ICONV_CONST char **)(&ptr_inbuf), &inbytesleft,
                             &ptr_outbuf, &outbytesleft);
                if (err == (size_t)(-1))
                {
                    switch (errno)
                    {
                        case EINVAL:
                            done = 1;
                            break;
                        case E2BIG:
                            done = 1;
                            break;
                        case EILSEQ:
                            if (from_utf8)
                            {
                                next_char = utf8_next_char (ptr_inbuf);
                                if (next_char)
                                {
                                    inbytesleft -= next_char - ptr_inbuf;
                                    ptr_inbuf = next_char;
                                }
                                else
                                {
                                    inbytesleft--;
                                    ptr_inbuf++;
                                }
                            }
                            else
                            {
                                ptr_inbuf++;
                                inbytesleft--;
                            }
                            ptr_outbuf[0] = '?';
                            ptr_outbuf++;
                            outbytesleft--;
                            break;
                    }
                }
                else
                {
                    if (!ptr_inbuf_shift)
                    {
                        ptr_inbuf_shift = ptr_inbuf;
                        ptr_inbuf = NULL;
                        inbytesleft = 0;
                    }
                    else
                        done = 1;
                }
            }
            if (ptr_inbuf_shift)
                ptr_inbuf = ptr_inbuf_shift;
            ptr_outbuf[0] = '\0';
            free (inbuf);
            iconv_close (cd);
        }
    }
    else
        outbuf = strdup (string);
#else
    /* make C compiler happy */
    (void) from_utf8;
    (void) from_code;
    (void) to_code;
    outbuf = strdup (string);
#endif /* HAVE_ICONV */

    return outbuf;
}

/*
 * Converts a string to WeeChat internal storage charset (UTF-8).
 *
 * Note: result must be freed after use.
 */

char *
string_iconv_to_internal (const char *charset, const char *string)
{
    char *input, *output;

    if (!string)
        return NULL;

    input = strdup (string);
    if (!input)
        return NULL;

    /*
     * optimized for UTF-8: if charset is NULL => we use term charset => if
     * this charset is already UTF-8, then no iconv is needed
     */
    if (local_utf8 && (!charset || !charset[0]))
        return input;

    if (utf8_has_8bits (input) && utf8_is_valid (input, -1, NULL))
        return input;

    output = string_iconv (0,
                           (charset && charset[0]) ?
                           charset : weechat_local_charset,
                           WEECHAT_INTERNAL_CHARSET,
                           input);
    if (!output)
        return input;
    utf8_normalize (output, '?');
    free (input);
    return output;
}

/*
 * Converts internal string to terminal charset, for display or write of
 * configuration files.
 *
 * Note: result must be freed after use.
 */

char *
string_iconv_from_internal (const char *charset, const char *string)
{
    char *input, *output;

    if (!string)
        return NULL;

    input = strdup (string);
    if (!input)
        return NULL;

    /* if the locale is wrong, we keep UTF-8 */
    if (!weechat_locale_ok)
        return input;

    /*
     * optimized for UTF-8: if charset is NULL => we use term charset => if
     * this charset is already UTF-8, then no iconv is needed
     */
    if (local_utf8 && (!charset || !charset[0]))
        return input;

    utf8_normalize (input, '?');
    output = string_iconv (1,
                           WEECHAT_INTERNAL_CHARSET,
                           (charset && charset[0]) ?
                           charset : weechat_local_charset,
                           input);
    if (!output)
        return input;
    free (input);
    return output;
}

/*
 * Encodes a string to terminal charset and calls fprintf.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
string_fprintf (FILE *file, const char *data, ...)
{
    char *buf2;
    int rc, num_written;

    rc = 0;

    if (!data)
        return rc;

    weechat_va_format (data);
    if (vbuffer)
    {
        buf2 = string_iconv_from_internal (NULL, vbuffer);
        num_written = fprintf (file, "%s", (buf2) ? buf2 : vbuffer);
        rc = (num_written == (int)strlen ((buf2) ? buf2 : vbuffer)) ? 1 : 0;
        free (buf2);
        free (vbuffer);
    }

    return rc;
}

/*
 * Formats a string with size and unit name (bytes, KB, MB, GB).
 *
 * Note: result must be freed after use.
 */

char *
string_format_size (unsigned long long size)
{
    char *unit_name[] = { "",
                          /* TRANSLATORS: file size unit "kilobyte" */
                          N_("KB"),
                          /* TRANSLATORS: file size unit "megabyte" */
                          N_("MB"),
                          /* TRANSLATORS: file size unit "gigabyte" */
                          N_("GB"),
                          /* TRANSLATORS: file size unit "terabyte" */
                          N_("TB") };
    char *unit_format[] = { "%.0f", "%.1f", "%.02f", "%.02f", "%.02f" };
    float unit_divide[] = { 1.0,
                            1000.0,
                            1000.0 * 1000.0,
                            1000.0 * 1000.0 * 1000.0,
                            1000.0 * 1000.0 * 1000.0 * 1000.0 };
    char format_size[128], str_size[128];
    int num_unit;
    float size_float;

    str_size[0] = '\0';

    if (size < 10ULL * 1000ULL)
        num_unit = 0;
    else if (size < 1000ULL * 1000ULL)
        num_unit = 1;
    else if (size < 1000ULL * 1000ULL * 1000ULL)
        num_unit = 2;
    else if (size < 1000ULL * 1000ULL * 1000ULL * 1000ULL)
        num_unit = 3;
    else
        num_unit = 4;

    snprintf (format_size, sizeof (format_size),
              "%s %%s",
              unit_format[num_unit]);
    size_float = ((float)size) / ((float)(unit_divide[num_unit]));
    snprintf (str_size, sizeof (str_size),
              format_size,
              size_float,
              (num_unit == 0) ?
              NG_("byte", "bytes", size_float) : _(unit_name[num_unit]));

    return strdup (str_size);
}

/*
 * Parses a string with a size and returns the size in bytes.
 *
 * The format is "123" or "123x" or "123 x"  where "123" is any positive
 * integer number and "x" the unit, which can be one of (lower or upper case
 * are accepted):
 *
 *   b  bytes (default if unit is missing)
 *   k  kilobytes (1k = 1000 bytes)
 *   m  megabytes (1m = 1000k = 1,000,000 bytes)
 *   g  gigabytes (1g = 1000m = 1,000,000,000 bytes)
 *   t  terabytes (1t = 1000g = 1,000,000,000,000 bytes)
 *
 * Returns the parsed size, 0 if error.
 */

unsigned long long
string_parse_size (const char *size)
{
    const char *pos;
    char *str_number, *error;
    long long number;
    unsigned long long result;

    str_number = NULL;
    result = 0;

    if (!size || !size[0])
        goto end;

    pos = size;
    while (isdigit ((unsigned char)pos[0]))
    {
        pos++;
    }

    if (pos == size)
        goto end;

    str_number = string_strndup (size, pos - size);
    if (!str_number)
        goto end;

    error = NULL;
    number = strtoll (str_number, &error, 10);
    if (!error || error[0])
        goto end;
    if (number < 0)
        goto end;

    while (pos[0] == ' ')
    {
        pos++;
    }

    if (pos[0] && pos[1])
        goto end;

    switch (pos[0])
    {
        case '\0':
            result = number;
            break;
        case 'b':
        case 'B':
            result = number;
            break;
        case 'k':
        case 'K':
            result = number * 1000ULL;
            break;
        case 'm':
        case 'M':
            result = number * 1000ULL * 1000ULL;
            break;
        case 'g':
        case 'G':
            result = number * 1000ULL * 1000ULL * 1000ULL;
            break;
        case 't':
        case 'T':
            result = number * 1000ULL * 1000ULL * 1000ULL * 1000ULL;
            break;
    }

end:
    free (str_number);
    return result;
}

/*
 * Encodes a string in base16 (hexadecimal).
 *
 * Argument "length" is number of bytes in "from" to convert (commonly
 * strlen(from)).
 *
 * Returns length of string in "*to" (it does not count final \0),
 * -1 if error.
 */

int
string_base16_encode (const char *from, int length, char *to)
{
    int i, count;
    const char *hexa = "0123456789ABCDEF";

    if (!from || !to)
        return -1;

    count = 0;

    for (i = 0; i < length; i++)
    {
        to[count++] = hexa[((unsigned char)from[i]) / 16];
        to[count++] = hexa[((unsigned char)from[i]) % 16];
    }
    to[count] = '\0';

    return count;
}

/*
 * Decodes a base16 string (hexadecimal).
 *
 * Returns length of string in "*to" (it does not count final \0),
 * -1 if error.
 */

int
string_base16_decode (const char *from, char *to)
{
    int length, i, pos, count;
    unsigned char value;

    if (!from || !to)
        return -1;

    count = 0;

    length = strlen (from) / 2;

    for (i = 0; i < length; i++)
    {
        pos = i * 2;
        value = 0;
        /* 4 bits on the left */
        if ((from[pos] >= '0') && (from[pos] <= '9'))
            value |= (from[pos] - '0') << 4;
        else if ((from[pos] >= 'a') && (from[pos] <= 'f'))
            value |= (from[pos] - 'a' + 10) << 4;
        else if ((from[pos] >= 'A') && (from[pos] <= 'F'))
            value |= (from[pos] - 'A' + 10) << 4;
        /* 4 bits on the right */
        pos++;
        if ((from[pos] >= '0') && (from[pos] <= '9'))
            value |= from[pos] - '0';
        else if ((from[pos] >= 'a') && (from[pos] <= 'f'))
            value |= from[pos] - 'a' + 10;
        else if ((from[pos] >= 'A') && (from[pos] <= 'F'))
            value |= from[pos] - 'A' + 10;

        to[count++] = value;
    }
    to[count] = '\0';

    return count;
}

/*
 * Encodes a string in base32.
 *
 * Argument "length" is number of bytes in "from" to convert (commonly
 * strlen(from)).
 *
 * This function is inspired by:
 *   https://github.com/google/google-authenticator-libpam/blob/master/src/base32.c
 *
 * Original copyright:
 *
 *   Copyright 2010 Google Inc.
 *   Author: Markus Gutschke
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *        https://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 * Returns length of string in "*to" (it does not count final \0),
 * -1 if error.
 */

int
string_base32_encode (const char *from, int length, char *to)
{
    unsigned char base32_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int count, value, next, bits_left, pad, index;
    int length_padding[8] = { 0, 0, 6, 0, 4, 3, 0, 2 };

    if (!from || !to)
        return -1;

    count = 0;

    if (length > 0)
    {
        value = from[0];
        next = 1;
        bits_left = 8;
        while ((bits_left > 0) || (next < length))
        {
            if (bits_left < 5)
            {
                if (next < length)
                {
                    value <<= 8;
                    value |= from[next++] & 0xFF;
                    bits_left += 8;
                }
                else
                {
                    pad = 5 - bits_left;
                    value <<= pad;
                    bits_left += pad;
                }
            }
            index = 0x1F & (value >> (bits_left - 5));
            bits_left -= 5;
            to[count++] = base32_table[index];
        }
    }
    pad = length_padding[count % 8];
    while (pad > 0)
    {
        to[count++] = '=';
        pad--;
    }
    to[count] = '\0';

    return count;
}

/*
 * Decodes a base32 string.
 *
 * This function is inspired by:
 *   https://github.com/google/google-authenticator-libpam/blob/master/src/base32.c
 *
 * Original copyright:
 *
 *   Copyright 2010 Google Inc.
 *   Author: Markus Gutschke
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *        https://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *
 * Returns length of string in "*to" (it does not count final \0),
 * -1 if error.
 */

int
string_base32_decode (const char *from, char *to)
{
    const char *ptr_from;
    int value, bits_left, count;
    unsigned char c;

    if (!from || !to)
        return -1;

    ptr_from = from;
    value = 0;
    bits_left = 0;
    count = 0;

    while (ptr_from[0])
    {
        c = (unsigned char)ptr_from[0];
        value <<= 5;
        if (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')))
        {
            c = (c & 0x1F) - 1;
        }
        else if ((c >= '2') && (c <= '7'))
        {
            c -= '2' - 26;
        }
        else if (c == '=')
        {
            /* padding */
            break;
        }
        else
        {
            /* invalid base32 char */
            return -1;
        }
        value |= c;
        bits_left += 5;
        if (bits_left >= 8)
        {
            to[count++] = value >> (bits_left - 8);
            bits_left -= 8;
        }
        ptr_from++;
    }
    to[count] = '\0';

    return count;
}

/*
 * Converts 3 bytes of 8 bits in 4 bytes of 6 bits.
 */

void
string_convbase64_8x3_to_6x4 (int url, const char *from, char *to)
{
    unsigned char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char base64_table_url[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz0123456789-_";
    const unsigned char *ptr_table;

    ptr_table = (url) ? base64_table_url : base64_table;

    to[0] = ptr_table [ (from[0] & 0xfc) >> 2 ];
    to[1] = ptr_table [ ((from[0] & 0x03) << 4) + ((from[1] & 0xf0) >> 4) ];
    to[2] = ptr_table [ ((from[1] & 0x0f) << 2) + ((from[2] & 0xc0) >> 6) ];
    to[3] = ptr_table [ from[2] & 0x3f ];
}

/*
 * Encodes a string in base64.
 *
 * If url == 1, base64url is decoded, otherwise standard base64.
 *
 * Base64url is the same as base64 with these chars replaced:
 *   “+” --> “-” (minus)
 *   "/" --> “_” (underline)
 *   no padding char ("=")
 *
 * Argument "length" is number of bytes in "from" to convert (commonly
 * strlen(from)).
 *
 * Returns length of string in "*to" (it does not count final \0),
 * -1 if error.
 */

int
string_base64_encode (int url, const char *from, int length, char *to)
{
    const char *ptr_from;
    char rest[3];
    int count;

    if (!from || !to)
        return -1;

    ptr_from = from;
    count = 0;

    while (length >= 3)
    {
        string_convbase64_8x3_to_6x4 (url, ptr_from, to + count);
        ptr_from += 3;
        count += 4;
        length -= 3;
    }

    if (length > 0)
    {
        rest[0] = 0;
        rest[1] = 0;
        rest[2] = 0;
        switch (length)
        {
            case 1 :
                rest[0] = ptr_from[0];
                string_convbase64_8x3_to_6x4 (url, rest, to + count);
                count += 2;
                if (!url)
                {
                    to[count] = '=';
                    count++;
                    to[count] = '=';
                    count++;
                }
                break;
            case 2 :
                rest[0] = ptr_from[0];
                rest[1] = ptr_from[1];
                string_convbase64_8x3_to_6x4 (url, rest, to + count);
                count += 3;
                if (!url)
                {
                    to[count] = '=';
                    count++;
                }
                break;
        }
        to[count] = '\0';
    }
    else
        to[count] = '\0';

    return count;
}

/*
 * Converts 4 bytes of 6 bits to 3 bytes of 8 bits.
 */

void
string_convbase64_6x4_to_8x3 (const unsigned char *from, unsigned char *to)
{
    to[0] = from[0] << 2 | from[1] >> 4;
    to[1] = from[1] << 4 | from[2] >> 2;
    to[2] = from[2] << 6 | from[3];
}

/*
 * Decodes a base64 string.
 *
 * If url == 1, base64url is decoded, otherwise standard base64.
 *
 * Base64url is the same as base64 with these chars replaced:
 *   “+” --> “-” (minus)
 *   "/" --> “_” (underline)
 *   no padding char ("=")
 *
 * Returns length of string in "*to" (it does not count final \0),
 * -1 if error.
 */

int
string_base64_decode (int url, const char *from, char *to)
{
    const char *ptr_from;
    int length, to_length, i;
    char *ptr_to;
    unsigned char c, in[4], out[3];
    unsigned char base64_table[] = "|$$$}rstuvwxyz{$$$$$$$>?"
        "@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

    if (!from || !to)
        return -1;

    ptr_from = from;
    ptr_to = to;

    ptr_to[0] = '\0';
    to_length = 0;

    while (ptr_from && ptr_from[0])
    {
        length = 0;
        in[0] = 0;
        in[1] = 0;
        in[2] = 0;
        in[3] = 0;
        for (i = 0; i < 4; i++)
        {
            if (!ptr_from[0])
                break;
            c = (unsigned char) ptr_from[0];
            if (url && (c == '-'))
                c = '+';
            else if (url && (c == '_'))
                c = '/';
            ptr_from++;
            c = ((c < 43) || (c > 122)) ? 0 : base64_table[c - 43];
            if (c)
                c = (c == '$') ? 0 : c - 61;
            if (c)
            {
                length++;
                in[i] = c - 1;
            }
            else
                break;
        }
        if (length > 0)
        {
            string_convbase64_6x4_to_8x3 (in, out);
            for (i = 0; i < length - 1; i++)
            {
                ptr_to[0] = out[i];
                ptr_to++;
                to_length++;
            }
        }
    }

    ptr_to[0] = '\0';

    return to_length;
}

/*
 * Encodes a string, according to "base" parameter:
 *   - "16": base16
 *   - "32": base32
 *   - "64": base64
 *   - "64url": base64url: same as base64 with no padding ("="), chars replaced:
 *              “+” --> “-” and "/" --> “_”
 *
 * Returns length of string in "*to" (it does not count final \0),
 * -1 if error.
 */

int
string_base_encode (const char *base, const char *from, int length, char *to)
{
    if (!base || !from || (length <= 0) || !to)
        return -1;

    if (strcmp (base, "16") == 0)
        return string_base16_encode (from, length, to);

    if (strcmp (base, "32") == 0)
        return string_base32_encode (from, length, to);

    if (strcmp (base, "64") == 0)
        return string_base64_encode (0, from, length, to);

    if (strcmp (base, "64url") == 0)
        return string_base64_encode (1, from, length, to);

    return -1;
}

/*
 * Decodes a string, according to "base" parameter:
 *   - "16": base16
 *   - "32": base32
 *   - "64": base64
 *   - "64url": base64url: same as base64 with no padding ("="), chars replaced:
 *              “+” --> “-” and "/" --> “_”
 *
 * Returns length of string in "*to" (it does not count final \0),
 * -1 if error.
 */

int
string_base_decode (const char *base, const char *from, char *to)
{
    if (!base || !from || !to)
        return -1;

    if (strcmp (base, "16") == 0)
        return string_base16_decode (from, to);

    if (strcmp (base, "32") == 0)
        return string_base32_decode (from, to);

    if (strcmp (base, "64") == 0)
        return string_base64_decode (0, from, to);

    if (strcmp (base, "64url") == 0)
        return string_base64_decode (1, from, to);

    return -1;
}

/*
 * Dumps a data buffer as hexadecimal + ascii.
 *
 * Note: result must be freed after use.
 */

char *
string_hex_dump (const char *data, int data_size, int bytes_per_line,
                 const char *prefix, const char *suffix)
{
    char *buf, *str_hexa, *str_ascii, str_format_line[64], *str_line;
    int length_hexa, length_ascii, length_prefix, length_suffix, length_line;
    int hexa_pos, ascii_pos, i;

    if (!data || (data_size < 1) || (bytes_per_line < 1))
        return NULL;

    str_hexa = NULL;
    str_ascii = NULL;
    str_line = NULL;
    buf = NULL;

    length_hexa = bytes_per_line * 3;
    str_hexa = malloc (length_hexa + 1);
    if (!str_hexa)
        goto end;

    length_ascii = bytes_per_line * 2;
    str_ascii = malloc (length_ascii + 1);
    if (!str_ascii)
        goto end;

    length_prefix = (prefix) ? strlen (prefix) : 0;
    length_suffix = (suffix) ? strlen (suffix) : 0;

    length_line = length_prefix + (bytes_per_line * 3) + 2 + length_ascii +
        length_suffix;
    str_line = malloc (length_line + 1);
    if (!str_line)
        goto end;

    buf = malloc ((((data_size / bytes_per_line) + 1) * (length_line + 1)) + 1);
    if (!buf)
        goto end;
    buf[0] = '\0';

    snprintf (str_format_line, sizeof (str_format_line),
              "%%s%%-%ds  %%-%ds%%s",
              length_hexa,
              length_ascii);

    hexa_pos = 0;
    ascii_pos = 0;
    for (i = 0; i < data_size; i++)
    {
        snprintf (str_hexa + hexa_pos, 4,
                  "%02X ", (unsigned char)(data[i]));
        hexa_pos += 3;
        snprintf (str_ascii + ascii_pos, 3, "%c ",
                  ((((unsigned char)data[i]) < 32)
                   || (((unsigned char)data[i]) > 127)) ?
                  '.' : (unsigned char)(data[i]));
        ascii_pos += 2;
        if (ascii_pos == bytes_per_line * 2)
        {
            if (buf[0])
                strcat (buf, "\n");
            str_ascii[ascii_pos - 1] = '\0';
            snprintf (str_line, length_line + 1,
                      str_format_line,
                      (prefix) ? prefix : "",
                      str_hexa,
                      str_ascii,
                      (suffix) ? suffix : "");
            strcat (buf, str_line);
            hexa_pos = 0;
            ascii_pos = 0;
        }
    }
    if (ascii_pos > 0)
    {
        if (buf[0])
            strcat (buf, "\n");
        str_ascii[ascii_pos - 1] = '\0';
        str_ascii[ascii_pos] = '\0';
        snprintf (str_line, length_line + 1,
                  str_format_line,
                  (prefix) ? prefix : "",
                  str_hexa,
                  str_ascii,
                  (suffix) ? suffix : "");
        strcat (buf, str_line);
    }

end:
    free (str_hexa);
    free (str_ascii);
    free (str_line);
    return buf;
}

/*
 * Checks if a string is a command.
 *
 * Returns:
 *   1: first char of string is a command char
 *   0: string is not a command
 */

int
string_is_command_char (const char *string)
{
    const char *ptr_command_chars;

    if (!string)
        return 0;

    if (string[0] == '/')
        return 1;

    ptr_command_chars = CONFIG_STRING(config_look_command_chars);
    if (!ptr_command_chars || !ptr_command_chars[0])
        return 0;

    while (ptr_command_chars && ptr_command_chars[0])
    {
        if (string_charcmp (ptr_command_chars, string) == 0)
            return 1;
        ptr_command_chars = utf8_next_char (ptr_command_chars);
    }

    return 0;
}

/*
 * Gets pointer to input text for buffer.
 *
 * Returns pointer inside "string" argument or NULL if it's a command (by
 * default a command starts with a single '/').
 */

const char *
string_input_for_buffer (const char *string)
{
    char *pos_slash, *pos_space, *pos_newline;
    const char *next_char;

    if (!string)
        return NULL;

    /* "/ " is not a command */
    if (strncmp (string, "/ ", 2) == 0)
        return string;

    /* special case for C comments pasted in input line */
    if (strncmp (string, "/*", 2) == 0)
        return string;

    /*
     * special case if string starts with '/': to allow to paste a path line
     * "/path/to/file.txt", we check if next '/' is after a space/newline
     * or not
     */
    if (string[0] == '/')
    {
        pos_slash = strchr (string + 1, '/');
        pos_space = strchr (string + 1, ' ');
        pos_newline = strchr (string + 1, '\n');

        /*
         * if there are no other '/' or if '/' is after first space/newline,
         * then it is a command, and return NULL
         */
        if (!pos_slash
            || (pos_space && (pos_slash > pos_space))
            || (pos_newline && (pos_slash > pos_newline)))
            return NULL;

        return (string[1] == '/') ? string + 1 : string;
    }

    /* if string does not start with a command char, then it's not command */
    if (!string_is_command_char (string))
        return string;

    next_char = utf8_next_char (string);

    /* next char is a space, then it's not a command */
    if (next_char[0] == ' ')
        return string;

    /* check if first char is doubled: if yes, then it's not a command */
    if (string_charcmp (string, next_char) == 0)
        return next_char;

    /* string is a command */
    return NULL;
}

/*
 * Returns the number of bytes in common between two strings (this function
 * works with bytes and not UTF-8 chars).
 */

int
string_get_common_bytes_count (const char *string1, const char *string2)
{
    const char *ptr_str1;
    int found;

    if (!string1 || !string2)
        return 0;

    found = 0;
    ptr_str1 = string1;
    while (ptr_str1[0])
    {
        if (strchr (string2, ptr_str1[0]))
            found++;
        ptr_str1++;
    }
    return found;
}

/*
 * Returns the distance between two strings using the Levenshtein algorithm.
 * See: https://en.wikipedia.org/wiki/Levenshtein_distance
 */

int
string_levenshtein (const char *string1, const char *string2,
                    int case_sensitive)
{
    int x, y, length1, length2, last_diag, old_diag;
    wint_t char1, char2;
    const char *ptr_str1, *ptr_str2;

    length1 = (string1) ? utf8_strlen (string1) : 0;
    length2 = (string2) ? utf8_strlen (string2) : 0;

    if (length1 == 0)
        return length2;
    if (length2 == 0)
        return length1;

    int column[length1 + 1];

    for (y = 1; y <= length1; y++)
    {
        column[y] = y;
    }

    ptr_str2 = string2;

    for (x = 1; x <= length2; x++)
    {
        char2 = (case_sensitive) ?
            (wint_t)utf8_char_int (ptr_str2) :
            towlower (utf8_char_int (ptr_str2));
        column[0] = x;
        ptr_str1 = string1;
        for (y = 1, last_diag = x - 1; y <= length1; y++)
        {
            char1 = (case_sensitive) ?
                (wint_t)utf8_char_int (ptr_str1) :
                towlower (utf8_char_int (ptr_str1));
            old_diag = column[y];
            column[y] = MIN3(
                column[y] + 1,
                column[y - 1] + 1,
                last_diag + ((char1 == char2) ? 0 : 1));
            last_diag = old_diag;
            ptr_str1 = utf8_next_char (ptr_str1);
        }
        ptr_str2 = utf8_next_char (ptr_str2);
    }

    return column[length1];
}

/*
 * Replaces ${vars} using a callback that returns replacement value (this value
 * must be newly allocated because it will be freed in this function).
 *
 * Nested variables are supported, for example: "${var1:${var2}}".
 *
 * If allow_escape == 1, the prefix/suffix can be escaped with a backslash
 * (which is then omitted in the result).
 * If allow_escape == 0, the backslash is kept as-is and cannot be
 * used to escape the prefix/suffix.
 *
 * Argument "list_prefix_no_replace" is a list to prevent replacements in
 * string if beginning with one of the prefixes. For example if the list is
 * { "if:", NULL } and string is: "${if:cond?true:false}${test${abc}}"
 * then the "if:cond?true:false" is NOT replaced (via a recursive call) but
 * "test${abc}" will be replaced.
 *
 * Argument "errors" (if not NULL) is set with number of keys not found by
 * callback.
 *
 * Note: result must be freed after use.
 */

char *
string_replace_with_callback (const char *string,
                              const char *prefix,
                              const char *suffix,
                              int allow_escape,
                              const char **list_prefix_no_replace,
                              char *(*callback)(void *data,
                                                const char *prefix,
                                                const char *text,
                                                const char *suffix),
                              void *callback_data,
                              int *errors)
{
    int length_prefix, length_suffix, length, length_value, index_string;
    int index_result, sub_count, sub_level, sub_errors, replace, i;
    char *result, *result2, *key, *key2, *value;
    const char *pos_end_name;

    if (errors)
        *errors = 0;

    if (!string || !prefix || !prefix[0] || !suffix || !suffix[0] || !callback)
        return NULL;

    length_prefix = strlen (prefix);
    length_suffix = strlen (suffix);

    length = strlen (string) + 1;
    result = malloc (length);
    if (result)
    {
        index_string = 0;
        index_result = 0;
        while (string[index_string])
        {
            if ((string[index_string] == '\\')
                && (string[index_string + 1] == prefix[0]))
            {
                if (allow_escape)
                {
                    index_string++;
                    result[index_result++] = string[index_string++];
                }
                else
                {
                    result[index_result++] = string[index_string++];
                    result[index_result++] = string[index_string++];
                }
            }
            else if (strncmp (string + index_string, prefix, length_prefix) == 0)
            {
                sub_count = 0;
                sub_level = 0;
                pos_end_name = string + index_string + length_prefix;
                while (pos_end_name[0])
                {
                    if (strncmp (pos_end_name, suffix, length_suffix) == 0)
                    {
                        if (sub_level == 0)
                            break;
                        sub_level--;
                    }
                    if (allow_escape
                        && (pos_end_name[0] == '\\')
                        && (pos_end_name[1] == prefix[0]))
                    {
                        pos_end_name++;
                    }
                    else if (strncmp (pos_end_name, prefix, length_prefix) == 0)
                    {
                        sub_count++;
                        sub_level++;
                    }
                    pos_end_name++;
                }
                key = string_strndup (string + index_string + length_prefix,
                                      pos_end_name - (string + index_string + length_prefix));
                if (key)
                {
                    if (sub_count > 0)
                    {
                        replace = 1;
                        if (list_prefix_no_replace)
                        {
                            for (i = 0; list_prefix_no_replace[i]; i++)
                            {
                                if (strncmp (
                                        key, list_prefix_no_replace[i],
                                        strlen (list_prefix_no_replace[i])) == 0)
                                {
                                    replace = 0;
                                    break;
                                }
                            }
                        }
                        if (replace)
                        {
                            sub_errors = 0;
                            key2 = string_replace_with_callback (
                                key,
                                prefix,
                                suffix,
                                1,
                                list_prefix_no_replace,
                                callback,
                                callback_data,
                                &sub_errors);
                            if (errors)
                                (*errors) += sub_errors;
                            free (key);
                            key = key2;
                        }
                    }
                    value = (*callback) (callback_data,
                                         prefix,
                                         (key) ? key : "",
                                         (pos_end_name[0]) ? suffix : "");
                    if (value)
                    {
                        length_value = strlen (value);
                        if (length_value > 0)
                        {
                            length += length_value;
                            result2 = realloc (result, length);
                            if (!result2)
                            {
                                free (result);
                                free (key);
                                free (value);
                                return NULL;
                            }
                            result = result2;
                            strcpy (result + index_result, value);
                            index_result += length_value;
                        }
                        if (pos_end_name[0])
                            index_string = pos_end_name - string + length_suffix;
                        else
                            index_string = pos_end_name - string;
                        free (value);
                    }
                    else
                    {
                        result[index_result++] = string[index_string++];
                        if (errors)
                            (*errors)++;
                    }
                    free (key);
                }
                else
                    result[index_result++] = string[index_string++];
            }
            else
                result[index_result++] = string[index_string++];
        }
        result[index_result] = '\0';
    }

    return result;
}

/*
 * Extracts priority and name from a string.
 *
 * String can be:
 *   - a simple name like "test":
 *       => priority = default_priority, name = "test"
 *   - a priority + "|" + name, like "500|test":
 *       => priority = 500, name = "test"
 */

void
string_get_priority_and_name (const char *string,
                              int *priority, const char **name,
                              int default_priority)
{
    char *pos, *str_priority, *error;
    long number;

    if (priority)
        *priority = default_priority;
    if (name)
        *name = string;

    if (!string)
        return;

    pos = strchr (string, '|');
    if (pos)
    {
        str_priority = string_strndup (string, pos - string);
        if (str_priority)
        {
            error = NULL;
            number = strtol (str_priority, &error, 10);
            if (error && !error[0])
            {
                if (priority)
                    *priority = number;
                if (name)
                    *name = pos + 1;
            }
            free (str_priority);
        }
    }
}

/*
 * Hashes a shared string.
 * The string starts after the reference count, which is skipped.
 *
 * Returns the hash of the shared string (variant of djb2).
 */

unsigned long long
string_shared_hash_key (struct t_hashtable *hashtable,
                        const void *key)
{
    /* make C compiler happy */
    (void) hashtable;

    return hashtable_hash_key_djb2 (((const char *)key) + sizeof (string_shared_count_t));
}

/*
 * Compares two shared strings.
 * Each string starts after the reference count, which is skipped.
 *
 * Returns:
 *   < 0: key1 < key2
 *     0: key1 == key2
 *   > 0: key1 > key2
 */

int
string_shared_keycmp (struct t_hashtable *hashtable,
                      const void *key1, const void *key2)
{
    /* make C compiler happy */
    (void) hashtable;

    return strcmp (((const char *)key1) + sizeof (string_shared_count_t),
                   ((const char *)key2) + sizeof (string_shared_count_t));
}

/*
 * Frees a shared string.
 */

void
string_shared_free_key (struct t_hashtable *hashtable, void *key)
{
    /* make C compiler happy */
    (void) hashtable;

    free (key);
}

/*
 * Gets a pointer to a shared string.
 *
 * A shared string is an entry in the hashtable "string_hashtable_shared", with:
 * - key: reference count (unsigned integer on 32 bits) + string
 * - value: NULL pointer (not used)
 *
 * The initial reference count is set to 1 and is incremented each time this
 * function is called for a same string (string content, not the pointer).
 *
 * Returns the pointer to the shared string (start of string in key, after the
 * reference count), NULL if error.
 * The string returned has exactly same content as string received in argument,
 * but the pointer to the string is different.
 *
 * IMPORTANT: the returned string must NEVER be changed in any way, because it
 * is used itself as the key of the hashtable.
 */

const char *
string_shared_get (const char *string)
{
    struct t_hashtable_item *ptr_item;
    char *key;
    int length;

    if (!string)
        return NULL;

    if (!string_hashtable_shared)
    {
        /*
         * use large htable inside hashtable to prevent too many collisions,
         * which would slow down search of a string in the hashtable
         */
        string_hashtable_shared = hashtable_new (1024,
                                                 WEECHAT_HASHTABLE_POINTER,
                                                 WEECHAT_HASHTABLE_POINTER,
                                                 &string_shared_hash_key,
                                                 &string_shared_keycmp);
        if (!string_hashtable_shared)
            return NULL;

        string_hashtable_shared->callback_free_key = &string_shared_free_key;
    }

    length = sizeof (string_shared_count_t) + strlen (string) + 1;
    key = malloc (length);
    if (!key)
        return NULL;
    *((string_shared_count_t *)key) = 1;
    strcpy (key + sizeof (string_shared_count_t), string);

    ptr_item = hashtable_get_item (string_hashtable_shared, key, NULL);
    if (ptr_item)
    {
        /*
         * the string already exists in the hashtable, then just increase the
         * reference count on the string
         */
        (*((string_shared_count_t *)(ptr_item->key)))++;
        free (key);
    }
    else
    {
        /* add the shared string in the hashtable */
        ptr_item = hashtable_set (string_hashtable_shared, key, NULL);
        if (!ptr_item)
            free (key);
    }

    return (ptr_item) ?
        ((const char *)ptr_item->key) + sizeof (string_shared_count_t) : NULL;
}

/*
 * Frees a shared string.
 *
 * The reference count of the string is decremented. If it becomes 0, then the
 * shared string is removed from the hashtable (and then the string is really
 * destroyed).
 */

void
string_shared_free (const char *string)
{
    string_shared_count_t *ptr_count;

    if (!string)
        return;

    ptr_count = (string_shared_count_t *)(string - sizeof (string_shared_count_t));

    (*ptr_count)--;

    if (*ptr_count == 0)
        hashtable_remove (string_hashtable_shared, ptr_count);
}

/*
 * Allocates a dynamic string (with a variable length).
 *
 * The parameter size_alloc is the initial allocated size, which must be
 * greater than zero.
 *
 * Returns the pointer to the allocated string, which is initialized as empty
 * string.
 *
 * The string returned can be used with following restrictions:
 *   - changes are allowed in the string, between the first char and the final
 *     '\0', which must remain at its current location,
 *   - no other '\0' may be added in the string,
 *   - content can be added in the string with function string_dyn_concat(),
 *   - string can be freed with function string_dyn_free() (do NEVER call
 *     directly free() on the string).
 *
 * Note: result must be freed after use with function string_dyn_free().
 */

char **
string_dyn_alloc (int size_alloc)
{
    struct t_string_dyn *string_dyn;

    if (size_alloc <= 0)
        return NULL;

    string_dyn = malloc (sizeof (*string_dyn));
    if (!string_dyn)
        return NULL;

    string_dyn->string = malloc (size_alloc);
    if (!string_dyn->string)
    {
        free (string_dyn);
        return NULL;
    }

    string_dyn->string[0] = '\0';
    string_dyn->size_alloc = size_alloc;
    string_dyn->size = 1;

    return &(string_dyn->string);
}

/*
 * Copies "new_string" into a dynamic string and replaces its current content
 * (adjusts its size accordingly).
 *
 * The string pointer (*string) is updated with the new allocated string
 * if the string had to be extended, or the same pointer if there was enough
 * size to copy the new string.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
string_dyn_copy (char **string, const char *new_string)
{
    struct t_string_dyn *ptr_string_dyn;
    char *string_realloc;
    string_dyn_size_t length_new, new_size_alloc;

    if (!string || !*string)
        return 0;

    ptr_string_dyn = (struct t_string_dyn *)string;

    length_new = (new_string) ? strlen (new_string) : 0;

    if (length_new + 1 > ptr_string_dyn->size_alloc)
    {
        /* compute new size_alloc for the string + add */
        new_size_alloc = (ptr_string_dyn->size_alloc < 2) ?
            2 : ptr_string_dyn->size_alloc + (ptr_string_dyn->size_alloc / 2);
        if (new_size_alloc < length_new + 1)
            new_size_alloc = length_new + 1;
        string_realloc = realloc (ptr_string_dyn->string, new_size_alloc);
        if (!string_realloc)
            return 0;
        ptr_string_dyn->string = string_realloc;
        ptr_string_dyn->size_alloc = new_size_alloc;
    }

    /* copy "new_string" in "string" */
    if (new_string)
        memmove (ptr_string_dyn->string, new_string, length_new + 1);
    else
        ptr_string_dyn->string[0] = '\0';
    ptr_string_dyn->size = length_new + 1;

    return 1;
}

/*
 * Concatenates a string to a dynamic string and adjusts its size accordingly.
 *
 * The parameter "bytes" is the max number of bytes to concatenate
 * (a terminating null byte '\0' is automatically added); value -1 means
 * automatic: whole string "add" is concatenated.
 *
 * The string pointer (*string) is updated with the new allocated string
 * if the string had to be extended, or the same pointer if there was enough
 * size to concatenate the new string.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
string_dyn_concat (char **string, const char *add, int bytes)
{
    struct t_string_dyn *ptr_string_dyn;
    char *string_realloc;
    string_dyn_size_t length_add, new_size_alloc, new_size;

    if (!string || !*string)
        return 0;

    if (!add || !add[0] || (bytes == 0))
        return 1;

    ptr_string_dyn = (struct t_string_dyn *)string;

    length_add = strlen (add);
    if ((bytes >= 0) && (bytes < (int)length_add))
        length_add = bytes;

    new_size = ptr_string_dyn->size + length_add;

    if (new_size > ptr_string_dyn->size_alloc)
    {
        /* compute new size_alloc for the string + add */
        new_size_alloc = (ptr_string_dyn->size_alloc < 2) ?
            2 : ptr_string_dyn->size_alloc + (ptr_string_dyn->size_alloc / 2);
        if (new_size_alloc < new_size)
            new_size_alloc = new_size;
        string_realloc = realloc (ptr_string_dyn->string, new_size_alloc);
        if (!string_realloc)
        {
            free (ptr_string_dyn->string);
            free (ptr_string_dyn);
            return 0;
        }
        ptr_string_dyn->string = string_realloc;
        ptr_string_dyn->size_alloc = new_size_alloc;
    }

    /* concatenate "add" after "string" */
    memmove (ptr_string_dyn->string + ptr_string_dyn->size - 1,
             add,
             length_add);
    ptr_string_dyn->size = new_size;
    ptr_string_dyn->string[new_size - 1] = '\0';

    return 1;
}

/*
 * Frees a dynamic string.
 *
 * The argument "string" is a pointer on a string returned by function
 * string_dyn_alloc or a string pointer modified by string_dyn_concat.
 *
 * If free_string == 1, the string itself is freed in the structure.
 *
 * If free_string == 0, the pointer (*string) remains valid after this call,
 * and the caller must manually free the string with a call to free().
 * Be careful, the pointer in *string may change after this call because
 * the string can be reallocated to its exact size.
 *
 * Returns the pointer to the string if "free_string" is 0 (string
 * pointer is still valid), or NULL if "free_string" is 1 (string
 * has been freed).
 */

char *
string_dyn_free (char **string, int free_string)
{
    struct t_string_dyn *ptr_string_dyn;
    char *ptr_string, *string_realloc;

    if (!string || !*string)
        return NULL;

    ptr_string_dyn = (struct t_string_dyn *)string;

    if (free_string)
    {
        free (ptr_string_dyn->string);
        ptr_string = NULL;
    }
    else
    {
        /* if needed, realloc the string to its exact size */
        if (ptr_string_dyn->size_alloc > ptr_string_dyn->size)
        {
            string_realloc = realloc (ptr_string_dyn->string,
                                      ptr_string_dyn->size);
            if (string_realloc)
                ptr_string_dyn->string = string_realloc;
        }
        ptr_string = ptr_string_dyn->string;
    }

    free (ptr_string_dyn);

    return ptr_string;
}

/*
 * Concatenates strings, using a separator (which can be NULL or empty string
 * to not use any separator).
 *
 * Last argument must be NULL to terminate the variable list of arguments.
 */

const char *
string_concat (const char *separator, ...)
{
    va_list args;
    const char *str;
    int index;

    string_concat_index = (string_concat_index + 1) % 8;

    if (string_concat_buffer[string_concat_index])
    {
        string_dyn_copy (string_concat_buffer[string_concat_index], NULL);
    }
    else
    {
        string_concat_buffer[string_concat_index] = string_dyn_alloc (128);
        if (!string_concat_buffer[string_concat_index])
            return NULL;
    }

    index = 0;
    va_start (args, separator);
    while (1)
    {
        str = va_arg (args, const char *);
        if (!str)
            break;
        if ((index > 0) && separator && separator[0])
        {
            string_dyn_concat (string_concat_buffer[string_concat_index],
                               separator, -1);
        }
        string_dyn_concat (string_concat_buffer[string_concat_index], str, -1);
        index++;
    }
    va_end (args);

    return (const char *)(*string_concat_buffer[string_concat_index]);
}

/*
 * Initializes string.
 */

void
string_init (void)
{
    int i;

    for (i = 0; i < STRING_NUM_CONCAT_BUFFERS; i++)
    {
        string_concat_buffer[i] = NULL;
    }
}

/*
 * Frees all allocated data.
 */

void
string_end (void)
{
    int i;

    if (string_hashtable_shared)
    {
        hashtable_free (string_hashtable_shared);
        string_hashtable_shared = NULL;
    }
    for (i = 0; i < STRING_NUM_CONCAT_BUFFERS; i++)
    {
        if (string_concat_buffer[i])
        {
            string_dyn_free (string_concat_buffer[i], 1);
            string_concat_buffer[i] = NULL;
        }
    }
}
