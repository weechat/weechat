/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC list functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "src/core/core-arraylist.h"
#include "src/core/core-config-file.h"
#include "src/core/core-hashtable.h"
#include "src/core/core-hdata.h"
#include "src/core/core-hook.h"
#include "src/core/hook/hook-command-run.h"
#include "src/core/hook/hook-hdata.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-line.h"
#include "src/gui/gui-window.h"
#include "src/plugins/plugin.h"
#include "src/plugins/irc/irc-config.h"
#include "src/plugins/irc/irc-list.h"
#include "src/plugins/irc/irc-server.h"

extern struct t_hdata *irc_list_hdata_list_channel;

extern int irc_list_compare_cb (void *data, struct t_arraylist *arraylist,
                                void *pointer1, void *pointer2);
extern void irc_list_free_cb (void *data, struct t_arraylist *arraylist,
                              void *pointer);
extern void irc_list_buffer_set_localvar_filter (struct t_gui_buffer *buffer,
                                                 struct t_irc_server *server);
extern void irc_list_set_filter (struct t_irc_server *server,
                                 const char *filter);
extern void irc_list_set_sort (struct t_irc_server *server, const char *sort);
extern void irc_list_add_channel_in_hashtable (struct t_hashtable *hashtable,
                                               struct t_irc_list_channel *channel);
extern int irc_list_string_match (const char *string, const char *mask);
extern int irc_list_channel_match_filter (struct t_irc_server *server,
                                          struct t_irc_list_channel *channel);
extern void irc_list_filter_channels (struct t_irc_server *server);
extern int irc_list_parse_messages (struct t_irc_server *server,
                                    const char *output);
extern void irc_list_display_line (struct t_irc_server *server, int line);
extern void irc_list_set_current_line (struct t_irc_server *server, int line);
extern void irc_list_get_window_info (struct t_gui_window *window,
                                      int *start_line_y, int *chat_height);
extern void irc_list_check_line_outside_window (struct t_irc_server *server);
extern int irc_list_mouse_hsignal_cb (const void *pointer, void *data,
                                      const char *signal,
                                      struct t_hashtable *hashtable);
}

#define LIST_MESSAGES                                                   \
    ":server 321 alice Channel :Users  Name\n"                          \
    ":server 322 alice #weechat 120 :WeeChat \x02" "chat\x02 client\n"  \
    ":server 322 alice ##Linux 300 :Linux discussions\n"                \
    ":server 322 alice #abc 5 :Some topic\n"                            \
    ":server 322 alice #test xyz :invalid count\n"                      \
    ":server 322 alice #short 2\n"                                      \
    ":server 323 alice :End of /LIST"

TEST_GROUP(IrcList)
{
};

TEST_GROUP(IrcListWithServer)
{
    struct t_irc_server *server;
    char commands[4096];
    struct t_hook *hook_commands;

    static int command_run_cb (const void *pointer, void *data,
                               struct t_gui_buffer *buffer,
                               const char *command)
    {
        char *ptr_commands;

        (void) data;
        (void) buffer;

        ptr_commands = (char *)pointer;
        if (ptr_commands[0])
            strcat (ptr_commands, "|");
        strcat (ptr_commands, command);

        return WEECHAT_RC_OK_EAT;
    }

    void setup ()
    {
        server = irc_server_alloc ("test_list");
        CHECK(server);
        if (!irc_list_hdata_list_channel)
            irc_list_hdata_list_channel = hook_hdata_get (NULL, "irc_list_channel");
        commands[0] = '\0';
        hook_commands = NULL;
    }

    void teardown ()
    {
        if (hook_commands)
            unhook (hook_commands);
        irc_server_free (server);
        config_file_option_reset (irc_config_look_list_buffer_sort, 1);
        config_file_option_reset (irc_config_look_list_buffer_topic_strip_colors,
                                  1);
        config_file_option_reset (irc_config_look_new_list_position, 1);
    }

    /* Catch commands (they are not executed). */
    void catch_commands (const char *command)
    {
        hook_commands = hook_command_run (NULL, command,
                                          &command_run_cb, commands, NULL);
    }

    /* Return channel name in filtered list at given index. */
    const char *filter_name (int index)
    {
        struct t_irc_list_channel *ptr_channel;

        ptr_channel = (struct t_irc_list_channel *)arraylist_get (
            server->list->filter_channels, index);
        return (ptr_channel) ? ptr_channel->name : NULL;
    }

    /* Return message (without colors) displayed on line y of list buffer. */
    char *buffer_line (int y)
    {
        struct t_gui_line *ptr_line;

        for (ptr_line = server->list->buffer->own_lines->first_line; ptr_line;
             ptr_line = ptr_line->next_line)
        {
            if (ptr_line->data->y == y)
                return gui_color_decode (ptr_line->data->message, NULL);
        }
        return NULL;
    }

    /* Return 1 if the line y is displayed with a color, 0 otherwise. */
    int buffer_line_has_color (int y)
    {
        struct t_gui_line *ptr_line;

        for (ptr_line = server->list->buffer->own_lines->first_line; ptr_line;
             ptr_line = ptr_line->next_line)
        {
            if (ptr_line->data->y == y)
                return (ptr_line->data->message[0] == GUI_COLOR_COLOR_CHAR) ? 1 : 0;
        }
        return 0;
    }

    /* Create list buffer and parse the default messages. */
    void create_list (const char *sort)
    {
        server->list->buffer = irc_list_create_buffer (server);
        CHECK(server->list->buffer);
        irc_list_set_sort (server, sort);
        LONGS_EQUAL(1, irc_list_parse_messages (server, LIST_MESSAGES));
    }
};

/*
 * Test functions:
 *   irc_list_compare_cb
 */

TEST(IrcListWithServer, CompareCb)
{
    struct t_irc_list_channel chan1 = { (char *)"#abc", (char *)"abc", 10,
                                        (char *)"topic" };
    struct t_irc_list_channel chan2 = { (char *)"##ABD", (char *)"ABD", 10,
                                        (char *)"topic" };

    LONGS_EQUAL(1, irc_list_compare_cb (NULL, NULL, &chan1, &chan2));

    /* Case-sensitive: "ABD" < "abc". */
    irc_list_set_sort (server, "name2");
    CHECK(irc_list_compare_cb (server, NULL, &chan1, &chan2) > 0);
    CHECK(irc_list_compare_cb (server, NULL, &chan2, &chan1) < 0);

    /* Case-insensitive: "abc" < "abd". */
    irc_list_set_sort (server, "~name2");
    CHECK(irc_list_compare_cb (server, NULL, &chan1, &chan2) < 0);
    CHECK(irc_list_compare_cb (server, NULL, &chan2, &chan1) > 0);

    /* Reverse + case-insensitive. */
    irc_list_set_sort (server, "-~name2");
    CHECK(irc_list_compare_cb (server, NULL, &chan1, &chan2) > 0);
    irc_list_set_sort (server, "~-name2");
    CHECK(irc_list_compare_cb (server, NULL, &chan1, &chan2) > 0);

    /* Double "~": case-sensitive again. */
    irc_list_set_sort (server, "~~name2");
    CHECK(irc_list_compare_cb (server, NULL, &chan1, &chan2) > 0);

    /* Equal users, then second field is used. */
    irc_list_set_sort (server, "-users,~name2");
    CHECK(irc_list_compare_cb (server, NULL, &chan1, &chan2) < 0);
    chan2.users = 20;
    CHECK(irc_list_compare_cb (server, NULL, &chan1, &chan2) > 0);

    /* All fields equal: 1 is returned. */
    irc_list_set_sort (server, "users,topic");
    chan2.users = 10;
    LONGS_EQUAL(1, irc_list_compare_cb (server, NULL, &chan1, &chan2));
}

/*
 * Test functions:
 *   irc_list_free_cb
 */

TEST(IrcList, FreeCb)
{
    struct t_irc_list_channel *channel;

    irc_list_free_cb (NULL, NULL, NULL);

    channel = (struct t_irc_list_channel *)malloc (sizeof (*channel));
    channel->name = strdup ("#test");
    channel->name2 = strdup ("test");
    channel->users = 1;
    channel->topic = NULL;
    irc_list_free_cb (NULL, NULL, channel);
}

/*
 * Test functions:
 *   irc_list_buffer_set_localvar_filter
 *   irc_list_set_filter
 */

TEST(IrcListWithServer, SetFilter)
{
    irc_list_buffer_set_localvar_filter (NULL, NULL);
    irc_list_buffer_set_localvar_filter (gui_buffers, NULL);

    /* Without buffer. */
    irc_list_set_filter (server, "abc");
    STRCMP_EQUAL("abc", server->list->filter);
    irc_list_set_filter (server, NULL);
    POINTERS_EQUAL(NULL, server->list->filter);

    server->list->buffer = irc_list_create_buffer (server);
    CHECK(server->list->buffer);
    STRCMP_EQUAL("*", gui_buffer_get_string (server->list->buffer,
                                             "localvar_filter"));

    irc_list_set_filter (server, "n:weechat");
    STRCMP_EQUAL("n:weechat", server->list->filter);
    STRCMP_EQUAL("n:weechat", gui_buffer_get_string (server->list->buffer,
                                                     "localvar_filter"));

    irc_list_set_filter (server, "*");
    POINTERS_EQUAL(NULL, server->list->filter);
    STRCMP_EQUAL("*", gui_buffer_get_string (server->list->buffer,
                                             "localvar_filter"));
}

/*
 * Test functions:
 *   irc_list_set_sort
 */

TEST(IrcListWithServer, SetSort)
{
    /* Default sort. */
    irc_list_set_sort (server, NULL);
    STRCMP_EQUAL("~name2", server->list->sort);
    LONGS_EQUAL(1, server->list->sort_fields_count);
    STRCMP_EQUAL("~name2", server->list->sort_fields[0]);

    config_file_option_set (irc_config_look_list_buffer_sort, "-users", 1);
    irc_list_set_sort (server, "");
    STRCMP_EQUAL("-users", server->list->sort);

    irc_list_set_sort (server, ",-users,,~name2,");
    STRCMP_EQUAL(",-users,,~name2,", server->list->sort);
    LONGS_EQUAL(2, server->list->sort_fields_count);
    STRCMP_EQUAL("-users", server->list->sort_fields[0]);
    STRCMP_EQUAL("~name2", server->list->sort_fields[1]);
}

/*
 * Test functions:
 *   irc_list_add_channel_in_hashtable
 */

TEST(IrcList, AddChannelInHashtable)
{
    struct t_hashtable *hashtable;
    struct t_irc_list_channel channel = { (char *)"##test", (char *)"test",
                                          42, (char *)"the topic" };

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL, NULL);
    CHECK(hashtable);

    irc_list_add_channel_in_hashtable (hashtable, &channel);
    LONGS_EQUAL(4, hashtable->items_count);
    STRCMP_EQUAL("##test", (const char *)hashtable_get (hashtable, "name"));
    STRCMP_EQUAL("test", (const char *)hashtable_get (hashtable, "name2"));
    STRCMP_EQUAL("42", (const char *)hashtable_get (hashtable, "users"));
    STRCMP_EQUAL("the topic", (const char *)hashtable_get (hashtable, "topic"));

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   irc_list_string_match
 */

TEST(IrcList, StringMatch)
{
    /* Without "*": case-insensitive search of mask in string. */
    LONGS_EQUAL(1, irc_list_string_match ("#weechat", "chat"));
    LONGS_EQUAL(1, irc_list_string_match ("#weechat", "CHAT"));
    LONGS_EQUAL(0, irc_list_string_match ("#weechat", ""));
    LONGS_EQUAL(0, irc_list_string_match ("#weechat", "xyz"));

    /* With "*": case-insensitive match of whole string. */
    LONGS_EQUAL(1, irc_list_string_match ("#weechat", "#wee*"));
    LONGS_EQUAL(1, irc_list_string_match ("#weechat", "#WEE*"));
    LONGS_EQUAL(1, irc_list_string_match ("#weechat", "*chat"));
    LONGS_EQUAL(0, irc_list_string_match ("#weechat", "wee*"));
    LONGS_EQUAL(0, irc_list_string_match ("#weechat", "*wee"));
}

/*
 * Test functions:
 *   irc_list_channel_match_filter
 */

TEST(IrcListWithServer, ChannelMatchFilter)
{
    struct t_irc_list_channel channel = { (char *)"#weechat",
                                          (char *)"weechat", 120,
                                          (char *)"Chat client" };
    struct t_irc_list_channel channel_no_topic = { (char *)"#abc",
                                                   (char *)"abc", 3, NULL };

    /* No filter. */
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));

    /* Name or topic. */
    irc_list_set_filter (server, "weechat");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "client");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel_no_topic));
    irc_list_set_filter (server, "*abc");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel_no_topic));
    irc_list_set_filter (server, "xyz");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel));

    /* Name. */
    irc_list_set_filter (server, "n:wee");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "n:client");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel));

    /* Topic. */
    irc_list_set_filter (server, "t:client");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "t:*client");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "t:wee");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "t:abc");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel_no_topic));

    /* Users. */
    irc_list_set_filter (server, "u:120");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "u:121");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "u:>119");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "u:>120");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "u:<121");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "u:<120");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "u:abc");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "u:>abc");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "u:<abc");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel));

    /* Condition. */
    irc_list_set_filter (server, "c:${users} > 100 && ${name2} == weechat");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel_no_topic));
    irc_list_set_filter (server, "c:${irc_list_channel.topic} =~ ^Chat");
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel));
    irc_list_set_filter (server, "c:${users} < 10");
    LONGS_EQUAL(0, irc_list_channel_match_filter (server, &channel));
    LONGS_EQUAL(1, irc_list_channel_match_filter (server, &channel_no_topic));
}

/*
 * Test functions:
 *   irc_list_parse_messages
 *   irc_list_filter_channels
 */

TEST(IrcListWithServer, ParseMessages)
{
    struct t_irc_list_channel *ptr_channel;

    /* Only messages 322 with at least 3 parameters are kept. */
    LONGS_EQUAL(1, irc_list_parse_messages (server, LIST_MESSAGES));
    LONGS_EQUAL(5, arraylist_size (server->list->channels));
    LONGS_EQUAL(8, server->list->name_max_length);

    /* Default sort on case-insensitive name without prefix. */
    STRCMP_EQUAL("~name2", server->list->sort);
    LONGS_EQUAL(5, arraylist_size (server->list->filter_channels));
    STRCMP_EQUAL("#abc", filter_name (0));
    STRCMP_EQUAL("##Linux", filter_name (1));
    STRCMP_EQUAL("#short", filter_name (2));
    STRCMP_EQUAL("#test", filter_name (3));
    STRCMP_EQUAL("#weechat", filter_name (4));

    /* Channel with topic: colors stripped. */
    ptr_channel = (struct t_irc_list_channel *)arraylist_get (
        server->list->channels, 0);
    STRCMP_EQUAL("#weechat", ptr_channel->name);
    STRCMP_EQUAL("weechat", ptr_channel->name2);
    LONGS_EQUAL(120, ptr_channel->users);
    STRCMP_EQUAL("WeeChat chat client", ptr_channel->topic);

    /* Name without all prefix chars. */
    ptr_channel = (struct t_irc_list_channel *)arraylist_get (
        server->list->channels, 1);
    STRCMP_EQUAL("##Linux", ptr_channel->name);
    STRCMP_EQUAL("Linux", ptr_channel->name2);

    /* Invalid count of users. */
    ptr_channel = (struct t_irc_list_channel *)arraylist_get (
        server->list->channels, 3);
    STRCMP_EQUAL("#test", ptr_channel->name);
    LONGS_EQUAL(0, ptr_channel->users);

    /* No topic. */
    ptr_channel = (struct t_irc_list_channel *)arraylist_get (
        server->list->channels, 4);
    STRCMP_EQUAL("#short", ptr_channel->name);
    POINTERS_EQUAL(NULL, ptr_channel->topic);

    /* Keep colors in topic. */
    config_file_option_set (irc_config_look_list_buffer_topic_strip_colors,
                            "off", 1);
    LONGS_EQUAL(1, irc_list_parse_messages (server, LIST_MESSAGES));
    ptr_channel = (struct t_irc_list_channel *)arraylist_get (
        server->list->channels, 0);
    CHECK(strcmp (ptr_channel->topic, "WeeChat chat client") != 0);
    CHECK(strstr (ptr_channel->topic, "client"));

    /* Sort by users (biggest first), with a filter. */
    irc_list_set_sort (server, "-users");
    irc_list_set_filter (server, "u:>4");
    irc_list_filter_channels (server);
    LONGS_EQUAL(3, arraylist_size (server->list->filter_channels));
    STRCMP_EQUAL("##Linux", filter_name (0));
    STRCMP_EQUAL("#weechat", filter_name (1));
    STRCMP_EQUAL("#abc", filter_name (2));

    /* No channel in output. */
    LONGS_EQUAL(1, irc_list_parse_messages (server,
                                            ":server 323 alice :End"));
    LONGS_EQUAL(0, arraylist_size (server->list->channels));
    LONGS_EQUAL(0, arraylist_size (server->list->filter_channels));
    LONGS_EQUAL(0, server->list->name_max_length);

    /* Empty output. */
    LONGS_EQUAL(0, irc_list_parse_messages (server, ""));
    POINTERS_EQUAL(NULL, server->list->channels);
}

/*
 * Test functions:
 *   irc_list_buffer_set_title
 *   irc_list_display_line
 *   irc_list_buffer_refresh
 */

TEST(IrcListWithServer, BufferRefresh)
{
    char *str;

    /* No buffer: nothing done. */
    irc_list_buffer_set_title (NULL);
    irc_list_buffer_set_title (server);
    irc_list_buffer_refresh (NULL, 1);
    irc_list_buffer_refresh (server, 1);

    /* Empty list. */
    server->list->buffer = irc_list_create_buffer (server);
    CHECK(server->list->buffer);
    irc_list_buffer_refresh (server, 0);
    WEE_TEST_STR("Empty list of channels, try \"$\" to refresh list",
                 buffer_line (1));
    STRCMP_EQUAL("0 channels (total: 0) | Filter: * | Sort:  | "
                 "Keys: ctrl+j=join channel | "
                 "Input: $=refresh, s:x,y=sort, words=filter, *=reset filter, "
                 "q=close buffer",
                 server->list->buffer->title);

    irc_list_set_sort (server, "-users");
    LONGS_EQUAL(1, irc_list_parse_messages (server, LIST_MESSAGES));
    irc_list_set_filter (server, "u:>4");
    irc_list_filter_channels (server);
    irc_list_buffer_refresh (server, 1);
    STRCMP_EQUAL("3 channels (total: 5) | Filter: u:>4 | Sort: -users | "
                 "Keys: ctrl+j=join channel | "
                 "Input: $=refresh, s:x,y=sort, words=filter, *=reset filter, "
                 "q=close buffer",
                 server->list->buffer->title);
    LONGS_EQUAL(3, server->list->buffer->own_lines->lines_count);
    WEE_TEST_STR("##Linux      300  Linux discussions", buffer_line (0));
    WEE_TEST_STR("#weechat     120  WeeChat chat client", buffer_line (1));
    WEE_TEST_STR("#abc           5  Some topic", buffer_line (2));
    LONGS_EQUAL(1, buffer_line_has_color (0));
    LONGS_EQUAL(0, buffer_line_has_color (1));

    /* Line outside the filtered channels: no line displayed. */
    irc_list_display_line (server, 3);
    WEE_TEST_STR(NULL, buffer_line (3));

    /* Channel without topic. */
    irc_list_set_filter (server, "n:short");
    irc_list_filter_channels (server);
    irc_list_buffer_refresh (server, 1);
    WEE_TEST_STR("#short         2  ", buffer_line (0));
}

/*
 * Test functions:
 *   irc_list_set_current_line
 *   irc_list_move_line_relative
 *   irc_list_move_line_absolute
 */

TEST(IrcListWithServer, MoveLine)
{
    /* No channels: nothing done. */
    server->list->buffer = irc_list_create_buffer (server);
    CHECK(server->list->buffer);
    irc_list_move_line_relative (server, 1);
    LONGS_EQUAL(0, server->list->selected_line);
    irc_list_move_line_absolute (server, 2);
    LONGS_EQUAL(0, server->list->selected_line);

    irc_list_set_sort (server, NULL);
    LONGS_EQUAL(1, irc_list_parse_messages (server, LIST_MESSAGES));
    irc_list_buffer_refresh (server, 1);
    LONGS_EQUAL(1, buffer_line_has_color (0));

    /* Invalid lines: ignored. */
    irc_list_set_current_line (server, -1);
    LONGS_EQUAL(0, server->list->selected_line);
    irc_list_set_current_line (server, 5);
    LONGS_EQUAL(0, server->list->selected_line);

    irc_list_set_current_line (server, 2);
    LONGS_EQUAL(2, server->list->selected_line);
    LONGS_EQUAL(0, buffer_line_has_color (0));
    LONGS_EQUAL(1, buffer_line_has_color (2));

    irc_list_move_line_relative (server, 1);
    LONGS_EQUAL(3, server->list->selected_line);
    irc_list_move_line_relative (server, -2);
    LONGS_EQUAL(1, server->list->selected_line);
    irc_list_move_line_relative (server, -10);
    LONGS_EQUAL(0, server->list->selected_line);
    irc_list_move_line_relative (server, 10);
    LONGS_EQUAL(4, server->list->selected_line);
    LONGS_EQUAL(1, buffer_line_has_color (4));

    irc_list_move_line_absolute (server, 0);
    LONGS_EQUAL(0, server->list->selected_line);
    irc_list_move_line_absolute (server, -1);
    LONGS_EQUAL(4, server->list->selected_line);
    irc_list_move_line_absolute (server, 2);
    LONGS_EQUAL(2, server->list->selected_line);
    irc_list_move_line_absolute (server, 100);
    LONGS_EQUAL(4, server->list->selected_line);
}

/*
 * Test functions:
 *   irc_list_get_window_info
 *   irc_list_check_line_outside_window
 *   irc_list_window_scrolled_cb
 *   irc_list_scroll_horizontal
 */

TEST(IrcListWithServer, Window)
{
    struct t_gui_buffer *old_buffer;
    int start_line_y, chat_height;

    create_list (NULL);
    irc_list_buffer_refresh (server, 1);

    /* Buffer not displayed: nothing done. */
    irc_list_check_line_outside_window (server);
    irc_list_scroll_horizontal (server, 50);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_window_scrolled_cb (NULL, NULL, "window_scrolled",
                                             WEECHAT_HOOK_SIGNAL_POINTER,
                                             gui_current_window));
    LONGS_EQUAL(0, server->list->selected_line);

    old_buffer = gui_current_window->buffer;
    gui_window_switch_to_buffer (gui_current_window, server->list->buffer, 1);

    irc_list_get_window_info (gui_current_window, &start_line_y, &chat_height);
    LONGS_EQUAL(0, start_line_y);
    CHECK(chat_height > 0);

    irc_list_check_line_outside_window (server);
    irc_list_scroll_horizontal (server, -200);
    irc_list_scroll_horizontal (server, 200);

    server->list->selected_line = 3;
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_window_scrolled_cb (NULL, NULL, "window_scrolled",
                                             WEECHAT_HOOK_SIGNAL_POINTER,
                                             gui_current_window));
    LONGS_EQUAL(3, server->list->selected_line);

    gui_window_switch_to_buffer (gui_current_window, old_buffer, 1);
}

/*
 * Test functions:
 *   irc_list_join_channel
 */

TEST(IrcListWithServer, JoinChannel)
{
    catch_commands ("/join*");

    /* No channels: nothing done. */
    irc_list_join_channel (server);
    STRCMP_EQUAL("", commands);

    create_list (NULL);
    irc_list_join_channel (server);
    STRCMP_EQUAL("/join #abc", commands);

    commands[0] = '\0';
    irc_list_set_current_line (server, 4);
    irc_list_join_channel (server);
    STRCMP_EQUAL("/join #weechat", commands);

    /* Selected line outside filtered channels. */
    commands[0] = '\0';
    irc_list_set_filter (server, "n:linux");
    irc_list_filter_channels (server);
    irc_list_join_channel (server);
    STRCMP_EQUAL("", commands);
}

/*
 * Test functions:
 *   irc_list_buffer_input_data
 */

TEST(IrcListWithServer, BufferInputData)
{
    struct t_gui_buffer *buffer;

    catch_commands ("/*");
    create_list (NULL);
    buffer = server->list->buffer;

    /* Buffer without server. */
    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (gui_buffers, "$"));
    STRCMP_EQUAL("", commands);

    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, "$"));
    STRCMP_EQUAL("/list", commands);

    commands[0] = '\0';
    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, "j"));
    STRCMP_EQUAL("/join #abc", commands);

    commands[0] = '\0';
    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, "<<"));
    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, ">>"));
    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, "<"));
    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, ">"));
    STRCMP_EQUAL("/list -go 0|/list -go end|/list -left|/list -right",
                 commands);

    /* Sort. */
    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, "s:-users"));
    STRCMP_EQUAL("-users", server->list->sort);
    STRCMP_EQUAL("##Linux", filter_name (0));

    /* Filter. */
    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, "  topic"));
    STRCMP_EQUAL("topic", server->list->filter);
    LONGS_EQUAL(1, arraylist_size (server->list->filter_channels));
    STRCMP_EQUAL("#abc", filter_name (0));

    /* Only spaces: filter unchanged. */
    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, "   "));
    STRCMP_EQUAL("topic", server->list->filter);

    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, "*"));
    POINTERS_EQUAL(NULL, server->list->filter);
    LONGS_EQUAL(5, arraylist_size (server->list->filter_channels));

    /* Close buffer. */
    LONGS_EQUAL(WEECHAT_RC_OK, irc_list_buffer_input_data (buffer, "q"));
    POINTERS_EQUAL(NULL, server->list->buffer);
    POINTERS_EQUAL(NULL, server->list->channels);
}

/*
 * Test functions:
 *   irc_list_create_buffer
 */

TEST(IrcListWithServer, CreateBuffer)
{
    struct t_gui_buffer *buffer;
    int number;

    server->list->buffer = irc_list_create_buffer (server);
    buffer = server->list->buffer;
    CHECK(buffer);
    STRCMP_EQUAL("irc.list_test_list", buffer->full_name);
    LONGS_EQUAL(GUI_BUFFER_TYPE_FREE, buffer->type);
    STRCMP_EQUAL("list", gui_buffer_get_string (buffer, "localvar_type"));
    STRCMP_EQUAL("test_list", gui_buffer_get_string (buffer,
                                                     "localvar_server"));
    STRCMP_EQUAL("test_list", gui_buffer_get_string (buffer,
                                                     "localvar_channel"));
    STRCMP_EQUAL("1", gui_buffer_get_string (buffer, "localvar_no_log"));
    STRCMP_EQUAL("*", gui_buffer_get_string (buffer, "localvar_filter"));
    STRCMP_EQUAL("-", buffer->highlight_words);
    LONGS_EQUAL(7, buffer->keys_count);
    gui_buffer_close (buffer);
    POINTERS_EQUAL(NULL, server->list->buffer);

    /* Position "next": after current buffer. */
    config_file_option_set (irc_config_look_new_list_position, "next", 1);
    number = gui_current_window->buffer->number;
    server->list->buffer = irc_list_create_buffer (server);
    CHECK(server->list->buffer);
    LONGS_EQUAL(number + 1, server->list->buffer->number);
}

/*
 * Test functions:
 *   irc_list_hsignal_redirect_list_cb
 */

TEST(IrcListWithServer, HsignalRedirectListCb)
{
    struct t_hashtable *hashtable;

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL, NULL);
    CHECK(hashtable);

    server->list->buffer = irc_list_create_buffer (server);
    CHECK(server->list->buffer);

    /* Error in redirection. */
    hashtable_set (hashtable, "error", "timeout");
    hashtable_set (hashtable, "server", "test_list");
    hashtable_set (hashtable, "output", LIST_MESSAGES);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_hsignal_redirect_list_cb (NULL, NULL, "test",
                                                   hashtable));
    POINTERS_EQUAL(NULL, server->list->channels);

    /* Unknown server. */
    hashtable_remove (hashtable, "error");
    hashtable_set (hashtable, "server", "xxx");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_hsignal_redirect_list_cb (NULL, NULL, "test",
                                                   hashtable));
    POINTERS_EQUAL(NULL, server->list->channels);

    /* Server without buffer. */
    hashtable_set (hashtable, "server", "test_list");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_hsignal_redirect_list_cb (NULL, NULL, "test",
                                                   hashtable));
    POINTERS_EQUAL(NULL, server->list->channels);

    /* No output. */
    irc_server_create_buffer (server);
    CHECK(server->buffer);
    hashtable_remove (hashtable, "output");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_hsignal_redirect_list_cb (NULL, NULL, "test",
                                                   hashtable));
    POINTERS_EQUAL(NULL, server->list->channels);

    /* Output: channels parsed and displayed. */
    hashtable_set (hashtable, "output", LIST_MESSAGES);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_hsignal_redirect_list_cb (NULL, NULL, "test",
                                                   hashtable));
    LONGS_EQUAL(5, arraylist_size (server->list->channels));
    LONGS_EQUAL(5, server->list->buffer->own_lines->lines_count);

    hashtable_free (hashtable);
}

/*
 * Test functions:
 *   irc_list_export
 */

TEST(IrcListWithServer, Export)
{
    char *filename, content[1024];
    FILE *file;
    size_t size;
    struct stat st;

    filename = string_eval_path_home ("${weechat_data_dir}/test_list.txt",
                                      NULL, NULL, NULL);
    CHECK(filename);
    unlink (filename);

    /* No list buffer. */
    LONGS_EQUAL(0, irc_list_export (NULL, filename));
    LONGS_EQUAL(0, irc_list_export (server, filename));

    create_list ("-users");
    irc_list_set_filter (server, "u:>4");
    irc_list_filter_channels (server);

    /* Invalid path. */
    LONGS_EQUAL(0, irc_list_export (server, "/nonexistent_dir/test.txt"));

    LONGS_EQUAL(1, irc_list_export (server, filename));
    LONGS_EQUAL(0, stat (filename, &st));
    LONGS_EQUAL(0600, st.st_mode & 0777);
    file = fopen (filename, "r");
    CHECK(file);
    size = fread (content, 1, sizeof (content) - 1, file);
    content[size] = '\0';
    fclose (file);
    STRCMP_EQUAL("##Linux (300): \"Linux discussions\"\n"
                 "#weechat (120): \"WeeChat chat client\"\n"
                 "#abc (5): \"Some topic\"\n",
                 content);

    /* All channels, including one without topic. */
    irc_list_set_filter (server, NULL);
    irc_list_filter_channels (server);
    LONGS_EQUAL(1, irc_list_export (server, filename));
    file = fopen (filename, "r");
    CHECK(file);
    size = fread (content, 1, sizeof (content) - 1, file);
    content[size] = '\0';
    fclose (file);
    STRCMP_EQUAL("##Linux (300): \"Linux discussions\"\n"
                 "#weechat (120): \"WeeChat chat client\"\n"
                 "#abc (5): \"Some topic\"\n"
                 "#short (2): \"\"\n"
                 "#test (0): \"invalid count\"\n",
                 content);

    unlink (filename);
    free (filename);
}

/*
 * Test functions:
 *   irc_list_reset
 *   irc_list_free_data
 */

TEST(IrcListWithServer, ResetFreeData)
{
    irc_list_reset (NULL);
    irc_list_free_data (NULL);

    create_list ("-users");
    irc_list_set_filter (server, "abc");
    irc_list_set_current_line (server, 1);

    irc_list_reset (server);
    LONGS_EQUAL(0, arraylist_size (server->list->channels));
    LONGS_EQUAL(0, arraylist_size (server->list->filter_channels));
    LONGS_EQUAL(0, server->list->name_max_length);
    STRCMP_EQUAL("-users", server->list->sort);
    STRCMP_EQUAL("abc", server->list->filter);
    LONGS_EQUAL(0, server->list->selected_line);

    /* Sort is set to default if not set. */
    free (server->list->sort);
    server->list->sort = NULL;
    irc_list_reset (server);
    STRCMP_EQUAL("~name2", server->list->sort);

    irc_list_free_data (server);
    POINTERS_EQUAL(NULL, server->list->channels);
    POINTERS_EQUAL(NULL, server->list->filter_channels);
    LONGS_EQUAL(0, server->list->name_max_length);
    POINTERS_EQUAL(NULL, server->list->filter);
    POINTERS_EQUAL(NULL, server->list->sort);
    POINTERS_EQUAL(NULL, server->list->sort_fields);
    LONGS_EQUAL(0, server->list->sort_fields_count);
    LONGS_EQUAL(0, server->list->selected_line);
    CHECK(server->list->buffer);
}

/*
 * Test functions:
 *   irc_list_alloc
 */

TEST(IrcList, Alloc)
{
    struct t_irc_list *list;

    list = irc_list_alloc ();
    CHECK(list);
    POINTERS_EQUAL(NULL, list->buffer);
    POINTERS_EQUAL(NULL, list->channels);
    POINTERS_EQUAL(NULL, list->filter_channels);
    LONGS_EQUAL(0, list->name_max_length);
    STRCMP_EQUAL(NULL, list->filter);
    STRCMP_EQUAL(NULL, list->sort);
    POINTERS_EQUAL(NULL, list->sort_fields);
    LONGS_EQUAL(0, list->sort_fields_count);
    LONGS_EQUAL(0, list->selected_line);
    free (list);
}

/*
 * Test functions:
 *   irc_list_free
 */

TEST(IrcListWithServer, Free)
{
    struct t_irc_list *list;

    irc_list_free (NULL);

    create_list (NULL);
    irc_list_free (server);
    POINTERS_EQUAL(NULL, server->list);
    POINTERS_EQUAL(NULL, gui_buffer_search_by_full_name ("irc.list_test_list"));

    /* Already freed: nothing done. */
    irc_list_free (server);

    /* Restore a list for teardown. */
    list = irc_list_alloc ();
    CHECK(list);
    server->list = list;
}

/*
 * Test functions:
 *   irc_list_hdata_list_channel_cb
 *   irc_list_hdata_list_cb
 */

TEST(IrcListWithServer, Hdata)
{
    struct t_hdata *hdata;
    struct t_irc_list_channel *ptr_channel;

    hdata = hook_hdata_get (NULL, "irc_list_channel");
    CHECK(hdata);
    LONGS_EQUAL(WEECHAT_HDATA_STRING, hdata_get_var_type (hdata, "name"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING, hdata_get_var_type (hdata, "name2"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER, hdata_get_var_type (hdata, "users"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING, hdata_get_var_type (hdata, "topic"));

    create_list (NULL);
    ptr_channel = (struct t_irc_list_channel *)arraylist_get (
        server->list->channels, 0);
    STRCMP_EQUAL("#weechat", hdata_string (hdata, ptr_channel, "name"));
    LONGS_EQUAL(120, hdata_integer (hdata, ptr_channel, "users"));

    hdata = hook_hdata_get (NULL, "irc_list");
    CHECK(hdata);
    LONGS_EQUAL(WEECHAT_HDATA_POINTER, hdata_get_var_type (hdata, "buffer"));
    STRCMP_EQUAL("buffer", hdata_get_var_hdata (hdata, "buffer"));
    LONGS_EQUAL(WEECHAT_HDATA_STRING, hdata_get_var_type (hdata, "sort"));
    LONGS_EQUAL(WEECHAT_HDATA_INTEGER,
                hdata_get_var_type (hdata, "selected_line"));
    STRCMP_EQUAL("~name2", hdata_string (hdata, server->list, "sort"));
    LONGS_EQUAL(8, hdata_integer (hdata, server->list, "name_max_length"));
}

/*
 * Test functions:
 *   irc_list_mouse_hsignal_cb
 */

TEST(IrcListWithServer, MouseHsignalCb)
{
    struct t_hashtable *hashtable;
    char str_pointer[64];

    hashtable = hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_STRING,
                               NULL, NULL);
    CHECK(hashtable);

    create_list (NULL);
    catch_commands ("/join*");

    /* Missing keys. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_mouse_hsignal_cb (NULL, NULL, "test", hashtable));

    /* Invalid buffer pointer. */
    hashtable_set (hashtable, "_key", "button1");
    hashtable_set (hashtable, "_buffer", "zzz");
    hashtable_set (hashtable, "_chat_line_y", "2");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_mouse_hsignal_cb (NULL, NULL, "test", hashtable));
    hashtable_set (hashtable, "_buffer", "0x0");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_mouse_hsignal_cb (NULL, NULL, "test", hashtable));
    LONGS_EQUAL(0, server->list->selected_line);

    /* Click: select line. */
    snprintf (str_pointer, sizeof (str_pointer),
              "%p", (void *)server->list->buffer);
    hashtable_set (hashtable, "_buffer", str_pointer);
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_mouse_hsignal_cb (NULL, NULL, "test", hashtable));
    LONGS_EQUAL(2, server->list->selected_line);
    STRCMP_EQUAL("", commands);

    /* Button 2: select line and join channel. */
    hashtable_set (hashtable, "_key", "button2");
    hashtable_set (hashtable, "_chat_line_y", "4");
    LONGS_EQUAL(WEECHAT_RC_OK,
                irc_list_mouse_hsignal_cb (NULL, NULL, "test", hashtable));
    LONGS_EQUAL(4, server->list->selected_line);
    STRCMP_EQUAL("/join #weechat", commands);

    hashtable_free (hashtable);
}
