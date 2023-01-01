/*
 * test-irc-protocol.cpp - test IRC protocol functions
 *
 * Copyright (C) 2019-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/wee-arraylist.h"
#include "src/core/wee-config-file.h"
#include "src/core/wee-hashtable.h"
#include "src/core/wee-hook.h"
#include "src/core/wee-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-ctcp.h"
#include "src/plugins/irc/irc-protocol.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-nick.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/typing/typing-config.h"
#include "src/plugins/typing/typing-status.h"
#include "src/plugins/xfer/xfer-buffer.h"

extern int irc_protocol_is_numeric_command (const char *str);
extern int irc_protocol_log_level_for_command (const char *command);
extern const char *irc_protocol_nick_address (struct t_irc_server *server,
                                              int server_message,
                                              struct t_irc_nick *nick,
                                              const char *nickname,
                                              const char *address);
extern char *irc_protocol_string_params (const char **params,
                                         int arg_start, int arg_end);
extern char *irc_protocol_cap_to_enable (const char *capabilities,
                                         int sasl_requested);
}

#define IRC_FAKE_SERVER "fake"
#define IRC_MSG_005 "PREFIX=(ohv)@%+ MAXLIST=bqeI:100 MODES=4 "         \
    "NETWORK=StaticBox STATUSMSG=@+ CALLERID=g "                        \
    "CASEMAPPING=strict-rfc1459 NICKLEN=30 MAXNICKLEN=31 "              \
    "USERLEN=16 HOSTLEN=32 CHANNELLEN=50 TOPICLEN=390 DEAF=D "          \
    "CHANTYPES=# CHANMODES=eIbq,k,flj,CFLMPQScgimnprstuz "              \
    "MONITOR=100"
#define IRC_ALL_CAPS "account-notify,away-notify,cap-notify,chghost,"   \
    "extended-join,invite-notify,message-tags,multi-prefix,"            \
    "server-time,setname,userhost-in-names"

#define WEE_CHECK_CAP_TO_ENABLE(__result, __string, __sasl_requested)   \
    str = irc_protocol_cap_to_enable (__string, __sasl_requested);      \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

#define RECV(__irc_msg)                                                 \
    server_recv (__irc_msg);

#define CHECK_CORE(__message)                                           \
    if (!record_search ("core.weechat", __message))                     \
    {                                                                   \
        char **msg = server_build_error (                               \
            "Core message not displayed",                               \
            __message,                                                  \
            "All messages displayed");                                  \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_SRV(__message)                                            \
    if (!record_search ("irc.server." IRC_FAKE_SERVER, __message))      \
    {                                                                   \
        char **msg = server_build_error (                               \
            "Server message not displayed",                             \
            __message,                                                  \
            "All messages displayed");                                  \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_ERROR_PARAMS(__command, __params, __expected_params)      \
    CHECK_SRV("=!= irc: too few parameters received in command "        \
              "\"" __command "\" (received: " #__params ", "            \
              "expected: at least " #__expected_params ")");

#define CHECK_ERROR_NICK(__command)                                     \
    CHECK_SRV("=!= irc: command \"" __command "\" received without "    \
              "nick");

#define CHECK_ERROR_PARSE(__command, __message)                         \
    CHECK_SRV("=!= irc: failed to parse command \"" __command "\" "     \
              "(please report to developers): \"" __message "\"");

#define CHECK_CHAN(__message)                                           \
    if (!record_search ("irc." IRC_FAKE_SERVER ".#test", __message))    \
    {                                                                   \
        char **msg = server_build_error (                               \
            "Channel message not displayed",                            \
            __message,                                                  \
            "All messages displayed");                                  \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_PV(__nick, __message)                                     \
    if (!record_search ("irc." IRC_FAKE_SERVER "." __nick, __message))  \
    {                                                                   \
        char **msg = server_build_error (                               \
            "Private message not displayed",                            \
            __message,                                                  \
            "All messages displayed");                                  \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_NO_MSG                                                    \
    if (arraylist_size (recorded_messages) > 0)                         \
    {                                                                   \
        char **msg = server_build_error (                               \
            "Unexpected message(s) displayed",                          \
            NULL,                                                       \
            NULL);                                                      \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_SENT(__message)                                           \
    if ((__message != NULL)                                             \
        && !arraylist_search (sent_messages, (void *)__message,         \
                              NULL, NULL))                              \
    {                                                                   \
        char **msg = server_build_error (                               \
            "Message not sent to server",                               \
            __message,                                                  \
            "All messages sent");                                       \
        sent_msg_dump (msg);                                            \
        FAIL(string_dyn_free (msg, 0));                                 \
    }                                                                   \
    else if ((__message == NULL)                                        \
             && (arraylist_size (sent_messages) > 0))                   \
    {                                                                   \
        char **msg = server_build_error (                               \
            "Unexpected response(s) sent to the IRC server",            \
            NULL,                                                       \
            NULL);                                                      \
        sent_msg_dump (msg);                                            \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define SRV_INIT                                                        \
    RECV(":server 001 alice :Welcome on this server, alice!");          \
    CHECK_SRV("-- Welcome on this server, alice!");

#define SRV_INIT_JOIN                                                   \
    SRV_INIT;                                                           \
    RECV(":alice!user_a@host_a JOIN #test");                            \
    CHECK_CHAN("--> alice (user_a@host_a) has joined #test");

#define SRV_INIT_JOIN2                                                  \
    SRV_INIT_JOIN;                                                      \
    RECV(":bob!user_b@host_b JOIN #test");                              \
    CHECK_CHAN("--> bob (user_b@host_b) has joined #test");

struct t_irc_server *ptr_server = NULL;
struct t_arraylist *sent_messages = NULL;
struct t_hook *hook_signal_irc_out = NULL;

TEST_GROUP(IrcProtocol)
{
};

/*
 * Tests functions:
 *   irc_protocol_is_numeric_command
 */

TEST(IrcProtocol, IsNumericCommand)
{
    LONGS_EQUAL(0, irc_protocol_is_numeric_command (NULL));
    LONGS_EQUAL(0, irc_protocol_is_numeric_command (""));
    LONGS_EQUAL(0, irc_protocol_is_numeric_command ("abc"));

    LONGS_EQUAL(1, irc_protocol_is_numeric_command ("0"));
    LONGS_EQUAL(1, irc_protocol_is_numeric_command ("1"));
    LONGS_EQUAL(1, irc_protocol_is_numeric_command ("12"));
    LONGS_EQUAL(1, irc_protocol_is_numeric_command ("123"));
}

/*
 * Tests functions:
 *   irc_protocol_log_level_for_command
 */

TEST(IrcProtocol, LogLevelForCommand)
{
    LONGS_EQUAL(0, irc_protocol_log_level_for_command (NULL));
    LONGS_EQUAL(0, irc_protocol_log_level_for_command (""));

    LONGS_EQUAL(1, irc_protocol_log_level_for_command ("privmsg"));
    LONGS_EQUAL(1, irc_protocol_log_level_for_command ("notice"));

    LONGS_EQUAL(2, irc_protocol_log_level_for_command ("nick"));

    LONGS_EQUAL(4, irc_protocol_log_level_for_command ("join"));
    LONGS_EQUAL(4, irc_protocol_log_level_for_command ("part"));
    LONGS_EQUAL(4, irc_protocol_log_level_for_command ("quit"));
    LONGS_EQUAL(4, irc_protocol_log_level_for_command ("nick_back"));

    LONGS_EQUAL(3, irc_protocol_log_level_for_command ("001"));
    LONGS_EQUAL(3, irc_protocol_log_level_for_command ("away"));
    LONGS_EQUAL(3, irc_protocol_log_level_for_command ("kick"));
    LONGS_EQUAL(3, irc_protocol_log_level_for_command ("topic"));
}

/*
 * Tests functions:
 *   irc_protocol_tags
 */

TEST(IrcProtocol, Tags)
{
    struct t_hashtable *tags_empty, *tags_1, *tags_2;

    tags_empty = hashtable_new (32,
                                WEECHAT_HASHTABLE_STRING,
                                WEECHAT_HASHTABLE_STRING,
                                NULL, NULL);

    tags_1 = hashtable_new (32,
                            WEECHAT_HASHTABLE_STRING,
                            WEECHAT_HASHTABLE_STRING,
                            NULL, NULL);
    hashtable_set (tags_1, "key1", "value1");

    tags_2 = hashtable_new (32,
                            WEECHAT_HASHTABLE_STRING,
                            WEECHAT_HASHTABLE_STRING,
                            NULL, NULL);
    hashtable_set (tags_2, "key1", "value1");
    hashtable_set (tags_2, "key_2,comma", "value2,comma");

    POINTERS_EQUAL(NULL, irc_protocol_tags (NULL, NULL, NULL, NULL, NULL));

    /* command */
    STRCMP_EQUAL("irc_privmsg,log1",
                 irc_protocol_tags ("privmsg", NULL, NULL, NULL, NULL));
    STRCMP_EQUAL("irc_join,log4",
                 irc_protocol_tags ("join", NULL, NULL, NULL, NULL));

    /* command + irc_msg_tags */
    STRCMP_EQUAL("irc_privmsg,log1",
                 irc_protocol_tags ("privmsg", tags_empty, NULL, NULL, NULL));
    STRCMP_EQUAL("irc_join,log4",
                 irc_protocol_tags ("join", tags_empty, NULL, NULL, NULL));
    STRCMP_EQUAL("irc_privmsg,irc_tag_key1_value1,log1",
                 irc_protocol_tags ("privmsg", tags_1, NULL, NULL, NULL));
    STRCMP_EQUAL("irc_join,irc_tag_key1_value1,log4",
                 irc_protocol_tags ("join", tags_1, NULL, NULL, NULL));
    STRCMP_EQUAL("irc_privmsg,irc_tag_key1_value1,irc_tag_key-2;comma_value2;comma,log1",
                 irc_protocol_tags ("privmsg", tags_2, NULL, NULL, NULL));
    STRCMP_EQUAL("irc_join,irc_tag_key1_value1,irc_tag_key-2;comma_value2;comma,log4",
                 irc_protocol_tags ("join", tags_2, NULL, NULL, NULL));

    /* command + extra_tags */
    STRCMP_EQUAL("irc_privmsg,log1",
                 irc_protocol_tags ("privmsg", NULL, "", NULL, NULL));
    STRCMP_EQUAL("irc_join,log4",
                 irc_protocol_tags ("join", NULL, "", NULL, NULL));
    STRCMP_EQUAL("irc_privmsg,tag1,tag2,log1",
                 irc_protocol_tags ("privmsg", NULL, "tag1,tag2", NULL, NULL));
    STRCMP_EQUAL("irc_join,tag1,tag2,log4",
                 irc_protocol_tags ("join", NULL, "tag1,tag2", NULL, NULL));

    /* command + irc_msg_tags + extra_tags + nick */
    STRCMP_EQUAL("irc_privmsg,irc_tag_key1_value1,irc_tag_key-2;comma_value2;comma,"
                 "tag1,tag2,log1",
                 irc_protocol_tags ("privmsg", tags_2, "tag1,tag2", "", NULL));
    STRCMP_EQUAL("irc_join,irc_tag_key1_value1,irc_tag_key-2;comma_value2;comma,"
                 "tag1,tag2,log4",
                 irc_protocol_tags ("join", tags_2, "tag1,tag2", "", NULL));
    STRCMP_EQUAL("irc_privmsg,irc_tag_key1_value1,irc_tag_key-2;comma_value2;comma,"
                 "tag1,tag2,nick_alice,log1",
                 irc_protocol_tags ("privmsg", tags_2, "tag1,tag2", "alice", NULL));
    STRCMP_EQUAL("irc_join,irc_tag_key1_value1,irc_tag_key-2;comma_value2;comma,"
                 "tag1,tag2,nick_bob,log4",
                 irc_protocol_tags ("join", tags_2, "tag1,tag2", "bob", NULL));

    /* command + irc_msg_tags + extra_tags + nick + address */
    STRCMP_EQUAL("irc_privmsg,irc_tag_key1_value1,irc_tag_key-2;comma_value2;comma,"
                 "tag1,tag2,nick_alice,log1",
                 irc_protocol_tags ("privmsg", tags_2, "tag1,tag2", "alice", ""));
    STRCMP_EQUAL("irc_join,irc_tag_key1_value1,irc_tag_key-2;comma_value2;comma,"
                 "tag1,tag2,nick_bob,log4",
                 irc_protocol_tags ("join", tags_2, "tag1,tag2", "bob", ""));
    STRCMP_EQUAL("irc_privmsg,irc_tag_key1_value1,irc_tag_key-2;comma_value2;comma,"
                 "tag1,tag2,nick_alice,host_example.com,log1",
                 irc_protocol_tags ("privmsg", tags_2, "tag1,tag2", "alice",
                                    "example.com"));
    STRCMP_EQUAL("irc_join,irc_tag_key1_value1,irc_tag_key-2;comma_value2;comma,"
                 "tag1,tag2,nick_bob,host_example.com,log4",
                 irc_protocol_tags ("join", tags_2, "tag1,tag2", "bob",
                                    "example.com"));

    hashtable_free (tags_empty);
    hashtable_free (tags_1);
    hashtable_free (tags_2);
}

/*
 * Tests functions:
 *   irc_protocol_parse_time
 */

TEST(IrcProtocol, ParseTime)
{
    /* invalid time formats */
    LONGS_EQUAL(0, irc_protocol_parse_time (NULL));
    LONGS_EQUAL(0, irc_protocol_parse_time (""));
    LONGS_EQUAL(0, irc_protocol_parse_time ("invalid"));

    /* incomplete time formats */
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01"));
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01-13"));
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01-13T14"));
    LONGS_EQUAL(0, irc_protocol_parse_time ("2019-01-13T14:37"));

    /* valid time with ISO 8601 format*/
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("2019-01-13T13:38:19.123Z"));
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("2019-01-13T13:38:19.123"));
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("2019-01-13T13:38:19"));

    /* valid time as timestamp */
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("1547386699.123"));
    LONGS_EQUAL(1547386699, irc_protocol_parse_time ("1547386699"));
}

/*
 * Tests functions:
 *   irc_protocol_string_params
 */

TEST(IrcProtocol, StringParams)
{
    char *str;
    const char *params_empty[] = { "", NULL };
    const char *params_1[] = { "param1", NULL };
    const char *params_2[] = { "param1", "param2", NULL };
    const char *params_3[] = { "param1", "param2", "param3", NULL };

    /* invalid arguments */
    WEE_TEST_STR("", irc_protocol_string_params (NULL, -1, -1));
    WEE_TEST_STR("", irc_protocol_string_params (NULL, 0, 0));
    WEE_TEST_STR("", irc_protocol_string_params (NULL, 0, -1));
    WEE_TEST_STR("", irc_protocol_string_params (params_1, 1, 0));

    /* empty parameter */
    WEE_TEST_STR("", irc_protocol_string_params (params_empty, 0, 0));
    WEE_TEST_STR("", irc_protocol_string_params (params_empty, 1, 1));

    /* one parameter */
    WEE_TEST_STR("param1", irc_protocol_string_params (params_1, 0, 0));
    WEE_TEST_STR("", irc_protocol_string_params (params_1, 1, 1));

    /* two parameters */
    WEE_TEST_STR("param1 param2", irc_protocol_string_params (params_2, 0, 1));
    WEE_TEST_STR("param2", irc_protocol_string_params (params_2, 1, 1));
    WEE_TEST_STR("", irc_protocol_string_params (params_2, 2, 2));

    /* three parameters */
    WEE_TEST_STR("param1 param2 param3", irc_protocol_string_params (params_3, 0, 2));
    WEE_TEST_STR("param2 param3", irc_protocol_string_params (params_3, 1, 2));
    WEE_TEST_STR("param2", irc_protocol_string_params (params_3, 1, 1));
    WEE_TEST_STR("param3", irc_protocol_string_params (params_3, 2, 2));
    WEE_TEST_STR("", irc_protocol_string_params (params_3, 3, 3));
}

TEST_GROUP(IrcProtocolWithServer)
{
    void server_recv (const char *command)
    {
        char str_command[4096];

        record_start ();
        arraylist_clear (sent_messages);

        snprintf (str_command, sizeof (str_command),
                  "/command -buffer irc.server." IRC_FAKE_SERVER " irc "
                  "/server fakerecv \"%s\"",
                  command);
        run_cmd_quiet (str_command);

        record_stop ();
    }

    char **server_build_error (const char *msg1, const char *message,
                               const char *msg2)
    {
        char **msg;

        msg = string_dyn_alloc (1024);
        string_dyn_concat (msg, msg1, -1);
        if (message)
        {
            string_dyn_concat (msg, ": \"", -1);
            string_dyn_concat (msg, message, -1);
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

    static int signal_irc_out_cb (const void *pointer, void *data,
                                  const char *signal, const char *type_data,
                                  void *signal_data)
    {
        /* make C++ compiler happy */
        (void) pointer;
        (void) data;
        (void) signal;
        (void) type_data;

        if (signal_data)
            arraylist_add (sent_messages, strdup ((const char *)signal_data));

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

    void sent_msg_dump (char **msg)
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
        /* initialize list of messages sent to the server */
        if (sent_messages)
        {
            arraylist_clear (sent_messages);
        }
        else
        {
            sent_messages = arraylist_new (16, 0, 1,
                                           &sent_msg_cmp_cb, NULL,
                                           &sent_msg_free_cb, NULL);
        }

        if (!hook_signal_irc_out)
        {
            hook_signal_irc_out = hook_signal (NULL,
                                               IRC_FAKE_SERVER ",irc_out1_*",
                                               &signal_irc_out_cb, NULL, NULL);
        }

        /* create a fake server (no I/O) */
        run_cmd_quiet ("/mute /server add " IRC_FAKE_SERVER " fake:127.0.0.1 "
                       "-nicks=nick1,nick2,nick3");

        /* connect to the fake server */
        run_cmd_quiet ("/connect " IRC_FAKE_SERVER);

        /* get the server pointer */
        ptr_server = irc_server_search (IRC_FAKE_SERVER);
    }

    void teardown ()
    {
        /* disconnect and delete the fake server */
        run_cmd_quiet ("/mute /disconnect " IRC_FAKE_SERVER);
        run_cmd_quiet ("/mute /server del " IRC_FAKE_SERVER);
        ptr_server = NULL;
    }
};

/*
 * Tests functions:
 *   irc_protocol_nick_address
 */

TEST(IrcProtocolWithServer, NickAddress)
{
    struct t_irc_nick *ptr_nick;
    char result[1024];

    SRV_INIT_JOIN;

    ptr_nick = ptr_server->channels->nicks;

    STRCMP_EQUAL("", irc_protocol_nick_address (NULL, 0, NULL,
                                                NULL, NULL));
    STRCMP_EQUAL("", irc_protocol_nick_address (ptr_server, 0, NULL,
                                                NULL, NULL));
    STRCMP_EQUAL("", irc_protocol_nick_address (ptr_server, 0, ptr_nick,
                                                NULL, NULL));
    STRCMP_EQUAL("", irc_protocol_nick_address (ptr_server, 0, ptr_nick,
                                                NULL, NULL));

    snprintf (result, sizeof (result),
              "%s%s%s",
              ptr_nick->color,
              "alice",
              gui_color_get_custom ("reset"));
    STRCMP_EQUAL(result, irc_protocol_nick_address (ptr_server, 0, ptr_nick,
                                                    "alice", NULL));

    snprintf (result, sizeof (result),
              "%s%s %s(%s%s%s)%s",
              ptr_nick->color,
              "alice",
              gui_color_search_config ("chat_delimiters"),
              gui_color_search_config ("chat_host"),
              "example.com",
              gui_color_search_config ("chat_delimiters"),
              gui_color_get_custom ("reset"));
    STRCMP_EQUAL(result, irc_protocol_nick_address (ptr_server, 0, ptr_nick,
                                                    "alice", "example.com"));

    config_file_option_set (irc_config_look_color_nicks_in_server_messages,
                            "off", 1);
    snprintf (result, sizeof (result),
              "%s%s %s(%s%s%s)%s",
              ptr_nick->color,
              "alice",
              gui_color_search_config ("chat_delimiters"),
              gui_color_search_config ("chat_host"),
              "example.com",
              gui_color_search_config ("chat_delimiters"),
              gui_color_get_custom ("reset"));
    STRCMP_EQUAL(result, irc_protocol_nick_address (ptr_server, 0, ptr_nick,
                                                    "alice", "example.com"));
    snprintf (result, sizeof (result),
              "%s%s %s(%s%s%s)%s",
              gui_color_search_config ("chat_nick"),
              "alice",
              gui_color_search_config ("chat_delimiters"),
              gui_color_search_config ("chat_host"),
              "example.com",
              gui_color_search_config ("chat_delimiters"),
              gui_color_get_custom ("reset"));
    STRCMP_EQUAL(result, irc_protocol_nick_address (ptr_server, 1, ptr_nick,
                                                    "alice", "example.com"));
    config_file_option_reset (irc_config_look_color_nicks_in_server_messages, 1);
}

/*
 * Tests functions:
 *   irc_protocol_recv_command (command not found)
 */

TEST(IrcProtocolWithServer, recv_command_not_found)
{
    SRV_INIT;

    RECV(":alice!user@host XYZ");
    CHECK_SRV("=!= irc: command \"XYZ\" not found: \":alice!user@host XYZ\"");

    RECV(":alice!user@host XYZ abc def");
    CHECK_SRV("=!= irc: command \"XYZ\" not found: \":alice!user@host XYZ "
              "abc def\"");

    RECV(":alice!user@host 099");
    CHECK_ERROR_PARAMS("099", 0, 1);

    RECV(":alice!user@host 099 abc def");
    CHECK_SRV("-- abc def");
}

/*
 * Tests functions:
 *   irc_protocol_recv_command (invalid message)
 */

TEST(IrcProtocolWithServer, recv_command_invalid_message)
{
    SRV_INIT;

    RECV(":");
    CHECK_NO_MSG;

    RECV("abc");
    CHECK_SRV("=!= irc: command \"abc\" not found: \"abc\"");

    RECV(":alice!user@host");
    CHECK_NO_MSG;

    RECV("@");
    CHECK_SRV("=!= irc: command \"@\" not found: \"@\"");

    RECV("@test");
    CHECK_SRV("=!= irc: command \"@test\" not found: \"@test\"");

    RECV("@test :");
    CHECK_NO_MSG;

    RECV("@test :abc");
    CHECK_NO_MSG;
}

/*
 * Tests functions:
 *   irc_protocol_cb_account (without account-notify capability)
 */

TEST(IrcProtocolWithServer, account_without_account_notify_cap)
{
    struct t_irc_nick *ptr_nick;

    SRV_INIT_JOIN2;

    RECV(":bob!user@host PRIVMSG alice :hi Alice!");

    ptr_nick = ptr_server->channels->last_nick;

    POINTERS_EQUAL(NULL, ptr_nick->account);

    /* not enough parameters */
    RECV(":bob!user@host ACCOUNT");
    CHECK_ERROR_PARAMS("account", 0, 1);

    POINTERS_EQUAL(NULL, ptr_nick->account);

    RECV(":bob!user@host ACCOUNT *");
    CHECK_CHAN("-- bob has unidentified");
    CHECK_PV("bob", "-- bob has unidentified");
    POINTERS_EQUAL(NULL, ptr_nick->account);

    RECV(":bob!user@host ACCOUNT :*");
    CHECK_CHAN("-- bob has unidentified");
    CHECK_PV("bob", "-- bob has unidentified");
    POINTERS_EQUAL(NULL, ptr_nick->account);

    RECV(":bob!user@host ACCOUNT new_account");
    CHECK_CHAN("-- bob has identified as new_account");
    CHECK_PV("bob", "-- bob has identified as new_account");
    POINTERS_EQUAL(NULL, ptr_nick->account);

    RECV(":bob!user@host ACCOUNT :new_account");
    CHECK_CHAN("-- bob has identified as new_account");
    CHECK_PV("bob", "-- bob has identified as new_account");
    POINTERS_EQUAL(NULL, ptr_nick->account);
}

/*
 * Tests functions:
 *   irc_protocol_cb_account (with account-notify capability)
 */

TEST(IrcProtocolWithServer, account_with_account_notify_cap)
{
    struct t_irc_nick *ptr_nick;

    /* assume "account-notify" capability is enabled in server */
    hashtable_set (ptr_server->cap_list, "account-notify", NULL);

    SRV_INIT_JOIN;

    ptr_nick = ptr_server->channels->nicks;

    POINTERS_EQUAL(NULL, ptr_nick->account);

    RECV(":alice!user@host ACCOUNT new_account");
    CHECK_CHAN("-- alice has identified as new_account");
    STRCMP_EQUAL("new_account", ptr_nick->account);

    RECV(":alice!user@host ACCOUNT :new_account2");
    CHECK_CHAN("-- alice has identified as new_account2");
    STRCMP_EQUAL("new_account2", ptr_nick->account);

    RECV(":alice!user@host ACCOUNT *");
    CHECK_CHAN("-- alice has unidentified");
    POINTERS_EQUAL(NULL, ptr_nick->account);

    RECV(":alice!user@host ACCOUNT :new_account3");
    CHECK_CHAN("-- alice has identified as new_account3");
    STRCMP_EQUAL("new_account3", ptr_nick->account);

    RECV(":alice!user@host ACCOUNT :*");
    CHECK_CHAN("-- alice has unidentified");
    POINTERS_EQUAL(NULL, ptr_nick->account);
}

/*
 * Tests functions:
 *   irc_protocol_cb_authenticate
 */

TEST(IrcProtocolWithServer, authenticate)
{
    SRV_INIT;

    /* not enough parameters */
    RECV("AUTHENTICATE");
    CHECK_ERROR_PARAMS("authenticate", 0, 1);
    RECV(":server.address AUTHENTICATE");
    CHECK_ERROR_PARAMS("authenticate", 0, 1);

    RECV("AUTHENTICATE "
         "QQDaUzXAmVffxuzFy77XWBGwABBQAgdinelBrKZaR3wE7nsIETuTVY=");
    CHECK_NO_MSG;
    RECV(":server.address AUTHENTICATE "
         "QQDaUzXAmVffxuzFy77XWBGwABBQAgdinelBrKZaR3wE7nsIETuTVY=");
    CHECK_NO_MSG;
}

/*
 * Tests functions:
 *   irc_protocol_cb_away
 */

TEST(IrcProtocolWithServer, away)
{
    struct t_irc_nick *ptr_nick;

    SRV_INIT_JOIN;

    /* missing nick */
    RECV("AWAY");
    CHECK_ERROR_NICK("away");

    ptr_nick = ptr_server->channels->nicks;

    LONGS_EQUAL(0, ptr_nick->away);

    RECV(":alice!user@host AWAY Bye");
    CHECK_NO_MSG;
    LONGS_EQUAL(1, ptr_nick->away);

    RECV(":alice!user@host AWAY :Holidays now!");
    CHECK_NO_MSG;
    LONGS_EQUAL(1, ptr_nick->away);

    RECV(":alice!user@host AWAY");
    CHECK_NO_MSG;
    LONGS_EQUAL(0, ptr_nick->away);
}

/*
 * Tests functions:
 *   irc_protocol_cap_to_enable
 */

TEST(IrcProtocol, cap_to_enable)
{
    char *str;

    WEE_CHECK_CAP_TO_ENABLE("", NULL, 0);
    WEE_CHECK_CAP_TO_ENABLE("", "", 0);
    WEE_CHECK_CAP_TO_ENABLE("extended-join", "extended-join", 0);
    WEE_CHECK_CAP_TO_ENABLE("extended-join,sasl", "extended-join", 1);
    WEE_CHECK_CAP_TO_ENABLE(IRC_ALL_CAPS, "*", 0);
    WEE_CHECK_CAP_TO_ENABLE(IRC_ALL_CAPS ",sasl", "*", 1);
    WEE_CHECK_CAP_TO_ENABLE(IRC_ALL_CAPS ",!away-notify,!extended-join,sasl",
                            "*,!away-notify,!extended-join", 1);
}

/*
 * Tests functions:
 *   irc_protocol_cb_cap
 */

TEST(IrcProtocolWithServer, cap)
{
    SRV_INIT;

    /* not enough parameters */
    RECV("CAP");
    CHECK_ERROR_PARAMS("cap", 0, 2);
    RECV("CAP *");
    CHECK_ERROR_PARAMS("cap", 1, 2);
    RECV(":server CAP");
    CHECK_ERROR_PARAMS("cap", 0, 2);
    RECV(":server CAP *");
    CHECK_ERROR_PARAMS("cap", 1, 2);

    /* CAP LS */
    RECV("CAP * LS :multi-prefix sasl");
    CHECK_SRV("-- irc: client capability, server supports: multi-prefix sasl");
    RECV("CAP * LS * :multi-prefix sasl");
    CHECK_NO_MSG;
    RECV(":server CAP * LS :multi-prefix sasl");
    CHECK_SRV("-- irc: client capability, server supports: multi-prefix sasl");
    RECV(":server CAP * LS * :multi-prefix sasl");
    CHECK_NO_MSG;

    /* CAP LIST */
    RECV("CAP * LIST :multi-prefix sasl");
    CHECK_SRV("-- irc: client capability, currently enabled: multi-prefix sasl");
    RECV("CAP * LIST * :multi-prefix sasl");
    CHECK_NO_MSG;
    RECV(":server CAP * LIST :multi-prefix sasl");
    CHECK_SRV("-- irc: client capability, currently enabled: multi-prefix sasl");
    RECV(":server CAP * LIST * :multi-prefix sasl");
    CHECK_NO_MSG;

    /* CAP NEW */
    RECV("CAP * NEW :multi-prefix sasl");
    CHECK_SRV("-- irc: client capability, now available: multi-prefix sasl");
    RECV(":server CAP * NEW :multi-prefix sasl");
    CHECK_SRV("-- irc: client capability, now available: multi-prefix sasl");

    /* CAP DEL */
    RECV("CAP * DEL :multi-prefix sasl");
    CHECK_SRV("-- irc: client capability, removed: multi-prefix sasl");
    RECV(":server CAP * DEL :multi-prefix sasl");
    CHECK_SRV("-- irc: client capability, removed: multi-prefix sasl");

    /* CAP ACK */
    RECV("CAP * ACK :sasl");
    CHECK_SRV("-- irc: client capability, enabled: sasl");
    RECV(":server CAP * ACK :sasl");
    CHECK_SRV("-- irc: client capability, enabled: sasl");

    /* CAP NAK */
    RECV("CAP * NAK :sasl");
    CHECK_SRV("=!= irc: client capability, refused: sasl");
    RECV(":server CAP * NAK :sasl");
    CHECK_SRV("=!= irc: client capability, refused: sasl");
}

/*
 * Tests functions:
 *   irc_protocol_cb_chghost
 */

TEST(IrcProtocolWithServer, chghost)
{
    struct t_irc_nick *ptr_nick, *ptr_nick2;

    SRV_INIT_JOIN2;

    RECV(":bob!user@host PRIVMSG alice :hi Alice!");

    ptr_nick = ptr_server->channels->nicks;
    ptr_nick2 = ptr_server->channels->last_nick;

    STRCMP_EQUAL("user_a@host_a", ptr_nick->host);

    /* not enough parameters */
    RECV(":alice!user@host CHGHOST");
    CHECK_ERROR_PARAMS("chghost", 0, 2);
    RECV(":alice!user@host CHGHOST user2");
    CHECK_ERROR_PARAMS("chghost", 1, 2);

    /* missing nick */
    RECV("CHGHOST user2 host2");
    CHECK_ERROR_NICK("chghost");

    STRCMP_EQUAL("user_a@host_a", ptr_nick->host);

    /* self nick */
    RECV(":alice!user@host CHGHOST user2 host2");
    CHECK_CHAN("-- alice (user@host) has changed host to user2@host2");
    STRCMP_EQUAL("user2@host2", ptr_nick->host);

    RECV(":alice!user@host CHGHOST user2 host2");
    CHECK_CHAN("-- alice (user@host) has changed host to user2@host2");
    STRCMP_EQUAL("user2@host2", ptr_nick->host);

    RECV(":alice!user2@host2 CHGHOST user3 :host3");
    CHECK_CHAN("-- alice (user2@host2) has changed host to user3@host3");
    STRCMP_EQUAL("user3@host3", ptr_nick->host);

    /* another nick */
    RECV(":bob!user@host CHGHOST user_bob_2 host_bob_2");
    CHECK_CHAN("-- bob (user@host) has changed host to user_bob_2@host_bob_2");
    STRCMP_EQUAL("user_bob_2@host_bob_2", ptr_nick2->host);
    CHECK_PV("bob", "-- bob (user@host) has changed host to user_bob_2@host_bob_2");
}

/*
 * Tests functions:
 *   irc_protocol_cb_error
 */

TEST(IrcProtocolWithServer, error)
{
    SRV_INIT;

    /* not enough parameters */
    RECV("ERROR");
    CHECK_ERROR_PARAMS("error", 0, 1);

    RECV("ERROR test");
    CHECK_SRV("=!= test");
    RECV("ERROR :Closing Link: irc.server.org (Bad Password)");
    CHECK_SRV("=!= Closing Link: irc.server.org (Bad Password)");
}

/*
 * Tests functions:
 *   irc_protocol_cb_fail
 */

TEST(IrcProtocolWithServer, fail)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server FAIL");
    CHECK_ERROR_PARAMS("fail", 0, 2);
    RECV(":server FAIL *");
    CHECK_ERROR_PARAMS("fail", 1, 2);
    RECV(":server FAIL COMMAND");
    CHECK_ERROR_PARAMS("fail", 1, 2);

    RECV(":server FAIL * TEST");
    CHECK_SRV("=!= Failure: [] TEST");
    RECV(":server FAIL * TEST :the message");
    CHECK_SRV("=!= Failure: [TEST] the message");
    RECV(":server FAIL * TEST TEST2");
    CHECK_SRV("=!= Failure: [TEST] TEST2");
    RECV(":server FAIL * TEST TEST2 :the message");
    CHECK_SRV("=!= Failure: [TEST TEST2] the message");

    RECV(":server FAIL COMMAND TEST");
    CHECK_SRV("=!= Failure: COMMAND [] TEST");
    RECV(":server FAIL COMMAND TEST :the message");
    CHECK_SRV("=!= Failure: COMMAND [TEST] the message");
    RECV(":server FAIL COMMAND TEST TEST2");
    CHECK_SRV("=!= Failure: COMMAND [TEST] TEST2");
    RECV(":server FAIL COMMAND TEST TEST2 :the message");
    CHECK_SRV("=!= Failure: COMMAND [TEST TEST2] the message");
}

/*
 * Tests functions:
 *   irc_protocol_cb_invite
 */

TEST(IrcProtocolWithServer, invite)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":bob!user@host INVITE");
    CHECK_ERROR_PARAMS("invite", 0, 2);
    RECV(":bob!user@host INVITE alice");
    CHECK_ERROR_PARAMS("invite", 1, 2);

    /* missing nick */
    RECV("INVITE alice #channel");
    CHECK_ERROR_NICK("invite");

    RECV(":bob!user@host INVITE alice #channel");
    CHECK_SRV("-- You have been invited to #channel by bob");
    RECV(":bob!user@host INVITE xxx #channel");
    CHECK_SRV("-- bob has invited xxx to #channel");
}

/*
 * Tests functions:
 *   irc_protocol_cb_join
 */

TEST(IrcProtocolWithServer, join)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    SRV_INIT;

    POINTERS_EQUAL(NULL, ptr_server->channels);

    /* not enough parameters */
    RECV(":alice!user@host JOIN");
    CHECK_ERROR_PARAMS("join", 0, 1);

    /* missing nick */
    RECV("JOIN #test");
    CHECK_ERROR_NICK("join");

    POINTERS_EQUAL(NULL, ptr_server->channels);

    /* join of a user while the channel does not yet exist in local */
    RECV(":bob!user@host JOIN #test");
    CHECK_NO_MSG;

    RECV(":alice!user@host JOIN #test");
    CHECK_CHAN("--> alice (user@host) has joined #test");

    ptr_channel = ptr_server->channels;

    CHECK(ptr_channel);
    CHECK(ptr_channel == ptr_server->last_channel);

    LONGS_EQUAL(IRC_CHANNEL_TYPE_CHANNEL, ptr_channel->type);
    STRCMP_EQUAL("#test", ptr_channel->name);
    POINTERS_EQUAL(NULL, ptr_channel->topic);
    POINTERS_EQUAL(NULL, ptr_channel->modes);
    LONGS_EQUAL(0, ptr_channel->limit);
    POINTERS_EQUAL(NULL, ptr_channel->key);
    LONGS_EQUAL(0, ptr_channel->checking_whox);
    POINTERS_EQUAL(NULL, ptr_channel->away_message);
    LONGS_EQUAL(0, ptr_channel->has_quit_server);
    LONGS_EQUAL(0, ptr_channel->cycle);
    LONGS_EQUAL(0, ptr_channel->part);
    LONGS_EQUAL(0, ptr_channel->part);
    POINTERS_EQUAL(NULL, ptr_channel->pv_remote_nick_color);
    POINTERS_EQUAL(NULL, ptr_channel->hook_autorejoin);

    ptr_nick = ptr_channel->nicks;

    LONGS_EQUAL(1, ptr_channel->nicks_count);
    CHECK(ptr_nick);
    CHECK(ptr_nick == ptr_channel->last_nick);
    STRCMP_EQUAL("alice", ptr_nick->name);
    STRCMP_EQUAL("user@host", ptr_nick->host);
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);
    LONGS_EQUAL(0, ptr_nick->away);
    POINTERS_EQUAL(NULL, ptr_nick->account);
    POINTERS_EQUAL(NULL, ptr_nick->realname);
    CHECK(ptr_nick->color);

    CHECK(ptr_channel->buffer);

    RECV(":bob!user@host JOIN #test * :Bob Name");
    CHECK_CHAN("--> bob (Bob Name) (user@host) has joined #test");

    ptr_nick = ptr_channel->last_nick;

    LONGS_EQUAL(2, ptr_channel->nicks_count);
    CHECK(ptr_nick);
    STRCMP_EQUAL("bob", ptr_nick->name);
    STRCMP_EQUAL("user@host", ptr_nick->host);
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);
    LONGS_EQUAL(0, ptr_nick->away);
    POINTERS_EQUAL(NULL, ptr_nick->account);
    STRCMP_EQUAL("Bob Name", ptr_nick->realname);
    CHECK(ptr_nick->color);

    RECV(":carol!user@host JOIN #test carol_account :Carol Name");
    CHECK_CHAN("--> carol [carol_account] (Carol Name) (user@host) "
               "has joined #test");

    ptr_nick = ptr_channel->last_nick;

    LONGS_EQUAL(3, ptr_channel->nicks_count);
    CHECK(ptr_nick);
    STRCMP_EQUAL("carol", ptr_nick->name);
    STRCMP_EQUAL("user@host", ptr_nick->host);
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);
    LONGS_EQUAL(0, ptr_nick->away);
    STRCMP_EQUAL("carol_account", ptr_nick->account);
    STRCMP_EQUAL("Carol Name", ptr_nick->realname);
    CHECK(ptr_nick->color);
}

/*
 * Tests functions:
 *   irc_protocol_cb_kick
 */

TEST(IrcProtocolWithServer, kick)
{
    struct t_irc_channel *ptr_channel;

    SRV_INIT;

    POINTERS_EQUAL(NULL, ptr_server->channels);

    RECV(":alice!user@host JOIN #test");
    CHECK_CHAN("--> alice (user@host) has joined #test");
    RECV(":bob!user@host JOIN #test");
    CHECK_CHAN("--> bob (user@host) has joined #test");

    ptr_channel = ptr_server->channels;

    CHECK(ptr_channel);
    CHECK(ptr_channel->nicks);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    CHECK(ptr_channel->nicks->next_nick);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);

    /* not enough parameters */
    RECV(":alice!user@host KICK");
    CHECK_ERROR_PARAMS("kick", 0, 2);
    RECV(":alice!user@host KICK #test");
    CHECK_ERROR_PARAMS("kick", 1, 2);

    /* missing nick */
    RECV("KICK #test bob");
    CHECK_ERROR_NICK("kick");

    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);

    /* channel not found */
    RECV(":alice!user@host KICK #xyz bob :the reason");
    CHECK_NO_MSG;

    /* kick without a reason */
    RECV(":alice!user@host KICK #test bob");
    CHECK_CHAN("<-- alice has kicked bob");
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    RECV(":bob!user@host JOIN #test");
    CHECK_CHAN("--> bob (user@host) has joined #test");

    /* with kick a reason */
    RECV(":alice!user@host KICK #test bob :no spam here! ");
    CHECK_CHAN("<-- alice has kicked bob (no spam here! )");
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    RECV(":bob!user@host JOIN #test");
    CHECK_CHAN("--> bob (user@host) has joined #test");

    /* kick of self nick */
    RECV(":bob!user@host KICK #test alice :no spam here! ");
    CHECK_CHAN("<-- bob has kicked alice (no spam here! )");
    POINTERS_EQUAL(NULL, ptr_channel->nicks);
}

/*
 * Tests functions:
 *   irc_protocol_cb_kill
 */

TEST(IrcProtocolWithServer, kill)
{
    struct t_irc_channel *ptr_channel;

    SRV_INIT_JOIN2;

    ptr_channel = ptr_server->channels;

    CHECK(ptr_channel);
    CHECK(ptr_channel->nicks);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    CHECK(ptr_channel->nicks->next_nick);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);

    /* not enough parameters */
    RECV(":alice!user@host KILL");
    CHECK_ERROR_PARAMS("kill", 0, 1);

    /* missing nick */
    RECV("KILL alice");
    CHECK_ERROR_NICK("kill");

    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);

    /* kill without a reason */
    RECV(":bob!user@host KILL alice");
    CHECK_CHAN("<-- You were killed by bob");
    POINTERS_EQUAL(NULL, ptr_channel->nicks);

    RECV(":alice!user@host JOIN #test");
    CHECK_CHAN("--> alice (user@host) has joined #test");
    RECV(":bob!user@host JOIN #test");
    CHECK_CHAN("--> bob (user@host) has joined #test");

    /* kill with a reason */
    RECV(":bob!user@host KILL alice :killed by admin ");
    CHECK_CHAN("<-- You were killed by bob (killed by admin )");
    POINTERS_EQUAL(NULL, ptr_channel->nicks);
}

/*
 * Tests functions:
 *   irc_protocol_cb_mode
 */

TEST(IrcProtocolWithServer, mode)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    SRV_INIT_JOIN;

    ptr_channel = ptr_server->channels;
    CHECK(ptr_channel);
    POINTERS_EQUAL(NULL, ptr_channel->modes);
    ptr_nick = ptr_channel->nicks;
    CHECK(ptr_nick);
    STRCMP_EQUAL("alice", ptr_nick->name);
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);

    /* not enough parameters */
    RECV(":admin MODE");
    CHECK_ERROR_PARAMS("mode", 0, 2);
    RECV(":admin MODE #test");
    CHECK_ERROR_PARAMS("mode", 1, 2);

    /* missing nick */
    RECV("MODE #test +nt");
    CHECK_ERROR_NICK("mode");

    POINTERS_EQUAL(NULL, ptr_channel->modes);

    /* channel mode */
    RECV(":admin MODE #test +nt");
    CHECK_CHAN("-- Mode #test [+nt] by admin");
    STRCMP_EQUAL("+tn", ptr_channel->modes);

    /* channel mode removed */
    RECV(":admin MODE #test -n");
    CHECK_CHAN("-- Mode #test [-n] by admin");
    STRCMP_EQUAL("+t", ptr_channel->modes);

    /* channel mode removed */
    RECV(":admin MODE #test -t");
    CHECK_CHAN("-- Mode #test [-t] by admin");
    POINTERS_EQUAL(NULL, ptr_channel->modes);

    /* nick mode '@' on channel #test */
    RECV(":admin MODE #test +o alice");
    CHECK_CHAN("-- Mode #test [+o alice] by admin");
    STRCMP_EQUAL("@ ", ptr_nick->prefixes);
    STRCMP_EQUAL("@", ptr_nick->prefix);

    /* another nick mode '+' on channel #test */
    RECV(":admin MODE #test +v alice");
    CHECK_CHAN("-- Mode #test [+v alice] by admin");
    STRCMP_EQUAL("@+", ptr_nick->prefixes);
    STRCMP_EQUAL("@", ptr_nick->prefix);

    /* nick mode '@' removed on channel #test */
    RECV(":admin MODE #test -o alice");
    CHECK_CHAN("-- Mode #test [-o alice] by admin");
    STRCMP_EQUAL(" +", ptr_nick->prefixes);
    STRCMP_EQUAL("+", ptr_nick->prefix);

    /* nick mode '+' removed on channel #test */
    RECV(":admin MODE #test -v alice");
    CHECK_CHAN("-- Mode #test [-v alice] by admin");
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);

    /* nick mode 'i' */
    POINTERS_EQUAL(NULL, ptr_server->nick_modes);
    RECV(":admin MODE alice +i");
    CHECK_SRV("-- User mode [+i] by admin");
    STRCMP_EQUAL("i", ptr_server->nick_modes);

    /* nick mode 'R' */
    RECV(":admin MODE alice +R");
    CHECK_SRV("-- User mode [+R] by admin");
    STRCMP_EQUAL("iR", ptr_server->nick_modes);

    /* remove nick mode 'i' */
    RECV(":admin MODE alice -i");
    CHECK_SRV("-- User mode [-i] by admin");
    STRCMP_EQUAL("R", ptr_server->nick_modes);
}

/*
 * Tests functions:
 *   irc_protocol_cb_nick
 */

TEST(IrcProtocolWithServer, nick)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick1, *ptr_nick2;

    SRV_INIT_JOIN2;

    RECV(":bob!user@host PRIVMSG alice :hi Alice!");
    CHECK_PV("bob", "bob hi Alice!");

    ptr_channel = ptr_server->channels;
    CHECK(ptr_channel);
    ptr_nick1 = ptr_channel->nicks;
    CHECK(ptr_nick1);
    ptr_nick2 = ptr_nick1->next_nick;
    CHECK(ptr_nick2);
    STRCMP_EQUAL("alice", ptr_nick1->name);
    STRCMP_EQUAL("bob", ptr_nick2->name);

    /* not enough parameters */
    RECV(":alice!user@host NICK");
    CHECK_ERROR_PARAMS("nick", 0, 1);
    STRCMP_EQUAL("alice", ptr_nick1->name);
    STRCMP_EQUAL("bob", ptr_nick2->name);

    /* missing nick */
    RECV("NICK alice_away");
    CHECK_ERROR_NICK("nick");

    /* new nick for alice */
    RECV(":alice!user@host NICK alice_away");
    CHECK_SRV("-- You are now known as alice_away");
    CHECK_CHAN("-- You are now known as alice_away");
    STRCMP_EQUAL("alice_away", ptr_nick1->name);

    /* new nick for alice_away (with ":") */
    RECV(":alice_away!user@host NICK :alice2");
    CHECK_SRV("-- You are now known as alice2");
    CHECK_CHAN("-- You are now known as alice2");
    STRCMP_EQUAL("alice2", ptr_nick1->name);

    /* new nick for bob */
    RECV(":bob!user@host NICK bob_away");
    CHECK_CHAN("-- bob is now known as bob_away");
    CHECK_PV("bob_away", "-- bob is now known as bob_away");
    STRCMP_EQUAL("bob_away", ptr_nick2->name);

    /* new nick for bob_away (with ":") */
    RECV(":bob_away!user@host NICK :bob2");
    CHECK_CHAN("-- bob_away is now known as bob2");
    CHECK_PV("bob2", "-- bob_away is now known as bob2");
    STRCMP_EQUAL("bob2", ptr_nick2->name);

    STRCMP_EQUAL("bob2", ptr_server->last_channel->name);
}

/*
 * Tests functions:
 *   irc_protocol_cb_note
 */

TEST(IrcProtocolWithServer, note)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server NOTE");
    CHECK_ERROR_PARAMS("note", 0, 2);
    RECV(":server NOTE *");
    CHECK_ERROR_PARAMS("note", 1, 2);
    RECV(":server NOTE COMMAND");
    CHECK_ERROR_PARAMS("note", 1, 2);

    RECV(":server NOTE * TEST");
    CHECK_SRV("-- Note: [] TEST");
    RECV(":server NOTE * TEST :the message");
    CHECK_SRV("-- Note: [TEST] the message");
    RECV(":server NOTE * TEST TEST2");
    CHECK_SRV("-- Note: [TEST] TEST2");
    RECV(":server NOTE * TEST TEST2 :the message");
    CHECK_SRV("-- Note: [TEST TEST2] the message");

    RECV(":server NOTE COMMAND TEST");
    CHECK_SRV("-- Note: COMMAND [] TEST");
    RECV(":server NOTE COMMAND TEST :the message");
    CHECK_SRV("-- Note: COMMAND [TEST] the message");
    RECV(":server NOTE COMMAND TEST TEST2");
    CHECK_SRV("-- Note: COMMAND [TEST] TEST2");
    RECV(":server NOTE COMMAND TEST TEST2 :the message");
    CHECK_SRV("-- Note: COMMAND [TEST TEST2] the message");
}

/*
 * Tests functions:
 *   irc_protocol_cb_notice
 */

TEST(IrcProtocolWithServer, notice)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV("NOTICE");
    CHECK_ERROR_PARAMS("notice", 0, 2);
    RECV("NOTICE AUTH");
    CHECK_ERROR_PARAMS("notice", 1, 2);
    RECV(":bob!user@host NOTICE");
    CHECK_ERROR_PARAMS("notice", 0, 2);
    RECV(":bob!user@host NOTICE #test");
    CHECK_ERROR_PARSE("notice", ":bob!user@host NOTICE #test");
    RECV(":bob!user@host NOTICE alice");
    CHECK_ERROR_PARSE("notice", ":bob!user@host NOTICE alice");

    /* notice from server */
    RECV("NOTICE AUTH :*** Looking up your hostname... ");
    CHECK_SRV("-- *** Looking up your hostname... ");
    RECV(":server.address NOTICE AUTH :*** Looking up your hostname... ");
    CHECK_SRV("-- server.address: *** Looking up your hostname... ");
    RECV(":server.address NOTICE * :*** Looking up your hostname... ");
    CHECK_SRV("-- server.address: *** Looking up your hostname... ");

    /* notice to channel/user */
    RECV(":server.address NOTICE #test :a notice ");
    CHECK_CHAN("-- Notice(server.address): a notice ");
    RECV(":server.address NOTICE alice :a notice ");
    CHECK_SRV("-- server.address: a notice ");
    RECV(":bob!user@host NOTICE #test :a notice ");
    CHECK_CHAN("-- Notice(bob): a notice ");
    RECV(":bob!user@host NOTICE alice :a notice ");
    CHECK_SRV("-- bob (user@host): a notice ");

    /* notice to ops of channel */
    RECV(":server.address NOTICE @#test :a notice ");
    CHECK_CHAN("-- Notice:@(server.address): a notice ");
    RECV(":bob!user@host NOTICE @#test :a notice ");
    CHECK_CHAN("-- Notice:@(bob): a notice ");

    /* notice from self nick (case of bouncer) */
    RECV(":alice!user@host NOTICE alice :a notice ");
    CHECK_SRV("-- Notice -> alice: a notice ");

    /* notice with channel name at beginning */
    RECV(":server.address NOTICE alice :[#test] a notice ");
    CHECK_CHAN("-- PvNotice(server.address): a notice ");
    RECV(":server.address NOTICE alice :(#test) a notice ");
    CHECK_CHAN("-- PvNotice(server.address): a notice ");
    RECV(":server.address NOTICE alice :{#test} a notice ");
    CHECK_CHAN("-- PvNotice(server.address): a notice ");
    RECV(":server.address NOTICE alice :<#test> a notice ");
    CHECK_CHAN("-- PvNotice(server.address): a notice ");
    RECV(":bob!user@host NOTICE alice :[#test] a notice ");
    CHECK_CHAN("-- PvNotice(bob): a notice ");
    RECV(":bob!user@host NOTICE alice :(#test) a notice ");
    CHECK_CHAN("-- PvNotice(bob): a notice ");
    RECV(":bob!user@host NOTICE alice :{#test} a notice ");
    CHECK_CHAN("-- PvNotice(bob): a notice ");
    RECV(":bob!user@host NOTICE alice :<#test> a notice ");
    CHECK_CHAN("-- PvNotice(bob): a notice ");

    /* broken CTCP to channel */
    RECV(":bob!user@host NOTICE #test :\01");
    CHECK_SRV("-- CTCP reply from bob: ");
    RECV(":bob!user@host NOTICE #test :\01TEST");
    CHECK_SRV("-- CTCP reply from bob: TEST");
    RECV(":bob!user@host NOTICE #test :\01ACTION");
    CHECK_SRV("-- CTCP reply from bob: ACTION");
    RECV(":bob!user@host NOTICE #test :\01ACTION is testing");
    CHECK_SRV("-- CTCP reply from bob: ACTION is testing");
    RECV(":bob!user@host NOTICE #test :\01VERSION");
    CHECK_SRV("-- CTCP reply from bob: VERSION");
    RECV(":bob!user@host NOTICE #test :\01DCC");
    CHECK_SRV("-- CTCP reply from bob: DCC");
    RECV(":bob!user@host NOTICE #test :\01DCC SEND");
    CHECK_SRV("-- CTCP reply from bob: DCC SEND");
    RECV(":bob!user@host NOTICE #test :\01DCC SEND file.txt");
    CHECK_SRV("-- CTCP reply from bob: DCC SEND file.txt");
    RECV(":bob!user@host NOTICE #test :\01DCC SEND file.txt 1 2 3");
    CHECK_SRV("-- CTCP reply from bob: DCC SEND file.txt 1 2 3");

    /* broken CTCP to user */
    RECV(":bob!user@host NOTICE alice :\01");
    CHECK_SRV("-- CTCP reply from bob: ");
    RECV(":bob!user@host NOTICE alice :\01TEST");
    CHECK_SRV("-- CTCP reply from bob: TEST");
    RECV(":bob!user@host NOTICE alice :\01ACTION");
    CHECK_SRV("-- CTCP reply from bob: ACTION");
    RECV(":bob!user@host NOTICE alice :\01ACTION is testing");
    CHECK_SRV("-- CTCP reply from bob: ACTION is testing");
    RECV(":bob!user@host NOTICE alice :\01VERSION");
    CHECK_SRV("-- CTCP reply from bob: VERSION");
    RECV(":bob!user@host NOTICE alice :\01DCC");
    CHECK_SRV("-- CTCP reply from bob: DCC");
    RECV(":bob!user@host NOTICE alice :\01DCC SEND");
    CHECK_SRV("-- CTCP reply from bob: DCC SEND");
    RECV(":bob!user@host NOTICE alice :\01DCC SEND file.txt");
    CHECK_SRV("-- CTCP reply from bob: DCC SEND file.txt");
    RECV(":bob!user@host NOTICE alice :\01DCC SEND file.txt 1 2 3");
    CHECK_SRV("-- CTCP reply from bob: DCC SEND file.txt 1 2 3");

    /* valid CTCP to channel */
    RECV(":bob!user@host NOTICE #test :\01TEST\01");
    CHECK_SRV("-- CTCP reply from bob: TEST");
    RECV(":bob!user@host NOTICE #test :\01ACTION\01");
    CHECK_SRV("-- CTCP reply from bob: ACTION");
    RECV(":bob!user@host NOTICE #test :\01ACTION is testing\01");
    CHECK_SRV("-- CTCP reply from bob: ACTION is testing");
    RECV(":bob!user@host NOTICE #test :\01VERSION\01");
    CHECK_SRV("-- CTCP reply from bob: VERSION");
    RECV(":bob!user@host NOTICE #test :\01DCC SEND file.txt 1 2 3\01");
    CHECK_SRV("-- CTCP reply from bob: DCC SEND file.txt 1 2 3");

    /* valid CTCP to user */
    RECV(":bob!user@host NOTICE alice :\01TEST\01");
    CHECK_SRV("-- CTCP reply from bob: TEST");
    RECV(":bob!user@host NOTICE alice :\01ACTION\01");
    CHECK_SRV("-- CTCP reply from bob: ACTION");
    RECV(":bob!user@host NOTICE alice :\01ACTION is testing\01");
    CHECK_SRV("-- CTCP reply from bob: ACTION is testing");
    RECV(":bob!user@host NOTICE alice :\01VERSION\01");
    CHECK_SRV("-- CTCP reply from bob: VERSION");
    RECV(":bob!user@host NOTICE alice :\01DCC SEND file.txt 1 2 3\01");
    CHECK_SRV("-- CTCP reply from bob: DCC SEND file.txt 1 2 3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_part
 */

TEST(IrcProtocolWithServer, part)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":alice!user@host PART");
    CHECK_ERROR_PARAMS("part", 0, 1);

    /* missing nick */
    RECV("PART #test");
    CHECK_ERROR_NICK("part");

    STRCMP_EQUAL("#test", ptr_server->channels->name);
    CHECK(ptr_server->channels->nicks);
    LONGS_EQUAL(0, ptr_server->channels->part);

    /* channel not found */
    RECV(":alice!user@host PART #xyz");
    CHECK_NO_MSG;
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    CHECK(ptr_server->channels->nicks);
    LONGS_EQUAL(0, ptr_server->channels->part);

    /* without part message */
    RECV(":alice!user@host PART #test");
    CHECK_CHAN("<-- alice (user@host) has left #test");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    POINTERS_EQUAL(NULL, ptr_server->channels->nicks);
    LONGS_EQUAL(1, ptr_server->channels->part);

    /* without part message (but empty trailing parameter) */
    RECV(":alice!user@host JOIN #test");
    RECV(":alice!user@host PART #test :");
    CHECK_CHAN("<-- alice (user@host) has left #test");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    POINTERS_EQUAL(NULL, ptr_server->channels->nicks);
    LONGS_EQUAL(1, ptr_server->channels->part);

    /* with part message */
    RECV(":alice!user@host JOIN #test");
    RECV(":alice!user@host PART #test :part message ");
    CHECK_CHAN("<-- alice (user@host) has left #test (part message )");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    POINTERS_EQUAL(NULL, ptr_server->channels->nicks);
    LONGS_EQUAL(1, ptr_server->channels->part);

    RECV(":alice!user@host JOIN #test");
    CHECK_CHAN("--> alice (user@host) has joined #test");
    RECV(":bob!user@host JOIN #test");
    CHECK_CHAN("--> bob (user@host) has joined #test");

    /* part from another user */
    RECV(":bob!user@host PART #test :part message ");
    CHECK_CHAN("<-- bob (user@host) has left #test (part message )");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    CHECK(ptr_server->channels->nicks == ptr_server->channels->last_nick);
    LONGS_EQUAL(0, ptr_server->channels->part);
}

/*
 * Tests functions:
 *   irc_protocol_cb_ping
 */

TEST(IrcProtocolWithServer, ping)
{
    SRV_INIT;

    /* not enough parameters, no response */
    RECV("PING");
    CHECK_ERROR_PARAMS("ping", 0, 1);
    CHECK_SENT(NULL);

    RECV("PING :123456789");
    CHECK_NO_MSG;
    CHECK_SENT("PONG :123456789");
}

/*
 * Tests functions:
 *   irc_protocol_cb_pong
 */

TEST(IrcProtocolWithServer, pong)
{
    SRV_INIT;

    RECV(":server PONG");
    CHECK_SRV("PONG");
    RECV(":server PONG server");
    CHECK_SRV("PONG");
    RECV(":server PONG server :info");
    CHECK_SRV("PONG: info");
    RECV(":server PONG server :extra info");
    CHECK_SRV("PONG: extra info");
}

/*
 * Tests functions:
 *   irc_protocol_cb_privmsg
 */

TEST(IrcProtocolWithServer, privmsg)
{
    char *info, message[1024];

    SRV_INIT_JOIN2;

    /* not enough parameters */
    RECV(":bob!user@host PRIVMSG");
    CHECK_ERROR_PARAMS("privmsg", 0, 2);
    RECV(":bob!user@host PRIVMSG #test");
    CHECK_ERROR_PARAMS("privmsg", 1, 2);
    RECV(":bob!user@host PRIVMSG alice");
    CHECK_ERROR_PARAMS("privmsg", 1, 2);

    /* missing nick */
    RECV("PRIVMSG #test :this is the message");
    CHECK_ERROR_NICK("privmsg");

    /* message to channel/user */
    RECV(":bob!user@host PRIVMSG #test :this is the message ");
    CHECK_CHAN("bob this is the message ");
    RECV(":bob!user@host PRIVMSG alice :this is the message ");
    CHECK_PV("bob", "bob this is the message ");

    /* message with tags to channel/user */
    RECV("@tag1=value1;tag2=value2 :bob!user@host PRIVMSG #test "
         ":this is the message ");
    CHECK_CHAN("bob this is the message ");
    RECV("@tag1=value1;tag2=value2 :bob!user@host PRIVMSG alice "
         ":this is the message ");
    CHECK_PV("bob", "bob this is the message ");

    /* message to ops of channel */
    RECV(":bob!user@host PRIVMSG @#test :this is the message ");
    CHECK_CHAN("-- Msg:@(bob): this is the message ");

    /* message from self nick (case of bouncer) */
    RECV(":alice!user@host PRIVMSG alice :this is the message ");
    CHECK_PV("alice", "alice this is the message ");

    /* broken CTCP to channel */
    RECV(":bob!user@host PRIVMSG #test :\01");
    CHECK_CHAN("-- Unknown CTCP requested by bob: ");
    RECV(":bob!user@host PRIVMSG #test :\01TEST");
    CHECK_CHAN("-- Unknown CTCP requested by bob: TEST");
    RECV(":bob!user@host PRIVMSG #test :\01ACTION");
    CHECK_CHAN(" * bob");
    RECV(":bob!user@host PRIVMSG #test :\01ACTION is testing");
    CHECK_CHAN(" * bob is testing");
    RECV(":bob!user@host PRIVMSG #test :\01VERSION");
    CHECK_CHAN("-- CTCP requested by bob: VERSION");
    info = irc_ctcp_replace_variables (ptr_server,
                                       irc_ctcp_get_reply (ptr_server,
                                                           "VERSION"));
    snprintf (message, sizeof (message),
              "-- CTCP reply to bob: VERSION %s", info);
    CHECK_CHAN(message);
    snprintf (message, sizeof (message),
              "NOTICE bob :\01VERSION %s\01", info);
    CHECK_SENT(message);
    free (info);
    RECV(":bob!user@host PRIVMSG #test :\01DCC");
    CHECK_NO_MSG;
    RECV(":bob!user@host PRIVMSG #test :\01DCC SEND");
    CHECK_NO_MSG;
    RECV(":bob!user@host PRIVMSG #test :\01DCC SEND file.txt");
    CHECK_SRV("=!= irc: cannot parse \"privmsg\" command");
    RECV(":bob!user@host PRIVMSG #test :\01DCC SEND file.txt 1 2 3");
    CHECK_CORE("xfer: incoming file from bob (0.0.0.1, irc." IRC_FAKE_SERVER
               "), name: file.txt, 3 bytes (protocol: dcc)");

    /* broken CTCP to user */
    RECV(":bob!user@host PRIVMSG alice :\01");
    CHECK_SRV("-- Unknown CTCP requested by bob: ");
    RECV(":bob!user@host PRIVMSG alice :\01TEST");
    CHECK_SRV("-- Unknown CTCP requested by bob: TEST");
    RECV(":bob!user@host PRIVMSG alice :\01ACTION");
    CHECK_PV("bob", " * bob");
    RECV(":bob!user@host PRIVMSG alice :\01ACTION is testing");
    CHECK_PV("bob", " * bob is testing");
    RECV(":bob!user@host PRIVMSG alice :\01VERSION");
    info = irc_ctcp_replace_variables (ptr_server,
                                       irc_ctcp_get_reply (ptr_server,
                                                           "VERSION"));
    snprintf (message, sizeof (message),
              "-- CTCP reply to bob: VERSION %s", info);
    CHECK_SRV(message);
    free (info);
    RECV(":bob!user@host PRIVMSG alice :\01DCC");
    CHECK_NO_MSG;
    RECV(":bob!user@host PRIVMSG alice :\01DCC SEND");
    CHECK_NO_MSG;
    RECV(":bob!user@host PRIVMSG alice :\01DCC SEND file.txt");
    CHECK_SRV("=!= irc: cannot parse \"privmsg\" command");
    RECV(":bob!user@host PRIVMSG alice :\01DCC SEND file.txt 1 2 3");
    CHECK_CORE("xfer: incoming file from bob (0.0.0.1, irc." IRC_FAKE_SERVER
               "), name: file.txt, 3 bytes (protocol: dcc)");

    /* valid CTCP to channel */
    RECV(":bob!user@host PRIVMSG #test :\01TEST\01");
    RECV(":bob!user@host PRIVMSG #test :\01ACTION\01");
    CHECK_CHAN(" * bob");
    RECV(":bob!user@host PRIVMSG #test :\01ACTION is testing\01");
    CHECK_CHAN(" * bob is testing");
    RECV(":bob!user@host PRIVMSG #test :\01VERSION\01");
    RECV(":bob!user@host PRIVMSG #test :\01DCC SEND file.txt 1 2 3\01");

    /* valid CTCP to user */
    RECV(":bob!user@host PRIVMSG alice :\01TEST\01");
    CHECK_SENT(NULL);
    RECV(":bob!user@host PRIVMSG alice :\01ACTION\01");
    CHECK_SENT(NULL);
    RECV(":bob!user@host PRIVMSG alice :\01ACTION is testing\01");
    CHECK_SENT(NULL);
    RECV(":bob!user@host PRIVMSG alice :\01VERSION\01");
    CHECK_SRV("-- CTCP requested by bob: VERSION");
    info = irc_ctcp_replace_variables (ptr_server,
                                       irc_ctcp_get_reply (ptr_server,
                                                           "VERSION"));
    snprintf (message, sizeof (message),
              "-- CTCP reply to bob: VERSION %s", info);
    CHECK_SRV(message);
    snprintf (message, sizeof (message),
              "NOTICE bob :\01VERSION %s\01", info);
    CHECK_SENT(message);
    free (info);
    RECV(":bob!user@host PRIVMSG alice :\01SOURCE\01");
    info = hook_info_get (NULL, "weechat_site_download", "");
    snprintf (message, sizeof (message),
              "NOTICE bob :\01SOURCE %s\01", info);
    CHECK_SENT(message);
    free (info);
    RECV(":bob!user@host PRIVMSG alice :\01DCC SEND file.txt 1 2 3\01");
    CHECK_CORE("xfer: incoming file from bob (0.0.0.1, irc." IRC_FAKE_SERVER
               "), name: file.txt, 3 bytes (protocol: dcc)");
    CHECK_SENT(NULL);

    /* close xfer buffer */
    if (xfer_buffer)
        gui_buffer_close (xfer_buffer);
}

/*
 * Tests functions:
 *   irc_protocol_cb_quit
 */

TEST(IrcProtocolWithServer, quit)
{
    struct t_irc_channel *ptr_channel;

    SRV_INIT_JOIN;

    /* missing nick */
    RECV("QUIT");
    CHECK_ERROR_NICK("quit");

    RECV(":bob!user@host PRIVMSG alice :hi Alice!");
    CHECK_PV("bob", "bob hi Alice!");

    ptr_channel = ptr_server->channels;

    /* without quit message */
    RECV(":bob!user@host JOIN #test");
    RECV(":bob!user@host QUIT");
    CHECK_CHAN("<-- bob (user@host) has quit");
    LONGS_EQUAL(1, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    /* without quit message (but empty trailing parameter) */
    RECV(":bob!user@host JOIN #test");
    RECV(":bob!user@host QUIT :");
    CHECK_CHAN("<-- bob (user@host) has quit");
    LONGS_EQUAL(1, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    /* with quit message */
    RECV(":bob!user@host JOIN #test");
    RECV(":bob!user@host QUIT :quit message ");
    CHECK_CHAN("<-- bob (user@host) has quit (quit message )");
    LONGS_EQUAL(1, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);
}

/*
 * Tests functions:
 *   irc_protocol_cb_setname (without setname capability)
 */

TEST(IrcProtocolWithServer, setname_without_setname_cap)
{
    struct t_irc_nick *ptr_nick;

    SRV_INIT_JOIN2;

    ptr_nick = ptr_server->channels->nicks;

    POINTERS_EQUAL(NULL, ptr_nick->realname);

    /* not enough parameters */
    RECV(":alice!user@host SETNAME");
    CHECK_ERROR_PARAMS("setname", 0, 1);

    /* missing nick */
    RECV("SETNAME :new bob realname");
    CHECK_ERROR_NICK("setname");

    POINTERS_EQUAL(NULL, ptr_nick->realname);

    /* real name of "bob" has changed */
    RECV(":bob!user@host SETNAME :new bob realname ");
    CHECK_CHAN("-- bob has changed real name to \"new bob realname \"");
    POINTERS_EQUAL(NULL, ptr_nick->realname);

    /* self real name has changed */
    RECV(":alice!user@host SETNAME :new alice realname ");
    CHECK_SRV("-- Your real name has been set to \"new alice realname \"");
    POINTERS_EQUAL(NULL, ptr_nick->realname);
}

/*
 * Tests functions:
 *   irc_protocol_cb_setname (with setname capability)
 */

TEST(IrcProtocolWithServer, setname_with_setname_cap)
{
    struct t_irc_nick *ptr_nick, *ptr_nick2;

    /* assume "setname" capability is enabled in server */
    hashtable_set (ptr_server->cap_list, "setname", NULL);

    SRV_INIT_JOIN2;

    ptr_nick = ptr_server->channels->nicks;
    ptr_nick2 = ptr_server->channels->last_nick;

    POINTERS_EQUAL(NULL, ptr_nick->realname);

    /* real name of "bob" has changed */
    RECV(":bob!user@host SETNAME :new bob realname ");
    CHECK_CHAN("-- bob has changed real name to \"new bob realname \"");
    STRCMP_EQUAL("new bob realname ", ptr_nick2->realname);

    /* self real name has changed */
    RECV(":alice!user@host SETNAME :new realname");
    CHECK_SRV("-- Your real name has been set to \"new realname\"");
    STRCMP_EQUAL("new realname", ptr_nick->realname);

    /* self real name has changed */
    RECV(":alice!user@host SETNAME :new realname2");
    CHECK_SRV("-- Your real name has been set to \"new realname2\"");
    STRCMP_EQUAL("new realname2", ptr_nick->realname);
}

/*
 * Tests functions:
 *   irc_protocol_cb_tagmsg
 */

TEST(IrcProtocolWithServer, tagmsg)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_typing_status *ptr_typing_status;

    SRV_INIT_JOIN2;

    /* not enough parameters */
    RECV(":bob!user@host TAGMSG");
    CHECK_ERROR_PARAMS("tagmsg", 0, 1);

    /* no tags */
    RECV(":bob!user@host TAGMSG #test");
    CHECK_NO_MSG;
    RECV(":bob!user@host TAGMSG :#test");
    CHECK_NO_MSG;

    /* with tags */
    RECV("@tag1=123;tag2=456 :bob!user@host TAGMSG #test ");
    CHECK_NO_MSG;
    RECV("@tag1=123;tag2=456 :bob!user@host TAGMSG :#test ");
    CHECK_NO_MSG;

    /* check typing status */
    ptr_buffer = ptr_server->channels->buffer;

    config_file_option_set (irc_config_look_typing_status_nicks, "on", 1);
    config_file_option_set (typing_config_look_enabled_nicks, "on", 1);

    POINTERS_EQUAL(NULL, typing_status_nick_search (ptr_buffer, "bob"));

    RECV("@+typing=active :bob!user@host TAGMSG #test");
    ptr_typing_status = typing_status_nick_search (ptr_buffer, "bob");
    CHECK(ptr_typing_status);
    LONGS_EQUAL(TYPING_STATUS_STATE_TYPING, ptr_typing_status->state);

    RECV("@+typing=paused :bob!user@host TAGMSG :#test");
    ptr_typing_status = typing_status_nick_search (ptr_buffer, "bob");
    CHECK(ptr_typing_status);
    LONGS_EQUAL(TYPING_STATUS_STATE_PAUSED, ptr_typing_status->state);

    RECV("@+typing=done :bob!user@host TAGMSG #test");
    POINTERS_EQUAL(NULL, typing_status_nick_search (ptr_buffer, "bob"));

    config_file_option_reset (typing_config_look_enabled_nicks, 1);
    config_file_option_reset (irc_config_look_typing_status_nicks, 1);
}

/*
 * Tests functions:
 *   irc_protocol_cb_topic
 */

TEST(IrcProtocolWithServer, topic)
{
    struct t_irc_channel *ptr_channel;

    SRV_INIT_JOIN;

    ptr_channel = ptr_server->channels;
    POINTERS_EQUAL(NULL, ptr_channel->topic);

    /* not enough parameters */
    RECV(":alice!user@host TOPIC");
    CHECK_ERROR_PARAMS("topic", 0, 1);

    /* missing nick */
    RECV("TOPIC #test :new topic");
    CHECK_ERROR_NICK("topic");

    POINTERS_EQUAL(NULL, ptr_channel->topic);

    /* not a channel */
    RECV(":alice!user@host TOPIC bob");
    CHECK_SRV("=!= irc: \"topic\" command received without channel");

    /* empty topic */
    RECV(":alice!user@host TOPIC #test");
    CHECK_CHAN("-- alice has unset topic for #test");
    POINTERS_EQUAL(NULL, ptr_channel->topic);

    /* empty topic (with empty trailing parameter) */
    RECV(":alice!user@host TOPIC #test :");
    CHECK_CHAN("-- alice has unset topic for #test");
    POINTERS_EQUAL(NULL, ptr_channel->topic);

    /* new topic */
    RECV(":alice!user@host TOPIC #test :new topic ");
    CHECK_CHAN("-- alice has changed topic for #test to \"new topic \"");
    STRCMP_EQUAL("new topic ", ptr_channel->topic);

    /* another new topic */
    RECV(":alice!user@host TOPIC #test :another new topic ");
    CHECK_CHAN("-- alice has changed topic for #test from \"new topic \" to "
               "\"another new topic \"");
    STRCMP_EQUAL("another new topic ", ptr_channel->topic);

    /* empty topic */
    RECV(":alice!user@host TOPIC #test");
    CHECK_CHAN("-- alice has unset topic for #test (old topic: "
               "\"another new topic \")");
    POINTERS_EQUAL(NULL, ptr_channel->topic);
}

/*
 * Tests functions:
 *   irc_protocol_cb_wallops
 */

TEST(IrcProtocolWithServer, wallops)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":alice!user@host WALLOPS");
    CHECK_ERROR_PARAMS("wallops", 0, 1);

    RECV(":alice!user@host WALLOPS message ");
    CHECK_SRV("-- Wallops from alice (user@host): message");

    RECV(":alice!user@host WALLOPS :message from admin ");
    CHECK_SRV("-- Wallops from alice (user@host): message from admin ");
}

/*
 * Tests functions:
 *   irc_protocol_cb_warn
 */

TEST(IrcProtocolWithServer, warn)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server WARN");
    CHECK_ERROR_PARAMS("warn", 0, 2);
    RECV(":server WARN *");
    CHECK_ERROR_PARAMS("warn", 1, 2);
    RECV(":server WARN COMMAND");
    CHECK_ERROR_PARAMS("warn", 1, 2);

    RECV(":server WARN * TEST");
    CHECK_SRV("=!= Warning: [] TEST");
    RECV(":server WARN * TEST :the message");
    CHECK_SRV("=!= Warning: [TEST] the message");
    RECV(":server WARN * TEST TEST2");
    CHECK_SRV("=!= Warning: [TEST] TEST2");
    RECV(":server WARN * TEST TEST2 :the message");
    CHECK_SRV("=!= Warning: [TEST TEST2] the message");

    RECV(":server WARN COMMAND TEST");
    CHECK_SRV("=!= Warning: COMMAND [] TEST");
    RECV(":server WARN COMMAND TEST :the message");
    CHECK_SRV("=!= Warning: COMMAND [TEST] the message");
    RECV(":server WARN COMMAND TEST TEST2");
    CHECK_SRV("=!= Warning: COMMAND [TEST] TEST2");
    RECV(":server WARN COMMAND TEST TEST2 :the message");
    CHECK_SRV("=!= Warning: COMMAND [TEST TEST2] the message");
}

/*
 * Tests functions:
 *   irc_protocol_cb_001 (connected to IRC server, empty)
 */

TEST(IrcProtocolWithServer, 001_empty)
{
    LONGS_EQUAL(0, ptr_server->is_connected);
    STRCMP_EQUAL("nick1", ptr_server->nick);

    /* not enough parameters */
    RECV(":server 001");
    CHECK_ERROR_PARAMS("001", 0, 1);

    RECV(":server 001 alice");
    CHECK_SRV("--");
    LONGS_EQUAL(1, ptr_server->is_connected);
    STRCMP_EQUAL("alice", ptr_server->nick);
}

/*
 * Tests functions:
 *   irc_protocol_cb_001 (connected to IRC server, welcome message)
 */

TEST(IrcProtocolWithServer, 001_welcome)
{
    run_cmd_quiet ("/mute /set irc.server." IRC_FAKE_SERVER ".autojoin \"#autojoin1\"");
    run_cmd_quiet ("/mute /set irc.server." IRC_FAKE_SERVER ".command "
                   "\"/join #test1;/join #test2;/query remote_nick\"");
    LONGS_EQUAL(0, ptr_server->is_connected);
    STRCMP_EQUAL("nick1", ptr_server->nick);

    RECV(":server 001 alice :Welcome on this server, alice!");
    CHECK_SRV("-- Welcome on this server, alice!");

    LONGS_EQUAL(1, ptr_server->is_connected);
    STRCMP_EQUAL("alice", ptr_server->nick);
    CHECK(ptr_server->channels);
    STRCMP_EQUAL("remote_nick", ptr_server->channels->name);
}

/*
 * Tests functions:
 *   irc_protocol_cb_005 (infos from server, empty)
 */

TEST(IrcProtocolWithServer, 005_empty)
{
    SRV_INIT;

    POINTERS_EQUAL(NULL, ptr_server->prefix_modes);
    POINTERS_EQUAL(NULL, ptr_server->prefix_chars);

    RECV(":server 005 alice TEST=A");
    CHECK_SRV("-- TEST=A");

    POINTERS_EQUAL(NULL, ptr_server->prefix_modes);
    POINTERS_EQUAL(NULL, ptr_server->prefix_chars);
}

/*
 * Tests functions:
 *   irc_protocol_cb_005 (infos from server, full)
 */

TEST(IrcProtocolWithServer, 005_full)
{
    SRV_INIT;

    POINTERS_EQUAL(NULL, ptr_server->prefix_modes);
    POINTERS_EQUAL(NULL, ptr_server->prefix_chars);
    LONGS_EQUAL(0, ptr_server->nick_max_length);
    LONGS_EQUAL(0, ptr_server->user_max_length);
    LONGS_EQUAL(0, ptr_server->host_max_length);
    LONGS_EQUAL(0, ptr_server->casemapping);
    POINTERS_EQUAL(NULL, ptr_server->chantypes);
    POINTERS_EQUAL(NULL, ptr_server->chanmodes);
    LONGS_EQUAL(0, ptr_server->monitor);
    POINTERS_EQUAL(NULL, ptr_server->isupport);

    RECV(":server 005 alice " IRC_MSG_005 " :are supported");
    CHECK_SRV("-- " IRC_MSG_005 " are supported");

    STRCMP_EQUAL("ohv", ptr_server->prefix_modes);
    STRCMP_EQUAL("@%+", ptr_server->prefix_chars);
    LONGS_EQUAL(30, ptr_server->nick_max_length);
    LONGS_EQUAL(16, ptr_server->user_max_length);
    LONGS_EQUAL(32, ptr_server->host_max_length);
    LONGS_EQUAL(1, ptr_server->casemapping);
    STRCMP_EQUAL("#", ptr_server->chantypes);
    STRCMP_EQUAL("eIbq,k,flj,CFLMPQScgimnprstuz", ptr_server->chanmodes);
    LONGS_EQUAL(100, ptr_server->monitor);
    STRCMP_EQUAL(IRC_MSG_005, ptr_server->isupport);

    /* check that realloc of info is OK if we receive the message again */
    RECV(":server 005 alice " IRC_MSG_005 " :are supported");
    CHECK_SRV("-- " IRC_MSG_005 " are supported");

    STRCMP_EQUAL("ohv", ptr_server->prefix_modes);
    STRCMP_EQUAL("@%+", ptr_server->prefix_chars);
    LONGS_EQUAL(30, ptr_server->nick_max_length);
    LONGS_EQUAL(16, ptr_server->user_max_length);
    LONGS_EQUAL(32, ptr_server->host_max_length);
    LONGS_EQUAL(1, ptr_server->casemapping);
    STRCMP_EQUAL("#", ptr_server->chantypes);
    STRCMP_EQUAL("eIbq,k,flj,CFLMPQScgimnprstuz", ptr_server->chanmodes);
    LONGS_EQUAL(100, ptr_server->monitor);
    STRCMP_EQUAL(IRC_MSG_005 " " IRC_MSG_005, ptr_server->isupport);
}

/*
 * Tests functions:
 *   irc_protocol_cb_005 (infos from server, multiple messages)
 */

TEST(IrcProtocolWithServer, 005_multiple_messages)
{
    SRV_INIT;

    POINTERS_EQUAL(NULL, ptr_server->prefix_modes);
    POINTERS_EQUAL(NULL, ptr_server->prefix_chars);
    LONGS_EQUAL(0, ptr_server->host_max_length);
    POINTERS_EQUAL(NULL, ptr_server->isupport);

    RECV(":server 005 alice PREFIX=(ohv)@%+ :are supported");
    CHECK_SRV("-- PREFIX=(ohv)@%+ are supported");
    STRCMP_EQUAL("ohv", ptr_server->prefix_modes);
    STRCMP_EQUAL("@%+", ptr_server->prefix_chars);
    STRCMP_EQUAL("PREFIX=(ohv)@%+", ptr_server->isupport);

    RECV(":server 005 alice HOSTLEN=24 :are supported");
    CHECK_SRV("-- HOSTLEN=24 are supported");
    LONGS_EQUAL(24, ptr_server->host_max_length);
    STRCMP_EQUAL("PREFIX=(ohv)@%+ HOSTLEN=24", ptr_server->isupport);
}

/*
 * Tests functions:
 *   irc_protocol_cb_008 (server notice mask)
 */

TEST(IrcProtocolWithServer, 008)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 008");
    CHECK_ERROR_PARAMS("008", 0, 2);
    RECV(":server 008 alice");
    CHECK_ERROR_PARAMS("008", 1, 2);

    RECV(":server 008 alice +Zbfkrsuy :Server notice mask");
    CHECK_SRV("-- Server notice mask for alice: +Zbfkrsuy Server notice mask");
}

/*
 * Tests functions:
 *   irc_protocol_cb_221 (user mode string)
 */

TEST(IrcProtocolWithServer, 221)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 221");
    CHECK_ERROR_PARAMS("221", 0, 2);
    RECV(":server 221 alice");
    CHECK_ERROR_PARAMS("221", 1, 2);

    POINTERS_EQUAL(NULL, ptr_server->nick_modes);

    RECV(":server 221 alice :+abc");
    CHECK_SRV("-- User mode for alice is [+abc]");
    STRCMP_EQUAL("abc", ptr_server->nick_modes);

    RECV(":server 221 alice :-abc");
    CHECK_SRV("-- User mode for alice is [-abc]");
    POINTERS_EQUAL(NULL, ptr_server->nick_modes);
}

/*
 * Tests functions:
 *   irc_protocol_cb_whois_nick_msg
 *
 * Messages:
 *   223: whois (charset is)
 *   264: whois (is using encrypted connection)
 *   275: whois (secure connection)
 *   276: whois (has client certificate fingerprint)
 *   307: whois (registered nick)
 *   310: whois (help mode)
 *   313: whois (operator)
 *   318: whois (end)
 *   319: whois (channels)
 *   320: whois (identified user)
 *   326: whois (has oper privs)
 *   335: is a bot on
 *   378: whois (connecting from)
 *   379: whois (using modes)
 *   671: whois (secure connection)
 */

TEST(IrcProtocolWithServer, whois_nick_msg)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 223");
    CHECK_ERROR_PARAMS("223", 0, 2);
    RECV(":server 223 alice");
    CHECK_ERROR_PARAMS("223", 1, 2);
    RECV(":server 264");
    CHECK_ERROR_PARAMS("264", 0, 2);
    RECV(":server 264 alice");
    CHECK_ERROR_PARAMS("264", 1, 2);
    RECV(":server 275");
    CHECK_ERROR_PARAMS("275", 0, 2);
    RECV(":server 275 alice");
    CHECK_ERROR_PARAMS("275", 1, 2);
    RECV(":server 276");
    CHECK_ERROR_PARAMS("276", 0, 2);
    RECV(":server 276 alice");
    CHECK_ERROR_PARAMS("276", 1, 2);
    RECV(":server 307");
    CHECK_ERROR_PARAMS("307", 0, 2);
    RECV(":server 307 alice");
    CHECK_ERROR_PARAMS("307", 1, 2);
    RECV(":server 310");
    CHECK_ERROR_PARAMS("310", 0, 2);
    RECV(":server 310 alice");
    CHECK_ERROR_PARAMS("310", 1, 2);
    RECV(":server 313");
    CHECK_ERROR_PARAMS("313", 0, 2);
    RECV(":server 313 alice");
    CHECK_ERROR_PARAMS("313", 1, 2);
    RECV(":server 318");
    CHECK_ERROR_PARAMS("318", 0, 2);
    RECV(":server 318 alice");
    CHECK_ERROR_PARAMS("318", 1, 2);
    RECV(":server 319");
    CHECK_ERROR_PARAMS("319", 0, 2);
    RECV(":server 319 alice");
    CHECK_ERROR_PARAMS("319", 1, 2);
    RECV(":server 320");
    CHECK_ERROR_PARAMS("320", 0, 2);
    RECV(":server 320 alice");
    CHECK_ERROR_PARAMS("320", 1, 2);
    RECV(":server 326");
    CHECK_ERROR_PARAMS("326", 0, 2);
    RECV(":server 326 alice");
    CHECK_ERROR_PARAMS("326", 1, 2);
    RECV(":server 335");
    CHECK_ERROR_PARAMS("335", 0, 2);
    RECV(":server 335 alice");
    CHECK_ERROR_PARAMS("335", 1, 2);
    RECV(":server 378");
    CHECK_ERROR_PARAMS("378", 0, 2);
    RECV(":server 378 alice");
    CHECK_ERROR_PARAMS("378", 1, 2);
    RECV(":server 379");
    CHECK_ERROR_PARAMS("379", 0, 2);
    RECV(":server 379 alice");
    CHECK_ERROR_PARAMS("379", 1, 2);
    RECV(":server 671");
    CHECK_ERROR_PARAMS("671", 0, 2);
    RECV(":server 671 alice");
    CHECK_ERROR_PARAMS("671", 1, 2);

    RECV(":server 223 alice bob UTF-8");
    CHECK_SRV("-- [bob] UTF-8");
    RECV(":server 223 alice bob :UTF-8");
    CHECK_SRV("-- [bob] UTF-8");
    RECV(":server 223 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 264 alice bob :is using encrypted connection");
    CHECK_SRV("-- [bob] is using encrypted connection");
    RECV(":server 264 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 275 alice bob :is using secure connection");
    CHECK_SRV("-- [bob] is using secure connection");
    RECV(":server 275 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 276 alice bob :has client certificate fingerprint");
    CHECK_SRV("-- [bob] has client certificate fingerprint");
    RECV(":server 276 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 307 alice bob :registered nick");
    CHECK_SRV("-- [bob] registered nick");
    RECV(":server 307 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 310 alice bob :help mode");
    CHECK_SRV("-- [bob] help mode");
    RECV(":server 310 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 313 alice bob :operator");
    CHECK_SRV("-- [bob] operator");
    RECV(":server 313 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 318 alice bob :end");
    CHECK_SRV("-- [bob] end");
    RECV(":server 318 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 319 alice bob :channels");
    CHECK_SRV("-- [bob] channels");
    RECV(":server 319 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 320 alice bob :identified user");
    CHECK_SRV("-- [bob] identified user");
    RECV(":server 320 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 326 alice bob :has oper privs");
    CHECK_SRV("-- [bob] has oper privs");
    RECV(":server 326 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 335 alice bob :is a bot");
    CHECK_SRV("-- [bob] is a bot");
    RECV(":server 335 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 378 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 378 alice bob :connecting from");
    CHECK_SRV("-- [bob] connecting from");
    RECV(":server 378 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 379 alice bob :using modes");
    CHECK_SRV("-- [bob] using modes");
    RECV(":server 379 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 671 alice bob :secure connection");
    CHECK_SRV("-- [bob] secure connection");
    RECV(":server 671 alice bob");
    CHECK_SRV("-- bob");
}

/*
 * Tests functions:
 *   irc_protocol_cb_whowas_nick_msg
 *
 * Messages:
 *   369: whowas (end)
 */

TEST(IrcProtocolWithServer, whowas_nick_msg)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 369");
    CHECK_ERROR_PARAMS("369", 0, 2);
    RECV(":server 369 alice");
    CHECK_ERROR_PARAMS("369", 1, 2);

    RECV(":server 369 alice bob end");
    CHECK_SRV("-- [bob] end");
    RECV(":server 369 alice bob :end");
    CHECK_SRV("-- [bob] end");
    RECV(":server 369 alice bob");
    CHECK_SRV("-- bob");
}

/*
 * Tests functions:
 *   irc_protocol_cb_301 (away message)
 */

TEST(IrcProtocolWithServer, 301)
{
    SRV_INIT;

    RECV(":bob!user@host PRIVMSG alice :hi Alice!");
    CHECK_PV("bob", "bob hi Alice!");

    /* not enough parameters */
    RECV(":server 301");
    CHECK_ERROR_PARAMS("301", 0, 1);

    POINTERS_EQUAL(NULL, ptr_server->channels->away_message);

    RECV(":server 301 alice bob");
    CHECK_NO_MSG;
    POINTERS_EQUAL(NULL, ptr_server->channels->away_message);

    RECV(":server 301 alice bob :I am away ");
    CHECK_PV("bob", "-- [bob] is away: I am away ");
    STRCMP_EQUAL("I am away ", ptr_server->channels->away_message);
}

/*
 * Tests functions:
 *   irc_protocol_cb_303 (ison)
 */

TEST(IrcProtocolWithServer, 303)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 303");
    CHECK_ERROR_PARAMS("303", 0, 2);
    RECV(":server 303 alice");
    CHECK_ERROR_PARAMS("303", 1, 2);

    RECV(":server 303 alice :nick1 nick2");
    CHECK_SRV("-- Users online: nick1 nick2");
}

/*
 * Tests functions:
 *   irc_protocol_cb_305 (unaway)
 *   irc_protocol_cb_306 (away)
 */

TEST(IrcProtocolWithServer, 305_306)
{
    SRV_INIT;

    RECV(":bob!user@host PRIVMSG alice :hi Alice!");
    CHECK_PV("bob", "bob hi Alice!");

    /* not enough parameters */
    RECV(":server 305");
    CHECK_ERROR_PARAMS("305", 0, 1);
    RECV(":server 306");
    CHECK_ERROR_PARAMS("306", 0, 1);

    POINTERS_EQUAL(NULL, ptr_server->channels->away_message);

    RECV(":server 306 alice");  /* now away */
    CHECK_NO_MSG;
    LONGS_EQUAL(1, ptr_server->is_away);

    RECV(":server 305 alice");
    CHECK_NO_MSG;
    LONGS_EQUAL(0, ptr_server->is_away);

    RECV(":server 306 alice :We'll miss you");  /* now away */
    CHECK_SRV("-- We'll miss you");
    LONGS_EQUAL(1, ptr_server->is_away);

    RECV(":server 305 alice :Does this mean you're really back?");
    CHECK_SRV("-- Does this mean you're really back?");
    LONGS_EQUAL(0, ptr_server->is_away);
}

/*
 * Tests functions:
 *   irc_protocol_cb_311 (whois, user)
 */

TEST(IrcProtocolWithServer, 311)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 311");
    CHECK_ERROR_PARAMS("311", 0, 3);
    RECV(":server 311 alice");
    CHECK_ERROR_PARAMS("311", 1, 3);
    RECV(":server 311 alice bob");
    CHECK_ERROR_PARAMS("311", 2, 3);
    RECV(":server 311 alice bob user");

    /* non-standard parameters (using default whois callback) */
    RECV(":server 311 alice bob user");
    CHECK_SRV("-- [bob] user");

    /* standard parameters */
    RECV(":server 311 alice bob user host * :real name");
    CHECK_SRV("-- [bob] (user@host): real name");
}

/*
 * Tests functions:
 *   irc_protocol_cb_312 (whois, server)
 */

TEST(IrcProtocolWithServer, 312)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 312");
    CHECK_ERROR_PARAMS("312", 0, 3);
    RECV(":server 312 alice");
    CHECK_ERROR_PARAMS("312", 1, 3);
    RECV(":server 312 alice bob");
    CHECK_ERROR_PARAMS("312", 2, 3);

    /* non-standard parameters (using default whois callback) */
    RECV(":server 312 alice bob server");
    CHECK_SRV("-- [bob] server");

    /* standard parameters */
    RECV(":server 312 alice bob server :https://example.com/");
    CHECK_SRV("-- [bob] server (https://example.com/)");
}

/*
 * Tests functions:
 *   irc_protocol_cb_314 (whowas)
 */

TEST(IrcProtocolWithServer, 314)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 314");
    CHECK_ERROR_PARAMS("314", 0, 3);
    RECV(":server 314 alice");
    CHECK_ERROR_PARAMS("314", 1, 3);
    RECV(":server 314 alice bob");
    CHECK_ERROR_PARAMS("314", 2, 3);

    /* non-standard parameters (using default whowas callback) */
    RECV(":server 314 alice bob user");
    CHECK_SRV("-- [bob] user");

    /* standard parameters */
    RECV(":server 314 alice bob user host * :real name");
    CHECK_SRV("-- [bob] (user@host) was real name");
}

/*
 * Tests functions:
 *   irc_protocol_cb_315 (end of /who)
 */

TEST(IrcProtocolWithServer, 315)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 315");
    CHECK_ERROR_PARAMS("315", 0, 3);
    RECV(":server 315 alice");
    CHECK_ERROR_PARAMS("315", 1, 3);
    RECV(":server 315 alice #test");
    CHECK_ERROR_PARAMS("315", 2, 3);

    RECV(":server 315 alice #test End of /WHO list.");
    CHECK_SRV("-- [#test] End of /WHO list.");
}

/*
 * Tests functions:
 *   irc_protocol_cb_317 (whois, idle)
 */

TEST(IrcProtocolWithServer, 317)
{
    time_t time;
    char message[1024];

    SRV_INIT;

    /* not enough parameters */
    RECV(":server 317");
    CHECK_ERROR_PARAMS("317", 0, 4);
    RECV(":server 317 alice");
    CHECK_ERROR_PARAMS("317", 1, 4);
    RECV(":server 317 alice bob");
    CHECK_ERROR_PARAMS("317", 2, 4);
    RECV(":server 317 alice bob 122877");
    CHECK_ERROR_PARAMS("317", 3, 4);

    /* signon at 03/12/2008 @ 1:18pm (UTC) */
    RECV(":server 317 alice bob 122877 1205327880");
    CHECK_SRV("-- [bob] idle: 1 day, 10 hours 07 minutes 57 seconds, "
              "signon at: Wed, 12 Mar 2008 13:18:00");
    RECV(":server 317 alice bob 122877 1205327880 :seconds idle, signon time");
    CHECK_SRV("-- [bob] idle: 1 day, 10 hours 07 minutes 57 seconds, "
              "signon at: Wed, 12 Mar 2008 13:18:00");

    /* signon 2 minutes ago */
    time = time_t (NULL);
    time -= 120;
    snprintf (message, sizeof (message),
              ":server 317 alice bob 30 %lld :seconds idle, signon time",
              (long long)time);
    RECV(message);
}

/*
 * Tests functions:
 *   irc_protocol_cb_321 (/list start)
 */

TEST(IrcProtocolWithServer, 321)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 321");
    CHECK_ERROR_PARAMS("321", 0, 2);
    RECV(":server 321 alice");
    CHECK_ERROR_PARAMS("321", 1, 2);

    RECV(":server 321 alice #test");
    CHECK_SRV("-- #test");
    RECV(":server 321 alice #test Users");
    CHECK_SRV("-- #test Users");
    RECV(":server 321 alice #test :Users  Name");
    CHECK_SRV("-- #test Users  Name");
}

/*
 * Tests functions:
 *   irc_protocol_cb_322 (channel for /list)
 */

TEST(IrcProtocolWithServer, 322)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 322");
    CHECK_ERROR_PARAMS("322", 0, 3);
    RECV(":server 322 alice");
    CHECK_ERROR_PARAMS("322", 1, 3);
    RECV(":server 322 alice #test");
    CHECK_ERROR_PARAMS("322", 2, 3);

    RECV(":server 322 alice #test 3");
    CHECK_SRV("-- #test(3)");
    RECV(":server 322 alice #test 3 :topic of channel ");
    CHECK_SRV("-- #test(3): topic of channel ");

    run_cmd_quiet ("/list -server " IRC_FAKE_SERVER " -re #test.*");
    CHECK_SRV("-- #test(3): topic of channel ");

    RECV(":server 322 alice #test 3");
    CHECK_SRV("-- #test(3)");
    RECV(":server 322 alice #test 3 :topic of channel ");
    CHECK_SRV("-- #test(3): topic of channel ");

    RECV(":server 322 alice #xyz 3");
    CHECK_NO_MSG;
    RECV(":server 322 alice #xyz 3 :topic of channel ");
    CHECK_NO_MSG;
}

/*
 * Tests functions:
 *   irc_protocol_cb_323 (end of /list)
 */

TEST(IrcProtocolWithServer, 323)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 323");
    CHECK_ERROR_PARAMS("323", 0, 1);

    RECV(":server 323 alice");
    CHECK_SRV("--");
    RECV(":server 323 alice end");
    CHECK_SRV("-- end");
    RECV(":server 323 alice :End of /LIST");
    CHECK_SRV("-- End of /LIST");
}

/*
 * Tests functions:
 *   irc_protocol_cb_324 (channel mode)
 */

TEST(IrcProtocolWithServer, 324)
{
    SRV_INIT_JOIN;

    POINTERS_EQUAL(NULL, ptr_server->channels->modes);

    /* not enough parameters */
    RECV(":server 324");
    CHECK_ERROR_PARAMS("324", 0, 2);
    RECV(":server 324 alice");
    CHECK_ERROR_PARAMS("324", 1, 2);

    RECV(":server 324 alice #test +nt");
    CHECK_NO_MSG;
    STRCMP_EQUAL("+nt", ptr_server->channels->modes);

    RECV(":server 324 alice #test");
    CHECK_CHAN("-- Mode #test []");
    POINTERS_EQUAL(NULL, ptr_server->channels->modes);
}

/*
 * Tests functions:
 *   irc_protocol_cb_327 (whois, host)
 */

TEST(IrcProtocolWithServer, 327)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 327");
    CHECK_ERROR_PARAMS("327", 0, 3);
    RECV(":server 327 alice");
    CHECK_ERROR_PARAMS("327", 1, 3);
    RECV(":server 327 alice bob");
    CHECK_ERROR_PARAMS("327", 2, 3);

    /* non-standard parameters (using default whois callback) */
    RECV(":server 327 alice bob host");
    CHECK_SRV("-- [bob] host");

    /* standard parameters */
    RECV(":server 327 alice bob host 1.2.3.4");
    CHECK_SRV("-- [bob] host 1.2.3.4");
    RECV(":server 327 alice bob host 1.2.3.4 :real name");
    CHECK_SRV("-- [bob] host 1.2.3.4 (real name)");
}

/*
 * Tests functions:
 *   irc_protocol_cb_328 (channel URL)
 */

TEST(IrcProtocolWithServer, 328)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 328");
    CHECK_ERROR_PARAMS("328", 0, 3);
    RECV(":server 328 alice");
    CHECK_ERROR_PARAMS("328", 1, 3);
    RECV(":server 328 alice #test");
    CHECK_ERROR_PARAMS("328", 2, 3);

    RECV(":server 328 alice #test :https://example.com/");
    CHECK_CHAN("-- URL for #test: https://example.com/");
    RECV(":server 328 alice #test :URL is https://example.com/");
    CHECK_CHAN("-- URL for #test: URL is https://example.com/");

}

/*
 * Tests functions:
 *   irc_protocol_cb_329 (channel creation date)
 */

TEST(IrcProtocolWithServer, 329)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 329");
    CHECK_ERROR_PARAMS("329", 0, 3);
    RECV(":server 329 alice");
    CHECK_ERROR_PARAMS("329", 1, 3);
    RECV(":server 329 alice #test");
    CHECK_ERROR_PARAMS("329", 2, 3);

    RECV(":server 329 alice #test 1205327894");
    CHECK_CHAN("-- Channel created on Wed, 12 Mar 2008 13:18:14");
    RECV(":server 329 alice #test :1205327894");
    CHECK_CHAN("-- Channel created on Wed, 12 Mar 2008 13:18:14");

    /* channel not found */
    RECV(":server 329 alice #xyz 1205327894");
    CHECK_SRV("-- Channel #xyz created on Wed, 12 Mar 2008 13:18:14");
    RECV(":server 329 alice #xyz :1205327894");
    CHECK_SRV("-- Channel #xyz created on Wed, 12 Mar 2008 13:18:14");
}

/*
 * Tests functions:
 *   irc_protocol_cb_330 (whois, is logged in as)
 *   irc_protocol_cb_343 (whois, is opered as)
 */

TEST(IrcProtocolWithServer, 330_343)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 330");
    CHECK_ERROR_PARAMS("330", 0, 3);
    RECV(":server 330 alice");
    CHECK_ERROR_PARAMS("330", 1, 3);
    RECV(":server 330 alice bob");
    CHECK_ERROR_PARAMS("330", 2, 3);

    /* not enough parameters */
    RECV(":server 343");
    CHECK_ERROR_PARAMS("343", 0, 3);
    RECV(":server 343 alice");
    CHECK_ERROR_PARAMS("343", 1, 3);
    RECV(":server 343 alice bob");
    CHECK_ERROR_PARAMS("343", 2, 3);

    RECV(":server 330 alice bob bob2");
    CHECK_SRV("-- [bob] bob2");
    RECV(":server 330 alice bob bob2 :is logged in as");
    CHECK_SRV("-- [bob] is logged in as bob2");

    RECV(":server 343 alice bob bob2");
    CHECK_SRV("-- [bob] bob2");
    RECV(":server 343 alice bob bob2 :is opered as");
    CHECK_SRV("-- [bob] is opered as bob2");
}

/*
 * Tests functions:
 *   irc_protocol_cb_331 (no topic for channel)
 */

TEST(IrcProtocolWithServer, 331)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 331");
    CHECK_ERROR_PARAMS("331", 0, 2);
    RECV(":server 331 alice");
    CHECK_ERROR_PARAMS("331", 1, 2);

    RECV(":server 331 alice #test");
    CHECK_CHAN("-- No topic set for channel #test");

    /* channel not found */
    RECV(":server 331 alice #xyz");
    CHECK_SRV("-- No topic set for channel #xyz");
}

/*
 * Tests functions:
 *   irc_protocol_cb_332 (topic of channel)
 */

TEST(IrcProtocolWithServer, 332)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 332");
    CHECK_ERROR_PARAMS("332", 0, 2);
    RECV(":server 332 alice");
    CHECK_ERROR_PARAMS("332", 1, 2);

    POINTERS_EQUAL(NULL, ptr_server->channels->topic);

    RECV(":server 332 alice #test");
    CHECK_CHAN("-- Topic for #test is \"\"");

    RECV(":server 332 alice #test :the new topic ");
    CHECK_CHAN("-- Topic for #test is \"the new topic \"");
    STRCMP_EQUAL("the new topic ", ptr_server->channels->topic);
}

/*
 * Tests functions:
 *   irc_protocol_cb_333 (infos about topic (nick / date))
 */

TEST(IrcProtocolWithServer, 333)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 333");
    CHECK_ERROR_PARAMS("333", 0, 3);
    RECV(":server 333 alice");
    CHECK_ERROR_PARAMS("333", 1, 3);
    RECV(":server 333 alice #test");
    CHECK_ERROR_PARAMS("333", 2, 3);

    RECV(":server 333 alice #test nick!user@host");
    CHECK_NO_MSG;
    RECV(":server 333 alice #test nick!user@host 1205428096");
    CHECK_CHAN("-- Topic set by nick (user@host) on Thu, 13 Mar 2008 17:08:16");
    RECV(":server 333 alice #test 1205428096");
    CHECK_CHAN("-- Topic set on Thu, 13 Mar 2008 17:08:16");

    /* channel not found */
    RECV(":server 333 alice #xyz nick!user@host");
    CHECK_NO_MSG;
    RECV(":server 333 alice #xyz nick!user@host 1205428096");
    CHECK_SRV("-- Topic for #xyz set by nick (user@host) on "
              "Thu, 13 Mar 2008 17:08:16");
    RECV(":server 333 alice #xyz 1205428096");
    CHECK_SRV("-- Topic for #xyz set on Thu, 13 Mar 2008 17:08:16");
}

/*
 * Tests functions:
 *   irc_protocol_cb_338 (whois, host)
 */

TEST(IrcProtocolWithServer, 338)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 338");
    CHECK_ERROR_PARAMS("338", 0, 3);
    RECV(":server 338 alice");
    CHECK_ERROR_PARAMS("338", 1, 3);
    RECV(":server 338 alice bob");
    CHECK_ERROR_PARAMS("338", 2, 3);

    RECV(":server 338 alice bob hostname :actually using host");
    CHECK_SRV("-- [bob] actually using host hostname");

    /* on Rizon server */
    RECV(":server 338 alice bob :is actually bob@example.com [1.2.3.4]");
    CHECK_SRV("-- [bob] is actually bob@example.com [1.2.3.4]");
}

/*
 * Tests functions:
 *   irc_protocol_cb_341 (inviting)
 */

TEST(IrcProtocolWithServer, 341)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 341");
    CHECK_ERROR_PARAMS("341", 0, 3);
    RECV(":server 341 alice");
    CHECK_ERROR_PARAMS("341", 1, 3);
    RECV(":server 341 alice bob");
    CHECK_ERROR_PARAMS("341", 2, 3);

    RECV(":server 341 alice bob #test");
    CHECK_SRV("-- alice has invited bob to #test");
    RECV(":server 341 alice bob :#test");
    CHECK_SRV("-- alice has invited bob to #test");
}

/*
 * Tests functions:
 *   irc_protocol_cb_344 (channel reop)
 */

TEST(IrcProtocolWithServer, 344)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 344");
    CHECK_ERROR_PARAMS("344", 0, 3);
    RECV(":server 344 alice");
    CHECK_ERROR_PARAMS("344", 1, 3);
    RECV(":server 344 alice #test");
    CHECK_ERROR_PARAMS("344", 2, 3);

    /* channel reop (IRCnet) */
    RECV(":server 344 alice #test nick!user@host");
    CHECK_SRV("-- Channel reop #test: nick!user@host");
    RECV(":server 344 alice #test :nick!user@host");
    CHECK_SRV("-- Channel reop #test: nick!user@host");

    /* channel reop (IRCnet), channel not found */
    RECV(":server 344 alice #xyz nick!user@host");
    CHECK_SRV("-- Channel reop #xyz: nick!user@host");
    RECV(":server 344 alice #xyz :nick!user@host");
    CHECK_SRV("-- Channel reop #xyz: nick!user@host");

    /* whois, geo info (UnrealIRCd) */
    RECV(":server 344 alice bob FR :is connecting from France");
    CHECK_SRV("-- [bob] FR is connecting from France");
}

/*
 * Tests functions:
 *   irc_protocol_cb_345 (end of channel reop)
 */

TEST(IrcProtocolWithServer, 345)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 345");
    CHECK_ERROR_PARAMS("345", 0, 3);
    RECV(":server 345 alice");
    CHECK_ERROR_PARAMS("345", 1, 3);
    RECV(":server 345 alice #test");
    CHECK_ERROR_PARAMS("345", 2, 3);

    RECV(":server 345 alice #test end");
    CHECK_SRV("-- #test: end");
    RECV(":server 345 alice #test :End of Channel Reop List");
    CHECK_SRV("-- #test: End of Channel Reop List");

    /* channel not found */
    RECV(":server 345 alice #xyz end");
    CHECK_SRV("-- #xyz: end");
    RECV(":server 345 alice #xyz :End of Channel Reop List");
    CHECK_SRV("-- #xyz: End of Channel Reop List");
}

/*
 * Tests functions:
 *   irc_protocol_cb_346 (channel invite list)
 */

TEST(IrcProtocolWithServer, 346)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 346");
    CHECK_ERROR_PARAMS("346", 0, 3);
    RECV(":server 346 alice");
    CHECK_ERROR_PARAMS("346", 1, 3);
    RECV(":server 346 alice #test");
    CHECK_ERROR_PARAMS("346", 2, 3);

    RECV(":server 346 alice #test invitemask");
    CHECK_CHAN("-- [#test] [1] invitemask invited");
    RECV(":server 346 alice #test invitemask nick!user@host");
    CHECK_CHAN("-- [#test] [2] invitemask invited by nick (user@host)");
    RECV(":server 346 alice #test invitemask nick!user@host 1205590879");
    CHECK_CHAN("-- [#test] [3] invitemask invited by nick (user@host) "
              "on Sat, 15 Mar 2008 14:21:19");

    /* channel not found */
    RECV(":server 346 alice #xyz invitemask");
    CHECK_SRV("-- [#xyz] invitemask invited");
    RECV(":server 346 alice #xyz invitemask nick!user@host");
    CHECK_SRV("-- [#xyz] invitemask invited by nick (user@host)");
    RECV(":server 346 alice #xyz invitemask nick!user@host 1205590879");
    CHECK_SRV("-- [#xyz] invitemask invited by nick (user@host) "
              "on Sat, 15 Mar 2008 14:21:19");
}

/*
 * Tests functions:
 *   irc_protocol_cb_347 (end of channel invite list)
 */

TEST(IrcProtocolWithServer, 347)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 347");
    CHECK_ERROR_PARAMS("347", 0, 2);
    RECV(":server 347 alice");
    CHECK_ERROR_PARAMS("347", 1, 2);

    RECV(":server 347 alice #test");
    CHECK_CHAN("-- [#test]");
    RECV(":server 347 alice #test end");
    CHECK_CHAN("-- [#test] end");
    RECV(":server 347 alice #test :End of Channel Invite List");
    CHECK_CHAN("-- [#test] End of Channel Invite List");

    /* channel not found */
    RECV(":server 347 alice #xyz");
    CHECK_SRV("-- [#xyz]");
    RECV(":server 347 alice #xyz end");
    CHECK_SRV("-- [#xyz] end");
    RECV(":server 347 alice #xyz :End of Channel Invite List");
    CHECK_SRV("-- [#xyz] End of Channel Invite List");
}

/*
 * Tests functions:
 *   irc_protocol_cb_348 (channel exception list)
 */

TEST(IrcProtocolWithServer, 348)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 348");
    CHECK_ERROR_PARAMS("348", 0, 3);
    RECV(":server 348 alice");
    CHECK_ERROR_PARAMS("348", 1, 3);
    RECV(":server 348 alice #test");
    CHECK_ERROR_PARAMS("348", 2, 3);

    RECV(":server 348 alice #test nick1!user1@host1");
    CHECK_CHAN("-- [#test] [1] exception nick1!user1@host1");
    RECV(":server 348 alice #test nick1!user1@host1 nick2!user2@host2");
    CHECK_CHAN("-- [#test] [2] exception nick1!user1@host1 "
               "by nick2 (user2@host2)");
    RECV(":server 348 alice #test nick1!user1@host1 nick2!user2@host2 "
         "1205585109");
    CHECK_CHAN("-- [#test] [3] exception nick1!user1@host1 "
               "by nick2 (user2@host2) on Sat, 15 Mar 2008 12:45:09");

    /* channel not found */
    RECV(":server 348 alice #xyz nick1!user1@host1");
    CHECK_SRV("-- [#xyz] exception nick1!user1@host1");
    RECV(":server 348 alice #xyz nick1!user1@host1 nick2!user2@host2");
    CHECK_SRV("-- [#xyz] exception nick1!user1@host1 "
              "by nick2 (user2@host2)");
    RECV(":server 348 alice #xyz nick1!user1@host1 nick2!user2@host2 "
         "1205585109");
    CHECK_SRV("-- [#xyz] exception nick1!user1@host1 "
              "by nick2 (user2@host2) on Sat, 15 Mar 2008 12:45:09");
}

/*
 * Tests functions:
 *   irc_protocol_cb_349 (end of channel exception list)
 */

TEST(IrcProtocolWithServer, 349)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 349");
    CHECK_ERROR_PARAMS("349", 0, 2);
    RECV(":server 349 alice");
    CHECK_ERROR_PARAMS("349", 1, 2);

    RECV(":server 349 alice #test");
    CHECK_CHAN("-- [#test]");
    RECV(":server 349 alice #test end");
    CHECK_CHAN("-- [#test] end");
    RECV(":server 349 alice #test :End of Channel Exception List");
    CHECK_CHAN("-- [#test] End of Channel Exception List");

    /* channel not found */
    RECV(":server 349 alice #xyz");
    CHECK_SRV("-- [#xyz]");
    RECV(":server 349 alice #xyz end");
    CHECK_SRV("-- [#xyz] end");
    RECV(":server 349 alice #xyz :End of Channel Exception List");
    CHECK_SRV("-- [#xyz] End of Channel Exception List");
}


/*
 * Tests functions:
 *   irc_protocol_cb_350 (whois, gateway)
 */

TEST(IrcProtocolWithServer, 350)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 350");
    CHECK_ERROR_PARAMS("350", 0, 2);
    RECV(":server 350 alice");
    CHECK_ERROR_PARAMS("350", 1, 2);

    /* non-standard parameters (using whois_nick_msg callback) */
    RECV(":server 350 alice bob :something here");
    CHECK_SRV("-- [bob] something here");
    RECV(":server 350 alice bob * :something here");
    CHECK_SRV("-- [bob] * something here");

    /* non-standard parameters (using default whois callback) */
    RECV(":server 350 alice bob");
    CHECK_SRV("-- bob");

    /* standard parameters */
    RECV(":server 350 alice bob * * :is connected via the WebIRC gateway");
    CHECK_SRV("-- [bob] is connected via the WebIRC gateway");
    RECV(":server 350 alice bob example.com * :is connected via the WebIRC gateway");
    CHECK_SRV("-- [bob] (example.com) is connected via the WebIRC gateway");
    RECV(":server 350 alice bob * 1.2.3.4 :is connected via the WebIRC gateway");
    CHECK_SRV("-- [bob] (1.2.3.4) is connected via the WebIRC gateway");
    RECV(":server 350 alice bob example.com 1.2.3.4 :is connected via the WebIRC gateway");
    CHECK_SRV("-- [bob] (example.com, 1.2.3.4) is connected via the WebIRC gateway");
}

/*
 * Tests functions:
 *   irc_protocol_cb_351 (server version)
 */

TEST(IrcProtocolWithServer, 351)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 351");
    CHECK_ERROR_PARAMS("351", 0, 3);
    RECV(":server 351 alice");
    CHECK_ERROR_PARAMS("351", 1, 3);
    RECV(":server 351 alice dancer-ircd-1.0");
    CHECK_ERROR_PARAMS("351", 2, 3);

    RECV(":server 351 alice dancer-ircd-1.0 server");
    CHECK_SRV("-- dancer-ircd-1.0 server");
    RECV(":server 351 alice dancer-ircd-1.0 server :iMZ dncrTS/v4");
    CHECK_SRV("-- dancer-ircd-1.0 server (iMZ dncrTS/v4)");
}

/*
 * Tests functions:
 *   irc_protocol_cb_352 (who)
 */

TEST(IrcProtocolWithServer, 352)
{
    struct t_irc_nick *ptr_nick, *ptr_nick2;

    SRV_INIT_JOIN2;

    /* not enough parameters */
    RECV(":server 352");
    CHECK_ERROR_PARAMS("352", 0, 3);
    RECV(":server 352 alice");
    CHECK_ERROR_PARAMS("352", 1, 3);
    RECV(":server 352 alice #test");
    CHECK_ERROR_PARAMS("352", 2, 3);

    /* not enough parameters, but silently ignored */
    RECV(":server 352 alice #test user");
    CHECK_NO_MSG;
    RECV(":server 352 alice #test user host");
    CHECK_NO_MSG;
    RECV(":server 352 alice #test user host server");
    CHECK_NO_MSG;

    ptr_nick = ptr_server->channels->nicks;
    ptr_nick2 = ptr_server->channels->last_nick;

    STRCMP_EQUAL("user_a@host_a", ptr_nick->host);
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick->away);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick->realname);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 352 alice #test user2 host2 server bob");
    CHECK_SRV("-- [#test] bob (user2@host2) ()");
    STRCMP_EQUAL("user2@host2", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 352 alice #test user3 host3 server bob *");
    CHECK_SRV("-- [#test] bob (user3@host3) * ()");
    STRCMP_EQUAL("user3@host3", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 352 alice #test user4 host4 server bob * :0 real name 1");
    CHECK_SRV("-- [#test] bob (user4@host4) * 0 (real name 1)");
    STRCMP_EQUAL("user4@host4", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("real name 1", ptr_nick2->realname);

    RECV(":server 352 alice #test user5 host5 server bob H@ :0 real name 2");
    CHECK_SRV("-- [#test] bob (user5@host5) H@ 0 (real name 2)");
    STRCMP_EQUAL("user5@host5", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("real name 2", ptr_nick2->realname);

    RECV(":server 352 alice #test user6 host6 server bob G@ :0 real name 3");
    CHECK_SRV("-- [#test] bob (user6@host6) G@ 0 (real name 3)");
    STRCMP_EQUAL("user6@host6", ptr_nick2->host);
    LONGS_EQUAL(1, ptr_nick2->away);
    STRCMP_EQUAL("real name 3", ptr_nick2->realname);

    RECV(":server 352 alice #test user7 host7 server bob * :0 real name 4");
    CHECK_SRV("-- [#test] bob (user7@host7) * 0 (real name 4)");
    STRCMP_EQUAL("user7@host7", ptr_nick2->host);
    LONGS_EQUAL(1, ptr_nick2->away);
    STRCMP_EQUAL("real name 4", ptr_nick2->realname);

    RECV(":server 352 alice #test user8 host8 server bob H@ :0 real name 5");
    CHECK_SRV("-- [#test] bob (user8@host8) H@ 0 (real name 5)");
    STRCMP_EQUAL("user8@host8", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("real name 5", ptr_nick2->realname);

    RECV(":server 352 alice #test user8 host8 server bob H@ :0");
    CHECK_SRV("-- [#test] bob (user8@host8) H@ 0 ()");
    STRCMP_EQUAL("user8@host8", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("real name 5", ptr_nick2->realname);

    /* nothing should have changed in the first nick */
    STRCMP_EQUAL("user_a@host_a", ptr_nick->host);
    LONGS_EQUAL(0, ptr_nick->away);
    POINTERS_EQUAL(NULL, ptr_nick->realname);

    /* channel not found */
    RECV(":server 352 alice #xyz user");
    CHECK_NO_MSG;
    RECV(":server 352 alice #xyz user host");
    CHECK_NO_MSG;
    RECV(":server 352 alice #xyz user host server");
    CHECK_NO_MSG;
    RECV(":server 352 alice #xyz user host server bob");
    CHECK_SRV("-- [#xyz] bob (user@host) ()");
    RECV(":server 352 alice #xyz user host server bob *");
    CHECK_SRV("-- [#xyz] bob (user@host) * ()");
    RECV(":server 352 alice #xyz user host server bob * :0 nick");
    CHECK_SRV("-- [#xyz] bob (user@host) * 0 (nick)");
    RECV(":server 352 alice #xyz user host server bob H@ :0 nick");
    CHECK_SRV("-- [#xyz] bob (user@host) H@ 0 (nick)");
    RECV(":server 352 alice #xyz user host server bob G@ :0 nick");
    CHECK_SRV("-- [#xyz] bob (user@host) G@ 0 (nick)");
}

/*
 * Tests functions:
 *   irc_protocol_cb_353 (list of users on a channel)
 */

TEST(IrcProtocolWithServer, 353)
{
    struct t_irc_channel *ptr_channel;

    SRV_INIT_JOIN2;

    ptr_channel = ptr_server->channels;

    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick->next_nick);

    /* not enough parameters */
    RECV(":server 353");
    CHECK_ERROR_PARAMS("353", 0, 3);
    RECV(":server 353 alice");
    CHECK_ERROR_PARAMS("353", 1, 3);
    RECV(":server 353 alice #test");
    CHECK_ERROR_PARAMS("353", 2, 3);
    RECV(":server 353 alice =");
    CHECK_ERROR_PARAMS("353", 2, 3);
    RECV(":server 353 alice = #test");
    CHECK_ERROR_PARSE("353", ":server 353 alice = #test");

    RECV(":server 353 alice #test :alice");
    CHECK_NO_MSG;
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick->next_nick);

    RECV(":server 353 alice #test :alice bob @carol +dan!user@host");
    CHECK_NO_MSG;
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);
    STRCMP_EQUAL("carol", ptr_channel->nicks->next_nick->next_nick->name);
    STRCMP_EQUAL("@", ptr_channel->nicks->next_nick->next_nick->prefix);
    STRCMP_EQUAL("dan",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->name);
    STRCMP_EQUAL("+",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->prefix);
    STRCMP_EQUAL("user@host",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->host);
    POINTERS_EQUAL(NULL,
                   ptr_channel->nicks->next_nick->next_nick->next_nick->next_nick);

    RECV(":server 353 alice = #test :alice");
    CHECK_NO_MSG;
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);
    STRCMP_EQUAL("carol", ptr_channel->nicks->next_nick->next_nick->name);
    STRCMP_EQUAL("@", ptr_channel->nicks->next_nick->next_nick->prefix);
    STRCMP_EQUAL("dan",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->name);
    STRCMP_EQUAL("+",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->prefix);
    STRCMP_EQUAL("user@host",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->host);
    POINTERS_EQUAL(NULL,
                   ptr_channel->nicks->next_nick->next_nick->next_nick->next_nick);

    RECV(":server 353 alice = #test :alice bob @carol +dan!user@host");
    CHECK_NO_MSG;
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);
    STRCMP_EQUAL("carol", ptr_channel->nicks->next_nick->next_nick->name);
    STRCMP_EQUAL("@", ptr_channel->nicks->next_nick->next_nick->prefix);
    STRCMP_EQUAL("dan",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->name);
    STRCMP_EQUAL("+",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->prefix);
    STRCMP_EQUAL("user@host",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->host);
    POINTERS_EQUAL(NULL,
                   ptr_channel->nicks->next_nick->next_nick->next_nick->next_nick);

    /* with option irc.look.color_nicks_in_names enabled */
    config_file_option_set (irc_config_look_color_nicks_in_names, "on", 1);
    RECV(":server 353 alice = #test :alice bob @carol +dan!user@host");
    config_file_option_unset (irc_config_look_color_nicks_in_names);

    /* channel not found */
    RECV(":server 353 alice #xyz :alice");
    CHECK_SRV("-- Nicks #xyz: [alice]");
    RECV(":server 353 alice #xyz :alice bob @carol +dan!user@host");
    CHECK_SRV("-- Nicks #xyz: [alice bob @carol +dan]");

    /* channel not found */
    RECV(":server 353 alice = #xyz :alice");
    CHECK_SRV("-- Nicks #xyz: [alice]");
    RECV(":server 353 alice = #xyz :alice bob @carol +dan!user@host");
    CHECK_SRV("-- Nicks #xyz: [alice bob @carol +dan]");
}

/*
 * Tests functions:
 *   irc_protocol_cb_354 (WHOX output)
 */

TEST(IrcProtocolWithServer, 354)
{
    struct t_irc_nick *ptr_nick, *ptr_nick2;

    /* assume "account-notify" capability is enabled in server */
    hashtable_set (ptr_server->cap_list, "account-notify", NULL);

    SRV_INIT_JOIN2;

    /* not enough parameters */
    RECV(":server 354");
    CHECK_ERROR_PARAMS("354", 0, 2);
    RECV(":server 354 alice");
    CHECK_ERROR_PARAMS("354", 1, 2);

    ptr_nick = ptr_server->channels->nicks;
    ptr_nick2 = ptr_server->channels->last_nick;

    STRCMP_EQUAL("user_a@host_a", ptr_nick->host);
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick->away);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick->account);
    POINTERS_EQUAL(NULL, ptr_nick2->account);
    POINTERS_EQUAL(NULL, ptr_nick->realname);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test");
    CHECK_SRV("-- [#test]");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick2->account);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2");
    CHECK_SRV("-- [#test] user2");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick2->account);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 :trailing parameter");
    CHECK_SRV("-- [#test] user2 trailing parameter");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick2->account);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2");
    CHECK_SRV("-- [#test] user2 host2");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick2->account);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2 server");
    CHECK_SRV("-- [#test] user2 host2 server");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick2->account);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2 server bob");
    CHECK_SRV("-- [#test] user2 host2 server bob");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick2->account);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2 server bob *");
    CHECK_SRV("-- [#test] user2 host2 server bob *");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick2->account);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2 server bob H@ 0");
    CHECK_SRV("-- [#test] user2 host2 server bob H@ 0");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    POINTERS_EQUAL(NULL, ptr_nick2->account);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2 server bob * 0 account2");
    CHECK_SRV("-- [#test] bob [account2] (user2@host2) * 0 ()");
    STRCMP_EQUAL("user2@host2", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("account2", ptr_nick2->account);
    POINTERS_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user3 host3 server bob * 0 account3 "
         ":real name 2");
    CHECK_SRV("-- [#test] bob [account3] (user3@host3) * 0 (real name 2)");
    STRCMP_EQUAL("user3@host3", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("account3", ptr_nick2->account);
    STRCMP_EQUAL("real name 2", ptr_nick2->realname);

    RECV(":server 354 alice #test user4 host4 server bob H@ 0 account4 "
         ":real name 3");
    CHECK_SRV("-- [#test] bob [account4] (user4@host4) H@ 0 (real name 3)");
    STRCMP_EQUAL("user4@host4", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("account4", ptr_nick2->account);
    STRCMP_EQUAL("real name 3", ptr_nick2->realname);

    RECV(":server 354 alice #test user5 host5 server bob G@ 0 account5 "
         ":real name 4");
    CHECK_SRV("-- [#test] bob [account5] (user5@host5) G@ 0 (real name 4)");
    STRCMP_EQUAL("user5@host5", ptr_nick2->host);
    LONGS_EQUAL(1, ptr_nick2->away);
    STRCMP_EQUAL("account5", ptr_nick2->account);
    STRCMP_EQUAL("real name 4", ptr_nick2->realname);

    RECV(":server 354 alice #test user6 host6 server bob * 0 account6 "
         ":real name 5");
    CHECK_SRV("-- [#test] bob [account6] (user6@host6) * 0 (real name 5)");
    STRCMP_EQUAL("user6@host6", ptr_nick2->host);
    LONGS_EQUAL(1, ptr_nick2->away);
    STRCMP_EQUAL("account6", ptr_nick2->account);
    STRCMP_EQUAL("real name 5", ptr_nick2->realname);

    RECV(":server 354 alice #test user7 host7 server bob H@ 0 account7 "
         ":real name 6");
    CHECK_SRV("-- [#test] bob [account7] (user7@host7) H@ 0 (real name 6)");
    STRCMP_EQUAL("user7@host7", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("account7", ptr_nick2->account);
    STRCMP_EQUAL("real name 6", ptr_nick2->realname);

    /* nothing should have changed in the first nick */
    STRCMP_EQUAL("user_a@host_a", ptr_nick->host);
    LONGS_EQUAL(0, ptr_nick->away);
    POINTERS_EQUAL(NULL, ptr_nick->account);
    POINTERS_EQUAL(NULL, ptr_nick->realname);

    /* channel not found */
    RECV(":server 354 alice #xyz");
    CHECK_SRV("-- [#xyz]");
    RECV(":server 354 alice #xyz user2");
    CHECK_SRV("-- [#xyz] user2");
    RECV(":server 354 alice #xyz user2 host2");
    CHECK_SRV("-- [#xyz] user2 host2");
    RECV(":server 354 alice #xyz user2 host2 server");
    CHECK_SRV("-- [#xyz] user2 host2 server");
    RECV(":server 354 alice #xyz user2 host2 server bob");
    CHECK_SRV("-- [#xyz] user2 host2 server bob");
    RECV(":server 354 alice #xyz user2 host2 server bob *");
    CHECK_SRV("-- [#xyz] user2 host2 server bob *");
    RECV(":server 354 alice #xyz user2 host2 server bob G@ 0");
    CHECK_SRV("-- [#xyz] user2 host2 server bob G@ 0");
    RECV(":server 354 alice #xyz user2 host2 server bob H@ 0 account");
    CHECK_SRV("-- [#xyz] bob [account] (user2@host2) H@ 0 ()");
    RECV(":server 354 alice #xyz user2 host2 server bob G@ 0 account "
         ":real name");
    CHECK_SRV("-- [#xyz] bob [account] (user2@host2) G@ 0 (real name)");
}

/*
 * Tests functions:
 *   irc_protocol_cb_366 (end of /names list)
 */

TEST(IrcProtocolWithServer, 366)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 366");
    CHECK_ERROR_PARAMS("366", 0, 3);
    RECV(":server 366 alice");
    CHECK_ERROR_PARAMS("366", 1, 3);
    RECV(":server 366 alice #test");
    CHECK_ERROR_PARAMS("366", 2, 3);

    RECV(":server 366 alice #test end");
    CHECK_CHAN("-- Channel #test: 1 nick (0 ops, 0 voices, 1 normal)");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("-- Channel #test: 1 nick (0 ops, 0 voices, 1 normal)");

    RECV(":server 353 alice = #test :bob");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("-- Channel #test: 2 nicks (0 ops, 0 voices, 2 normals)");

    RECV(":server 353 alice = #test :@carol");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("-- Channel #test: 3 nicks (1 op, 0 voices, 2 normals)");

    RECV(":server 353 alice = #test :+dan!user@host");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("-- Channel #test: 4 nicks (1 op, 1 voice, 2 normals)");

    RECV(":server 353 alice = #test :@evans");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("-- Channel #test: 5 nicks (2 ops, 1 voice, 2 normals)");

    RECV(":server 353 alice = #test :+fred");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("-- Channel #test: 6 nicks (2 ops, 2 voices, 2 normals)");

    RECV(":server 353 alice = #test :greg");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("-- Channel #test: 7 nicks (2 ops, 2 voices, 3 normals)");

    /* channel not found */
    RECV(":server 366 alice #xyz end");
    CHECK_SRV("-- #xyz: end");
    RECV(":server 366 alice #xyz :End of /NAMES list");
    CHECK_SRV("-- #xyz: End of /NAMES list");
}

/*
 * Tests functions:
 *   irc_protocol_cb_367 (banlist)
 */

TEST(IrcProtocolWithServer, 367)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 367");
    CHECK_ERROR_PARAMS("367", 0, 3);
    RECV(":server 367 alice");
    CHECK_ERROR_PARAMS("367", 1, 3);
    RECV(":server 367 alice #test");
    CHECK_ERROR_PARAMS("367", 2, 3);

    RECV(":server 367 alice #test nick1!user1@host1");
    CHECK_CHAN("-- [#test] [1] nick1!user1@host1 banned");
    RECV(":server 367 alice #test nick1!user1@host1 nick2!user2@host2");
    CHECK_CHAN("-- [#test] [2] nick1!user1@host1 banned "
               "by nick2 (user2@host2)");
    RECV(":server 367 alice #test nick1!user1@host1 nick2!user2@host2 "
         "1205585109");
    CHECK_CHAN("-- [#test] [3] nick1!user1@host1 banned "
               "by nick2 (user2@host2) on Sat, 15 Mar 2008 12:45:09");

    /* channel not found */
    RECV(":server 367 alice #xyz nick1!user1@host1");
    CHECK_SRV("-- [#xyz] nick1!user1@host1 banned");
    RECV(":server 367 alice #xyz nick1!user1@host1 nick2!user2@host2");
    CHECK_SRV("-- [#xyz] nick1!user1@host1 banned by nick2 (user2@host2)");
    RECV(":server 367 alice #xyz nick1!user1@host1 nick2!user2@host2 "
         "1205585109");
    CHECK_SRV("-- [#xyz] nick1!user1@host1 banned by nick2 (user2@host2) "
              "on Sat, 15 Mar 2008 12:45:09");
}

/*
 * Tests functions:
 *   irc_protocol_cb_368 (end of banlist)
 */

TEST(IrcProtocolWithServer, 368)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 368");
    CHECK_ERROR_PARAMS("368", 0, 2);
    RECV(":server 368 alice");
    CHECK_ERROR_PARAMS("368", 1, 2);

    RECV(":server 368 alice #test");
    CHECK_CHAN("-- [#test]");
    RECV(":server 368 alice #test end");
    CHECK_CHAN("-- [#test] end");
    RECV(":server 368 alice #test :End of Channel Ban List");
    CHECK_CHAN("-- [#test] End of Channel Ban List");

    /* channel not found */
    RECV(":server 368 alice #xyz");
    CHECK_SRV("-- [#xyz]");
    RECV(":server 368 alice #xyz end");
    CHECK_SRV("-- [#xyz] end");
    RECV(":server 368 alice #xyz :End of Channel Ban List");
    CHECK_SRV("-- [#xyz] End of Channel Ban List");
}

/*
 * Tests functions:
 *   irc_protocol_cb_401 (no such nick/channel)
 */

TEST(IrcProtocolWithServer, 401)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 401");
    CHECK_ERROR_PARAMS("401", 0, 2);
    RECV(":server 401 alice");
    CHECK_ERROR_PARAMS("401", 1, 2);

    RECV(":server 401 alice bob");
    CHECK_SRV("-- bob");
    RECV(":server 401 alice bob :No such nick/channel");
    CHECK_SRV("-- bob: No such nick/channel");
}

/*
 * Tests functions:
 *   irc_protocol_cb_402 (no such server)
 */

TEST(IrcProtocolWithServer, 402)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 402");
    CHECK_ERROR_PARAMS("402", 0, 2);
    RECV(":server 402 alice");
    CHECK_ERROR_PARAMS("402", 1, 2);

    RECV(":server 402 alice server");
    CHECK_SRV("-- server");
    RECV(":server 402 alice server :No such server");
    CHECK_SRV("-- server: No such server");
}

/*
 * Tests functions:
 *   irc_protocol_cb_404 (cannot send to channel)
 */

TEST(IrcProtocolWithServer, 404)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 404");
    CHECK_ERROR_PARAMS("404", 0, 2);
    RECV(":server 404 alice");
    CHECK_ERROR_PARAMS("404", 1, 2);

    RECV(":server 404 alice #test");
    CHECK_SRV("-- #test");
    RECV(":server 404 alice #test :Cannot send to channel");
    CHECK_CHAN("-- #test: Cannot send to channel");
    RECV(":server 404 alice #test2 :Cannot send to channel");
    CHECK_SRV("-- #test2: Cannot send to channel");
}

/*
 * Tests functions:
 *   irc_protocol_cb_432 (erroneous nickname, not connected)
 */

TEST(IrcProtocolWithServer, 432_not_connected)
{
    RECV(":server 432 * alice error");
    CHECK_SRV("-- * alice error");
    CHECK_SRV("=!= irc: nickname \"nick1\" is invalid, "
              "trying nickname \"nick2\"");

    RECV(":server 432 * :alice error");
    CHECK_SRV("-- * alice error");
    CHECK_SRV("=!= irc: nickname \"nick2\" is invalid, "
              "trying nickname \"nick3\"");

    RECV(":server 432 * alice :Erroneous Nickname");
    CHECK_SRV("-- * alice Erroneous Nickname");
    CHECK_SRV("=!= irc: nickname \"nick3\" is invalid, "
              "trying nickname \"nick1_\"");

    RECV(":server 432 * alice1 :Erroneous Nickname");
    CHECK_SRV("-- * alice1 Erroneous Nickname");
    CHECK_SRV("=!= irc: nickname \"nick1_\" is invalid, "
              "trying nickname \"nick1__\"");
}

/*
 * Tests functions:
 *   irc_protocol_cb_432 (erroneous nickname, connected)
 */

TEST(IrcProtocolWithServer, 432_connected)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 432");
    CHECK_ERROR_PARAMS("432", 0, 2);
    RECV(":server 432 alice");
    CHECK_ERROR_PARAMS("432", 1, 2);

    RECV(":server 432 * alice");
    CHECK_SRV("-- * alice");
    RECV(":server 432 * alice error");
    CHECK_SRV("-- * alice error");
    RECV(":server 432 * alice :Erroneous Nickname");
    CHECK_SRV("-- * alice Erroneous Nickname");
}

/*
 * Tests functions:
 *   irc_protocol_cb_433 (nickname already in use, not connected)
 */

TEST(IrcProtocolWithServer, 433_not_connected)
{
    RECV(":server 433 * alice error");
    CHECK_SRV("-- irc: nickname \"nick1\" is already in use, "
              "trying nickname \"nick2\"");

    RECV(":server 433 * alice :Nickname is already in use.");
    CHECK_SRV("-- irc: nickname \"nick2\" is already in use, "
              "trying nickname \"nick3\"");

    RECV(":server 433 * alice1 :Nickname is already in use.");
    CHECK_SRV("-- irc: nickname \"nick3\" is already in use, "
              "trying nickname \"nick1_\"");
}

/*
 * Tests functions:
 *   irc_protocol_cb_433 (nickname already in use, connected)
 */

TEST(IrcProtocolWithServer, 433_connected)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 433");
    CHECK_ERROR_PARAMS("433", 0, 2);
    RECV(":server 433 alice");
    CHECK_ERROR_PARAMS("433", 1, 2);

    RECV(":server 433 * alice");
    CHECK_SRV("-- * alice");
    RECV(":server 433 * alice error");
    CHECK_SRV("-- * alice error");
    RECV(":server 433 * alice :Nickname is already in use.");
    CHECK_SRV("-- * alice Nickname is already in use.");
}

/*
 * Tests functions:
 *   irc_protocol_cb_437 (nick/channel temporarily unavailable, not connected)
 */

TEST(IrcProtocolWithServer, 437_not_connected)
{
    RECV(":server 437 * alice error");
    CHECK_SRV("-- * alice error");
    RECV(":server 437 * alice :Nick/channel is temporarily unavailable");
    CHECK_SRV("-- * alice Nick/channel is temporarily unavailable");
    RECV(":server 437 * alice1 :Nick/channel is temporarily unavailable");
    CHECK_SRV("-- * alice1 Nick/channel is temporarily unavailable");
}

/*
 * Tests functions:
 *   irc_protocol_cb_437 (nick/channel temporarily unavailable, connected)
 */

TEST(IrcProtocolWithServer, 437_connected)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 437");
    CHECK_ERROR_PARAMS("437", 0, 2);
    RECV(":server 437 alice");
    CHECK_ERROR_PARAMS("437", 1, 2);

    RECV(":server 437 * alice");
    CHECK_SRV("-- * alice");
    RECV(":server 437 * alice error");
    CHECK_SRV("-- * alice error");
    RECV(":server 437 * alice :Nick/channel is temporarily unavailable");
    CHECK_SRV("-- * alice Nick/channel is temporarily unavailable");
}

/*
 * Tests functions:
 *   irc_protocol_cb_438 (not authorized to change nickname)
 */

TEST(IrcProtocolWithServer, 438)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 438");
    CHECK_ERROR_PARAMS("438", 0, 2);
    RECV(":server 438 alice");
    CHECK_ERROR_PARAMS("438", 1, 2);

    RECV(":server 438 alice alice2");
    CHECK_SRV("-- alice alice2");
    RECV(":server 438 alice alice2 error");
    CHECK_SRV("-- error (alice => alice2)");
    RECV(":server 438 alice alice2 :Nick change too fast. "
         "Please wait 30 seconds.");
    CHECK_SRV("-- Nick change too fast. Please wait 30 seconds. "
              "(alice => alice2)");
}

/*
 * Tests functions:
 *   irc_protocol_cb_470 (forwarding to another channel)
 */

TEST(IrcProtocolWithServer, 470)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 470");
    CHECK_ERROR_PARAMS("470", 0, 2);
    RECV(":server 470 alice");
    CHECK_ERROR_PARAMS("470", 1, 2);

    RECV(":server 470 alice #test");
    CHECK_SRV("-- #test");
    RECV(":server 470 alice #test #test2");
    CHECK_SRV("-- #test: #test2");
    RECV(":server 470 alice #test #test2 forwarding");
    CHECK_SRV("-- #test: #test2 forwarding");
    RECV(":server 470 alice #test #test2 :Forwarding to another channel");
    CHECK_SRV("-- #test: #test2 Forwarding to another channel");
}

/*
 * Tests functions:
 *   irc_protocol_cb_524 (help not found)
 */

TEST(IrcProtocolWithServer, 524)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 524");
    CHECK_ERROR_PARAMS("524", 0, 2);
    RECV(":server 524 alice");
    CHECK_ERROR_PARAMS("524", 1, 2);

    RECV(":server 524 alice UNKNOWN");
    CHECK_SRV("--");
    RECV(":server 524 alice UNKNOWN :Help not found");
    CHECK_SRV("-- Help not found");
}

/*
 * Tests functions:
 *   irc_protocol_cb_704 (start of HELP/HELPOP)
 */

TEST(IrcProtocolWithServer, 704)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 704");
    CHECK_ERROR_PARAMS("704", 0, 2);
    RECV(":server 704 alice");
    CHECK_ERROR_PARAMS("704", 1, 2);

    RECV(":server 704 alice MODE");
    CHECK_SRV("--");
    RECV(":server 704 alice MODE "
         ":MODE <target> [<modestring> [<mode arguments>...]]");
    CHECK_SRV("-- MODE <target> [<modestring> [<mode arguments>...]]");
}

/*
 * Tests functions:
 *   irc_protocol_cb_705 (body of HELP/HELPOP)
 */

TEST(IrcProtocolWithServer, 705)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 705");
    CHECK_ERROR_PARAMS("705", 0, 2);
    RECV(":server 705 alice");
    CHECK_ERROR_PARAMS("705", 1, 2);

    RECV(":server 705 alice MODE");
    CHECK_SRV("--");
    RECV(":server 705 alice MODE :Sets and removes modes from the given target.");
    CHECK_SRV("-- Sets and removes modes from the given target.");
}

/*
 * Tests functions:
 *   irc_protocol_cb_706 (end of HELP/HELPOP)
 */

TEST(IrcProtocolWithServer, 706)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 706");
    CHECK_ERROR_PARAMS("706", 0, 2);
    RECV(":server 706 alice");
    CHECK_ERROR_PARAMS("706", 1, 2);

    RECV(":server 706 alice MODE");
    CHECK_SRV("--");
    RECV(":server 706 alice MODE :End of /HELPOP");
    CHECK_SRV("-- End of /HELPOP");
}

/*
 * Tests functions:
 *   irc_protocol_cb_728 (quietlist)
 */

TEST(IrcProtocolWithServer, 728)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 728");
    CHECK_ERROR_PARAMS("728", 0, 4);
    RECV(":server 728 alice");
    CHECK_ERROR_PARAMS("728", 1, 4);
    RECV(":server 728 alice #test");
    CHECK_ERROR_PARAMS("728", 2, 4);
    RECV(":server 728 alice #test q");
    CHECK_ERROR_PARAMS("728", 3, 4);

    RECV(":server 728 alice #test q nick1!user1@host1");
    CHECK_CHAN("-- [#test] nick1!user1@host1 quieted");
    RECV(":server 728 alice #test q nick1!user1@host1 alice!user@host");
    CHECK_CHAN("-- [#test] nick1!user1@host1 quieted by alice (user@host)");
    RECV(":server 728 alice #test q nick1!user1@host1 alice!user@host "
         "1351350090");
    CHECK_CHAN("-- [#test] nick1!user1@host1 quieted by alice (user@host) "
               "on Sat, 27 Oct 2012 15:01:30");

    /* channel not found */
    RECV(":server 728 alice #xyz q nick1!user1@host1");
    CHECK_SRV("-- [#xyz] nick1!user1@host1 quieted");
    RECV(":server 728 alice #xyz q nick1!user1@host1 alice!user@host");
    CHECK_SRV("-- [#xyz] nick1!user1@host1 quieted by alice (user@host)");
    RECV(":server 728 alice #xyz q nick1!user1@host1 alice!user@host "
         "1351350090");
    CHECK_SRV("-- [#xyz] nick1!user1@host1 quieted by alice (user@host) "
              "on Sat, 27 Oct 2012 15:01:30");
}

/*
 * Tests functions:
 *   irc_protocol_cb_729 (end of quietlist)
 */

TEST(IrcProtocolWithServer, 729)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 729");
    CHECK_ERROR_PARAMS("729", 0, 3);
    RECV(":server 729 alice");
    CHECK_ERROR_PARAMS("729", 1, 3);
    RECV(":server 729 alice #test");
    CHECK_ERROR_PARAMS("729", 2, 3);

    RECV(":server 729 alice #test q");
    CHECK_CHAN("-- [#test]");
    RECV(":server 729 alice #test q end");
    CHECK_CHAN("-- [#test] end");
    RECV(":server 729 alice #test q :End of Channel Quiet List");
    CHECK_CHAN("-- [#test] End of Channel Quiet List");

    /* channel not found */
    RECV(":server 729 alice #xyz q");
    CHECK_SRV("-- [#xyz]");
    RECV(":server 729 alice #xyz q end");
    CHECK_SRV("-- [#xyz] end");
    RECV(":server 729 alice #xyz q :End of Channel Quiet List");
    CHECK_SRV("-- [#xyz] End of Channel Quiet List");
}

/*
 * Tests functions:
 *   irc_protocol_cb_730 (monitored nicks are online (RPL_MONONLINE))
 *   irc_protocol_cb_731 (monitored nicks are offline (RPL_MONOFFLINE))
 */

TEST(IrcProtocolWithServer, 730)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 730");
    CHECK_ERROR_PARAMS("730", 0, 2);
    RECV(":server 730 alice");
    CHECK_ERROR_PARAMS("730", 1, 2);
    RECV(":server 731");
    CHECK_ERROR_PARAMS("731", 0, 2);
    RECV(":server 731 alice");
    CHECK_ERROR_PARAMS("731", 1, 2);

    RECV(":server 730 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_NO_MSG;

    RECV(":server 731 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_NO_MSG;

    /* with notify on nick1 */
    run_cmd_quiet ("/notify add nick1 " IRC_FAKE_SERVER);
    RECV(":server 730 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("-- notify: nick1 (user1@host1) is connected");
    RECV(":server 731 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("-- notify: nick1 (user1@host1) has quit");

    /* with notify on nick1 and nick2 */
    run_cmd_quiet ("/notify add nick2 " IRC_FAKE_SERVER);
    RECV(":server 730 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("-- notify: nick1 (user1@host1) has connected");
    CHECK_SRV("-- notify: nick2 (user2@host2) is connected");

    run_cmd_quiet ("/mute /notify del nick1 " IRC_FAKE_SERVER);
    run_cmd_quiet ("/mute /notify del nick2 " IRC_FAKE_SERVER);
}

/*
 * Tests functions:
 *   irc_protocol_cb_732 (list of monitored nicks (RPL_MONLIST))
 */

TEST(IrcProtocolWithServer, 732)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 732");
    CHECK_ERROR_PARAMS("732", 0, 1);

    RECV(":server 732 alice");
    CHECK_SRV("--");
    RECV(":server 732 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("-- nick1!user1@host1,nick2!user2@host2");
}

/*
 * Tests functions:
 *   irc_protocol_cb_733 (end of a monitor list (RPL_ENDOFMONLIST))
 */

TEST(IrcProtocolWithServer, 733)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 733");
    CHECK_ERROR_PARAMS("733", 0, 1);

    RECV(":server 733 alice");
    CHECK_SRV("--");
    RECV(":server 733 alice end");
    CHECK_SRV("-- end");
    RECV(":server 733 alice :End of MONITOR list");
    CHECK_SRV("-- End of MONITOR list");
}

/*
 * Tests functions:
 *   irc_protocol_cb_734 (monitor list is full (ERR_MONLISTFULL))
 */

TEST(IrcProtocolWithServer, 734)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 734");
    CHECK_ERROR_PARAMS("734", 0, 3);
    RECV(":server 734 alice");
    CHECK_ERROR_PARAMS("734", 1, 3);
    RECV(":server 734 alice 10");
    CHECK_ERROR_PARAMS("734", 2, 3);

    RECV(":server 734 alice 10 nick1,nick2");
    CHECK_SRV("=!=  (10)");
    RECV(":server 734 alice 10 nick1,nick2 full");
    CHECK_SRV("=!= full (10)");
    RECV(":server 734 alice 10 nick1,nick2 :Monitor list is full");
    CHECK_SRV("=!= Monitor list is full (10)");
}

/*
 * Tests functions:
 *   irc_protocol_cb_900 (logged in as (SASL))
 */

TEST(IrcProtocolWithServer, 900)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 900");
    CHECK_ERROR_PARAMS("900", 0, 4);
    RECV(":server 900 alice");
    CHECK_ERROR_PARAMS("900", 1, 4);
    RECV(":server 900 alice alice!user@host");
    CHECK_ERROR_PARAMS("900", 2, 4);
    RECV(":server 900 alice alice!user@host alice");
    CHECK_ERROR_PARAMS("900", 3, 4);

    RECV(":server 900 alice alice!user@host alice logged");
    CHECK_SRV("-- logged (alice!user@host)");
    RECV(":server 900 alice alice!user@host alice "
         ":You are now logged in as mynick");
    CHECK_SRV("-- You are now logged in as mynick (alice!user@host)");
    RECV(":server 900 * * alice :You are now logged in as mynick");
    CHECK_SRV("-- You are now logged in as mynick");
}

/*
 * Tests functions:
 *   irc_protocol_cb_901 (you are now logged out)
 */

TEST(IrcProtocolWithServer, 901)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 901");
    CHECK_ERROR_PARAMS("901", 0, 3);
    RECV(":server 901 alice");
    CHECK_ERROR_PARAMS("901", 1, 3);
    RECV(":server 901 alice nick!user@host");
    CHECK_ERROR_PARAMS("901", 2, 3);

    RECV(":server 901 alice nick!user@host logged");
    CHECK_SRV("-- logged");
    RECV(":server 901 alice nick!user@host :You are now logged out");
    CHECK_SRV("-- You are now logged out");
}

/*
 * Tests functions:
 *   irc_protocol_cb_903 (SASL OK)
 *   irc_protocol_cb_907 (SASL OK)
 */

TEST(IrcProtocolWithServer, 903_907)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 903");
    CHECK_ERROR_PARAMS("903", 0, 1);

    /* not enough parameters */
    RECV(":server 907");
    CHECK_ERROR_PARAMS("907", 0, 1);

    RECV(":server 903 alice ok");
    CHECK_SRV("-- ok");
    RECV(":server 903 alice :SASL authentication successful");
    CHECK_SRV("-- SASL authentication successful");
    RECV(":server 903 * :SASL authentication successful");
    CHECK_SRV("-- SASL authentication successful");


    RECV(":server 907 alice ok");
    CHECK_SRV("-- ok");
    RECV(":server 907 alice :SASL authentication successful");
    CHECK_SRV("-- SASL authentication successful");
}

/*
 * Tests functions:
 *   irc_protocol_cb_902 (SASL failed)
 *   irc_protocol_cb_904 (SASL failed)
 *   irc_protocol_cb_905 (SASL failed)
 *   irc_protocol_cb_906 (SASL failed)
 */

TEST(IrcProtocolWithServer, 902_904_905_906)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 902");
    CHECK_ERROR_PARAMS("902", 0, 1);

    /* not enough parameters */
    RECV(":server 904");
    CHECK_ERROR_PARAMS("904", 0, 1);

    /* not enough parameters */
    RECV(":server 905");
    CHECK_ERROR_PARAMS("905", 0, 1);

    /* not enough parameters */
    RECV(":server 906");
    CHECK_ERROR_PARAMS("906", 0, 1);

    RECV(":server 902 alice error");
    CHECK_SRV("-- error");
    RECV(":server 902 alice :SASL authentication failed");
    CHECK_SRV("-- SASL authentication failed");

    RECV(":server 904 alice error");
    CHECK_SRV("-- error");
    RECV(":server 904 alice :SASL authentication failed");
    CHECK_SRV("-- SASL authentication failed");

    RECV(":server 905 alice error");
    CHECK_SRV("-- error");
    RECV(":server 905 alice :SASL authentication failed");
    CHECK_SRV("-- SASL authentication failed");

    RECV(":server 906 alice error");
    CHECK_SRV("-- error");
    RECV(":server 906 alice :SASL authentication failed");
    CHECK_SRV("-- SASL authentication failed");
}

/*
 * Tests functions:
 *   irc_protocol_cb_server_mode_reason
 *
 * Messages:
 *   973: whois (secure connection)
 *   974: whois (secure connection)
 *   975: whois (secure connection)
 */

TEST(IrcProtocolWithServer, server_mode_reason)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 973");
    CHECK_ERROR_PARAMS("973", 0, 1);

    RECV(":server 973 alice");
    CHECK_NO_MSG;
    RECV(":server 973 alice mode");
    CHECK_SRV("-- mode");
    RECV(":server 973 alice mode test");
    CHECK_SRV("-- mode: test");
    RECV(":server 973 alice mode :test");
    CHECK_SRV("-- mode: test");

    RECV(":server 974 alice");
    CHECK_NO_MSG;
    RECV(":server 974 alice mode");
    CHECK_SRV("-- mode");
    RECV(":server 974 alice mode test");
    CHECK_SRV("-- mode: test");
    RECV(":server 974 alice mode :test");
    CHECK_SRV("-- mode: test");

    RECV(":server 975 alice");
    CHECK_NO_MSG;
    RECV(":server 975 alice mode");
    CHECK_SRV("-- mode");
    RECV(":server 975 alice mode test");
    CHECK_SRV("-- mode: test");
    RECV(":server 975 alice mode :test");
    CHECK_SRV("-- mode: test");

    RECV(":server 973 bob");
    CHECK_SRV("-- bob");
    RECV(":server 973 bob mode");
    CHECK_SRV("-- bob: mode");
    RECV(":server 973 bob mode test");
    CHECK_SRV("-- bob: mode test");
    RECV(":server 973 bob mode :test");
    CHECK_SRV("-- bob: mode test");

    RECV(":server 974 bob");
    CHECK_SRV("-- bob");
    RECV(":server 974 bob mode");
    CHECK_SRV("-- bob: mode");
    RECV(":server 974 bob mode test");
    CHECK_SRV("-- bob: mode test");
    RECV(":server 974 bob mode :test");
    CHECK_SRV("-- bob: mode test");

    RECV(":server 975 bob");
    CHECK_SRV("-- bob");
    RECV(":server 975 bob mode");
    CHECK_SRV("-- bob: mode");
    RECV(":server 975 bob mode test");
    CHECK_SRV("-- bob: mode test");
    RECV(":server 975 bob mode :test");
    CHECK_SRV("-- bob: mode test");
}
