/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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

/* wee-string.c: string functions for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>

#if defined(__OpenBSD__)
#include <utf8/wchar.h>
#else
#include <wchar.h>
#endif

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
#include "wee-utf8.h"
#include "../gui/gui-color.h"


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
    
    /* replace all occurences */
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
                        if (isxdigit (string[1])
                            && isxdigit (string[2]))
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
 * string_has_highlight: return 1 if string contains a highlight (using list of
 *                       words to highlight)
 *                       return 0 if no highlight is found in string
 */

int
string_has_highlight (const char *string, const char *highlight_words)
{
    char *msg, *highlight, *match, *match_pre, *match_post, *msg_pos, *pos, *pos_end;
    int end, length, startswith, endswith, wildcard_start, wildcard_end;
    
    if (!string || !string[0] || !highlight_words || !highlight_words[0])
        return 0;
    
    /* convert both strings to lower case */
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
            /* highlight found! */
            while ((match = strstr (msg_pos, pos)) != NULL)
            {
                match_pre = match - 1;
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
 * string_mask_to_regex: convert a mask (string with only "*" as joker) to a
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
 * string_explode: explode a string according to separators
 *                 examples:
 *                   string_explode ("abc de  fghi", " ", 0, 0, NULL)
 *                     ==> array[0] = "abc"
 *                         array[1] = "de"
 *                         array[2] = "fghi"
 *                         array[3] = NULL
 *                   string_explode ("abc de  fghi", " ", 1, 0, NULL)
 *                     ==> array[0] = "abc de  fghi"
 *                         array[1] = "de  fghi"
 *                         array[2] = "fghi"
 *                         array[3] = NULL
 */

char **
string_explode (const char *string, const char *separators, int keep_eol,
                int num_items_max, int *num_items)
{
    int i, j, n_items;
    char *string2, **array;
    char *ptr, *ptr1, *ptr2;
    
    if (num_items != NULL)
        *num_items = 0;
    
    if (!string || !string[0] || !separators || !separators[0])
        return NULL;
    
    string2 = string_strip (string, 1, 1, separators);
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
    ptr2 = string2;
    
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
 * string_free_exploded: free an exploded string
 */

void
string_free_exploded (char **exploded_string)
{
    int i;
    
    if (exploded_string)
    {
        for (i = 0; exploded_string[i]; i++)
            free (exploded_string[i]);
        free (exploded_string);
    }
}

/*
 * string_build_with_exploded: build a string with exploded string
 *                             note: returned value has to be free() after use
 */

char *
string_build_with_exploded (const char **exploded_string, const char *separator)
{
    int i, length, length_separator;
    char *result;
    
    if (!exploded_string)
        return NULL;
    
    length = 0;
    length_separator = (separator) ? strlen (separator) : 0;
    
    for (i = 0; exploded_string[i]; i++)
    {
        length += strlen (exploded_string[i]) + length_separator;
    }
    
    result = malloc (length + 1);
    if (result)
    {
        result[0] = '\0';
        
        for (i = 0; exploded_string[i]; i++)
        {
            strcat (result, exploded_string[i]);
            if (separator && exploded_string[i + 1])
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
    char **array;
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

    array = realloc (array, (arr_idx + 1) * sizeof(array[0]));

    return array;
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
    
    /* optimize for UTF-8: if charset is NULL => we use term charset =>
       if ths charset is already UTF-8, then no iconv needed */
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
    
    /* optimize for UTF-8: if charset is NULL => we use term charset =>
       if ths charset is already UTF-8, then no iconv needed */
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
 */

void
string_iconv_fprintf (FILE *file, const char *data, ...)
{
    va_list argptr;
    char *buf, *buf2;

    buf = malloc (128 * 1024);
    if (!buf)
        return;
    
    va_start (argptr, data);
    vsnprintf (buf, 128 * 1024, data, argptr);
    va_end (argptr);
    
    buf2 = string_iconv_from_internal (NULL, buf);
    fprintf (file, "%s", (buf2) ? buf2 : buf);
    
    free (buf);
    if (buf2)
        free (buf2);
}

/*
 * string_format_size: format a string with size and unit name (bytes, KB, MB, GB)
 *                     note: returned value has to be free() after use
 */

char *
string_format_size (unsigned long size)
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
