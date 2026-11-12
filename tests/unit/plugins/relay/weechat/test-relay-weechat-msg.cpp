/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test relay WeeChat protocol (messages) */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/core/hook/hook-infolist.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/plugins/irc/irc-channel.h"
#include "src/plugins/relay/weechat/relay-weechat-msg.h"
}

/*
 * Get a string in a message at a given position (the string is
 * prefixed by its length on 4 bytes).
 *
 * Position is updated to the byte after the string.
 *
 * Note: result must be freed after use.
 */

char *
test_relay_weechat_msg_get_string (struct t_relay_weechat_msg *msg,
                                   int *position)
{
    uint32_t length32;
    int length;
    char *string;

    if (*position + 4 > msg->data_size)
        return NULL;
    memcpy (&length32, msg->data + *position, 4);
    length = (int)ntohl (length32);
    *position += 4;
    if ((length < 0) || (*position + length > msg->data_size))
        return NULL;
    string = (char *)malloc (length + 1);
    if (!string)
        return NULL;
    memcpy (string, msg->data + *position, length);
    string[length] = '\0';
    *position += length;
    return string;
}

/*
 * Get path of hdata added in a message (first object after the message id).
 *
 * Note: result must be freed after use.
 */

char *
test_relay_weechat_msg_get_hdata_path (struct t_relay_weechat_msg *msg)
{
    char *id;
    int position;

    /* Skip size (4 bytes) and compression flag (1 byte). */
    position = 5;

    id = test_relay_weechat_msg_get_string (msg, &position);
    free (id);

    if ((position + 3 > msg->data_size)
        || (strncmp (msg->data + position, RELAY_WEECHAT_MSG_OBJ_HDATA, 3) != 0))
    {
        return NULL;
    }
    position += 3;

    return test_relay_weechat_msg_get_string (msg, &position);
}

#define WEE_CHECK_ADD_HDATA(__result, __path_returned, __path, __keys)  \
    msg = relay_weechat_msg_new (NULL, "test");                         \
    CHECK(msg);                                                         \
    LONGS_EQUAL(__result,                                               \
                relay_weechat_msg_add_hdata (msg, __path, __keys));     \
    path_returned = test_relay_weechat_msg_get_hdata_path (msg);        \
    if (__path_returned == NULL)                                        \
    {                                                                   \
        POINTERS_EQUAL(NULL, path_returned);                            \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__path_returned, path_returned);                   \
    }                                                                   \
    free (path_returned);                                               \
    relay_weechat_msg_free (msg);

/*
 * Get a short string in a message at a given position (the string is
 * prefixed by its length on 1 byte), used for types "lon", "ptr", "tim".
 *
 * Position is updated to the byte after the string.
 */

void
test_relay_weechat_msg_get_short_string (struct t_relay_weechat_msg *msg,
                                         int *position,
                                         char *string, int size)
{
    int length;

    string[0] = '\0';
    if (*position + 1 > msg->data_size)
        return;
    length = (unsigned char)msg->data[*position];
    *position += 1;
    if ((length >= size) || (*position + length > msg->data_size))
        return;
    memcpy (string, msg->data + *position, length);
    string[length] = '\0';
    *position += length;
}

/*
 * Get an integer (4 bytes) in a message at a given position.
 *
 * Position is updated to the byte after the integer.
 */

int
test_relay_weechat_msg_get_int (struct t_relay_weechat_msg *msg,
                                int *position)
{
    uint32_t value32;

    if (*position + 4 > msg->data_size)
        return -1;
    memcpy (&value32, msg->data + *position, 4);
    *position += 4;
    return (int)ntohl (value32);
}

#define WEE_CHECK_INFOLIST_VAR(__name, __type)                          \
    str = test_relay_weechat_msg_get_string (msg, &position);           \
    STRCMP_EQUAL(__name, str);                                          \
    free (str);                                                         \
    CHECK(position + 3 <= msg->data_size);                              \
    STRNCMP_EQUAL(__type, msg->data + position, 3);                     \
    position += 3;

TEST_GROUP(RelayWeechatMsg)
{
    /*
     * Return an infolist with one item and variables of all types
     * (callback used in tests).
     */

    static struct t_infolist *
    test_infolist_cb (const void *pointer, void *data,
                      const char *infolist_name, void *obj_pointer,
                      const char *arguments)
    {
        struct t_infolist *infolist;
        struct t_infolist_item *ptr_item;

        /* Make C++ compiler happy. */
        (void) pointer;
        (void) data;
        (void) infolist_name;
        (void) obj_pointer;
        (void) arguments;

        infolist = infolist_new (NULL);
        ptr_item = infolist_new_item (infolist);
        infolist_new_var_integer (ptr_item, "int", -123);
        infolist_new_var_string (ptr_item, "str", "abc");
        infolist_new_var_pointer (ptr_item, "ptr", (void *)0x1234abcd);
        infolist_new_var_buffer (ptr_item, "buf", (void *)"xyz", 3);
        infolist_new_var_time (ptr_item, "time", 1321993456);
        infolist_new_var_long (ptr_item, "long", -1234567890L);
        infolist_new_var_longlong (ptr_item, "longlong", -9876543210123LL);

        return infolist;
    }
};

/*
 * Test functions:
 *   relay_weechat_msg_new
 *   relay_weechat_msg_add_string
 */

TEST(RelayWeechatMsg, AddStringRgbColors)
{
    struct t_relay_weechat_msg *msg;
    char string[256], expected[256], *str;
    int position, fg, rgb_colors;

    fg = gui_color_convert_rgb_to_term_display (0xff8800, 256);

    /*
     * RGB colors with attributes (as built for IRC hex colors and ANSI
     * colors), the string does not contain "@#"
     */
    snprintf (string, sizeof (string), "%s" "a" "%s" "b #ff8800",
              "\x19" "F@|#ff8800", "\x19" "*@*#ff8800~@#000080");
    snprintf (expected, sizeof (expected), "%s%05d" "a" "%s%05d~@%05d" "b #ff8800",
              "\x19" "F@|", fg, "\x19" "*@*", fg,
              gui_color_convert_rgb_to_term_display (0x000080, 256));

    for (rgb_colors = 0; rgb_colors <= 1; rgb_colors++)
    {
        msg = relay_weechat_msg_new (NULL, "test");
        CHECK(msg);
        LONGS_EQUAL(0, msg->rgb_colors);
        msg->rgb_colors = rgb_colors;
        position = msg->data_size;
        relay_weechat_msg_add_string (msg, string);
        str = test_relay_weechat_msg_get_string (msg, &position);
        /*
         * Without client or if client does not support RGB colors: replaced
         * by nearest terminal color; otherwise string unchanged.
         */
        STRCMP_EQUAL((rgb_colors) ? string : expected, str);
        free (str);
        relay_weechat_msg_free (msg);
    }
}

/*
 * Test functions:
 *   relay_weechat_msg_add_color_name
 */

TEST(RelayWeechatMsg, AddColorNameRgbColors)
{
    struct t_relay_weechat_msg *msg;
    char expected[64], *str;
    int position, rgb_colors;

    snprintf (expected, sizeof (expected), "*%d:red",
              gui_color_convert_rgb_to_term_display (0xff8800, 256));

    for (rgb_colors = 0; rgb_colors <= 1; rgb_colors++)
    {
        msg = relay_weechat_msg_new (NULL, "test");
        CHECK(msg);
        msg->rgb_colors = rgb_colors;
        position = msg->data_size;
        relay_weechat_msg_add_color_name (msg, "*#ff8800:red");
        relay_weechat_msg_add_color_name (msg, "lightred");
        relay_weechat_msg_add_color_name (msg, NULL);
        str = test_relay_weechat_msg_get_string (msg, &position);
        STRCMP_EQUAL((rgb_colors) ? "*#ff8800:red" : expected, str);
        free (str);
        str = test_relay_weechat_msg_get_string (msg, &position);
        STRCMP_EQUAL("lightred", str);
        free (str);
        POINTERS_EQUAL(NULL, test_relay_weechat_msg_get_string (msg, &position));
        relay_weechat_msg_free (msg);
    }
}

/*
 * Test functions:
 *   relay_weechat_msg_add_infolist
 */

TEST(RelayWeechatMsg, AddInfolist)
{
    struct t_relay_weechat_msg *msg;
    struct t_hook *hook;
    char *str, value[256];
    int position, size_before;

    hook = hook_infolist (NULL, "test_relay_msg", "", "", "",
                          &test_infolist_cb, NULL, NULL);
    CHECK(hook);

    msg = relay_weechat_msg_new (NULL, "test");
    CHECK(msg);

    /* Unknown infolist: nothing added. */
    size_before = msg->data_size;
    relay_weechat_msg_add_infolist (msg, "test_relay_msg_unknown", NULL,
                                    NULL);
    LONGS_EQUAL(size_before, msg->data_size);

    relay_weechat_msg_add_infolist (msg, "test_relay_msg", NULL, NULL);

    /* Skip size (4 bytes), compression flag (1 byte) and id. */
    position = 5;
    str = test_relay_weechat_msg_get_string (msg, &position);
    STRCMP_EQUAL("test", str);
    free (str);

    STRNCMP_EQUAL(RELAY_WEECHAT_MSG_OBJ_INFOLIST, msg->data + position, 3);
    position += 3;
    str = test_relay_weechat_msg_get_string (msg, &position);
    STRCMP_EQUAL("test_relay_msg", str);
    free (str);
    LONGS_EQUAL(1, test_relay_weechat_msg_get_int (msg, &position));
    LONGS_EQUAL(7, test_relay_weechat_msg_get_int (msg, &position));

    WEE_CHECK_INFOLIST_VAR("int", RELAY_WEECHAT_MSG_OBJ_INT);
    LONGS_EQUAL(-123, test_relay_weechat_msg_get_int (msg, &position));

    WEE_CHECK_INFOLIST_VAR("str", RELAY_WEECHAT_MSG_OBJ_STRING);
    str = test_relay_weechat_msg_get_string (msg, &position);
    STRCMP_EQUAL("abc", str);
    free (str);

    WEE_CHECK_INFOLIST_VAR("ptr", RELAY_WEECHAT_MSG_OBJ_POINTER);
    test_relay_weechat_msg_get_short_string (msg, &position,
                                             value, sizeof (value));
    STRCMP_EQUAL("1234abcd", value);

    WEE_CHECK_INFOLIST_VAR("buf", RELAY_WEECHAT_MSG_OBJ_BUFFER);
    str = test_relay_weechat_msg_get_string (msg, &position);
    STRCMP_EQUAL("xyz", str);
    free (str);

    WEE_CHECK_INFOLIST_VAR("time", RELAY_WEECHAT_MSG_OBJ_TIME);
    test_relay_weechat_msg_get_short_string (msg, &position,
                                             value, sizeof (value));
    STRCMP_EQUAL("1321993456", value);

    WEE_CHECK_INFOLIST_VAR("long", RELAY_WEECHAT_MSG_OBJ_LONG);
    test_relay_weechat_msg_get_short_string (msg, &position,
                                             value, sizeof (value));
    STRCMP_EQUAL("-1234567890", value);

    WEE_CHECK_INFOLIST_VAR("longlong", RELAY_WEECHAT_MSG_OBJ_LONG);
    test_relay_weechat_msg_get_short_string (msg, &position,
                                             value, sizeof (value));
    STRCMP_EQUAL("-9876543210123", value);

    /* End of message. */
    LONGS_EQUAL(msg->data_size, position);

    relay_weechat_msg_free (msg);

    unhook (hook);
}

/*
 * Test functions:
 *   relay_weechat_msg_add_hdata
 */

TEST(RelayWeechatMsg, AddHdata)
{
    struct t_relay_weechat_msg *msg;
    struct t_irc_channel_speaking speaking;
    char *path_returned, **path, **path_expected, str_pointer[64];
    int i;

    /* Invalid paths. */
    WEE_CHECK_ADD_HDATA(0, NULL, "", NULL);
    WEE_CHECK_ADD_HDATA(0, NULL, "buffer", NULL);
    WEE_CHECK_ADD_HDATA(0, NULL, "xxx:gui_buffers", NULL);
    WEE_CHECK_ADD_HDATA(0, NULL, "buffer:xxx", NULL);
    WEE_CHECK_ADD_HDATA(0, NULL, "buffer:gui_buffers/xxx", NULL);
    WEE_CHECK_ADD_HDATA(0, NULL, "buffer:gui_buffers", "xxx");

    /* Valid paths. */
    WEE_CHECK_ADD_HDATA(1, "buffer", "buffer:gui_buffers", "number");
    WEE_CHECK_ADD_HDATA(1, "buffer", "buffer:gui_buffers(*)", "number");
    WEE_CHECK_ADD_HDATA(1, "buffer/lines/line/line_data",
                        "buffer:gui_buffers/lines/first_line/data",
                        "message");
    WEE_CHECK_ADD_HDATA(1, "buffer/lines/line/line_data",
                        "buffer:gui_buffers(*)/lines/first_line(-2)/data",
                        "message");
    snprintf (str_pointer, sizeof (str_pointer),
              "buffer:%p/lines/last_line/data", gui_buffers);
    WEE_CHECK_ADD_HDATA(1, "buffer/lines/line/line_data", str_pointer,
                        "message");

    /*
     * Path returned longer than twice the path received: each "/next_nick"
     * (10 bytes) is replaced by "/irc_channel_speaking" (21 bytes).
     */
    speaking.nick = (char *)"alice";
    speaking.time_last_message = 0;
    speaking.prev_nick = &speaking;
    speaking.next_nick = &speaking;
    path = string_dyn_alloc (256);
    path_expected = string_dyn_alloc (256);
    snprintf (str_pointer, sizeof (str_pointer),
              "irc_channel_speaking:%p", &speaking);
    string_dyn_concat (path, str_pointer, -1);
    string_dyn_concat (path_expected, "irc_channel_speaking", -1);
    for (i = 0; i < 100; i++)
    {
        string_dyn_concat (path, "/next_nick", -1);
        string_dyn_concat (path_expected, "/irc_channel_speaking", -1);
    }
    WEE_CHECK_ADD_HDATA(1, *path_expected, *path, "nick");
    string_dyn_free (path, 1);
    string_dyn_free (path_expected, 1);
}
