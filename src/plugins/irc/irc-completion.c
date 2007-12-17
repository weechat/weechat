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

/* irc-command.c: IRC commands managment */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irc.h"
#include "irc-completion.h"
#include "irc-config.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"


/*
 * irc_completion_server_cb: callback for completion with current IRC server
 */

int
irc_completion_server_cb (void *data, char *completion,
                          struct t_gui_buffer *buffer, struct t_weelist *list)
{
    IRC_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion;
    
    if (ptr_server)
        weechat_list_add (list, ptr_server->name, "sort");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_completion_server_nicks_cb: callback for completion with nicks
 *                                 of all IRC servers
 */

int
irc_completion_server_nicks_cb (void *data, char *completion,
                                struct t_gui_buffer *buffer,
                                struct t_weelist *list)
{
    struct t_irc_server *ptr_server2;
    struct t_irc_channel *ptr_channel2;
    struct t_irc_nick *ptr_nick;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion;
    (void) buffer;

    if (ptr_server && ptr_channel)
    {
        for (ptr_server2 = irc_servers; ptr_server2;
             ptr_server2 = ptr_server2->next_server)
        {
            for (ptr_channel2 = ptr_server2->channels; ptr_channel2;
                 ptr_channel2 = ptr_channel2->next_channel)
            {
                if ((ptr_channel2 != ptr_channel)
                    && (ptr_channel2->type == IRC_CHANNEL_TYPE_CHANNEL))
                {
                    for (ptr_nick = ptr_channel2->nicks; ptr_nick;
                         ptr_nick = ptr_nick->next_nick)
                    {
                        weechat_list_add (list, ptr_nick->nick, "sort");
                    }
                }
            }
        }
        
        /* add current channel nicks at beginning */
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
        {
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                 ptr_nick = ptr_nick->next_nick)
            {
                weechat_list_add (list, ptr_nick->nick, "beginning");
            }
        }

        /* add self nick at the end */
        weechat_list_add (list, ptr_server->nick, "end");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_completion_servers_cb: callback for completion with IRC servers
 */

int
irc_completion_servers_cb (void *data, char *completion,
                           struct t_gui_buffer *buffer, struct t_weelist *list)
{
    struct t_irc_server *ptr_server;
    
    /* make C compiler happy */
    (void) data;
    (void) completion;
    (void) buffer;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        weechat_list_add (list, ptr_server->name, "sort");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_completion_channel_cb: callback for completion with current IRC channel
 */

int
irc_completion_channel_cb (void *data, char *completion,
                           struct t_gui_buffer *buffer, struct t_weelist *list)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion;
    
    if (ptr_channel)
        weechat_list_add (list, ptr_channel->name, "sort");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_completion_channel_nicks_cb: callback for completion with nicks
 *                                  of current IRC channel
 */

int
irc_completion_channel_nicks_cb (void *data, char *completion,
                                 struct t_gui_buffer *buffer,
                                 struct t_weelist *list)
{
    struct t_irc_nick *ptr_nick;
    char *nick;
    int list_size, i;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion;
    
    if (ptr_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                 ptr_nick = ptr_nick->next_nick)
            {
                weechat_list_add (list, ptr_nick->nick, "sort");
            }
            
            /* add nicks speaking recently on this channel */
            if (weechat_config_boolean (irc_config_irc_nick_completion_smart))
            {
                list_size = weechat_list_size (ptr_channel->nicks_speaking);
                for (i = 0; i < list_size; i++)
                {
                    nick = weechat_list_string (weechat_list_get (ptr_channel->nicks_speaking, i));
                    if (nick && irc_nick_search (ptr_channel, nick))
                    {
                        weechat_list_add (list, nick, "beginning");
                    }
                }
            }
            
            /* add self nick at the end */
            weechat_list_add (list, ptr_server->nick, "end");
        }
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            || (ptr_channel->type == IRC_CHANNEL_TYPE_DCC_CHAT))
        {
            weechat_list_add (list, ptr_channel->name, "sort");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_completion_channel_nicks_hosts_cb: callback for completion with nicks
 *                                        and hosts of current IRC channel
 */

int
irc_completion_channel_nicks_hosts_cb (void *data, char *completion,
                                       struct t_gui_buffer *buffer,
                                       struct t_weelist *list)
{
    struct t_irc_nick *ptr_nick;
    char *buf;
    int length;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion;
    
    if (ptr_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                 ptr_nick = ptr_nick->next_nick)
            {
                weechat_list_add (list, ptr_nick->nick, "sort");
                if (ptr_nick->host)
                {
                    length = strlen (ptr_nick->nick) + 1 +
                        strlen (ptr_nick->host) + 1;
                    buf = (char *)malloc (length);
                    if (buf)
                    {
                        snprintf (buf, length, "%s!%s",
                                  ptr_nick->nick, ptr_nick->host);
                        weechat_list_add (list, buf, "sort");
                        free (buf);
                    }
                }
            }
        }
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            || (ptr_channel->type == IRC_CHANNEL_TYPE_DCC_CHAT))
        {
            weechat_list_add (list, ptr_channel->name, "sort");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_completion_channel_topic_cb: callback for completion with topic of
 *                                  current IRC channel
 */

int
irc_completion_channel_topic_cb (void *data, char *completion,
                                 struct t_gui_buffer *buffer,
                                 struct t_weelist *list)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) completion;
    
    if (ptr_channel && ptr_channel->topic && ptr_channel->topic[0])
    {
        weechat_list_add (list, ptr_channel->topic, "sort");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_completion_channels_cb: callback for completion with IRC channels
 */

int
irc_completion_channels_cb (void *data, char *completion,
                            struct t_gui_buffer *buffer, struct t_weelist *list)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) data;
    (void) completion;
    (void) buffer;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            weechat_list_add (list, ptr_channel->name, "sort");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_completion_msg_part_cb: callback for completion with default part message
 */

int
irc_completion_msg_part_cb (void *data, char *completion,
                            struct t_gui_buffer *buffer, struct t_weelist *list)
{
    /* make C compiler happy */
    (void) data;
    (void) completion;
    (void) buffer;

    if (weechat_config_string (irc_config_irc_default_msg_part)
        && weechat_config_string (irc_config_irc_default_msg_part)[0])
    {
        weechat_list_add (list,
                          weechat_config_string (irc_config_irc_default_msg_part),
                          "sort");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_completion_init: init completion for IRC plugin
 */

void
irc_completion_init ()
{
    weechat_hook_completion ("irc_server", &irc_completion_server_cb, NULL);
    weechat_hook_completion ("irc_server_nicks",
                             &irc_completion_server_nicks_cb, NULL);
    weechat_hook_completion ("irc_servers", &irc_completion_servers_cb, NULL);
    weechat_hook_completion ("irc_channel", &irc_completion_channel_cb, NULL);
    weechat_hook_completion ("irc_channel_nicks",
                             &irc_completion_channel_nicks_cb, NULL);
    weechat_hook_completion ("irc_channel_nicks_hosts",
                             &irc_completion_channel_nicks_hosts_cb, NULL);
    weechat_hook_completion ("irc_channel_topic",
                             &irc_completion_channel_topic_cb, NULL);
    weechat_hook_completion ("irc_channels", &irc_completion_channels_cb, NULL);
    weechat_hook_completion ("irc_msg_part", &irc_completion_msg_part_cb, NULL);
}
