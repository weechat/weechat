/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay configuration functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "src/core/weechat.h"
#include "src/core/core-config-file.h"
#include "src/core/core-string.h"
#include "src/plugins/plugin.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-server.h"

extern int relay_config_check_port_cb (const void *pointer, void *data,
                                       struct t_config_option *option,
                                       const char *value);
extern int relay_config_check_path_cb (const void *pointer, void *data,
                                       struct t_config_option *option,
                                       const char *value);
extern int relay_config_check_network_tls_priorities (
    const void *pointer, void *data, struct t_config_option *option,
    const char *value);
extern int relay_config_check_network_unix_socket_permissions (
    const void *pointer, void *data, struct t_config_option *option,
    const char *value);
extern int relay_config_check_irc_backlog_tags (
    const void *pointer, void *data, struct t_config_option *option,
    const char *value);
}

#define TEST_SOCKET "%h/test_relay_config.sock"

TEST_GROUP(RelayConfig)
{
    void setup ()
    {
        /* Disable auto-open of relay buffer (it would pollute other tests). */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);
    }

    void teardown ()
    {
        record_stop ();
        run_cmd_quiet ("/mute /relay del ipv4.weechat");
        run_cmd_quiet ("/mute /relay del ipv4.api");
        run_cmd_quiet ("/mute /relay del unix.weechat");
        run_cmd_quiet ("/mute /relay del unix.api");
        config_file_option_reset (relay_config_network_allowed_ips, 1);
        config_file_option_reset (relay_config_network_password_hash_algo, 1);
        config_file_option_reset (
            relay_config_network_websocket_allowed_origins, 1);
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);
    }
};

/*
 * Test functions:
 *   relay_config_check_port_cb
 */

TEST(RelayConfig, CheckPortCb)
{
    struct t_relay_server *ptr_server;
    struct t_config_option *ptr_option;

    /* Invalid or out of range values: validated later by WeeChat. */
    LONGS_EQUAL(1, relay_config_check_port_cb (NULL, NULL, NULL, "abc"));
    LONGS_EQUAL(1, relay_config_check_port_cb (NULL, NULL, NULL, "-1"));
    LONGS_EQUAL(1, relay_config_check_port_cb (NULL, NULL, NULL, "70000"));
    LONGS_EQUAL(1, relay_config_check_port_cb (NULL, NULL, NULL, "++abc"));
    LONGS_EQUAL(1, relay_config_check_port_cb (NULL, NULL, NULL, "--abc"));

    /* Relay on a port chosen by the system (port 0). */
    run_cmd_quiet ("/mute /relay add ipv4.weechat 0");
    ptr_server = relay_server_search ("ipv4.weechat");
    CHECK(ptr_server);
    config_file_search_with_string ("relay.port.ipv4.weechat", NULL, NULL,
                                    &ptr_option, NULL);
    CHECK(ptr_option);

    /* Port not used. */
    LONGS_EQUAL(1, relay_config_check_port_cb (NULL, NULL, ptr_option,
                                               "12345"));

    /* Port already used. */
    record_start ();
    LONGS_EQUAL(0, relay_config_check_port_cb (NULL, NULL, ptr_option, "0"));
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "relay: error: port \"0\" is already used", NULL);

    /* Relative values (current value is 0). */
    LONGS_EQUAL(1, relay_config_check_port_cb (NULL, NULL, ptr_option, "++1"));
    LONGS_EQUAL(0, relay_config_check_port_cb (NULL, NULL, ptr_option, "++0"));
    LONGS_EQUAL(0, relay_config_check_port_cb (NULL, NULL, ptr_option, "--0"));
    LONGS_EQUAL(1, relay_config_check_port_cb (NULL, NULL, ptr_option, "--1"));
}

/*
 * Test functions:
 *   relay_config_check_path_length
 */

TEST(RelayConfig, CheckPathLength)
{
    char path[256];

    LONGS_EQUAL(0, relay_config_check_path_length (""));
    LONGS_EQUAL(1, relay_config_check_path_length ("/tmp/relay.sock"));

    memset (path, 'a', sizeof (path) - 1);
    path[sizeof (path) - 1] = '\0';
    LONGS_EQUAL(0, relay_config_check_path_length (path));
}

/*
 * Test functions:
 *   relay_config_check_path_available
 */

TEST(RelayConfig, CheckPathAvailable)
{
    struct t_relay_server *ptr_server;
    char *path_file, *path_in_file;
    FILE *file;

    CHECK(string_asprintf (&path_file, "%s/test_relay_config.txt",
                           weechat_runtime_dir) >= 0);
    CHECK(string_asprintf (&path_in_file, "%s/test", path_file) >= 0);

    /* File not found: available. */
    unlink (path_file);
    LONGS_EQUAL(0, relay_config_check_path_available (path_file));

    /* Regular file: not available. */
    file = fopen (path_file, "w");
    CHECK(file);
    fclose (file);
    LONGS_EQUAL(-1, relay_config_check_path_available (path_file));

    /* Invalid path (a parent is a regular file). */
    LONGS_EQUAL(-2, relay_config_check_path_available (path_in_file));
    unlink (path_file);

    /* Existing socket: available. */
    run_cmd_quiet ("/mute /relay add unix.weechat " TEST_SOCKET);
    ptr_server = relay_server_search ("unix.weechat");
    CHECK(ptr_server);
    LONGS_EQUAL(0, relay_config_check_path_available (ptr_server->path));

    free (path_file);
    free (path_in_file);
}

/*
 * Test functions:
 *   relay_config_check_path_cb
 */

TEST(RelayConfig, CheckPathCb)
{
    struct t_relay_server *ptr_server;

    record_start ();
    LONGS_EQUAL(0, relay_config_check_path_cb (NULL, NULL, NULL, ""));
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=", "relay: error: path is empty",
                     NULL);

    LONGS_EQUAL(1, relay_config_check_path_cb (NULL, NULL, NULL,
                                               "/tmp/relay.sock"));

    /* Path already used. */
    run_cmd_quiet ("/mute /relay add unix.weechat " TEST_SOCKET);
    ptr_server = relay_server_search ("unix.weechat");
    CHECK(ptr_server);
    LONGS_EQUAL(0, relay_config_check_path_cb (NULL, NULL, NULL,
                                               ptr_server->path));
    LONGS_EQUAL(0, relay_config_check_path_cb (NULL, NULL, NULL,
                                               TEST_SOCKET));
}

/*
 * Test functions:
 *   relay_config_create_option_port_path
 *   relay_config_change_path_cb
 *   relay_config_delete_path_cb
 *   relay_config_change_port_cb
 *   relay_config_delete_port_cb
 */

TEST(RelayConfig, CreateOptionPortPath)
{
    struct t_relay_server *ptr_server;
    struct t_config_option *ptr_option;
    char *path1, *path2;

    CHECK(string_asprintf (&path1, "%s/test_relay_config.sock",
                           weechat_runtime_dir) >= 0);
    CHECK(string_asprintf (&path2, "%s/test_relay_config2.sock",
                           weechat_runtime_dir) >= 0);

    /* Unknown protocol. */
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                relay_config_create_option_port_path (
                    NULL, NULL, relay_config_file, relay_config_section_port,
                    "xxx", "0"));
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "relay: error: unknown protocol \"xxx\"", NULL);

    /* Name not allowed for protocol "weechat". */
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                relay_config_create_option_port_path (
                    NULL, NULL, relay_config_file, relay_config_section_port,
                    "weechat.test", "0"));
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "relay: error: name is not allowed for protocol "
                     "\"weechat\"",
                     NULL);
    POINTERS_EQUAL(NULL, relay_server_search ("weechat.test"));

    /* UNIX socket. */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                relay_config_create_option_port_path (
                    NULL, NULL, relay_config_file, relay_config_section_path,
                    "unix.weechat", TEST_SOCKET));
    ptr_server = relay_server_search ("unix.weechat");
    CHECK(ptr_server);
    STRCMP_EQUAL(path1, ptr_server->path);
    config_file_search_with_string ("relay.path.unix.weechat", NULL, NULL,
                                    &ptr_option, NULL);
    CHECK(ptr_option);
    STRCMP_EQUAL(TEST_SOCKET, CONFIG_STRING(ptr_option));

    /* Relay already exists. */
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                relay_config_create_option_port_path (
                    NULL, NULL, relay_config_file, relay_config_section_path,
                    "unix.weechat", "%h/other.sock"));
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "relay: error: relay for \"unix.weechat\" already exists",
                     NULL);

    /* Path already used (evaluated path). */
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                relay_config_create_option_port_path (
                    NULL, NULL, relay_config_file, relay_config_section_path,
                    "unix.api", path1));
    record_stop ();
    POINTERS_EQUAL(NULL, relay_server_search ("unix.api"));

    /* Path already used (path to evaluate). */
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                relay_config_create_option_port_path (
                    NULL, NULL, relay_config_file, relay_config_section_path,
                    "unix.api", TEST_SOCKET));
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "relay: error: path \"" TEST_SOCKET "\" is already used",
                     NULL);
    POINTERS_EQUAL(NULL, relay_server_search ("unix.api"));

    /* Change path: socket is moved. */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_CHANGED,
                config_file_option_set (ptr_option,
                                        "%h/test_relay_config2.sock", 1));
    STRCMP_EQUAL(path2, ptr_server->path);
    LONGS_EQUAL(0, access (path2, F_OK));
    LONGS_EQUAL(-1, access (path1, F_OK));

    /* Delete option: server is deleted. */
    config_file_option_unset (ptr_option);
    POINTERS_EQUAL(NULL, relay_server_search ("unix.weechat"));
    LONGS_EQUAL(-1, access (path2, F_OK));

    /* TCP port (chosen by the system). */
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE,
                relay_config_create_option_port_path (
                    NULL, NULL, relay_config_file, relay_config_section_port,
                    "ipv4.weechat", "0"));
    ptr_server = relay_server_search ("ipv4.weechat");
    CHECK(ptr_server);
    LONGS_EQUAL(0, ptr_server->port);
    config_file_search_with_string ("relay.port.ipv4.weechat", NULL, NULL,
                                    &ptr_option, NULL);
    CHECK(ptr_option);
    LONGS_EQUAL(0, CONFIG_INTEGER(ptr_option));

    /* Port already used. */
    record_start ();
    LONGS_EQUAL(WEECHAT_CONFIG_OPTION_SET_ERROR,
                relay_config_create_option_port_path (
                    NULL, NULL, relay_config_file, relay_config_section_port,
                    "ipv4.api", "0"));
    record_stop ();
    RECORD_CHECK_MSG("core.weechat", "=!=",
                     "relay: error: port \"0\" is already used", NULL);
    POINTERS_EQUAL(NULL, relay_server_search ("ipv4.api"));

    /* Delete option: server is deleted. */
    config_file_option_unset (ptr_option);
    POINTERS_EQUAL(NULL, relay_server_search ("ipv4.weechat"));

    free (path1);
    free (path2);
}

/*
 * Test functions:
 *   relay_config_check_network_totp_secret
 */

TEST(RelayConfig, CheckNetworkTotpSecret)
{
    LONGS_EQUAL(1, relay_config_check_network_totp_secret (NULL, NULL, NULL,
                                                           ""));
    LONGS_EQUAL(1, relay_config_check_network_totp_secret (
                    NULL, NULL, NULL, "JBSWY3DPEHPK3PXP"));
    LONGS_EQUAL(1, relay_config_check_network_totp_secret (
                    NULL, NULL, NULL, "${raw:JBSWY3DPEHPK3PXP}"));

    record_start ();
    LONGS_EQUAL(0, relay_config_check_network_totp_secret (NULL, NULL, NULL,
                                                           "invalid!"));
    record_stop ();
    CHECK(record_search_msg_regex ("core.weechat",
                                   "invalid value for option "
                                   "\"relay\\.network\\.totp_secret\""));
}

/*
 * Test functions:
 *   relay_config_check_network_tls_priorities
 */

TEST(RelayConfig, CheckNetworkTlsPriorities)
{
    LONGS_EQUAL(1, relay_config_check_network_tls_priorities (
                    NULL, NULL, NULL, "NORMAL"));

    record_start ();
    LONGS_EQUAL(0, relay_config_check_network_tls_priorities (
                    NULL, NULL, NULL, ""));
    LONGS_EQUAL(0, relay_config_check_network_tls_priorities (
                    NULL, NULL, NULL, "NORMAL:xxx"));
    record_stop ();
    CHECK(record_search_msg_regex ("core.weechat",
                                   "invalid priorities string, error at this "
                                   "position in string: \"xxx\""));
}

/*
 * Test functions:
 *   relay_config_check_network_unix_socket_permissions
 */

TEST(RelayConfig, CheckNetworkUnixSocketPermissions)
{
    LONGS_EQUAL(0, relay_config_check_network_unix_socket_permissions (
                    NULL, NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_config_check_network_unix_socket_permissions (
                    NULL, NULL, NULL, ""));
    LONGS_EQUAL(0, relay_config_check_network_unix_socket_permissions (
                    NULL, NULL, NULL, "60"));
    LONGS_EQUAL(0, relay_config_check_network_unix_socket_permissions (
                    NULL, NULL, NULL, "6000"));
    LONGS_EQUAL(0, relay_config_check_network_unix_socket_permissions (
                    NULL, NULL, NULL, "608"));
    LONGS_EQUAL(0, relay_config_check_network_unix_socket_permissions (
                    NULL, NULL, NULL, "abc"));
    LONGS_EQUAL(1, relay_config_check_network_unix_socket_permissions (
                    NULL, NULL, NULL, "600"));
    LONGS_EQUAL(1, relay_config_check_network_unix_socket_permissions (
                    NULL, NULL, NULL, "777"));
}

/*
 * Test functions:
 *   relay_config_check_irc_backlog_tags
 */

TEST(RelayConfig, CheckIrcBacklogTags)
{
    LONGS_EQUAL(1, relay_config_check_irc_backlog_tags (NULL, NULL, NULL,
                                                        "*"));
    LONGS_EQUAL(1, relay_config_check_irc_backlog_tags (NULL, NULL, NULL,
                                                        ""));
    LONGS_EQUAL(1, relay_config_check_irc_backlog_tags (NULL, NULL, NULL,
                                                        "irc_privmsg"));
    LONGS_EQUAL(1, relay_config_check_irc_backlog_tags (
                    NULL, NULL, NULL, "irc_join,irc_part,irc_quit"));
    LONGS_EQUAL(0, relay_config_check_irc_backlog_tags (NULL, NULL, NULL,
                                                        "xxx"));
    LONGS_EQUAL(0, relay_config_check_irc_backlog_tags (
                    NULL, NULL, NULL, "irc_join,xxx"));
}

/*
 * Test functions:
 *   relay_config_change_network_allowed_ips
 */

TEST(RelayConfig, ChangeNetworkAllowedIps)
{
    POINTERS_EQUAL(NULL, relay_config_regex_allowed_ips);

    config_file_option_set (relay_config_network_allowed_ips,
                            "^(123\\.45\\.67\\.89|192\\.160\\..*)$", 1);
    CHECK(relay_config_regex_allowed_ips);
    LONGS_EQUAL(0, regexec (relay_config_regex_allowed_ips, "123.45.67.89",
                            0, NULL, 0));
    LONGS_EQUAL(0, regexec (relay_config_regex_allowed_ips, "192.160.1.2",
                            0, NULL, 0));
    CHECK(regexec (relay_config_regex_allowed_ips, "10.0.0.1",
                   0, NULL, 0) != 0);

    /* Invalid regex: no restriction. */
    config_file_option_set (relay_config_network_allowed_ips, "(", 1);
    POINTERS_EQUAL(NULL, relay_config_regex_allowed_ips);

    config_file_option_set (relay_config_network_allowed_ips, "", 1);
    POINTERS_EQUAL(NULL, relay_config_regex_allowed_ips);
}

/*
 * Test functions:
 *   relay_config_change_network_websocket_allowed_origins
 */

TEST(RelayConfig, ChangeNetworkWebsocketAllowedOrigins)
{
    POINTERS_EQUAL(NULL, relay_config_regex_websocket_allowed_origins);

    config_file_option_set (relay_config_network_websocket_allowed_origins,
                            "^https?://example\\.com$", 1);
    CHECK(relay_config_regex_websocket_allowed_origins);
    LONGS_EQUAL(0, regexec (relay_config_regex_websocket_allowed_origins,
                            "https://example.com", 0, NULL, 0));
    CHECK(regexec (relay_config_regex_websocket_allowed_origins,
                   "https://example.org", 0, NULL, 0) != 0);

    config_file_option_set (relay_config_network_websocket_allowed_origins,
                            "(", 1);
    POINTERS_EQUAL(NULL, relay_config_regex_websocket_allowed_origins);

    config_file_option_set (relay_config_network_websocket_allowed_origins,
                            "", 1);
    POINTERS_EQUAL(NULL, relay_config_regex_websocket_allowed_origins);
}

/*
 * Test functions:
 *   relay_config_change_network_password_hash_algo
 */

TEST(RelayConfig, ChangeNetworkPasswordHashAlgo)
{
    config_file_option_set (relay_config_network_password_hash_algo,
                            "plain,sha256,pbkdf2+sha512", 1);
    CHECK(relay_config_network_password_hash_algo_list);
    STRCMP_EQUAL("plain", relay_config_network_password_hash_algo_list[0]);
    STRCMP_EQUAL("sha256", relay_config_network_password_hash_algo_list[1]);
    STRCMP_EQUAL("pbkdf2+sha512",
                 relay_config_network_password_hash_algo_list[2]);
    POINTERS_EQUAL(NULL, relay_config_network_password_hash_algo_list[3]);
}
