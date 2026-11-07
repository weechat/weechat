/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay IRC protocol */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-line.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-server.h"
#include "src/plugins/relay/irc/relay-irc.h"

extern int relay_irc_command_relayed (const char *irc_command);
extern int relay_irc_command_ignored (const char *irc_command);
extern int relay_irc_search_server_capability (const char *capability);
extern struct t_hashtable *relay_irc_message_parse (const char *message);
extern void relay_irc_sendf (struct t_relay_client *client,
                             const char *format, ...);
extern void relay_irc_parse_cap_message (struct t_relay_client *client,
                                         struct t_hashtable *parsed_msg);
extern void relay_irc_parse_ctcp (const char *message,
                                  char **ctcp_type, char **ctcp_params);
extern int relay_irc_tag_relay_client_id (const char *tags);
extern int relay_irc_tag_relayed (struct t_relay_client *client,
                                  const char *tag);
extern void relay_irc_get_line_info (struct t_relay_client *client,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_line_data *line_data,
                                     int *irc_command, int *irc_action,
                                     time_t *date, const char **nick,
                                     const char **nick1, const char **nick2,
                                     const char **host, char **tags,
                                     char **message);
extern char *relay_irc_remove_tags_not_negotiated (
    struct t_relay_client *client,
    const char *message);
extern void relay_irc_input_send (struct t_relay_client *client,
                                  const char *irc_channel,
                                  const char *options,
                                  const char *format, ...);
extern int relay_irc_cap_enabled (struct t_relay_client *client,
                                  const char *capability);
extern int relay_irc_get_supported_caps (struct t_relay_client *client);
extern struct t_arraylist *relay_irc_get_list_caps ();
extern int relay_irc_signal_irc_in2_cb (const void *pointer, void *data,
                                        const char *signal,
                                        const char *type_data,
                                        void *signal_data);
extern int relay_irc_signal_irc_outtags_cb (const void *pointer, void *data,
                                            const char *signal,
                                            const char *type_data,
                                            void *signal_data);
extern int relay_irc_signal_irc_disc_cb (const void *pointer, void *data,
                                         const char *signal,
                                         const char *type_data,
                                         void *signal_data);
extern int relay_irc_hsignal_irc_redir_cb (const void *pointer, void *data,
                                           const char *signal,
                                           struct t_hashtable *hashtable);
extern void relay_irc_send_channel_backlog (struct t_relay_client *client,
                                            const char *channel,
                                            struct t_gui_buffer *buffer);
extern void relay_irc_send_join (struct t_relay_client *client,
                                 const char *channel);
extern void relay_irc_send_join_channels (struct t_relay_client *client);
extern void relay_irc_hook_signals (struct t_relay_client *client);
extern int relay_irc_capability_compare_cb (void *data,
                                            struct t_arraylist *arraylist,
                                            void *pointer1, void *pointer2);
extern void relay_irc_capability_free_db (void *data,
                                          struct t_arraylist *arraylist,
                                          void *pointer);
}

#define CLIENT_RECV(__irc_msg)                                          \
    test_client_recv (__irc_msg);

#define CLIENT_SEND(__irc_msg)                                          \
    test_client_send (__irc_msg);

#define CHECK_SENT_CLIENT(__message)                                    \
    if ((__message != NULL)                                             \
        && !arraylist_search (sent_messages_client, (void *)__message,  \
                              NULL, NULL))                              \
    {                                                                   \
        char **msg = test_build_error (                                 \
            "Message not sent to the relay client",                     \
            NULL,                                                       \
            __message,                                                  \
            NULL,                                                       \
            "All messages sent");                                       \
        sent_msg_dump (sent_messages_client, msg);                      \
        FAIL(string_dyn_free (msg, 0));                                 \
    }                                                                   \
    else if ((__message == NULL)                                        \
             && (arraylist_size (sent_messages_client) > 0))            \
    {                                                                   \
        char **msg = test_build_error (                                 \
            "Unexpected message(s) sent to the relay client",           \
            NULL,                                                       \
            NULL,                                                       \
            NULL,                                                       \
            NULL);                                                      \
        sent_msg_dump (sent_messages_client, msg);                      \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_SENT_IRC(__message)                                       \
    if ((__message != NULL)                                             \
        && !arraylist_search (sent_messages_irc, (void *)__message,     \
                              NULL, NULL))                              \
    {                                                                   \
        char **msg = test_build_error (                                 \
            "Message not sent to the IRC server",                       \
            NULL,                                                       \
            __message,                                                  \
            NULL,                                                       \
            "All messages sent");                                       \
        sent_msg_dump (sent_messages_irc, msg);                         \
        FAIL(string_dyn_free (msg, 0));                                 \
    }                                                                   \
    else if ((__message == NULL)                                        \
             && (arraylist_size (sent_messages_irc) > 0))               \
    {                                                                   \
        char **msg = test_build_error (                                 \
            "Unexpected message(s) sent to the IRC server",             \
            NULL,                                                       \
            NULL,                                                       \
            NULL,                                                       \
            NULL);                                                      \
        sent_msg_dump (sent_messages_irc, msg);                         \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define WEE_CHECK_REMOVE_TAGS(__result, __message)                      \
    message = relay_irc_remove_tags_not_negotiated (ptr_relay_client,   \
                                                    __message);         \
    STRCMP_EQUAL(__result, message);                                    \
    free (message);

TEST_GROUP(RelayIrc)
{
};

TEST_GROUP(RelayIrcWithClient)
{
    struct t_irc_server *ptr_server = NULL;
    struct t_relay_server *ptr_relay_server = NULL;
    struct t_relay_client *ptr_relay_client = NULL;
    struct t_arraylist *sent_messages_client = NULL;
    struct t_arraylist *sent_messages_irc = NULL;
    struct t_hook *hook_modifier_relay_irc_out = NULL;
    struct t_hook *hook_signal_irc_input_send = NULL;

    void test_client_recv (const char *data)
    {
        arraylist_clear (sent_messages_client);
        arraylist_clear (sent_messages_irc);

        relay_irc_recv (ptr_relay_client, data);
    }

    void test_client_send (const char *data)
    {
        arraylist_clear (sent_messages_client);
        arraylist_clear (sent_messages_irc);

        relay_irc_sendf (ptr_relay_client, "%s", data);
    }

    char **test_build_error (const char *msg1,
                             const char *prefix,
                             const char *message,
                             const char *tags,
                             const char *msg2)
    {
        char **msg;

        msg = string_dyn_alloc (1024);
        string_dyn_concat (msg, msg1, -1);
        if (message)
        {
            string_dyn_concat (msg, ": prefix=\"", -1);
            string_dyn_concat (msg, prefix, -1);
            string_dyn_concat (msg, "\", message=\"", -1);
            string_dyn_concat (msg, message, -1);
            string_dyn_concat (msg, "\", tags=\"", -1);
            string_dyn_concat (msg, tags, -1);
            string_dyn_concat (msg, "\"\n", -1);
        }
        else
        {
            string_dyn_concat (msg, ":\n", -1);
        }
        if (msg2)
        {
            string_dyn_concat (msg, msg2, -1);
            string_dyn_concat (msg, ":\n", -1);
        }
        return msg;
    }

    static char *modifier_relay_irc_out_cb (const void *pointer,
                                            void *data,
                                            const char *modifier,
                                            const char *modifier_data,
                                            const char *string)
    {
        /* Make C++ compiler happy. */
        (void) data;
        (void) modifier;
        (void) modifier_data;

        if (string)
            arraylist_add ((struct t_arraylist *)pointer, strdup (string));

        return NULL;
    }

    static int signal_irc_input_send_cb (const void *pointer, void *data,
                                         const char *signal,
                                         const char *type_data,
                                         void *signal_data)
    {
        /* Make C++ compiler happy. */
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

    static int sent_msg_cmp_cb (void *data, struct t_arraylist *arraylist,
                                void *pointer1, void *pointer2)
    {
        /* Make C++ compiler happy. */
        (void) data;
        (void) arraylist;

        return strcmp ((char *)pointer1, (char *)pointer2);
    }

    static void sent_msg_free_cb (void *data, struct t_arraylist *arraylist,
                                  void *pointer)
    {
        /* Make C++ compiler happy. */
        (void) data;
        (void) arraylist;

        free (pointer);
    }

    void sent_msg_dump (struct t_arraylist *sent_messages, char **msg)
    {
        int i;

        for (i = 0; i < arraylist_size (sent_messages); i++)
        {
            string_dyn_concat (msg, "  \"", -1);
            string_dyn_concat (msg,
                               (const char *)arraylist_get (sent_messages, i),
                               -1);
            string_dyn_concat (msg, "\"\n", -1);
        }
    }

    void setup ()
    {
        /* Initialize list of messages sent to the relay client. */
        if (sent_messages_client)
        {
            arraylist_clear (sent_messages_client);
        }
        else
        {
            sent_messages_client = arraylist_new (16, 0, 1,
                                                  &sent_msg_cmp_cb, NULL,
                                                  &sent_msg_free_cb, NULL);
        }

        /* Initialize list of messages sent to the IRC server. */
        if (sent_messages_irc)
        {
            arraylist_clear (sent_messages_irc);
        }
        else
        {
            sent_messages_irc = arraylist_new (16, 0, 1,
                                               &sent_msg_cmp_cb, NULL,
                                               &sent_msg_free_cb, NULL);
        }

        /* Disable auto-open of relay buffer. */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);

        /* Set relay password. */
        config_file_option_set (relay_config_network_password, "secret", 1);

        if (!hook_modifier_relay_irc_out)
        {
            hook_modifier_relay_irc_out = hook_modifier (
                NULL,
                "relay_client_irc_out1",
                &modifier_relay_irc_out_cb, sent_messages_client, NULL);
        }

        if (!hook_signal_irc_input_send)
        {
            hook_signal_irc_input_send = hook_signal (
                NULL,
                "irc_input_send",
                &signal_irc_input_send_cb, sent_messages_irc, NULL);
        }

        /* Create a fake server (no I/O). */
        run_cmd_quiet ("/mute /server add test fake:127.0.0.1 "
                       "-nicks=nick1,nick2,nick3");

        /* Get the server pointer. */
        ptr_server = irc_server_search ("test");

        /* Connect to the fake server. */
        run_cmd_quiet ("/connect test");

        /* Simulate connection OK to server. */
        run_cmd_quiet ("/command -buffer irc.server.test irc "
                       "/server fakerecv "
                       "\":server 001 alice :Welcome on this server, nick1!\"");

        /* Create a relay server. */
        ptr_relay_server = relay_server_new (
            "irc.test",
            RELAY_PROTOCOL_IRC,
            "test",
            9001,
            NULL,  /* path */
            1,  /* ipv4 */
            0,  /* ipv6 */
            0,  /* tls */
            0);  /* unix_socket */

        /* Create a fake relay client (no I/O). */
        ptr_relay_client = relay_client_new (-1, "test", ptr_relay_server);
    }

    void teardown ()
    {
        relay_client_free (ptr_relay_client);
        ptr_relay_client = NULL;

        relay_server_free (ptr_relay_server);
        ptr_relay_server = NULL;

        /* Disconnect and delete the fake server. */
        run_cmd_quiet ("/mute /disconnect test");
        run_cmd_quiet ("/mute /server del test");
        ptr_server = NULL;

        /* Restore auto-open of relay buffer. */
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);

        /* Restore relay password. */
        config_file_option_reset (relay_config_network_password, 1);

        /* Restore backlog options. */
        config_file_option_reset (relay_config_irc_backlog_max_number, 1);
        config_file_option_reset (relay_config_irc_backlog_tags, 1);
        config_file_option_reset (relay_config_irc_backlog_time_format, 1);
    }
};

/*
 * Test functions:
 *   relay_irc_command_relayed
 */

TEST(RelayIrc, CommandRelayed)
{
    LONGS_EQUAL(0, relay_irc_command_relayed (NULL));
    LONGS_EQUAL(0, relay_irc_command_relayed (""));
    LONGS_EQUAL(0, relay_irc_command_relayed ("unknown"));

    LONGS_EQUAL(1, relay_irc_command_relayed ("privmsg"));
    LONGS_EQUAL(1, relay_irc_command_relayed ("PRIVMSG"));
    LONGS_EQUAL(1, relay_irc_command_relayed ("notice"));
    LONGS_EQUAL(1, relay_irc_command_relayed ("Notice"));
}

/*
 * Test functions:
 *   relay_irc_command_ignored
 */

TEST(RelayIrc, CommandIgnored)
{
    LONGS_EQUAL(0, relay_irc_command_ignored (NULL));
    LONGS_EQUAL(0, relay_irc_command_ignored (""));
    LONGS_EQUAL(0, relay_irc_command_ignored ("unknown"));

    LONGS_EQUAL(1, relay_irc_command_ignored ("cap"));
    LONGS_EQUAL(1, relay_irc_command_ignored ("CAP"));
    LONGS_EQUAL(1, relay_irc_command_ignored ("pong"));
    LONGS_EQUAL(1, relay_irc_command_ignored ("Pong"));
    LONGS_EQUAL(1, relay_irc_command_ignored ("quit"));
}

/*
 * Test functions:
 *   relay_irc_search_backlog_commands_tags
 */

TEST(RelayIrc, SearchBacklogCommandsTags)
{
    LONGS_EQUAL(-1, relay_irc_search_backlog_commands_tags (NULL));
    LONGS_EQUAL(-1, relay_irc_search_backlog_commands_tags (""));
    LONGS_EQUAL(-1, relay_irc_search_backlog_commands_tags ("unknown"));
    LONGS_EQUAL(-1, relay_irc_search_backlog_commands_tags ("IRC_JOIN"));

    CHECK(relay_irc_search_backlog_commands_tags ("irc_join") >= 0);
    CHECK(relay_irc_search_backlog_commands_tags ("irc_privmsg") >= 0);
}

/*
 * Test functions:
 *   relay_irc_search_server_capability
 */

TEST(RelayIrc, SearchServerCapability)
{
    LONGS_EQUAL(-1, relay_irc_search_server_capability (NULL));
    LONGS_EQUAL(-1, relay_irc_search_server_capability (""));
    LONGS_EQUAL(-1, relay_irc_search_server_capability ("unknown"));

    CHECK(relay_irc_search_server_capability ("server-time") >= 0);
    CHECK(relay_irc_search_server_capability ("echo-message") >= 0);
}

/*
 * Test functions:
 *   relay_irc_message_parse
 */

TEST(RelayIrc, MessageParse)
{
    struct t_hashtable *hashtable;

    POINTERS_EQUAL(NULL, relay_irc_message_parse (NULL));

    hashtable = relay_irc_message_parse ("");
    CHECK(hashtable);
    LONGS_EQUAL(14, hashtable->items_count);
    STRCMP_EQUAL("", (const char *)hashtable_get (hashtable, "tags"));
    STRCMP_EQUAL("", (const char *)hashtable_get (hashtable, "message_without_tags"));
    STRCMP_EQUAL("", (const char *)hashtable_get (hashtable, "nick"));
    STRCMP_EQUAL("", (const char *)hashtable_get (hashtable, "user"));
    STRCMP_EQUAL("", (const char *)hashtable_get (hashtable, "host"));
    STRCMP_EQUAL("", (const char *)hashtable_get (hashtable, "command"));
    STRCMP_EQUAL("", (const char *)hashtable_get (hashtable, "channel"));
    STRCMP_EQUAL("", (const char *)hashtable_get (hashtable, "arguments"));
    STRCMP_EQUAL("", (const char *)hashtable_get (hashtable, "text"));
    STRCMP_EQUAL("0", (const char *)hashtable_get (hashtable, "num_params"));
    STRCMP_EQUAL("-1", (const char *)hashtable_get (hashtable, "pos_command"));
    STRCMP_EQUAL("-1", (const char *)hashtable_get (hashtable, "pos_arguments"));
    STRCMP_EQUAL("-1", (const char *)hashtable_get (hashtable, "pos_channel"));
    STRCMP_EQUAL("-1", (const char *)hashtable_get (hashtable, "pos_text"));
    hashtable_free (hashtable);

    hashtable = relay_irc_message_parse (
        "@time=2015-06-27T16:40:35.000Z :nick!user@host PRIVMSG #weechat :Hello world!");
    CHECK(hashtable);
    LONGS_EQUAL(17, hashtable->items_count);
    STRCMP_EQUAL("time=2015-06-27T16:40:35.000Z", (const char *)hashtable_get (hashtable, "tags"));
    STRCMP_EQUAL("2015-06-27T16:40:35.000Z", (const char *)hashtable_get (hashtable, "tag_time"));
    STRCMP_EQUAL(":nick!user@host PRIVMSG #weechat :Hello world!", (const char *)hashtable_get (hashtable, "message_without_tags"));
    STRCMP_EQUAL("nick", (const char *)hashtable_get (hashtable, "nick"));
    STRCMP_EQUAL("user", (const char *)hashtable_get (hashtable, "user"));
    STRCMP_EQUAL("nick!user@host", (const char *)hashtable_get (hashtable, "host"));
    STRCMP_EQUAL("PRIVMSG", (const char *)hashtable_get (hashtable, "command"));
    STRCMP_EQUAL("#weechat", (const char *)hashtable_get (hashtable, "channel"));
    STRCMP_EQUAL("#weechat :Hello world!", (const char *)hashtable_get (hashtable, "arguments"));
    STRCMP_EQUAL("Hello world!", (const char *)hashtable_get (hashtable, "text"));
    STRCMP_EQUAL("2", (const char *)hashtable_get (hashtable, "num_params"));
    STRCMP_EQUAL("#weechat", (const char *)hashtable_get (hashtable, "param1"));
    STRCMP_EQUAL("Hello world!", (const char *)hashtable_get (hashtable, "param2"));
    STRCMP_EQUAL("47", (const char *)hashtable_get (hashtable, "pos_command"));
    STRCMP_EQUAL("55", (const char *)hashtable_get (hashtable, "pos_arguments"));
    STRCMP_EQUAL("55", (const char *)hashtable_get (hashtable, "pos_channel"));
    STRCMP_EQUAL("65", (const char *)hashtable_get (hashtable, "pos_text"));
    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   relay_irc_sendf
 */

TEST(RelayIrcWithClient, Sendf)
{
    relay_irc_sendf (NULL, NULL);
    relay_irc_sendf (NULL, "test");
    relay_irc_sendf (ptr_relay_client, NULL);

    CLIENT_SEND("PING");
    CHECK_SENT_CLIENT("PING");

    CLIENT_SEND("PRIVMSG #test :test message");
    CHECK_SENT_CLIENT("PRIVMSG #test :test message");
}

/*
 * Test functions:
 *   relay_irc_parse_cap_message
 */

TEST(RelayIrcWithClient, ParseCapMessage)
{
    struct t_hashtable *hashtable;

    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, irc_cap_echo_message));

    /* CAP NAK: ignored */
    hashtable = relay_irc_message_parse (":server CAP * NAK echo-message");
    relay_irc_parse_cap_message (ptr_relay_client, hashtable);
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, irc_cap_echo_message));
    hashtable_free (hashtable);

    /* CAP ACK with unknown capability */
    hashtable = relay_irc_message_parse (":server CAP * ACK unknown");
    relay_irc_parse_cap_message (ptr_relay_client, hashtable);
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, irc_cap_echo_message));
    hashtable_free (hashtable);

    /* CAP ACK with extended-join and echo-message */
    hashtable = relay_irc_message_parse (":server CAP * ACK extended-join echo-message");
    relay_irc_parse_cap_message (ptr_relay_client, hashtable);
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, irc_cap_echo_message));
    hashtable_free (hashtable);

    /* CAP ACK with -extended-join and -echo-message */
    hashtable = relay_irc_message_parse (":server CAP * ACK -extended-join -echo-message");
    relay_irc_parse_cap_message (ptr_relay_client, hashtable);
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, irc_cap_echo_message));
    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   relay_irc_signal_irc_in2_cb
 */

TEST(RelayIrcWithClient, SignalIrcIn2Cb)
{
    RELAY_IRC_DATA(ptr_relay_client, nick) = strdup ("alice");

    /* Message relayed to client. */
    arraylist_clear (sent_messages_client);
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_irc_signal_irc_in2_cb (
                    ptr_relay_client, NULL, "test,irc_in2_privmsg",
                    WEECHAT_HOOK_SIGNAL_STRING,
                    (void *)":bob!user@host PRIVMSG #test :hello"));
    CHECK_SENT_CLIENT(":bob!user@host PRIVMSG #test :hello");

    /* Message without host: address of relay is used. */
    arraylist_clear (sent_messages_client);
    relay_irc_signal_irc_in2_cb (ptr_relay_client, NULL, "test,irc_in2_join",
                                 WEECHAT_HOOK_SIGNAL_STRING,
                                 (void *)"JOIN #test");
    CHECK_SENT_CLIENT(":weechat.relay.irc JOIN #test");

    /* PING and PONG are not relayed. */
    arraylist_clear (sent_messages_client);
    relay_irc_signal_irc_in2_cb (ptr_relay_client, NULL, "test,irc_in2_ping",
                                 WEECHAT_HOOK_SIGNAL_STRING,
                                 (void *)"PING :server");
    relay_irc_signal_irc_in2_cb (ptr_relay_client, NULL, "test,irc_in2_pong",
                                 WEECHAT_HOOK_SIGNAL_STRING,
                                 (void *)":server PONG server :123");
    CHECK_SENT_CLIENT(NULL);

    /* Self nick changed: nick updated in client. */
    arraylist_clear (sent_messages_client);
    relay_irc_signal_irc_in2_cb (ptr_relay_client, NULL, "test,irc_in2_nick",
                                 WEECHAT_HOOK_SIGNAL_STRING,
                                 (void *)":ALICE!user@host NICK :alice2");
    STRCMP_EQUAL("alice2", RELAY_IRC_DATA(ptr_relay_client, nick));
    CHECK_SENT_CLIENT(":ALICE!user@host NICK :alice2");

    /* Nick changed for another nick: client nick unchanged. */
    relay_irc_signal_irc_in2_cb (ptr_relay_client, NULL, "test,irc_in2_nick",
                                 WEECHAT_HOOK_SIGNAL_STRING,
                                 (void *)":bob!user@host NICK :bob2");
    STRCMP_EQUAL("alice2", RELAY_IRC_DATA(ptr_relay_client, nick));

    /* Capability "echo-message" enabled on server. */
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, irc_cap_echo_message));
    relay_irc_signal_irc_in2_cb (ptr_relay_client, NULL, "test,irc_in2_cap",
                                 WEECHAT_HOOK_SIGNAL_STRING,
                                 (void *)":server CAP alice2 ACK :echo-message");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, irc_cap_echo_message));
}

/*
 * Test functions:
 *   relay_irc_tag_relay_client_id
 */

TEST(RelayIrc, TagRelayClientId)
{
    LONGS_EQUAL(-1, relay_irc_tag_relay_client_id (NULL));
    LONGS_EQUAL(-1, relay_irc_tag_relay_client_id (""));
    LONGS_EQUAL(-1, relay_irc_tag_relay_client_id ("zzz"));
    LONGS_EQUAL(-1, relay_irc_tag_relay_client_id ("relay_client_abc"));

    LONGS_EQUAL(0, relay_irc_tag_relay_client_id ("relay_client_0"));
    LONGS_EQUAL(123, relay_irc_tag_relay_client_id ("relay_client_123"));
}

/*
 * Test functions:
 *   relay_irc_signal_irc_outtags_cb
 */

TEST(RelayIrcWithClient, SignalIrcOuttagsCb)
{
    char str_tags[128];

    RELAY_IRC_DATA(ptr_relay_client, nick) = strdup ("alice");

    /* Message sent by another client (or WeeChat): relayed. */
    arraylist_clear (sent_messages_client);
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_irc_signal_irc_outtags_cb (
                    ptr_relay_client, NULL, "test,irc_outtags_privmsg",
                    WEECHAT_HOOK_SIGNAL_STRING,
                    (void *)"relay_client_9999;PRIVMSG #test :hello\r\n"));
    CHECK_SENT_CLIENT(":alice PRIVMSG #test :hello");

    /* Message without tags. */
    arraylist_clear (sent_messages_client);
    relay_irc_signal_irc_outtags_cb (ptr_relay_client, NULL,
                                     "test,irc_outtags_notice",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     (void *)";NOTICE bob :a notice");
    CHECK_SENT_CLIENT(":alice NOTICE bob :a notice");

    /* Message sent by this client: not relayed (already displayed). */
    arraylist_clear (sent_messages_client);
    snprintf (str_tags, sizeof (str_tags),
              "relay_client_%d;PRIVMSG #test :hello", ptr_relay_client->id);
    relay_irc_signal_irc_outtags_cb (ptr_relay_client, NULL,
                                     "test,irc_outtags_privmsg",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     (void *)str_tags);
    CHECK_SENT_CLIENT(NULL);

    /* Command not relayed. */
    relay_irc_signal_irc_outtags_cb (ptr_relay_client, NULL,
                                     "test,irc_outtags_mode",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     (void *)";MODE #test +o bob");
    CHECK_SENT_CLIENT(NULL);

    /* Capability "echo-message" enabled: not relayed. */
    RELAY_IRC_DATA(ptr_relay_client, irc_cap_echo_message) = 1;
    relay_irc_signal_irc_outtags_cb (ptr_relay_client, NULL,
                                     "test,irc_outtags_privmsg",
                                     WEECHAT_HOOK_SIGNAL_STRING,
                                     (void *)";PRIVMSG #test :hello");
    CHECK_SENT_CLIENT(NULL);
}

/*
 * Test functions:
 *   relay_irc_signal_irc_disc_cb
 */

TEST(RelayIrcWithClient, SignalIrcDiscCb)
{
    relay_client_set_status (ptr_relay_client, RELAY_STATUS_CONNECTED);

    /* Another server disconnected: nothing done. */
    arraylist_clear (sent_messages_client);
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_irc_signal_irc_disc_cb (ptr_relay_client, NULL,
                                              "irc_server_disconnected",
                                              WEECHAT_HOOK_SIGNAL_STRING,
                                              (void *)"other"));
    CHECK_SENT_CLIENT(NULL);
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, ptr_relay_client->status);

    /* Server of client disconnected: client is disconnected. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_irc_signal_irc_disc_cb (ptr_relay_client, NULL,
                                              "irc_server_disconnected",
                                              WEECHAT_HOOK_SIGNAL_STRING,
                                              (void *)"test"));
    CHECK_SENT_CLIENT(":weechat.relay.irc ERROR :WeeChat: disconnected from "
                      "server \"test\"");
    CHECK_SENT_CLIENT(":weechat.relay.irc ERROR :Closing Link");
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, ptr_relay_client->status);
}

/*
 * Test functions:
 *   relay_irc_tag_relayed
 */

TEST(RelayIrcWithClient, TagRelayed)
{
    LONGS_EQUAL(0, relay_irc_tag_relayed (NULL, NULL));
    LONGS_EQUAL(0, relay_irc_tag_relayed (NULL, "time"));
    LONGS_EQUAL(0, relay_irc_tag_relayed (ptr_relay_client, NULL));
    LONGS_EQUAL(0, relay_irc_tag_relayed (ptr_relay_client, ""));

    /* No capability negotiated by the client: no tag relayed. */
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, server_capabilities));
    LONGS_EQUAL(0, relay_irc_tag_relayed (ptr_relay_client, "time"));
    LONGS_EQUAL(0, relay_irc_tag_relayed (ptr_relay_client,
                                          "time=2026-09-08T04:25:41.259Z"));

    /* Capability "echo-message" allows no tag. */
    RELAY_IRC_DATA(ptr_relay_client, server_capabilities) =
        1 << RELAY_IRC_CAPAB_ECHO_MESSAGE;
    LONGS_EQUAL(0, relay_irc_tag_relayed (ptr_relay_client, "time"));

    /* Capability "server-time" allows only the tag "time". */
    RELAY_IRC_DATA(ptr_relay_client, server_capabilities) =
        1 << RELAY_IRC_CAPAB_SERVER_TIME;
    LONGS_EQUAL(1, relay_irc_tag_relayed (ptr_relay_client, "time"));
    LONGS_EQUAL(1, relay_irc_tag_relayed (ptr_relay_client, "time="));
    LONGS_EQUAL(1, relay_irc_tag_relayed (ptr_relay_client,
                                          "time=2026-09-08T04:25:41.259Z"));
    LONGS_EQUAL(0, relay_irc_tag_relayed (ptr_relay_client, "tim"));
    LONGS_EQUAL(0, relay_irc_tag_relayed (ptr_relay_client, "times"));
    LONGS_EQUAL(0, relay_irc_tag_relayed (ptr_relay_client, "msgid=abcdef"));
    LONGS_EQUAL(0, relay_irc_tag_relayed (ptr_relay_client, "account=alice"));
}

/*
 * Test functions:
 *   relay_irc_remove_tags_not_negotiated
 */

TEST(RelayIrcWithClient, RemoveTagsNotNegotiated)
{
    char *message;

    POINTERS_EQUAL(NULL, relay_irc_remove_tags_not_negotiated (NULL, NULL));
    POINTERS_EQUAL(NULL, relay_irc_remove_tags_not_negotiated (NULL, "test"));
    POINTERS_EQUAL(NULL,
                   relay_irc_remove_tags_not_negotiated (ptr_relay_client,
                                                         NULL));

    /* No capability negotiated by the client: all tags are removed. */
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, server_capabilities));
    WEE_CHECK_REMOVE_TAGS("", "");
    WEE_CHECK_REMOVE_TAGS("", "@time=2026-09-08T04:25:41.259Z");
    WEE_CHECK_REMOVE_TAGS(":server 318 alice bob :End of /WHOIS list.",
                          ":server 318 alice bob :End of /WHOIS list.");
    WEE_CHECK_REMOVE_TAGS(":server 318 alice bob :End of /WHOIS list.",
                          "@time=2026-09-08T04:25:41.259Z "
                          ":server 318 alice bob :End of /WHOIS list.");
    WEE_CHECK_REMOVE_TAGS(":server 318 alice bob :End of /WHOIS list.",
                          "@time=2026-09-08T04:25:41.259Z;msgid=abcdef  "
                          ":server 318 alice bob :End of /WHOIS list.");

    /* Capability "server-time" negotiated: only the tag "time" is kept. */
    RELAY_IRC_DATA(ptr_relay_client, server_capabilities) =
        1 << RELAY_IRC_CAPAB_SERVER_TIME;
    WEE_CHECK_REMOVE_TAGS(":server 318 alice bob :End of /WHOIS list.",
                          ":server 318 alice bob :End of /WHOIS list.");
    WEE_CHECK_REMOVE_TAGS("@time=2026-09-08T04:25:41.259Z "
                          ":server 318 alice bob :End of /WHOIS list.",
                          "@time=2026-09-08T04:25:41.259Z "
                          ":server 318 alice bob :End of /WHOIS list.");
    WEE_CHECK_REMOVE_TAGS("@time=2026-09-08T04:25:41.259Z "
                          ":server 318 alice bob :End of /WHOIS list.",
                          "@msgid=abcdef;time=2026-09-08T04:25:41.259Z;"
                          "account=alice "
                          ":server 318 alice bob :End of /WHOIS list.");
    WEE_CHECK_REMOVE_TAGS(":server 318 alice bob :End of /WHOIS list.",
                          "@msgid=abcdef;account=alice "
                          ":server 318 alice bob :End of /WHOIS list.");
}

/*
 * Test functions:
 *   relay_irc_hsignal_irc_redir_cb
 */

TEST(RelayIrcWithClient, HsignalIrcRedirCb)
{
    struct t_hashtable *hashtable;
    char signal[128];

    hashtable = hashtable_new (32, WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING, NULL, NULL);
    hashtable_set (hashtable, "output",
                   ":server 311 alice bob user host * :Bob\n"
                   "@time=2026-01-01T00:00:00.000Z;msgid=abc "
                   ":server 318 alice bob :End of /WHOIS list.");

    /* Invalid signal or unknown client: nothing sent. */
    arraylist_clear (sent_messages_client);
    relay_irc_hsignal_irc_redir_cb (ptr_relay_client, NULL,
                                    "irc_redirection_relay_xxx", hashtable);
    relay_irc_hsignal_irc_redir_cb (ptr_relay_client, NULL,
                                    "irc_redirection_relay_9999_whois",
                                    hashtable);
    CHECK_SENT_CLIENT(NULL);

    /* Messages sent to client, without tags not negotiated. */
    snprintf (signal, sizeof (signal), "irc_redirection_relay_%d_whois",
              ptr_relay_client->id);
    LONGS_EQUAL(WEECHAT_RC_OK,
                relay_irc_hsignal_irc_redir_cb (ptr_relay_client, NULL,
                                                signal, hashtable));
    LONGS_EQUAL(2, arraylist_size (sent_messages_client));
    CHECK_SENT_CLIENT(":server 311 alice bob user host * :Bob");
    CHECK_SENT_CLIENT(":server 318 alice bob :End of /WHOIS list.");

    /* Capability "server-time" negotiated: tag "time" kept. */
    arraylist_clear (sent_messages_client);
    RELAY_IRC_DATA(ptr_relay_client, server_capabilities) |=
        1 << RELAY_IRC_CAPAB_SERVER_TIME;
    relay_irc_hsignal_irc_redir_cb (ptr_relay_client, NULL, signal, hashtable);
    CHECK_SENT_CLIENT("@time=2026-01-01T00:00:00.000Z "
                      ":server 318 alice bob :End of /WHOIS list.");

    /* No output: nothing sent. */
    arraylist_clear (sent_messages_client);
    hashtable_remove (hashtable, "output");
    relay_irc_hsignal_irc_redir_cb (ptr_relay_client, NULL, signal, hashtable);
    CHECK_SENT_CLIENT(NULL);

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   relay_irc_get_line_info
 */

TEST(RelayIrcWithClient, GetLineInfo)
{
    struct t_gui_line_data *ptr_line_data;
    int irc_command, irc_action;
    time_t date;
    const char *nick, *nick1, *nick2, *host;
    char *tags, *message;

    /* Test date: 2023-12-25T10:29:09.456789Z */
    gui_chat_printf_datetime_tags (gui_buffers, 1703500149, 456789,
                                   "irc_privmsg,nick_alice,host_user@host",
                                   "alice\thello");
    ptr_line_data = gui_buffers->own_lines->last_line->data;

    /* Default time format */
    relay_irc_get_line_info (ptr_relay_client, gui_buffers, ptr_line_data,
                             &irc_command, &irc_action, &date,
                             &nick, &nick1, &nick2, &host, &tags, &message);
    CHECK(irc_command >= 0);
    LONGS_EQUAL(0, irc_action);
    LONGS_EQUAL(1703500149, date);
    STRCMP_EQUAL("alice", nick);
    STRCMP_EQUAL("user@host", host);
    POINTERS_EQUAL(NULL, tags);
    STRCMP_EQUAL("[10:29] hello", message);
    free (message);

    /* Extra specifiers are supported */
    config_file_option_set (relay_config_irc_backlog_time_format,
                            "%@[%T.%.3] ", 1);
    relay_irc_get_line_info (ptr_relay_client, gui_buffers, ptr_line_data,
                             &irc_command, &irc_action, &date,
                             &nick, &nick1, &nick2, &host, &tags, &message);
    POINTERS_EQUAL(NULL, tags);
    STRCMP_EQUAL("[10:29:09.456] hello", message);
    free (message);

    /* Empty time format */
    config_file_option_set (relay_config_irc_backlog_time_format, "", 1);
    relay_irc_get_line_info (ptr_relay_client, gui_buffers, ptr_line_data,
                             &irc_command, &irc_action, &date,
                             &nick, &nick1, &nick2, &host, &tags, &message);
    POINTERS_EQUAL(NULL, tags);
    STRCMP_EQUAL("hello", message);
    free (message);

    config_file_option_reset (relay_config_irc_backlog_time_format, 1);
}

/*
 * Test functions:
 *   relay_irc_send_channel_backlog
 */

TEST(RelayIrcWithClient, SendChannelBacklog)
{
    struct t_gui_buffer *buffer;

    RELAY_IRC_DATA(ptr_relay_client, nick) = strdup ("alice");
    config_file_option_set (relay_config_irc_backlog_time_format, "", 1);
    config_file_option_set (relay_config_irc_backlog_tags, "*", 1);

    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":alice!user_a@host_a JOIN #test\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":bob!user_b@host_b JOIN #test\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":bob!user_b@host_b PRIVMSG #test :hello\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":bob!user_b@host_b PRIVMSG #test :\001ACTION waves\001\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":bob!user_b@host_b NICK :bob2\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":bob2!user_b@host_b PART #test\"");
    buffer = gui_buffer_search_by_full_name ("irc.test.#test");
    CHECK(buffer);

    /* All IRC messages sent to client (except join/part/quit of self nick). */
    arraylist_clear (sent_messages_client);
    relay_irc_send_channel_backlog (ptr_relay_client, "#test", buffer);
    LONGS_EQUAL(5, arraylist_size (sent_messages_client));
    CHECK(!arraylist_search (sent_messages_client,
                             (void *)":alice!user_a@host_a JOIN :#test",
                             NULL, NULL));
    CHECK_SENT_CLIENT(":bob!user_b@host_b JOIN :#test");
    CHECK_SENT_CLIENT(":bob!user_b@host_b PRIVMSG #test :hello");
    CHECK_SENT_CLIENT(":bob!user_b@host_b PRIVMSG #test :\001ACTION waves\001");
    CHECK_SENT_CLIENT(":bob NICK :bob2");
    CHECK_SENT_CLIENT(":bob2!user_b@host_b PART #test");

    /* Max number of messages. */
    arraylist_clear (sent_messages_client);
    config_file_option_set (relay_config_irc_backlog_max_number, "1", 1);
    relay_irc_send_channel_backlog (ptr_relay_client, "#test", buffer);
    LONGS_EQUAL(1, arraylist_size (sent_messages_client));
    CHECK_SENT_CLIENT(":bob2!user_b@host_b PART #test");
    config_file_option_reset (relay_config_irc_backlog_max_number, 1);

    /* Only private messages, with server-time. */
    arraylist_clear (sent_messages_client);
    config_file_option_set (relay_config_irc_backlog_tags, "irc_privmsg", 1);
    RELAY_IRC_DATA(ptr_relay_client, server_capabilities) |=
        1 << RELAY_IRC_CAPAB_SERVER_TIME;
    relay_irc_send_channel_backlog (ptr_relay_client, "#test", buffer);
    LONGS_EQUAL(2, arraylist_size (sent_messages_client));
    CHECK(strncmp ((const char *)arraylist_get (sent_messages_client, 0),
                   "@time=", 6) == 0);
    CHECK(strstr ((const char *)arraylist_get (sent_messages_client, 0),
                  ".000Z :bob!user_b@host_b PRIVMSG #test :hello"));

    /* Empty buffer: nothing sent. */
    arraylist_clear (sent_messages_client);
    gui_buffer_clear (buffer);
    relay_irc_send_channel_backlog (ptr_relay_client, "#test", buffer);
    CHECK_SENT_CLIENT(NULL);

    config_file_option_reset (relay_config_irc_backlog_time_format, 1);
    config_file_option_reset (relay_config_irc_backlog_tags, 1);
}

/*
 * Test functions:
 *   relay_irc_send_join
 */

TEST(RelayIrcWithClient, SendJoin)
{
    RELAY_IRC_DATA(ptr_relay_client, nick) = strdup ("alice");
    config_file_option_set (relay_config_irc_backlog_time_format, "", 1);

    /* Unknown channel: default host, no topic, empty list of nicks. */
    arraylist_clear (sent_messages_client);
    relay_irc_send_join (ptr_relay_client, "#unknown");
    CHECK_SENT_CLIENT(":alice!weechat@proxy JOIN #unknown");
    CHECK_SENT_CLIENT(":weechat.relay.irc 366 alice #unknown "
                      ":End of /NAMES list.");

    /* Channel joined, with topic, nicks and backlog. */
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":alice!user_a@host_a JOIN #test\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":server 332 alice #test :the topic\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":server 353 alice = #test :@alice +bob carol\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":server 366 alice #test :End of /NAMES list\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":bob!user_b@host_b PRIVMSG #test :hello\"");
    arraylist_clear (sent_messages_client);
    relay_irc_send_join (ptr_relay_client, "#test");
    CHECK_SENT_CLIENT(":alice!user_a@host_a JOIN #test");
    CHECK_SENT_CLIENT(":weechat.relay.irc 332 alice #test :the topic");
    CHECK_SENT_CLIENT(":weechat.relay.irc 353 alice = #test "
                      ":@alice +bob carol");
    CHECK_SENT_CLIENT(":weechat.relay.irc 366 alice #test "
                      ":End of /NAMES list.");
    CHECK_SENT_CLIENT(":bob!user_b@host_b PRIVMSG #test :hello");

    config_file_option_reset (relay_config_irc_backlog_time_format, 1);
}

/*
 * Test functions:
 *   relay_irc_send_join_channels
 */

TEST(RelayIrcWithClient, SendJoinChannels)
{
    RELAY_IRC_DATA(ptr_relay_client, nick) = strdup ("alice");
    config_file_option_set (relay_config_irc_backlog_time_format, "", 1);

    /* No channels. */
    arraylist_clear (sent_messages_client);
    relay_irc_send_join_channels (ptr_relay_client);
    CHECK_SENT_CLIENT(NULL);

    /* Channel with nicks, channel without nicks, private buffer. */
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":alice!user_a@host_a JOIN #test\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":alice!user_a@host_a JOIN #empty\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":alice!user_a@host_a PART #empty\"");
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":bob!user_b@host_b PRIVMSG alice :private\"");
    arraylist_clear (sent_messages_client);
    relay_irc_send_join_channels (ptr_relay_client);
    CHECK_SENT_CLIENT(":alice!user_a@host_a JOIN #test");
    CHECK_SENT_CLIENT(":bob!user_b@host_b PRIVMSG bob :private");
    CHECK(!arraylist_search (sent_messages_client,
                             (void *)":alice!user_a@host_a JOIN #empty",
                             NULL, NULL));

    config_file_option_reset (relay_config_irc_backlog_time_format, 1);
}

/*
 * Test functions:
 *   relay_irc_input_send
 */

TEST(RelayIrcWithClient, InputSend)
{
    arraylist_clear (sent_messages_irc);
    relay_irc_input_send (ptr_relay_client, "#test", "priority_high",
                          "this is a test");
    CHECK_SENT_IRC("test;#test;priority_high;relay_client_1;this is a test");
}

/*
 * Test functions:
 *   relay_irc_hook_signals
 *   relay_irc_close_connection
 */

TEST(RelayIrcWithClient, HookSignals)
{
    char *protocol_args;

    /* No IRC server: no hooks. */
    protocol_args = ptr_relay_client->protocol_args;
    ptr_relay_client->protocol_args = NULL;
    relay_irc_hook_signals (ptr_relay_client);
    POINTERS_EQUAL(NULL, RELAY_IRC_DATA(ptr_relay_client, hook_signal_irc_in2));
    ptr_relay_client->protocol_args = protocol_args;

    relay_irc_hook_signals (ptr_relay_client);
    CHECK(RELAY_IRC_DATA(ptr_relay_client, hook_signal_irc_in2));
    CHECK(RELAY_IRC_DATA(ptr_relay_client, hook_signal_irc_outtags));
    CHECK(RELAY_IRC_DATA(ptr_relay_client, hook_signal_irc_disc));
    CHECK(RELAY_IRC_DATA(ptr_relay_client, hook_hsignal_irc_redir));

    /* Message received from IRC server relayed to client. */
    arraylist_clear (sent_messages_client);
    run_cmd_quiet ("/command -buffer irc.server.test irc /server fakerecv "
                   "\":bob!user_b@host_b PRIVMSG alice :hi\"");
    CHECK_SENT_CLIENT(":bob!user_b@host_b PRIVMSG alice :hi");

    /* Close connection: hooks are removed. */
    RELAY_IRC_DATA(ptr_relay_client, connected) = 1;
    relay_irc_close_connection (ptr_relay_client);
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, connected));
    POINTERS_EQUAL(NULL, RELAY_IRC_DATA(ptr_relay_client, hook_signal_irc_in2));
    POINTERS_EQUAL(NULL, RELAY_IRC_DATA(ptr_relay_client, hook_signal_irc_outtags));
    POINTERS_EQUAL(NULL, RELAY_IRC_DATA(ptr_relay_client, hook_signal_irc_disc));
    POINTERS_EQUAL(NULL, RELAY_IRC_DATA(ptr_relay_client, hook_hsignal_irc_redir));
    relay_irc_close_connection (ptr_relay_client);
}

/*
 * Test functions:
 *   relay_irc_capability_compare_cb
 */

TEST(RelayIrc, CapabilityCompareCb)
{
    LONGS_EQUAL(0, relay_irc_capability_compare_cb (NULL, NULL,
                                                    (void *)"abc",
                                                    (void *)"abc"));
    CHECK(relay_irc_capability_compare_cb (NULL, NULL,
                                           (void *)"abc",
                                           (void *)"abd") < 0);
    CHECK(relay_irc_capability_compare_cb (NULL, NULL,
                                           (void *)"abd",
                                           (void *)"abc") > 0);
}

/*
 * Test functions:
 *   relay_irc_capability_free_db
 */

TEST(RelayIrc, CapabilityFreeDb)
{
    relay_irc_capability_free_db (NULL, NULL, NULL);
    relay_irc_capability_free_db (NULL, NULL, strdup ("server-time"));
}

/*
 * Test functions:
 *   relay_irc_cap_enabled
 */

TEST(RelayIrcWithClient, CapEnabled)
{
    LONGS_EQUAL(0, relay_irc_cap_enabled (NULL, NULL));
    LONGS_EQUAL(0, relay_irc_cap_enabled (NULL, "echo-message"));
    LONGS_EQUAL(0, relay_irc_cap_enabled (ptr_relay_client, NULL));
    LONGS_EQUAL(0, relay_irc_cap_enabled (ptr_relay_client, ""));

    LONGS_EQUAL(0, relay_irc_cap_enabled (ptr_relay_client, "echo-message"));

    hashtable_set (ptr_server->cap_list, "echo-message", NULL);
    LONGS_EQUAL(1, relay_irc_cap_enabled (ptr_relay_client, "echo-message"));
    hashtable_remove (ptr_server->cap_list, "echo-message");

    LONGS_EQUAL(0, relay_irc_cap_enabled (ptr_relay_client, "echo-message"));
}

/*
 * Test functions:
 *   relay_irc_get_supported_caps
 */

TEST(RelayIrcWithClient, GetSupportedCaps)
{
    int supported_caps;

    supported_caps = relay_irc_get_supported_caps (ptr_relay_client);
    LONGS_EQUAL(1 << RELAY_IRC_CAPAB_SERVER_TIME, supported_caps);

    hashtable_set (ptr_server->cap_list, "echo-message", NULL);
    supported_caps = relay_irc_get_supported_caps (ptr_relay_client);
    LONGS_EQUAL((1 << RELAY_IRC_CAPAB_SERVER_TIME)
                | (1 << RELAY_IRC_CAPAB_ECHO_MESSAGE),
                supported_caps);
    hashtable_remove (ptr_server->cap_list, "echo-message");
}

/*
 * Test functions:
 *   relay_irc_get_list_caps
 */

TEST(RelayIrc, RelayGetListCaps)
{
    struct t_arraylist *list_caps;
    int i, size;

    list_caps = relay_irc_get_list_caps ();
    CHECK(list_caps);
    size = arraylist_size (list_caps);
    LONGS_EQUAL(RELAY_IRC_NUM_CAPAB, size);

    /* Check that it's properly sorted. */
    for (i = 1; i < size; i++)
    {
        CHECK(strcmp ((const char *)arraylist_get (list_caps, i - 1),
                      (const char *)arraylist_get (list_caps, i)) < 0);
    }

    arraylist_free (list_caps);
}

/*
 * Test functions:
 *   relay_irc_recv_command_capab
 */

TEST(RelayIrcWithClient, RecvCommandCapab)
{
    relay_client_set_status (ptr_relay_client, RELAY_STATUS_CONNECTING);

    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, server_capabilities));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_ls_received));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));

    /* Not enough parameters */
    CLIENT_RECV(":alice!user@host CAP");

    /* List supported capabilities. */
    CLIENT_RECV(":alice!user@host CAP LS");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick LS :server-time");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, cap_ls_received));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));

    /* Enable "echo-message" in IRC server and list supported capabilities. */
    hashtable_set (ptr_server->cap_list, "echo-message", NULL);
    CLIENT_RECV(":alice!user@host CAP LS");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick LS :echo-message server-time");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, cap_ls_received));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));
    hashtable_remove (ptr_server->cap_list, "echo-message");

    /* Request unknown capability: reject. */
    CLIENT_RECV(":alice!user@host CAP REQ unknown");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick NAK :unknown");
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, server_capabilities));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));

    /* Request 1 supported capability: accept. */
    CLIENT_RECV(":alice!user@host CAP REQ server-time");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick ACK :server-time");
    CHECK(RELAY_IRC_DATA(ptr_relay_client, server_capabilities)
          & (1 << RELAY_IRC_CAPAB_SERVER_TIME));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));
    RELAY_IRC_DATA(ptr_relay_client, server_capabilities) = 0;

    /* Request 2 supported capabilities: accept. */
    hashtable_set (ptr_server->cap_list, "echo-message", NULL);
    CLIENT_RECV(":alice!user@host CAP REQ :server-time echo-message");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick ACK :server-time echo-message");
    CHECK(RELAY_IRC_DATA(ptr_relay_client, server_capabilities)
          & ((1 << RELAY_IRC_CAPAB_SERVER_TIME)
             | (1 << RELAY_IRC_CAPAB_ECHO_MESSAGE)));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));
    RELAY_IRC_DATA(ptr_relay_client, server_capabilities) = 0;
    hashtable_remove (ptr_server->cap_list, "echo-message");

    /* Request unknown + supported capabilities: reject both. */
    CLIENT_RECV(":alice!user@host CAP REQ :server-time unknown");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick NAK :server-time unknown");
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, server_capabilities));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));

    /* Request with empty list: end of capability negotiation. */
    CLIENT_RECV(":alice!user@host CAP REQ :");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick NAK :");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));

    RELAY_IRC_DATA(ptr_relay_client, cap_end_received) = 0;

    /* End capability negotiation. */
    CLIENT_RECV(":alice!user@host CAP END");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));
}

/*
 * Test functions:
 *   relay_irc_parse_ctcp
 */

TEST(RelayIrcWithClient, ParseCtcp)
{
    char *ctcp_type, *ctcp_params;

    relay_irc_parse_ctcp (NULL, NULL, NULL);
    relay_irc_parse_ctcp ("test", NULL, NULL);

    ctcp_type = (char *)0x01;
    ctcp_params = (char *)0x01;
    relay_irc_parse_ctcp (NULL, &ctcp_type, &ctcp_params);
    STRCMP_EQUAL(NULL, ctcp_type);
    STRCMP_EQUAL(NULL, ctcp_params);

    ctcp_type = (char *)0x01;
    ctcp_params = (char *)0x01;
    relay_irc_parse_ctcp ("\001ACTION is testing\001", &ctcp_type, &ctcp_params);
    STRCMP_EQUAL("ACTION", ctcp_type);
    STRCMP_EQUAL("is testing", ctcp_params);
    free (ctcp_type);
    free (ctcp_params);

    ctcp_type = (char *)0x01;
    ctcp_params = (char *)0x01;
    relay_irc_parse_ctcp ("\001ACTION   is testing  \001 extra", &ctcp_type, &ctcp_params);
    STRCMP_EQUAL("ACTION", ctcp_type);
    STRCMP_EQUAL("  is testing  ", ctcp_params);
    free (ctcp_type);
    free (ctcp_params);

    ctcp_type = (char *)0x01;
    ctcp_params = (char *)0x01;
    relay_irc_parse_ctcp ("\001VERSION\001", &ctcp_type, &ctcp_params);
    STRCMP_EQUAL("VERSION", ctcp_type);
    STRCMP_EQUAL(NULL, ctcp_params);
    free (ctcp_type);

    ctcp_type = (char *)0x01;
    ctcp_params = (char *)0x01;
    relay_irc_parse_ctcp ("\001ACTION is testing", &ctcp_type, &ctcp_params);
    STRCMP_EQUAL(NULL, ctcp_type);
    STRCMP_EQUAL(NULL, ctcp_params);

    ctcp_type = (char *)0x01;
    ctcp_params = (char *)0x01;
    relay_irc_parse_ctcp ("\001VERSION", &ctcp_type, &ctcp_params);
    STRCMP_EQUAL(NULL, ctcp_type);
    STRCMP_EQUAL(NULL, ctcp_params);

    ctcp_type = (char *)0x01;
    ctcp_params = (char *)0x01;
    relay_irc_parse_ctcp ("test", &ctcp_type, &ctcp_params);
    STRCMP_EQUAL(NULL, ctcp_type);
    STRCMP_EQUAL(NULL, ctcp_params);

    ctcp_type = (char *)0x01;
    ctcp_params = (char *)0x01;
    relay_irc_parse_ctcp ("", &ctcp_type, &ctcp_params);
    STRCMP_EQUAL(NULL, ctcp_type);
    STRCMP_EQUAL(NULL, ctcp_params);
}

/*
 * Test functions:
 *   relay_irc_recv
 */

TEST(RelayIrcWithClient, Recv)
{
    relay_client_set_status (ptr_relay_client, RELAY_STATUS_CONNECTING);

    /* NICK */
    CLIENT_RECV("NICK alice");
    STRCMP_EQUAL("alice", RELAY_IRC_DATA(ptr_relay_client, nick));

    CLIENT_RECV("NICK bob");
    STRCMP_EQUAL("bob", RELAY_IRC_DATA(ptr_relay_client, nick));

    /* PASS */
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, password_ok));

    CLIENT_RECV("PASS invalid");
    LONGS_EQUAL(RELAY_STATUS_CONNECTING, ptr_relay_client->status);

    CLIENT_RECV("PASS secret");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, password_ok));
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, ptr_relay_client->status);

    free (ptr_relay_client->protocol_args);
    ptr_relay_client->protocol_args = NULL;
    relay_client_set_status (ptr_relay_client, RELAY_STATUS_CONNECTING);
    RELAY_IRC_DATA(ptr_relay_client, password_ok) = 0;

    CLIENT_RECV("PASS test2:secret");
    STRCMP_EQUAL("test2", ptr_relay_client->protocol_args);
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, password_ok));
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, ptr_relay_client->status);
    free (ptr_relay_client->protocol_args);
    ptr_relay_client->protocol_args = strdup ("test");

    /* USER */
    relay_client_set_status (ptr_relay_client, RELAY_STATUS_CONNECTING);
    CLIENT_RECV("USER alice 0 * :alice");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, user_received));
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, connected));
    STRCMP_EQUAL("alice", RELAY_IRC_DATA(ptr_relay_client, nick));
    CHECK_SENT_CLIENT(":bob!proxy NICK :alice");
    CHECK_SENT_CLIENT(":weechat.relay.irc 001 alice :Welcome to the "
                      "Internet Relay Chat Network alice!weechat@proxy");

    /* JOIN */
    CLIENT_RECV("JOIN #test");
    CHECK_SENT_IRC("test;;priority_high;relay_client_1;/join #test");

    /* PART */
    CLIENT_RECV("PART #test");
    CHECK_SENT_IRC("test;;priority_high;relay_client_1;/part #test");

    /* PING */
    CLIENT_RECV("PING :12345");
    CHECK_SENT_CLIENT(":weechat.relay.irc PONG weechat.relay.irc :12345");

    /* NOTICE */
    CLIENT_RECV("NOTICE bob :a notice");
    CHECK_SENT_IRC("test;;priority_high;relay_client_1;/notice bob a notice");

    /* PRIVMSG to channel */
    CLIENT_RECV("PRIVMSG #test :message to channel");
    CHECK_SENT_IRC("test;#test;priority_high,user_message;relay_client_1;"
                   "message to channel");

    /* PRIVMSG to user */
    CLIENT_RECV("PRIVMSG bob :private message");
    CHECK_SENT_IRC("test;;priority_high;relay_client_1;"
                   "/query bob private message");

    /* WHOIS */
    CLIENT_RECV("WHOIS bob");
    CHECK_SENT_IRC("test;;priority_high;relay_client_1;/quote WHOIS bob");
}

/*
 * Test functions:
 *   relay_irc_recv (message too long)
 */

TEST(RelayIrcWithClient, RecvTooLong)
{
    char *message;
    size_t size;

    size = RELAY_IRC_MESSAGE_MAX_LENGTH + 2;
    message = (char *)malloc (size);
    CHECK(message);

    relay_client_set_status (ptr_relay_client, RELAY_STATUS_CONNECTING);

    /* NICK too long, before any nick is set: ignored, reply 417 with "*" */
    memcpy (message, "NICK ", 5);
    memset (message + 5, 'a', RELAY_IRC_MESSAGE_MAX_LENGTH - 4);
    message[RELAY_IRC_MESSAGE_MAX_LENGTH + 1] = '\0';
    LONGS_EQUAL(RELAY_IRC_MESSAGE_MAX_LENGTH + 1, strlen (message));
    CLIENT_RECV(message);
    POINTERS_EQUAL(NULL, RELAY_IRC_DATA(ptr_relay_client, nick));
    CHECK_SENT_CLIENT(":weechat.relay.irc 417 * :Input line was too long");
    LONGS_EQUAL(RELAY_STATUS_CONNECTING, ptr_relay_client->status);

    /* NICK with max length: accepted */
    message[RELAY_IRC_MESSAGE_MAX_LENGTH] = '\0';
    CLIENT_RECV(message);
    STRCMP_EQUAL(message + 5, RELAY_IRC_DATA(ptr_relay_client, nick));
    CHECK_SENT_CLIENT(NULL);

    /* NICK too long, nick already set: ignored, reply 417 with nick */
    CLIENT_RECV("NICK alice");
    STRCMP_EQUAL("alice", RELAY_IRC_DATA(ptr_relay_client, nick));
    message[RELAY_IRC_MESSAGE_MAX_LENGTH] = 'a';
    CLIENT_RECV(message);
    STRCMP_EQUAL("alice", RELAY_IRC_DATA(ptr_relay_client, nick));
    CHECK_SENT_CLIENT(":weechat.relay.irc 417 alice "
                      ":Input line was too long");
    LONGS_EQUAL(RELAY_STATUS_CONNECTING, ptr_relay_client->status);

    /* PASS too long: ignored */
    memcpy (message, "PASS secret ", 12);
    CLIENT_RECV(message);
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, password_ok));
    LONGS_EQUAL(RELAY_STATUS_CONNECTING, ptr_relay_client->status);

    /* Connect the client */
    CLIENT_RECV("PASS secret");
    CLIENT_RECV("USER alice 0 * :alice");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, connected));

    /* PRIVMSG too long: not sent to IRC server, reply 417 */
    memcpy (message, "PRIVMSG #test :", 15);
    CLIENT_RECV(message);
    CHECK_SENT_IRC(NULL);
    CHECK_SENT_CLIENT(":weechat.relay.irc 417 alice "
                      ":Input line was too long");
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, ptr_relay_client->status);

    free (message);
}

/*
 * Test functions:
 *   relay_irc_alloc
 *   relay_irc_get_initial_status
 *   relay_irc_free
 */

TEST(RelayIrcWithClient, Alloc)
{
    struct t_relay_client client, *ptr_client;

    memset (&client, 0, sizeof (client));
    ptr_client = &client;

    /* Password set: password expected. */
    relay_irc_alloc (&client);
    CHECK(client.protocol_data);
    STRCMP_EQUAL("weechat.relay.irc", RELAY_IRC_DATA(ptr_client, address));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_client, password_ok));
    POINTERS_EQUAL(NULL, RELAY_IRC_DATA(ptr_client, nick));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_client, connected));
    LONGS_EQUAL(RELAY_STATUS_AUTHENTICATING,
                relay_irc_get_initial_status (&client));
    relay_irc_free (&client);
    POINTERS_EQUAL(NULL, client.protocol_data);
    relay_irc_free (&client);
    relay_irc_free (NULL);

    /* No password: no password expected. */
    config_file_option_set (relay_config_network_password, "", 1);
    relay_irc_alloc (&client);
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_client, password_ok));
    LONGS_EQUAL(RELAY_STATUS_CONNECTED, relay_irc_get_initial_status (&client));
    relay_irc_free (&client);
}

/*
 * Test functions:
 *   relay_irc_alloc_with_infolist
 */

TEST(RelayIrcWithClient, AllocWithInfolist)
{
    struct t_relay_client client, *ptr_client;
    struct t_infolist *infolist;
    struct t_infolist_item *item;

    memset (&client, 0, sizeof (client));
    ptr_client = &client;
    client.protocol_args = (char *)"test";

    /* Client connected: signals are hooked. */
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "address", "my.address");
    infolist_new_var_integer (item, "password_ok", 1);
    infolist_new_var_string (item, "nick", "alice");
    infolist_new_var_integer (item, "user_received", 1);
    infolist_new_var_integer (item, "cap_ls_received", 1);
    infolist_new_var_integer (item, "cap_end_received", 1);
    infolist_new_var_integer (item, "connected", 1);
    infolist_new_var_integer (item, "irc_cap_echo_message", 1);
    infolist_new_var_integer (item, "server_capabilities", 3);
    CHECK(infolist_next (infolist));
    relay_irc_alloc_with_infolist (&client, infolist);
    CHECK(client.protocol_data);
    STRCMP_EQUAL("my.address", RELAY_IRC_DATA(ptr_client, address));
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_client, password_ok));
    STRCMP_EQUAL("alice", RELAY_IRC_DATA(ptr_client, nick));
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_client, user_received));
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_client, cap_ls_received));
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_client, cap_end_received));
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_client, connected));
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_client, irc_cap_echo_message));
    LONGS_EQUAL(3, RELAY_IRC_DATA(ptr_client, server_capabilities));
    CHECK(RELAY_IRC_DATA(ptr_client, hook_signal_irc_in2));
    CHECK(RELAY_IRC_DATA(ptr_client, hook_hsignal_irc_redir));
    relay_irc_free (&client);
    infolist_free (infolist);

    /* Client not connected, without nick: no hooks. */
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "address", "my.address");
    infolist_new_var_integer (item, "connected", 0);
    CHECK(infolist_next (infolist));
    relay_irc_alloc_with_infolist (&client, infolist);
    POINTERS_EQUAL(NULL, RELAY_IRC_DATA(ptr_client, nick));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_client, connected));
    POINTERS_EQUAL(NULL, RELAY_IRC_DATA(ptr_client, hook_signal_irc_in2));
    POINTERS_EQUAL(NULL, RELAY_IRC_DATA(ptr_client, hook_hsignal_irc_redir));
    relay_irc_free (&client);
    infolist_free (infolist);
}

/*
 * Test functions:
 *   relay_irc_add_to_infolist
 */

TEST(RelayIrcWithClient, AddToInfolist)
{
    struct t_infolist *infolist;
    struct t_infolist_item *item;

    RELAY_IRC_DATA(ptr_relay_client, nick) = strdup ("alice");
    RELAY_IRC_DATA(ptr_relay_client, connected) = 1;
    relay_client_set_status (ptr_relay_client, RELAY_STATUS_CONNECTED);

    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    LONGS_EQUAL(0, relay_irc_add_to_infolist (NULL, ptr_relay_client, 0));
    LONGS_EQUAL(0, relay_irc_add_to_infolist (item, NULL, 0));

    LONGS_EQUAL(1, relay_irc_add_to_infolist (item, ptr_relay_client, 0));
    CHECK(infolist_next (infolist));
    LONGS_EQUAL(1, infolist_integer (infolist, "connected"));
    STRCMP_EQUAL("weechat.relay.irc", infolist_string (infolist, "address"));
    LONGS_EQUAL(0, infolist_integer (infolist, "password_ok"));
    STRCMP_EQUAL("alice", infolist_string (infolist, "nick"));
    infolist_free (infolist);

    /* Force disconnected state. */
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    LONGS_EQUAL(1, relay_irc_add_to_infolist (item, ptr_relay_client, 1));
    CHECK(infolist_next (infolist));
    LONGS_EQUAL(0, infolist_integer (infolist, "connected"));
    infolist_free (infolist);
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, connected));
}

/*
 * Test functions:
 *   relay_irc_print_log
 */

TEST(RelayIrcWithClient, PrintLog)
{
    struct t_relay_client client, *ptr_client;

    memset (&client, 0, sizeof (client));
    ptr_client = &client;
    relay_irc_print_log (ptr_client);
    relay_irc_print_log (ptr_relay_client);
}
