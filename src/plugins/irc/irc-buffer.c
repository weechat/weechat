/*
 * irc-buffer.c - buffer functions for IRC plugin
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <limits.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-join.h"
#include "irc-list.h"
#include "irc-raw.h"
#include "irc-server.h"


/*
 * Gets IRC server and channel pointers with a buffer pointer (buffer may be a
 * server or a channel).
 */

void
irc_buffer_get_server_and_channel (struct t_gui_buffer *buffer,
                                   struct t_irc_server **server,
                                   struct t_irc_channel **channel)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;

    if (server)
        *server = NULL;
    if (channel)
        *channel = NULL;

    if (!buffer)
        return;

    /* look for a server or channel using this buffer */
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer == buffer)
        {
            if (server)
                *server = ptr_server;
            return;
        }

        if (ptr_server->list->buffer == buffer)
        {
            if (server)
                *server = ptr_server;
            return;
        }

        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->buffer == buffer)
            {
                if (server)
                    *server = ptr_server;
                if (channel)
                    *channel = ptr_channel;
                return;
            }
        }
    }

    /* no server or channel found */
}

/*
 * Builds buffer name with a server and a channel.
 *
 * Note: result must be freed after use.
 */

char *
irc_buffer_build_name (const char *server, const char *channel)
{
    char **buffer;

    buffer = weechat_string_dyn_alloc (128);

    if (!server && !channel)
        goto end;

    if (server && channel)
    {
        weechat_string_dyn_concat (buffer, server, -1);
        weechat_string_dyn_concat (buffer, ".", -1);
        weechat_string_dyn_concat (buffer, channel, -1);
    }
    else if (server)
    {
        weechat_string_dyn_concat (buffer, "server.", -1);
        weechat_string_dyn_concat (buffer, server, -1);
    }
    else
    {
        weechat_string_dyn_concat (buffer, channel, -1);
    }

end:
    return weechat_string_dyn_free (buffer, 0);
}

/*
 * Closes all channels/privates of a given server.
 */

void
irc_buffer_close_server_channels (struct t_irc_server *server)
{
    struct t_hdata *hdata_buffer;
    struct t_gui_buffer *ptr_buffer, *ptr_next_buffer;
    const char *ptr_type, *ptr_server_name;

    hdata_buffer = weechat_hdata_get ("buffer");
    ptr_buffer = weechat_hdata_get_list (hdata_buffer, "gui_buffers");

    while (ptr_buffer)
    {
        ptr_next_buffer = weechat_hdata_move (hdata_buffer, ptr_buffer, 1);

        if (weechat_buffer_get_pointer (ptr_buffer,
                                        "plugin") == weechat_irc_plugin)
        {
            ptr_type = weechat_buffer_get_string (ptr_buffer, "localvar_type");
            ptr_server_name = weechat_buffer_get_string (ptr_buffer,
                                                         "localvar_server");
            if (ptr_type && ptr_type[0]
                && ptr_server_name && ptr_server_name[0]
                && ((strcmp (ptr_type, "channel") == 0)
                    || (strcmp (ptr_type, "private") == 0))
                && (strcmp (ptr_server_name, server->name) == 0))
            {
                weechat_buffer_close (ptr_buffer);
            }
        }

        ptr_buffer = ptr_next_buffer;
    }
}

/*
 * Callback called when a buffer is closed.
 */

int
irc_buffer_close_cb (const void *pointer, void *data,
                     struct t_gui_buffer *buffer)
{
    struct t_irc_channel *next_channel;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (buffer == irc_raw_buffer)
    {
        irc_raw_buffer = NULL;
    }
    else
    {
        if (ptr_channel)
        {
            /*
             * remove channel from autojoin if autojoin_dynamic is set,
             * still connected to server and not quitting/upgrading WeeChat
             */
            if (ptr_server
                && IRC_SERVER_OPTION_BOOLEAN(ptr_server,
                                             IRC_SERVER_OPTION_AUTOJOIN_DYNAMIC)
                && ptr_server->is_connected
                && !irc_signal_quit_received
                && !irc_signal_upgrade_received)
            {
                irc_join_remove_channel_from_autojoin (ptr_server,
                                                       ptr_channel->name);
            }
            /* send PART for channel if its buffer is closed */
            if ((ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                && (ptr_channel->nicks))
            {
                irc_command_part_channel (ptr_server, ptr_channel->name, NULL);
            }
            irc_channel_free (ptr_server, ptr_channel);
        }
        else if (ptr_server && (ptr_server->buffer == buffer))
        {
            if (!ptr_server->disconnected)
            {
                /* send QUIT to server, then disconnect */
                irc_command_quit_server (ptr_server, NULL);
                irc_server_disconnect (ptr_server, 0, 0);
            }

            /* disable reconnection */
            ptr_server->reconnect_delay = 0;
            ptr_server->reconnect_start = 0;

            /* consider auto-join has never been done */
            ptr_server->autojoin_done = 0;

            /* close server channels/privates */
            ptr_channel = ptr_server->channels;
            while (ptr_channel)
            {
                next_channel = ptr_channel->next_channel;
                if (ptr_channel->buffer != buffer)
                    weechat_buffer_close (ptr_channel->buffer);
                ptr_channel = next_channel;
            }

            /*
             * close remaining channels/privates
             * (which are not yet in server->channels)
             */
            irc_buffer_close_server_channels (ptr_server);

            /* close list buffer */
            if (ptr_server->list->buffer)
                weechat_buffer_close (ptr_server->list->buffer);

            ptr_server->buffer = NULL;
        }
        else if (ptr_server && (ptr_server->list->buffer == buffer))
        {
            irc_list_free_data (ptr_server);
            ptr_server->list->buffer = NULL;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for comparing two nicks in nicklist (called when searching a nick in
 * nicklist).
 * The "casemapping" of server is used in comparison.
 *
 * Returns:
 *  -1: nick1 < nick2
 *   0: nick1 == nick2
 *   1: nick2 > nick2
 */

int
irc_buffer_nickcmp_cb (const void *pointer, void *data,
                       struct t_gui_buffer *buffer,
                       const char *nick1,
                       const char *nick2)
{
    struct t_irc_server *server;

    /* make C compiler happy */
    (void) data;

    if (pointer)
        server = (struct t_irc_server *)pointer;
    else
        irc_buffer_get_server_and_channel (buffer, &server, NULL);

    if (server)
    {
        return irc_server_strcasecmp (server, nick1, nick2);
    }
    else
    {
        /* default is RFC 1459 casemapping comparison */
        return weechat_strcasecmp_range (nick1, nick2, 29);
    }
}

/*
 * Searches for the server buffer with the lowest number.
 *
 * Returns pointer to buffer found, NULL if not found.
 */

struct t_gui_buffer *
irc_buffer_search_server_lowest_number ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_server *ptr_server;
    int number, number_found;

    ptr_buffer = NULL;
    number_found = INT_MAX;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer)
        {
            number = weechat_buffer_get_integer (ptr_server->buffer, "number");
            if (number < number_found)
            {
                number_found = number;
                ptr_buffer = ptr_server->buffer;
            }
        }
    }
    return ptr_buffer;
}

/*
 * Searches for the private buffer with the lowest number.
 * If server is not NULL, searches only for this server.
 *
 * Returns pointer to buffer found, NULL if not found.
 */

struct t_gui_buffer *
irc_buffer_search_private_lowest_number (struct t_irc_server *server)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    int number, number_found;

    ptr_buffer = NULL;
    number_found = INT_MAX;

    for (ptr_server = (server) ? server : irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
                && ptr_channel->buffer)
            {
                number = weechat_buffer_get_integer (ptr_channel->buffer,
                                                     "number");
                if (number < number_found)
                {
                    number_found = number;
                    ptr_buffer = ptr_channel->buffer;
                }
            }
        }
        if (server)
            break;
    }
    return ptr_buffer;
}

/*
 * Moves new channel/pv or list buffer near server.
 *
 * Parameters:
 *   list_buffer: 1 if it is a /list buffer, 0 otherwise
 *   channel_type: -1, IRC_CHANNEL_TYPE_CHANNEL or IRC_CHANNEL_TYPE_PRIVATE
 */

void
irc_buffer_move_near_server (struct t_irc_server *server,
                             int list_buffer, int channel_type,
                             struct t_gui_buffer *buffer)
{
    int number, number_channel, number_last_channel, number_last_private;
    int number_found;
    char str_number[32];
    const char *ptr_type, *ptr_server_name;
    struct t_hdata *hdata_buffer;
    struct t_gui_buffer *ptr_buffer;

    number = weechat_buffer_get_integer (buffer, "number");
    number_last_channel = 0;
    number_last_private = 0;
    number_found = 0;

    hdata_buffer = weechat_hdata_get ("buffer");
    ptr_buffer = weechat_hdata_get_list (hdata_buffer, "gui_buffers");
    while (ptr_buffer)
    {
        if ((ptr_buffer != buffer)
            && (weechat_buffer_get_pointer (ptr_buffer,
                                            "plugin") == weechat_irc_plugin))
        {
            ptr_type = weechat_buffer_get_string (ptr_buffer,
                                                  "localvar_type");
            ptr_server_name = weechat_buffer_get_string (ptr_buffer,
                                                         "localvar_server");
            number_channel = weechat_buffer_get_integer (ptr_buffer,
                                                         "number");
            if (ptr_type && ptr_type[0]
                && ptr_server_name && ptr_server_name[0]
                && (strcmp (ptr_server_name, server->name) == 0))
            {
                if (strcmp (ptr_type, "channel") == 0)
                {
                    if (number_channel > number_last_channel)
                        number_last_channel = number_channel;
                }
                else if (strcmp (ptr_type, "private") == 0)
                {
                    if (number_channel > number_last_private)
                        number_last_private = number_channel;
                }
            }
        }
        /* move to next buffer */
        ptr_buffer = weechat_hdata_move (hdata_buffer, ptr_buffer, 1);
    }

    if (list_buffer)
    {
        if ((number_last_private > 0)
            && (number_last_private > number_last_channel))
        {
            number_found = number_last_private + 1;
        }
        else if ((number_last_channel > 0)
                 && (number_last_channel > number_last_private))
        {
            number_found = number_last_channel + 1;
        }
    }
    else
    {
        /* use last channel/pv number + 1 */
        switch (channel_type)
        {
            case IRC_CHANNEL_TYPE_CHANNEL:
                if (number_last_channel > 0)
                    number_found = number_last_channel + 1;
                break;
            case IRC_CHANNEL_TYPE_PRIVATE:
                if (number_last_private > 0)
                    number_found = number_last_private + 1;
                else if (number_last_channel > 0)
                    number_found = number_last_channel + 1;
                break;
        }
    }

    if ((number_found == 0)
        && (weechat_config_enum (irc_config_look_server_buffer) ==
            IRC_CONFIG_LOOK_SERVER_BUFFER_INDEPENDENT))
    {
        number_found = weechat_buffer_get_integer (server->buffer, "number") + 1;
    }

    /* switch to number found */
    if ((number_found >= 1) && (number_found != number))
    {
        snprintf (str_number, sizeof (str_number), "%d", number_found);
        weechat_buffer_set (buffer, "number", str_number);
    }
}
