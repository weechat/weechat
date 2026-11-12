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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "src/core/weechat.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-hotlist.h"
#include "src/gui/gui-key.h"
#include "src/gui/gui-line.h"
#include "src/gui/gui-nicklist.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-config.h"
#include "src/plugins/relay/relay-json.h"
#include "src/plugins/relay/relay-remote.h"
#include "src/plugins/relay/api/relay-api.h"
#include "src/plugins/relay/api/remote/relay-remote-event.h"
#include "src/plugins/relay/api/remote/relay-remote-network.h"

extern int relay_remote_event_line_is_already_read (struct t_gui_buffer *ptr_buffer,
                                                    long long line_id);
extern struct t_gui_buffer *relay_remote_event_search_buffer (struct t_relay_remote *remote,
                                                              long long id);
extern char **relay_remote_build_string_tags (struct t_relay_json *json_tags,
                                              struct t_gui_buffer *buffer,
                                              long long line_id, int highlight);
extern struct t_gui_line *relay_remote_event_search_line_by_id (struct t_gui_buffer *buffer,
                                                                long long id);
extern long long relay_remote_event_get_buffer_id (struct t_gui_buffer *buffer);
extern void relay_remote_event_line_add (struct t_relay_remote_event *event);
extern void relay_remote_event_handle_nick (struct t_gui_buffer *buffer,
                                            struct t_relay_json *json);
extern void relay_remote_event_handle_nick_group (struct t_gui_buffer *buffer,
                                                  struct t_relay_json *json);
extern void relay_remote_event_apply_props (void *data,
                                            struct t_hashtable *hashtable,
                                            const void *key,
                                            const void *value);
extern int relay_remote_event_check_local_var (const char *name);
extern void relay_remote_event_initial_sync_buffers (struct t_relay_remote_event *event);
extern void relay_remote_event_clear_buffers (struct t_relay_remote *remote);
extern void relay_remote_event_sync_with_remote (struct t_relay_remote *remote);
extern void relay_remote_network_close_connection (struct t_relay_remote *remote);
}

#define WEE_CHECK_TAGS(__result, __json_tags, __buffer, __line_id,      \
                       __highlight)                                     \
    json_tags = (__json_tags) ? relay_json_parse (__json_tags) : NULL;  \
    tags = relay_remote_build_string_tags (json_tags, __buffer,         \
                                           __line_id, __highlight);     \
    CHECK(tags);                                                        \
    STRCMP_EQUAL(__result, *tags);                                      \
    string_dyn_free (tags, 1);                                          \
    relay_json_free (json_tags);

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

    /* Invalid arguments */
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (NULL, -1));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (NULL, 123));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, -1));

    /* Local variables not set on buffer */
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 0));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));

    /* Only "last_read_line_id" set on buffer */
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "123");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));

    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_first_line_not_read", "0");

    /* Invalid local variable */
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "abc");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "123abc");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));

    /* Read marker before the first line: nothing read on the remote. */
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "-1");
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_first_line_not_read", "1");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 0));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, INT_MAX));

    /* No read marker: all the lines have been read on the remote. */
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_first_line_not_read", "0");
    LONGS_EQUAL(1, relay_remote_event_line_is_already_read (buffer, 0));
    LONGS_EQUAL(1, relay_remote_event_line_is_already_read (buffer, 123));
    LONGS_EQUAL(1, relay_remote_event_line_is_already_read (buffer, INT_MAX));

    /* Lines up to id 123 read on the remote. */
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "123");
    LONGS_EQUAL(1, relay_remote_event_line_is_already_read (buffer, 0));
    LONGS_EQUAL(1, relay_remote_event_line_is_already_read (buffer, 122));
    LONGS_EQUAL(1, relay_remote_event_line_is_already_read (buffer, 123));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 124));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, INT_MAX));

    gui_buffer_close (buffer);

    /*
     * Buffer with free content: the identifier of a line is its number ("y"),
     * so a line is never considered as read.
     */
    buffer = gui_buffer_new_user ("test_line_is_already_read_free",
                                  GUI_BUFFER_TYPE_FREE);
    CHECK(buffer);

    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "123");
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_first_line_not_read", "0");
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 0));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 122));
    LONGS_EQUAL(0, relay_remote_event_line_is_already_read (buffer, 123));

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   relay_remote_build_string_tags
 */

TEST(RelayRemoteEvent, BuildStringTags)
{
    struct t_gui_buffer *buffer;
    struct t_relay_json *json_tags;
    char **tags;

    buffer = gui_buffer_new_user ("test_build_string_tags",
                                  GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    /* No buffer: the line can not be read on the remote. */
    WEE_CHECK_TAGS("relay_remote_line_id_42", NULL, NULL, 42, 0);
    WEE_CHECK_TAGS("relay_remote_line_id_42", "[]", NULL, 42, 0);
    WEE_CHECK_TAGS("relay_remote_line_id_42", "{}", NULL, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,relay_remote_line_id_42",
                   "[\"irc_privmsg\", 123, null]", NULL, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,nick_alice,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"nick_alice\"]", NULL, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", NULL, 42, 0);

    /* Highlight: any "notify_xxx" tag is replaced by "notify_highlight". */
    WEE_CHECK_TAGS("irc_privmsg,notify_highlight,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", NULL, 42, 1);

    /* Highlight without any "notify_xxx" tag: the tag is added. */
    WEE_CHECK_TAGS("irc_privmsg,notify_highlight,relay_remote_line_id_42",
                   "[\"irc_privmsg\"]", NULL, 42, 1);

    /* Buffer without the local variable: the line is not read. */
    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 42, 0);

    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "100");
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_first_line_not_read", "0");

    /* Line not read yet on the remote (id greater than last read line id). */
    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_101",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 101, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_highlight,relay_remote_line_id_101",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 101, 1);

    /* Negative line id: the line can not be read on the remote. */
    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_-1",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, -1, 0);

    /*
     * Line already read on the remote: any "notify_xxx" tag is removed and
     * the tag "notify_none" is added.
     */
    WEE_CHECK_TAGS("notify_none,relay_remote_line_id_42", NULL, buffer, 42, 0);
    WEE_CHECK_TAGS("notify_none,relay_remote_line_id_42", "[]", buffer, 42, 0);
    WEE_CHECK_TAGS("notify_none,relay_remote_line_id_42",
                   "[\"notify_message\"]", buffer, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_none,relay_remote_line_id_42",
                   "[\"irc_privmsg\"]", buffer, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_none,relay_remote_line_id_100",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 100, 0);

    /* The tag "notify_highlight" is not added when the line is already read. */
    WEE_CHECK_TAGS("irc_privmsg,notify_none,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 42, 1);
    WEE_CHECK_TAGS("irc_privmsg,notify_none,relay_remote_line_id_42",
                   "[\"irc_privmsg\"]", buffer, 42, 1);

    /*
     * Non-regression tests: no empty tag must be left when a "notify_xxx" tag
     * is removed, wherever it is in the list of tags.
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

    /*
     * No read marker on the remote and it is not before the first line: all
     * the lines have been read.
     */
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "-1");
    WEE_CHECK_TAGS("irc_privmsg,notify_none,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 42, 0);

    /* Read marker before the first line on the remote: nothing has been read. */
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_first_line_not_read", "1");
    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 42, 0);

    gui_buffer_close (buffer);

    /*
     * Buffer with free content: the identifier of a line is its number ("y"),
     * so the tags are never changed, even with a local variable set.
     */
    buffer = gui_buffer_new_user ("test_build_string_tags_free",
                                  GUI_BUFFER_TYPE_FREE);
    CHECK(buffer);
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_last_read_line_id", "100");
    gui_buffer_set (buffer,
                    "localvar_set_relay_remote_first_line_not_read", "0");

    WEE_CHECK_TAGS("irc_privmsg,notify_message,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 42, 0);
    WEE_CHECK_TAGS("irc_privmsg,notify_highlight,relay_remote_line_id_42",
                   "[\"irc_privmsg\", \"notify_message\"]", buffer, 42, 1);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   relay_remote_event_cb_buffer
 */

TEST(RelayRemoteEvent, CbBuffer)
{
    struct t_relay_remote *remote;
    struct t_gui_buffer *buffer;

    remote = relay_remote_new ("testcbbuffer",
                               "https://localhost:9000",
                               NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(remote);

    /* The buffer is created with the properties received from the remote. */
    relay_remote_event_recv (
        remote,
        "{\"code\": 200, "
        "\"body_type\": \"buffer\", "
        "\"body\": {\"id\": 123, \"name\": \"irc.libera.#test\", "
        "\"short_name\": \"#test\", "
        "\"number\": 4, "
        "\"type\": \"formatted\", "
        "\"hidden\": false, "
        "\"title\": \"the title\", "
        "\"modes\": \"+nt\", "
        "\"input_prompt\": \"alice\", "
        "\"nicklist\": true, "
        "\"nicklist_case_sensitive\": false, "
        "\"nicklist_display_groups\": true, "
        "\"time_displayed\": true, "
        "\"prefix_displayed\": true, "
        "\"day_change\": false, "
        "\"last_read_line_id\": 5, "
        "\"first_line_not_read\": false, "
        "\"local_variables\": {\"plugin\": \"irc\", "
        "\"name\": \"libera.#test\", \"channel\": \"#test\"}, "
        "\"keys\": [{\"key\": \"meta-A\", \"command\": \"/test\"}]}}");

    buffer = relay_remote_event_search_buffer (remote, 123);
    CHECK(buffer);
    STRCMP_EQUAL("remote.testcbbuffer.irc.libera.#test",
                 gui_buffer_get_string (buffer, "name"));
    STRCMP_EQUAL("#test", gui_buffer_get_string (buffer, "short_name"));
    STRCMP_EQUAL("the title", gui_buffer_get_string (buffer, "title"));
    STRCMP_EQUAL("+nt", gui_buffer_get_string (buffer, "modes"));
    STRCMP_EQUAL("alice", gui_buffer_get_string (buffer, "input_prompt"));
    LONGS_EQUAL(GUI_BUFFER_TYPE_FORMATTED, buffer->type);
    LONGS_EQUAL(0, gui_buffer_get_integer (buffer, "hidden"));
    LONGS_EQUAL(1, gui_buffer_get_integer (buffer, "nicklist"));
    LONGS_EQUAL(0, gui_buffer_get_integer (buffer, "nicklist_case_sensitive"));
    LONGS_EQUAL(1, gui_buffer_get_integer (buffer, "nicklist_display_groups"));
    LONGS_EQUAL(1, gui_buffer_get_integer (buffer, "time_for_each_line"));
    LONGS_EQUAL(1, gui_buffer_get_integer (buffer, "prefix_for_each_line"));
    LONGS_EQUAL(0, gui_buffer_get_integer (buffer, "day_change"));

    /* Extra local variables set for the remote. */
    STRCMP_EQUAL("testcbbuffer",
                 gui_buffer_get_string (buffer, "localvar_relay_remote"));
    STRCMP_EQUAL("123",
                 gui_buffer_get_string (buffer, "localvar_relay_remote_id"));
    STRCMP_EQUAL("4",
                 gui_buffer_get_string (buffer,
                                        "localvar_relay_remote_number"));
    STRCMP_EQUAL("5",
                 gui_buffer_get_string (
                     buffer, "localvar_relay_remote_last_read_line_id"));
    STRCMP_EQUAL("0",
                 gui_buffer_get_string (
                     buffer, "localvar_relay_remote_first_line_not_read"));

    /*
     * The local variables received are set in the buffer, except the reserved
     * ones ("plugin" and "name"), which keep their local value.
     */
    STRCMP_EQUAL("#test", gui_buffer_get_string (buffer, "localvar_channel"));
    STRCMP_EQUAL("relay", gui_buffer_get_string (buffer, "localvar_plugin"));
    STRCMP_EQUAL("remote.testcbbuffer.irc.libera.#test",
                 gui_buffer_get_string (buffer, "localvar_name"));

    /* The keys received are added in the buffer. */
    LONGS_EQUAL(1, buffer->keys_count);
    CHECK(buffer->keys);
    STRCMP_EQUAL("meta-A", buffer->keys->key);
    STRCMP_EQUAL("/test", buffer->keys->command);

    /* The same buffer is updated with the new properties received. */
    relay_remote_event_recv (
        remote,
        "{\"code\": 200, "
        "\"body_type\": \"buffer\", "
        "\"body\": {\"id\": 123, \"name\": \"irc.libera.#test\", "
        "\"short_name\": \"#test2\", "
        "\"title\": \"new title\", "
        "\"time_displayed\": false, "
        "\"prefix_displayed\": false, "
        "\"day_change\": true}}");

    POINTERS_EQUAL(buffer, relay_remote_event_search_buffer (remote, 123));
    STRCMP_EQUAL("#test2", gui_buffer_get_string (buffer, "short_name"));
    STRCMP_EQUAL("new title", gui_buffer_get_string (buffer, "title"));
    LONGS_EQUAL(0, gui_buffer_get_integer (buffer, "time_for_each_line"));
    LONGS_EQUAL(0, gui_buffer_get_integer (buffer, "prefix_for_each_line"));
    LONGS_EQUAL(1, gui_buffer_get_integer (buffer, "day_change"));

    /*
     * The time and the prefix are displayed when the fields "time_displayed"
     * and "prefix_displayed" are not received from the remote.
     */
    relay_remote_event_recv (
        remote,
        "{\"code\": 200, "
        "\"body_type\": \"buffer\", "
        "\"body\": {\"id\": 123, \"name\": \"irc.libera.#test\"}}");

    LONGS_EQUAL(1, gui_buffer_get_integer (buffer, "time_for_each_line"));
    LONGS_EQUAL(1, gui_buffer_get_integer (buffer, "prefix_for_each_line"));

    /*
     * The day change is displayed when the field "day_change" is not
     * received from the remote.
     */
    gui_buffer_set (buffer, "day_change", "0");
    LONGS_EQUAL(0, gui_buffer_get_integer (buffer, "day_change"));
    relay_remote_event_recv (
        remote,
        "{\"code\": 200, "
        "\"body_type\": \"buffer\", "
        "\"body\": {\"id\": 123, \"name\": \"irc.libera.#test\"}}");
    LONGS_EQUAL(1, gui_buffer_get_integer (buffer, "day_change"));

    gui_buffer_close (buffer);

    /* Buffer created without the fields "time_displayed"/"prefix_displayed". */
    relay_remote_event_recv (
        remote,
        "{\"code\": 200, "
        "\"body_type\": \"buffer\", "
        "\"body\": {\"id\": 456, \"name\": \"irc.libera.#test2\", "
        "\"type\": \"formatted\"}}");

    buffer = relay_remote_event_search_buffer (remote, 456);
    CHECK(buffer);
    LONGS_EQUAL(1, gui_buffer_get_integer (buffer, "time_for_each_line"));
    LONGS_EQUAL(1, gui_buffer_get_integer (buffer, "prefix_for_each_line"));

    gui_buffer_close (buffer);

    /*
     * Buffer with free content created without the fields "time_displayed"
     * and "prefix_displayed": neither the time nor the prefix are displayed.
     */
    relay_remote_event_recv (
        remote,
        "{\"code\": 200, "
        "\"body_type\": \"buffer\", "
        "\"body\": {\"id\": 789, \"name\": \"core.free\", "
        "\"type\": \"free\"}}");

    buffer = relay_remote_event_search_buffer (remote, 789);
    CHECK(buffer);
    LONGS_EQUAL(GUI_BUFFER_TYPE_FREE, buffer->type);
    LONGS_EQUAL(0, gui_buffer_get_integer (buffer, "time_for_each_line"));
    LONGS_EQUAL(0, gui_buffer_get_integer (buffer, "prefix_for_each_line"));

    gui_buffer_close (buffer);
    relay_remote_free (remote);
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

    /* Lines 4 and 5 have been read on the remote. */
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

    /* Line 6 has not been read on the remote. */
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

/*
 * Test functions:
 *   relay_remote_event_cb_buffer
 *   relay_remote_event_cb_line
 *   relay_remote_event_line_add
 *   relay_remote_event_recv
 *
 * Test that all the lines of a buffer without read marker are received with the
 * tag "notify_none", unless the marker is before the first line: the remote
 * sends "last_read_line_id" == -1 in both cases, so the field
 * "first_line_not_read" is used to distinguish them.
 */

TEST(RelayRemoteEvent, RecvBufferNoReadMarker)
{
    struct t_relay_remote *remote;
    struct t_gui_buffer *buffer;
    struct t_gui_line *line;

    /* No read marker: all the lines have been read on the remote. */
    remote = relay_remote_new ("testnomarker",
                               "https://localhost:9000",
                               NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(remote);

    relay_remote_event_recv (
        remote,
        "{\"code\": 200, "
        "\"body_type\": \"buffer\", "
        "\"body\": {\"id\": 123, \"name\": \"irc.libera.#test\", "
        "\"last_read_line_id\": -1, "
        "\"first_line_not_read\": false, "
        "\"lines\": ["
        "{\"id\": 4, \"message\": \"line read\", "
        "\"tags\": [\"irc_privmsg\", \"notify_message\"]}]}}");

    buffer = relay_remote_event_search_buffer (remote, 123);
    CHECK(buffer);
    STRCMP_EQUAL(
        "0",
        gui_buffer_get_string (buffer,
                               "localvar_relay_remote_first_line_not_read"));

    line = relay_remote_event_search_line_by_id (buffer, 4);
    CHECK(line);
    LONGS_EQUAL(3, line->data->tags_count);
    STRCMP_EQUAL("irc_privmsg", line->data->tags_array[0]);
    STRCMP_EQUAL("notify_none", line->data->tags_array[1]);
    STRCMP_EQUAL("relay_remote_line_id_4", line->data->tags_array[2]);
    LONGS_EQUAL(-1, line->data->notify_level);

    gui_buffer_close (buffer);
    relay_remote_free (remote);

    /* Read marker before the first line: nothing has been read on the remote. */
    remote = relay_remote_new ("testfirstnotread",
                               "https://localhost:9000",
                               NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(remote);

    relay_remote_event_recv (
        remote,
        "{\"code\": 200, "
        "\"body_type\": \"buffer\", "
        "\"body\": {\"id\": 456, \"name\": \"irc.libera.#test2\", "
        "\"last_read_line_id\": -1, "
        "\"first_line_not_read\": true, "
        "\"lines\": ["
        "{\"id\": 4, \"message\": \"line not read\", "
        "\"tags\": [\"irc_privmsg\", \"notify_message\"]}]}}");

    buffer = relay_remote_event_search_buffer (remote, 456);
    CHECK(buffer);
    STRCMP_EQUAL(
        "1",
        gui_buffer_get_string (buffer,
                               "localvar_relay_remote_first_line_not_read"));

    line = relay_remote_event_search_line_by_id (buffer, 4);
    CHECK(line);
    LONGS_EQUAL(3, line->data->tags_count);
    STRCMP_EQUAL("irc_privmsg", line->data->tags_array[0]);
    STRCMP_EQUAL("notify_message", line->data->tags_array[1]);
    STRCMP_EQUAL("relay_remote_line_id_4", line->data->tags_array[2]);
    LONGS_EQUAL(GUI_HOTLIST_MESSAGE, line->data->notify_level);

    gui_buffer_close (buffer);
    relay_remote_free (remote);
}

TEST_GROUP(RelayRemoteEventWithRemote)
{
    struct t_relay_remote *remote;
    int sock_peer;
    char data_sent[65536];

    /*
     * Receive an event (JSON) from the remote.
     */

    void recv (const char *json)
    {
        relay_remote_event_recv (remote, json);
    }

    /*
     * Create a buffer for the remote, with the given id and name, and
     * optional extra JSON fields (must start with ", ").
     */

    struct t_gui_buffer *create_buffer (long long id, const char *name,
                                        const char *extra)
    {
        char json[8192];

        snprintf (json, sizeof (json),
                  "{\"code\": 200, \"body_type\": \"buffer\", "
                  "\"body\": {\"id\": %lld, \"name\": \"%s\"%s}}",
                  id, name, (extra) ? extra : "");
        recv (json);
        return relay_remote_event_search_buffer (remote, id);
    }

    /*
     * Read all data sent by the remote (on the other end of the socket).
     *
     * Return number of bytes read.
     */

    int peer_recv ()
    {
        int num_read, total;

        total = 0;
        while (total < (int)sizeof (data_sent) - 1)
        {
            num_read = recv_peer (data_sent + total,
                                  sizeof (data_sent) - 1 - total);
            if (num_read <= 0)
                break;
            total += num_read;
        }
        data_sent[total] = '\0';
        return total;
    }

    int recv_peer (char *data, int size)
    {
        return ::recv (sock_peer, data, size, 0);
    }

    /*
     * Close all buffers of the remote.
     */

    void close_buffers ()
    {
        struct t_gui_buffer *ptr_buffer, *ptr_next_buffer;
        const char *ptr_name;

        ptr_buffer = gui_buffers;
        while (ptr_buffer)
        {
            ptr_next_buffer = ptr_buffer->next_buffer;
            ptr_name = gui_buffer_get_string (ptr_buffer,
                                              "localvar_relay_remote");
            if (ptr_name && (strcmp (ptr_name, remote->name) == 0))
                gui_buffer_close (ptr_buffer);
            ptr_buffer = ptr_next_buffer;
        }
    }

    void setup ()
    {
        int sv[2];

        config_file_option_set (relay_config_look_auto_open_buffer, "off", 1);

        remote = relay_remote_new ("testremote", "http://localhost:1",
                                   "off", "0", "", "off", "", "");
        CHECK(remote);

        /* Not connected: data is sent as-is to the socket (no websocket). */
        LONGS_EQUAL(0, socketpair (AF_UNIX, SOCK_STREAM, 0, sv));
        fcntl (sv[0], F_SETFL, fcntl (sv[0], F_GETFL) | O_NONBLOCK);
        fcntl (sv[1], F_SETFL, fcntl (sv[1], F_GETFL) | O_NONBLOCK);
        remote->sock = sv[0];
        sock_peer = sv[1];
    }

    void teardown ()
    {
        close_buffers ();
        relay_remote_network_close_connection (remote);
        relay_remote_free (remote);
        if (sock_peer >= 0)
            close (sock_peer);
        config_file_option_reset (relay_config_look_auto_open_buffer, 1);
    }
};

/*
 * Test functions:
 *   relay_remote_event_search_buffer
 */

TEST(RelayRemoteEventWithRemote, SearchBuffer)
{
    struct t_gui_buffer *buffer;
    struct t_relay_remote *remote2;

    POINTERS_EQUAL(NULL, relay_remote_event_search_buffer (NULL, 123));
    POINTERS_EQUAL(NULL, relay_remote_event_search_buffer (remote, -1));
    POINTERS_EQUAL(NULL, relay_remote_event_search_buffer (remote, 123));

    buffer = create_buffer (123, "irc.libera.#test", NULL);
    CHECK(buffer);
    POINTERS_EQUAL(buffer, relay_remote_event_search_buffer (remote, 123));
    POINTERS_EQUAL(NULL, relay_remote_event_search_buffer (remote, 12));
    POINTERS_EQUAL(NULL, relay_remote_event_search_buffer (remote, 1234));

    /* Same id on another remote: not found. */
    remote2 = relay_remote_new ("testremote2", "http://localhost:1",
                                NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(remote2);
    POINTERS_EQUAL(NULL, relay_remote_event_search_buffer (remote2, 123));
    relay_remote_free (remote2);
}

/*
 * Test functions:
 *   relay_remote_event_get_buffer_id
 */

TEST(RelayRemoteEventWithRemote, GetBufferId)
{
    struct t_gui_buffer *buffer;

    LONGS_EQUAL(-1, relay_remote_event_get_buffer_id (NULL));
    LONGS_EQUAL(-1, relay_remote_event_get_buffer_id (gui_buffers));

    buffer = create_buffer (1730000000123456LL, "irc.libera.#test", NULL);
    CHECK(buffer);
    CHECK(1730000000123456LL == relay_remote_event_get_buffer_id (buffer));

    gui_buffer_set (buffer, "localvar_set_relay_remote_id", "abc");
    LONGS_EQUAL(-1, relay_remote_event_get_buffer_id (buffer));
}

/*
 * Test functions:
 *   relay_remote_event_cb_line
 *   relay_remote_event_line_add
 *   relay_remote_event_search_line_by_id
 *   relay_remote_event_line_update
 */

TEST(RelayRemoteEventWithRemote, CbLine)
{
    struct t_gui_buffer *buffer;
    struct t_gui_line *line;

    /* Read marker on remote: lines with id > 0 have not been read. */
    buffer = create_buffer (123, "irc.libera.#test",
                            ", \"last_read_line_id\": 0");
    CHECK(buffer);

    POINTERS_EQUAL(NULL, relay_remote_event_search_line_by_id (NULL, 1));
    POINTERS_EQUAL(NULL, relay_remote_event_search_line_by_id (buffer, 1));

    /* Line added. */
    recv ("{\"event_name\": \"buffer_line_added\", \"buffer_id\": 123, "
          "\"body_type\": \"line\", "
          "\"body\": {\"id\": 1, \"y\": -1, "
          "\"date\": \"2026-01-02T03:04:05.123456Z\", "
          "\"prefix\": \"alice\", \"message\": \"hello\", "
          "\"highlight\": false, "
          "\"tags\": [\"irc_privmsg\", \"notify_message\"]}}");
    line = relay_remote_event_search_line_by_id (buffer, 1);
    CHECK(line);
    POINTERS_EQUAL(buffer->own_lines->last_line, line);
    STRCMP_EQUAL("alice", line->data->prefix);
    STRCMP_EQUAL("hello", line->data->message);
    LONGS_EQUAL(1767323045, line->data->date);
    LONGS_EQUAL(123456, line->data->date_usec);
    LONGS_EQUAL(3, line->data->tags_count);
    STRCMP_EQUAL("irc_privmsg", line->data->tags_array[0]);
    STRCMP_EQUAL("notify_message", line->data->tags_array[1]);
    STRCMP_EQUAL("relay_remote_line_id_1", line->data->tags_array[2]);

    /* Line with highlight and without prefix. */
    recv ("{\"event_name\": \"buffer_line_added\", \"buffer_id\": 123, "
          "\"body_type\": \"line\", "
          "\"body\": {\"id\": 2, \"message\": \"hello alice\", "
          "\"highlight\": true, \"tags\": [\"notify_message\"]}}");
    line = relay_remote_event_search_line_by_id (buffer, 2);
    CHECK(line);
    STRCMP_EQUAL("", line->data->prefix);
    STRCMP_EQUAL("hello alice", line->data->message);
    STRCMP_EQUAL("notify_highlight", line->data->tags_array[0]);
    POINTERS_EQUAL(line->prev_line,
                   relay_remote_event_search_line_by_id (buffer, 1));

    /* Line updated. */
    recv ("{\"event_name\": \"buffer_line_data_changed\", \"buffer_id\": 123, "
          "\"body_type\": \"line\", "
          "\"body\": {\"id\": 1, "
          "\"date\": \"2026-01-02T03:04:06.000000Z\", "
          "\"prefix\": \"bob\", \"message\": \"hello (edited)\", "
          "\"tags\": [\"irc_privmsg\"]}}");
    line = relay_remote_event_search_line_by_id (buffer, 1);
    CHECK(line);
    STRCMP_EQUAL("bob", line->data->prefix);
    STRCMP_EQUAL("hello (edited)", line->data->message);
    LONGS_EQUAL(1767323046, line->data->date);
    LONGS_EQUAL(2, line->data->tags_count);
    STRCMP_EQUAL("relay_remote_line_id_1", line->data->tags_array[1]);

    /* Update of an unknown line: ignored. */
    recv ("{\"event_name\": \"buffer_line_data_changed\", \"buffer_id\": 123, "
          "\"body_type\": \"line\", "
          "\"body\": {\"id\": 99, \"message\": \"unknown\"}}");
    LONGS_EQUAL(2, buffer->own_lines->lines_count);

    /* Line for an unknown buffer: ignored. */
    recv ("{\"event_name\": \"buffer_line_added\", \"buffer_id\": 999, "
          "\"body_type\": \"line\", "
          "\"body\": {\"id\": 3, \"message\": \"test\"}}");
    LONGS_EQUAL(2, buffer->own_lines->lines_count);
}

/*
 * Test functions:
 *   relay_remote_event_line_add (buffer with free content)
 */

TEST(RelayRemoteEventWithRemote, LineAdd)
{
    struct t_gui_buffer *buffer;

    relay_remote_event_line_add (NULL);

    buffer = create_buffer (123, "fset.fset", ", \"type\": \"free\"");
    CHECK(buffer);
    LONGS_EQUAL(GUI_BUFFER_TYPE_FREE, buffer->type);

    recv ("{\"event_name\": \"buffer_line_added\", \"buffer_id\": 123, "
          "\"body_type\": \"line\", "
          "\"body\": {\"id\": 2, \"y\": 2, \"message\": \"line 2\"}}");
    LONGS_EQUAL(3, buffer->own_lines->lines_count);
    STRCMP_EQUAL("line 2", buffer->own_lines->last_line->data->message);
    LONGS_EQUAL(2, buffer->own_lines->last_line->data->y);
}

/*
 * Test functions:
 *   relay_remote_event_cb_nick_group
 *   relay_remote_event_cb_nick
 *   relay_remote_event_handle_nick_group
 *   relay_remote_event_handle_nick
 */

TEST(RelayRemoteEventWithRemote, CbNick)
{
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    struct t_gui_nick *nick;

    relay_remote_event_handle_nick (NULL, NULL);
    relay_remote_event_handle_nick_group (NULL, NULL);

    buffer = create_buffer (123, "irc.libera.#test", ", \"nicklist\": true");
    CHECK(buffer);
    CHECK(buffer->nicklist_root);

    /* The root group has id 0 on remote and locally. */
    recv ("{\"code\": 200, \"body_type\": \"buffer\", "
          "\"body\": {\"id\": 123, \"name\": \"irc.libera.#test\", "
          "\"nicklist\": true, "
          "\"nicklist_root\": {\"id\": 0, \"parent_group_id\": -1, "
          "\"name\": \"root\", \"visible\": false, "
          "\"groups\": [{\"id\": 2, \"parent_group_id\": 0, "
          "\"name\": \"000|o\", \"color_name\": \"weechat.color.nicklist_group\", "
          "\"visible\": true, "
          "\"nicks\": [{\"id\": 3, \"parent_group_id\": 2, "
          "\"name\": \"alice\", \"color_name\": \"cyan\", \"prefix\": \"@\", "
          "\"prefix_color_name\": \"lightgreen\", \"visible\": true}]}]}}}");
    group = gui_nicklist_search_group (buffer, NULL, "000|o");
    CHECK(group);
    LONGS_EQUAL(2, group->id);
    nick = gui_nicklist_search_nick (buffer, NULL, "alice");
    CHECK(nick);
    LONGS_EQUAL(3, nick->id);
    STRCMP_EQUAL("cyan", nick->color);
    STRCMP_EQUAL("@", nick->prefix);
    STRCMP_EQUAL("lightgreen", nick->prefix_color);
    POINTERS_EQUAL(group, nick->group);

    /* Nick added. */
    recv ("{\"event_name\": \"nicklist_nick_added\", \"buffer_id\": 123, "
          "\"body_type\": \"nick\", "
          "\"body\": {\"id\": 4, \"parent_group_id\": 2, \"name\": \"bob\", "
          "\"color_name\": \"red\", \"prefix\": \" \", "
          "\"prefix_color_name\": \"\", \"visible\": true}}");
    nick = gui_nicklist_search_nick (buffer, NULL, "bob");
    CHECK(nick);
    LONGS_EQUAL(4, nick->id);

    /* Nick changed. */
    recv ("{\"event_name\": \"nicklist_nick_changed\", \"buffer_id\": 123, "
          "\"body_type\": \"nick\", "
          "\"body\": {\"id\": 4, \"parent_group_id\": 2, \"name\": \"bob\", "
          "\"color_name\": \"green\", \"prefix\": \"+\", "
          "\"prefix_color_name\": \"yellow\", \"visible\": false}}");
    STRCMP_EQUAL("green", nick->color);
    STRCMP_EQUAL("+", nick->prefix);
    LONGS_EQUAL(0, nick->visible);

    /* Nick with unknown parent group: ignored. */
    recv ("{\"event_name\": \"nicklist_nick_added\", \"buffer_id\": 123, "
          "\"body_type\": \"nick\", "
          "\"body\": {\"id\": 5, \"parent_group_id\": 99, \"name\": \"carol\", "
          "\"visible\": true}}");
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "carol"));

    /* Group added, then changed. */
    recv ("{\"event_name\": \"nicklist_group_added\", \"buffer_id\": 123, "
          "\"body_type\": \"nick_group\", "
          "\"body\": {\"id\": 6, \"parent_group_id\": 0, \"name\": \"001|v\", "
          "\"color_name\": \"blue\", \"visible\": true}}");
    group = gui_nicklist_search_group (buffer, NULL, "001|v");
    CHECK(group);
    STRCMP_EQUAL("blue", group->color);
    recv ("{\"event_name\": \"nicklist_group_changed\", \"buffer_id\": 123, "
          "\"body_type\": \"nick_group\", "
          "\"body\": {\"id\": 6, \"parent_group_id\": 0, \"name\": \"001|v\", "
          "\"color_name\": \"magenta\", \"visible\": false}}");
    STRCMP_EQUAL("magenta", group->color);
    LONGS_EQUAL(0, group->visible);

    /* Nick removed. */
    recv ("{\"event_name\": \"nicklist_nick_removing\", \"buffer_id\": 123, "
          "\"body_type\": \"nick\", \"body\": {\"id\": 4}}");
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "bob"));
    recv ("{\"event_name\": \"nicklist_nick_removing\", \"buffer_id\": 123, "
          "\"body_type\": \"nick\", \"body\": {\"id\": 99}}");

    /* Group removed (with its nicks). */
    recv ("{\"event_name\": \"nicklist_group_removing\", \"buffer_id\": 123, "
          "\"body_type\": \"nick_group\", \"body\": {\"id\": 2}}");
    POINTERS_EQUAL(NULL, gui_nicklist_search_group (buffer, NULL, "000|o"));
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "alice"));
    recv ("{\"event_name\": \"nicklist_group_removing\", \"buffer_id\": 123, "
          "\"body_type\": \"nick_group\", \"body\": {\"id\": 99}}");
    CHECK(gui_nicklist_search_group (buffer, NULL, "001|v"));
}

/*
 * Test functions:
 *   relay_remote_event_apply_props
 */

TEST(RelayRemoteEventWithRemote, ApplyProps)
{
    struct t_gui_buffer *buffer;
    struct t_hashtable *props;

    buffer = create_buffer (123, "irc.libera.#test", NULL);
    CHECK(buffer);

    props = hashtable_new (8, WEECHAT_HASHTABLE_STRING,
                           WEECHAT_HASHTABLE_STRING, NULL, NULL);
    CHECK(props);
    hashtable_set (props, "title", "new title");
    hashtable_set (props, "localvar_set_test", "value");
    hashtable_map (props, &relay_remote_event_apply_props, buffer);
    STRCMP_EQUAL("new title", gui_buffer_get_string (buffer, "title"));
    STRCMP_EQUAL("value", gui_buffer_get_string (buffer, "localvar_test"));
    hashtable_free (props);
}

/*
 * Test functions:
 *   relay_remote_event_buffer_input
 */

TEST(RelayRemoteEventWithRemote, BufferInputCb)
{
    struct t_gui_buffer *buffer;

    relay_remote_event_buffer_input (NULL, "test");
    relay_remote_event_buffer_input (gui_buffers, "test");
    LONGS_EQUAL(0, peer_recv ());

    buffer = create_buffer (123, "irc.libera.#test", NULL);
    CHECK(buffer);

    /* Input sent to the remote. */
    relay_remote_event_buffer_input (buffer, "hello \"world\"");
    CHECK(peer_recv () > 0);
    STRCMP_EQUAL("{\"request\":\"POST /api/input\","
                 "\"body\":{\"buffer_id\":123,"
                 "\"command\":\"hello \\\"world\\\"\"}}",
                 data_sent);

    /* Invalid buffer id: nothing sent. */
    gui_buffer_set (buffer, "localvar_set_relay_remote_id", "abc");
    relay_remote_event_buffer_input (buffer, "test");
    LONGS_EQUAL(0, peer_recv ());
    gui_buffer_set (buffer, "localvar_set_relay_remote_id", "123");

    /* Unable to send: disconnected. */
    close (sock_peer);
    sock_peer = -1;
    relay_remote_set_status (remote, RELAY_STATUS_AUTHENTICATING);
    relay_remote_event_buffer_input (buffer, "test");
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(-1, remote->sock);
}

/*
 * Test functions:
 *   relay_remote_event_check_local_var
 */

TEST(RelayRemoteEventWithRemote, CheckLocalVar)
{
    LONGS_EQUAL(0, relay_remote_event_check_local_var (NULL));
    LONGS_EQUAL(0, relay_remote_event_check_local_var ("plugin"));
    LONGS_EQUAL(0, relay_remote_event_check_local_var ("name"));
    LONGS_EQUAL(0, relay_remote_event_check_local_var ("relay_remote"));
    LONGS_EQUAL(0, relay_remote_event_check_local_var ("relay_remote_id"));
    LONGS_EQUAL(0, relay_remote_event_check_local_var ("relay_remote_xxx"));

    LONGS_EQUAL(1, relay_remote_event_check_local_var (""));
    LONGS_EQUAL(1, relay_remote_event_check_local_var ("channel"));
    LONGS_EQUAL(1, relay_remote_event_check_local_var ("nick"));
    LONGS_EQUAL(1, relay_remote_event_check_local_var ("relay"));
    LONGS_EQUAL(1, relay_remote_event_check_local_var ("plugins"));
    LONGS_EQUAL(1, relay_remote_event_check_local_var ("names"));
}

/*
 * Test functions:
 *   relay_remote_event_cb_buffer (local variables)
 *   relay_remote_event_remove_localvar_cb
 */

TEST(RelayRemoteEventWithRemote, RemoveLocalVarCb)
{
    struct t_gui_buffer *buffer;

    buffer = create_buffer (
        123, "irc.libera.#test",
        ", \"local_variables\": {\"channel\": \"#test\", \"nick\": \"alice\", "
        "\"relay_remote_id\": \"999\", \"number\": 42}");
    CHECK(buffer);
    STRCMP_EQUAL("#test", gui_buffer_get_string (buffer, "localvar_channel"));
    STRCMP_EQUAL("alice", gui_buffer_get_string (buffer, "localvar_nick"));
    /* Reserved variable and variables that are not strings are ignored. */
    STRCMP_EQUAL("123",
                 gui_buffer_get_string (buffer, "localvar_relay_remote_id"));
    POINTERS_EQUAL(NULL, gui_buffer_get_string (buffer, "localvar_number"));

    /* Local variable removed on remote. */
    recv ("{\"event_name\": \"buffer_localvar_removed\", \"buffer_id\": 123, "
          "\"body_type\": \"buffer\", "
          "\"body\": {\"id\": 123, \"name\": \"irc.libera.#test\", "
          "\"local_variables\": {\"channel\": \"#test\"}}}");
    STRCMP_EQUAL("#test", gui_buffer_get_string (buffer, "localvar_channel"));
    POINTERS_EQUAL(NULL, gui_buffer_get_string (buffer, "localvar_nick"));

    /* Reserved variables are kept. */
    STRCMP_EQUAL("relay", gui_buffer_get_string (buffer, "localvar_plugin"));
    STRCMP_EQUAL("testremote",
                 gui_buffer_get_string (buffer, "localvar_relay_remote"));
    STRCMP_EQUAL("123",
                 gui_buffer_get_string (buffer, "localvar_relay_remote_id"));
}

/*
 * Test functions:
 *   relay_remote_event_initial_sync_buffers
 */

TEST(RelayRemoteEventWithRemote, InitialSyncBuffers)
{
    relay_remote_event_initial_sync_buffers (NULL);

    CHECK(create_buffer (1, "core.weechat", NULL));
    CHECK(create_buffer (2, "irc.server.libera", NULL));
    CHECK(create_buffer (3, "irc.libera.#test", NULL));
    LONGS_EQUAL(0, remote->synced);

    /* Buffers not received on initial sync are closed. */
    recv ("{\"code\": 200, \"request_id\": \"initial_sync\", "
          "\"body_type\": \"buffers\", "
          "\"body\": [{\"id\": 1, \"name\": \"core.weechat\"}, "
          "{\"id\": 3, \"name\": \"irc.libera.#test\"}, "
          "{\"id\": 4, \"name\": \"irc.libera.#new\"}]}");
    CHECK(relay_remote_event_search_buffer (remote, 1));
    POINTERS_EQUAL(NULL, relay_remote_event_search_buffer (remote, 2));
    CHECK(relay_remote_event_search_buffer (remote, 3));
    CHECK(relay_remote_event_search_buffer (remote, 4));
    LONGS_EQUAL(1, remote->synced);

    /* Response not for initial sync: buffers are not closed. */
    recv ("{\"code\": 200, \"body_type\": \"buffers\", "
          "\"body\": [{\"id\": 1, \"name\": \"core.weechat\"}]}");
    CHECK(relay_remote_event_search_buffer (remote, 3));
}

/*
 * Test functions:
 *   relay_remote_event_cb_buffer_cleared
 */

TEST(RelayRemoteEventWithRemote, CbBufferCleared)
{
    struct t_gui_buffer *buffer;

    buffer = create_buffer (
        123, "irc.libera.#test",
        ", \"lines\": [{\"id\": 1, \"message\": \"line 1\"}, "
        "{\"id\": 2, \"message\": \"line 2\"}]");
    CHECK(buffer);
    LONGS_EQUAL(2, buffer->own_lines->lines_count);

    /* Unknown buffer: ignored. */
    recv ("{\"event_name\": \"buffer_cleared\", \"buffer_id\": 999, "
          "\"body_type\": \"buffer\", \"body\": {\"id\": 999}}");
    LONGS_EQUAL(2, buffer->own_lines->lines_count);

    recv ("{\"event_name\": \"buffer_cleared\", \"buffer_id\": 123, "
          "\"body_type\": \"buffer\", \"body\": {\"id\": 123}}");
    LONGS_EQUAL(0, buffer->own_lines->lines_count);
}

/*
 * Test functions:
 *   relay_remote_event_cb_buffer_closed
 */

TEST(RelayRemoteEventWithRemote, CbBufferClosed)
{
    CHECK(create_buffer (123, "irc.libera.#test", NULL));

    /* Event "buffer_closing" is ignored. */
    recv ("{\"event_name\": \"buffer_closing\", \"buffer_id\": 123, "
          "\"body_type\": \"buffer\", \"body\": {\"id\": 123}}");
    CHECK(relay_remote_event_search_buffer (remote, 123));

    /* Unknown buffer: ignored. */
    recv ("{\"event_name\": \"buffer_closed\", \"buffer_id\": 999, "
          "\"body_type\": \"buffer\", \"body\": {\"id\": 999}}");
    CHECK(relay_remote_event_search_buffer (remote, 123));

    recv ("{\"event_name\": \"buffer_closed\", \"buffer_id\": 123, "
          "\"body_type\": \"buffer\", \"body\": {\"id\": 123}}");
    POINTERS_EQUAL(NULL, relay_remote_event_search_buffer (remote, 123));
}

/*
 * Test functions:
 *   relay_remote_event_cb_input
 */

TEST(RelayRemoteEventWithRemote, CbInput)
{
    struct t_gui_buffer *buffer;

    buffer = create_buffer (123, "irc.libera.#test",
                            ", \"input\": \"initial\", \"input_position\": 3");
    CHECK(buffer);
    STRCMP_EQUAL("initial", gui_buffer_get_string (buffer, "input"));
    LONGS_EQUAL(3, gui_buffer_get_integer (buffer, "input_pos"));

    recv ("{\"event_name\": \"input_text_changed\", \"buffer_id\": 123, "
          "\"body_type\": \"buffer\", "
          "\"body\": {\"id\": 123, \"input_prompt\": \"[alice]\", "
          "\"input\": \"hello world\", \"input_position\": 5}}");
    STRCMP_EQUAL("[alice]", gui_buffer_get_string (buffer, "input_prompt"));
    STRCMP_EQUAL("hello world", gui_buffer_get_string (buffer, "input"));
    LONGS_EQUAL(5, gui_buffer_get_integer (buffer, "input_pos"));

    /* Unknown buffer: ignored. */
    recv ("{\"event_name\": \"input_text_changed\", \"buffer_id\": 999, "
          "\"body_type\": \"buffer\", "
          "\"body\": {\"id\": 999, \"input\": \"other\"}}");
    STRCMP_EQUAL("hello world", gui_buffer_get_string (buffer, "input"));
}

/*
 * Test functions:
 *   relay_remote_event_clear_buffers
 */

TEST(RelayRemoteEventWithRemote, ClearBuffers)
{
    struct t_gui_buffer *buffer;
    struct t_relay_remote *remote2;

    relay_remote_event_clear_buffers (NULL);

    buffer = create_buffer (
        123, "irc.libera.#test",
        ", \"nicklist\": true, "
        "\"lines\": [{\"id\": 1, \"message\": \"line 1\"}], "
        "\"nicklist_root\": {\"id\": 0, \"parent_group_id\": -1, "
        "\"name\": \"root\", "
        "\"nicks\": [{\"id\": 2, \"parent_group_id\": 0, \"name\": \"alice\", "
        "\"visible\": true}]}");
    CHECK(buffer);
    LONGS_EQUAL(1, buffer->own_lines->lines_count);
    CHECK(gui_nicklist_search_nick (buffer, NULL, "alice"));

    /* Buffers of another remote are not cleared. */
    remote2 = relay_remote_new ("testremote2", "http://localhost:1",
                                NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(remote2);
    relay_remote_event_clear_buffers (remote2);
    LONGS_EQUAL(1, buffer->own_lines->lines_count);
    relay_remote_free (remote2);

    relay_remote_event_clear_buffers (remote);
    LONGS_EQUAL(0, buffer->own_lines->lines_count);
    POINTERS_EQUAL(NULL, gui_nicklist_search_nick (buffer, NULL, "alice"));
}

/*
 * Test functions:
 *   relay_remote_event_sync_with_remote
 */

TEST(RelayRemoteEventWithRemote, SyncWithRemote)
{
    relay_remote_event_sync_with_remote (NULL);
    LONGS_EQUAL(0, peer_recv ());

    relay_remote_event_sync_with_remote (remote);
    CHECK(peer_recv () > 0);
    STRCMP_EQUAL("[{\"request\":\"GET /api/buffers?lines=-1000&nicks=true"
                 "&colors=weechat&rgb_colors=true\","
                 "\"request_id\":\"initial_sync\"},"
                 "{\"request\":\"POST /api/sync\","
                 "\"body\":{\"colors\":\"weechat\",\"rgb_colors\":true}}]",
                 data_sent);
}

/*
 * Test functions:
 *   relay_remote_event_cb_version
 */

TEST(RelayRemoteEventWithRemote, CbVersion)
{
    struct t_gui_buffer *buffer;
    char json[1024];

    buffer = create_buffer (
        123, "irc.libera.#test",
        ", \"lines\": [{\"id\": 1, \"message\": \"line 1\"}]");
    CHECK(buffer);

    /* Same API version: buffers cleared and sync with remote. */
    snprintf (json, sizeof (json),
              "{\"code\": 200, \"body_type\": \"version\", "
              "\"body\": {\"weechat_version\": \"5.0.0\", "
              "\"weechat_version_git\": \"v5.0.0\", "
              "\"relay_api_version\": \"%s\"}}",
              RELAY_API_VERSION_STR);
    recv (json);
    LONGS_EQUAL(1, remote->version_ok);
    LONGS_EQUAL(0, buffer->own_lines->lines_count);
    CHECK(peer_recv () > 0);
    STRNCMP_EQUAL("[{\"request\":\"GET /api/buffers?", data_sent, 30);

    /* Version already checked: no sync. */
    recv (json);
    LONGS_EQUAL(0, peer_recv ());

    /* Different API version: disconnected. */
    remote->version_ok = 0;
    recv ("{\"code\": 200, \"body_type\": \"version\", "
          "\"body\": {\"weechat_version\": \"4.0.0\", "
          "\"weechat_version_git\": \"\", "
          "\"relay_api_version\": \"0.0.1\"}}");
    LONGS_EQUAL(0, remote->version_ok);
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(-1, remote->sock);
}

/*
 * Test functions:
 *   relay_remote_event_cb_upgrade
 */

TEST(RelayRemoteEventWithRemote, CbUpgrade)
{
    relay_remote_set_status (remote, RELAY_STATUS_CONNECTED);
    recv ("{\"event_name\": \"upgrade\", \"buffer_id\": -1, "
          "\"body_type\": null, \"body\": null}");
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(-1, remote->sock);
}

/*
 * Test functions:
 *   relay_remote_event_cb_quit
 */

TEST(RelayRemoteEventWithRemote, CbQuit)
{
    relay_remote_set_status (remote, RELAY_STATUS_CONNECTED);
    recv ("{\"event_name\": \"quit\", \"buffer_id\": -1, "
          "\"body_type\": null, \"body\": null}");
    LONGS_EQUAL(RELAY_STATUS_DISCONNECTED, remote->status);
    LONGS_EQUAL(-1, remote->sock);
}

/*
 * Test functions:
 *   relay_remote_event_recv
 */

TEST(RelayRemoteEventWithRemote, Recv)
{
    relay_remote_event_recv (NULL, "{}");
    relay_remote_event_recv (remote, NULL);

    /* Invalid JSON: ignored. */
    recv ("");
    recv ("not json");
    recv ("{\"code\": 200");

    /* Response without body: ignored. */
    recv ("{\"code\": 200, \"message\": \"OK\"}");
    recv ("{\"code\": 204, \"message\": \"No Content\"}");

    /* Unknown body type or event: ignored. */
    recv ("{\"code\": 200, \"body_type\": \"unknown\", \"body\": {}}");
    recv ("{\"event_name\": \"unknown_event\", \"buffer_id\": 1, "
          "\"body_type\": \"buffer\", \"body\": {\"id\": 1}}");
    POINTERS_EQUAL(NULL, relay_remote_event_search_buffer (remote, 1));

    /* Event for a buffer: buffer created. */
    recv ("{\"event_name\": \"buffer_opened\", \"buffer_id\": 1, "
          "\"body_type\": \"buffer\", "
          "\"body\": {\"id\": 1, \"name\": \"irc.libera.#new\"}}");
    CHECK(relay_remote_event_search_buffer (remote, 1));

    /* Array of buffers. */
    recv ("{\"code\": 200, \"body_type\": \"buffers\", "
          "\"body\": [{\"id\": 2, \"name\": \"irc.libera.#a\"}, "
          "{\"id\": 3, \"name\": \"irc.libera.#b\"}]}");
    CHECK(relay_remote_event_search_buffer (remote, 2));
    CHECK(relay_remote_event_search_buffer (remote, 3));

    /* Still connected. */
    LONGS_EQUAL(0, remote->sock == -1);
}
