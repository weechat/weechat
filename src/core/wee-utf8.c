/*
 * wee-utf8.c - UTF-8 string functions
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "weechat.h"
#include "wee-utf8.h"
#include "wee-config.h"
#include "wee-string.h"


int local_utf8 = 0;


/*
 * Initializes UTF-8 in WeeChat.
 */

void
utf8_init ()
{
    local_utf8 = (string_strcasecmp (weechat_local_charset, "UTF-8") == 0);
}

/*
 * Checks if a string has some 8-bit chars.
 *
 * Returns:
 *   1: string has 8-bit chars
 *   0: string has only 7-bit chars
 */

int
utf8_has_8bits (const char *string)
{
    while (string && string[0])
    {
        if (string[0] & 0x80)
            return 1;
        string++;
    }
    return 0;
}

/*
 * Checks if a string is UTF-8 valid.
 *
 * Returns:
 *   1: string is UTF-8 valid
 *   0: string it not UTF-8 valid, and then if error is not NULL, it is set with
 *      first non valid UTF-8 char in string
 */

int
utf8_is_valid (const char *string, char **error)
{
    while (string && string[0])
    {
        /* UTF-8, 2 bytes, should be: 110vvvvv 10vvvvvv */
        if (((unsigned char)(string[0]) & 0xE0) == 0xC0)
        {
            if (!string[1] || (((unsigned char)(string[1]) & 0xC0) != 0x80))
            {
                if (error)
                    *error = (char *)string;
                return 0;
            }
            string += 2;
        }
        /* UTF-8, 3 bytes, should be: 1110vvvv 10vvvvvv 10vvvvvv */
        else if (((unsigned char)(string[0]) & 0xF0) == 0xE0)
        {
            if (!string[1] || !string[2]
                || (((unsigned char)(string[1]) & 0xC0) != 0x80)
                || (((unsigned char)(string[2]) & 0xC0) != 0x80))
            {
                if (error)
                    *error = (char *)string;
                return 0;
            }
            string += 3;
        }
        /* UTF-8, 4 bytes, should be: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
        else if (((unsigned char)(string[0]) & 0xF8) == 0xF0)
        {
            if (!string[1] || !string[2] || !string[3]
                || (((unsigned char)(string[1]) & 0xC0) != 0x80)
                || (((unsigned char)(string[2]) & 0xC0) != 0x80)
                || (((unsigned char)(string[3]) & 0xC0) != 0x80))
            {
                if (error)
                    *error = (char *)string;
                return 0;
            }
            string += 4;
        }
        /* UTF-8, 1 byte, should be: 0vvvvvvv */
        else if ((unsigned char)(string[0]) >= 0x80)
        {
            if (error)
                *error = (char *)string;
            return 0;
        }
        else
            string++;
    }
    if (error)
        *error = NULL;
    return 1;
}

/*
 * Normalizes an string: removes non UTF-8 chars and replaces them by a
 * "replacement" char.
 */

void
utf8_normalize (char *string, char replacement)
{
    char *error;

    while (string && string[0])
    {
        if (utf8_is_valid (string, &error))
            return;
        error[0] = replacement;
        string = error + 1;
    }
}

/*
 * Returns pointer to previous UTF-8 char in a string.
 */

char *
utf8_prev_char (const char *string_start, const char *string)
{
    if (!string || (string <= string_start))
        return NULL;

    string--;

    if (((unsigned char)(string[0]) & 0xC0) == 0x80)
    {
        /* UTF-8, at least 2 bytes */
        string--;
        if (string < string_start)
            return (char *)string + 1;
        if (((unsigned char)(string[0]) & 0xC0) == 0x80)
        {
            /* UTF-8, at least 3 bytes */
            string--;
            if (string < string_start)
                return (char *)string + 1;
            if (((unsigned char)(string[0]) & 0xC0) == 0x80)
            {
                /* UTF-8, 4 bytes */
                string--;
                if (string < string_start)
                    return (char *)string + 1;
                return (char *)string;
            }
            else
                return (char *)string;
        }
        else
            return (char *)string;
    }
    return (char *)string;
}

/*
 * Returns pointer to next UTF-8 char in a string.
 */

char *
utf8_next_char (const char *string)
{
    if (!string)
        return NULL;

    /* UTF-8, 2 bytes: 110vvvvv 10vvvvvv */
    if (((unsigned char)(string[0]) & 0xE0) == 0xC0)
    {
        if (!string[1])
            return (char *)string + 1;
        return (char *)string + 2;
    }
    /* UTF-8, 3 bytes: 1110vvvv 10vvvvvv 10vvvvvv */
    else if (((unsigned char)(string[0]) & 0xF0) == 0xE0)
    {
        if (!string[1])
            return (char *)string + 1;
        if (!string[2])
            return (char *)string + 2;
        return (char *)string + 3;
    }
    /* UTF-8, 4 bytes: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
    else if (((unsigned char)(string[0]) & 0xF8) == 0xF0)
    {
        if (!string[1])
            return (char *)string + 1;
        if (!string[2])
            return (char *)string + 2;
        if (!string[3])
            return (char *)string + 3;
        return (char *)string + 4;
    }
    /* UTF-8, 1 byte: 0vvvvvvv */
    return (char *)string + 1;
}

/*
 * Returns UTF-8 char as an integer.
 */

int
utf8_char_int (const char *string)
{
    const unsigned char *ptr_string;

    if (!string)
        return 0;

    ptr_string = (unsigned char *)string;

    /* UTF-8, 2 bytes: 110vvvvv 10vvvvvv */
    if ((ptr_string[0] & 0xE0) == 0xC0)
    {
        if (!ptr_string[1])
            return (int)(ptr_string[0] & 0x1F);
        return ((int)(ptr_string[0] & 0x1F) << 6) +
            ((int)(ptr_string[1] & 0x3F));
    }
    /* UTF-8, 3 bytes: 1110vvvv 10vvvvvv 10vvvvvv */
    else if ((ptr_string[0] & 0xF0) == 0xE0)
    {
        if (!ptr_string[1])
            return (int)(ptr_string[0] & 0x0F);
        if (!ptr_string[2])
            return (((int)(ptr_string[0] & 0x0F)) << 6) +
                ((int)(ptr_string[1] & 0x3F));
        return (((int)(ptr_string[0] & 0x0F)) << 12) +
            (((int)(ptr_string[1] & 0x3F)) << 6) +
            ((int)(ptr_string[2] & 0x3F));
    }
    /* UTF-8, 4 bytes: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
    else if ((ptr_string[0] & 0xF8) == 0xF0)
    {
        if (!ptr_string[1])
            return (int)(ptr_string[0] & 0x07);
        if (!ptr_string[2])
            return (((int)(ptr_string[0] & 0x07)) << 6) +
                ((int)(ptr_string[1] & 0x3F));
        if (!ptr_string[3])
            return (((int)(ptr_string[0] & 0x07)) << 12) +
                (((int)(ptr_string[1] & 0x3F)) << 6) +
                ((int)(ptr_string[2] & 0x3F));
        return (((int)(ptr_string[0] & 0x07)) << 18) +
            (((int)(ptr_string[1] & 0x3F)) << 12) +
            (((int)(ptr_string[2] & 0x3F)) << 6) +
            ((int)(ptr_string[3] & 0x3F));
    }
    /* UTF-8, 1 byte: 0vvvvvvv */
    return (int)ptr_string[0];
}

/*
 * Gets wide char from string (first char).
 */

wint_t
utf8_wide_char (const char *string)
{
    int char_size;
    wint_t result;

    if (!string || !string[0])
        return WEOF;

    char_size = utf8_char_size (string);
    switch (char_size)
    {
        case 1:
            result = (wint_t)string[0];
            break;
        case 2:
            result = ((wint_t)((unsigned char)string[0])) << 8
                |  ((wint_t)((unsigned char)string[1]));
            break;
        case 3:
            result = ((wint_t)((unsigned char)string[0])) << 16
                |  ((wint_t)((unsigned char)string[1])) << 8
                |  ((wint_t)((unsigned char)string[2]));
            break;
        case 4:
            result = ((wint_t)((unsigned char)string[0])) << 24
                |  ((wint_t)((unsigned char)string[1])) << 16
                |  ((wint_t)((unsigned char)string[2])) << 8
                |  ((wint_t)((unsigned char)string[3]));
            break;
        default:
            result = WEOF;
    }
    return result;
}

/*
 * Gets size of UTF-8 char (in bytes).
 *
 * Returns an integer between 0 and 4.
 */

int
utf8_char_size (const char *string)
{
    if (!string)
        return 0;

    return utf8_next_char (string) - string;
}

/*
 * Returns length of an UTF-8 string in number of chars (not bytes).
 * Result is <= strlen(string).
 */

int
utf8_strlen (const char *string)
{
    int length;

    if (!string)
        return 0;

    length = 0;
    while (string && string[0])
    {
        string = utf8_next_char (string);
        length++;
    }
    return length;
}

/*
 * Returns length of an UTF-8 string for N bytes max in string.
 */

int
utf8_strnlen (const char *string, int bytes)
{
    char *start;
    int length;

    if (!string)
        return 0;

    start = (char *)string;
    length = 0;
    while (string && string[0] && (string - start < bytes))
    {
        string = utf8_next_char (string);
        length++;
    }
    return length;
}

/*
 * Returns number of chars needed on screen to display the UTF-8 string.
 */

int
utf8_strlen_screen (const char *string)
{
    int length, num_char;
    wchar_t *alloc_wstring, *ptr_wstring, wstring[4+2];

    if (!string || !string[0])
        return 0;

    if (!local_utf8)
        return utf8_strlen (string);

    alloc_wstring = NULL;

    if (!string[1] || !string[2] || !string[3] || !string[4])
    {
        /* optimization for max 4 chars: no malloc */
        num_char = 4 + 1;
        ptr_wstring = wstring;
    }
    else
    {
        num_char = mbstowcs (NULL, string, 0) + 1;
        alloc_wstring = malloc ((num_char + 1) * sizeof (alloc_wstring[0]));
        if (!alloc_wstring)
            return utf8_strlen (string);
        ptr_wstring = alloc_wstring;
    }

    if (mbstowcs (ptr_wstring, string, num_char) != (size_t)(-1))
        length = wcswidth (ptr_wstring, num_char);
    else
        length = utf8_strlen (string);

    if (alloc_wstring)
        free (alloc_wstring);

    return length;
}

/*
 * Compares two UTF-8 chars (case sensitive).
 */

int
utf8_charcmp (const char *string1, const char *string2)
{
    int length1, length2, i, diff;

    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);

    length1 = utf8_char_size (string1);
    length2 = utf8_char_size (string2);

    i = 0;
    while ((i < length1) && (i < length2))
    {
        diff = (int)((unsigned char) string1[i]) - (int)((unsigned char) string2[i]);
        if (diff != 0)
            return diff;
        i++;
    }
    /* string1 == string2 ? */
    if ((i == length1) && (i == length2))
        return 0;
    /* string1 < string2 ? */
    if (i == length1)
        return 1;
    /* string1 > string2 */
    return -1;
}

/*
 * Compares two UTF-8 chars (case is ignored).
 */

int
utf8_charcasecmp (const char *string1, const char *string2)
{
    wint_t wchar1, wchar2;

    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);

    wchar1 = utf8_wide_char (string1);
    if ((wchar1 >= 'A') && (wchar1 <= 'Z'))
        wchar1 += ('a' - 'A');

    wchar2 = utf8_wide_char (string2);
    if ((wchar2 >= 'A') && (wchar2 <= 'Z'))
        wchar2 += ('a' - 'A');

    return (wchar1 < wchar2) ? -1 : ((wchar1 == wchar2) ? 0 : 1);
}

/*
 * Compares two UTF-8 chars (case is ignored) using a range.
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
 * Returns:
 *   < 0: char1 < char2
 *     0: char1 == char2
 *   > 0: char1 > char2
 */

int
utf8_charcasecmp_range (const char *string1, const char *string2, int range)
{
    wint_t wchar1, wchar2;

    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);

    wchar1 = utf8_wide_char (string1);
    if ((wchar1 >= (wint_t)'A') && (wchar1 < (wint_t)('A' + range)))
        wchar1 += ('a' - 'A');

    wchar2 = utf8_wide_char (string2);
    if ((wchar2 >= (wint_t)'A') && (wchar2 < (wint_t)('A' + range)))
        wchar2 += ('a' - 'A');

    return (wchar1 < wchar2) ? -1 : ((wchar1 == wchar2) ? 0 : 1);
}

/*
 * Returns number of chars needed on screen to display the UTF-8 char.
 */

int
utf8_char_size_screen (const char *string)
{
    int char_size;
    char utf_char[16];

    if (!string)
        return 0;

    char_size = utf8_char_size (string);
    if (char_size == 0)
        return 0;

    memcpy (utf_char, string, char_size);
    utf_char[char_size] = '\0';

    return utf8_strlen_screen (utf_char);
}

/*
 * Moves forward N chars in an UTF-8 string.
 */

char *
utf8_add_offset (const char *string, int offset)
{
    if (!string)
        return NULL;

    while (string && string[0] && (offset > 0))
    {
        string = utf8_next_char (string);
        offset--;
    }
    return (char *)string;
}

/*
 * Returns real position in UTF-8 string, in bytes.
 *
 * Argument "pos" is a number of chars (not bytes).
 *
 * Example: ("déca", 2) returns 3.
 */

int
utf8_real_pos (const char *string, int pos)
{
    int count, real_pos;
    char *next_char;

    if (!string)
        return pos;

    count = 0;
    real_pos = 0;
    while (string && string[0] && (count < pos))
    {
        next_char = utf8_next_char (string);
        real_pos += (next_char - string);
        string = next_char;
        count++;
    }
    return real_pos;
}

/*
 * Returns position in UTF-8 string, in chars.
 *
 * Argument "real_pos" is a number of bytes (not chars).
 *
 * Example: ("déca", 3) returns 2.
 */

int
utf8_pos (const char *string, int real_pos)
{
    int count;
    char *limit;

    if (!string || !weechat_local_charset)
        return real_pos;

    count = 0;
    limit = (char *)string + real_pos;
    while (string && string[0] && (string < limit))
    {
        string = utf8_next_char (string);
        count++;
    }
    return count;
}

/*
 * Duplicates an UTF-8 string, with max N chars.
 */

char *
utf8_strndup (const char *string, int length)
{
    const char *end;

    if (!string || (length < 0))
        return NULL;

    if (length == 0)
        return strdup ("");

    end = utf8_add_offset (string, length);
    if (!end || (end == string))
        return strdup (string);

    return string_strndup (string, end - string);
}
