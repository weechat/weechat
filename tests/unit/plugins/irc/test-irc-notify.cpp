/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC notify functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hdata.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/core/hook/hook-hdata.h"
#include "src/core/hook/hook-signal.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-notify.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/logger/logger-config.h"

extern char *irc_notify_build_message_with_nicks (struct t_irc_server *server,
                                                  const char *irc_message,
                                                  const char *separator,
                                                  int *num_nicks);
extern void irc_notify_display (struct t_irc_server *server,
                                struct t_gui_buffer *buffer,
                                struct t_irc_notify *notify);
extern const char *irc_notify_get_tags (struct t_config_option *option,
                                        const char *type, const char *nick);
extern void irc_notify_send_signal (struct t_irc_notify *notify,
                                    const char *type,
                                    const char *away_message);
extern void irc_notify_set_away_message (struct t_irc_notify *notify,
                                         const char *away_message);
extern int irc_notify_hsignal_cb (const void *pointer, void *data,
                                  const char *signal,
                                  struct t_hashtable *hashtable);
}

#define IRC_FAKE_SERVER "fake_notify"
#define SRV_BUFFER "irc.server." IRC_FAKE_SERVER

#define RECV(__irc_msg)                                                 \
    server_recv (__irc_msg);

#define CHECK_SENT(__message)                                           \
    if (!arraylist_search (sent_messages, (void *)__message,            \
                           NULL, NULL))                                 \
    {                                                                   \
        char **msg = string_dyn_alloc (1024);                           \
        string_dyn_concat (msg, "Message not sent: ", -1);              \
        string_dyn_concat (msg, __message, -1);                         \
        string_dyn_concat (msg, "\nMessages sent:\n", -1);              \
        dump_sent (msg);                                                \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_NO_MSG_SENT                                               \
    LONGS_EQUAL(0, arraylist_size (sent_messages));

TEST_GROUP(IrcNotify)
{
};

TEST_GROUP(IrcNotifyWithServer)
{
    struct t_irc_server *server = NULL;
    struct t_arraylist *sent_messages = NULL;
    struct t_hook *hook_signal_irc_out = NULL;
    struct t_hook *hook_signal_notify = NULL;
    char signal_received[1024];

    static int signal_irc_out_cb (const void *pointer, void *data,
                                  const char *signal, const char *type_data,
                                  void *signal_data)
    {
        (void) data;
        (void) signal;
        (void) type_data;

        if (signal_data)
        {
            arraylist_add ((struct t_arraylist *)pointer,
                           strdup ((const char *)signal_data));
        }
        return WEECHAT_RC_OK;
    }

    static int signal_notify_cb (const void *pointer, void *data,
                                 const char *signal, const char *type_data,
                                 void *signal_data)
    {
        (void) data;
        (void) type_data;

        snprintf ((char *)pointer, 1024, "%s:%s",
                  signal, (signal_data) ? (const char *)signal_data : "");
        return WEECHAT_RC_OK;
    }

    static int sent_msg_cmp_cb (void *data, struct t_arraylist *arraylist,
                                void *pointer1, void *pointer2)
    {
        (void) data;
        (void) arraylist;

        return strcmp ((char *)pointer1, (char *)pointer2);
    }

    static void sent_msg_free_cb (void *data, struct t_arraylist *arraylist,
                                  void *pointer)
    {
        (void) data;
        (void) arraylist;

        free (pointer);
    }

    void dump_sent (char **msg)
    {
        int i;

        for (i = 0; i < arraylist_size (sent_messages); i++)
        {
            string_dyn_concat (msg, "  \"", -1);
            string_dyn_concat (
                msg, (const char *)arraylist_get (sent_messages, i), -1);
            string_dyn_concat (msg, "\"\n", -1);
        }
    }

    void server_recv (const char *command)
    {
        char str_command[8192];

        record_start ();
        arraylist_clear (sent_messages);
        snprintf (str_command, sizeof (str_command),
                  "/command -buffer " SRV_BUFFER " irc "
                  "/server fakerecv \"%s\"",
                  command);
        run_cmd_quiet (str_command);
        record_stop ();
    }

    void setup ()
    {
        sent_messages = arraylist_new (16, 0, 1,
                                       &sent_msg_cmp_cb, NULL,
                                       &sent_msg_free_cb, NULL);
        hook_signal_irc_out = hook_signal (NULL,
                                           IRC_FAKE_SERVER ",irc_out1_*",
                                           &signal_irc_out_cb, sent_messages,
                                           NULL);
        signal_received[0] = '\0';
        hook_signal_notify = hook_signal (NULL, "irc_notify_*",
                                          &signal_notify_cb, signal_received,
                                          NULL);

        config_file_option_set (logger_config_look_backlog, "0", 1);

        /* Create a fake server (no I/O), not connected. */
        run_cmd_quiet ("/mute /server add " IRC_FAKE_SERVER " fake:127.0.0.1 "
                       "-nicks=alice -anti_flood=0");
        server = irc_server_search (IRC_FAKE_SERVER);
        CHECK(server);
    }

    void teardown ()
    {
        record_stop ();
        run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
        run_cmd_quiet ("/mute /server del " IRC_FAKE_SERVER);
        server = NULL;
        unhook (hook_signal_irc_out);
        unhook (hook_signal_notify);
        arraylist_free (sent_messages);
        config_file_option_reset (logger_config_look_backlog, 1);
    }

    /* Connect to the fake server. */
    void connect ()
    {
        run_cmd_quiet ("/connect " IRC_FAKE_SERVER);
        RECV(":server 001 alice :Welcome");
        LONGS_EQUAL(1, server->is_connected);
        arraylist_clear (sent_messages);
    }
};

/*
 * Test functions:
 *   irc_notify_valid
 *   irc_notify_search
 *   irc_notify_new
 *   irc_notify_free
 *   irc_notify_free_all
 */

TEST(IrcNotifyWithServer, NewSearchFree)
{
    struct t_irc_notify *notify1, *notify2, *notify3, notify_stack;

    POINTERS_EQUAL(NULL, irc_notify_new (NULL, "bob", 0));
    POINTERS_EQUAL(NULL, irc_notify_new (server, NULL, 0));
    POINTERS_EQUAL(NULL, irc_notify_new (server, "", 0));

    notify1 = irc_notify_new (server, "bob", 0);
    CHECK(notify1);
    POINTERS_EQUAL(server, notify1->server);
    STRCMP_EQUAL("bob", notify1->nick);
    LONGS_EQUAL(0, notify1->check_away);
    LONGS_EQUAL(-1, notify1->is_on_server);
    POINTERS_EQUAL(NULL, notify1->away_message);
    notify2 = irc_notify_new (server, "Carol", 1);
    CHECK(notify2);
    LONGS_EQUAL(2, server->notify_count);
    POINTERS_EQUAL(notify1, server->notify_list);
    POINTERS_EQUAL(notify2, server->last_notify);

    /* Search. */
    POINTERS_EQUAL(NULL, irc_notify_search (NULL, "bob"));
    POINTERS_EQUAL(NULL, irc_notify_search (server, NULL));
    POINTERS_EQUAL(NULL, irc_notify_search (server, "xxx"));
    POINTERS_EQUAL(notify1, irc_notify_search (server, "bob"));
    POINTERS_EQUAL(notify2, irc_notify_search (server, "carol"));

    /* Valid. */
    LONGS_EQUAL(0, irc_notify_valid (server, NULL));
    LONGS_EQUAL(0, irc_notify_valid (server, &notify_stack));
    LONGS_EQUAL(0, irc_notify_valid (NULL, &notify_stack));
    LONGS_EQUAL(1, irc_notify_valid (server, notify2));
    LONGS_EQUAL(1, irc_notify_valid (NULL, notify2));

    /* Limit with MONITOR. */
    server->monitor = 2;
    POINTERS_EQUAL(NULL, irc_notify_new (server, "dave", 0));
    server->monitor = 3;
    notify3 = irc_notify_new (server, "dave", 0);
    CHECK(notify3);
    server->monitor = 0;

    /* Free (middle of list, then first, then all). */
    irc_notify_free (NULL, notify2, 1);
    irc_notify_free (server, NULL, 1);
    irc_notify_free (server, notify2, 1);
    STRCMP_EQUAL("irc_notify_removed:", signal_received);
    LONGS_EQUAL(2, server->notify_count);
    POINTERS_EQUAL(notify3, notify1->next_notify);
    POINTERS_EQUAL(notify1, notify3->prev_notify);
    irc_notify_free (server, notify1, 1);
    POINTERS_EQUAL(notify3, server->notify_list);
    POINTERS_EQUAL(notify3, server->last_notify);
    irc_notify_free_all (server);
    POINTERS_EQUAL(NULL, server->notify_list);
    POINTERS_EQUAL(NULL, server->last_notify);
    LONGS_EQUAL(0, server->notify_count);
    CHECK_NO_MSG_SENT;
}

/*
 * Test functions:
 *   irc_notify_set_server_option
 *   irc_notify_new_for_server
 *   irc_notify_new_for_all_servers
 */

TEST(IrcNotifyWithServer, ServerOption)
{
    struct t_irc_notify *ptr_notify;

    irc_notify_set_server_option (NULL);

    /* Empty list. */
    irc_notify_set_server_option (server);
    STRCMP_EQUAL("", CONFIG_STRING(server->options[IRC_SERVER_OPTION_NOTIFY]));

    irc_notify_new (server, "bob", 0);
    irc_notify_new (server, "carol", 1);
    irc_notify_new (server, "dave", 0);
    irc_notify_set_server_option (server);
    STRCMP_EQUAL("bob,carol away,dave",
                 CONFIG_STRING(server->options[IRC_SERVER_OPTION_NOTIFY]));

    /* Build the notify list from the option. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_NOTIFY],
                            ",bob,carol   away/xxx,,dave AWAY,eve other", 1);
    LONGS_EQUAL(4, server->notify_count);
    ptr_notify = server->notify_list;
    STRCMP_EQUAL("bob", ptr_notify->nick);
    LONGS_EQUAL(0, ptr_notify->check_away);
    ptr_notify = ptr_notify->next_notify;
    STRCMP_EQUAL("carol", ptr_notify->nick);
    LONGS_EQUAL(1, ptr_notify->check_away);
    ptr_notify = ptr_notify->next_notify;
    STRCMP_EQUAL("dave", ptr_notify->nick);
    LONGS_EQUAL(1, ptr_notify->check_away);
    ptr_notify = ptr_notify->next_notify;
    STRCMP_EQUAL("eve", ptr_notify->nick);
    LONGS_EQUAL(0, ptr_notify->check_away);

    /* Empty option: empty list. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_NOTIFY], "", 1);
    LONGS_EQUAL(0, server->notify_count);

    /* All servers. */
    config_file_option_set (server->options[IRC_SERVER_OPTION_NOTIFY],
                            "bob", 1);
    irc_notify_new_for_all_servers ();
    LONGS_EQUAL(1, server->notify_count);
    CHECK_NO_MSG_SENT;
}

/*
 * Test functions:
 *   irc_notify_build_message_with_nicks
 */

TEST(IrcNotifyWithServer, BuildMessageWithNicks)
{
    char *str;
    int num_nicks;

    WEE_TEST_STR("ISON :",
                 irc_notify_build_message_with_nicks (server, "ISON :", " ",
                                                      &num_nicks));
    LONGS_EQUAL(0, num_nicks);

    irc_notify_new (server, "bob", 0);
    irc_notify_new (server, "carol", 1);
    WEE_TEST_STR("ISON :bob carol",
                 irc_notify_build_message_with_nicks (server, "ISON :", " ",
                                                      &num_nicks));
    LONGS_EQUAL(2, num_nicks);
    WEE_TEST_STR("MONITOR + bob,carol",
                 irc_notify_build_message_with_nicks (server, "MONITOR + ",
                                                      ",", &num_nicks));
    LONGS_EQUAL(2, num_nicks);
}

/*
 * Test functions:
 *   irc_notify_check_now
 *   irc_notify_send_monitor
 *   irc_notify_free (with MONITOR)
 *   irc_notify_free_all (with MONITOR)
 */

TEST(IrcNotifyWithServer, CheckNowMonitor)
{
    struct t_irc_notify *notify1, *notify2;
    char nick[64];
    int i;

    /* Not connected: nothing sent. */
    notify1 = irc_notify_new (server, "bob", 1);
    irc_notify_check_now (notify1);
    CHECK_NO_MSG_SENT;
    irc_notify_free_all (server);

    connect ();

    /* ISON + WHOIS. */
    notify1 = irc_notify_new (server, "bob", 1);
    irc_notify_check_now (notify1);
    CHECK_SENT("ISON :bob");
    CHECK_SENT("WHOIS :bob");
    LONGS_EQUAL(2, arraylist_size (sent_messages));

    /* MONITOR. */
    arraylist_clear (sent_messages);
    server->monitor = 100;
    notify2 = irc_notify_new (server, "carol", 0);
    irc_notify_check_now (notify2);
    CHECK_SENT("MONITOR + carol");
    LONGS_EQUAL(1, arraylist_size (sent_messages));

    arraylist_clear (sent_messages);
    irc_notify_send_monitor (server);
    CHECK_SENT("MONITOR + bob,carol");

    /* Many nicks: message is split. */
    for (i = 0; i < 60; i++)
    {
        snprintf (nick, sizeof (nick), "nick_with_a_long_name_%02d", i);
        irc_notify_new (server, nick, 0);
    }
    arraylist_clear (sent_messages);
    irc_notify_send_monitor (server);
    CHECK(arraylist_size (sent_messages) >= 3);

    /* Remove monitored nicks. */
    arraylist_clear (sent_messages);
    irc_notify_free (server, notify2, 1);
    CHECK_SENT("MONITOR - carol");
    arraylist_clear (sent_messages);
    irc_notify_free (server, notify1, 0);
    CHECK_NO_MSG_SENT;
    irc_notify_free_all (server);
    CHECK_SENT("MONITOR C");

    /* Notify list from option: MONITOR sent. */
    arraylist_clear (sent_messages);
    config_file_option_set (server->options[IRC_SERVER_OPTION_NOTIFY],
                            "dave,eve", 1);
    CHECK_SENT("MONITOR + dave,eve");
    server->monitor = 0;
}

/*
 * Test functions:
 *   irc_notify_display
 *   irc_notify_display_list
 */

TEST(IrcNotifyWithServer, DisplayList)
{
    struct t_irc_notify *notify1, *notify2, *notify3;

    connect ();

    /* Empty list. */
    record_start ();
    irc_notify_display_list (server);
    RECORD_CHECK_MSG(SRV_BUFFER, "", "Notify list is empty on this server",
                     NULL);
    record_start ();
    irc_notify_display_list (NULL);
    RECORD_CHECK_MSG("core.weechat", "",
                     "Notify list is empty on all servers", NULL);

    notify1 = irc_notify_new (server, "bob", 0);
    notify2 = irc_notify_new (server, "carol", 1);
    notify3 = irc_notify_new (server, "dave", 1);
    notify2->is_on_server = 0;
    notify3->is_on_server = 1;
    notify3->away_message = strdup ("gone");

    record_start ();
    irc_notify_display_list (server);
    RECORD_CHECK_MSG(SRV_BUFFER, "", "Notify list for " IRC_FAKE_SERVER ":",
                     NULL);
    RECORD_CHECK_MSG(SRV_BUFFER, "",
                     "  bob @ " IRC_FAKE_SERVER ": unknown", NULL);
    RECORD_CHECK_MSG(SRV_BUFFER, "",
                     "  carol @ " IRC_FAKE_SERVER ": offline", NULL);
    RECORD_CHECK_MSG(SRV_BUFFER, "",
                     "  dave @ " IRC_FAKE_SERVER ": online (away: \"gone\")",
                     NULL);

    notify3->is_on_server = 0;
    record_start ();
    irc_notify_display_list (NULL);
    RECORD_CHECK_MSG("core.weechat", "", "Notify list for all servers:",
                     NULL);
    RECORD_CHECK_MSG("core.weechat", "",
                     "  dave @ " IRC_FAKE_SERVER ": online (away: \"gone\")",
                     NULL);

    free (notify3->away_message);
    notify3->away_message = NULL;
    notify3->is_on_server = 1;
    record_start ();
    irc_notify_display (server, NULL, notify3);
    RECORD_CHECK_MSG("core.weechat", "",
                     "  dave @ " IRC_FAKE_SERVER ": online", NULL);
    (void) notify1;
}

/*
 * Test functions:
 *   irc_notify_get_tags
 *   irc_notify_send_signal
 */

TEST(IrcNotifyWithServer, TagsSignal)
{
    struct t_irc_notify *notify;

    STRCMP_EQUAL("irc_notify,irc_notify_join,nick_bob,notify_message,log3",
                 irc_notify_get_tags (irc_config_look_notify_tags_ison,
                                      "join", "bob"));
    config_file_option_set (irc_config_look_notify_tags_ison, "", 1);
    STRCMP_EQUAL("irc_notify,irc_notify_join,nick_bob,log3",
                 irc_notify_get_tags (irc_config_look_notify_tags_ison,
                                      "join", "bob"));
    config_file_option_reset (irc_config_look_notify_tags_ison, 1);

    notify = irc_notify_new (server, "bob", 0);
    irc_notify_send_signal (notify, "join", NULL);
    STRCMP_EQUAL("irc_notify_join:" IRC_FAKE_SERVER ",bob", signal_received);
    irc_notify_send_signal (notify, "away", "");
    STRCMP_EQUAL("irc_notify_away:" IRC_FAKE_SERVER ",bob", signal_received);
    irc_notify_send_signal (notify, "away", "gone");
    STRCMP_EQUAL("irc_notify_away:" IRC_FAKE_SERVER ",bob,gone",
                 signal_received);
}

/*
 * Test functions:
 *   irc_notify_display_is_on
 *   irc_notify_set_is_on_server
 *   irc_notify_set_away_message
 */

TEST(IrcNotifyWithServer, IsOnAway)
{
    struct t_irc_notify *notify;

    connect ();

    /* Without notify. */
    record_start ();
    irc_notify_display_is_on (server, "bob", "user@host", NULL, 1);
    RECORD_CHECK_MSG(SRV_BUFFER, "--",
                     "notify: bob (user@host) is connected",
                     "irc_notify,irc_notify_join,nick_bob,notify_message,"
                     "log3");
    irc_notify_display_is_on (server, "bob", NULL, NULL, 0);
    RECORD_CHECK_MSG(SRV_BUFFER, "--", "notify: bob is offline", NULL);

    notify = irc_notify_new (server, "bob", 1);
    irc_notify_set_is_on_server (NULL, NULL, 1);

    /* Unknown -> online: "is connected". */
    record_start ();
    irc_notify_set_is_on_server (notify, "user@host", 1);
    RECORD_CHECK_MSG(SRV_BUFFER, "--", "notify: bob (user@host) is connected",
                     NULL);
    STRCMP_EQUAL("irc_notify_join:" IRC_FAKE_SERVER ",bob", signal_received);
    LONGS_EQUAL(1, notify->is_on_server);

    /* Same status: nothing displayed. */
    record_start ();
    irc_notify_set_is_on_server (notify, NULL, 1);
    LONGS_EQUAL(0, record_count_messages ());

    /* Online -> offline: "has quit". */
    irc_notify_set_is_on_server (notify, NULL, 0);
    RECORD_CHECK_MSG(SRV_BUFFER, "--", "notify: bob has quit", NULL);
    STRCMP_EQUAL("irc_notify_quit:" IRC_FAKE_SERVER ",bob", signal_received);

    /* Offline -> online: "has connected". */
    irc_notify_set_is_on_server (notify, NULL, 1);
    RECORD_CHECK_MSG(SRV_BUFFER, "--", "notify: bob has connected", NULL);

    /* Away messages. */
    irc_notify_set_away_message (NULL, "gone");
    record_start ();
    irc_notify_set_away_message (notify, NULL);
    LONGS_EQUAL(0, record_count_messages ());
    irc_notify_set_away_message (notify, "gone");
    RECORD_CHECK_MSG(SRV_BUFFER, "--", "notify: bob is now away: \"gone\"",
                     NULL);
    STRCMP_EQUAL("irc_notify_away:" IRC_FAKE_SERVER ",bob,gone",
                 signal_received);
    STRCMP_EQUAL("gone", notify->away_message);
    record_start ();
    irc_notify_set_away_message (notify, "gone");
    LONGS_EQUAL(0, record_count_messages ());
    irc_notify_set_away_message (notify, "still gone");
    RECORD_CHECK_MSG(SRV_BUFFER, "--",
                     "notify: bob is still away: \"still gone\"", NULL);
    STRCMP_EQUAL("irc_notify_still_away:" IRC_FAKE_SERVER ",bob,still gone",
                 signal_received);
    irc_notify_set_away_message (notify, NULL);
    RECORD_CHECK_MSG(SRV_BUFFER, "--", "notify: bob is back", NULL);
    STRCMP_EQUAL("irc_notify_back:" IRC_FAKE_SERVER ",bob", signal_received);
    POINTERS_EQUAL(NULL, notify->away_message);
}

/*
 * Test functions:
 *   irc_notify_timer_ison_cb
 *   irc_notify_hsignal_cb (ISON)
 */

TEST(IrcNotifyWithServer, Ison)
{
    struct t_irc_notify *notify1, *notify2, *notify3;

    notify1 = irc_notify_new (server, "bob", 0);
    notify2 = irc_notify_new (server, "Carol", 0);

    /* Not connected: nothing sent. */
    irc_notify_timer_ison_cb (NULL, NULL, 0);
    CHECK_NO_MSG_SENT;

    connect ();

    irc_notify_timer_ison_cb (NULL, NULL, 0);
    CHECK_SENT("ISON :bob Carol");
    LONGS_EQUAL(1, arraylist_size (sent_messages));

    /* Reply: bob is online, carol is offline. */
    RECV(":server 303 alice :BOB");
    LONGS_EQUAL(1, notify1->is_on_server);
    LONGS_EQUAL(0, notify2->is_on_server);

    /* Nick added after the ISON was sent: status unchanged. */
    irc_notify_timer_ison_cb (NULL, NULL, 0);
    notify3 = irc_notify_new (server, "dave", 0);
    record_start ();
    RECV(":server 303 alice :carol");
    LONGS_EQUAL(0, notify1->is_on_server);
    LONGS_EQUAL(1, notify2->is_on_server);
    LONGS_EQUAL(-1, notify3->is_on_server);

    /* Empty reply: all nicks sent are offline. */
    irc_notify_timer_ison_cb (NULL, NULL, 0);
    CHECK_SENT("ISON :bob Carol dave");
    RECV(":server 303 alice :");
    LONGS_EQUAL(0, notify2->is_on_server);
    LONGS_EQUAL(0, notify3->is_on_server);

    /* With MONITOR: ISON is not sent. */
    server->monitor = 100;
    arraylist_clear (sent_messages);
    irc_notify_timer_ison_cb (NULL, NULL, 0);
    CHECK_NO_MSG_SENT;
    server->monitor = 0;
}

/*
 * Test functions:
 *   irc_notify_timer_whois_cb
 *   irc_notify_hsignal_cb (WHOIS)
 */

TEST(IrcNotifyWithServer, Whois)
{
    struct t_irc_notify *notify;

    irc_notify_new (server, "carol", 0);
    notify = irc_notify_new (server, "bob", 1);

    /* Not connected: nothing sent. */
    irc_notify_timer_whois_cb (NULL, NULL, 0);
    CHECK_NO_MSG_SENT;

    connect ();

    /* Only nicks with away check. */
    irc_notify_timer_whois_cb (NULL, NULL, 0);
    CHECK_SENT("WHOIS :bob");
    LONGS_EQUAL(1, arraylist_size (sent_messages));

    /* Nick is away. */
    RECV(":server 311 alice bob user host * :Bob");
    RECV(":server 301 alice bob :gone");
    RECV(":server 318 alice bob :End of /WHOIS list.");
    STRCMP_EQUAL("gone", notify->away_message);

    /* Nick is back. */
    irc_notify_timer_whois_cb (NULL, NULL, 0);
    RECV(":server 311 alice bob user host * :Bob");
    RECV(":server 318 alice bob :End of /WHOIS list.");
    POINTERS_EQUAL(NULL, notify->away_message);

    /* No such nick: away message unchanged. */
    notify->away_message = strdup ("gone");
    irc_notify_timer_whois_cb (NULL, NULL, 0);
    RECV(":server 401 alice bob :No such nick/channel");
    RECV(":server 318 alice bob :End of /WHOIS list.");
    STRCMP_EQUAL("gone", notify->away_message);
}

/*
 * Test functions:
 *   irc_notify_hsignal_cb (errors)
 */

TEST(IrcNotifyWithServer, HsignalErrors)
{
    struct t_hashtable *hashtable;
    struct t_irc_notify *notify;

    notify = irc_notify_new (server, "bob", 0);

    hashtable = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING, NULL, NULL);

    /* Error in redirection. */
    hashtable_set (hashtable, "error", "timeout");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_notify_hsignal_cb (NULL, NULL, "test", hashtable));
    hashtable_remove (hashtable, "error");

    /* Missing keys. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_notify_hsignal_cb (NULL, NULL, "test", hashtable));

    /* Unknown server. */
    hashtable_set (hashtable, "server", "xxx");
    hashtable_set (hashtable, "pattern", "ison");
    hashtable_set (hashtable, "command", "ISON :bob");
    hashtable_set (hashtable, "output", ":server 303 alice :bob");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_notify_hsignal_cb (NULL, NULL, "test", hashtable));

    /* Command without arguments. */
    hashtable_set (hashtable, "server", IRC_FAKE_SERVER);
    hashtable_set (hashtable, "command", "ISON");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_notify_hsignal_cb (NULL, NULL, "test", hashtable));
    hashtable_set (hashtable, "command", "ISON :");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_notify_hsignal_cb (NULL, NULL, "test", hashtable));
    LONGS_EQUAL(-1, notify->is_on_server);

    /* OK. */
    hashtable_set (hashtable, "command", "ISON :bob");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_notify_hsignal_cb (NULL, NULL, "test", hashtable));
    LONGS_EQUAL(1, notify->is_on_server);

    /* WHOIS of an unknown notify: ignored. */
    hashtable_set (hashtable, "pattern", "whois");
    hashtable_set (hashtable, "command", "WHOIS :xxx");
    hashtable_set (hashtable, "output", ":server 301 alice xxx :gone");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_notify_hsignal_cb (NULL, NULL, "test", hashtable));

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   irc_notify_hdata_notify_cb
 *   irc_notify_add_to_infolist
 *   irc_notify_print_log
 *   irc_notify_hook_timer_ison
 *   irc_notify_hook_timer_whois
 */

TEST(IrcNotifyWithServer, HdataInfolistLog)
{
    struct t_irc_notify *notify;
    struct t_hdata *hdata;
    struct t_infolist *infolist;

    notify = irc_notify_new (server, "bob", 1);
    notify->away_message = strdup ("gone");

    hdata = hook_hdata_get (NULL, "irc_notify");
    CHECK(hdata);
    STRCMP_EQUAL("bob", hdata_string (hdata, notify, "nick"));
    LONGS_EQUAL(1, hdata_integer (hdata, notify, "check_away"));
    STRCMP_EQUAL("gone", hdata_string (hdata, notify, "away_message"));
    POINTERS_EQUAL(server, hdata_pointer (hdata, notify, "server"));

    LONGS_EQUAL(0, irc_notify_add_to_infolist (NULL, notify));
    infolist = infolist_new (NULL);
    LONGS_EQUAL(0, irc_notify_add_to_infolist (infolist, NULL));
    LONGS_EQUAL(1, irc_notify_add_to_infolist (infolist, notify));
    CHECK(infolist_next (infolist));
    STRCMP_EQUAL(IRC_FAKE_SERVER, infolist_string (infolist, "server_name"));
    STRCMP_EQUAL("bob", infolist_string (infolist, "nick"));
    LONGS_EQUAL(1, infolist_integer (infolist, "check_away"));
    LONGS_EQUAL(-1, infolist_integer (infolist, "is_on_server"));
    STRCMP_EQUAL("gone", infolist_string (infolist, "away_message"));
    infolist_free (infolist);

    irc_notify_print_log (server);

    irc_notify_hook_timer_ison ();
    irc_notify_hook_timer_whois ();
}
