/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2012 Simon Arlott
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

#ifndef WEECHAT_HOOK_CONNECT_H
#define WEECHAT_HOOK_CONNECT_H

#include <gnutls/gnutls.h>

struct t_weechat_plugin;
struct t_infolist_item;

#define HOOK_CONNECT(hook, var) (((struct t_hook_connect *)hook->hook_data)->var)

/* used if socketpair() function is NOT available */
#define HOOK_CONNECT_MAX_SOCKETS 4

typedef int (t_hook_callback_connect)(const void *pointer, void *data,
                                      int status, int gnutls_rc, int sock,
                                      const char *error,
                                      const char *ip_address);

typedef int (gnutls_callback_t)(const void *pointer, void *data,
                                gnutls_session_t tls_session,
                                const gnutls_datum_t *req_ca, int nreq,
                                const gnutls_pk_algorithm_t *pk_algos,
                                int pk_algos_len,
#if LIBGNUTLS_VERSION_NUMBER >= 0x020b00 /* 2.11.0 */
                                gnutls_retr2_st *answer,
#else
                                gnutls_retr_st *answer,
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x020b00 */
                                int action);

struct t_hook_connect
{
    t_hook_callback_connect *callback; /* connect callback                  */
    char *proxy;                       /* proxy (optional)                  */
    char *address;                     /* peer address                      */
    int port;                          /* peer port                         */
    int ipv6;                          /* use IPv6                          */
    int sock;                          /* socket (set when connected)       */
    int retry;                         /* retry count                       */
    gnutls_session_t *gnutls_sess;     /* GnuTLS session (TLS connection)   */
    gnutls_callback_t *gnutls_cb;      /* GnuTLS callback during handshake  */
    int gnutls_dhkey_size;             /* Diffie Hellman Key Exchange size  */
    char *gnutls_priorities;           /* GnuTLS priorities                 */
    char *local_hostname;              /* force local hostname (optional)   */
    int child_read;                    /* to read data in pipe from child   */
    int child_write;                   /* to write data in pipe for child   */
    int child_recv;                    /* to read data from child socket    */
    int child_send;                    /* to write data to child socket     */
    pid_t child_pid;                   /* pid of child process (connecting) */
    struct t_hook *hook_child_timer;   /* timer for child process timeout   */
    struct t_hook *hook_fd;            /* pointer to fd hook                */
    struct t_hook *handshake_hook_fd;  /* fd hook for handshake             */
    struct t_hook *handshake_hook_timer; /* timer for handshake timeout     */
    int handshake_fd_flags;            /* socket flags saved for handshake  */
    char *handshake_ip_address;        /* ip address (used for handshake)   */
    /* sockets used if socketpair() is NOT available */
    int sock_v4[HOOK_CONNECT_MAX_SOCKETS];  /* IPv4 sockets for connecting  */
    int sock_v6[HOOK_CONNECT_MAX_SOCKETS];  /* IPv6 sockets for connecting  */
};

extern char *hook_connect_get_description (struct t_hook *hook);
extern struct t_hook *hook_connect (struct t_weechat_plugin *plugin,
                                    const char *proxy, const char *address,
                                    int port, int ipv6, int retry,
                                    void *gnutls_session, void *gnutls_cb,
                                    int gnutls_dhkey_size,
                                    const char *gnutls_priorities,
                                    const char *local_hostname,
                                    t_hook_callback_connect *callback,
                                    const void *callback_pointer,
                                    void *callback_data);
extern int hook_connect_gnutls_verify_certificates (gnutls_session_t tls_session);
extern int hook_connect_gnutls_set_certificates (gnutls_session_t tls_session,
                                                 const gnutls_datum_t *req_ca, int nreq,
                                                 const gnutls_pk_algorithm_t *pk_algos,
                                                 int pk_algos_len,
#if LIBGNUTLS_VERSION_NUMBER >= 0x020b00 /* 2.11.0 */
                                                 gnutls_retr2_st *answer);
#else
                                                 gnutls_retr_st *answer);
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x020b00 */
extern void hook_connect_free_data (struct t_hook *hook);
extern int hook_connect_add_to_infolist (struct t_infolist_item *item,
                                         struct t_hook *hook);
extern void hook_connect_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_CONNECT_H */
