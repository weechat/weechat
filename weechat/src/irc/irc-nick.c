/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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

/* irc-nick.c: manages nick list for channels */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../common/weechat.h"
#include "irc.h"


/*
 * nick_find_color: find a color for a nick (less used will be better!)
 */

int
nick_find_color (t_irc_channel *channel)
{
    int i, color_less_used, min_used;
    int count_used[COLOR_WIN_NICK_NUMBER];
    t_irc_nick *ptr_nick;
    
    /* initialize array for counting usage of color */
    for (i = 0; i < COLOR_WIN_NICK_NUMBER; i++)
        count_used[i] = 0;
    
    /* summarize each color usage */
    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
        count_used[ptr_nick->color - COLOR_WIN_NICK_FIRST]++;
    
    /* look for color less used on channel */
    color_less_used = -1;
    min_used = INT_MAX;
    for (i = 0; i < COLOR_WIN_NICK_NUMBER; i++)
    {
        if (count_used[i] < min_used)
        {
            color_less_used = i;
            min_used = count_used[i];
        }
    }
    
    return (color_less_used < 0) ?
        COLOR_WIN_NICK_FIRST : COLOR_WIN_NICK_FIRST + color_less_used;
}

/*
 * nick_compare: compare two nicks
 *               return: -1 is nick1 < nick2
 *                        0 if nick1 = nick2
 *                       +1 if nick1 > nick2
 *               status sort: operator > voice > normal nick
 */

int
nick_compare (t_irc_nick *nick1, t_irc_nick *nick2)
{
    int score1, score2, comp;
    
    score1 = - ( (nick1->is_op * 8) + (nick1->is_halfop * 4) + (nick1->has_voice * 2));
    score2 = - ( (nick2->is_op * 8) + (nick2->is_halfop * 4) + (nick2->has_voice * 2));
    
    comp = strcasecmp(nick1->nick, nick2->nick);
    if (comp > 0)
        score1++;
    else
        if (comp < 0)
            score2++;
    
    /* nick1 > nick2 */
    if (score1 > score2)
        return 1;
    /* nick1 < nick2 */
    if (score1 < score2)
        return -1;
    /* nick1 == nick2 */
    return 0;
}

/*
 * nick_find_pos: find position for a nick (for sorting nick list)
 */

t_irc_nick *
nick_find_pos (t_irc_channel *channel, t_irc_nick *nick)
{
    t_irc_nick *ptr_nick;
    
    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        if (nick_compare (nick, ptr_nick) < 0)
            return ptr_nick;
    }
    return NULL;
}

/*
 * nick_insert_sorted: insert nick into sorted list
 */

void
nick_insert_sorted (t_irc_channel *channel, t_irc_nick *nick)
{
    t_irc_nick *pos_nick;
    
    if (channel->nicks)
    {
        pos_nick = nick_find_pos (channel, nick);
        
        if (pos_nick)
        {
            /* insert nick into the list (before nick found) */
            nick->prev_nick = pos_nick->prev_nick;
            nick->next_nick = pos_nick;
            if (pos_nick->prev_nick)
                pos_nick->prev_nick->next_nick = nick;
            else
                channel->nicks = nick;
            pos_nick->prev_nick = nick;
        }
        else
        {
            /* add nick to the end */
            nick->prev_nick = channel->last_nick;
            nick->next_nick = NULL;
            channel->last_nick->next_nick = nick;
            channel->last_nick = nick;
        }
    }
    else
    {
        nick->prev_nick = NULL;
        nick->next_nick = NULL;
        channel->nicks = nick;
        channel->last_nick = nick;
    }
}

/*
 * nick_new: allocate a new nick for a channel and add it to the nick list
 */

t_irc_nick *
nick_new (t_irc_channel *channel, char *nick_name,
          int is_op, int is_halfop, int has_voice)
{
    t_irc_nick *new_nick;
    
    /* nick already exists on this channel? */
    if ((new_nick = nick_search (channel, nick_name)))
    {
        /* update nick */
        new_nick->is_op = is_op;
        new_nick->is_halfop = is_halfop;
        new_nick->has_voice = has_voice;
        return new_nick;
    }
    
    /* alloc memory for new nick */
    if ((new_nick = (t_irc_nick *) malloc (sizeof (t_irc_nick))) == NULL)
    {
        gui_printf (channel->window,
                    _("%s cannot allocate new nick\n"), WEECHAT_ERROR);
        return NULL;
    }

    /* initialize new nick */
    new_nick->nick = strdup (nick_name);
    new_nick->is_op = is_op;
    new_nick->is_halfop = is_halfop;
    new_nick->has_voice = has_voice;
    if (strcasecmp (new_nick->nick, SERVER(channel->window)->nick) == 0)
        new_nick->color = COLOR_WIN_NICK_SELF;
    else
        new_nick->color = nick_find_color (channel);

    nick_insert_sorted (channel, new_nick);
    
    /* all is ok, return address of new nick */
    return new_nick;
}

/*
 * nick_resort: resort nick in the list
 */

void
nick_resort (t_irc_channel *channel, t_irc_nick *nick)
{
    /* temporarly remove nick from list */
    if (nick == channel->nicks)
        channel->nicks = nick->next_nick;
    else
        nick->prev_nick->next_nick = nick->next_nick;
    if (nick->next_nick)
        nick->next_nick->prev_nick = nick->prev_nick;
    if (nick == channel->last_nick)
        channel->last_nick = nick->prev_nick;
    
    /* insert again nick into sorted list */
    nick_insert_sorted (channel, nick);
}

/*
 * nick_change: change nickname and move it if necessary (list is sorted)
 */

void
nick_change (t_irc_channel *channel, t_irc_nick *nick, char *new_nick)
{
    /* change nickname */
    if (nick->nick)
        free (nick->nick);
    nick->nick = strdup (new_nick);
    
    /* insert again nick into sorted list */
    nick_resort (channel, nick);
}

/*
 * nick_free: free a nick and remove it from nicks queue
 */

void
nick_free (t_irc_channel *channel, t_irc_nick *nick)
{
    t_irc_nick *new_nicks;

    /* remove nick from queue */
    if (channel->last_nick == nick)
        channel->last_nick = nick->prev_nick;
    if (nick->prev_nick)
    {
        (nick->prev_nick)->next_nick = nick->next_nick;
        new_nicks = channel->nicks;
    }
    else
        new_nicks = nick->next_nick;
    
    if (nick->next_nick)
        (nick->next_nick)->prev_nick = nick->prev_nick;

    /* free data */
    if (nick->nick)
        free (nick->nick);
    free (nick);
    channel->nicks = new_nicks;
}

/*
 * nick_free_all: free all allocated nicks for a channel
 */

void
nick_free_all (t_irc_channel *channel)
{
    /* remove all nicks for the channel */
    while (channel->nicks)
        nick_free (channel, channel->nicks);
}

/*
 * nick_search: returns pointer on a nick
 */

t_irc_nick *
nick_search (t_irc_channel *channel, char *nickname)
{
    t_irc_nick *ptr_nick;
                    
    for (ptr_nick = channel->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        if (strcasecmp (ptr_nick->nick, nickname) == 0)
            return ptr_nick;
    }
    return NULL;
}

/*
 * nick_count: returns number of nicks (total, op, halfop, voice) on a channel
 */

void
nick_count (t_irc_channel *channel, int *total, int *count_op,
            int *count_halfop, int *count_voice, int *count_normal)
{
    t_irc_nick *ptr_nick;
    
    (*total) = 0;
    (*count_op) = 0;
    (*count_halfop) = 0;
    (*count_voice) = 0;
    (*count_normal) = 0;
    for (ptr_nick = channel->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        (*total)++;
        if (ptr_nick->is_op)
            (*count_op)++;
        else
        {
            if (ptr_nick->is_halfop)
                (*count_halfop)++;
            else
            {
                if (ptr_nick->has_voice)
                    (*count_voice)++;
                else
                    (*count_normal)++;
            }
        }
    }
}

/*
 * nick_get_max_length: returns longer nickname on a channel
 */

int
nick_get_max_length (t_irc_channel *channel)
{
    int length, max_length;
    t_irc_nick *ptr_nick;
    
    max_length = 0;
    for (ptr_nick = channel->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        length = strlen (ptr_nick->nick);
        if (length > max_length)
            max_length = length;
    }
    return max_length;
}
