/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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


/*
 * strndup: define strndup function if not existing (FreeBSD and maybe other)
 */

#ifndef HAVE_STRNDUP
char *
strndup (char *string, int length)
{
    char *result;
    
    if ((int)strlen (string) < length)
        return strdup (string);
    
    result = (char *)malloc (length + 1);
    if (!result)
        return NULL;
    
    memcpy (result, string, length);
    result[length] = '\0';
    
    return result;
}
#endif

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
string_strcasecmp (char *string1, char *string2)
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
string_strncasecmp (char *string1, char *string2, int max)
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
string_strcmp_ignore_chars (char *string1, char *string2, char *chars_ignored,
                            int case_sensitive)
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
        /* skip digits */
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
        
        /* skip digits */
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
string_strcasestr (char *string, char *search)
{
    int length_search;
    
    length_search = strlen (search);
    
    if (!string || !search || (length_search == 0))
        return NULL;
    
    while (string[0])
    {
        if (string_strncasecmp (string, search, length_search) == 0)
            return string;
        
        string++;
    }
    
    return NULL;
}

/*
 * string_replace: replace a string by new one in a string
 *                 note: returned value has to be free() after use
 */

char *
string_replace (char *string, char *search, char *replace)
{
    char *pos, *new_string;
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
    new_string = (char *)malloc (length_new * sizeof (char));
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
string_remove_quotes (char *string, char *quotes)
{
    int length;
    char *pos_start, *pos_end;
    
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
        return strndup (pos_start + 1, pos_end - pos_start - 1);
    }
    
    return strdup (string);
}

/*
 * string_convert_hex_chars: convert hex chars (\x??) to value
 */

char *
string_convert_hex_chars (char *string)
{
    char *output, hex_str[8], *error;
    int pos_output;
    long number;

    output = (char *)malloc (strlen (string) + 1);
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
                            if (error && (error[0] == '\0'))
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
string_explode (char *string, char *separators, int keep_eol,
                int num_items_max, int *num_items)
{
    int i, n_items;
    char **array;
    char *ptr, *ptr1, *ptr2;

    if (num_items != NULL)
        *num_items = 0;
    
    if (!string || !string[0] || !separators || !separators[0])
        return NULL;
    
    /* calculate number of items */
    ptr = string;
    i = 1;
    while ((ptr = strpbrk (ptr, separators)))
    {
        while (ptr[0] && (strchr (separators, ptr[0]) != NULL))
            ptr++;
        i++;
    }
    n_items = i;

    if ((num_items_max != 0) && (n_items > num_items_max))
        n_items = num_items_max;
    
    array =
        (char **)malloc ((n_items + 1) * sizeof (char *));
    
    ptr1 = string;
    ptr2 = string;
    
    for (i = 0; i < n_items; i++)
    {
        while (ptr1[0] && (strchr (separators, ptr1[0]) != NULL))
            ptr1++;
        if (i == (n_items - 1) || (ptr2 = strpbrk (ptr1, separators)) == NULL)
            if ((ptr2 = strchr (ptr1, '\r')) == NULL)
                if ((ptr2 = strchr (ptr1, '\n')) == NULL)
                    ptr2 = strchr (ptr1, '\0');
        
        if ((ptr1 == NULL) || (ptr2 == NULL))
        {
            array[i] = NULL;
        }
        else
        {
            if (ptr2 - ptr1 > 0)
            {
                if (keep_eol)
                    array[i] = strdup (ptr1);
                else
                {
                    array[i] =
                        (char *)malloc ((ptr2 - ptr1 + 1) * sizeof (char));
                    array[i] = strncpy (array[i], ptr1, ptr2 - ptr1);
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
 * string_split_command: split a list of commands separated by 'sep'
 *                       and ecscaped with '\'
 *                       - empty commands are removed
 *                       - spaces on the left of each commands are stripped
 *                       Result must be freed with free_multi_command
 */

char **
string_split_command (char *command, char separator)
{
    int nb_substr, arr_idx, str_idx, type;
    char **array;
    char *buffer, *ptr, *p;

    if (!command || !command[0])
	return NULL;
    
    nb_substr = 1;
    ptr = command;
    while ( (p = strchr(ptr, separator)) != NULL)
    {
	nb_substr++;
	ptr = ++p;
    }

    array = (char **)malloc ((nb_substr + 1) * sizeof(char *));
    if (!array)
	return NULL;
    
    buffer = (char *)malloc ( (strlen(command) + 1) * sizeof (char));
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

    array = (char **)realloc (array, (arr_idx + 1) * sizeof(char *));

    return array;
}

/*
 * string_free_splitted_command : free a list of commands splitted
 *                                with string_split_command
 */

void
string_free_splitted_command (char **splitted_command)
{
    int i;

    if (splitted_command)
    {
        for (i = 0; splitted_command[i]; i++)
            free (splitted_command[i]);
        free (splitted_command);
    }
}

/*
 * string_iconv: convert string to another charset
 */

char *
string_iconv (int from_utf8, char *from_code, char *to_code, char *string)
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
            ptr_inbuf = inbuf;
            inbytesleft = strlen (inbuf);
            outbytesleft = inbytesleft * 4;
            outbuf = (char *)malloc (outbytesleft + 2);
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
 */

char *
string_iconv_to_internal (char *charset, char *string)
{
    char *input, *output;
    
    if (!string)
        return NULL;
    
    input = strdup (string);
    
    /* optimize for UTF-8: if charset is NULL => we use term charset =>
       if ths charset is already UTF-8, then no iconv needed */
    if (local_utf8 && (!charset || !charset[0]))
        return input;
    
    if (input)
    {
        if (utf8_has_8bits (input) && utf8_is_valid (input, NULL))
            return input;
        
        output = string_iconv (0,
                               (charset && charset[0]) ?
                               charset : local_charset,
                               WEECHAT_INTERNAL_CHARSET,
                               input);
        utf8_normalize (output, '?');
        free (input);
        return output;
    }
    return NULL;
}

/*
 * string_iconv_from_internal: convert internal string to terminal charset,
 *                             for display
 */

char *
string_iconv_from_internal (char *charset, char *string)
{
    char *input, *output;
    
    if (!string)
        return NULL;
    
    input = strdup (string);
    
    /* optimize for UTF-8: if charset is NULL => we use term charset =>
       if ths charset is already UTF-8, then no iconv needed */
    if (local_utf8 && (!charset || !charset[0]))
        return input;
    
    if (input)
    {
        utf8_normalize (input, '?');
        output = string_iconv (1,
                               WEECHAT_INTERNAL_CHARSET,
                               (charset && charset[0]) ?
                               charset : local_charset,
                               input);
        free (input);
        return output;
    }
    return NULL;
}

/*
 * string_iconv_fprintf: encode to terminal charset, then call fprintf on a file
 */

void
string_iconv_fprintf (FILE *file, char *data, ...)
{
    va_list argptr;
    static char buf[4096];
    char *buf2;
    
    va_start (argptr, data);
    vsnprintf (buf, sizeof (buf) - 1, data, argptr);
    va_end (argptr);
    
    buf2 = string_iconv_from_internal (NULL, buf);
    fprintf (file, "%s", (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}
