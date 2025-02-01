/*
 * test-relay-irc.cpp - test relay IRC protocol
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
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
extern void relay_irc_input_send (struct t_relay_client *client,
                                  const char *irc_channel,
                                  const char *options,
                                  const char *format, ...);
extern int relay_irc_cap_enabled (struct t_relay_client *client,
                                  const char *capability);
extern int relay_irc_get_supported_caps (struct t_relay_client *client);
extern struct t_arraylist *relay_irc_get_list_caps ();
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
        /* make C++ compiler happy */
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
        /* make C++ compiler happy */
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
        /* make C++ compiler happy */
        (void) data;
        (void) arraylist;

        return strcmp ((char *)pointer1, (char *)pointer2);
    }

    static void sent_msg_free_cb (void *data, struct t_arraylist *arraylist,
                                  void *pointer)
    {
        /* make C++ compiler happy */
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
        /* initialize list of messages sent to the relay client */
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

        /* initialize list of messages sent to the IRC server */
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

        /* disable auto-open of relay buffer */
        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);

        /* set relay password */
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

        /* create a fake server (no I/O) */
        run_cmd_quiet ("/mute /server add test fake:127.0.0.1 "
                       "-nicks=nick1,nick2,nick3");

        /* get the server pointer */
        ptr_server = irc_server_search ("test");

        /* connect to the fake server */
        run_cmd_quiet ("/connect test");

        /* simulate connection OK to server */
        run_cmd_quiet ("/command -buffer irc.server.test irc "
                       "/server fakerecv "
                       "\":server 001 alice :Welcome on this server, nick1!\"");

        /* create a relay server */
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

        /* create a fake relay client (no I/O) */
        ptr_relay_client = relay_client_new (-1, "test", ptr_relay_server);
    }

    void teardown ()
    {
        relay_client_free (ptr_relay_client);
        ptr_relay_client = NULL;

        relay_server_free (ptr_relay_server);
        ptr_relay_server = NULL;

        /* disconnect and delete the fake server */
        run_cmd_quiet ("/mute /disconnect test");
        run_cmd_quiet ("/mute /server del test");
        ptr_server = NULL;

        /* restore auto-open of relay buffer */
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);

        /* restore relay password */
        config_file_option_reset (relay_config_network_password, 1);
    }
};

/*
 * Tests functions:
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
 * Tests functions:
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
 * Tests functions:
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
 * Tests functions:
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
 * Tests functions:
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
 * Tests functions:
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
 * Tests functions:
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
 * Tests functions:
 *   relay_irc_signal_irc_in2_cb
 */

TEST(RelayIrc, SignalIrcIn2Cb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
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
 * Tests functions:
 *   relay_irc_signal_irc_outtags_cb
 */

TEST(RelayIrc, SignalIrcOuttagsCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_signal_irc_disc_cb
 */

TEST(RelayIrc, SignalIrcDiscCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_hsignal_irc_redir_cb
 */

TEST(RelayIrc, HsignalIrcRedirCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_get_line_info
 */

TEST(RelayIrc, GetLineInfo)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_send_channel_backlog
 */

TEST(RelayIrc, SendChannelBacklog)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_send_join
 */

TEST(RelayIrc, SendJoin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_send_join_channels
 */

TEST(RelayIrc, SendJoinChannels)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
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
 * Tests functions:
 *   relay_irc_hook_signals
 */

TEST(RelayIrc, HookSignals)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_capability_compare_cb
 */

TEST(RelayIrc, CapabilityCompareCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_capability_free_db
 */

TEST(RelayIrc, CapabilityFreeDb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
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
 * Tests functions:
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
 * Tests functions:
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

    /* check that it's properly sorted */
    for (i = 1; i < size; i++)
    {
        CHECK(strcmp ((const char *)arraylist_get (list_caps, i - 1),
                      (const char *)arraylist_get (list_caps, i)) < 0);
    }

    arraylist_free (list_caps);
}

/*
 * Tests functions:
 *   relay_irc_recv_command_capab
 */

TEST(RelayIrcWithClient, RecvCommandCapab)
{
    relay_client_set_status (ptr_relay_client, RELAY_STATUS_CONNECTING);

    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, server_capabilities));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_ls_received));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));

    /* not enough parameters */
    CLIENT_RECV(":alice!user@host CAP");

    /* list supported capabilities */
    CLIENT_RECV(":alice!user@host CAP LS");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick LS :server-time");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, cap_ls_received));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));

    /* enable "echo-message" in IRC server and list supported capabilities */
    hashtable_set (ptr_server->cap_list, "echo-message", NULL);
    CLIENT_RECV(":alice!user@host CAP LS");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick LS :echo-message server-time");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, cap_ls_received));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));
    hashtable_remove (ptr_server->cap_list, "echo-message");

    /* request unknown capability: reject */
    CLIENT_RECV(":alice!user@host CAP REQ unknown");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick NAK :unknown");
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, server_capabilities));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));

    /* request 1 supported capability: accept */
    CLIENT_RECV(":alice!user@host CAP REQ server-time");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick ACK :server-time");
    CHECK(RELAY_IRC_DATA(ptr_relay_client, server_capabilities)
          & (1 << RELAY_IRC_CAPAB_SERVER_TIME));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));
    RELAY_IRC_DATA(ptr_relay_client, server_capabilities) = 0;

    /* request 2 supported capabilities: accept */
    hashtable_set (ptr_server->cap_list, "echo-message", NULL);
    CLIENT_RECV(":alice!user@host CAP REQ :server-time echo-message");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick ACK :server-time echo-message");
    CHECK(RELAY_IRC_DATA(ptr_relay_client, server_capabilities)
          & ((1 << RELAY_IRC_CAPAB_SERVER_TIME)
             | (1 << RELAY_IRC_CAPAB_ECHO_MESSAGE)));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));
    RELAY_IRC_DATA(ptr_relay_client, server_capabilities) = 0;
    hashtable_remove (ptr_server->cap_list, "echo-message");

    /* request unknown + supported capabilities: reject both */
    CLIENT_RECV(":alice!user@host CAP REQ :server-time unknown");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick NAK :server-time unknown");
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, server_capabilities));
    LONGS_EQUAL(0, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));

    /* request with empty list: end of capability negotiation */
    CLIENT_RECV(":alice!user@host CAP REQ :");
    CHECK_SENT_CLIENT(":weechat.relay.irc CAP nick NAK :");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));

    RELAY_IRC_DATA(ptr_relay_client, cap_end_received) = 0;

    /* end capability negotiation */
    CLIENT_RECV(":alice!user@host CAP END");
    LONGS_EQUAL(1, RELAY_IRC_DATA(ptr_relay_client, cap_end_received));
}

/*
 * Tests functions:
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
 * Tests functions:
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
 * Tests functions:
 *   relay_irc_close_connection
 */

TEST(RelayIrc, CloseConnection)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_alloc
 */

TEST(RelayIrc, Alloc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_alloc_with_infolist
 */

TEST(RelayIrc, AllocWithInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_get_initial_status
 */

TEST(RelayIrc, GetInitialStatus)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_free
 */

TEST(RelayIrc, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_add_to_infolist
 */

TEST(RelayIrc, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_irc_print_log
 */

TEST(RelayIrc, PrintLog)
{
    /* TODO: write tests */
}
