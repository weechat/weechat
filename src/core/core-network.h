/*
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

#ifndef WEECHAT_NETWORK_H
#define WEECHAT_NETWORK_H

#include <sys/types.h>
#include <sys/socket.h>

struct t_hook;

struct t_network_socks4
{
    char version;         /* 1 byte : socks version: 4 or 5                 */
    char method;          /* 1 byte : socks method: connect (1) or bind (2) */
    unsigned short port;  /* 2 bytes: destination port                      */
    unsigned int address; /* 4 bytes: destination address                   */
    char user[128];       /* username                                       */
};

struct t_network_socks5
{
    char version;         /* 1 byte: socks version : 4 or 5                 */
    char nmethods;        /* 1 byte: size in byte(s) of field 'method',     */
                          /*              here 1 byte                       */
    char method;          /* 1 byte: socks method : noauth (0),             */
                          /*              auth(user/pass) (2), ...          */
};

extern int network_init_gnutls_ok;
extern int network_num_certs_system;
extern int network_num_certs_user;
extern int network_num_certs;

extern void network_init_gcrypt ();
extern void network_load_ca_files (int force_display);
extern void network_reload_ca_files (int force_display);
extern void network_init_gnutls ();
extern void network_end ();
extern int network_pass_proxy (const char *proxy, int sock,
                               const char *address, int port);
extern int network_connect_to (const char *proxy, struct sockaddr *address,
                               socklen_t address_length);
extern void network_connect_with_fork (struct t_hook *hook_connect);

#endif /* WEECHAT_NETWORK_H */
