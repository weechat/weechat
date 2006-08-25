/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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

/* irc-channel.c: manages a chat (channel or private chat) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../common/weechat.h"
#include "irc.h"
#include "../common/log.h"
#include "../common/utf8.h"
#include "../common/util.h"
#include "../common/weeconfig.h"
#include "../gui/gui.h"


char *channel_modes = "iklmnstp";


/*
 * channel_new: allocate a new channel for a server and add it to the server queue
 */

t_irc_channel *
channel_new (t_irc_server *server, int channel_type, char *channel_name)
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
    
    if (!server || !channel)
        return;
    
    /* close DCC CHAT */
    if (channel->dcc_chat)
    {
        ((t_irc_dcc *)(channel->dcc_chat))->channel = NULL;
        if (!DCC_ENDED(((t_irc_dcc *)(channel->dcc_chat))->status))
        {
            dcc_close ((t_irc_dcc *)(channel->dcc_chat), DCC_ABORTED);
            dcc_redraw (1);
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
    nick_free_all (channel);
    if (channel->away_message)
        free (channel->away_message);
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
 *                 WARNING: DCC chat channels are not returned by this function
 */

t_irc_channel *
channel_search (t_irc_server *server, char *channel_name)
{
    t_irc_channel *ptr_channel;
    
    if (!server || !channel_name)
        return NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type != CHANNEL_TYPE_DCC_CHAT)
            && (ascii_strcasecmp (ptr_channel->name, channel_name) == 0))
            return ptr_channel;
    }
    return NULL;
}

/*
 * channel_search_any: returns pointer on a channel with name
 */

t_irc_channel *
channel_search_any (t_irc_server *server, char *channel_name)
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
 * channel_search_any_without_buffer: returns pointer on a channel with name
 *                                    looks only for channels without buffer
 */

t_irc_channel *
channel_search_any_without_buffer (t_irc_server *server, char *channel_name)
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
 * channel_search_dcc: returns pointer on a DCC chat channel with name
 */

t_irc_channel *
channel_search_dcc (t_irc_server *server, char *channel_name)
{
    t_irc_channel *ptr_channel;
    
    if (!server || !channel_name)
        return NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type == CHANNEL_TYPE_DCC_CHAT)
            && (ascii_strcasecmp (ptr_channel->name, channel_name) == 0))
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
    
    if (!string)
        return 0;
    
    first_char[0] = string[0];
    first_char[1] = '\0';
    return (strpbrk (first_char, CHANNEL_PREFIX)) ? 1 : 0;    
}

/*
 * channel_get_charset_decode_iso: get decode iso value for channel
 *                                 if not found for channel, look for server
 *                                 if not found for server, look for global
 */

char *
channel_get_charset_decode_iso (t_irc_server *server, t_irc_channel *channel)
{
    char *pos, *result;
    int length;
    
    if (!server)
        return (cfg_look_charset_decode_iso) ?
            strdup (cfg_look_charset_decode_iso) : strdup ("");
    
    if (!channel)
        return server_get_charset_decode_iso (server);
    
    config_option_list_get_value (&(server->charset_decode_iso),
                                  channel->name, &pos, &length);
    if (pos && (length > 0))
    {
        result = strdup (pos);
        result[length] = '\0';
        return result;
    }
    
    return server_get_charset_decode_iso (server);
}

/*
 * channel_get_charset_decode_utf: get decode utf value for channel
 *                                 if not found for channel, look for server
 *                                 if not found for server, look for global
 */

char *
channel_get_charset_decode_utf (t_irc_server *server, t_irc_channel *channel)
{
    char *pos, *result;
    int length;
    
    if (!server)
        return (cfg_look_charset_decode_utf) ?
            strdup (cfg_look_charset_decode_utf) : strdup ("");
    
    if (!channel)
        return server_get_charset_decode_utf (server);
    
    config_option_list_get_value (&(server->charset_decode_utf),
                                  channel->name, &pos, &length);
    if (pos && (length > 0))
    {
        result = strdup (pos);
        result[length] = '\0';
        return result;
    }
    
    return server_get_charset_decode_utf (server);
}

/*
 * channel_get_charset_encode: get encode value for channel
 *                             if not found for channel, look for server
 *                             if not found for server, look for global
 */

char *
channel_get_charset_encode (t_irc_server *server, t_irc_channel *channel)
{
    char *pos, *result;
    int length;
    
    if (!server)
        return (cfg_look_charset_encode) ?
            strdup (cfg_look_charset_encode) : strdup ("");
    
    if (!channel)
        return server_get_charset_encode (server);
    
    config_option_list_get_value (&(server->charset_encode),
                                  channel->name, &pos, &length);
    if (pos && (length > 0))
    {
        result = strdup (pos);
        result[length] = '\0';
        return result;
    }
    
    return server_get_charset_encode (server);
}

/*
 * channel_iconv_decode: convert string to local charset
 */

char *
channel_iconv_decode (t_irc_server *server, t_irc_channel *channel, char *string)
{
    char *from_charset, *string2;
    
    if (!local_utf8 || !utf8_is_valid (string))
    {
        if (local_utf8)
            from_charset = channel_get_charset_decode_iso (server, channel);
        else
            from_charset = channel_get_charset_decode_utf (server, channel);
        string2 = weechat_iconv (from_charset,
                                 (cfg_look_charset_internal && cfg_look_charset_internal[0]) ?
                                 cfg_look_charset_internal : local_charset,
                                 string);
        free (from_charset);
        return string2;
    }
    else
        return strdup (string);
}

/*
 *
 */

char *
channel_iconv_encode (t_irc_server *server, t_irc_channel *channel, char *string)
{
    char *to_charset, *string2;
    
    to_charset = channel_get_charset_encode (server, channel);
    string2 = weechat_iconv ((cfg_look_charset_internal && cfg_look_charset_internal[0]) ?
                             cfg_look_charset_internal : local_charset,
                             to_charset,
                             string);
    free (to_charset);
    return string2;
}

/*
 * channel_remove_away: remove away for all nicks on a channel
 */

void
channel_remove_away (t_irc_channel *channel)
{
    t_irc_nick *ptr_nick;
    
    if (channel->type == CHANNEL_TYPE_CHANNEL)
    {
        for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
        {
            NICK_SET_FLAG(ptr_nick, 0, NICK_AWAY);
        }
        gui_nicklist_draw (channel->buffer, 0, 0);
    }
}

/*
 * channel_check_away: check for away on a channel
 */

void
channel_check_away (t_irc_server *server, t_irc_channel *channel, int force)
{
    if (channel->type == CHANNEL_TYPE_CHANNEL)
    {
        if (force || (cfg_irc_away_check_max_nicks == 0) ||
            (channel->nicks_count <= cfg_irc_away_check_max_nicks))
        {
            channel->checking_away++;
            server_sendf (server, "WHO %s\r\n", channel->name);
        }
        else
            channel_remove_away (channel);
    }
}

/*
 * channel_set_away: set/unset away status for a channel
 */

void
channel_set_away (t_irc_channel *channel, char *nick, int is_away)
{
    t_irc_nick *ptr_nick;
    
    if (channel->type == CHANNEL_TYPE_CHANNEL)
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
    
    ptr_channel = channel_search_dcc (ptr_dcc->server, ptr_dcc->nick);
    if (!ptr_channel)
    {
        ptr_channel = channel_new (ptr_dcc->server, CHANNEL_TYPE_DCC_CHAT,
                                   ptr_dcc->nick);
        if (!ptr_channel)
            return 0;
        gui_buffer_new (gui_current_window, ptr_dcc->server, ptr_channel,
                    BUFFER_TYPE_STANDARD, 0);
    }
    
    if (ptr_channel->dcc_chat &&
        (!DCC_ENDED(((t_irc_dcc *)(ptr_channel->dcc_chat))->status)))
        return 0;
    
    ptr_channel->dcc_chat = ptr_dcc;
    ptr_dcc->channel = ptr_channel;
    gui_window_redraw_buffer (ptr_channel->buffer);
    return 1;
}

/*
 * channel_get_notify_level: get channel notify level
 */

int
channel_get_notify_level (t_irc_server *server, t_irc_channel *channel)
{
    char *name, *pos, *pos2;
    int server_default_notify, notify;
    
    if ((!server) || (!channel))
        return NOTIFY_LEVEL_DEFAULT;
    
    if ((!server->notify_levels) || (!server->notify_levels[0]))
        return NOTIFY_LEVEL_DEFAULT;
    
    server_default_notify = server_get_default_notify_level (server);
    
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
    if ((notify >= NOTIFY_LEVEL_MIN) && (notify <= NOTIFY_LEVEL_MAX))
        return notify;

    return server_default_notify;
}

/*
 * server_set_notify_level: set channel notify level
 */

void
channel_set_notify_level (t_irc_server *server, t_irc_channel *channel, int notify)
{
    char level_string[2];
    
    if ((!server) || (!channel))
        return;
    
    level_string[0] = notify + '0';
    level_string[1] = '\0';
    config_option_list_set (&(server->notify_levels), channel->name, level_string);
}

/*
 * channel_print_log: print channel infos in log (usually for crash dump)
 */

void
channel_print_log (t_irc_channel *channel)
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
    weechat_log_printf ("     prev_channel . . . . : 0x%X\n",   channel->prev_channel);
    weechat_log_printf ("     next_channel . . . . : 0x%X\n",   channel->next_channel);
}
