/*
 * irc-typing.c - manage typing status on channels/private
 *
 * Copyright (C) 2021-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-config.h"
#include "irc-server.h"


/*
 * Callback for signals "typing_self_*".
 */

int
irc_typing_signal_typing_self_cb (const void *pointer, void *data,
                                  const char *signal, const char *type_data,
                                  void *signal_data)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    int new_state;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    /* sending self typing status is allowed? */
    if (!weechat_config_boolean (irc_config_look_typing_status_self))
        return WEECHAT_RC_OK;

    /* search server/channel with buffer */
    irc_buffer_get_server_and_channel (signal_data, &ptr_server, &ptr_channel);
    if (!ptr_server || !ptr_channel)
        return WEECHAT_RC_OK;

    /* typing not allowed on server? */
    if (!ptr_server->typing_allowed)
        return WEECHAT_RC_OK;

    /* typing works only if capability "message-tags" is enabled */
    if (!weechat_hashtable_has_key (ptr_server->cap_list, "message-tags"))
        return WEECHAT_RC_OK;

    if (strcmp (signal, "typing_self_typing") == 0)
        new_state = IRC_CHANNEL_TYPING_STATE_ACTIVE;
    else if (strcmp (signal, "typing_self_paused") == 0)
        new_state = IRC_CHANNEL_TYPING_STATE_PAUSED;
    else if (strcmp (signal, "typing_self_cleared") == 0)
        new_state = IRC_CHANNEL_TYPING_STATE_DONE;
    else if (strcmp (signal, "typing_self_sent") == 0)
        new_state = IRC_CHANNEL_TYPING_STATE_OFF;
    else
        new_state = -1;

    if ((new_state >= 0) && (new_state != ptr_channel->typing_state))
    {
        ptr_channel->typing_state = new_state;
        ptr_channel->typing_status_sent = 0;
    }

    return WEECHAT_RC_OK;
}

/*
 * Sends self typing status to channels/privates of a server.
 */

void
irc_typing_send_to_targets (struct t_irc_server *server)
{
    struct t_irc_channel *ptr_channel;
    time_t current_time;

    if (!weechat_config_boolean (irc_config_look_typing_status_self)
        || !server->typing_allowed)
    {
        return;
    }

    current_time = time (NULL);

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (!ptr_channel->part
            && (ptr_channel->typing_state != IRC_CHANNEL_TYPING_STATE_OFF)
            && (ptr_channel->typing_status_sent + 3 < current_time))
        {
            irc_server_sendf (
                server,
                IRC_SERVER_SEND_OUTQ_PRIO_LOW, NULL,
                "@+typing=%s TAGMSG %s",
                irc_channel_typing_state_string[ptr_channel->typing_state],
                ptr_channel->name);
            if (ptr_channel->typing_state == IRC_CHANNEL_TYPING_STATE_ACTIVE)
            {
                ptr_channel->typing_status_sent = current_time;
            }
            else
            {
                ptr_channel->typing_state = IRC_CHANNEL_TYPING_STATE_OFF;
                ptr_channel->typing_status_sent = 0;
            }
        }
    }
}

/*
 * Sets state of a nick on a channel.
 */

void
irc_typing_channel_set_nick (struct t_irc_channel *channel, const char *nick,
                             int state)
{
    char signal_data[1024];

    snprintf (signal_data, sizeof (signal_data),
              "%p;%s;%s",
              channel->buffer,
              (state == IRC_CHANNEL_TYPING_STATE_ACTIVE) ? "typing" :
              ((state == IRC_CHANNEL_TYPING_STATE_PAUSED) ? "paused" : "off"),
              nick);
    weechat_hook_signal_send ("typing_set_nick",
                              WEECHAT_HOOK_SIGNAL_STRING,
                              signal_data);
}

/*
 * Resets all nicks state on a channel.
 */

void
irc_typing_channel_reset (struct t_irc_channel *channel)
{
    weechat_hook_signal_send ("typing_reset_buffer",
                              WEECHAT_HOOK_SIGNAL_POINTER,
                              channel->buffer);
}
