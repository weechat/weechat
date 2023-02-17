/*
 * test-logger-backlog.cpp - test logger backlog functions
 *
 * Copyright (C) 2022-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/core/wee-config.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-line.h"
#include "src/plugins/logger/logger-config.h"

extern void logger_backlog_display_line (struct t_gui_buffer *buffer,
                                         const char *line);
}

TEST_GROUP(LoggerBacklog)
{
};

/*
 * Tests functions:
 *   logger_backlog_check_conditions
 */

TEST(LoggerBacklog, CheckConditions)
{
    /* TODO: write tests */
}

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
