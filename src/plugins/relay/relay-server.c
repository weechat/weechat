/*
 * relay-server.c - server functions for relay plugin
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

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
 *   string                    ipv4 ipv6 tls unix protocol protocol_args
 *   ---------------------------------------------------------------
 *   irc.libera                1    1    0   0    irc      libera
 *   tls.irc.libera            1    1    1   0    irc      libera
 *   ipv4.irc.libera           1    0    0   0    irc      libera
 *   ipv6.irc.libera           0    1    0   0    irc      libera
 *   ipv4.ipv6.irc.libera      1    1    0   0    irc      libera
 *   ipv6.tls.irc.libera       0    1    1   0    irc      libera
 *   weechat                   1    1    0   0    weechat
 *   tls.weechat               1    1    1   0    weechat
 *   ipv6.tls.weechat          0    1    1   0    weechat
 *   unix.weechat              0    0    0   1    weechat
 *
 * Note: *protocol and *protocol_args must be freed after use.
 */

void
relay_server_get_protocol_args (const char *protocol_and_args,
                                int *ipv4, int *ipv6, int *tls,
                                int *unix_socket,
                                char **protocol, char **protocol_args)
{
    int opt_ipv4, opt_ipv6, opt_tls, opt_unix_socket;
    char *pos;

    opt_ipv4 = -1;
    opt_ipv6 = -1;
    opt_tls = 0;
    opt_unix_socket = -1;
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
        else if (strncmp (protocol_and_args, "tls.", 4) == 0)
        {
            opt_tls = 1;
            protocol_and_args += 4;
        }
        else if (strncmp (protocol_and_args, "unix.", 5) == 0)
        {
            opt_unix_socket = 1;
            protocol_and_args += 5;
        }
        else
            break;
    }
    if ((opt_ipv4 == -1) && (opt_ipv6 == -1) && (opt_unix_socket == -1))
    {
        /*
         * no IPv4/IPv6/UNIX specified, then use defaults:
         * - IPv4 enabled
         * - IPv6 enabled if option relay.network.ipv6 is on
         */
        opt_ipv4 = 1;
        opt_ipv6 = weechat_config_boolean (relay_config_network_ipv6);
        opt_unix_socket = 0;
    }
    else
    {
        if (opt_ipv4 == -1)
            opt_ipv4 = 0;
        if (opt_ipv6 == -1)
            opt_ipv6 = 0;
        if (opt_unix_socket == -1)
            opt_unix_socket = 0;
    }
    if (!opt_ipv4 && !opt_ipv6 && !opt_unix_socket)
    {
        /* IPv4/IPv6/UNIX disabled (should never occur!) */
        opt_ipv4 = 1;
    }
    if (ipv4)
        *ipv4 = opt_ipv4;
    if (ipv6)
        *ipv6 = opt_ipv6;
    if (tls)
        *tls = opt_tls;
    if (unix_socket)
        *unix_socket = opt_unix_socket;

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

    if (!protocol_and_args)
        return NULL;

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
 * Searches for a server by path. Only returns servers using a UNIX socket.
 *
 * Returns pointer to new server, NULL if not found.
 */

struct t_relay_server *
relay_server_search_path (const char *path)
{
    struct t_relay_server *ptr_server;

    if (!path)
        return NULL;

    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /* only include UNIX socket relays, to allow for numerical paths */
        if (ptr_server->unix_socket && (strcmp (path, ptr_server->path) == 0))
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
        if (server->unix_socket)
            unlink (server->path);
        if (!relay_signal_upgrade_received)
        {
            weechat_printf (NULL,
                            _("%s: socket closed for %s (%s: %s)"),
                            RELAY_PLUGIN_NAME,
                            server->protocol_string,
                            (server->unix_socket) ? _("path") : _("port"),
                            server->path);
        }
    }
}

/*
 * Reads data from a client which is connecting on socket.
 */

int
relay_server_sock_cb (const void *pointer, void *data, int fd)
{
    struct t_relay_server *server;
    struct sockaddr_in client_addr;
    struct sockaddr_in6 client_addr6;
    struct sockaddr_un client_addr_unix;
    socklen_t client_addr_size;
    void *ptr_addr;
    int client_fd, flags, set, max_clients, num_clients_on_port;
    char ipv4_address[INET_ADDRSTRLEN + 1], ipv6_address[INET6_ADDRSTRLEN + 1];
    char unix_address[sizeof (client_addr_unix.sun_path)];
    char *ptr_ip_address, *relay_password, *relay_totp_secret;

    /* make C compiler happy */
    (void) data;
    (void) fd;

    relay_password = NULL;
    relay_totp_secret = NULL;

    server = (struct t_relay_server *)pointer;

    if (server->ipv6)
    {
        ptr_addr = &client_addr6;
        client_addr_size = sizeof (struct sockaddr_in6);
    }
    else if (server->ipv4)
    {
        ptr_addr = &client_addr;
        client_addr_size = sizeof (struct sockaddr_in);
    }
    else
    {
        ptr_addr = &client_addr_unix;
        client_addr_size = sizeof (struct sockaddr_un);
    }

    memset (ptr_addr, 0, client_addr_size);

    client_fd = accept (server->sock, (struct sockaddr *)ptr_addr,
                        &client_addr_size);
    if (client_fd < 0)
    {
        if (server->unix_socket)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot accept client on path %s (%s): "
                              "error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            server->path, server->protocol_string,
                            errno, strerror (errno));
        }
        else
        {
            weechat_printf (NULL,
                            _("%s%s: cannot accept client on port %d (%s): "
                              "error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            server->port, server->protocol_string,
                            errno, strerror (errno));
        }
        goto error;
    }

    /* check if relay password is empty and if it is not allowed */
    relay_password = weechat_string_eval_expression (
        weechat_config_string (relay_config_network_password),
        NULL, NULL, NULL);
    if (!weechat_config_boolean (relay_config_network_allow_empty_password)
        && (!relay_password || !relay_password[0]))
    {
        weechat_printf (NULL,
                        _("%s%s: cannot accept client because relay password "
                          "is empty, and option "
                          "relay.network.allow_empty_password is off"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        goto error;
    }

    if (server->protocol == RELAY_PROTOCOL_WEECHAT)
    {
        /*
         * TOTP can be enabled only as second factor, in addition to the
         * password (only for weechat protocol)
         */
        relay_totp_secret = weechat_string_eval_expression (
            weechat_config_string (relay_config_network_totp_secret),
            NULL, NULL, NULL);
        if ((!relay_password || !relay_password[0])
            && relay_totp_secret && relay_totp_secret[0])
        {
            weechat_printf (NULL,
                            _("%s%s: Time-based One-Time Password (TOTP) "
                              "can be enabled only as second factor, if the "
                              "password is not empty"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME);
            goto error;
        }
        if (!relay_config_check_network_totp_secret (
                NULL, NULL, NULL,
                weechat_config_string (relay_config_network_totp_secret)))
        {
            goto error;
        }
    }

    /* check if we have reached the max number of clients on this port */
    max_clients = weechat_config_integer (relay_config_network_max_clients);
    if (max_clients > 0)
    {
        num_clients_on_port = relay_client_count_active_by_port (server->port);
        if (num_clients_on_port >= max_clients)
        {
            weechat_printf (
                NULL,
                NG_("%s%s: client not allowed (max %d client is "
                    "allowed at same time)",
                    "%s%s: client not allowed (max %d clients are "
                    "allowed at same time)",
                    max_clients),
                weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                max_clients);
            goto error;
        }
    }

    /* get the IP address */
    ptr_ip_address = NULL;
    if (server->ipv6)
    {
        if (inet_ntop (AF_INET6,
                       &(client_addr6.sin6_addr),
                       ipv6_address,
                       INET6_ADDRSTRLEN))
        {
            ptr_ip_address = ipv6_address;

            if (strncmp (ptr_ip_address, "::ffff:", 7) == 0)
            {
                /* actually an IPv4-mapped IPv6 address, so skip "::ffff:" */
                ptr_ip_address += 7;
            }
        }
    }
    else if (server->ipv4)
    {
        if (inet_ntop (AF_INET,
                       &(client_addr.sin_addr),
                       ipv4_address,
                       INET_ADDRSTRLEN))
        {
            ptr_ip_address = ipv4_address;
        }
    }
    else
    {
        snprintf (unix_address, sizeof (unix_address),
                  "%s",
                  client_addr_unix.sun_path);
        ptr_ip_address = unix_address;
    }

    /* check if IP is allowed, if not, just close socket */
    if (relay_config_regex_allowed_ips
        && (regexec (relay_config_regex_allowed_ips, ptr_ip_address, 0, NULL, 0) != 0))
    {
        if (weechat_relay_plugin->debug >= 1)
        {
            weechat_printf (NULL,
                            _("%s%s: IP address \"%s\" not allowed for relay"),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME,
                            ptr_ip_address);
        }
        goto error;
    }

    /* set non-blocking mode for socket */
    flags = fcntl (client_fd, F_GETFL);
    if (flags == -1)
        flags = 0;
    fcntl (client_fd, F_SETFL, flags | O_NONBLOCK);

    /* set socket option SO_REUSEADDR (only for TCP socket) */
    if (!server->unix_socket)
    {
        set = 1;
        if (setsockopt (client_fd, SOL_SOCKET, SO_REUSEADDR,
                        (void *) &set, sizeof (set)) < 0)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot set socket option \"%s\" to %d: "
                              "error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            "SO_REUSEADDR", set, errno, strerror (errno));
            goto error;
        }
    }

    /* add the client */
    relay_client_new (client_fd, ptr_ip_address, server);

    goto end;

error:
    if (client_fd >= 0)
        close (client_fd);

end:
    if (relay_password)
        free (relay_password);
    if (relay_totp_secret)
        free (relay_totp_secret);

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
    int domain, set, max_clients, addr_size, rc;
    struct sockaddr_in server_addr;
    struct sockaddr_in6 server_addr6;
    struct sockaddr_un server_addr_unix;
    const char *bind_address;
    void *ptr_addr;

    bind_address = weechat_config_string (relay_config_network_bind_address);

    if (server->ipv6)
    {
        domain = AF_INET6;
        memset (&server_addr6, 0, sizeof (struct sockaddr_in6));
        server_addr6.sin6_family = domain;
        server_addr6.sin6_port = htons (server->port);
        server_addr6.sin6_addr = in6addr_any;
        if (bind_address && bind_address[0])
        {
            if (!inet_pton (domain, bind_address, &server_addr6.sin6_addr))
            {
                weechat_printf (NULL,
                                /* TRANSLATORS: second "%s" is "IPv4" or "IPv6" */
                                _("%s%s: invalid bind address \"%s\" for %s"),
                                weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                                bind_address, "IPv6");
                return 0;
            }
        }
        ptr_addr = &server_addr6;
        addr_size = sizeof (struct sockaddr_in6);
    }
    else if (server->ipv4)
    {
        domain = AF_INET;
        memset (&server_addr, 0, sizeof (struct sockaddr_in));
        server_addr.sin_family = domain;
        server_addr.sin_port = htons (server->port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        if (bind_address && bind_address[0])
        {
            if (!inet_pton (domain, bind_address, &server_addr.sin_addr))
            {
                weechat_printf (NULL,
                                /* TRANSLATORS: second "%s" is "IPv4" or "IPv6" */
                                _("%s%s: invalid bind address \"%s\" for %s"),
                                weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                                bind_address, "IPv4");
                return 0;
            }
        }
        ptr_addr = &server_addr;
        addr_size = sizeof (struct sockaddr_in);
    }
    else
    {
        domain = AF_UNIX;
        memset (&server_addr_unix, 0, sizeof (struct sockaddr_un));
        server_addr_unix.sun_family = domain;
        snprintf (server_addr_unix.sun_path,
                  sizeof (server_addr_unix.sun_path),
                  "%s",
                  server->path);
        ptr_addr = &server_addr_unix;
        addr_size = sizeof (struct sockaddr_un);
        if (!relay_config_check_path_length (server->path))
        {
            weechat_printf (NULL,
                            _("%s%s: socket path \"%s\" is invalid"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            server->path);
            return 0;
        }
        rc = relay_config_check_path_available (server->path);
        switch (rc)
        {
            case -1:
                weechat_printf (NULL,
                                _("%s%s: socket path \"%s\" already exists "
                                  "and is not a socket"),
                                weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                                server->path);
                break;
            case -2:
                weechat_printf (NULL,
                                _("%s%s: socket path \"%s\" is invalid"),
                                weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                                server->path);
        }
        if (rc < 0)
            return 0;
        /* just in case a socket already exists */
        unlink (server->path);
    }


    /* create socket */
    server->sock = socket (domain, SOCK_STREAM, 0);
    if (server->sock < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot create socket: error %d %s"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                        errno, strerror (errno));
        if (errno == EAFNOSUPPORT)
        {
            weechat_printf (NULL,
                            _("%s%s: try /set relay.network.ipv6 off"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        }
        return 0;
    }

#if defined(IPV6_V6ONLY) && !defined(__OpenBSD__)
    /* set option IPV6_V6ONLY to 0 or 1 */
    if (server->ipv6)
    {
        set = (server->ipv4) ? 0 : 1;
        if (setsockopt (server->sock, IPPROTO_IPV6, IPV6_V6ONLY,
                        (void *) &set, sizeof (set)) < 0)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot set socket option \"%s\" "
                              "to %d: error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            "IPV6_V6ONLY", set, errno, strerror (errno));
            close (server->sock);
            server->sock = -1;
            return 0;
        }
    }
#endif

    /* set option SO_REUSEADDR to 1 (only for TCP socket) */
    if (!server->unix_socket)
    {
        set = 1;
        if (setsockopt (server->sock, SOL_SOCKET, SO_REUSEADDR,
                        (void *) &set, sizeof (set)) < 0)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot set socket option \"%s\" to %d: "
                              "error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            "SO_REUSEADDR", set, errno, strerror (errno));
            close (server->sock);
            server->sock = -1;
            return 0;
        }
    }

    /* set option SO_KEEPALIVE to 1 (only for TCP socket) */
    if (!server->unix_socket)
    {
        set = 1;
        if (setsockopt (server->sock, SOL_SOCKET, SO_KEEPALIVE,
                        (void *) &set, sizeof (set)) < 0)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot set socket option \"%s\" to %d: "
                              "error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            "SO_KEEPALIVE", set, errno, strerror (errno));
            close (server->sock);
            server->sock = -1;
            return 0;
        }
    }

    /* bind */
    if (bind (server->sock, (struct sockaddr *)ptr_addr, addr_size) < 0)
    {
        if (server->unix_socket)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot \"bind\" on path %s (%s): "
                              "error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            server->path, server->protocol_string,
                            errno, strerror (errno));
        }
        else
        {
            weechat_printf (NULL,
                            _("%s%s: cannot \"bind\" on port %d (%s): "
                              "error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            server->port, server->protocol_string,
                            errno, strerror (errno));
        }
        close (server->sock);
        server->sock = -1;
        return 0;
    }

    /* change permissions: only the owner can use the unix socket */
    if (server->unix_socket)
        chmod (server->path, 0700);

#ifdef SOMAXCONN
    if (listen (server->sock, SOMAXCONN) != 0)
#else
    if (listen (server->sock, 1) != 0)
#endif
    {
        if (server->unix_socket)
        {
            weechat_printf (NULL,
                            _("%s%s: cannot \"listen\" on path %s (%s): "
                              "error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            server->path, server->protocol_string,
                            errno, strerror (errno));
        }
        else
        {
            weechat_printf (NULL,
                            _("%s%s: cannot \"listen\" on port %d (%s): "
                              "error %d %s"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            server->port, server->protocol_string,
                            errno, strerror (errno));
        }
        close (server->sock);
        server->sock = -1;
        return 0;
    }

    max_clients = weechat_config_integer (relay_config_network_max_clients);
    if (max_clients > 0)
    {
        if (server->unix_socket)
        {
            weechat_printf (
                NULL,
                NG_("%s: listening on path %s (relay: %s, %s, max %d client)",
                    "%s: listening on path %s (relay: %s, %s, max %d clients)",
                    max_clients),
                RELAY_PLUGIN_NAME,
                server->path,
                server->protocol_string,
                ((server->ipv4 && server->ipv6) ? "IPv4+6" : ((server->ipv6) ? "IPv6" : ((server->ipv4) ? "IPv4" : "UNIX"))),
                max_clients);
        }
        else
        {
            weechat_printf (
                NULL,
                NG_("%s: listening on port %d (relay: %s, %s, max %d client)",
                    "%s: listening on port %d (relay: %s, %s, max %d clients)",
                    max_clients),
                RELAY_PLUGIN_NAME,
                server->port,
                server->protocol_string,
                ((server->ipv4 && server->ipv6) ? "IPv4+6" : ((server->ipv6) ? "IPv6" : ((server->ipv4) ? "IPv4" : "UNIX"))),
                max_clients);
        }
    }
    else
    {
        if (server->unix_socket)
        {
            weechat_printf (
                NULL,
                _("%s: listening on path %s (relay: %s, %s)"),
                RELAY_PLUGIN_NAME,
                server->path,
                server->protocol_string,
                ((server->ipv4 && server->ipv6) ? "IPv4+6" : ((server->ipv6) ? "IPv6" : ((server->ipv4) ? "IPv4" : "UNIX"))));
        }
        else
        {
            weechat_printf (
                NULL,
                _("%s: listening on port %d (relay: %s, %s)"),
                RELAY_PLUGIN_NAME,
                server->port,
                server->protocol_string,
                ((server->ipv4 && server->ipv6) ? "IPv4+6" : ((server->ipv6) ? "IPv6" : ((server->ipv4) ? "IPv4" : "UNIX"))));
        }
    }
    server->hook_fd = weechat_hook_fd (server->sock,
                                       1, 0, 0,
                                       &relay_server_sock_cb,
                                       server, NULL);

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
                  const char *protocol_args, int port, const char *path,
                  int ipv4, int ipv6, int tls, int unix_socket)
{
    struct t_relay_server *new_server, *dup_server;
    struct t_hashtable *options;

    if (!protocol_string)
        return NULL;

    /* look for duplicate ports/paths */
    dup_server = (unix_socket) ?
        relay_server_search_path (path) : relay_server_search_port (port);
    if (dup_server)
    {
        if (unix_socket)
        {
            weechat_printf (NULL, _("%s%s: error: path \"%s\" is already used"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            path);
        }
        else
        {
            weechat_printf (NULL, _("%s%s: error: port \"%d\" is already used"),
                            weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                            port);
        }
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
        options = weechat_hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL, NULL);
        if (options)
            weechat_hashtable_set (options, "directory", "runtime");
        new_server->path = weechat_string_eval_path_home (path,
                                                          NULL, NULL, options);
        if (options)
            weechat_hashtable_free (options);
        new_server->ipv4 = ipv4;
        new_server->ipv6 = ipv6;
        new_server->tls = tls;
        new_server->unix_socket = unix_socket;
        new_server->sock = -1;
        new_server->hook_fd = NULL;
        new_server->start_time = 0;
        new_server->last_client_disconnect = 0;

        relay_server_create_socket (new_server);

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
 * Updates path in a server.
 */

void
relay_server_update_path (struct t_relay_server *server, const char *path)
{
    char *new_path;
    struct t_hashtable *options;

    options = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (options)
        weechat_hashtable_set (options, "directory", "runtime");
    new_path = weechat_string_eval_path_home (path, NULL, NULL, options);
    if (options)
        weechat_hashtable_free (options);
    if (!new_path)
        return;

    if (strcmp (new_path, server->path) != 0)
    {
        relay_server_close_socket (server);
        free (server->path);
        server->path = strdup (new_path);
        server->port = -1;
        relay_server_create_socket (server);
    }

    free (new_path);
}

/*
 * Updates port in a server.
 */

void
relay_server_update_port (struct t_relay_server *server, int port)
{
    char str_path[128];

    if (port != server->port)
    {
        relay_server_close_socket (server);
        server->port = port;
        snprintf (str_path, sizeof (str_path), "%d", port);
        free (server->path);
        server->path = strdup (str_path);
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
    free (server->path);

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
    if (!weechat_infolist_new_var_string (ptr_item, "path", server->path))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ipv4", server->ipv4))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ipv6", server->ipv6))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "tls", server->tls))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "unix_socket", server->unix_socket))
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
        weechat_log_printf ("  path. . . . . . . . . : %s",    ptr_server->path);
        weechat_log_printf ("  ipv4. . . . . . . . . : %d",    ptr_server->ipv4);
        weechat_log_printf ("  ipv6. . . . . . . . . : %d",    ptr_server->ipv6);
        weechat_log_printf ("  tls . . . . . . . . . : %d",    ptr_server->tls);
        weechat_log_printf ("  unix_socket . . . . . : %d",    ptr_server->unix_socket);
        weechat_log_printf ("  sock. . . . . . . . . : %d",    ptr_server->sock);
        weechat_log_printf ("  hook_fd . . . . . . . : 0x%lx", ptr_server->hook_fd);
        weechat_log_printf ("  start_time. . . . . . : %lld",  (long long)ptr_server->start_time);
        weechat_log_printf ("  last_client_disconnect: %lld",  (long long)ptr_server->last_client_disconnect);
        weechat_log_printf ("  prev_server . . . . . : 0x%lx", ptr_server->prev_server);
        weechat_log_printf ("  next_server . . . . . : 0x%lx", ptr_server->next_server);
    }
}
