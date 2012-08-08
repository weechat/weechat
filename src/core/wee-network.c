/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2010 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2010 Gu1ll4um3r0m41n <aeroxteam@gmail.com>
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
 * wee-network.c: network functions for WeeChat
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#ifdef HAVE_GCRYPT
#include <gcrypt.h>
#endif

#include "weechat.h"
#include "wee-network.h"
#include "wee-hook.h"
#include "wee-config.h"
#include "wee-proxy.h"
#include "wee-string.h"
#include "../plugins/plugin.h"


int network_init_ok = 0;

#ifdef HAVE_GNUTLS
gnutls_certificate_credentials gnutls_xcred; /* GnuTLS client credentials   */
#endif


/*
 * network_set_gnutls_ca_file: set trust file with option gnutls_ca_file
 */

void
network_set_gnutls_ca_file ()
{
#ifdef HAVE_GNUTLS
    char *ca_path, *ca_path2;

    ca_path = string_expand_home (CONFIG_STRING(config_network_gnutls_ca_file));
    if (ca_path)
    {
        ca_path2 = string_replace (ca_path, "%h", weechat_home);
        if (ca_path2)
        {
            gnutls_certificate_set_x509_trust_file (gnutls_xcred, ca_path2,
                                                    GNUTLS_X509_FMT_PEM);
            free (ca_path2);
        }
        free (ca_path);
    }
#endif
}

/*
 * network_init: init network
 */

void
network_init ()
{
#ifdef HAVE_GNUTLS
    gnutls_global_init ();
    gnutls_certificate_allocate_credentials (&gnutls_xcred);

    network_set_gnutls_ca_file ();
#if LIBGNUTLS_VERSION_NUMBER >= 0x02090a
    /* for gnutls >= 2.9.10 */
    gnutls_certificate_set_verify_function (gnutls_xcred,
                                            &hook_connect_gnutls_verify_certificates);
#endif
#if LIBGNUTLS_VERSION_NUMBER >= 0x020b00
    /* for gnutls >= 2.11.0 */
    gnutls_certificate_set_retrieve_function (gnutls_xcred,
                                              &hook_connect_gnutls_set_certificates);
#else
    /* for gnutls < 2.11.0 */
    gnutls_certificate_client_set_retrieve_function (gnutls_xcred,
                                                     &hook_connect_gnutls_set_certificates);
#endif
#endif
#ifdef HAVE_GCRYPT
    gcry_check_version (GCRYPT_VERSION);
    gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
    gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif
    network_init_ok = 1;
}

/*
 * network_end: end network
 */

void
network_end ()
{
    if (network_init_ok)
    {
#ifdef HAVE_GNUTLS
        gnutls_certificate_free_credentials (gnutls_xcred);
        gnutls_global_deinit();
#endif
        network_init_ok = 0;
    }
}

/*
 * network_send_with_retry: send data on a socket with retry
 *                          return number of bytes sent, or -1 if error
 *                          Note: this function is blocking, it must be called
 *                          only in a forked process
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
 * network_recv_with_retry: receive data on a socket with retry
 *                          return number of bytes received, or -1 if error
 *                          Note: this function is blocking, it must be called
 *                          only in a forked process
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
 * network_pass_httpproxy: establish connection/authentification to an
 *                         http proxy
 *                         return 1 if connection is ok
 *                                0 if error
 *                         Note: this function is blocking, it must be called
 *                         only in a forked process
 */

int
network_pass_httpproxy (struct t_proxy *proxy, int sock, const char *address,
                        int port)
{
    char buffer[256], authbuf[128], authbuf_base64[512];
    int length;

    if (CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])
        && CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME])[0])
    {
        /* authentification */
        snprintf (authbuf, sizeof (authbuf), "%s:%s",
                  CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME]),
                  (CONFIG_STRING(proxy->options[PROXY_OPTION_PASSWORD])) ?
                  CONFIG_STRING(proxy->options[PROXY_OPTION_PASSWORD]) : "");
        string_encode_base64 (authbuf, strlen (authbuf), authbuf_base64);
        length = snprintf (buffer, sizeof (buffer),
                           "CONNECT %s:%d HTTP/1.0\r\nProxy-Authorization: "
                           "Basic %s\r\n\r\n",
                           address, port, authbuf_base64);
    }
    else
    {
        /* no authentification */
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

    /* connection ok */
    return 1;
}

/*
 * network_resolve: resolve hostname on its IP address
 *                  (works with ipv4 and ipv6)
 *                  return 1 if resolution is ok
 *                         0 if error
 */

int
network_resolve (const char *hostname, char *ip, int *version)
{
    char ipbuffer[NI_MAXHOST];
    struct addrinfo *res;

    if (version != NULL)
        *version = 0;

    res = NULL;

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

    /* resolution ok */
    return 1;
}

/*
 * network_pass_socks4proxy: establish connection/authentification thru a
 *                           socks4 proxy
 *                           return 1 if connection is ok
 *                                  0 if error
 *                           Note: this function is blocking, it must be called
 *                           only in a forked process
 */

int
network_pass_socks4proxy (struct t_proxy *proxy, int sock, const char *address,
                          int port)
{
    /* socks4 protocol is explained here: http://en.wikipedia.org/wiki/SOCKS */

    struct t_network_socks4 socks4;
    unsigned char buffer[24];
    char ip_addr[NI_MAXHOST];
    int length;

    socks4.version = 4;
    socks4.method = 1;
    socks4.port = htons (port);
    network_resolve (address, ip_addr, NULL);
    socks4.address = inet_addr (ip_addr);
    strncpy (socks4.user, CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME]),
             sizeof (socks4.user) - 1);

    length = 8 + strlen (socks4.user) + 1;
    if (network_send_with_retry (sock, (char *) &socks4, length, 0) != length)
        return 0;

    if (network_recv_with_retry (sock, buffer, sizeof (buffer), 0) < 2)
        return 0;

    /* connection ok */
    if ((buffer[0] == 0) && (buffer[1] == 90))
        return 1;

    /* connection failed */
    return 0;
}

/*
 * network_pass_socks5proxy: establish connection/authentification thru a
 *                           socks5 proxy
 *                           return 1 if connection is ok
 *                                  0 if error
 *                           Note: this function is blocking, it must be called
 *                           only in a forked process
 */

int
network_pass_socks5proxy (struct t_proxy *proxy, int sock, const char *address,
                          int port)
{
    /*
     * socks5 protocol is explained in RFC 1928
     * socks5 authentication with username/pass is explained in RFC 1929
     */

    struct t_network_socks5 socks5;
    unsigned char buffer[288];
    int username_len, password_len, addr_len, addr_buffer_len;
    unsigned char *addr_buffer;

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
        username_len = strlen (CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME]));
        password_len = strlen (CONFIG_STRING(proxy->options[PROXY_OPTION_PASSWORD]));

        /* make username/password buffer */
        buffer[0] = 1;
        buffer[1] = (unsigned char) username_len;
        memcpy(buffer + 2, CONFIG_STRING(proxy->options[PROXY_OPTION_USERNAME]), username_len);
        buffer[2 + username_len] = (unsigned char) password_len;
        memcpy (buffer + 3 + username_len,
                CONFIG_STRING(proxy->options[PROXY_OPTION_PASSWORD]), password_len);

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

    /* connection ok */
    return 1;
}

/*
 * network_pass_proxy: establish connection/authentification to a proxy
 *                     return 1 if connection is ok
 *                            0 if error
 *                     Note: this function is blocking, it must be called
 *                     only in a forked process
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
 * network_connect: connect to a remote host and wait for connection if socket
 *                  is non blocking
 *                  return 1 if connect is ok, 0 if connect failed
 *                  Note: this function is blocking, it must be called
 *                  only in a forked process
 */

int
network_connect (int sock, const struct sockaddr *addr, socklen_t addrlen)
{
    fd_set write_fds;
    int ready, value;
    socklen_t len;

    if (connect (sock, addr, addrlen) == 0)
        return 1;

    if (errno != EINPROGRESS)
        return 0;

    /* for non-blocking sockets, the connect() may fail with EINPROGRESS,
     * if this happens, we wait for writability on socket and check
     * the option SO_ERROR, which is 0 if connect is ok (see man connect)
     */
    while (1)
    {
        FD_ZERO (&write_fds);
        FD_SET (sock, &write_fds);
        ready = select (sock + 1, NULL, &write_fds, NULL, NULL);
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
 * network_connect_to: connect to a remote host
 *                     return 1 if connection is ok
 *                            0 if error
 *                     Note: this function is blocking, it must be called
 *                     only in a forked process
 */

int
network_connect_to (const char *proxy, int sock,
                    unsigned long address, int port)
{
    struct t_proxy *ptr_proxy;
    struct sockaddr_in addr;
    struct hostent *hostent;
    char *ip4;

    ptr_proxy = NULL;
    if (proxy && proxy[0])
    {
        ptr_proxy = proxy_search (proxy);
        if (!ptr_proxy)
            return 0;
    }

    if (ptr_proxy)
    {
        memset (&addr, 0, sizeof (addr));
        addr.sin_addr.s_addr = htonl (address);
        ip4 = inet_ntoa(addr.sin_addr);

        memset (&addr, 0, sizeof (addr));
        addr.sin_port = htons (CONFIG_INTEGER(ptr_proxy->options[PROXY_OPTION_PORT]));
        addr.sin_family = AF_INET;
        hostent = gethostbyname (CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_ADDRESS]));
        if (!hostent)
            return 0;
        memcpy(&(addr.sin_addr), *(hostent->h_addr_list), sizeof(struct in_addr));
        if (!network_connect (sock, (struct sockaddr *) &addr, sizeof (addr)))
            return 0;
        if (!network_pass_proxy (proxy, sock, ip4, port))
            return 0;
    }
    else
    {
        memset (&addr, 0, sizeof (addr));
        addr.sin_port = htons (port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (address);
        if (!network_connect (sock, (struct sockaddr *) &addr, sizeof (addr)))
            return 0;
    }

    return 1;
}

/*
 * network_connect_child: child process trying to connect to peer
 */

void
network_connect_child (struct t_hook *hook_connect)
{
    struct t_proxy *ptr_proxy;
    struct addrinfo hints, *res, *res_local, *ptr_res;
    char status_str[2], *ptr_address, *status_with_string;
    char ipv4_address[INET_ADDRSTRLEN + 1], ipv6_address[INET6_ADDRSTRLEN + 1];
    char status_without_string[1 + 5 + 1];
    const char *error;
    int rc, length, num_written;

    res = NULL;
    res_local = NULL;

    status_str[1] = '\0';

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
            return;
        }
    }

    if (ptr_proxy)
    {
        /* get info about peer */
        memset (&hints, 0, sizeof (hints));
        hints.ai_family = (CONFIG_BOOLEAN(ptr_proxy->options[PROXY_OPTION_IPV6])) ?
            AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        rc = getaddrinfo (CONFIG_STRING(ptr_proxy->options[PROXY_OPTION_ADDRESS]), NULL, &hints, &res);
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
            if (status_with_string)
                free (status_with_string);
            (void) num_written;
            return;
        }
        if (!res)
        {
            /* adddress not found */
            snprintf (status_without_string, sizeof (status_without_string),
                      "%c00000", '0' + WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND);
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_without_string, strlen (status_without_string));
            (void) num_written;
            return;
        }
        if ((CONFIG_BOOLEAN(ptr_proxy->options[PROXY_OPTION_IPV6]) && (res->ai_family != AF_INET6))
            || ((!CONFIG_BOOLEAN(ptr_proxy->options[PROXY_OPTION_IPV6]) && (res->ai_family != AF_INET))))
        {
            /* IP address not found */
            snprintf (status_without_string, sizeof (status_without_string),
                      "%c00000", '0' + WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND);
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_without_string, strlen (status_without_string));
            (void) num_written;
            freeaddrinfo (res);
            return;
        }

        if (CONFIG_BOOLEAN(ptr_proxy->options[PROXY_OPTION_IPV6]))
            ((struct sockaddr_in6 *)(res->ai_addr))->sin6_port = htons (CONFIG_INTEGER(ptr_proxy->options[PROXY_OPTION_PORT]));
        else
            ((struct sockaddr_in *)(res->ai_addr))->sin_port = htons (CONFIG_INTEGER(ptr_proxy->options[PROXY_OPTION_PORT]));

        /* connect to peer */
        if (!network_connect (HOOK_CONNECT(hook_connect, sock),
                              res->ai_addr, res->ai_addrlen))
        {
            /* connection refused */
            snprintf (status_without_string, sizeof (status_without_string),
                      "%c00000", '0' + WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED);
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_without_string, strlen (status_without_string));
            (void) num_written;
            freeaddrinfo (res);
            return;
        }

        if (!network_pass_proxy (HOOK_CONNECT(hook_connect, proxy),
                                 HOOK_CONNECT(hook_connect, sock),
                                 HOOK_CONNECT(hook_connect, address),
                                 HOOK_CONNECT(hook_connect, port)))
        {
            /* proxy fails to connect to peer */
            snprintf (status_without_string, sizeof (status_without_string),
                      "%c00000", '0' + WEECHAT_HOOK_CONNECT_PROXY_ERROR);
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_without_string, strlen (status_without_string));
            (void) num_written;
            freeaddrinfo (res);
            return;
        }

        status_str[0] = '0' + WEECHAT_HOOK_CONNECT_OK;
    }
    else
    {
        /* set local hostname/IP if asked by user */
        if (HOOK_CONNECT(hook_connect, local_hostname)
            && HOOK_CONNECT(hook_connect, local_hostname[0]))
        {
            memset (&hints, 0, sizeof(hints));
            hints.ai_family = (HOOK_CONNECT(hook_connect, ipv6)) ? AF_INET6 : AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            rc = getaddrinfo (HOOK_CONNECT(hook_connect, local_hostname),
                              NULL, &hints, &res_local);
            if (rc != 0)
            {
                /* fails to set local hostname/IP */
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
                if (status_with_string)
                    free (status_with_string);
                (void) num_written;
                if (res_local)
                    freeaddrinfo (res_local);
                return;
            }
            else if (!res_local
                     || (HOOK_CONNECT(hook_connect, ipv6)
                         && (res_local->ai_family != AF_INET6))
                     || ((!HOOK_CONNECT(hook_connect, ipv6)
                          && (res_local->ai_family != AF_INET))))
            {
                /* fails to set local hostname/IP */
                snprintf (status_without_string, sizeof (status_without_string),
                          "%c00000", '0' + WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR);
                num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                     status_without_string, strlen (status_without_string));
                (void) num_written;
                if (res_local)
                    freeaddrinfo (res_local);
                return;
            }
            if (bind (HOOK_CONNECT(hook_connect, sock),
                      res_local->ai_addr, res_local->ai_addrlen) < 0)
            {
                /* fails to set local hostname/IP */
                snprintf (status_without_string, sizeof (status_without_string),
                          "%c00000", '0' + WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR);
                num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                     status_without_string, strlen (status_without_string));
                (void) num_written;
                if (res_local)
                    freeaddrinfo (res_local);
                return;
            }
        }

        /* get info about peer */
        memset (&hints, 0, sizeof(hints));
        hints.ai_family = (HOOK_CONNECT(hook_connect, ipv6)) ? AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        rc = getaddrinfo (HOOK_CONNECT(hook_connect, address),
                          NULL, &hints, &res);
        if (rc != 0)
        {
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
            if (status_with_string)
                free (status_with_string);
            (void) num_written;
            if (res)
                freeaddrinfo (res);
            if (res_local)
                freeaddrinfo (res_local);
            return;
        }
        else if (!res)
        {
            /* address not found */
            snprintf (status_without_string, sizeof (status_without_string),
                      "%c00000", '0' + WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND);
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_without_string, strlen (status_without_string));
            (void) num_written;
            if (res)
                freeaddrinfo (res);
            if (res_local)
                freeaddrinfo (res_local);
            return;
        }

        status_str[0] = '0' + WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND;

        /* try all IP addresses found, stop when connection is ok */
        for (ptr_res = res; ptr_res; ptr_res = ptr_res->ai_next)
        {
            /* skip IP address if it's not good family */
            if ((HOOK_CONNECT(hook_connect, ipv6) && (ptr_res->ai_family != AF_INET6))
                || ((!HOOK_CONNECT(hook_connect, ipv6) && (ptr_res->ai_family != AF_INET))))
                continue;

            /* connect to peer */
            if (HOOK_CONNECT(hook_connect, ipv6))
                ((struct sockaddr_in6 *)(ptr_res->ai_addr))->sin6_port =
                    htons (HOOK_CONNECT(hook_connect, port));
            else
                ((struct sockaddr_in *)(ptr_res->ai_addr))->sin_port =
                    htons (HOOK_CONNECT(hook_connect, port));

            if (network_connect (HOOK_CONNECT(hook_connect, sock),
                                 ptr_res->ai_addr, ptr_res->ai_addrlen))
            {
                status_str[0] = '0' + WEECHAT_HOOK_CONNECT_OK;
                break;
            }
            else
                status_str[0] = '0' + WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED;
        }
    }

    if (status_str[0] == '0' + WEECHAT_HOOK_CONNECT_OK)
    {
        status_with_string = NULL;
        ptr_address = NULL;
        if (HOOK_CONNECT(hook_connect, ipv6))
        {
            if (inet_ntop (AF_INET6,
                           &((struct sockaddr_in6 *)(res->ai_addr))->sin6_addr,
                           ipv6_address,
                           INET6_ADDRSTRLEN))
            {
                ptr_address = ipv6_address;
            }
        }
        else
        {
            if (inet_ntop (AF_INET,
                           &((struct sockaddr_in *)(res->ai_addr))->sin_addr,
                           ipv4_address,
                           INET_ADDRSTRLEN))
            {
                ptr_address = ipv4_address;
            }
        }
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
            free (status_with_string);
        }
        else
        {
            snprintf (status_without_string, sizeof (status_without_string),
                      "%s00000", status_str);
            num_written = write (HOOK_CONNECT(hook_connect, child_write),
                                 status_without_string, strlen (status_without_string));
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

    if (res)
        freeaddrinfo (res);
    if (res_local)
        freeaddrinfo (res_local);
}

/*
 * network_connect_child_timer_cb: timer for timeout of child process
 */

int
network_connect_child_timer_cb (void *arg_hook_connect, int remaining_calls)
{
    struct t_hook *hook_connect;

    /* make C compiler happy */
    (void) remaining_calls;

    hook_connect = (struct t_hook *)arg_hook_connect;

    HOOK_CONNECT(hook_connect, hook_child_timer) = NULL;

    (void) (HOOK_CONNECT(hook_connect, callback))
        (hook_connect->callback_data,
         WEECHAT_HOOK_CONNECT_TIMEOUT,
         0, NULL, NULL);
    unhook (hook_connect);

    return WEECHAT_RC_OK;
}

/*
 * network_connect_gnutls_handshake_fd_cb: callback for gnutls handshake
 *                                         (used to not block WeeChat if
 *                                         handshake takes some time to finish)
 */

#ifdef HAVE_GNUTLS
int
network_connect_gnutls_handshake_fd_cb (void *arg_hook_connect, int fd)
{
    struct t_hook *hook_connect;
    int rc, direction, flags;

    /* make C compiler happy */
    (void) fd;

    hook_connect = (struct t_hook *)arg_hook_connect;

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
            (hook_connect->callback_data,
             WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR,
             rc,
             gnutls_strerror (rc),
             HOOK_CONNECT(hook_connect, handshake_ip_address));
        unhook (hook_connect);
    }
    else
    {
        fcntl (HOOK_CONNECT(hook_connect, sock), F_SETFL,
               HOOK_CONNECT(hook_connect, handshake_fd_flags));
#if LIBGNUTLS_VERSION_NUMBER < 0x02090a
        /*
         * gnutls only has the gnutls_certificate_set_verify_function()
         * function since version 2.9.10. We need to call our verify
         * function manually after the handshake for old gnutls versions
         */
        if (hook_connect_gnutls_verify_certificates (*HOOK_CONNECT(hook_connect, gnutls_sess)) != 0)
        {
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_data,
                 WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR,
                 rc,
                 "Error in the certificate.",
                 HOOK_CONNECT(hook_connect, handshake_ip_address));
            unhook (hook_connect);
            return WEECHAT_RC_OK;
        }
#endif
        unhook (HOOK_CONNECT(hook_connect, handshake_hook_fd));
        (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_data, WEECHAT_HOOK_CONNECT_OK, 0, NULL,
                 HOOK_CONNECT(hook_connect, handshake_ip_address));
        unhook (hook_connect);
    }

    return WEECHAT_RC_OK;
}
#endif

/*
 * network_connect_gnutls_handshake_timer_cb: timer for timeout on handshake
 */

#ifdef HAVE_GNUTLS
int
network_connect_gnutls_handshake_timer_cb (void *arg_hook_connect,
                                           int remaining_calls)
{
    struct t_hook *hook_connect;

    /* make C compiler happy */
    (void) remaining_calls;

    hook_connect = (struct t_hook *)arg_hook_connect;

    HOOK_CONNECT(hook_connect, handshake_hook_timer) = NULL;

    (void) (HOOK_CONNECT(hook_connect, callback))
            (hook_connect->callback_data,
             WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR,
             GNUTLS_E_EXPIRED,
             gnutls_strerror (GNUTLS_E_EXPIRED),
             HOOK_CONNECT(hook_connect, handshake_ip_address));
    unhook (hook_connect);

    return WEECHAT_RC_OK;
}
#endif

/*
 * network_connect_child_read_cb: read connection progress from child process
 */

int
network_connect_child_read_cb (void *arg_hook_connect, int fd)
{
    struct t_hook *hook_connect;
    char buffer[1], buf_size[6], *cb_error, *cb_ip_address, *error;
    int num_read;
    long size_msg;
#ifdef HAVE_GNUTLS
    int rc, direction;
#endif

    /* make C compiler happy */
    (void) fd;

    hook_connect = (struct t_hook *)arg_hook_connect;

    cb_error = NULL;
    cb_ip_address = NULL;

    num_read = read (HOOK_CONNECT(hook_connect, child_read),
                     buffer, sizeof (buffer));
    if (num_read > 0)
    {
        if (buffer[0] - '0' == WEECHAT_HOOK_CONNECT_OK)
        {
            /* connection ok, read IP address */
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
                                          (gnutls_transport_ptr) ((ptrdiff_t) HOOK_CONNECT(hook_connect, sock)));
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
                                 hook_connect);
                    HOOK_CONNECT(hook_connect, handshake_hook_timer) =
                        hook_timer (hook_connect->plugin,
                                    CONFIG_INTEGER(config_network_gnutls_handshake_timeout) * 1000,
                                    0, 1,
                                    &network_connect_gnutls_handshake_timer_cb,
                                    hook_connect);
                    return WEECHAT_RC_OK;
                }
                else if (rc != GNUTLS_E_SUCCESS)
                {
                    (void) (HOOK_CONNECT(hook_connect, callback))
                        (hook_connect->callback_data,
                         WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR,
                         rc,
                         gnutls_strerror (rc),
                         cb_ip_address);
                    unhook (hook_connect);
                    if (cb_ip_address)
                        free (cb_ip_address);
                    return WEECHAT_RC_OK;
                }
                fcntl (HOOK_CONNECT(hook_connect, sock), F_SETFL,
                       HOOK_CONNECT(hook_connect, handshake_fd_flags));
#if LIBGNUTLS_VERSION_NUMBER < 0x02090a
                /*
                 * gnutls only has the gnutls_certificate_set_verify_function()
                 * function since version 2.9.10. We need to call our verify
                 * function manually after the handshake for old gnutls versions
                 */
                if (hook_connect_gnutls_verify_certificates (*HOOK_CONNECT(hook_connect, gnutls_sess)) != 0)
                {
                    (void) (HOOK_CONNECT(hook_connect, callback))
                        (hook_connect->callback_data,
                         WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR,
                         rc,
                         "Error in the certificate.",
                         cb_ip_address);
                    unhook (hook_connect);
                    if (cb_ip_address)
                        free (cb_ip_address);
                    return WEECHAT_RC_OK;
                }
#endif
            }
#endif
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
            (hook_connect->callback_data, buffer[0] - '0', 0,
             cb_error, cb_ip_address);
        unhook (hook_connect);
    }

    if (cb_error)
        free (cb_error);
    if (cb_ip_address)
        free (cb_ip_address);

    return WEECHAT_RC_OK;
}

/*
 * network_connect_with_fork: connect with fork (called by hook_connect() only!)
 */

void
network_connect_with_fork (struct t_hook *hook_connect)
{
    int child_pipe[2];
#ifdef HAVE_GNUTLS
    int rc;
    const char *pos_error;
#endif
    pid_t pid;

#ifdef HAVE_GNUTLS
    /* initialize GnuTLS if SSL asked */
    if (HOOK_CONNECT(hook_connect, gnutls_sess))
    {
        if (gnutls_init (HOOK_CONNECT(hook_connect, gnutls_sess), GNUTLS_CLIENT) != GNUTLS_E_SUCCESS)
        {
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_data,
                 WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR,
                 0, NULL, NULL);
            unhook (hook_connect);
            return;
        }
        rc = gnutls_priority_set_direct (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                         HOOK_CONNECT(hook_connect, gnutls_priorities),
                                         &pos_error);
        if (rc != GNUTLS_E_SUCCESS)
        {
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_data,
                 WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR,
                 0, _("invalid priorities"), NULL);
            unhook (hook_connect);
            return;
        }
        gnutls_credentials_set (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                GNUTLS_CRD_CERTIFICATE,
                                gnutls_xcred);
        gnutls_transport_set_ptr (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                  (gnutls_transport_ptr) ((unsigned long) HOOK_CONNECT(hook_connect, sock)));
    }
#endif

    /* create pipe for child process */
    if (pipe (child_pipe) < 0)
    {
        (void) (HOOK_CONNECT(hook_connect, callback))
            (hook_connect->callback_data,
             WEECHAT_HOOK_CONNECT_MEMORY_ERROR,
             0, NULL, NULL);
        unhook (hook_connect);
        return;
    }
    HOOK_CONNECT(hook_connect, child_read) = child_pipe[0];
    HOOK_CONNECT(hook_connect, child_write) = child_pipe[1];

    switch (pid = fork ())
    {
        /* fork failed */
        case -1:
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_data,
                 WEECHAT_HOOK_CONNECT_MEMORY_ERROR,
                 0, NULL, NULL);
            unhook (hook_connect);
            return;
        /* child process */
        case 0:
            setuid (getuid ());
            close (HOOK_CONNECT(hook_connect, child_read));
            network_connect_child (hook_connect);
            _exit (EXIT_SUCCESS);
    }
    /* parent process */
    HOOK_CONNECT(hook_connect, child_pid) = pid;
    close (HOOK_CONNECT(hook_connect, child_write));
    HOOK_CONNECT(hook_connect, child_write) = -1;
    HOOK_CONNECT(hook_connect, hook_child_timer) = hook_timer (hook_connect->plugin,
                                                               CONFIG_INTEGER(config_network_connection_timeout) * 1000,
                                                               0, 1,
                                                               &network_connect_child_timer_cb,
                                                               hook_connect);
    HOOK_CONNECT(hook_connect, hook_fd) = hook_fd (hook_connect->plugin,
                                                   HOOK_CONNECT(hook_connect, child_read),
                                                   1, 0, 0,
                                                   &network_connect_child_read_cb,
                                                   hook_connect);
}
