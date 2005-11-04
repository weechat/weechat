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

/* irc-nick.c: manages nick list for channels */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../common/weechat.h"
#include "irc.h"
#include "../common/weeconfig.h"


/*
 * nick_find_color: find a color for a nick (according to nick letters)
 */

int
nick_find_color (t_irc_nick *nick)
{
    int i, color;
    
    color = 0;
    for (i = strlen (nick->nick) - 1; i >= 0; i--)
    {
        color += (int)(nick->nick[i]);
    }
    color = (color % cfg_look_color_nicks_number);
    
    return COLOR_WIN_NICK_1 + color;
}

/*
 * nick_score_for_sort: return score for sorting nick, according to privileges
 */

int
nick_score_for_sort (t_irc_nick *nick)
{
    if (nick->is_chanowner)
        return -32;
    if (nick->is_chanadmin)
        return -16;
    if (nick->is_op)
        return -8;
    if (nick->is_halfop)
        return -4;
    if (nick->has_voice)
        return -2;
    return 0;
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
    
    score1 = nick_score_for_sort (nick1);
    score2 = nick_score_for_sort (nick2);
    
    comp = ascii_strcasecmp (nick1->nick, nick2->nick);
    if (comp > 0)
        score1++;
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
          int is_chanowner, int is_chanadmin, int is_op, int is_halfop,
          int has_voice)
{
    t_irc_nick *new_nick;
    
    /* nick already exists on this channel? */
    if ((new_nick = nick_search (channel, nick_name)))
    {
        /* update nick */
        new_nick->is_chanowner = is_chanowner;
        new_nick->is_chanadmin = is_chanadmin;
        new_nick->is_op = is_op;
        new_nick->is_halfop = is_halfop;
        new_nick->has_voice = has_voice;
        return new_nick;
    }
    
    /* alloc memory for new nick */
    if ((new_nick = (t_irc_nick *) malloc (sizeof (t_irc_nick))) == NULL)
    {
        gui_printf (channel->buffer,
                    _("%s cannot allocate new nick\n"), WEECHAT_ERROR);
        return NULL;
    }

    /* initialize new nick */
    new_nick->nick = strdup (nick_name);
    new_nick->is_chanowner = is_chanowner;
    new_nick->is_chanadmin = is_chanadmin;
    new_nick->is_op = is_op;
    new_nick->is_halfop = is_halfop;
    new_nick->has_voice = has_voice;
    new_nick->is_away = 0;
    if (ascii_strcasecmp (new_nick->nick, SERVER(channel->buffer)->nick) == 0)
        new_nick->color = COLOR_WIN_NICK_SELF;
    else
        new_nick->color = nick_find_color (new_nick);

    nick_insert_sorted (channel, new_nick);
    
    channel->nicks_count++;
    
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
    int nick_is_me;
    
    nick_is_me = (strcmp (nick->nick, SERVER(channel->buffer)->nick) == 0) ? 1 : 0;
    
    /* change nickname */
    if (nick->nick)
        free (nick->nick);
    nick->nick = strdup (new_nick);
    if (nick_is_me)
        nick->color = COLOR_WIN_NICK_SELF;
    else
        nick->color = nick_find_color (nick);
    
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

    channel->nicks_count--;
    
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
    
    /* sould be zero, but prevent any bug :D */
    channel->nicks_count = 0;
}

/*
 * nick_search: returns pointer on a nick
 */

t_irc_nick *
nick_search (t_irc_channel *channel, char *nickname)
{
    t_irc_nick *ptr_nick;
    
    if (!nickname)
        return NULL;
    
    for (ptr_nick = channel->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        if (ascii_strcasecmp (ptr_nick->nick, nickname) == 0)
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
        if ((ptr_nick->is_chanowner) || (ptr_nick->is_chanadmin) || (ptr_nick->is_op))
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
    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        length = strlen (ptr_nick->nick);
        if (length > max_length)
            max_length = length;
    }
    return max_length;
}

/*
 * nick_set_away: set/unset away status for a channel
 */

void
nick_set_away (t_irc_channel *channel, t_irc_nick *nick, int is_away)
{
    if (nick->is_away != is_away)
    {
        nick->is_away = is_away;
        gui_draw_buffer_nick (channel->buffer, 0);
    }
}

/*
 * nick_print_log: print nick infos in log (usually for crash dump)
 */

void
nick_print_log (t_irc_nick *nick)
{
    wee_log_printf ("=> nick %s (addr:0x%X)]\n",    nick->nick, nick);
    wee_log_printf ("     is_chanowner . : %d\n",   nick->is_chanowner);
    wee_log_printf ("     is_chanadmin . : %d\n",   nick->is_chanadmin);
    wee_log_printf ("     is_op. . . . . : %d\n",   nick->is_op);
    wee_log_printf ("     is_halfop. . . : %d\n",   nick->is_halfop);
    wee_log_printf ("     has_voice. . . : %d\n",   nick->has_voice);
    wee_log_printf ("     is_away. . . . : %d\n",   nick->is_away);
    wee_log_printf ("     color. . . . . : %d\n",   nick->color);
    wee_log_printf ("     prev_nick. . . : 0x%X\n", nick->prev_nick);
    wee_log_printf ("     next_nick. . . : 0x%X\n", nick->next_nick);
}
