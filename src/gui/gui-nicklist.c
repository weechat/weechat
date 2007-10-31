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

/* gui-nicklist.c: nicklist functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "../core/weechat.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "gui-nicklist.h"


/*
 * gui_nicklist_compare: compare two nicks
 *                       return: -1 is nick1 < nick2
 *                                0 if nick1 = nick2
 *                               +1 if nick1 > nick2
 */

int
gui_nicklist_compare (struct t_gui_buffer *buffer,
                      struct t_gui_nick *nick1, struct t_gui_nick *nick2)
{
    if (nick1->sort_index > nick2->sort_index)
        return 1;
    
    if (nick1->sort_index < nick2->sort_index)
        return -1;
    
    /* sort index are the same, then use alphabetical sorting */
    if (buffer->nick_case_sensitive)
        return strcmp (nick1->nick, nick2->nick);
    else
        return string_strcasecmp (nick1->nick, nick2->nick);
}

/*
 * gui_nicklist_find_pos: find position for a nick (for sorting nicklist)
 */

struct t_gui_nick *
gui_nicklist_find_pos (struct t_gui_buffer *buffer, struct t_gui_nick *nick)
{
    struct t_gui_nick *ptr_nick;
    
    for (ptr_nick = buffer->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        if (gui_nicklist_compare (buffer, nick, ptr_nick) < 0)
            return ptr_nick;
    }
    return NULL;
}

/*
 * gui_nicklist_insert_sorted: insert nick into sorted list
 */

void
gui_nicklist_insert_sorted (struct t_gui_buffer *buffer, struct t_gui_nick *nick)
{
    struct t_gui_nick *pos_nick;
    
    if (buffer->nicks)
    {
        pos_nick = gui_nicklist_find_pos (buffer, nick);
        
        if (pos_nick)
        {
            /* insert nick into the list (before nick found) */
            nick->prev_nick = pos_nick->prev_nick;
            nick->next_nick = pos_nick;
            if (pos_nick->prev_nick)
                pos_nick->prev_nick->next_nick = nick;
            else
                buffer->nicks = nick;
            pos_nick->prev_nick = nick;
        }
        else
        {
            /* add nick to the end */
            nick->prev_nick = buffer->last_nick;
            nick->next_nick = NULL;
            buffer->last_nick->next_nick = nick;
            buffer->last_nick = nick;
        }
    }
    else
    {
        nick->prev_nick = NULL;
        nick->next_nick = NULL;
        buffer->nicks = nick;
        buffer->last_nick = nick;
    }
}

/*
 * gui_nicklist_resort: resort a nick in nicklist
 */

void
gui_nicklist_resort (struct t_gui_buffer *buffer, struct t_gui_nick *nick)
{
    /* temporarly remove nick from list */
    if (nick == buffer->nicks)
        buffer->nicks = nick->next_nick;
    else
        nick->prev_nick->next_nick = nick->next_nick;
    if (nick->next_nick)
        nick->next_nick->prev_nick = nick->prev_nick;
    if (nick == buffer->last_nick)
        buffer->last_nick = nick->prev_nick;
    
    /* insert again nick into sorted list */
    gui_nicklist_insert_sorted (buffer, nick);
}

/*
 * gui_nicklist_search: search a nick in buffer nicklist
 */

struct t_gui_nick *
gui_nicklist_search (struct t_gui_buffer *buffer, char *nick)
{
    struct t_gui_nick *ptr_nick;

    for (ptr_nick = buffer->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        if ((buffer->nick_case_sensitive
             && (strcmp (ptr_nick->nick, nick) == 0))
            || (!buffer->nick_case_sensitive
                && (string_strcasecmp (ptr_nick->nick, nick) == 0)))
            return ptr_nick;
    }
    
    /* nick not found */
    return NULL;
}

/*
 * gui_nicklist_add: add a nick to nicklist for a buffer
 */

struct t_gui_nick *
gui_nicklist_add (struct t_gui_buffer *buffer, char *nick, int sort_index,
                  int color_nick, char prefix, int color_prefix)
{
    struct t_gui_nick *new_nick;
    
    if (!nick || gui_nicklist_search (buffer, nick))
        return NULL;
    
    new_nick = (struct t_gui_nick *)malloc (sizeof (struct t_gui_nick));
    if (!new_nick)
        return NULL;
    
    new_nick->nick = strdup (nick);
    new_nick->sort_index = sort_index;
    new_nick->color_nick = color_nick;
    new_nick->prefix = prefix;
    new_nick->color_prefix = color_prefix;
    
    gui_nicklist_insert_sorted (buffer, new_nick);
    
    buffer->nicks_count++;
    
    return new_nick;
}

/*
 * gui_nicklist_update: update a nick in nicklist
 */

void
gui_nicklist_update (struct t_gui_buffer *buffer, struct t_gui_nick *nick,
                     char *new_nick, int sort_index,
                     int color_nick, char prefix, int color_prefix)
{
    if (!nick)
        return;
    
    if (new_nick)
    {
        free (nick->nick);
        nick->nick = strdup (new_nick);
    }
    nick->sort_index = sort_index;
    nick->color_nick = color_nick;
    nick->prefix = prefix;
    nick->color_prefix = color_prefix;
    
    gui_nicklist_resort (buffer, nick);
}

/*
 * gui_nicklist_free: remove a nick to nicklist for a buffer
 */

void
gui_nicklist_free (struct t_gui_buffer *buffer, struct t_gui_nick *nick)
{
    if (nick->nick)
        free (nick->nick);
    
    /* remove nick from nicks list */
    if (nick->prev_nick)
        nick->prev_nick->next_nick = nick->next_nick;
    if (nick->next_nick)
        nick->next_nick->prev_nick = nick->prev_nick;
    if (buffer->nicks == nick)
        buffer->nicks = nick->next_nick;
    if (buffer->last_nick == nick)
        buffer->last_nick = nick->prev_nick;
    
    if (buffer->nicks_count > 0)
        buffer->nicks_count--;
}

/*
 * gui_nicklist_free_all: remove all nicks in nicklist
 */

void
gui_nicklist_free_all (struct t_gui_buffer *buffer)
{
    while (buffer->nicks)
    {
        gui_nicklist_free (buffer, buffer->nicks);
    }
}

/*
 * gui_nicklist_remove: remove a nickname to nicklist for a buffer
 *                      return 1 if a nick was removed, 0 otherwise
 */

int
gui_nicklist_remove (struct t_gui_buffer *buffer, char *nick)
{
    struct t_gui_nick *ptr_nick;
    
    ptr_nick = gui_nicklist_search (buffer, nick);
    if (!ptr_nick)
    return 0;
    
    gui_nicklist_free (buffer, ptr_nick);
    return 1;
}

/*
 * gui_nicklist_get_max_length: return longer nickname on a buffer
 */

int
gui_nicklist_get_max_length (struct t_gui_buffer *buffer)
{
    int length, max_length;
    struct t_gui_nick *ptr_nick;
    
    max_length = 0;
    for (ptr_nick = buffer->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        length = utf8_strlen_screen (ptr_nick->nick);
        if (length > max_length)
            max_length = length;
    }
    return max_length;
}
