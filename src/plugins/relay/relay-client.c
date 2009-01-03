/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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

/* relay-client.c: client functions for relay plugin */


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
#include "relay-config.h"
#include "relay-buffer.h"


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
 * relay_client_sendf: send formatted data to client
 */

int
relay_client_sendf (struct t_relay_client *client, const char *format, ...)
{
    va_list args;
    static char buffer[4096];
    char str_length[8];
    int length, num_sent;
    
    if (!client)
        return 0;
    
    va_start (args, format);
    vsnprintf (buffer + 7, sizeof (buffer) - 7 - 1, format, args);
    va_end (args);
    
    length = strlen (buffer + 7);
    snprintf (str_length, sizeof (str_length), "%07d", length);
    memcpy (buffer, str_length, 7);
    
    num_sent = send (client->sock, buffer, length + 7, 0);
    
    client->bytes_sent += length + 7;
    
    if (num_sent < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: error sending data to client %s"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                        strerror (errno));
    }
    
    return num_sent;
}

/*
 * relay_client_send_infolist: send infolist to client
 */

void
relay_client_send_infolist (struct t_relay_client *client,
                            const char *name,
                            struct t_infolist *infolist)
{
    const char *fields;
    char **argv;
    int i, argc, size;
    
    relay_client_sendf (client, "name %s", name);
    
    while (weechat_infolist_next (infolist))
    {
        fields = weechat_infolist_fields (infolist);
        if (fields)
        {
            argv = weechat_string_explode (fields, ",", 0, 0, &argc);
            if (argv && (argc > 0))
            {
                for (i = 0; i < argc; i++)
                {
                    switch (argv[i][0])
                    {
                        case 'i':
                            relay_client_sendf (client, "%s %c %d",
                                                argv[i] + 2, argv[i][0],
                                                weechat_infolist_integer (infolist,
                                                                          argv[i] + 2));
                            break;
                        case 's':
                            relay_client_sendf (client, "%s %c %s",
                                                argv[i] + 2, argv[i][0],
                                                weechat_infolist_string (infolist,
                                                                         argv[i] + 2));
                            break;
                        case 'p':
                            relay_client_sendf (client, "%s %c %lx",
                                                argv[i] + 2, argv[i][0],
                                                (long unsigned int)weechat_infolist_pointer (infolist,
                                                                                             argv[i] + 2));
                            break;
                        case 'b':
                            relay_client_sendf (client, "%s %c %lx",
                                                argv[i] + 2, argv[i][0],
                                                (long unsigned int)weechat_infolist_buffer (infolist,
                                                                                            argv[i] + 2,
                                                                                            &size));
                            break;
                        case 't':
                            relay_client_sendf (client, "%s %c %ld",
                                                argv[i] + 2, argv[i][0],
                                                weechat_infolist_time (infolist, argv[i] + 2));
                            break;
                    }
                }
            }
            if (argv)
                weechat_string_free_exploded (argv);
        }
    }
}

/*
 * relay_client_recv_cb: read data from a client
 */

int
relay_client_recv_cb (void *arg_client)
{
    struct t_relay_client *client;
    static char buffer[4096 + 2];
    struct t_infolist *infolist;
    int num_read;
    
    client = (struct t_relay_client *)arg_client;
    
    num_read = recv (client->sock, buffer, sizeof (buffer) - 1, 0);
    if (num_read > 0)
    {
        client->bytes_recv += num_read;
        buffer[num_read] = '\0';
        if (buffer[num_read - 1] == '\n')
            buffer[--num_read] = '\0';
        if (buffer[num_read - 1] == '\r')
            buffer[--num_read] = '\0';
        if (weechat_relay_plugin->debug)
        {
            weechat_printf (NULL, "%s: data received from %s: \"%s\"",
                            RELAY_PLUGIN_NAME, client->address, buffer);
        }
        if (weechat_strcasecmp (buffer, "quit") == 0)
            relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
        else
        {
            infolist = weechat_infolist_get (buffer, NULL, NULL);
            if (infolist)
            {
                relay_client_send_infolist (client, buffer, infolist);
                weechat_infolist_free (infolist);
            }
        }
        relay_buffer_refresh (NULL);
    }
    else
    {
        relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * relay_client_new: create a new client
 */

struct t_relay_client *
relay_client_new (int sock, char *address)
{
    struct t_relay_client *new_client;
    
    new_client = malloc (sizeof (*new_client));
    if (new_client)
    {
        new_client->sock = sock;
        new_client->address = strdup ((address) ? address : "?");
        new_client->status = RELAY_STATUS_CONNECTED;
        new_client->start_time = time (NULL);
        new_client->hook_fd = NULL;
        new_client->hook_timer = NULL;
        new_client->last_activity = new_client->start_time;
        new_client->bytes_recv = 0;
        new_client->bytes_sent = 0;
        
        new_client->prev_client = NULL;
        new_client->next_client = relay_clients;
        if (relay_clients)
            relay_clients->prev_client = new_client;
        else
            last_relay_client = new_client;
        relay_clients = new_client;
        
        weechat_printf (NULL,
                        _("%s: new client @ %s"),
                        RELAY_PLUGIN_NAME,
                        new_client->address);

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
 * relay_client_set_status: set status for a client
 */

void
relay_client_set_status (struct t_relay_client *client,
                         enum t_relay_status status)
{
    client->status = status;
    
    if (RELAY_CLIENT_HAS_ENDED(client->status))
    {
        if (client->hook_fd)
        {
            weechat_unhook (client->hook_fd);
            client->hook_fd = NULL;
        }
        if (client->hook_timer)
        {
            weechat_unhook (client->hook_timer);
            client->hook_timer = NULL;
        }
        switch (client->status)
        {
            case RELAY_STATUS_AUTH_FAILED:
                weechat_printf (NULL,
                                _("%s%s: authentication failed with client @ %s"),
                                weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                                client->address);
                break;
            case RELAY_STATUS_DISCONNECTED:
                weechat_printf (NULL,
                                _("%s: disconnected from client @ %s"),
                                RELAY_PLUGIN_NAME, client->address);
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
    if (client->hook_fd)
        weechat_unhook (client->hook_fd);
    if (client->hook_timer)
        weechat_unhook (client->hook_timer);
    
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
    
    if (!weechat_infolist_new_var_integer (ptr_item, "sock", client->sock))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "address", client->address))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "status", client->status))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "status_string", relay_client_status_string[client->status]))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "start_time", client->start_time))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "hook_fd", client->hook_fd))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "hook_timer", client->hook_timer))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "last_activity", client->last_activity))
        return 0;
    snprintf (value, sizeof (value), "%lu", client->bytes_recv);
    if (!weechat_infolist_new_var_string (ptr_item, "bytes_recv", value))
        return 0;
    snprintf (value, sizeof (value), "%lu", client->bytes_sent);
    if (!weechat_infolist_new_var_string (ptr_item, "bytes_sent", value))
        return 0;
    
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
        weechat_log_printf ("  sock. . . . . . . . : %d",    ptr_client->sock);
        weechat_log_printf ("  address . . . . . . : '%s'", ptr_client->address);
        weechat_log_printf ("  status. . . . . . . : %d (%s)",
                            ptr_client->status,
                            relay_client_status_string[ptr_client->status]);
        weechat_log_printf ("  start_time. . . . . : %ld",   ptr_client->start_time);
        weechat_log_printf ("  hook_fd . . . . . . : 0x%lx", ptr_client->hook_fd);
        weechat_log_printf ("  hook_timer. . . . . : 0x%lx", ptr_client->hook_timer);
        weechat_log_printf ("  last_activity . . . : %ld",   ptr_client->last_activity);
        weechat_log_printf ("  bytes_recv. . . . . : %lu",   ptr_client->bytes_recv);
        weechat_log_printf ("  bytes_sent. . . . . : %lu",   ptr_client->bytes_sent);
        weechat_log_printf ("  prev_client . . . . : 0x%lx", ptr_client->prev_client);
        weechat_log_printf ("  next_client . . . . : 0x%lx", ptr_client->next_client);
    }
}
