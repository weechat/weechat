/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* util.c: some useful functions for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include "weechat.h"
#include "utf8.h"
#include "weeconfig.h"


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
 * ascii_tolower: locale independant string conversion to lower case
 */

void
ascii_tolower (char *string)
{
    while (string && string[0])
    {
        if ((string[0] >= 'A') && (string[0] <= 'Z'))
            string[0] += ('a' - 'A');
        string++;
    }
}

/*
 * ascii_toupper: locale independant string conversion to upper case
 */

void
ascii_toupper (char *string)
{
    while (string && string[0])
    {
        if ((string[0] >= 'a') && (string[0] <= 'z'))
            string[0] -= ('a' - 'A');
        string++;
    }
}

/*
 * ascii_strcasecmp: locale and case independent string comparison
 */

int
ascii_strcasecmp (char *string1, char *string2)
{
    int c1, c2;
    
    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);
    
    while (string1[0] && string2[0])
    {
        c1 = (int)((unsigned char) string1[0]);
        c2 = (int)((unsigned char) string2[0]);
        
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 += ('a' - 'A');
        
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 += ('a' - 'A');
        
        if ((c1 - c2) != 0)
            return c1 - c2;
        
        string1++;
        string2++;
    }
    
    return (string1[0]) ? 1 : ((string2[0]) ? -1 : 0);
}

/*
 * ascii_strncasecmp: locale and case independent string comparison
 *                    with max length
 */

int
ascii_strncasecmp (char *string1, char *string2, int max)
{
    int c1, c2, count;
    
    if (!string1 || !string2)
        return (string1) ? 1 : ((string2) ? -1 : 0);
    
    count = 0;
    while ((count < max) && string1[0] && string2[0])
    {
        c1 = (int)((unsigned char) string1[0]);
        c2 = (int)((unsigned char) string2[0]);
        
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 += ('a' - 'A');
        
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 += ('a' - 'A');
        
        if ((c1 - c2) != 0)
            return c1 - c2;
        
        string1++;
        string2++;
        count++;
    }
    
    if (count >= max)
        return 0;
    else
        return (string1[0]) ? 1 : ((string2[0]) ? -1 : 0);
}

/*
 * ascii_strcasestr: locale and case independent string search
 */

char *
ascii_strcasestr (char *string, char *search)
{
    int length_search;
    
    length_search = strlen (search);
    
    if (!string || !search || (length_search == 0))
        return NULL;
    
    while (string[0])
    {
        if (ascii_strncasecmp (string, search, length_search) == 0)
            return string;
        
        string++;
    }
    
    return NULL;
}

/*
 * weechat_iconv: convert string to another charset
 */

char *
weechat_iconv (int from_utf8, char *from_code, char *to_code, char *string)
{
    char *outbuf;
    
#ifdef HAVE_ICONV
    iconv_t cd;
    char *inbuf, *ptr_inbuf, *ptr_outbuf, *next_char;
    int done;
    size_t err, inbytesleft, outbytesleft;
    
    if (from_code && from_code[0] && to_code && to_code[0]
        && (ascii_strcasecmp(from_code, to_code) != 0))
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
            outbuf = (char *) malloc (outbytesleft + 2);
            ptr_outbuf = outbuf;
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
                    done = 1;
            }
            ptr_outbuf[0] = '\0';
            free (inbuf);
            iconv_close (cd);
        }
    }
    else
        outbuf = strdup (string);
#else
    /* make C compiler happy */
    (void) from_code;
    (void) to_code;
    outbuf = strdup (string);
#endif /* HAVE_ICONV */
    
    return outbuf;
}

/*
 * weechat_iconv_to_internal: convert user string (input, script, ..) to
 *                            WeeChat internal storage charset
 */

char *
weechat_iconv_to_internal (char *charset, char *string)
{
    char *input, *output;
    
    input = strdup (string);
    
    /* optimize for UTF-8: if charset is NULL => we use term charset =>
       if ths charset is already UTF-8, then no iconv needed */
    if (local_utf8 && (!charset || !charset[0]))
        return input;
    
    if (input)
    {
        if (utf8_is_valid (input, NULL))
            return input;
        
        output = weechat_iconv (0,
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
 * weechat_iconv_from_internal: convert internal string to terminal charset,
 *                              for display
 */

char *
weechat_iconv_from_internal (char *charset, char *string)
{
    char *input, *output;
    
    input = strdup (string);
    
    /* optimize for UTF-8: if charset is NULL => we use term charset =>
       if ths charset is already UTF-8, then no iconv needed */
    if (local_utf8 && (!charset || !charset[0]))
        return input;
    
    if (input)
    {
        utf8_normalize (input, '?');
        output = weechat_iconv (1,
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
 * weechat_iconv_fprintf: encode to terminal charset, then call fprintf on a file
 */

void
weechat_iconv_fprintf (FILE *file, char *data, ...)
{
    va_list argptr;
    static char buf[4096];
    char *buf2;
    
    va_start (argptr, data);
    vsnprintf (buf, sizeof (buf) - 1, data, argptr);
    va_end (argptr);
    
    buf2 = weechat_iconv_from_internal (NULL, buf);
    fprintf (file, "%s", (buf2) ? buf2 : buf);
    if (buf2)
        free (buf2);
}

/*
 * weechat_strreplace: replace a string by new one in a string
 *                     note: returned value has to be free() after use
 */

char *
weechat_strreplace (char *string, char *search, char *replace)
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
 * get_timeval_diff: calculates difference between two times (return in milliseconds)
 */

long
get_timeval_diff (struct timeval *tv1, struct timeval *tv2)
{
    long diff_sec, diff_usec;
    
    diff_sec = tv2->tv_sec - tv1->tv_sec;
    diff_usec = tv2->tv_usec - tv1->tv_usec;
    
    if (diff_usec < 0)
    {
        diff_usec += 1000000;
        diff_sec--;
    }
    return ((diff_usec / 1000) + (diff_sec * 1000));
}

/*
 * explode_string: explode a string according to separators
 */

char **
explode_string (char *string, char *separators, int num_items_max,
                int *num_items)
{
    int i, n_items;
    char **array;
    char *ptr, *ptr1, *ptr2;

    if (num_items != NULL)
        *num_items = 0;
    
    if (!string || !string[0])
        return NULL;
    
    /* calculate number of items */
    ptr = string;
    i = 1;
    while ((ptr = strpbrk (ptr, separators)))
    {
        while (strchr (separators, ptr[0]) != NULL)
            ptr++;
        i++;
    }
    n_items = i;

    if ((num_items_max != 0) && (n_items > num_items_max))
        n_items = num_items_max;
    
    array =
        (char **) malloc ((n_items + 1) * sizeof (char *));
    
    ptr1 = string;
    ptr2 = string;
    
    for (i = 0; i < n_items; i++)
    {
        while (strchr (separators, ptr1[0]) != NULL)
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
                array[i] =
                    (char *) malloc ((ptr2 - ptr1 + 1) * sizeof (char));
                array[i] = strncpy (array[i], ptr1, ptr2 - ptr1);
                array[i][ptr2 - ptr1] = '\0';
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
 * free_exploded_string: free an exploded string
 */

void
free_exploded_string (char **exploded_string)
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
 * split_multi_command: split a list of commands separated by 'sep'
 *                      and ecscaped with '\'
 *                      - empty commands are removed
 *                      - spaces on the left of each commands are stripped
 *                      Result must be freed with free_multi_command
 */

char **
split_multi_command (char *command, char sep)
{
    int nb_substr, arr_idx, str_idx, type;
    char **array;
    char *buffer, *ptr, *p;

    if (command == NULL)
	return NULL;
    
    nb_substr = 1;
    ptr = command;
    while ( (p = strchr(ptr, sep)) != NULL)
    {
	nb_substr++;
	ptr = ++p;
    }

    array = (char **) malloc ((nb_substr + 1) * sizeof(char *));
    if (!array)
	return NULL;
    
    buffer = (char *) malloc ( (strlen(command) + 1) * sizeof (char));
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

    array = (char **) realloc (array, (arr_idx + 1) * sizeof(char *));

    return array;
}

/*
 * free_multi_command : free a list of commands splitted
 *                      with split_multi_command
 */

void
free_multi_command (char **commands)
{
    int i;

    if (commands)
    {
        for (i = 0; commands[i]; i++)
            free (commands[i]);
        free (commands);
    }
}
