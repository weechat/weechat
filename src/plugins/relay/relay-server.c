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
 * relay-server.c: server functions for relay plugin
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-server.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-config.h"


struct t_relay_server *relay_servers = NULL;
struct t_relay_server *last_relay_server = NULL;


/*
 * relay_server_get_protocol_args: get protocol and arguments from a string
 *                                 with format "protocol.args"
 *                                 Note: *protocol and *protocol_args must be
 *                                 freed after use
 */

void
relay_server_get_protocol_args (const char *protocol_and_args,
                                char **protocol, char **protocol_args)
{
    char *pos;

    pos = strchr (protocol_and_args, '.');
    if (pos)
    {
        *protocol = weechat_strndup (protocol_and_args,
                                     pos - protocol_and_args);
        *protocol_args = strdup (pos + 1);
    }
    else
    {
        *protocol = strdup (protocol_and_args);
        *protocol_args = NULL;
    }
}

/*
 * relay_server_search: search server by protocol.args
 */

struct t_relay_server *
relay_server_search (const char *protocol_and_args)
{
    char *protocol, *protocol_args;
    struct t_relay_server *ptr_server;

    relay_server_get_protocol_args (protocol_and_args,
                                    &protocol, &protocol_args);

    ptr_server = NULL;

    if (protocol)
    {
        for (ptr_server = relay_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (strcmp (protocol, relay_protocol_string[ptr_server->protocol]) == 0)
            {
                if (!protocol_args && !ptr_server->protocol_args)
                    break;
                if (protocol_args && ptr_server->protocol_args
                    && (strcmp (protocol_args, ptr_server->protocol_args) == 0))
                {
                    break;
                }
            }
        }
    }

    if (protocol)
        free (protocol);
    if (protocol_args)
        free (protocol_args);

    return ptr_server;
}

/*
 * relay_server_search_port: search server by port
 */

struct t_relay_server *
relay_server_search_port (int port)
{
    struct t_relay_server *ptr_server;

    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->port == port)
            return ptr_server;
    }

    /* server not found */
    return NULL;
}

/*
 * relay_server_close_socket: close socket for a relay server
 */

void
relay_server_close_socket (struct t_relay_server *server)
{
    if (server->hook_fd)
    {
        weechat_unhook (server->hook_fd);
        server->hook_fd = NULL;
    }
    if (server->sock >= 0)
    {
        close (server->sock);
        server->sock = -1;

        if (!relay_signal_upgrade_received)
        {
            weechat_printf (NULL,
                            _("%s: socket closed for %s%s%s (port %d)"),
                            RELAY_PLUGIN_NAME,
                            relay_protocol_string[server->protocol],
                            (server->protocol_args) ? "." : "",
                            (server->protocol_args) ? server->protocol_args : "",
                            server->port);
        }
    }
}

/*
 * relay_server_sock_cb: read data from a client which is connecting on socket
 */

int
relay_server_sock_cb (void *data, int fd)
{
    struct t_relay_server *server;
    struct sockaddr_in client_addr;
    socklen_t client_length;
    int client_fd, flags;
    char ipv4_address[INET_ADDRSTRLEN + 1], *ptr_address;

    /* make C compiler happy */
    (void) fd;

    server = (struct t_relay_server *)data;

    client_length = sizeof (client_addr);
    memset (&client_addr, 0, client_length);

    client_fd = accept (server->sock, (struct sockaddr *) &client_addr,
                        &client_length);
    if (client_fd < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot accept client on port %d (%s%s%s)"),
                        weechat_prefix ("error"),
                        RELAY_PLUGIN_NAME,
                        server->port,
                        relay_protocol_string[server->protocol],
                        (server->protocol_args) ? "." : "",
                        (server->protocol_args) ? server->protocol_args : "");
        return WEECHAT_RC_OK;
    }

    ptr_address = NULL;
    if (inet_ntop (AF_INET,
                   &(client_addr.sin_addr),
                   ipv4_address,
                   INET_ADDRSTRLEN))
    {
        ptr_address = ipv4_address;
    }

    /* check if IP is allowed, if not, just close socket */
    if (relay_config_regex_allowed_ips
        && (regexec (relay_config_regex_allowed_ips, ptr_address, 0, NULL, 0) != 0))
    {
        if (weechat_relay_plugin->debug >= 2)
        {
            weechat_printf (NULL,
                            _("%s%s: IP address \"%s\" not allowed for relay"),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME,
                            ptr_address);
        }
        close (client_fd);
        return WEECHAT_RC_OK;
    }

    /* set non-blocking mode for socket */
    flags = fcntl (client_fd, F_GETFL);
    if (flags == -1)
        flags = 0;
    fcntl (client_fd, F_SETFL, flags | O_NONBLOCK);

    /* add the client */
    relay_client_new (client_fd, ptr_address, server);

    return WEECHAT_RC_OK;
}

/*
 * relay_server_create_socket: create socket and server on port
 */

int
relay_server_create_socket (struct t_relay_server *server)
{
    int set, max_clients;
    struct sockaddr_in server_addr;

    server->sock = socket (AF_INET, SOCK_STREAM, 0);
    if (server->sock < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot create socket"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        return 0;
    }

    set = 1;
    if (setsockopt (server->sock, SOL_SOCKET, SO_REUSEADDR,
                    (void *) &set, sizeof (set)) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot set socket option "
                          "\"SO_REUSEADDR\""),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        close (server->sock);
        server->sock = -1;
        return 0;
    }

    set = 1;
    if (setsockopt (server->sock, SOL_SOCKET, SO_KEEPALIVE,
                    (void *) &set, sizeof (set)) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot set socket option "
                          "\"SO_KEEPALIVE\""),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        close (server->sock);
        server->sock = -1;
        return 0;
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    if (weechat_config_string (relay_config_network_bind_address)
        && weechat_config_string (relay_config_network_bind_address)[0])
    {
        server_addr.sin_addr.s_addr = inet_addr (weechat_config_string (relay_config_network_bind_address));
    }
    else
    {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    }
    server_addr.sin_port = htons (server->port);

    if (bind (server->sock, (struct sockaddr *) &server_addr,
              sizeof (server_addr)) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: error with \"bind\" on port %d (%s%s%s)"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                        server->port,
                        relay_protocol_string[server->protocol],
                        (server->protocol_args) ? "." : "",
                        (server->protocol_args) ? server->protocol_args : "");
        close (server->sock);
        server->sock = -1;
        return 0;
    }

    max_clients = weechat_config_integer (relay_config_network_max_clients);

    listen (server->sock, max_clients);

    weechat_printf (NULL,
                    _("%s: listening on port %d (relay: %s%s%s, max %d clients)"),
                    RELAY_PLUGIN_NAME,
                    server->port,
                    relay_protocol_string[server->protocol],
                    (server->protocol_args) ? "." : "",
                    (server->protocol_args) ? server->protocol_args : "",
                    max_clients);

    server->hook_fd = weechat_hook_fd (server->sock,
                                       1, 0, 0,
                                       &relay_server_sock_cb,
                                       server);

    server->start_time = time (NULL);

    return 1;
}

/*
 * relay_server_new: add a socket relaying on a port
 */

struct t_relay_server *
relay_server_new (enum t_relay_protocol protocol,
                  const char *protocol_args,
                  int port)
{
    struct t_relay_server *new_server;

    if (relay_server_search_port (port))
    {
        weechat_printf (NULL, _("%s%s: error: port \"%d\" is already used"),
                        weechat_prefix ("error"),
                        RELAY_PLUGIN_NAME, port);
        return NULL;
    }

    new_server = malloc (sizeof (*new_server));
    if (new_server)
    {
        new_server->protocol = protocol;
        new_server->protocol_args =
            (protocol_args) ? strdup (protocol_args) : NULL;
        new_server->port = port;
        new_server->sock = -1;
        new_server->hook_fd = NULL;
        new_server->start_time = 0;

        if (!relay_server_create_socket (new_server))
        {
            if (new_server->protocol_args)
                free (new_server->protocol_args);
            free (new_server);
            return NULL;
        }

        new_server->prev_server = NULL;
        new_server->next_server = relay_servers;
        if (relay_servers)
            relay_servers->prev_server = new_server;
        else
            last_relay_server = new_server;
        relay_servers = new_server;
    }
    else
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory for listening on new port"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
    }

    return new_server;
}

/*
 * relay_server_update_port: update port in a server
 */

void
relay_server_update_port (struct t_relay_server *server, int port)
{
    if (port != server->port)
    {
        relay_server_close_socket (server);
        server->port = port;
        relay_server_create_socket (server);
    }
}

/*
 * relay_server_free: remove a server
 */

void
relay_server_free (struct t_relay_server *server)
{
    struct t_relay_server *new_relay_servers;

    if (!server)
        return;

    /* remove server from list */
    if (last_relay_server == server)
        last_relay_server = server->prev_server;
    if (server->prev_server)
    {
        (server->prev_server)->next_server = server->next_server;
        new_relay_servers = relay_servers;
    }
    else
        new_relay_servers = server->next_server;
    if (server->next_server)
        (server->next_server)->prev_server = server->prev_server;

    /* free data */
    relay_server_close_socket (server);
    if (server->protocol_args)
        free (server->protocol_args);

    free (server);

    relay_servers = new_relay_servers;
}

/*
 * relay_server_free_all: remove all servers
 */

void
relay_server_free_all ()
{
    while (relay_servers)
    {
        relay_server_free (relay_servers);
    }
}

/*
 * relay_server_print_log: print server infos in log (usually for crash dump)
 */

void
relay_server_print_log ()
{
    struct t_relay_server *ptr_server;

    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[relay server (addr:0x%lx)]", ptr_server);
        weechat_log_printf ("  protocol. . . . . . : %d (%s)",
                            ptr_server->protocol,
                            relay_protocol_string[ptr_server->protocol]);
        weechat_log_printf ("  protocol_args . . . : '%s'",  ptr_server->protocol_args);
        weechat_log_printf ("  port. . . . . . . . : %d",    ptr_server->port);
        weechat_log_printf ("  sock. . . . . . . . : %d",    ptr_server->sock);
        weechat_log_printf ("  hook_fd . . . . . . : 0x%lx", ptr_server->hook_fd);
        weechat_log_printf ("  start_time. . . . . : %ld",   ptr_server->start_time);
        weechat_log_printf ("  prev_server . . . . : 0x%lx", ptr_server->prev_server);
        weechat_log_printf ("  next_server . . . . : 0x%lx", ptr_server->next_server);
    }
}
