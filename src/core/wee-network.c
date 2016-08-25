/*
 * wee-network.c - network functions
 *
 * Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2010 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2010 Gu1ll4um3r0m41n <aeroxteam@gmail.com>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* _XPG4_2 is needed on SunOS for macros like CMSG_SPACE */
/* __EXTENSIONS__ is needed on SunOS for constants like NI_MAXHOST */
#ifdef __sun
#define _XPG4_2
#define __EXTENSIONS__
#endif

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netdb.h>
#include <resolv.h>
#include <errno.h>
#include <gcrypt.h>
#include <sys/time.h>
#if defined(__OpenBSD__)
#include <sys/uio.h>
#endif

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include "weechat.h"
#include "wee-network.h"
#include "wee-eval.h"
#include "wee-hook.h"
#include "wee-config.h"
#include "wee-proxy.h"
#include "wee-string.h"
#include "../plugins/plugin.h"


int network_init_gnutls_ok = 0;

#ifdef HAVE_GNUTLS
gnutls_certificate_credentials_t gnutls_xcred; /* GnuTLS client credentials */
#endif /* HAVE_GNUTLS */


/*
 * Initializes gcrypt.
 */

void
network_init_gcrypt ()
{
    if (!weechat_no_gcrypt)
    {
        gcry_check_version (GCRYPT_VERSION);
        gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
        gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
    }
}

/*
 * Sets trust file with option "gnutls_ca_file".
 */

void
network_set_gnutls_ca_file ()
{
#ifdef HAVE_GNUTLS
    char *ca_path, *ca_path2;

    if (weechat_no_gnutls)
        return;

    ca_path = string_expand_home (CONFIG_STRING(config_network_gnutls_ca_file));
    if (ca_path)
    {
        ca_path2 = string_replace (ca_path, "%h", weechat_home);
        if (ca_path2)
        {
            char *ca_path3 = ca_path2;
            char *single_path = malloc (strlen (ca_path3) + 1);

            while (1) {
                if (strchr (ca_path3, ':') == NULL)
                {
                    /* If there is no colon, just use the whole string and 
                     * return. */
                    gnutls_certificate_set_x509_trust_file (gnutls_xcred,
                            ca_path3, GNUTLS_X509_FMT_PEM);
                    break;
                }
                else
                {
                    /* If there is a colon in ca_path3, use the path up to the
                     * colon, and feed it to the function */
                    strncpy (single_path, ca_path3, 
                            strchr (ca_path3, ':') - ca_path3);
                    gnutls_certificate_set_x509_trust_file (gnutls_xcred,
                            single_path, GNUTLS_X509_FMT_PEM);
                    /* Then advance ca_path3 to the character after the colon */
                    ca_path3 = strchr (ca_path3, ':') + 1;
                }
            };
            free (ca_path2);
            free (single_path);
        }
        free (ca_path);
    }
#endif /* HAVE_GNUTLS */
}

/*
 * Initializes GnuTLS.
 */

void
network_init_gnutls ()
{
#ifdef HAVE_GNUTLS
    if (!weechat_no_gnutls)
    {
        gnutls_global_init ();
        gnutls_certificate_allocate_credentials (&gnutls_xcred);

        network_set_gnutls_ca_file ();
#if LIBGNUTLS_VERSION_NUMBER >= 0x02090a /* 2.9.10 */
        gnutls_certificate_set_verify_function (gnutls_xcred,
                                                &hook_connect_gnutls_verify_certificates);
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x02090a */
#if LIBGNUTLS_VERSION_NUMBER >= 0x020b00 /* 2.11.0 */
        gnutls_certificate_set_retrieve_function (gnutls_xcred,
                                                  &hook_connect_gnutls_set_certificates);
#else
        gnutls_certificate_client_set_retrieve_function (gnutls_xcred,
                                                         &hook_connect_gnutls_set_certificates);
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x020b00 */
    }
#endif /* HAVE_GNUTLS */

    network_init_gnutls_ok = 1;
}

/*
 * Ends network.
 */

void
network_end ()
{
    if (network_init_gnutls_ok)
    {
#ifdef HAVE_GNUTLS
        if (!weechat_no_gnutls)
        {
            gnutls_certificate_free_credentials (gnutls_xcred);
            gnutls_global_deinit();
        }
#endif /* HAVE_GNUTLS */
        network_init_gnutls_ok = 0;
    }
}

/*
 * Sends data on a socket with retry.
 *
 * WARNING: this function is blocking, it must be called only in a forked
 * process.
 *
 * Returns number of bytes sent, -1 if error.
 */

int
network_send_with_retry (int sock, const void *buffer, int length, int flags)
{
    int total_sent, num_sent;

    total_sent = 0;

    num_sent = send (sock, buffer, length, flags);
    if (num_sent > 0)
        total_sent += num_sent;

    while (total_sent < length)
    {
        if ((num_sent == -1) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
            return total_sent;
        usleep (100);
        num_sent = send (sock, buffer + total_sent, length - total_sent, flags);
        if (num_sent > 0)
            total_sent += num_sent;
    }
    return total_sent;
}

/*
 * Receives data on a socket with retry.
 *
 * WARNING: this function is blocking, it must be called only in a forked
 * process.
 *
 * Returns number of bytes received, -1 if error.
 */

int
network_recv_with_retry (int sock, void *buffer, int length, int flags)
{
    int total_recv, num_recv;

    total_recv = 0;

    num_recv = recv (sock, buffer, length, flags);
    if (num_recv > 0)
        total_recv += num_recv;

    while (num_recv == -1)
    {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
            return total_recv;
        usleep (100);
        num_recv = recv (sock, buffer + total_recv, length - total_recv, flags);
        if (num_recv > 0)
            total_recv += num_recv;
    }
    return total_recv;
}

/*
 * Establishes a connection and authenticates with a HTTP proxy.
 *
 * WARNING: this function is blocking, it must be called only in a forked
 * process.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
network_pass_httpproxy (struct t_proxy *proxy, int sock, const char *address,
                        int port)
{
    char buffer[256], authbuf[128], authbuf_base64[512], *username, *password;
    int length;

    if (CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])
        && CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])[0])
    {
        /* authentication */
        username = eval_expression (CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME]),
                                    NULL, NULL, NULL);
        if (!username)
            return 0;
        password = eval_expression (CONFIG_STRING(proxy->options[PROXY_OPTION_PASSWORD]),
                                    NULL, NULL, NULL);
        if (!password)
        {
            free (username);
            return 0;
        }
        snprintf (authbuf, sizeof (authbuf), "%s:%s", username, password);
        free (username);
        free (password);
        string_encode_base64 (authbuf, strlen (authbuf), authbuf_base64);
        length = snprintf (buffer, sizeof (buffer),
                           "CONNECT %s:%d HTTP/1.0\r\nProxy-Authorization: "
                           "Basic %s\r\n\r\n",
                           address, port, authbuf_base64);
    }
    else
    {
        /* no authentication */
        length = snprintf (buffer, sizeof (buffer),
                           "CONNECT %s:%d HTTP/1.0\r\n\r\n", address, port);
    }

    if (network_send_with_retry (sock, buffer, length, 0) != length)
        return 0;

    /* success result must be like: "HTTP/1.0 200 OK" */
    if (network_recv_with_retry (sock, buffer, sizeof (buffer), 0) < 12)
        return 0;

    if (memcmp (buffer, "HTTP/", 5) || memcmp (buffer + 9, "200", 3))
        return 0;

    /* connection OK */
    return 1;
}

/*
 * Resolves a hostname to its IP address (works with IPv4 and IPv6).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
network_resolve (const char *hostname, char *ip, int *version)
{
    char ipbuffer[NI_MAXHOST];
    struct addrinfo *res;

    if (version != NULL)
        *version = 0;

    res = NULL;

    res_init ();

    if (getaddrinfo (hostname, NULL, NULL, &res) != 0)
        return 0;

    if (!res)
        return 0;

    if (getnameinfo (res->ai_addr, res->ai_addrlen, ipbuffer, sizeof(ipbuffer),
                     NULL, 0, NI_NUMERICHOST) != 0)
    {
        freeaddrinfo (res);
        return 0;
    }

    if ((res->ai_family == AF_INET) && (version != NULL))
        *version = 4;
    if ((res->ai_family == AF_INET6) && (version != NULL))
        *version = 6;

    strcpy (ip, ipbuffer);

    freeaddrinfo (res);

    /* resolution OK */
    return 1;
}

/*
 * Establishes a connection and authenticates with a socks4 proxy.
 *
 * The socks4 protocol is explained here: http://en.wikipedia.org/wiki/SOCKS
 *
 * WARNING: this function is blocking, it must be called only in a forked
 * process.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
network_pass_socks4proxy (struct t_proxy *proxy, int sock, const char *address,
                          int port)
{
    struct t_network_socks4 socks4;
    unsigned char buffer[24];
    char ip_addr[NI_MAXHOST], *username;
    int length;

    username = eval_expression (CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME]),
                                NULL, NULL, NULL);
    if (!username)
        return 0;

    socks4.version = 4;
    socks4.method = 1;
    socks4.port = htons (port);
    network_resolve (address, ip_addr, NULL);
    socks4.address = inet_addr (ip_addr);
    strncpy (socks4.user, username, sizeof (socks4.user) - 1);

    free (username);

    length = 8 + strlen (socks4.user) + 1;
    if (network_send_with_retry (sock, (char *) &socks4, length, 0) != length)
        return 0;

    if (network_recv_with_retry (sock, buffer, sizeof (buffer), 0) < 2)
        return 0;

    /* connection OK */
    if ((buffer[0] == 0) && (buffer[1] == 90))
        return 1;

    /* connection failed */
    return 0;
}

/*
 * Establishes a connection and authenticates with a socks5 proxy.
 *
 * The socks5 protocol is explained in RFC 1928.
 * The socks5 authentication with username/pass is explained in RFC 1929.
 *
 * WARNING: this function is blocking, it must be called only in a forked
 * process.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
network_pass_socks5proxy (struct t_proxy *proxy, int sock, const char *address,
                          int port)
{
    struct t_network_socks5 socks5;
    unsigned char buffer[288];
    int username_len, password_len, addr_len, addr_buffer_len;
    unsigned char *addr_buffer;
    char *username, *password;

    socks5.version = 5;
    socks5.nmethods = 1;

    if (CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])
        && CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])[0])
        socks5.method = 2; /* with authentication */
    else
        socks5.method = 0; /* without authentication */

    if (network_send_with_retry (sock, (char *) &socks5, sizeof (socks5), 0) < (int)sizeof (socks5))
        return 0;

    /* server socks5 must respond with 2 bytes */
    if (network_recv_with_retry (sock, buffer, 2, 0) < 2)
        return 0;

    if (CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])
        && CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])[0])
    {
        /*
         * with authentication
         *   -> socks server must respond with :
         *       - socks version (buffer[0]) = 5 => socks5
         *       - socks method  (buffer[1]) = 2 => authentication
         */

        if (buffer[0] != 5 || buffer[1] != 2)
            return 0;

        /* authentication as in RFC 1929 */
        username = eval_expression (CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME]),
                                    NULL, NULL, NULL);
        if (!username)
            return 0;
        password = eval_expression (CONFIG_STRING(proxy->options[PROXY_OPTION_PASSWORD]),
                                    NULL, NULL, NULL);
        if (!password)
        {
            free (username);
            return 0;
        }
        username_len = strlen (username);
        password_len = strlen (password);

        /* make username/password buffer */
        buffer[0] = 1;
        buffer[1] = (unsigned char) username_len;
        memcpy (buffer + 2, username, username_len);
        buffer[2 + username_len] = (unsigned char) password_len;
        memcpy (buffer + 3 + username_len, password, password_len);

        free (username);
        free (password);

        if (network_send_with_retry (sock, buffer, 3 + username_len + password_len, 0) < 3 + username_len + password_len)
            return 0;

        /* server socks5 must respond with 2 bytes */
        if (network_recv_with_retry (sock, buffer, 2, 0) < 2)
            return 0;

        /* buffer[1] = auth state, must be 0 for success */
        if (buffer[1] != 0)
            return 0;
    }
    else
    {
        /*
         * without authentication
         *   -> socks server must respond with :
         *       - socks version (buffer[0]) = 5 => socks5
         *       - socks method  (buffer[1]) = 0 => no authentication
         */
        if (!((buffer[0] == 5) && (buffer[1] == 0)))
            return 0;
    }

    /* authentication successful then giving address/port to connect */
    addr_len = strlen(address);
    addr_buffer_len = 4 + 1 + addr_len + 2;
    addr_buffer = malloc (addr_buffer_len * sizeof(*addr_buffer));
    if (!addr_buffer)
        return 0;
    addr_buffer[0] = 5;   /* version 5 */
    addr_buffer[1] = 1;   /* command: 1 for connect */
    addr_buffer[2] = 0;   /* reserved */
    addr_buffer[3] = 3;   /* address type : ipv4 (1), domainname (3), ipv6 (4) */
    addr_buffer[4] = (unsigned char) addr_len;
    memcpy (addr_buffer + 5, address, addr_len); /* server address */
    *((unsigned short *) (addr_buffer + 5 + addr_len)) = htons (port); /* server port */

    if (network_send_with_retry (sock, addr_buffer, addr_buffer_len, 0) < addr_buffer_len)
    {
        free (addr_buffer);
        return 0;
    }
    free (addr_buffer);

    /* dialog with proxy server */
    if (network_recv_with_retry (sock, buffer, 4, 0) < 4)
        return 0;

    if (!((buffer[0] == 5) && (buffer[1] == 0)))
        return 0;

    /* buffer[3] = address type */
    switch (buffer[3])
    {
        case 1:
            /*
             * ipv4
             * server socks return server bound address and port
             * address of 4 bytes and port of 2 bytes (= 6 bytes)
             */
            if (network_recv_with_retry (sock, buffer, 6, 0) < 6)
                return 0;
            break;
        case 3:
            /*
             * domainname
             * server socks return server bound address and port
             */
            /* read address length */
            if (network_recv_with_retry (sock, buffer, 1, 0) < 1)
                return 0;
            addr_len = buffer[0];
            /* read address + port = addr_len + 2 */
            if (network_recv_with_retry (sock, buffer, addr_len + 2, 0) < addr_len + 2)
                return 0;
            break;
        case 4:
            /*
             * ipv6
             * server socks return server bound address and port
             * address of 16 bytes and port of 2 bytes (= 18 bytes)
             */
            if (network_recv_with_retry (sock, buffer, 18, 0) < 18)
                return 0;
            break;
        default:
            return 0;
    }

    /* connection OK */
    return 1;
}

/*
 * Establishes a connection and authenticates with a proxy.
 *
 * WARNING: this function is blocking, it must be called only in a forked
 * process.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
network_pass_proxy (const char *proxy, int sock, const char *address, int port)
{
    int rc;
    struct t_proxy *ptr_proxy;

    rc = 0;

    ptr_proxy = proxy_search (proxy);
    if (ptr_proxy)
    {
        switch (CONFIG_INTEGER(ptr_proxy->options[PROXY_OPTION_TYPE]))
        {
            case PROXY_TYPE_HTTP:
                rc = network_pass_httpproxy (ptr_proxy, sock, address, port);
                break;
            case PROXY_TYPE_SOCKS4:
                rc = network_pass_socks4proxy (ptr_proxy, sock, address, port);
                break;
            case PROXY_TYPE_SOCKS5:
                rc = network_pass_socks5proxy (ptr_proxy, sock, address, port);
                break;
        }
    }
    return rc;
}

/*
 * Connects to a remote host and wait for connection if socket is non blocking.
 *
 * WARNING: this function is blocking, it must be called only in a forked
 * process.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
network_connect (int sock, const struct sockaddr *addr, socklen_t addrlen)
{
    struct pollfd poll_fd;
    int ready, value;
    socklen_t len;

    if (connect (sock, addr, addrlen) == 0)
        return 1;

    if (errno != EINPROGRESS)
        return 0;

    /*
     * for non-blocking sockets, the connect() may fail with EINPROGRESS,
     * if this happens, we wait for writability on socket and check
     * the option SO_ERROR, which is 0 if connect is OK (see man connect)
     */
    while (1)
    {
        poll_fd.fd = sock;
        poll_fd.events = POLLOUT;
        poll_fd.revents = 0;
        ready = poll (&poll_fd, 1, -1);
        if (ready < 0)
            break;
        if (ready > 0)
        {
            len = sizeof (value);
            if (getsockopt (sock, SOL_SOCKET, SO_ERROR, &value, &len) == 0)
            {
                return (value == 0) ? 1 : 0;
            }
        }
    }

    return 0;
}

/*
 * Connects to a remote host.
 *
 * WARNING: this function is blocking, it must be called only in a forked
 * process.
 *
 * Returns:
 *   >= 0: connected socket fd
 *     -1: error
 */

int
network_connect_to (const char *proxy, struct sockaddr *address,
                    socklen_t address_length)
{
    struct t_proxy *ptr_proxy;
    struct addrinfo *proxy_addrinfo, hints;
    char str_port[16], ip[NI_MAXHOST];
    int port, sock;

    sock = -1;
    proxy_addrinfo = NULL;

    if (!address || (address_length == 0))
        return -1;

    ptr_proxy = NULL;
    if (proxy && proxy[0])
    {
        ptr_proxy = proxy_search (proxy);
        if (!ptr_proxy)
            return -1;
    }

    if (ptr_proxy)
    {
        /* get IP address/port */
        if (getnameinfo (address, address_length, ip, sizeof (ip),
                         str_port, sizeof (str_port),
                         NI_NUMERICHOST | NI_NUMERICSERV) != 0)
        {
            goto error;
        }
        port = atoi (str_port);

        /* get sockaddr for proxy */
        memset (&hints, 0, sizeof (hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_NUMERICSERV;
        snprintf (str_port, sizeof (str_port), "%d",
                 CONFIG_INTEGER(ptr_proxy->options[PROXY_OPTION_PORT]));
        res_init ();
        if (getaddrinfo (CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_ADDRESS]),
                         str_port, &hints, &proxy_addrinfo) != 0)
        {
            goto error;
        }

        /* connect and pass address to proxy */
        sock = socket (proxy_addrinfo->ai_family, SOCK_STREAM, 0);
        if (sock == -1)
            goto error;
        if (!network_connect (sock, proxy_addrinfo->ai_addr,
                              proxy_addrinfo->ai_addrlen))
            goto error;
        if (!network_pass_proxy (proxy, sock, ip, port))
            goto error;
    }
    else
    {
        sock = socket (address->sa_family, SOCK_STREAM, 0);
        if (sock == -1)
            goto error;
        if (!network_connect (sock, address, address_length))
            goto error;
    }

    if (proxy_addrinfo)
        freeaddrinfo (proxy_addrinfo);

    return sock;

error:
    if (sock >= 0)
        close (sock);
    if (proxy_addrinfo)
        freeaddrinfo (proxy_addrinfo);
    return -1;
}

/*
 * Connects to peer in a child process.
 */

void
network_connect_child (struct t_hook *hook_connect)
{
    struct t_proxy *ptr_proxy;
    struct addrinfo hints, *res_local, *res_remote, *ptr_res, *ptr_loc;
    char port[NI_MAXSERV + 1];
    char status_str[2], *ptr_address, *status_with_string;
    char remote_address[NI_MAXHOST + 1];
    char status_without_string[1 + 5 + 1];
    const char *error;
    int rc, length, num_written;
    int sock, set, flags, j;
    struct msghdr msg;
    struct cmsghdr *cmsg;
    char msg_buf[CMSG_SPACE(sizeof (sock))];
    struct iovec iov[1];
    char iov_data[1] = { 0 };
    /*
     * indicates that something is wrong with whichever group of
     * servers is being tried first after connecting, so start at
     * a different offset to increase the chance of success
     */
    int retry, rand_num, i;
    int num_groups, tmp_num_groups, num_hosts, tmp_host;
    struct addrinfo **res_reorder;
    int last_af;
    struct timeval tv_time;

    res_local = NULL;
    res_remote = NULL;
    res_reorder = NULL;
    port[0] = '\0';

    status_str[1] = '\0';
    status_with_string = NULL;

    ptr_address = NULL;

    gettimeofday (&tv_time, NULL);
    srand ((tv_time.tv_sec * tv_time.tv_usec) ^ getpid ());

    ptr_proxy = NULL;
    if (HOOK_CONNECT(hook_connect, proxy)
        && HOOK_CONNECT(hook_connect, proxy)[0])
    {
        ptr_proxy = proxy_search (HOOK_CONNECT(hook_connect, proxy));
        if (!ptr_proxy)
        {
            /* proxy not found */
            snprintf (status_without_string, sizeof (status_without_string),
                      "%c00000", '0' + WEECHAT_HOOK_CONNECT_PROXY_ERROR);
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_without_string, strlen (status_without_string));
            (void) num_written;
            goto end;
        }
    }

    /* get info about peer */
    memset (&hints, 0, sizeof (hints));
    hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
    hints.ai_flags = AI_ADDRCONFIG;
#endif /* AI_ADDRCONFIG */
    res_init ();
    if (ptr_proxy)
    {
        hints.ai_family = (CONFIG_BOOLEAN(ptr_proxy->options[PROXY_OPTION_IPV6])) ?
            AF_UNSPEC : AF_INET;
        snprintf (port, sizeof (port), "%d", CONFIG_INTEGER(ptr_proxy->options[PROXY_OPTION_PORT]));
        rc = getaddrinfo (CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_ADDRESS]),
                          port, &hints, &res_remote);
    }
    else
    {
        hints.ai_family = HOOK_CONNECT(hook_connect, ipv6) ? AF_UNSPEC : AF_INET;
        snprintf (port, sizeof (port), "%d", HOOK_CONNECT(hook_connect, port));
        rc = getaddrinfo (HOOK_CONNECT(hook_connect, address), port, &hints, &res_remote);
    }

    if (rc != 0)
    {
        /* address not found */
        status_with_string = NULL;
        error = gai_strerror (rc);
        if (error)
        {
            length = 1 + 5 + strlen (error) + 1;
            status_with_string = malloc (length);
            if (status_with_string)
            {
                snprintf (status_with_string, length, "%c%05d%s",
                          '0' + WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND,
                          (int)strlen (error), error);
            }
        }
        if (status_with_string)
        {
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_with_string, strlen (status_with_string));
        }
        else
        {
            snprintf (status_without_string, sizeof (status_without_string),
                      "%c00000", '0' + WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND);
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_without_string, strlen (status_without_string));
        }
        (void) num_written;
        goto end;
    }

    if (!res_remote)
    {
        /* address not found */
        snprintf (status_without_string, sizeof (status_without_string),
                  "%c00000", '0' + WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND);
        num_written = write (HOOK_CONNECT(hook_connect, child_write),
                             status_without_string, strlen (status_without_string));
        (void) num_written;
        goto end;
    }

    /* set local hostname/IP if asked by user */
    if (HOOK_CONNECT(hook_connect, local_hostname)
        && HOOK_CONNECT(hook_connect, local_hostname[0]))
    {
        memset (&hints, 0, sizeof (hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
        hints.ai_flags = AI_ADDRCONFIG;
#endif /* AI_ADDRCONFIG */
        rc = getaddrinfo (HOOK_CONNECT(hook_connect, local_hostname),
                          NULL, &hints, &res_local);
        if (rc != 0)
        {
            /* address not found */
            status_with_string = NULL;
            error = gai_strerror (rc);
            if (error)
            {
                length = 1 + 5 + strlen (error) + 1;
                status_with_string = malloc (length);
                if (status_with_string)
                {
                    snprintf (status_with_string, length, "%c%05d%s",
                              '0' + WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR,
                              (int)strlen (error), error);
                }
            }
            if (status_with_string)
            {
                num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                     status_with_string, strlen (status_with_string));
            }
            else
            {
                snprintf (status_without_string, sizeof (status_without_string),
                          "%c00000", '0' + WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR);
                num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                     status_without_string, strlen (status_without_string));
            }
            (void) num_written;
            goto end;
        }

        if (!res_local)
        {
            /* address not found */
            snprintf (status_without_string, sizeof (status_without_string),
                      "%c00000", '0' + WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR);
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_without_string, strlen (status_without_string));
            (void) num_written;
            goto end;
        }
    }

    /* res_local != NULL now indicates that bind() is required */

    /*
     * count all the groups of hosts by tracking family, e.g.
     * 0 = [2001:db8::1, 2001:db8::2,
     * 1 =  192.0.2.1, 192.0.2.2,
     * 2 =  2002:c000:201::1, 2002:c000:201::2]
     */
    last_af = AF_UNSPEC;
    num_groups = 0;
    num_hosts = 0;
    for (ptr_res = res_remote; ptr_res; ptr_res = ptr_res->ai_next)
    {
        if (ptr_res->ai_family != last_af)
            if (last_af != AF_UNSPEC)
                num_groups++;

        num_hosts++;
        last_af = ptr_res->ai_family;
    }
    if (last_af != AF_UNSPEC)
        num_groups++;

    res_reorder = malloc (sizeof (*res_reorder) * num_hosts);
    if (!res_reorder)
    {
        snprintf (status_without_string, sizeof (status_without_string),
                  "%c00000", '0' + WEECHAT_HOOK_CONNECT_MEMORY_ERROR);
        num_written = write (HOOK_CONNECT(hook_connect, child_write),
                             status_without_string, strlen (status_without_string));
        (void) num_written;
        goto end;
    }

    /* reorder groups */
    retry = HOOK_CONNECT(hook_connect, retry);
    if (num_groups > 0)
    {
        retry %= num_groups;
        i = 0;

        last_af = AF_UNSPEC;
        tmp_num_groups = 0;
        tmp_host = i; /* start of current group */

        /* top of list */
        for (ptr_res = res_remote; ptr_res; ptr_res = ptr_res->ai_next)
        {
            if (ptr_res->ai_family != last_af)
            {
                if (last_af != AF_UNSPEC)
                    tmp_num_groups++;

                tmp_host = i;
            }

            if (tmp_num_groups >= retry)
            {
                /* shuffle while adding */
                rand_num = tmp_host + (rand() % ((i + 1) - tmp_host));
                if (rand_num == i)
                    res_reorder[i++] = ptr_res;
                else
                {
                    res_reorder[i++] = res_reorder[rand_num];
                    res_reorder[rand_num] = ptr_res;
                }
            }

            last_af = ptr_res->ai_family;
        }

        last_af = AF_UNSPEC;
        tmp_num_groups = 0;
        tmp_host = i; /* start of current group */

        /* remainder of list */
        for (ptr_res = res_remote; ptr_res; ptr_res = ptr_res->ai_next)
        {
            if (ptr_res->ai_family != last_af)
            {
                if (last_af != AF_UNSPEC)
                    tmp_num_groups++;

                tmp_host = i;
            }

            if (tmp_num_groups < retry)
            {
                /* shuffle while adding */
                rand_num = tmp_host + (rand() % ((i + 1) - tmp_host));
                if (rand_num == i)
                    res_reorder[i++] = ptr_res;
                else
                {
                    res_reorder[i++] = res_reorder[rand_num];
                    res_reorder[rand_num] = ptr_res;
                }
            }
            else
                break;

            last_af = ptr_res->ai_family;
        }
    }
    else
    {
        /* no IP addresses found (all AF_UNSPEC) */
        snprintf (status_without_string, sizeof (status_without_string),
                  "%c00000", '0' + WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND);
        num_written = write (HOOK_CONNECT(hook_connect, child_write),
                             status_without_string, strlen (status_without_string));
        (void) num_written;
        goto end;
    }

    status_str[0] = '0' + WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND;

    /* try all IP addresses found, stop when connection is OK */
    sock = -1;
    for (i = 0; i < num_hosts; i++)
    {
        ptr_res = res_reorder[i];

        if (hook_socketpair_ok)
        {
            /* create a socket */
            sock = socket (ptr_res->ai_family,
                           ptr_res->ai_socktype,
                           ptr_res->ai_protocol);
        }
        else
        {
            /* use pre-created socket pool */
            sock = -1;
            for (j = 0; j < HOOK_CONNECT_MAX_SOCKETS; j++)
            {
                if (ptr_res->ai_family == AF_INET)
                {
                    sock = HOOK_CONNECT(hook_connect, sock_v4[j]);
                    if (sock != -1)
                    {
                        HOOK_CONNECT(hook_connect, sock_v4[j]) = -1;
                        break;
                    }
                }
                else if (ptr_res->ai_family == AF_INET6)
                {
                    sock = HOOK_CONNECT(hook_connect, sock_v6[j]);
                    if (sock != -1)
                    {
                        HOOK_CONNECT(hook_connect, sock_v6[j]) = -1;
                        break;
                    }
                }
            }
            if (sock < 0)
                continue;
        }
        if (sock < 0)
        {
            status_str[0] = '0' + WEECHAT_HOOK_CONNECT_SOCKET_ERROR;
            continue;
        }

        /* set SO_REUSEADDR option for socket */
        set = 1;
        setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (void *) &set, sizeof (set));

        /* set SO_KEEPALIVE option for socket */
        set = 1;
        setsockopt (sock, SOL_SOCKET, SO_KEEPALIVE, (void *) &set, sizeof (set));

        /* set flag O_NONBLOCK on socket */
        flags = fcntl (sock, F_GETFL);
        if (flags == -1)
            flags = 0;
        fcntl (sock, F_SETFL, flags | O_NONBLOCK);

        if (res_local)
        {
            rc = -1;

            /* bind local hostname/IP if asked by user */
            for (ptr_loc = res_local; ptr_loc; ptr_loc = ptr_loc->ai_next)
            {
                if (ptr_loc->ai_family != ptr_res->ai_family)
                    continue;

                rc = bind (sock, ptr_loc->ai_addr, ptr_loc->ai_addrlen);
                if (rc < 0)
                    continue;
            }

            if (rc < 0)
            {
                status_str[0] = '0' + WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR;
                close (sock);
                sock = -1;
                continue;
            }
        }

        /* connect to peer */
        if (network_connect (sock, ptr_res->ai_addr, ptr_res->ai_addrlen))
        {
            status_str[0] = '0' + WEECHAT_HOOK_CONNECT_OK;
            rc = getnameinfo (ptr_res->ai_addr, ptr_res->ai_addrlen,
                              remote_address, sizeof (remote_address),
                              NULL, 0, NI_NUMERICHOST);
            if (rc == 0)
                ptr_address = remote_address;
            break;
        }
        else
        {
            status_str[0] = '0' + WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED;
            close (sock);
            sock = -1;
        }
    }

    HOOK_CONNECT(hook_connect, sock) = sock;

    if (ptr_proxy && status_str[0] == '0' + WEECHAT_HOOK_CONNECT_OK)
    {
        if (!network_pass_proxy (HOOK_CONNECT(hook_connect, proxy),
                                 HOOK_CONNECT(hook_connect, sock),
                                 HOOK_CONNECT(hook_connect, address),
                                 HOOK_CONNECT(hook_connect, port)))
        {
            /* proxy fails to connect to peer */
            status_str[0] = '0' + WEECHAT_HOOK_CONNECT_PROXY_ERROR;
        }
    }

    if (status_str[0] == '0' + WEECHAT_HOOK_CONNECT_OK)
    {
        status_with_string = NULL;
        if (ptr_address)
        {
            length = strlen (status_str) + 5 + strlen (ptr_address) + 1;
            status_with_string = malloc (length);
            if (status_with_string)
            {
                snprintf (status_with_string, length, "%s%05d%s",
                          status_str, (int)strlen (ptr_address), ptr_address);
            }
        }

        if (status_with_string)
        {
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_with_string, strlen (status_with_string));
            (void) num_written;
        }
        else
        {
            snprintf (status_without_string, sizeof (status_without_string),
                      "%s00000", status_str);
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_without_string, strlen (status_without_string));
            (void) num_written;
        }

        /* send the socket to the parent process */
        if (hook_socketpair_ok)
        {
            memset (&msg, 0, sizeof (msg));
            msg.msg_control = msg_buf;
            msg.msg_controllen = sizeof (msg_buf);

            /* send 1 byte of data (not required on Linux, required by BSD/OSX) */
            memset (iov, 0, sizeof (iov));
            iov[0].iov_base = iov_data;
            iov[0].iov_len = 1;
            msg.msg_iov = iov;
            msg.msg_iovlen = 1;

            cmsg = CMSG_FIRSTHDR(&msg);
            cmsg->cmsg_level = SOL_SOCKET;
            cmsg->cmsg_type = SCM_RIGHTS;
            cmsg->cmsg_len = CMSG_LEN(sizeof (sock));
            memcpy(CMSG_DATA(cmsg), &sock, sizeof (sock));
            msg.msg_controllen = cmsg->cmsg_len;
            num_written = sendmsg (HOOK_CONNECT(hook_connect, child_send), &msg, 0);
            (void) num_written;
        }
        else
        {
            num_written = write (HOOK_CONNECT(hook_connect, child_write), &sock, sizeof (sock));
            (void) num_written;
        }
    }
    else
    {
        snprintf (status_without_string, sizeof (status_without_string),
                  "%s00000", status_str);
        num_written = write (HOOK_CONNECT(hook_connect, child_write),
                             status_without_string, strlen (status_without_string));
        (void) num_written;
    }

end:
    if (status_with_string)
        free (status_with_string);
    if (res_reorder)
        free (res_reorder);
    if (res_local)
        freeaddrinfo (res_local);
    if (res_remote)
        freeaddrinfo (res_remote);
}

/*
 * Timer callback for timeout of child process.
 */

int
network_connect_child_timer_cb (const void *pointer, void *data,
                                int remaining_calls)
{
    struct t_hook *hook_connect;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    hook_connect = (struct t_hook *)pointer;

    HOOK_CONNECT(hook_connect, hook_child_timer) = NULL;

    (void) (HOOK_CONNECT(hook_connect, callback))
        (hook_connect->callback_pointer,
         hook_connect->callback_data,
         WEECHAT_HOOK_CONNECT_TIMEOUT,
         0, -1, NULL, NULL);
    unhook (hook_connect);

    return WEECHAT_RC_OK;
}

/*
 * Callback for GnuTLS handshake.
 *
 * This callback is used to not block WeeChat (handshake takes some time to
 * finish).
 */

#ifdef HAVE_GNUTLS
int
network_connect_gnutls_handshake_fd_cb (const void *pointer, void *data,
                                        int fd)
{
    struct t_hook *hook_connect;
    int rc, direction, flags;

    /* make C compiler happy */
    (void) data;
    (void) fd;

    hook_connect = (struct t_hook *)pointer;

    rc = gnutls_handshake (*HOOK_CONNECT(hook_connect, gnutls_sess));

    if ((rc == GNUTLS_E_AGAIN) || (rc == GNUTLS_E_INTERRUPTED))
    {
        direction = gnutls_record_get_direction (*HOOK_CONNECT(hook_connect, gnutls_sess));
        flags = HOOK_FD(HOOK_CONNECT(hook_connect, handshake_hook_fd), flags);
        if ((((flags & HOOK_FD_FLAG_READ) == HOOK_FD_FLAG_READ)
             && (direction != 0))
            || (((flags & HOOK_FD_FLAG_WRITE) == HOOK_FD_FLAG_WRITE)
                && (direction != 1)))
        {
            HOOK_FD(HOOK_CONNECT(hook_connect, handshake_hook_fd), flags) =
                (direction) ? HOOK_FD_FLAG_WRITE: HOOK_FD_FLAG_READ;
        }
    }
    else if (rc != GNUTLS_E_SUCCESS)
    {
        (void) (HOOK_CONNECT(hook_connect, callback))
            (hook_connect->callback_pointer,
             hook_connect->callback_data,
             WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR, rc,
             HOOK_CONNECT(hook_connect, sock),
             gnutls_strerror (rc),
             HOOK_CONNECT(hook_connect, handshake_ip_address));
        unhook (hook_connect);
    }
    else
    {
        fcntl (HOOK_CONNECT(hook_connect, sock), F_SETFL,
               HOOK_CONNECT(hook_connect, handshake_fd_flags));
#if LIBGNUTLS_VERSION_NUMBER < 0x02090a /* 2.9.10 */
        /*
         * gnutls only has the gnutls_certificate_set_verify_function()
         * function since version 2.9.10. We need to call our verify
         * function manually after the handshake for old gnutls versions
         */
        if (hook_connect_gnutls_verify_certificates (*HOOK_CONNECT(hook_connect, gnutls_sess)) != 0)
        {
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_pointer,
                 hook_connect->callback_data,
                 WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR, rc,
                 HOOK_CONNECT(hook_connect, sock),
                 "Error in the certificate.",
                 HOOK_CONNECT(hook_connect, handshake_ip_address));
            unhook (hook_connect);
            return WEECHAT_RC_OK;
        }
#endif /* LIBGNUTLS_VERSION_NUMBER < 0x02090a */
        unhook (HOOK_CONNECT(hook_connect, handshake_hook_fd));
        (void) (HOOK_CONNECT(hook_connect, callback))
            (hook_connect->callback_pointer,
             hook_connect->callback_data,
             WEECHAT_HOOK_CONNECT_OK, 0,
             HOOK_CONNECT(hook_connect, sock),
             NULL, HOOK_CONNECT(hook_connect, handshake_ip_address));
        unhook (hook_connect);
    }

    return WEECHAT_RC_OK;
}
#endif /* HAVE_GNUTLS */

/*
 * Timer callback for timeout of handshake.
 */

#ifdef HAVE_GNUTLS
int
network_connect_gnutls_handshake_timer_cb (const void *pointer,
                                           void *data,
                                           int remaining_calls)
{
    struct t_hook *hook_connect;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    hook_connect = (struct t_hook *)pointer;

    HOOK_CONNECT(hook_connect, handshake_hook_timer) = NULL;

    (void) (HOOK_CONNECT(hook_connect, callback))
        (hook_connect->callback_pointer,
         hook_connect->callback_data,
         WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR,
         GNUTLS_E_EXPIRED,
         HOOK_CONNECT(hook_connect, sock),
         gnutls_strerror (GNUTLS_E_EXPIRED),
         HOOK_CONNECT(hook_connect, handshake_ip_address));
    unhook (hook_connect);

    return WEECHAT_RC_OK;
}
#endif /* HAVE_GNUTLS */

/*
 * Reads connection progress from child process.
 */

int
network_connect_child_read_cb (const void *pointer, void *data, int fd)
{
    struct t_hook *hook_connect;
    char buffer[1], buf_size[6], *cb_error, *cb_ip_address, *error;
    int num_read;
    long size_msg;
#ifdef HAVE_GNUTLS
    int rc, direction;
#endif /* HAVE_GNUTLS */
    int sock, i;
    struct msghdr msg;
    struct cmsghdr *cmsg;
    char msg_buf[CMSG_SPACE(sizeof (sock))];
    struct iovec iov[1];
    char iov_data[1];

    /* make C compiler happy */
    (void) data;
    (void) fd;

    hook_connect = (struct t_hook *)pointer;

    cb_error = NULL;
    cb_ip_address = NULL;
    sock = -1;

    num_read = read (HOOK_CONNECT(hook_connect, child_read),
                     buffer, sizeof (buffer));
    if (num_read == sizeof (buffer))
    {
        if (buffer[0] - '0' == WEECHAT_HOOK_CONNECT_OK)
        {
            /* connection OK, read IP address */
            buf_size[5] = '\0';
            num_read = read (HOOK_CONNECT(hook_connect, child_read),
                             buf_size, 5);
            if (num_read == 5)
            {
                error = NULL;
                size_msg = strtol (buf_size, &error, 10);
                if (error && !error[0] && (size_msg > 0))
                {
                    cb_ip_address = malloc (size_msg + 1);
                    if (cb_ip_address)
                    {
                        num_read = read (HOOK_CONNECT(hook_connect, child_read),
                                         cb_ip_address, size_msg);
                        if (num_read == size_msg)
                            cb_ip_address[size_msg] = '\0';
                        else
                        {
                            free (cb_ip_address);
                            cb_ip_address = NULL;
                        }
                    }
                }
            }

            if (hook_socketpair_ok)
            {
                /* receive the socket from the child process */
                memset (&msg, 0, sizeof (msg));
                msg.msg_control = msg_buf;
                msg.msg_controllen = sizeof (msg_buf);

                /* recv 1 byte of data (not required on Linux, required by BSD/OSX) */
                memset (iov, 0, sizeof (iov));
                iov[0].iov_base = iov_data;
                iov[0].iov_len = 1;
                msg.msg_iov = iov;
                msg.msg_iovlen = 1;

                if (recvmsg (HOOK_CONNECT(hook_connect, child_recv), &msg, 0) >= 0)
                {
                    cmsg = CMSG_FIRSTHDR(&msg);
                    if (cmsg != NULL
                        && cmsg->cmsg_level == SOL_SOCKET
                        && cmsg->cmsg_type == SCM_RIGHTS
                        && cmsg->cmsg_len >= sizeof (sock))
                    {
                        memcpy(&sock, CMSG_DATA(cmsg), sizeof (sock));
                    }
                }
            }
            else
            {
                num_read = read (HOOK_CONNECT(hook_connect, child_read), &sock, sizeof (sock));
                (void) num_read;

                /* prevent unhook process from closing the socket */
                for (i = 0; i < HOOK_CONNECT_MAX_SOCKETS; i++)
                {
                    if (HOOK_CONNECT(hook_connect, sock_v4[i]) == sock)
                        HOOK_CONNECT(hook_connect, sock_v4[i]) = -1;

                    if (HOOK_CONNECT(hook_connect, sock_v6[i]) == sock)
                        HOOK_CONNECT(hook_connect, sock_v6[i]) = -1;
                }
            }

            HOOK_CONNECT(hook_connect, sock) = sock;

#ifdef HAVE_GNUTLS
            if (HOOK_CONNECT(hook_connect, gnutls_sess))
            {
                /*
                 * the socket needs to be non-blocking since the call to
                 * gnutls_handshake can block
                 */
                HOOK_CONNECT(hook_connect, handshake_fd_flags) =
                    fcntl (HOOK_CONNECT(hook_connect, sock), F_GETFL);
                if (HOOK_CONNECT(hook_connect, handshake_fd_flags) == -1)
                    HOOK_CONNECT(hook_connect, handshake_fd_flags) = 0;
                fcntl (HOOK_CONNECT(hook_connect, sock), F_SETFL,
                       HOOK_CONNECT(hook_connect, handshake_fd_flags) | O_NONBLOCK);
                gnutls_transport_set_ptr (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                          (gnutls_transport_ptr_t) ((ptrdiff_t) HOOK_CONNECT(hook_connect, sock)));
                if (HOOK_CONNECT(hook_connect, gnutls_dhkey_size) > 0)
                {
                    gnutls_dh_set_prime_bits (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                              (unsigned int) HOOK_CONNECT(hook_connect, gnutls_dhkey_size));
                }
                rc = gnutls_handshake (*HOOK_CONNECT(hook_connect, gnutls_sess));
                if ((rc == GNUTLS_E_AGAIN) || (rc == GNUTLS_E_INTERRUPTED))
                {
                    /*
                     * gnutls was unable to proceed with the handshake without
                     * blocking: non fatal error, we just have to wait for an
                     * event about handshake
                     */
                    unhook (HOOK_CONNECT(hook_connect, hook_fd));
                    HOOK_CONNECT(hook_connect, hook_fd) = NULL;
                    direction = gnutls_record_get_direction (*HOOK_CONNECT(hook_connect, gnutls_sess));
                    HOOK_CONNECT(hook_connect, handshake_ip_address) = cb_ip_address;
                    HOOK_CONNECT(hook_connect, handshake_hook_fd) =
                        hook_fd (hook_connect->plugin,
                                 HOOK_CONNECT(hook_connect, sock),
                                 (!direction ? 1 : 0), (direction  ? 1 : 0), 0,
                                 &network_connect_gnutls_handshake_fd_cb,
                                 hook_connect, NULL);
                    HOOK_CONNECT(hook_connect, handshake_hook_timer) =
                        hook_timer (hook_connect->plugin,
                                    CONFIG_INTEGER(config_network_gnutls_handshake_timeout) * 1000,
                                    0, 1,
                                    &network_connect_gnutls_handshake_timer_cb,
                                    hook_connect, NULL);
                    return WEECHAT_RC_OK;
                }
                else if (rc != GNUTLS_E_SUCCESS)
                {
                    (void) (HOOK_CONNECT(hook_connect, callback))
                        (hook_connect->callback_pointer,
                         hook_connect->callback_data,
                         WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR,
                         rc, sock,
                         gnutls_strerror (rc),
                         cb_ip_address);
                    unhook (hook_connect);
                    if (cb_ip_address)
                        free (cb_ip_address);
                    return WEECHAT_RC_OK;
                }
                fcntl (HOOK_CONNECT(hook_connect, sock), F_SETFL,
                       HOOK_CONNECT(hook_connect, handshake_fd_flags));
#if LIBGNUTLS_VERSION_NUMBER < 0x02090a /* 2.9.10 */
                /*
                 * gnutls only has the gnutls_certificate_set_verify_function()
                 * function since version 2.9.10. We need to call our verify
                 * function manually after the handshake for old gnutls versions
                 */
                if (hook_connect_gnutls_verify_certificates (*HOOK_CONNECT(hook_connect, gnutls_sess)) != 0)
                {
                    (void) (HOOK_CONNECT(hook_connect, callback))
                        (hook_connect->callback_pointer,
                         hook_connect->callback_data,
                         WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR,
                         rc, sock,
                         "Error in the certificate.",
                         cb_ip_address);
                    unhook (hook_connect);
                    if (cb_ip_address)
                        free (cb_ip_address);
                    return WEECHAT_RC_OK;
                }
#endif /* LIBGNUTLS_VERSION_NUMBER < 0x02090a */
            }
#endif /* HAVE_GNUTLS */
        }
        else
        {
            /* connection error, read error */
            buf_size[5] = '\0';
            num_read = read (HOOK_CONNECT(hook_connect, child_read),
                             buf_size, 5);
            if (num_read == 5)
            {
                error = NULL;
                size_msg = strtol (buf_size, &error, 10);
                if (error && !error[0] && (size_msg > 0))
                {
                    cb_error = malloc (size_msg + 1);
                    if (cb_error)
                    {
                        num_read = read (HOOK_CONNECT(hook_connect, child_read),
                                         cb_error, size_msg);
                        if (num_read == size_msg)
                            cb_error[size_msg] = '\0';
                        else
                        {
                            free (cb_error);
                            cb_error = NULL;
                        }
                    }
                }
            }
        }
        (void) (HOOK_CONNECT(hook_connect, callback))
            (hook_connect->callback_pointer,
             hook_connect->callback_data,
             buffer[0] - '0', 0,
             sock, cb_error, cb_ip_address);
        unhook (hook_connect);
    }
    else
    {
        (void) (HOOK_CONNECT(hook_connect, callback))
            (hook_connect->callback_pointer,
             hook_connect->callback_data,
             WEECHAT_HOOK_CONNECT_MEMORY_ERROR,
             0, sock, "child_read_cb", NULL);
        unhook (hook_connect);
    }

    if (cb_error)
        free (cb_error);
    if (cb_ip_address)
        free (cb_ip_address);

    return WEECHAT_RC_OK;
}

/*
 * Connects with fork (called by hook_connect() only!).
 */

void
network_connect_with_fork (struct t_hook *hook_connect)
{
    int child_pipe[2], child_socket[2], rc, i;
#ifdef HAVE_GNUTLS
    const char *pos_error;
#endif /* HAVE_GNUTLS */
    pid_t pid;

#ifdef HAVE_GNUTLS
    /* initialize GnuTLS if SSL asked */
    if (HOOK_CONNECT(hook_connect, gnutls_sess))
    {
        if (gnutls_init (HOOK_CONNECT(hook_connect, gnutls_sess), GNUTLS_CLIENT) != GNUTLS_E_SUCCESS)
        {
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_pointer,
                 hook_connect->callback_data,
                 WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR,
                 0, -1, NULL, NULL);
            unhook (hook_connect);
            return;
        }
        rc = gnutls_server_name_set (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                     GNUTLS_NAME_DNS,
                                     HOOK_CONNECT(hook_connect, address),
                                     strlen (HOOK_CONNECT(hook_connect, address)));
        if (rc != GNUTLS_E_SUCCESS)
        {
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_pointer,
                 hook_connect->callback_data,
                 WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR,
                 0, -1, _("set server name indication (SNI) failed"), NULL);
            unhook (hook_connect);
            return;
        }
        rc = gnutls_priority_set_direct (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                         HOOK_CONNECT(hook_connect, gnutls_priorities),
                                         &pos_error);
        if (rc != GNUTLS_E_SUCCESS)
        {
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_pointer,
                 hook_connect->callback_data,
                 WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR,
                 0, -1, _("invalid priorities"), NULL);
            unhook (hook_connect);
            return;
        }
        gnutls_credentials_set (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                GNUTLS_CRD_CERTIFICATE,
                                gnutls_xcred);
        gnutls_transport_set_ptr (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                  (gnutls_transport_ptr_t) ((unsigned long) HOOK_CONNECT(hook_connect, sock)));
    }
#endif /* HAVE_GNUTLS */

    /* create pipe for child process */
    if (pipe (child_pipe) < 0)
    {
        (void) (HOOK_CONNECT(hook_connect, callback))
            (hook_connect->callback_pointer,
             hook_connect->callback_data,
             WEECHAT_HOOK_CONNECT_MEMORY_ERROR,
             0, -1, "pipe", NULL);
        unhook (hook_connect);
        return;
    }
    HOOK_CONNECT(hook_connect, child_read) = child_pipe[0];
    HOOK_CONNECT(hook_connect, child_write) = child_pipe[1];

    if (hook_socketpair_ok)
    {
        /* create socket for child process */
        if (socketpair (AF_LOCAL, SOCK_DGRAM, 0, child_socket) < 0)
        {
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_pointer,
                 hook_connect->callback_data,
                 WEECHAT_HOOK_CONNECT_MEMORY_ERROR,
                 0, -1, "socketpair", NULL);
            unhook (hook_connect);
            return;
        }
        HOOK_CONNECT(hook_connect, child_recv) = child_socket[0];
        HOOK_CONNECT(hook_connect, child_send) = child_socket[1];
    }
    else
    {
        for (i = 0; i < HOOK_CONNECT_MAX_SOCKETS; i++)
        {
            HOOK_CONNECT(hook_connect, sock_v4[i]) = socket (AF_INET, SOCK_STREAM, 0);
            HOOK_CONNECT(hook_connect, sock_v6[i]) = socket (AF_INET6, SOCK_STREAM, 0);
        }
    }

    switch (pid = fork ())
    {
        /* fork failed */
        case -1:
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_pointer,
                 hook_connect->callback_data,
                 WEECHAT_HOOK_CONNECT_MEMORY_ERROR,
                 0, -1, "fork", NULL);
            unhook (hook_connect);
            return;
        /* child process */
        case 0:
            rc = setuid (getuid ());
            (void) rc;
            close (HOOK_CONNECT(hook_connect, child_read));
            if (hook_socketpair_ok)
                close (HOOK_CONNECT(hook_connect, child_recv));
            network_connect_child (hook_connect);
            _exit (EXIT_SUCCESS);
    }
    /* parent process */
    HOOK_CONNECT(hook_connect, child_pid) = pid;
    close (HOOK_CONNECT(hook_connect, child_write));
    HOOK_CONNECT(hook_connect, child_write) = -1;
    if (hook_socketpair_ok)
    {
        close (HOOK_CONNECT(hook_connect, child_send));
        HOOK_CONNECT(hook_connect, child_send) = -1;
    }
    HOOK_CONNECT(hook_connect, hook_child_timer) = hook_timer (hook_connect->plugin,
                                                               CONFIG_INTEGER(config_network_connection_timeout) * 1000,
                                                               0, 1,
                                                               &network_connect_child_timer_cb,
                                                               hook_connect,
                                                               NULL);
    HOOK_CONNECT(hook_connect, hook_fd) = hook_fd (hook_connect->plugin,
                                                   HOOK_CONNECT(hook_connect, child_read),
                                                   1, 0, 0,
                                                   &network_connect_child_read_cb,
                                                   hook_connect, NULL);
}
