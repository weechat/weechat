/*
 * hook-connect.c - WeeChat connect hook
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../weechat.h"
#include "../core-hook.h"
#include "../core-infolist.h"
#include "../core-log.h"
#include "../core-network.h"
#include "../../plugins/plugin.h"


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_connect_get_description (struct t_hook *hook)
{
    char str_desc[1024];

    snprintf (str_desc, sizeof (str_desc),
              "socket: %d, address: %s, port: %d, child pid: %d",
              HOOK_CONNECT(hook, sock),
              HOOK_CONNECT(hook, address),
              HOOK_CONNECT(hook, port),
              HOOK_CONNECT(hook, child_pid));

    return strdup (str_desc);
}

/*
 * Hooks a connection to a peer (using fork).
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_connect (struct t_weechat_plugin *plugin, const char *proxy,
              const char *address, int port, int ipv6, int retry,
              void *gnutls_sess, void *gnutls_cb, int gnutls_dhkey_size,
              const char *gnutls_priorities, const char *local_hostname,
              t_hook_callback_connect *callback,
              const void *callback_pointer,
              void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_connect *new_hook_connect;
    int i;

    if (!address || (port <= 0) || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_connect = malloc (sizeof (*new_hook_connect));
    if (!new_hook_connect)
    {
        free (new_hook);
        return NULL;
    }

    hook_init_data (new_hook, plugin, HOOK_TYPE_CONNECT, HOOK_PRIORITY_DEFAULT,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_connect;
    new_hook_connect->callback = callback;
    new_hook_connect->proxy = (proxy) ? strdup (proxy) : NULL;
    new_hook_connect->address = strdup (address);
    new_hook_connect->port = port;
    new_hook_connect->sock = -1;
    new_hook_connect->ipv6 = ipv6;
    new_hook_connect->retry = retry;
    new_hook_connect->gnutls_sess = gnutls_sess;
    new_hook_connect->gnutls_cb = gnutls_cb;
    new_hook_connect->gnutls_dhkey_size = gnutls_dhkey_size;
    new_hook_connect->gnutls_priorities = (gnutls_priorities) ?
        strdup (gnutls_priorities) : NULL;
    new_hook_connect->local_hostname = (local_hostname) ?
        strdup (local_hostname) : NULL;
    new_hook_connect->child_read = -1;
    new_hook_connect->child_write = -1;
    new_hook_connect->child_recv = -1;
    new_hook_connect->child_send = -1;
    new_hook_connect->child_pid = 0;
    new_hook_connect->hook_child_timer = NULL;
    new_hook_connect->hook_fd = NULL;
    new_hook_connect->handshake_hook_fd = NULL;
    new_hook_connect->handshake_hook_timer = NULL;
    new_hook_connect->handshake_fd_flags = 0;
    new_hook_connect->handshake_ip_address = NULL;
    if (!hook_socketpair_ok)
    {
        for (i = 0; i < HOOK_CONNECT_MAX_SOCKETS; i++)
        {
            new_hook_connect->sock_v4[i] = -1;
            new_hook_connect->sock_v6[i] = -1;
        }
    }

    hook_add_to_list (new_hook);

    network_connect_with_fork (new_hook);

    return new_hook;
}

/*
 * Verifies certificates.
 */

int
hook_connect_gnutls_verify_certificates (gnutls_session_t tls_session)
{
    struct t_hook *ptr_hook;
    int rc;

    rc = -1;
    ptr_hook = weechat_hooks[HOOK_TYPE_CONNECT];
    while (ptr_hook)
    {
        /* looking for the right hook using to the gnutls session pointer */
        if (!ptr_hook->deleted
            && HOOK_CONNECT(ptr_hook, gnutls_sess)
            && (*(HOOK_CONNECT(ptr_hook, gnutls_sess)) == tls_session))
        {
            rc = (int) (HOOK_CONNECT(ptr_hook, gnutls_cb))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 tls_session, NULL, 0,
                 NULL, 0, NULL,
                 WEECHAT_HOOK_CONNECT_GNUTLS_CB_VERIFY_CERT);
            break;
        }
        ptr_hook = ptr_hook->next_hook;
    }

    return rc;
}

/*
 * Sets certificates.
 */

int
hook_connect_gnutls_set_certificates (gnutls_session_t tls_session,
                                      const gnutls_datum_t *req_ca, int nreq,
                                      const gnutls_pk_algorithm_t *pk_algos,
                                      int pk_algos_len,
                                      gnutls_retr2_st *answer)
{
    struct t_hook *ptr_hook;
    int rc;

    rc = -1;
    ptr_hook = weechat_hooks[HOOK_TYPE_CONNECT];
    while (ptr_hook)
    {
        /* looking for the right hook using to the gnutls session pointer */
        if (!ptr_hook->deleted
            && HOOK_CONNECT(ptr_hook, gnutls_sess)
            && (*(HOOK_CONNECT(ptr_hook, gnutls_sess)) == tls_session))
        {
            rc = (int) (HOOK_CONNECT(ptr_hook, gnutls_cb))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 tls_session, req_ca, nreq,
                 pk_algos, pk_algos_len, answer,
                 WEECHAT_HOOK_CONNECT_GNUTLS_CB_SET_CERT);
            break;
        }
        ptr_hook = ptr_hook->next_hook;
    }

    return rc;
}

/*
 * Frees data in a connect hook.
 */

void
hook_connect_free_data (struct t_hook *hook)
{
    int i;

    if (!hook || !hook->hook_data)
        return;

    if (HOOK_CONNECT(hook, proxy))
    {
        free (HOOK_CONNECT(hook, proxy));
        HOOK_CONNECT(hook, proxy) = NULL;
    }
    if (HOOK_CONNECT(hook, address))
    {
        free (HOOK_CONNECT(hook, address));
        HOOK_CONNECT(hook, address) = NULL;
    }
    if (HOOK_CONNECT(hook, gnutls_priorities))
    {
        free (HOOK_CONNECT(hook, gnutls_priorities));
        HOOK_CONNECT(hook, gnutls_priorities) = NULL;
    }
    if (HOOK_CONNECT(hook, local_hostname))
    {
        free (HOOK_CONNECT(hook, local_hostname));
        HOOK_CONNECT(hook, local_hostname) = NULL;
    }
    if (HOOK_CONNECT(hook, hook_child_timer))
    {
        unhook (HOOK_CONNECT(hook, hook_child_timer));
        HOOK_CONNECT(hook, hook_child_timer) = NULL;
    }
    if (HOOK_CONNECT(hook, hook_fd))
    {
        unhook (HOOK_CONNECT(hook, hook_fd));
        HOOK_CONNECT(hook, hook_fd) = NULL;
    }
    if (HOOK_CONNECT(hook, handshake_hook_fd))
    {
        unhook (HOOK_CONNECT(hook, handshake_hook_fd));
        HOOK_CONNECT(hook, handshake_hook_fd) = NULL;
    }
    if (HOOK_CONNECT(hook, handshake_hook_timer))
    {
        unhook (HOOK_CONNECT(hook, handshake_hook_timer));
        HOOK_CONNECT(hook, handshake_hook_timer) = NULL;
    }
    if (HOOK_CONNECT(hook, handshake_ip_address))
    {
        free (HOOK_CONNECT(hook, handshake_ip_address));
        HOOK_CONNECT(hook, handshake_ip_address) = NULL;
    }
    if (HOOK_CONNECT(hook, child_pid) > 0)
    {
        kill (HOOK_CONNECT(hook, child_pid), SIGKILL);
        hook_schedule_clean_process (HOOK_CONNECT(hook, child_pid));
        HOOK_CONNECT(hook, child_pid) = 0;
    }
    if (HOOK_CONNECT(hook, child_read) != -1)
    {
        close (HOOK_CONNECT(hook, child_read));
        HOOK_CONNECT(hook, child_read) = -1;
    }
    if (HOOK_CONNECT(hook, child_write) != -1)
    {
        close (HOOK_CONNECT(hook, child_write));
        HOOK_CONNECT(hook, child_write) = -1;
    }
    if (HOOK_CONNECT(hook, child_recv) != -1)
    {
        close (HOOK_CONNECT(hook, child_recv));
        HOOK_CONNECT(hook, child_recv) = -1;
    }
    if (HOOK_CONNECT(hook, child_send) != -1)
    {
        close (HOOK_CONNECT(hook, child_send));
        HOOK_CONNECT(hook, child_send) = -1;
    }
    if (!hook_socketpair_ok)
    {
        for (i = 0; i < HOOK_CONNECT_MAX_SOCKETS; i++)
        {
            if (HOOK_CONNECT(hook, sock_v4[i]) != -1)
            {
                close (HOOK_CONNECT(hook, sock_v4[i]));
                HOOK_CONNECT(hook, sock_v4[i]) = -1;
            }
            if (HOOK_CONNECT(hook, sock_v6[i]) != -1)
            {
                close (HOOK_CONNECT(hook, sock_v6[i]));
                HOOK_CONNECT(hook, sock_v6[i]) = -1;
            }
        }
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds connect hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_connect_add_to_infolist (struct t_infolist_item *item,
                              struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_CONNECT(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "address", HOOK_CONNECT(hook, address)))
        return 0;
    if (!infolist_new_var_integer (item, "port", HOOK_CONNECT(hook, port)))
        return 0;
    if (!infolist_new_var_integer (item, "sock", HOOK_CONNECT(hook, sock)))
        return 0;
    if (!infolist_new_var_integer (item, "ipv6", HOOK_CONNECT(hook, ipv6)))
        return 0;
    if (!infolist_new_var_integer (item, "retry", HOOK_CONNECT(hook, retry)))
        return 0;
    if (!infolist_new_var_pointer (item, "gnutls_sess", HOOK_CONNECT(hook, gnutls_sess)))
        return 0;
    if (!infolist_new_var_pointer (item, "gnutls_cb", HOOK_CONNECT(hook, gnutls_cb)))
        return 0;
    if (!infolist_new_var_integer (item, "gnutls_dhkey_size", HOOK_CONNECT(hook, gnutls_dhkey_size)))
        return 0;
    if (!infolist_new_var_string (item, "local_hostname", HOOK_CONNECT(hook, local_hostname)))
        return 0;
    if (!infolist_new_var_integer (item, "child_read", HOOK_CONNECT(hook, child_read)))
        return 0;
    if (!infolist_new_var_integer (item, "child_write", HOOK_CONNECT(hook, child_write)))
        return 0;
    if (!infolist_new_var_integer (item, "child_recv", HOOK_CONNECT(hook, child_recv)))
        return 0;
    if (!infolist_new_var_integer (item, "child_send", HOOK_CONNECT(hook, child_send)))
        return 0;
    if (!infolist_new_var_integer (item, "child_pid", HOOK_CONNECT(hook, child_pid)))
        return 0;
    if (!infolist_new_var_pointer (item, "hook_child_timer", HOOK_CONNECT(hook, hook_child_timer)))
        return 0;
    if (!infolist_new_var_pointer (item, "hook_fd", HOOK_CONNECT(hook, hook_fd)))
        return 0;
    if (!infolist_new_var_pointer (item, "handshake_hook_fd", HOOK_CONNECT(hook, handshake_hook_fd)))
        return 0;
    if (!infolist_new_var_pointer (item, "handshake_hook_timer", HOOK_CONNECT(hook, handshake_hook_timer)))
        return 0;
    if (!infolist_new_var_integer (item, "handshake_fd_flags", HOOK_CONNECT(hook, handshake_fd_flags)))
        return 0;
    if (!infolist_new_var_string (item, "handshake_ip_address", HOOK_CONNECT(hook, handshake_ip_address)))
        return 0;

    return 1;
}

/*
 * Prints connect hook data in WeeChat log file (usually for crash dump).
 */

void
hook_connect_print_log (struct t_hook *hook)
{
    int i;

    if (!hook || !hook->hook_data)
        return;

    log_printf ("  connect data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_CONNECT(hook, callback));
    log_printf ("    proxy . . . . . . . . : '%s'", HOOK_CONNECT(hook, proxy));
    log_printf ("    address . . . . . . . : '%s'", HOOK_CONNECT(hook, address));
    log_printf ("    port. . . . . . . . . : %d", HOOK_CONNECT(hook, port));
    log_printf ("    sock. . . . . . . . . : %d", HOOK_CONNECT(hook, sock));
    log_printf ("    ipv6. . . . . . . . . : %d", HOOK_CONNECT(hook, ipv6));
    log_printf ("    retry . . . . . . . . : %d", HOOK_CONNECT(hook, retry));
    log_printf ("    gnutls_sess . . . . . : %p", HOOK_CONNECT(hook, gnutls_sess));
    log_printf ("    gnutls_cb . . . . . . : %p", HOOK_CONNECT(hook, gnutls_cb));
    log_printf ("    gnutls_dhkey_size . . : %d", HOOK_CONNECT(hook, gnutls_dhkey_size));
    log_printf ("    gnutls_priorities . . : '%s'", HOOK_CONNECT(hook, gnutls_priorities));
    log_printf ("    local_hostname. . . . : '%s'", HOOK_CONNECT(hook, local_hostname));
    log_printf ("    child_read. . . . . . : %d", HOOK_CONNECT(hook, child_read));
    log_printf ("    child_write . . . . . : %d", HOOK_CONNECT(hook, child_write));
    log_printf ("    child_recv. . . . . . : %d", HOOK_CONNECT(hook, child_recv));
    log_printf ("    child_send. . . . . . : %d", HOOK_CONNECT(hook, child_send));
    log_printf ("    child_pid . . . . . . : %d", HOOK_CONNECT(hook, child_pid));
    log_printf ("    hook_child_timer. . . : %p", HOOK_CONNECT(hook, hook_child_timer));
    log_printf ("    hook_fd . . . . . . . : %p", HOOK_CONNECT(hook, hook_fd));
    log_printf ("    handshake_hook_fd . . : %p", HOOK_CONNECT(hook, handshake_hook_fd));
    log_printf ("    handshake_hook_timer. : %p", HOOK_CONNECT(hook, handshake_hook_timer));
    log_printf ("    handshake_fd_flags. . : %d", HOOK_CONNECT(hook, handshake_fd_flags));
    log_printf ("    handshake_ip_address. : '%s'", HOOK_CONNECT(hook, handshake_ip_address));
    if (!hook_socketpair_ok)
    {
        for (i = 0; i < HOOK_CONNECT_MAX_SOCKETS; i++)
        {
            log_printf ("    sock_v4[%03d]. . . . . : '%d'", i, HOOK_CONNECT(hook, sock_v4[i]));
            log_printf ("    sock_v6[%03d]. . . . . : '%d'", i, HOOK_CONNECT(hook, sock_v6[i]));
        }
    }
}
