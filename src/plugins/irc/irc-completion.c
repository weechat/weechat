/*
 * irc-completion.c - completion for IRC commands
 *
 * Copyright (C) 2003-2018 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-completion.h"
#include "irc-config.h"
#include "irc-ignore.h"
#include "irc-modelist.h"
#include "irc-nick.h"
#include "irc-notify.h"
#include "irc-server.h"


/*
 * Adds current server to completion list.
 */

int
irc_completion_server_cb (const void *pointer, void *data,
                          const char *completion_item,
                          struct t_gui_buffer *buffer,
                          struct t_gui_completion *completion)
{
    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_server)
    {
        weechat_hook_completion_list_add (completion, ptr_server->name,
                                          0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds self nick of current server to completion list.
 */

int
irc_completion_server_nick_cb (const void *pointer, void *data,
                               const char *completion_item,
                               struct t_gui_buffer *buffer,
                               struct t_gui_completion *completion)
{
    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_server && ptr_server->nick)
    {
        weechat_hook_completion_list_add (completion, ptr_server->nick,
                                          1, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds channels of current server to completion list.
 */

int
irc_completion_server_channels_cb (const void *pointer, void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    struct t_irc_channel *ptr_channel2;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_server)
    {
        for (ptr_channel2 = ptr_server->channels; ptr_channel2;
             ptr_channel2 = ptr_channel2->next_channel)
        {
            if (ptr_channel2->type == IRC_CHANNEL_TYPE_CHANNEL)
            {
                weechat_hook_completion_list_add (completion, ptr_channel2->name,
                                                  0, WEECHAT_LIST_POS_SORT);
            }
        }

        /* add current channel first in list */
        if (ptr_channel)
        {
            weechat_hook_completion_list_add (completion, ptr_channel->name,
                                              0, WEECHAT_LIST_POS_BEGINNING);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds privates of current server to completion list.
 */

int
irc_completion_server_privates_cb (const void *pointer, void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    struct t_irc_channel *ptr_channel;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            {
                weechat_hook_completion_list_add (completion, ptr_channel->name,
                                                  0, WEECHAT_LIST_POS_SORT);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds nicks of current server to completion list.
 */

int
irc_completion_server_nicks_cb (const void *pointer, void *data,
                                const char *completion_item,
                                struct t_gui_buffer *buffer,
                                struct t_gui_completion *completion)
{
    struct t_irc_channel *ptr_channel2;
    struct t_irc_nick *ptr_nick;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_server)
    {
        for (ptr_channel2 = ptr_server->channels; ptr_channel2;
             ptr_channel2 = ptr_channel2->next_channel)
        {
            if (ptr_channel2->type == IRC_CHANNEL_TYPE_CHANNEL)
            {
                for (ptr_nick = ptr_channel2->nicks; ptr_nick;
                     ptr_nick = ptr_nick->next_nick)
                {
                    weechat_hook_completion_list_add (completion, ptr_nick->name,
                                                      1, WEECHAT_LIST_POS_SORT);
                }
            }
        }

        /* add self nick at the end */
        weechat_hook_completion_list_add (completion, ptr_server->nick,
                                          1, WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds servers to completion list.
 */

int
irc_completion_servers_cb (const void *pointer, void *data,
                           const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    struct t_irc_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        weechat_hook_completion_list_add (completion, ptr_server->name,
                                          0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds current channel to completion list.
 */

int
irc_completion_channel_cb (const void *pointer, void *data,
                           const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_channel)
    {
        weechat_hook_completion_list_add (completion, ptr_channel->name,
                                          0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds recent speakers to completion list.
 */

void
irc_completion_channel_nicks_add_speakers (struct t_gui_completion *completion,
                                           struct t_irc_server *server,
                                           struct t_irc_channel *channel,
                                           int highlight)
{
    int list_size, i;
    const char *nick;

    if (channel->nicks_speaking[highlight])
    {
        list_size = weechat_list_size (channel->nicks_speaking[highlight]);
        for (i = 0; i < list_size; i++)
        {
            nick = weechat_list_string (
                weechat_list_get (channel->nicks_speaking[highlight], i));
            if (nick && irc_nick_search (server, channel, nick))
            {
                weechat_hook_completion_list_add (completion,
                                                  nick,
                                                  1,
                                                  WEECHAT_LIST_POS_BEGINNING);
            }
        }
    }
}

/*
 * Adds nicks of current channel to completion list.
 */

int
irc_completion_channel_nicks_cb (const void *pointer, void *data,
                                 const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    struct t_irc_nick *ptr_nick;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_channel)
    {
        switch (ptr_channel->type)
        {
            case IRC_CHANNEL_TYPE_CHANNEL:
                for (ptr_nick = ptr_channel->nicks; ptr_nick;
                     ptr_nick = ptr_nick->next_nick)
                {
                    weechat_hook_completion_list_add (completion,
                                                      ptr_nick->name,
                                                      1,
                                                      WEECHAT_LIST_POS_SORT);
                }
                /* add recent speakers on channel */
                if (weechat_config_integer (irc_config_look_nick_completion_smart) == IRC_CONFIG_NICK_COMPLETION_SMART_SPEAKERS)
                {
                    irc_completion_channel_nicks_add_speakers (completion, ptr_server, ptr_channel, 0);
                }
                /* add nicks whose make highlights on me recently on this channel */
                if (weechat_config_integer (irc_config_look_nick_completion_smart) == IRC_CONFIG_NICK_COMPLETION_SMART_SPEAKERS_HIGHLIGHTS)
                {
                    irc_completion_channel_nicks_add_speakers (completion, ptr_server, ptr_channel, 1);
                }
                /* add self nick at the end */
                weechat_hook_completion_list_add (completion,
                                                  ptr_server->nick,
                                                  1,
                                                  WEECHAT_LIST_POS_END);
                break;
            case IRC_CHANNEL_TYPE_PRIVATE:
                /* remote nick */
                weechat_hook_completion_list_add (completion,
                                                  ptr_channel->name,
                                                  1,
                                                  WEECHAT_LIST_POS_SORT);
                /* add self nick at the end */
                weechat_hook_completion_list_add (completion,
                                                  ptr_server->nick,
                                                  1,
                                                  WEECHAT_LIST_POS_END);
                break;
        }
        ptr_channel->nick_completion_reset = 0;
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds nicks and hosts of current channel to completion list.
 */

int
irc_completion_channel_nicks_hosts_cb (const void *pointer, void *data,
                                       const char *completion_item,
                                       struct t_gui_buffer *buffer,
                                       struct t_gui_completion *completion)
{
    struct t_irc_nick *ptr_nick;
    char *buf;
    int length;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_channel)
    {
        switch (ptr_channel->type)
        {
            case IRC_CHANNEL_TYPE_CHANNEL:
                for (ptr_nick = ptr_channel->nicks; ptr_nick;
                     ptr_nick = ptr_nick->next_nick)
                {
                    weechat_hook_completion_list_add (completion,
                                                      ptr_nick->name,
                                                      1,
                                                      WEECHAT_LIST_POS_SORT);
                    if (ptr_nick->host)
                    {
                        length = strlen (ptr_nick->name) + 1 +
                            strlen (ptr_nick->host) + 1;
                        buf = malloc (length);
                        if (buf)
                        {
                            snprintf (buf, length, "%s!%s",
                                      ptr_nick->name, ptr_nick->host);
                            weechat_hook_completion_list_add (
                                completion, buf, 0, WEECHAT_LIST_POS_SORT);
                            free (buf);
                        }
                    }
                }
                break;
            case IRC_CHANNEL_TYPE_PRIVATE:
                weechat_hook_completion_list_add (
                    completion, ptr_channel->name, 1, WEECHAT_LIST_POS_SORT);
                break;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds modelist masks current channel to completion list.
 */

int
irc_completion_modelist_cb (const void *pointer, void *data,
                            const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    char *pos;
    struct t_irc_modelist *ptr_modelist;
    struct t_irc_modelist_item *ptr_item;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    pos = strchr (completion_item, ':');
    if (pos)
        pos++;

    if (pos && pos[0] && ptr_channel)
    {
        ptr_modelist = irc_modelist_search (ptr_channel, pos[0]);
        if (ptr_modelist)
        {
            for (ptr_item = ptr_modelist->items; ptr_item; ptr_item = ptr_item->next_item)
            {
                weechat_hook_completion_list_add (completion,
                                                  ptr_item->mask,
                                                  0, WEECHAT_LIST_POS_END);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds topic of current channel to completion list.
 */

int
irc_completion_channel_topic_cb (const void *pointer, void *data,
                                 const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    char *topic;
    int length;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_channel && ptr_channel->topic && ptr_channel->topic[0])
    {
        if (irc_server_strncasecmp (ptr_server, ptr_channel->topic,
                                    ptr_channel->name,
                                    strlen (ptr_channel->name)) == 0)
        {
            /*
             * if topic starts with channel name, add another channel name
             * before topic, so that completion will be:
             *   /topic #test #test is a test channel
             * instead of
             *   /topic #test is a test channel
             */
            length = strlen (ptr_channel->name) + strlen (ptr_channel->topic) + 16 + 1;
            topic = malloc (length);
            if (topic)
            {
                snprintf (topic, length, "%s %s",
                          ptr_channel->name, ptr_channel->topic);
            }
        }
        else
            topic = strdup (ptr_channel->topic);

        weechat_hook_completion_list_add (completion,
                                          (topic) ? topic : ptr_channel->topic,
                                          0, WEECHAT_LIST_POS_SORT);
        if (topic)
            free (topic);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds channels of all servers to completion list.
 */

int
irc_completion_channels_cb (const void *pointer, void *data,
                            const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    struct t_irc_server *ptr_server2;
    struct t_irc_channel *ptr_channel2;
    struct t_weelist *channels_current_server;
    int i;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    channels_current_server = weechat_list_new ();

    for (ptr_server2 = irc_servers; ptr_server2;
         ptr_server2 = ptr_server2->next_server)
    {
        for (ptr_channel2 = ptr_server2->channels; ptr_channel2;
             ptr_channel2 = ptr_channel2->next_channel)
        {
            if (ptr_channel2->type == IRC_CHANNEL_TYPE_CHANNEL)
            {
                if (ptr_server2 == ptr_server)
                {
                    /* will be added later to completions */
                    weechat_list_add (channels_current_server,
                                      ptr_channel2->name,
                                      WEECHAT_LIST_POS_SORT,
                                      NULL);
                }
                else
                {
                    weechat_hook_completion_list_add (completion,
                                                      ptr_channel2->name,
                                                      0,
                                                      WEECHAT_LIST_POS_SORT);
                }
            }
        }
    }

    /* add channels of current server first in list */
    for (i = weechat_list_size (channels_current_server) - 1; i >= 0; i--)
    {
        weechat_hook_completion_list_add (
            completion,
            weechat_list_string (
                weechat_list_get (channels_current_server, i)),
            0,
            WEECHAT_LIST_POS_BEGINNING);
    }
    weechat_list_free (channels_current_server);

    /* add current channel first in list */
    if (ptr_channel)
    {
        weechat_hook_completion_list_add (completion, ptr_channel->name,
                                          0, WEECHAT_LIST_POS_BEGINNING);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds privates of all servers to completion list.
 */

int
irc_completion_privates_cb (const void *pointer, void *data,
                            const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            {
                weechat_hook_completion_list_add (completion, ptr_channel->name,
                                                  0, WEECHAT_LIST_POS_SORT);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds default kick message to completion list.
 */

int
irc_completion_msg_kick_cb (const void *pointer, void *data,
                            const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    const char *msg_kick;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_server)
    {
        msg_kick = IRC_SERVER_OPTION_STRING(ptr_server,
                                            IRC_SERVER_OPTION_MSG_KICK);
        if (msg_kick && msg_kick[0])
        {
            weechat_hook_completion_list_add (completion, msg_kick,
                                              0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds default part message to completion list.
 */

int
irc_completion_msg_part_cb (const void *pointer, void *data,
                            const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    const char *msg_part;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_server)
    {
        msg_part = IRC_SERVER_OPTION_STRING(ptr_server,
                                            IRC_SERVER_OPTION_MSG_PART);
        if (msg_part && msg_part[0])
        {
            weechat_hook_completion_list_add (completion, msg_part,
                                              0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds ignore numbers to completion list.
 */

int
irc_completion_ignores_numbers_cb (const void *pointer, void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    struct t_irc_ignore *ptr_ignore;
    char str_number[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_ignore = irc_ignore_list; ptr_ignore;
         ptr_ignore = ptr_ignore->next_ignore)
    {
        snprintf (str_number, sizeof (str_number), "%d", ptr_ignore->number);
        weechat_hook_completion_list_add (completion, str_number,
                                          0, WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds nicks in notify list to completion list.
 */

int
irc_completion_notify_nicks_cb (const void *pointer, void *data,
                                const char *completion_item,
                                struct t_gui_buffer *buffer,
                                struct t_gui_completion *completion)
{
    struct t_irc_notify *ptr_notify;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;

    if (ptr_server)
    {
        for (ptr_notify = ptr_server->notify_list; ptr_notify;
             ptr_notify = ptr_notify->next_notify)
        {
            weechat_hook_completion_list_add (completion, ptr_notify->nick,
                                              0, WEECHAT_LIST_POS_SORT);
        }
    }
    else
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            for (ptr_notify = ptr_server->notify_list; ptr_notify;
                 ptr_notify = ptr_notify->next_notify)
            {
                weechat_hook_completion_list_add (completion, ptr_notify->nick,
                                                  0, WEECHAT_LIST_POS_SORT);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks completions.
 */

void
irc_completion_init ()
{
    weechat_hook_completion ("irc_server",
                             N_("current IRC server"),
                             &irc_completion_server_cb, NULL, NULL);
    weechat_hook_completion ("irc_server_nick",
                             N_("nick on current IRC server"),
                             &irc_completion_server_nick_cb, NULL, NULL);
    weechat_hook_completion ("irc_server_channels",
                             N_("channels on current IRC server"),
                             &irc_completion_server_channels_cb, NULL, NULL);
    weechat_hook_completion ("irc_server_privates",
                             N_("privates on current IRC server"),
                             &irc_completion_server_privates_cb, NULL, NULL);
    weechat_hook_completion ("irc_server_nicks",
                             N_("nicks on all channels of current IRC server"),
                             &irc_completion_server_nicks_cb, NULL, NULL);
    weechat_hook_completion ("irc_servers",
                             N_("IRC servers (internal names)"),
                             &irc_completion_servers_cb, NULL, NULL);
    weechat_hook_completion ("irc_channel",
                             N_("current IRC channel"),
                             &irc_completion_channel_cb, NULL, NULL);
    weechat_hook_completion ("nick",
                             N_("nicks of current IRC channel"),
                             &irc_completion_channel_nicks_cb, NULL, NULL);
    weechat_hook_completion ("irc_channel_nicks_hosts",
                             N_("nicks and hostnames of current IRC channel"),
                             &irc_completion_channel_nicks_hosts_cb, NULL, NULL);
    weechat_hook_completion ("irc_modelist",
                             N_("modelist masks of current IRC channel; "
                                "required argument: modelist mode"),
                             &irc_completion_modelist_cb, NULL, NULL);
    weechat_hook_completion ("irc_channel_topic",
                             N_("topic of current IRC channel"),
                             &irc_completion_channel_topic_cb, NULL, NULL);
    weechat_hook_completion ("irc_channels",
                             N_("channels on all IRC servers"),
                             &irc_completion_channels_cb, NULL, NULL);
    weechat_hook_completion ("irc_privates",
                             N_("privates on all IRC servers"),
                             &irc_completion_privates_cb, NULL, NULL);
    weechat_hook_completion ("irc_msg_kick",
                             N_("default kick message"),
                             &irc_completion_msg_kick_cb, NULL, NULL);
    weechat_hook_completion ("irc_msg_part",
                             N_("default part message for IRC channel"),
                             &irc_completion_msg_part_cb, NULL, NULL);
    weechat_hook_completion ("irc_ignores_numbers",
                             N_("numbers for defined ignores"),
                             &irc_completion_ignores_numbers_cb, NULL, NULL);
    weechat_hook_completion ("irc_notify_nicks",
                             N_("nicks in notify list"),
                             &irc_completion_notify_nicks_cb, NULL, NULL);
}
