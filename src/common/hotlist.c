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

/* hotlist.c: WeeChat hotlist (buffers with activity) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "weechat.h"
#include "hotlist.h"
#include "../irc/irc.h"
#include "../gui/gui.h"


t_weechat_hotlist *hotlist = NULL;
t_weechat_hotlist *last_hotlist = NULL;
t_gui_buffer *hotlist_initial_buffer = NULL;


/*
 * hotlist_search: find hotlist with buffer pointer
 */

t_weechat_hotlist *
hotlist_search (t_gui_buffer *buffer)
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
hotlist_find_pos (t_weechat_hotlist *new_hotlist)
{
    t_weechat_hotlist *ptr_hotlist;
    
    for (ptr_hotlist = hotlist; ptr_hotlist; ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        if (new_hotlist->priority > ptr_hotlist->priority)
            return ptr_hotlist;
    }
    return NULL;
}

/*
 * hotlist_add: add a buffer to hotlist, with priority
 */

void
hotlist_add (int priority, t_irc_server *server, t_gui_buffer *buffer,
             int allow_current_buffer)
{
    t_weechat_hotlist *new_hotlist, *pos_hotlist;
    
    if (!buffer)
        return;
    
    /* do not highlight current buffer */
    if ((buffer == gui_current_window->buffer)
        && (!allow_current_buffer) && (!gui_buffer_is_scrolled (buffer)))
        return;
    
    if ((pos_hotlist = hotlist_search (buffer)))
    {
        /* return if priority is greater or equal than the one to add */
        if (pos_hotlist->priority >= priority)
            return;
        /* remove buffer if present with lower priority and go on */
        hotlist_free (pos_hotlist);
    }
    
    if ((new_hotlist = (t_weechat_hotlist *) malloc (sizeof (t_weechat_hotlist))) == NULL)
    {
        gui_printf (NULL,
                    _("%s cannot add a buffer to hotlist\n"), WEECHAT_ERROR);
        return;
    }
    
    new_hotlist->priority = priority;
    new_hotlist->server = server;
    new_hotlist->buffer = buffer;
    
    if (hotlist)
    {
        pos_hotlist = hotlist_find_pos (new_hotlist);
        
        if (pos_hotlist)
        {
            /* insert hotlist into the hotlist (before hotlist found) */
            new_hotlist->prev_hotlist = pos_hotlist->prev_hotlist;
            new_hotlist->next_hotlist = pos_hotlist;
            if (pos_hotlist->prev_hotlist)
                pos_hotlist->prev_hotlist->next_hotlist = new_hotlist;
            else
                hotlist = new_hotlist;
            pos_hotlist->prev_hotlist = new_hotlist;
        }
        else
        {
            /* add hotlist to the end */
            new_hotlist->prev_hotlist = last_hotlist;
            new_hotlist->next_hotlist = NULL;
            last_hotlist->next_hotlist = new_hotlist;
            last_hotlist = new_hotlist;
        }
    }
    else
    {
        new_hotlist->prev_hotlist = NULL;
        new_hotlist->next_hotlist = NULL;
        hotlist = new_hotlist;
        last_hotlist = new_hotlist;
    }
}

/*
 * hotlist_free: free a hotlist and remove it from hotlist queue
 */

void
hotlist_free (t_weechat_hotlist *ptr_hotlist)
{
    t_weechat_hotlist *new_hotlist;

    /* remove hotlist from queue */
    if (last_hotlist == ptr_hotlist)
        last_hotlist = ptr_hotlist->prev_hotlist;
    if (ptr_hotlist->prev_hotlist)
    {
        (ptr_hotlist->prev_hotlist)->next_hotlist = ptr_hotlist->next_hotlist;
        new_hotlist = hotlist;
    }
    else
        new_hotlist = ptr_hotlist->next_hotlist;
    
    if (ptr_hotlist->next_hotlist)
        (ptr_hotlist->next_hotlist)->prev_hotlist = ptr_hotlist->prev_hotlist;
    
    free (ptr_hotlist);
    hotlist = new_hotlist;
}

/*
 * hotlist_free_all: free all hotlists
 */

void
hotlist_free_all ()
{
    /* remove all hotlists */
    while (hotlist)
        hotlist_free (hotlist);
}

/*
 * hotlist_remove_buffer: remove a buffer from hotlist
 */

void
hotlist_remove_buffer (t_gui_buffer *buffer)
{
    t_weechat_hotlist *pos_hotlist;

    pos_hotlist = hotlist_search (buffer);
    if (pos_hotlist)
        hotlist_free (pos_hotlist);
}
