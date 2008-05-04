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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "weechat.h"
#include "wee-network.h"
#include "wee-config.h"
#include "wee-string.h"


/*
 * network_convbase64_8x3_to_6x4 : convert 3 bytes of 8 bits in 4 bytes of 6 bits
 */

void
network_convbase64_8x3_to_6x4 (char *from, char *to)
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
network_base64encode (char *from, char *to)
{
    char *f, *t;
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
 *                         return : 
 *                          - 0 if connexion throw proxy was successful
 *                          - 1 if connexion fails
 */

int
network_pass_httpproxy (int sock, char *address, int port)
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
        return 1;
    
    n = recv (sock, buffer, sizeof (buffer), 0);
    
    /* success result must be like: "HTTP/1.0 200 OK" */
    if (n < 12)
        return 1;
    
    if (memcmp (buffer, "HTTP/", 5) || memcmp (buffer + 9, "200", 3))
        return 1;
    
    return 0;
}

/*
 * network_resolve: resolve hostname on its IP address
 *                  (works with ipv4 and ipv6)
 *                  return :
 *                   - 0 if resolution was successful
 *                   - 1 if resolution fails
 */

int
network_resolve (char *hostname, char *ip, int *version)
{
    char ipbuffer[NI_MAXHOST];
    struct addrinfo *res;
    
    if (version != NULL)
        *version = 0;
    
    res = NULL;
    
    if (getaddrinfo (hostname, NULL, NULL, &res) != 0)
        return 1;
    
    if (!res)
        return 1;
    
    if (getnameinfo (res->ai_addr, res->ai_addrlen, ipbuffer, sizeof(ipbuffer),
                     NULL, 0, NI_NUMERICHOST) != 0)
    {
        freeaddrinfo (res);
        return 1;
    }
    
    if ((res->ai_family == AF_INET) && (version != NULL))
        *version = 4;
    if ((res->ai_family == AF_INET6) && (version != NULL))
        *version = 6;
    
    strcpy (ip, ipbuffer);
    
    freeaddrinfo (res);
    
    return 0;
}

/*
 * network_pass_socks4proxy: establish connection/authentification thru a
 *                           socks4 proxy
 *                           return : 
 *                            - 0 if connexion thru proxy was successful
 *                            - 1 if connexion fails
 */

int
network_pass_socks4proxy (int sock, char *address, int port)
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
    
    if (buffer[0] == 0 && buffer[1] == 90)
        return 0;
    
    return 1;
}

/*
 * network_pass_socks5proxy: establish connection/authentification thru a
 *                           socks5 proxy
 *                           return : 
 *                            - 0 if connexion thru proxy was successful
 *                            - 1 if connexion fails
 */

int
network_pass_socks5proxy (int sock, char *address, int port)
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
        return 1;
    
    if (CONFIG_STRING(config_proxy_username)
        && CONFIG_STRING(config_proxy_username)[0])
    {
        /* with authentication */
        /*   -> socks server must respond with :
         *       - socks version (buffer[0]) = 5 => socks5
         *       - socks method  (buffer[1]) = 2 => authentication
         */
        
        if (buffer[0] != 5 || buffer[1] != 2)
            return 1;
        
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
            return 1;
        
        /* buffer[1] = auth state, must be 0 for success */
        if (buffer[1] != 0)
            return 1;
    }
    else
    {
        /* without authentication */
        /*   -> socks server must respond with :
         *       - socks version (buffer[0]) = 5 => socks5
         *       - socks method  (buffer[1]) = 0 => no authentication
         */
        if (!(buffer[0] == 5 && buffer[1] == 0))
            return 1;
    }
    
    /* authentication successful then giving address/port to connect */
    addr_len = strlen(address);
    addr_buffer_len = 4 + 1 + addr_len + 2;
    addr_buffer = malloc (addr_buffer_len * sizeof(*addr_buffer));
    if (!addr_buffer)
        return 1;
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
        return 1;
    
    if (!(buffer[0] == 5 && buffer[1] == 0))
        return 1;
    
    /* buffer[3] = address type */
    switch (buffer[3])
    {
        case 1:
            /* ipv4 
             * server socks return server bound address and port
             * address of 4 bytes and port of 2 bytes (= 6 bytes)
             */
            if (recv (sock, buffer, 6, 0) != 6)
                return 1;
            break;
        case 3:
            /* domainname
             * server socks return server bound address and port
             */
            /* reading address length */
            if (recv (sock, buffer, 1, 0) != 1)
                return 1;    
            addr_len = buffer[0];
            /* reading address + port = addr_len + 2 */
            if (recv (sock, buffer, addr_len + 2, 0) != (addr_len + 2))
                return 1;
            break;
        case 4:
            /* ipv6
             * server socks return server bound address and port
             * address of 16 bytes and port of 2 bytes (= 18 bytes)
             */
            if (recv (sock, buffer, 18, 0) != 18)
                return 1;
            break;
        default:
            return 1;
    }
    
    return 0;
}

/*
 * network_pass_proxy: establish connection/authentification to a proxy
 *                     return : 
 *                      - 0 if connexion throw proxy was successful
 *                      - 1 if connexion fails
 */

int
network_pass_proxy (int sock, char *address, int port)
{
    int rc;
    
    rc = 1;
    if (CONFIG_BOOLEAN(config_proxy_type))
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
 *                     return 1 if ok, 0 if failed
 */

int
network_connect_to (int sock, unsigned long address, int port)
{
    struct sockaddr_in addr;
    struct hostent *hostent;
    char *ip4;
    int ret;
    
    if (CONFIG_BOOLEAN(config_proxy_type))
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
        memcpy(&(addr.sin_addr),*(hostent->h_addr_list), sizeof(struct in_addr));
        ret = connect (sock, (struct sockaddr *) &addr, sizeof (addr));
        if ((ret == -1) && (errno != EINPROGRESS))
            return 0;
        if (network_pass_proxy (sock, ip4, port) == -1)
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
