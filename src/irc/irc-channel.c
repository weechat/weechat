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

/* irc-channel.c: manages a chat (channel or private chat) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "../common/weechat.h"
#include "irc.h"


t_irc_channel *current_channel = NULL;


/*
 * channel_new: allocate a new channel for a server and add it to the server queue
 */

t_irc_channel *
channel_new (t_irc_server *server, int channel_type, char *channel_name)
{
    t_irc_channel *new_channel;

    #if DEBUG >= 1
    log_printf ("joining channel %s\n", channel_name);
    #endif
    
    /* alloc memory for new channel */
    if ((new_channel = (t_irc_channel *) malloc (sizeof (t_irc_channel))) == NULL)
    {
        fprintf (stderr, _("%s cannot allocate new channel"), WEECHAT_ERROR);
        return NULL;
    }

    /* initialize new channel */
    new_channel->type = channel_type;
    new_channel->name = strdup (channel_name);
    new_channel->topic = NULL;
    new_channel->nicks = NULL;
    new_channel->last_nick = NULL;

    /* add new channel to queue */
    new_channel->prev_channel = server->last_channel;
    new_channel->next_channel = NULL;
    if (server->channels)
        server->last_channel->next_channel = new_channel;
    else
        server->channels = new_channel;
    server->last_channel = new_channel;

    gui_window_new (server, new_channel);
    
    /* all is ok, return address of new channel */
    return new_channel;
}

/*
 * channel_free: free a channel and remove it from channels queue
 */

void
channel_free (t_irc_server *server, t_irc_channel *channel)
{
    t_irc_channel *new_channels;

    /* remove channel from queue */
    if (server->last_channel == channel)
        server->last_channel = channel->prev_channel;
    if (channel->prev_channel)
    {
        (channel->prev_channel)->next_channel = channel->next_channel;
        new_channels = server->channels;
    }
    else
        new_channels = channel->next_channel;
    
    if (channel->next_channel)
        (channel->next_channel)->prev_channel = channel->prev_channel;

    /* free data */
    if (channel->name)
        free (channel->name);
    if (channel->topic)
        free (channel->topic);
    nick_free_all (channel);
    free (channel);
    server->channels = new_channels;
}

/*
 * channel_free_all: free all allocated channels
 */

void
channel_free_all (t_irc_server *server)
{
    /* remove all channels for the server */
    while (server->channels)
        channel_free (server, server->channels);
}

/*
 * channel_search: returns pointer on a channel with name
 */

t_irc_channel *
channel_search (t_irc_server *server, char *channel_name)
{
    t_irc_channel *ptr_channel;
                    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (strcasecmp (ptr_channel->name, channel_name) == 0)
            return ptr_channel;
    }
    return NULL;
}

/*
 * string_is_channel: returns 1 if string is channel
 */

int
string_is_channel (char *string)
{
    char first_char[2];
    
    first_char[0] = string[0];
    first_char[1] = '\0';
    return (strpbrk (first_char, CHANNEL_PREFIX)) ? 1 : 0;    
}
