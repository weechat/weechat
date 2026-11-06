/*
 * SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test network functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "src/core/core-network.h"
#include "src/core/core-proxy.h"

extern int network_is_ip_address (const char *address);
extern int network_addrinfo_has_family (struct addrinfo *addrinfo,
                                        int family);
extern int network_send_with_retry (int sock, const void *buffer, int length,
                                    int flags);
extern int network_recv_with_retry (int sock, void *buffer, int length,
                                    int flags);
extern int network_pass_httpproxy (struct t_proxy *proxy, int sock,
                                   const char *address, int port);
extern int network_resolve (const char *hostname, char *ip, int *version);
extern int network_pass_socks4proxy (struct t_proxy *proxy, int sock,
                                     const char *address, int port);
extern int network_pass_socks5proxy (struct t_proxy *proxy, int sock,
                                     const char *address, int port);
extern int network_connect (int sock, const struct sockaddr *addr,
                            socklen_t addrlen);
}

TEST_GROUP(CoreNetwork)
{
};

/*
 * Test functions:
 *   network_init_gcrypt
 */

TEST(CoreNetwork, InitGcrypt)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_load_system_ca_file
 */

TEST(CoreNetwork, LoadSystemCaFile)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_load_user_ca_files
 */

TEST(CoreNetwork, LoadUserCaFiles)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_load_ca_files
 */

TEST(CoreNetwork, LoadCaFiles)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_reload_ca_files
 */

TEST(CoreNetwork, ReloadCaFiles)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_init_gnutls
 */

TEST(CoreNetwork, InitGnutls)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_end
 */

TEST(CoreNetwork, End)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_is_ip_address
 */

TEST(CoreNetwork, IsIpAddress)
{
    /* Invalid address */
    LONGS_EQUAL(0, network_is_ip_address (NULL));
    LONGS_EQUAL(0, network_is_ip_address (""));
    LONGS_EQUAL(0, network_is_ip_address ("abc"));
    LONGS_EQUAL(0, network_is_ip_address ("1"));
    LONGS_EQUAL(0, network_is_ip_address ("1.2"));
    LONGS_EQUAL(0, network_is_ip_address ("1.2.3"));
    LONGS_EQUAL(0, network_is_ip_address ("1.2.3.a"));
    LONGS_EQUAL(0, network_is_ip_address ("1.2.3.4.5"));
#ifndef __APPLE__
    /* inet_pton on macOS accepts leading zeros. */
    LONGS_EQUAL(0, network_is_ip_address ("001.002.003.004"));
#endif

    /* Valid IPv4 */
    LONGS_EQUAL(1, network_is_ip_address ("127.0.0.1"));
    LONGS_EQUAL(1, network_is_ip_address ("1.2.3.4"));

    /* Valid IPv6 */
    LONGS_EQUAL(1, network_is_ip_address ("::1"));
    LONGS_EQUAL(1, network_is_ip_address ("2001:0db8:0000:85a3:0000:0000:ac1f:8001"));
    LONGS_EQUAL(1, network_is_ip_address ("2001:db8:0:85a3:0:0:ac1f:8001"));
    LONGS_EQUAL(1, network_is_ip_address ("2001:db8:0:85a3::ac1f:8001"));
}

/*
 * Test functions:
 *   network_addrinfo_has_family
 */

TEST(CoreNetwork, AddrinfoHasFamily)
{
    struct addrinfo addr1, addr2;

    memset (&addr1, 0, sizeof (addr1));
    memset (&addr2, 0, sizeof (addr2));

    /* Empty list */
    LONGS_EQUAL(0, network_addrinfo_has_family (NULL, AF_INET));
    LONGS_EQUAL(0, network_addrinfo_has_family (NULL, AF_INET6));

    /* One IPv4 address */
    addr1.ai_family = AF_INET;
    LONGS_EQUAL(1, network_addrinfo_has_family (&addr1, AF_INET));
    LONGS_EQUAL(0, network_addrinfo_has_family (&addr1, AF_INET6));

    /* IPv4 + IPv6 addresses */
    addr2.ai_family = AF_INET6;
    addr1.ai_next = &addr2;
    LONGS_EQUAL(1, network_addrinfo_has_family (&addr1, AF_INET));
    LONGS_EQUAL(1, network_addrinfo_has_family (&addr1, AF_INET6));
    LONGS_EQUAL(0, network_addrinfo_has_family (&addr1, AF_UNSPEC));
}

/*
 * Test functions:
 *   network_connect_child
 */

TEST(CoreNetwork, ConnectChild)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_connect_child_timer_cb
 */

TEST(CoreNetwork, ConnectChildTimerCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_connect_gnutls_handshake_fd_cb
 */

TEST(CoreNetwork, ConnectGnutlsHandshakeFdCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_connect_gnutls_handshake_timer_cb
 */

TEST(CoreNetwork, ConnectGnutlsHandshakeTimerCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_connect_child_read_cb
 */

TEST(CoreNetwork, ConnectChildReadCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   network_connect_with_fork
 */

TEST(CoreNetwork, ConnectWithFork)
{
    /* TODO: write tests */
}

TEST_GROUP(CoreNetworkWithSocket)
{
    int sock;
    int sock_peer;
    unsigned char data_sent[8192];
    int data_sent_size;
    struct t_proxy *proxy;

    /*
     * Write the reply of the peer (fake proxy) in advance, then shut down the
     * peer for writing: the function tested gets the data then end of file,
     * so it can never block.
     */

    void peer_reply (const void *data, int size)
    {
        if (size > 0)
            LONGS_EQUAL(size, send (sock_peer, data, size, 0));
        LONGS_EQUAL(0, shutdown (sock_peer, SHUT_WR));
    }

    /*
     * Read all data sent to the peer.
     */

    void peer_read ()
    {
        int num_read;

        data_sent_size = 0;
        while (data_sent_size < (int)sizeof (data_sent))
        {
            num_read = recv (sock_peer, data_sent + data_sent_size,
                             sizeof (data_sent) - data_sent_size,
                             MSG_DONTWAIT);
            if (num_read <= 0)
                break;
            data_sent_size += num_read;
        }
    }

    void new_proxy (const char *type, const char *username,
                    const char *password)
    {
        proxy_free (proxy);
        proxy = proxy_new ("test_proxy", type, "off", "127.0.0.1", "1080",
                           username, password);
        CHECK(proxy);
    }

    void setup ()
    {
        int sv[2];

        LONGS_EQUAL(0, socketpair (AF_UNIX, SOCK_STREAM, 0, sv));
        sock = sv[0];
        sock_peer = sv[1];
        data_sent_size = 0;
        proxy = NULL;
    }

    void teardown ()
    {
        close (sock);
        close (sock_peer);
        proxy_free (proxy);
    }
};

/*
 * Test functions:
 *   network_send_with_retry
 */

TEST(CoreNetworkWithSocket, SendWithRetry)
{
    LONGS_EQUAL(4, network_send_with_retry (sock, "test", 4, 0));
    peer_read ();
    LONGS_EQUAL(4, data_sent_size);
    MEMCMP_EQUAL("test", data_sent, 4);

    /* Peer closed: error, nothing sent. */
    close (sock_peer);
    sock_peer = socket (AF_UNIX, SOCK_STREAM, 0);
    LONGS_EQUAL(0, network_send_with_retry (sock, "test", 4, 0));
}

/*
 * Test functions:
 *   network_recv_with_retry
 */

TEST(CoreNetworkWithSocket, RecvWithRetry)
{
    char buffer[16];

    peer_reply ("hello", 5);

    /* Data available: returned (up to the size asked). */
    LONGS_EQUAL(2, network_recv_with_retry (sock, buffer, 2, 0));
    MEMCMP_EQUAL("he", buffer, 2);
    LONGS_EQUAL(3, network_recv_with_retry (sock, buffer, sizeof (buffer), 0));
    MEMCMP_EQUAL("llo", buffer, 3);

    /* End of file. */
    LONGS_EQUAL(0, network_recv_with_retry (sock, buffer, sizeof (buffer), 0));
}

/*
 * Test functions:
 *   network_pass_httpproxy
 */

TEST(CoreNetworkWithSocket, PassHttpproxy)
{
    const char *reply_ok = "HTTP/1.1 200 Connection established\r\n\r\n";

    /* Without authentication. */
    new_proxy ("http", "", "");
    peer_reply (reply_ok, strlen (reply_ok));
    LONGS_EQUAL(1, network_pass_httpproxy (proxy, sock, "irc.example.org",
                                           6697));
    peer_read ();
    data_sent[data_sent_size] = '\0';
    STRCMP_EQUAL("CONNECT irc.example.org:6697 HTTP/1.0\r\n\r\n",
                 (char *)data_sent);
}

TEST(CoreNetworkWithSocket, PassHttpproxyAuth)
{
    const char *reply_ok = "HTTP/1.0 200 OK\r\n\r\n";

    /* With authentication (username and password are evaluated). */
    new_proxy ("http", "alice", "${env:WEECHAT_TEST_PROXY_PASSWORD}");
    setenv ("WEECHAT_TEST_PROXY_PASSWORD", "secret", 1);
    peer_reply (reply_ok, strlen (reply_ok));
    LONGS_EQUAL(1, network_pass_httpproxy (proxy, sock, "127.0.0.1", 6667));
    unsetenv ("WEECHAT_TEST_PROXY_PASSWORD");
    peer_read ();
    data_sent[data_sent_size] = '\0';
    /* "alice:secret" in base64 */
    STRCMP_EQUAL("CONNECT 127.0.0.1:6667 HTTP/1.0\r\n"
                 "Proxy-Authorization: Basic YWxpY2U6c2VjcmV0\r\n\r\n",
                 (char *)data_sent);
}

TEST(CoreNetworkWithSocket, PassHttpproxyErrors)
{
    const char *reply_denied = "HTTP/1.1 407 Proxy Authentication Required\r\n\r\n";

    new_proxy ("http", "", "");

    /* Connection refused by proxy. */
    peer_reply (reply_denied, strlen (reply_denied));
    LONGS_EQUAL(0, network_pass_httpproxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassHttpproxyShortReply)
{
    new_proxy ("http", "", "");

    /* Reply too short, then not HTTP. */
    peer_reply ("HTTP/1.1 20", 11);
    LONGS_EQUAL(0, network_pass_httpproxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassHttpproxyNotHttp)
{
    new_proxy ("http", "", "");

    peer_reply ("SSH-2.0-OpenSSH_9.9\r\n", 21);
    LONGS_EQUAL(0, network_pass_httpproxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassHttpproxyNoReply)
{
    new_proxy ("http", "", "");

    /* Connection closed by proxy. */
    peer_reply (NULL, 0);
    LONGS_EQUAL(0, network_pass_httpproxy (proxy, sock, "127.0.0.1", 6667));
}

/*
 * Test functions:
 *   network_pass_socks4proxy
 */

TEST(CoreNetworkWithSocket, PassSocks4proxy)
{
    const unsigned char reply_ok[8] = { 0, 90, 0, 0, 0, 0, 0, 0 };

    new_proxy ("socks4", "alice", "");
    peer_reply (reply_ok, sizeof (reply_ok));
    LONGS_EQUAL(1, network_pass_socks4proxy (proxy, sock, "192.168.1.2",
                                             6667));

    /* Request: version, command, port, IPv4 address, user + '\0'. */
    peer_read ();
    LONGS_EQUAL(8 + 6, data_sent_size);
    BYTES_EQUAL(4, data_sent[0]);
    BYTES_EQUAL(1, data_sent[1]);
    BYTES_EQUAL(0x1A, data_sent[2]);
    BYTES_EQUAL(0x0B, data_sent[3]);
    BYTES_EQUAL(192, data_sent[4]);
    BYTES_EQUAL(168, data_sent[5]);
    BYTES_EQUAL(1, data_sent[6]);
    BYTES_EQUAL(2, data_sent[7]);
    MEMCMP_EQUAL("alice", data_sent + 8, 6);
}

TEST(CoreNetworkWithSocket, PassSocks4proxyErrors)
{
    const unsigned char reply_rejected[8] = { 0, 91, 0, 0, 0, 0, 0, 0 };

    new_proxy ("socks4", "", "");

    /* Request rejected by proxy. */
    peer_reply (reply_rejected, sizeof (reply_rejected));
    LONGS_EQUAL(0, network_pass_socks4proxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassSocks4proxyShortReply)
{
    const unsigned char reply_short[1] = { 0 };

    new_proxy ("socks4", "", "");

    peer_reply (reply_short, sizeof (reply_short));
    LONGS_EQUAL(0, network_pass_socks4proxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassSocks4proxyIpv6)
{
    new_proxy ("socks4", "", "");

    /* IPv6 address: not supported by socks4, nothing sent. */
    peer_reply (NULL, 0);
    LONGS_EQUAL(0, network_pass_socks4proxy (proxy, sock, "::1", 6667));
    peer_read ();
    LONGS_EQUAL(0, data_sent_size);
}

TEST(CoreNetworkWithSocket, PassSocks4proxyUsernameMaxLength)
{
    const unsigned char reply_ok[8] = { 0, 90, 0, 0, 0, 0, 0, 0 };
    char username[256];

    /* Username with 127 chars: OK (max length). */
    memset (username, 'a', 127);
    username[127] = '\0';
    new_proxy ("socks4", username, "");
    peer_reply (reply_ok, sizeof (reply_ok));
    LONGS_EQUAL(1, network_pass_socks4proxy (proxy, sock, "127.0.0.1", 6667));
    peer_read ();
    LONGS_EQUAL(8 + 127 + 1, data_sent_size);
    MEMCMP_EQUAL(username, data_sent + 8, 128);
}

TEST(CoreNetworkWithSocket, PassSocks4proxyUsernameTooLong)
{
    char username[256];

    /* Username with 128 chars: too long, nothing sent. */
    memset (username, 'a', 128);
    username[128] = '\0';
    new_proxy ("socks4", username, "");
    peer_reply (NULL, 0);
    LONGS_EQUAL(0, network_pass_socks4proxy (proxy, sock, "127.0.0.1", 6667));
    peer_read ();
    LONGS_EQUAL(0, data_sent_size);
}

TEST(CoreNetworkWithSocket, PassSocks4proxyResolveError)
{
    new_proxy ("socks4", "", "");

    /* Address can not be resolved: nothing sent. */
    peer_reply (NULL, 0);
    LONGS_EQUAL(0, network_pass_socks4proxy (proxy, sock, "", 6667));
    peer_read ();
    LONGS_EQUAL(0, data_sent_size);
}

/*
 * Test functions:
 *   network_pass_socks5proxy
 */

TEST(CoreNetworkWithSocket, PassSocks5proxy)
{
    /* Method "no authentication", then success with IPv4 bound address. */
    const unsigned char reply_ok[] = {
        5, 0,
        5, 0, 0, 1, 10, 0, 0, 1, 0x1A, 0x0B,
    };

    new_proxy ("socks5", "", "");
    peer_reply (reply_ok, sizeof (reply_ok));
    LONGS_EQUAL(1, network_pass_socks5proxy (proxy, sock, "irc.example.org",
                                             6697));

    /* Greeting (3 bytes), then connect request with domain name. */
    peer_read ();
    LONGS_EQUAL(3 + 4 + 1 + 15 + 2, data_sent_size);
    BYTES_EQUAL(5, data_sent[0]);
    BYTES_EQUAL(1, data_sent[1]);
    BYTES_EQUAL(0, data_sent[2]);
    BYTES_EQUAL(5, data_sent[3]);
    BYTES_EQUAL(1, data_sent[4]);
    BYTES_EQUAL(0, data_sent[5]);
    BYTES_EQUAL(3, data_sent[6]);
    BYTES_EQUAL(15, data_sent[7]);
    MEMCMP_EQUAL("irc.example.org", data_sent + 8, 15);
    BYTES_EQUAL(0x1A, data_sent[23]);
    BYTES_EQUAL(0x29, data_sent[24]);
}

TEST(CoreNetworkWithSocket, PassSocks5proxyAuth)
{
    /*
     * Method "username/password", authentication OK, then success with
     * domain name and IPv6 bound addresses.
     */
    const unsigned char reply_ok[] = {
        5, 2,
        1, 0,
        5, 0, 0, 3, 4, 'h', 'o', 's', 't', 0x1A, 0x0B,
    };

    new_proxy ("socks5", "alice", "secret");
    peer_reply (reply_ok, sizeof (reply_ok));
    LONGS_EQUAL(1, network_pass_socks5proxy (proxy, sock, "127.0.0.1", 6667));

    /* Greeting, then authentication (RFC 1929), then connect request. */
    peer_read ();
    BYTES_EQUAL(5, data_sent[0]);
    BYTES_EQUAL(1, data_sent[1]);
    BYTES_EQUAL(2, data_sent[2]);
    BYTES_EQUAL(1, data_sent[3]);
    BYTES_EQUAL(5, data_sent[4]);
    MEMCMP_EQUAL("alice", data_sent + 5, 5);
    BYTES_EQUAL(6, data_sent[10]);
    MEMCMP_EQUAL("secret", data_sent + 11, 6);
    BYTES_EQUAL(5, data_sent[17]);
}

TEST(CoreNetworkWithSocket, PassSocks5proxyIpv6Bound)
{
    unsigned char reply_ok[2 + 4 + 18];

    memset (reply_ok, 0, sizeof (reply_ok));
    reply_ok[0] = 5;
    reply_ok[1] = 0;
    reply_ok[2] = 5;
    reply_ok[3] = 0;
    reply_ok[5] = 4;

    new_proxy ("socks5", "", "");
    peer_reply (reply_ok, sizeof (reply_ok));
    LONGS_EQUAL(1, network_pass_socks5proxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassSocks5proxyWrongMethod)
{
    /* Proxy asks for authentication, no username set. */
    const unsigned char reply[] = { 5, 2 };

    new_proxy ("socks5", "", "");
    peer_reply (reply, sizeof (reply));
    LONGS_EQUAL(0, network_pass_socks5proxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassSocks5proxyAuthFailed)
{
    const unsigned char reply[] = { 5, 2, 1, 1 };

    new_proxy ("socks5", "alice", "wrong");
    peer_reply (reply, sizeof (reply));
    LONGS_EQUAL(0, network_pass_socks5proxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassSocks5proxyCredentialsTooLong)
{
    const unsigned char reply[] = { 5, 2 };
    char username[300];

    /* Username longer than 255 bytes (RFC 1929): refused. */
    memset (username, 'a', sizeof (username) - 1);
    username[sizeof (username) - 1] = '\0';
    new_proxy ("socks5", username, "secret");
    peer_reply (reply, sizeof (reply));
    LONGS_EQUAL(0, network_pass_socks5proxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassSocks5proxyConnectFailed)
{
    /* Connection refused by destination host (reply code 5). */
    const unsigned char reply[] = { 5, 0, 5, 5, 0, 1 };

    new_proxy ("socks5", "", "");
    peer_reply (reply, sizeof (reply));
    LONGS_EQUAL(0, network_pass_socks5proxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassSocks5proxyInvalidAddressType)
{
    const unsigned char reply[] = { 5, 0, 5, 0, 0, 9 };

    new_proxy ("socks5", "", "");
    peer_reply (reply, sizeof (reply));
    LONGS_EQUAL(0, network_pass_socks5proxy (proxy, sock, "127.0.0.1", 6667));
}

TEST(CoreNetworkWithSocket, PassSocks5proxyTruncatedReply)
{
    /* IPv4 bound address truncated. */
    const unsigned char reply[] = { 5, 0, 5, 0, 0, 1, 10, 0 };

    new_proxy ("socks5", "", "");
    peer_reply (reply, sizeof (reply));
    LONGS_EQUAL(0, network_pass_socks5proxy (proxy, sock, "127.0.0.1", 6667));
}

/*
 * Test functions:
 *   network_pass_proxy
 */

TEST(CoreNetworkWithSocket, PassProxy)
{
    const char *reply_ok = "HTTP/1.1 200 OK\r\n\r\n";

    /* Unknown proxy. */
    LONGS_EQUAL(0, network_pass_proxy ("unknown_proxy", sock, "127.0.0.1",
                                       6667));

    new_proxy ("http", "", "");
    peer_reply (reply_ok, strlen (reply_ok));
    LONGS_EQUAL(1, network_pass_proxy ("test_proxy", sock, "127.0.0.1",
                                       6667));
}

TEST(CoreNetworkWithSocket, PassProxySocks5)
{
    const unsigned char reply_ok[] = { 5, 0, 5, 0, 0, 1, 1, 2, 3, 4, 0, 1 };

    new_proxy ("socks5", "", "");
    peer_reply (reply_ok, sizeof (reply_ok));
    LONGS_EQUAL(1, network_pass_proxy ("test_proxy", sock, "127.0.0.1",
                                       6667));
}

TEST(CoreNetworkWithSocket, PassProxySocks4)
{
    const unsigned char reply_ok[8] = { 0, 90, 0, 0, 0, 0, 0, 0 };

    new_proxy ("socks4", "", "");
    peer_reply (reply_ok, sizeof (reply_ok));
    LONGS_EQUAL(1, network_pass_proxy ("test_proxy", sock, "127.0.0.1",
                                       6667));
}

/*
 * Test functions:
 *   network_resolve
 */

TEST(CoreNetwork, Resolve)
{
    char ip[NI_MAXHOST];
    int version;
    struct addrinfo *res;

    /* Numeric addresses only (no DNS query). */
    ip[0] = '\0';
    LONGS_EQUAL(1, network_resolve ("127.0.0.1", ip, &version));
    STRCMP_EQUAL("127.0.0.1", ip);
    LONGS_EQUAL(4, version);

    /*
     * IPv6 address: checked only if the system can resolve it (without hints,
     * getaddrinfo ignores IPv6 addresses if no IPv6 address is configured).
     */
    res = NULL;
    if (getaddrinfo ("::1", NULL, NULL, &res) == 0)
    {
        freeaddrinfo (res);
        LONGS_EQUAL(1, network_resolve ("::1", ip, &version));
        STRCMP_EQUAL("::1", ip);
        LONGS_EQUAL(6, version);
    }
    else
    {
        LONGS_EQUAL(0, network_resolve ("::1", ip, &version));
    }

    LONGS_EQUAL(1, network_resolve ("10.1.2.3", ip, NULL));
    STRCMP_EQUAL("10.1.2.3", ip);
}

/*
 * Test functions:
 *   network_connect
 *   network_connect_to
 */

TEST(CoreNetwork, Connect)
{
    struct sockaddr_in addr, addr_closed;
    socklen_t addr_len;
    int sock_listen, sock_closed, sock, flags;
    char str_port[16];
    struct t_proxy *proxy;

    /* Listening socket on a random port. */
    sock_listen = socket (AF_INET, SOCK_STREAM, 0);
    CHECK(sock_listen >= 0);
    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    addr.sin_port = 0;
    LONGS_EQUAL(0, bind (sock_listen, (struct sockaddr *)&addr, sizeof (addr)));
    LONGS_EQUAL(0, listen (sock_listen, 5));
    addr_len = sizeof (addr);
    LONGS_EQUAL(0, getsockname (sock_listen, (struct sockaddr *)&addr,
                                &addr_len));

    /* Port with nothing listening (socket bound then closed). */
    sock_closed = socket (AF_INET, SOCK_STREAM, 0);
    CHECK(sock_closed >= 0);
    memset (&addr_closed, 0, sizeof (addr_closed));
    addr_closed.sin_family = AF_INET;
    addr_closed.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    LONGS_EQUAL(0, bind (sock_closed, (struct sockaddr *)&addr_closed,
                         sizeof (addr_closed)));
    addr_len = sizeof (addr_closed);
    LONGS_EQUAL(0, getsockname (sock_closed, (struct sockaddr *)&addr_closed,
                                &addr_len));
    close (sock_closed);

    /* Blocking socket. */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    LONGS_EQUAL(1, network_connect (sock, (struct sockaddr *)&addr,
                                    sizeof (addr)));
    close (sock);
    sock = socket (AF_INET, SOCK_STREAM, 0);
    LONGS_EQUAL(0, network_connect (sock, (struct sockaddr *)&addr_closed,
                                    sizeof (addr_closed)));
    close (sock);

    /* Non-blocking socket. */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    flags = fcntl (sock, F_GETFL);
    fcntl (sock, F_SETFL, flags | O_NONBLOCK);
    LONGS_EQUAL(1, network_connect (sock, (struct sockaddr *)&addr,
                                    sizeof (addr)));
    close (sock);
    sock = socket (AF_INET, SOCK_STREAM, 0);
    fcntl (sock, F_SETFL, flags | O_NONBLOCK);
    LONGS_EQUAL(0, network_connect (sock, (struct sockaddr *)&addr_closed,
                                    sizeof (addr_closed)));
    close (sock);

    /* Connect without proxy. */
    LONGS_EQUAL(-1, network_connect_to (NULL, NULL, 0));
    LONGS_EQUAL(-1, network_connect_to (NULL, (struct sockaddr *)&addr, 0));
    sock = network_connect_to (NULL, (struct sockaddr *)&addr, sizeof (addr));
    CHECK(sock >= 0);
    close (sock);
    sock = network_connect_to ("", (struct sockaddr *)&addr, sizeof (addr));
    CHECK(sock >= 0);
    close (sock);
    LONGS_EQUAL(-1, network_connect_to (NULL, (struct sockaddr *)&addr_closed,
                                        sizeof (addr_closed)));

    /* Unknown proxy. */
    LONGS_EQUAL(-1, network_connect_to ("unknown_proxy",
                                        (struct sockaddr *)&addr,
                                        sizeof (addr)));

    /* Proxy not reachable (nothing listening on its port). */
    snprintf (str_port, sizeof (str_port), "%d", ntohs (addr_closed.sin_port));
    proxy = proxy_new ("test_proxy", "http", "off", "127.0.0.1", str_port,
                       "", "");
    CHECK(proxy);
    LONGS_EQUAL(-1, network_connect_to ("test_proxy",
                                        (struct sockaddr *)&addr,
                                        sizeof (addr)));
    proxy_free (proxy);

    close (sock_listen);
}
