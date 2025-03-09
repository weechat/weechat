/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_RELAY_SERVER_H
#define WEECHAT_PLUGIN_RELAY_SERVER_H

#include <time.h>

#define RELAY_SERVER_GNUTLS_DH_BITS 1024

struct t_relay_server
{
    char *protocol_string;             /* example: "ipv6.tls.irc.libera"    */
    enum t_relay_protocol protocol;    /* protocol (irc/weechat)            */
    char *protocol_args;               /* arguments used for protocol       */
                                       /* example: server for irc protocol  */
    int port;                          /* listening on this port            */
                                       /* or UNIX socket, if negative.      */
    char *path;                        /* listening on this path (UNIX),    */
                                       /* contains string representation of */
                                       /* port if IP                        */
    int ipv4;                          /* IPv4 protocol enabled             */
    int ipv6;                          /* IPv6 protocol enabled             */
    int tls;                           /* 1 if TLS is enabled               */
    int unix_socket;                   /* 1 if UNIX socket                  */
    int sock;                          /* socket for connection             */
    struct t_hook *hook_fd;            /* hook for socket                   */
    time_t start_time;                 /* start time                        */
    time_t last_client_disconnect;     /* last time a client disconnected   */
    struct t_relay_server *prev_server;/* link to previous server           */
    struct t_relay_server *next_server;/* link to next server               */
};

extern struct t_relay_server *relay_servers;
extern struct t_relay_server *last_relay_server;

extern void relay_server_get_protocol_args (const char *protocol_and_string,
                                            int *ipv4, int *ipv6,
                                            int *tls, int *unix_socket,
                                            char **protocol,
                                            char **protocol_args);
extern struct t_relay_server *relay_server_search (const char *protocol_and_args);
extern struct t_relay_server *relay_server_search_port (int port);
extern struct t_relay_server *relay_server_search_path (const char *path);
extern void relay_server_close_socket (struct t_relay_server *server);
extern int relay_server_create_socket (struct t_relay_server *server);
extern struct t_relay_server *relay_server_new (const char *protocol_string,
                                                enum t_relay_protocol protocol,
                                                const char *protocol_args,
                                                int port, const char *path,
                                                int ipv4, int ipv6,
                                                int tls, int unix_socket);
extern void relay_server_update_path (struct t_relay_server *server,
                                      const char *path);
extern void relay_server_update_port (struct t_relay_server *server, int port);
extern void relay_server_free (struct t_relay_server *server);
extern void relay_server_free_all (void);
extern int relay_server_add_to_infolist (struct t_infolist *infolist,
                                         struct t_relay_server *server);
extern void relay_server_print_log (void);

#endif /* WEECHAT_PLUGIN_RELAY_SERVER_H */
