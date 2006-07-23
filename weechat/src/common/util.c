/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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
#include <string.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include "weechat.h"
#include "weeconfig.h"


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
weechat_iconv (char *from_code, char *to_code, char *string)
{
    char *outbuf;
    
#ifdef HAVE_ICONV
    iconv_t cd;
    char *inbuf;
    ICONV_CONST char *ptr_inbuf;
    char *ptr_outbuf;
    size_t inbytesleft, outbytesleft;
    
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
            iconv (cd, &ptr_inbuf, &inbytesleft, &ptr_outbuf, &outbytesleft);
            if (inbytesleft != 0)
            {
                free (outbuf);
                outbuf = strdup (string);
            }
            else
                ptr_outbuf[0] = '\0';
            free (inbuf);
            iconv_close (cd);
        }
    }
    else
        outbuf = strdup (string);
#else
    /* make gcc happy */
    (void) from_code;
    (void) to_code;
    outbuf = strdup (string);
#endif /* HAVE_ICONV */
    
    return outbuf;
}

/*
 * weechat_iconv_check: check a charset
 *                      if a charset is NULL, internal charset is used
 */

int
weechat_iconv_check (char *from_code, char *to_code)
{
#ifdef HAVE_ICONV
    iconv_t cd;
    
    if (!from_code || !from_code[0])
        from_code = (cfg_look_charset_internal && cfg_look_charset_internal[0]) ?
            cfg_look_charset_internal : local_charset;

    if (!to_code || !to_code[0])
        to_code = (cfg_look_charset_internal && cfg_look_charset_internal[0]) ?
            cfg_look_charset_internal : local_charset;

    cd = iconv_open (to_code, from_code);
    if (cd == (iconv_t)(-1))
        return 0;
    iconv_close (cd);
    return 1;
#else
    return 1;
#endif
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
