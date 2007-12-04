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

/* wee-list.c: sorted lists management */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "wee-list.h"
#include "wee-log.h"
#include "wee-string.h"


/*
 * weelist_get_size: get list size (number of elements)
 */

int
weelist_get_size (struct t_weelist *weelist)
{
    struct t_weelist *ptr_weelist;
    int count;
    
    count = 0;
    
    for (ptr_weelist = weelist; ptr_weelist;
         ptr_weelist = ptr_weelist->next_weelist)
    {
        count++;
    }
    
    return count;
}

/*
 * weelist_search: search data in a list
 */

struct t_weelist *
weelist_search (struct t_weelist *weelist, char *data)
{
    struct t_weelist *ptr_weelist;
    
    for (ptr_weelist = weelist; ptr_weelist;
         ptr_weelist = ptr_weelist->next_weelist)
    {
        if (string_strcasecmp (data, ptr_weelist->data) == 0)
            return ptr_weelist;
    }
    /* word not found in list */
    return NULL;
}

/*
 * weelist_find_pos: find position for data (keeping list sorted)
 */

struct t_weelist *
weelist_find_pos (struct t_weelist *weelist, char *data)
{
    struct t_weelist *ptr_weelist;
    
    for (ptr_weelist = weelist; ptr_weelist;
         ptr_weelist = ptr_weelist->next_weelist)
    {
        if (string_strcasecmp (data, ptr_weelist->data) < 0)
            return ptr_weelist;
    }
    /* position not found, best position is at the end */
    return NULL;
}

/*
 * weelist_insert: insert an element to the list (at good position)
 */

void
weelist_insert (struct t_weelist **weelist, struct t_weelist **last_weelist,
                struct t_weelist *element, int position)
{
    struct t_weelist *pos_weelist;
    
    if (*weelist)
    {
        /* remove element if already in list */
        pos_weelist = weelist_search (*weelist, element->data);
        if (pos_weelist)
            weelist_remove (weelist, last_weelist, pos_weelist);
    }

    if (*weelist)
    {
        /* search position for new element, according to pos asked */
        pos_weelist = NULL;
        switch (position)
        {
            case WEELIST_POS_SORT:
                pos_weelist = weelist_find_pos (*weelist, element->data);
                break;
            case WEELIST_POS_BEGINNING:
                pos_weelist = *weelist;
                break;
            case WEELIST_POS_END:
                pos_weelist = NULL;
                break;
        }
        
        if (pos_weelist)
        {
            /* insert data into the list (before position found) */
            element->prev_weelist = pos_weelist->prev_weelist;
            element->next_weelist = pos_weelist;
            if (pos_weelist->prev_weelist)
                (pos_weelist->prev_weelist)->next_weelist = element;
            else
                *weelist = element;
            pos_weelist->prev_weelist = element;
        }
        else
        {
            /* add data to the end */
            element->prev_weelist = *last_weelist;
            element->next_weelist = NULL;
            (*last_weelist)->next_weelist = element;
            *last_weelist = element;
        }
    }
    else
    {
        element->prev_weelist = NULL;
        element->next_weelist = NULL;
        *weelist = element;
        *last_weelist = element;
    }
}

/*
 * weelist_add: create new data and add it to list
 */

struct t_weelist *
weelist_add (struct t_weelist **weelist, struct t_weelist **last_weelist,
             char *data, int position)
{
    struct t_weelist *new_weelist;
    
    if (!data || (!data[0]))
        return NULL;
    
    if ((new_weelist = ((struct t_weelist *) malloc (sizeof (struct t_weelist)))))
    {
        new_weelist->data = strdup (data);
        weelist_insert (weelist, last_weelist, new_weelist, position);
        return new_weelist;
    }
    /* failed to allocate new element */
    return NULL;
}

/*
 * weelist_remove: remove an element from a list
 */

void
weelist_remove (struct t_weelist **weelist, struct t_weelist **last_weelist,
                struct t_weelist *element)
{
    struct t_weelist *new_weelist;
    
    if (!element || !(*weelist))
        return;
    
    /* remove element from list */
    if (*last_weelist == element)
        *last_weelist = element->prev_weelist;
    if (element->prev_weelist)
    {
        (element->prev_weelist)->next_weelist = element->next_weelist;
        new_weelist = *weelist;
    }
    else
        new_weelist = element->next_weelist;
    
    if (element->next_weelist)
        (element->next_weelist)->prev_weelist = element->prev_weelist;

    /* free data */
    if (element->data)
        free (element->data);
    free (element);
    *weelist = new_weelist;
}

/*
 * weelist_remove_all: remove all elements from a list
 */

void
weelist_remove_all (struct t_weelist **weelist, struct t_weelist **last_weelist)
{
    while (*weelist)
    {
        weelist_remove (weelist, last_weelist, *weelist);
    }
}

/*
 * weelist_print_log: print weelist in log (usually for crash dump)
 */

void
weelist_print_log (struct t_weelist *weelist, char *name)
{
    struct t_weelist *ptr_weelist;
    
    for (ptr_weelist = weelist; ptr_weelist;
         ptr_weelist = ptr_weelist->next_weelist)
    {
        log_printf ("[%s (addr:0x%X)]\n", name, ptr_weelist);
        log_printf ("  data . . . . . . . . . : '%s'\n", ptr_weelist->data);
        log_printf ("  prev_weelist . . . . . : 0x%X\n", ptr_weelist->prev_weelist);
        log_printf ("  next_weelist . . . . . : 0x%X\n", ptr_weelist->next_weelist);
    }
}
