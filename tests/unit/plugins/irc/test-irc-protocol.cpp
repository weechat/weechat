/*
 * test-irc-protocol.cpp - test IRC protocol functions
 *
 * Copyright (C) 2019-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "tests/tests-record.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hook.h"
#include "src/core/core-input.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-batch.h"
#include "src/plugins/irc/irc-ctcp.h"
#include "src/plugins/irc/irc-protocol.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-nick.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/logger/logger-config.h"
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
    "CASEMAPPING=strict-rfc1459 LINELEN=4096 NICKLEN=30 MAXNICKLEN=31 " \
    "USERLEN=16 HOSTLEN=32 CHANNELLEN=50 TOPICLEN=390 DEAF=D "          \
    "CHANTYPES=# CHANMODES=eIbq,k,flj,CFLMPQScgimnprstuz "              \
    "MONITOR=100 UTF8MAPPING=rfc8265 UTF8ONLY"
#define IRC_ALL_CAPS "account-notify,account-tag,away-notify,batch,"    \
    "cap-notify,chghost,draft/multiline,echo-message,extended-join,"    \
    "invite-notify,message-tags,multi-prefix,server-time,setname,"      \
    "userhost-in-names"

#define WEE_CHECK_PROTOCOL_TAGS(__result, __server, __command, __tags,  \
                                __extra_tags)                           \
    ctxt.server = __server;                                             \
    ctxt.command = (char *)__command;                                   \
    ctxt.tags = __tags;                                                 \
    STRCMP_EQUAL(__result, irc_protocol_tags (&ctxt, __extra_tags));

#define WEE_CHECK_CAP_TO_ENABLE(__result, __string, __sasl_requested)   \
    str = irc_protocol_cap_to_enable (__string, __sasl_requested);      \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

#define RECV(__irc_msg)                                                 \
    server_recv (__irc_msg);

#define CHECK_CORE(__prefix, __message)                                 \
    if (!record_search ("core.weechat", __prefix, __message, NULL))     \
    {                                                                   \
        char **msg = build_error (                                      \
            "Core message not displayed",                               \
            __prefix,                                                   \
            __message,                                                  \
            NULL,                                                       \
            "All messages displayed");                                  \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_SRV(__prefix, __message, __tags)                          \
    if (!record_search ("irc.server." IRC_FAKE_SERVER, __prefix,        \
                        __message, __tags))                             \
    {                                                                   \
        char **msg = build_error (                                      \
            "Server message not displayed",                             \
            __prefix,                                                   \
            __message,                                                  \
            __tags,                                                     \
            "All messages displayed");                                  \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_ERROR_PARAMS(__command, __params, __expected_params)      \
    CHECK_SRV("=!=",                                                    \
              "irc: too few parameters received in command "            \
              "\"" __command "\" (received: " #__params ", "            \
              "expected: at least " #__expected_params ")",             \
              "");

#define CHECK_ERROR_NICK(__command)                                     \
    CHECK_SRV("=!=",                                                    \
              "irc: command \"" __command "\" received without nick",   \
              "");

#define CHECK_ERROR_PARSE(__command, __message)                         \
    CHECK_SRV("=!=",                                                    \
              "irc: failed to parse command \"" __command "\" "         \
              "(please report to developers): \"" __message "\"",       \
              "");

#define CHECK_CHAN(__prefix, __message, __tags)                         \
    if (!record_search ("irc." IRC_FAKE_SERVER ".#test", __prefix,      \
                        __message, __tags))                             \
    {                                                                   \
        char **msg = build_error (                                      \
            "Channel message not displayed",                            \
            __prefix,                                                   \
            __message,                                                  \
            __tags,                                                     \
            "All messages displayed");                                  \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_CHAN_DATE_VALUE(__prefix, __message, __tags,              \
                              __date_sec, __date_usec)                  \
    {                                                                   \
        struct timeval tv_now;                                          \
        const char *ptr_date;                                           \
        long value;                                                     \
        char *error;                                                    \
        gettimeofday (&tv_now, NULL);                                   \
        struct t_hashtable *record = record_search (                    \
            "irc." IRC_FAKE_SERVER ".#test",                            \
            __prefix, __message, __tags);                               \
        if (!record)                                                    \
        {                                                               \
            char **msg = build_error (                                  \
                "Channel message not displayed",                        \
                __prefix,                                               \
                __message,                                              \
                __tags,                                                 \
                "All messages displayed");                              \
            record_dump (msg);                                          \
            FAIL(string_dyn_free (msg, 0));                             \
        }                                                               \
        ptr_date = (const char *)hashtable_get (record, "date");        \
        CHECK(ptr_date);                                                \
        value = strtol (ptr_date, &error, 10);                          \
        CHECK(error && !error[0]);                                      \
        LONGS_EQUAL(__date_sec, value);                                 \
        ptr_date = (const char *)hashtable_get (record, "date_usec");   \
        CHECK(ptr_date);                                                \
        value = strtol (ptr_date, &error, 10);                          \
        CHECK(error && !error[0]);                                      \
        LONGS_EQUAL(__date_usec, value);                                \
    }

#define CHECK_CHAN_DATE_NOW(__prefix, __message, __tags)                \
    {                                                                   \
        struct timeval tv_now;                                          \
        const char *ptr_date;                                           \
        long value;                                                     \
        char *error;                                                    \
        gettimeofday (&tv_now, NULL);                                   \
        struct t_hashtable *record = record_search (                    \
            "irc." IRC_FAKE_SERVER ".#test",                            \
            __prefix, __message, __tags);                               \
        if (!record)                                                    \
        {                                                               \
            char **msg = build_error (                                  \
                "Channel message not displayed",                        \
                __prefix,                                               \
                __message,                                              \
                __tags,                                                 \
                "All messages displayed");                              \
            record_dump (msg);                                          \
            FAIL(string_dyn_free (msg, 0));                             \
        }                                                               \
        ptr_date = (const char *)hashtable_get (record, "date");        \
        CHECK(ptr_date);                                                \
        value = strtol (ptr_date, &error, 10);                          \
        CHECK(error && !error[0]);                                      \
        CHECK(value >= tv_now.tv_sec - 5);                              \
        CHECK(value <= tv_now.tv_sec + 5);                              \
    }

#define CHECK_PV(__nick, __prefix, __message, __tags)                   \
    if (!record_search ("irc." IRC_FAKE_SERVER "." __nick,              \
                        __prefix, __message, __tags))                   \
    {                                                                   \
        char **msg = build_error (                                      \
            "Private message not displayed",                            \
            __prefix,                                                   \
            __message,                                                  \
            __tags,                                                     \
            "All messages displayed");                                  \
        record_dump (msg);                                              \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define CHECK_PV_CLOSE(__nick, __prefix, __message, __tags)             \
    CHECK_PV(__nick, __prefix, __message, __tags);                      \
    gui_buffer_close (                                                  \
        gui_buffer_search_by_full_name ("irc." IRC_FAKE_SERVER          \
                                        "." __nick));

#define CHECK_NO_MSG                                                    \
    if (arraylist_size (recorded_messages) > 0)                         \
    {                                                                   \
        char **msg = build_error (                                      \
            "Unexpected message(s) displayed",                          \
            NULL,                                                       \
            NULL,                                                       \
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
        char **msg = build_error (                                      \
            "Message not sent to the IRC server",                       \
            NULL,                                                       \
            __message,                                                  \
            NULL,                                                       \
            "All messages sent");                                       \
        sent_msg_dump (msg);                                            \
        FAIL(string_dyn_free (msg, 0));                                 \
    }                                                                   \
    else if ((__message == NULL)                                        \
             && (arraylist_size (sent_messages) > 0))                   \
    {                                                                   \
        char **msg = build_error (                                      \
            "Unexpected response(s) sent to the IRC server",            \
            NULL,                                                       \
            NULL,                                                       \
            NULL,                                                       \
            NULL);                                                      \
        sent_msg_dump (msg);                                            \
        FAIL(string_dyn_free (msg, 0));                                 \
    }

#define SRV_INIT                                                        \
    RECV(":server 001 alice :Welcome on this server, alice!");          \
    CHECK_SRV("--", "Welcome on this server, alice!",                   \
              "irc_001,irc_numeric,nick_server,log3");

#define SRV_INIT_JOIN                                                   \
    SRV_INIT;                                                           \
    RECV(":alice!user_a@host_a JOIN #test");                            \
    CHECK_CHAN("-->", "alice (user_a@host_a) has joined #test",         \
               "irc_join,nick_alice,host_user_a@host_a,log4");

#define SRV_INIT_JOIN2                                                  \
    SRV_INIT_JOIN;                                                      \
    RECV(":bob!user_b@host_b JOIN #test");                              \
    CHECK_CHAN("-->", "bob (user_b@host_b) has joined #test",           \
               "irc_join,irc_smart_filter,nick_bob,host_user_b@host_b," \
               "log4");

TEST_GROUP(IrcProtocol)
{
};

TEST_GROUP(IrcProtocolWithServer)
{
    struct t_irc_server *ptr_server = NULL;
    struct t_arraylist *sent_messages = NULL;
    struct t_hook *hook_signal_irc_out = NULL;

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

    void server_input_data (const char *buffer, const char *data)
    {
        struct t_gui_buffer *ptr_buffer;

        record_start ();
        arraylist_clear (sent_messages);

        ptr_buffer = gui_buffer_search_by_full_name (buffer);
        if (ptr_buffer)
            input_data (ptr_buffer, data, NULL, 0, 0);

        record_stop ();
    }

    char **build_error (const char *msg1,
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

    static int signal_irc_out_cb (const void *pointer, void *data,
                                  const char *signal, const char *type_data,
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
            hook_signal_irc_out = hook_signal (
                NULL,
                IRC_FAKE_SERVER ",irc_out1_*",
                &signal_irc_out_cb, sent_messages, NULL);
        }

        /*
         * disable backlog feature during tests, so we are not polluted by
         * these messages when buffers are opened
         */
        config_file_option_set (logger_config_look_backlog, "0", 1);

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

        /* restore backlog feature */
        config_file_option_reset (logger_config_look_backlog, 1);
    }
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

TEST(IrcProtocolWithServer, Tags)
{
    struct t_irc_protocol_ctxt ctxt;
    struct t_hashtable *tags_empty, *tags_1, *tags_2;

    SRV_INIT;

    memset (&ctxt, 0, sizeof (ctxt));

    ctxt.nick = strdup ("alice");
    ctxt.nick_is_me = 1;
    ctxt.address = strdup ("user@example.com");

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
    hashtable_set (tags_2, "key_3_empty", "");
    hashtable_set (tags_2, "key_4_null", NULL);

    WEE_CHECK_PROTOCOL_TAGS("nick_alice,host_user@example.com",
                            NULL, NULL, NULL, NULL);

    /* command */
    WEE_CHECK_PROTOCOL_TAGS("irc_privmsg,nick_alice,host_user@example.com,log1",
                            NULL, "privmsg", NULL, NULL);
    WEE_CHECK_PROTOCOL_TAGS("irc_join,nick_alice,host_user@example.com,log4",
                            NULL, "join", NULL, NULL);

    /* command + irc_msg_tags */
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,nick_alice,host_user@example.com,log1",
        NULL, "privmsg", tags_empty, NULL);
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_join,nick_alice,host_user@example.com,log4",
        NULL, "join", tags_empty, NULL);
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,irc_tag_key1=value1,nick_alice,host_user@example.com,log1",
        NULL, "privmsg", tags_1, NULL);
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_join,irc_tag_key1=value1,nick_alice,host_user@example.com,log4",
        NULL, "join", tags_1, NULL);
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,irc_tag_key1=value1,"
        "irc_tag_key_2;comma=value2;comma,"
        "irc_tag_key_3_empty=,irc_tag_key_4_null,nick_alice,"
        "host_user@example.com,log1",
        NULL, "privmsg", tags_2, NULL);
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_join,irc_tag_key1=value1,"
        "irc_tag_key_2;comma=value2;comma,"
        "irc_tag_key_3_empty=,irc_tag_key_4_null,nick_alice,"
        "host_user@example.com,log4",
        NULL, "join", tags_2, NULL);

    /* command + extra_tags */
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,nick_alice,host_user@example.com,log1",
        NULL, "privmsg", NULL, "");
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_join,nick_alice,host_user@example.com,log4",
        NULL, "join", NULL, "");
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,tag1,tag2,nick_alice,host_user@example.com,log1",
        NULL, "privmsg", NULL, "tag1,tag2");
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_join,tag1,tag2,nick_alice,host_user@example.com,log4",
        NULL, "join", NULL, "tag1,tag2");

    /* command + irc_msg_tags + extra_tags + nick */
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,irc_tag_key1=value1,irc_tag_key_2;comma=value2;comma,"
        "irc_tag_key_3_empty=,irc_tag_key_4_null,tag1,tag2,nick_bob,log1",
        NULL, "privmsg", tags_2, "tag1,tag2,nick_bob");

    /* command + irc_msg_tags + extra_tags + nick + address */
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,irc_tag_key1=value1,irc_tag_key_2;comma=value2;comma,"
        "irc_tag_key_3_empty=,irc_tag_key_4_null,tag1,tag2,nick_bob,"
        "host_user@host,log1",
        NULL, "privmsg", tags_2,
        "tag1,tag2,nick_bob,host_user@host");

    /* self message */
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,self_msg,host_user@example.com,log1",
        NULL, "privmsg", NULL, "self_msg");

    /* server + self message */
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,self_msg,nick_alice,host_user@example.com,log1",
        ptr_server, "privmsg", NULL, "self_msg");

    /* server + self message + host */
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,self_msg,host_user@host,nick_alice,log1",
        ptr_server, "privmsg", NULL, "self_msg,host_user@host");

    /* server + self message (other nick) + host */
    ctxt.nick_is_me = 0;
    WEE_CHECK_PROTOCOL_TAGS(
        "irc_privmsg,self_msg,nick_bob,log1",
        ptr_server, "privmsg", NULL, "self_msg,nick_bob");

    hashtable_free (tags_empty);
    hashtable_free (tags_1);
    hashtable_free (tags_2);

    free (ctxt.nick);
    free (ctxt.address);
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

/*
 * Tests send of messages to channel (STATUSMSG and normal) and nick,
 * without capability "echo-message" enabled:
 *   - message (text)
 *   - notice (/notice)
 *   - action (/me + /ctcp)
 *   - CTCP (/ctcp)
 */

TEST(IrcProtocolWithServer, SendMessagesWithoutEchoMessage)
{
    const char *buffer_server = "irc.server." IRC_FAKE_SERVER;
    const char *buffer_chan = "irc." IRC_FAKE_SERVER ".#test";
    const char *buffer_pv = "irc." IRC_FAKE_SERVER ".bob";

    SRV_INIT_JOIN;

    /* open private buffer */
    RECV(":bob!user@host PRIVMSG alice :hi Alice!");

    /* message to channel (text in buffer) */
    server_input_data (buffer_chan, "\002msg chan 1");
    CHECK_SENT("PRIVMSG #test :\002msg chan 1");
    CHECK_CHAN("alice", "msg chan 1",
               "irc_privmsg,self_msg,notify_none,no_highlight,prefix_nick_white,"
               "nick_alice,log1");

    /* message to channel (with /msg <channel>) */
    server_input_data (buffer_server, "/msg #test \002msg chan 2");
    CHECK_SENT("PRIVMSG #test :\002msg chan 2");
    CHECK_CHAN("alice", "msg chan 2",
               "irc_privmsg,self_msg,notify_none,no_highlight,prefix_nick_white,"
               "nick_alice,log1");

    /* message to channel (with /msg <channel>), channel not joined */
    server_input_data (buffer_server, "/msg #zzz \002msg chan not joined");
    CHECK_SENT("PRIVMSG #zzz :\002msg chan not joined");
    CHECK_SRV("--", "Msg(alice) -> #zzz: msg chan not joined",
              "irc_privmsg,self_msg,notify_none,no_highlight,nick_alice,log1");

    /* STATUSMSG message to channel (with /msg @<channel>) */
    server_input_data (buffer_server, "/msg @#test \002msg chan ops");
    CHECK_SENT("PRIVMSG @#test :\002msg chan ops");
    CHECK_CHAN("--", "Msg(alice) -> @#test: msg chan ops",
               "irc_privmsg,self_msg,notify_none,no_highlight,nick_alice,log1");

    /* STATUSMSG message to channel (with /msg @<channel>), channel not joined */
    server_input_data (buffer_server, "/msg @#zzz \002msg chan ops not joined");
    CHECK_SENT("PRIVMSG @#zzz :\002msg chan ops not joined");
    CHECK_SRV("--", "Msg(alice) -> @#zzz: msg chan ops not joined",
              "irc_privmsg,self_msg,notify_none,no_highlight,nick_alice,log1");

    /* message to a nick (text in private buffer) */
    server_input_data (buffer_pv, "\002msg pv 1");
    CHECK_SENT("PRIVMSG bob :\002msg pv 1");
    CHECK_PV("bob", "alice", "msg pv 1",
             "irc_privmsg,self_msg,notify_none,no_highlight,prefix_nick_white,"
             "nick_alice,log1");

    /* message to a nick (with /msg <nick>) */
    server_input_data (buffer_server, "/msg bob \002msg pv 2");
    CHECK_SENT("PRIVMSG bob :\002msg pv 2");
    CHECK_PV("bob", "alice", "msg pv 2",
             "irc_privmsg,self_msg,notify_none,no_highlight,prefix_nick_white,"
             "nick_alice,log1");

    /* message to a nick (with /msg <nick>), hidden password */
    server_input_data (buffer_server, "/msg nickserv identify secret");
    CHECK_SENT("PRIVMSG nickserv :identify secret");
    CHECK_SRV("--", "Msg(alice) -> nickserv: identify ******",
              "irc_privmsg,self_msg,notify_none,no_highlight,nick_alice,log1");

    /* message to a nick (with /query <nick>) */
    server_input_data (buffer_server, "/query bob \002msg pv 3");
    CHECK_SENT("PRIVMSG bob :\002msg pv 3");
    CHECK_PV("bob", "alice", "msg pv 3",
             "irc_privmsg,self_msg,notify_none,no_highlight,prefix_nick_white,"
             "nick_alice,log1");

    /* message to a nick (with /query <nick>), private buffer does not exist */
    server_input_data (buffer_server, "/query bob_query \002msg pv 4");
    CHECK_SENT("PRIVMSG bob_query :\002msg pv 4");
    CHECK_PV("bob_query", "alice", "msg pv 4",
             "irc_privmsg,self_msg,notify_none,no_highlight,prefix_nick_white,"
             "nick_alice,log1");

    /* notice to channel */
    server_input_data (buffer_server, "/notice #test \002notice chan");
    CHECK_SENT("NOTICE #test :\002notice chan");
    CHECK_CHAN("--", "Notice(alice) -> #test: notice chan",
               "irc_notice,self_msg,notify_none,no_highlight,nick_alice,log1");

    /* STATUSMSG notice to channel */
    server_input_data (buffer_server, "/notice @#test \002notice chan ops");
    CHECK_SENT("NOTICE @#test :\002notice chan ops");
    CHECK_CHAN("--", "Notice(alice) -> @#test: notice chan ops",
               "irc_notice,self_msg,notify_none,no_highlight,nick_alice,log1");

    /* notice to a nick */
    server_input_data (buffer_server, "/notice bob \002notice pv");
    CHECK_SENT("NOTICE bob :\002notice pv");
    CHECK_PV("bob", "--", "Notice(alice) -> bob: notice pv",
             "irc_notice,self_msg,notify_none,no_highlight,nick_alice,log1");

    /* action on channel (with /me) */
    server_input_data (buffer_chan, "/me \002action chan 1");
    CHECK_SENT("PRIVMSG #test :\001ACTION \002action chan 1\001");
    CHECK_CHAN(" *", "alice action chan 1",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* action on channel (with /me), no message */
    server_input_data (buffer_chan, "/me");
    CHECK_SENT("PRIVMSG #test :\001ACTION\001");
    CHECK_CHAN(" *", "alice",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* action on channel (with raw code: "\001ACTION") */
    server_input_data (buffer_chan, "\001ACTION \002is testing\001");
    CHECK_SENT("PRIVMSG #test :\001ACTION \002is testing\001");
    CHECK_CHAN(" *", "alice is testing",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* action on channel (with raw code: "\001ACTION"), no message */
    server_input_data (buffer_chan, "\001ACTION\001");
    CHECK_SENT("PRIVMSG #test :\001ACTION\001");
    CHECK_CHAN(" *", "alice",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* action on channel (with /action *) */
    server_input_data (buffer_chan, "/action * \002action chan 2");
    CHECK_SENT("PRIVMSG #test :\001ACTION \002action chan 2\001");
    CHECK_CHAN(" *", "alice action chan 2",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* action on channel (with /action <channel>) */
    server_input_data (buffer_server, "/action #test \002action chan 3");
    CHECK_SENT("PRIVMSG #test :\001ACTION \002action chan 3\001");
    CHECK_CHAN(" *", "alice action chan 3",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* action on channel (with /action <channel>), no message */
    server_input_data (buffer_chan, "/action #test");
    CHECK_SENT("PRIVMSG #test :\001ACTION\001");
    CHECK_CHAN(" *", "alice",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* STATUSMSG action on channel (with /action @<channel>) */
    server_input_data (buffer_server, "/action @#test \002action chan 4");
    CHECK_SENT("PRIVMSG @#test :\001ACTION \002action chan 4\001");
    CHECK_CHAN("--", "Action -> @#test: alice action chan 4",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* STATUSMSG action on channel (with /action @<channel>), no message */
    server_input_data (buffer_server, "/action @#test");
    CHECK_SENT("PRIVMSG @#test :\001ACTION\001");
    CHECK_CHAN("--", "Action -> @#test: alice",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* action on channel (with /ctcp <channel> action) */
    server_input_data (buffer_server, "/ctcp #test action \002action chan 5");
    CHECK_SENT("PRIVMSG #test :\001ACTION \002action chan 5\001");
    CHECK_CHAN(" *", "alice action chan 5",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* action on channel (with /ctcp <channel> action), no message */
    server_input_data (buffer_server, "/ctcp #test action");
    CHECK_SENT("PRIVMSG #test :\001ACTION\001");
    CHECK_CHAN(" *", "alice",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* STATUSMSG action on channel (with /ctcp @<channel> action) */
    server_input_data (buffer_server, "/ctcp @#test action \002action chan ops");
    CHECK_SENT("PRIVMSG @#test :\001ACTION \002action chan ops\001");
    CHECK_CHAN("--", "Action -> @#test: alice action chan ops",
               "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* action in private (with /me) */
    server_input_data (buffer_pv, "/me \002action pv 1");
    CHECK_SENT("PRIVMSG bob :\001ACTION \002action pv 1\001");
    CHECK_PV("bob", " *", "alice action pv 1",
             "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
             "nick_alice,log1");

    /* action in private (with /ctcp) */
    server_input_data (buffer_server, "/ctcp bob action \002action pv 2");
    CHECK_SENT("PRIVMSG bob :\001ACTION \002action pv 2\001");
    CHECK_PV("bob", " *", "alice action pv 2",
             "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
             "nick_alice,log1");

    /* action in private (with /ctcp), without private buffer */
    server_input_data (buffer_server, "/ctcp bob2 action \002action pv 3");
    CHECK_SENT("PRIVMSG bob2 :\001ACTION \002action pv 3\001");
    CHECK_SRV("--", "Action -> bob2: alice action pv 3",
              "irc_privmsg,irc_action,self_msg,notify_none,no_highlight,"
              "nick_alice,log1");

    /* CTCP version to channel */
    server_input_data (buffer_server, "/ctcp #test version");
    CHECK_SENT("PRIVMSG #test :\001VERSION\001");
    CHECK_CHAN("--", "CTCP query to #test: VERSION",
               "irc_privmsg,irc_ctcp,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* unknown CTCP to channel */
    server_input_data (buffer_server, "/ctcp #test unknown1 \002some args");
    CHECK_SENT("PRIVMSG #test :\001UNKNOWN1 \002some args\001");
    CHECK_CHAN("--", "CTCP query to #test: UNKNOWN1 some args",
               "irc_privmsg,irc_ctcp,self_msg,notify_none,no_highlight,"
               "nick_alice,log1");

    /* CTCP version to nick */
    server_input_data (buffer_server, "/ctcp bob version");
    CHECK_SENT("PRIVMSG bob :\001VERSION\001");
    CHECK_PV("bob", "--", "CTCP query to bob: VERSION",
             "irc_privmsg,irc_ctcp,self_msg,notify_none,no_highlight,"
             "nick_alice,log1");

    /* CTCP version to other nick (no private buffer) */
    server_input_data (buffer_server, "/ctcp other_nick version");
    CHECK_SENT("PRIVMSG other_nick :\001VERSION\001");
    CHECK_SRV("--", "CTCP query to other_nick: VERSION",
              "irc_privmsg,irc_ctcp,self_msg,notify_none,no_highlight,"
              "nick_alice,log1");

    /* unknown CTCP to nick */
    server_input_data (buffer_server, "/ctcp bob unknown2 \002some args");
    CHECK_SENT("PRIVMSG bob :\001UNKNOWN2 \002some args\001");
    CHECK_PV("bob", "--", "CTCP query to bob: UNKNOWN2 some args",
             "irc_privmsg,irc_ctcp,self_msg,notify_none,no_highlight,"
             "nick_alice,log1");
}

/*
 * Tests send of messages to channel (STATUSMSG and normal) and nick,
 * with capability "echo-message" enabled:
 *   - message (text)
 *   - notice (/notice)
 *   - action (/me + /ctcp)
 *   - CTCP (/ctcp)
 */

TEST(IrcProtocolWithServer, SendMessagesWithEchoMessage)
{
    const char *buffer_server = "irc.server." IRC_FAKE_SERVER;
    const char *buffer_chan = "irc." IRC_FAKE_SERVER ".#test";
    const char *buffer_pv = "irc." IRC_FAKE_SERVER ".bob";

    /* assume "echo-message" capability is enabled in server */
    hashtable_set (ptr_server->cap_list, "echo-message", NULL);

    SRV_INIT_JOIN;

    /* open private buffer */
    RECV(":bob!user@host PRIVMSG alice :hi Alice!");

    /* message to channel (text in buffer) */
    server_input_data (buffer_chan, "\002msg chan 1");
    CHECK_SENT("PRIVMSG #test :\002msg chan 1");
    CHECK_NO_MSG;

    /* message to channel (with /msg <channel>) */
    server_input_data (buffer_server, "/msg #test \002msg chan 2");
    CHECK_SENT("PRIVMSG #test :\002msg chan 2");
    CHECK_NO_MSG;

    /* message to channel (with /msg <channel>), channel not joined */
    server_input_data (buffer_server, "/msg #zzz \002msg chan not joined");
    CHECK_SENT("PRIVMSG #zzz :\002msg chan not joined");
    CHECK_NO_MSG;

    /* STATUSMSG message to channel (with /msg @<channel>) */
    server_input_data (buffer_server, "/msg @#test \002msg chan ops");
    CHECK_SENT("PRIVMSG @#test :\002msg chan ops");
    CHECK_NO_MSG;

    /* STATUSMSG message to channel (with /msg @<channel>), channel not joined */
    server_input_data (buffer_server, "/msg @#zzz \002msg chan ops not joined");
    CHECK_SENT("PRIVMSG @#zzz :\002msg chan ops not joined");
    CHECK_NO_MSG;

    /* message to a nick (text in private buffer) */
    server_input_data (buffer_pv, "\002msg pv 1");
    CHECK_SENT("PRIVMSG bob :\002msg pv 1");
    CHECK_NO_MSG;

    /* message to a nick (with /msg <nick>) */
    server_input_data (buffer_server, "/msg bob \002msg pv 2");
    CHECK_SENT("PRIVMSG bob :\002msg pv 2");
    CHECK_NO_MSG;

    /* message to a nick (with /msg <nick>), hidden password */
    server_input_data (buffer_server, "/msg nickserv identify secret");
    CHECK_SENT("PRIVMSG nickserv :identify secret");
    CHECK_NO_MSG;

    /* message to a nick (with /query <nick>) */
    server_input_data (buffer_server, "/query bob \002msg pv 3");
    CHECK_SENT("PRIVMSG bob :\002msg pv 3");
    CHECK_NO_MSG;

    /* message to a nick (with /query <nick>), private buffer does not exist */
    server_input_data (buffer_server, "/query bob_query \002msg pv 4");
    CHECK_SENT("PRIVMSG bob_query :\002msg pv 4");
    CHECK_NO_MSG;

    /* notice to channel */
    server_input_data (buffer_server, "/notice #test \002notice chan");
    CHECK_SENT("NOTICE #test :\002notice chan");
    CHECK_NO_MSG;

    /* STATUSMSG notice to channel */
    server_input_data (buffer_server, "/notice @#test \002notice chan ops");
    CHECK_SENT("NOTICE @#test :\002notice chan ops");
    CHECK_NO_MSG;

    /* notice to a nick */
    server_input_data (buffer_server, "/notice bob \002notice pv");
    CHECK_SENT("NOTICE bob :\002notice pv");
    CHECK_NO_MSG;

    /* action on channel (with /me) */
    server_input_data (buffer_chan, "/me \002action chan 1");
    CHECK_SENT("PRIVMSG #test :\001ACTION \002action chan 1\001");
    CHECK_NO_MSG;

    /* action on channel (with /me), no message */
    server_input_data (buffer_chan, "/me");
    CHECK_SENT("PRIVMSG #test :\001ACTION\001");
    CHECK_NO_MSG;

    /* action on channel (with raw code: "\001ACTION") */
    server_input_data (buffer_chan, "\001ACTION \002is testing\001");
    CHECK_SENT("PRIVMSG #test :\001ACTION \002is testing\001");
    CHECK_NO_MSG;

    /* action on channel (with raw code: "\001ACTION"), no message */
    server_input_data (buffer_chan, "\001ACTION\001");
    CHECK_SENT("PRIVMSG #test :\001ACTION\001");
    CHECK_NO_MSG;

    /* action on channel (with /action *) */
    server_input_data (buffer_chan, "/action * \002action chan 2");
    CHECK_SENT("PRIVMSG #test :\001ACTION \002action chan 2\001");
    CHECK_NO_MSG;

    /* action on channel (with /action <channel>) */
    server_input_data (buffer_server, "/action #test \002action chan 3");
    CHECK_SENT("PRIVMSG #test :\001ACTION \002action chan 3\001");
    CHECK_NO_MSG;

    /* action on channel (with /action <channel>), no message */
    server_input_data (buffer_chan, "/action #test");
    CHECK_SENT("PRIVMSG #test :\001ACTION\001");
    CHECK_NO_MSG;

    /* STATUSMSG action on channel (with /action @<channel>) */
    server_input_data (buffer_server, "/action @#test \002action chan 4");
    CHECK_SENT("PRIVMSG @#test :\001ACTION \002action chan 4\001");
    CHECK_NO_MSG;

    /* STATUSMSG action on channel (with /action @<channel>), no message */
    server_input_data (buffer_server, "/action @#test");
    CHECK_SENT("PRIVMSG @#test :\001ACTION\001");
    CHECK_NO_MSG;

    /* action on channel (with /ctcp <channel> action) */
    server_input_data (buffer_server, "/ctcp #test action \002action chan 5");
    CHECK_SENT("PRIVMSG #test :\001ACTION \002action chan 5\001");
    CHECK_NO_MSG;

    /* STATUSMSG action on channel (with /ctcp @<channel> action) */
    server_input_data (buffer_server, "/ctcp @#test action \002action chan ops");
    CHECK_SENT("PRIVMSG @#test :\001ACTION \002action chan ops\001");
    CHECK_NO_MSG;

    /* action in private (with /me) */
    server_input_data (buffer_pv, "/me \002action pv 1");
    CHECK_SENT("PRIVMSG bob :\001ACTION \002action pv 1\001");
    CHECK_NO_MSG;

    /* action in private (with /ctcp) */
    server_input_data (buffer_server, "/ctcp bob action \002action pv 2");
    CHECK_SENT("PRIVMSG bob :\001ACTION \002action pv 2\001");
    CHECK_NO_MSG;

    /* action in private (with /ctcp), without private buffer */
    server_input_data (buffer_server, "/ctcp bob2 action \002action pv 3");
    CHECK_SENT("PRIVMSG bob2 :\001ACTION \002action pv 3\001");
    CHECK_NO_MSG;

    /* CTCP version to channel */
    server_input_data (buffer_server, "/ctcp #test version");
    CHECK_SENT("PRIVMSG #test :\001VERSION\001");
    CHECK_NO_MSG;

    /* unknown CTCP to channel */
    server_input_data (buffer_server, "/ctcp #test unknown1 \002some args");
    CHECK_SENT("PRIVMSG #test :\001UNKNOWN1 \002some args\001");
    CHECK_NO_MSG;

    /* CTCP version to nick */
    server_input_data (buffer_server, "/ctcp bob version");
    CHECK_SENT("PRIVMSG bob :\001VERSION\001");
    CHECK_NO_MSG;

    /* CTCP version to other nick (no private buffer) */
    server_input_data (buffer_server, "/ctcp other_nick version");
    CHECK_SENT("PRIVMSG other_nick :\001VERSION\001");
    CHECK_NO_MSG;

    /* unknown CTCP to nick */
    server_input_data (buffer_server, "/ctcp bob unknown2 \002some args");
    CHECK_SENT("PRIVMSG bob :\001UNKNOWN2 \002some args\001");
    CHECK_NO_MSG;

    hashtable_remove (ptr_server->cap_list, "echo-message");
}

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
    CHECK_SRV("=!=", "irc: command \"XYZ\" not found: \":alice!user@host XYZ\"",
              "");

    RECV(":alice!user@host XYZ abc :\002def");
    CHECK_SRV("=!=",
              "irc: command \"XYZ\" not found: \":alice!user@host XYZ abc :\002def\"",
              "");

    RECV(":alice!user@host 099");
    CHECK_ERROR_PARAMS("099", 0, 1);

    RECV(":alice!user@host 099 abc :\002def");
    CHECK_SRV("--", "abc def",
              "irc_099,irc_numeric,nick_alice,host_user@host,log3");
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
    CHECK_SRV("=!=", "irc: command \"abc\" not found: \"abc\"", "");

    RECV(":alice!user@host");
    CHECK_NO_MSG;

    RECV("@");
    CHECK_SRV("=!=", "irc: command \"@\" not found: \"@\"", "");

    RECV("@test");
    CHECK_SRV("=!=", "irc: command \"@test\" not found: \"@test\"", "");

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

    STRCMP_EQUAL(NULL, ptr_nick->account);

    /* not enough parameters */
    RECV(":bob!user@host ACCOUNT");
    CHECK_ERROR_PARAMS("account", 0, 1);

    STRCMP_EQUAL(NULL, ptr_nick->account);

    RECV(":bob!user@host ACCOUNT *");
    CHECK_CHAN("--", "bob has unidentified",
               "irc_account,irc_smart_filter,nick_bob,host_user@host,log3");
    CHECK_PV("bob", "--", "bob has unidentified",
             "irc_account,nick_bob,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_nick->account);

    RECV(":bob!user@host ACCOUNT :*");
    CHECK_CHAN("--", "bob has unidentified",
               "irc_account,irc_smart_filter,nick_bob,host_user@host,log3");
    CHECK_PV("bob", "--", "bob has unidentified",
             "irc_account,nick_bob,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_nick->account);

    RECV(":bob!user@host ACCOUNT new_account");
    CHECK_CHAN("--", "bob has identified as new_account",
               "irc_account,irc_smart_filter,nick_bob,host_user@host,log3");
    CHECK_PV("bob", "--", "bob has identified as new_account",
             "irc_account,nick_bob,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_nick->account);

    RECV(":bob!user@host ACCOUNT :\002new_account");
    CHECK_CHAN("--", "bob has identified as new_account",
               "irc_account,irc_smart_filter,nick_bob,host_user@host,log3");
    CHECK_PV("bob", "--", "bob has identified as new_account",
             "irc_account,nick_bob,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_nick->account);
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

    STRCMP_EQUAL(NULL, ptr_nick->account);

    RECV(":alice!user@host ACCOUNT new_account");
    CHECK_CHAN("--", "alice has identified as new_account",
               "irc_account,nick_alice,host_user@host,log3");
    STRCMP_EQUAL("new_account", ptr_nick->account);

    RECV(":alice!user@host ACCOUNT : \002new account with spaces ");
    CHECK_CHAN("--", "alice has identified as  new account with spaces ",
               "irc_account,nick_alice,host_user@host,log3");
    STRCMP_EQUAL(" \002new account with spaces ", ptr_nick->account);

    RECV(":alice!user@host ACCOUNT *");
    CHECK_CHAN("--", "alice has unidentified",
               "irc_account,nick_alice,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_nick->account);

    RECV(":alice!user@host ACCOUNT :new_account2");
    CHECK_CHAN("--", "alice has identified as new_account2",
               "irc_account,nick_alice,host_user@host,log3");
    STRCMP_EQUAL("new_account2", ptr_nick->account);

    RECV(":alice!user@host ACCOUNT :*");
    CHECK_CHAN("--", "alice has unidentified",
               "irc_account,nick_alice,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_nick->account);

    hashtable_remove (ptr_server->cap_list, "account-notify");
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
         "QQDaUzXAmVffxuzFy77XWBGwABBQAgdinelBrKZaR3wE7nsIETuTVY= ");
    CHECK_NO_MSG;
    RECV(":server.address AUTHENTICATE "
         "QQDaUzXAmVffxuzFy77XWBGwABBQAgdinelBrKZaR3wE7nsIETuTVY= ");
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

    RECV(":alice!user@host AWAY : \002Holidays now! ");
    CHECK_NO_MSG;
    LONGS_EQUAL(1, ptr_nick->away);

    RECV(":alice!user@host AWAY");
    CHECK_NO_MSG;
    LONGS_EQUAL(0, ptr_nick->away);
}

/*
 * Tests functions:
 *   irc_protocol_cb_batch (without batch cap)
 */

TEST(IrcProtocolWithServer, batch_without_batch_cap)
{
    SRV_INIT_JOIN2;

    /* not enough parameters */
    RECV(":server BATCH");
    CHECK_ERROR_PARAMS("batch", 0, 1);
    RECV(":server BATCH +test");
    CHECK_NO_MSG;

    /* invalid reference: does not start with '+' or '-' */
    RECV(":server BATCH zzz type");
    CHECK_NO_MSG;
    POINTERS_EQUAL(NULL, ptr_server->batches);

    /* start batch without parameters */
    RECV(":server BATCH +ref example");
    CHECK_NO_MSG;
    POINTERS_EQUAL(NULL, irc_batch_search (ptr_server, "ref"));

    /* new messages with batch reference */
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test : \002this is a test ");
    CHECK_CHAN("bob", " this is a test ",
               "irc_privmsg,irc_tag_batch=ref,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :second test");
    CHECK_CHAN("bob", "second test",
               "irc_privmsg,irc_tag_batch=ref,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :third test");
    CHECK_CHAN("bob", "third test",
               "irc_privmsg,irc_tag_batch=ref,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");

    /* end batch */
    RECV(":server BATCH -ref");
    CHECK_NO_MSG;

    /* start batch with parameters */
    RECV(":server BATCH +ref example param1 param2 param3");
    CHECK_NO_MSG;
    POINTERS_EQUAL(NULL, irc_batch_search (ptr_server, "ref"));

    /* new messages with batch reference */
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :test 1");
    CHECK_CHAN("bob", "test 1",
               "irc_privmsg,irc_tag_batch=ref,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :test 2");
    CHECK_CHAN("bob", "test 2",
               "irc_privmsg,irc_tag_batch=ref,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :test 3");
    CHECK_CHAN("bob", "test 3",
               "irc_privmsg,irc_tag_batch=ref,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");

    /* end batch */
    RECV(":server BATCH -ref");
    CHECK_NO_MSG;

    /* start/end batch without parameters */
    RECV(":server BATCH +ref example");
    RECV(":server BATCH -ref");
    CHECK_NO_MSG;
    POINTERS_EQUAL(NULL, irc_batch_search (ptr_server, "ref"));

    /* interleaving batches */
    RECV(":server BATCH +1 example");
    CHECK_NO_MSG;
    RECV("@batch=1 :bob!user_b@host_b PRIVMSG #test :message 1");
    CHECK_CHAN("bob", "message 1",
               "irc_privmsg,irc_tag_batch=1,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV(":server BATCH +2 example");
    CHECK_NO_MSG;
    RECV("@batch=1 :bob!user_b@host_b PRIVMSG #test :message 2");
    CHECK_CHAN("bob", "message 2",
               "irc_privmsg,irc_tag_batch=1,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV("@batch=2 :bob!user_b@host_b PRIVMSG #test :message 4");
    CHECK_CHAN("bob", "message 4",
               "irc_privmsg,irc_tag_batch=2,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV("@batch=1 :bob!user_b@host_b PRIVMSG #test :message 3");
    CHECK_CHAN("bob", "message 3",
               "irc_privmsg,irc_tag_batch=1,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV(":server BATCH -1");
    CHECK_NO_MSG;
    RECV("@batch=2 :bob!user_b@host_b PRIVMSG #test :message 5");
    CHECK_CHAN("bob", "message 5",
               "irc_privmsg,irc_tag_batch=2,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV(":server BATCH -2");
    CHECK_NO_MSG;

    /* nested batch */
    RECV(":server BATCH +ref1 example1");
    CHECK_NO_MSG;
    RECV("@batch=ref1 :server BATCH +ref2 example2");
    RECV("@batch=ref1 :bob!user_b@host_b PRIVMSG #test :test ref1");
    CHECK_CHAN("bob", "test ref1",
               "irc_privmsg,irc_tag_batch=ref1,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV("@batch=ref2 :bob!user_b@host_b PRIVMSG #test :test ref2");
    CHECK_CHAN("bob", "test ref2",
               "irc_privmsg,irc_tag_batch=ref2,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV(":server BATCH -ref2");
    CHECK_NO_MSG;
    RECV(":server BATCH -ref1");
    CHECK_NO_MSG;

    /* multiline */
    RECV(":server BATCH +ref draft/multiline #test");
    CHECK_NO_MSG;
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :line 1");
    CHECK_CHAN("bob", "line 1",
               "irc_privmsg,irc_tag_batch=ref,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :line 2");
    CHECK_CHAN("bob", "line 2",
               "irc_privmsg,irc_tag_batch=ref,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV(":server BATCH -ref");
    CHECK_NO_MSG;

    /* multiline with CTCP */
    RECV(":server BATCH +ref draft/multiline #test");
    CHECK_NO_MSG;
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :\001ACTION is testing");
    CHECK_CHAN(" *", "bob is testing",
               "irc_privmsg,irc_tag_batch=ref,irc_action,notify_message,"
               "nick_bob,host_user_b@host_b,log1");
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :again\001");
    CHECK_CHAN("bob", "again\001",
               "irc_privmsg,irc_tag_batch=ref,notify_message,"
               "prefix_nick_248,nick_bob,host_user_b@host_b,log1");
    RECV(":server BATCH -ref");
    CHECK_NO_MSG;
}

/*
 * Tests functions:
 *   irc_protocol_cb_batch (with batch cap)
 */

TEST(IrcProtocolWithServer, batch_with_batch_cap)
{
    struct t_irc_batch *ptr_batch;

    /* assume "batch" capability  is enabled in server */
    hashtable_set (ptr_server->cap_list, "batch", NULL);

    SRV_INIT_JOIN2;

    /* not enough parameters */
    RECV(":server BATCH");
    CHECK_ERROR_PARAMS("batch", 0, 1);
    RECV(":server BATCH +test");
    CHECK_ERROR_PARSE("batch", ":server BATCH +test");

    /* invalid reference: does not start with '+' or '-' */
    RECV(":server BATCH zzz type");
    CHECK_NO_MSG;
    POINTERS_EQUAL(NULL, ptr_server->batches);

    /* start batch without parameters */
    RECV(":server BATCH +ref example ");
    CHECK_NO_MSG;
    ptr_batch = irc_batch_search (ptr_server, "ref");
    CHECK(ptr_batch);
    STRCMP_EQUAL(NULL, ptr_batch->parent_ref);
    STRCMP_EQUAL("example", ptr_batch->type);
    STRCMP_EQUAL(NULL, ptr_batch->parameters);
    POINTERS_EQUAL(NULL, ptr_batch->messages);
    LONGS_EQUAL(0, ptr_batch->end_received);
    LONGS_EQUAL(0, ptr_batch->messages_processed);

    /* new messages with batch reference */
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test : \002this is a test ");
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :second test");
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :third test");
    CHECK_NO_MSG;

    /* end batch */
    RECV(":server BATCH -ref");
    CHECK_CHAN("bob", " this is a test ",
               "irc_privmsg,irc_tag_batch=ref,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    CHECK_CHAN("bob", "second test",
               "irc_privmsg,irc_tag_batch=ref,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    CHECK_CHAN("bob", "third test",
               "irc_privmsg,irc_tag_batch=ref,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    POINTERS_EQUAL(NULL, irc_batch_search (ptr_server, "ref"));

    /* start batch with parameters */
    RECV(":server BATCH +ref example param1 param2 param3 ");
    CHECK_NO_MSG;
    ptr_batch = irc_batch_search (ptr_server, "ref");
    CHECK(ptr_batch);
    STRCMP_EQUAL(NULL, ptr_batch->parent_ref);
    STRCMP_EQUAL("example", ptr_batch->type);
    STRCMP_EQUAL("param1 param2 param3", ptr_batch->parameters);
    POINTERS_EQUAL(NULL, ptr_batch->messages);
    LONGS_EQUAL(0, ptr_batch->end_received);
    LONGS_EQUAL(0, ptr_batch->messages_processed);

    /* new messages with batch reference */
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :test 1");
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :test 2");
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :test 3");
    CHECK_NO_MSG;

    /* end batch */
    RECV(":server BATCH -ref");
    CHECK_CHAN("bob", "test 1",
               "irc_privmsg,irc_tag_batch=ref,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    CHECK_CHAN("bob", "test 2",
               "irc_privmsg,irc_tag_batch=ref,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    CHECK_CHAN("bob", "test 3",
               "irc_privmsg,irc_tag_batch=ref,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    POINTERS_EQUAL(NULL, irc_batch_search (ptr_server, "ref"));

    /* start/end batch without parameters */
    RECV(":server BATCH +ref example");
    RECV(":server BATCH -ref");
    CHECK_NO_MSG;
    POINTERS_EQUAL(NULL, irc_batch_search (ptr_server, "ref"));

    /* interleaving batches */
    RECV(":server BATCH +1 example");
    CHECK_NO_MSG;
    CHECK(irc_batch_search (ptr_server, "1"));
    RECV("@batch=1 :bob!user_b@host_b PRIVMSG #test :message 1");
    CHECK_NO_MSG;
    RECV(":server BATCH +2 example");
    CHECK_NO_MSG;
    CHECK(irc_batch_search (ptr_server, "2"));
    RECV("@batch=1 :bob!user_b@host_b PRIVMSG #test :message 2");
    CHECK_NO_MSG;
    RECV("@batch=2 :bob!user_b@host_b PRIVMSG #test :message 4");
    CHECK_NO_MSG;
    RECV("@batch=1 :bob!user_b@host_b PRIVMSG #test :message 3");
    CHECK_NO_MSG;
    RECV(":server BATCH -1");
    CHECK_CHAN("bob", "message 1",
               "irc_privmsg,irc_tag_batch=1,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    CHECK_CHAN("bob", "message 2",
               "irc_privmsg,irc_tag_batch=1,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    CHECK_CHAN("bob", "message 3",
               "irc_privmsg,irc_tag_batch=1,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    POINTERS_EQUAL(NULL, irc_batch_search (ptr_server, "1"));
    RECV("@batch=2 :bob!user_b@host_b PRIVMSG #test :message 5");
    CHECK_NO_MSG;
    RECV(":server BATCH -2");
    CHECK_CHAN("bob", "message 4",
               "irc_privmsg,irc_tag_batch=2,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    CHECK_CHAN("bob", "message 5",
               "irc_privmsg,irc_tag_batch=2,irc_batch_type_example,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    POINTERS_EQUAL(NULL, irc_batch_search (ptr_server, "2"));

    /* nested batch */
    RECV(":server BATCH +ref1 example1");
    CHECK_NO_MSG;
    CHECK(irc_batch_search (ptr_server, "ref1"));
    RECV("@batch=ref1 :server BATCH +ref2 example2");
    CHECK_NO_MSG;
    CHECK(irc_batch_search (ptr_server, "ref2"));
    RECV("@batch=ref1 :bob!user_b@host_b PRIVMSG #test :test ref1");
    CHECK_NO_MSG;
    RECV("@batch=ref2 :bob!user_b@host_b PRIVMSG #test :test ref2");
    CHECK_NO_MSG;
    RECV(":server BATCH -ref2");
    CHECK_NO_MSG;
    CHECK(irc_batch_search (ptr_server, "ref2"));
    RECV(":server BATCH -ref1");
    CHECK_CHAN("bob", "test ref1",
               "irc_privmsg,irc_tag_batch=ref1,irc_batch_type_example1,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    CHECK_CHAN("bob", "test ref2",
               "irc_privmsg,irc_tag_batch=ref2,irc_batch_type_example2,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    POINTERS_EQUAL(NULL, irc_batch_search (ptr_server, "ref1"));
    POINTERS_EQUAL(NULL, irc_batch_search (ptr_server, "ref2"));

    /* multiline */
    RECV("@time=2023-08-09T07:43:01.830Z;msgid=icqfzy7zdbpix4gy8pvzuv49kw "
         ":server BATCH +ref draft/multiline #test");
    CHECK_NO_MSG;
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :line 1");
    CHECK_NO_MSG;
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :line 2");
    CHECK_NO_MSG;
    RECV(":server BATCH -ref");
    CHECK_CHAN("bob", "line 1",
               "irc_privmsg,irc_tag_batch=ref,irc_tag_time=2023-08-09T07:43:01.830Z,"
               "irc_tag_msgid=icqfzy7zdbpix4gy8pvzuv49kw,"
               "irc_batch_type_draft/multiline,notify_message,prefix_nick_248,"
               "nick_bob,host_user_b@host_b,log1");
    CHECK_CHAN("bob", "line 2",
               "irc_privmsg,irc_tag_batch=ref,irc_tag_time=2023-08-09T07:43:01.830Z,"
               "irc_tag_msgid=icqfzy7zdbpix4gy8pvzuv49kw,irc_batch_type_draft/multiline,"
               "notify_message,prefix_nick_248,nick_bob,host_user_b@host_b,log1");

    /* multiline with CTCP */
    RECV(":server BATCH +ref draft/multiline #test");
    CHECK_NO_MSG;
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :\001ACTION is testing");
    CHECK_NO_MSG;
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :again\001");
    CHECK_NO_MSG;
    RECV(":server BATCH -ref");
    CHECK_CHAN(" *", "bob is testing",
               "irc_privmsg,irc_tag_batch=ref,irc_batch_type_draft/multiline,"
               "irc_action,notify_message,nick_bob,host_user_b@host_b,log1");
    CHECK_CHAN("bob", "again\001",
               "irc_privmsg,irc_tag_batch=ref,irc_batch_type_draft/multiline,"
               "notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");
    RECV(":bob!user_b@host_b PRIVMSG #test :prout\001");
    CHECK_CHAN("bob", "prout\001",
               "irc_privmsg,notify_message,prefix_nick_248,nick_bob,"
               "host_user_b@host_b,log1");

    /* assume "draft/multiline" capability is enabled in server */
    hashtable_set (ptr_server->cap_list, "draft/multiline", NULL);
    irc_server_set_buffer_input_multiline (ptr_server, 1);

    /* multiline */
    RECV("@time=2023-08-09T07:43:01.830Z;msgid=icqfzy7zdbpix4gy8pvzuv49kw "
         ":server BATCH +ref draft/multiline #test");
    CHECK_NO_MSG;
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :line 1");
    CHECK_NO_MSG;
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :line 2");
    CHECK_NO_MSG;
    RECV(":server BATCH -ref");
    CHECK_CHAN("bob", "line 1\n"
               "line 2",
               "irc_privmsg,irc_tag_batch=ref,irc_tag_time=2023-08-09T07:43:01.830Z,"
               "irc_tag_msgid=icqfzy7zdbpix4gy8pvzuv49kw,"
               "irc_batch_type_draft/multiline,notify_message,prefix_nick_248,"
               "nick_bob,host_user_b@host_b,log1");

    /* multiline with CTCP */
    RECV(":server BATCH +ref draft/multiline #test");
    CHECK_NO_MSG;
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :\001ACTION is testing");
    CHECK_NO_MSG;
    RECV("@batch=ref :bob!user_b@host_b PRIVMSG #test :again\001");
    CHECK_NO_MSG;
    RECV(":server BATCH -ref");
    CHECK_CHAN(" *", "bob is testing\n"
               "again",
               "irc_privmsg,irc_tag_batch=ref,irc_batch_type_draft/multiline,"
               "irc_action,notify_message,nick_bob,host_user_b@host_b,log1");

    hashtable_remove (ptr_server->cap_list, "draft/multiline");
    irc_server_set_buffer_input_multiline (ptr_server, 0);

    hashtable_remove (ptr_server->cap_list, "batch");
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
    RECV("CAP * LS :multi-prefix sasl ");
    CHECK_SRV("--", "irc: client capability, server supports: multi-prefix sasl",
              "irc_cap,log3");
    RECV("CAP * LS * :multi-prefix sasl");
    CHECK_NO_MSG;
    RECV(":server CAP * LS :multi-prefix sasl");
    CHECK_SRV("--", "irc: client capability, server supports: multi-prefix sasl",
              "irc_cap,log3");
    RECV(":server CAP * LS * :multi-prefix sasl");
    CHECK_NO_MSG;

    /* CAP LIST */
    RECV("CAP * LIST : multi-prefix sasl ");
    CHECK_SRV("--", "irc: client capability, currently enabled: multi-prefix sasl",
              "irc_cap,log3");
    RECV("CAP * LIST * :multi-prefix sasl");
    CHECK_NO_MSG;
    RECV(":server CAP * LIST :multi-prefix sasl");
    CHECK_SRV("--", "irc: client capability, currently enabled: multi-prefix sasl",
              "irc_cap,log3");
    RECV(":server CAP * LIST * :multi-prefix sasl");
    CHECK_NO_MSG;

    /* CAP NEW */
    RECV("CAP * NEW : multi-prefix sasl ");
    CHECK_SRV("--", "irc: client capability, now available:  multi-prefix sasl ",
              "irc_cap,log3");
    RECV(":server CAP * NEW :multi-prefix sasl");
    CHECK_SRV("--", "irc: client capability, now available: multi-prefix sasl",
              "irc_cap,log3");

    /* CAP DEL */
    RECV("CAP * DEL : multi-prefix sasl ");
    CHECK_SRV("--", "irc: client capability, removed:  multi-prefix sasl ",
              "irc_cap,log3");
    RECV(":server CAP * DEL :multi-prefix sasl");
    CHECK_SRV("--", "irc: client capability, removed: multi-prefix sasl",
              "irc_cap,log3");

    /* CAP ACK */
    RECV("CAP * ACK : sasl ");
    CHECK_SRV("--", "irc: client capability, enabled: sasl", "irc_cap,log3");
    RECV(":server CAP * ACK :sasl");
    CHECK_SRV("--", "irc: client capability, enabled: sasl", "irc_cap,log3");

    /* CAP NAK */
    RECV("CAP * NAK : sasl ");
    CHECK_SRV("=!=", "irc: client capability, refused:  sasl ", "irc_cap,log3");
    RECV(":server CAP * NAK :sasl");
    CHECK_SRV("=!=", "irc: client capability, refused: sasl", "irc_cap,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_chghost
 */

TEST(IrcProtocolWithServer, chghost)
{
    struct t_irc_nick *ptr_nick, *ptr_nick2;

    SRV_INIT_JOIN2;

    RECV(":bob!user_\00304red@host_\00304red PRIVMSG alice :hi Alice!");

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
    RECV(":alice!user@host CHGHOST user2 host2 ");
    CHECK_CHAN("--", "alice (user@host) has changed host to user2@host2",
               "irc_chghost,new_host_user2@host2,nick_alice,host_user@host,log3");
    STRCMP_EQUAL("user2@host2", ptr_nick->host);

    RECV(":alice!user@host CHGHOST user2 host2");
    CHECK_CHAN("--", "alice (user@host) has changed host to user2@host2",
               "irc_chghost,new_host_user2@host2,nick_alice,host_user@host,log3");
    STRCMP_EQUAL("user2@host2", ptr_nick->host);

    RECV(":alice!user2@host2 CHGHOST user3 :host3");
    CHECK_CHAN("--", "alice (user2@host2) has changed host to user3@host3",
               "irc_chghost,new_host_user3@host3,nick_alice,host_user2@host2,log3");
    STRCMP_EQUAL("user3@host3", ptr_nick->host);

    /* another nick */
    RECV(":bob!user_\00304red@host_\00304red CHGHOST user_\00302blue host_\00302blue");
    CHECK_CHAN("--",
               "bob (user_red@host_red) has changed host to user_blue@host_blue",
               "irc_chghost,new_host_user_\00302blue@host_\00302blue,irc_smart_filter,"
               "nick_bob,host_user_\00304red@host_\00304red,log3");
    STRCMP_EQUAL("user_\00302blue@host_\00302blue", ptr_nick2->host);
    CHECK_PV("bob",
             "--",
             "bob (user_red@host_red) has changed host to user_blue@host_blue",
             "irc_chghost,new_host_user_\00302blue@host_\00302blue,nick_bob,"
             "host_user_\00304red@host_\00304red,log3");
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
    CHECK_SRV("=!=", "test", "irc_error,log3");
    RECV("ERROR : Closing Link: irc.server.org (\002Bad Password\002) ");
    CHECK_SRV("=!=", " Closing Link: irc.server.org (Bad Password) ",
              "irc_error,log3");
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
    CHECK_SRV("=!=", "Failure: [] TEST", "irc_fail,nick_server,log3");
    RECV(":server FAIL * TEST : \002the message ");
    CHECK_SRV("=!=", "Failure: [TEST]  the message ", "irc_fail,nick_server,log3");
    RECV(":server FAIL * TEST TEST2");
    CHECK_SRV("=!=", "Failure: [TEST] TEST2", "irc_fail,nick_server,log3");
    RECV(":server FAIL * TEST TEST2 :the message");
    CHECK_SRV("=!=", "Failure: [TEST TEST2] the message",
              "irc_fail,nick_server,log3");

    RECV(":server FAIL COMMAND TEST");
    CHECK_SRV("=!=", "Failure: COMMAND [] TEST", "irc_fail,nick_server,log3");
    RECV(":server FAIL COMMAND TEST :the message");
    CHECK_SRV("=!=", "Failure: COMMAND [TEST] the message",
              "irc_fail,nick_server,log3");
    RECV(":server FAIL COMMAND TEST TEST2");
    CHECK_SRV("=!=", "Failure: COMMAND [TEST] TEST2", "irc_fail,nick_server,log3");
    RECV(":server FAIL COMMAND TEST TEST2 :the message");
    CHECK_SRV("=!=", "Failure: COMMAND [TEST TEST2] the message",
              "irc_fail,nick_server,log3");
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

    RECV(":bob!user@host INVITE alice #channel ");
    CHECK_SRV("--", "You have been invited to #channel by bob",
              "irc_invite,notify_highlight,nick_bob,host_user@host,log3");
    RECV(":bob!user@host INVITE xxx #channel");
    CHECK_SRV("--", "bob has invited xxx to #channel",
              "irc_invite,nick_bob,host_user@host,log3");
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

    RECV(":alice!user@host JOIN #test ");
    CHECK_CHAN("-->", "alice (user@host) has joined #test",
               "irc_join,nick_alice,host_user@host,log4");

    ptr_channel = ptr_server->channels;

    CHECK(ptr_channel);
    CHECK(ptr_channel == ptr_server->last_channel);

    LONGS_EQUAL(IRC_CHANNEL_TYPE_CHANNEL, ptr_channel->type);
    STRCMP_EQUAL("#test", ptr_channel->name);
    STRCMP_EQUAL(NULL, ptr_channel->topic);
    STRCMP_EQUAL(NULL, ptr_channel->modes);
    LONGS_EQUAL(0, ptr_channel->limit);
    STRCMP_EQUAL(NULL, ptr_channel->key);
    LONGS_EQUAL(0, ptr_channel->checking_whox);
    STRCMP_EQUAL(NULL, ptr_channel->away_message);
    LONGS_EQUAL(0, ptr_channel->has_quit_server);
    LONGS_EQUAL(0, ptr_channel->cycle);
    LONGS_EQUAL(0, ptr_channel->part);
    LONGS_EQUAL(0, ptr_channel->part);
    STRCMP_EQUAL(NULL, ptr_channel->pv_remote_nick_color);
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
    STRCMP_EQUAL(NULL, ptr_nick->account);
    STRCMP_EQUAL(NULL, ptr_nick->realname);
    CHECK(ptr_nick->color);

    CHECK(ptr_channel->buffer);

    RECV(":bob!user@host JOIN #test  *  :   ");
    CHECK_CHAN("-->", "bob (   ) (user@host) has joined #test",
               "irc_join,irc_smart_filter,nick_bob,host_user@host,log4");

    ptr_nick = ptr_channel->last_nick;

    LONGS_EQUAL(2, ptr_channel->nicks_count);
    CHECK(ptr_nick);
    STRCMP_EQUAL("bob", ptr_nick->name);
    STRCMP_EQUAL("user@host", ptr_nick->host);
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);
    LONGS_EQUAL(0, ptr_nick->away);
    STRCMP_EQUAL(NULL, ptr_nick->account);
    STRCMP_EQUAL("   ", ptr_nick->realname);
    CHECK(ptr_nick->color);

    RECV(":carol!user@host JOIN #test carol_account : \002Carol Name ");
    CHECK_CHAN("-->",
               "carol [carol_account] ( Carol Name ) (user@host) "
               "has joined #test",
               "irc_join,irc_smart_filter,nick_carol,host_user@host,log4");

    ptr_nick = ptr_channel->last_nick;

    LONGS_EQUAL(3, ptr_channel->nicks_count);
    CHECK(ptr_nick);
    STRCMP_EQUAL("carol", ptr_nick->name);
    STRCMP_EQUAL("user@host", ptr_nick->host);
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);
    LONGS_EQUAL(0, ptr_nick->away);
    STRCMP_EQUAL("carol_account", ptr_nick->account);
    STRCMP_EQUAL(" \002Carol Name ", ptr_nick->realname);
    CHECK(ptr_nick->color);

    /* join with option irc.look.display_host_join set to off */
    config_file_option_set (irc_config_look_display_host_join, "off", 1);
    RECV(":dan!user@host JOIN #test");
    CHECK_CHAN("-->",
               "dan has joined #test",
               "irc_join,irc_smart_filter,nick_dan,host_user@host,log4");
    config_file_option_reset (irc_config_look_display_host_join, 1);

    /* join with option irc.look.display_host_join_local set to off */
    config_file_option_set (irc_config_look_display_host_join_local, "off", 1);
    RECV(":alice!user@host PART #test");
    RECV(":alice!user@host JOIN #test");
    CHECK_CHAN("-->", "alice has joined #test",
               "irc_join,nick_alice,host_user@host,log4");
    config_file_option_reset (irc_config_look_display_host_join_local, 1);
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
    CHECK_CHAN("-->", "alice (user@host) has joined #test",
               "irc_join,nick_alice,host_user@host,log4");
    RECV(":bob!user@host JOIN #test");
    CHECK_CHAN("-->", "bob (user@host) has joined #test",
               "irc_join,irc_smart_filter,nick_bob,host_user@host,log4");

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
    RECV(":alice!user@host KICK #xyz bob :\002the reason");
    CHECK_NO_MSG;

    /* kick without a reason */
    RECV(":alice!user@host KICK #test bob");
    CHECK_CHAN("<--", "alice has kicked bob",
               "irc_kick,nick_alice,host_user@host,log3");
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    RECV(":bob!user@host JOIN #test");
    CHECK_CHAN("-->", "bob (user@host) has joined #test",
               "irc_join,irc_smart_filter,nick_bob,host_user@host,log4");

    /* with kick a reason */
    RECV(":alice!user@host KICK #test bob :\002no spam here! ");
    CHECK_CHAN("<--", "alice has kicked bob (no spam here! )",
               "irc_kick,nick_alice,host_user@host,log3");
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    RECV(":bob!user@host JOIN #test");
    CHECK_CHAN("-->", "bob (user@host) has joined #test",
               "irc_join,irc_smart_filter,nick_bob,host_user@host,log4");

    /* kick of self nick */
    RECV(":bob!user@host KICK #test alice :\002no spam here! ");
    CHECK_CHAN("<--", "bob has kicked alice (no spam here! )",
               "irc_kick,nick_bob,host_user@host,log3");
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
    CHECK_CHAN("<--", "You were killed by bob",
               "irc_kill,nick_bob,host_user@host,log3");
    POINTERS_EQUAL(NULL, ptr_channel->nicks);

    RECV(":alice!user@host JOIN #test");
    CHECK_CHAN("-->", "alice (user@host) has joined #test",
               "irc_join,nick_alice,host_user@host,log4");
    RECV(":bob!user@host JOIN #test");
    CHECK_CHAN("-->", "bob (user@host) has joined #test",
               "irc_join,irc_smart_filter,nick_bob,host_user@host,log4");

    /* kill with a reason */
    RECV(":bob!user@host KILL alice :\002killed by admin ");
    CHECK_CHAN("<--", "You were killed by bob (killed by admin )",
               "irc_kill,nick_bob,host_user@host,log3");
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
    STRCMP_EQUAL(NULL, ptr_channel->modes);
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

    STRCMP_EQUAL(NULL, ptr_channel->modes);

    /* channel mode */
    RECV(":admin!user@host MODE #test +nt ");
    CHECK_CHAN("--", "Mode #test [+nt] by admin",
               "irc_mode,nick_admin,host_user@host,log3");
    STRCMP_EQUAL("+tn", ptr_channel->modes);

    /* channel mode removed */
    RECV(":admin!user@host MODE #test -n");
    CHECK_CHAN("--", "Mode #test [-n] by admin",
               "irc_mode,nick_admin,host_user@host,log3");
    STRCMP_EQUAL("+t", ptr_channel->modes);

    /* channel mode removed */
    RECV(":admin!user@host MODE #test -t");
    CHECK_CHAN("--", "Mode #test [-t] by admin",
               "irc_mode,nick_admin,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_channel->modes);

    /* nick mode '@' on channel #test */
    RECV(":admin!user@host MODE #test +o alice ");
    CHECK_CHAN("--", "Mode #test [+o alice] by admin",
               "irc_mode,nick_admin,host_user@host,log3");
    STRCMP_EQUAL("@ ", ptr_nick->prefixes);
    STRCMP_EQUAL("@", ptr_nick->prefix);

    /* another nick mode '+' on channel #test */
    RECV(":admin!user@host MODE #test +v alice");
    CHECK_CHAN("--", "Mode #test [+v alice] by admin",
               "irc_mode,nick_admin,host_user@host,log3");
    STRCMP_EQUAL("@+", ptr_nick->prefixes);
    STRCMP_EQUAL("@", ptr_nick->prefix);

    /* nick mode '@' removed on channel #test */
    RECV(":admin!user@host MODE #test -o alice");
    CHECK_CHAN("--", "Mode #test [-o alice] by admin",
               "irc_mode,nick_admin,host_user@host,log3");
    STRCMP_EQUAL(" +", ptr_nick->prefixes);
    STRCMP_EQUAL("+", ptr_nick->prefix);

    /* nick mode '+' removed on channel #test */
    RECV(":admin!user@host MODE #test -v alice");
    CHECK_CHAN("--", "Mode #test [-v alice] by admin",
               "irc_mode,nick_admin,host_user@host,log3");
    STRCMP_EQUAL("  ", ptr_nick->prefixes);
    STRCMP_EQUAL(" ", ptr_nick->prefix);

    /* nick mode 'i' */
    STRCMP_EQUAL(NULL, ptr_server->nick_modes);
    RECV(":admin!user@host MODE alice +i");
    CHECK_SRV("--", "User mode [+i] by admin",
              "irc_mode,nick_admin,host_user@host,log3");
    STRCMP_EQUAL("i", ptr_server->nick_modes);

    /* nick mode 'R' */
    RECV(":admin!user@host MODE alice +R");
    CHECK_SRV("--", "User mode [+R] by admin",
              "irc_mode,nick_admin,host_user@host,log3");
    STRCMP_EQUAL("iR", ptr_server->nick_modes);

    /* remove nick mode 'i' */
    RECV(":admin!user@host MODE alice -i");
    CHECK_SRV("--", "User mode [-i] by admin",
              "irc_mode,nick_admin,host_user@host,log3");
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
    CHECK_PV("bob", "bob", "hi Alice!",
             "irc_privmsg,notify_private,prefix_nick_248,nick_bob,"
             "host_user@host,log1");

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
    RECV(":alice!user@host NICK alice_away ");
    CHECK_SRV("--", "You are now known as alice_away",
              "irc_nick,irc_nick1_alice,irc_nick2_alice_away,"
              "nick_alice,host_user@host,log2");
    CHECK_CHAN("--", "You are now known as alice_away",
               "irc_nick,irc_nick1_alice,irc_nick2_alice_away,"
               "nick_alice,host_user@host,log2");
    STRCMP_EQUAL("alice_away", ptr_nick1->name);

    /* new nick for alice_away (with ":") */
    RECV(":alice_away!user@host NICK :alice2");
    CHECK_SRV("--", "You are now known as alice2",
              "irc_nick,irc_nick1_alice_away,irc_nick2_alice2,"
              "nick_alice_away,host_user@host,log2");
    CHECK_CHAN("--", "You are now known as alice2",
               "irc_nick,irc_nick1_alice_away,irc_nick2_alice2,"
               "nick_alice_away,host_user@host,log2");
    STRCMP_EQUAL("alice2", ptr_nick1->name);

    /* new nick for bob */
    RECV(":bob!user@host NICK bob_away");
    CHECK_CHAN("--", "bob is now known as bob_away",
               "irc_nick,irc_smart_filter,irc_nick1_bob,irc_nick2_bob_away,"
               "nick_bob,host_user@host,log2");
    CHECK_PV("bob_away", "--", "bob is now known as bob_away",
             "irc_nick,irc_nick1_bob,irc_nick2_bob_away,nick_bob,host_user@host,"
             "log2");
    STRCMP_EQUAL("bob_away", ptr_nick2->name);

    /* new nick for bob_away (with ":") */
    RECV(":bob_away!user@host NICK :bob2");
    CHECK_CHAN("--", "bob_away is now known as bob2",
               "irc_nick,irc_smart_filter,irc_nick1_bob_away,"
               "irc_nick2_bob2,nick_bob_away,host_user@host,log2");
    CHECK_PV("bob2", "--", "bob_away is now known as bob2",
             "irc_nick,irc_nick1_bob_away,irc_nick2_bob2,"
             "nick_bob_away,host_user@host,log2");
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
    CHECK_SRV("--", "Note: [] TEST", "irc_note,nick_server,log3");
    RECV(":server NOTE * TEST : \002the message ");
    CHECK_SRV("--", "Note: [TEST]  the message ", "irc_note,nick_server,log3");
    RECV(":server NOTE * TEST TEST2");
    CHECK_SRV("--", "Note: [TEST] TEST2", "irc_note,nick_server,log3");
    RECV(":server NOTE * TEST TEST2 :the message");
    CHECK_SRV("--", "Note: [TEST TEST2] the message", "irc_note,nick_server,log3");

    RECV(":server NOTE COMMAND TEST");
    CHECK_SRV("--", "Note: COMMAND [] TEST", "irc_note,nick_server,log3");
    RECV(":server NOTE COMMAND TEST :the message");
    CHECK_SRV("--", "Note: COMMAND [TEST] the message", "irc_note,nick_server,log3");
    RECV(":server NOTE COMMAND TEST TEST2");
    CHECK_SRV("--", "Note: COMMAND [TEST] TEST2", "irc_note,nick_server,log3");
    RECV(":server NOTE COMMAND TEST TEST2 :the message");
    CHECK_SRV("--", "Note: COMMAND [TEST TEST2] the message",
              "irc_note,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_notice
 */

TEST(IrcProtocolWithServer, notice)
{
    int echo_message;

    SRV_INIT_JOIN;

    /* test without and with capability "echo-message" */
    for (echo_message = 0; echo_message < 2; echo_message++)
    {
        if (echo_message == 1)
        {
            /* assume "echo-message" capability is enabled in server */
            hashtable_set (ptr_server->cap_list, "echo-message", NULL);
        }

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
        RECV("NOTICE AUTH :\002*** Looking up your hostname... ");
        CHECK_SRV("--", "*** Looking up your hostname... ", "irc_notice,log1");
        RECV(":server.address NOTICE AUTH :*** Looking up your hostname... ");
        CHECK_SRV("--", "server.address: *** Looking up your hostname... ",
                  "irc_notice,notify_private,nick_server.address,log1");
        RECV(":server.address NOTICE * :*** Looking up your hostname... ");
        CHECK_SRV("--", "server.address: *** Looking up your hostname... ",
                  "irc_notice,notify_private,nick_server.address,log1");

        /* notice to channel/user */
        RECV(":server.address NOTICE #test :\002a notice ");
        CHECK_CHAN("--", "Notice(server.address) -> #test: a notice ",
                   "irc_notice,notify_message,nick_server.address,log1");
        RECV(":server.address NOTICE alice :a notice ");
        CHECK_SRV("--", "server.address: a notice ",
                  "irc_notice,notify_private,nick_server.address,log1");
        RECV(":bob!user@host NOTICE #test :a notice ");
        CHECK_CHAN("--", "Notice(bob) -> #test: a notice ",
                   "irc_notice,notify_message,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :a notice ");
        CHECK_SRV("--", "bob (user@host): a notice ",
                  "irc_notice,notify_private,nick_bob,host_user@host,log1");

        /* notice to channel/user with color in address */
        RECV(":bob!user_\00304red@host_\00304red NOTICE #test :a notice ");
        CHECK_CHAN("--", "Notice(bob) -> #test: a notice ",
                   "irc_notice,notify_message,nick_bob,host_user_\00304red@host_\00304red,log1");
        RECV(":bob!user_\00304red@host_\00304red NOTICE alice :a notice ");
        CHECK_SRV("--", "bob (user_red@host_red): a notice ",
                  "irc_notice,notify_private,nick_bob,host_user_\00304red@host_\00304red,log1");

        /* notice to channel/user with option irc.look.display_host_notice set to off */
        config_file_option_set (irc_config_look_display_host_notice, "off", 1);
        RECV(":server.address NOTICE #test :\002a notice ");
        CHECK_CHAN("--", "Notice(server.address) -> #test: a notice ",
                   "irc_notice,notify_message,nick_server.address,log1");
        RECV(":server.address NOTICE alice :a notice ");
        CHECK_SRV("--", "server.address: a notice ",
                  "irc_notice,notify_private,nick_server.address,log1");
        RECV(":bob!user@host NOTICE #test :a notice ");
        CHECK_CHAN("--", "Notice(bob) -> #test: a notice ",
                   "irc_notice,notify_message,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :a notice ");
        CHECK_SRV("--", "bob: a notice ",
                  "irc_notice,notify_private,nick_bob,host_user@host,log1");
        config_file_option_reset (irc_config_look_display_host_notice, 1);

        /*
         * notice to channel/user from self nick
         * (case of bouncer or if echo-message capability is enabled)
         */
        RECV(":alice!user@host NOTICE #test :\002a notice ");
        CHECK_CHAN("--", "Notice(alice) -> #test: a notice ",
                   "irc_notice,self_msg,notify_none,no_highlight,nick_alice,"
                   "host_user@host,log1");

        /* notice to ops of channel */
        RECV(":server.address NOTICE @#test :\002a notice ");
        CHECK_CHAN("--", "Notice(server.address) -> @#test: a notice ",
                   "irc_notice,notify_message,nick_server.address,log1");
        RECV(":bob!user@host NOTICE @#test :a notice ");
        CHECK_CHAN("--", "Notice(bob) -> @#test: a notice ",
                   "irc_notice,notify_message,nick_bob,host_user@host,log1");

        /*
         * notice to ops of channel from self nick
         * (case of bouncer or if echo-message capability is enabled)
         */
        RECV(":alice!user@host NOTICE @#test :\002a notice ");
        CHECK_CHAN("--", "Notice(alice) -> @#test: a notice ",
                   "irc_notice,self_msg,notify_none,no_highlight,nick_alice,"
                   "host_user@host,log1");

        /*
         * notice from self nick
         * (case of bouncer or if echo-message capability is enabled)
         */
        RECV(":alice!user@host NOTICE alice :\002a notice ");
        CHECK_SRV("--", "Notice -> alice: a notice ",
                  "irc_notice,notify_private,nick_alice,host_user@host,log1");

        /* notice with channel name at beginning */
        RECV(":server.address NOTICE alice :[#test] \002a notice ");
        CHECK_CHAN("--", "PvNotice(server.address): a notice ",
                   "irc_notice,nick_server.address,log1");
        RECV(":server.address NOTICE alice :(#test) a notice ");
        CHECK_CHAN("--", "PvNotice(server.address): a notice ",
                   "irc_notice,nick_server.address,log1");
        RECV(":server.address NOTICE alice :{#test} a notice ");
        CHECK_CHAN("--", "PvNotice(server.address): a notice ",
                   "irc_notice,nick_server.address,log1");
        RECV(":server.address NOTICE alice :<#test> a notice ");
        CHECK_CHAN("--", "PvNotice(server.address): a notice ",
                   "irc_notice,nick_server.address,log1");
        RECV(":bob!user@host NOTICE alice :[#test] a notice ");
        CHECK_CHAN("--", "PvNotice(bob): a notice ",
                   "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :(#test) a notice ");
        CHECK_CHAN("--", "PvNotice(bob): a notice ",
                   "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :{#test} a notice ");
        CHECK_CHAN("--", "PvNotice(bob): a notice ",
                   "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :<#test> a notice ");
        CHECK_CHAN("--", "PvNotice(bob): a notice ",
                   "irc_notice,nick_bob,host_user@host,log1");

        /*
         * notice to another nick with channel name at beginning
         * (case of a notice sent if echo-message capability is enabled)
         */
        RECV(":alice!user@host NOTICE bob :[#test] \002a notice ");
        CHECK_SRV("--", "Notice -> bob: [#test] a notice ",
                   "irc_notice,notify_private,nick_alice,host_user@host,log1");

        /* broken CTCP to channel */
        RECV(":bob!user@host NOTICE #test :\001");
        CHECK_SRV("--", "CTCP reply from bob: ",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001TEST");
        CHECK_SRV("--", "CTCP reply from bob: TEST",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001ACTION");
        CHECK_SRV("--", "CTCP reply from bob: ACTION",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001ACTION is testing");
        CHECK_SRV("--", "CTCP reply from bob: ACTION is testing",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001VERSION");
        CHECK_SRV("--", "CTCP reply from bob: VERSION",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001DCC");
        CHECK_SRV("--", "CTCP reply from bob: DCC",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001DCC SEND");
        CHECK_SRV("--", "CTCP reply from bob: DCC SEND",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001DCC SEND file.txt");
        CHECK_SRV("--", "CTCP reply from bob: DCC SEND file.txt",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001DCC SEND file.txt 1 2 3");
        CHECK_SRV("--", "CTCP reply from bob: DCC SEND file.txt 1 2 3",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");

        /* broken CTCP to user */
        RECV(":bob!user@host NOTICE alice :\001");
        CHECK_SRV("--", "CTCP reply from bob: ",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001TEST");
        CHECK_SRV("--", "CTCP reply from bob: TEST",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001ACTION");
        CHECK_SRV("--", "CTCP reply from bob: ACTION",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001ACTION is testing");
        CHECK_SRV("--", "CTCP reply from bob: ACTION is testing",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001VERSION");
        CHECK_SRV("--", "CTCP reply from bob: VERSION",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001DCC");
        CHECK_SRV("--", "CTCP reply from bob: DCC",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001DCC SEND");
        CHECK_SRV("--", "CTCP reply from bob: DCC SEND",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001DCC SEND file.txt");
        CHECK_SRV("--", "CTCP reply from bob: DCC SEND file.txt",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001DCC SEND file.txt 1 2 3");
        CHECK_SRV("--", "CTCP reply from bob: DCC SEND file.txt 1 2 3",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");

        /* valid CTCP to channel */
        RECV(":bob!user@host NOTICE #test :\001TEST\001");
        CHECK_SRV("--", "CTCP reply from bob: TEST",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001ACTION\001");
        CHECK_SRV("--", "CTCP reply from bob: ACTION",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001ACTION is testing\001");
        CHECK_SRV("--", "CTCP reply from bob: ACTION is testing",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001VERSION\001");
        CHECK_SRV("--", "CTCP reply from bob: VERSION",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE #test :\001DCC SEND file.txt 1 2 3\001");
        CHECK_SRV("--", "CTCP reply from bob: DCC SEND file.txt 1 2 3",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");

        /* valid CTCP to user */
        RECV(":bob!user@host NOTICE alice :\001TEST\001");
        CHECK_SRV("--", "CTCP reply from bob: TEST",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001ACTION\001");
        CHECK_SRV("--", "CTCP reply from bob: ACTION",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001ACTION is testing\001");
        CHECK_SRV("--", "CTCP reply from bob: ACTION is testing",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001VERSION\001");
        CHECK_SRV("--", "CTCP reply from bob: VERSION",
                  "irc_notice,nick_bob,host_user@host,log1");
        RECV(":bob!user@host NOTICE alice :\001DCC SEND file.txt 1 2 3\001");
        CHECK_SRV("--", "CTCP reply from bob: DCC SEND file.txt 1 2 3",
                  "irc_notice,irc_ctcp,nick_bob,host_user@host,log1");

        if (echo_message == 1)
            hashtable_remove (ptr_server->cap_list, "echo-message");
    }
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
    CHECK_CHAN("<--", "alice (user@host) has left #test",
               "irc_part,nick_alice,host_user@host,log4");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    POINTERS_EQUAL(NULL, ptr_server->channels->nicks);
    LONGS_EQUAL(1, ptr_server->channels->part);

    /* without part message (but empty trailing parameter) */
    RECV(":alice!user@host JOIN #test");
    RECV(":alice!user@host PART #test :");
    CHECK_CHAN("<--", "alice (user@host) has left #test",
               "irc_part,nick_alice,host_user@host,log4");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    POINTERS_EQUAL(NULL, ptr_server->channels->nicks);
    LONGS_EQUAL(1, ptr_server->channels->part);

    /* with part message */
    RECV(":alice!user@host JOIN #test");
    RECV(":alice!user@host PART #test :\002part message ");
    CHECK_CHAN("<--", "alice (user@host) has left #test (part message )",
               "irc_part,nick_alice,host_user@host,log4");
    STRCMP_EQUAL("#test", ptr_server->channels->name);
    POINTERS_EQUAL(NULL, ptr_server->channels->nicks);
    LONGS_EQUAL(1, ptr_server->channels->part);

    RECV(":alice!user@host JOIN #test");
    CHECK_CHAN("-->", "alice (user@host) has joined #test",
               "irc_join,nick_alice,host_user@host,log4");
    RECV(":bob!user@host JOIN #test");
    CHECK_CHAN("-->", "bob (user@host) has joined #test",
               "irc_join,irc_smart_filter,nick_bob,host_user@host,log4");

    /* part from another user */
    RECV(":bob!user@host PART #test :part message ");
    CHECK_CHAN("<--", "bob (user@host) has left #test (part message )",
               "irc_part,irc_smart_filter,nick_bob,host_user@host,log4");
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

    RECV("PING :\002123456789 ");
    CHECK_NO_MSG;
    CHECK_SENT("PONG :\002123456789 ");
}

/*
 * Tests functions:
 *   irc_protocol_cb_pong
 */

TEST(IrcProtocolWithServer, pong)
{
    SRV_INIT;

    RECV(":server PONG");
    CHECK_SRV("", "PONG", "irc_pong,nick_server,log3");
    RECV(":server PONG server");
    CHECK_SRV("", "PONG", "irc_pong,nick_server,log3");
    RECV(":server PONG server : \002info ");
    CHECK_SRV("", "PONG:  info ", "irc_pong,nick_server,log3");
    RECV(":server PONG server :extra info");
    CHECK_SRV("", "PONG: extra info", "irc_pong,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_privmsg
 */

TEST(IrcProtocolWithServer, privmsg)
{
    char *info, message[1024];
    int echo_message;

    SRV_INIT_JOIN2;

    /* test without and with capability "echo-message" */
    for (echo_message = 0; echo_message < 2; echo_message++)
    {
        if (echo_message == 1)
        {
            /* assume "echo-message" capability is enabled in server */
            hashtable_set (ptr_server->cap_list, "echo-message", NULL);
        }

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
        RECV(":bob!user@host PRIVMSG #test :\002this is the message ");
        CHECK_CHAN_DATE_NOW("bob", "this is the message ",
                            "irc_privmsg,notify_message,prefix_nick_248,nick_bob,"
                            "host_user@host,log1");
        RECV(":bob!user@host PRIVMSG alice :this is the message ");
        CHECK_PV_CLOSE("bob", "bob", "this is the message ",
                       "irc_privmsg,notify_private,prefix_nick_248,nick_bob,"
                       "host_user@host,log1");

        /* message with tags to channel/user */
        RECV("@tag1=value1;tag2=value2 :bob!user@host PRIVMSG #test "
             ":\002this is the message ");
        CHECK_CHAN_DATE_NOW("bob", "this is the message ",
                            "irc_privmsg,irc_tag_tag1=value1,irc_tag_tag2=value2,"
                            "notify_message,prefix_nick_248,nick_bob,host_user@host,log1");
        RECV("@tag1=value1;tag2=value2 :bob!user@host PRIVMSG alice "
             ":this is the message ");
        CHECK_PV_CLOSE("bob", "bob", "this is the message ",
                       "irc_privmsg,irc_tag_tag1=value1,irc_tag_tag2=value2,"
                       "notify_private,prefix_nick_248,nick_bob,host_user@host,log1");

        /* message with tags + time as timestamp to channel/user */
        RECV("@tag1=value1;tag2=value2;time=1703500149 :bob!user@host PRIVMSG #test "
             ":\002this is the message ");
        CHECK_CHAN_DATE_VALUE(
            "bob",
            "this is the message ",
            "irc_privmsg,irc_tag_tag1=value1,irc_tag_tag2=value2,"
            "irc_tag_time=1703500149,notify_message,prefix_nick_248,nick_bob,"
            "host_user@host,log1",
            1703500149, 0);

        /* message with tags + time as timestamp with milliseconds to channel/user */
        RECV("@tag1=value1;tag2=value2;time=1703500149.456 :bob!user@host PRIVMSG #test "
             ":\002this is the message ");
        CHECK_CHAN_DATE_VALUE(
            "bob",
            "this is the message ",
            "irc_privmsg,irc_tag_tag1=value1,irc_tag_tag2=value2,"
            "irc_tag_time=1703500149.456,notify_message,prefix_nick_248,nick_bob,"
            "host_user@host,log1",
            1703500149, 456000);

        /* message with tags + time as timestamp with microseconds to channel/user */
        RECV("@tag1=value1;tag2=value2;time=1703500149.456789 :bob!user@host PRIVMSG #test "
             ":\002this is the message ");
        CHECK_CHAN_DATE_VALUE(
            "bob",
            "this is the message ",
            "irc_privmsg,irc_tag_tag1=value1,irc_tag_tag2=value2,"
            "irc_tag_time=1703500149.456789,notify_message,prefix_nick_248,nick_bob,"
            "host_user@host,log1",
            1703500149, 456789);

        /* message with tags + time as ISO 8601 with microseconds to channel/user */
        RECV("@tag1=value1;tag2=value2;time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG #test :\002this is the message ");
        CHECK_CHAN_DATE_VALUE(
            "bob",
            "this is the message ",
            "irc_privmsg,irc_tag_tag1=value1,irc_tag_tag2=value2,"
            "irc_tag_time=2023-12-25T10:29:09.456789Z,notify_message,"
            "prefix_nick_248,nick_bob,"
            "host_user@host,log1",
            1703500149, 456789);

        /*
         * message to channel/user from self nick
         * (case of bouncer or if echo-message capability is enabled)
         */
        RECV(":alice!user@host PRIVMSG #test :\002this is the message ");
        CHECK_CHAN("alice", "this is the message ",
                   "irc_privmsg,self_msg,notify_none,no_highlight,"
                   "prefix_nick_white,nick_alice,host_user@host,log1");

        /* message to ops of channel */
        RECV(":bob!user@host PRIVMSG @#test :\002this is the message ");
        CHECK_CHAN("--", "Msg(bob) -> @#test: this is the message ",
                   "irc_privmsg,notify_message,nick_bob,host_user@host,log1");

        /*
         * message to ops of channel from self nick
         * (case of bouncer or if echo-message capability is enabled)
         */
        RECV(":alice!user@host PRIVMSG @#test :\002this is the message ");
        CHECK_CHAN("--", "Msg(alice) -> @#test: this is the message ",
                   "irc_privmsg,self_msg,notify_none,no_highlight,nick_alice,"
                   "host_user@host,log1");

        /*
         * message from self nick in private
         * (case of bouncer or if echo-message capability is enabled)
         */
        if (echo_message == 0)
        {
            /* without echo-message */
            RECV(":alice!user@host PRIVMSG bob :\002this is the message ");
            CHECK_PV_CLOSE("bob", "alice", "this is the message ",
                           "irc_privmsg,self_msg,notify_none,no_highlight,"
                           "prefix_nick_white,nick_alice,host_user@host,log1");
        }
        else
        {
            /* with echo-message */
            RECV(":alice!user@host PRIVMSG bob :\002this is the message ");
            CHECK_PV_CLOSE("bob", "alice", "this is the message ",
                           "irc_privmsg,self_msg,notify_none,no_highlight,"
                           "prefix_nick_white,nick_alice,host_user@host,log1");
            /* with echo-message, option irc.look.open_pv_buffer_echo_msg off */
            config_file_option_set (irc_config_look_open_pv_buffer_echo_msg,
                                    "off", 1);
            RECV(":alice!user@host PRIVMSG bob :\002this is the message ");
            CHECK_SRV("--", "Msg(alice) -> bob: this is the message ",
                      "irc_privmsg,self_msg,notify_none,no_highlight,"
                      "nick_alice,host_user@host,log1");
            config_file_option_reset (irc_config_look_open_pv_buffer_echo_msg, 1);
        }

        /*
         * message from self nick in private, with password hidden (nickserv)
         * (case of bouncer or if echo-message capability is enabled)
         */
        if (echo_message == 0)
        {
            /* without echo-message */
            RECV(":alice!user@host PRIVMSG nickserv :identify secret");
            CHECK_PV_CLOSE("nickserv", "alice", "identify ******",
                           "irc_privmsg,self_msg,notify_none,no_highlight,"
                           "prefix_nick_white,nick_alice,host_user@host,log1");
        }
        else
        {
            /* with echo-message */
            RECV(":alice!user@host PRIVMSG nickserv :identify secret");
            CHECK_PV_CLOSE("nickserv", "alice", "identify ******",
                           "irc_privmsg,self_msg,notify_none,no_highlight,"
                           "prefix_nick_white,nick_alice,host_user@host,log1");
            /* with echo-message, option irc.look.open_pv_buffer_echo_msg off */
            config_file_option_set (irc_config_look_open_pv_buffer_echo_msg,
                                    "off", 1);
            RECV(":alice!user@host PRIVMSG nickserv :identify secret");
            CHECK_SRV("--", "Msg(alice) -> nickserv: identify ******",
                      "irc_privmsg,self_msg,notify_none,no_highlight,"
                      "nick_alice,host_user@host,log1");
            config_file_option_reset (irc_config_look_open_pv_buffer_echo_msg, 1);
        }

        /* broken CTCP to channel */
        RECV(":bob!user@host PRIVMSG #test :\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "Unknown CTCP requested by bob: ",
                   "irc_privmsg,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host PRIVMSG #test :\001TEST");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "Unknown CTCP requested by bob: TEST",
                   "irc_privmsg,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host PRIVMSG #test :\001ACTION");
        CHECK_SENT(NULL);
        CHECK_CHAN(" *", "bob",
                   "irc_privmsg,irc_action,notify_message,nick_bob,"
                   "host_user@host,log1");
        RECV(":bob!user@host PRIVMSG #test :\001ACTION \002is testing");
        CHECK_SENT(NULL);
        CHECK_CHAN(" *", "bob is testing",
                   "irc_privmsg,irc_action,notify_message,nick_bob,"
                   "host_user@host,log1");
        info = irc_ctcp_eval_reply (ptr_server,
                                    irc_ctcp_get_reply (ptr_server, "VERSION"));
        RECV(":bob!user@host PRIVMSG #test :\001VERSION");
        snprintf (message, sizeof (message),
                  "NOTICE bob :\001VERSION %s\001", info);
        CHECK_SENT(message);
        CHECK_CHAN("--", "CTCP requested by bob: VERSION",
                   "irc_privmsg,irc_ctcp,nick_bob,host_user@host,log1");
        snprintf (message, sizeof (message),
                  "CTCP reply to bob: VERSION %s", info);
        if (echo_message == 0)
        {
            /* reply is displayed only if echo-message is NOT enabled */
            CHECK_SRV("--", message,
                      "irc_privmsg,irc_ctcp,irc_ctcp_reply,self_msg,notify_none,"
                      "no_highlight,nick_alice,log1");
        }
        snprintf (message, sizeof (message),
                  "NOTICE bob :\001VERSION %s\001", info);
        CHECK_SENT(message);
        free (info);
        RECV(":bob!user@host PRIVMSG #test :\001DCC");
        CHECK_SENT(NULL);
        CHECK_NO_MSG;
        RECV(":bob!user@host PRIVMSG #test :\001DCC SEND");
        CHECK_SENT(NULL);
        CHECK_NO_MSG;
        RECV(":bob!user@host PRIVMSG #test :\001DCC SEND file.txt");
        CHECK_SENT(NULL);
        CHECK_SRV("=!=", "irc: cannot parse \"privmsg\" command", "");
        RECV(":bob!user@host PRIVMSG #test :\001DCC SEND file.txt 1 2 3");
        CHECK_SENT(NULL);
        CHECK_CORE("",
                   "xfer: incoming file from bob (0.0.0.1, irc." IRC_FAKE_SERVER
                   "), name: file.txt, 3 bytes (protocol: dcc)");

        /* broken CTCP to user */
        RECV(":bob!user@host PRIVMSG alice :\001");
        CHECK_SENT(NULL);
        CHECK_SRV("--", "Unknown CTCP requested by bob: ",
                  "irc_privmsg,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host PRIVMSG alice :\001TEST");
        CHECK_SENT(NULL);
        CHECK_SRV("--", "Unknown CTCP requested by bob: TEST",
                  "irc_privmsg,irc_ctcp,nick_bob,host_user@host,log1");
        RECV(":bob!user@host PRIVMSG alice :\001ACTION");
        CHECK_SENT(NULL);
        CHECK_PV_CLOSE("bob", " *", "bob",
                       "irc_privmsg,irc_action,notify_private,nick_bob,"
                       "host_user@host,log1");
        RECV(":bob!user@host PRIVMSG alice :\001ACTION \002is testing");
        CHECK_SENT(NULL);
        CHECK_PV_CLOSE("bob", " *", "bob is testing",
                       "irc_privmsg,irc_action,notify_private,nick_bob,"
                       "host_user@host,log1");
        info = irc_ctcp_eval_reply (ptr_server,
                                    irc_ctcp_get_reply (ptr_server, "VERSION"));
        RECV(":bob!user@host PRIVMSG alice :\001VERSION");
        snprintf (message, sizeof (message),
                  "NOTICE bob :\001VERSION %s\001", info);
        CHECK_SENT(message);
        snprintf (message, sizeof (message),
                  "CTCP reply to bob: VERSION %s", info);
        if (echo_message == 0)
        {
            /* reply is displayed only if echo-message is NOT enabled */
            CHECK_SRV("--", message,
                      "irc_privmsg,irc_ctcp,irc_ctcp_reply,self_msg,notify_none,"
                      "no_highlight,nick_alice,log1");
        }
        free (info);
        RECV(":bob!user@host PRIVMSG alice :\001DCC");
        CHECK_SENT(NULL);
        CHECK_NO_MSG;
        RECV(":bob!user@host PRIVMSG alice :\001DCC SEND");
        CHECK_SENT(NULL);
        CHECK_NO_MSG;
        RECV(":bob!user@host PRIVMSG alice :\001DCC SEND file.txt");
        CHECK_SENT(NULL);
        CHECK_SRV("=!=", "irc: cannot parse \"privmsg\" command", "");
        RECV(":bob!user@host PRIVMSG alice :\001DCC SEND file.txt 1 2 3");
        CHECK_SENT(NULL);
        CHECK_CORE("",
                   "xfer: incoming file from bob (0.0.0.1, irc." IRC_FAKE_SERVER
                   "), name: file.txt, 3 bytes (protocol: dcc)");

        /* valid CTCP to channel */
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG #test :\001TEST\001");
        CHECK_SENT(NULL);
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG #test :\001ACTION\001");
        CHECK_SENT(NULL);
        CHECK_CHAN(" *", "bob",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_action,notify_message,nick_bob,host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG #test :\001ACTION is testing with \002bold\002\001");
        CHECK_SENT(NULL);
        CHECK_CHAN(" *", "bob is testing with bold",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_action,notify_message,nick_bob,host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG #test :\001PING 1703496549 905284\001");
        CHECK_SENT("NOTICE bob :\001PING 1703496549 905284\001");
        CHECK_CHAN("--", "CTCP requested by bob: PING 1703496549 905284",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_ctcp,nick_bob,host_user@host,log1");
        if (echo_message == 0)
        {
            /* reply is displayed only if echo-message is NOT enabled */
            CHECK_SRV("--", "CTCP reply to bob: PING 1703496549 905284",
                      "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                      "irc_ctcp,irc_ctcp_reply,self_msg,notify_none,"
                      "no_highlight,nick_alice,log1");
        }
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG #test :\001UNKNOWN\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "Unknown CTCP requested by bob: UNKNOWN",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_ctcp,nick_bob,host_user@host,log1");

        /* valid CTCP to ops of channel */
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG @#test :\001ACTION\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "Action -> @#test: bob",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_action,notify_message,nick_bob,host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG @#test :\001ACTION \002is testing\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "Action -> @#test: bob is testing",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_action,notify_message,nick_bob,host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG @#test :\001PING 1703496549 905284\001");
        CHECK_SENT("NOTICE bob :\001PING 1703496549 905284\001");
        CHECK_CHAN("--", "CTCP requested by bob: PING 1703496549 905284",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_ctcp,nick_bob,host_user@host,log1");
        if (echo_message == 0)
        {
            /* reply is displayed only if echo-message is NOT enabled */
            CHECK_SRV("--", "CTCP reply to bob: PING 1703496549 905284",
                      "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                      "irc_ctcp,irc_ctcp_reply,self_msg,notify_none,"
                      "no_highlight,nick_alice,log1");
        }
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG @#test :\001UNKNOWN\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "Unknown CTCP requested by bob: UNKNOWN",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_ctcp,nick_bob,host_user@host,log1");

        /*
         * valid CTCP to channel from self nick
         * (case of bouncer or if echo-message capability is enabled)
         */
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":alice!user@host PRIVMSG #test :\001VERSION\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "CTCP query to #test: VERSION",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_ctcp,self_msg,notify_none,no_highlight,nick_alice,"
                   "host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":alice!user@host PRIVMSG #test :\001ACTION\001");
        CHECK_SENT(NULL);
        CHECK_CHAN(" *", "alice",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_action,self_msg,notify_none,no_highlight,nick_alice,"
                   "host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":alice!user@host PRIVMSG #test :\001ACTION is testing with \002bold\002\001");
        CHECK_SENT(NULL);
        CHECK_CHAN(" *", "alice is testing with bold",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_action,self_msg,notify_none,no_highlight,nick_alice,"
                   "host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":alice!user@host PRIVMSG #test :\001PING 1703496549 905284\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "CTCP query to #test: PING 1703496549 905284",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_ctcp,self_msg,notify_none,no_highlight,"
                   "nick_alice,host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":alice!user@host PRIVMSG #test :\001UNKNOWN\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "CTCP query to #test: UNKNOWN",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_ctcp,self_msg,notify_none,no_highlight,"
                   "nick_alice,host_user@host,log1");

        /*
         * valid CTCP to ops of channel from self nick
         * (case of bouncer or if echo-message capability is enabled)
         */
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":alice!user@host PRIVMSG @#test :\001ACTION\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "Action -> @#test: alice",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_action,self_msg,notify_none,no_highlight,nick_alice,"
                   "host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":alice!user@host PRIVMSG @#test :\001ACTION \002is testing\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "Action -> @#test: alice is testing",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_action,self_msg,notify_none,no_highlight,nick_alice,"
                   "host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":alice!user@host PRIVMSG @#test :\001PING 1703496549 905284\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "CTCP query to @#test: PING 1703496549 905284",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_ctcp,self_msg,notify_none,no_highlight,"
                   "nick_alice,host_user@host,log1");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":alice!user@host PRIVMSG @#test :\001UNKNOWN\001");
        CHECK_SENT(NULL);
        CHECK_CHAN("--", "CTCP query to @#test: UNKNOWN",
                   "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                   "irc_ctcp,self_msg,notify_none,no_highlight,"
                   "nick_alice,host_user@host,log1");

        /* valid CTCP to user */
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG alice :\001TEST\001");
        CHECK_SENT(NULL);
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG alice :\001ACTION\001");
        CHECK_SENT(NULL);
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG alice :\001ACTION \002is testing\001");
        CHECK_SENT(NULL);
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG alice :\001PING 1703496549 905284\001");
        CHECK_SENT("NOTICE bob :\001PING 1703496549 905284\001");
        CHECK_SRV("--", "CTCP requested by bob: PING 1703496549 905284",
                  "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                  "irc_ctcp,nick_bob,host_user@host,log1");
        if (echo_message == 0)
        {
            /* reply is displayed only if echo-message is NOT enabled */
            CHECK_SRV("--", "CTCP reply to bob: PING 1703496549 905284",
                      "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                      "irc_ctcp,irc_ctcp_reply,self_msg,notify_none,"
                      "no_highlight,nick_alice,log1");
        }
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG alice :\001UNKNOWN\001");
        CHECK_SENT(NULL);
        CHECK_SRV("--", "Unknown CTCP requested by bob: UNKNOWN",
                  "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                  "irc_ctcp,nick_bob,host_user@host,log1");
        info = irc_ctcp_eval_reply (ptr_server,
                                    irc_ctcp_get_reply (ptr_server, "VERSION"));
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG alice :\001VERSION\001");
        snprintf (message, sizeof (message),
                  "NOTICE bob :\001VERSION %s\001", info);
        CHECK_SENT(message);
        CHECK_SRV("--", "CTCP requested by bob: VERSION",
                  "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                  "irc_ctcp,nick_bob,host_user@host,log1");
        snprintf (message, sizeof (message),
                  "CTCP reply to bob: VERSION %s", info);
        if (echo_message == 0)
        {
            /* reply is displayed only if echo-message is NOT enabled */
            CHECK_SRV("--", message,
                      "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                      "irc_ctcp,irc_ctcp_reply,self_msg,notify_none,no_highlight,"
                      "nick_alice,log1");
        }
        snprintf (message, sizeof (message),
                  "NOTICE bob :\001VERSION %s\001", info);
        CHECK_SENT(message);
        free (info);
        info = hook_info_get (NULL, "weechat_site_download", "");
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG alice :\001SOURCE\001");
        snprintf (message, sizeof (message),
                  "NOTICE bob :\001SOURCE %s\001", info);
        CHECK_SENT(message);
        free (info);
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":bob!user@host PRIVMSG alice :\001DCC SEND file.txt 1 2 3\001");
        CHECK_SENT(NULL);
        CHECK_CORE("",
                   "xfer: incoming file from bob (0.0.0.1, irc." IRC_FAKE_SERVER
                   "), name: file.txt, 3 bytes (protocol: dcc)");
        CHECK_SENT(NULL);

        /*
         * valid CTCP to user from self nick
         * (case of bouncer or if echo-message capability is enabled)
         */
        RECV("@time=2023-12-25T10:29:09.456789Z "
             ":alice!user@host PRIVMSG alice :\001CLIENTINFO\001");
        if (echo_message == 0)
        {
            CHECK_SENT("NOTICE alice :\001CLIENTINFO ACTION CLIENTINFO DCC "
                       "PING SOURCE TIME VERSION\001");
            CHECK_SRV("--", "CTCP requested by alice: CLIENTINFO",
                      "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                      "irc_ctcp,nick_alice,host_user@host,log1");
            CHECK_SRV("--", "CTCP reply to alice: CLIENTINFO ACTION CLIENTINFO "
                      "DCC PING SOURCE TIME VERSION",
                      "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                      "irc_ctcp,irc_ctcp_reply,self_msg,notify_none,"
                      "no_highlight,nick_alice,host_user@host,log1");
        }
        else
        {
            CHECK_SENT(NULL);
            CHECK_SRV("--", "CTCP query to alice: CLIENTINFO",
                      "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                      "irc_ctcp,self_msg,notify_none,no_highlight,nick_alice,"
                      "host_user@host,log1");
            RECV("@time=2023-12-25T10:29:09.456789Z "
                 ":alice!user@host PRIVMSG alice :\001CLIENTINFO\001");
            CHECK_SENT("NOTICE alice :\001CLIENTINFO ACTION CLIENTINFO DCC "
                       "PING SOURCE TIME VERSION\001");
            CHECK_SRV("--", "CTCP requested by alice: CLIENTINFO",
                      "irc_privmsg,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                      "irc_ctcp,nick_alice,host_user@host,log1");
            RECV("@time=2023-12-25T10:29:09.456789Z "
                 ":alice!user@host NOTICE alice :\001CLIENTINFO DCC PING "
                 "SOURCE TIME VERSION\001");
            CHECK_SENT(NULL);
            CHECK_SRV("--", "CTCP reply to alice: CLIENTINFO DCC PING "
                      "SOURCE TIME VERSION",
                      "irc_notice,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                      "irc_ctcp,irc_ctcp_reply,self_msg,notify_none,"
                      "no_highlight,nick_alice,host_user@host,log1");
            RECV("@time=2023-12-25T10:29:09.456789Z "
                 ":alice!user@host NOTICE alice :\001CLIENTINFO DCC PING "
                 "SOURCE TIME VERSION\001");
            CHECK_SENT(NULL);
            CHECK_SRV("--", "CTCP reply from alice: CLIENTINFO DCC PING "
                      "SOURCE TIME VERSION",
                      "irc_notice,irc_tag_time=2023-12-25T10:29:09.456789Z,"
                      "irc_ctcp,nick_alice,host_user@host,log1");
        }

        /* close xfer buffer */
        if (xfer_buffer)
            gui_buffer_close (xfer_buffer);

        if (echo_message == 1)
            hashtable_remove (ptr_server->cap_list, "echo-message");
    }
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
    CHECK_PV("bob", "bob", "hi Alice!",
             "irc_privmsg,notify_private,prefix_nick_248,nick_bob,"
             "host_user@host,log1");

    ptr_channel = ptr_server->channels;

    /* without quit message */
    RECV(":bob!user@host JOIN #test");
    RECV(":bob!user@host QUIT");
    CHECK_CHAN("<--", "bob (user@host) has quit",
               "irc_quit,irc_smart_filter,nick_bob,host_user@host,log4");
    LONGS_EQUAL(1, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    /* without quit message (but empty trailing parameter) */
    RECV(":bob!user@host JOIN #test");
    RECV(":bob!user@host QUIT :");
    CHECK_CHAN("<--", "bob (user@host) has quit",
               "irc_quit,irc_smart_filter,nick_bob,host_user@host,log4");
    LONGS_EQUAL(1, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    /* with quit message */
    RECV(":bob!user@host JOIN #test");
    RECV(":bob!user@host QUIT :\002quit message ");
    CHECK_CHAN("<--", "bob (user@host) has quit (quit message )",
               "irc_quit,irc_smart_filter,nick_bob,host_user@host,log4");
    LONGS_EQUAL(1, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);

    /* with quit message */
    RECV(":bob!user_\00304red@host_\00304red JOIN #test");
    RECV(":bob!user_\00304red@host_\00304red QUIT :\002quit message ");
    CHECK_CHAN("<--", "bob (user_red@host_red) has quit (quit message )",
               "irc_quit,irc_smart_filter,nick_bob,host_user_\00304red@host_\00304red,log4");
    LONGS_EQUAL(1, ptr_channel->nicks_count);
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    POINTERS_EQUAL(NULL, ptr_channel->nicks->next_nick);
    RECV(":bob!user_\00304red@host_\00304red JOIN #test");
    CHECK_PV("bob", "-->", "bob (user_red@host_red) is back on server",
             "irc_nick_back,nick_bob,host_user_\00304red@host_\00304red,log4");
    RECV(":bob!user_\00304red@host_\00304red QUIT :\002quit message ");

    /* quit with option irc.look.display_host_quit set to off */
    config_file_option_set (irc_config_look_display_host_quit, "off", 1);
    RECV(":bob!user@host JOIN #test");
    RECV(":bob!user@host QUIT :\002quit message ");
    CHECK_CHAN("<--", "bob has quit (quit message )",
               "irc_quit,irc_smart_filter,nick_bob,host_user@host,log4");
    config_file_option_reset (irc_config_look_display_host_quit, 1);
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

    STRCMP_EQUAL(NULL, ptr_nick->realname);

    /* not enough parameters */
    RECV(":alice!user@host SETNAME");
    CHECK_ERROR_PARAMS("setname", 0, 1);

    /* missing nick */
    RECV("SETNAME :new bob realname");
    CHECK_ERROR_NICK("setname");

    STRCMP_EQUAL(NULL, ptr_nick->realname);

    /* real name of "bob" has changed */
    RECV(":bob!user@host SETNAME :\002new bob realname ");
    CHECK_CHAN("--", "bob has changed real name to \"new bob realname \"",
               "irc_setname,irc_smart_filter,nick_bob,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_nick->realname);

    /* self real name has changed */
    RECV(":alice!user@host SETNAME :\002new alice realname ");
    CHECK_SRV("--", "Your real name has been set to \"new alice realname \"",
              "irc_setname,nick_alice,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_nick->realname);
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

    STRCMP_EQUAL(NULL, ptr_nick->realname);

    /* real name of "bob" has changed */
    RECV(":bob!user@host SETNAME :\002new bob realname ");
    CHECK_CHAN("--", "bob has changed real name to \"new bob realname \"",
               "irc_setname,irc_smart_filter,nick_bob,host_user@host,log3");
    STRCMP_EQUAL("\002new bob realname ", ptr_nick2->realname);

    /* self real name has changed */
    RECV(":alice!user@host SETNAME :new realname");
    CHECK_SRV("--", "Your real name has been set to \"new realname\"",
              "irc_setname,nick_alice,host_user@host,log3");
    STRCMP_EQUAL("new realname", ptr_nick->realname);

    /* self real name has changed */
    RECV(":alice!user@host SETNAME :new realname2");
    CHECK_SRV("--", "Your real name has been set to \"new realname2\"",
              "irc_setname,nick_alice,host_user@host,log3");
    STRCMP_EQUAL("new realname2", ptr_nick->realname);

    hashtable_remove (ptr_server->cap_list, "setname");
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

    RECV("@+typing=active :bob!user@host TAGMSG #test ");
    ptr_typing_status = typing_status_nick_search (ptr_buffer, "bob");
    CHECK(ptr_typing_status);
    LONGS_EQUAL(TYPING_STATUS_STATE_TYPING, ptr_typing_status->state);

    RECV("@+typing=paused :bob!user@host TAGMSG : #test ");
    ptr_typing_status = typing_status_nick_search (ptr_buffer, "bob");
    CHECK(ptr_typing_status);
    LONGS_EQUAL(TYPING_STATUS_STATE_PAUSED, ptr_typing_status->state);

    RECV("@+typing=done :bob!user@host TAGMSG #test ");
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
    STRCMP_EQUAL(NULL, ptr_channel->topic);

    /* not enough parameters */
    RECV(":alice!user@host TOPIC");
    CHECK_ERROR_PARAMS("topic", 0, 1);

    /* missing nick */
    RECV("TOPIC #test :new topic");
    CHECK_ERROR_NICK("topic");

    STRCMP_EQUAL(NULL, ptr_channel->topic);

    /* not a channel */
    RECV(":alice!user@host TOPIC bob");
    CHECK_SRV("=!=", "irc: \"topic\" command received without channel", "");

    /* empty topic */
    RECV(":alice!user@host TOPIC #test");
    CHECK_CHAN("--", "alice has unset topic for #test",
               "irc_topic,nick_alice,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_channel->topic);

    /* empty topic (with empty trailing parameter) */
    RECV(":alice!user@host TOPIC #test :");
    CHECK_CHAN("--", "alice has unset topic for #test",
               "irc_topic,nick_alice,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_channel->topic);

    /* new topic */
    RECV(":alice!user@host TOPIC #test :\002new topic ");
    CHECK_CHAN("--", "alice has changed topic for #test to \"new topic \"",
               "irc_topic,nick_alice,host_user@host,log3");
    STRCMP_EQUAL("\002new topic ", ptr_channel->topic);

    /* another new topic */
    RECV(":alice!user_\00304red@host_\00304red TOPIC #test :\00304another new topic ");
    CHECK_CHAN("--",
               "alice has changed topic for #test from "
               "\"new topic \" to \"another new topic \"",
               "irc_topic,nick_alice,host_user_\00304red@host_\00304red,log3");
    STRCMP_EQUAL("\00304another new topic ", ptr_channel->topic);

    /* empty topic */
    RECV(":alice!user@host TOPIC #test");
    CHECK_CHAN("--",
               "alice has unset topic for #test (old topic: "
               "\"another new topic \")",
               "irc_topic,nick_alice,host_user@host,log3");
    STRCMP_EQUAL(NULL, ptr_channel->topic);
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
    CHECK_SRV("--", "Wallops from alice (user@host): message",
              "irc_wallops,notify_private,nick_alice,host_user@host,log3");

    RECV(":alice!user@host WALLOPS :\002message from admin ");
    CHECK_SRV("--", "Wallops from alice (user@host): message from admin ",
              "irc_wallops,notify_private,nick_alice,host_user@host,log3");

    /* wallops with option irc.look.display_host_wallops set to off */
    config_file_option_set (irc_config_look_display_host_wallops, "off", 1);
    RECV(":alice!user@host WALLOPS :message from admin ");
    CHECK_SRV("--", "Wallops from alice: message from admin ",
              "irc_wallops,notify_private,nick_alice,host_user@host,log3");
    config_file_option_reset (irc_config_look_display_host_wallops, 1);
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
    CHECK_SRV("=!=", "Warning: [] TEST", "irc_warn,nick_server,log3");
    RECV(":server WARN * TEST : \002the message ");
    CHECK_SRV("=!=", "Warning: [TEST]  the message ", "irc_warn,nick_server,log3");
    RECV(":server WARN * TEST TEST2");
    CHECK_SRV("=!=", "Warning: [TEST] TEST2", "irc_warn,nick_server,log3");
    RECV(":server WARN * TEST TEST2 :the message");
    CHECK_SRV("=!=", "Warning: [TEST TEST2] the message", "irc_warn,nick_server,log3");

    RECV(":server WARN COMMAND TEST");
    CHECK_SRV("=!=", "Warning: COMMAND [] TEST", "irc_warn,nick_server,log3");
    RECV(":server WARN COMMAND TEST :the message");
    CHECK_SRV("=!=", "Warning: COMMAND [TEST] the message", "irc_warn,nick_server,log3");
    RECV(":server WARN COMMAND TEST TEST2");
    CHECK_SRV("=!=", "Warning: COMMAND [TEST] TEST2", "irc_warn,nick_server,log3");
    RECV(":server WARN COMMAND TEST TEST2 :the message");
    CHECK_SRV("=!=", "Warning: COMMAND [TEST TEST2] the message",
              "irc_warn,nick_server,log3");
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
    CHECK_SRV("--", "", "irc_001,irc_numeric,nick_server,log3");
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

    RECV(":server 001 alice : Welcome on this server, alice! ");
    CHECK_SRV("--", " Welcome on this server, alice! ",
              "irc_001,irc_numeric,nick_server,log3");

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

    STRCMP_EQUAL(NULL, ptr_server->prefix_modes);
    STRCMP_EQUAL(NULL, ptr_server->prefix_chars);

    RECV(":server 005 alice TEST=A");
    CHECK_SRV("--", "TEST=A", "irc_005,irc_numeric,nick_server,log3");

    STRCMP_EQUAL(NULL, ptr_server->prefix_modes);
    STRCMP_EQUAL(NULL, ptr_server->prefix_chars);
}

/*
 * Tests functions:
 *   irc_protocol_cb_005 (infos from server, full)
 */

TEST(IrcProtocolWithServer, 005_full)
{
    SRV_INIT;

    STRCMP_EQUAL(NULL, ptr_server->prefix_modes);
    STRCMP_EQUAL(NULL, ptr_server->prefix_chars);
    LONGS_EQUAL(0, ptr_server->msg_max_length);
    LONGS_EQUAL(0, ptr_server->nick_max_length);
    LONGS_EQUAL(0, ptr_server->user_max_length);
    LONGS_EQUAL(0, ptr_server->host_max_length);
    LONGS_EQUAL(0, ptr_server->casemapping);
    STRCMP_EQUAL(NULL, ptr_server->chantypes);
    STRCMP_EQUAL(NULL, ptr_server->chanmodes);
    LONGS_EQUAL(0, ptr_server->monitor);
    LONGS_EQUAL(IRC_SERVER_UTF8MAPPING_NONE, ptr_server->utf8mapping);
    LONGS_EQUAL(0, ptr_server->utf8only);
    STRCMP_EQUAL(NULL, ptr_server->isupport);

    RECV(":server 005 alice " IRC_MSG_005 " : are supported ");
    CHECK_SRV("--", IRC_MSG_005 "  are supported ",
              "irc_005,irc_numeric,nick_server,log3");

    STRCMP_EQUAL("ohv", ptr_server->prefix_modes);
    STRCMP_EQUAL("@%+", ptr_server->prefix_chars);
    LONGS_EQUAL(4096, ptr_server->msg_max_length);
    LONGS_EQUAL(30, ptr_server->nick_max_length);
    LONGS_EQUAL(16, ptr_server->user_max_length);
    LONGS_EQUAL(32, ptr_server->host_max_length);
    LONGS_EQUAL(1, ptr_server->casemapping);
    STRCMP_EQUAL("#", ptr_server->chantypes);
    STRCMP_EQUAL("eIbq,k,flj,CFLMPQScgimnprstuz", ptr_server->chanmodes);
    LONGS_EQUAL(100, ptr_server->monitor);
    LONGS_EQUAL(IRC_SERVER_UTF8MAPPING_RFC8265, ptr_server->utf8mapping);
    LONGS_EQUAL(1, ptr_server->utf8only);
    STRCMP_EQUAL(IRC_MSG_005, ptr_server->isupport);

    /* check that realloc of info is OK if we receive the message again */
    RECV(":server 005 alice " IRC_MSG_005 " :are supported");
    CHECK_SRV("--", IRC_MSG_005 " are supported",
              "irc_005,irc_numeric,nick_server,log3");

    STRCMP_EQUAL("ohv", ptr_server->prefix_modes);
    STRCMP_EQUAL("@%+", ptr_server->prefix_chars);
    LONGS_EQUAL(4096, ptr_server->msg_max_length);
    LONGS_EQUAL(30, ptr_server->nick_max_length);
    LONGS_EQUAL(16, ptr_server->user_max_length);
    LONGS_EQUAL(32, ptr_server->host_max_length);
    LONGS_EQUAL(1, ptr_server->casemapping);
    STRCMP_EQUAL("#", ptr_server->chantypes);
    STRCMP_EQUAL("eIbq,k,flj,CFLMPQScgimnprstuz", ptr_server->chanmodes);
    LONGS_EQUAL(100, ptr_server->monitor);
    LONGS_EQUAL(IRC_SERVER_UTF8MAPPING_RFC8265, ptr_server->utf8mapping);
    LONGS_EQUAL(1, ptr_server->utf8only);
    STRCMP_EQUAL(IRC_MSG_005 " " IRC_MSG_005, ptr_server->isupport);
}

/*
 * Tests functions:
 *   irc_protocol_cb_005 (infos from server, multiple messages)
 */

TEST(IrcProtocolWithServer, 005_multiple_messages)
{
    SRV_INIT;

    STRCMP_EQUAL(NULL, ptr_server->prefix_modes);
    STRCMP_EQUAL(NULL, ptr_server->prefix_chars);
    LONGS_EQUAL(0, ptr_server->host_max_length);
    STRCMP_EQUAL(NULL, ptr_server->isupport);

    RECV(":server 005 alice PREFIX=(ohv)@%+ :are supported");
    CHECK_SRV("--", "PREFIX=(ohv)@%+ are supported",
              "irc_005,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("ohv", ptr_server->prefix_modes);
    STRCMP_EQUAL("@%+", ptr_server->prefix_chars);
    STRCMP_EQUAL("PREFIX=(ohv)@%+", ptr_server->isupport);

    RECV(":server 005 alice HOSTLEN=24 :are supported");
    CHECK_SRV("--", "HOSTLEN=24 are supported",
              "irc_005,irc_numeric,nick_server,log3");
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

    RECV(":server 008 alice +Zbfkrsuy : \002Server notice mask ");
    CHECK_SRV("--",
              "Server notice mask for alice: +Zbfkrsuy  Server notice mask ",
              "irc_008,irc_numeric,nick_server,log3");
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

    STRCMP_EQUAL(NULL, ptr_server->nick_modes);

    RECV(":server 221 alice : +abc ");
    CHECK_SRV("--", "User mode for alice is [ +abc ]",
              "irc_221,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("abc", ptr_server->nick_modes);

    RECV(":server 221 alice :-abc");
    CHECK_SRV("--", "User mode for alice is [-abc]",
              "irc_221,irc_numeric,nick_server,log3");
    STRCMP_EQUAL(NULL, ptr_server->nick_modes);
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
 *   335: whois (is a bot on)
 *   337: whois ((is hiding idle time)
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
    RECV(":server 337");
    CHECK_ERROR_PARAMS("337", 0, 2);
    RECV(":server 337 alice");
    CHECK_ERROR_PARAMS("337", 1, 2);
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
    CHECK_SRV("--", "[bob] UTF-8", "irc_223,irc_numeric,nick_server,log3");
    RECV(":server 223 alice bob :UTF-8");
    CHECK_SRV("--", "[bob] UTF-8", "irc_223,irc_numeric,nick_server,log3");
    RECV(":server 223 alice bob : UTF-8 ");
    CHECK_SRV("--", "[bob]  UTF-8 ", "irc_223,irc_numeric,nick_server,log3");
    RECV(":server 223 alice bob");
    CHECK_SRV("--", "bob", "irc_223,irc_numeric,nick_server,log3");
    RECV(":server 264 alice bob : \002is using encrypted connection ");
    CHECK_SRV("--", "[bob]  is using encrypted connection ",
              "irc_264,irc_numeric,nick_server,log3");
    RECV(":server 264 alice bob");
    CHECK_SRV("--", "bob", "irc_264,irc_numeric,nick_server,log3");
    RECV(":server 275 alice bob : is using secure connection ");
    CHECK_SRV("--", "[bob]  is using secure connection ",
              "irc_275,irc_numeric,nick_server,log3");
    RECV(":server 275 alice bob");
    CHECK_SRV("--", "bob", "irc_275,irc_numeric,nick_server,log3");
    RECV(":server 276 alice bob : has client certificate fingerprint ");
    CHECK_SRV("--", "[bob]  has client certificate fingerprint ",
              "irc_276,irc_numeric,nick_server,log3");
    RECV(":server 276 alice bob");
    CHECK_SRV("--", "bob", "irc_276,irc_numeric,nick_server,log3");
    RECV(":server 307 alice bob : registered nick ");
    CHECK_SRV("--", "[bob]  registered nick ", "irc_307,irc_numeric,nick_server,log3");
    RECV(":server 307 alice bob");
    CHECK_SRV("--", "bob", "irc_307,irc_numeric,nick_server,log3");
    RECV(":server 310 alice bob : help mode ");
    CHECK_SRV("--", "[bob]  help mode ", "irc_310,irc_numeric,nick_server,log3");
    RECV(":server 310 alice bob");
    CHECK_SRV("--", "bob", "irc_310,irc_numeric,nick_server,log3");
    RECV(":server 313 alice bob : operator ");
    CHECK_SRV("--", "[bob]  operator ", "irc_313,irc_numeric,nick_server,log3");
    RECV(":server 313 alice bob");
    CHECK_SRV("--", "bob", "irc_313,irc_numeric,nick_server,log3");
    RECV(":server 318 alice bob : end ");
    CHECK_SRV("--", "[bob]  end ", "irc_318,irc_numeric,nick_server,log3");
    RECV(":server 318 alice bob");
    CHECK_SRV("--", "bob", "irc_318,irc_numeric,nick_server,log3");
    RECV(":server 319 alice bob : channels ");
    CHECK_SRV("--", "[bob]  channels ", "irc_319,irc_numeric,nick_server,log3");
    RECV(":server 319 alice bob");
    CHECK_SRV("--", "bob", "irc_319,irc_numeric,nick_server,log3");
    RECV(":server 320 alice bob : identified user ");
    CHECK_SRV("--", "[bob]  identified user ", "irc_320,irc_numeric,nick_server,log3");
    RECV(":server 320 alice bob");
    CHECK_SRV("--", "bob", "irc_320,irc_numeric,nick_server,log3");
    RECV(":server 326 alice bob : has oper privs ");
    CHECK_SRV("--", "[bob]  has oper privs ", "irc_326,irc_numeric,nick_server,log3");
    RECV(":server 326 alice bob");
    CHECK_SRV("--", "bob", "irc_326,irc_numeric,nick_server,log3");
    RECV(":server 335 alice bob : is a bot ");
    CHECK_SRV("--", "[bob]  is a bot ", "irc_335,irc_numeric,nick_server,log3");
    RECV(":server 335 alice bob");
    CHECK_SRV("--", "bob", "irc_335,irc_numeric,nick_server,log3");
    RECV(":server 337 alice bob : is hiding their idle time ");
    CHECK_SRV("--", "[bob]  is hiding their idle time ", "irc_337,irc_numeric,nick_server,log3");
    RECV(":server 337 alice bob");
    CHECK_SRV("--", "bob", "irc_337,irc_numeric,nick_server,log3");
    RECV(":server 378 alice bob");
    CHECK_SRV("--", "bob", "irc_378,irc_numeric,nick_server,log3");
    RECV(":server 378 alice bob : connecting from ");
    CHECK_SRV("--", "[bob]  connecting from ", "irc_378,irc_numeric,nick_server,log3");
    RECV(":server 378 alice bob");
    CHECK_SRV("--", "bob", "irc_378,irc_numeric,nick_server,log3");
    RECV(":server 379 alice bob : using modes ");
    CHECK_SRV("--", "[bob]  using modes ", "irc_379,irc_numeric,nick_server,log3");
    RECV(":server 379 alice bob");
    CHECK_SRV("--", "bob", "irc_379,irc_numeric,nick_server,log3");
    RECV(":server 671 alice bob : secure connection ");
    CHECK_SRV("--", "[bob]  secure connection ", "irc_671,irc_numeric,nick_server,log3");
    RECV(":server 671 alice bob");
    CHECK_SRV("--", "bob", "irc_671,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "[bob] end", "irc_369,irc_numeric,nick_server,log3");
    RECV(":server 369 alice bob : \002end ");
    CHECK_SRV("--", "[bob]  end ", "irc_369,irc_numeric,nick_server,log3");
    RECV(":server 369 alice bob");
    CHECK_SRV("--", "bob", "irc_369,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_301 (away message)
 */

TEST(IrcProtocolWithServer, 301)
{
    SRV_INIT;

    RECV(":bob!user@host PRIVMSG alice :hi Alice!");
    CHECK_PV("bob", "bob", "hi Alice!",
             "irc_privmsg,notify_private,prefix_nick_248,nick_bob,"
             "host_user@host,log1");

    /* not enough parameters */
    RECV(":server 301");
    CHECK_ERROR_PARAMS("301", 0, 1);

    STRCMP_EQUAL(NULL, ptr_server->channels->away_message);

    RECV(":server 301 alice bob");
    CHECK_NO_MSG;
    STRCMP_EQUAL(NULL, ptr_server->channels->away_message);

    RECV(":server 301 alice bob : \002I am away ");
    CHECK_PV("bob", "--", "[bob] is away:  I am away ",
             "irc_301,irc_numeric,nick_server,log3");
    STRCMP_EQUAL(" \002I am away ", ptr_server->channels->away_message);
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

    RECV(":server 303 alice : nick1 nick2 ");
    CHECK_SRV("--", "Users online:  nick1 nick2 ",
              "irc_303,irc_numeric,nick_server,log3");
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
    CHECK_PV("bob", "bob", "hi Alice!",
             "irc_privmsg,notify_private,prefix_nick_248,nick_bob,"
             "host_user@host,log1");

    /* not enough parameters */
    RECV(":server 305");
    CHECK_ERROR_PARAMS("305", 0, 1);
    RECV(":server 306");
    CHECK_ERROR_PARAMS("306", 0, 1);

    STRCMP_EQUAL(NULL, ptr_server->channels->away_message);

    RECV(":server 306 alice");  /* now away */
    CHECK_NO_MSG;
    LONGS_EQUAL(1, ptr_server->is_away);

    RECV(":server 305 alice");
    CHECK_NO_MSG;
    LONGS_EQUAL(0, ptr_server->is_away);

    RECV(":server 306 alice : \002We'll miss you ");  /* now away */
    CHECK_SRV("--", " We'll miss you ", "irc_306,irc_numeric,nick_server,log3");
    LONGS_EQUAL(1, ptr_server->is_away);

    RECV(":server 305 alice : \002Does this mean you're really back? ");
    CHECK_SRV("--", " Does this mean you're really back? ",
              "irc_305,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "[bob] user", "irc_311,irc_numeric,nick_server,log3");

    /* standard parameters */
    RECV(":server 311 alice bob user_\00304red host_\00302blue * : \002real name ");
    CHECK_SRV("--", "[bob] (user_red@host_blue):  real name ",
              "irc_311,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "[bob] server", "irc_312,irc_numeric,nick_server,log3");

    /* standard parameters */
    RECV(":server 312 alice bob server : \002https://example.com/ ");
    CHECK_SRV("--", "[bob] server ( https://example.com/ )",
              "irc_312,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "[bob] user", "irc_314,irc_numeric,nick_server,log3");

    /* standard parameters */
    RECV(":server 314 alice bob user host * : \002real name ");
    CHECK_SRV("--", "[bob] (user@host) was  real name ",
              "irc_314,irc_numeric,nick_server,log3");
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

    RECV(":server 315 alice #test end");
    CHECK_SRV("--", "[#test] end",
              "irc_315,irc_numeric,nick_server,log3");

    RECV(":server 315 alice #test : \002End of /WHO list. ");
    CHECK_SRV("--", "[#test]  End of /WHO list. ",
              "irc_315,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--",
              "[bob] idle: 1 day, 10 hours 07 minutes 57 seconds, "
              "signon at: Wed, 12 Mar 2008 13:18:00",
              "irc_317,irc_numeric,nick_server,log3");
    RECV(":server 317 alice bob 122877 1205327880 :\002seconds idle, signon time ");
    CHECK_SRV("--",
              "[bob] idle: 1 day, 10 hours 07 minutes 57 seconds, "
              "signon at: Wed, 12 Mar 2008 13:18:00",
              "irc_317,irc_numeric,nick_server,log3");

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
    CHECK_SRV("--", "#test", "irc_321,irc_numeric,nick_server,log3");
    RECV(":server 321 alice #test Users");
    CHECK_SRV("--", "#test Users", "irc_321,irc_numeric,nick_server,log3");
    RECV(":server 321 alice #test : \002Users  Name ");
    CHECK_SRV("--", "#test  Users  Name ", "irc_321,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "#test(3)", "irc_322,irc_numeric,nick_server,log3");
    RECV(":server 322 alice #test 3 :\002topic of channel ");
    CHECK_SRV("--", "#test(3): topic of channel ",
              "irc_322,irc_numeric,nick_server,log3");

    run_cmd_quiet ("/list -server " IRC_FAKE_SERVER " -raw #test.*");
    CHECK_SRV("--", "#test(3): topic of channel ",
              "irc_322,irc_numeric,nick_server,log3");

    RECV(":server 322 alice #test 3");
    CHECK_SRV("--", "#test(3)",
              "irc_322,irc_numeric,nick_server,log3");
    RECV(":server 322 alice #test 3 :topic of channel ");
    CHECK_SRV("--", "#test(3): topic of channel ",
              "irc_322,irc_numeric,nick_server,log3");

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
    CHECK_SRV("--", "", "irc_323,irc_numeric,nick_server,log3");
    RECV(":server 323 alice end");
    CHECK_SRV("--", "end", "irc_323,irc_numeric,nick_server,log3");
    RECV(":server 323 alice : \002End of /LIST ");
    CHECK_SRV("--", " End of /LIST ", "irc_323,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_324 (channel mode)
 */

TEST(IrcProtocolWithServer, 324)
{
    SRV_INIT_JOIN;

    STRCMP_EQUAL(NULL, ptr_server->channels->modes);

    /* not enough parameters */
    RECV(":server 324");
    CHECK_ERROR_PARAMS("324", 0, 2);
    RECV(":server 324 alice");
    CHECK_ERROR_PARAMS("324", 1, 2);

    RECV(":server 324 alice #test +nt");
    CHECK_NO_MSG;
    STRCMP_EQUAL("+nt", ptr_server->channels->modes);

    RECV(":server 324 alice #test +nst ");
    CHECK_CHAN("--", "Mode #test [+nst]", "irc_324,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("+nst", ptr_server->channels->modes);

    RECV(":server 324 alice #test");
    CHECK_CHAN("--", "Mode #test []", "irc_324,irc_numeric,nick_server,log3");
    STRCMP_EQUAL(NULL, ptr_server->channels->modes);
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
    CHECK_SRV("--", "[bob] host", "irc_327,irc_numeric,nick_server,log3");

    /* standard parameters */
    RECV(":server 327 alice bob host 1.2.3.4");
    CHECK_SRV("--", "[bob] host 1.2.3.4", "irc_327,irc_numeric,nick_server,log3");
    RECV(":server 327 alice bob host_\00304red 1.2.3.4 : \002real name ");
    CHECK_SRV("--", "[bob] host_red 1.2.3.4 ( real name )",
              "irc_327,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "URL for #test: https://example.com/",
               "irc_328,irc_numeric,nick_server,log3");
    RECV(":server 328 alice #test : \002URL is https://example.com/ ");
    CHECK_CHAN("--", "URL for #test:  URL is https://example.com/ ",
               "irc_328,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "Channel created on Wed, 12 Mar 2008 13:18:14",
               "irc_329,irc_numeric,nick_server,log3");
    RECV(":server 329 alice #test :1205327894");
    CHECK_CHAN("--", "Channel created on Wed, 12 Mar 2008 13:18:14",
               "irc_329,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 329 alice #xyz 1205327894");
    CHECK_SRV("--", "Channel #xyz created on Wed, 12 Mar 2008 13:18:14",
              "irc_329,irc_numeric,nick_server,log3");
    RECV(":server 329 alice #xyz :1205327894 ");
    CHECK_SRV("--", "Channel #xyz created on Wed, 12 Mar 2008 13:18:14",
              "irc_329,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "[bob] bob2", "irc_330,irc_numeric,nick_server,log3");
    RECV(":server 330 alice bob bob2 : \002is logged in as ");
    CHECK_SRV("--", "[bob]  is logged in as  bob2",
              "irc_330,irc_numeric,nick_server,log3");

    RECV(":server 343 alice bob bob2");
    CHECK_SRV("--", "[bob] bob2", "irc_343,irc_numeric,nick_server,log3");
    RECV(":server 343 alice bob bob2 :\002is opered as ");
    CHECK_SRV("--", "[bob] is opered as  bob2",
              "irc_343,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "No topic set for channel #test",
               "irc_331,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 331 alice #xyz ");
    CHECK_SRV("--", "No topic set for channel #xyz",
              "irc_331,irc_numeric,nick_server,log3");
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

    STRCMP_EQUAL(NULL, ptr_server->channels->topic);

    RECV(":server 332 alice #test");
    CHECK_CHAN("--", "Topic for #test is \"\"",
               "irc_332,irc_numeric,nick_server,log3");

    RECV(":server 332 alice #test :\002the new topic ");
    CHECK_CHAN("--", "Topic for #test is \"the new topic \"",
               "irc_332,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("\002the new topic ",
                 ptr_server->channels->topic);
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

    RECV(":server 333 alice #test nick!user_\00304red@host_\00304red");
    CHECK_NO_MSG;
    RECV(":server 333 alice #test nick!user_\00304red@host_\00304red 1205428096");
    CHECK_CHAN("--",
               "Topic set by nick (user_red@host_red) on Thu, 13 Mar 2008 17:08:16",
               "irc_333,irc_numeric,nick_server,log3");
    RECV(":server 333 alice #test 1205428096 ");
    CHECK_CHAN("--", "Topic set on Thu, 13 Mar 2008 17:08:16",
               "irc_333,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 333 alice #xyz nick!user_\00304red@host_\00304red");
    CHECK_NO_MSG;
    RECV(":server 333 alice #xyz nick!user_\00304red@host_\00304red 1205428096");
    CHECK_SRV("--",
              "Topic for #xyz set by nick (user_red@host_red) on "
              "Thu, 13 Mar 2008 17:08:16",
              "irc_333,irc_numeric,nick_server,log3");
    RECV(":server 333 alice #xyz 1205428096");
    CHECK_SRV("--", "Topic for #xyz set on Thu, 13 Mar 2008 17:08:16",
              "irc_333,irc_numeric,nick_server,log3");
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

    RECV(":server 338 alice bob host_\00304red : \002actually using host ");
    CHECK_SRV("--", "[bob]  actually using host  host_red",
              "irc_338,irc_numeric,nick_server,log3");

    /* on Rizon server */
    RECV(":server 338 alice bob :\002is actually bob_\00304red@example_\00304red.com [1.2.3.4]");
    CHECK_SRV("--", "[bob] is actually bob_red@example_red.com [1.2.3.4]",
              "irc_338,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "alice has invited bob to #test",
              "irc_341,irc_numeric,nick_alice,log3");
    RECV(":server 341 alice bob : #test ");
    CHECK_SRV("--", "alice has invited bob to  #test ",
              "irc_341,irc_numeric,nick_alice,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_344 (channel reop / whois (geo info))
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
    CHECK_SRV("--", "Channel reop #test: nick!user@host",
              "irc_344,irc_numeric,nick_server,log3");
    RECV(":server 344 alice #test : nick!user@host ");
    CHECK_SRV("--", "Channel reop #test:  nick!user@host ",
              "irc_344,irc_numeric,nick_server,log3");

    /* channel reop (IRCnet), channel not found */
    RECV(":server 344 alice #xyz nick!user@host");
    CHECK_SRV("--", "Channel reop #xyz: nick!user@host",
              "irc_344,irc_numeric,nick_server,log3");
    RECV(":server 344 alice #xyz : nick!user@host ");
    CHECK_SRV("--", "Channel reop #xyz:  nick!user@host ",
              "irc_344,irc_numeric,nick_server,log3");

    /* whois, geo info (UnrealIRCd) */
    RECV(":server 344 alice bob FR : \002is connecting from France ");
    CHECK_SRV("--", "[bob]  is connecting from France  (FR)",
              "irc_344,irc_numeric,nick_server,log3");

    /* whois, geo info (UnrealIRCd), no country code */
    RECV(":server 344 alice bob : \002is connecting from France ");
    CHECK_SRV("--", "[bob]  is connecting from France ",
              "irc_344,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "#test: end", "irc_345,irc_numeric,nick_server,log3");
    RECV(":server 345 alice #test : \002End of Channel Reop List ");
    CHECK_SRV("--", "#test:  End of Channel Reop List ",
              "irc_345,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 345 alice #xyz end");
    CHECK_SRV("--", "#xyz: end", "irc_345,irc_numeric,nick_server,log3");
    RECV(":server 345 alice #xyz :\002End of Channel Reop List");
    CHECK_SRV("--", "#xyz: End of Channel Reop List",
              "irc_345,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "[#test] [1] invitemask invited",
               "irc_346,irc_numeric,nick_server,log3");
    RECV(":server 346 alice #test invitemask nick!user_\00304red@host_\00304red");
    CHECK_CHAN("--", "[#test] [2] invitemask invited by nick (user_red@host_red)",
               "irc_346,irc_numeric,nick_server,log3");
    RECV(":server 346 alice #test invitemask nick!user_\00304red@host_\00304red 1205590879 ");
    CHECK_CHAN("--",
               "[#test] [3] invitemask invited by nick (user_red@host_red) "
               "on Sat, 15 Mar 2008 14:21:19",
               "irc_346,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 346 alice #xyz invitemask");
    CHECK_SRV("--", "[#xyz] invitemask invited",
              "irc_346,irc_numeric,nick_server,log3");
    RECV(":server 346 alice #xyz invitemask nick!user_\00304red@host_\00304red");
    CHECK_SRV("--", "[#xyz] invitemask invited by nick (user_red@host_red)",
              "irc_346,irc_numeric,nick_server,log3");
    RECV(":server 346 alice #xyz invitemask nick!user_\00304red@host_\00304red 1205590879");
    CHECK_SRV("--",
              "[#xyz] invitemask invited by nick (user_red@host_red) "
              "on Sat, 15 Mar 2008 14:21:19",
              "irc_346,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "[#test]", "irc_347,irc_numeric,nick_server,log3");
    RECV(":server 347 alice #test end");
    CHECK_CHAN("--", "[#test] end", "irc_347,irc_numeric,nick_server,log3");
    RECV(":server 347 alice #test : \002End of Channel Invite List ");
    CHECK_CHAN("--", "[#test]  End of Channel Invite List ",
               "irc_347,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 347 alice #xyz");
    CHECK_SRV("--", "[#xyz]", "irc_347,irc_numeric,nick_server,log3");
    RECV(":server 347 alice #xyz end");
    CHECK_SRV("--", "[#xyz] end", "irc_347,irc_numeric,nick_server,log3");
    RECV(":server 347 alice #xyz :\002End of Channel Invite List");
    CHECK_SRV("--", "[#xyz] End of Channel Invite List",
              "irc_347,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "[#test] [1] exception nick1!user1@host1",
               "irc_348,irc_numeric,nick_server,log3");
    RECV(":server 348 alice #test nick1!user_\00304red@host_\00304red "
         "nick2!user_\00302blue@host_\00302blue");
    CHECK_CHAN("--",
               "[#test] [2] exception nick1!user_red@host_red "
               "by nick2 (user_blue@host_blue)",
               "irc_348,irc_numeric,nick_server,log3");
    RECV(":server 348 alice #test nick1!user_\00304red@host_\00304red "
         "nick2!user_\00302blue@host_\00302blue 1205585109 ");
    CHECK_CHAN("--",
               "[#test] [3] exception nick1!user_red@host_red "
               "by nick2 (user_blue@host_blue) on Sat, 15 Mar 2008 12:45:09",
               "irc_348,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 348 alice #xyz nick1!user_\00304red@host_\00304red");
    CHECK_SRV("--", "[#xyz] exception nick1!user_red@host_red",
              "irc_348,irc_numeric,nick_server,log3");
    RECV(":server 348 alice #xyz nick1!user_\00304red@host_\00304red "
         "nick2!user_\00302blue@host_\00302blue");
    CHECK_SRV("--",
              "[#xyz] exception nick1!user_red@host_red "
              "by nick2 (user_blue@host_blue)",
              "irc_348,irc_numeric,nick_server,log3");
    RECV(":server 348 alice #xyz nick1!user_\00304red@host_\00304red "
         "nick2!user_\00302blue@host_\00302blue 1205585109");
    CHECK_SRV("--",
              "[#xyz] exception nick1!user_red@host_red "
              "by nick2 (user_blue@host_blue) on Sat, 15 Mar 2008 12:45:09",
              "irc_348,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "[#test]", "irc_349,irc_numeric,nick_server,log3");
    RECV(":server 349 alice #test end");
    CHECK_CHAN("--", "[#test] end", "irc_349,irc_numeric,nick_server,log3");
    RECV(":server 349 alice #test :\002End of Channel Exception List ");
    CHECK_CHAN("--", "[#test] End of Channel Exception List ",
               "irc_349,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 349 alice #xyz");
    CHECK_SRV("--", "[#xyz]", "irc_349,irc_numeric,nick_server,log3");
    RECV(":server 349 alice #xyz end");
    CHECK_SRV("--", "[#xyz] end", "irc_349,irc_numeric,nick_server,log3");
    RECV(":server 349 alice #xyz :\002End of Channel Exception List");
    CHECK_SRV("--", "[#xyz] End of Channel Exception List",
              "irc_349,irc_numeric,nick_server,log3");
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
    RECV(":server 350 alice bob : \002something here ");
    CHECK_SRV("--", "[bob]  something here ",
              "irc_350,irc_numeric,nick_server,log3");
    RECV(":server 350 alice bob * : something here ");
    CHECK_SRV("--", "[bob]  something here ",
              "irc_350,irc_numeric,nick_server,log3");

    /* non-standard parameters (using default whois callback) */
    RECV(":server 350 alice bob");
    CHECK_SRV("--", "bob", "irc_350,irc_numeric,nick_server,log3");

    /* standard parameters */
    RECV(":server 350 alice bob * * : \002is connected via the WebIRC gateway ");
    CHECK_SRV("--", "[bob]  is connected via the WebIRC gateway ",
              "irc_350,irc_numeric,nick_server,log3");
    RECV(":server 350 alice bob example.com * :is connected via the WebIRC gateway");
    CHECK_SRV("--", "[bob] (example.com) is connected via the WebIRC gateway",
              "irc_350,irc_numeric,nick_server,log3");
    RECV(":server 350 alice bob * 1.2.3.4 :is connected via the WebIRC gateway");
    CHECK_SRV("--", "[bob] (1.2.3.4) is connected via the WebIRC gateway",
              "irc_350,irc_numeric,nick_server,log3");
    RECV(":server 350 alice bob example.com 1.2.3.4 :is connected via the WebIRC gateway");
    CHECK_SRV("--",
              "[bob] (example.com, 1.2.3.4) is connected via the WebIRC gateway",
              "irc_350,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "dancer-ircd-1.0 server",
              "irc_351,irc_numeric,nick_server,log3");
    RECV(":server 351 alice dancer-ircd-1.0 server : iMZ \002dncrTS/v4 ");
    CHECK_SRV("--", "dancer-ircd-1.0 server ( iMZ dncrTS/v4 )",
              "irc_351,irc_numeric,nick_server,log3");
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
    STRCMP_EQUAL(NULL, ptr_nick->realname);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 352 alice #test user_\00304red host_\00304red server bob");
    CHECK_SRV("--", "[#test] bob (user_red@host_red) ()",
              "irc_352,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_\00304red@host_\00304red", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 352 alice #test user3 host3 server bob *");
    CHECK_SRV("--", "[#test] bob (user3@host3) * ()",
              "irc_352,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user3@host3", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 352 alice #test user4 host4 server bob * :0  \002real name 1 ");
    CHECK_SRV("--", "[#test] bob (user4@host4) * 0 (real name 1 )",
              "irc_352,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user4@host4", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("\002real name 1 ", ptr_nick2->realname);

    RECV(":server 352 alice #test user5 host5 server bob H@ :0 real name 2");
    CHECK_SRV("--", "[#test] bob (user5@host5) H@ 0 (real name 2)",
              "irc_352,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user5@host5", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("real name 2", ptr_nick2->realname);

    RECV(":server 352 alice #test user6 host6 server bob G@ :0 real name 3");
    CHECK_SRV("--", "[#test] bob (user6@host6) G@ 0 (real name 3)",
              "irc_352,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user6@host6", ptr_nick2->host);
    LONGS_EQUAL(1, ptr_nick2->away);
    STRCMP_EQUAL("real name 3", ptr_nick2->realname);

    RECV(":server 352 alice #test user7 host7 server bob * :0 real name 4");
    CHECK_SRV("--", "[#test] bob (user7@host7) * 0 (real name 4)",
              "irc_352,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user7@host7", ptr_nick2->host);
    LONGS_EQUAL(1, ptr_nick2->away);
    STRCMP_EQUAL("real name 4", ptr_nick2->realname);

    RECV(":server 352 alice #test user_\00304red host_\00304red server bob H@ :0 real name \00302blue");
    CHECK_SRV("--", "[#test] bob (user_red@host_red) H@ 0 (real name blue)",
              "irc_352,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_\00304red@host_\00304red", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("real name \00302blue", ptr_nick2->realname);

    RECV(":server 352 alice #test user8 host8 server bob H@ :0");
    CHECK_SRV("--", "[#test] bob (user8@host8) H@ 0 ()",
              "irc_352,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user8@host8", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("real name \00302blue", ptr_nick2->realname);

    /* nothing should have changed in the first nick */
    STRCMP_EQUAL("user_a@host_a", ptr_nick->host);
    LONGS_EQUAL(0, ptr_nick->away);
    STRCMP_EQUAL(NULL, ptr_nick->realname);

    /* channel not found */
    RECV(":server 352 alice #xyz user");
    CHECK_NO_MSG;
    RECV(":server 352 alice #xyz user host");
    CHECK_NO_MSG;
    RECV(":server 352 alice #xyz user host server");
    CHECK_NO_MSG;
    RECV(":server 352 alice #xyz user host server bob");
    CHECK_SRV("--", "[#xyz] bob (user@host) ()",
              "irc_352,irc_numeric,nick_server,log3");
    RECV(":server 352 alice #xyz user host server bob *");
    CHECK_SRV("--", "[#xyz] bob (user@host) * ()",
              "irc_352,irc_numeric,nick_server,log3");
    RECV(":server 352 alice #xyz user host server bob * :0 nick");
    CHECK_SRV("--", "[#xyz] bob (user@host) * 0 (nick)",
              "irc_352,irc_numeric,nick_server,log3");
    RECV(":server 352 alice #xyz user host server bob H@ :0 nick");
    CHECK_SRV("--", "[#xyz] bob (user@host) H@ 0 (nick)",
              "irc_352,irc_numeric,nick_server,log3");
    RECV(":server 352 alice #xyz user host server bob G@ :0 nick");
    CHECK_SRV("--", "[#xyz] bob (user@host) G@ 0 (nick)",
              "irc_352,irc_numeric,nick_server,log3");
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

    RECV(":server 353 alice #test :alice bob @carol  +dan!user_\00304red@host_\00304red ");
    CHECK_NO_MSG;
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);
    STRCMP_EQUAL("carol", ptr_channel->nicks->next_nick->next_nick->name);
    STRCMP_EQUAL("@", ptr_channel->nicks->next_nick->next_nick->prefix);
    STRCMP_EQUAL("dan",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->name);
    STRCMP_EQUAL("+",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->prefix);
    STRCMP_EQUAL("user_\00304red@host_\00304red",
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
    STRCMP_EQUAL("user_\00304red@host_\00304red",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->host);
    POINTERS_EQUAL(NULL,
                   ptr_channel->nicks->next_nick->next_nick->next_nick->next_nick);

    RECV(":server 353 alice = #test :alice bob @carol +dan!user_\00304red@host_\00304red");
    CHECK_NO_MSG;
    STRCMP_EQUAL("alice", ptr_channel->nicks->name);
    STRCMP_EQUAL("bob", ptr_channel->nicks->next_nick->name);
    STRCMP_EQUAL("carol", ptr_channel->nicks->next_nick->next_nick->name);
    STRCMP_EQUAL("@", ptr_channel->nicks->next_nick->next_nick->prefix);
    STRCMP_EQUAL("dan",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->name);
    STRCMP_EQUAL("+",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->prefix);
    STRCMP_EQUAL("user_\00304red@host_\00304red",
                 ptr_channel->nicks->next_nick->next_nick->next_nick->host);
    POINTERS_EQUAL(NULL,
                   ptr_channel->nicks->next_nick->next_nick->next_nick->next_nick);

    /* with option irc.look.color_nicks_in_names enabled */
    config_file_option_set (irc_config_look_color_nicks_in_names, "on", 1);
    RECV(":server 353 alice = #test :alice bob @carol +dan!user@host");
    config_file_option_unset (irc_config_look_color_nicks_in_names);

    /* channel not found */
    RECV(":server 353 alice #xyz :alice");
    CHECK_SRV("--", "Nicks #xyz: [alice]",
              "irc_353,irc_numeric,nick_server,log3");
    RECV(":server 353 alice #xyz :alice bob @carol +dan!user@host");
    CHECK_SRV("--", "Nicks #xyz: [alice bob @carol +dan]",
              "irc_353,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 353 alice = #xyz :alice");
    CHECK_SRV("--", "Nicks #xyz: [alice]",
              "irc_353,irc_numeric,nick_server,log3");
    RECV(":server 353 alice = #xyz :alice bob @carol +dan!user@host");
    CHECK_SRV("--", "Nicks #xyz: [alice bob @carol +dan]",
              "irc_353,irc_numeric,nick_server,log3");
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
    STRCMP_EQUAL(NULL, ptr_nick->account);
    STRCMP_EQUAL(NULL, ptr_nick2->account);
    STRCMP_EQUAL(NULL, ptr_nick->realname);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test");
    CHECK_SRV("--", "[#test]", "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL(NULL, ptr_nick2->account);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2");
    CHECK_SRV("--", "[#test] user2", "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL(NULL, ptr_nick2->account);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 :trailing parameter");
    CHECK_SRV("--", "[#test] user2 trailing parameter",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL(NULL, ptr_nick2->account);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2");
    CHECK_SRV("--", "[#test] user2 host2", "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL(NULL, ptr_nick2->account);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2 server");
    CHECK_SRV("--", "[#test] user2 host2 server",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL(NULL, ptr_nick2->account);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2 server bob");
    CHECK_SRV("--", "[#test] user2 host2 server bob",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL(NULL, ptr_nick2->account);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2 server bob *");
    CHECK_SRV("--", "[#test] user2 host2 server bob *",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL(NULL, ptr_nick2->account);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2 server bob H@ 0");
    CHECK_SRV("--", "[#test] user2 host2 server bob H@ 0",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_b@host_b", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL(NULL, ptr_nick2->account);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user2 host2 server bob * 0 \002account2");
    CHECK_SRV("--", "[#test] bob [account2] (user2@host2) * 0 ()",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user2@host2", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("\002account2", ptr_nick2->account);
    STRCMP_EQUAL(NULL, ptr_nick2->realname);

    RECV(":server 354 alice #test user3 host3 server bob * 0 \002account3 "
         ": \002real name 2 ");
    CHECK_SRV("--", "[#test] bob [account3] (user3@host3) * 0 ( real name 2 )",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user3@host3", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("\002account3", ptr_nick2->account);
    STRCMP_EQUAL(" \002real name 2 ", ptr_nick2->realname);

    RECV(":server 354 alice #test user4 host4 server bob H@ 0 account4 "
         ":real name 3");
    CHECK_SRV("--",
              "[#test] bob [account4] (user4@host4) H@ 0 (real name 3)",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user4@host4", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("account4", ptr_nick2->account);
    STRCMP_EQUAL("real name 3", ptr_nick2->realname);

    RECV(":server 354 alice #test user5 host5 server bob G@ 0 account5 "
         ":real name 4");
    CHECK_SRV("--",
              "[#test] bob [account5] (user5@host5) G@ 0 (real name 4)",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user5@host5", ptr_nick2->host);
    LONGS_EQUAL(1, ptr_nick2->away);
    STRCMP_EQUAL("account5", ptr_nick2->account);
    STRCMP_EQUAL("real name 4", ptr_nick2->realname);

    RECV(":server 354 alice #test user6 host6 server bob * 0 account6 "
         ":real name 5");
    CHECK_SRV("--",
              "[#test] bob [account6] (user6@host6) * 0 (real name 5)",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user6@host6", ptr_nick2->host);
    LONGS_EQUAL(1, ptr_nick2->away);
    STRCMP_EQUAL("account6", ptr_nick2->account);
    STRCMP_EQUAL("real name 5", ptr_nick2->realname);

    RECV(":server 354 alice #test user_\00304red host_\00304red server bob "
         "H@ 0 account_\00304red :real name \00302blue");
    CHECK_SRV("--",
              "[#test] bob [account_red] (user_red@host_red) H@ 0 "
              "(real name blue)",
              "irc_354,irc_numeric,nick_server,log3");
    STRCMP_EQUAL("user_\00304red@host_\00304red", ptr_nick2->host);
    LONGS_EQUAL(0, ptr_nick2->away);
    STRCMP_EQUAL("account_\00304red", ptr_nick2->account);
    STRCMP_EQUAL("real name \00302blue", ptr_nick2->realname);

    /* nothing should have changed in the first nick */
    STRCMP_EQUAL("user_a@host_a", ptr_nick->host);
    LONGS_EQUAL(0, ptr_nick->away);
    STRCMP_EQUAL(NULL, ptr_nick->account);
    STRCMP_EQUAL(NULL, ptr_nick->realname);

    /* channel not found */
    RECV(":server 354 alice #xyz");
    CHECK_SRV("--", "[#xyz]", "irc_354,irc_numeric,nick_server,log3");
    RECV(":server 354 alice #xyz user2");
    CHECK_SRV("--", "[#xyz] user2", "irc_354,irc_numeric,nick_server,log3");
    RECV(":server 354 alice #xyz user2 host2");
    CHECK_SRV("--", "[#xyz] user2 host2", "irc_354,irc_numeric,nick_server,log3");
    RECV(":server 354 alice #xyz user2 host2 server");
    CHECK_SRV("--", "[#xyz] user2 host2 server",
              "irc_354,irc_numeric,nick_server,log3");
    RECV(":server 354 alice #xyz user2 host2 server bob");
    CHECK_SRV("--", "[#xyz] user2 host2 server bob",
              "irc_354,irc_numeric,nick_server,log3");
    RECV(":server 354 alice #xyz user2 host2 server bob *");
    CHECK_SRV("--", "[#xyz] user2 host2 server bob *",
              "irc_354,irc_numeric,nick_server,log3");
    RECV(":server 354 alice #xyz user2 host2 server bob G@ 0");
    CHECK_SRV("--", "[#xyz] user2 host2 server bob G@ 0",
              "irc_354,irc_numeric,nick_server,log3");
    RECV(":server 354 alice #xyz user2 host2 server bob H@ 0 account");
    CHECK_SRV("--", "[#xyz] bob [account] (user2@host2) H@ 0 ()",
              "irc_354,irc_numeric,nick_server,log3");
    RECV(":server 354 alice #xyz user2 host2 server bob G@ 0 account "
         ":real name");
    CHECK_SRV("--", "[#xyz] bob [account] (user2@host2) G@ 0 (real name)",
              "irc_354,irc_numeric,nick_server,log3");

    hashtable_remove (ptr_server->cap_list, "account-notify");
}

/*
 * Tests functions:
 *   irc_protocol_get_string_channel_nicks
 *   irc_protocol_get_string_channel_nicks_count
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
    CHECK_CHAN("--", "Channel #test: 1 nick (0 ops, 0 voiced, 1 regular)",
               "irc_366,irc_numeric,nick_server,log3");
    RECV(":server 366 alice #test : \002End of /NAMES list ");
    CHECK_CHAN("--", "Channel #test: 1 nick (0 ops, 0 voiced, 1 regular)",
               "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :bob");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("--", "Channel #test: 2 nicks (0 ops, 0 voiced, 2 regular)",
               "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :@carol");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("--", "Channel #test: 3 nicks (1 op, 0 voiced, 2 regular)",
               "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :+dan!user@host");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("--", "Channel #test: 4 nicks (1 op, 1 voiced, 2 regular)",
               "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :@evans");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("--", "Channel #test: 5 nicks (2 ops, 1 voiced, 2 regular)",
               "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :+fred");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("--", "Channel #test: 6 nicks (2 ops, 2 voiced, 2 regular)",
               "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :greg");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("--", "Channel #test: 7 nicks (2 ops, 2 voiced, 3 regular)",
               "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 005 alice " IRC_MSG_005 " :are supported");

    RECV(":server 353 alice = #test :%harry");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("--",
               "Channel #test: 8 nicks (2 ops, 1 halfop, 2 voiced, 3 regular)",
               "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :%ian");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN("--",
               "Channel #test: 9 nicks (2 ops, 2 halfops, 2 voiced, 3 regular)",
               "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 005 alice PREFIX=(qaohv)~&@%+ :are supported");

    RECV(":server 353 alice = #test :~jessica");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN(
        "--",
        "Channel #test: 10 nicks (1 owner, 0 admins, 2 ops, 2 halfops, 2 voiced, 3 regular)",
        "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :&karl");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN(
        "--",
        "Channel #test: 11 nicks (1 owner, 1 admin, 2 ops, 2 halfops, 2 voiced, 3 regular)",
        "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :&mike");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN(
        "--",
        "Channel #test: 12 nicks (1 owner, 2 admins, 2 ops, 2 halfops, 2 voiced, 3 regular)",
        "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :~olivia");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN(
        "--",
        "Channel #test: 13 nicks (2 owners, 2 admins, 2 ops, 2 halfops, 2 voiced, 3 regular)",
        "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 005 alice PREFIX=(zqaohv)?~&@%+ :are supported");

    RECV(":server 353 alice = #test :?peggy");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN(
        "--",
        "Channel #test: 14 nicks (1 +z, 2 owners, 2 admins, 2 ops, 2 halfops, 2 voiced, 3 regular)",
        "irc_366,irc_numeric,nick_server,log3");

    RECV(":server 353 alice = #test :?robert");
    RECV(":server 366 alice #test :End of /NAMES list");
    CHECK_CHAN(
        "--",
        "Channel #test: 15 nicks (2 +z, 2 owners, 2 admins, 2 ops, 2 halfops, 2 voiced, 3 regular)",
        "irc_366,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 366 alice #xyz end");
    CHECK_SRV("--", "#xyz: end", "irc_366,irc_numeric,nick_server,log3");
    RECV(":server 366 alice #xyz : End of /NAMES list ");
    CHECK_SRV("--", "#xyz:  End of /NAMES list ",
              "irc_366,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "[#test] [1] nick1!user1@host1 banned",
               "irc_367,irc_numeric,nick_server,log3");
    RECV(":server 367 alice #test nick1!user1@host1 nick2!user2@host2 ");
    CHECK_CHAN("--",
               "[#test] [2] nick1!user1@host1 banned by nick2 (user2@host2)",
               "irc_367,irc_numeric,nick_server,log3");
    RECV(":server 367 alice #test nick1!user1@host1 nick2!user2@host2 "
         "1205585109 ");
    CHECK_CHAN("--",
               "[#test] [3] nick1!user1@host1 banned "
               "by nick2 (user2@host2) on Sat, 15 Mar 2008 12:45:09",
               "irc_367,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 367 alice #xyz nick1!user1@host1");
    CHECK_SRV("--", "[#xyz] nick1!user1@host1 banned",
              "irc_367,irc_numeric,nick_server,log3");
    RECV(":server 367 alice #xyz nick1!user1@host1 nick2!user2@host2");
    CHECK_SRV("--", "[#xyz] nick1!user1@host1 banned by nick2 (user2@host2)",
              "irc_367,irc_numeric,nick_server,log3");
    RECV(":server 367 alice #xyz nick1!user1@host1 nick2!user2@host2 "
         "1205585109");
    CHECK_SRV("--", "[#xyz] nick1!user1@host1 banned by nick2 (user2@host2) "
              "on Sat, 15 Mar 2008 12:45:09",
              "irc_367,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "[#test]", "irc_368,irc_numeric,nick_server,log3");
    RECV(":server 368 alice #test end");
    CHECK_CHAN("--", "[#test] end", "irc_368,irc_numeric,nick_server,log3");
    RECV(":server 368 alice #test : \002End of Channel Ban List ");
    CHECK_CHAN("--", "[#test]  End of Channel Ban List ",
               "irc_368,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 368 alice #xyz");
    CHECK_SRV("--", "[#xyz]", "irc_368,irc_numeric,nick_server,log3");
    RECV(":server 368 alice #xyz end");
    CHECK_SRV("--", "[#xyz] end", "irc_368,irc_numeric,nick_server,log3");
    RECV(":server 368 alice #xyz :\002End of Channel Ban List");
    CHECK_SRV("--", "[#xyz] End of Channel Ban List",
              "irc_368,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "bob", "irc_401,irc_numeric,nick_server,log3");
    RECV(":server 401 alice bob : \002No such nick/channel ");
    CHECK_SRV("--", "bob:  No such nick/channel ",
              "irc_401,irc_numeric,nick_server,log3");

    RECV(":server 401 alice #unknown :\002No such nick/channel");
    CHECK_SRV("--", "#unknown: No such nick/channel",
              "irc_401,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "server", "irc_402,irc_numeric,nick_server,log3");
    RECV(":server 402 alice server : \002No such server ");
    CHECK_SRV("--", "server:  No such server ",
              "irc_402,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_403 (no such channel)
 */

TEST(IrcProtocolWithServer, 403)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 403");
    CHECK_ERROR_PARAMS("403", 0, 2);
    RECV(":server 403 alice");
    CHECK_ERROR_PARAMS("403", 1, 2);

    RECV(":server 403 alice #test2");
    CHECK_SRV("--", "#test2", "irc_403,irc_numeric,nick_server,log3");
    RECV(":server 403 alice #test2 : \002No such channel ");
    CHECK_SRV("--", "#test2:  No such channel ",
              "irc_403,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "#test", "irc_404,irc_numeric,nick_server,log3");
    RECV(":server 404 alice #test : \002Cannot send to channel ");
    CHECK_CHAN("--", "#test:  Cannot send to channel ",
               "irc_404,irc_numeric,nick_server,log3");
    RECV(":server 404 alice #test2 :\002Cannot send to channel");
    CHECK_SRV("--", "#test2: Cannot send to channel",
              "irc_404,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_405 (too many channels)
 */

TEST(IrcProtocolWithServer, 405)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 405");
    CHECK_ERROR_PARAMS("405", 0, 2);
    RECV(":server 405 alice");
    CHECK_ERROR_PARAMS("405", 1, 2);

    RECV(":server 405 alice #test2");
    CHECK_SRV("--", "#test2", "irc_405,irc_numeric,nick_server,log3");
    RECV(":server 405 alice #test2 : \002You have joined too many channels ");
    CHECK_SRV("--", "#test2:  You have joined too many channels ",
              "irc_405,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_406 (was no such nick)
 */

TEST(IrcProtocolWithServer, 406)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 406");
    CHECK_ERROR_PARAMS("406", 0, 2);
    RECV(":server 406 alice");
    CHECK_ERROR_PARAMS("406", 1, 2);

    RECV(":server 406 alice bob");
    CHECK_SRV("--", "bob", "irc_406,irc_numeric,nick_server,log3");
    RECV(":server 406 alice bob : \002There was no such nick ");
    CHECK_SRV("--", "bob:  There was no such nick ",
              "irc_406,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_407 (too many targets)
 */

TEST(IrcProtocolWithServer, 407)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 407");
    CHECK_ERROR_PARAMS("407", 0, 2);
    RECV(":server 407 alice");
    CHECK_ERROR_PARAMS("407", 1, 2);

    RECV(":server 407 alice bob@host");
    CHECK_SRV("--", "bob@host", "irc_407,irc_numeric,nick_server,log3");
    RECV(":server 407 alice bob@host : \002Duplicate recipients. No message delivered ");
    CHECK_SRV("--", "bob@host:  Duplicate recipients. No message delivered ",
              "irc_407,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_409 (no origin)
 */

TEST(IrcProtocolWithServer, 409)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 409");
    CHECK_ERROR_PARAMS("409", 0, 2);
    RECV(":server 409 alice");
    CHECK_ERROR_PARAMS("409", 1, 2);

    RECV(":server 409 alice : \002No origin specified ");
    CHECK_SRV("--", " No origin specified ",
              "irc_409,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_411 (no recipient)
 */

TEST(IrcProtocolWithServer, 411)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 411");
    CHECK_ERROR_PARAMS("411", 0, 2);
    RECV(":server 411 alice");
    CHECK_ERROR_PARAMS("411", 1, 2);

    RECV(":server 411 alice : \002No recipient given (PRIVMSG) ");
    CHECK_SRV("--", " No recipient given (PRIVMSG) ",
              "irc_411,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_412 (no text to send)
 */

TEST(IrcProtocolWithServer, 412)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 412");
    CHECK_ERROR_PARAMS("412", 0, 2);
    RECV(":server 412 alice");
    CHECK_ERROR_PARAMS("412", 1, 2);

    RECV(":server 412 alice : \002No text to send ");
    CHECK_SRV("--", " No text to send ",
              "irc_412,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_413 (no toplevel)
 */

TEST(IrcProtocolWithServer, 413)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 413");
    CHECK_ERROR_PARAMS("413", 0, 2);
    RECV(":server 413 alice");
    CHECK_ERROR_PARAMS("413", 1, 2);

    RECV(":server 413 alice mask");
    CHECK_SRV("--", "mask", "irc_413,irc_numeric,nick_server,log3");
    RECV(":server 413 alice mask : \002No toplevel domain specified ");
    CHECK_SRV("--", "mask:  No toplevel domain specified ",
              "irc_413,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_414 (wildcard in toplevel domain)
 */

TEST(IrcProtocolWithServer, 414)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 414");
    CHECK_ERROR_PARAMS("414", 0, 2);
    RECV(":server 414 alice");
    CHECK_ERROR_PARAMS("414", 1, 2);

    RECV(":server 414 alice mask");
    CHECK_SRV("--", "mask", "irc_414,irc_numeric,nick_server,log3");
    RECV(":server 414 alice mask : \002Wildcard in toplevel domain ");
    CHECK_SRV("--", "mask:  Wildcard in toplevel domain ",
              "irc_414,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_415 (cannot send to channel)
 */

TEST(IrcProtocolWithServer, 415)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 415");
    CHECK_ERROR_PARAMS("415", 0, 2);
    RECV(":server 415 alice");
    CHECK_ERROR_PARAMS("415", 1, 2);

    RECV(":server 415 alice #test");
    CHECK_SRV("--", "#test", "irc_415,irc_numeric,nick_server,log3");
    RECV(":server 415 alice #test : \002Cannot send message to channel (+R) ");
    CHECK_CHAN("--", "#test:  Cannot send message to channel (+R) ",
               "irc_415,irc_numeric,nick_server,log3");
    RECV(":server 415 alice #test2 :\002Cannot send message to channel (+R)");
    CHECK_SRV("--", "#test2: Cannot send message to channel (+R)",
              "irc_415,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_421 (unknown command)
 */

TEST(IrcProtocolWithServer, 421)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 421");
    CHECK_ERROR_PARAMS("421", 0, 2);
    RECV(":server 421 alice");
    CHECK_ERROR_PARAMS("421", 1, 2);

    RECV(":server 421 alice UNKNOWN");
    CHECK_SRV("--", "UNKNOWN", "irc_421,irc_numeric,nick_server,log3");
    RECV(":server 421 alice UNKNOWN : \002Unknown command ");
    CHECK_SRV("--", "UNKNOWN:  Unknown command ",
              "irc_421,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_422 (MOTD is missing)
 */

TEST(IrcProtocolWithServer, 422)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 422");
    CHECK_ERROR_PARAMS("422", 0, 2);
    RECV(":server 422 alice");
    CHECK_ERROR_PARAMS("422", 1, 2);

    RECV(":server 422 alice : \002MOTD file is missing ");
    CHECK_SRV("--", " MOTD file is missing ",
              "irc_422,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_423 (no administrative info)
 */

TEST(IrcProtocolWithServer, 423)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 423");
    CHECK_ERROR_PARAMS("423", 0, 2);
    RECV(":server 423 alice");
    CHECK_ERROR_PARAMS("423", 1, 2);

    RECV(":server 423 alice server");
    CHECK_SRV("--", "server", "irc_423,irc_numeric,nick_server,log3");
    RECV(":server 423 alice server : \002No administrative info available ");
    CHECK_SRV("--", "server:  No administrative info available ",
              "irc_423,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_424 (file error)
 */

TEST(IrcProtocolWithServer, 424)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 424");
    CHECK_ERROR_PARAMS("424", 0, 2);
    RECV(":server 424 alice");
    CHECK_ERROR_PARAMS("424", 1, 2);

    RECV(":server 424 alice : \002File error doing read on /path/to/file ");
    CHECK_SRV("--", " File error doing read on /path/to/file ",
              "irc_424,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_431 (no nickname given)
 */

TEST(IrcProtocolWithServer, 431)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 431");
    CHECK_ERROR_PARAMS("431", 0, 2);
    RECV(":server 431 alice");
    CHECK_ERROR_PARAMS("431", 1, 2);

    RECV(":server 431 alice : \002No nickname given ");
    CHECK_SRV("--", " No nickname given ",
              "irc_431,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_432 (erroneous nickname, not connected)
 */

TEST(IrcProtocolWithServer, 432_not_connected)
{
    RECV(":server 432 * alice error");
    CHECK_SRV("--", "* alice error", "");
    CHECK_SRV("=!=",
              "irc: nickname \"nick1\" is invalid, trying nickname \"nick2\"",
              "");

    RECV(":server 432 * :alice error");
    CHECK_SRV("--", "* alice error", "");
    CHECK_SRV("=!=",
              "irc: nickname \"nick2\" is invalid, trying nickname \"nick3\"",
              "");

    RECV(":server 432 * alice : \002Erroneous Nickname ");
    CHECK_SRV("--", "* alice  Erroneous Nickname ", "");
    CHECK_SRV("=!=",
              "irc: nickname \"nick3\" is invalid, trying nickname \"nick1_\"",
              "");

    RECV(":server 432 * alice1 :\002Erroneous Nickname");
    CHECK_SRV("--", "* alice1 Erroneous Nickname", "");
    CHECK_SRV("=!=",
              "irc: nickname \"nick1_\" is invalid, "
              "trying nickname \"nick1__\"",
              "");
}

/*
 * Tests functions:
 *   irc_protocol_cb_432 (erroneous nickname, connected)
 */

TEST(IrcProtocolWithServer, 432_connected)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 432");
    CHECK_ERROR_PARAMS("432", 0, 2);
    RECV(":server 432 alice");
    CHECK_ERROR_PARAMS("432", 1, 2);

    RECV(":server 432 alice test%+");
    CHECK_SRV("--", "test%+", "irc_432,irc_numeric,nick_server,log3");
    RECV(":server 432 alice test%+ error");
    CHECK_SRV("--", "test%+: error", "irc_432,irc_numeric,nick_server,log3");
    RECV(":server 432 alice test%+ : \002Erroneous Nickname ");
    CHECK_SRV("--", "test%+:  Erroneous Nickname ",
              "irc_432,irc_numeric,nick_server,log3");

    /*
     * special case: erroneous nick is a channel: check that the message is
     * still displayed on the server buffer
     */
    RECV(":server 432 alice #test : \002Erroneous Nickname ");
    CHECK_SRV("--", "#test:  Erroneous Nickname ",
              "irc_432,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_433 (nickname already in use, not connected)
 */

TEST(IrcProtocolWithServer, 433_not_connected)
{
    RECV(":server 433 * alice error");
    CHECK_SRV("--",
              "irc: nickname \"nick1\" is already in use, "
              "trying nickname \"nick2\"",
              "");

    RECV(":server 433 * alice : Nickname is already in use. ");
    CHECK_SRV("--",
              "irc: nickname \"nick2\" is already in use, "
              "trying nickname \"nick3\"",
              "");

    RECV(":server 433 * alice1 :Nickname is already in use.");
    CHECK_SRV("--",
              "irc: nickname \"nick3\" is already in use, "
              "trying nickname \"nick1_\"",
              "");
}

/*
 * Tests functions:
 *   irc_protocol_cb_433 (nickname already in use, connected)
 */

TEST(IrcProtocolWithServer, 433_connected)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 433");
    CHECK_ERROR_PARAMS("433", 0, 2);
    RECV(":server 433 alice");
    CHECK_ERROR_PARAMS("433", 1, 2);

    RECV(":server 433 alice test");
    CHECK_SRV("--", "test", "irc_433,irc_numeric,nick_server,log3");
    RECV(":server 433 alice test error");
    CHECK_SRV("--", "test: error", "irc_433,irc_numeric,nick_server,log3");
    RECV(":server 433 alice test : \002Nickname is already in use. ");
    CHECK_SRV("--", "test:  Nickname is already in use. ",
              "irc_433,irc_numeric,nick_server,log3");

    /*
     * special case: nickname already used looks like a channel (it should
     * never happen in practice): check that the message is still displayed
     * on the server buffer
     */
    RECV(":server 433 alice #test : \002Nickname is already in use. ");
    CHECK_SRV("--", "#test:  Nickname is already in use. ",
              "irc_433,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_436 (nickname collision)
 */

TEST(IrcProtocolWithServer, 436)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 436");
    CHECK_ERROR_PARAMS("436", 0, 2);
    RECV(":server 436 alice");
    CHECK_ERROR_PARAMS("436", 1, 2);

    RECV(":server 436 alice bob");
    CHECK_SRV("--", "bob", "irc_436,irc_numeric,nick_server,log3");
    RECV(":server 436 alice bob error");
    CHECK_SRV("--", "bob: error", "irc_436,irc_numeric,nick_server,log3");
    RECV(":server 436 alice bob : \002Nickname collision KILL ");
    CHECK_SRV("--", "bob:  Nickname collision KILL ",
              "irc_436,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_437 (nick/channel temporarily unavailable, not connected)
 */

TEST(IrcProtocolWithServer, 437_not_connected)
{
    RECV(":server 437 * alice error");
    CHECK_SRV("--", "* alice error", "irc_437,irc_numeric,nick_server,log3");
    RECV(":server 437 * alice : \002Nick/channel is temporarily unavailable ");
    CHECK_SRV("--", "* alice  Nick/channel is temporarily unavailable ",
              "irc_437,irc_numeric,nick_server,log3");
    RECV(":server 437 * alice1 :\002Nick/channel is temporarily unavailable");
    CHECK_SRV("--", "* alice1 Nick/channel is temporarily unavailable",
              "irc_437,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_437 (nick/channel temporarily unavailable, connected)
 */

TEST(IrcProtocolWithServer, 437_connected)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 437");
    CHECK_ERROR_PARAMS("437", 0, 2);
    RECV(":server 437 alice");
    CHECK_ERROR_PARAMS("437", 1, 2);

    RECV(":server 437 * alice");
    CHECK_SRV("--", "* alice", "irc_437,irc_numeric,nick_server,log3");
    RECV(":server 437 * alice error");
    CHECK_SRV("--", "* alice error", "irc_437,irc_numeric,nick_server,log3");
    RECV(":server 437 * alice : \002Nick/channel is temporarily unavailable ");
    CHECK_SRV("--", "* alice  Nick/channel is temporarily unavailable ",
              "irc_437,irc_numeric,nick_server,log3");
    RECV(":server 437 alice #test :\002Cannot change nickname while banned on channel");
    CHECK_SRV("--", "#test: Cannot change nickname while banned on channel",
              "irc_437,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "alice alice2", "irc_438,irc_numeric,nick_server,log3");
    RECV(":server 438 alice alice2 error");
    CHECK_SRV("--", "error (alice => alice2)",
              "irc_438,irc_numeric,nick_server,log3");
    RECV(":server 438 alice alice2 : \002Nick change too fast. "
         "Please wait 30 seconds. ");
    CHECK_SRV("--",
              " Nick change too fast. Please wait 30 seconds.  (alice => alice2)",
              "irc_438,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_441 (user not in channel)
 */

TEST(IrcProtocolWithServer, 441)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 441");
    CHECK_ERROR_PARAMS("441", 0, 2);
    RECV(":server 441 alice");
    CHECK_ERROR_PARAMS("441", 1, 2);

    RECV(":server 441 alice bob");
    CHECK_SRV("--", "bob", "irc_441,irc_numeric,nick_server,log3");
    RECV(":server 441 alice bob #test2");
    CHECK_SRV("--", "bob: #test2", "irc_441,irc_numeric,nick_server,log3");
    RECV(":server 441 alice bob #test2 : \002They aren't on that channel ");
    CHECK_SRV("--", "bob: #test2  They aren't on that channel ",
              "irc_441,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_442 (not on channel)
 */

TEST(IrcProtocolWithServer, 442)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 442");
    CHECK_ERROR_PARAMS("442", 0, 2);
    RECV(":server 442 alice");
    CHECK_ERROR_PARAMS("442", 1, 2);

    RECV(":server 442 alice #test2");
    CHECK_SRV("--", "#test2", "irc_442,irc_numeric,nick_server,log3");
    RECV(":server 442 alice #test2 : \002You're not on that channel ");
    CHECK_SRV("--", "#test2:  You're not on that channel ",
              "irc_442,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_443 (user already on channel)
 */

TEST(IrcProtocolWithServer, 443)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 443");
    CHECK_ERROR_PARAMS("443", 0, 2);
    RECV(":server 443 alice");
    CHECK_ERROR_PARAMS("443", 1, 2);

    RECV(":server 443 alice bob");
    CHECK_SRV("--", "bob", "irc_443,irc_numeric,nick_server,log3");
    RECV(":server 443 alice bob #test2");
    CHECK_SRV("--", "bob: #test2", "irc_443,irc_numeric,nick_server,log3");
    RECV(":server 443 alice bob #test2 : \002is already on channel ");
    CHECK_SRV("--", "bob: #test2  is already on channel ",
              "irc_443,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_444 (user not logged in)
 */

TEST(IrcProtocolWithServer, 444)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 444");
    CHECK_ERROR_PARAMS("444", 0, 2);
    RECV(":server 444 alice");
    CHECK_ERROR_PARAMS("444", 1, 2);

    RECV(":server 444 alice bob");
    CHECK_SRV("--", "bob", "irc_444,irc_numeric,nick_server,log3");
    RECV(":server 444 alice bob error");
    CHECK_SRV("--", "bob: error", "irc_444,irc_numeric,nick_server,log3");
    RECV(":server 444 alice bob : \002User not logged in ");
    CHECK_SRV("--", "bob:  User not logged in ",
              "irc_444,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_445 (SUMMON has been disabled)
 */

TEST(IrcProtocolWithServer, 445)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 445");
    CHECK_ERROR_PARAMS("445", 0, 2);
    RECV(":server 445 alice");
    CHECK_ERROR_PARAMS("445", 1, 2);

    RECV(":server 445 alice : \002SUMMON has been disabled ");
    CHECK_SRV("--", " SUMMON has been disabled ",
              "irc_445,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_446 (USERS has been disabled)
 */

TEST(IrcProtocolWithServer, 446)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 446");
    CHECK_ERROR_PARAMS("446", 0, 2);
    RECV(":server 446 alice");
    CHECK_ERROR_PARAMS("446", 1, 2);

    RECV(":server 446 alice : \002USERS has been disabled ");
    CHECK_SRV("--", " USERS has been disabled ",
              "irc_446,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_451 (you are not registered)
 */

TEST(IrcProtocolWithServer, 451)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 451");
    CHECK_ERROR_PARAMS("451", 0, 2);
    RECV(":server 451 alice");
    CHECK_ERROR_PARAMS("451", 1, 2);

    RECV(":server 451 alice : \002You have not registered ");
    CHECK_SRV("--", " You have not registered ",
              "irc_451,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_461 (not enough parameters)
 */

TEST(IrcProtocolWithServer, 461)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 461");
    CHECK_ERROR_PARAMS("461", 0, 2);
    RECV(":server 461 alice");
    CHECK_ERROR_PARAMS("461", 1, 2);

    RECV(":server 461 alice PRIVMSG");
    CHECK_SRV("--", "PRIVMSG", "irc_461,irc_numeric,nick_server,log3");
    RECV(":server 461 alice PRIVMSG : \002Not enough parameters ");
    CHECK_SRV("--", "PRIVMSG:  Not enough parameters ",
              "irc_461,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_462 (you may not register)
 */

TEST(IrcProtocolWithServer, 462)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 462");
    CHECK_ERROR_PARAMS("462", 0, 2);
    RECV(":server 462 alice");
    CHECK_ERROR_PARAMS("462", 1, 2);

    RECV(":server 462 alice : \002You may not reregister ");
    CHECK_SRV("--", " You may not reregister ",
              "irc_462,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_463 (host not privileged)
 */

TEST(IrcProtocolWithServer, 463)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 463");
    CHECK_ERROR_PARAMS("463", 0, 2);
    RECV(":server 463 alice");
    CHECK_ERROR_PARAMS("463", 1, 2);

    RECV(":server 463 alice : \002Your host isn't among the privileged ");
    CHECK_SRV("--", " Your host isn't among the privileged ",
              "irc_463,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_464 (password incorrect)
 */

TEST(IrcProtocolWithServer, 464)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 464");
    CHECK_ERROR_PARAMS("464", 0, 2);
    RECV(":server 464 alice");
    CHECK_ERROR_PARAMS("464", 1, 2);

    RECV(":server 464 alice : \002Password incorrect ");
    CHECK_SRV("--", " Password incorrect ",
              "irc_464,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_465 (banned from this server)
 */

TEST(IrcProtocolWithServer, 465)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 465");
    CHECK_ERROR_PARAMS("465", 0, 2);
    RECV(":server 465 alice");
    CHECK_ERROR_PARAMS("465", 1, 2);

    RECV(":server 465 alice : \002You are banned from this server ");
    CHECK_SRV("--", " You are banned from this server ",
              "irc_465,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_467 (channel key already set)
 */

TEST(IrcProtocolWithServer, 467)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 467");
    CHECK_ERROR_PARAMS("467", 0, 2);
    RECV(":server 467 alice");
    CHECK_ERROR_PARAMS("467", 1, 2);

    RECV(":server 467 alice #test2");
    CHECK_SRV("--", "#test2", "irc_467,irc_numeric,nick_server,log3");
    RECV(":server 467 alice #test2 : \002Channel key already set ");
    CHECK_SRV("--", "#test2:  Channel key already set ",
              "irc_467,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "#test", "irc_470,irc_numeric,nick_server,log3");
    RECV(":server 470 alice #test #test2");
    CHECK_SRV("--", "#test: #test2", "irc_470,irc_numeric,nick_server,log3");
    RECV(":server 470 alice #test #test2 forwarding");
    CHECK_SRV("--", "#test: #test2 forwarding",
              "irc_470,irc_numeric,nick_server,log3");
    RECV(":server 470 alice #test #test2 : \002Forwarding to another channel ");
    CHECK_SRV("--", "#test: #test2  Forwarding to another channel ",
              "irc_470,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_471 (channel is already full)
 */

TEST(IrcProtocolWithServer, 471)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 471");
    CHECK_ERROR_PARAMS("471", 0, 2);
    RECV(":server 471 alice");
    CHECK_ERROR_PARAMS("471", 1, 2);

    RECV(":server 471 alice #test2");
    CHECK_SRV("--", "#test2", "irc_471,irc_numeric,nick_server,log3");
    RECV(":server 471 alice #test2 : \002Cannot join channel (+l) ");
    CHECK_SRV("--", "#test2:  Cannot join channel (+l) ",
              "irc_471,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_472 (unknown mode char to me)
 */

TEST(IrcProtocolWithServer, 472)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 472");
    CHECK_ERROR_PARAMS("472", 0, 2);
    RECV(":server 472 alice");
    CHECK_ERROR_PARAMS("472", 1, 2);

    RECV(":server 472 alice x");
    CHECK_SRV("--", "x", "irc_472,irc_numeric,nick_server,log3");
    RECV(":server 472 alice x : \002is unknown mode char to me ");
    CHECK_SRV("--", "x:  is unknown mode char to me ",
              "irc_472,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_473 (cannot join (invite only))
 */

TEST(IrcProtocolWithServer, 473)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 473");
    CHECK_ERROR_PARAMS("473", 0, 2);
    RECV(":server 473 alice");
    CHECK_ERROR_PARAMS("473", 1, 2);

    RECV(":server 473 alice #test2");
    CHECK_SRV("--", "#test2", "irc_473,irc_numeric,nick_server,log3");
    RECV(":server 473 alice #test2 : \002Cannot join channel (+i) ");
    CHECK_SRV("--", "#test2:  Cannot join channel (+i) ",
              "irc_473,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_474 (cannot join (banned))
 */

TEST(IrcProtocolWithServer, 474)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 474");
    CHECK_ERROR_PARAMS("474", 0, 2);
    RECV(":server 474 alice");
    CHECK_ERROR_PARAMS("474", 1, 2);

    RECV(":server 474 alice #test2");
    CHECK_SRV("--", "#test2", "irc_474,irc_numeric,nick_server,log3");
    RECV(":server 474 alice #test2 : \002Cannot join channel (+b) ");
    CHECK_SRV("--", "#test2:  Cannot join channel (+b) ",
              "irc_474,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_475 (cannot join (bad key))
 */

TEST(IrcProtocolWithServer, 475)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 475");
    CHECK_ERROR_PARAMS("475", 0, 2);
    RECV(":server 475 alice");
    CHECK_ERROR_PARAMS("475", 1, 2);

    RECV(":server 475 alice #test2");
    CHECK_SRV("--", "#test2", "irc_475,irc_numeric,nick_server,log3");
    RECV(":server 475 alice #test2 : \002Cannot join channel (+k) ");
    CHECK_SRV("--", "#test2:  Cannot join channel (+k) ",
              "irc_475,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_476 (bad channel mask)
 */

TEST(IrcProtocolWithServer, 476)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 476");
    CHECK_ERROR_PARAMS("476", 0, 2);
    RECV(":server 476 alice");
    CHECK_ERROR_PARAMS("476", 1, 2);

    RECV(":server 476 alice #test2");
    CHECK_SRV("--", "#test2", "irc_476,irc_numeric,nick_server,log3");
    RECV(":server 476 alice #test2 : \002Bad Channel Mask ");
    CHECK_SRV("--", "#test2:  Bad Channel Mask ",
              "irc_476,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_477 (channel doesn't support modes)
 */

TEST(IrcProtocolWithServer, 477)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 477");
    CHECK_ERROR_PARAMS("477", 0, 2);
    RECV(":server 477 alice");
    CHECK_ERROR_PARAMS("477", 1, 2);

    RECV(":server 477 alice #test2");
    CHECK_SRV("--", "#test2", "irc_477,irc_numeric,nick_server,log3");
    RECV(":server 477 alice #test2 : \002Channel doesn't support modes ");
    CHECK_SRV("--", "#test2:  Channel doesn't support modes ",
              "irc_477,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_481 (you're not an IRC operator)
 */

TEST(IrcProtocolWithServer, 481)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 481");
    CHECK_ERROR_PARAMS("481", 0, 2);
    RECV(":server 481 alice");
    CHECK_ERROR_PARAMS("481", 1, 2);

    RECV(":server 481 alice : \002Permission Denied- You're not an IRC operator ");
    CHECK_SRV("--", " Permission Denied- You're not an IRC operator ",
              "irc_481,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_482 (you're not channel operator)
 */

TEST(IrcProtocolWithServer, 482)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 482");
    CHECK_ERROR_PARAMS("482", 0, 2);
    RECV(":server 482 alice");
    CHECK_ERROR_PARAMS("482", 1, 2);

    RECV(":server 482 alice #test2");
    CHECK_SRV("--", "#test2", "irc_482,irc_numeric,nick_server,log3");
    RECV(":server 482 alice #test2 : \002You're not channel operator ");
    CHECK_SRV("--", "#test2:  You're not channel operator ",
              "irc_482,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_483 (you can't kill a server!)
 */

TEST(IrcProtocolWithServer, 483)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 483");
    CHECK_ERROR_PARAMS("483", 0, 2);
    RECV(":server 483 alice");
    CHECK_ERROR_PARAMS("483", 1, 2);

    RECV(":server 483 alice : \002You cant kill a server! ");
    CHECK_SRV("--", " You cant kill a server! ",
              "irc_483,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_484 (your connection is restricted!)
 */

TEST(IrcProtocolWithServer, 484)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 484");
    CHECK_ERROR_PARAMS("484", 0, 2);
    RECV(":server 484 alice");
    CHECK_ERROR_PARAMS("484", 1, 2);

    RECV(":server 484 alice : \002Your connection is restricted! ");
    CHECK_SRV("--", " Your connection is restricted! ",
              "irc_484,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_485 (not the original channel operator)
 */

TEST(IrcProtocolWithServer, 485)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 485");
    CHECK_ERROR_PARAMS("485", 0, 2);
    RECV(":server 485 alice");
    CHECK_ERROR_PARAMS("485", 1, 2);

    RECV(":server 485 alice : \002You're not the original channel operator ");
    CHECK_SRV("--", " You're not the original channel operator ",
              "irc_485,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_491 (no O-lines for your host)
 */

TEST(IrcProtocolWithServer, 491)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 491");
    CHECK_ERROR_PARAMS("491", 0, 2);
    RECV(":server 491 alice");
    CHECK_ERROR_PARAMS("491", 1, 2);

    RECV(":server 491 alice : \002No O-lines for your host ");
    CHECK_SRV("--", " No O-lines for your host ",
              "irc_491,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_501 (unknown mode flag)
 */

TEST(IrcProtocolWithServer, 501)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 501");
    CHECK_ERROR_PARAMS("501", 0, 2);
    RECV(":server 501 alice");
    CHECK_ERROR_PARAMS("501", 1, 2);

    RECV(":server 501 alice : \002Unknown MODE flag ");
    CHECK_SRV("--", " Unknown MODE flag ",
              "irc_501,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_502 (can't change mode for other users)
 */

TEST(IrcProtocolWithServer, 502)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 502");
    CHECK_ERROR_PARAMS("502", 0, 2);
    RECV(":server 502 alice");
    CHECK_ERROR_PARAMS("502", 1, 2);

    RECV(":server 502 alice : \002Cant change mode for other users ");
    CHECK_SRV("--", " Cant change mode for other users ",
              "irc_502,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "UNKNOWN", "irc_524,irc_numeric,nick_server,log3");
    RECV(":server 524 alice UNKNOWN : \002Help not found ");
    CHECK_SRV("--", "UNKNOWN:  Help not found ",
              "irc_524,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_569 (whois, connecting from)
 */

TEST(IrcProtocolWithServer, 569)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 569");
    CHECK_ERROR_PARAMS("569", 0, 2);
    RECV(":server 569 alice");
    CHECK_ERROR_PARAMS("569", 1, 2);

    /* whois, connecting from (UnrealIRCd) */
    RECV(":server 569 alice bob 12345 : \002is connecting from AS12345 [Hoster] ");
    CHECK_SRV("--", "[bob]  is connecting from AS12345 [Hoster]  (12345)",
              "irc_569,irc_numeric,nick_server,log3");

    /* whois, connecting from (UnrealIRCd), no ASN */
    RECV(":server 569 alice bob :\002is connecting from AS12345 [Hoster]");
    CHECK_SRV("--", "[bob] is connecting from AS12345 [Hoster]",
              "irc_569,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "MODE", "irc_704,irc_numeric,nick_server,log3");
    RECV(":server 704 alice MODE "
         ": MODE <target> [<modestring> [<mode arguments>...]] ");
    CHECK_SRV("--", "MODE:  MODE <target> [<modestring> [<mode arguments>...]] ",
              "irc_704,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "MODE", "irc_705,irc_numeric,nick_server,log3");
    RECV(":server 705 alice MODE : \002Sets and removes modes from the given target. ");
    CHECK_SRV("--", "MODE:  Sets and removes modes from the given target. ",
              "irc_705,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "MODE", "irc_706,irc_numeric,nick_server,log3");
    RECV(":server 706 alice MODE : \002End of /HELPOP ");
    CHECK_SRV("--", "MODE:  End of /HELPOP ",
              "irc_706,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_710 (knock: has asked for an invite)
 */

TEST(IrcProtocolWithServer, 710)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 710");
    CHECK_ERROR_PARAMS("710", 0, 3);
    RECV(":server 710 #test");
    CHECK_ERROR_PARAMS("710", 1, 3);
    RECV(":server 710 #test #test");
    CHECK_ERROR_PARAMS("710", 2, 3);

    RECV(":server 710 #test #test nick1!user1@host1");
    CHECK_CHAN("--", "nick1 (user1@host1) has asked for an invite",
               "irc_710,irc_numeric,notify_message,nick_nick1,host_user1@host1,log3");
    RECV(":server 710 #test #test nick1!user1@host1 : \002has asked for an invite. ");
    CHECK_CHAN("--", "nick1 (user1@host1)  has asked for an invite. ",
               "irc_710,irc_numeric,notify_message,nick_nick1,host_user1@host1,log3");

    /* channel not found */
    RECV(":server 710 #xyz #xyz nick1!user1@host1");
    CHECK_ERROR_PARSE("710", ":server 710 #xyz #xyz nick1!user1@host1");
    RECV(":server 710 #xyz #xyz nick1!user1@host1 : \002has asked for an invite. ");
    CHECK_ERROR_PARSE("710", ":server 710 #xyz #xyz nick1!user1@host1 : \002has asked for an invite. ");
}

/*
 * Tests functions:
 *   irc_protocol_cb_711 (knock: has been delivered)
 */

TEST(IrcProtocolWithServer, 711)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 711");
    CHECK_ERROR_PARAMS("711", 0, 3);
    RECV(":server 711 alice");
    CHECK_ERROR_PARAMS("711", 1, 3);
    RECV(":server 711 alice #test");
    CHECK_ERROR_PARAMS("711", 2, 3);

    RECV(":server 711 alice #test : \002Your KNOCK has been delivered. ");
    CHECK_SRV("--", "#test:  Your KNOCK has been delivered. ",
              "irc_711,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_712 (knock: too many knocks)
 */

TEST(IrcProtocolWithServer, 712)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 712");
    CHECK_ERROR_PARAMS("712", 0, 3);
    RECV(":server 712 alice");
    CHECK_ERROR_PARAMS("712", 1, 3);
    RECV(":server 712 alice #test");
    CHECK_ERROR_PARAMS("712", 2, 3);

    RECV(":server 712 alice #test : \002Too many KNOCKs (channel). ");
    CHECK_SRV("--", "#test:  Too many KNOCKs (channel). ",
              "irc_712,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_713 (knock: channel is open)
 */

TEST(IrcProtocolWithServer, 713)
{
    SRV_INIT;

    /* not enough parameters */
    RECV(":server 713");
    CHECK_ERROR_PARAMS("713", 0, 3);
    RECV(":server 713 alice");
    CHECK_ERROR_PARAMS("713", 1, 3);
    RECV(":server 713 alice #test");
    CHECK_ERROR_PARAMS("713", 2, 3);

    RECV(":server 713 alice #test : \002Channel is open. ");
    CHECK_SRV("--", "#test:  Channel is open. ",
              "irc_713,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_714 (knock: already on that channel)
 */

TEST(IrcProtocolWithServer, 714)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 714");
    CHECK_ERROR_PARAMS("714", 0, 3);
    RECV(":server 714 alice");
    CHECK_ERROR_PARAMS("714", 1, 3);
    RECV(":server 714 alice #test");
    CHECK_ERROR_PARAMS("714", 2, 3);

    RECV(":server 714 alice #test : \002You are already on that channel. ");
    CHECK_SRV("--", "#test:  You are already on that channel. ",
              "irc_714,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_716 (nick is in +g mode)
 */

TEST(IrcProtocolWithServer, 716)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 716");
    CHECK_ERROR_PARAMS("716", 0, 2);
    RECV(":server 716 alice");
    CHECK_ERROR_PARAMS("716", 1, 2);

    RECV(":server 716 alice bob : \002is in +g mode and must manually allow you to "
         "message them. Your message was discarded. ");
    CHECK_SRV("--",
              "bob:  is in +g mode and must manually allow you to message them. "
              "Your message was discarded. ",
              "irc_716,irc_numeric,nick_server,log3");

    /* open private buffer */
    RECV(":bob!user@host PRIVMSG alice :hi Alice!");

    RECV(":server 716 alice bob : \002is in +g mode and must manually allow you to "
         "message them. Your message was discarded. ");
    CHECK_PV("bob", "--",
             "bob:  is in +g mode and must manually allow you to message them. "
             "Your message was discarded. ",
             "irc_716,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_717 (nick has been informed that you messaged them)
 */

TEST(IrcProtocolWithServer, 717)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 717");
    CHECK_ERROR_PARAMS("717", 0, 2);
    RECV(":server 717 alice");
    CHECK_ERROR_PARAMS("717", 1, 2);

    RECV(":server 717 alice bob : \002has been informed that you messaged them. ");
    CHECK_SRV("--", "bob:  has been informed that you messaged them. ",
              "irc_717,irc_numeric,nick_server,log3");

    /* open private buffer */
    RECV(":bob!user@host PRIVMSG alice :hi Alice!");
    RECV(":server 717 alice bob : \002has been informed that you messaged them. ");
    CHECK_PV("bob", "--",
             "bob:  has been informed that you messaged them. ",
             "irc_717,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "[#test] nick1!user1@host1 quieted",
               "irc_728,irc_numeric,nick_server,log3");
    RECV(":server 728 alice #test q nick1!user1@host1 alice!user@host");
    CHECK_CHAN("--", "[#test] nick1!user1@host1 quieted by alice (user@host)",
               "irc_728,irc_numeric,nick_server,log3");
    RECV(":server 728 alice #test q nick1!user1@host1 alice!user@host "
         "1351350090 ");
    CHECK_CHAN("--",
               "[#test] nick1!user1@host1 quieted by alice (user@host) "
               "on Sat, 27 Oct 2012 15:01:30",
               "irc_728,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 728 alice #xyz q nick1!user1@host1");
    CHECK_SRV("--", "[#xyz] nick1!user1@host1 quieted",
              "irc_728,irc_numeric,nick_server,log3");
    RECV(":server 728 alice #xyz q nick1!user1@host1 alice!user@host");
    CHECK_SRV("--", "[#xyz] nick1!user1@host1 quieted by alice (user@host)",
              "irc_728,irc_numeric,nick_server,log3");
    RECV(":server 728 alice #xyz q nick1!user1@host1 alice!user@host "
         "1351350090 ");
    CHECK_SRV("--", "[#xyz] nick1!user1@host1 quieted by alice (user@host) "
              "on Sat, 27 Oct 2012 15:01:30",
              "irc_728,irc_numeric,nick_server,log3");
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
    CHECK_CHAN("--", "[#test]", "irc_729,irc_numeric,nick_server,log3");
    RECV(":server 729 alice #test q end");
    CHECK_CHAN("--", "[#test] end", "irc_729,irc_numeric,nick_server,log3");
    RECV(":server 729 alice #test q : \002End of Channel Quiet List ");
    CHECK_CHAN("--", "[#test]  End of Channel Quiet List ",
               "irc_729,irc_numeric,nick_server,log3");

    /* channel not found */
    RECV(":server 729 alice #xyz q");
    CHECK_SRV("--", "[#xyz]", "irc_729,irc_numeric,nick_server,log3");
    RECV(":server 729 alice #xyz q end");
    CHECK_SRV("--", "[#xyz] end", "irc_729,irc_numeric,nick_server,log3");
    RECV(":server 729 alice #xyz q : \002End of Channel Quiet List ");
    CHECK_SRV("--", "[#xyz]  End of Channel Quiet List ",
              "irc_729,irc_numeric,nick_server,log3");
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

    /* without notify */
    RECV(":server 730 alice : nick1!user1@host1,nick2!user2@host2 ");
    CHECK_SRV("--", "notify: nick1 (user1@host1) is connected",
              "irc_notify,irc_notify_join,nick_nick1,notify_message,log3");
    CHECK_SRV("--", "notify: nick2 (user2@host2) is connected",
              "irc_notify,irc_notify_join,nick_nick2,notify_message,log3");
    RECV(":server 730 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("--", "notify: nick1 (user1@host1) is connected",
              "irc_notify,irc_notify_join,nick_nick1,notify_message,log3");
    CHECK_SRV("--", "notify: nick2 (user2@host2) is connected",
              "irc_notify,irc_notify_join,nick_nick2,notify_message,log3");

    RECV(":server 731 alice : nick1!user1@host1,nick2!user2@host2 ");
    CHECK_SRV("--", "notify: nick1 (user1@host1) is offline",
              "irc_notify,irc_notify_quit,nick_nick1,notify_message,log3");
    CHECK_SRV("--", "notify: nick2 (user2@host2) is offline",
              "irc_notify,irc_notify_quit,nick_nick2,notify_message,log3");
    RECV(":server 731 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("--", "notify: nick1 (user1@host1) is offline",
              "irc_notify,irc_notify_quit,nick_nick1,notify_message,log3");
    CHECK_SRV("--", "notify: nick2 (user2@host2) is offline",
              "irc_notify,irc_notify_quit,nick_nick2,notify_message,log3");

    /* with notify on nick1 */
    run_cmd_quiet ("/notify add nick1 " IRC_FAKE_SERVER);

    RECV(":server 730 alice : nick1!user1@host1,nick2!user2@host2 ");
    CHECK_SRV("--", "notify: nick1 (user1@host1) is connected",
              "irc_notify,irc_notify_join,nick_nick1,notify_message,log3");
    CHECK_SRV("--", "notify: nick2 (user2@host2) is connected",
              "irc_notify,irc_notify_join,nick_nick2,notify_message,log3");
    RECV(":server 730 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("--", "notify: nick2 (user2@host2) is connected",
              "irc_notify,irc_notify_join,nick_nick2,notify_message,log3");

    RECV(":server 731 alice : nick1!user1@host1,nick2!user2@host2 ");
    CHECK_SRV("--", "notify: nick1 (user1@host1) has quit",
              "irc_notify,irc_notify_quit,nick_nick1,notify_message,log3");
    CHECK_SRV("--", "notify: nick2 (user2@host2) is offline",
              "irc_notify,irc_notify_quit,nick_nick2,notify_message,log3");
    RECV(":server 731 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("--", "notify: nick2 (user2@host2) is offline",
              "irc_notify,irc_notify_quit,nick_nick2,notify_message,log3");

    RECV(":server 730 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("--", "notify: nick1 (user1@host1) has connected",
              "irc_notify,irc_notify_join,nick_nick1,notify_message,log3");
    CHECK_SRV("--", "notify: nick2 (user2@host2) is connected",
              "irc_notify,irc_notify_join,nick_nick2,notify_message,log3");

    run_cmd_quiet ("/mute /notify del nick1 " IRC_FAKE_SERVER);

    /* with notify on nick1 and nick2 */
    run_cmd_quiet ("/notify add nick1 " IRC_FAKE_SERVER);
    run_cmd_quiet ("/notify add nick2 " IRC_FAKE_SERVER);

    RECV(":server 730 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("--", "notify: nick1 (user1@host1) is connected",
              "irc_notify,irc_notify_join,nick_nick1,notify_message,log3");
    CHECK_SRV("--", "notify: nick2 (user2@host2) is connected",
              "irc_notify,irc_notify_join,nick_nick2,notify_message,log3");

    RECV(":server 731 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("--", "notify: nick1 (user1@host1) has quit",
              "irc_notify,irc_notify_quit,nick_nick1,notify_message,log3");
    CHECK_SRV("--", "notify: nick2 (user2@host2) has quit",
              "irc_notify,irc_notify_quit,nick_nick2,notify_message,log3");

    RECV(":server 730 alice :nick1!user1@host1,nick2!user2@host2");
    CHECK_SRV("--", "notify: nick1 (user1@host1) has connected",
              "irc_notify,irc_notify_join,nick_nick1,notify_message,log3");
    CHECK_SRV("--", "notify: nick2 (user2@host2) has connected",
              "irc_notify,irc_notify_join,nick_nick2,notify_message,log3");

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
    CHECK_SRV("--", "", "irc_732,irc_numeric,nick_server,log3");
    RECV(":server 732 alice : nick1!user1@host1,nick2!user2@host2 ");
    CHECK_SRV("--", "nick1!user1@host1,nick2!user2@host2",
              "irc_732,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "", "irc_733,irc_numeric,nick_server,log3");
    RECV(":server 733 alice end");
    CHECK_SRV("--", "end", "irc_733,irc_numeric,nick_server,log3");
    RECV(":server 733 alice : End of MONITOR list ");
    CHECK_SRV("--", " End of MONITOR list ", "irc_733,irc_numeric,nick_server,log3");
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
    CHECK_SRV("=!=", " (10)", "irc_734,irc_numeric,nick_server,log3");
    RECV(":server 734 alice 10 nick1,nick2 full");
    CHECK_SRV("=!=", "full (10)", "irc_734,irc_numeric,nick_server,log3");
    RECV(":server 734 alice 10 nick1,nick2 : \002Monitor list is full ");
    CHECK_SRV("=!=", " Monitor list is full  (10)",
              "irc_734,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_742 (mode cannot be set)
 */

TEST(IrcProtocolWithServer, 742)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 742");
    CHECK_ERROR_PARAMS("742", 0, 2);
    RECV(":server 742 alice");
    CHECK_ERROR_PARAMS("742", 1, 2);

    RECV(":server 742 alice #test");
    CHECK_SRV("--", "#test", "irc_742,irc_numeric,nick_server,log3");
    RECV(":server 742 alice #test n nstlk : \002MODE cannot be set due to channel "
         "having an active MLOCK restriction policy ");
    CHECK_CHAN("--",
               "#test: n nstlk  MODE cannot be set due to channel having "
               "an active MLOCK restriction policy ",
               "irc_742,irc_numeric,nick_server,log3");
    RECV(":server 742 alice #test2 n nstlk :\002MODE cannot be set due to channel "
         "having an active MLOCK restriction policy");
    CHECK_SRV("--",
              "#test2: n nstlk MODE cannot be set due to channel having "
               "an active MLOCK restriction policy",
              "irc_742,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "logged (alice!user@host)",
              "irc_900,irc_numeric,nick_server,log3");
    RECV(":server 900 alice alice!user@host alice : "
         "\002You are now logged in as mynick ");
    CHECK_SRV("--", " You are now logged in as mynick  (alice!user@host)",
              "irc_900,irc_numeric,nick_server,log3");
    RECV(":server 900 * * alice : \002You are now logged in as mynick ");
    CHECK_SRV("--", " You are now logged in as mynick ",
              "irc_900,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "logged", "irc_901,irc_numeric,nick_server,log3");
    RECV(":server 901 alice nick!user@host : \002You are now logged out ");
    CHECK_SRV("--", " You are now logged out ",
              "irc_901,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "ok", "irc_903,irc_numeric,nick_server,log3");
    RECV(":server 903 alice : \002SASL authentication successful ");
    CHECK_SRV("--", " SASL authentication successful ",
              "irc_903,irc_numeric,nick_server,log3");
    RECV(":server 903 * : SASL authentication successful ");
    CHECK_SRV("--", " SASL authentication successful ",
              "irc_903,irc_numeric,nick_server,log3");


    RECV(":server 907 alice ok");
    CHECK_SRV("--", "ok", "irc_907,irc_numeric,nick_server,log3");
    RECV(":server 907 alice : \002SASL authentication successful ");
    CHECK_SRV("--", " SASL authentication successful ",
              "irc_907,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "error", "irc_902,irc_numeric,nick_server,log3");
    RECV(":server 902 alice : \002SASL authentication failed ");
    CHECK_SRV("--", " SASL authentication failed ",
              "irc_902,irc_numeric,nick_server,log3");

    RECV(":server 904 alice error");
    CHECK_SRV("--", "error", "irc_904,irc_numeric,nick_server,log3");
    RECV(":server 904 alice : \002SASL authentication failed ");
    CHECK_SRV("--", " SASL authentication failed ",
              "irc_904,irc_numeric,nick_server,log3");

    RECV(":server 905 alice error");
    CHECK_SRV("--", "error", "irc_905,irc_numeric,nick_server,log3");
    RECV(":server 905 alice : \002SASL authentication failed ");
    CHECK_SRV("--", " SASL authentication failed ",
              "irc_905,irc_numeric,nick_server,log3");

    RECV(":server 906 alice error");
    CHECK_SRV("--", "error", "irc_906,irc_numeric,nick_server,log3");
    RECV(":server 906 alice : \002SASL authentication failed ");
    CHECK_SRV("--", " SASL authentication failed ",
              "irc_906,irc_numeric,nick_server,log3");
}

/*
 * Tests functions:
 *   irc_protocol_cb_936 (censored word)
 */

TEST(IrcProtocolWithServer, 936)
{
    SRV_INIT_JOIN;

    /* not enough parameters */
    RECV(":server 936");
    CHECK_ERROR_PARAMS("936", 0, 2);
    RECV(":server 936 alice");
    CHECK_ERROR_PARAMS("936", 1, 2);

    RECV(":server 936 alice #test");
    CHECK_SRV("--", "#test", "irc_936,irc_numeric,nick_server,log3");
    RECV(":server 936 alice #test CENSORED_WORD "
         ": \002Your message contained a censored word, and was blocked ");
    CHECK_CHAN("--",
               "#test: CENSORED_WORD  Your message contained a censored word, "
               "and was blocked ",
               "irc_936,irc_numeric,nick_server,log3");
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
    CHECK_SRV("--", "mode", "irc_973,irc_numeric,nick_server,log3");
    RECV(":server 973 alice mode test");
    CHECK_SRV("--", "mode: test", "irc_973,irc_numeric,nick_server,log3");
    RECV(":server 973 alice mode : test ");
    CHECK_SRV("--", "mode:  test ", "irc_973,irc_numeric,nick_server,log3");

    RECV(":server 974 alice");
    CHECK_NO_MSG;
    RECV(":server 974 alice mode");
    CHECK_SRV("--", "mode", "irc_974,irc_numeric,nick_server,log3");
    RECV(":server 974 alice mode test");
    CHECK_SRV("--", "mode: test", "irc_974,irc_numeric,nick_server,log3");
    RECV(":server 974 alice mode : test ");
    CHECK_SRV("--", "mode:  test ", "irc_974,irc_numeric,nick_server,log3");

    RECV(":server 975 alice");
    CHECK_NO_MSG;
    RECV(":server 975 alice mode");
    CHECK_SRV("--", "mode", "irc_975,irc_numeric,nick_server,log3");
    RECV(":server 975 alice mode test");
    CHECK_SRV("--", "mode: test", "irc_975,irc_numeric,nick_server,log3");
    RECV(":server 975 alice mode : test ");
    CHECK_SRV("--", "mode:  test ", "irc_975,irc_numeric,nick_server,log3");

    RECV(":server 973 bob");
    CHECK_SRV("--", "bob", "irc_973,irc_numeric,nick_server,log3");
    RECV(":server 973 bob mode");
    CHECK_SRV("--", "bob: mode", "irc_973,irc_numeric,nick_server,log3");
    RECV(":server 973 bob mode test");
    CHECK_SRV("--", "bob: mode test", "irc_973,irc_numeric,nick_server,log3");
    RECV(":server 973 bob mode : test ");
    CHECK_SRV("--", "bob: mode  test ", "irc_973,irc_numeric,nick_server,log3");

    RECV(":server 974 bob");
    CHECK_SRV("--", "bob", "irc_974,irc_numeric,nick_server,log3");
    RECV(":server 974 bob mode");
    CHECK_SRV("--", "bob: mode", "irc_974,irc_numeric,nick_server,log3");
    RECV(":server 974 bob mode test");
    CHECK_SRV("--", "bob: mode test", "irc_974,irc_numeric,nick_server,log3");
    RECV(":server 974 bob mode : test ");
    CHECK_SRV("--", "bob: mode  test ", "irc_974,irc_numeric,nick_server,log3");

    RECV(":server 975 bob");
    CHECK_SRV("--", "bob", "irc_975,irc_numeric,nick_server,log3");
    RECV(":server 975 bob mode");
    CHECK_SRV("--", "bob: mode", "irc_975,irc_numeric,nick_server,log3");
    RECV(":server 975 bob mode test");
    CHECK_SRV("--", "bob: mode test", "irc_975,irc_numeric,nick_server,log3");
    RECV(":server 975 bob mode : test ");
    CHECK_SRV("--", "bob: mode  test ", "irc_975,irc_numeric,nick_server,log3");
}
