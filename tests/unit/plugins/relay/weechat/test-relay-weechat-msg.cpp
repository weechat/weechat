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
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
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
    msg = relay_weechat_msg_new ("test");                               \
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

TEST_GROUP(RelayWeechatMsg)
{
};

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
