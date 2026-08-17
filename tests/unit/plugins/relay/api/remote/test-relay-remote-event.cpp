/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test event functions for relay remote */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <limits.h>
#include <cjson/cJSON.h>
#include "src/core/core-config-file.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-hotlist.h"
#include "src/gui/gui-line.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-remote.h"
#include "src/plugins/relay/api/remote/relay-remote-event.h"

extern int relay_remote_event_line_is_already_read (struct t_gui_buffer *ptr_buffer,
                                                    long long line_id);
extern struct t_gui_buffer *relay_remote_event_search_buffer (struct t_relay_remote *remote,
                                                              long long id);
extern char **relay_remote_build_string_tags (cJSON *json_tags,
                                              struct t_gui_buffer *buffer,
                                              long long line_id, int highlight);
extern struct t_gui_line *relay_remote_event_search_line_by_id (struct t_gui_buffer *buffer,
                                                                long long id);
}

#define WEE_CHECK_TAGS(__result, __json_tags, __buffer, __line_id,      \
                       __highlight)                                     \
    json_tags = (__json_tags) ? cJSON_Parse (__json_tags) : NULL;       \
    tags = relay_remote_build_string_tags (json_tags, __buffer,         \
                                           __line_id, __highlight);     \
    CHECK(tags);                                                        \
    STRCMP_EQUAL(__result, *tags);                                      \
    string_dyn_free (tags, 1);                                          \
    cJSON_Delete (json_tags);

TEST_GROUP(RelayRemoteEvent)
{
};

/*
 * Test functions:
 *   relay_remote_event_line_is_already_read
 */

TEST(RelayRemoteEvent, LineIsAlreadyRead)
{
    struct t_gui_buffer *buffer;

    buffer = gui_buffer_new_user ("test_line_already_read",
                                  GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    /* invalid arguments */
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (NULL, -1));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (NULL, 123));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, -1));

    /* local variable not set on buffer */
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 0));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));

    /* invalid local variable */
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "abc");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "123abc");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));

    /* nothing read on the remote */
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "-1");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 0));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));

    /* lines up to id 123 read on the remote */
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "123");
    LONGS_EQUAL(1, relay_remote_event_line_is_already_read (buffer, 0));
    LONGS_EQUAL(1, relay_remote_event_line_is_already_read (buffer, 122));
    LONGS_EQUAL(1, relay_remote_event_line_is_already_read (buffer, 123));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 124));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, INT_MAX));

    gui_buffer_close (buffer);

    /*
     * buffer with free content: the identifier of a line is its number ("y"),
     * so a line is never considered as read
     */
    buffer = gui_buffer_new_user ("test_line_is_already_read_free",
                                  GUI_BUFFER_TYPE_FREE);
    CHECK(buffer);

    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "123");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 0));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 122));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   relay_remote_event_search_buffer
 */

TEST(RelayRemoteEvent, SearchBuffer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_get_buffer_id
 */

TEST(RelayRemoteEvent, GetBufferId)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_build_string_tags
 */

TEST(RelayRemoteEvent, BuildStringTags)
{
    struct t_gui_buffer *buffer;
    cJSON *json_tags;
    char **tags;

    buffer = gui_buffer_new_user ("test_build_string_tags",
                                  GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    /* no buffer: the line can not be read on the remote */
    WEE_CHECK_TAGS("relay_remote_line_id_42", NULL, NULL, 42, 0);
    WEE_CHECK_TAGS("relay_remote_line_id_42", "[]", NULL, 42, 0);
    WEE_CHECK_TAGS("relay_remote_line_id_42", "{}", NULL, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,relay_remote_line_id_42",
                   "[\"irc_privmsg\", 123, null]", NULL, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,nick_alice,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"nick_alice\"]", NULL, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", NULL, 42, 0);

    /* highlight: any "notify_xxx" tag is replaced by "notify_highlight" */
    WEE_CHECK_TAGS("irc_privmsg,notify_highlight,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", NULL, 42, 1);

    /* highlight without any "notify_xxx" tag: the tag is added */
    WEE_CHECK_TAGS("irc_privmsg,notify_highlight,relay_remote_line_id_42",
                   "[\"irc_privmsg\"]", NULL, 42, 1);

    /* buffer without the local variable: the line is not read */
    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 42, 0);

    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "100");

    /* line not read yet on the remote (id greater than last read line id) */
    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_101",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 101, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_highlight,relay_remote_line_id_101",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 101, 1);

    /* negative line id: the line can not be read on the remote */
    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_-1",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, -1, 0);

    /*
     * line already read on the remote: any "notify_xxx" tag is removed and
     * the tag "notify_none" is added
     */
    WEE_CHECK_TAGS("notify_none,relay_remote_line_id_42", NULL, buffer, 42, 0);
    WEE_CHECK_TAGS("notify_none,relay_remote_line_id_42", "[]", buffer, 42, 0);
    WEE_CHECK_TAGS("notify_none,relay_remote_line_id_42",
                   "[\"notify_message\"]", buffer, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_none,relay_remote_line_id_42",
                   "[\"irc_privmsg\"]", buffer, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_none,relay_remote_line_id_100",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 100, 0);

    /* the tag "notify_highlight" is not added when the line is already read */
    WEE_CHECK_TAGS("irc_privmsg,notify_none,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 42, 1);
    WEE_CHECK_TAGS("irc_privmsg,notify_none,relay_remote_line_id_42",
                   "[\"irc_privmsg\"]", buffer, 42, 1);

    /*
     * non-regression tests: no empty tag must be left when a "notify_xxx" tag
     * is removed, wherever it is in the list of tags
     */
    WEE_CHECK_TAGS("irc_privmsg,nick_alice,notify_none,"
                   "relay_remote_line_id_42",
                   "[\"notify_message\", \"irc_privmsg\", \"nick_alice\"]",
                   buffer, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,nick_alice,notify_none,"
                   "relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\", \"nick_alice\"]",
                   buffer, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,nick_alice,notify_none,"
                   "relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"nick_alice\", \"notify_message\"]",
                   buffer, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_none,relay_remote_line_id_42",
                   "[\"notify_message\", \"irc_privmsg\", \"notify_private\"]",
                   buffer, 42, 0);

    gui_buffer_close (buffer);

    /*
     * buffer with free content: the identifier of a line is its number ("y"),
     * so the tags are never changed, even with a local variable set
     */
    buffer = gui_buffer_new_user ("test_build_string_tags_free",
                                  GUI_BUFFER_TYPE_FREE);
    CHECK(buffer);
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "100");

    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_highlight,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 42, 1);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   relay_remote_event_line_add
 */

TEST(RelayRemoteEvent, LineAdd)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_search_line_by_id
 */

TEST(RelayRemoteEvent, SearchLineById)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_line_update
 */

TEST(RelayRemoteEvent, LineUpdate)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_cb_line
 */

TEST(RelayRemoteEvent, CbLine)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_handle_nick
 */

TEST(RelayRemoteEvent, HandleNick)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_handle_nick_group
 */

TEST(RelayRemoteEvent, HandleNickGroup)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_cb_nick_group
 */

TEST(RelayRemoteEvent, CbNickGroup)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_cb_nick
 */

TEST(RelayRemoteEvent, CbNick)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_apply_props
 */

TEST(RelayRemoteEvent, ApplyProps)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_buffer_input_cb
 */

TEST(RelayRemoteEvent, BufferInputCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_check_local_var
 */

TEST(RelayRemoteEvent, CheckLocalVar)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_remove_localvar_cb
 */

TEST(RelayRemoteEvent, RemoveLocalVarCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_initial_sync_buffers
 */

TEST(RelayRemoteEvent, InitialSyncBuffers)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_cb_buffer
 */

TEST(RelayRemoteEvent, CbBuffer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_cb_buffer_cleared
 */

TEST(RelayRemoteEvent, CbBufferCleared)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_cb_buffer_closed
 */

TEST(RelayRemoteEvent, CbBufferClosed)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_cb_vinput
 */

TEST(RelayRemoteEvent, CbInput)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_clear_buffers
 */

TEST(RelayRemoteEvent, ClearBuffers)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_sync_with_remote
 */

TEST(RelayRemoteEvent, SyncWithRemote)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_cb_version
 */

TEST(RelayRemoteEvent, CbVersion)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_cb_upgrade
 */

TEST(RelayRemoteEvent, CbUpgrade)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_cb_quit
 */

TEST(RelayRemoteEvent, CbQuit)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_recv
 */

TEST(RelayRemoteEvent, Recv)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_remote_event_recv
 *
 * Non-regression test: an event received with an array as body calls the
 * callback once per array item, and the callback for event "buffer_closed"
 * closes the buffer, so the buffer must be searched again before each call
 * (otherwise the pointer is used after the buffer has been freed).
 */

TEST(RelayRemoteEvent, RecvBufferClosedArray)
{
    struct t_relay_remote *remote;

    remote = relay_remote_new ("testbufclosed",
                               "https://localhost:9000",
                               NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(remote);

    relay_remote_event_recv (
        remote,
        "{\"code\": 200, "
        "\"body_type\": \"buffer\", "
        "\"body\": {\"id\": 123, \"name\": \"core.weechat\"}}");
    CHECK(relay_remote_event_search_buffer (remote, 123));

    relay_remote_event_recv (
        remote,
        "{\"event_name\": \"buffer_closed\", "
        "\"buffer_id\": 123, "
        "\"body_type\": \"buffer\", "
        "\"body\": [{}, {}]}");
    POINTERS_EQUAL(NULL, relay_remote_event_search_buffer (remote, 123));

    relay_remote_free (remote);
}

/*
 * Test functions:
 *   relay_remote_event_cb_buffer
 *   relay_remote_event_cb_line
 *   relay_remote_event_line_add
 *   relay_remote_event_recv
 *
 * Test that lines already read on the remote are received with the tag
 * "notify_none" and therefore have no notify level.
 */

TEST(RelayRemoteEvent, RecvBufferLinesAlreadyRead)
{
    struct t_relay_remote *remote;
    struct t_gui_buffer *buffer;
    struct t_gui_line *line;

    remote = relay_remote_new ("testlinesread",
                               "https://localhost:9000",
                               NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(remote);

    relay_remote_event_recv (
        remote,
        "{\"code\": 200, "
        "\"body_type\": \"buffer\", "
        "\"body\": {\"id\": 123, \"name\": \"irc.libera.#test\", "
        "\"last_read_line_id\": 5, "
        "\"lines\": ["
        "{\"id\": 4, \"message\": \"line read\", "
        "\"tags\": [\"irc_privmsg\", \"notify_message\"]}, "
        "{\"id\": 5, \"message\": \"line read\", "
        "\"tags\": [\"irc_privmsg\", \"notify_message\"]}, "
        "{\"id\": 6, \"message\": \"line not read\", "
        "\"tags\": [\"irc_privmsg\", \"notify_message\"]}]}}");

    buffer = relay_remote_event_search_buffer (remote, 123);
    CHECK(buffer);
    STRCMP_EQUAL(
        "5",
        gui_buffer_get_string (buffer,
                               "localvar_relay_remote_last_read_line_id"));

    /* lines 4 and 5 have been read on the remote */
    line = relay_remote_event_search_line_by_id (buffer, 4);
    CHECK(line);
    LONGS_EQUAL(3, line->data->tags_count);
    STRCMP_EQUAL("irc_privmsg", line->data->tags_array[0]);
    STRCMP_EQUAL("notify_none", line->data->tags_array[1]);
    STRCMP_EQUAL("relay_remote_line_id_4", line->data->tags_array[2]);
    LONGS_EQUAL(-1, line->data->notify_level);
    line = relay_remote_event_search_line_by_id (buffer, 5);
    CHECK(line);
    LONGS_EQUAL(-1, line->data->notify_level);

    /* line 6 has not been read on the remote */
    line = relay_remote_event_search_line_by_id (buffer, 6);
    CHECK(line);
    LONGS_EQUAL(3, line->data->tags_count);
    STRCMP_EQUAL("irc_privmsg", line->data->tags_array[0]);
    STRCMP_EQUAL("notify_message", line->data->tags_array[1]);
    STRCMP_EQUAL("relay_remote_line_id_6", line->data->tags_array[2]);
    LONGS_EQUAL(GUI_HOTLIST_MESSAGE, line->data->notify_level);

    gui_buffer_close (buffer);
    relay_remote_free (remote);
}
