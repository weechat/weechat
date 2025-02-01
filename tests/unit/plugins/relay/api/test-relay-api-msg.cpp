/*
 * test-relay-api-msg.cpp - test relay API protocol (messages)
 *
 * Copyright (C) 2024-2025 Sébastien Helleu <flashcode@flashtux.org>
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

extern "C"
{
#include <time.h>
#include <sys/time.h>
#include <cjson/cJSON.h>
#include "src/core/core-hdata.h"
#include "src/core/core-util.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-hotlist.h"
#include "src/gui/gui-input.h"
#include "src/gui/gui-line.h"
#include "src/gui/gui-nicklist.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-client.h"
#include "src/plugins/relay/api/relay-api.h"
#include "src/plugins/relay/api/relay-api-msg.h"
}

#define WEE_CHECK_OBJ_STR(__expected, __json, __name)                   \
    json_obj = cJSON_GetObjectItem (__json, __name);                    \
    CHECK(json_obj);                                                    \
    CHECK(cJSON_IsString (json_obj));                                   \
    STRCMP_EQUAL(__expected, cJSON_GetStringValue (json_obj));

#define WEE_CHECK_OBJ_STRN(__expected, __length, __json, __name)        \
    json_obj = cJSON_GetObjectItem (__json, __name);                    \
    CHECK(json_obj);                                                    \
    CHECK(cJSON_IsString (json_obj));                                   \
    STRNCMP_EQUAL(__expected, cJSON_GetStringValue (json_obj),          \
                  __length);

#define WEE_CHECK_OBJ_NUM(__expected, __json, __name)                   \
    json_obj = cJSON_GetObjectItem (__json, __name);                    \
    CHECK(json_obj);                                                    \
    CHECK(cJSON_IsNumber (json_obj));                                   \
    CHECK(__expected == cJSON_GetNumberValue (json_obj));

#define WEE_CHECK_OBJ_BOOL(__expected, __json, __name)                  \
    json_obj = cJSON_GetObjectItem (__json, __name);                    \
    CHECK(json_obj);                                                    \
    CHECK(cJSON_IsBool (json_obj));                                     \
    LONGS_EQUAL(__expected, cJSON_IsTrue (json_obj) ? 1 : 0);

TEST_GROUP(RelayApiMsg)
{
};

/*
 * Tests functions:
 *   relay_api_msg_send_json_internal
 */

TEST(RelayApiMsg, SendJsonInternal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_msg_send_json
 */

TEST(RelayApiMsg, SendJson)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_msg_send_error_json
 */

TEST(RelayApiMsg, SendErrorJson)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_msg_send_event
 */

TEST(RelayApiMsg, SendEvent)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_api_msg_buffer_add_local_vars_cb
 *   relay_api_msg_buffer_to_json
 *   relay_api_msg_nick_to_json
 *   relay_api_msg_nick_group_to_json
 */

TEST(RelayApiMsg, BufferToJson)
{
    cJSON *json, *json_obj, *json_local_vars, *json_keys, *json_key;
    cJSON *json_lines, *json_line;
    cJSON *json_nicklist_root, *json_nicks, *json_groups, *json_group;
    cJSON *json_group_nicks, *json_nick;
    struct t_gui_buffer *buffer;
    struct t_gui_nick_group *group;
    long long group_id;
    char *color;

    json = relay_api_msg_buffer_to_json (NULL, 0L, 0L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    POINTERS_EQUAL(NULL, cJSON_GetObjectItem (json, "name"));
    cJSON_Delete (json);

    gui_buffer_set (gui_buffers, "key_bind_meta-y,1", "/test1");
    gui_buffer_set (gui_buffers, "key_bind_meta-y,2", "/test2 arg");

    /* buffer without lines and nicks */
    json = relay_api_msg_buffer_to_json (gui_buffers, 0L, 0L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    WEE_CHECK_OBJ_NUM(gui_buffers->id, json, "id");
    WEE_CHECK_OBJ_STR("core.weechat", json, "name");
    WEE_CHECK_OBJ_STR("weechat", json, "short_name");
    WEE_CHECK_OBJ_NUM(1, json, "number");
    WEE_CHECK_OBJ_STR("formatted", json, "type");
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
    json_local_vars = cJSON_GetObjectItem (json, "local_variables");
    CHECK(json_local_vars);
    CHECK(cJSON_IsObject (json_local_vars));
    json_keys = cJSON_GetObjectItem (json, "keys");
    CHECK(json_keys);
    LONGS_EQUAL(2, cJSON_GetArraySize (json_keys));
    json_key = cJSON_GetArrayItem (json_keys, 0);
    CHECK(json_key);
    WEE_CHECK_OBJ_STR("meta-y,1", json_key, "key");
    WEE_CHECK_OBJ_STR("/test1", json_key, "command");
    json_key = cJSON_GetArrayItem (json_keys, 1);
    CHECK(json_key);
    WEE_CHECK_OBJ_STR("meta-y,2", json_key, "key");
    WEE_CHECK_OBJ_STR("/test2 arg", json_key, "command");
    WEE_CHECK_OBJ_STR("core", json_local_vars, "plugin");
    WEE_CHECK_OBJ_STR("weechat", json_local_vars, "name");
    POINTERS_EQUAL(NULL, cJSON_GetObjectItem (json, "lines"));
    POINTERS_EQUAL(NULL, cJSON_GetObjectItem (json, "nicks"));
    cJSON_Delete (json);

    gui_buffer_hide (gui_buffers);
    gui_buffer_set_time_for_each_line (gui_buffers, 0);

    json = relay_api_msg_buffer_to_json (gui_buffers, 0L, 0L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    WEE_CHECK_OBJ_BOOL(1, json, "hidden");
    WEE_CHECK_OBJ_BOOL(0, json, "time_displayed");
    cJSON_Delete (json);

    gui_buffer_unhide (gui_buffers);
    gui_buffer_set_time_for_each_line (gui_buffers, 1);

    /* buffer with 2 lines, without nicks */
    json = relay_api_msg_buffer_to_json (gui_buffers, 2L, 0L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    json_lines = cJSON_GetObjectItem (json, "lines");
    CHECK(json_lines);
    CHECK(cJSON_IsArray (json_lines));
    LONGS_EQUAL(2, cJSON_GetArraySize (json_lines));
    cJSON_Delete (json);

    /* create a user buffer with 1 group / 4 nicks */
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

    /* buffer with no lines and 1 group / 4 nicks */
    json = relay_api_msg_buffer_to_json (buffer, 1L, 0L, 1, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    WEE_CHECK_OBJ_BOOL(1, json, "nicklist");
    WEE_CHECK_OBJ_BOOL(0, json, "nicklist_case_sensitive");
    WEE_CHECK_OBJ_BOOL(0, json, "nicklist_display_groups");
    json_lines = cJSON_GetObjectItem (json, "lines");
    CHECK(json_lines);
    CHECK(cJSON_IsArray (json_lines));
    LONGS_EQUAL(0, cJSON_GetArraySize (json_lines));
    json_nicklist_root = cJSON_GetObjectItem (json, "nicklist_root");
    CHECK(json_nicklist_root);
    CHECK(cJSON_IsObject (json_nicklist_root));
    WEE_CHECK_OBJ_NUM(0, json_nicklist_root, "id");
    WEE_CHECK_OBJ_STR("root", json_nicklist_root, "name");
    WEE_CHECK_OBJ_STR("", json_nicklist_root, "color_name");
    WEE_CHECK_OBJ_STR("", json_nicklist_root, "color");
    json_groups = cJSON_GetObjectItem (json_nicklist_root, "groups");
    CHECK(json_groups);
    CHECK(cJSON_IsArray (json_groups));
    LONGS_EQUAL(1, cJSON_GetArraySize (json_groups));
    json_group = cJSON_GetArrayItem (json_groups, 0);
    CHECK(json_group);
    CHECK(cJSON_IsObject (json_group));
    json_obj = cJSON_GetObjectItem (json_group, "id");
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    group_id = cJSON_GetNumberValue (json_obj);
    CHECK(group_id > 0);
    json_obj = cJSON_GetObjectItem (json_group, "parent_group_id");
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(cJSON_GetNumberValue (json_obj) == 0);
    WEE_CHECK_OBJ_STR("group1", json_group, "name");
    WEE_CHECK_OBJ_STR("magenta", json_group, "color_name");
    color = gui_color_encode_ansi (gui_color_get_custom ("magenta"));
    WEE_CHECK_OBJ_STR(color, json_group, "color");
    free (color);
    json_group_nicks = cJSON_GetObjectItem (json_group, "nicks");
    CHECK(json_group_nicks);
    CHECK(cJSON_IsArray (json_group_nicks));
    LONGS_EQUAL(3, cJSON_GetArraySize (json_group_nicks));
    json_nick = cJSON_GetArrayItem (json_group_nicks, 0);
    CHECK(json_nick);
    CHECK(cJSON_IsObject (json_nick));
    json_obj = cJSON_GetObjectItem (json_nick, "id");
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(cJSON_GetNumberValue (json_obj) > 0);
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
    json_nick = cJSON_GetArrayItem (json_group_nicks, 1);
    CHECK(json_nick);
    CHECK(cJSON_IsObject (json_nick));
    json_obj = cJSON_GetObjectItem (json_nick, "id");
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(cJSON_GetNumberValue (json_obj) > 0);
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
    json_nick = cJSON_GetArrayItem (json_group_nicks, 2);
    CHECK(json_nick);
    CHECK(cJSON_IsObject (json_nick));
    json_obj = cJSON_GetObjectItem (json_nick, "id");
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(cJSON_GetNumberValue (json_obj) > 0);
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
    json_nicks = cJSON_GetObjectItem (json_nicklist_root, "nicks");
    CHECK(json_nicks);
    CHECK(cJSON_IsArray (json_nicks));
    LONGS_EQUAL(1, cJSON_GetArraySize (json_nicks));
    json_nick = cJSON_GetArrayItem (json_nicks, 0);
    CHECK(json_nick);
    CHECK(cJSON_IsObject (json_nick));
    json_obj = cJSON_GetObjectItem (json_nick, "id");
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(cJSON_GetNumberValue (json_obj) > 0);
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
    cJSON_Delete (json);

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
    CHECK(cJSON_IsObject (json));
    json_lines = cJSON_GetObjectItem (json, "lines");
    CHECK(json_lines);
    CHECK(cJSON_IsArray (json_lines));
    LONGS_EQUAL(2, cJSON_GetArraySize (json_lines));
    json_line = cJSON_GetArrayItem (json_lines, 0);
    CHECK(json_line);
    WEE_CHECK_OBJ_STR("test line 1", json_line, "message");
    json_line = cJSON_GetArrayItem (json_lines, 1);
    CHECK(json_line);
    WEE_CHECK_OBJ_STR("test line 2", json_line, "message");
    cJSON_Delete (json);

    json = relay_api_msg_buffer_to_json (buffer, 1L, -2L, 0, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    json_lines = cJSON_GetObjectItem (json, "lines");
    CHECK(json_lines);
    CHECK(cJSON_IsArray (json_lines));
    LONGS_EQUAL(2, cJSON_GetArraySize (json_lines));
    json_line = cJSON_GetArrayItem (json_lines, 0);
    CHECK(json_line);
    WEE_CHECK_OBJ_STR("test line 4", json_line, "message");
    json_line = cJSON_GetArrayItem (json_lines, 1);
    CHECK(json_line);
    WEE_CHECK_OBJ_STR("test line 5", json_line, "message");
    cJSON_Delete (json);

    gui_buffer_close (buffer);
}

/*
 * Tests functions:
 *   relay_api_msg_line_data_to_json
 *   relay_api_msg_lines_to_json
 */

TEST(RelayApiMsg, LinesToJson)
{
    char str_msg1[1024], str_msg2[1024], *str_msg_ansi, str_date[128];
    cJSON *json, *json_line, *json_obj, *json_tags, *json_tag;
    struct timeval tv;
    struct tm gm_time;

    snprintf (str_msg1, sizeof (str_msg1), "%s", "this is the first line");
    gui_chat_printf_date_tags (NULL, 0, "tag1,tag2,tag3",
                               "%s\t%s", "nick1", str_msg1);

    snprintf (str_msg2, sizeof (str_msg2),
              "this is the second line with %s" "green",
              gui_color_get_custom ("green"));
    gui_chat_printf (NULL, "%s", str_msg2);

    /* two lines with ANSI colors */
    json = relay_api_msg_lines_to_json (gui_buffers, -2, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(cJSON_IsArray (json));
    LONGS_EQUAL(2, cJSON_GetArraySize (json));
    /* first line */
    json_line = cJSON_GetArrayItem (json, 0);
    CHECK(json_line);
    CHECK(cJSON_IsObject (json_line));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->prev_line->data->id,
                      json_line, "id");
    WEE_CHECK_OBJ_NUM(-1, json_line, "y");
    gmtime_r (&(gui_buffers->own_lines->last_line->prev_line->data->date), &gm_time);
    tv.tv_sec = mktime (&gm_time);
    tv.tv_usec = gui_buffers->own_lines->last_line->prev_line->data->date_usec;
    util_strftimeval (str_date, sizeof (str_date), "%FT%T.%fZ", &tv);
    WEE_CHECK_OBJ_STR(str_date, json_line, "date");
    gmtime_r (&(gui_buffers->own_lines->last_line->prev_line->data->date_printed), &gm_time);
    tv.tv_sec = mktime (&gm_time);
    tv.tv_usec = gui_buffers->own_lines->last_line->prev_line->data->date_usec_printed;
    util_strftimeval (str_date, sizeof (str_date), "%FT%T.%fZ", &tv);
    WEE_CHECK_OBJ_STR(str_date, json_line, "date_printed");
    WEE_CHECK_OBJ_BOOL(0, json_line, "highlight");
    WEE_CHECK_OBJ_STR("nick1", json_line, "prefix");
    WEE_CHECK_OBJ_STR(str_msg1, json_line, "message");
    json_tags = cJSON_GetObjectItem (json_line, "tags");
    CHECK(json_tags);
    CHECK(cJSON_IsArray (json_tags));
    LONGS_EQUAL(3, cJSON_GetArraySize (json_tags));
    json_tag = cJSON_GetArrayItem (json_tags, 0);
    CHECK(json_tag);
    CHECK(cJSON_IsString (json_tag));
    STRCMP_EQUAL("tag1", cJSON_GetStringValue (json_tag));
    json_tag = cJSON_GetArrayItem (json_tags, 1);
    CHECK(json_tag);
    CHECK(cJSON_IsString (json_tag));
    STRCMP_EQUAL("tag2", cJSON_GetStringValue (json_tag));
    json_tag = cJSON_GetArrayItem (json_tags, 2);
    CHECK(json_tag);
    CHECK(cJSON_IsString (json_tag));
    STRCMP_EQUAL("tag3", cJSON_GetStringValue (json_tag));
    /* second line */
    json_line = cJSON_GetArrayItem (json, 1);
    CHECK(json_line);
    CHECK(cJSON_IsObject (json_line));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->data->id,
                      json_line, "id");
    WEE_CHECK_OBJ_NUM(-1, json_line, "y");
    gmtime_r (&(gui_buffers->own_lines->last_line->data->date), &gm_time);
    tv.tv_sec = mktime (&gm_time);
    tv.tv_usec = gui_buffers->own_lines->last_line->data->date_usec;
    util_strftimeval (str_date, sizeof (str_date), "%FT%T.%fZ", &tv);
    WEE_CHECK_OBJ_STR(str_date, json_line, "date");
    gmtime_r (&(gui_buffers->own_lines->last_line->data->date_printed), &gm_time);
    tv.tv_sec = mktime (&gm_time);
    tv.tv_usec = gui_buffers->own_lines->last_line->data->date_usec_printed;
    util_strftimeval (str_date, sizeof (str_date), "%FT%T.%fZ", &tv);
    WEE_CHECK_OBJ_STR(str_date, json_line, "date_printed");
    WEE_CHECK_OBJ_BOOL(0, json_line, "highlight");
    WEE_CHECK_OBJ_STR("", json_line, "prefix");
        str_msg_ansi = gui_color_encode_ansi (str_msg2);
    CHECK(str_msg_ansi);
    WEE_CHECK_OBJ_STR(str_msg_ansi, json_line, "message");
    free (str_msg_ansi);
    json_tags = cJSON_GetObjectItem (json_line, "tags");
    CHECK(json_tags);
    CHECK(cJSON_IsArray (json_tags));
    LONGS_EQUAL(0, cJSON_GetArraySize (json_tags));
    cJSON_Delete (json);

    /* with ANSI colors */
    json = relay_api_msg_lines_to_json (gui_buffers, -1, RELAY_API_COLORS_ANSI);
    CHECK(json);
    CHECK(cJSON_IsArray (json));
    LONGS_EQUAL(1, cJSON_GetArraySize (json));
    json_line = cJSON_GetArrayItem (json, 0);
    CHECK(json_line);
    CHECK(cJSON_IsObject (json_line));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->data->id,
                      json_line, "id");
    str_msg_ansi = gui_color_encode_ansi (str_msg2);
    CHECK(str_msg_ansi);
    WEE_CHECK_OBJ_STR(str_msg_ansi, json_line, "message");
    free (str_msg_ansi);
    cJSON_Delete (json);

    /* one line with WeeChat colors */
    json = relay_api_msg_lines_to_json (gui_buffers, -1, RELAY_API_COLORS_WEECHAT);
    CHECK(json);
    CHECK(cJSON_IsArray (json));
    LONGS_EQUAL(1, cJSON_GetArraySize (json));
    json_line = cJSON_GetArrayItem (json, 0);
    CHECK(json_line);
    CHECK(cJSON_IsObject (json_line));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->data->id,
                      json_line, "id");
    WEE_CHECK_OBJ_STR(str_msg2, json_line, "message");
    cJSON_Delete (json);

    /* one line without colors */
    json = relay_api_msg_lines_to_json (gui_buffers, -1, RELAY_API_COLORS_STRIP);
    CHECK(json);
    CHECK(cJSON_IsArray (json));
    LONGS_EQUAL(1, cJSON_GetArraySize (json));
    json_line = cJSON_GetArrayItem (json, 0);
    CHECK(json_line);
    CHECK(cJSON_IsObject (json_line));
    WEE_CHECK_OBJ_NUM(gui_buffers->own_lines->last_line->data->id,
                      json_line, "id");
    WEE_CHECK_OBJ_STR("this is the second line with green", json_line, "message");
    cJSON_Delete (json);
}

/*
 * Tests functions:
 *   relay_api_msg_completion_to_json
 */

TEST(RelayApiMsg, CompletionToJson)
{
    cJSON *json, *json_obj, *json_item;

    // check empty json result
    json = relay_api_msg_completion_to_json (NULL);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    POINTERS_EQUAL(NULL, cJSON_GetObjectItem (json, "priority"));
    cJSON_Delete (json);

    // set example input
    gui_buffer_set (gui_buffers, "input", "/co");
    gui_buffer_set (gui_buffers, "input_pos", "3");

    // perform completion
    gui_input_complete_next (gui_buffers);
    STRCMP_EQUAL("/color ", gui_buffers->input_buffer);

    // convert to json
    json = relay_api_msg_completion_to_json (gui_buffers->completion);
    CHECK(json);
    CHECK(cJSON_IsObject (json));

    json_obj = cJSON_GetObjectItem (json, "context");
    CHECK(json_obj);
    CHECK(cJSON_IsString (json_obj));
    STRCMP_EQUAL("command", cJSON_GetStringValue (json_obj));

    json_obj = cJSON_GetObjectItem (json, "base_word");
    CHECK(json_obj);
    CHECK(cJSON_IsString (json_obj));
    STRCMP_EQUAL("co", cJSON_GetStringValue (json_obj));

    json_obj = cJSON_GetObjectItem (json, "position_replace");
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK_EQUAL(1, cJSON_GetNumberValue (json_obj));

    json_obj = cJSON_GetObjectItem (json, "add_space");
    CHECK(json_obj);
    CHECK(cJSON_IsBool (json_obj));
    CHECK(cJSON_IsTrue (json_obj));

    json_obj = cJSON_GetObjectItem (json, "list");
    CHECK(json_obj);
    CHECK(cJSON_IsArray (json_obj));
    CHECK_EQUAL(3, cJSON_GetArraySize (json_obj));
    json_item = cJSON_GetArrayItem (json_obj, 0);
    CHECK(json_item);
    CHECK(cJSON_IsString (json_item));
    STRCMP_EQUAL("color", cJSON_GetStringValue (json_item));
    json_item = cJSON_GetArrayItem (json_obj, 1);
    CHECK(json_item);
    CHECK(cJSON_IsString (json_item));
    STRCMP_EQUAL("command", cJSON_GetStringValue (json_item));
    json_item = cJSON_GetArrayItem (json_obj, 2);
    CHECK(json_item);
    CHECK(cJSON_IsString (json_item));
    STRCMP_EQUAL("connect", cJSON_GetStringValue (json_item));

    cJSON_Delete (json);

    gui_buffer_set (gui_buffers, "input", "");
}

/*
 * Tests functions:
 *   relay_api_msg_hotlist_to_json
 */

TEST(RelayApiMsg, HotlistToJson)
{
    char str_date[128];
    cJSON *json, *json_obj, *json_count;
    time_t time_value;
    struct timeval tv;
    struct tm gm_time;

    json = relay_api_msg_hotlist_to_json (NULL);
    CHECK(json);
    CHECK(cJSON_IsObject (json));
    POINTERS_EQUAL(NULL, cJSON_GetObjectItem (json, "priority"));
    cJSON_Delete (json);

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
    CHECK(cJSON_IsObject (json));
    WEE_CHECK_OBJ_NUM(GUI_HOTLIST_HIGHLIGHT, json, "priority");
    time_value = hdata_time (relay_hdata_hotlist, gui_hotlist, "time");
    gmtime_r (&time_value, &gm_time);
    tv.tv_sec = mktime (&gm_time);
    tv.tv_usec = hdata_integer (relay_hdata_hotlist, gui_hotlist, "time_usec");
    util_strftimeval (str_date, sizeof (str_date), "%FT%T.%fZ", &tv);
    WEE_CHECK_OBJ_STR(str_date, json, "date");
    WEE_CHECK_OBJ_NUM(gui_buffers->id, json, "buffer_id");
    json_count = cJSON_GetObjectItem (json, "count");
    CHECK(json_count);
    CHECK(cJSON_IsArray (json_count));
    json_obj = cJSON_GetArrayItem (json_count, 0);
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(1 == cJSON_GetNumberValue (json_obj));
    json_obj = cJSON_GetArrayItem (json_count, 1);
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(2 == cJSON_GetNumberValue (json_obj));
    json_obj = cJSON_GetArrayItem (json_count, 2);
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(3 == cJSON_GetNumberValue (json_obj));
    json_obj = cJSON_GetArrayItem (json_count, 3);
    CHECK(json_obj);
    CHECK(cJSON_IsNumber (json_obj));
    CHECK(4 == cJSON_GetNumberValue (json_obj));
    cJSON_Delete (json);

    gui_hotlist_remove_buffer (gui_buffers, 1);
}
