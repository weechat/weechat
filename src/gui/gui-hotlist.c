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

/* hotlist.c: WeeChat hotlist (buffers with activity) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "hotlist.h"
#include "log.h"
#include "util.h"
#include "weeconfig.h"
#include "../protocols/irc/irc.h"
#include "../gui/gui.h"


t_weechat_hotlist *weechat_hotlist = NULL;
t_weechat_hotlist *last_weechat_hotlist = NULL;
t_gui_buffer *hotlist_initial_buffer = NULL;


/*
 * hotlist_search: find hotlist with buffer pointer
 */

t_weechat_hotlist *
hotlist_search (t_weechat_hotlist *hotlist, t_gui_buffer *buffer)
{
    t_weechat_hotlist *ptr_hotlist;
    
    for (ptr_hotlist = hotlist; ptr_hotlist; ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        if (ptr_hotlist->buffer == buffer)
            return ptr_hotlist;
    }
    return NULL;
}

/*
 * hotlist_find_pos: find position for a inserting in hotlist (for sorting hotlist)
 */

t_weechat_hotlist *
hotlist_find_pos (t_weechat_hotlist *hotlist, t_weechat_hotlist *new_hotlist)
{
    t_weechat_hotlist *ptr_hotlist;
    
    switch (cfg_look_hotlist_sort)
    {
        case CFG_LOOK_HOTLIST_SORT_GROUP_TIME_ASC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (get_timeval_diff (&(new_hotlist->creation_time),
                                              &(ptr_hotlist->creation_time)) > 0)))
                    return ptr_hotlist;
            }
            break;
        case CFG_LOOK_HOTLIST_SORT_GROUP_TIME_DESC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (get_timeval_diff (&(new_hotlist->creation_time),
                                              &(ptr_hotlist->creation_time)) < 0)))
                    return ptr_hotlist;
            }
            break;
        case CFG_LOOK_HOTLIST_SORT_GROUP_NUMBER_ASC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (new_hotlist->buffer->number < ptr_hotlist->buffer->number)))
                    return ptr_hotlist;
            }
            break;
        case CFG_LOOK_HOTLIST_SORT_GROUP_NUMBER_DESC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if ((new_hotlist->priority > ptr_hotlist->priority)
                    || ((new_hotlist->priority == ptr_hotlist->priority)
                        && (new_hotlist->buffer->number > ptr_hotlist->buffer->number)))
                    return ptr_hotlist;
            }
            break;
        case CFG_LOOK_HOTLIST_SORT_NUMBER_ASC:
            for (ptr_hotlist = hotlist; ptr_hotlist;
                 ptr_hotlist = ptr_hotlist->next_hotlist)
            {
                if (new_hotlist->buffer->number < ptr_hotlist->buffer->number)
                    return ptr_hotlist;
            }
            break;
        case CFG_LOOK_HOTLIST_SORT_NUMBER_DESC:
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
 * hotlist_add_hotlist: add new hotlist in list
 */

void
hotlist_add_hotlist (t_weechat_hotlist **hotlist, t_weechat_hotlist **last_hotlist,
                     t_weechat_hotlist *new_hotlist)
{
    t_weechat_hotlist *pos_hotlist;
    
    if (*hotlist)
    {
        pos_hotlist = hotlist_find_pos (*hotlist, new_hotlist);
        
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
 * hotlist_add: add a buffer to hotlist, with priority
 *              if creation_time is NULL, current time is used
 */

void
hotlist_add (int priority, struct timeval *creation_time,
             t_irc_server *server, t_gui_buffer *buffer,
             int allow_current_buffer)
{
    t_weechat_hotlist *new_hotlist, *ptr_hotlist;
    
    if (!buffer)
        return;
    
    /* do not highlight current buffer */
    if ((buffer == gui_current_window->buffer)
        && (!allow_current_buffer) && (!gui_buffer_is_scrolled (buffer)))
        return;
    
    if ((ptr_hotlist = hotlist_search (weechat_hotlist, buffer)))
    {
        /* return if priority is greater or equal than the one to add */
        if (ptr_hotlist->priority >= priority)
            return;
        /* remove buffer if present with lower priority and go on */
        hotlist_free (&weechat_hotlist, &last_weechat_hotlist, ptr_hotlist);
    }
    
    if ((new_hotlist = (t_weechat_hotlist *) malloc (sizeof (t_weechat_hotlist))) == NULL)
    {
        gui_printf (NULL,
                    _("%s cannot add a buffer to hotlist\n"), WEECHAT_ERROR);
        return;
    }
    
    new_hotlist->priority = priority;
    if (creation_time)
        memcpy (&(new_hotlist->creation_time),
                creation_time, sizeof (creation_time));
    else
        gettimeofday (&(new_hotlist->creation_time), NULL);
    new_hotlist->server = server;
    new_hotlist->buffer = buffer;
    new_hotlist->next_hotlist = NULL;
    new_hotlist->prev_hotlist = NULL;
    
    hotlist_add_hotlist (&weechat_hotlist, &last_weechat_hotlist, new_hotlist);
}

/*
 * hotlist_dup: duplicate hotlist element
 */

t_weechat_hotlist *
hotlist_dup (t_weechat_hotlist *hotlist)
{
    t_weechat_hotlist *new_hotlist;
    
    if ((new_hotlist = (t_weechat_hotlist *) malloc (sizeof (t_weechat_hotlist))))
    {
        new_hotlist->priority = hotlist->priority;
        memcpy (&(new_hotlist->creation_time), &(hotlist->creation_time),
                sizeof (new_hotlist->creation_time));
        new_hotlist->server = hotlist->server;
        new_hotlist->buffer = hotlist->buffer;
        new_hotlist->prev_hotlist = NULL;
        new_hotlist->next_hotlist = NULL;
        return new_hotlist;
    }
    return NULL;
}

/*
 * hotlist_resort: resort hotlist with new sort type
 */

void
hotlist_resort ()
{
    t_weechat_hotlist *new_hotlist, *last_new_hotlist;
    t_weechat_hotlist *ptr_hotlist, *element;
    
    /* copy and resort hotlist in new linked list */
    new_hotlist = NULL;
    last_new_hotlist = NULL;
    for (ptr_hotlist = weechat_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        element = hotlist_dup (ptr_hotlist);
        hotlist_add_hotlist (&new_hotlist, &last_new_hotlist, element);
    }

    hotlist_free_all (&weechat_hotlist, &last_weechat_hotlist);

    weechat_hotlist = new_hotlist;
    last_weechat_hotlist = last_new_hotlist;
}

/*
 * hotlist_free: free a hotlist and remove it from hotlist queue
 */

void
hotlist_free (t_weechat_hotlist **hotlist, t_weechat_hotlist **last_hotlist,
              t_weechat_hotlist *ptr_hotlist)
{
    t_weechat_hotlist *new_hotlist;

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
 * hotlist_free_all: free all hotlists
 */

void
hotlist_free_all (t_weechat_hotlist **hotlist, t_weechat_hotlist **last_hotlist)
{
    /* remove all hotlists */
    while (*hotlist)
        hotlist_free (hotlist, last_hotlist, *hotlist);
}

/*
 * hotlist_remove_buffer: remove a buffer from hotlist
 */

void
hotlist_remove_buffer (t_gui_buffer *buffer)
{
    t_weechat_hotlist *pos_hotlist;

    pos_hotlist = hotlist_search (weechat_hotlist, buffer);
    if (pos_hotlist)
        hotlist_free (&weechat_hotlist, &last_weechat_hotlist, pos_hotlist);
}

/*
 * hotlist_print_log: print hotlist in log (usually for crash dump)
 */

void
hotlist_print_log ()
{
    t_weechat_hotlist *ptr_hotlist;
    
    for (ptr_hotlist = weechat_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        weechat_log_printf ("[hotlist (addr:0x%X)]\n", ptr_hotlist);
        weechat_log_printf ("  priority . . . . . . . : %d\n",   ptr_hotlist->priority);
        weechat_log_printf ("  creation_time. . . . . : tv_sec:%d, tv_usec:%d\n",
                            ptr_hotlist->creation_time.tv_sec,
                            ptr_hotlist->creation_time.tv_usec);
        weechat_log_printf ("  server . . . . . . . . : 0x%X\n", ptr_hotlist->server);
        weechat_log_printf ("  buffer . . . . . . . . : 0x%X\n", ptr_hotlist->buffer);
        weechat_log_printf ("  prev_hotlist . . . . . : 0x%X\n", ptr_hotlist->prev_hotlist);
        weechat_log_printf ("  next_hotlist . . . . . : 0x%X\n", ptr_hotlist->next_hotlist);
    }
}
