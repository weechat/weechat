/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* wee-utf8.c: UTF-8 string functions for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "wee-utf8.h"
#include "wee-config.h"
#include "wee-string.h"


int local_utf8 = 0;

/*
 * utf8_init: initializes UTF-8 in WeeChat
 */

void
utf8_init ()
{
    local_utf8 = (string_strcasecmp (local_charset, "UTF-8") == 0);
}

/*
 * utf8_has_8bits: return 1 if string has 8-bits chars, 0 if only 7-bits chars
 */

int
utf8_has_8bits (char *string)
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
 * utf8_is_valid: return 1 if UTF-8 string is valid, 0 otherwise
 *                if error is not NULL, it's set with first non valid UTF-8
 *                char in string, if any
 */

int
utf8_is_valid (char *string, char **error)
{
    while (string && string[0])
    {
        /* UTF-8, 2 bytes, should be: 110vvvvv 10vvvvvv */
        if (((unsigned char)(string[0]) & 0xE0) == 0xC0)
        {
            if (!string[1] || (((unsigned char)(string[1]) & 0xC0) != 0x80))
            {
                if (error)
                    *error = string;
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
                    *error = string;
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
                    *error = string;
                return 0;
            }
            string += 4;
        }
        /* UTF-8, 1 byte, should be: 0vvvvvvv */
        else if ((unsigned char)(string[0]) >= 0x80)
        {
            if (error)
                *error = string;
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
 * utf8_normalize: normalize UTF-8 string: remove non UTF-8 chars and
 *                 replace them by a char
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
 * utf8_prev_char: return previous UTF-8 char in a string
 */

char *
utf8_prev_char (char *string_start, char *string)
{
    if (!string || (string <= string_start))
        return NULL;
    
    string--;
    
    if (((unsigned char)(string[0]) & 0xC0) == 0x80)
    {
        /* UTF-8, at least 2 bytes */
        string--;
        if (string < string_start)
            return string + 1;
        if (((unsigned char)(string[0]) & 0xC0) == 0x80)
        {
            /* UTF-8, at least 3 bytes */
            string--;
            if (string < string_start)
                return string + 1;
            if (((unsigned char)(string[0]) & 0xC0) == 0x80)
            {
                /* UTF-8, 4 bytes */
                string--;
                if (string < string_start)
                    return string + 1;
                return string;
            }
            else
                return string;
        }
        else
            return string;
    }
    return string;
}

/*
 * utf8_next_char: return next UTF-8 char in a string
 */

char *
utf8_next_char (char *string)
{
    if (!string)
        return NULL;
    
    /* UTF-8, 2 bytes: 110vvvvv 10vvvvvv */
    if (((unsigned char)(string[0]) & 0xE0) == 0xC0)
    {
        if (!string[1])
            return string + 1;
        return string + 2;
    }
    /* UTF-8, 3 bytes: 1110vvvv 10vvvvvv 10vvvvvv */
    else if (((unsigned char)(string[0]) & 0xF0) == 0xE0)
    {
        if (!string[1])
            return string + 1;
        if (!string[2])
            return string + 2;
        return string + 3;
    }
    /* UTF-8, 4 bytes: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
    else if (((unsigned char)(string[0]) & 0xF8) == 0xF0)
    {
        if (!string[1])
            return string + 1;
        if (!string[2])
            return string + 2;
        if (!string[3])
            return string + 3;
        return string + 4;
    }
    /* UTF-8, 1 byte: 0vvvvvvv */
    return string + 1;
}

/*
 * utf8_char_size: return UTF-8 char size (in bytes)
 */

int
utf8_char_size (char *string)
{
    if (!string)
        return 0;
    
    return utf8_next_char (string) - string;
}

/*
 * utf8_strlen: return length of an UTF-8 string (<= strlen(string))
 */

int
utf8_strlen (char *string)
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
 * utf8_strnlen: return length of an UTF-8 string, for N bytes max in string
 */

int
utf8_strnlen (char *string, int bytes)
{
    char *start;
    int length;
    
    if (!string)
        return 0;
    
    start = string;
    length = 0;
    while (string && string[0] && (string - start < bytes))
    {
        string = utf8_next_char (string);
        length++;
    }
    return length;
}

/*
 * utf8_strlen_screen: return number of chars needed on screen to display
 *                     UTF-8 string
 */

int
utf8_strlen_screen (char *string)
{
    int length, num_char;
    wchar_t *wstring;
    
    if (!string)
        return 0;
    
    if (!local_utf8)
        return utf8_strlen (string);
    
    num_char = mbstowcs (NULL, string, 0) + 1;
    wstring = (wchar_t *)malloc ((num_char + 1) * sizeof (wchar_t));
    if (!wstring)
        return utf8_strlen (string);
    
    if (mbstowcs (wstring, string, num_char) == (size_t)(-1))
    {
        free (wstring);
        return utf8_strlen (string);
    }
    
    length = wcswidth (wstring, num_char);
    free (wstring);
    return length;
}

/*
 * utf8_charcasecmp: compare two utf8 chars (case is ignored)
 */

int
utf8_charcasecmp (char *string1, char *string2)
{
    int length1, length2, i, char1, char2, diff;
    
    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);
    
    length1 = utf8_char_size (string1);
    length2 = utf8_char_size (string2);
    
    char1 = (int)((unsigned char) string1[0]);
    char2 = (int)((unsigned char) string2[0]);
    
    if ((char1 >= 'A') && (char1 <= 'Z'))
        char1 += ('a' - 'A');
    
    if ((char2 >= 'A') && (char2 <= 'Z'))
        char2 += ('a' - 'A');
    
    diff = char1 - char2;
    if (diff != 0)
        return diff;
    
    i = 1;
    while ((i < length1) && (i < length2))
    {
        diff = (int)((unsigned char) string1[0]) - (int)((unsigned char) string2[0]);
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
 * utf8_char_size_screen: return number of chars needed on screen to display
 *                        UTF-8 char
 */

int
utf8_char_size_screen (char *string)
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
 * utf8_add_offset: moves forward N chars in an UTF-8 string
 */

char *
utf8_add_offset (char *string, int offset)
{
    int count;

    if (!string)
        return string;
    
    count = 0;
    while (string && string[0] && (count < offset))
    {
        string = utf8_next_char (string);
        count++;
    }
    return string;
}

/*
 * utf8_real_pos: get real position in UTF-8
 *                for example: ("aébc", 2) returns 3
 */

int
utf8_real_pos (char *string, int pos)
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
 * utf8_pos: get position in UTF-8
 *           for example: ("aébc", 3) returns 2
 */

int
utf8_pos (char *string, int real_pos)
{
    int count;
    char *limit;
    
    if (!string || !local_charset)
        return real_pos;
    
    count = 0;
    limit = string + real_pos;
    while (string && string[0] && (string < limit))
    {
        string = utf8_next_char (string);
        count++;
    }
    return count;
}
