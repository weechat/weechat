/*
 * core-utf8.c - UTF-8 string functions
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "weechat.h"
#include "core-utf8.h"
#include "core-config.h"
#include "core-string.h"


int local_utf8 = 0;


/*
 * Initializes UTF-8 in WeeChat.
 */

void
utf8_init ()
{
    local_utf8 = (string_strcasecmp (weechat_local_charset, "utf-8") == 0);
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
 * If length is <= 0, checks whole string.
 * If length is > 0, checks only this number of chars (not bytes).
 *
 * Returns:
 *   1: string is UTF-8 valid
 *   0: string it not UTF-8 valid, and then if error is not NULL, it is set
 *      with first non valid UTF-8 char in string
 */

int
utf8_is_valid (const char *string, int length, char **error)
{
    int code_point, current_char;

    current_char = 0;

    while (string && string[0]
           && ((length <= 0) || (current_char < length)))
    {
        /*
         * UTF-8, 2 bytes, should be: 110vvvvv 10vvvvvv
         * and in range: U+0080 - U+07FF
         */
        if (((unsigned char)(string[0]) & 0xE0) == 0xC0)
        {
            if (!string[1] || (((unsigned char)(string[1]) & 0xC0) != 0x80))
                goto invalid;
            code_point = utf8_char_int (string);
            if ((code_point < 0x0080) || (code_point > 0x07FF))
                goto invalid;
            string += 2;
        }
        /*
         * UTF-8, 3 bytes, should be: 1110vvvv 10vvvvvv 10vvvvvv
         * and in range: U+0800 - U+FFFF
         * (note: high and low surrogate halves used by UTF-16 (U+D800 through
         * U+DFFF) are not legal Unicode values)
         */
        else if (((unsigned char)(string[0]) & 0xF0) == 0xE0)
        {
            if (!string[1] || !string[2]
                || (((unsigned char)(string[1]) & 0xC0) != 0x80)
                || (((unsigned char)(string[2]) & 0xC0) != 0x80))
            {
                goto invalid;
            }
            code_point = utf8_char_int (string);
            if ((code_point < 0x0800)
                || (code_point > 0xFFFF)
                || ((code_point >= 0xD800) && (code_point <= 0xDFFF)))
            {
                goto invalid;
            }
            string += 3;
        }
        /*
         * UTF-8, 4 bytes, should be: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv
         * and in range: U+10000 - U+1FFFFF
         */
        else if (((unsigned char)(string[0]) & 0xF8) == 0xF0)
        {
            if (!string[1] || !string[2] || !string[3]
                || (((unsigned char)(string[1]) & 0xC0) != 0x80)
                || (((unsigned char)(string[2]) & 0xC0) != 0x80)
                || (((unsigned char)(string[3]) & 0xC0) != 0x80))
            {
                goto invalid;
            }
            code_point = utf8_char_int (string);
            if ((code_point < 0x10000) || (code_point > 0x1FFFFF))
                goto invalid;
            string += 4;
        }
        /* UTF-8, 1 byte, should be: 0vvvvvvv */
        else if ((unsigned char)(string[0]) >= 0x80)
            goto invalid;
        else
            string++;
        current_char++;
    }
    if (error)
        *error = NULL;
    return 1;

invalid:
    if (error)
        *error = (char *)string;
    return 0;
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
        if (utf8_is_valid (string, -1, &error))
            return;
        error[0] = replacement;
        string = error + 1;
    }
}

/*
 * Gets pointer to previous UTF-8 char in a string.
 *
 * Returns pointer to previous UTF-8 char, NULL if not found (for example
 * "string_start" was reached).
 */

const char *
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
            {
                return string;
            }
        }
        else
        {
            return string;
        }
    }
    return string;
}

/*
 * Gets pointer to next UTF-8 char in a string.
 *
 * Returns pointer to next UTF-8 char, NULL if string was NULL.
 */

const char *
utf8_next_char (const char *string)
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
 * Gets pointer to the beginning of the UTF-8 line in a string.
 *
 * Returns pointer to the beginning of the UTF-8 line, NULL if string was NULL.
 */

const char *
utf8_beginning_of_line (const char *string_start, const char *string)
{
    if (string && (string[0] == '\n'))
        string = utf8_prev_char (string_start, string);

    while (string && (string[0] != '\n'))
    {
        string = utf8_prev_char (string_start, string);
    }

    if (string)
        return utf8_next_char (string);

    return string_start;
}

/*
 * Gets pointer to the end of the UTF-8 line in a string.
 *
 * Returns pointer to the end of the UTF-8 line, NULL if string was NULL.
 */

const char *
utf8_end_of_line (const char *string)
{
    if (!string)
        return NULL;

    while (string[0] && (string[0] != '\n'))
    {
        string = utf8_next_char (string);
    }

    return string;
}

/*
 * Gets UTF-8 char as an integer.
 *
 * Returns the UTF-8 char as integer number.
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
 * Converts a unicode char (as unsigned integer) to a string.
 *
 * The string must have a size >= 5
 * (4 bytes for the UTF-8 char + the final '\0').
 *
 * In case of error (if unicode value is > 0x1FFFFF), the string is set to an
 * empty string (string[0] == '\0').
 *
 * Returns the number of bytes in the UTF-8 char (not counting the final '\0').
 */

int
utf8_int_string (unsigned int unicode_value, char *string)
{
    int num_bytes;

    num_bytes = 0;

    if (!string)
        return num_bytes;

    string[0] = '\0';

    if (unicode_value == 0)
    {
        /* NUL char */
    }
    else if (unicode_value <= 0x007F)
    {
        /* UTF-8, 1 byte: 0vvvvvvv */
        string[0] = unicode_value;
        string[1] = '\0';
        num_bytes = 1;
    }
    else if (unicode_value <= 0x07FF)
    {
        /* UTF-8, 2 bytes: 110vvvvv 10vvvvvv */
        string[0] = 0xC0 | ((unicode_value >> 6) & 0x1F);
        string[1] = 0x80 | (unicode_value & 0x3F);
        string[2] = '\0';
        num_bytes = 2;
    }
    else if (unicode_value <= 0xFFFF)
    {
        /* UTF-8, 3 bytes: 1110vvvv 10vvvvvv 10vvvvvv */
        string[0] = 0xE0 | ((unicode_value >> 12) & 0x0F);
        string[1] = 0x80 | ((unicode_value >> 6) & 0x3F);
        string[2] = 0x80 | (unicode_value & 0x3F);
        string[3] = '\0';
        num_bytes = 3;
    }
    else if (unicode_value <= 0x1FFFFF)
    {
        /* UTF-8, 4 bytes: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
        string[0] = 0xF0 | ((unicode_value >> 18) & 0x07);
        string[1] = 0x80 | ((unicode_value >> 12) & 0x3F);
        string[2] = 0x80 | ((unicode_value >> 6) & 0x3F);
        string[3] = 0x80 | (unicode_value & 0x3F);
        string[4] = '\0';
        num_bytes = 4;
    }

    return num_bytes;
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
 * Gets length of an UTF-8 string in number of chars (not bytes).
 * Result is <= strlen (string).
 *
 * Returns length of string (>= 0).
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
 * Gets length of an UTF-8 string for N bytes max in string.
 *
 * Returns length of string (>= 0).
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
 * Gets number of chars needed on screen to display the UTF-8 char.
 *
 * Returns the number of chars (>= 0).
 */

int
utf8_char_size_screen (const char *string)
{
    wchar_t codepoint;

    if (!string || !string[0])
        return 0;

    if (string[0] == '\t')
        return CONFIG_INTEGER(config_look_tab_width);

    /*
     * chars < 32 are displayed with a letter/symbol and reverse video,
     * so exactly one column
     */
    if (((unsigned char)string[0]) < 32)
        return 1;

    codepoint = (wchar_t)utf8_char_int (string);

    /*
     * special chars not displayed (because not handled by WeeChat):
     *   U+00AD: soft hyphen      (wcwidth == 1)
     *   U+200B: zero width space (wcwidth == 0)
     */
    if ((codepoint == 0x00AD) || (codepoint == 0x200B))
    {
        return -1;
    }

    return wcwidth (codepoint);
}

/*
 * Gets number of chars needed on screen to display the UTF-8 string.
 *
 * Returns the number of chars (>= 0).
 */

int
utf8_strlen_screen (const char *string)
{
    int size_screen, size_screen_char;
    const char *ptr_string;

    if (!string)
        return 0;

    if (!local_utf8)
        return utf8_strlen (string);

    size_screen = 0;
    ptr_string = string;
    while (ptr_string && ptr_string[0])
    {
        size_screen_char = utf8_char_size_screen (ptr_string);
        /* count only chars that use at least one column */
        if (size_screen_char > 0)
            size_screen += size_screen_char;
        ptr_string = utf8_next_char (ptr_string);
    }

    return size_screen;
}

/*
 * Moves forward N chars in an UTF-8 string.
 *
 * Returns pointer to the new position in string.
 */

const char *
utf8_add_offset (const char *string, int offset)
{
    if (!string)
        return NULL;

    while (string && string[0] && (offset > 0))
    {
        string = utf8_next_char (string);
        offset--;
    }
    return string;
}

/*
 * Gets real position in UTF-8 string, in bytes.
 *
 * Argument "pos" is a number of chars (not bytes).
 *
 * Example: ("déca", 2) returns 3.
 *
 * Returns the real position (>= 0).
 */

int
utf8_real_pos (const char *string, int pos)
{
    int count, real_pos;
    const char *next_char;

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
 * Gets position in UTF-8 string, in chars.
 *
 * Argument "real_pos" is a number of bytes (not chars).
 *
 * Example: ("déca", 3) returns 2.
 *
 * Returns the position in string.
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
 *
 * Note: result must be freed after use.
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

/*
 * Copies max N chars from a string to another and adds null byte at the end.
 *
 * Note: the target string "dest" must be long enough.
 */

void
utf8_strncpy (char *dest, const char *string, int length)
{
    const char *end;

    if (!dest)
        return;

    dest[0] = '\0';

    if (!string || (length <= 0))
        return;

    end = utf8_add_offset (string, length);
    if (!end || (end == string))
        return;

    memcpy (dest, string, end - string);
    dest[end - string] = '\0';
}
