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

/* irc-channel.c: manages a chat (channel or private chat) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../common/weechat.h"
#include "irc.h"


char *channel_modes = "iklmnst";


/*
 * channel_new: allocate a new channel for a server and add it to the server queue
 */

t_irc_channel *
channel_new (t_irc_server *server, int channel_type, char *channel_name,
             int switch_to_buffer)
{
    t_irc_channel *new_channel;
    
    /* alloc memory for new channel */
    if ((new_channel = (t_irc_channel *) malloc (sizeof (t_irc_channel))) == NULL)
    {
        fprintf (stderr, _("%s cannot allocate new channel"), WEECHAT_ERROR);
        return NULL;
    }

    /* initialize new channel */
    new_channel->type = channel_type;
    new_channel->dcc_chat = NULL;
    new_channel->name = strdup (channel_name);
    new_channel->topic = NULL;
    memset (new_channel->modes, ' ', sizeof (new_channel->modes));
    new_channel->modes[sizeof (new_channel->modes) - 1] = '\0';
    new_channel->limit = 0;
    new_channel->key = NULL;
    new_channel->checking_away = 0;
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

    gui_buffer_new (gui_current_window, server, new_channel, 0, switch_to_buffer);
    
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

    /* close DCC CHAT */
    if ((t_irc_dcc *)(channel->dcc_chat) &&
        (!DCC_ENDED(((t_irc_dcc *)(channel->dcc_chat))->status)))
    {
        dcc_close ((t_irc_dcc *)(channel->dcc_chat), DCC_ABORTED);
        dcc_redraw (1);
    }
    
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

/*
 * channel_remove_away: remove away for all nicks on a channel
 */

void
channel_remove_away (t_irc_channel *channel)
{
    t_irc_nick *ptr_nick;
    
    if (channel->type == CHAT_CHANNEL)
    {
        for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
        {
            ptr_nick->is_away = 0;
        }
        gui_draw_buffer_nick (channel->buffer, 0);
    }
}

/*
 * channel_check_away: check for away on a channel
 */

void
channel_check_away (t_irc_server *server, t_irc_channel *channel)
{
    if (channel->type == CHAT_CHANNEL)
    {
        channel->checking_away++;
        server_sendf (server, "WHO %s\r\n", channel->name);
    }
}

/*
 * channel_set_away: set/unset away status for a channel
 */

void
channel_set_away (t_irc_channel *channel, char *nick, int is_away)
{
    t_irc_nick *ptr_nick;
    
    if (channel->type == CHAT_CHANNEL)
    {
        ptr_nick = nick_search (channel, nick);
        if (ptr_nick)
            nick_set_away (channel, ptr_nick, is_away);
    }
}

/*
 * channel_create_dcc: create DCC CHAT channel
 */

int
channel_create_dcc (t_irc_dcc *ptr_dcc)
{
    t_irc_channel *ptr_channel;
    
    ptr_channel = channel_search (ptr_dcc->server, ptr_dcc->nick);
    if (!ptr_channel)
        ptr_channel = channel_new (ptr_dcc->server, CHAT_PRIVATE,
                                   ptr_dcc->nick, 0);
    if (!ptr_channel)
        return 0;
    
    if (ptr_channel->dcc_chat &&
        (!DCC_ENDED(((t_irc_dcc *)(ptr_channel->dcc_chat))->status)))
        return 0;
    
    ptr_channel->dcc_chat = ptr_dcc;
    ptr_dcc->channel = ptr_channel;
    gui_redraw_buffer (ptr_channel->buffer);
    return 1;
}

/*
 * channel_remove_dcc: remove a DCC CHAT
 */

void
channel_remove_dcc (t_irc_dcc *ptr_dcc)
{
    t_irc_channel *ptr_channel;
    
    for (ptr_channel = ptr_dcc->server->channels; ptr_channel;
        ptr_channel = ptr_channel->next_channel)
    {
        if ((t_irc_dcc *)(ptr_channel->dcc_chat) == ptr_dcc)
        {
            ptr_channel->dcc_chat = NULL;
            gui_redraw_buffer (ptr_channel->buffer);
        }
    }
}
