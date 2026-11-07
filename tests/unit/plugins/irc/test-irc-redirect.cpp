/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC redirect functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hdata.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/core/hook/hook-hdata.h"
#include "src/core/hook/hook-hsignal.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc.h"
#include "src/plugins/irc/irc-redirect.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/logger/logger-config.h"

extern struct t_irc_redirect_pattern *irc_redirect_pattern_search (const char *name);
extern void irc_redirect_pattern_free (struct t_irc_redirect_pattern *redirect_pattern);
extern void irc_redirect_init_command (struct t_irc_redirect *redirect,
                                       const char *command);
extern int irc_redirect_message_match_hash (struct t_irc_redirect *redirect,
                                            const char *command,
                                            char **arguments_argv,
                                            int arguments_argc,
                                            struct t_hashtable *cmd_hash);
extern void irc_redirect_message_add (struct t_irc_redirect *redirect,
                                      const char *message,
                                      const char *command);
extern void irc_redirect_stop (struct t_irc_redirect *redirect,
                               const char *error);
extern void irc_server_outqueue_add (struct t_irc_server *server,
                                     int priority, const char *command,
                                     const char *msg1, const char *msg2,
                                     int modified, const char *tags,
                                     struct t_irc_redirect *redirect);
}

#define IRC_FAKE_SERVER "fake_redirect"
#define SRV_BUFFER "irc.server." IRC_FAKE_SERVER

#define RECV(__irc_msg)                                                 \
    server_recv (__irc_msg);

TEST_GROUP(IrcRedirect)
{
};

TEST_GROUP(IrcRedirectWithServer)
{
    struct t_irc_server *server = NULL;
    struct t_hook *hook_redirection = NULL;
    struct t_hashtable *result = NULL;
    int count_results;

    static int redirection_cb (const void *pointer, void *data,
                               const char *signal,
                               struct t_hashtable *hashtable)
    {
        struct t_hashtable **ptr_result;

        (void) data;
        (void) signal;

        ptr_result = (struct t_hashtable **)pointer;
        hashtable_free (*ptr_result);
        *ptr_result = hashtable_dup (hashtable);
        return WEECHAT_RC_OK;
    }

    void server_recv (const char *command)
    {
        char str_command[8192];

        snprintf (str_command, sizeof (str_command),
                  "/command -buffer " SRV_BUFFER " irc "
                  "/server fakerecv \"%s\"",
                  command);
        run_cmd_quiet (str_command);
    }

    void setup ()
    {
        config_file_option_set (logger_config_look_backlog, "0", 1);
        run_cmd_quiet ("/mute /server add " IRC_FAKE_SERVER " fake:127.0.0.1 "
                       "-nicks=alice -anti_flood=0");
        server = irc_server_search (IRC_FAKE_SERVER);
        CHECK(server);
        result = NULL;
        hook_redirection = hook_hsignal (NULL, "irc_redirection_test_*",
                                         &redirection_cb, &result, NULL);
    }

    void teardown ()
    {
        struct t_irc_redirect_pattern *ptr_pattern;

        record_stop ();
        unhook (hook_redirection);
        hashtable_free (result);
        result = NULL;
        run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
        run_cmd_quiet ("/mute /server del " IRC_FAKE_SERVER);
        server = NULL;
        ptr_pattern = irc_redirect_pattern_search ("test_pattern");
        if (ptr_pattern)
            irc_redirect_pattern_free (ptr_pattern);
        config_file_option_reset (logger_config_look_backlog, 1);
    }

    /* Connect to the fake server (without redirect). */
    void connect ()
    {
        run_cmd_quiet ("/connect " IRC_FAKE_SERVER);
        RECV(":server 001 alice :Welcome");
        LONGS_EQUAL(1, server->is_connected);
        irc_redirect_free_all (server);
    }

    /* Return a value in result of last redirection. */
    const char *res (const char *key)
    {
        return (result) ? (const char *)hashtable_get (result, key) : NULL;
    }
};

/*
 * Test functions:
 *   irc_redirect_pattern_search
 *   irc_redirect_pattern_new
 *   irc_redirect_pattern_free
 */

TEST(IrcRedirectWithServer, Pattern)
{
    struct t_irc_redirect_pattern *ptr_pattern, *pattern1, *pattern2;

    /* Default patterns. */
    POINTERS_EQUAL(NULL, irc_redirect_pattern_search (NULL));
    POINTERS_EQUAL(NULL, irc_redirect_pattern_search ("xxx"));
    ptr_pattern = irc_redirect_pattern_search ("whois");
    CHECK(ptr_pattern);
    STRCMP_EQUAL("whois", ptr_pattern->name);
    LONGS_EQUAL(0, ptr_pattern->temp_pattern);
    LONGS_EQUAL(IRC_REDIRECT_TIMEOUT_DEFAULT, ptr_pattern->timeout);
    STRCMP_EQUAL("311:1", ptr_pattern->cmd_start);
    STRCMP_EQUAL("318:1,401:1,402:1,431:1,461", ptr_pattern->cmd_stop);
    STRCMP_EQUAL("318:1", ptr_pattern->cmd_extra);

    /* Errors. */
    record_start ();
    POINTERS_EQUAL(NULL, irc_redirect_pattern_new (NULL, 0, 0, NULL, "1", NULL));
    POINTERS_EQUAL(NULL, irc_redirect_pattern_new ("test", 0, 0, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, irc_redirect_pattern_new ("test", 0, 0, NULL, "", NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "missing argument \"cmd_stop\" for redirect pattern");
    POINTERS_EQUAL(NULL, irc_redirect_pattern_new ("whois", 0, 0, NULL, "1", NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "redirect pattern \"whois\" already exists");
    record_stop ();

    /* New patterns, then free them (last, first). */
    pattern1 = irc_redirect_pattern_new ("test_pattern", 1, 30,
                                         "311", "318", "319");
    CHECK(pattern1);
    STRCMP_EQUAL("test_pattern", pattern1->name);
    LONGS_EQUAL(1, pattern1->temp_pattern);
    LONGS_EQUAL(30, pattern1->timeout);
    STRCMP_EQUAL("311", pattern1->cmd_start);
    STRCMP_EQUAL("318", pattern1->cmd_stop);
    STRCMP_EQUAL("319", pattern1->cmd_extra);
    POINTERS_EQUAL(pattern1, last_irc_redirect_pattern);
    pattern2 = irc_redirect_pattern_new ("test_pattern2", 0, 0,
                                         NULL, "318", NULL);
    CHECK(pattern2);
    LONGS_EQUAL(IRC_REDIRECT_TIMEOUT_DEFAULT, pattern2->timeout);
    POINTERS_EQUAL(NULL, pattern2->cmd_start);
    POINTERS_EQUAL(NULL, pattern2->cmd_extra);
    POINTERS_EQUAL(pattern2, last_irc_redirect_pattern);
    POINTERS_EQUAL(pattern2, pattern1->next_redirect);

    irc_redirect_pattern_free (NULL);
    irc_redirect_pattern_free (pattern1);
    POINTERS_EQUAL(NULL, irc_redirect_pattern_search ("test_pattern"));
    POINTERS_EQUAL(pattern2, last_irc_redirect_pattern);
    irc_redirect_pattern_free (pattern2);
    POINTERS_EQUAL(NULL, irc_redirect_pattern_search ("test_pattern2"));
    CHECK(last_irc_redirect_pattern != pattern2);
}

/*
 * Test functions:
 *   irc_redirect_pattern_hsignal_cb
 */

TEST(IrcRedirectWithServer, PatternHsignal)
{
    struct t_hashtable *hashtable;
    struct t_irc_redirect_pattern *ptr_pattern;

    hashtable = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING, NULL, NULL);

    LONGS_EQUAL(WEECHAT_RC_ERROR,
                irc_redirect_pattern_hsignal_cb (NULL, NULL, "irc_redirect_pattern",
                                                 NULL));

    /* Missing pattern or stop commands. */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                irc_redirect_pattern_hsignal_cb (NULL, NULL, "irc_redirect_pattern",
                                                 hashtable));
    hashtable_set (hashtable, "pattern", "test_pattern");
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                irc_redirect_pattern_hsignal_cb (NULL, NULL, "irc_redirect_pattern",
                                                 hashtable));

    /* Invalid timeout: default timeout. */
    hashtable_set (hashtable, "cmd_start", "311");
    hashtable_set (hashtable, "cmd_stop", "318");
    hashtable_set (hashtable, "cmd_extra", "319");
    hashtable_set (hashtable, "timeout", "abc");
    LONGS_EQUAL(WEECHAT_RC_OK,
                hook_hsignal_send ("irc_redirect_pattern", hashtable));
    ptr_pattern = irc_redirect_pattern_search ("test_pattern");
    CHECK(ptr_pattern);
    LONGS_EQUAL(1, ptr_pattern->temp_pattern);
    LONGS_EQUAL(IRC_REDIRECT_TIMEOUT_DEFAULT, ptr_pattern->timeout);
    STRCMP_EQUAL("319", ptr_pattern->cmd_extra);
    irc_redirect_pattern_free (ptr_pattern);

    hashtable_set (hashtable, "timeout", "45");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_redirect_pattern_hsignal_cb (NULL, NULL, "irc_redirect_pattern",
                                                 hashtable));
    ptr_pattern = irc_redirect_pattern_search ("test_pattern");
    CHECK(ptr_pattern);
    LONGS_EQUAL(45, ptr_pattern->timeout);

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   irc_redirect_new
 *   irc_redirect_new_with_commands
 *   irc_redirect_search_available
 *   irc_redirect_free
 *   irc_redirect_free_all
 */

TEST(IrcRedirectWithServer, New)
{
    struct t_irc_redirect *redirect1, *redirect2, *redirect3;
    int *value;

    /* Not connected. */
    record_start ();
    POINTERS_EQUAL(NULL, irc_redirect_new (server, "whois", "test", 1, NULL, 0,
                                           NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat", "no connection to server "
                           "\"" IRC_FAKE_SERVER "\" for redirect");

    connect ();

    /* Missing or unknown pattern, missing signal. */
    POINTERS_EQUAL(NULL, irc_redirect_new (server, NULL, "test", 1, NULL, 0,
                                           NULL));
    POINTERS_EQUAL(NULL, irc_redirect_new (server, "", "test", 1, NULL, 0,
                                           NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "missing argument \"pattern\" for redirect");
    POINTERS_EQUAL(NULL, irc_redirect_new (server, "whois", NULL, 1, NULL, 0,
                                           NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "missing argument \"signal\" for redirect");
    POINTERS_EQUAL(NULL, irc_redirect_new (server, "xxx", "test", 1, NULL, 0,
                                           NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat", "redirect pattern \"xxx\" not found");
    record_stop ();
    POINTERS_EQUAL(NULL, server->redirects);

    /* Redirect with default pattern. */
    redirect1 = irc_redirect_new (server, "whois", "test", 0, "bob", 0,
                                  "318,311");
    CHECK(redirect1);
    POINTERS_EQUAL(server, redirect1->server);
    STRCMP_EQUAL("whois", redirect1->pattern);
    STRCMP_EQUAL("test", redirect1->signal);
    LONGS_EQUAL(1, redirect1->count);
    LONGS_EQUAL(1, redirect1->current_count);
    STRCMP_EQUAL("bob", redirect1->string);
    LONGS_EQUAL(IRC_REDIRECT_TIMEOUT_DEFAULT, redirect1->timeout);
    POINTERS_EQUAL(NULL, redirect1->command);
    LONGS_EQUAL(0, redirect1->assigned_to_command);
    LONGS_EQUAL(0, redirect1->start_time);

    /* Commands (with argument index) and filter. */
    LONGS_EQUAL(1, redirect1->cmd_start->items_count);
    value = (int *)hashtable_get (redirect1->cmd_start, "311");
    CHECK(value);
    LONGS_EQUAL(1, *value);
    LONGS_EQUAL(5, redirect1->cmd_stop->items_count);
    value = (int *)hashtable_get (redirect1->cmd_stop, "461");
    CHECK(value);
    LONGS_EQUAL(-1, *value);
    LONGS_EQUAL(1, redirect1->cmd_extra->items_count);
    LONGS_EQUAL(2, redirect1->cmd_filter->items_count);
    CHECK(hashtable_has_key (redirect1->cmd_filter, "311"));

    /* Temporary pattern: removed after use. */
    CHECK(irc_redirect_pattern_new ("test_pattern", 1, 30, NULL, "pong,402",
                                    NULL));
    redirect2 = irc_redirect_new (server, "test_pattern", "test", 3, NULL, 0,
                                  NULL);
    CHECK(redirect2);
    POINTERS_EQUAL(NULL, irc_redirect_pattern_search ("test_pattern"));
    LONGS_EQUAL(3, redirect2->count);
    LONGS_EQUAL(30, redirect2->timeout);
    POINTERS_EQUAL(NULL, redirect2->cmd_start);
    CHECK(hashtable_has_key (redirect2->cmd_stop, "PONG"));
    POINTERS_EQUAL(NULL, redirect2->cmd_extra);
    POINTERS_EQUAL(NULL, redirect2->cmd_filter);

    /* Timeout given. */
    redirect3 = irc_redirect_new (server, "ping", "test", 1, NULL, 10, NULL);
    CHECK(redirect3);
    LONGS_EQUAL(10, redirect3->timeout);
    POINTERS_EQUAL(redirect1, server->redirects);
    POINTERS_EQUAL(redirect3, server->last_redirect);

    /* Search first redirect not assigned to a command. */
    POINTERS_EQUAL(NULL, irc_redirect_search_available (NULL));
    POINTERS_EQUAL(redirect1, irc_redirect_search_available (server));
    redirect1->assigned_to_command = 1;
    POINTERS_EQUAL(redirect2, irc_redirect_search_available (server));
    redirect2->assigned_to_command = 1;
    redirect3->assigned_to_command = 1;
    POINTERS_EQUAL(NULL, irc_redirect_search_available (server));

    /* Free redirect in the middle, then all. */
    irc_redirect_free (NULL);
    irc_redirect_free (redirect2);
    POINTERS_EQUAL(redirect3, redirect1->next_redirect);
    POINTERS_EQUAL(redirect1, redirect3->prev_redirect);
    irc_redirect_free_all (server);
    POINTERS_EQUAL(NULL, server->redirects);
    POINTERS_EQUAL(NULL, server->last_redirect);
}

/*
 * Test functions:
 *   irc_redirect_init_command
 */

TEST(IrcRedirectWithServer, InitCommand)
{
    struct t_irc_redirect *redirect;
    int old_debug;

    connect ();

    irc_redirect_init_command (NULL, "WHOIS bob");

    redirect = irc_redirect_new (server, "whois", "test", 1, NULL, 0, NULL);
    CHECK(redirect);
    irc_redirect_init_command (redirect, "WHOIS bob\r\n");
    STRCMP_EQUAL("WHOIS bob", redirect->command);
    LONGS_EQUAL(1, redirect->assigned_to_command);
    CHECK(redirect->start_time > 0);
    free (redirect->command);

    irc_redirect_init_command (redirect, "WHOIS carol\n");
    STRCMP_EQUAL("WHOIS carol", redirect->command);
    free (redirect->command);

    irc_redirect_init_command (redirect, NULL);
    POINTERS_EQUAL(NULL, redirect->command);

    /* Debug: message displayed. */
    old_debug = weechat_irc_plugin->debug;
    weechat_irc_plugin->debug = 2;
    record_start ();
    irc_redirect_init_command (redirect, "WHOIS dave");
    RECORD_CHECK_MSG_REGEX(SRV_BUFFER, "starting redirection for command "
                           "\"WHOIS dave\" on server \"" IRC_FAKE_SERVER "\" "
                           "\\(redirect pattern: \"whois\"\\)");
    weechat_irc_plugin->debug = old_debug;
}

/*
 * Test functions:
 *   irc_redirect_message_match_hash
 */

TEST(IrcRedirectWithServer, MessageMatchHash)
{
    struct t_irc_redirect *redirect;
    char *argv[] = { (char *)"alice", (char *)"bob", (char *)"user", NULL };

    connect ();

    redirect = irc_redirect_new (server, "whois", "test", 1, NULL, 0, NULL);
    CHECK(redirect);

    /* Without string. */
    LONGS_EQUAL(0, irc_redirect_message_match_hash (redirect, "999", argv, 3,
                                                    redirect->cmd_stop));
    LONGS_EQUAL(1, irc_redirect_message_match_hash (redirect, "318", argv, 3,
                                                    redirect->cmd_stop));
    LONGS_EQUAL(1, irc_redirect_message_match_hash (redirect, "318", NULL, 0,
                                                    redirect->cmd_stop));

    /* With string (case-insensitive) at the expected argument index. */
    redirect->string = strdup ("BOB");
    LONGS_EQUAL(1, irc_redirect_message_match_hash (redirect, "318", argv, 3,
                                                    redirect->cmd_stop));
    LONGS_EQUAL(0, irc_redirect_message_match_hash (redirect, "318", NULL, 0,
                                                    redirect->cmd_stop));
    LONGS_EQUAL(0, irc_redirect_message_match_hash (redirect, "318", argv, 1,
                                                    redirect->cmd_stop));
    argv[1] = (char *)"carol";
    LONGS_EQUAL(0, irc_redirect_message_match_hash (redirect, "318", argv, 3,
                                                    redirect->cmd_stop));

    /* No argument index for command: string not checked. */
    LONGS_EQUAL(1, irc_redirect_message_match_hash (redirect, "461", argv, 3,
                                                    redirect->cmd_stop));
}

/*
 * Test functions:
 *   irc_redirect_message_add
 *   irc_redirect_stop
 */

TEST(IrcRedirectWithServer, AddStop)
{
    struct t_irc_redirect *redirect;

    connect ();

    redirect = irc_redirect_new (server, "whois", "test", 2, NULL, 0, "311,318");
    CHECK(redirect);
    redirect->command = strdup ("WHOIS bob");

    /* Filter: command not in filter is ignored. */
    irc_redirect_message_add (redirect, ":server 319 alice bob :#test", "319");
    POINTERS_EQUAL(NULL, redirect->output);
    irc_redirect_message_add (redirect, ":server 311 alice bob", "311");
    STRCMP_EQUAL(":server 311 alice bob", redirect->output);
    LONGS_EQUAL(22, redirect->output_size);
    irc_redirect_message_add (redirect, ":server 318 alice bob", "318");
    STRCMP_EQUAL(":server 311 alice bob\n:server 318 alice bob",
                 redirect->output);
    LONGS_EQUAL(44, redirect->output_size);

    /* Count not reached: redirect continues. */
    redirect->cmd_start_received = 1;
    redirect->cmd_stop_received = 1;
    irc_redirect_stop (redirect, NULL);
    POINTERS_EQUAL(NULL, result);
    LONGS_EQUAL(2, redirect->current_count);
    LONGS_EQUAL(0, redirect->cmd_start_received);
    LONGS_EQUAL(0, redirect->cmd_stop_received);

    /* Count reached: hsignal sent and redirect freed. */
    irc_redirect_stop (redirect, NULL);
    CHECK(result);
    STRCMP_EQUAL("", res ("error"));
    STRCMP_EQUAL(":server 311 alice bob\n:server 318 alice bob",
                 res ("output"));
    STRCMP_EQUAL("44", res ("output_size"));
    STRCMP_EQUAL(IRC_FAKE_SERVER, res ("server"));
    STRCMP_EQUAL("whois", res ("pattern"));
    STRCMP_EQUAL("test", res ("signal"));
    STRCMP_EQUAL("WHOIS bob", res ("command"));
    POINTERS_EQUAL(NULL, server->redirects);

    /* Error: hsignal sent even if count is not reached. */
    redirect = irc_redirect_new (server, "whois", "test", 5, NULL, 0, NULL);
    CHECK(redirect);
    irc_redirect_stop (redirect, "timeout");
    STRCMP_EQUAL("timeout", res ("error"));
    STRCMP_EQUAL("", res ("output"));
    STRCMP_EQUAL("0", res ("output_size"));
    POINTERS_EQUAL(NULL, server->redirects);
}

/*
 * Test functions:
 *   irc_redirect_message (with messages received from server)
 */

TEST(IrcRedirectWithServer, MessageWhois)
{
    connect ();

    /* Redirect assigned to the next command sent. */
    CHECK(irc_redirect_new (server, "whois", "test", 1, "bob", 0, NULL));
    irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "WHOIS :bob");
    CHECK(server->redirects);
    LONGS_EQUAL(1, server->redirects->assigned_to_command);
    STRCMP_EQUAL("WHOIS :bob", server->redirects->command);

    /* Reply for another nick: not redirected. */
    RECV(":server 311 alice carol user host * :Carol");
    LONGS_EQUAL(0, server->redirects->cmd_start_received);

    /* Start, other messages, then stop (also extra): redirect ends. */
    RECV(":server 311 alice bob user host * :Bob");
    LONGS_EQUAL(1, server->redirects->cmd_start_received);
    RECV(":server 319 alice bob :#test");
    POINTERS_EQUAL(NULL, result);
    RECV(":server 318 alice bob :End of /WHOIS list.");
    CHECK(result);
    STRCMP_EQUAL("", res ("error"));
    STRCMP_EQUAL(":server 311 alice bob user host * :Bob\n"
                 ":server 319 alice bob :#test\n"
                 ":server 318 alice bob :End of /WHOIS list.",
                 res ("output"));
    STRCMP_EQUAL("WHOIS :bob", res ("command"));
    POINTERS_EQUAL(NULL, server->redirects);

    /* Stop without start (no such nick). */
    CHECK(irc_redirect_new (server, "whois", "test", 1, "xxx", 0, NULL));
    irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "WHOIS :xxx");
    RECV(":server 401 alice xxx :No such nick/channel");
    CHECK(server->redirects);
    LONGS_EQUAL(1, server->redirects->cmd_stop_received);
    RECV(":server 318 alice xxx :End of /WHOIS list.");
    STRCMP_EQUAL(":server 401 alice xxx :No such nick/channel\n"
                 ":server 318 alice xxx :End of /WHOIS list.",
                 res ("output"));
    POINTERS_EQUAL(NULL, server->redirects);
}

TEST(IrcRedirectWithServer, MessageExtra)
{
    connect ();

    /* Stop, then extra command. */
    CHECK(irc_redirect_new (server, "mode_channel", "test", 1, "#test", 0,
                            NULL));
    irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "MODE #test");
    RECV(":server 324 alice #test +nt");
    CHECK(server->redirects);
    RECV(":server 329 alice #test 1700000000");
    STRCMP_EQUAL(":server 324 alice #test +nt\n"
                 ":server 329 alice #test 1700000000",
                 res ("output"));
    POINTERS_EQUAL(NULL, server->redirects);

    /* Stop, then another command: redirect ends without it. */
    hashtable_free (result);
    result = NULL;
    CHECK(irc_redirect_new (server, "mode_channel", "test", 1, "#test", 0,
                            NULL));
    irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "MODE #test");
    RECV(":server 324 alice #test +nt");
    POINTERS_EQUAL(NULL, result);
    RECV(":server 332 alice #test :topic");
    STRCMP_EQUAL(":server 324 alice #test +nt", res ("output"));
    POINTERS_EQUAL(NULL, server->redirects);
}

TEST(IrcRedirectWithServer, MessageCount)
{
    connect ();

    /* Two replies expected (count = 2). */
    CHECK(irc_redirect_new (server, "ison", "test", 2, NULL, 0, NULL));
    irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "ISON :bob");
    RECV(":server 303 alice :bob");
    POINTERS_EQUAL(NULL, result);
    CHECK(server->redirects);
    LONGS_EQUAL(2, server->redirects->current_count);
    RECV(":server 303 alice :");
    STRCMP_EQUAL(":server 303 alice :bob\n:server 303 alice :",
                 res ("output"));
    POINTERS_EQUAL(NULL, server->redirects);

    /* Redirect not assigned to a command: messages not redirected. */
    CHECK(irc_redirect_new (server, "ison", "test", 1, NULL, 0, NULL));
    LONGS_EQUAL(0, irc_redirect_message (server, ":server 303 alice :bob",
                                         "303", "alice :bob"));
    LONGS_EQUAL(0, irc_redirect_message (NULL, "msg", "303", NULL));
    LONGS_EQUAL(0, irc_redirect_message (server, NULL, "303", NULL));
    LONGS_EQUAL(0, irc_redirect_message (server, "msg", NULL, NULL));
}

/*
 * Test functions:
 *   irc_redirect_command_hsignal_cb
 */

TEST(IrcRedirectWithServer, CommandHsignal)
{
    struct t_hashtable *hashtable;

    hashtable = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING, NULL, NULL);

    LONGS_EQUAL(WEECHAT_RC_ERROR,
                irc_redirect_command_hsignal_cb (NULL, NULL,
                                                 "irc_redirect_command", NULL));

    /* Missing or unknown server. */
    record_start ();
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                irc_redirect_command_hsignal_cb (NULL, NULL,
                                                 "irc_redirect_command",
                                                 hashtable));
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "missing argument \"server\" for redirect");
    hashtable_set (hashtable, "server", "xxx");
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                irc_redirect_command_hsignal_cb (NULL, NULL,
                                                 "irc_redirect_command",
                                                 hashtable));
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "server \"xxx\" not found for redirect");
    record_stop ();

    connect ();

    /* Invalid count/timeout: default values. */
    hashtable_set (hashtable, "server", IRC_FAKE_SERVER);
    hashtable_set (hashtable, "pattern", "whois");
    hashtable_set (hashtable, "signal", "test");
    hashtable_set (hashtable, "string", "bob");
    hashtable_set (hashtable, "count", "abc");
    hashtable_set (hashtable, "timeout", "abc");
    hashtable_set (hashtable, "cmd_filter", "318");
    LONGS_EQUAL(WEECHAT_RC_OK,
                hook_hsignal_send ("irc_redirect_command", hashtable));
    CHECK(server->redirects);
    LONGS_EQUAL(1, server->redirects->count);
    LONGS_EQUAL(IRC_REDIRECT_TIMEOUT_DEFAULT, server->redirects->timeout);
    STRCMP_EQUAL("bob", server->redirects->string);
    CHECK(server->redirects->cmd_filter);
    irc_redirect_free_all (server);

    hashtable_set (hashtable, "count", "3");
    hashtable_set (hashtable, "timeout", "20");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_redirect_command_hsignal_cb (NULL, NULL,
                                                 "irc_redirect_command",
                                                 hashtable));
    CHECK(server->redirects);
    LONGS_EQUAL(3, server->redirects->count);
    LONGS_EQUAL(20, server->redirects->timeout);

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   irc_redirect_free (pointer removed from outqueue)
 */

TEST(IrcRedirectWithServer, FreeOutqueue)
{
    struct t_irc_redirect *redirect;

    connect ();

    redirect = irc_redirect_new (server, "ping", "test", 1, NULL, 0, NULL);
    CHECK(redirect);
    /* Outqueue with low priority (index 2). */
    irc_server_outqueue_add (server, 2, "PING", "PING :test", NULL, 0, NULL,
                             redirect);
    CHECK(server->outqueue[2]);
    POINTERS_EQUAL(redirect, server->outqueue[2]->redirect);
    irc_redirect_free (redirect);
    POINTERS_EQUAL(NULL, server->outqueue[2]->redirect);
}

/*
 * Test functions:
 *   irc_redirect_hdata_redirect_pattern_cb
 *   irc_redirect_hdata_redirect_cb
 *   irc_redirect_pattern_add_to_infolist
 *   irc_redirect_add_to_infolist
 *   irc_redirect_pattern_print_log
 *   irc_redirect_print_log
 */

TEST(IrcRedirectWithServer, HdataInfolistLog)
{
    struct t_irc_redirect_pattern *ptr_pattern;
    struct t_irc_redirect *redirect;
    struct t_hdata *hdata;
    struct t_infolist *infolist;

    connect ();

    ptr_pattern = irc_redirect_pattern_search ("whois");
    redirect = irc_redirect_new (server, "whois", "test", 2, "bob", 0, "318");
    CHECK(redirect);

    hdata = hook_hdata_get (NULL, "irc_redirect_pattern");
    CHECK(hdata);
    STRCMP_EQUAL("whois", hdata_string (hdata, ptr_pattern, "name"));
    STRCMP_EQUAL("311:1", hdata_string (hdata, ptr_pattern, "cmd_start"));
    hdata = hook_hdata_get (NULL, "irc_redirect");
    CHECK(hdata);
    STRCMP_EQUAL("bob", hdata_string (hdata, redirect, "string"));
    LONGS_EQUAL(2, hdata_integer (hdata, redirect, "count"));

    LONGS_EQUAL(0, irc_redirect_pattern_add_to_infolist (NULL, ptr_pattern));
    LONGS_EQUAL(0, irc_redirect_add_to_infolist (NULL, redirect));
    infolist = infolist_new (NULL);
    LONGS_EQUAL(0, irc_redirect_pattern_add_to_infolist (infolist, NULL));
    LONGS_EQUAL(0, irc_redirect_add_to_infolist (infolist, NULL));
    LONGS_EQUAL(1, irc_redirect_pattern_add_to_infolist (infolist, ptr_pattern));
    LONGS_EQUAL(1, irc_redirect_add_to_infolist (infolist, redirect));
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL("whois", infolist_string (infolist, "name"));
    STRCMP_EQUAL("318:1,401:1,402:1,431:1,461",
                 infolist_string (infolist, "cmd_stop"));
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL(IRC_FAKE_SERVER, infolist_string (infolist, "server_name"));
    STRCMP_EQUAL("whois", infolist_string (infolist, "pattern"));
    STRCMP_EQUAL("bob", infolist_string (infolist, "string"));
    LONGS_EQUAL(2, infolist_integer (infolist, "count"));
    CHECK(strstr (infolist_string (infolist, "cmd_start"), "311:1"));
    STRCMP_EQUAL("318:(null)", infolist_string (infolist, "cmd_filter"));
    infolist_free (infolist);

    irc_redirect_pattern_print_log ();
    irc_redirect_print_log (server);
}
