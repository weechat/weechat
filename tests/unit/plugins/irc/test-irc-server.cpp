/*
 * SPDX-FileCopyrightText: 2020-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC protocol functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/core/hook/hook-timer.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-nick.h"
#include "src/plugins/irc/irc-raw.h"
#include "src/plugins/irc/irc-server.h"

extern int irc_server_fingerprint_search_algo_with_size (int size);
extern char *irc_server_eval_fingerprint (struct t_irc_server *server);
extern char *irc_server_build_autojoin (struct t_irc_server *server);
extern void irc_server_set_index_current_address (struct t_irc_server *server,
                                                  int index);
extern int irc_server_connect_cb (const void *pointer, void *data,
                                  int status, int gnutls_rc, int sock,
                                  const char *error, const char *ip_address);
extern int irc_server_compare_fingerprints (const char *fingerprint,
                                            const unsigned char *fingerprint_server,
                                            ssize_t fingerprint_size);
extern struct t_irc_server *last_irc_server;
extern int irc_server_get_nick_index (struct t_irc_server *server);
extern int irc_server_send (struct t_irc_server *server, const char *buffer,
                            int size_buf);
extern char *irc_server_get_tags_to_send (const char *tags);
extern int irc_server_outqueue_all_empty (struct t_irc_server *server);
extern void irc_server_outqueue_add (struct t_irc_server *server,
                                     int priority, const char *command,
                                     const char *msg1, const char *msg2,
                                     int modified, const char *tags,
                                     struct t_irc_redirect *redirect);
extern void irc_server_outqueue_free (struct t_irc_server *server,
                                      int priority,
                                      struct t_irc_outqueue *outqueue);
extern int irc_server_outqueue_timer_cb (const void *pointer, void *data,
                                         int remaining_calls);
extern void irc_server_outqueue_send_one_msg (struct t_irc_server *server,
                                              struct t_irc_outqueue *message);
extern void irc_server_outqueue_send (struct t_irc_server *server);
extern void irc_server_send_one_msg (struct t_irc_server *server, int flags,
                                     const char *message, const char *nick,
                                     const char *command, const char *channel,
                                     const char *tags);
extern void irc_server_msgq_add_msg (struct t_irc_server *server,
                                     const char *msg);
extern void irc_server_msgq_add_unterminated (struct t_irc_server *server,
                                              const char *string);
extern int irc_server_check_certificate_fingerprint (struct t_irc_server *server,
                                                     gnutls_x509_crt_t certificate,
                                                     const char *good_fingerprints);
}

#define IRC_FAKE_SERVER "fake"

/* self-signed certificate (EC P-256, CN=weechat.test), used for fingerprints */
#define IRC_TEST_CERT                                                   \
    "-----BEGIN CERTIFICATE-----\n"                                     \
    "MIIBhDCCASugAwIBAgIUFV/1wDW8MZC8yAaFkAtGtqDKitUwCgYIKoZIzj0EAwIw\n" \
    "FzEVMBMGA1UEAwwMd2VlY2hhdC50ZXN0MCAXDTI2MTEwNDIwMzcxMloYDzIxMjYx\n" \
    "MDExMjAzNzEyWjAXMRUwEwYDVQQDDAx3ZWVjaGF0LnRlc3QwWTATBgcqhkjOPQIB\n" \
    "BggqhkjOPQMBBwNCAATYSec4mq4mTKXVrEKBEHKxiD2ptHgmeSVr57qATeci/Ivr\n" \
    "dIPhoI0mrBFUwdOzjnHwHtKerQR1Eobrxub/b0hvo1MwUTAdBgNVHQ4EFgQUZrh/\n" \
    "OVFjfDjYpGP3lfjlYXqcVvEwHwYDVR0jBBgwFoAUZrh/OVFjfDjYpGP3lfjlYXqc\n" \
    "VvEwDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNHADBEAiARL0WkLAD1JZON\n" \
    "nSvJm2xpJyxjrYx6n9SHj959rbWlvwIgG1SYsjdSsZqTnyz3FIkdl9B7hR8ulkJa\n" \
    "XlUUmOWG29E=\n"                                                     \
    "-----END CERTIFICATE-----\n"

/* fingerprints of the certificate above */
#define IRC_TEST_CERT_SHA1 "e2e262c92ffb004a4554b889864a0e6b1c99d6c5"
#define IRC_TEST_CERT_SHA256                                            \
    "214a2b3083b38dae6c305647958a6d7bca79b9819fb4b19d048f683ee32436ef"
#define IRC_TEST_CERT_SHA512                                            \
    "dee9a796f2a78031834ea060186bb242c3da42776235c8982e7f792c5a1c7c05"  \
    "b2ddaac1af607b1b27c4d4a97467a547d79f8d523082982ff3f9b5588318f89f"

int test_irc_server_signal_count = 0;
char test_irc_server_signal[256];
char test_irc_server_signal_data[1024];
char test_irc_server_signal_commands[1024];

/*
 * Reset data of signals received.
 */

void
test_irc_server_signal_reset ()
{
    test_irc_server_signal_count = 0;
    test_irc_server_signal[0] = '\0';
    test_irc_server_signal_data[0] = '\0';
    test_irc_server_signal_commands[0] = '\0';
}

/*
 * Callback for IRC signals: store the last signal and its data, and the
 * list of commands (the part after the last "_" in signal name).
 */

int
test_irc_server_signal_cb (const void *pointer, void *data,
                           const char *signal, const char *type_data,
                           void *signal_data)
{
    const char *pos;

    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) type_data;

    test_irc_server_signal_count++;
    snprintf (test_irc_server_signal, sizeof (test_irc_server_signal),
              "%s", signal);
    snprintf (test_irc_server_signal_data,
              sizeof (test_irc_server_signal_data),
              "%s", (signal_data) ? (const char *)signal_data : "");
    pos = strrchr (signal, '_');
    if (test_irc_server_signal_commands[0])
    {
        strncat (test_irc_server_signal_commands, ",",
                 sizeof (test_irc_server_signal_commands)
                 - strlen (test_irc_server_signal_commands) - 1);
    }
    strncat (test_irc_server_signal_commands, (pos) ? pos + 1 : signal,
             sizeof (test_irc_server_signal_commands)
             - strlen (test_irc_server_signal_commands) - 1);

    return WEECHAT_RC_OK;
}

/*
 * Callback for IRC modifiers: drop the message if it contains "drop",
 * return the same message if it contains "same", otherwise add
 * " (modified)" at the end.
 */

char *
test_irc_server_modifier_cb (const void *pointer, void *data,
                             const char *modifier, const char *modifier_data,
                             const char *string)
{
    char *result;

    /* Make C++ compiler happy. */
    (void) pointer;
    (void) data;
    (void) modifier;
    (void) modifier_data;

    if (strstr (string, "drop"))
        return strdup ("");

    if (strstr (string, "same"))
        return strdup (string);

    string_asprintf (&result, "%s (modified)", string);
    return result;
}

/*
 * Search an option by its full name.
 */

struct t_config_option *
test_irc_server_search_option (const char *option_name)
{
    struct t_config_option *ptr_option;

    config_file_search_with_string (option_name, NULL, NULL, &ptr_option,
                                    NULL);
    return ptr_option;
}

/*
 * Free the queue of messages received.
 */

void
test_irc_server_msgq_free ()
{
    struct t_irc_message *next_message;

    while (irc_recv_msgq)
    {
        next_message = irc_recv_msgq->next_message;
        free (irc_recv_msgq->data);
        free (irc_recv_msgq);
        irc_recv_msgq = next_message;
    }
    irc_msgq_last_msg = NULL;
}

TEST_GROUP(IrcServer)
{
};

/*
 * Test functions:
 *   irc_server_valid
 */

TEST(IrcServer, Valid)
{
    struct t_irc_server *server;
    int dummy;

    server = irc_server_alloc ("server1");

    LONGS_EQUAL(0, irc_server_valid (NULL));
    LONGS_EQUAL(0, irc_server_valid ((struct t_irc_server *)0x1));
    /* Stack address: it can never be a server (allocated on heap). */
    LONGS_EQUAL(0, irc_server_valid ((struct t_irc_server *)&dummy));

    LONGS_EQUAL(1, irc_server_valid (server));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_msgq_add_unterminated (via irc_server_msgq_add_buffer)
 *
 * Check that data received without any end-of-line does not grow the
 * unterminated message buffer without limit.
 */

TEST(IrcServer, MsgqAddBufferLimit)
{
    struct t_irc_server *server;
    char chunk[4097];
    int i;
    size_t length1, length2;

    server = irc_server_alloc ("server_msgq");
    CHECK(server);

    memset (chunk, 'a', sizeof (chunk) - 1);
    chunk[sizeof (chunk) - 1] = '\0';

    /* Feed a lot of data with no end-of-line. */
    for (i = 0; i < 100; i++)
    {
        irc_server_msgq_add_buffer (server, chunk);
    }
    CHECK(server->unterminated_message);
    length1 = strlen (server->unterminated_message);

    /* The buffer must be bounded (not ~400 KB). */
    CHECK(length1 <= IRC_SERVER_RECV_MSG_MAX_LENGTH + sizeof (chunk));

    /* Feeding more data must not grow the buffer any further. */
    for (i = 0; i < 100; i++)
    {
        irc_server_msgq_add_buffer (server, chunk);
    }
    length2 = strlen (server->unterminated_message);
    LONGS_EQUAL(length1, length2);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_search
 */

TEST(IrcServer, Search)
{
    struct t_irc_server *server1, *server2;

    server1 = irc_server_alloc ("srv1");
    server2 = irc_server_alloc ("abc");

    POINTERS_EQUAL(NULL, irc_server_search (NULL));
    POINTERS_EQUAL(NULL, irc_server_search ("does_not_exist"));
    POINTERS_EQUAL(NULL, irc_server_search ("SRV1"));
    POINTERS_EQUAL(NULL, irc_server_search ("ABC"));

    POINTERS_EQUAL(server1, irc_server_search ("srv1"));
    POINTERS_EQUAL(server2, irc_server_search ("abc"));

    irc_server_free (server1);
    irc_server_free (server2);
}

/*
 * Test functions:
 *   irc_server_search_option
 */

TEST(IrcServer, SearchOption)
{
    LONGS_EQUAL(-1, irc_server_search_option (NULL));
    LONGS_EQUAL(-1, irc_server_search_option (""));
    LONGS_EQUAL(-1, irc_server_search_option ("does_not_exist"));

    CHECK(irc_server_search_option ("addresses") >= 0);
    CHECK(irc_server_search_option ("ADDRESSES") >= 0);
    CHECK(irc_server_search_option ("autojoin") >= 0);
    CHECK(irc_server_search_option ("AUTOJOIN") >= 0);
}

/*
 * Test functions:
 *   irc_server_search_casemapping
 */

TEST(IrcServer, SearchCasemapping)
{
    LONGS_EQUAL(-1, irc_server_search_casemapping (NULL));
    LONGS_EQUAL(-1, irc_server_search_casemapping (""));
    LONGS_EQUAL(-1, irc_server_search_casemapping ("does_not_exist"));

    CHECK(irc_server_search_casemapping ("rfc1459") >= 0);
    CHECK(irc_server_search_casemapping ("RFC1459") >= 0);
    CHECK(irc_server_search_casemapping ("strict-rfc1459") >= 0);
    CHECK(irc_server_search_casemapping ("STRICT-RFC1459") >= 0);
}

/*
 * Test functions:
 *   irc_server_search_utf8mapping
 */

TEST(IrcServer, SearchUtf8mapping)
{
    LONGS_EQUAL(-1, irc_server_search_utf8mapping (NULL));
    LONGS_EQUAL(-1, irc_server_search_utf8mapping (""));
    LONGS_EQUAL(-1, irc_server_search_utf8mapping ("does_not_exist"));

    CHECK(irc_server_search_utf8mapping ("none") >= 0);
    CHECK(irc_server_search_utf8mapping ("NONE") >= 0);
    CHECK(irc_server_search_utf8mapping ("rfc8265") >= 0);
    CHECK(irc_server_search_utf8mapping ("RFC8265") >= 0);
}

/*
 * Test functions:
 *   irc_server_strcasecmp
 *   irc_server_strncasecmp
 */

TEST(IrcServer, Strcasecmp)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, NULL, NULL));
    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, "", ""));

    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, NULL, NULL, 0));
    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "", "", 0));
    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, NULL, NULL, 1));
    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "", "", 1));

    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, "abc", "abc"));
    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, "abc", "ABC"));

    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "abc", "abc", 1));
    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "abc", "ABC", 1));

    LONGS_EQUAL(-19, irc_server_strcasecmp (NULL, "abc", "test"));
    LONGS_EQUAL(19, irc_server_strcasecmp (NULL, "test", "abc"));

    LONGS_EQUAL(-19, irc_server_strncasecmp (NULL, "abc", "test", 1));
    LONGS_EQUAL(19, irc_server_strncasecmp (NULL, "test", "abc", 1));

    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "atest", "abc", 1));

    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, "nick[a]", "nick{a}"));
    LONGS_EQUAL(0, irc_server_strcasecmp (NULL, "nick^a", "nick~a"));

    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "nick[a]", "nick{a}", 10));
    LONGS_EQUAL(0, irc_server_strncasecmp (NULL, "nick^a", "nick~a", 10));

    LONGS_EQUAL(0, irc_server_strcasecmp (server, "nick[a]", "nick{a}"));
    LONGS_EQUAL(0, irc_server_strcasecmp (server, "nick^a", "nick~a"));

    LONGS_EQUAL(0, irc_server_strncasecmp (server, "nick[a]", "nick{a}", 10));
    LONGS_EQUAL(0, irc_server_strncasecmp (server, "nick^a", "nick~a", 10));

    server->casemapping = IRC_SERVER_CASEMAPPING_STRICT_RFC1459;

    LONGS_EQUAL(0, irc_server_strcasecmp (server, "nick[a]", "nick{a}"));
    LONGS_EQUAL(-32, irc_server_strcasecmp (server, "nick^a", "nick~a"));
    LONGS_EQUAL(32, irc_server_strcasecmp (server, "nick~a", "nick^a"));

    LONGS_EQUAL(0, irc_server_strncasecmp (server, "nick[a]", "nick{a}", 10));
    LONGS_EQUAL(-32, irc_server_strncasecmp (server, "nick^a", "nick~a", 10));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick~a", "nick^a", 10));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick_ô", "nick_Ô", 10));

    server->casemapping = IRC_SERVER_CASEMAPPING_ASCII;

    LONGS_EQUAL(-32, irc_server_strcasecmp (server, "nick[a]", "nick{a}"));
    LONGS_EQUAL(32, irc_server_strcasecmp (server, "nick{a}", "nick[a]"));
    LONGS_EQUAL(-32, irc_server_strcasecmp (server, "nick^a", "nick~a"));
    LONGS_EQUAL(32, irc_server_strcasecmp (server, "nick~a", "nick^a"));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick_ô", "nick_Ô", 10));

    LONGS_EQUAL(-32, irc_server_strncasecmp (server, "nick[a]", "nick{a}", 10));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick{a}", "nick[a]", 10));
    LONGS_EQUAL(-32, irc_server_strncasecmp (server, "nick^a", "nick~a", 10));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick~a", "nick^a", 10));
    LONGS_EQUAL(32, irc_server_strncasecmp (server, "nick_ô", "nick_Ô", 10));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_eval_expression
 */

TEST(IrcServer, EvalExpression)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("server1");
    CHECK(server);

    WEE_TEST_STR("", irc_server_eval_expression (NULL, "${server}"));
    WEE_TEST_STR("", irc_server_eval_expression (NULL, "${irc_server}"));
    WEE_TEST_STR("", irc_server_eval_expression (NULL, "${irc_server.name}"));

    WEE_TEST_STR("server1", irc_server_eval_expression (server, "${server}"));
    WEE_TEST_STR("server1", irc_server_eval_expression (server, "${irc_server.name}"));

    str = irc_server_eval_expression (server, "${irc_server}");
    STRNCMP_EQUAL("0x", str, 2);
    free (str);

    str = irc_server_eval_expression (server, "${username}");
    CHECK(str && str[0]);
    free (str);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_fingerprint_search_algo_with_size
 */

TEST(IrcServer, FingerprintSearchAlgoWithSize)
{
    LONGS_EQUAL(-1, irc_server_fingerprint_search_algo_with_size (-1));
    LONGS_EQUAL(-1, irc_server_fingerprint_search_algo_with_size (0));
    LONGS_EQUAL(-1, irc_server_fingerprint_search_algo_with_size (-1));
    LONGS_EQUAL(-1, irc_server_fingerprint_search_algo_with_size (1024));

    LONGS_EQUAL(IRC_FINGERPRINT_ALGO_SHA1,
                irc_server_fingerprint_search_algo_with_size (160));
    LONGS_EQUAL(IRC_FINGERPRINT_ALGO_SHA256,
                irc_server_fingerprint_search_algo_with_size (256));
    LONGS_EQUAL(IRC_FINGERPRINT_ALGO_SHA512,
                irc_server_fingerprint_search_algo_with_size (512));
}

/*
 * Test functions:
 *   irc_server_eval_fingerprint
 */

TEST(IrcServer, EvalFingerprint)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("server1");
    CHECK(server);

    STRCMP_EQUAL(NULL, irc_server_eval_fingerprint (NULL));

    WEE_TEST_STR("", irc_server_eval_fingerprint (server));

    /* Invalid: evaluated to empty string */
    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
                            "${empty_value}", 1);
    STRCMP_EQUAL(NULL, irc_server_eval_fingerprint (server));

    /* Invalid fingerprint value */
    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
                            "invalid", 1);
    STRCMP_EQUAL(NULL, irc_server_eval_fingerprint (server));

    /* Invalid fingerprint value (same length as SHA-1) */
    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
                            "zzzz0aeb5ebce80ad5c201ebc358d616904czzzz", 1);
    STRCMP_EQUAL(NULL, irc_server_eval_fingerprint (server));

    /* Valid SHA-1 fingerprint */
    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
                            "340b0aeb5ebce80ad5c201ebc358d616904ca84e", 1);
    WEE_TEST_STR("340b0aeb5ebce80ad5c201ebc358d616904ca84e",
                 irc_server_eval_fingerprint (server));

    /* Valid SHA-256 fingerprint */
    config_file_option_set (
        server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
        "6a52951b8c2541c82bf11c83534631447dbae36b6576fe79fa6a5d3467eb3af9", 1);
    WEE_TEST_STR("6a52951b8c2541c82bf11c83534631447dbae36b6576fe79fa6a5d3467eb3af9",
                 irc_server_eval_fingerprint (server));

    /* Valid SHA-256 fingerprint */
    config_file_option_set (
        server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
        "738c7bb821afe25b6be60386883bd8edb3e972bee442f7d75c01aa65155b5887"
        "c3512362e1008eb71cdd343449440b0ea0559b1e11743009ddf62ab1d3618ace", 1);
    WEE_TEST_STR(
        "738c7bb821afe25b6be60386883bd8edb3e972bee442f7d75c01aa65155b5887"
        "c3512362e1008eb71cdd343449440b0ea0559b1e11743009ddf62ab1d3618ace",
        irc_server_eval_fingerprint (server));

    /* Valid SHA-1 + SHA-256 fingerprints */
    config_file_option_set (
        server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT],
        "340b0aeb5ebce80ad5c201ebc358d616904ca84e,"
        "6a52951b8c2541c82bf11c83534631447dbae36b6576fe79fa6a5d3467eb3af9", 1);
    WEE_TEST_STR(
        "340b0aeb5ebce80ad5c201ebc358d616904ca84e,"
        "6a52951b8c2541c82bf11c83534631447dbae36b6576fe79fa6a5d3467eb3af9",
        irc_server_eval_fingerprint (server));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_sasl_get_creds
 */

TEST(IrcServer, SaslGetCreds)
{
    struct t_irc_server *server;
    char *username, *password, *key;

    server = irc_server_alloc ("server1");
    CHECK(server);

    /* Options not set. */
    irc_server_sasl_get_creds (server, &username, &password, &key);
    STRCMP_EQUAL("", username);
    STRCMP_EQUAL("", password);
    STRCMP_EQUAL("", key);
    free (username);
    free (password);
    free (key);

    /* Options set and evaluated. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_USERNAME],
                            "user_${server}", 1);
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_PASSWORD],
                            "pass_${server}", 1);
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_KEY],
                            "/path/key.pem", 1);
    irc_server_sasl_get_creds (server, &username, &password, &key);
    STRCMP_EQUAL("user_server1", username);
    STRCMP_EQUAL("pass_server1", password);
    STRCMP_EQUAL("/path/key.pem", key);
    free (username);
    free (password);
    free (key);

    /* Temporary username/password (set by /auth) take precedence. */
    server->sasl_temp_username = strdup ("temp_user");
    server->sasl_temp_password = strdup ("temp_pass");
    irc_server_sasl_get_creds (server, &username, &password, &key);
    STRCMP_EQUAL("temp_user", username);
    STRCMP_EQUAL("temp_pass", password);
    STRCMP_EQUAL("temp_pass", key);
    free (username);
    free (password);
    free (key);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_sasl_enabled
 */

TEST(IrcServer, SaslEnabled)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

    /* Default mechanism (plain): username and password required. */
    LONGS_EQUAL(0, irc_server_sasl_enabled (server));
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_USERNAME],
                            "alice", 1);
    LONGS_EQUAL(0, irc_server_sasl_enabled (server));
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_PASSWORD],
                            "secret", 1);
    LONGS_EQUAL(1, irc_server_sasl_enabled (server));
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_USERNAME],
                            "", 1);
    LONGS_EQUAL(0, irc_server_sasl_enabled (server));

    /* Mechanism "external": always enabled. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_MECHANISM],
                            "external", 1);
    LONGS_EQUAL(1, irc_server_sasl_enabled (server));

    /* Mechanism "ecdsa-nist256p-challenge": username and key required. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_MECHANISM],
                            "ecdsa-nist256p-challenge", 1);
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_PASSWORD],
                            "", 1);
    LONGS_EQUAL(0, irc_server_sasl_enabled (server));
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_USERNAME],
                            "alice", 1);
    LONGS_EQUAL(0, irc_server_sasl_enabled (server));
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_KEY],
                            "/path/key.pem", 1);
    LONGS_EQUAL(1, irc_server_sasl_enabled (server));

    /* Mechanism "scram-sha-256": username and password required. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_SASL_MECHANISM],
                            "scram-sha-256", 1);
    LONGS_EQUAL(0, irc_server_sasl_enabled (server));
    server->sasl_temp_password = strdup ("temp");
    LONGS_EQUAL(1, irc_server_sasl_enabled (server));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_name_without_port
 */

TEST(IrcServer, GetNameWithoutPort)
{
    char *str;

    STRCMP_EQUAL(NULL, irc_server_get_name_without_port (NULL));

    WEE_TEST_STR("", irc_server_get_name_without_port (""));
    WEE_TEST_STR("example.com", irc_server_get_name_without_port ("example.com"));
    WEE_TEST_STR("example.com", irc_server_get_name_without_port ("example.com/6697"));
}

/*
 * Test functions:
 *   irc_server_get_short_description
 *   irc_server_set_addresses
 */

TEST(IrcServer, SetAddresses)
{
    struct t_irc_server *server;
    char *str;

    STRCMP_EQUAL(NULL, irc_server_get_short_description (NULL));

    server = irc_server_alloc ("server1");

    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS], "off", 1);

    LONGS_EQUAL(0, irc_server_set_addresses (NULL, NULL, 0));
    LONGS_EQUAL(0, irc_server_set_addresses (NULL, "irc.example.org", 0));

    LONGS_EQUAL(1, irc_server_set_addresses (server, "fake:irc.fake.org", 0));
    LONGS_EQUAL(1, server->fake_server);
    STRCMP_EQUAL("irc.fake.org", server->addresses_eval);
    LONGS_EQUAL(1, server->addresses_count);
    STRCMP_EQUAL("irc.fake.org", server->addresses_array[0]);
    LONGS_EQUAL(6667, server->ports_array[0]);
    LONGS_EQUAL(0, server->retry_array[0]);

    WEE_TEST_STR("irc.fake.org/6667 (fake, TLS: disabled)",
                 irc_server_get_short_description (server));

    LONGS_EQUAL(1, irc_server_set_addresses (server, "irc.example.org", 0));
    LONGS_EQUAL(0, server->fake_server);
    STRCMP_EQUAL("irc.example.org", server->addresses_eval);
    LONGS_EQUAL(1, server->addresses_count);
    STRCMP_EQUAL("irc.example.org", server->addresses_array[0]);
    LONGS_EQUAL(6667, server->ports_array[0]);
    LONGS_EQUAL(0, server->retry_array[0]);

    WEE_TEST_STR("irc.example.org/6667 (TLS: disabled)",
                 irc_server_get_short_description (server));

    LONGS_EQUAL(1,
                irc_server_set_addresses (
                    server, "irc.example.org,irc2.example.org/6666", 0));
    LONGS_EQUAL(0, server->fake_server);
    STRCMP_EQUAL("irc.example.org,irc2.example.org/6666", server->addresses_eval);
    LONGS_EQUAL(2, server->addresses_count);
    STRCMP_EQUAL("irc.example.org", server->addresses_array[0]);
    STRCMP_EQUAL("irc2.example.org", server->addresses_array[1]);
    LONGS_EQUAL(6667, server->ports_array[0]);
    LONGS_EQUAL(6666, server->ports_array[1]);
    LONGS_EQUAL(0, server->retry_array[0]);
    LONGS_EQUAL(0, server->retry_array[1]);

    WEE_TEST_STR("irc.example.org/6667, irc2.example.org/6666 (TLS: disabled)",
                 irc_server_get_short_description (server));

    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS], "on", 1);

    LONGS_EQUAL(1,
                irc_server_set_addresses (
                    server, "irc.example.org,irc2.example.org/7000", 1));
    LONGS_EQUAL(0, server->fake_server);
    STRCMP_EQUAL("irc.example.org,irc2.example.org/7000", server->addresses_eval);
    LONGS_EQUAL(2, server->addresses_count);
    STRCMP_EQUAL("irc.example.org", server->addresses_array[0]);
    STRCMP_EQUAL("irc2.example.org", server->addresses_array[1]);
    LONGS_EQUAL(6697, server->ports_array[0]);
    LONGS_EQUAL(7000, server->ports_array[1]);
    LONGS_EQUAL(0, server->retry_array[0]);
    LONGS_EQUAL(0, server->retry_array[1]);

    WEE_TEST_STR("irc.example.org/6697, irc2.example.org/7000 (TLS: enabled)",
                 irc_server_get_short_description (server));

    LONGS_EQUAL(0,
                irc_server_set_addresses (
                    server, "irc.example.org,irc2.example.org/7000", 1));
    LONGS_EQUAL(0, server->fake_server);
    STRCMP_EQUAL("irc.example.org,irc2.example.org/7000", server->addresses_eval);
    LONGS_EQUAL(2, server->addresses_count);
    STRCMP_EQUAL("irc.example.org", server->addresses_array[0]);
    STRCMP_EQUAL("irc2.example.org", server->addresses_array[1]);
    LONGS_EQUAL(6697, server->ports_array[0]);
    LONGS_EQUAL(7000, server->ports_array[1]);
    LONGS_EQUAL(0, server->retry_array[0]);
    LONGS_EQUAL(0, server->retry_array[1]);

    WEE_TEST_STR("irc.example.org/6697, irc2.example.org/7000 (TLS: enabled)",
                 irc_server_get_short_description (server));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_set_index_current_address
 */

TEST(IrcServer, SetIndexCurrentAddress)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS], "off", 1);
    config_file_option_set (server->options[IRC_SERVER_OPTION_ADDRESSES],
                            "host1/1001,host2/1002,host3", 1);

    irc_server_set_index_current_address (server, 1);
    LONGS_EQUAL(3, server->addresses_count);
    LONGS_EQUAL(1, server->index_current_address);
    STRCMP_EQUAL("host2", server->current_address);
    LONGS_EQUAL(1002, server->current_port);

    /* Current retry is saved for the previous address. */
    server->current_retry = 5;
    irc_server_set_index_current_address (server, 2);
    LONGS_EQUAL(2, server->index_current_address);
    STRCMP_EQUAL("host3", server->current_address);
    LONGS_EQUAL(6667, server->current_port);
    LONGS_EQUAL(0, server->current_retry);
    LONGS_EQUAL(5, server->retry_array[1]);

    /* Index modulo number of addresses, retry of the address is restored. */
    irc_server_set_index_current_address (server, 4);
    LONGS_EQUAL(1, server->index_current_address);
    STRCMP_EQUAL("host2", server->current_address);
    LONGS_EQUAL(5, server->current_retry);

    /* Retries of all addresses are kept when switching addresses. */
    server->current_retry = 7;
    irc_server_set_index_current_address (server, 0);
    LONGS_EQUAL(0, server->current_retry);
    irc_server_set_index_current_address (server, 2);
    irc_server_set_index_current_address (server, 1);
    LONGS_EQUAL(7, server->current_retry);

    /* TLS changed: ports are updated, retries are kept. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS], "on", 1);
    irc_server_set_index_current_address (server, 2);
    LONGS_EQUAL(6697, server->current_port);
    LONGS_EQUAL(7, server->retry_array[1]);
    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS], "off", 1);
    irc_server_set_index_current_address (server, 1);
    LONGS_EQUAL(1002, server->current_port);
    LONGS_EQUAL(7, server->current_retry);

    /* Evaluated addresses changed: index is reset to 0. */
    setenv ("WEECHAT_TEST_IRC_ADDRESSES", "host1,host2", 1);
    config_file_option_set (server->options[IRC_SERVER_OPTION_ADDRESSES],
                            "${env:WEECHAT_TEST_IRC_ADDRESSES}", 1);
    irc_server_set_index_current_address (server, 1);
    LONGS_EQUAL(2, server->addresses_count);
    LONGS_EQUAL(1, server->index_current_address);
    STRCMP_EQUAL("host2", server->current_address);
    setenv ("WEECHAT_TEST_IRC_ADDRESSES", "other1,other2,other3", 1);
    irc_server_set_index_current_address (server, 1);
    LONGS_EQUAL(3, server->addresses_count);
    LONGS_EQUAL(0, server->index_current_address);
    STRCMP_EQUAL("other1", server->current_address);
    LONGS_EQUAL(0, server->current_retry);
    unsetenv ("WEECHAT_TEST_IRC_ADDRESSES");

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_set_nicks
 */

TEST(IrcServer, SetNicks)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

    /* NULL or empty value: default nicks. */
    irc_server_set_nicks (server, NULL);
    LONGS_EQUAL(5, server->nicks_count);
    STRCMP_EQUAL("weechat1", server->nicks_array[0]);
    STRCMP_EQUAL("weechat5", server->nicks_array[4]);

    irc_server_set_nicks (server, "alice");
    LONGS_EQUAL(1, server->nicks_count);
    STRCMP_EQUAL("alice", server->nicks_array[0]);

    /* Separators are stripped and collapsed, value is evaluated. */
    irc_server_set_nicks (server, ",alice,bob,,${server},");
    LONGS_EQUAL(3, server->nicks_count);
    STRCMP_EQUAL("alice", server->nicks_array[0]);
    STRCMP_EQUAL("bob", server->nicks_array[1]);
    STRCMP_EQUAL("server1", server->nicks_array[2]);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_set_nick
 */

TEST(IrcServer, SetNick)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;

    run_cmd_quiet ("/mute /server add test_nick fake:127.0.0.1");
    run_cmd_quiet ("/connect test_nick");
    server = irc_server_search ("test_nick");
    CHECK(server);
    CHECK(server->buffer);
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    irc_server_set_nick (server, "alice");
    STRCMP_EQUAL("alice", server->nick);
    STRCMP_EQUAL("alice", gui_buffer_get_string (server->buffer, "localvar_nick"));
    STRCMP_EQUAL("alice", gui_buffer_get_string (channel->buffer, "localvar_nick"));

    /* Same nick: no change. */
    irc_server_set_nick (server, "alice");
    STRCMP_EQUAL("alice", server->nick);

    irc_server_set_nick (server, "bob");
    STRCMP_EQUAL("bob", server->nick);
    STRCMP_EQUAL("bob", gui_buffer_get_string (server->buffer, "localvar_nick"));
    STRCMP_EQUAL("bob", gui_buffer_get_string (channel->buffer, "localvar_nick"));

    irc_server_set_nick (server, NULL);
    POINTERS_EQUAL(NULL, server->nick);

    run_cmd_quiet ("/mute /disconnect test_nick");
    run_cmd_quiet ("/mute /server del test_nick");
}

/*
 * Test functions:
 *   irc_server_set_host
 */

TEST(IrcServer, SetHost)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;

    run_cmd_quiet ("/mute /server add test_host fake:127.0.0.1");
    run_cmd_quiet ("/connect test_host");
    server = irc_server_search ("test_host");
    CHECK(server);
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    irc_server_set_host (server, NULL);
    POINTERS_EQUAL(NULL, server->host);

    irc_server_set_host (server, "user@host");
    STRCMP_EQUAL("user@host", server->host);
    STRCMP_EQUAL("user@host",
                 gui_buffer_get_string (server->buffer, "localvar_host"));
    STRCMP_EQUAL("user@host",
                 gui_buffer_get_string (channel->buffer, "localvar_host"));

    /* Same host: no change. */
    irc_server_set_host (server, "user@host");
    STRCMP_EQUAL("user@host", server->host);

    irc_server_set_host (server, NULL);
    POINTERS_EQUAL(NULL, server->host);

    run_cmd_quiet ("/mute /disconnect test_host");
    run_cmd_quiet ("/mute /server del test_host");
}

/*
 * Test functions:
 *   irc_server_get_nick_index
 */

TEST(IrcServer, GetNickIndex)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

    irc_server_set_nicks (server, "alice,bob,carol");

    LONGS_EQUAL(-1, irc_server_get_nick_index (server));
    server->nick = strdup ("bob");
    LONGS_EQUAL(1, irc_server_get_nick_index (server));
    free (server->nick);
    server->nick = strdup ("carol");
    LONGS_EQUAL(2, irc_server_get_nick_index (server));
    free (server->nick);
    server->nick = strdup ("Alice");
    LONGS_EQUAL(-1, irc_server_get_nick_index (server));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_alternate_nick
 */

TEST(IrcServer, GetAlternateNick)
{
    struct t_irc_server *server;
    const char *nick;
    char expected[64];
    int i;

    server = irc_server_alloc ("server1");
    CHECK(server);

    /* Simulate the login: first nick is the one tried first. */
    irc_server_set_nicks (server, "abcde,fghi,jkl");
    server->nick = strdup ("abcde");
    server->nick_first_tried = 0;
    server->nick_alternate_number = -1;

#define WEE_NEXT_NICK(__expected)                                       \
    nick = irc_server_get_alternate_nick (server);                      \
    STRCMP_EQUAL(__expected, nick);                                     \
    if (nick)                                                           \
    {                                                                   \
        free (server->nick);                                            \
        server->nick = strdup (nick);                                   \
    }

    /* Other nicks of the list. */
    WEE_NEXT_NICK("fghi");
    WEE_NEXT_NICK("jkl");

    /* Then "_" added to the first nick, until 9 chars. */
    WEE_NEXT_NICK("abcde_");
    WEE_NEXT_NICK("abcde__");
    WEE_NEXT_NICK("abcde___");
    WEE_NEXT_NICK("abcde____");

    /* Then a number at the end, from 1 to 99. */
    for (i = 1; i <= 99; i++)
    {
        snprintf (expected, sizeof (expected), "abcde%s%d",
                  (i < 10) ? "___" : "__", i);
        WEE_NEXT_NICK(expected);
    }

    /* No more nicks. */
    WEE_NEXT_NICK(NULL);

    /* Alternate nicks disabled: stop after the list of nicks. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_NICKS_ALTERNATE],
                            "off", 1);
    free (server->nick);
    server->nick = strdup ("abcde");
    server->nick_first_tried = 0;
    server->nick_alternate_number = -1;
    WEE_NEXT_NICK("fghi");
    WEE_NEXT_NICK("jkl");
    WEE_NEXT_NICK(NULL);

    /* Nick not in the list: all nicks of the list are tried. */
    config_file_option_reset (server->options[IRC_SERVER_OPTION_NICKS_ALTERNATE],
                              1);
    free (server->nick);
    server->nick = strdup ("other");
    server->nick_first_tried = -1;
    server->nick_alternate_number = -1;
    WEE_NEXT_NICK("abcde");
    WEE_NEXT_NICK("fghi");
    WEE_NEXT_NICK("jkl");
    WEE_NEXT_NICK("abcde_");

    /* First nick longer than 9 chars: truncated, with a number. */
    irc_server_set_nicks (server, "averylongnick");
    free (server->nick);
    server->nick = strdup ("averylongnick");
    server->nick_first_tried = 0;
    server->nick_alternate_number = -1;
    WEE_NEXT_NICK("averylon1");
    WEE_NEXT_NICK("averylon2");

#undef WEE_NEXT_NICK

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_isupport_value
 */

TEST(IrcServer, GetIsupportValue)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("test_clienttagdeny");
    CHECK(server);

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup ("");

    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, NULL));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, ""));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TEST"));

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup ("AWAYLEN=307 BOT=B CASEMAPPING=ascii "
                               "CHANLIMIT=#:10 EMPTY= INVEX KICKLEN=307 WHOX");

    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, NULL));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, ""));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "xxx"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "AWAYLE"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "WHO"));

    STRCMP_EQUAL("307", irc_server_get_isupport_value (server, "AWAYLEN"));
    STRCMP_EQUAL("B", irc_server_get_isupport_value (server, "BOT"));
    STRCMP_EQUAL("ascii", irc_server_get_isupport_value (server, "CASEMAPPING"));
    STRCMP_EQUAL("#:10", irc_server_get_isupport_value (server, "CHANLIMIT"));
    STRCMP_EQUAL("", irc_server_get_isupport_value (server, "EMPTY"));
    STRCMP_EQUAL("", irc_server_get_isupport_value (server, "INVEX"));
    STRCMP_EQUAL("307", irc_server_get_isupport_value (server, "KICKLEN"));
    STRCMP_EQUAL("", irc_server_get_isupport_value (server, "WHOX"));

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup ("TEST SECOND");

    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "T"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TES"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "EST"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TESTT"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "SEC"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "COND"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "SECONDD"));

    STRCMP_EQUAL("", irc_server_get_isupport_value (server, "TEST"));
    STRCMP_EQUAL("", irc_server_get_isupport_value (server, "SECOND"));

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup ("TEST=abc");

    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "T"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TES"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "EST"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TESTT"));

    STRCMP_EQUAL("abc", irc_server_get_isupport_value (server, "TEST"));

    if (server->isupport)
        free (server->isupport);
    server->isupport = strdup ("  TEST=abc  ");

    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "T"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TES"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "EST"));
    STRCMP_EQUAL(NULL, irc_server_get_isupport_value (server, "TESTT"));

    STRCMP_EQUAL("abc", irc_server_get_isupport_value (server, "TEST"));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_set_prefix_modes_chars
 */

TEST(IrcServer, SetPrefixModesChars)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick;
    char modes[IRC_SERVER_PREFIX_MAX_MODES * 2 + 1];
    char chars[IRC_SERVER_PREFIX_MAX_MODES * 2 + 1];
    char prefix[IRC_SERVER_PREFIX_MAX_MODES * 4 + 8];

    irc_server_set_prefix_modes_chars (NULL, NULL);
    irc_server_set_prefix_modes_chars (NULL, "(ov)@+");

    run_cmd_quiet ("/mute /server add test_prefix fake:127.0.0.1");
    run_cmd_quiet ("/connect test_prefix");

    server = irc_server_search ("test_prefix");
    CHECK(server);

    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    nick = irc_nick_new (server, channel, "bob", "user@host", "@", 0, NULL, NULL);
    CHECK(nick);

    STRCMP_EQUAL(NULL, server->prefix_modes);
    STRCMP_EQUAL(NULL, server->prefix_chars);
    STRCMP_EQUAL("@ ", nick->prefixes);

    /* Reset with default values already set: no change. */
    irc_server_set_prefix_modes_chars (server, NULL);
    STRCMP_EQUAL(NULL, server->prefix_modes);
    STRCMP_EQUAL(NULL, server->prefix_chars);
    STRCMP_EQUAL("@ ", nick->prefixes);

    /* More prefix chars: nick prefixes are extended. */
    irc_server_set_prefix_modes_chars (server, "(qaohv)~&@%+");
    STRCMP_EQUAL("qaohv", server->prefix_modes);
    STRCMP_EQUAL("~&@%+", server->prefix_chars);
    STRCMP_EQUAL("@    ", nick->prefixes);

    /* Less prefix chars: nick prefixes are truncated. */
    irc_server_set_prefix_modes_chars (server, "(ov)@+");
    STRCMP_EQUAL("ov", server->prefix_modes);
    STRCMP_EQUAL("@+", server->prefix_chars);
    STRCMP_EQUAL("@ ", nick->prefixes);

    /* Missing prefix chars are replaced by spaces. */
    irc_server_set_prefix_modes_chars (server, "(ohv)@");
    STRCMP_EQUAL("ohv", server->prefix_modes);
    STRCMP_EQUAL("@  ", server->prefix_chars);
    STRCMP_EQUAL("@  ", nick->prefixes);

    /* Too many prefix modes: extra modes and chars are ignored. */
    memset (modes, 'a', sizeof (modes) - 1);
    modes[sizeof (modes) - 1] = '\0';
    memset (chars, '!', sizeof (chars) - 1);
    chars[sizeof (chars) - 1] = '\0';
    snprintf (prefix, sizeof (prefix), "(%s)%s", modes, chars);
    irc_server_set_prefix_modes_chars (server, prefix);
    LONGS_EQUAL(IRC_SERVER_PREFIX_MAX_MODES, strlen (server->prefix_modes));
    LONGS_EQUAL(IRC_SERVER_PREFIX_MAX_MODES, strlen (server->prefix_chars));
    LONGS_EQUAL(IRC_SERVER_PREFIX_MAX_MODES, strlen (nick->prefixes));
    LONGS_EQUAL(IRC_SERVER_PREFIX_MAX_MODES,
                strspn (server->prefix_modes, "a"));
    LONGS_EQUAL(IRC_SERVER_PREFIX_MAX_MODES,
                strspn (server->prefix_chars, "!"));
    CHECK(nick->prefixes[0] == '@');

    /* No prefix at all. */
    irc_server_set_prefix_modes_chars (server, "()");
    STRCMP_EQUAL("", server->prefix_modes);
    STRCMP_EQUAL("", server->prefix_chars);
    STRCMP_EQUAL("", nick->prefixes);

    /* Reset: default values are used, nick prefixes are extended. */
    irc_server_set_prefix_modes_chars (server, NULL);
    STRCMP_EQUAL(NULL, server->prefix_modes);
    STRCMP_EQUAL(NULL, server->prefix_chars);
    STRCMP_EQUAL("  ", nick->prefixes);

    irc_nick_free (server, channel, nick);
    if (channel->buffer)
        gui_buffer_close (channel->buffer);

    run_cmd_quiet ("/mute /disconnect test_prefix");
    run_cmd_quiet ("/mute /server del test_prefix");
}

/*
 * Test functions:
 *   irc_server_set_clienttagdeny
 */

TEST(IrcServer, SetClienttagdeny)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("test_clienttagdeny");
    CHECK(server);

    STRCMP_EQUAL(NULL, server->clienttagdeny);
    LONGS_EQUAL(0, server->clienttagdeny_count);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*");
    irc_server_set_clienttagdeny (server, NULL);
    STRCMP_EQUAL(NULL, server->clienttagdeny);
    LONGS_EQUAL(0, server->clienttagdeny_count);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*");
    irc_server_set_clienttagdeny (server, "");
    STRCMP_EQUAL(NULL, server->clienttagdeny);
    LONGS_EQUAL(0, server->clienttagdeny_count);
    POINTERS_EQUAL(NULL, server->clienttagdeny_array);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*");
    STRCMP_EQUAL("*", server->clienttagdeny);
    LONGS_EQUAL(1, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL(NULL, server->clienttagdeny_array[1]);
    LONGS_EQUAL(0, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*,-foo");
    STRCMP_EQUAL("*,-foo", server->clienttagdeny);
    LONGS_EQUAL(2, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL("!foo", server->clienttagdeny_array[1]);
    STRCMP_EQUAL(NULL, server->clienttagdeny_array[2]);
    LONGS_EQUAL(0, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*,-foo,-example/bar");
    STRCMP_EQUAL("*,-foo,-example/bar", server->clienttagdeny);
    LONGS_EQUAL(3, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL("!foo", server->clienttagdeny_array[1]);
    STRCMP_EQUAL("!example/bar", server->clienttagdeny_array[2]);
    STRCMP_EQUAL(NULL, server->clienttagdeny_array[3]);
    LONGS_EQUAL(0, server->typing_allowed);

    irc_server_set_clienttagdeny (server, "*,-foo,-example/bar,-typing");
    STRCMP_EQUAL("*,-foo,-example/bar,-typing", server->clienttagdeny);
    LONGS_EQUAL(4, server->clienttagdeny_count);
    STRCMP_EQUAL("*", server->clienttagdeny_array[0]);
    STRCMP_EQUAL("!foo", server->clienttagdeny_array[1]);
    STRCMP_EQUAL("!example/bar", server->clienttagdeny_array[2]);
    STRCMP_EQUAL("!typing", server->clienttagdeny_array[3]);
    STRCMP_EQUAL(NULL, server->clienttagdeny_array[4]);
    LONGS_EQUAL(1, server->typing_allowed);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_set_lag
 */

TEST(IrcServer, SetLag)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;

    run_cmd_quiet ("/mute /server add test_lag fake:127.0.0.1");
    run_cmd_quiet ("/connect test_lag");
    server = irc_server_search ("test_lag");
    CHECK(server);
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    /* Lag below the minimum to show (500 ms by default). */
    server->lag = 100;
    irc_server_set_lag (server);
    POINTERS_EQUAL(NULL, gui_buffer_get_string (server->buffer, "localvar_lag"));
    POINTERS_EQUAL(NULL, gui_buffer_get_string (channel->buffer, "localvar_lag"));

    /* Lag with milliseconds. */
    server->lag = 1234;
    server->lag_check_time.tv_sec = 0;
    irc_server_set_lag (server);
    STRCMP_EQUAL("1.234", gui_buffer_get_string (server->buffer, "localvar_lag"));
    STRCMP_EQUAL("1.234", gui_buffer_get_string (channel->buffer, "localvar_lag"));

    /* Lag check in progress: seconds only. */
    server->lag_check_time.tv_sec = 1;
    irc_server_set_lag (server);
    STRCMP_EQUAL("1", gui_buffer_get_string (server->buffer, "localvar_lag"));
    server->lag = 800;
    irc_server_set_lag (server);
    STRCMP_EQUAL("0.800", gui_buffer_get_string (server->buffer, "localvar_lag"));

    /* Back to no lag. */
    server->lag = 0;
    server->lag_check_time.tv_sec = 0;
    irc_server_set_lag (server);
    POINTERS_EQUAL(NULL, gui_buffer_get_string (server->buffer, "localvar_lag"));
    POINTERS_EQUAL(NULL, gui_buffer_get_string (channel->buffer, "localvar_lag"));

    run_cmd_quiet ("/mute /disconnect test_lag");
    run_cmd_quiet ("/mute /server del test_lag");
}

/*
 * Test functions:
 *   irc_server_get_prefix_modes
 */

TEST(IrcServer, GetPrefixModes)
{
    struct t_irc_server *server;

    STRCMP_EQUAL("ov", irc_server_get_prefix_modes (NULL));

    server = irc_server_alloc ("server1");
    CHECK(server);
    STRCMP_EQUAL("ov", irc_server_get_prefix_modes (server));
    irc_server_set_prefix_modes_chars (server, "(qaohv)~&@%+");
    STRCMP_EQUAL("qaohv", irc_server_get_prefix_modes (server));
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_prefix_chars
 */

TEST(IrcServer, GetPrefixChars)
{
    struct t_irc_server *server;

    STRCMP_EQUAL("@+", irc_server_get_prefix_chars (NULL));

    server = irc_server_alloc ("server1");
    CHECK(server);
    STRCMP_EQUAL("@+", irc_server_get_prefix_chars (server));
    irc_server_set_prefix_modes_chars (server, "(qaohv)~&@%+");
    STRCMP_EQUAL("~&@%+", irc_server_get_prefix_chars (server));
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_prefix_mode_index
 */

TEST(IrcServer, GetPrefixModeIndex)
{
    struct t_irc_server *server;

    LONGS_EQUAL(-1, irc_server_get_prefix_mode_index (NULL, 'o'));

    server = irc_server_alloc ("server1");
    CHECK(server);
    LONGS_EQUAL(0, irc_server_get_prefix_mode_index (server, 'o'));
    LONGS_EQUAL(1, irc_server_get_prefix_mode_index (server, 'v'));
    LONGS_EQUAL(-1, irc_server_get_prefix_mode_index (server, 'h'));
    irc_server_set_prefix_modes_chars (server, "(qaohv)~&@%+");
    LONGS_EQUAL(0, irc_server_get_prefix_mode_index (server, 'q'));
    LONGS_EQUAL(3, irc_server_get_prefix_mode_index (server, 'h'));
    LONGS_EQUAL(4, irc_server_get_prefix_mode_index (server, 'v'));
    LONGS_EQUAL(-1, irc_server_get_prefix_mode_index (server, 'x'));
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_prefix_char_index
 */

TEST(IrcServer, GetPrefixCharIndex)
{
    struct t_irc_server *server;

    LONGS_EQUAL(-1, irc_server_get_prefix_char_index (NULL, '@'));

    server = irc_server_alloc ("server1");
    CHECK(server);
    LONGS_EQUAL(0, irc_server_get_prefix_char_index (server, '@'));
    LONGS_EQUAL(1, irc_server_get_prefix_char_index (server, '+'));
    LONGS_EQUAL(-1, irc_server_get_prefix_char_index (server, '%'));
    irc_server_set_prefix_modes_chars (server, "(qaohv)~&@%+");
    LONGS_EQUAL(0, irc_server_get_prefix_char_index (server, '~'));
    LONGS_EQUAL(3, irc_server_get_prefix_char_index (server, '%'));
    LONGS_EQUAL(-1, irc_server_get_prefix_char_index (server, '!'));
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_prefix_mode_for_char
 */

TEST(IrcServer, GetPrefixModeForChar)
{
    struct t_irc_server *server;

    BYTES_EQUAL(' ', irc_server_get_prefix_mode_for_char (NULL, '@'));

    server = irc_server_alloc ("server1");
    CHECK(server);
    BYTES_EQUAL('o', irc_server_get_prefix_mode_for_char (server, '@'));
    BYTES_EQUAL('v', irc_server_get_prefix_mode_for_char (server, '+'));
    BYTES_EQUAL(' ', irc_server_get_prefix_mode_for_char (server, '%'));
    irc_server_set_prefix_modes_chars (server, "(qaohv)~&@%+");
    BYTES_EQUAL('q', irc_server_get_prefix_mode_for_char (server, '~'));
    BYTES_EQUAL('h', irc_server_get_prefix_mode_for_char (server, '%'));
    BYTES_EQUAL(' ', irc_server_get_prefix_mode_for_char (server, '!'));
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_prefix_char_for_mode
 */

TEST(IrcServer, GetPrefixCharForMode)
{
    struct t_irc_server *server;

    BYTES_EQUAL(' ', irc_server_get_prefix_char_for_mode (NULL, 'o'));

    server = irc_server_alloc ("server1");
    CHECK(server);
    BYTES_EQUAL('@', irc_server_get_prefix_char_for_mode (server, 'o'));
    BYTES_EQUAL('+', irc_server_get_prefix_char_for_mode (server, 'v'));
    BYTES_EQUAL(' ', irc_server_get_prefix_char_for_mode (server, 'h'));
    irc_server_set_prefix_modes_chars (server, "(qaohv)~&@%+");
    BYTES_EQUAL('~', irc_server_get_prefix_char_for_mode (server, 'q'));
    BYTES_EQUAL('%', irc_server_get_prefix_char_for_mode (server, 'h'));
    BYTES_EQUAL(' ', irc_server_get_prefix_char_for_mode (server, 'x'));
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_cap_multi_prefix_enabled
 */

TEST(IrcServer, CapMultiPrefixEnabled)
{
    struct t_irc_server *server;

    LONGS_EQUAL(0, irc_server_cap_multi_prefix_enabled (NULL));

    server = irc_server_alloc ("server1");
    CHECK(server);

    LONGS_EQUAL(0, irc_server_cap_multi_prefix_enabled (server));

    hashtable_set (server->cap_list, "away-notify", NULL);
    LONGS_EQUAL(0, irc_server_cap_multi_prefix_enabled (server));

    hashtable_set (server->cap_list, "multi-prefix", NULL);
    LONGS_EQUAL(1, irc_server_cap_multi_prefix_enabled (server));

    hashtable_remove (server->cap_list, "multi-prefix");
    LONGS_EQUAL(0, irc_server_cap_multi_prefix_enabled (server));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_chanmodes
 */

TEST(IrcServer, GetChanmodes)
{
    struct t_irc_server *server;

    STRCMP_EQUAL("beI,k,l", irc_server_get_chanmodes (NULL));

    server = irc_server_alloc ("server1");
    CHECK(server);
    STRCMP_EQUAL("beI,k,l", irc_server_get_chanmodes (server));
    server->chanmodes = strdup ("eIbq,k,flj,CFLMPQScgimnprstuz");
    STRCMP_EQUAL("eIbq,k,flj,CFLMPQScgimnprstuz",
                 irc_server_get_chanmodes (server));
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_prefix_char_statusmsg
 */

TEST(IrcServer, PrefixCharStatusmsg)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

    /* No STATUSMSG: prefix chars are used. */
    LONGS_EQUAL(1, irc_server_prefix_char_statusmsg (server, '@'));
    LONGS_EQUAL(1, irc_server_prefix_char_statusmsg (server, '+'));
    LONGS_EQUAL(0, irc_server_prefix_char_statusmsg (server, '%'));
    LONGS_EQUAL(0, irc_server_prefix_char_statusmsg (server, '#'));

    /* STATUSMSG is used if defined. */
    free (server->isupport);
    server->isupport = strdup ("CHANTYPES=# STATUSMSG=@% PREFIX=(ov)@+");
    LONGS_EQUAL(1, irc_server_prefix_char_statusmsg (server, '@'));
    LONGS_EQUAL(1, irc_server_prefix_char_statusmsg (server, '%'));
    LONGS_EQUAL(0, irc_server_prefix_char_statusmsg (server, '+'));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_max_modes
 */

TEST(IrcServer, GetMaxModes)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

#define WEE_CHECK_MAX_MODES(__expected, __isupport)                     \
    free (server->isupport);                                            \
    server->isupport = strdup (__isupport);                             \
    LONGS_EQUAL(__expected, irc_server_get_max_modes (server));

    /* Default is 4. */
    WEE_CHECK_MAX_MODES(4, "");
    WEE_CHECK_MAX_MODES(4, "MODES");
    WEE_CHECK_MAX_MODES(4, "MODES=");
    WEE_CHECK_MAX_MODES(4, "MODES=abc");

    WEE_CHECK_MAX_MODES(1, "MODES=1");
    WEE_CHECK_MAX_MODES(6, "CHANTYPES=# MODES=6 NETWORK=test");
    WEE_CHECK_MAX_MODES(128, "MODES=128");

    /* Min is 1, max is 128. */
    WEE_CHECK_MAX_MODES(1, "MODES=0");
    WEE_CHECK_MAX_MODES(1, "MODES=-5");
    WEE_CHECK_MAX_MODES(128, "MODES=129");
    WEE_CHECK_MAX_MODES(128, "MODES=99999");

#undef WEE_CHECK_MAX_MODES

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_get_default_msg
 */

TEST(IrcServer, GetDefaultMsg)
{
    struct t_irc_server *server;
    char *str, *version, expected[256];

    server = irc_server_alloc ("server1");
    CHECK(server);
    server->nick = strdup ("alice");

    WEE_TEST_STR("", irc_server_get_default_msg ("", server, NULL, NULL));
    WEE_TEST_STR("bye", irc_server_get_default_msg ("bye", server, NULL, NULL));

    /* Deprecated "%v": replaced by WeeChat version. */
    version = hook_info_get (NULL, "version", "");
    CHECK(version);
    snprintf (expected, sizeof (expected), "WeeChat %s", version);
    WEE_TEST_STR(expected,
                 irc_server_get_default_msg ("WeeChat %v", server, NULL, NULL));
    free (version);

    /* "%v" not replaced if there is an expression. */
    WEE_TEST_STR("%v server1",
                 irc_server_get_default_msg ("%v ${server}", server,
                                             NULL, NULL));

    /* Variables: server, channel, nick, target. */
    WEE_TEST_STR("server1//alice/",
                 irc_server_get_default_msg (
                     "${server}/${channel}/${nick}/${target}", server,
                     NULL, NULL));
    WEE_TEST_STR("server1/#test/alice/bob",
                 irc_server_get_default_msg (
                     "${server}/${channel}/${nick}/${target}", server,
                     "#test", "bob"));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_has_channels
 */

TEST(IrcServer, HasChannels)
{
    struct t_irc_server *server;

    LONGS_EQUAL(0, irc_server_has_channels (NULL));

    run_cmd_quiet ("/mute /server add test_channels fake:127.0.0.1");
    run_cmd_quiet ("/connect test_channels");
    server = irc_server_search ("test_channels");
    CHECK(server);

    LONGS_EQUAL(0, irc_server_has_channels (server));

    /* A private buffer is not a channel. */
    CHECK(irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0));
    LONGS_EQUAL(0, irc_server_has_channels (server));

    CHECK(irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0));
    LONGS_EQUAL(1, irc_server_has_channels (server));

    run_cmd_quiet ("/mute /disconnect test_channels");
    run_cmd_quiet ("/mute /server del test_channels");
}

/*
 * Test functions:
 *   irc_server_alloc
 */

TEST(IrcServer, Alloc)
{
    struct t_irc_server *server;
    int i;

    server = irc_server_alloc ("server1");
    CHECK(server);

    /* Another server with same name is not allowed. */
    POINTERS_EQUAL(NULL, irc_server_alloc ("server1"));

    POINTERS_EQUAL(server, irc_server_search ("server1"));
    POINTERS_EQUAL(server, last_irc_server);
    STRCMP_EQUAL("server1", server->name);
    LONGS_EQUAL(0, server->fake_server);
    LONGS_EQUAL(-1, server->sock);
    LONGS_EQUAL(0, server->is_connected);
    LONGS_EQUAL(0, server->addresses_count);
    POINTERS_EQUAL(NULL, server->nick);
    LONGS_EQUAL(-1, server->nick_alternate_number);
    CHECK(server->cap_ls);
    CHECK(server->cap_list);
    LONGS_EQUAL(IRC_SERVER_CASEMAPPING_RFC1459, server->casemapping);
    POINTERS_EQUAL(NULL, server->buffer);
    POINTERS_EQUAL(NULL, server->channels);
    for (i = 0; i < IRC_SERVER_NUM_OUTQUEUES_PRIO; i++)
    {
        POINTERS_EQUAL(NULL, server->outqueue[i]);
    }

    /* Options are created, without value (inherited from defaults). */
    for (i = 0; i < IRC_SERVER_NUM_OPTIONS; i++)
    {
        CHECK(server->options[i]);
    }
    LONGS_EQUAL(1, config_file_option_is_null (
                    server->options[IRC_SERVER_OPTION_NICKS]));

    irc_server_free (server);
    POINTERS_EQUAL(NULL, irc_server_search ("server1"));
}

/*
 * Test functions:
 *   irc_server_option_set_no_eval
 */

TEST(IrcServer, OptionSetNoEval)
{
    struct t_irc_server *server;
    char *str;

    server = irc_server_alloc ("my_ircd");
    CHECK(server);

    irc_server_option_set_no_eval (NULL, "test");
    irc_server_option_set_no_eval (
        server->options[IRC_SERVER_OPTION_PASSWORD], NULL);
    POINTERS_EQUAL(NULL,
                   CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));

    irc_server_option_set_no_eval (
        server->options[IRC_SERVER_OPTION_PASSWORD], "secret");
    STRCMP_EQUAL("secret",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));

    irc_server_option_set_no_eval (
        server->options[IRC_SERVER_OPTION_PASSWORD], "a${info:version}b${x}");
    STRCMP_EQUAL("a\\${info:version}b\\${x}",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    str = irc_server_eval_expression (
        server,
        CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL("a${info:version}b${x}", str);
    free (str);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_alloc_with_url
 */

TEST(IrcServer, AllocWithUrl)
{
    struct t_irc_server *server;

    POINTERS_EQUAL(NULL, irc_server_alloc_with_url (NULL));
    POINTERS_EQUAL(NULL, irc_server_alloc_with_url (""));
    POINTERS_EQUAL(NULL, irc_server_alloc_with_url ("test"));
    POINTERS_EQUAL(NULL, irc_server_alloc_with_url ("test://irc.example.org"));

    /* Address */
    server = irc_server_alloc_with_url ("irc://irc.example.org");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6667",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* Address/port */
    server = irc_server_alloc_with_url ("irc://irc.example.org:7000");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/7000",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* Address/port, IPv6 */
    server = irc_server_alloc_with_url ("irc6://irc.example.org:7000");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/7000",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* Address, TLS */
    server = irc_server_alloc_with_url ("ircs://irc.example.org");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6697",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* Address, IPv6, TLS */
    server = irc_server_alloc_with_url ("irc6s://irc.example.org");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6697",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* Address/port, TLS */
    server = irc_server_alloc_with_url ("ircs://irc.example.org:7000");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/7000",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* Address, nick */
    server = irc_server_alloc_with_url ("irc://alice@irc.example.org");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6667",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL("alice,alice2,alice3,alice4,alice5",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* Address, nick, password */
    server = irc_server_alloc_with_url ("irc://alice:secret@irc.example.org");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6667",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL("secret",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL("alice,alice2,alice3,alice4,alice5",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL(NULL, CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* Address, nick, password, channels */
    server = irc_server_alloc_with_url (
        "irc://alice:secret@irc.example.org/#test1,#test2");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/6667",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_IPV6]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    STRCMP_EQUAL("secret",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL("alice,alice2,alice3,alice4,alice5",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL("#test1,#test2",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);

    /* "${" is escaped in options (they must not be evaluated). */
    server = irc_server_alloc_with_url (
        "irc://${n}:${p}@irc.example.org:${x}/#${info:version},#a${b}");
    CHECK(server);
    STRCMP_EQUAL(server->name, "irc.example.org");
    STRCMP_EQUAL("irc.example.org/\\${x}",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_ADDRESSES]));
    STRCMP_EQUAL("\\${p}",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_PASSWORD]));
    STRCMP_EQUAL("\\${n},\\${n}2,\\${n}3,\\${n}4,\\${n}5",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));
    STRCMP_EQUAL("#\\${info:version},#a\\${b}",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_AUTOJOIN]));
    irc_server_free (server);
}


/*
 * Test functions:
 *   irc_server_apply_command_line_options
 */

TEST(IrcServer, ApplyCommandLineOptions)
{
    struct t_irc_server *server;
    char *argv[] = { (char *)"-tls", (char *)"-noautoconnect",
                     (char *)"-nicks=alice,bob", (char *)"not_an_option",
                     (char *)"-unknown", (char *)"-nounknown",
                     (char *)"-autojoin_delay=invalid", NULL };

    server = irc_server_alloc ("server1");
    CHECK(server);

    config_file_option_set (server->options[IRC_SERVER_OPTION_TLS], "off", 1);
    config_file_option_set (server->options[IRC_SERVER_OPTION_AUTOCONNECT],
                            "on", 1);

    irc_server_apply_command_line_options (server, 0, argv);
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));

    irc_server_apply_command_line_options (server, 7, argv);
    LONGS_EQUAL(1, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_TLS]));
    LONGS_EQUAL(0, CONFIG_BOOLEAN(server->options[IRC_SERVER_OPTION_AUTOCONNECT]));
    STRCMP_EQUAL("alice,bob",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_outqueue_add
 */

TEST(IrcServer, OutqueueAdd)
{
    struct t_irc_server *server;
    struct t_irc_outqueue *ptr_outqueue;

    server = irc_server_alloc ("server1");
    CHECK(server);

    irc_server_outqueue_add (server, 1, "PRIVMSG", "PRIVMSG #a :1",
                             "PRIVMSG #a :1 (modified)\r\n", 1, "tag1", NULL);
    irc_server_outqueue_add (server, 1, NULL, NULL, "TEST\r\n", 0, NULL, NULL);

    ptr_outqueue = server->outqueue[1];
    CHECK(ptr_outqueue);
    STRCMP_EQUAL("PRIVMSG", ptr_outqueue->command);
    STRCMP_EQUAL("PRIVMSG #a :1", ptr_outqueue->message_before_mod);
    STRCMP_EQUAL("PRIVMSG #a :1 (modified)\r\n",
                 ptr_outqueue->message_after_mod);
    LONGS_EQUAL(1, ptr_outqueue->modified);
    STRCMP_EQUAL("tag1", ptr_outqueue->tags);
    POINTERS_EQUAL(NULL, ptr_outqueue->prev_outqueue);

    /* Unknown command. */
    ptr_outqueue = ptr_outqueue->next_outqueue;
    CHECK(ptr_outqueue);
    STRCMP_EQUAL("unknown", ptr_outqueue->command);
    POINTERS_EQUAL(NULL, ptr_outqueue->message_before_mod);
    POINTERS_EQUAL(NULL, ptr_outqueue->tags);
    POINTERS_EQUAL(ptr_outqueue, server->last_outqueue[1]);
    POINTERS_EQUAL(NULL, ptr_outqueue->next_outqueue);

    /* Other queues are empty. */
    POINTERS_EQUAL(NULL, server->outqueue[0]);
    POINTERS_EQUAL(NULL, server->outqueue[2]);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_outqueue_free
 */

TEST(IrcServer, OutqueueFree)
{
    struct t_irc_server *server;
    struct t_irc_outqueue *msg1, *msg2, *msg3;

    server = irc_server_alloc ("server1");
    CHECK(server);

    irc_server_outqueue_free (NULL, 0, NULL);
    irc_server_outqueue_free (server, 0, NULL);

    irc_server_outqueue_add (server, 2, "A", NULL, "A\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 2, "B", NULL, "B\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 2, "C", NULL, "C\r\n", 0, NULL, NULL);
    msg1 = server->outqueue[2];
    msg2 = msg1->next_outqueue;
    msg3 = msg2->next_outqueue;

    /* Free message in the middle. */
    irc_server_outqueue_free (server, 2, msg2);
    POINTERS_EQUAL(msg1, server->outqueue[2]);
    POINTERS_EQUAL(msg3, msg1->next_outqueue);
    POINTERS_EQUAL(msg1, msg3->prev_outqueue);

    /* Free last message. */
    irc_server_outqueue_free (server, 2, msg3);
    POINTERS_EQUAL(msg1, server->last_outqueue[2]);
    POINTERS_EQUAL(NULL, msg1->next_outqueue);

    /* Free first (and only) message. */
    irc_server_outqueue_free (server, 2, msg1);
    POINTERS_EQUAL(NULL, server->outqueue[2]);
    POINTERS_EQUAL(NULL, server->last_outqueue[2]);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_outqueue_free_all
 */

TEST(IrcServer, OutqueueFreeAll)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

    irc_server_outqueue_free_all (server, 0);

    irc_server_outqueue_add (server, 0, "A", NULL, "A\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 0, "B", NULL, "B\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 1, "C", NULL, "C\r\n", 0, NULL, NULL);
    irc_server_outqueue_free_all (server, 0);
    POINTERS_EQUAL(NULL, server->outqueue[0]);
    POINTERS_EQUAL(NULL, server->last_outqueue[0]);
    CHECK(server->outqueue[1]);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_free_data
 */

TEST(IrcServer, FreeData)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_free
 */

TEST(IrcServer, Free)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_free_all
 */

TEST(IrcServer, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_copy
 */

TEST(IrcServer, Copy)
{
    struct t_irc_server *server, *server2;

    server = irc_server_alloc ("server1");
    CHECK(server);
    server->fake_server = 1;
    config_file_option_set (server->options[IRC_SERVER_OPTION_NICKS],
                            "alice,bob", 1);
    config_file_option_set (server->options[IRC_SERVER_OPTION_ANTI_FLOOD],
                            "500", 1);

    /* Copy to an existing server: not allowed. */
    POINTERS_EQUAL(NULL, irc_server_copy (server, "server1"));

    server2 = irc_server_copy (server, "server2");
    CHECK(server2);
    STRCMP_EQUAL("server2", server2->name);
    LONGS_EQUAL(1, server2->fake_server);
    STRCMP_EQUAL("alice,bob",
                 CONFIG_STRING(server2->options[IRC_SERVER_OPTION_NICKS]));
    LONGS_EQUAL(500, CONFIG_INTEGER(server2->options[IRC_SERVER_OPTION_ANTI_FLOOD]));

    /* Options not set are not copied (still inherited from defaults). */
    LONGS_EQUAL(1, config_file_option_is_null (
                    server2->options[IRC_SERVER_OPTION_PASSWORD]));

    /* Options are independent. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_NICKS],
                            "carol", 1);
    STRCMP_EQUAL("alice,bob",
                 CONFIG_STRING(server2->options[IRC_SERVER_OPTION_NICKS]));

    irc_server_free (server2);
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_rename
 */

TEST(IrcServer, Rename)
{
    struct t_irc_server *server, *server2;
    struct t_irc_channel *channel;

    server2 = irc_server_alloc ("test_rename2");
    CHECK(server2);

    run_cmd_quiet ("/mute /server add test_rename fake:127.0.0.1 "
                   "-nicks=alice");
    run_cmd_quiet ("/connect test_rename");
    server = irc_server_search ("test_rename");
    CHECK(server);
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);

    /* Rename to an existing server: not allowed. */
    LONGS_EQUAL(0, irc_server_rename (server, "test_rename2"));
    STRCMP_EQUAL("test_rename", server->name);

    LONGS_EQUAL(1, irc_server_rename (server, "test_renamed"));
    STRCMP_EQUAL("test_renamed", server->name);
    POINTERS_EQUAL(server, irc_server_search ("test_renamed"));
    POINTERS_EQUAL(NULL, irc_server_search ("test_rename"));

    /* Options are renamed. */
    POINTERS_EQUAL(NULL, test_irc_server_search_option ("irc.server.test_rename.nicks"));
    POINTERS_EQUAL(server->options[IRC_SERVER_OPTION_NICKS],
                   test_irc_server_search_option ("irc.server.test_renamed.nicks"));
    STRCMP_EQUAL("alice",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NICKS]));

    /* Buffers are renamed. */
    STRCMP_EQUAL("server.test_renamed",
                 gui_buffer_get_string (server->buffer, "name"));
    STRCMP_EQUAL("test_renamed",
                 gui_buffer_get_string (server->buffer, "short_name"));
    STRCMP_EQUAL("test_renamed",
                 gui_buffer_get_string (server->buffer, "localvar_server"));
    STRCMP_EQUAL("irc.test_renamed",
                 gui_buffer_get_string (server->buffer,
                                        "localvar_charset_modifier"));
    STRCMP_EQUAL("test_renamed.#test",
                 gui_buffer_get_string (channel->buffer, "name"));
    STRCMP_EQUAL("test_renamed",
                 gui_buffer_get_string (channel->buffer, "localvar_server"));

    run_cmd_quiet ("/mute /disconnect test_renamed");
    run_cmd_quiet ("/mute /server del test_renamed");
    irc_server_free (server2);
}

/*
 * Test functions:
 *   irc_server_reorder
 */

TEST(IrcServer, Reorder)
{
    struct t_irc_server *server1, *server2, *server3;
    const char *servers1[] = { "test_reorder3", "test_reorder1" };
    const char *servers2[] = { "unknown", "test_reorder2" };

    /* Servers added in the tests are freed, so the list is empty. */
    POINTERS_EQUAL(NULL, irc_servers);

    server1 = irc_server_alloc ("test_reorder1");
    server2 = irc_server_alloc ("test_reorder2");
    server3 = irc_server_alloc ("test_reorder3");
    CHECK(server1 && server2 && server3);

    LONGS_EQUAL(0, irc_server_reorder (NULL, 0));
    LONGS_EQUAL(0, irc_server_reorder (servers1, 0));

    /* Move server3 to first position, server1 is then the second one. */
    LONGS_EQUAL(1, irc_server_reorder (servers1, 2));
    POINTERS_EQUAL(server3, irc_servers);
    POINTERS_EQUAL(server1, server3->next_server);
    POINTERS_EQUAL(server2, server1->next_server);
    POINTERS_EQUAL(NULL, server2->next_server);
    POINTERS_EQUAL(server2, last_irc_server);
    POINTERS_EQUAL(NULL, server3->prev_server);
    POINTERS_EQUAL(server3, server1->prev_server);
    POINTERS_EQUAL(server1, server2->prev_server);

    /* Same order: nothing moved. */
    LONGS_EQUAL(0, irc_server_reorder (servers1, 2));

    /* Unknown server is ignored. */
    LONGS_EQUAL(1, irc_server_reorder (servers2, 2));
    POINTERS_EQUAL(server2, irc_servers);
    POINTERS_EQUAL(server3, server2->next_server);
    POINTERS_EQUAL(server1, server3->next_server);
    POINTERS_EQUAL(server1, last_irc_server);

    irc_server_free (server1);
    irc_server_free (server2);
    irc_server_free (server3);
}

/*
 * Test functions:
 *   irc_server_send_signal
 */

TEST(IrcServer, SendSignal)
{
    struct t_irc_server *server;
    struct t_hook *hook;

    server = irc_server_alloc ("server1");
    CHECK(server);

    hook = hook_signal (NULL, "server1,irc_*", &test_irc_server_signal_cb,
                        NULL, NULL);
    CHECK(hook);

    test_irc_server_signal_reset ();
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_server_send_signal (server, "irc_out", "PRIVMSG",
                                        "PRIVMSG #test :hello", NULL));
    STRCMP_EQUAL("server1,irc_out_PRIVMSG", test_irc_server_signal);
    STRCMP_EQUAL("PRIVMSG #test :hello", test_irc_server_signal_data);

    /* With tags. */
    test_irc_server_signal_reset ();
    irc_server_send_signal (server, "irc_outtags", "PRIVMSG",
                            "PRIVMSG #test :hello", "tag1,tag2");
    STRCMP_EQUAL("server1,irc_outtags_PRIVMSG", test_irc_server_signal);
    STRCMP_EQUAL("tag1,tag2;PRIVMSG #test :hello",
                 test_irc_server_signal_data);

    unhook (hook);
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_send
 */

TEST(IrcServer, Send)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

    /* Fake server: data is not sent, but considered as sent. */
    server->fake_server = 1;
    LONGS_EQUAL(4, irc_server_send (server, "test", 4));

    /* Empty buffer: nothing sent. */
    server->fake_server = 0;
    LONGS_EQUAL(0, irc_server_send (server, "test", 0));
    LONGS_EQUAL(0, irc_server_send (server, "test", -1));

    /* Invalid socket: error. */
    LONGS_EQUAL(-1, irc_server_send (server, "test", 4));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_set_send_default_tags
 */

TEST(IrcServer, SetSendDefaultTags)
{
    char *str;

    POINTERS_EQUAL(NULL, irc_server_get_tags_to_send (NULL));

    irc_server_set_send_default_tags ("default1,default2");
    WEE_TEST_STR("default1,default2", irc_server_get_tags_to_send (NULL));
    irc_server_set_send_default_tags (NULL);
    POINTERS_EQUAL(NULL, irc_server_get_tags_to_send (NULL));
}

/*
 * Test functions:
 *   irc_server_get_tags_to_send
 */

TEST(IrcServer, GetTagsToSend)
{
    char *str;

    irc_server_set_send_default_tags (NULL);
    POINTERS_EQUAL(NULL, irc_server_get_tags_to_send (NULL));
    WEE_TEST_STR("", irc_server_get_tags_to_send (""));
    WEE_TEST_STR("tag1,tag2", irc_server_get_tags_to_send ("tag1,tag2"));

    irc_server_set_send_default_tags ("default");
    WEE_TEST_STR("default", irc_server_get_tags_to_send (NULL));
    WEE_TEST_STR("tag1,default", irc_server_get_tags_to_send ("tag1"));
    irc_server_set_send_default_tags (NULL);
}

/*
 * Test functions:
 *   irc_server_outqueue_all_empty
 */

TEST(IrcServer, OutqueueAllEmpty)
{
    struct t_irc_server *server;
    int i;

    server = irc_server_alloc ("server1");
    CHECK(server);

    LONGS_EQUAL(1, irc_server_outqueue_all_empty (server));
    for (i = 0; i < IRC_SERVER_NUM_OUTQUEUES_PRIO; i++)
    {
        irc_server_outqueue_add (server, i, "PING", NULL, "PING :x\r\n",
                                 0, NULL, NULL);
        LONGS_EQUAL(0, irc_server_outqueue_all_empty (server));
        irc_server_outqueue_free_all (server, i);
        LONGS_EQUAL(1, irc_server_outqueue_all_empty (server));
    }

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_outqueue_timer_cb
 */

TEST(IrcServer, OutqueueTimerCb)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);
    server->fake_server = 1;

    irc_server_outqueue_add (server, 1, "A", NULL, "A\r\n", 0, NULL, NULL);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_server_outqueue_timer_cb (server, NULL, 0));
    POINTERS_EQUAL(NULL, server->outqueue[1]);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_outqueue_timer_remove
 */

TEST(IrcServer, OutqueueTimerRemove)
{
    struct t_irc_server *server;

    irc_server_outqueue_timer_remove (NULL);

    server = irc_server_alloc ("server1");
    CHECK(server);

    irc_server_outqueue_timer_remove (server);
    POINTERS_EQUAL(NULL, server->hook_timer_anti_flood);

    irc_server_outqueue_timer_add (server);
    CHECK(server->hook_timer_anti_flood);
    irc_server_outqueue_timer_remove (server);
    POINTERS_EQUAL(NULL, server->hook_timer_anti_flood);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_outqueue_timer_add
 */

TEST(IrcServer, OutqueueTimerAdd)
{
    struct t_irc_server *server;
    struct t_hook *ptr_hook;

    irc_server_outqueue_timer_add (NULL);

    server = irc_server_alloc ("server1");
    CHECK(server);

    config_file_option_set (server->options[IRC_SERVER_OPTION_ANTI_FLOOD],
                            "1500", 1);
    irc_server_outqueue_timer_add (server);
    ptr_hook = server->hook_timer_anti_flood;
    CHECK(ptr_hook);
    LONGS_EQUAL(1500, HOOK_TIMER(ptr_hook, interval));

    /* Timer is replaced. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_ANTI_FLOOD],
                            "300", 1);
    irc_server_outqueue_timer_add (server);
    CHECK(server->hook_timer_anti_flood);
    LONGS_EQUAL(300, HOOK_TIMER(server->hook_timer_anti_flood, interval));

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_outqueue_send_one_msg
 */

TEST(IrcServer, OutqueueSendOneMsg)
{
    struct t_irc_server *server;
    struct t_hook *hook;

    server = irc_server_alloc ("server1");
    CHECK(server);
    server->fake_server = 1;

    hook = hook_signal (NULL, "server1,irc_out*", &test_irc_server_signal_cb,
                        NULL, NULL);
    CHECK(hook);

    irc_server_outqueue_send_one_msg (NULL, NULL);
    irc_server_outqueue_send_one_msg (server, NULL);

    irc_server_outqueue_add (server, 1, "PRIVMSG", "PRIVMSG #a :hi",
                             "PRIVMSG #a :hello\r\n", 1, "tag1", NULL);

    test_irc_server_signal_reset ();
    irc_server_outqueue_send_one_msg (server, server->outqueue[1]);
    LONGS_EQUAL(2, test_irc_server_signal_count);

    /* Signals are sent without the final "\r\n". */
    STRCMP_EQUAL("server1,irc_outtags_PRIVMSG", test_irc_server_signal);
    STRCMP_EQUAL("tag1;PRIVMSG #a :hello", test_irc_server_signal_data);

    /* Message is unchanged. */
    STRCMP_EQUAL("PRIVMSG #a :hello\r\n",
                 server->outqueue[1]->message_after_mod);

    /* Message without content after modifier: nothing sent. */
    irc_server_outqueue_add (server, 2, "PING", "PING :x", NULL,
                             1, NULL, NULL);
    test_irc_server_signal_reset ();
    irc_server_outqueue_send_one_msg (server, server->outqueue[2]);
    LONGS_EQUAL(0, test_irc_server_signal_count);

    unhook (hook);
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_outqueue_send
 */

TEST(IrcServer, OutqueueSend)
{
    struct t_irc_server *server;
    struct t_hook *hook;

    server = irc_server_alloc ("server1");
    CHECK(server);
    server->fake_server = 1;

    hook = hook_signal (NULL, "server1,irc_out_*", &test_irc_server_signal_cb,
                        NULL, NULL);
    CHECK(hook);

    /* Empty queues: timer is removed. */
    irc_server_outqueue_timer_add (server);
    irc_server_outqueue_send (server);
    POINTERS_EQUAL(NULL, server->hook_timer_anti_flood);

    /*
     * Anti-flood enabled: one message with immediate priority, then one
     * message with high priority.
     */
    config_file_option_set (server->options[IRC_SERVER_OPTION_ANTI_FLOOD],
                            "2000", 1);
    irc_server_outqueue_add (server, 0, "A", NULL, "A\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 0, "B", NULL, "B\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 1, "C", NULL, "C\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 1, "D", NULL, "D\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 2, "E", NULL, "E\r\n", 0, NULL, NULL);
    test_irc_server_signal_reset ();
    irc_server_outqueue_send (server);
    STRCMP_EQUAL("A,C", test_irc_server_signal_commands);
    CHECK(server->hook_timer_anti_flood);

    /* Next call (from timer): next immediate and high priority messages. */
    test_irc_server_signal_reset ();
    irc_server_outqueue_send (server);
    STRCMP_EQUAL("B,D", test_irc_server_signal_commands);

    /* Then one message with low priority. */
    test_irc_server_signal_reset ();
    irc_server_outqueue_send (server);
    STRCMP_EQUAL("E", test_irc_server_signal_commands);
    CHECK(server->hook_timer_anti_flood);

    /* Queues empty: timer is removed. */
    irc_server_outqueue_send (server);
    POINTERS_EQUAL(NULL, server->hook_timer_anti_flood);

    /* Anti-flood disabled: all messages are sent, without timer. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_ANTI_FLOOD],
                            "0", 1);
    irc_server_outqueue_add (server, 2, "A", NULL, "A\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 1, "B", NULL, "B\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 2, "C", NULL, "C\r\n", 0, NULL, NULL);
    irc_server_outqueue_add (server, 0, "D", NULL, "D\r\n", 0, NULL, NULL);
    test_irc_server_signal_reset ();
    irc_server_outqueue_send (server);
    STRCMP_EQUAL("D,B,A,C", test_irc_server_signal_commands);
    LONGS_EQUAL(1, irc_server_outqueue_all_empty (server));
    POINTERS_EQUAL(NULL, server->hook_timer_anti_flood);

    unhook (hook);
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_send_one_msg
 */

TEST(IrcServer, SendOneMsg)
{
    struct t_irc_server *server;
    struct t_hook *hook;

    server = irc_server_alloc ("server1");
    CHECK(server);
    server->fake_server = 1;

    /* Not connected: immediate priority. */
    irc_server_send_one_msg (server, 0, "PING :1", NULL, "PING", NULL, NULL);
    CHECK(server->outqueue[0]);
    STRCMP_EQUAL("PING", server->outqueue[0]->command);
    STRCMP_EQUAL("PING :1\r\n", server->outqueue[0]->message_after_mod);
    POINTERS_EQUAL(NULL, server->outqueue[0]->message_before_mod);
    LONGS_EQUAL(0, server->outqueue[0]->modified);

    /* Message not modified: displayed only once in raw buffer. */
    irc_server_outqueue_send_one_msg (server, server->outqueue[0]);
    CHECK(last_irc_raw_message);
    STRCMP_EQUAL("PING :1", last_irc_raw_message->message);
    LONGS_EQUAL(IRC_RAW_FLAG_SEND, last_irc_raw_message->flags);
    CHECK(!last_irc_raw_message->prev_message
          || (strcmp (last_irc_raw_message->prev_message->message,
                      "PING :1") != 0));
    irc_server_outqueue_free_all (server, 0);

    /* Connected: low priority by default. */
    server->is_connected = 1;
    irc_server_send_one_msg (server, 0, "PING :2", NULL, "PING", NULL, NULL);
    CHECK(server->outqueue[2]);
    irc_server_outqueue_free_all (server, 2);

    /* Priority given in flags. */
    irc_server_send_one_msg (server, IRC_SERVER_SEND_OUTQ_PRIO_IMMEDIATE,
                             "PING :3", NULL, "PING", NULL, NULL);
    CHECK(server->outqueue[0]);
    irc_server_send_one_msg (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH,
                             "PING :4", NULL, "PING", NULL, NULL);
    CHECK(server->outqueue[1]);
    irc_server_send_one_msg (server, IRC_SERVER_SEND_OUTQ_PRIO_LOW,
                             "PING :5", NULL, "PING", NULL, "tag1");
    CHECK(server->outqueue[2]);
    STRCMP_EQUAL("tag1", server->outqueue[2]->tags);
    irc_server_outqueue_free_all (server, 0);
    irc_server_outqueue_free_all (server, 1);
    irc_server_outqueue_free_all (server, 2);

    /* Multiple lines: one message queued per line. */
    irc_server_send_one_msg (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH,
                             "PING :a\nPING :b", NULL, "PING", NULL, NULL);
    CHECK(server->outqueue[1]);
    STRCMP_EQUAL("PING :a\r\n", server->outqueue[1]->message_after_mod);
    CHECK(server->outqueue[1]->next_outqueue);
    STRCMP_EQUAL("PING :b\r\n",
                 server->outqueue[1]->next_outqueue->message_after_mod);
    irc_server_outqueue_free_all (server, 1);

    /* Message modified by a modifier. */
    hook = hook_modifier (NULL, "irc_out_PING", &test_irc_server_modifier_cb,
                          NULL, NULL);
    CHECK(hook);
    irc_server_send_one_msg (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH,
                             "PING :x", NULL, "PING", NULL, NULL);
    CHECK(server->outqueue[1]);
    STRCMP_EQUAL("PING :x", server->outqueue[1]->message_before_mod);
    STRCMP_EQUAL("PING :x (modified)\r\n",
                 server->outqueue[1]->message_after_mod);
    LONGS_EQUAL(1, server->outqueue[1]->modified);
    irc_server_outqueue_free_all (server, 1);

    /* Modifier returning the same message: not modified. */
    irc_server_send_one_msg (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH,
                             "PING :same", NULL, "PING", NULL, NULL);
    CHECK(server->outqueue[1]);
    POINTERS_EQUAL(NULL, server->outqueue[1]->message_before_mod);
    STRCMP_EQUAL("PING :same\r\n", server->outqueue[1]->message_after_mod);
    LONGS_EQUAL(0, server->outqueue[1]->modified);
    irc_server_outqueue_free_all (server, 1);

    /* Message dropped by a modifier. */
    irc_server_send_one_msg (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH,
                             "PING :drop", NULL, "PING", NULL, NULL);
    POINTERS_EQUAL(NULL, server->outqueue[1]);
    unhook (hook);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_sendf
 */

TEST(IrcServer, Sendf)
{
    struct t_irc_server *server;
    struct t_hook *hook_out1, *hook_out, *hook_mod;
    struct t_arraylist *list;
    char long_text[1024];

    POINTERS_EQUAL(NULL, irc_server_sendf (NULL, 0, NULL, "PING :x"));

    server = irc_server_alloc ("server1");
    CHECK(server);
    server->fake_server = 1;

    hook_out1 = hook_signal (NULL, "server1,irc_out1_*",
                             &test_irc_server_signal_cb, NULL, NULL);
    hook_out = hook_signal (NULL, "server1,irc_out_*",
                            &test_irc_server_signal_cb, NULL, NULL);
    CHECK(hook_out1 && hook_out);

    /* Not connected: message sent immediately. */
    test_irc_server_signal_reset ();
    POINTERS_EQUAL(NULL, irc_server_sendf (server, 0, NULL,
                                           "PRIVMSG %s :%s", "#test", "hello"));
    STRCMP_EQUAL("PRIVMSG,PRIVMSG", test_irc_server_signal_commands);
    STRCMP_EQUAL("PRIVMSG #test :hello", test_irc_server_signal_data);
    LONGS_EQUAL(1, irc_server_outqueue_all_empty (server));

    /* List of messages returned. */
    test_irc_server_signal_reset ();
    list = irc_server_sendf (server, IRC_SERVER_SEND_RETURN_LIST, NULL,
                             "PRIVMSG #test :hello");
    CHECK(list);
    LONGS_EQUAL(1, arraylist_size (list));
    STRCMP_EQUAL("hello", (const char *)arraylist_get (list, 0));
    arraylist_free (list);

    /* Multiline not allowed: only the first line is sent. */
    test_irc_server_signal_reset ();
    irc_server_sendf (server, 0, NULL, "PRIVMSG #test :line1\nline2");
    STRCMP_EQUAL("PRIVMSG,PRIVMSG", test_irc_server_signal_commands);
    STRCMP_EQUAL("PRIVMSG #test :line1", test_irc_server_signal_data);
    test_irc_server_signal_reset ();
    irc_server_sendf (server, 0, NULL, "PRIVMSG #test :line1\rline2");
    STRCMP_EQUAL("PRIVMSG #test :line1", test_irc_server_signal_data);

    /* Long message: split in multiple messages. */
    memset (long_text, 'a', 600);
    long_text[600] = '\0';
    test_irc_server_signal_reset ();
    list = irc_server_sendf (server, IRC_SERVER_SEND_RETURN_LIST, NULL,
                             "PRIVMSG #test :%s", long_text);
    CHECK(list);
    LONGS_EQUAL(2, arraylist_size (list));
    STRCMP_EQUAL("PRIVMSG,PRIVMSG,PRIVMSG", test_irc_server_signal_commands);
    arraylist_free (list);

    /* Message dropped by modifier "irc_out1_xxx". */
    hook_mod = hook_modifier (NULL, "irc_out1_PRIVMSG",
                              &test_irc_server_modifier_cb, NULL, NULL);
    CHECK(hook_mod);
    test_irc_server_signal_reset ();
    irc_server_sendf (server, 0, NULL, "PRIVMSG #test :drop");
    LONGS_EQUAL(0, test_irc_server_signal_count);

    /* Message modified by modifier "irc_out1_xxx". */
    test_irc_server_signal_reset ();
    irc_server_sendf (server, 0, NULL, "PRIVMSG #test :hello");
    STRCMP_EQUAL("PRIVMSG #test :hello (modified)",
                 test_irc_server_signal_data);
    unhook (hook_mod);

    /* Connected, with anti-flood: next messages are queued. */
    server->is_connected = 1;
    test_irc_server_signal_reset ();
    irc_server_sendf (server, 0, NULL, "PRIVMSG #test :msg1");
    irc_server_sendf (server, 0, NULL, "PRIVMSG #test :msg2");
    STRCMP_EQUAL("PRIVMSG,PRIVMSG,PRIVMSG", test_irc_server_signal_commands);
    CHECK(server->outqueue[2]);
    STRCMP_EQUAL("PRIVMSG #test :msg2\r\n",
                 server->outqueue[2]->message_after_mod);
    CHECK(server->hook_timer_anti_flood);

    /* Messages with immediate priority are sent anyway. */
    test_irc_server_signal_reset ();
    irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_IMMEDIATE, NULL,
                      "PING :now");
    STRCMP_EQUAL("PING,PING", test_irc_server_signal_commands);
    CHECK(server->outqueue[2]);

    unhook (hook_out);
    unhook (hook_out1);
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_msgq_add_msg
 */

TEST(IrcServer, MsgqAddMsg)
{
    struct t_irc_server *server;

    POINTERS_EQUAL(NULL, irc_recv_msgq);

    server = irc_server_alloc ("server1");
    CHECK(server);

    /* Empty message: ignored. */
    irc_server_msgq_add_msg (server, "");
    POINTERS_EQUAL(NULL, irc_recv_msgq);

    irc_server_msgq_add_msg (server, "PING :1");
    irc_server_msgq_add_msg (server, "PING :2");
    CHECK(irc_recv_msgq);
    POINTERS_EQUAL(server, irc_recv_msgq->server);
    STRCMP_EQUAL("PING :1", irc_recv_msgq->data);
    CHECK(irc_recv_msgq->next_message);
    STRCMP_EQUAL("PING :2", irc_recv_msgq->next_message->data);
    POINTERS_EQUAL(irc_recv_msgq->next_message, irc_msgq_last_msg);

    /* Unterminated message is prepended (even if message is empty). */
    server->unterminated_message = strdup ("PING :");
    irc_server_msgq_add_msg (server, "3");
    STRCMP_EQUAL("PING :3", irc_msgq_last_msg->data);
    POINTERS_EQUAL(NULL, server->unterminated_message);
    server->unterminated_message = strdup ("PING :4");
    irc_server_msgq_add_msg (server, "");
    STRCMP_EQUAL("PING :4", irc_msgq_last_msg->data);

    test_irc_server_msgq_free ();
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_msgq_add_unterminated
 */

TEST(IrcServer, MsgqAddUnterminated)
{
    struct t_irc_server *server;

    server = irc_server_alloc ("server1");
    CHECK(server);

    irc_server_msgq_add_unterminated (server, "");
    POINTERS_EQUAL(NULL, server->unterminated_message);

    irc_server_msgq_add_unterminated (server, "PING");
    STRCMP_EQUAL("PING", server->unterminated_message);
    irc_server_msgq_add_unterminated (server, "");
    STRCMP_EQUAL("PING", server->unterminated_message);
    irc_server_msgq_add_unterminated (server, " :test");
    STRCMP_EQUAL("PING :test", server->unterminated_message);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_msgq_add_buffer
 */

TEST(IrcServer, MsgqAddBuffer)
{
    struct t_irc_server *server;
    char buffer[256];

    POINTERS_EQUAL(NULL, irc_recv_msgq);

    server = irc_server_alloc ("server1");
    CHECK(server);

    /* "\r\n", "\n" and "\r" alone ("\r" is ignored). */
    snprintf (buffer, sizeof (buffer),
              "PING :1\r\nPING :2\nPI\rNG :3\r\n\r\nPING :4");
    irc_server_msgq_add_buffer (server, buffer);
    CHECK(irc_recv_msgq);
    STRCMP_EQUAL("PING :1", irc_recv_msgq->data);
    STRCMP_EQUAL("PING :2", irc_recv_msgq->next_message->data);
    STRCMP_EQUAL("PING :3", irc_recv_msgq->next_message->next_message->data);
    POINTERS_EQUAL(irc_recv_msgq->next_message->next_message,
                   irc_msgq_last_msg);
    STRCMP_EQUAL("PING :4", server->unterminated_message);

    /* End of unterminated message. */
    snprintf (buffer, sizeof (buffer), "2\r\n");
    irc_server_msgq_add_buffer (server, buffer);
    STRCMP_EQUAL("PING :42", irc_msgq_last_msg->data);
    POINTERS_EQUAL(NULL, server->unterminated_message);

    test_irc_server_msgq_free ();
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_msgq_flush
 */

TEST(IrcServer, MsgqFlush)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_recv_cb
 */

TEST(IrcServer, RecvCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_timer_connection_cb
 */

TEST(IrcServer, TimerConnectionCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_timer_sasl_cb
 */

TEST(IrcServer, TimerSaslCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_check_join_manual_cb
 */

TEST(IrcServer, CheckJoinManualCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_check_join_noswitch_cb
 */

TEST(IrcServer, CheckJoinNoswitchCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_check_join_smart_filtered_cb
 */

TEST(IrcServer, CheckJoinSmartFilteredCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_timer_cb
 */

TEST(IrcServer, TimerCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_close_connection
 */

TEST(IrcServer, CloseConnection)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_reconnect_schedule
 */

TEST(IrcServer, ReconnectSchedule)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_login
 */

TEST(IrcServer, Login)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_switch_address
 */

TEST(IrcServer, SwitchAddress)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_connect_cb
 */

TEST(IrcServer, ConnectCb)
{
    struct t_irc_server *server;

    run_cmd_quiet ("/mute /server add test_connect "
                   "irc.example.org,irc2.example.org");

    server = irc_server_search ("test_connect");
    CHECK(server);

    /* local hostname error: switch to next address (#540) */
    irc_server_set_index_current_address (server, 1);
    LONGS_EQUAL(2, server->addresses_count);
    STRCMP_EQUAL("irc2.example.org", server->current_address);
    LONGS_EQUAL(0, server->retry_array[1]);
    LONGS_EQUAL(0, server->reconnect_start);
    irc_server_connect_cb (server, NULL,
                           WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR,
                           0, -1, "test error", NULL);
    LONGS_EQUAL(0, server->index_current_address);
    STRCMP_EQUAL("irc.example.org", server->current_address);
    LONGS_EQUAL(1, server->retry_array[1]);
    CHECK(server->reconnect_start > 0);

    run_cmd_quiet ("/mute /server del test_connect");
}

/*
 * Test functions:
 *   irc_server_set_buffer_title
 */

TEST(IrcServer, SetBufferTitle)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_create_buffer
 */

TEST(IrcServer, CreateBuffer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_fingerprint_str_sizes
 */

TEST(IrcServer, FingerprintStrSizes)
{
    char *str;

    WEE_TEST_STR("128=SHA-512, 64=SHA-256, 40=SHA-1",
                 irc_server_fingerprint_str_sizes ());
}

/*
 * Test functions:
 *   irc_server_compare_fingerprints
 */

TEST(IrcServer, CompareFingerprints)
{
    const unsigned char fp[4] = { 0x01, 0xab, 0xcd, 0xef };

    /* size mismatch */
    LONGS_EQUAL(-1, irc_server_compare_fingerprints ("", fp, 4));
    LONGS_EQUAL(-1, irc_server_compare_fingerprints ("01abcd", fp, 4));
    LONGS_EQUAL(-1, irc_server_compare_fingerprints ("01abcdef00", fp, 4));
    LONGS_EQUAL(-1, irc_server_compare_fingerprints ("01abcdef", fp, 3));

    /* invalid hexadecimal */
    LONGS_EQUAL(-1, irc_server_compare_fingerprints ("zzabcdef", fp, 4));
    LONGS_EQUAL(-1, irc_server_compare_fingerprints ("01abcdzz", fp, 4));

    /* different fingerprint */
    LONGS_EQUAL(-1, irc_server_compare_fingerprints ("00abcdef", fp, 4));
    LONGS_EQUAL(-1, irc_server_compare_fingerprints ("01abcdee", fp, 4));
    LONGS_EQUAL(-1, irc_server_compare_fingerprints ("efcdab01", fp, 4));

    /* same fingerprint */
    LONGS_EQUAL(0, irc_server_compare_fingerprints ("01abcdef", fp, 4));
    LONGS_EQUAL(0, irc_server_compare_fingerprints ("01ABCDEF", fp, 4));
    LONGS_EQUAL(0, irc_server_compare_fingerprints ("01AbCdEf", fp, 4));
}

/*
 * Test functions:
 *   irc_server_check_certificate_fingerprint
 */

TEST(IrcServer, CheckCertificateFingerprint)
{
    struct t_irc_server *server;
    gnutls_x509_crt_t cert;
    gnutls_datum_t data;

    server = irc_server_alloc ("server1");
    CHECK(server);

    LONGS_EQUAL(GNUTLS_E_SUCCESS, gnutls_x509_crt_init (&cert));
    data.data = (unsigned char *)IRC_TEST_CERT;
    data.size = strlen (IRC_TEST_CERT);
    LONGS_EQUAL(GNUTLS_E_SUCCESS,
                gnutls_x509_crt_import (cert, &data, GNUTLS_X509_FMT_PEM));

    /* no fingerprint or invalid size */
    LONGS_EQUAL(0, irc_server_check_certificate_fingerprint (server, cert, ""));
    LONGS_EQUAL(0, irc_server_check_certificate_fingerprint (server, cert,
                                                             ","));
    LONGS_EQUAL(0, irc_server_check_certificate_fingerprint (server, cert,
                                                             "abcd"));
    LONGS_EQUAL(
        0,
        irc_server_check_certificate_fingerprint (
            server, cert, "e2e262c92ffb004a4554b889864a0e6b1c99d6"));

    /* wrong fingerprints */
    LONGS_EQUAL(
        0,
        irc_server_check_certificate_fingerprint (
            server, cert, "e2e262c92ffb004a4554b889864a0e6b1c99d6c6"));
    LONGS_EQUAL(
        0,
        irc_server_check_certificate_fingerprint (
            server, cert,
            "0000000000000000000000000000000000000000,"
            "214a2b3083b38dae6c305647958a6d7bca79b9819fb4b19d048f683ee32436e0"));

    /* good fingerprint: SHA-1, SHA-256, SHA-512 */
    LONGS_EQUAL(1, irc_server_check_certificate_fingerprint (
                    server, cert, IRC_TEST_CERT_SHA1));
    LONGS_EQUAL(1, irc_server_check_certificate_fingerprint (
                    server, cert, IRC_TEST_CERT_SHA256));
    LONGS_EQUAL(1, irc_server_check_certificate_fingerprint (
                    server, cert, IRC_TEST_CERT_SHA512));
    LONGS_EQUAL(1, irc_server_check_certificate_fingerprint (
                    server, cert, "E2E262C92FFB004A4554B889864A0E6B1C99D6C5"));

    /* good fingerprint among other ones */
    LONGS_EQUAL(
        1,
        irc_server_check_certificate_fingerprint (
            server, cert,
            "abcd,0000000000000000000000000000000000000000,"
            IRC_TEST_CERT_SHA256));
    LONGS_EQUAL(
        1,
        irc_server_check_certificate_fingerprint (
            server, cert,
            ",," IRC_TEST_CERT_SHA512 ",," IRC_TEST_CERT_SHA1 ","));

    gnutls_x509_crt_deinit (cert);

    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_gnutls_callback
 */

TEST(IrcServer, GnutlsCallback)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_connect
 */

TEST(IrcServer, Connect)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_reconnect
 */

TEST(IrcServer, Reconnect)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_auto_connect_timer_cb
 */

TEST(IrcServer, AutoConnectTimerCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_auto_connect
 */

TEST(IrcServer, AutoConnect)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_disconnect
 */

TEST(IrcServer, Disconnect)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_disconnect_all
 */

TEST(IrcServer, DisconnectAll)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_execute_command
 */

TEST(IrcServer, ExecuteCommand)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_autojoin_create_buffers
 */

TEST(IrcServer, AutojoinCreateBuffers)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_autojoin_channels
 */

TEST(IrcServer, AutojoinChannels)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_get_channel_count
 */

TEST(IrcServer, GetChannelCount)
{
    struct t_irc_server *server;

    run_cmd_quiet ("/mute /server add test_count fake:127.0.0.1");
    run_cmd_quiet ("/connect test_count");
    server = irc_server_search ("test_count");
    CHECK(server);

    LONGS_EQUAL(0, irc_server_get_channel_count (server));
    LONGS_EQUAL(0, irc_server_get_pv_count (server));

    CHECK(irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test1", 0, 0));
    CHECK(irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0));
    CHECK(irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test2", 0, 0));
    LONGS_EQUAL(2, irc_server_get_channel_count (server));
    LONGS_EQUAL(1, irc_server_get_pv_count (server));

    run_cmd_quiet ("/mute /disconnect test_count");
    run_cmd_quiet ("/mute /server del test_count");
}

/*
 * Test functions:
 *   irc_server_get_pv_count
 */

TEST(IrcServer, GetPvCount)
{
    struct t_irc_server *server;

    run_cmd_quiet ("/mute /server add test_pv fake:127.0.0.1");
    run_cmd_quiet ("/connect test_pv");
    server = irc_server_search ("test_pv");
    CHECK(server);

    LONGS_EQUAL(0, irc_server_get_pv_count (server));
    CHECK(irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0));
    CHECK(irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0));
    CHECK(irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "carol", 0, 0));
    LONGS_EQUAL(2, irc_server_get_pv_count (server));

    run_cmd_quiet ("/mute /disconnect test_pv");
    run_cmd_quiet ("/mute /server del test_pv");
}

/*
 * Test functions:
 *   irc_server_remove_away
 */

TEST(IrcServer, RemoveAway)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_irc_nick *nick;

    run_cmd_quiet ("/mute /server add test_away fake:127.0.0.1");
    run_cmd_quiet ("/connect test_away");
    server = irc_server_search ("test_away");
    CHECK(server);
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);
    nick = irc_nick_new (server, channel, "bob", "user@host", NULL, 1,
                         NULL, NULL);
    CHECK(nick);
    LONGS_EQUAL(1, nick->away);

    /* Not connected: nothing done. */
    server->last_away_check = 123;
    irc_server_remove_away (server);
    LONGS_EQUAL(1, nick->away);
    LONGS_EQUAL(123, server->last_away_check);

    server->is_connected = 1;
    irc_server_remove_away (server);
    LONGS_EQUAL(0, nick->away);
    LONGS_EQUAL(0, server->last_away_check);

    server->is_connected = 0;
    run_cmd_quiet ("/mute /disconnect test_away");
    run_cmd_quiet ("/mute /server del test_away");
}

/*
 * Test functions:
 *   irc_server_check_away
 */

TEST(IrcServer, CheckAway)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    struct t_hook *hook;

    run_cmd_quiet ("/mute /server add test_away fake:127.0.0.1");
    run_cmd_quiet ("/connect test_away");
    server = irc_server_search ("test_away");
    CHECK(server);
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);
    CHECK(irc_nick_new (server, channel, "bob", "user@host", NULL, 0,
                        NULL, NULL));

    hook = hook_signal (NULL, "test_away,irc_out_*",
                        &test_irc_server_signal_cb, NULL, NULL);
    CHECK(hook);
    config_file_option_set (server->options[IRC_SERVER_OPTION_ANTI_FLOOD],
                            "0", 1);
    config_file_option_set (server->options[IRC_SERVER_OPTION_AWAY_CHECK],
                            "5", 1);

    /* Not connected: nothing done. */
    test_irc_server_signal_reset ();
    irc_server_check_away (server);
    LONGS_EQUAL(0, test_irc_server_signal_count);
    LONGS_EQUAL(0, server->last_away_check);

    /* Connected: WHO sent on channels. */
    server->is_connected = 1;
    irc_server_check_away (server);
    STRCMP_EQUAL("WHO #test", test_irc_server_signal_data);
    CHECK(server->last_away_check > 0);
    LONGS_EQUAL(1, channel->checking_whox);

    /* WHOX supported. */
    free (server->isupport);
    server->isupport = strdup ("WHOX");
    test_irc_server_signal_reset ();
    irc_server_check_away (server);
    STRCMP_EQUAL("WHO #test %cuhsnfdar", test_irc_server_signal_data);

    unhook (hook);
    server->is_connected = 0;
    run_cmd_quiet ("/mute /disconnect test_away");
    run_cmd_quiet ("/mute /server del test_away");
}

/*
 * Test functions:
 *   irc_server_set_away
 */

TEST(IrcServer, SetAway)
{
    struct t_irc_server *server;
    struct t_irc_channel *channel, *channel_pv;
    struct t_irc_nick *nick;

    run_cmd_quiet ("/mute /server add test_away fake:127.0.0.1");
    run_cmd_quiet ("/connect test_away");
    server = irc_server_search ("test_away");
    CHECK(server);
    channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL, "#test", 0, 0);
    CHECK(channel);
    channel_pv = irc_channel_new (server, IRC_CHANNEL_TYPE_PRIVATE, "bob", 0, 0);
    CHECK(channel_pv);
    nick = irc_nick_new (server, channel, "alice", "user@host", NULL, 0,
                         NULL, NULL);
    CHECK(nick);
    server->away_message = strdup ("gone");

    /* Not connected: nothing done. */
    irc_server_set_away (server, "alice", 1);
    LONGS_EQUAL(0, nick->away);
    POINTERS_EQUAL(NULL, gui_buffer_get_string (server->buffer, "localvar_away"));

    server->is_connected = 1;
    irc_server_set_away (server, "alice", 1);
    LONGS_EQUAL(1, nick->away);
    STRCMP_EQUAL("gone", gui_buffer_get_string (server->buffer, "localvar_away"));
    STRCMP_EQUAL("gone", gui_buffer_get_string (channel->buffer, "localvar_away"));
    STRCMP_EQUAL("gone",
                 gui_buffer_get_string (channel_pv->buffer, "localvar_away"));

    /* Unknown nick: only local variables are set. */
    irc_server_set_away (server, "unknown", 0);
    LONGS_EQUAL(1, nick->away);
    POINTERS_EQUAL(NULL, gui_buffer_get_string (server->buffer, "localvar_away"));

    irc_server_set_away (server, "alice", 0);
    LONGS_EQUAL(0, nick->away);
    POINTERS_EQUAL(NULL, gui_buffer_get_string (channel->buffer, "localvar_away"));
    POINTERS_EQUAL(NULL,
                   gui_buffer_get_string (channel_pv->buffer, "localvar_away"));

    server->is_connected = 0;
    run_cmd_quiet ("/mute /disconnect test_away");
    run_cmd_quiet ("/mute /server del test_away");
}

/*
 * Test functions:
 *   irc_server_away_whois_hsignal_cb
 */

TEST(IrcServer, AwayWhoisHsignalCb)
{
    struct t_irc_server *server;
    struct t_hashtable *hashtable;

    server = irc_server_alloc ("srv_away");
    CHECK(server);
    server->nick = strdup ("alice");
    server->is_connected = 1;
    server->is_away = 1;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL, NULL);
    CHECK(hashtable);

    /* Missing server and output */
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_server_away_whois_hsignal_cb (NULL, NULL, NULL, hashtable));
    STRCMP_EQUAL(NULL, server->away_message);

    /* Error on redirection */
    hashtable_set (hashtable, "error", "timeout");
    hashtable_set (hashtable, "server", "srv_away");
    hashtable_set (hashtable, "output",
                   ":server 301 alice alice :gone for lunch");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_server_away_whois_hsignal_cb (NULL, NULL, NULL, hashtable));
    STRCMP_EQUAL(NULL, server->away_message);
    hashtable_remove (hashtable, "error");

    /* Unknown server */
    hashtable_set (hashtable, "server", "srv_unknown");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_server_away_whois_hsignal_cb (NULL, NULL, NULL, hashtable));
    STRCMP_EQUAL(NULL, server->away_message);
    hashtable_set (hashtable, "server", "srv_away");

    /* No reply 301 */
    hashtable_set (hashtable, "output",
                   ":server 311 alice alice user host * :real name\n"
                   ":server 318 alice alice :End of /WHOIS list.");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_server_away_whois_hsignal_cb (NULL, NULL, NULL, hashtable));
    STRCMP_EQUAL(NULL, server->away_message);

    /* Reply 301 for another nick */
    hashtable_set (hashtable, "output",
                   ":server 301 alice bob :gone for lunch");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_server_away_whois_hsignal_cb (NULL, NULL, NULL, hashtable));
    STRCMP_EQUAL(NULL, server->away_message);

    /* Not away any more */
    server->is_away = 0;
    hashtable_set (hashtable, "output",
                   ":server 301 alice alice :gone for lunch");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_server_away_whois_hsignal_cb (NULL, NULL, NULL, hashtable));
    STRCMP_EQUAL(NULL, server->away_message);
    server->is_away = 1;

    /* Away message received */
    hashtable_set (hashtable, "output",
                   ":server 311 alice alice user host * :real name\n"
                   ":server 301 alice ALICE :gone for lunch\n"
                   ":server 318 alice alice :End of /WHOIS list.");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_server_away_whois_hsignal_cb (NULL, NULL, NULL, hashtable));
    STRCMP_EQUAL("gone for lunch", server->away_message);

    /* Away message already known: not changed */
    hashtable_set (hashtable, "output",
                   ":server 301 alice alice :another message");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_server_away_whois_hsignal_cb (NULL, NULL, NULL, hashtable));
    STRCMP_EQUAL("gone for lunch", server->away_message);

    hashtable_free (hashtable);

    server->is_connected = 0;
    irc_server_free (server);
}

/*
 * Test functions:
 *   irc_server_xfer_send_ready_cb
 */

TEST(IrcServer, XferSendReadyCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_xfer_resume_ready_cb
 */

TEST(IrcServer, XferResumeReadyCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_xfer_send_accept_resume_cb
 */

TEST(IrcServer, XferSendAcceptResumeCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_hdata_server_cb
 */

TEST(IrcServer, HdataServerCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_add_to_infolist
 */

TEST(IrcServer, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   irc_server_print_log
 */

TEST(IrcServer, PrintLog)
{
    /* TODO: write tests */
}

TEST_GROUP(IrcServerConnected)
{
    struct t_irc_server *ptr_server;

    void server_recv (const char *command)
    {
        char str_command[4096];

        snprintf (str_command, sizeof (str_command),
                  "/command -buffer irc.server." IRC_FAKE_SERVER " irc "
                  "/server fakerecv \"%s\"",
                  command);
        run_cmd_quiet (str_command);
    }

    void setup ()
    {
        /* Create a fake server (no I/O). */
        run_cmd_quiet ("/mute /server add " IRC_FAKE_SERVER " fake:127.0.0.1 "
                       "-nicks=nick1,nick2,nick3");

        /* Connect to the fake server. */
        run_cmd_quiet ("/mute /connect " IRC_FAKE_SERVER);

        /* Get the server pointer. */
        ptr_server = irc_server_search (IRC_FAKE_SERVER);
    }

    void teardown ()
    {
        /* Disconnect and delete the fake server. */
        run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
        run_cmd_quiet ("/mute /server del " IRC_FAKE_SERVER);
        ptr_server = NULL;
    }
};

/*
 * Test functions:
 *   irc_server_build_autojoin
 */

TEST(IrcServerConnected, BuildAutojoin)
{
    char *str;

    server_recv (":server 001 alice");

    STRCMP_EQUAL(NULL, irc_server_build_autojoin (ptr_server));

    /* Join one channel. */
    server_recv (":alice!user@host JOIN #test1");
    WEE_TEST_STR("#test1", irc_server_build_autojoin (ptr_server));

    /* Simulate a "parted" channel. */
    ptr_server->channels->part = 1;
    STRCMP_EQUAL(NULL, irc_server_build_autojoin (ptr_server));

    /* Restore "part" flag. */
    ptr_server->channels->part = 0;
    WEE_TEST_STR("#test1", irc_server_build_autojoin (ptr_server));

    /* Add a key on channel. */
    server_recv (":alice!user@host MODE #test1 +k key1");
    WEE_TEST_STR("#test1 key1",
                 irc_server_build_autojoin (ptr_server));

    /* Remove key. */
    server_recv (":alice!user@host MODE #test1 -k");
    WEE_TEST_STR("#test1",
                 irc_server_build_autojoin (ptr_server));

    /* Join a second channel. */
    server_recv (":alice!user@host JOIN #test2");
    WEE_TEST_STR("#test1,#test2", irc_server_build_autojoin (ptr_server));

    /* Join a third channel. */
    server_recv (":alice!user@host JOIN #test3");
    WEE_TEST_STR("#test1,#test2,#test3",
                 irc_server_build_autojoin (ptr_server));

    /* Add key on first channel. */
    server_recv (":alice!user@host MODE #test1 +k key1");
    WEE_TEST_STR("#test1,#test2,#test3 key1",
                 irc_server_build_autojoin (ptr_server));

    /* Add key on second channel. */
    server_recv (":alice!user@host MODE #test2 +k key2");
    WEE_TEST_STR("#test1,#test2,#test3 key1,key2",
                 irc_server_build_autojoin (ptr_server));

    /* Add key on third channel. */
    server_recv (":alice!user@host MODE #test3 +k key3");
    WEE_TEST_STR("#test1,#test2,#test3 key1,key2,key3",
                 irc_server_build_autojoin (ptr_server));

    /* Remove key from first channel. */
    server_recv (":alice!user@host MODE #test1 -k");
    WEE_TEST_STR("#test2,#test3,#test1 key2,key3",
                 irc_server_build_autojoin (ptr_server));

    /* Remove key from second channel. */
    server_recv (":alice!user@host MODE #test2 -k");
    WEE_TEST_STR("#test3,#test1,#test2 key3",
                 irc_server_build_autojoin (ptr_server));

    /* Remove key from third channel. */
    server_recv (":alice!user@host MODE #test3 -k");
    WEE_TEST_STR("#test1,#test2,#test3",
                 irc_server_build_autojoin (ptr_server));
}
