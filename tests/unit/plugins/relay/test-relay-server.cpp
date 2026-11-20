/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay server functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "src/core/weechat.h"
#include "src/core/core-config-file.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/plugins/plugin.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-server.h"

extern char *relay_server_eval_path (const char *path);
}

#define TEST_SOCKET1 "%h/test_relay_server1.sock"
#define TEST_SOCKET2 "%h/test_relay_server2.sock"

#define WEE_CHECK_PROTOCOL_ARGS(__ipv4, __ipv6, __tls, __unix,          \
                                __protocol, __args, __string)           \
    relay_server_get_protocol_args (__string, &ipv4, &ipv6, &tls,       \
                                    &unix_socket, &protocol, &args);    \
    LONGS_EQUAL(__ipv4, ipv4);                                          \
    LONGS_EQUAL(__ipv6, ipv6);                                          \
    LONGS_EQUAL(__tls, tls);                                            \
    LONGS_EQUAL(__unix, unix_socket);                                   \
    STRCMP_EQUAL(__protocol, protocol);                                 \
    STRCMP_EQUAL(__args, args);                                         \
    free (protocol);                                                    \
    free (args);

TEST_GROUP(RelayServer)
{
    void setup ()
    {
        /* Disable auto-open of relay buffer (it would pollute other tests). */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);
    }

    void teardown ()
    {
        config_file_option_reset (relay_config_network_ipv6, 1);
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);
    }

    /* Create a server listening on a UNIX socket. */

    struct t_relay_server *new_unix (const char *protocol_string,
                                     enum t_relay_protocol protocol,
                                     const char *path)
    {
        return relay_server_new (protocol_string, protocol, NULL,
                                 -1,  /* port */
                                 path,
                                 0,  /* ipv4 */
                                 0,  /* ipv6 */
                                 0,  /* tls */
                                 1);  /* unix_socket */
    }

    /* Return the evaluated path of a socket. */

    char *eval_path (const char *path)
    {
        char *result;

        result = NULL;
        if (strncmp (path, "%h/", 3) == 0)
            string_asprintf (&result, "%s/%s", weechat_runtime_dir, path + 3);
        return result;
    }
};

/*
 * Test functions:
 *   relay_server_get_protocol_args
 */

TEST(RelayServer, GetProtocolArgs)
{
    int ipv4, ipv6, tls, unix_socket;
    char *protocol, *args;

    config_file_option_set (relay_config_network_ipv6, "on", 1);

    WEE_CHECK_PROTOCOL_ARGS(1, 1, 0, 0, "", NULL, "");
    WEE_CHECK_PROTOCOL_ARGS(1, 1, 0, 0, "irc", "libera", "irc.libera");
    WEE_CHECK_PROTOCOL_ARGS(1, 1, 1, 0, "irc", "libera", "tls.irc.libera");
    WEE_CHECK_PROTOCOL_ARGS(1, 0, 0, 0, "irc", "libera", "ipv4.irc.libera");
    WEE_CHECK_PROTOCOL_ARGS(0, 1, 0, 0, "irc", "libera", "ipv6.irc.libera");
    WEE_CHECK_PROTOCOL_ARGS(1, 1, 0, 0, "irc", "libera",
                            "ipv4.ipv6.irc.libera");
    WEE_CHECK_PROTOCOL_ARGS(0, 1, 1, 0, "irc", "libera",
                            "ipv6.tls.irc.libera");
    WEE_CHECK_PROTOCOL_ARGS(0, 1, 1, 0, "irc", "libera",
                            "tls.ipv6.irc.libera");
    WEE_CHECK_PROTOCOL_ARGS(1, 1, 0, 0, "irc", "server.with.dots",
                            "irc.server.with.dots");
    WEE_CHECK_PROTOCOL_ARGS(1, 1, 0, 0, "weechat", NULL, "weechat");
    WEE_CHECK_PROTOCOL_ARGS(1, 1, 1, 0, "weechat", NULL, "tls.weechat");
    WEE_CHECK_PROTOCOL_ARGS(0, 1, 1, 0, "weechat", NULL, "ipv6.tls.weechat");
    WEE_CHECK_PROTOCOL_ARGS(0, 0, 0, 1, "weechat", NULL, "unix.weechat");
    WEE_CHECK_PROTOCOL_ARGS(0, 0, 1, 1, "weechat", NULL, "unix.tls.weechat");
    WEE_CHECK_PROTOCOL_ARGS(1, 1, 1, 0, "api", NULL, "tls.api");
    WEE_CHECK_PROTOCOL_ARGS(0, 0, 0, 1, "api", NULL, "unix.api");
    WEE_CHECK_PROTOCOL_ARGS(1, 1, 0, 0, "xxx", NULL, "xxx");

    /* Default for IPv6 is the value of option relay.network.ipv6. */
    config_file_option_set (relay_config_network_ipv6, "off", 1);
    WEE_CHECK_PROTOCOL_ARGS(1, 0, 0, 0, "weechat", NULL, "weechat");
    WEE_CHECK_PROTOCOL_ARGS(1, 0, 1, 0, "weechat", NULL, "tls.weechat");
    WEE_CHECK_PROTOCOL_ARGS(0, 1, 0, 0, "weechat", NULL, "ipv6.weechat");

    /* Output pointers can be NULL. */
    relay_server_get_protocol_args ("tls.irc.libera",
                                    NULL, NULL, NULL, NULL, NULL, NULL);
}

/*
 * Test functions:
 *   relay_server_eval_path
 */

TEST(RelayServer, EvalPath)
{
    char *path1, *str;

    path1 = eval_path (TEST_SOCKET1);
    CHECK(path1);

    POINTERS_EQUAL(NULL, relay_server_eval_path (NULL));
    WEE_TEST_STR("", relay_server_eval_path (""));
    WEE_TEST_STR("/tmp/relay.sock", relay_server_eval_path ("/tmp/relay.sock"));
    WEE_TEST_STR(path1, relay_server_eval_path (TEST_SOCKET1));

    free (path1);
}

/*
 * Test functions:
 *   relay_server_new
 *   relay_server_search
 *   relay_server_search_port
 *   relay_server_search_path
 *   relay_server_free
 */

TEST(RelayServer, NewSearchFree)
{
    struct t_relay_server *server1, *server2, *server3;
    char *path1, *path2;

    path1 = eval_path (TEST_SOCKET1);
    CHECK(path1);
    path2 = eval_path (TEST_SOCKET2);
    CHECK(path2);

    POINTERS_EQUAL(NULL, relay_server_new (NULL, RELAY_PROTOCOL_WEECHAT,
                                           NULL, -1, TEST_SOCKET1,
                                           0, 0, 0, 1));

    server1 = new_unix ("unix.weechat", RELAY_PROTOCOL_WEECHAT, TEST_SOCKET1);
    CHECK(server1);
    STRCMP_EQUAL("unix.weechat", server1->protocol_string);
    LONGS_EQUAL(RELAY_PROTOCOL_WEECHAT, server1->protocol);
    POINTERS_EQUAL(NULL, server1->protocol_args);
    LONGS_EQUAL(-1, server1->port);
    STRCMP_EQUAL(path1, server1->path);
    LONGS_EQUAL(1, server1->unix_socket);
    CHECK(server1->sock >= 0);
    CHECK(server1->hook_fd);
    CHECK(server1->start_time > 0);
    LONGS_EQUAL(0, access (path1, F_OK));

    server2 = new_unix ("unix.api", RELAY_PROTOCOL_API, TEST_SOCKET2);
    CHECK(server2);

    /* Path already used: server not created. */
    POINTERS_EQUAL(NULL, new_unix ("unix.irc.test", RELAY_PROTOCOL_IRC,
                                   path1));
    POINTERS_EQUAL(NULL, new_unix ("unix.irc.test", RELAY_PROTOCOL_IRC,
                                   TEST_SOCKET1));

    /* TCP server on a port chosen by the system (port 0). */
    server3 = relay_server_new ("ipv4.irc.test", RELAY_PROTOCOL_IRC, "test",
                                0, "0", 1, 0, 0, 0);
    CHECK(server3);
    STRCMP_EQUAL("test", server3->protocol_args);
    LONGS_EQUAL(0, server3->unix_socket);

    /* Port already used: server not created. */
    POINTERS_EQUAL(NULL, relay_server_new ("ipv4.api", RELAY_PROTOCOL_API,
                                           NULL, 0, "0", 1, 0, 0, 0));

    /* Search by protocol and arguments. */
    POINTERS_EQUAL(NULL, relay_server_search (NULL));
    POINTERS_EQUAL(NULL, relay_server_search (""));
    POINTERS_EQUAL(NULL, relay_server_search ("weechat"));
    POINTERS_EQUAL(server1, relay_server_search ("unix.weechat"));
    POINTERS_EQUAL(server2, relay_server_search ("unix.api"));
    POINTERS_EQUAL(server3, relay_server_search ("ipv4.irc.test"));

    /* Search by port. */
    POINTERS_EQUAL(server3, relay_server_search_port (0));
    POINTERS_EQUAL(NULL, relay_server_search_port (12345));

    /* Search by path (only for UNIX sockets). */
    POINTERS_EQUAL(NULL, relay_server_search_path (NULL));
    POINTERS_EQUAL(NULL, relay_server_search_path (""));
    POINTERS_EQUAL(server1, relay_server_search_path (path1));
    POINTERS_EQUAL(server2, relay_server_search_path (path2));
    /* The path is evaluated before the search. */
    POINTERS_EQUAL(server1, relay_server_search_path (TEST_SOCKET1));
    POINTERS_EQUAL(server2, relay_server_search_path (TEST_SOCKET2));
    POINTERS_EQUAL(NULL, relay_server_search_path ("0"));

    /* Free servers: socket files are removed. */
    relay_server_free (NULL);
    relay_server_free (server1);
    POINTERS_EQUAL(NULL, relay_server_search ("unix.weechat"));
    LONGS_EQUAL(-1, access (path1, F_OK));
    relay_server_free (server2);
    relay_server_free (server3);
    POINTERS_EQUAL(NULL, relay_server_search ("unix.api"));
    POINTERS_EQUAL(NULL, relay_server_search ("ipv4.irc.test"));
    LONGS_EQUAL(-1, access (path2, F_OK));

    free (path1);
    free (path2);
}

/*
 * Test functions:
 *   relay_server_update_path
 *   relay_server_close_socket
 *   relay_server_create_socket
 */

TEST(RelayServer, UpdatePath)
{
    struct t_relay_server *server;
    char *path1, *path2;
    int old_sock;

    path1 = eval_path (TEST_SOCKET1);
    CHECK(path1);
    path2 = eval_path (TEST_SOCKET2);
    CHECK(path2);

    server = new_unix ("unix.weechat", RELAY_PROTOCOL_WEECHAT, TEST_SOCKET1);
    CHECK(server);
    old_sock = server->sock;

    /* Same path: nothing changes. */
    relay_server_update_path (server, TEST_SOCKET1);
    STRCMP_EQUAL(path1, server->path);
    LONGS_EQUAL(old_sock, server->sock);

    /* New path: socket is moved. */
    relay_server_update_path (server, TEST_SOCKET2);
    STRCMP_EQUAL(path2, server->path);
    LONGS_EQUAL(-1, server->port);
    CHECK(server->sock >= 0);
    LONGS_EQUAL(-1, access (path1, F_OK));
    LONGS_EQUAL(0, access (path2, F_OK));

    /* Close socket. */
    relay_server_close_socket (server);
    LONGS_EQUAL(-1, server->sock);
    POINTERS_EQUAL(NULL, server->hook_fd);
    LONGS_EQUAL(-1, access (path2, F_OK));

    /* Close again: nothing is done. */
    relay_server_close_socket (server);

    /* Create socket again. */
    LONGS_EQUAL(1, relay_server_create_socket (server));
    CHECK(server->sock >= 0);
    LONGS_EQUAL(0, access (path2, F_OK));

    relay_server_free (server);

    free (path1);
    free (path2);
}

/*
 * Test functions:
 *   relay_server_update_port
 */

TEST(RelayServer, UpdatePort)
{
    struct t_relay_server *server;

    server = relay_server_new ("ipv4.weechat", RELAY_PROTOCOL_WEECHAT, NULL,
                               0, "0", 1, 0, 0, 0);
    CHECK(server);

    /* Same port: nothing changes. */
    relay_server_update_port (server, 0);
    LONGS_EQUAL(0, server->port);
    STRCMP_EQUAL("0", server->path);

    relay_server_close_socket (server);
    LONGS_EQUAL(-1, server->sock);

    relay_server_free (server);
}

/*
 * Test functions:
 *   relay_server_add_to_infolist
 */

TEST(RelayServer, AddToInfolist)
{
    struct t_relay_server *server;
    struct t_infolist *infolist;
    char *path1;

    path1 = eval_path (TEST_SOCKET1);
    CHECK(path1);

    server = new_unix ("unix.weechat", RELAY_PROTOCOL_WEECHAT, TEST_SOCKET1);
    CHECK(server);

    infolist = infolist_new (NULL);
    CHECK(infolist);

    LONGS_EQUAL(0, relay_server_add_to_infolist (NULL, server));
    LONGS_EQUAL(0, relay_server_add_to_infolist (infolist, NULL));

    LONGS_EQUAL(1, relay_server_add_to_infolist (infolist, server));
    infolist_reset_item_cursor (infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("unix.weechat",
                 infolist_string (infolist, "protocol_string"));
    LONGS_EQUAL(RELAY_PROTOCOL_WEECHAT,
                infolist_integer (infolist, "protocol"));
    LONGS_EQUAL(-1, infolist_integer (infolist, "port"));
    STRCMP_EQUAL(path1, infolist_string (infolist, "path"));
    LONGS_EQUAL(1, infolist_integer (infolist, "unix_socket"));
    LONGS_EQUAL(server->sock, infolist_integer (infolist, "sock"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);
    relay_server_free (server);
    free (path1);
}

/*
 * Test functions:
 *   relay_server_print_log
 */

TEST(RelayServer, PrintLog)
{
    struct t_relay_server *server;

    server = new_unix ("unix.weechat", RELAY_PROTOCOL_WEECHAT, TEST_SOCKET1);
    CHECK(server);
    relay_server_print_log ();
    relay_server_free (server);
}
