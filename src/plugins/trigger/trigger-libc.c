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

/* weechat-trigger-libirc.c: Tiny libc */

/*
 * c_is_number: return 1 if string is an number
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <fnmatch.h>
#undef FNM_CASEFOLD
#define FNM_CASEFOLD (1 << 4)

/*
 * c_is_number : is a string a number (an integer)
 *       return : 1 if true else 0
 */

int
c_is_number (char *s)
{
    int i, l, r;
    
    if (!s)
	return 0;
    
    r = 1;
    l = strlen (s);    
    for (i=0; i<l; i++)
    {
	if (!isdigit(s[i]))
	{
	    r = 0;
	    break;
	}
    }
    return r;
}

/*
 * c_to_number : convert a string to a number (an integer)
 *       return the number
 */

int
c_to_number (char *s)
{
    int number = 0;
    
    if (c_is_number (s) == 1)
	number = strtol (s, NULL, 10);
    
    return number;
}

/*
 * c_strndup: strndup function (not existing in FreeBSD and maybe other)
 */

char *
c_strndup (char *string, int length)
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
 * c_ascii_tolower: locale independant string conversion to lower case
 */

void
c_ascii_tolower (char *string)
{
    while (string && string[0])
    {
        if ((string[0] >= 'A') && (string[0] <= 'Z'))
            string[0] += ('a' - 'A');
        string++;
    }
}

/*
 * c_ascii_toupper: locale independant string conversion to upper case
 */

void
c_ascii_toupper (char *string)
{
    while (string && string[0])
    {
        if ((string[0] >= 'a') && (string[0] <= 'z'))
            string[0] -= ('a' - 'A');
        string++;
    }
}

/*
 * weechat_strreplace: replace a string by new one in a string
 *                     note: returned value has to be free() after use
 */

char *
c_weechat_strreplace (char *string, char *search, char *replace)
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
 * explode_string: explode a string according to separators
 */

char **
c_explode_string (char *string, char *separators, int num_items_max,
                int *num_items)
{
    int i, n_items;
    char **array;
    char *ptr, *ptr1, *ptr2;

    if (num_items != NULL)
        *num_items = 0;

    n_items = num_items_max;

    if (!string || !string[0])
        return NULL;

    if (num_items_max == 0)
    {
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
    }

    array = malloc ((num_items_max ? n_items : n_items + 1) *
                         sizeof (array[0]));

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
                array[i] = malloc (ptr2 - ptr1 + 1);
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
    if (num_items_max == 0)
    {
        array[i] = NULL;
        if (num_items != NULL)
            *num_items = i;
    }
    else
    {
        if (num_items != NULL)
            *num_items = num_items_max;
    }

    return array;
}

/*
 * c_free_exploded_string: free an exploded string
 */

void
c_free_exploded_string (char **exploded_string)
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
 * c_split_multi_command: split a list of commands separated by 'sep'
 *                      and ecscaped with '\'
 *                      - empty commands are removed
 *                      - spaces on the left of each commands are stripped
 *                      Result must be freed with free_multi_command
 */

char **
c_split_multi_command (char *command, char sep)
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

    array = malloc ((nb_substr + 1) * sizeof(array[0]));
    if (!array)
	return NULL;
    
    buffer = malloc ((strlen(command) + 1));
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
 * c_free_multi_command : free a list of commands splitted
 *                      with split_multi_command
 */

void
c_free_multi_command (char **commands)
{
    int i;

    if (commands)
    {
        for (i = 0; commands[i]; i++)
            free (commands[i]);
        free (commands);
    }
}

/*
 * c_join_string : join a list of string with 'sep' as glue
 *                 result must be freed with c_free_joined_string
 */

char *
c_join_string(char **list, char *sep)
{
    int i, len;
    char *str;
    
    str = NULL;
    if (list)
    {
	len = 0;
        for (i = 0; list[i]; i++)
	    len += strlen (list[i]);
	
	len += i*strlen (sep) + 1;
	str = malloc (len);
	if (str)
	{
	    for (i = 0; list[i]; i++)
	    {
		if (i == 0)
		    strcpy (str, list[i]);
		else
		{
		    strcat (str, sep);
		    strcat (str, list[i]);
		}
	    }
	}
    }
    
    return str;
}

/*
 * c_free_joined_string : free a string joined with c_join_string
 */

void
c_free_joined_string (char *str)
{
    free (str);
}

/*
 * c_match_string : case sensitive matching
 *        return 1 if it matches else 0
 *
 */
int 
c_match_string (char *string, char *pattern)
{
    if (!string || !pattern)
	return 0;
    return fnmatch (pattern, string, 0) == 0 ? 1 : 0;
}


/*
 * c_imatch_string : case insensitive matching
 *        return 1 if it matches else 0
 *
 */
int 
c_imatch_string (char *string, char *pattern)
{
    if (!string || !pattern)
	return 0;    
    return fnmatch (pattern, string, FNM_CASEFOLD) == 0 ? 1 : 0;
}
