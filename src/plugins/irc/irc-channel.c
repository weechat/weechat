/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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


#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-channel.h"
#include "irc-buffer.h"
#include "irc-config.h"
#include "irc-nick.h"
#include "irc-server.h"
#include "irc-input.h"


/*
 * irc_channel_valid: check if a channel pointer exists for a server
 *                    return 1 if channel exists
 *                           0 if channel is not found
 */

int
irc_channel_valid (struct t_irc_server *server, struct t_irc_channel *channel)
{
    struct t_irc_channel *ptr_channel;
    
    if (!server)
        return 0;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel == channel)
            return 1;
    }
    
    /* channel not found */
    return 0;
}

/*
 * irc_channel_new: allocate a new channel for a server and add it to servers
 *                  list
 */

struct t_irc_channel *
irc_channel_new (struct t_irc_server *server, int channel_type,
                 const char *channel_name, int switch_to_channel)
{
    struct t_irc_channel *new_channel;
    struct t_gui_buffer *new_buffer;
    char *buffer_name;
    
    /* alloc memory for new channel */
    if ((new_channel = malloc (sizeof (*new_channel))) == NULL)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot allocate new channel"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return NULL;
    }
    
    /* create buffer for channel (or use existing one) */
    buffer_name = irc_buffer_build_name (server->name, channel_name);
    new_buffer = weechat_buffer_search (IRC_PLUGIN_NAME, buffer_name);
    if (new_buffer)
        weechat_nicklist_remove_all (new_buffer);
    else
    {
        new_buffer = weechat_buffer_new (buffer_name,
                                         &irc_input_data_cb, NULL,
                                         &irc_buffer_close_cb, NULL);
        if (!new_buffer)
        {
            free (new_channel);
            return NULL;
        }
    }
    
    if (channel_type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        weechat_buffer_set (new_buffer, "nick", server->nick);
        if (weechat_config_integer (weechat_config_get ("weechat.look.nicklist")))
            weechat_buffer_set (new_buffer, "nicklist", "1");
        weechat_buffer_set (new_buffer, "nicklist_display_groups", "0");
        weechat_nicklist_add_group (new_buffer, NULL, IRC_NICK_GROUP_OP,
                                    "nicklist_group", 1);
        weechat_nicklist_add_group (new_buffer, NULL, IRC_NICK_GROUP_HALFOP,
                                    "nicklist_group", 1);
        weechat_nicklist_add_group (new_buffer, NULL, IRC_NICK_GROUP_VOICE,
                                    "nicklist_group", 1);
        weechat_nicklist_add_group (new_buffer, NULL, IRC_NICK_GROUP_CHANUSER,
                                    "nicklist_group", 1);
        weechat_nicklist_add_group (new_buffer, NULL, IRC_NICK_GROUP_NORMAL,
                                    "nicklist_group", 1);
    }
    
    /* set highlights settings on channel buffer */
    weechat_buffer_set (new_buffer, "highlight_words", server->nick);
    if (weechat_config_string (irc_config_look_highlight_tags)
        && weechat_config_string (irc_config_look_highlight_tags)[0])
    {
        weechat_buffer_set (new_buffer, "highlight_tags",
                            weechat_config_string (irc_config_look_highlight_tags));
    }
    
    /* initialize new channel */
    new_channel->type = channel_type;
    new_channel->name = strdup (channel_name);
    new_channel->topic = NULL;
    new_channel->modes = NULL;
    new_channel->limit = 0;
    new_channel->key = NULL;
    new_channel->nicks_count = 0;
    new_channel->checking_away = 0;
    new_channel->away_message = NULL;
    new_channel->cycle = 0;
    new_channel->display_creation_date = 0;
    new_channel->nick_completion_reset = 0;
    new_channel->nicks = NULL;
    new_channel->last_nick = NULL;
    new_channel->buffer = new_buffer;
    new_channel->nicks_speaking = NULL;
    new_channel->buffer_as_string = NULL;
    
    /* add new channel to channels list */
    new_channel->prev_channel = server->last_channel;
    new_channel->next_channel = NULL;
    if (server->channels)
        (server->last_channel)->next_channel = new_channel;
    else
        server->channels = new_channel;
    server->last_channel = new_channel;
    
    if (switch_to_channel)
        weechat_buffer_set (new_buffer, "display", "1");
    
    /* all is ok, return address of new channel */
    return new_channel;
}

/*
 * irc_channel_set_topic: set topic for a channel
 */

void
irc_channel_set_topic (struct t_irc_channel *channel, char *topic)
{
    if (channel->topic)
        free (channel->topic);
    
    channel->topic = (topic) ? strdup (topic) : NULL;
    weechat_buffer_set (channel->buffer, "title", channel->topic);
}

/*
 * irc_channel_free: free a channel and remove it from channels list
 */

void
irc_channel_free (struct t_irc_server *server, struct t_irc_channel *channel)
{
    struct t_irc_channel *new_channels;
    
    if (!server || !channel)
        return;
    
    /* remove channel from channels list */
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
        weechat_list_free (channel->nicks_speaking);
    if (channel->buffer_as_string)
        free (channel->buffer_as_string);
    
    free (channel);
    
    server->channels = new_channels;
}

/*
 * irc_channel_free_all: free all allocated channels
 */

void
irc_channel_free_all (struct t_irc_server *server)
{
    /* remove all channels for the server */
    while (server->channels)
        irc_channel_free (server, server->channels);
}

/*
 * irc_channel_search: returns pointer on a channel with name
 */

struct t_irc_channel *
irc_channel_search (struct t_irc_server *server, const char *channel_name)
{
    struct t_irc_channel *ptr_channel;
    
    if (!server || !channel_name)
        return NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (weechat_strcasecmp (ptr_channel->name, channel_name) == 0)
            return ptr_channel;
    }
    return NULL;
}

/*
 * irc_channel_search_any: returns pointer on a channel with name
 */

struct t_irc_channel *
irc_channel_search_any (struct t_irc_server *server, const char *channel_name)
{
    struct t_irc_channel *ptr_channel;
    
    if (!server || !channel_name)
        return NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (weechat_strcasecmp (ptr_channel->name, channel_name) == 0)
            return ptr_channel;
    }
    return NULL;
}

/*
 * irc_channel_search_any_without_buffer: returns pointer on a channel with name
 *                                        looks only for channels without buffer
 */

struct t_irc_channel *
irc_channel_search_any_without_buffer (struct t_irc_server *server,
                                       const char *channel_name)
{
    struct t_irc_channel *ptr_channel;
    
    if (!server || !channel_name)
        return NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (!ptr_channel->buffer
            && (weechat_strcasecmp (ptr_channel->name, channel_name) == 0))
            return ptr_channel;
    }
    return NULL;
}

/*
 * irc_channel_is_channel: returns 1 if string is channel
 */

int
irc_channel_is_channel (const char *string)
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
irc_channel_remove_away (struct t_irc_channel *channel)
{
    struct t_irc_nick *ptr_nick;
    
    if (channel->type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
        {
            irc_nick_set (channel, ptr_nick, 0, IRC_NICK_AWAY);
        }
    }
}

/*
 * irc_channel_check_away: check for away on a channel
 */

void
irc_channel_check_away (struct t_irc_server *server,
                        struct t_irc_channel *channel, int force)
{
    if (channel->type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        if (force
            || (weechat_config_integer (irc_config_network_away_check_max_nicks) == 0)
            || (channel->nicks_count <= weechat_config_integer (irc_config_network_away_check_max_nicks)))
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
irc_channel_set_away (struct t_irc_channel *channel, const char *nick, int is_away)
{
    (void) channel;
    (void) nick;
    (void) is_away;
    
    /*struct t_irc_nick *ptr_nick;
    
    if (channel->type == IRC_CHANNEL_TYPE_CHANNEL)
    {
        ptr_nick = irc_nick_search (channel, nick);
        if (ptr_nick)
            irc_nick_set_away (channel, ptr_nick, is_away);
    }
    */
}

/*
 * irc_channel_add_nick_speaking: add a nick speaking on a channel
 */

void
irc_channel_add_nick_speaking (struct t_irc_channel *channel, const char *nick)
{
    int size, to_remove, i;

    if (!channel->nicks_speaking)
        channel->nicks_speaking = weechat_list_new ();
    
    weechat_list_add (channel->nicks_speaking, nick, WEECHAT_LIST_POS_END);
    
    size = weechat_list_size (channel->nicks_speaking);
    if (size > IRC_CHANNEL_NICKS_SPEAKING_LIMIT)
    {
        to_remove = size - IRC_CHANNEL_NICKS_SPEAKING_LIMIT;
        for (i = 0; i < to_remove; i++)
        {
            weechat_list_remove (channel->nicks_speaking,
                                 weechat_list_get (channel->nicks_speaking, 0));
        }
    }
}

/*
 * irc_channel_add_to_infolist: add a channel in an infolist
 *                              return 1 if ok, 0 if error
 */

int
irc_channel_add_to_infolist (struct t_infolist *infolist,
                             struct t_irc_channel *channel)
{
    struct t_infolist_item *ptr_item;
    
    if (!infolist || !channel)
        return 0;
    
    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!weechat_infolist_new_var_integer (ptr_item, "type", channel->type))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "name", channel->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "topic", channel->topic))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "modes", channel->modes))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "limit", channel->limit))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "key", channel->key))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "nicks_count", channel->nicks_count))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "checking_away", channel->checking_away))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "away_message", channel->away_message))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "cycle", channel->cycle))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "display_creation_date", channel->display_creation_date))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "nick_completion_reset", channel->nick_completion_reset))
        return 0;
    
    return 1;
}

/*
 * irc_channel_print_log: print channel infos in log (usually for crash dump)
 */

void
irc_channel_print_log (struct t_irc_channel *channel)
{
    struct t_weelist_item *ptr_item;
    int i;
    struct t_irc_nick *ptr_nick;
    
    weechat_log_printf ("");
    weechat_log_printf ("  => channel %s (addr:0x%x)]", channel->name, channel);
    weechat_log_printf ("       type . . . . . . . . : %d",     channel->type);
    weechat_log_printf ("       topic. . . . . . . . : '%s'",   channel->topic);
    weechat_log_printf ("       modes. . . . . . . . : '%s'",   channel->modes);
    weechat_log_printf ("       limit. . . . . . . . : %d",     channel->limit);
    weechat_log_printf ("       key. . . . . . . . . : '%s'",   channel->key);
    weechat_log_printf ("       checking_away. . . . : %d",     channel->checking_away);
    weechat_log_printf ("       away_message . . . . : '%s'",   channel->away_message);
    weechat_log_printf ("       cycle. . . . . . . . : %d",     channel->cycle);
    weechat_log_printf ("       display_creation_date: %d",     channel->display_creation_date);
    weechat_log_printf ("       nicks. . . . . . . . : 0x%x",   channel->nicks);
    weechat_log_printf ("       last_nick. . . . . . : 0x%x",   channel->last_nick);
    weechat_log_printf ("       buffer . . . . . . . : 0x%x",   channel->buffer);
    weechat_log_printf ("       nicks_speaking . . . : 0x%x",   channel->nicks_speaking);
    weechat_log_printf ("       prev_channel . . . . : 0x%x",   channel->prev_channel);
    weechat_log_printf ("       next_channel . . . . : 0x%x",   channel->next_channel);
    if (channel->nicks_speaking)
    {
        weechat_log_printf ("");
        i = 0;
        for (ptr_item = weechat_list_get (channel->nicks_speaking, 0);
             ptr_item; ptr_item = weechat_list_next (ptr_item))
        {
            weechat_log_printf ("         nick speaking %d: '%s'",
                                i, weechat_list_string (ptr_item));
            i++;
        }
    }
    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        irc_nick_print_log (ptr_nick);
    }
}
