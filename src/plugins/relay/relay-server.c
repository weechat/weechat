/*
 * relay-server.c - server functions for relay plugin
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * Extracts protocol, arguments and options from a string with format
 * "options.protocol.args".
 *
 * Examples:
 *
 *   string                    ipv4 ipv6 ssl protocol protocol_args
 *   ---------------------------------------------------------------
 *   irc.freenode              1    1    0   irc      freenode
 *   ssl.irc.freenode          1    1    1   irc      freenode
 *   ipv4.irc.freenode         1    0    0   irc      freenode
 *   ipv6.irc.freenode         0    1    0   irc      freenode
 *   ipv4.ipv6.irc.freenode    1    1    0   irc      freenode
 *   ipv6.ssl.irc.freenode     0    1    1   irc      freenode
 *   weechat                   1    1    0   weechat
 *   ssl.weechat               1    1    1   weechat
 *   ipv6.ssl.weechat          0    1    1   weechat
 *
 * Note: *protocol and *protocol_args must be freed after use.
 */

void
relay_server_get_protocol_args (const char *protocol_and_args,
                                int *ipv4, int *ipv6, int *ssl,
                                char **protocol, char **protocol_args)
{
    int opt_ipv4, opt_ipv6, opt_ssl;
    char *pos;

    opt_ipv4 = -1;
    opt_ipv6 = -1;
    opt_ssl = 0;
    while (1)
    {
        if (strncmp (protocol_and_args, "ipv4.", 5) == 0)
        {
            opt_ipv4 = 1;
            protocol_and_args += 5;
        }
        else if (strncmp (protocol_and_args, "ipv6.", 5) == 0)
        {
            opt_ipv6 = 1;
            protocol_and_args += 5;
        }
        else if (strncmp (protocol_and_args, "ssl.", 4) == 0)
        {
            opt_ssl = 1;
            protocol_and_args += 4;
        }
        else
            break;
    }
    if ((opt_ipv4 == -1) && (opt_ipv6 == -1))
    {
        /*
         * no IPv4/IPv6 specified, then use defaults:
         * - IPv4 enabled
         * - IPv6 enabled if option relay.network.ipv6 is on
         */
        opt_ipv4 = 1;
        opt_ipv6 = weechat_config_boolean (relay_config_network_ipv6);
    }
    else
    {
        if (opt_ipv4 == -1)
            opt_ipv4 = 0;
        if (opt_ipv6 == -1)
            opt_ipv6 = 0;
    }
    if (!opt_ipv4 && !opt_ipv6)
    {
        /* both IPv4/IPv6 disabled (should never occur!) */
        opt_ipv4 = 1;
    }
    if (ipv4)
        *ipv4 = opt_ipv4;
    if (ipv6)
        *ipv6 = opt_ipv6;
    if (ssl)
        *ssl = opt_ssl;

    pos = strchr (protocol_and_args, '.');
    if (pos)
    {
        if (protocol)
        {
            *protocol = weechat_strndup (protocol_and_args,
                                         pos - protocol_and_args);
        }
        if (protocol_args)
            *protocol_args = strdup (pos + 1);
    }
    else
    {
        if (protocol)
            *protocol = strdup (protocol_and_args);
        if (protocol_args)
            *protocol_args = NULL;
    }
}

/*
 * Searches for a server by protocol.args.
 *
 * Returns pointer to server, NULL if not found.
 */

struct t_relay_server *
relay_server_search (const char *protocol_and_args)
{
    struct t_relay_server *ptr_server;

    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (strcmp (protocol_and_args, ptr_server->protocol_string) == 0)
            return ptr_server;
    }

    return NULL;
}

/*
 * Searches for a server by port.
 *
 * Returns pointer to new server, NULL if not found.
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
 * Closes socket for a relay server.
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
                            _("%s: socket closed for %s (port %d)"),
                            RELAY_PLUGIN_NAME, server->protocol_string,
                            server->port);
        }
    }
}

/*
 * Reads data from a client which is connecting on socket.
 */

int
relay_server_sock_cb (void *data, int fd)
{
    struct t_relay_server *server;
    struct sockaddr_in client_addr;
    struct sockaddr_in6 client_addr6;
    socklen_t client_addr_size;
    void *ptr_addr;
    int client_fd, flags, set;
    char ipv4_address[INET_ADDRSTRLEN + 1], ipv6_address[INET6_ADDRSTRLEN + 1];
    char *ptr_ip_address;

    /* make C compiler happy */
    (void) fd;

    server = (struct t_relay_server *)data;

    if (server->ipv6)
    {
        ptr_addr = &client_addr6;
        client_addr_size = sizeof (struct sockaddr_in6);
    }
    else
    {
        ptr_addr = &client_addr;
        client_addr_size = sizeof (struct sockaddr_in);
    }

    memset (ptr_addr, 0, client_addr_size);

    client_fd = accept (server->sock, (struct sockaddr *)ptr_addr,
                        &client_addr_size);
    if (client_fd < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot accept client on port %d (%s)"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                        server->port, server->protocol_string);
        return WEECHAT_RC_OK;
    }

    ptr_ip_address = NULL;
    if (server->ipv6)
    {
        if (inet_ntop (AF_INET6,
                       &(client_addr6.sin6_addr),
                       ipv6_address,
                       INET6_ADDRSTRLEN))
        {
            ptr_ip_address = ipv6_address;
        }
    }
    else
    {
        if (inet_ntop (AF_INET,
                       &(client_addr.sin_addr),
                       ipv4_address,
                       INET_ADDRSTRLEN))
        {
            ptr_ip_address = ipv4_address;
        }
    }

    /* check if IP is allowed, if not, just close socket */
    if (relay_config_regex_allowed_ips
        && (regexec (relay_config_regex_allowed_ips, ptr_ip_address, 0, NULL, 0) != 0))
    {
        if (weechat_relay_plugin->debug >= 2)
        {
            weechat_printf (NULL,
                            _("%s%s: IP address \"%s\" not allowed for relay"),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME,
                            ptr_ip_address);
        }
        close (client_fd);
        return WEECHAT_RC_OK;
    }

    /* set non-blocking mode for socket */
    flags = fcntl (client_fd, F_GETFL);
    if (flags == -1)
        flags = 0;
    fcntl (client_fd, F_SETFL, flags | O_NONBLOCK);

    /* set socket option SO_REUSEADDR */
    set = 1;
    if (setsockopt (client_fd, SOL_SOCKET, SO_REUSEADDR,
                    (void *) &set, sizeof (set)) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot set socket option "
                          "\"SO_REUSEADDR\""),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        close (client_fd);
        return WEECHAT_RC_OK;
    }

    /* add the client */
    relay_client_new (client_fd, ptr_ip_address, server);

    return WEECHAT_RC_OK;
}

/*
 * Creates socket and server on port.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_server_create_socket (struct t_relay_server *server)
{
    int domain, set, max_clients, addr_size;
    struct sockaddr_in server_addr;
    struct sockaddr_in6 server_addr6;
    void *ptr_addr;

    if (server->ipv6)
    {
        domain = AF_INET6;
        memset (&server_addr6, 0, sizeof (struct sockaddr_in6));
        server_addr6.sin6_family = domain;
        server_addr6.sin6_port = htons (server->port);
        server_addr6.sin6_addr = in6addr_any;
        ptr_addr = &server_addr6;
        addr_size = sizeof (struct sockaddr_in6);
    }
    else
    {
        domain = AF_INET;
        memset (&server_addr, 0, sizeof (struct sockaddr_in));
        server_addr.sin_family = domain;
        server_addr.sin_port = htons (server->port);
        if (weechat_config_string (relay_config_network_bind_address)
            && weechat_config_string (relay_config_network_bind_address)[0])
        {
            server_addr.sin_addr.s_addr = inet_addr (weechat_config_string (relay_config_network_bind_address));
        }
        else
        {
            server_addr.sin_addr.s_addr = INADDR_ANY;
        }
        ptr_addr = &server_addr;
        addr_size = sizeof (struct sockaddr_in);
    }
    if (weechat_config_string (relay_config_network_bind_address)
        && weechat_config_string (relay_config_network_bind_address)[0])
    {
        inet_pton (domain,
                   weechat_config_string (relay_config_network_bind_address),
                   ptr_addr);
    }

    /* create socket */
    server->sock = socket (domain, SOCK_STREAM, 0);
    if (server->sock < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot create socket"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        return 0;
    }

#ifdef IPV6_V6ONLY
    /* set option IPV6_V6ONLY to 0 or 1 */
    if (server->ipv6)
    {
        set = (server->ipv4) ? 0 : 1;
        if (setsockopt (server->sock, IPPROTO_IPV6, IPV6_V6ONLY,
                        (void *) &set, sizeof (set)) < 0)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot set socket option \"IPV6_V6ONLY\" "
                              "to value %d"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            set);
            close (server->sock);
            server->sock = -1;
            return 0;
        }
    }
#endif

    /* set option SO_REUSEADDR to 1 */
    set = 1;
    if (setsockopt (server->sock, SOL_SOCKET, SO_REUSEADDR,
                    (void *) &set, sizeof (set)) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot set socket option \"SO_REUSEADDR\""),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        close (server->sock);
        server->sock = -1;
        return 0;
    }

    /* set option SO_KEEPALIVE to 1 */
    set = 1;
    if (setsockopt (server->sock, SOL_SOCKET, SO_KEEPALIVE,
                    (void *) &set, sizeof (set)) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot set socket option \"SO_KEEPALIVE\""),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        close (server->sock);
        server->sock = -1;
        return 0;
    }

    /* bind */
    if (bind (server->sock, (struct sockaddr *)ptr_addr, addr_size) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: error with \"bind\" on port %d (%s)"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                        server->port, server->protocol_string);
        close (server->sock);
        server->sock = -1;
        return 0;
    }

    max_clients = weechat_config_integer (relay_config_network_max_clients);

    listen (server->sock, max_clients);

    weechat_printf (NULL,
                    _("%s: listening on port %d (relay: %s, %s, max %d clients)"),
                    RELAY_PLUGIN_NAME,
                    server->port,
                    server->protocol_string,
                    ((server->ipv4 && server->ipv6) ? "IPv4+6" : ((server->ipv6) ? "IPv6" : "IPv4")),
                    max_clients);

    server->hook_fd = weechat_hook_fd (server->sock,
                                       1, 0, 0,
                                       &relay_server_sock_cb,
                                       server);

    server->start_time = time (NULL);

    return 1;
}

/*
 * Adds a socket relaying on a port.
 *
 * Returns pointer to new server, NULL if error.
 */

struct t_relay_server *
relay_server_new (const char *protocol_string, enum t_relay_protocol protocol,
                  const char *protocol_args, int port, int ipv4, int ipv6,
                  int ssl)
{
    struct t_relay_server *new_server;

    if (!protocol_string)
        return NULL;

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
        new_server->protocol_string = strdup (protocol_string);
        new_server->protocol = protocol;
        new_server->protocol_args =
            (protocol_args) ? strdup (protocol_args) : NULL;
        new_server->port = port;
        new_server->ipv4 = ipv4;
        new_server->ipv6 = ipv6;
        new_server->ssl = ssl;
        new_server->sock = -1;
        new_server->hook_fd = NULL;
        new_server->start_time = 0;
        new_server->last_client_disconnect = 0;

        if (!relay_server_create_socket (new_server))
        {
            if (new_server->protocol_string)
                free (new_server->protocol_string);
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
 * Updates port in a server.
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
 * Removes a server.
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
    if (server->protocol_string)
        free (server->protocol_string);
    if (server->protocol_args)
        free (server->protocol_args);

    free (server);

    relay_servers = new_relay_servers;
}

/*
 * Removes all servers.
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
 * Adds a server in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_server_add_to_infolist (struct t_infolist *infolist,
                              struct t_relay_server *server)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !server)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_string (ptr_item, "protocol_string", server->protocol_string))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "protocol", server->protocol))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "protocol_args", server->protocol_args))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "port", server->port))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ipv4", server->ipv4))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ipv6", server->ipv6))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ssl", server->ssl))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "sock", server->sock))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "hook_fd", server->hook_fd))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "start_time", server->start_time))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "last_client_disconnect", server->last_client_disconnect))
        return 0;

    return 1;
}

/*
 * Prints servers in WeeChat log file (usually for crash dump).
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
        weechat_log_printf ("  protocol_string . . . : '%s'",  ptr_server->protocol_string);
        weechat_log_printf ("  protocol. . . . . . . : %d (%s)",
                            ptr_server->protocol,
                            relay_protocol_string[ptr_server->protocol]);
        weechat_log_printf ("  protocol_args . . . . : '%s'",  ptr_server->protocol_args);
        weechat_log_printf ("  port. . . . . . . . . : %d",    ptr_server->port);
        weechat_log_printf ("  ipv4. . . . . . . . . : %d",    ptr_server->ipv4);
        weechat_log_printf ("  ipv6. . . . . . . . . : %d",    ptr_server->ipv6);
        weechat_log_printf ("  ssl . . . . . . . . . : %d",    ptr_server->ssl);
        weechat_log_printf ("  sock. . . . . . . . . : %d",    ptr_server->sock);
        weechat_log_printf ("  hook_fd . . . . . . . : 0x%lx", ptr_server->hook_fd);
        weechat_log_printf ("  start_time. . . . . . : %ld",   ptr_server->start_time);
        weechat_log_printf ("  last_client_disconnect: %ld", ptr_server->last_client_disconnect);
        weechat_log_printf ("  prev_server . . . . . : 0x%lx", ptr_server->prev_server);
        weechat_log_printf ("  next_server . . . . . : 0x%lx", ptr_server->next_server);
    }
}
