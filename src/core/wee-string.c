/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * wee-string.c: string functions for WeeChat
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
#include "wee-utf8.h"
#include "../gui/gui-color.h"
#include "../plugins/plugin.h"


/*
 * string_strndup: define strndup function for systems where this function does
 *                 not exist (FreeBSD and maybe other)
 */

char *
string_strndup (const char *string, int length)
{
    char *result;

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
 * string_tolower: locale independant string conversion to lower case
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
 * string_toupper: locale independant string conversion to upper case
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
 * string_strcasecmp: locale and case independent string comparison
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
            return diff;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
    }

    return (string1[0]) ? 1 : ((string2[0]) ? -1 : 0);
}

/*
 * string_strcasecmp_range: locale and case independent string comparison
 *                          using range to compare case:
 *                          - range = 26: A-Z         ==> a-z
 *                          - range = 29: A-Z [ \ ]   ==> a-z { | }
 *                          - range = 30: A-Z [ \ ] ^ ==> a-z { | } ~
 *                          (ranges 29 and 30 are used by some protocols like
 *                          IRC)
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
            return diff;

        string1 = utf8_next_char (string1);
        string2 = utf8_next_char (string2);
    }

    return (string1[0]) ? 1 : ((string2[0]) ? -1 : 0);
}

/*
 * string_strncasecmp: locale and case independent string comparison
 *                     with max length
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
            return diff;

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
 * string_strncasecmp_range: locale and case independent string comparison
 *                           with max length, using range to compare case:
 *                           - range = 26: A-Z         ==> a-z
 *                           - range = 29: A-Z [ \ ]   ==> a-z { | }
 *                           - range = 30: A-Z [ \ ] ^ ==> a-z { | } ~
 *                           (ranges 29 and 30 are used by some protocols like
 *                           IRC)
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
            return diff;

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
 * string_strcmp_ignore_chars: compare 2 strings, ignoring ignore some chars
 */

int
string_strcmp_ignore_chars (const char *string1, const char *string2,
                            const char *chars_ignored, int case_sensitive)
{
    int diff;

    if (!string1 && !string2)
        return 0;
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
    if ((!string1 || !string1[0]) && string2 && string2[0])
        return -1;
    if (string1 && string1[0] && (!string2 || !string2[0]))
        return 1;
    return 0;
}

/*
 * string_strcasestr: locale and case independent string search
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
 * string_match: return 1 if string matches a mask
 *               mask can begin or end with "*", no other "*" are allowed
 *               inside mask
 */

int
string_match (const char *string, const char *mask, int case_sensitive)
{
    char last, *mask2;
    int len_string, len_mask, rc;

    if (!mask || !mask[0])
        return 0;

    /* if mask is "*", then any string matches */
    if (strcmp (mask, "*") == 0)
        return 1;

    len_string = strlen (string);
    len_mask = strlen (mask);

    last = mask[len_mask - 1];

    /* mask begins with "*" */
    if ((mask[0] == '*') && (last != '*'))
    {
        /* not enough chars in string to match */
        if (len_string < len_mask - 1)
            return 0;
        /* check if end of string matches */
        if ((case_sensitive && (strcmp (string + len_string - (len_mask - 1),
                                        mask + 1) == 0))
            || (!case_sensitive && (string_strcasecmp (string + len_string - (len_mask - 1),
                                                       mask + 1) == 0)))
            return 1;
        /* no match */
        return 0;
    }

    /* mask ends with "*" */
    if ((mask[0] != '*') && (last == '*'))
    {
        /* not enough chars in string to match */
        if (len_string < len_mask - 1)
            return 0;
        /* check if beginning of string matches */
        if ((case_sensitive && (strncmp (string, mask, len_mask - 1) == 0))
            || (!case_sensitive && (string_strncasecmp (string,
                                                        mask,
                                                        len_mask - 1) == 0)))
            return 1;
        /* no match */
        return 0;
    }

    /* mask begins and ends with "*" */
    if ((mask[0] == '*') && (last == '*'))
    {
        /* not enough chars in string to match */
        if (len_string < len_mask - 1)
            return 0;
        /* keep only relevant chars in mask for searching string */
        mask2 = string_strndup (mask + 1, len_mask - 2);
        if (!mask2)
            return 0;
        /* search string */
        rc = ((case_sensitive && strstr (string, mask2))
              || (!case_sensitive && string_strcasestr (string, mask2))) ?
            1 : 0;
        /* free and return */
        free (mask2);
        return rc;
    }

    /* no "*" at all, compare strings */
    if ((case_sensitive && (strcmp (string, mask) == 0))
        || (!case_sensitive && (string_strcasecmp (string, mask) == 0)))
        return 1;

    /* no match */
    return 0;
}

/*
 * string_replace: replace a string by new one in a string
 *                 note: returned value has to be free() after use
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
 * string_expand_home: expand home in a PATH
 *                     (for example: "~/file.txt" => "/home/xxx/file.txt")
 *                     note: returned value has to be free() after use
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
 * string_remove_quotes: remove quotes at beginning/end of string
 *                       (ignore spaces if there are before first quote or
 *                        after last quote)
 *                       note: returned value has to be free() after use
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
 * string_strip: strip chars at beginning and/or end of string
 *               note: returned value has to be free() after use
 */

char *
string_strip (const char *string, int left, int right, const char *chars)
{
    const char *ptr_start, *ptr_end;

    if (!string)
        return NULL;

    if (!string[0])
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
 * string_convert_hex_chars: convert hex chars (\x??) to value
 *                           note: returned value has to be free() after use
 */

char *
string_convert_hex_chars (const char *string)
{
    char *output, hex_str[8], *error;
    int pos_output;
    long number;

    output = malloc (strlen (string) + 1);
    if (output)
    {
        pos_output = 0;
        while (string && string[0])
        {
            if (string[0] == '\\')
            {
                string++;
                switch (string[0])
                {
                    case '\\':
                        output[pos_output++] = '\\';
                        string++;
                        break;
                    case 'x':
                    case 'X':
                        if (isxdigit ((unsigned char)string[1])
                            && isxdigit ((unsigned char)string[2]))
                        {
                            snprintf (hex_str, sizeof (hex_str),
                                      "0x%c%c", string[1], string[2]);
                            number = strtol (hex_str, &error, 16);
                            if (error && !error[0])
                            {
                                output[pos_output++] = number;
                                string += 3;
                            }
                            else
                            {
                                output[pos_output++] = '\\';
                                output[pos_output++] = string[0];
                                string++;
                            }
                        }
                        else
                        {
                            output[pos_output++] = string[0];
                            string++;
                        }
                        break;
                    default:
                        output[pos_output++] = '\\';
                        output[pos_output++] = string[0];
                        string++;
                        break;
                }
            }
            else
            {
                output[pos_output++] = string[0];
                string++;
            }
        }
        output[pos_output] = '\0';
    }

    return output;
}

/*
 * string_is_word_char: return 1 if given character is a "word character"
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
 * string_mask_to_regex: convert a mask (string with only "*" as wildcard) to a
 *                       regex, paying attention to special chars in a regex
 */

char *
string_mask_to_regex (const char *mask)
{
    char *result;
    const char *ptr_mask;
    int index_result;
    char *regex_special_char = ".[]{}()?+";

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
 * string_regex_flags: get pointer on string after flags and return mask with
 *                     flags to compile regex
 *                     format of flags is:
 *                       (?eins-eins)string
 *                     flags:
 *                       e: POSIX extended regex (REG_EXTENDED)
 *                       i: case insensitive (REG_ICASE)
 *                       n: match-any-character operators don't match a newline (REG_NEWLINE)
 *                       s: support for substring addressing of matches is not required (REG_NOSUB)
 *                     examples (with default_flags = REG_EXTENDED):
 *                       "(?i)toto"  : regex "toto", flags = REG_EXTENDED | REG_ICASE
 *                       "(?i)toto"  : regex "toto", flags = REG_EXTENDED | REG_ICASE
 *                       "(?i-e)toto": regex "toto", flags = REG_ICASE
 */

const char *
string_regex_flags (const char *regex, int default_flags, int *flags)
{
    const char *ptr_regex, *ptr_flags;
    int set_flag, flag;
    char *pos;

    ptr_regex = regex;
    if (flags)
        *flags = default_flags;

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
 * string_regcomp: compile a regex using optional flags at beginning of string
 *                 (for format of flags in regex, see string_regex_flags())
 */

int
string_regcomp (void *preg, const char *regex, int default_flags)
{
    const char *ptr_regex;
    int flags;

    ptr_regex = string_regex_flags (regex, default_flags, &flags);
    return regcomp ((regex_t *)preg, ptr_regex, flags);
}

/*
 * string_has_highlight: return 1 if string contains a highlight (using list of
 *                       words to highlight)
 *                       return 0 if no highlight is found in string
 */

int
string_has_highlight (const char *string, const char *highlight_words)
{
    char *msg, *highlight, *match, *match_pre, *match_post, *msg_pos;
    char *pos, *pos_end, *ptr_str, *ptr_string_ref;
    int end, length, startswith, endswith, wildcard_start, wildcard_end, flags;

    if (!string || !string[0] || !highlight_words || !highlight_words[0])
        return 0;

    msg = strdup (string);
    if (!msg)
        return 0;
    string_tolower (msg);
    highlight = strdup (highlight_words);
    if (!highlight)
    {
        free (msg);
        return 0;
    }
    string_tolower (highlight);

    pos = highlight;
    end = 0;
    while (!end)
    {
        ptr_string_ref = (char *)string;
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

        if (flags & REG_ICASE)
        {
            for (ptr_str = pos; ptr_str < pos_end; ptr_str++)
            {
                if ((ptr_str[0] >= 'A') && (ptr_str[0] <= 'Z'))
                    ptr_str[0] += ('a' - 'A');
            }
            ptr_string_ref = msg;
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
            msg_pos = ptr_string_ref;
            /* highlight found! */
            while ((match = strstr (msg_pos, pos)) != NULL)
            {
                match_pre = utf8_prev_char (ptr_string_ref, match);
                if (!match_pre)
                    match_pre = match - 1;
                match_post = match + length;
                startswith = ((match == ptr_string_ref) || (!string_is_word_char (match_pre)));
                endswith = ((!match_post[0]) || (!string_is_word_char (match_post)));
                if ((wildcard_start && wildcard_end) ||
                    (!wildcard_start && !wildcard_end &&
                     startswith && endswith) ||
                    (wildcard_start && endswith) ||
                    (wildcard_end && startswith))
                {
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
 * string_has_highlight_regex_compiled: return 1 if string contains a highlight
 *                                      using a regular expression (any match
 *                                      in string must be surrounded by word
 *                                      chars)
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
        if ((rc != 0) || (regex_match.rm_so < 0) || (regex_match.rm_eo < 0))
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
 * string_has_highlight_regex: return 1 if string contains a highlight
 *                             using a regular expression (any match in string
 *                             must be surrounded by word chars)
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
 * string_split: split a string according to separators
 *               examples:
 *                 string_split ("abc de  fghi", " ", 0, 0, NULL)
 *                   ==> array[0] = "abc"
 *                       array[1] = "de"
 *                       array[2] = "fghi"
 *                       array[3] = NULL
 *                 string_split ("abc de  fghi", " ", 1, 0, NULL)
 *                   ==> array[0] = "abc de  fghi"
 *                       array[1] = "de  fghi"
 *                       array[2] = "fghi"
 *                       array[3] = NULL
 */

char **
string_split (const char *string, const char *separators, int keep_eol,
              int num_items_max, int *num_items)
{
    int i, j, n_items;
    char *string2, **array;
    char *ptr, *ptr1, *ptr2;

    if (num_items != NULL)
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
        while (ptr[0] && (strchr (separators, ptr[0]) != NULL))
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
        return NULL;

    ptr1 = string2;

    for (i = 0; i < n_items; i++)
    {
        while (ptr1[0] && (strchr (separators, ptr1[0]) != NULL))
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
                    array[i] = strdup (ptr1);
                    if (!array[i])
                    {
                        for (j = 0; j < n_items; j++)
                        {
                            if (array[j])
                                free (array[j]);
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
                                free (array[j]);
                        }
                        free (array);
                        free (string2);
                        return NULL;
                    }
                    strncpy (array[i], ptr1, ptr2 - ptr1);
                    array[i][ptr2 - ptr1] = '\0';
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
    if (num_items != NULL)
        *num_items = i;

    free (string2);

    return array;
}

/*
 * string_free_split: free a split string
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
 * string_build_with_split_string: build a string with a split string
 *                                 note: returned value has to be free() after
 *                                 use
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
 * string_split_command: split a list of commands separated by 'separator'
 *                       and ecscaped with '\'
 *                       - empty commands are removed
 *                       - spaces on the left of each commands are stripped
 *                       Result must be freed with free_multi_command
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
    while ( (p = strchr(ptr, separator)) != NULL)
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
            /* strip white spaces a the begining of the line */
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
 * string_free_split_command : free a list of commands split
 *                             with string_split_command
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
 * string_iconv: convert string to another charset
 *               note: returned value has to be free() after use
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
 * string_iconv_to_internal: convert user string (input, script, ..) to
 *                           WeeChat internal storage charset
 *                           note: returned value has to be free() after use
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
 * string_iconv_from_internal: convert internal string to terminal charset,
 *                             for display
 *                             note: returned value has to be free() after use
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
 * string_iconv_fprintf: encode to terminal charset, then call fprintf on a file
 *                       return 1 if ok, 0 if error
 */

int
string_iconv_fprintf (FILE *file, const char *data, ...)
{
    char *buf2;
    int rc, num_written;

    rc = 0;
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
 * string_format_size: format a string with size and unit name (bytes, KB, MB, GB)
 *                     note: returned value has to be free() after use
 */

char *
string_format_size (unsigned long long size)
{
    char *unit_name[] = { N_("bytes"), N_("KB"), N_("MB"), N_("GB") };
    char *unit_format[] = { "%.0f", "%.1f", "%.02f", "%.02f" };
    float unit_divide[] = { 1, 1024, 1024*1024, 1024*1024*1024 };
    char format_size[128], str_size[128];
    int num_unit;

    str_size[0] = '\0';

    if (size < 1024*10)
        num_unit = 0;
    else if (size < 1024*1024)
        num_unit = 1;
    else if (size < 1024*1024*1024)
        num_unit = 2;
    else
        num_unit = 3;

    snprintf (format_size, sizeof (format_size),
              "%s %%s",
              unit_format[num_unit]);
    snprintf (str_size, sizeof (str_size),
              format_size,
              ((float)size) / ((float)(unit_divide[num_unit])),
              (size <= 1) ? _("byte") : _(unit_name[num_unit]));

    return strdup (str_size);
}

/*
 * string_convbase64_8x3_to_6x4 : convert 3 bytes of 8 bits in 4 bytes of 6 bits
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
 * string_encode_base64: encode a string in base64
 *                       length is number of bytes in "from" to convert
 *                       (commonly strlen(from))
 */

void
string_encode_base64 (const char *from, int length, char *to)
{
    const char *ptr_from;
    char *ptr_to;

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
 * string_convbase64_6x4_to_8x3 : convert 4 bytes of 6 bits to 3 bytes of 8 bits
 */

void
string_convbase64_6x4_to_8x3 (const unsigned char *from, unsigned char *to)
{
    to[0] = from[0] << 2 | from[1] >> 4;
    to[1] = from[1] << 4 | from[2] >> 2;
    to[2] = ((from[2] << 6) & 0xc0) | from[3];
}

/*
 * string_decode_base64: decode a base64 string
 *                       return length of string in *to
 *                       (does not count final \0)
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

    ptr_from = from;
    ptr_to = to;

    ptr_to[0] = '\0';
    to_length = 0;

    while (ptr_from && ptr_from[0])
    {
        length = 0;
        for (i = 0; i < 4; i++)
        {
            c = 0;
            while (ptr_from[0] && (c == 0))
            {
                c = (unsigned char) ptr_from[0];
                ptr_from++;
                c = ((c < 43) || (c > 122)) ? 0 : base64_table[c - 43];
                if (c)
                    c = (c == '$') ? 0 : c - 61;
            }
            if (ptr_from[0])
            {
                length++;
                if (c)
                    in[i] = c - 1;
            }
            else
            {
                in[i] = '\0';
                break;
            }
        }
        if (length)
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
 * string_is_command_char: return 1 if first char of string is a command char,
 *                         otherwise 0
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
 * string_input_for_buffer: return pointer to input text for buffer (pointer
 *                          inside "string" argument)
 *                          or return NULL if it's a command
 *                          (by default, a command starts with a single '/')
 */

const char *
string_input_for_buffer (const char *string)
{
    char *pos_slash, *pos_space, *next_char;

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
         * if there's no other '/' of if '/' is after first space,
         * then it is a command, and return NULL
         */
        if (!pos_slash || (pos_space && pos_slash > pos_space))
            return NULL;

        return (string[1] == '/') ? string + 1 : string;
    }

    /* if string does not start with a command char, then it's not command */
    if (!string_is_command_char (string))
        return string;

    /* check if first char is doubled: if yes, then it's not a command */
    next_char = utf8_next_char (string);
    if (!next_char || !next_char[0])
        return string;
    if (utf8_charcmp (string, next_char) == 0)
        return next_char;

    /* string is a command */
    return NULL;
}

/*
 * string_replace_with_callback: replace ${codes} using a callback that return
 *                               replacement value (this value must be newly
 *                               allocated because it will be freed in this
 *                               function)
 *                               "errors" is set with number of keys not found
 *                               by callback
 */

char *
string_replace_with_callback (const char *string,
                              char *(*callback)(void *data, const char *text),
                              void *callback_data,
                              int *errors)
{
    int length, length_value, index_string, index_result;
    char *result, *result2, *key, *value;
    const char *pos_end_name;

    *errors = 0;

    if (!string)
        return NULL;

    length = strlen (string) + 1;
    result = malloc (length);
    if (result)
    {
        index_string = 0;
        index_result = 0;
        while (string[index_string])
        {
            if ((string[index_string] == '\\')
                && (string[index_string + 1] == '$'))
            {
                index_string++;
                result[index_result++] = string[index_string++];
            }
            else if ((string[index_string] == '$')
                     && (string[index_string + 1] == '{'))
            {
                pos_end_name = strchr (string + index_string + 2, '}');
                if (pos_end_name)
                {
                    key = string_strndup (string + index_string + 2,
                                          pos_end_name - (string + index_string + 2));
                    if (key)
                    {
                        value = (*callback) (callback_data, key);
                        if (value)
                        {
                            length_value = strlen (value);
                            length += length_value;
                            result2 = realloc (result, length);
                            if (!result2)
                            {
                                if (result)
                                    free (result);
                                free (key);
                                free (value);
                                return NULL;
                            }
                            result = result2;
                            strcpy (result + index_result, value);
                            index_result += length_value;
                            index_string += pos_end_name - string -
                                index_string + 1;
                            free (value);
                        }
                        else
                        {
                            result[index_result++] = string[index_string++];
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
            else
                result[index_result++] = string[index_string++];
        }
        result[index_result] = '\0';
    }

    return result;
}
