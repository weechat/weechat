/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* wee-network.c: network functions for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
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

#include "weechat.h"
#include "wee-network.h"
#include "wee-hook.h"
#include "wee-config.h"
#include "wee-string.h"
#include "../plugins/plugin.h"


#ifdef HAVE_GNUTLS
gnutls_certificate_credentials gnutls_xcred; /* GnuTLS client credentials   */
const int gnutls_cert_type_prio[] = { GNUTLS_CRT_X509, GNUTLS_CRT_OPENPGP, 0 };
#if LIBGNUTLS_VERSION_NUMBER >= 0x010700
    const int gnutls_prot_prio[] = { GNUTLS_TLS1_2, GNUTLS_TLS1_1,
                                     GNUTLS_TLS1_0, GNUTLS_SSL3, 0 };
#else
    const int gnutls_prot_prio[] = { GNUTLS_TLS1_1, GNUTLS_TLS1_0,
                                     GNUTLS_SSL3, 0 };
#endif
#endif


/*
 * network_init: init network
 */

void
network_init ()
{
#ifdef HAVE_GNUTLS
    gnutls_global_init ();
    gnutls_certificate_allocate_credentials (&gnutls_xcred);
    gnutls_certificate_set_x509_trust_file (gnutls_xcred, "ca.pem", GNUTLS_X509_FMT_PEM);
#endif
}

/*
 * network_end: end network
 */

void
network_end ()
{
#ifdef HAVE_GNUTLS
    gnutls_certificate_free_credentials (gnutls_xcred);
    gnutls_global_deinit();
#endif
}

/*
 * network_convbase64_8x3_to_6x4 : convert 3 bytes of 8 bits in 4 bytes of 6 bits
 */

void
network_convbase64_8x3_to_6x4 (const char *from, char *to)
{
    unsigned char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz0123456789+/";
    
    to[0] = base64_table [ (from[0] & 0xfc) >> 2 ];
    to[1] = base64_table [ ((from[0] & 0x03) << 4) + ((from[1] & 0xf0) >> 4) ];
    to[2] = base64_table [ ((from[1] & 0x0f) << 2) + ((from[2] & 0xc0) >> 6) ];
    to[3] = base64_table [ from[2] & 0x3f ];
}

/*
 * network_base64encode: encode a string in base64
 */

void
network_base64encode (const char *from, char *to)
{
    const char *f;
    char *t;
    int from_len;
    
    from_len = strlen (from);
    
    f = from;
    t = to;
    
    while (from_len >= 3)
    {
        network_convbase64_8x3_to_6x4 (f, t);
        f += 3 * sizeof (*f);
        t += 4 * sizeof (*t);
        from_len -= 3;
    }
    
    if (from_len > 0)
    {
        char rest[3] = { 0, 0, 0 };
        switch (from_len)
        {
            case 1 :
                rest[0] = f[0];
                network_convbase64_8x3_to_6x4 (rest, t);
                t[2] = t[3] = '=';
                break;
            case 2 :
                rest[0] = f[0];
                rest[1] = f[1];
                network_convbase64_8x3_to_6x4 (rest, t);
                t[3] = '=';
                break;
        }
        t[4] = 0;
    }
}

/*
 * network_pass_httpproxy: establish connection/authentification to an
 *                         http proxy
 *                         return 1 if connection is ok
 *                                0 if error
 */

int
network_pass_httpproxy (int sock, const char *address, int port)
{
    char buffer[256], authbuf[128], authbuf_base64[196];
    int n, m;
    
    if (CONFIG_STRING(config_proxy_username)
        && CONFIG_STRING(config_proxy_username)[0])
    {
        /* authentification */
        snprintf (authbuf, sizeof (authbuf), "%s:%s",
                  CONFIG_STRING(config_proxy_username),
                  (CONFIG_STRING(config_proxy_password)) ?
                  CONFIG_STRING(config_proxy_password) : "");
        network_base64encode (authbuf, authbuf_base64);
        n = snprintf (buffer, sizeof (buffer),
                      "CONNECT %s:%d HTTP/1.0\r\nProxy-Authorization: Basic %s\r\n\r\n",
                      address, port, authbuf_base64);
    }
    else 
    {
        /* no authentification */
        n = snprintf (buffer, sizeof (buffer),
                      "CONNECT %s:%d HTTP/1.0\r\n\r\n", address, port);
    }
    
    m = send (sock, buffer, n, 0);
    if (n != m)
        return 0;
    
    n = recv (sock, buffer, sizeof (buffer), 0);
    
    /* success result must be like: "HTTP/1.0 200 OK" */
    if (n < 12)
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
 */

int
network_pass_socks4proxy (int sock, const char *address, int port)
{
    /* 
     * socks4 protocol is explained here: 
     *  http://archive.socks.permeo.com/protocol/socks4.protocol
     *
     */
    
    struct s_socks4
    {
        char version;           /* 1 byte  */ /* socks version : 4 or 5 */
        char method;            /* 1 byte  */ /* socks method : connect (1) or bind (2) */
        unsigned short port;    /* 2 bytes */ /* destination port */
        unsigned long address;  /* 4 bytes */ /* destination address */
        char user[64];          /* username (64 characters seems to be enought) */
    } socks4;
    unsigned char buffer[24];
    char ip_addr[NI_MAXHOST];
    
    socks4.version = 4;
    socks4.method = 1;
    socks4.port = htons (port);
    network_resolve (address, ip_addr, NULL);
    socks4.address = inet_addr (ip_addr);
    strncpy (socks4.user, CONFIG_STRING(config_proxy_username),
             sizeof (socks4.user) - 1);
    
    send (sock, (char *) &socks4, 8 + strlen (socks4.user) + 1, 0);
    recv (sock, buffer, sizeof (buffer), 0);
    
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
 */

int
network_pass_socks5proxy (int sock, const char *address, int port)
{
    /* 
     * socks5 protocol is explained in RFC 1928
     * socks5 authentication with username/pass is explained in RFC 1929
     */
    
    struct s_sock5
    {
        char version;   /* 1 byte      */ /* socks version : 4 or 5 */
        char nmethods;  /* 1 byte      */ /* size in byte(s) of field 'method', here 1 byte */
        char method;    /* 1-255 bytes */ /* socks method : noauth (0), auth(user/pass) (2), ... */
    } socks5;
    unsigned char buffer[288];
    int username_len, password_len, addr_len, addr_buffer_len;
    unsigned char *addr_buffer;
    
    socks5.version = 5;
    socks5.nmethods = 1;
    
    if (CONFIG_STRING(config_proxy_username)
        && CONFIG_STRING(config_proxy_username)[0])
        socks5.method = 2; /* with authentication */
    else
        socks5.method = 0; /* without authentication */
    
    send (sock, (char *) &socks5, sizeof(socks5), 0);
    /* server socks5 must respond with 2 bytes */
    if (recv (sock, buffer, 2, 0) != 2)
        return 0;
    
    if (CONFIG_STRING(config_proxy_username)
        && CONFIG_STRING(config_proxy_username)[0])
    {
        /* with authentication */
        /*   -> socks server must respond with :
         *       - socks version (buffer[0]) = 5 => socks5
         *       - socks method  (buffer[1]) = 2 => authentication
         */
        
        if (buffer[0] != 5 || buffer[1] != 2)
            return 0;
        
        /* authentication as in RFC 1929 */
        username_len = strlen (CONFIG_STRING(config_proxy_username));
        password_len = strlen (CONFIG_STRING(config_proxy_password));
        
        /* make username/password buffer */
        buffer[0] = 1;
        buffer[1] = (unsigned char) username_len;
        memcpy(buffer + 2, CONFIG_STRING(config_proxy_username), username_len);
        buffer[2 + username_len] = (unsigned char) password_len;
        memcpy (buffer + 3 + username_len,
                CONFIG_STRING(config_proxy_password), password_len);
        
        send (sock, buffer, 3 + username_len + password_len, 0);
        
        /* server socks5 must respond with 2 bytes */
        if (recv (sock, buffer, 2, 0) != 2)
            return 0;
        
        /* buffer[1] = auth state, must be 0 for success */
        if (buffer[1] != 0)
            return 0;
    }
    else
    {
        /* without authentication */
        /*   -> socks server must respond with :
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
    
    send (sock, addr_buffer, addr_buffer_len, 0);
    free (addr_buffer);
    
    /* dialog with proxy server */
    if (recv (sock, buffer, 4, 0) != 4)
        return 0;
    
    if (!((buffer[0] == 5) && (buffer[1] == 0)))
        return 0;
    
    /* buffer[3] = address type */
    switch (buffer[3])
    {
        case 1:
            /* ipv4 
             * server socks return server bound address and port
             * address of 4 bytes and port of 2 bytes (= 6 bytes)
             */
            if (recv (sock, buffer, 6, 0) != 6)
                return 0;
            break;
        case 3:
            /* domainname
             * server socks return server bound address and port
             */
            /* reading address length */
            if (recv (sock, buffer, 1, 0) != 1)
                return 0;    
            addr_len = buffer[0];
            /* reading address + port = addr_len + 2 */
            if (recv (sock, buffer, addr_len + 2, 0) != (addr_len + 2))
                return 0;
            break;
        case 4:
            /* ipv6
             * server socks return server bound address and port
             * address of 16 bytes and port of 2 bytes (= 18 bytes)
             */
            if (recv (sock, buffer, 18, 0) != 18)
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
 */

int
network_pass_proxy (int sock, const char *address, int port)
{
    int rc;
    
    rc = 0;
    if (CONFIG_BOOLEAN(config_proxy_use))
    {
        if (string_strcasecmp (CONFIG_STRING(config_proxy_type), "http") == 0)
            rc = network_pass_httpproxy (sock, address, port);
        if (string_strcasecmp (CONFIG_STRING(config_proxy_type),  "socks4") == 0)
            rc = network_pass_socks4proxy (sock, address, port);
        if (string_strcasecmp (CONFIG_STRING(config_proxy_type), "socks5") == 0)
            rc = network_pass_socks5proxy (sock, address, port);
    }
    return rc;
}

/*
 * network_connect_to: connect to a remote host
 *                     return 1 if connection is ok
 *                            0 if error
 */

int
network_connect_to (int sock, unsigned long address, int port)
{
    struct sockaddr_in addr;
    struct hostent *hostent;
    char *ip4;
    int ret;
    
    if (CONFIG_BOOLEAN(config_proxy_use))
    {
        memset (&addr, 0, sizeof (addr));
        addr.sin_addr.s_addr = htonl (address);
        ip4 = inet_ntoa(addr.sin_addr);
        
        memset (&addr, 0, sizeof (addr));
        addr.sin_port = htons (CONFIG_INTEGER(config_proxy_port));
        addr.sin_family = AF_INET;
        hostent = gethostbyname (CONFIG_STRING(config_proxy_address));
        if (!hostent)
            return 0;
        memcpy(&(addr.sin_addr), *(hostent->h_addr_list), sizeof(struct in_addr));
        ret = connect (sock, (struct sockaddr *) &addr, sizeof (addr));
        if ((ret == -1) && (errno != EINPROGRESS))
            return 0;
        if (!network_pass_proxy (sock, ip4, port))
            return 0;
    }
    else
    {
        memset (&addr, 0, sizeof (addr));
        addr.sin_port = htons (port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (address);
        ret = connect (sock, (struct sockaddr *) &addr, sizeof (addr));
        if ((ret == -1) && (errno != EINPROGRESS))
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
    struct addrinfo hints, *res, *res_local, *ptr_res;
    char status_str[2], *ptr_address, *status_ok_with_address;
    char ipv4_address[INET_ADDRSTRLEN + 1], ipv6_address[INET6_ADDRSTRLEN + 1];
    char status_ok_without_address[1 + 5 + 1];
    int rc, length;
    
    res = NULL;
    res_local = NULL;
    
    status_str[1] = '\0';
    
    if (CONFIG_BOOLEAN(config_proxy_use))
    {
        /* get info about peer */
        memset (&hints, 0, sizeof (hints));
        hints.ai_family = (CONFIG_BOOLEAN(config_proxy_ipv6)) ? AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo (CONFIG_STRING(config_proxy_address), NULL, &hints, &res) !=0)
        {
            /* address not found */
            status_str[0] = '0' + WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND;
            write (HOOK_CONNECT(hook_connect, child_write), status_str, 1);
            return;
        }
        if (!res)
        {
            /* adddress not found */
            status_str[0] = '0' + WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND;
            write (HOOK_CONNECT(hook_connect, child_write), status_str, 1);
            return;
        }
        if ((CONFIG_BOOLEAN(config_proxy_ipv6) && (res->ai_family != AF_INET6))
            || ((!CONFIG_BOOLEAN(config_proxy_ipv6) && (res->ai_family != AF_INET))))
        {
            /* IP address not found */
            status_str[0] = '0' + WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND;
            write (HOOK_CONNECT(hook_connect, child_write), status_str, 1);
            freeaddrinfo (res);
            return;
        }
        
        if (CONFIG_BOOLEAN(config_proxy_ipv6))
            ((struct sockaddr_in6 *)(res->ai_addr))->sin6_port = htons (CONFIG_INTEGER(config_proxy_port));
        else
            ((struct sockaddr_in *)(res->ai_addr))->sin_port = htons (CONFIG_INTEGER(config_proxy_port));
        
        /* connect to peer */
        if (connect (HOOK_CONNECT(hook_connect, sock),
                     res->ai_addr, res->ai_addrlen) != 0)
        {
            /* connection refused */
            status_str[0] = '0' + WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED;
            write (HOOK_CONNECT(hook_connect, child_write), status_str, 1);
            freeaddrinfo (res);
            return;
        }
        
        if (!network_pass_proxy (HOOK_CONNECT(hook_connect, sock),
                                 HOOK_CONNECT(hook_connect, address),
                                 HOOK_CONNECT(hook_connect, port)))
        {
            /* proxy fails to connect to peer */
            status_str[0] = '0' + WEECHAT_HOOK_CONNECT_PROXY_ERROR;
            write (HOOK_CONNECT(hook_connect, child_write), status_str, 1);
            freeaddrinfo (res);
            return;
        }
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
            if ((rc != 0) || !res_local
                || (HOOK_CONNECT(hook_connect, ipv6)
                    && (res_local->ai_family != AF_INET6))
                || ((!HOOK_CONNECT(hook_connect, ipv6)
                     && (res_local->ai_family != AF_INET))))
            {
                /* fails to set local hostname/IP */
                status_str[0] = '0' + WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR;
                write (HOOK_CONNECT(hook_connect, child_write), status_str, 1);
                if (res_local)
                    freeaddrinfo (res_local);
                return;
            }
            if (bind (HOOK_CONNECT(hook_connect, sock),
                      res_local->ai_addr, res_local->ai_addrlen) < 0)
            {
                /* fails to set local hostname/IP */
                status_str[0] = '0' + WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR;
                write (HOOK_CONNECT(hook_connect, child_write), status_str, 1);
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
        if ((rc != 0) || !res)
        {
            /* address not found */
            status_str[0] = '0' + WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND;
            write (HOOK_CONNECT(hook_connect, child_write), status_str, 1);
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
            
            if (connect (HOOK_CONNECT(hook_connect, sock),
                         ptr_res->ai_addr, ptr_res->ai_addrlen) == 0)
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
        status_ok_with_address = NULL;
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
            status_ok_with_address = malloc (length);
            if (status_ok_with_address)
            {
                snprintf (status_ok_with_address, length, "%s%05d%s",
                          status_str, strlen (ptr_address), ptr_address);
            }
        }
        
        if (status_ok_with_address)
        {
            write (HOOK_CONNECT(hook_connect, child_write),
                   status_ok_with_address, strlen (status_ok_with_address));
            free (status_ok_with_address);
        }
        else
        {
            snprintf (status_ok_without_address, sizeof (status_ok_without_address),
                      "%s%05d", status_str, 0);
            write (HOOK_CONNECT(hook_connect, child_write),
                   status_ok_without_address, strlen (status_ok_without_address));
        }
    }
    else
    {
        write (HOOK_CONNECT(hook_connect, child_write), status_str, 1);
    }
    
    if (res)
        freeaddrinfo (res);
    if (res_local)
        freeaddrinfo (res_local);
}

/*
 * network_connect_child_read_cb: read connection progress from child process
 */

int
network_connect_child_read_cb (void *arg_hook_connect)
{
    struct t_hook *hook_connect;
    char buffer[1], buf_size_ip[6], *ip_address, *error;
    int num_read;
    long size_ip;
#ifdef HAVE_GNUTLS
    int rc;
#endif
    
    hook_connect = (struct t_hook *)arg_hook_connect;
    
    ip_address = NULL;
    
    num_read = read (HOOK_CONNECT(hook_connect, child_read),
                     buffer, sizeof (buffer));
    if (num_read > 0)
    {
        if (buffer[0] - '0' == WEECHAT_HOOK_CONNECT_OK)
        {
            buf_size_ip[5] = '\0';
            num_read = read (HOOK_CONNECT(hook_connect, child_read),
                             buf_size_ip, 5);
            if (num_read == 5)
            {
                error = NULL;
                size_ip = strtol (buf_size_ip, &error, 10);
                if (error && !error[0])
                {
                    ip_address = malloc (size_ip + 1);
                    if (ip_address)
                    {
                        num_read = read (HOOK_CONNECT(hook_connect, child_read),
                                         ip_address, size_ip);
                        if (num_read == size_ip)
                            ip_address[size_ip] = '\0';
                        else
                        {
                            free (ip_address);
                            ip_address = NULL;
                        }
                    }
                }
            }
#ifdef HAVE_GNUTLS
            if (HOOK_CONNECT(hook_connect, gnutls_sess))
            {
                gnutls_transport_set_ptr (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                          (gnutls_transport_ptr) ((unsigned long) HOOK_CONNECT(hook_connect, sock)));
                while (1)
                {
                    rc = gnutls_handshake (*HOOK_CONNECT(hook_connect, gnutls_sess));
                    if ((rc == GNUTLS_E_SUCCESS)
                        || ((rc != GNUTLS_E_AGAIN) && (rc != GNUTLS_E_INTERRUPTED)))
                        break;
                    usleep (1000);
                }
                if (rc != GNUTLS_E_SUCCESS)
                {
                    (void) (HOOK_CONNECT(hook_connect, callback))
                        (hook_connect->callback_data,
                         WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR,
                         ip_address);
                    unhook (hook_connect);
                    if (ip_address)
                        free (ip_address);
                    return WEECHAT_RC_OK;
                }
            }
#endif
        }
        (void) (HOOK_CONNECT(hook_connect, callback))
            (hook_connect->callback_data, buffer[0] - '0', ip_address);
        unhook (hook_connect);
    }
    
    if (ip_address)
        free (ip_address);
    
    return WEECHAT_RC_OK;
}

/*
 * network_connect_with_fork: connect with fork (called by hook_connect() only!)
 */

void
network_connect_with_fork (struct t_hook *hook_connect)
{
    int child_pipe[2];
#ifndef __CYGWIN__
    pid_t pid;
#endif
    
#ifdef HAVE_GNUTLS
    /* initialize GnuTLS if SSL asked */
    if (HOOK_CONNECT(hook_connect, gnutls_sess))
    {
        if (gnutls_init (HOOK_CONNECT(hook_connect, gnutls_sess), GNUTLS_CLIENT) != 0)
        {
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_data,
                 '0' + WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR,
                 NULL);
            unhook (hook_connect);
            return;
        }
        gnutls_set_default_priority (*HOOK_CONNECT(hook_connect, gnutls_sess));
        gnutls_certificate_type_set_priority (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                              gnutls_cert_type_prio);
        gnutls_protocol_set_priority (*HOOK_CONNECT(hook_connect, gnutls_sess),
                                      gnutls_prot_prio);
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
             '0' + WEECHAT_HOOK_CONNECT_MEMORY_ERROR,
             NULL);
        unhook (hook_connect);
        return;
    }
    HOOK_CONNECT(hook_connect, child_read) = child_pipe[0];
    HOOK_CONNECT(hook_connect, child_write) = child_pipe[1];
    
#ifdef __CYGWIN__
    /* connection may block under Cygwin, there's no other known way
       to do better today, since connect() in child process seems not to work
       any suggestion is welcome to improve that!
    */
    network_connect_child (hook_connect);
    network_connect_child_read_cb (hook_connect);
#else
    switch (pid = fork ())
    {
        /* fork failed */
        case -1:
            (void) (HOOK_CONNECT(hook_connect, callback))
                (hook_connect->callback_data,
                 '0' + WEECHAT_HOOK_CONNECT_MEMORY_ERROR,
                 NULL);
            unhook (hook_connect);
            return;
        /* child process */
        case 0:
            setuid (getuid ());
            network_connect_child (hook_connect);
            _exit (EXIT_SUCCESS);
    }
    /* parent process */
    HOOK_CONNECT(hook_connect, child_pid) = pid;
    HOOK_CONNECT(hook_connect, hook_fd) = hook_fd (NULL,
                                                   HOOK_CONNECT(hook_connect, child_read),
                                                   1, 0, 0,
                                                   network_connect_child_read_cb,
                                                   hook_connect);
#endif
}
