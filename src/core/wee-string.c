/*
 * wee-string.c - string functions
 *
 * Copyright (C) 2003-2014 Sébastien Helleu <flashcode@flashtux.org>
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
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <regex.h>
#include <wchar.h>
#include <stdint.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#ifndef ICONV_CONST
  #ifdef ICONV_2ARG_IS_CONST
    #define ICONV_CONST const
  #else
    #define ICONV_CONST
  #endif
#endif

#include "weechat.h"
#include "wee-string.h"
#include "wee-config.h"
#include "wee-hashtable.h"
#include "wee-utf8.h"
#include "../gui/gui-color.h"
#include "../plugins/plugin.h"


#define IS_OCTAL_DIGIT(c) ((c >= '0') && (c <= '7'))
#define HEX2DEC(c) (((c >= 'a') && (c <= 'f')) ? c - 'a' + 10 :         \
                    ((c >= 'A') && (c <= 'F')) ? c - 'A' + 10 :         \
                    c - '0')

typedef uint32_t string_shared_count_t;

struct t_hashtable *string_hashtable_shared = NULL;


/*
 * Defines a "strndup" function for systems where this function does not exist
 * (FreeBSD and maybe others).
 */

char *
string_strndup (const char *string, int length)
{
    char *result;

    if (!string)
        return NULL;

    if ((int)strlen (string) < length)
        return strdup (string);

    result = malloc (length + 1);
    if (!result)
        return NULL;

    memcpy (result, string, length);
    result[length] = '\0';

    return result;
}

/*
 * Converts string to lower case (locale independent).
 */

void
string_tolower (char *string)
{
    while (string && string[0])
    {
        if ((string[0] >= 'A') && (string[0] <= 'Z'))
            string[0] += ('a' - 'A');
        string = utf8_next_char (string);
    }
}

/*
 * Converts string to upper case (locale independent).
 */

void
string_toupper (char *string)
{
    while (string && string[0])
    {
        if ((string[0] >= 'a') && (string[0] <= 'z'))
            string[0] -= ('a' - 'A');
        string = utf8_next_char (string);
    }
}

/*
 * Compares two strings (locale and case independent).
 *
 * Returns:
 *   -1: string1 < string2
 *    0: string1 == string2
 *    1: string1 > string2
 */

int
string_strcasecmp (const char *string1, const char *string2)
{
    int diff;

    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);

    while (string1[0] && string2[0])
    {
        diff = utf8_charcasecmp (string1, string2);
        if (diff != 0)
            return (diff < 0) ? -1 : 1;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
    }

    return (string1[0]) ? 1 : ((string2[0]) ? -1 : 0);
}

/*
 * Compares two strings (locale and case independent) using a range.
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
 *   -1: string1 < string2
 *    0: string1 == string2
 *    1: string1 > string2
 */

int
string_strcasecmp_range (const char *string1, const char *string2, int range)
{
    int diff;

    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);

    while (string1[0] && string2[0])
    {
        diff = utf8_charcasecmp_range (string1, string2, range);
        if (diff != 0)
            return (diff < 0) ? -1 : 1;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
    }

    return (string1[0]) ? 1 : ((string2[0]) ? -1 : 0);
}

/*
 * Compares two strings with max length (locale and case independent).
 *
 * Returns:
 *    -1: string1 < string2
 *     0: string1 == string2
 *     1: string1 > string2
 */

int
string_strncasecmp (const char *string1, const char *string2, int max)
{
    int count, diff;

    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);

    count = 0;
    while ((count < max) && string1[0] && string2[0])
    {
        diff = utf8_charcasecmp (string1, string2);
        if (diff != 0)
            return (diff < 0) ? -1 : 1;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
        count++;
    }

    if (count >= max)
        return 0;
    else
        return (string1[0]) ? 1 : ((string2[0]) ? -1 : 0);
}

/*
 * Compares two strings with max length (locale and case independent) using a
 * range.
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
 *   -1: string1 < string2
 *    0: string1 == string2
 *    1: string1 > string2
 */

int
string_strncasecmp_range (const char *string1, const char *string2, int max,
                          int range)
{
    int count, diff;

    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);

    count = 0;
    while ((count < max) && string1[0] && string2[0])
    {
        diff = utf8_charcasecmp_range (string1, string2, range);
        if (diff != 0)
            return (diff < 0) ? -1 : 1;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
        count++;
    }

    if (count >= max)
        return 0;
    else
        return (string1[0]) ? 1 : ((string2[0]) ? -1 : 0);
}

/*
 * Compares two strings, ignoring some chars.
 *
 * Returns:
 *   -1: string1 < string2
 *    0: string1 == string2
 *    1: string1 > string2
 */

int
string_strcmp_ignore_chars (const char *string1, const char *string2,
                            const char *chars_ignored, int case_sensitive)
{
    int diff;

    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);

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
        if ((!string1 || !string1[0]) && (!string2 || !string2[0]))
            return 0;
        if ((!string1 || !string1[0]) && string2 && string2[0])
            return -1;
        if (string1 && string1[0] && (!string2 || !string2[0]))
            return 1;

        /* look at diff */
        diff = (case_sensitive) ?
            (int)string1[0] - (int)string2[0] : utf8_charcasecmp (string1, string2);
        if (diff != 0)
            return (diff < 0) ? -1 : 1;

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
    if ((!string1 || !string1[0]) && string2 && string2[0])
        return -1;
    if (string1 && string1[0] && (!string2 || !string2[0]))
        return 1;
    return 0;
}

/*
 * Searches for a string in another string (locale and case independent).
 *
 * Returns pointer to string found, or NULL if not found.
 */

char *
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
    const char *ptr_string, *ptr_mask, *pos_word, *pos_end;
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
            /* search the word anywhere in the string (from current position) */
            pos_word = (case_sensitive) ?
                strstr (ptr_string, word) : string_strcasestr (ptr_string, word);
            if (!pos_word)
            {
                free (word);
                return 0;
            }
            ptr_string = pos_word + length_word;
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
 * Expands home in a path.
 *
 * Example: "~/file.txt" => "/home/xxx/file.txt"
 *
 * Note: result must be freed after use.
 */

char *
string_expand_home (const char *path)
{
    char *ptr_home, *str;
    int length;

    if (!path)
        return NULL;

    if (!path[0] || (path[0] != '~')
        || ((path[1] && path[1] != DIR_SEPARATOR_CHAR)))
    {
        return strdup (path);
    }

    ptr_home = getenv ("HOME");

    length = strlen (ptr_home) + strlen (path + 1) + 1;
    str = malloc (length);
    if (!str)
        return strdup (path);

    snprintf (str, length, "%s%s", ptr_home, path + 1);

    return str;
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
 * Converts escaped chars to their value.
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
                        utf8_int_string (value, utf_char);
                        if (utf_char[0])
                        {
                            length = strlen (utf_char);
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
                    output[pos_output++] = '\\';
                    output[pos_output++] = ptr_string[0];
                    ptr_string++;
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
 * Checks if first char of string is a "word char".
 *
 * Returns:
 *   1: first char is a word char
 *   0: first char is not a word char
 */

int
string_is_word_char (const char *string)
{
    wint_t c = utf8_wide_char (string);

    if (c == WEOF)
        return 0;

    if (iswalnum (c))
        return 1;

    switch (c)
    {
        case '-':
        case '_':
        case '|':
            return 1;
    }

    /* not a 'word char' */
    return 0;
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
 *   i: case insensitive (REG_ICASE)
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
 */

int
string_regcomp (void *preg, const char *regex, int default_flags)
{
    const char *ptr_regex;
    int flags;

    if (!regex)
        return -1;

    ptr_regex = string_regex_flags (regex, default_flags, &flags);
    return regcomp ((regex_t *)preg, ptr_regex, flags);
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
    char *msg, *highlight, *match, *match_pre, *match_post, *msg_pos;
    char *pos, *pos_end;
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
                startswith = ((match == msg) || (!string_is_word_char (match_pre)));
                endswith = ((!match_post[0]) || (!string_is_word_char (match_post)));
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
            startswith = !string_is_word_char (match_pre);
        }
        endswith = 0;
        if (startswith)
        {
            endswith = ((regex_match.rm_eo == (int)strlen (string))
                        || !string_is_word_char (string + regex_match.rm_eo));
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
                    if (modified_replace)
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
        if (modified_replace)
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

    length = strlen (string) + 1;
    result = malloc (length);
    if (!result)
        return NULL;
    snprintf (result, length, "%s", string);

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

        if (str_replace)
            free (str_replace);

        if (end)
            break;

        start_offset = regex_match[0].rm_so + length_replace;
    }

    return result;
}

/*
 * Splits a string according to separators.
 *
 * This function must not be called directly (call string_split or
 * string_split_shared instead).
 *
 * Examples:
 *   string_split ("abc de  fghi", " ", 0, 0, NULL)
 *     ==> array[0] = "abc"
 *         array[1] = "de"
 *         array[2] = "fghi"
 *         array[3] = NULL
 *   string_split ("abc de  fghi", " ", 1, 0, NULL)
 *     ==> array[0] = "abc de  fghi"
 *         array[1] = "de  fghi"
 *         array[2] = "fghi"
 *         array[3] = NULL
 */

char **
string_split_internal (const char *string, const char *separators, int keep_eol,
                       int num_items_max, int *num_items, int shared)
{
    int i, j, n_items;
    char *string2, **array;
    char *ptr, *ptr1, *ptr2;
    const char *str_shared;

    if (num_items)
        *num_items = 0;

    if (!string || !string[0] || !separators || !separators[0])
        return NULL;

    string2 = string_strip (string, 1, (keep_eol == 2) ? 0 : 1, separators);
    if (!string2 || !string2[0])
        return NULL;

    /* calculate number of items */
    ptr = string2;
    i = 1;
    while ((ptr = strpbrk (ptr, separators)))
    {
        while (ptr[0] && strchr (separators, ptr[0]))
        {
            ptr++;
        }
        i++;
    }
    n_items = i;

    if ((num_items_max != 0) && (n_items > num_items_max))
        n_items = num_items_max;

    array = malloc ((n_items + 1) * sizeof (array[0]));
    if (!array)
    {
        free (string2);
        return NULL;
    }
    for (i = 0; i < n_items + 1; i++)
    {
        array[i] = NULL;
    }

    ptr1 = string2;

    for (i = 0; i < n_items; i++)
    {
        while (ptr1[0] && strchr (separators, ptr1[0]))
        {
            ptr1++;
        }
        if (i == (n_items - 1))
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

        if ((ptr1 == NULL) || (ptr2 == NULL))
        {
            array[i] = NULL;
        }
        else
        {
            if (ptr2 - ptr1 > 0)
            {
                if (keep_eol)
                {
                    array[i] = (shared) ? (char *)string_shared_get (ptr1) : strdup (ptr1);
                    if (!array[i])
                    {
                        for (j = 0; j < n_items; j++)
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
                }
                else
                {
                    array[i] = malloc (ptr2 - ptr1 + 1);
                    if (!array[i])
                    {
                        for (j = 0; j < n_items; j++)
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
                    strncpy (array[i], ptr1, ptr2 - ptr1);
                    array[i][ptr2 - ptr1] = '\0';
                    if (shared)
                    {
                        str_shared = string_shared_get (array[i]);
                        if (!str_shared)
                        {
                            for (j = 0; j < n_items; j++)
                            {
                                if (array[j])
                                    string_shared_free (array[j]);
                            }
                            free (array);
                            free (string2);
                            return NULL;
                        }
                        free (array[i]);
                        array[i] = (char *)str_shared;
                    }
                }
                ptr1 = ++ptr2;
            }
            else
            {
                array[i] = NULL;
            }
        }
    }

    array[i] = NULL;
    if (num_items)
        *num_items = i;

    free (string2);

    return array;
}

/*
 * Splits a string according to separators.
 *
 * For full description, see function string_split_internal.
 */

char **
string_split (const char *string, const char *separators, int keep_eol,
              int num_items_max, int *num_items)
{
    return string_split_internal (string, separators, keep_eol,
                                  num_items_max, num_items, 0);
}

/*
 * Splits a string according to separators, and use shared strings for the
 * strings in the array returned.
 *
 * For full description, see function string_split_internal.
 */

char **
string_split_shared (const char *string, const char *separators, int keep_eol,
                     int num_items_max, int *num_items)
{
    return string_split_internal (string, separators, keep_eol,
                                  num_items_max, num_items, 1);
}

/*
 * Splits a string like the shell does for a command with arguments.
 *
 * This function is a C conversion of Python class "shlex"
 * (file: Lib/shlex.py in Python repository)
 * Doc: http://docs.python.org/3/library/shlex.html
 *
 * Copyrights in shlex.py:
 *   Module and documentation by Eric S. Raymond, 21 Dec 1998
 *   Input stacking and error message cleanup added by ESR, March 2000
 *   push_source() and pop_source() made explicit by ESR, January 2001.
 *   Posix compliance, split(), string arguments, and
 *   iterator interface by Gustavo Niemeyer, April 2003.
 *
 * Note: result must be freed with string_free_split.
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
        ptr_next = utf8_next_char (ptr_string);
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
    int i;

    if (split_string)
    {
        for (i = 0; split_string[i]; i++)
            free (split_string[i]);
        free (split_string);
    }
}

/*
 * Frees a split string (using shared strings).
 */

void
string_free_split_shared (char **split_string)
{
    int i;

    if (split_string)
    {
        for (i = 0; split_string[i]; i++)
            string_shared_free (split_string[i]);
        free (split_string);
    }
}

/*
 * Builds a string with a split string.
 *
 * Note: result must be free after use.
 */

char *
string_build_with_split_string (const char **split_string,
                                const char *separator)
{
    int i, length, length_separator;
    char *result;

    if (!split_string)
        return NULL;

    length = 0;
    length_separator = (separator) ? strlen (separator) : 0;

    for (i = 0; split_string[i]; i++)
    {
        length += strlen (split_string[i]) + length_separator;
    }

    result = malloc (length + 1);
    if (result)
    {
        result[0] = '\0';

        for (i = 0; split_string[i]; i++)
        {
            strcat (result, split_string[i]);
            if (separator && split_string[i + 1])
                strcat (result, separator);
        }
    }

    return result;
}

/*
 * Splits a list of commands separated by 'separator' and escaped with '\'.
 * Empty commands are removed, spaces on the left of each commands are stripped.
 *
 * Note: result must be freed with free_multi_command.
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
    while ((p = strchr(ptr, separator)) != NULL)
    {
        nb_substr++;
        ptr = ++p;
    }

    array = malloc ((nb_substr + 1) * sizeof (array[0]));
    if (!array)
        return NULL;

    buffer = malloc (strlen(command) + 1);
    if (!buffer)
    {
        free (array);
        return NULL;
    }

    ptr = command;
    str_idx = 0;
    arr_idx = 0;
    while(*ptr != '\0')
    {
        type = 0;
        if (*ptr == ';')
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
            /* strip white spaces a the beginning of the line */
            while (*p == ' ') p++;
            if (p  && p[0])
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
    while (*p == ' ') p++;
    if (p  && p[0])
        array[arr_idx++] = strdup (p);

    array[arr_idx] = NULL;

    free (buffer);

    array2 = realloc (array, (arr_idx + 1) * sizeof(array[0]));
    if (!array2)
    {
        if (array)
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
    int i;

    if (split_command)
    {
        for (i = 0; split_command[i]; i++)
            free (split_command[i]);
        free (split_command);
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
    char *inbuf, *ptr_inbuf, *ptr_outbuf, *next_char;
    char *ptr_inbuf_shift;
    int done;
    size_t err, inbytesleft, outbytesleft;
#endif

    if (!string)
        return NULL;

#ifdef HAVE_ICONV
    if (from_code && from_code[0] && to_code && to_code[0]
        && (string_strcasecmp(from_code, to_code) != 0))
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

    if (utf8_has_8bits (input) && utf8_is_valid (input, NULL))
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
 * Converts internal string to terminal charset, for display.
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

    /*
     * optimized for UTF-8: if charset is NULL => we use term charset => if
     * this charset is already UTF-8, then no iconv needed
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
string_iconv_fprintf (FILE *file, const char *data, ...)
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
        if (buf2)
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
 * Encodes a string in base16 (hexadecimal).
 *
 * Argument "length" is number of bytes in "from" to convert (commonly
 * strlen(from)).
 */

void
string_encode_base16 (const char *from, int length, char *to)
{
    int i;
    const char *hexa = "0123456789ABCDEF";
    char *ptr_to;

    if (!from || !to)
        return;

    ptr_to = to;
    ptr_to[0] = '\0';
    for (i = 0; i < length; i++)
    {
        ptr_to[0] = hexa[((unsigned char)from[i]) / 16];
        ptr_to[1] = hexa[((unsigned char)from[i]) % 16];
        ptr_to += 2;
    }
    ptr_to[0] = '\0';
}

/*
 * Decodes a base16 string (hexadecimal).
 *
 * Returns length of string in "*to" (it does not count final \0).
 */

int
string_decode_base16 (const char *from, char *to)
{
    int length, to_length, i, pos;
    unsigned char *ptr_to, value;

    if (!from || !to)
        return 0;

    length = strlen (from) / 2;

    ptr_to = (unsigned char *)to;
    ptr_to[0] = '\0';
    to_length = 0;

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

        ptr_to[0] = value;
        ptr_to++;
        to_length++;
    }
    ptr_to[0] = '\0';

    return to_length;
}

/*
 * Converts 3 bytes of 8 bits in 4 bytes of 6 bits.
 */

void
string_convbase64_8x3_to_6x4 (const char *from, char *to)
{
    unsigned char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz0123456789+/";

    to[0] = base64_table [ (from[0] & 0xfc) >> 2 ];
    to[1] = base64_table [ ((from[0] & 0x03) << 4) + ((from[1] & 0xf0) >> 4) ];
    to[2] = base64_table [ ((from[1] & 0x0f) << 2) + ((from[2] & 0xc0) >> 6) ];
    to[3] = base64_table [ from[2] & 0x3f ];
}

/*
 * Encodes a string in base64.
 *
 * Argument "length" is number of bytes in "from" to convert (commonly
 * strlen(from)).
 */

void
string_encode_base64 (const char *from, int length, char *to)
{
    const char *ptr_from;
    char *ptr_to;

    if (!from || !to)
        return;

    ptr_from = from;
    ptr_to = to;

    while (length >= 3)
    {
        string_convbase64_8x3_to_6x4 (ptr_from, ptr_to);
        ptr_from += 3 * sizeof (*ptr_from);
        ptr_to += 4 * sizeof (*ptr_to);
        length -= 3;
    }

    if (length > 0)
    {
        char rest[3] = { 0, 0, 0 };
        switch (length)
        {
            case 1 :
                rest[0] = ptr_from[0];
                string_convbase64_8x3_to_6x4 (rest, ptr_to);
                ptr_to[2] = ptr_to[3] = '=';
                break;
            case 2 :
                rest[0] = ptr_from[0];
                rest[1] = ptr_from[1];
                string_convbase64_8x3_to_6x4 (rest, ptr_to);
                ptr_to[3] = '=';
                break;
        }
        ptr_to[4] = 0;
    }
    else
        ptr_to[0] = '\0';
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
 * Returns length of string in "*to" (it does not count final \0).
 */

int
string_decode_base64 (const char *from, char *to)
{
    const char *ptr_from;
    int length, to_length, i;
    char *ptr_to;
    unsigned char c, in[4], out[3];
    unsigned char base64_table[]="|$$$}rstuvwxyz{$$$$$$$>?"
        "@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

    if (!from || !to)
        return 0;

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
        if (utf8_charcmp (ptr_command_chars, string) == 0)
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
    char *pos_slash, *pos_space, *next_char;

    if (!string)
        return NULL;

    /* special case for C comments pasted in input line */
    if (strncmp (string, "/*", 2) == 0)
        return string;

    /*
     * special case if string starts with '/': to allow to paste a path line
     * "/path/to/file.txt", we check if next '/' is after a space or not
     */
    if (string[0] == '/')
    {
        pos_slash = strchr (string + 1, '/');
        pos_space = strchr (string + 1, ' ');

        /*
         * if there's no other '/' or if '/' is after first space,
         * then it is a command, and return NULL
         */
        if (!pos_slash || (pos_space && pos_slash > pos_space))
            return NULL;

        return (string[1] == '/') ? string + 1 : string;
    }

    /* if string does not start with a command char, then it's not command */
    if (!string_is_command_char (string))
        return string;

    next_char = utf8_next_char (string);

    /* there's no next char, then it's a command */
    if (!next_char || !next_char[0])
        return NULL;

    /* check if first char is doubled: if yes, then it's not a command */
    if (utf8_charcmp (string, next_char) == 0)
        return next_char;

    /* string is a command */
    return NULL;
}

/*
 * Replaces ${vars} using a callback that returns replacement value (this value
 * must be newly allocated because it will be freed in this function).
 *
 * Nested variables are supported, for example: "${var1:${var2}}".
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
                              char *(*callback)(void *data, const char *text),
                              void *callback_data,
                              int *errors)
{
    int length_prefix, length_suffix, length, length_value, index_string;
    int index_result, sub_count, sub_level, sub_errors;
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
                index_string++;
                result[index_result++] = string[index_string++];
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
                    if ((pos_end_name[0] == '\\')
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
                /* prefix without matching suffix => error! */
                if (!pos_end_name[0])
                {
                    result[index_result] = '\0';
                    if (errors)
                        (*errors)++;
                    return result;
                }
                key = string_strndup (string + index_string + length_prefix,
                                      pos_end_name - (string + index_string + length_prefix));
                if (key)
                {
                    if (sub_count > 0)
                    {
                        sub_errors = 0;
                        key2 = string_replace_with_callback (key, prefix,
                                                             suffix, callback,
                                                             callback_data,
                                                             &sub_errors);
                        if (errors)
                            (*errors) += sub_errors;
                        free (key);
                        key = key2;
                    }
                    value = (*callback) (callback_data, (key) ? key : "");
                    if (value)
                    {
                        length_value = strlen (value);
                        if (length_value > 0)
                        {
                            length += length_value;
                            result2 = realloc (result, length);
                            if (!result2)
                            {
                                if (result)
                                    free (result);
                                if (key)
                                    free (key);
                                free (value);
                                return NULL;
                            }
                            result = result2;
                            strcpy (result + index_result, value);
                            index_result += length_value;
                        }
                        index_string = pos_end_name - string + length_suffix;
                        free (value);
                    }
                    else
                    {
                        result[index_result++] = string[index_string++];
                        if (errors)
                            (*errors)++;
                    }
                    if (key)
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

    ptr_count = (string_shared_count_t *)(string - sizeof (string_shared_count_t));

    (*ptr_count)--;

    if (*ptr_count == 0)
        hashtable_remove (string_hashtable_shared, ptr_count);
}

/*
 * Frees all allocated data.
 */

void
string_end ()
{
    if (string_hashtable_shared)
    {
        hashtable_free (string_hashtable_shared);
        string_hashtable_shared = NULL;
    }
}
