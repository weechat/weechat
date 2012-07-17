/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * relay-client.c: client functions for relay plugin
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-client.h"
#include "irc/relay-irc.h"
#include "weechat/relay-weechat.h"
#include "relay-config.h"
#include "relay-buffer.h"
#include "relay-server.h"


char *relay_client_status_string[] =   /* strings for status                */
{ N_("connecting"), N_("waiting auth"),
  N_("connected"), N_("auth failed"), N_("disconnected")
};

struct t_relay_client *relay_clients = NULL;
struct t_relay_client *last_relay_client = NULL;
int relay_client_count = 0;            /* number of clients                 */


/*
 * relay_client_valid: check if a client pointer exists
 *                     return 1 if client exists
 *                            0 if client is not found
 */

int
relay_client_valid (struct t_relay_client *client)
{
    struct t_relay_client *ptr_client;

    if (!client)
        return 0;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        if (ptr_client == client)
            return 1;
    }

    /* client not found */
    return 0;
}

/*
 * relay_client_search_by_number: search a client by number (first client is 0)
 */

struct t_relay_client *
relay_client_search_by_number (int number)
{
    struct t_relay_client *ptr_client;
    int i;

    i = 0;
    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        if (i == number)
            return ptr_client;
        i++;
    }

    /* client not found */
    return NULL;
}

/*
 * relay_client_search_by_id: search a client by id
 */

struct t_relay_client *
relay_client_search_by_id (int id)
{
    struct t_relay_client *ptr_client;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        if (ptr_client->id == id)
            return ptr_client;
    }

    /* client not found */
    return NULL;
}

/*
 * relay_client_recv_cb: read data from a client
 */

int
relay_client_recv_cb (void *arg_client, int fd)
{
    struct t_relay_client *client;
    static char buffer[4096 + 2];
    int num_read;

    /* make C compiler happy */
    (void) fd;

    client = (struct t_relay_client *)arg_client;

    num_read = recv (client->sock, buffer, sizeof (buffer) - 1, 0);
    if (num_read > 0)
    {
        client->bytes_recv += num_read;
        buffer[num_read] = '\0';
        switch (client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_recv (client, buffer);
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_recv (client, buffer);
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }
        relay_buffer_refresh (NULL);
    }
    else
    {
        if ((num_read == 0)
            || ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
        {
            weechat_printf (NULL,
                            _("%s%s: reading data on socket for client %d: "
                              "error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            client->id,
                            errno,
                            (num_read == 0) ? _("(connection closed by peer)") :
                            strerror (errno));
            relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_client_outqueue_add: add a message in out queue
 */

void
relay_client_outqueue_add (struct t_relay_client *client, const char *data,
                           int data_size)
{
    struct t_relay_client_outqueue *new_outqueue;

    if (!client || !data || (data_size <= 0))
        return;

    new_outqueue = malloc (sizeof (*new_outqueue));
    if (new_outqueue)
    {
        new_outqueue->data = malloc (data_size);
        if (!new_outqueue->data)
        {
            free (new_outqueue);
            return;
        }
        memcpy (new_outqueue->data, data, data_size);
        new_outqueue->data_size = data_size;

        new_outqueue->prev_outqueue = client->last_outqueue;
        new_outqueue->next_outqueue = NULL;
        if (client->outqueue)
            client->last_outqueue->next_outqueue = new_outqueue;
        else
            client->outqueue = new_outqueue;
        client->last_outqueue = new_outqueue;
    }
}

/*
 * relay_client_outqueue_free: free a message in out queue
 */

void
relay_client_outqueue_free (struct t_relay_client *client,
                            struct t_relay_client_outqueue *outqueue)
{
    struct t_relay_client_outqueue *new_outqueue;

    /* remove outqueue message */
    if (client->last_outqueue == outqueue)
        client->last_outqueue = outqueue->prev_outqueue;
    if (outqueue->prev_outqueue)
    {
        (outqueue->prev_outqueue)->next_outqueue = outqueue->next_outqueue;
        new_outqueue = client->outqueue;
    }
    else
        new_outqueue = outqueue->next_outqueue;

    if (outqueue->next_outqueue)
        (outqueue->next_outqueue)->prev_outqueue = outqueue->prev_outqueue;

    /* free data */
    if (outqueue->data)
        free (outqueue->data);
    free (outqueue);

    /* set new head */
    client->outqueue = new_outqueue;
}

/*
 * relay_client_outqueue_free_all: free all outqueued messages
 */

void
relay_client_outqueue_free_all (struct t_relay_client *client)
{
    while (client->outqueue)
    {
        relay_client_outqueue_free (client, client->outqueue);
    }
}

/*
 * relay_client_send: send data to client (add in outqueue if it's impossible
 *                    to send now)
 *                    return number of bytes sent to client
 */

int
relay_client_send (struct t_relay_client *client, const char *data,
                   int data_size)
{
    int num_sent;

    if (client->sock < 0)
        return -1;

    num_sent = -1;

    /*
     * if outqueue is not empty, add to outqueue
     * (because message must be sent *after* messages already in outqueue)
     */
    if (client->outqueue)
    {
        relay_client_outqueue_add (client, data, data_size);
    }
    else
    {
        num_sent = send (client->sock, data, data_size, 0);
        if (num_sent >= 0)
        {
            if (num_sent > 0)
            {
                client->bytes_sent += num_sent;
                relay_buffer_refresh (NULL);
            }
            if (num_sent < data_size)
            {
                /* some data was not sent, add it to outqueue */
                relay_client_outqueue_add (client, data + num_sent,
                                           data_size - num_sent);
            }
        }
        else if (num_sent < 0)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                /* add message to queue (will be sent later) */
                relay_client_outqueue_add (client, data, data_size);
            }
            else
            {
                weechat_printf (NULL,
                                _("%s%s: sending data to client %d: error %d %s"),
                                weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                                client->id,
                                errno, strerror (errno));
                relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
            }
        }
    }

    return num_sent;
}

/*
 * relay_client_timer_cb: timer called each second to perform some operations
 *                        on clients
 */

int
relay_client_timer_cb (void *data, int remaining_calls)
{
    struct t_relay_client *ptr_client;
    int num_sent;
    char *buf;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        if (ptr_client->sock >= 0)
        {
            while (ptr_client->outqueue)
            {
                num_sent = send (ptr_client->sock, ptr_client->outqueue->data,
                                 ptr_client->outqueue->data_size, 0);
                if (num_sent >= 0)
                {
                    if (num_sent > 0)
                    {
                        ptr_client->bytes_sent += num_sent;
                        relay_buffer_refresh (NULL);
                    }
                    if (num_sent == ptr_client->outqueue->data_size)
                    {
                        /* whole data sent, remove outqueue */
                        relay_client_outqueue_free (ptr_client,
                                                    ptr_client->outqueue);
                    }
                    else
                    {
                        /*
                         * some data was not sent, update outqueue and stop
                         * sending data from outqueue
                         */
                        if (num_sent > 0)
                        {
                            buf = malloc (ptr_client->outqueue->data_size - num_sent);
                            if (buf)
                            {
                                memcpy (buf,
                                        ptr_client->outqueue->data + num_sent,
                                        ptr_client->outqueue->data_size - num_sent);
                                free (ptr_client->outqueue->data);
                                ptr_client->outqueue->data = buf;
                                ptr_client->outqueue->data_size = ptr_client->outqueue->data_size - num_sent;
                            }
                        }
                        break;
                    }
                }
                else if (num_sent < 0)
                {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                    {
                        /* we will retry later this client's queue */
                        break;
                    }
                    else
                    {
                        weechat_printf (NULL,
                                        _("%s%s: sending data to client %d: error %d %s"),
                                        weechat_prefix ("error"),
                                        RELAY_PLUGIN_NAME,
                                        ptr_client->id,
                                        errno, strerror (errno));
                        relay_client_set_status (ptr_client,
                                                 RELAY_STATUS_DISCONNECTED);
                    }
                }
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * relay_client_new: create a new client
 */

struct t_relay_client *
relay_client_new (int sock, const char *address, struct t_relay_server *server)
{
    struct t_relay_client *new_client;

    new_client = malloc (sizeof (*new_client));
    if (new_client)
    {
        new_client->id = (relay_clients) ? relay_clients->id + 1 : 1;
        new_client->sock = sock;
        new_client->address = strdup ((address) ? address : "?");
        new_client->status = RELAY_STATUS_CONNECTED;
        new_client->protocol = server->protocol;
        new_client->protocol_args = (server->protocol_args) ? strdup (server->protocol_args) : NULL;
        new_client->listen_start_time = server->start_time;
        new_client->start_time = time (NULL);
        new_client->end_time = 0;
        new_client->hook_fd = NULL;
        new_client->last_activity = new_client->start_time;
        new_client->bytes_recv = 0;
        new_client->bytes_sent = 0;

        new_client->protocol_data = NULL;
        switch (new_client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_alloc (new_client);
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_alloc (new_client);
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }

        new_client->outqueue = NULL;
        new_client->last_outqueue = NULL;

        new_client->prev_client = NULL;
        new_client->next_client = relay_clients;
        if (relay_clients)
            relay_clients->prev_client = new_client;
        else
            last_relay_client = new_client;
        relay_clients = new_client;

        weechat_printf (NULL,
                        _("%s: new client from %s%s%s on port %d (id: %d, relaying: %s%s%s)"),
                        RELAY_PLUGIN_NAME,
                        RELAY_COLOR_CHAT_HOST,
                        new_client->address,
                        RELAY_COLOR_CHAT,
                        server->port,
                        new_client->id,
                        relay_protocol_string[new_client->protocol],
                        (new_client->protocol_args) ? "." : "",
                        (new_client->protocol_args) ? new_client->protocol_args : "");

        new_client->hook_fd = weechat_hook_fd (new_client->sock,
                                               1, 0, 0,
                                               &relay_client_recv_cb,
                                               new_client);

        relay_client_count++;

        if (!relay_buffer
            && weechat_config_boolean (relay_config_look_auto_open_buffer))
        {
            relay_buffer_open ();
        }

        relay_buffer_refresh (WEECHAT_HOTLIST_PRIVATE);
    }
    else
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory for new client"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
    }

    return new_client;
}

/*
 * relay_client_new_with_infolist: create a new client using an infolist
 */

struct t_relay_client *
relay_client_new_with_infolist (struct t_infolist *infolist)
{
    struct t_relay_client *new_client;
    const char *str;

    new_client = malloc (sizeof (*new_client));
    if (new_client)
    {
        new_client->id = weechat_infolist_integer (infolist, "id");
        new_client->sock = weechat_infolist_integer (infolist, "sock");
        new_client->address = strdup (weechat_infolist_string (infolist, "address"));
        new_client->status = weechat_infolist_integer (infolist, "status");
        new_client->protocol = weechat_infolist_integer (infolist, "protocol");
        str = weechat_infolist_string (infolist, "protocol_args");
        new_client->protocol_args = (str) ? strdup (str) : NULL;
        new_client->listen_start_time = weechat_infolist_time (infolist, "listen_start_time");
        new_client->start_time = weechat_infolist_time (infolist, "start_time");
        new_client->end_time = weechat_infolist_time (infolist, "end_time");
        if (new_client->sock >= 0)
        {
            new_client->hook_fd = weechat_hook_fd (new_client->sock,
                                                   1, 0, 0,
                                                   &relay_client_recv_cb,
                                                   new_client);
        }
        else
            new_client->hook_fd = NULL;
        new_client->last_activity = weechat_infolist_time (infolist, "last_activity");
        sscanf (weechat_infolist_string (infolist, "bytes_recv"),
                "%lu", &(new_client->bytes_recv));
        sscanf (weechat_infolist_string (infolist, "bytes_sent"),
                "%lu", &(new_client->bytes_sent));

        switch (new_client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_alloc_with_infolist (new_client,
                                                   infolist);
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_alloc_with_infolist (new_client,
                                               infolist);
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }

        new_client->outqueue = NULL;
        new_client->last_outqueue = NULL;

        new_client->prev_client = NULL;
        new_client->next_client = relay_clients;
        if (relay_clients)
            relay_clients->prev_client = new_client;
        else
            last_relay_client = new_client;
        relay_clients = new_client;

        relay_client_count++;
    }

    return new_client;
}

/*
 * relay_client_set_status: set status for a client
 */

void
relay_client_set_status (struct t_relay_client *client,
                         enum t_relay_status status)
{
    client->status = status;

    if (RELAY_CLIENT_HAS_ENDED(client))
    {
        client->end_time = time (NULL);

        relay_client_outqueue_free_all (client);

        if (client->hook_fd)
        {
            weechat_unhook (client->hook_fd);
            client->hook_fd = NULL;
        }
        switch (client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_close_connection (client);
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_close_connection (client);
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }
        switch (client->status)
        {
            case RELAY_STATUS_AUTH_FAILED:
                weechat_printf (NULL,
                                _("%s%s: authentication failed with client %s%s%s (%s%s%s)"),
                                weechat_prefix ("error"),
                                RELAY_PLUGIN_NAME,
                                RELAY_COLOR_CHAT_HOST,
                                client->address,
                                RELAY_COLOR_CHAT,
                                relay_protocol_string[client->protocol],
                                (client->protocol_args) ? "." : "",
                                (client->protocol_args) ? client->protocol_args : "");
                break;
            case RELAY_STATUS_DISCONNECTED:
                weechat_printf (NULL,
                                _("%s: disconnected from client %s%s%s (%s%s%s)"),
                                RELAY_PLUGIN_NAME,
                                RELAY_COLOR_CHAT_HOST,
                                client->address,
                                RELAY_COLOR_CHAT,
                                relay_protocol_string[client->protocol],
                                (client->protocol_args) ? "." : "",
                                (client->protocol_args) ? client->protocol_args : "");
                break;
            default:
                break;
        }

        if (client->sock >= 0)
        {
            close (client->sock);
            client->sock = -1;
        }
    }

    relay_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
}

/*
 * relay_client_free: remove a client
 */

void
relay_client_free (struct t_relay_client *client)
{
    struct t_relay_client *new_relay_clients;

    if (!client)
        return;

    /* remove client from list */
    if (last_relay_client == client)
        last_relay_client = client->prev_client;
    if (client->prev_client)
    {
        (client->prev_client)->next_client = client->next_client;
        new_relay_clients = relay_clients;
    }
    else
        new_relay_clients = client->next_client;
    if (client->next_client)
        (client->next_client)->prev_client = client->prev_client;

    /* free data */
    if (client->address)
        free (client->address);
    if (client->protocol_args)
        free (client->protocol_args);
    if (client->hook_fd)
        weechat_unhook (client->hook_fd);
    if (client->protocol_data)
    {
        switch (client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_free (client);
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_free (client);
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }
    }
    relay_client_outqueue_free_all (client);

    free (client);

    relay_clients = new_relay_clients;

    relay_client_count--;
    if (relay_buffer_selected_line >= relay_client_count)
    {
        relay_buffer_selected_line = (relay_client_count == 0) ?
            0 : relay_client_count - 1;
    }
}

/*
 * relay_client_free_all: remove all clients
 */

void
relay_client_free_all ()
{
    while (relay_clients)
    {
        relay_client_free (relay_clients);
    }
}

/*
 * relay_client_disconnect: disconnect one client
 */

void
relay_client_disconnect (struct t_relay_client *client)
{
    if (client->sock >= 0)
    {
        relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
    }
}

/*
 * relay_client_disconnect_all: disconnect from all clients
 */

void
relay_client_disconnect_all ()
{
    struct t_relay_client *ptr_client;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        relay_client_disconnect (ptr_client);
    }
}

/*
 * relay_client_add_to_infolist: add a client in an infolist
 *                               return 1 if ok, 0 if error
 */

int
relay_client_add_to_infolist (struct t_infolist *infolist,
                              struct t_relay_client *client)
{
    struct t_infolist_item *ptr_item;
    char value[128];

    if (!infolist || !client)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_integer (ptr_item, "id", client->id))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "sock", client->sock))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "address", client->address))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "status", client->status))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "status_string", relay_client_status_string[client->status]))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "protocol", client->protocol))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "protocol_string", relay_protocol_string[client->protocol]))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "protocol_args", client->protocol_args))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "listen_start_time", client->listen_start_time))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "start_time", client->start_time))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "end_time", client->end_time))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "hook_fd", client->hook_fd))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "last_activity", client->last_activity))
        return 0;
    snprintf (value, sizeof (value), "%lu", client->bytes_recv);
    if (!weechat_infolist_new_var_string (ptr_item, "bytes_recv", value))
        return 0;
    snprintf (value, sizeof (value), "%lu", client->bytes_sent);
    if (!weechat_infolist_new_var_string (ptr_item, "bytes_sent", value))
        return 0;

    switch (client->protocol)
    {
        case RELAY_PROTOCOL_WEECHAT:
            relay_weechat_add_to_infolist (ptr_item, client);
            break;
        case RELAY_PROTOCOL_IRC:
            relay_irc_add_to_infolist (ptr_item, client);
            break;
        case RELAY_NUM_PROTOCOLS:
            break;
    }

    return 1;
}

/*
 * relay_client_print_log: print client infos in log (usually for crash dump)
 */

void
relay_client_print_log ()
{
    struct t_relay_client *ptr_client;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[relay client (addr:0x%lx)]", ptr_client);
        weechat_log_printf ("  id. . . . . . . . . . : %d",   ptr_client->id);
        weechat_log_printf ("  sock. . . . . . . . . : %d",   ptr_client->sock);
        weechat_log_printf ("  address . . . . . . . : '%s'", ptr_client->address);
        weechat_log_printf ("  status. . . . . . . . : %d (%s)",
                            ptr_client->status,
                            relay_client_status_string[ptr_client->status]);
        weechat_log_printf ("  protocol. . . . . . . : %d (%s)",
                            ptr_client->protocol,
                            relay_protocol_string[ptr_client->protocol]);
        weechat_log_printf ("  protocol_args . . . . : '%s'",  ptr_client->protocol_args);
        weechat_log_printf ("  listen_start_time . . : %ld",   ptr_client->listen_start_time);
        weechat_log_printf ("  start_time. . . . . . : %ld",   ptr_client->start_time);
        weechat_log_printf ("  end_time. . . . . . . : %ld",   ptr_client->end_time);
        weechat_log_printf ("  hook_fd . . . . . . . : 0x%lx", ptr_client->hook_fd);
        weechat_log_printf ("  last_activity . . . . : %ld",   ptr_client->last_activity);
        weechat_log_printf ("  bytes_recv. . . . . . : %lu",   ptr_client->bytes_recv);
        weechat_log_printf ("  bytes_sent. . . . . . : %lu",   ptr_client->bytes_sent);
        weechat_log_printf ("  protocol_data . . . . : 0x%lx", ptr_client->protocol_data);
        switch (ptr_client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_print_log (ptr_client);
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_print_log (ptr_client);
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }
        weechat_log_printf ("  outqueue. . . . . . . : 0x%lx", ptr_client->outqueue);
        weechat_log_printf ("  last_outqueue . . . . : 0x%lx", ptr_client->last_outqueue);
        weechat_log_printf ("  prev_client . . . . . : 0x%lx", ptr_client->prev_client);
        weechat_log_printf ("  next_client . . . . . : 0x%lx", ptr_client->next_client);
    }
}
