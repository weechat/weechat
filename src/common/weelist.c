/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* weelist.c: sorted lists management */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "weelist.h"


/*
 * weelist_search: search date in a list
 */

t_weelist *
weelist_search (t_weelist *weelist, char *data)
{
    t_weelist *ptr_weelist;
    
    for (ptr_weelist = weelist; ptr_weelist; ptr_weelist = ptr_weelist->next_weelist)
    {
        if (strcasecmp (data, ptr_weelist->data) == 0)
            return ptr_weelist;
    }
    /* word not found in list */
    return NULL;
}

/*
 * weelist_find_pos: find position for data (keeping list sorted)
 */

t_weelist *
weelist_find_pos (t_weelist *weelist, char *data)
{
    t_weelist *ptr_weelist;
    
    for (ptr_weelist = weelist; ptr_weelist; ptr_weelist = ptr_weelist->next_weelist)
    {
        if (strcasecmp (data, ptr_weelist->data) < 0)
            return ptr_weelist;
    }
    /* position not found, best position is at the end */
    return NULL;
}

/*
 * weelist_insert: insert an element to the list (at good position)
 */

void
weelist_insert (t_weelist **weelist, t_weelist **last_weelist, t_weelist *element)
{
    t_weelist *pos_weelist;
    
    pos_weelist = weelist_find_pos (*weelist, element->data);
    
    if (*weelist)
    {
        if (pos_weelist)
        {
            /* insert data into the list (before position found) */
            element->prev_weelist = pos_weelist->prev_weelist;
            element->next_weelist = pos_weelist;
            if (pos_weelist->prev_weelist)
                pos_weelist->prev_weelist->next_weelist = element;
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

t_weelist *
weelist_add (t_weelist **weelist, t_weelist **last_weelist, char *data)
{
    t_weelist *new_weelist;
    
    if (!data || (!data[0]))
        return NULL;
    
    if ((new_weelist = ((t_weelist *) malloc (sizeof (t_weelist)))))
    {
        new_weelist->data = strdup (data);
        weelist_insert (weelist, last_weelist, new_weelist);
        return new_weelist;
    }
    /* failed to allocate new element */
    return NULL;
}

/*
 * weelist_remove: free an element in a list
 */

void
weelist_remove (t_weelist **weelist, t_weelist **last_weelist, t_weelist *element)
{
    t_weelist *new_weelist;
    
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
