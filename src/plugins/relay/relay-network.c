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

/* relay-network.c: network functions for relay plugin */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-config.h"


int relay_network_sock = -1;           /* socket used for listening and     */
                                       /* waiting for clients               */
struct t_hook *relay_network_hook_fd = NULL;
int relay_network_listen_port = -1;    /* listening port                    */


/*
 * relay_network_close_socket: close socket
 */

void
relay_network_close_socket ()
{
    if (relay_network_hook_fd)
    {
        weechat_unhook (relay_network_hook_fd);
        relay_network_hook_fd = NULL;
    }
    if (relay_network_sock >= 0)
    {
        close (relay_network_sock);
        relay_network_sock = -1;
        weechat_printf (NULL,
                        _("%s: socket closed"),
                        RELAY_PLUGIN_NAME);
    }
}

/*
 * relay_network_sock_cb: read data from a client which is connecting on socket
 */

int
relay_network_sock_cb (void *data)
{
    struct sockaddr_in client_addr;
    unsigned int client_length;
    int client_fd;
    char ipv4_address[INET_ADDRSTRLEN + 1], *ptr_address;
    
    /* make C compiler happy */
    (void) data;
    
    client_length = sizeof (client_addr);
    memset (&client_addr, 0, client_length);
    
    client_fd = accept (relay_network_sock, (struct sockaddr *) &client_addr,
                        &client_length);
    if (client_fd < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot accept client"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
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
    
    relay_client_new (client_fd, ptr_address);
    
    return WEECHAT_RC_OK;
}

/*
 * relay_network_init: init socket and listen on port
 *                     return 1 if ok, 0 if error
 */

int
relay_network_init ()
{
    int set, args, port, port_start, port_end;
    struct sockaddr_in server_addr;
    const char *port_range;
    
    relay_network_close_socket ();
    
    if (!weechat_config_boolean (relay_config_network_enabled))
        return 1;
    
    port_range = weechat_config_string (relay_config_network_listen_port_range);
    if (!port_range || !port_range[0])
    {
        weechat_printf (NULL,
                        _("%s%s: option \"listen_port_range\" is not defined"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        return 0;
    }
    
    relay_network_sock = socket (AF_INET, SOCK_STREAM, 0);
    if (relay_network_sock < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot create socket"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        return 0;
    }
    
    set = 1;
    if (setsockopt (relay_network_sock, SOL_SOCKET, SO_REUSEADDR,
                    (void *) &set, sizeof (set)) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot set socket option "
                          "\"SO_REUSEADDR\""),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        close (relay_network_sock);
        relay_network_sock = -1;
        return 0;
    }
    
    set = 1;
    if (setsockopt (relay_network_sock, SOL_SOCKET, SO_KEEPALIVE,
                    (void *) &set, sizeof (set)) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot set socket option "
                          "\"SO_KEEPALIVE\""),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        close (relay_network_sock);
        relay_network_sock = -1;
        return 0;
    }
    
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    port = -1;
    
    /* find a free port in the specified range */
    args = sscanf (weechat_config_string (relay_config_network_listen_port_range),
                   "%d-%d", &port_start, &port_end);
    if (args > 0)
    {
        port = port_start;
        if (args == 1)
            port_end = port_start;
        
        /* loop through the entire allowed port range */
        while (port <= port_end)
        {
            /* attempt to bind to the free port */
            server_addr.sin_port = htons (port);
            if (bind (relay_network_sock, (struct sockaddr *) &server_addr,
                      sizeof (server_addr)) == 0)
                break;
            port++;
        }
        
        if (port > port_end)
            port = -1;
    }
    
    if (port < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot find available port for listening"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        close (relay_network_sock);
        relay_network_sock = -1;
        return 0;
    }
    
    relay_network_listen_port = port;
    
    listen (relay_network_sock, 5);
    
    weechat_printf (NULL,
                    _("%s: listening on port %d"),
                    RELAY_PLUGIN_NAME, relay_network_listen_port);
    
    relay_network_hook_fd = weechat_hook_fd (relay_network_sock,
                                             1, 0, 0,
                                             &relay_network_sock_cb,
                                             NULL);
    
    return 1;
}

/*
 * relay_network_end: close main socket
 */

void
relay_network_end ()
{
    relay_network_close_socket ();
}
