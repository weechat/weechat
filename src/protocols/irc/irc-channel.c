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

/* irc-channel.c: manages a chat (channel or private chat) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../../common/weechat.h"
#include "irc.h"
#include "../../common/log.h"
#include "../../common/utf8.h"
#include "../../common/util.h"
#include "../../common/weeconfig.h"
#include "../../gui/gui.h"


/*
 * irc_channel_new: allocate a new channel for a server and add it to the
 *                  server queue
 */

t_irc_channel *
irc_channel_new (t_irc_server *server, int channel_type, char *channel_name)
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
    new_channel->modes = NULL;
    new_channel->limit = 0;
    new_channel->key = NULL;
    new_channel->nicks_count = 0;
    new_channel->checking_away = 0;
    new_channel->away_message = NULL;
    new_channel->cycle = 0;
    new_channel->close = 0;
    new_channel->display_creation_date = 0;
    new_channel->nick_completion_reset = 0;
    new_channel->nicks = NULL;
    new_channel->last_nick = NULL;
    new_channel->buffer = NULL;
    new_channel->nicks_speaking = NULL;
    new_channel->last_nick_speaking = NULL;
    
    /* add new channel to queue */
    new_channel->prev_channel = server->last_channel;
    new_channel->next_channel = NULL;
    if (server->channels)
        server->last_channel->next_channel = new_channel;
    else
        server->channels = new_channel;
    server->last_channel = new_channel;
    
    /* all is ok, return address of new channel */
    return new_channel;
}

/*
 * irc_channel_free: free a channel and remove it from channels queue
 */

void
irc_channel_free (t_irc_server *server, t_irc_channel *channel)
{
    t_irc_channel *new_channels;
    
    if (!server || !channel)
        return;
    
    /* close DCC CHAT */
    if (channel->dcc_chat)
    {
        ((t_irc_dcc *)(channel->dcc_chat))->channel = NULL;
        if (!IRC_DCC_ENDED(((t_irc_dcc *)(channel->dcc_chat))->status))
        {
            irc_dcc_close ((t_irc_dcc *)(channel->dcc_chat), IRC_DCC_ABORTED);
            irc_dcc_redraw (1);
        }
    }
    
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
    if (channel->modes)
        free (channel->modes);
    if (channel->key)
        free (channel->key);
    irc_nick_free_all (channel);
    if (channel->away_message)
        free (channel->away_message);
    if (channel->nicks_speaking)
        weelist_remove_all (&(channel->nicks_speaking),
                            &(channel->last_nick_speaking));
    free (channel);
    server->channels = new_channels;
}

/*
 * irc_channel_free_all: free all allocated channels
 */

void
irc_channel_free_all (t_irc_server *server)
{
    /* remove all channels for the server */
    while (server->channels)
        irc_channel_free (server, server->channels);
}

/*
 * irc_channel_search: returns pointer on a channel with name
 *                     WARNING: DCC chat channels are not returned by this function
 */

t_irc_channel *
irc_channel_search (t_irc_server *server, char *channel_name)
{
    t_irc_channel *ptr_channel;
    
    if (!server || !channel_name)
        return NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type != IRC_CHANNEL_TYPE_DCC_CHAT)
            && (ascii_strcasecmp (ptr_channel->name, channel_name) == 0))
            return ptr_channel;
    }
    return NULL;
}

/*
 * irc_channel_search_any: returns pointer on a channel with name
 */

t_irc_channel *
irc_channel_search_any (t_irc_server *server, char *channel_name)
{
    t_irc_channel *ptr_channel;
    
    if (!server || !channel_name)
        return NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ascii_strcasecmp (ptr_channel->name, channel_name) == 0)
            return ptr_channel;
    }
    return NULL;
}

/*
 * irc_channel_search_any_without_buffer: returns pointer on a channel with name
 *                                        looks only for channels without buffer
 */

t_irc_channel *
irc_channel_search_any_without_buffer (t_irc_server *server, char *channel_name)
{
    t_irc_channel *ptr_channel;
    
    if (!server || !channel_name)
        return NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (!ptr_channel->buffer
            && (ascii_strcasecmp (ptr_channel->name, channel_name) == 0))
            return ptr_channel;
    }
    return NULL;
}

/*
 * irc_channel_search_dcc: returns pointer on a DCC chat channel with name
 */

t_irc_channel *
irc_channel_search_dcc (t_irc_server *server, char *channel_name)
{
    t_irc_channel *ptr_channel;
    
    if (!server || !channel_name)
        return NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_DCC_CHAT)
            && (ascii_strcasecmp (ptr_channel->name, channel_name) == 0))
            return ptr_channel;
    }
    return NULL;
}

/*
 * irc_channel_is_channel: returns 1 if string is channel
 */

int
irc_channel_is_channel (char *string)
{
    char first_char[2];
    
    if (!string)
        return 0;
    
    first_char[0] = string[0];
    first_char[1] = '\0';
    return (strpbrk (first_char, IRC_CHANNEL_PREFIX)) ? 1 : 0;    
}

/*
 * irc_channel_remove_away: remove away for all nicks on a channel
 */

void
irc_channel_remove_away (t_irc_channel *channel)
{
    t_irc_nick *ptr_nick;
    
    if (channel->type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
        {
            IRC_NICK_SET_FLAG(ptr_nick, 0, IRC_NICK_AWAY);
        }
        gui_nicklist_draw (channel->buffer, 0, 0);
    }
}

/*
 * irc_channel_check_away: check for away on a channel
 */

void
irc_channel_check_away (t_irc_server *server, t_irc_channel *channel, int force)
{
    if (channel->type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        if (force || (cfg_irc_away_check_max_nicks == 0) ||
            (channel->nicks_count <= cfg_irc_away_check_max_nicks))
        {
            channel->checking_away++;
            irc_server_sendf (server, "WHO %s", channel->name);
        }
        else
            irc_channel_remove_away (channel);
    }
}

/*
 * irc_channel_set_away: set/unset away status for a channel
 */

void
irc_channel_set_away (t_irc_channel *channel, char *nick, int is_away)
{
    t_irc_nick *ptr_nick;
    
    if (channel->type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        ptr_nick = irc_nick_search (channel, nick);
        if (ptr_nick)
            irc_nick_set_away (channel, ptr_nick, is_away);
    }
}

/*
 * irc_channel_create_dcc: create DCC CHAT channel
 */

int
irc_channel_create_dcc (t_irc_dcc *ptr_dcc)
{
    t_irc_channel *ptr_channel;
    
    ptr_channel = irc_channel_search_dcc (ptr_dcc->server, ptr_dcc->nick);
    if (!ptr_channel)
    {
        ptr_channel = irc_channel_new (ptr_dcc->server,
                                       IRC_CHANNEL_TYPE_DCC_CHAT,
                                       ptr_dcc->nick);
        if (!ptr_channel)
            return 0;
        gui_buffer_new (gui_current_window, ptr_dcc->server, ptr_channel,
                        GUI_BUFFER_TYPE_STANDARD, 0);
    }
    
    if (ptr_channel->dcc_chat &&
        (!IRC_DCC_ENDED(((t_irc_dcc *)(ptr_channel->dcc_chat))->status)))
        return 0;
    
    ptr_channel->dcc_chat = ptr_dcc;
    ptr_dcc->channel = ptr_channel;
    gui_window_redraw_buffer (ptr_channel->buffer);
    return 1;
}

/*
 * irc_channel_get_notify_level: get channel notify level
 */

int
irc_channel_get_notify_level (t_irc_server *server, t_irc_channel *channel)
{
    char *name, *pos, *pos2;
    int server_default_notify, notify;
    
    if ((!server) || (!channel))
        return GUI_NOTIFY_LEVEL_DEFAULT;
    
    if ((!server->notify_levels) || (!server->notify_levels[0]))
        return GUI_NOTIFY_LEVEL_DEFAULT;
    
    server_default_notify = irc_server_get_default_notify_level (server);
    if ((channel->type != IRC_CHANNEL_TYPE_CHANNEL)
        && (server_default_notify == 1))
        server_default_notify = 2;
    
    name = (char *) malloc (strlen (channel->name) + 2);
    strcpy (name, channel->name);
    strcat (name, ":");
    pos = strstr (server->notify_levels, name);
    free (name);
    if (!pos)
        return server_default_notify;
    
    pos2 = pos + strlen (channel->name);
    if (pos2[0] != ':')
        return server_default_notify;
    pos2++;
    if (!pos2[0])
        return server_default_notify;
    
    notify = (int)(pos2[0] - '0');
    if ((notify >= GUI_NOTIFY_LEVEL_MIN) && (notify <= GUI_NOTIFY_LEVEL_MAX))
        return notify;

    return server_default_notify;
}

/*
 * irc_channel_set_notify_level: set channel notify level
 */

void
irc_channel_set_notify_level (t_irc_server *server, t_irc_channel *channel,
                              int notify)
{
    char level_string[2];
    
    if ((!server) || (!channel))
        return;
    
    level_string[0] = notify + '0';
    level_string[1] = '\0';
    config_option_list_set (&(server->notify_levels), channel->name, level_string);
}

/*
 * irc_channel_add_nick_speaking: add a nick speaking on a channel
 */

void
irc_channel_add_nick_speaking (t_irc_channel *channel, char *nick)
{
    int size, to_remove, i;
    
    weelist_add (&(channel->nicks_speaking), &(channel->last_nick_speaking),
                 nick, WEELIST_POS_END);
    
    size = weelist_get_size (channel->nicks_speaking);
    if (size > IRC_CHANNEL_NICKS_SPEAKING_LIMIT)
    {
        to_remove = size - IRC_CHANNEL_NICKS_SPEAKING_LIMIT;
        for (i = 0; i < to_remove; i++)
        {
            weelist_remove (&(channel->nicks_speaking),
                            &(channel->last_nick_speaking),
                            channel->nicks_speaking);
        }
    }
}

/*
 * irc_channel_print_log: print channel infos in log (usually for crash dump)
 */

void
irc_channel_print_log (t_irc_channel *channel)
{
    weechat_log_printf ("=> channel %s (addr:0x%X)]\n", channel->name, channel);
    weechat_log_printf ("     type . . . . . . . . : %d\n",     channel->type);
    weechat_log_printf ("     dcc_chat . . . . . . : 0x%X\n",   channel->dcc_chat);
    weechat_log_printf ("     topic. . . . . . . . : '%s'\n",   channel->topic);
    weechat_log_printf ("     modes. . . . . . . . : '%s'\n",   channel->modes);
    weechat_log_printf ("     limit. . . . . . . . : %d\n",     channel->limit);
    weechat_log_printf ("     key. . . . . . . . . : '%s'\n",   channel->key);
    weechat_log_printf ("     checking_away. . . . : %d\n",     channel->checking_away);
    weechat_log_printf ("     away_message . . . . : '%s'\n",   channel->away_message);
    weechat_log_printf ("     cycle. . . . . . . . : %d\n",     channel->cycle);
    weechat_log_printf ("     close. . . . . . . . : %d\n",     channel->close);
    weechat_log_printf ("     display_creation_date: %d\n",     channel->close);
    weechat_log_printf ("     nicks. . . . . . . . : 0x%X\n",   channel->nicks);
    weechat_log_printf ("     last_nick. . . . . . : 0x%X\n",   channel->last_nick);
    weechat_log_printf ("     buffer . . . . . . . : 0x%X\n",   channel->buffer);
    weechat_log_printf ("     nicks_speaking . . . : 0x%X\n",   channel->nicks_speaking);
    weechat_log_printf ("     last_nick_speaking . : 0x%X\n",   channel->last_nick_speaking);
    weechat_log_printf ("     prev_channel . . . . : 0x%X\n",   channel->prev_channel);
    weechat_log_printf ("     next_channel . . . . : 0x%X\n",   channel->next_channel);
    if (channel->nicks_speaking)
    {
        weechat_log_printf ("\n");
        weelist_print_log (channel->nicks_speaking,
                           "channel nick speaking element");
    }
}
