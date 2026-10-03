/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay API protocol (messages) */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <limits.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hdata.h"
#include "src/core/core-hook.h"
#include "src/core/core-util.h"
#include "src/core/weechat.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-hotlist.h"
#include "src/gui/gui-input.h"
#include "src/gui/gui-line.h"
#include "src/gui/gui-nicklist.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/relay-json.h"
#include "src/plugins/relay/api/relay-api.h"
#include "src/plugins/relay/api/relay-api-msg.h"
}

/*
 * Return value of a JSON integer number, LLONG_MIN if the item is not an
 * integer number.
 */

long long
test_relay_api_msg_json_number (struct t_relay_json *json)
{
    long long value;

    return (relay_json_get_number (json, &value)) ? value : LLONG_MIN;
}

#define WEE_CHECK_OBJ_STR(__expected, __json, __name)                   \
    json_obj = relay_json_object_get (__json, __name);                  \
    CHECK(json_obj);                                                    \
    CHECK(relay_json_is_string (json_obj));                             \
    STRCMP_EQUAL(__expected, relay_json_get_string (json_obj));

#define WEE_CHECK_OBJ_STRN(__expected, __length, __json, __name)        \
    json_obj = relay_json_object_get (__json, __name);                  \
    CHECK(json_obj);                                                    \
    CHECK(relay_json_is_string (json_obj));                             \
    STRNCMP_EQUAL(__expected, relay_json_get_string (json_obj),         \
                  __length);

#define WEE_CHECK_OBJ_NUM(__expected, __json, __name)                   \
    json_obj = relay_json_object_get (__json, __name);                  \
    CHECK(json_obj);                                                    \
    CHECK(relay_json_is_number (json_obj));                             \
    CHECK((long long)(__expected) == test_relay_api_msg_json_number (json_obj));

#define WEE_CHECK_OBJ_BOOL(__expected, __json, __name)                  \
    json_obj = relay_json_object_get (__json, __name);                  \
    CHECK(json_obj);                                                    \
    CHECK(relay_json_is_bool (json_obj));                               \
    LONGS_EQUAL(__expected, relay_json_is_true (json_obj) ? 1 : 0);

TEST_GROUP(RelayApiMsg)
{
};

/*
 * Test functions:
 *   relay_api_msg_send_json_internal
 */

TEST(RelayApiMsg, SendJsonInternal)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_msg_send_json
 */

TEST(RelayApiMsg, SendJson)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_msg_send_error_json
 */

TEST(RelayApiMsg, SendErrorJson)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_msg_send_event
 */

TEST(RelayApiMsg, SendEvent)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   relay_api_msg_buffer_add_local_vars_cb
 *   relay_api_msg_buffer_to_json
 *   relay_api_msg_nick_to_json
 *   relay_api_msg_nick_group_to_json
 */

TEST(RelayApiMsg, BufferToJson)
{
    struct t_relay_json *json, *json_obj, *json_local_vars, *json_keys, *json_key;
    struct t_relay_json *json_lines, *json_line;
    struct t_relay_json *json_nicklist_root, *json_nicks, *json_groups, *json_group;
    struct t_relay_json *json_group_nicks, *json_nick;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    struct t_config_option *ptr_option;
    long long group_id;
    char *color;

    json = relay_api_msg_buffer_to_json (NULL, 0L, 0L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    POINTERS_EQUAL(NULL, relay_json_object_get (json, "name"));
    relay_json_free (json);

    gui_buffer_set (gui_buffers, "key_bind_meta-y,1", "/test1");
    gui_buffer_set (gui_buffers, "key_bind_meta-y,2", "/test2 arg");

    /* Buffer without lines and nicks */
    json = relay_api_msg_buffer_to_json (gui_buffers, 0L, 0L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    WEE_CHECK_OBJ_NUM(gui_buffers->id, json, "id");
    WEE_CHECK_OBJ_STR("core.weechat", json, "name");
    WEE_CHECK_OBJ_STR("weechat", json, "short_name");
    WEE_CHECK_OBJ_NUM(1, json, "number");
    WEE_CHECK_OBJ_STR("formatted", json, "type");
    WEE_CHECK_OBJ_STR("all", json, "notify");
    WEE_CHECK_OBJ_BOOL(0, json, "hidden");
    WEE_CHECK_OBJ_STRN("WeeChat", 7, json, "title");
    WEE_CHECK_OBJ_STR("", json, "modes");
    WEE_CHECK_OBJ_STR("", json, "input_prompt");
    WEE_CHECK_OBJ_STR("", json, "input");
    WEE_CHECK_OBJ_NUM(0, json, "input_position");
    WEE_CHECK_OBJ_BOOL(0, json, "input_multiline");
    WEE_CHECK_OBJ_BOOL(0, json, "nicklist");
    WEE_CHECK_OBJ_BOOL(0, json, "nicklist_case_sensitive");
    WEE_CHECK_OBJ_BOOL(1, json, "nicklist_display_groups");
    WEE_CHECK_OBJ_BOOL(1, json, "time_displayed");
    json_local_vars = relay_json_object_get (json, "local_variables");
    CHECK(json_local_vars);
    CHECK(relay_json_is_object (json_local_vars));
    json_keys = relay_json_object_get (json, "keys");
    CHECK(json_keys);
    LONGS_EQUAL(2, relay_json_array_size (json_keys));
    json_key = relay_json_array_get (json_keys, 0);
    CHECK(json_key);
    WEE_CHECK_OBJ_STR("meta-y,1", json_key, "key");
    WEE_CHECK_OBJ_STR("/test1", json_key, "command");
    json_key = relay_json_array_get (json_keys, 1);
    CHECK(json_key);
    WEE_CHECK_OBJ_STR("meta-y,2", json_key, "key");
    WEE_CHECK_OBJ_STR("/test2 arg", json_key, "command");
    WEE_CHECK_OBJ_STR("core", json_local_vars, "plugin");
    WEE_CHECK_OBJ_STR("weechat", json_local_vars, "name");
    POINTERS_EQUAL(NULL, relay_json_object_get (json, "lines"));
    WEE_CHECK_OBJ_NUM(-1, json, "last_read_line_id");
    POINTERS_EQUAL(NULL, relay_json_object_get (json, "nicks"));
    relay_json_free (json);

    gui_buffer_hide (gui_buffers);
    gui_buffer_set_time_for_each_line (gui_buffers, 0);
    gui_buffer_set_prefix_for_each_line (gui_buffers, 0);
    config_file_option_set_with_string ("weechat.notify.core.weechat",
                                        "highlight");

    json = relay_api_msg_buffer_to_json (gui_buffers, 0L, 0L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    WEE_CHECK_OBJ_STR("highlight", json, "notify");
    WEE_CHECK_OBJ_BOOL(1, json, "hidden");
    WEE_CHECK_OBJ_BOOL(0, json, "time_displayed");
    WEE_CHECK_OBJ_BOOL(0, json, "prefix_displayed");
    relay_json_free (json);

    gui_buffer_unhide (gui_buffers);
    gui_buffer_set_time_for_each_line (gui_buffers, 1);
    gui_buffer_set_prefix_for_each_line (gui_buffers, 1);
    config_file_search_with_string ("weechat.notify.core.weechat",
                                    NULL, NULL, &ptr_option, NULL);
    config_file_option_unset (ptr_option);

    /* Buffer with 2 lines, without nicks */
    json = relay_api_msg_buffer_to_json (gui_buffers, 2L, 0L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    json_lines = relay_json_object_get (json, "lines");
    CHECK(json_lines);
    CHECK(relay_json_is_array (json_lines));
    LONGS_EQUAL(2, relay_json_array_size (json_lines));
    relay_json_free (json);

    /* Create a user buffer with 1 group / 4 nicks. */
    buffer = gui_buffer_new_user ("test", GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);
    gui_buffer_set (buffer, "nicklist", "1");
    gui_buffer_set (buffer, "nicklist_case_sensitive", "0");
    gui_buffer_set (buffer, "nicklist_display_groups", "0");
    group = gui_nicklist_add_group (buffer, NULL, "group1", "magenta", 1);
    CHECK(group);
    CHECK(gui_nicklist_add_nick (buffer, group, "nick1", "blue", "@", "lightred", 1));
    CHECK(gui_nicklist_add_nick (buffer, group, "nick2", "green", NULL, NULL, 1));
    CHECK(gui_nicklist_add_nick (buffer, group, "nick3", "yellow", NULL, NULL, 1));
    CHECK(gui_nicklist_add_nick (buffer, NULL, "root_nick_hidden", "cyan", "+", "yellow", 0));

    /* Buffer with no lines and 1 group / 4 nicks */
    json = relay_api_msg_buffer_to_json (buffer, 1L, 0L, 1, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    WEE_CHECK_OBJ_BOOL(1, json, "nicklist");
    WEE_CHECK_OBJ_BOOL(0, json, "nicklist_case_sensitive");
    WEE_CHECK_OBJ_BOOL(0, json, "nicklist_display_groups");
    json_lines = relay_json_object_get (json, "lines");
    CHECK(json_lines);
    CHECK(relay_json_is_array (json_lines));
    LONGS_EQUAL(0, relay_json_array_size (json_lines));
    json_nicklist_root = relay_json_object_get (json, "nicklist_root");
    CHECK(json_nicklist_root);
    CHECK(relay_json_is_object (json_nicklist_root));
    WEE_CHECK_OBJ_NUM(0, json_nicklist_root, "id");
    WEE_CHECK_OBJ_STR("root", json_nicklist_root, "name");
    WEE_CHECK_OBJ_STR("", json_nicklist_root, "color_name");
    WEE_CHECK_OBJ_STR("", json_nicklist_root, "color");
    json_groups = relay_json_object_get (json_nicklist_root, "groups");
    CHECK(json_groups);
    CHECK(relay_json_is_array (json_groups));
    LONGS_EQUAL(1, relay_json_array_size (json_groups));
    json_group = relay_json_array_get (json_groups, 0);
    CHECK(json_group);
    CHECK(relay_json_is_object (json_group));
    json_obj = relay_json_object_get (json_group, "id");
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    group_id = test_relay_api_msg_json_number (json_obj);
    CHECK(group_id > 0);
    json_obj = relay_json_object_get (json_group, "parent_group_id");
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    CHECK(test_relay_api_msg_json_number (json_obj) == 0);
    WEE_CHECK_OBJ_STR("group1", json_group, "name");
    WEE_CHECK_OBJ_STR("magenta", json_group, "color_name");
    color = gui_color_encode_ansi (gui_color_get_custom ("magenta"));
    WEE_CHECK_OBJ_STR(color, json_group, "color");
    free (color);
    json_group_nicks = relay_json_object_get (json_group, "nicks");
    CHECK(json_group_nicks);
    CHECK(relay_json_is_array (json_group_nicks));
    LONGS_EQUAL(3, relay_json_array_size (json_group_nicks));
    json_nick = relay_json_array_get (json_group_nicks, 0);
    CHECK(json_nick);
    CHECK(relay_json_is_object (json_nick));
    json_obj = relay_json_object_get (json_nick, "id");
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    CHECK(test_relay_api_msg_json_number (json_obj) > 0);
    WEE_CHECK_OBJ_NUM(group_id, json_nick, "parent_group_id");
    WEE_CHECK_OBJ_STR("@", json_nick, "prefix");
    WEE_CHECK_OBJ_STR("lightred", json_nick, "prefix_color_name");
    color = gui_color_encode_ansi (gui_color_get_custom ("lightred"));
    WEE_CHECK_OBJ_STR(color, json_nick, "prefix_color");
    free (color);
    WEE_CHECK_OBJ_STR("nick1", json_nick, "name");
    WEE_CHECK_OBJ_STR("blue", json_nick, "color_name");
    color = gui_color_encode_ansi (gui_color_get_custom ("blue"));
    WEE_CHECK_OBJ_STR(color, json_nick, "color");
    free (color);
    WEE_CHECK_OBJ_BOOL(1, json_nick, "visible");
    json_nick = relay_json_array_get (json_group_nicks, 1);
    CHECK(json_nick);
    CHECK(relay_json_is_object (json_nick));
    json_obj = relay_json_object_get (json_nick, "id");
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    CHECK(test_relay_api_msg_json_number (json_obj) > 0);
    WEE_CHECK_OBJ_NUM(group_id, json_nick, "parent_group_id");
    WEE_CHECK_OBJ_STR("", json_nick, "prefix");
    WEE_CHECK_OBJ_STR("", json_nick, "prefix_color_name");
    WEE_CHECK_OBJ_STR("", json_nick, "prefix_color");
    WEE_CHECK_OBJ_STR("nick2", json_nick, "name");
    WEE_CHECK_OBJ_STR("green", json_nick, "color_name");
    color = gui_color_encode_ansi (gui_color_get_custom ("green"));
    WEE_CHECK_OBJ_STR(color, json_nick, "color");
    free (color);
    WEE_CHECK_OBJ_BOOL(1, json_nick, "visible");
    json_nick = relay_json_array_get (json_group_nicks, 2);
    CHECK(json_nick);
    CHECK(relay_json_is_object (json_nick));
    json_obj = relay_json_object_get (json_nick, "id");
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    CHECK(test_relay_api_msg_json_number (json_obj) > 0);
    WEE_CHECK_OBJ_NUM(group_id, json_nick, "parent_group_id");
    WEE_CHECK_OBJ_STR("", json_nick, "prefix");
    WEE_CHECK_OBJ_STR("", json_nick, "prefix_color_name");
    WEE_CHECK_OBJ_STR("", json_nick, "prefix_color");
    WEE_CHECK_OBJ_STR("nick3", json_nick, "name");
    WEE_CHECK_OBJ_STR("yellow", json_nick, "color_name");
    color = gui_color_encode_ansi (gui_color_get_custom ("yellow"));
    WEE_CHECK_OBJ_STR(color, json_nick, "color");
    free (color);
    WEE_CHECK_OBJ_BOOL(1, json_nick, "visible");
    json_nicks = relay_json_object_get (json_nicklist_root, "nicks");
    CHECK(json_nicks);
    CHECK(relay_json_is_array (json_nicks));
    LONGS_EQUAL(1, relay_json_array_size (json_nicks));
    json_nick = relay_json_array_get (json_nicks, 0);
    CHECK(json_nick);
    CHECK(relay_json_is_object (json_nick));
    json_obj = relay_json_object_get (json_nick, "id");
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    CHECK(test_relay_api_msg_json_number (json_obj) > 0);
    WEE_CHECK_OBJ_NUM(0, json_nick, "parent_group_id");
    WEE_CHECK_OBJ_STR("+", json_nick, "prefix");
    WEE_CHECK_OBJ_STR("yellow", json_nick, "prefix_color_name");
    color = gui_color_encode_ansi (gui_color_get_custom ("yellow"));
    WEE_CHECK_OBJ_STR(color, json_nick, "prefix_color");
    free (color);
    WEE_CHECK_OBJ_STR("root_nick_hidden", json_nick, "name");
    WEE_CHECK_OBJ_STR("cyan", json_nick, "color_name");
    color = gui_color_encode_ansi (gui_color_get_custom ("cyan"));
    WEE_CHECK_OBJ_STR(color, json_nick, "color");
    free (color);
    WEE_CHECK_OBJ_BOOL(0, json_nick, "visible");
    relay_json_free (json);

    gui_buffer_set (gui_buffers, "key_unbind_meta-y", "");

    gui_buffer_close (buffer);

    buffer = gui_buffer_new_user ("test", GUI_BUFFER_TYPE_FREE);
    CHECK(buffer);
    gui_chat_printf_y (buffer, 0, "test line 1");
    gui_chat_printf_y (buffer, 1, "test line 2");
    gui_chat_printf_y (buffer, 2, "test line 3");
    gui_chat_printf_y (buffer, 3, "test line 4");
    gui_chat_printf_y (buffer, 4, "test line 5");

    json = relay_api_msg_buffer_to_json (buffer, 1L, 2L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    json_lines = relay_json_object_get (json, "lines");
    CHECK(json_lines);
    CHECK(relay_json_is_array (json_lines));
    LONGS_EQUAL(2, relay_json_array_size (json_lines));
    json_line = relay_json_array_get (json_lines, 0);
    CHECK(json_line);
    WEE_CHECK_OBJ_STR("test line 1", json_line, "message");
    json_line = relay_json_array_get (json_lines, 1);
    CHECK(json_line);
    WEE_CHECK_OBJ_STR("test line 2", json_line, "message");
    relay_json_free (json);

    json = relay_api_msg_buffer_to_json (buffer, 1L, -2L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    json_lines = relay_json_object_get (json, "lines");
    CHECK(json_lines);
    CHECK(relay_json_is_array (json_lines));
    LONGS_EQUAL(2, relay_json_array_size (json_lines));
    json_line = relay_json_array_get (json_lines, 0);
    CHECK(json_line);
    WEE_CHECK_OBJ_STR("test line 4", json_line, "message");
    json_line = relay_json_array_get (json_lines, 1);
    CHECK(json_line);
    WEE_CHECK_OBJ_STR("test line 5", json_line, "message");
    relay_json_free (json);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   relay_api_msg_line_data_to_json
 *   relay_api_msg_lines_to_json
 */

TEST(RelayApiMsg, LinesToJson)
{
    char str_msg1[1024], str_msg2[1024], *str_msg_ansi, str_date[128];
    struct t_relay_json *json, *json_line, *json_obj, *json_tags, *json_tag;
    struct timeval tv;
    struct tm gm_time;

    snprintf (str_msg1, sizeof (str_msg1), "%s", "this is the first line");
    gui_chat_printf_date_tags (NULL, 0, "tag1,tag2,tag3",
                               "%s\t%s", "nick1", str_msg1);

    snprintf (str_msg2, sizeof (str_msg2),
              "this is the second line with %s" "green",
              gui_color_get_custom ("green"));
    gui_chat_printf (NULL, "%s", str_msg2);

    /* Two lines with ANSI colors */
    json = relay_api_msg_lines_to_json (gui_buffers, -2, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(relay_json_is_array (json));
    LONGS_EQUAL(2, relay_json_array_size (json));
    /* First line */
    json_line = relay_json_array_get (json, 0);
    CHECK(json_line);
    CHECK(relay_json_is_object (json_line));
    /* The line id is the date of print, with microseconds precision. */
    CHECK(gui_buffers->own_lines->last_line->prev_line->data->id
          > 1000000000000000LL);
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->prev_line->data->id,
                      json_line, "id");
    WEE_CHECK_OBJ_NUM(-1, json_line, "y");
    gmtime_r (&(gui_buffers->own_lines->last_line->prev_line->data->date), &gm_time);
    tv.tv_sec = mktime (&gm_time);
    tv.tv_usec = gui_buffers->own_lines->last_line->prev_line->data->date_usec;
    util_strftimeval (str_date, sizeof (str_date), "%@%FT%T.%fZ", &tv);
    WEE_CHECK_OBJ_STR(str_date, json_line, "date");
    CHECK(!relay_json_object_get (json_line, "date_printed"));
    WEE_CHECK_OBJ_BOOL(0, json_line, "highlight");
    WEE_CHECK_OBJ_STR("nick1", json_line, "prefix");
    WEE_CHECK_OBJ_STR(str_msg1, json_line, "message");
    json_tags = relay_json_object_get (json_line, "tags");
    CHECK(json_tags);
    CHECK(relay_json_is_array (json_tags));
    LONGS_EQUAL(3, relay_json_array_size (json_tags));
    json_tag = relay_json_array_get (json_tags, 0);
    CHECK(json_tag);
    CHECK(relay_json_is_string (json_tag));
    STRCMP_EQUAL("tag1", relay_json_get_string (json_tag));
    json_tag = relay_json_array_get (json_tags, 1);
    CHECK(json_tag);
    CHECK(relay_json_is_string (json_tag));
    STRCMP_EQUAL("tag2", relay_json_get_string (json_tag));
    json_tag = relay_json_array_get (json_tags, 2);
    CHECK(json_tag);
    CHECK(relay_json_is_string (json_tag));
    STRCMP_EQUAL("tag3", relay_json_get_string (json_tag));
    /* Second line */
    json_line = relay_json_array_get (json, 1);
    CHECK(json_line);
    CHECK(relay_json_is_object (json_line));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->data->id,
                      json_line, "id");
    WEE_CHECK_OBJ_NUM(-1, json_line, "y");
    gmtime_r (&(gui_buffers->own_lines->last_line->data->date), &gm_time);
    tv.tv_sec = mktime (&gm_time);
    tv.tv_usec = gui_buffers->own_lines->last_line->data->date_usec;
    util_strftimeval (str_date, sizeof (str_date), "%@%FT%T.%fZ", &tv);
    WEE_CHECK_OBJ_STR(str_date, json_line, "date");
    CHECK(!relay_json_object_get (json_line, "date_printed"));
    WEE_CHECK_OBJ_BOOL(0, json_line, "highlight");
    WEE_CHECK_OBJ_STR("", json_line, "prefix");
        str_msg_ansi = gui_color_encode_ansi (str_msg2);
    CHECK(str_msg_ansi);
    WEE_CHECK_OBJ_STR(str_msg_ansi, json_line, "message");
    free (str_msg_ansi);
    json_tags = relay_json_object_get (json_line, "tags");
    CHECK(json_tags);
    CHECK(relay_json_is_array (json_tags));
    LONGS_EQUAL(0, relay_json_array_size (json_tags));
    relay_json_free (json);

    /* With ANSI colors */
    json = relay_api_msg_lines_to_json (gui_buffers, -1, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(relay_json_is_array (json));
    LONGS_EQUAL(1, relay_json_array_size (json));
    json_line = relay_json_array_get (json, 0);
    CHECK(json_line);
    CHECK(relay_json_is_object (json_line));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->data->id,
                      json_line, "id");
    str_msg_ansi = gui_color_encode_ansi (str_msg2);
    CHECK(str_msg_ansi);
    WEE_CHECK_OBJ_STR(str_msg_ansi, json_line, "message");
    free (str_msg_ansi);
    relay_json_free (json);

    /* One line with WeeChat colors */
    json = relay_api_msg_lines_to_json (gui_buffers, -1, RELAY_API_COLORS_WEECHAT);
    CHECK(json);
    CHECK(relay_json_is_array (json));
    LONGS_EQUAL(1, relay_json_array_size (json));
    json_line = relay_json_array_get (json, 0);
    CHECK(json_line);
    CHECK(relay_json_is_object (json_line));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->data->id,
                      json_line, "id");
    WEE_CHECK_OBJ_STR(str_msg2, json_line, "message");
    relay_json_free (json);

    /* One line without colors */
    json = relay_api_msg_lines_to_json (gui_buffers, -1, RELAY_API_COLORS_STRIP);
    CHECK(json);
    CHECK(relay_json_is_array (json));
    LONGS_EQUAL(1, relay_json_array_size (json));
    json_line = relay_json_array_get (json, 0);
    CHECK(json_line);
    CHECK(relay_json_is_object (json_line));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->data->id,
                      json_line, "id");
    WEE_CHECK_OBJ_STR("this is the second line with green", json_line, "message");
    relay_json_free (json);
}

/*
 * Test functions:
 *   relay_api_msg_completion_to_json
 */

TEST(RelayApiMsg, CompletionToJson)
{
    struct t_relay_json *json, *json_obj, *json_item;

    // check empty json result
    json = relay_api_msg_completion_to_json (NULL);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    POINTERS_EQUAL(NULL, relay_json_object_get (json, "priority"));
    relay_json_free (json);

    // set example input
    gui_buffer_set (gui_buffers, "input", "/co");
    gui_buffer_set (gui_buffers, "input_pos", "3");

    // perform completion
    gui_input_complete_next (gui_buffers);
    STRCMP_EQUAL("/color ", gui_buffers->input_buffer);

    // convert to json
    json = relay_api_msg_completion_to_json (gui_buffers->completion);
    CHECK(json);
    CHECK(relay_json_is_object (json));

    json_obj = relay_json_object_get (json, "context");
    CHECK(json_obj);
    CHECK(relay_json_is_string (json_obj));
    STRCMP_EQUAL("command", relay_json_get_string (json_obj));

    json_obj = relay_json_object_get (json, "base_word");
    CHECK(json_obj);
    CHECK(relay_json_is_string (json_obj));
    STRCMP_EQUAL("co", relay_json_get_string (json_obj));

    json_obj = relay_json_object_get (json, "position_replace");
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    CHECK_EQUAL(1, test_relay_api_msg_json_number (json_obj));

    json_obj = relay_json_object_get (json, "add_space");
    CHECK(json_obj);
    CHECK(relay_json_is_bool (json_obj));
    CHECK(relay_json_is_true (json_obj));

    json_obj = relay_json_object_get (json, "list");
    CHECK(json_obj);
    CHECK(relay_json_is_array (json_obj));
    CHECK_EQUAL(3, relay_json_array_size (json_obj));
    json_item = relay_json_array_get (json_obj, 0);
    CHECK(json_item);
    CHECK(relay_json_is_string (json_item));
    STRCMP_EQUAL("color", relay_json_get_string (json_item));
    json_item = relay_json_array_get (json_obj, 1);
    CHECK(json_item);
    CHECK(relay_json_is_string (json_item));
    STRCMP_EQUAL("command", relay_json_get_string (json_item));
    json_item = relay_json_array_get (json_obj, 2);
    CHECK(json_item);
    CHECK(relay_json_is_string (json_item));
    STRCMP_EQUAL("connect", relay_json_get_string (json_item));

    relay_json_free (json);

    gui_buffer_set (gui_buffers, "input", "");
}

/*
 * Test functions:
 *   relay_api_msg_hotlist_to_json
 */

TEST(RelayApiMsg, HotlistToJson)
{
    char str_date[128], *str;
    struct t_relay_json *json, *json_obj, *json_count;
    time_t time_value;
    struct timeval tv;
    struct tm gm_time;
    long long old_buffer_id;

    json = relay_api_msg_hotlist_to_json (NULL);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    POINTERS_EQUAL(NULL, relay_json_object_get (json, "priority"));
    relay_json_free (json);

    gui_hotlist_add (gui_buffers, GUI_HOTLIST_LOW, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_MESSAGE, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_MESSAGE, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_PRIVATE, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_PRIVATE, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_PRIVATE, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_HIGHLIGHT, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_HIGHLIGHT, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_HIGHLIGHT, NULL, 0);
    gui_hotlist_add (gui_buffers, GUI_HOTLIST_HIGHLIGHT, NULL, 0);

    json = relay_api_msg_hotlist_to_json (gui_hotlist);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    WEE_CHECK_OBJ_NUM(int(GUI_HOTLIST_HIGHLIGHT), json, "priority");
    time_value = hdata_time (relay_hdata_hotlist, gui_hotlist, "time");
    gmtime_r (&time_value, &gm_time);
    tv.tv_sec = mktime (&gm_time);
    tv.tv_usec = hdata_integer (relay_hdata_hotlist, gui_hotlist, "time_usec");
    util_strftimeval (str_date, sizeof (str_date), "%@%FT%T.%fZ", &tv);
    WEE_CHECK_OBJ_STR(str_date, json, "date");
    WEE_CHECK_OBJ_NUM(gui_buffers->id, json, "buffer_id");
    json_count = relay_json_object_get (json, "count");
    CHECK(json_count);
    CHECK(relay_json_is_array (json_count));
    json_obj = relay_json_array_get (json_count, 0);
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    CHECK(1 == test_relay_api_msg_json_number (json_obj));
    json_obj = relay_json_array_get (json_count, 1);
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    CHECK(2 == test_relay_api_msg_json_number (json_obj));
    json_obj = relay_json_array_get (json_count, 2);
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    CHECK(3 == test_relay_api_msg_json_number (json_obj));
    json_obj = relay_json_array_get (json_count, 3);
    CHECK(json_obj);
    CHECK(relay_json_is_number (json_obj));
    CHECK(4 == test_relay_api_msg_json_number (json_obj));
    relay_json_free (json);

    /* buffer identifier must not be printed with exponent (like 1.7e+15) */
    old_buffer_id = gui_buffers->id;
    gui_buffers->id = 1709932823238640LL;
    json = relay_api_msg_hotlist_to_json (gui_hotlist);
    str = relay_json_print (json);
    CHECK(strstr (str, "\"buffer_id\":1709932823238640,"));
    free (str);
    relay_json_free (json);
    gui_buffers->id = old_buffer_id;

    gui_hotlist_remove_buffer (gui_buffers, 1);
}

/*
 * Test functions:
 *   relay_api_msg_script_to_json
 */

TEST(RelayApiMsg, ScriptToJson)
{
    struct t_hdata *ptr_hdata;
    void *ptr_script;
    struct t_relay_json *json, *json_obj;
    char path_testapigen[PATH_MAX], path_testapi[PATH_MAX];
    char *test_scripts_dir, str_command[(PATH_MAX * 2) + 128];
    const char *ptr_test_scripts_dir;

    POINTERS_EQUAL(NULL, relay_api_msg_script_to_json (NULL, NULL, NULL));

    ptr_hdata = hook_hdata_get (NULL, "python_script");

    json = relay_api_msg_script_to_json (ptr_hdata, NULL, NULL);
    CHECK(json);
    CHECK(relay_json_is_object (json));
    relay_json_free (json);

    /* Load a python script for this test. */
    ptr_test_scripts_dir = getenv ("WEECHAT_TESTS_SCRIPTS_DIR");
    test_scripts_dir = strdup (
        (ptr_test_scripts_dir) ?
        ptr_test_scripts_dir : "./scripts/python");
    snprintf (path_testapigen, sizeof (path_testapigen),
              "%s%s%s",
              test_scripts_dir,
              DIR_SEPARATOR,
              "testapigen.py");
    snprintf (path_testapi, sizeof (path_testapi),
              "%s%s%s",
              test_scripts_dir,
              DIR_SEPARATOR,
              "testapi.py");
    snprintf (str_command, sizeof (str_command),
              "/script load %s", path_testapigen);
    run_cmd (str_command);

    ptr_script = hdata_get_list (ptr_hdata, "scripts");
    CHECK(ptr_script);
    json = relay_api_msg_script_to_json (ptr_hdata, ptr_script, "py");
    CHECK(json);
    CHECK(relay_json_is_object (json));
    WEE_CHECK_OBJ_STR("testapigen.py", json, "name");
    WEE_CHECK_OBJ_STR("0.1", json, "version");
    WEE_CHECK_OBJ_STR("Generate scripting API test scripts", json, "description");
    WEE_CHECK_OBJ_STR("Sébastien Helleu <flashcode@flashtux.org>", json, "author");
    WEE_CHECK_OBJ_STR("GPL3", json, "license");
    relay_json_free (json);

    /* Unload script. */
    snprintf (str_command, sizeof (str_command),
              "/script unload -q weechat_testapi.py");
    run_cmd (str_command);
}
