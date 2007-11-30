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

/* gui-hotlist.c: hotlist management (buffers with activity) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-log.h"
#include "../core/wee-util.h"
#include "gui-hotlist.h"
#include "gui-buffer.h"
#include "gui-window.h"


struct t_gui_hotlist *gui_hotlist = NULL;
struct t_gui_hotlist *last_gui_hotlist = NULL;
struct t_gui_buffer *gui_hotlist_initial_buffer = NULL;

int gui_add_hotlist = 1;                    /* 0 is for temporarly disable  */
                                            /* hotlist add for all buffers  */


/*
 * gui_hotlist_search: find hotlist with buffer pointer
 */

struct t_gui_hotlist *
gui_hotlist_search (struct t_gui_hotlist *hotlist, struct t_gui_buffer *buffer)
{
    struct t_gui_hotlist *ptr_hotlist;
    
    for (ptr_hotlist = hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        if (ptr_hotlist->buffer == buffer)
            return ptr_hotlist;
    }
    return NULL;
}

/*
 * gui_hotlist_find_pos: find position for a inserting in hotlist
 *                       (for sorting hotlist)
 */

struct t_gui_hotlist *
gui_hotlist_find_pos (struct t_gui_hotlist *hotlist, struct t_gui_hotlist *new_hotlist)
{
    struct t_gui_hotlist *ptr_hotlist;
    
    switch (CONFIG_INTEGER(config_look_hotlist_sort))
    {
        case CONFIG_LOOK_HOTLIST_SORT_GROUP_TIME_ASC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (util_get_timeval_diff (&(new_hotlist->creation_time),
                                                   &(ptr_hotlist->creation_time)) > 0)))
                    return ptr_hotlist;
            }
            break;
        case CONFIG_LOOK_HOTLIST_SORT_GROUP_TIME_DESC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (util_get_timeval_diff (&(new_hotlist->creation_time),
                                                   &(ptr_hotlist->creation_time)) < 0)))
                    return ptr_hotlist;
            }
            break;
        case CONFIG_LOOK_HOTLIST_SORT_GROUP_NUMBER_ASC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (new_hotlist->buffer->number < ptr_hotlist->buffer->number)))
                    return ptr_hotlist;
            }
            break;
        case CONFIG_LOOK_HOTLIST_SORT_GROUP_NUMBER_DESC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (new_hotlist->buffer->number > ptr_hotlist->buffer->number)))
                    return ptr_hotlist;
            }
            break;
        case CONFIG_LOOK_HOTLIST_SORT_NUMBER_ASC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if (new_hotlist->buffer->number < ptr_hotlist->buffer->number)
                    return ptr_hotlist;
            }
            break;
        case CONFIG_LOOK_HOTLIST_SORT_NUMBER_DESC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if (new_hotlist->buffer->number > ptr_hotlist->buffer->number)
                    return ptr_hotlist;
            }
            break;
    }
    return NULL;
}

/*
 * gui_hotlist_add_hotlist: add new hotlist in list
 */

void
gui_hotlist_add_hotlist (struct t_gui_hotlist **hotlist,
                         struct t_gui_hotlist **last_hotlist,
                         struct t_gui_hotlist *new_hotlist)
{
    struct t_gui_hotlist *pos_hotlist;
    
    if (*hotlist)
    {
        pos_hotlist = gui_hotlist_find_pos (*hotlist, new_hotlist);
        
        if (pos_hotlist)
        {
            /* insert hotlist into the hotlist (before hotlist found) */
            new_hotlist->prev_hotlist = pos_hotlist->prev_hotlist;
            new_hotlist->next_hotlist = pos_hotlist;
            if (pos_hotlist->prev_hotlist)
                pos_hotlist->prev_hotlist->next_hotlist = new_hotlist;
            else
                *hotlist = new_hotlist;
            pos_hotlist->prev_hotlist = new_hotlist;
        }
        else
        {
            /* add hotlist to the end */
            new_hotlist->prev_hotlist = *last_hotlist;
            new_hotlist->next_hotlist = NULL;
            (*last_hotlist)->next_hotlist = new_hotlist;
            *last_hotlist = new_hotlist;
        }
    }
    else
    {
        new_hotlist->prev_hotlist = NULL;
        new_hotlist->next_hotlist = NULL;
        *hotlist = new_hotlist;
        *last_hotlist = new_hotlist;
    }
}

/*
 * gui_hotlist_add: add a buffer to hotlist, with priority
 *                  if creation_time is NULL, current time is used
 */

void
gui_hotlist_add (struct t_gui_buffer *buffer, int priority,
                 struct timeval *creation_time, int allow_current_buffer)
{
    struct t_gui_hotlist *new_hotlist, *ptr_hotlist;
    
    if (!buffer)
        return;
    
    /* do not highlight current buffer */
    if ((buffer == gui_current_window->buffer)
        && (!allow_current_buffer) && (!gui_buffer_is_scrolled (buffer)))
        return;
    
    if ((ptr_hotlist = gui_hotlist_search (gui_hotlist, buffer)))
    {
        /* return if priority is greater or equal than the one to add */
        if (ptr_hotlist->priority >= priority)
            return;
        /* remove buffer if present with lower priority and go on */
        gui_hotlist_free (&gui_hotlist, &last_gui_hotlist, ptr_hotlist);
    }
    
    if ((new_hotlist = (struct t_gui_hotlist *) malloc (sizeof (struct t_gui_hotlist))) == NULL)
    {
        log_printf (NULL,
                    _("Error: not enough memory to add a buffer to "
                      "hotlist\n"));
        return;
    }
    
    new_hotlist->priority = priority;
    if (creation_time)
        memcpy (&(new_hotlist->creation_time),
                creation_time, sizeof (creation_time));
    else
        gettimeofday (&(new_hotlist->creation_time), NULL);
    new_hotlist->buffer = buffer;
    new_hotlist->next_hotlist = NULL;
    new_hotlist->prev_hotlist = NULL;
    
    gui_hotlist_add_hotlist (&gui_hotlist, &last_gui_hotlist, new_hotlist);
}

/*
 * gui_hotlist_dup: duplicate hotlist element
 */

struct t_gui_hotlist *
gui_hotlist_dup (struct t_gui_hotlist *hotlist)
{
    struct t_gui_hotlist *new_hotlist;
    
    if ((new_hotlist = (struct t_gui_hotlist *) malloc (sizeof (struct t_gui_hotlist))))
    {
        new_hotlist->priority = hotlist->priority;
        memcpy (&(new_hotlist->creation_time), &(hotlist->creation_time),
                sizeof (new_hotlist->creation_time));
        new_hotlist->buffer = hotlist->buffer;
        new_hotlist->prev_hotlist = NULL;
        new_hotlist->next_hotlist = NULL;
        return new_hotlist;
    }
    return NULL;
}

/*
 * gui_hotlist_resort: resort hotlist with new sort type
 */

void
gui_hotlist_resort ()
{
    struct t_gui_hotlist *new_hotlist, *last_new_hotlist;
    struct t_gui_hotlist *ptr_hotlist, *element;
    
    /* copy and resort hotlist in new linked list */
    new_hotlist = NULL;
    last_new_hotlist = NULL;
    for (ptr_hotlist = gui_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        element = gui_hotlist_dup (ptr_hotlist);
        gui_hotlist_add_hotlist (&new_hotlist, &last_new_hotlist, element);
    }

    gui_hotlist_free_all (&gui_hotlist, &last_gui_hotlist);

    gui_hotlist = new_hotlist;
    last_gui_hotlist = last_new_hotlist;
}

/*
 * gui_hotlist_free: free a hotlist and remove it from hotlist queue
 */

void
gui_hotlist_free (struct t_gui_hotlist **hotlist,
                  struct t_gui_hotlist **last_hotlist,
                  struct t_gui_hotlist *ptr_hotlist)
{
    struct t_gui_hotlist *new_hotlist;

    /* remove hotlist from queue */
    if (*last_hotlist == ptr_hotlist)
        *last_hotlist = ptr_hotlist->prev_hotlist;
    if (ptr_hotlist->prev_hotlist)
    {
        (ptr_hotlist->prev_hotlist)->next_hotlist = ptr_hotlist->next_hotlist;
        new_hotlist = *hotlist;
    }
    else
        new_hotlist = ptr_hotlist->next_hotlist;
    
    if (ptr_hotlist->next_hotlist)
        (ptr_hotlist->next_hotlist)->prev_hotlist = ptr_hotlist->prev_hotlist;
    
    free (ptr_hotlist);
    *hotlist = new_hotlist;
}

/*
 * gui_hotlist_free_all: free all hotlists
 */

void
gui_hotlist_free_all (struct t_gui_hotlist **hotlist,
                      struct t_gui_hotlist **last_hotlist)
{
    /* remove all hotlists */
    while (*hotlist)
    {
        gui_hotlist_free (hotlist, last_hotlist, *hotlist);
    }
}

/*
 * gui_hotlist_remove_buffer: remove a buffer from hotlist
 */

void
gui_hotlist_remove_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_hotlist *pos_hotlist;

    pos_hotlist = gui_hotlist_search (gui_hotlist, buffer);
    if (pos_hotlist)
        gui_hotlist_free (&gui_hotlist, &last_gui_hotlist, pos_hotlist);
}

/*
 * gui_hotlist_print_log: print hotlist in log (usually for crash dump)
 */

void
gui_hotlist_print_log ()
{
    struct t_gui_hotlist *ptr_hotlist;
    
    for (ptr_hotlist = gui_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        log_printf ("[hotlist (addr:0x%X)]\n", ptr_hotlist);
        log_printf ("  priority . . . . . . . : %d\n",   ptr_hotlist->priority);
        log_printf ("  creation_time. . . . . : tv_sec:%d, tv_usec:%d\n",
                    ptr_hotlist->creation_time.tv_sec,
                    ptr_hotlist->creation_time.tv_usec);
        log_printf ("  buffer . . . . . . . . : 0x%X\n", ptr_hotlist->buffer);
        log_printf ("  prev_hotlist . . . . . : 0x%X\n", ptr_hotlist->prev_hotlist);
        log_printf ("  next_hotlist . . . . . : 0x%X\n", ptr_hotlist->next_hotlist);
    }
}
