/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC raw functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/core/core-config-file.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-line.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-raw.h"
#include "src/plugins/irc/irc-server.h"

extern char *irc_raw_filter;
extern int irc_raw_message_string_match (const char *string, const char *mask);
extern int irc_raw_message_match_filter (struct t_irc_raw_message *raw_message,
                                         const char *filter);
extern void irc_raw_message_print (struct t_irc_raw_message *raw_message);
extern void irc_raw_message_free (struct t_irc_raw_message *raw_message);
extern void irc_raw_message_free_all (void);
extern void irc_raw_message_remove_old (void);
}

TEST_GROUP(IrcRaw)
{
};

TEST_GROUP(IrcRawWithServer)
{
    struct t_irc_server *server;
    int old_debug;

    void setup ()
    {
        server = irc_server_alloc ("server_raw");
        CHECK(server);
        irc_raw_message_free_all ();
        old_debug = weechat_irc_plugin->debug;
    }

    void teardown ()
    {
        weechat_irc_plugin->debug = old_debug;
        if (irc_raw_buffer)
            gui_buffer_close (irc_raw_buffer);
        irc_raw_set_filter (NULL);
        irc_raw_message_free_all ();
        irc_server_free (server);
        config_file_option_reset (irc_config_look_raw_messages, 1);
    }

    /* Add a raw message for the test server. */
    struct t_irc_raw_message *add (int flags, const char *message)
    {
        struct t_irc_raw_message *raw_message;

        raw_message = irc_raw_message_add_to_list (1700000000, 123456, server,
                                                   flags, message);
        CHECK(raw_message);
        return raw_message;
    }

    /* Return the prefix + message (without colors) of last line in raw buffer. */
    char *last_line ()
    {
        struct t_gui_line *ptr_line;
        char *prefix, *message, *result;

        ptr_line = irc_raw_buffer->own_lines->last_line;
        if (!ptr_line)
            return NULL;
        prefix = gui_color_decode (ptr_line->data->prefix, NULL);
        message = gui_color_decode (ptr_line->data->message, NULL);
        string_asprintf (&result, "%s|%s", prefix, message);
        free (prefix);
        free (message);
        return result;
    }
};

/*
 * Test functions:
 *   irc_raw_message_string_match
 */

TEST(IrcRaw, StringMatch)
{
    /* Without "*": case-insensitive search of mask in string. */
    LONGS_EQUAL(1, irc_raw_message_string_match ("PRIVMSG #test :hello", "hello"));
    LONGS_EQUAL(1, irc_raw_message_string_match ("PRIVMSG #test :hello", "HELLO"));
    LONGS_EQUAL(0, irc_raw_message_string_match ("PRIVMSG #test :hello", "xyz"));

    /* With "*": match of whole string. */
    LONGS_EQUAL(1, irc_raw_message_string_match ("PRIVMSG #test :hello", "PRIVMSG*"));
    LONGS_EQUAL(1, irc_raw_message_string_match ("PRIVMSG #test :hello", "*hello"));
    LONGS_EQUAL(0, irc_raw_message_string_match ("PRIVMSG #test :hello", "hello*"));
}

/*
 * Test functions:
 *   irc_raw_message_match_filter
 */

TEST(IrcRawWithServer, MatchFilter)
{
    struct t_irc_raw_message *msg_recv, *msg_sent, *msg_redir;

    msg_recv = add (IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_MODIFIED,
                    ":alice!user@host PRIVMSG #test :hello");
    msg_sent = add (IRC_RAW_FLAG_SEND, "JOIN #weechat");
    msg_redir = add (IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_REDIRECT,
                     ":server 303 bob :carol");

    /* No filter. */
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_recv, NULL));
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_recv, ""));

    /* Condition. */
    LONGS_EQUAL(1, irc_raw_message_match_filter (
                    msg_recv,
                    "c:${recv} == 1 && ${sent} == 0 && ${modified} == 1 "
                    "&& ${redirected} == 0 && ${command} == PRIVMSG "
                    "&& ${nick} == alice && ${server} == server_raw"));
    LONGS_EQUAL(1, irc_raw_message_match_filter (
                    msg_recv, "c:${date} =- 2023-11-14T"));
    LONGS_EQUAL(1, irc_raw_message_match_filter (
                    msg_redir, "c:${redirected} == 1"));
    LONGS_EQUAL(0, irc_raw_message_match_filter (msg_sent, "c:${recv} == 1"));
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_sent, "c:${sent} == 1"));

    /* Server. */
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_recv, "s:server_raw"));
    LONGS_EQUAL(0, irc_raw_message_match_filter (msg_recv, "s:other"));

    /* Flags. */
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_recv, "f:recv"));
    LONGS_EQUAL(0, irc_raw_message_match_filter (msg_sent, "f:recv"));
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_sent, "f:sent"));
    LONGS_EQUAL(0, irc_raw_message_match_filter (msg_recv, "f:sent"));
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_recv, "f:modified"));
    LONGS_EQUAL(0, irc_raw_message_match_filter (msg_sent, "f:modified"));
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_redir, "f:redirected"));
    LONGS_EQUAL(0, irc_raw_message_match_filter (msg_recv, "f:redirected"));
    LONGS_EQUAL(0, irc_raw_message_match_filter (msg_recv, "f:xxx"));

    /* IRC command. */
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_recv, "m:privmsg"));
    LONGS_EQUAL(0, irc_raw_message_match_filter (msg_recv, "m:join"));
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_sent, "m:JOIN"));
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_redir, "m:303"));

    /* Text in message. */
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_recv, "hello"));
    LONGS_EQUAL(1, irc_raw_message_match_filter (msg_recv, "*hello"));
    LONGS_EQUAL(0, irc_raw_message_match_filter (msg_recv, "xyz"));
}

/*
 * Test functions:
 *   irc_raw_message_add_to_list
 *   irc_raw_message_free
 *   irc_raw_message_free_all
 *   irc_raw_message_free_server
 *   irc_raw_message_remove_old
 */

TEST(IrcRawWithServer, AddFree)
{
    struct t_irc_raw_message *msg1, *msg2, *msg3;

    POINTERS_EQUAL(NULL, irc_raw_message_add_to_list (0, 0, server, 0, NULL));
    LONGS_EQUAL(0, irc_raw_messages_count);

    msg1 = add (IRC_RAW_FLAG_RECV, "msg1");
    LONGS_EQUAL(1700000000, msg1->date);
    LONGS_EQUAL(123456, msg1->date_usec);
    POINTERS_EQUAL(server, msg1->server);
    LONGS_EQUAL(IRC_RAW_FLAG_RECV, msg1->flags);
    STRCMP_EQUAL("msg1", msg1->message);
    msg2 = add (IRC_RAW_FLAG_SEND, "msg2");
    msg3 = add (IRC_RAW_FLAG_SEND, "msg3");
    LONGS_EQUAL(3, irc_raw_messages_count);
    POINTERS_EQUAL(msg1, irc_raw_messages);
    POINTERS_EQUAL(msg3, last_irc_raw_message);

    /* Free message in the middle, then first, then last. */
    irc_raw_message_free (NULL);
    irc_raw_message_free (msg2);
    POINTERS_EQUAL(msg3, msg1->next_message);
    POINTERS_EQUAL(msg1, msg3->prev_message);
    irc_raw_message_free (msg1);
    POINTERS_EQUAL(msg3, irc_raw_messages);
    POINTERS_EQUAL(NULL, msg3->prev_message);
    irc_raw_message_free (msg3);
    POINTERS_EQUAL(NULL, irc_raw_messages);
    POINTERS_EQUAL(NULL, last_irc_raw_message);
    LONGS_EQUAL(0, irc_raw_messages_count);

    /* Limit on number of messages: oldest messages are removed. */
    config_file_option_set (irc_config_look_raw_messages, "2", 1);
    add (IRC_RAW_FLAG_RECV, "msg1");
    add (IRC_RAW_FLAG_RECV, "msg2");
    add (IRC_RAW_FLAG_RECV, "msg3");
    LONGS_EQUAL(2, irc_raw_messages_count);
    STRCMP_EQUAL("msg2", irc_raw_messages->message);
    STRCMP_EQUAL("msg3", last_irc_raw_message->message);
    config_file_option_set (irc_config_look_raw_messages, "1", 1);
    irc_raw_message_remove_old ();
    LONGS_EQUAL(0, irc_raw_messages_count);

    irc_raw_message_free_all ();
    LONGS_EQUAL(0, irc_raw_messages_count);
}

/*
 * Test functions:
 *   irc_raw_open
 *   irc_raw_set_localvar_filter
 *   irc_raw_set_title
 *   irc_raw_print
 *   irc_raw_message_print
 */

TEST(IrcRawWithServer, OpenPrint)
{
    char *str;

    /* Buffer not opened: message only added to list. */
    irc_raw_print (server, IRC_RAW_FLAG_RECV, NULL);
    LONGS_EQUAL(0, irc_raw_messages_count);
    irc_raw_print (server, IRC_RAW_FLAG_RECV, "PING :before");
    LONGS_EQUAL(1, irc_raw_messages_count);
    irc_raw_message_print (NULL);

    /* Open buffer: existing messages are displayed. */
    irc_raw_open (0);
    CHECK(irc_raw_buffer);
    STRCMP_EQUAL("irc.irc_raw", irc_raw_buffer->full_name);
    STRCMP_EQUAL("*", gui_buffer_get_string (irc_raw_buffer,
                                             "localvar_filter"));
    STRCMP_EQUAL("IRC raw messages | Filter: *", irc_raw_buffer->title);
    WEE_TEST_STR("--> server_raw|PING :before", last_line ());

    /* Already opened: same buffer. */
    irc_raw_open (1);
    STRCMP_EQUAL("irc.irc_raw", irc_raw_buffer->full_name);

    /* Prefix according to flags. */
    irc_raw_print (server, IRC_RAW_FLAG_SEND, "PONG :a");
    WEE_TEST_STR("<-- server_raw|PONG :a", last_line ());
    irc_raw_print (server, IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_MODIFIED, "msg");
    WEE_TEST_STR("==> server_raw|msg", last_line ());
    irc_raw_print (server, IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_REDIRECT, "msg");
    WEE_TEST_STR("R>> server_raw|msg", last_line ());
    irc_raw_print (server, IRC_RAW_FLAG_SEND | IRC_RAW_FLAG_MODIFIED, "msg");
    WEE_TEST_STR("<== server_raw|msg", last_line ());
    irc_raw_print (server,
                   IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_MODIFIED
                   | IRC_RAW_FLAG_REDIRECT, "msg");
    WEE_TEST_STR("--> server_raw|msg", last_line ());
    irc_raw_print (server,
                   IRC_RAW_FLAG_SEND | IRC_RAW_FLAG_REDIRECT, "msg");
    WEE_TEST_STR("<-- server_raw|msg", last_line ());

    /* Control chars, backslash and invalid UTF-8 are escaped. */
    irc_raw_print (server, IRC_RAW_FLAG_RECV,
                   "a\x01" "b\\c\xff" "d é");
    WEE_TEST_STR("--> server_raw|a\\x01b\\\\c\\xFFd é", last_line ());

    /* Debug >= 2: hexadecimal dump of message is added. */
    weechat_irc_plugin->debug = 2;
    irc_raw_print (server, IRC_RAW_FLAG_RECV, "AB");
    str = last_line ();
    CHECK(str);
    CHECK(strncmp (str, "|  > 41 42 ", 11) == 0);
    free (str);
    LONGS_EQUAL(IRC_RAW_FLAG_RECV | IRC_RAW_FLAG_BINARY,
                last_irc_raw_message->flags);
    weechat_irc_plugin->debug = 0;

    /* No message kept: message displayed then freed. */
    config_file_option_set (irc_config_look_raw_messages, "0", 1);
    irc_raw_message_free_all ();
    irc_raw_print (server, IRC_RAW_FLAG_RECV, "PING :not kept");
    WEE_TEST_STR("--> server_raw|PING :not kept", last_line ());
    LONGS_EQUAL(0, irc_raw_messages_count);
}

TEST(IrcRawWithServer, OpenDebug)
{
    /* Debug >= 1: buffer is opened on first message. */
    POINTERS_EQUAL(NULL, irc_raw_buffer);
    weechat_irc_plugin->debug = 1;
    irc_raw_print (server, IRC_RAW_FLAG_RECV, "PING :debug");
    CHECK(irc_raw_buffer);
    LONGS_EQUAL(1, irc_raw_buffer->own_lines->lines_count);

    /* Buffer closed. */
    gui_buffer_close (irc_raw_buffer);
    POINTERS_EQUAL(NULL, irc_raw_buffer);
}

/*
 * Test functions:
 *   irc_raw_set_filter
 *   irc_raw_filter_options
 *   irc_raw_refresh
 */

TEST(IrcRawWithServer, Filter)
{
    char *str;

    add (IRC_RAW_FLAG_RECV, ":alice!user@host PRIVMSG #test :hello");
    add (IRC_RAW_FLAG_SEND, "JOIN #weechat");
    add (IRC_RAW_FLAG_RECV, "PING :server");

    /* Without buffer: filter is set. */
    irc_raw_set_filter ("f:sent");
    STRCMP_EQUAL("f:sent", irc_raw_filter);
    irc_raw_refresh (1);

    irc_raw_open (0);
    LONGS_EQUAL(1, irc_raw_buffer->own_lines->lines_count);
    WEE_TEST_STR("<-- server_raw|JOIN #weechat", last_line ());

    irc_raw_filter_options ("m:ping");
    STRCMP_EQUAL("m:ping", irc_raw_filter);
    STRCMP_EQUAL("m:ping", gui_buffer_get_string (irc_raw_buffer,
                                                  "localvar_filter"));
    STRCMP_EQUAL("IRC raw messages | Filter: m:ping", irc_raw_buffer->title);
    LONGS_EQUAL(1, irc_raw_buffer->own_lines->lines_count);
    WEE_TEST_STR("--> server_raw|PING :server", last_line ());

    /* Messages not matching filter are not displayed. */
    irc_raw_print (server, IRC_RAW_FLAG_RECV, "PRIVMSG #test :hi");
    LONGS_EQUAL(1, irc_raw_buffer->own_lines->lines_count);

    /* Filter "*": no filter. */
    irc_raw_filter_options ("*");
    POINTERS_EQUAL(NULL, irc_raw_filter);
    LONGS_EQUAL(4, irc_raw_buffer->own_lines->lines_count);
    STRCMP_EQUAL("IRC raw messages | Filter: *", irc_raw_buffer->title);
}

/*
 * Test functions:
 *   irc_raw_add_to_infolist
 */

TEST(IrcRawWithServer, AddToInfolist)
{
    struct t_infolist *infolist;
    struct t_irc_raw_message *raw_message;

    raw_message = add (IRC_RAW_FLAG_SEND, "PONG :a");

    LONGS_EQUAL(0, irc_raw_add_to_infolist (NULL, raw_message));
    infolist = infolist_new (NULL);
    LONGS_EQUAL(0, irc_raw_add_to_infolist (infolist, NULL));
    LONGS_EQUAL(1, irc_raw_add_to_infolist (infolist, raw_message));
    CHECK(infolist_next (infolist));
    LONGS_EQUAL(1700000000, infolist_time (infolist, "date"));
    LONGS_EQUAL(123456, infolist_integer (infolist, "date_usec"));
    STRCMP_EQUAL("server_raw", infolist_string (infolist, "server"));
    LONGS_EQUAL(IRC_RAW_FLAG_SEND, infolist_integer (infolist, "flags"));
    STRCMP_EQUAL("PONG :a", infolist_string (infolist, "message"));
    infolist_free (infolist);
}
