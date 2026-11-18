/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay remote functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/plugins/plugin.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-remote.h"
#include "src/plugins/relay/relay-websocket.h"

extern int relay_remote_parse_url (const char *url,
                                   int *tls, char **address, int *port);
extern int relay_remote_name_valid (const char *name);
extern void relay_remote_send_signal (struct t_relay_remote *remote);
extern struct t_relay_remote *relay_remote_alloc (const char *name);
extern struct t_relay_remote *relay_remote_find_pos (
    struct t_relay_remote *remote, struct t_relay_remote *list_remotes);
extern void relay_remote_add (struct t_relay_remote *remote,
                              struct t_relay_remote **list_remotes,
                              struct t_relay_remote **last_list_remote);
}

/* Option of a remote. */
#define REMOTE_OPTION(__remote, __name)                                 \
    __remote->options[RELAY_REMOTE_OPTION_##__name]

#define WEE_CHECK_PARSE_URL(__result, __result_tls, __result_address,   \
                            __result_port, __url)                       \
    tls = -1;                                                           \
    address = NULL;                                                     \
    port = -1;                                                          \
    relay_remote_parse_url (__url, &tls, &address, &port);              \
    LONGS_EQUAL(__result_tls, tls);                                     \
    STRCMP_EQUAL(__result_address, address);                            \
    LONGS_EQUAL(__result_port, port);                                   \
    free (address);

TEST_GROUP(RelayRemote)
{
    struct t_hook *hook_signal_remote = NULL;
    char **signals = NULL;

    /* Save name of signals "relay_remote_*" sent (separated by spaces). */
    static int signal_cb (const void *pointer, void *data,
                          const char *signal, const char *type_data,
                          void *signal_data)
    {
        char **ptr_signals;

        (void) data;
        (void) type_data;
        (void) signal_data;

        ptr_signals = (char **)pointer;
        if ((*ptr_signals)[0])
            string_dyn_concat (ptr_signals, " ", -1);
        string_dyn_concat (ptr_signals, signal, -1);
        return WEECHAT_RC_OK;
    }

    void setup ()
    {
        signals = string_dyn_alloc (64);
        hook_signal_remote = hook_signal (NULL, "relay_remote_*",
                                          &signal_cb, signals, NULL);
    }

    void teardown ()
    {
        record_stop ();
        unhook (hook_signal_remote);
        hook_signal_remote = NULL;
        string_dyn_free (signals, 1);
        signals = NULL;
        relay_remote_free_all ();
        config_file_option_reset (
            relay_config_api_remote_autoreconnect_delay_growing, 1);
        config_file_option_reset (
            relay_config_api_remote_autoreconnect_delay_max, 1);
    }

    /* Create a remote with default options. */

    struct t_relay_remote *new_remote (const char *name, const char *url)
    {
        return relay_remote_new (name, url,
                                 NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* Free a remote allocated (not in list of remotes). */

    void free_alloc (struct t_relay_remote *remote)
    {
        free (remote->name);
        relay_websocket_deflate_free (remote->ws_deflate);
        free (remote);
    }
};

/*
 * Test functions:
 *   relay_remote_search_option
 */

TEST(RelayRemote, SearchOption)
{
    LONGS_EQUAL(-1, relay_remote_search_option (NULL));
    LONGS_EQUAL(-1, relay_remote_search_option (""));
    LONGS_EQUAL(-1, relay_remote_search_option ("zzz"));

    LONGS_EQUAL(0, relay_remote_search_option ("url"));
}

/*
 * Test functions:
 *   relay_remote_valid
 */

TEST(RelayRemote, Valid)
{
    struct t_relay_remote *remote;

    LONGS_EQUAL(0, relay_remote_valid (NULL));
    LONGS_EQUAL(0, relay_remote_valid ((struct t_relay_remote *)0x1));

    remote = new_remote ("test", "https://example.com");
    CHECK(remote);
    LONGS_EQUAL(1, relay_remote_valid (remote));

    relay_remote_free (remote);
    LONGS_EQUAL(0, relay_remote_valid (remote));
}

/*
 * Test functions:
 *   relay_remote_search
 */

TEST(RelayRemote, Search)
{
    struct t_relay_remote *remote;

    POINTERS_EQUAL(NULL, relay_remote_search (NULL));
    POINTERS_EQUAL(NULL, relay_remote_search (""));
    POINTERS_EQUAL(NULL, relay_remote_search ("test"));

    remote = new_remote ("test", "https://example.com");
    CHECK(remote);
    POINTERS_EQUAL(remote, relay_remote_search ("test"));
    POINTERS_EQUAL(NULL, relay_remote_search ("TEST"));
    POINTERS_EQUAL(NULL, relay_remote_search ("test2"));
}

/*
 * Test functions:
 *   relay_remote_search_by_number
 */

TEST(RelayRemote, SearchByNumber)
{
    struct t_relay_remote *remote_a, *remote_b, *remote_c;

    POINTERS_EQUAL(NULL, relay_remote_search_by_number (0));

    /* Remotes are sorted by name. */
    remote_b = new_remote ("b", "https://example.com");
    CHECK(remote_b);
    remote_a = new_remote ("a", "https://example.com");
    CHECK(remote_a);
    remote_c = new_remote ("c", "https://example.com");
    CHECK(remote_c);

    POINTERS_EQUAL(NULL, relay_remote_search_by_number (-1));
    POINTERS_EQUAL(remote_a, relay_remote_search_by_number (0));
    POINTERS_EQUAL(remote_b, relay_remote_search_by_number (1));
    POINTERS_EQUAL(remote_c, relay_remote_search_by_number (2));
    POINTERS_EQUAL(NULL, relay_remote_search_by_number (3));
}

/*
 * Test functions:
 *   relay_remote_name_valid
 */

TEST(RelayRemote, NameValid)
{
    LONGS_EQUAL(0, relay_remote_name_valid (NULL));
    LONGS_EQUAL(0, relay_remote_name_valid (""));
    LONGS_EQUAL(0, relay_remote_name_valid ("a b"));
    LONGS_EQUAL(0, relay_remote_name_valid ("a.b"));
    LONGS_EQUAL(0, relay_remote_name_valid ("a-b"));
    LONGS_EQUAL(0, relay_remote_name_valid ("a_b"));
    LONGS_EQUAL(0, relay_remote_name_valid ("caf\xc3\xa9"));

    LONGS_EQUAL(1, relay_remote_name_valid ("a"));
    LONGS_EQUAL(1, relay_remote_name_valid ("Remote42"));
}

/*
 * Test functions:
 *   relay_remote_parse_url
 */

TEST(RelayRemote, ParseUrl)
{
    int tls, port;
    char *address;

    LONGS_EQUAL(0, relay_remote_parse_url (NULL, NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_remote_parse_url ("", NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_remote_parse_url ("zzz", NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_remote_parse_url ("http://[::1", NULL, NULL, NULL));
    LONGS_EQUAL(0, relay_remote_parse_url ("https://[::1", NULL, NULL, NULL));

    LONGS_EQUAL(1, relay_remote_parse_url ("http://example.com", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com/", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com?option=1", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com/?option=1", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com:9876", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com:9876/", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com:9876?option=1", NULL, NULL, NULL));
    LONGS_EQUAL(1, relay_remote_parse_url ("https://example.com:9876/?option=1", NULL, NULL, NULL));

    WEE_CHECK_PARSE_URL(1, 0, "", RELAY_REMOTE_DEFAULT_PORT, "http://");
    WEE_CHECK_PARSE_URL(1, 1, "", RELAY_REMOTE_DEFAULT_PORT, "https://");

    WEE_CHECK_PARSE_URL(1, 0, "localhost", RELAY_REMOTE_DEFAULT_PORT, "http://localhost");
    WEE_CHECK_PARSE_URL(1, 1, "localhost", RELAY_REMOTE_DEFAULT_PORT, "https://localhost");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", RELAY_REMOTE_DEFAULT_PORT, "https://example.com");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", RELAY_REMOTE_DEFAULT_PORT, "https://example.com/");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", RELAY_REMOTE_DEFAULT_PORT, "https://example.com?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", RELAY_REMOTE_DEFAULT_PORT, "https://example.com/?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", 9876, "https://example.com:9876");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", 9876, "https://example.com:9876/");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", 9876, "https://example.com:9876?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "example.com", 9876, "https://example.com:9876/?option=1");

    WEE_CHECK_PARSE_URL(1, 0, "::1", RELAY_REMOTE_DEFAULT_PORT, "http://[::1]");
    WEE_CHECK_PARSE_URL(1, 1, "::1", RELAY_REMOTE_DEFAULT_PORT, "https://[::1]");
    WEE_CHECK_PARSE_URL(1, 1, "::1", RELAY_REMOTE_DEFAULT_PORT, "https://[::1]/");
    WEE_CHECK_PARSE_URL(1, 1, "::1", RELAY_REMOTE_DEFAULT_PORT, "https://[::1]?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "::1", RELAY_REMOTE_DEFAULT_PORT, "https://[::1]/?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "::1", 9876, "https://[::1]:9876");
    WEE_CHECK_PARSE_URL(1, 1, "::1", 9876, "https://[::1]:9876/");
    WEE_CHECK_PARSE_URL(1, 1, "::1", 9876, "https://[::1]:9876?option=1");
    WEE_CHECK_PARSE_URL(1, 1, "::1", 9876, "https://[::1]:9876/?option=1");
}

/*
 * Test functions:
 *   relay_remote_url_valid
 */

TEST(RelayRemote, UrlValid)
{
    LONGS_EQUAL(0, relay_remote_url_valid (NULL));
    LONGS_EQUAL(0, relay_remote_url_valid (""));
    LONGS_EQUAL(0, relay_remote_url_valid ("example.com"));
    LONGS_EQUAL(0, relay_remote_url_valid ("ftp://example.com"));
    LONGS_EQUAL(0, relay_remote_url_valid ("https://[::1"));
    LONGS_EQUAL(0, relay_remote_url_valid ("https://example.com:"));
    LONGS_EQUAL(0, relay_remote_url_valid ("https://example.com:abc"));
    LONGS_EQUAL(0, relay_remote_url_valid ("https://example.com:-1"));
    LONGS_EQUAL(0, relay_remote_url_valid ("https://example.com:65536"));

    LONGS_EQUAL(1, relay_remote_url_valid ("http://example.com"));
    LONGS_EQUAL(1, relay_remote_url_valid ("https://example.com:0"));
    LONGS_EQUAL(1, relay_remote_url_valid ("https://example.com:65535"));
    LONGS_EQUAL(1, relay_remote_url_valid ("https://[::1]:9000/"));
}

/*
 * Test functions:
 *   relay_remote_send_signal
 */

TEST(RelayRemote, SendSignal)
{
    struct t_relay_remote *remote;

    remote = new_remote ("test", "https://example.com");
    CHECK(remote);
    STRCMP_EQUAL("relay_remote_disconnected", *signals);

    string_dyn_copy (signals, NULL);
    remote->status = RELAY_STATUS_CONNECTED;
    relay_remote_send_signal (remote);
    STRCMP_EQUAL("relay_remote_connected", *signals);
}

/*
 * Test functions:
 *   relay_remote_alloc
 */

TEST(RelayRemote, Alloc)
{
    struct t_relay_remote *remote;
    int i;

    POINTERS_EQUAL(NULL, relay_remote_alloc (NULL));
    POINTERS_EQUAL(NULL, relay_remote_alloc (""));
    POINTERS_EQUAL(NULL, relay_remote_alloc ("a.b"));

    remote = relay_remote_alloc ("test");
    CHECK(remote);
    STRCMP_EQUAL("test", remote->name);
    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        POINTERS_EQUAL(NULL, remote->options[i]);
    }
    POINTERS_EQUAL(NULL, remote->address);
    LONGS_EQUAL(0, remote->port);
    LONGS_EQUAL(0, remote->tls);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(-1, remote->password_hash_algo);
    LONGS_EQUAL(-1, remote->password_hash_iterations);
    LONGS_EQUAL(-1, remote->totp);
    LONGS_EQUAL(-1, remote->sock);
    CHECK(remote->ws_deflate);
    POINTERS_EQUAL(NULL, remote->partial_ws_frame);
    LONGS_EQUAL(0, remote->partial_ws_frame_size);
    POINTERS_EQUAL(NULL, remote->prev_remote);
    POINTERS_EQUAL(NULL, remote->next_remote);

    /* Not added in list of remotes. */
    POINTERS_EQUAL(NULL, relay_remote_search ("test"));
    free_alloc (remote);

    /* Name already used. */
    CHECK(new_remote ("test", "https://example.com"));
    POINTERS_EQUAL(NULL, relay_remote_alloc ("test"));
}

/*
 * Test functions:
 *   relay_remote_find_pos
 */

TEST(RelayRemote, FindPos)
{
    struct t_relay_remote *remote_b, *remote_d, *remote;

    remote_b = new_remote ("b", "https://example.com");
    CHECK(remote_b);
    remote_d = new_remote ("d", "https://example.com");
    CHECK(remote_d);

    remote = relay_remote_alloc ("a");
    CHECK(remote);
    POINTERS_EQUAL(NULL, relay_remote_find_pos (remote, NULL));
    POINTERS_EQUAL(remote_b, relay_remote_find_pos (remote, relay_remotes));
    free_alloc (remote);

    remote = relay_remote_alloc ("c");
    CHECK(remote);
    POINTERS_EQUAL(remote_d, relay_remote_find_pos (remote, relay_remotes));
    free_alloc (remote);

    remote = relay_remote_alloc ("e");
    CHECK(remote);
    POINTERS_EQUAL(NULL, relay_remote_find_pos (remote, relay_remotes));
    free_alloc (remote);
}

/*
 * Test functions:
 *   relay_remote_add
 */

TEST(RelayRemote, Add)
{
    struct t_relay_remote *list, *last_list, *remote_a, *remote_b, *remote_c;

    list = NULL;
    last_list = NULL;

    remote_b = relay_remote_alloc ("b");
    CHECK(remote_b);
    remote_a = relay_remote_alloc ("a");
    CHECK(remote_a);
    remote_c = relay_remote_alloc ("c");
    CHECK(remote_c);

    /* Empty list. */
    relay_remote_add (remote_b, &list, &last_list);
    POINTERS_EQUAL(remote_b, list);
    POINTERS_EQUAL(remote_b, last_list);

    /* Add at the beginning. */
    relay_remote_add (remote_a, &list, &last_list);
    POINTERS_EQUAL(remote_a, list);
    POINTERS_EQUAL(remote_b, last_list);
    POINTERS_EQUAL(remote_b, remote_a->next_remote);
    POINTERS_EQUAL(remote_a, remote_b->prev_remote);

    /* Add at the end. */
    relay_remote_add (remote_c, &list, &last_list);
    POINTERS_EQUAL(remote_a, list);
    POINTERS_EQUAL(remote_c, last_list);
    POINTERS_EQUAL(remote_c, remote_b->next_remote);
    POINTERS_EQUAL(remote_b, remote_c->prev_remote);
    POINTERS_EQUAL(NULL, remote_c->next_remote);

    free_alloc (remote_a);
    free_alloc (remote_b);
    free_alloc (remote_c);
}

/*
 * Test functions:
 *   relay_remote_new_with_options
 */

TEST(RelayRemote, NewWithOptions)
{
    struct t_relay_remote *remote;
    struct t_config_option *ptr_option;
    int count;

    count = relay_remotes_count;

    /* Invalid URL: remote not created, options are deleted. */
    POINTERS_EQUAL(NULL, new_remote ("test", "ftp://example.com"));
    LONGS_EQUAL(count, relay_remotes_count);
    config_file_search_with_string ("relay.remote.test.url", NULL, NULL,
                                    &ptr_option, NULL);
    POINTERS_EQUAL(NULL, ptr_option);

    remote = new_remote ("test", "https://example.com:1234");
    CHECK(remote);
    LONGS_EQUAL(count + 1, relay_remotes_count);
    STRCMP_EQUAL("example.com", remote->address);
    LONGS_EQUAL(1234, remote->port);
    LONGS_EQUAL(1, remote->tls);
    STRCMP_EQUAL("https://example.com:1234",
                 CONFIG_STRING(REMOTE_OPTION(remote, URL)));
    config_file_search_with_string ("relay.remote.test.url", NULL, NULL,
                                    &ptr_option, NULL);
    POINTERS_EQUAL(REMOTE_OPTION(remote, URL), ptr_option);
}

/*
 * Test functions:
 *   relay_remote_new
 */

TEST(RelayRemote, New)
{
    struct t_relay_remote *remote;

    POINTERS_EQUAL(NULL, new_remote (NULL, "https://example.com"));
    POINTERS_EQUAL(NULL, new_remote ("", "https://example.com"));
    POINTERS_EQUAL(NULL, new_remote ("test", NULL));
    POINTERS_EQUAL(NULL, new_remote ("test", ""));
    POINTERS_EQUAL(NULL, new_remote ("a.b", "https://example.com"));

    /* Default options. */
    remote = new_remote ("test", "http://[::1]");
    CHECK(remote);
    STRCMP_EQUAL("::1", remote->address);
    LONGS_EQUAL(RELAY_REMOTE_DEFAULT_PORT, remote->port);
    LONGS_EQUAL(0, remote->tls);
    LONGS_EQUAL(0, CONFIG_BOOLEAN(REMOTE_OPTION(remote, AUTOCONNECT)));
    LONGS_EQUAL(10,
                CONFIG_INTEGER(REMOTE_OPTION(remote, AUTORECONNECT_DELAY)));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(REMOTE_OPTION(remote, TLS_VERIFY)));

    /* Name already used. */
    POINTERS_EQUAL(NULL, new_remote ("test", "https://example.com"));

    /* All options. */
    remote = relay_remote_new ("test2", "https://example.com", "on", "30",
                               "myproxy", "off", "secret", "ABCDEF");
    CHECK(remote);
    LONGS_EQUAL(1, CONFIG_BOOLEAN(REMOTE_OPTION(remote, AUTOCONNECT)));
    LONGS_EQUAL(30,
                CONFIG_INTEGER(REMOTE_OPTION(remote, AUTORECONNECT_DELAY)));
    STRCMP_EQUAL("myproxy", CONFIG_STRING(REMOTE_OPTION(remote, PROXY)));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(REMOTE_OPTION(remote, TLS_VERIFY)));
    STRCMP_EQUAL("secret", CONFIG_STRING(REMOTE_OPTION(remote, PASSWORD)));
    STRCMP_EQUAL("ABCDEF", CONFIG_STRING(REMOTE_OPTION(remote, TOTP_SECRET)));
}

/*
 * Test functions:
 *   relay_remote_new_with_infolist
 */

TEST(RelayRemote, NewWithInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_set_status
 */

TEST(RelayRemote, SetStatus)
{
    struct t_relay_remote *remote;

    remote = new_remote ("test", "https://example.com");
    CHECK(remote);

    /* Same status: no signal. */
    string_dyn_copy (signals, NULL);
    relay_remote_set_status (remote, RELAY_STATUS_DISCONNECTED);
    STRCMP_EQUAL("", *signals);

    relay_remote_set_status (remote, RELAY_STATUS_CONNECTING);
    LONGS_EQUAL(RELAY_STATUS_CONNECTING, remote->status);
    STRCMP_EQUAL("relay_remote_connecting", *signals);

    relay_remote_set_status (remote, RELAY_STATUS_AUTHENTICATING);
    STRCMP_EQUAL("relay_remote_connecting relay_remote_waiting_auth", *signals);

    remote->status = RELAY_STATUS_DISCONNECTED;
}

/*
 * Test functions:
 *   relay_remote_connect
 */

TEST(RelayRemote, Connect)
{
    LONGS_EQUAL(0, relay_remote_connect (NULL));
}

/*
 * Test functions:
 *   relay_remote_rename
 */

TEST(RelayRemote, Rename)
{
    struct t_relay_remote *remote_a, *remote_b;
    struct t_config_option *ptr_option;

    remote_a = new_remote ("a", "https://example.com");
    CHECK(remote_a);
    remote_b = new_remote ("b", "https://example.com");
    CHECK(remote_b);

    LONGS_EQUAL(0, relay_remote_rename (NULL, "c"));
    LONGS_EQUAL(0, relay_remote_rename (remote_a, NULL));
    LONGS_EQUAL(0, relay_remote_rename (remote_a, ""));
    LONGS_EQUAL(0, relay_remote_rename (remote_a, "c.d"));
    LONGS_EQUAL(0, relay_remote_rename (remote_a, "b"));
    STRCMP_EQUAL("a", remote_a->name);

    /* Rename "a" to "c": options are renamed, remotes are sorted. */
    LONGS_EQUAL(1, relay_remote_rename (remote_a, "c"));
    STRCMP_EQUAL("c", remote_a->name);
    POINTERS_EQUAL(remote_a, relay_remote_search ("c"));
    POINTERS_EQUAL(NULL, relay_remote_search ("a"));
    POINTERS_EQUAL(remote_b, relay_remotes);
    POINTERS_EQUAL(remote_a, last_relay_remote);
    config_file_search_with_string ("relay.remote.c.url", NULL, NULL,
                                    &ptr_option, NULL);
    POINTERS_EQUAL(remote_a->options[RELAY_REMOTE_OPTION_URL], ptr_option);
    config_file_search_with_string ("relay.remote.a.url", NULL, NULL,
                                    &ptr_option, NULL);
    POINTERS_EQUAL(NULL, ptr_option);

    /* Rename "c" to "a": remotes are sorted again. */
    LONGS_EQUAL(1, relay_remote_rename (remote_a, "a"));
    POINTERS_EQUAL(remote_a, relay_remotes);
    POINTERS_EQUAL(remote_b, last_relay_remote);
}

/*
 * Test functions:
 *   relay_remote_disconnect
 */

TEST(RelayRemote, Disconnect)
{
    struct t_relay_remote *remote;

    LONGS_EQUAL(0, relay_remote_disconnect (NULL));

    /* Not connected: nothing is done. */
    remote = new_remote ("test", "https://example.com");
    CHECK(remote);
    LONGS_EQUAL(1, relay_remote_disconnect (remote));
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(-1, remote->sock);
}

/*
 * Test functions:
 *   relay_remote_reconnect_schedule
 */

TEST(RelayRemote, ReconnectSchedule)
{
    struct t_relay_remote *remote;

    remote = new_remote ("test", "https://example.com");
    CHECK(remote);

    /* Auto-reconnection disabled. */
    config_file_option_set (REMOTE_OPTION(remote, AUTORECONNECT_DELAY), "0", 1);
    remote->reconnect_delay = 5;
    remote->reconnect_start = 1;
    relay_remote_reconnect_schedule (remote);
    LONGS_EQUAL(0, remote->reconnect_delay);
    LONGS_EQUAL(0, remote->reconnect_start);

    /* First reconnection: delay of option. */
    config_file_option_set (REMOTE_OPTION(remote, AUTORECONNECT_DELAY),
                            "30", 1);
    record_start ();
    relay_remote_reconnect_schedule (remote);
    record_stop ();
    LONGS_EQUAL(30, remote->reconnect_delay);
    CHECK(remote->reconnect_start > 0);
    RECORD_CHECK_MSG("core.weechat", "",
                     "remote[test]: reconnecting to remote relay in "
                     "30 seconds",
                     NULL);

    /* Next reconnection: growing delay. */
    record_start ();
    relay_remote_reconnect_schedule (remote);
    record_stop ();
    LONGS_EQUAL(60, remote->reconnect_delay);
    RECORD_CHECK_MSG("core.weechat", "",
                     "remote[test]: reconnecting to remote relay in 1 minute",
                     NULL);

    config_file_option_set (relay_config_api_remote_autoreconnect_delay_growing,
                            "3", 1);
    record_start ();
    relay_remote_reconnect_schedule (remote);
    record_stop ();
    LONGS_EQUAL(180, remote->reconnect_delay);
    RECORD_CHECK_MSG("core.weechat", "",
                     "remote[test]: reconnecting to remote relay in 3 minutes",
                     NULL);

    /* Max delay. */
    config_file_option_set (relay_config_api_remote_autoreconnect_delay_max,
                            "200", 1);
    record_start ();
    relay_remote_reconnect_schedule (remote);
    record_stop ();
    LONGS_EQUAL(200, remote->reconnect_delay);
    RECORD_CHECK_MSG("core.weechat", "",
                     "remote[test]: reconnecting to remote relay in "
                     "3 minutes, 20 seconds",
                     NULL);
}

/*
 * Test functions:
 *   relay_remote_reconnect
 */

TEST(RelayRemote, Reconnect)
{
    LONGS_EQUAL(0, relay_remote_reconnect (NULL));
}

/*
 * Test functions:
 *   relay_remote_timer
 */

TEST(RelayRemote, Timer)
{
    struct t_relay_remote *remote;

    remote = new_remote ("test", "https://example.com");
    CHECK(remote);

    /* No reconnection pending. */
    relay_remote_timer ();
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);

    /* Reconnection pending in the future. */
    remote->reconnect_delay = 3600;
    remote->reconnect_start = time (NULL);
    relay_remote_timer ();
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(3600, remote->reconnect_delay);
    CHECK(remote->reconnect_start > 0);
}

/*
 * Test functions:
 *   relay_remote_disconnect_all
 */

TEST(RelayRemote, DisconnectAll)
{
    struct t_relay_remote *remote1, *remote2;

    relay_remote_disconnect_all ();

    remote1 = new_remote ("test1", "https://example.com");
    CHECK(remote1);
    remote2 = new_remote ("test2", "https://example.com");
    CHECK(remote2);
    relay_remote_disconnect_all ();
    LONGS_EQUAL(-1, remote1->sock);
    LONGS_EQUAL(-1, remote2->sock);
}

/*
 * Test functions:
 *   relay_remote_free
 */

TEST(RelayRemote, Free)
{
    struct t_relay_remote *remote1, *remote2, *remote3;
    struct t_config_option *ptr_option;
    int count;

    relay_remote_free (NULL);

    count = relay_remotes_count;
    remote1 = new_remote ("test1", "https://example.com");
    CHECK(remote1);
    remote2 = new_remote ("test2", "https://example.com");
    CHECK(remote2);
    remote3 = new_remote ("test3", "https://example.com");
    CHECK(remote3);
    LONGS_EQUAL(count + 3, relay_remotes_count);

    /* Free remote in the middle of list. */
    relay_remote_free (remote2);
    LONGS_EQUAL(count + 2, relay_remotes_count);
    POINTERS_EQUAL(remote3, remote1->next_remote);
    POINTERS_EQUAL(remote1, remote3->prev_remote);
    config_file_search_with_string ("relay.remote.test2.url", NULL, NULL,
                                    &ptr_option, NULL);
    POINTERS_EQUAL(NULL, ptr_option);

    /* Free first and last remotes. */
    relay_remote_free (remote1);
    POINTERS_EQUAL(remote3, relay_remotes);
    POINTERS_EQUAL(remote3, last_relay_remote);
    relay_remote_free (remote3);
    POINTERS_EQUAL(NULL, relay_remotes);
    POINTERS_EQUAL(NULL, last_relay_remote);
    LONGS_EQUAL(count, relay_remotes_count);
}

/*
 * Test functions:
 *   relay_remote_free_all
 */

TEST(RelayRemote, FreeAll)
{
    relay_remote_free_all ();

    CHECK(new_remote ("test1", "https://example.com"));
    CHECK(new_remote ("test2", "https://example.com"));
    relay_remote_free_all ();
    POINTERS_EQUAL(NULL, relay_remotes);
    POINTERS_EQUAL(NULL, last_relay_remote);
    LONGS_EQUAL(0, relay_remotes_count);
}

/*
 * Test functions:
 *   relay_remote_add_to_infolist
 */

TEST(RelayRemote, AddToInfolist)
{
    struct t_relay_remote *remote;
    struct t_infolist *infolist;

    remote = new_remote ("test", "https://example.com:1234");
    CHECK(remote);

    infolist = infolist_new (NULL);
    CHECK(infolist);

    LONGS_EQUAL(0, relay_remote_add_to_infolist (NULL, remote, 0));
    LONGS_EQUAL(0, relay_remote_add_to_infolist (infolist, NULL, 0));

    /* Connected remote saved with a disconnected state. */
    remote->status = RELAY_STATUS_CONNECTED;
    remote->sock = 42;
    LONGS_EQUAL(1, relay_remote_add_to_infolist (infolist, remote, 1));
    LONGS_EQUAL(1, relay_remote_add_to_infolist (infolist, remote, 0));
    remote->status = RELAY_STATUS_DISCONNECTED;
    remote->sock = -1;

    infolist_reset_item_cursor (infolist);
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("test", infolist_string (infolist, "name"));
    STRCMP_EQUAL("example.com", infolist_string (infolist, "address"));
    LONGS_EQUAL(1234, infolist_integer (infolist, "port"));
    LONGS_EQUAL(1, infolist_integer (infolist, "tls"));
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED,
                infolist_integer (infolist, "status"));
    LONGS_EQUAL(-1, infolist_integer (infolist, "sock"));
    CHECK(infolist_next (infolist));
    LONGS_EQUAL(RELAY_STATUS_CONNECTED,
                infolist_integer (infolist, "status"));
    LONGS_EQUAL(42, infolist_integer (infolist, "sock"));
    POINTERS_EQUAL(NULL, infolist_next (infolist));

    infolist_free (infolist);
}

/*
 * Test functions:
 *   relay_remote_print_log
 */

TEST(RelayRemote, PrintLog)
{
    CHECK(new_remote ("test", "https://example.com"));
    relay_remote_print_log ();
}
