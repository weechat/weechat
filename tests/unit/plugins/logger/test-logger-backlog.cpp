/*
 * test-logger-backlog.cpp - test logger backlog functions
 *
 * Copyright (C) 2022-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/core-arraylist.h"
#include "src/core/core-config.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-line.h"
#include "src/plugins/logger/logger-config.h"

extern void logger_backlog_display_line (struct t_gui_buffer *buffer,
                                         const char *line);
extern struct t_arraylist *logger_backlog_group_messages (struct t_arraylist *lines);
}

TEST_GROUP(LoggerBacklog)
{
};

/*
 * Tests functions:
 *   logger_backlog_display_line
 */

TEST(LoggerBacklog, DisplayLine)
{
    char str_line[1024], string[1024];
    struct t_gui_line *ptr_line;
    struct t_gui_line_data *ptr_data;

    ptr_line = gui_buffers->own_lines->last_line;
    logger_backlog_display_line (gui_buffers, NULL);
    POINTERS_EQUAL(ptr_line, gui_buffers->own_lines->last_line);

    /* empty string */
    logger_backlog_display_line (gui_buffers, "");
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data->date != 1645288340);
    CHECK(ptr_data->date == ptr_data->date_printed);
    LONGS_EQUAL(3, ptr_data->tags_count);
    STRCMP_EQUAL("no_highlight", ptr_data->tags_array[0]);
    STRCMP_EQUAL("notify_none", ptr_data->tags_array[1]);
    STRCMP_EQUAL("logger_backlog", ptr_data->tags_array[2]);
    STRCMP_EQUAL("", ptr_data->prefix);
    snprintf (string, sizeof (string),
              "%s",
              gui_color_get_custom (
                  gui_color_get_name (
                      CONFIG_COLOR(logger_config_color_backlog_line))));
    STRCMP_EQUAL(string, ptr_data->message);

    /* invalid date */
    snprintf (str_line, sizeof (str_line),
              "invalid date\tnick\tthe message");
    logger_backlog_display_line (gui_buffers, str_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    CHECK(ptr_data->date != 1645288340);
    CHECK(ptr_data->date == ptr_data->date_printed);
    LONGS_EQUAL(3, ptr_data->tags_count);
    STRCMP_EQUAL("no_highlight", ptr_data->tags_array[0]);
    STRCMP_EQUAL("notify_none", ptr_data->tags_array[1]);
    STRCMP_EQUAL("logger_backlog", ptr_data->tags_array[2]);
    snprintf (string, sizeof (string),
              "%sinvalid date",
              gui_color_get_custom (
                  gui_color_get_name (
                      CONFIG_COLOR(logger_config_color_backlog_line))));
    STRCMP_EQUAL(string, ptr_data->prefix);
    snprintf (string, sizeof (string),
              "%snick\tthe message",
              gui_color_get_custom (
                  gui_color_get_name (
                      CONFIG_COLOR(logger_config_color_backlog_line))));
    STRCMP_EQUAL(string, ptr_data->message);

    /* valid line */
    snprintf (str_line, sizeof (str_line),
              "2022-02-19 16:32:20\tnick\tthe message");
    logger_backlog_display_line (gui_buffers, str_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    LONGS_EQUAL(1645288340, ptr_data->date);
    CHECK(ptr_data->date_printed > 1645288340);
    LONGS_EQUAL(3, ptr_data->tags_count);
    STRCMP_EQUAL("no_highlight", ptr_data->tags_array[0]);
    STRCMP_EQUAL("notify_none", ptr_data->tags_array[1]);
    STRCMP_EQUAL("logger_backlog", ptr_data->tags_array[2]);
    snprintf (string, sizeof (string),
              "%snick",
              gui_color_get_custom (
                  gui_color_get_name (
                      CONFIG_COLOR(logger_config_color_backlog_line))));
    STRCMP_EQUAL(string, ptr_data->prefix);
    snprintf (string, sizeof (string),
              "%sthe message",
              gui_color_get_custom (
                  gui_color_get_name (
                      CONFIG_COLOR(logger_config_color_backlog_line))));
    STRCMP_EQUAL(string, ptr_data->message);

    /* valid line with Tab in message */
    snprintf (str_line, sizeof (str_line),
              "2022-02-19 16:32:21\tnick\tthe message\twith tab");
    logger_backlog_display_line (gui_buffers, str_line);
    ptr_data = gui_buffers->own_lines->last_line->data;
    LONGS_EQUAL(1645288341, ptr_data->date);
    CHECK(ptr_data->date_printed > 1645288341);
    LONGS_EQUAL(3, ptr_data->tags_count);
    STRCMP_EQUAL("no_highlight", ptr_data->tags_array[0]);
    STRCMP_EQUAL("notify_none", ptr_data->tags_array[1]);
    STRCMP_EQUAL("logger_backlog", ptr_data->tags_array[2]);
    snprintf (string, sizeof (string),
              "%snick",
              gui_color_get_custom (
                  gui_color_get_name (
                      CONFIG_COLOR(logger_config_color_backlog_line))));
    STRCMP_EQUAL(string, ptr_data->prefix);
    snprintf (string, sizeof (string),
              "%sthe message\twith tab",
              gui_color_get_custom (
                  gui_color_get_name (
                      CONFIG_COLOR(logger_config_color_backlog_line))));
    STRCMP_EQUAL(string, ptr_data->message);
}

/*
 * Compares two messages in arraylist.
 */

int
test_logger_backlog_msg_cmp_cb (void *data,
                                struct t_arraylist *arraylist,
                                void *pointer1,
                                void *pointer2)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    return strcmp ((const char *)pointer1, (const char *)pointer2);
}

/*
 * Frees a message in arraylist.
 */

void
test_logger_backlog_msg_free_cb (void *data, struct t_arraylist *arraylist,
                                 void *pointer)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    free (pointer);
}

/*
 * Tests functions:
 *   logger_backlog_msg_cmp_cb
 *   logger_backlog_msg_free_cb
 *   logger_backlog_group_messages
 */

TEST(LoggerBacklog, GroupMessages)
{
    struct t_arraylist *lines, *messages;
    const char *test_lines_1[] = {
        "2023-06-04 21:15:34\t\tMessage 1",
        "2023-06-04 21:15:40\t\tMessage 2",
        NULL,
    };
    const char *test_lines_2[] = {
        "end of line",
        "2023-06-04 21:15:34\t\tFirst line",
        "of multiline message",
        "",
        "end of message",
        "2023-06-04 21:15:37\t\tTwo lines with empty line",
        "",
        "2023-06-04 21:15:40\t\tMessage on one line",
        NULL,
    };
    int i;

    POINTERS_EQUAL(NULL, logger_backlog_group_messages (NULL));

    lines = arraylist_new (32, 0, 1,
                           &test_logger_backlog_msg_cmp_cb, NULL,
                           &test_logger_backlog_msg_free_cb, NULL);

    for (i = 0; test_lines_1[i]; i++)
    {
        arraylist_add (lines, strdup (test_lines_1[i]));
    }

    messages = logger_backlog_group_messages (lines);
    CHECK(messages);
    LONGS_EQUAL(2, arraylist_size (messages));
    STRCMP_EQUAL("2023-06-04 21:15:34\t\tMessage 1",
                 (const char *)arraylist_get (messages, 0));
    STRCMP_EQUAL("2023-06-04 21:15:40\t\tMessage 2",
                 (const char *)arraylist_get (messages, 1));
    arraylist_free (messages);

    arraylist_clear (lines);

    for (i = 0; test_lines_2[i]; i++)
    {
        arraylist_add (lines, strdup (test_lines_2[i]));
    }

    messages = logger_backlog_group_messages (lines);
    CHECK(messages);
    LONGS_EQUAL(4, arraylist_size (messages));
    STRCMP_EQUAL("end of line",
                 (const char *)arraylist_get (messages, 0));
    STRCMP_EQUAL("2023-06-04 21:15:34\t\tFirst line\n"
                 "of multiline message\n"
                 "\n"
                 "end of message",
                 (const char *)arraylist_get (messages, 1));
    STRCMP_EQUAL("2023-06-04 21:15:37\t\tTwo lines with empty line\n",
                 (const char *)arraylist_get (messages, 2));
    STRCMP_EQUAL("2023-06-04 21:15:40\t\tMessage on one line",
                 (const char *)arraylist_get (messages, 3));
    arraylist_free (messages);


    arraylist_free (lines);
}

/*
 * Tests functions:
 *   logger_backlog_file
 */

TEST(LoggerBacklog, File)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   logger_backlog_signal_cb
 */

TEST(LoggerBacklog, SignalCb)
{
    /* TODO: write tests */
}
