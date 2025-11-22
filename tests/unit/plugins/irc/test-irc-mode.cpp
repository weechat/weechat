/*
 * SPDX-FileCopyrightText: 2019-2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* Test IRC mode functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/gui/gui-color.h"
#include "src/plugins/irc/irc-color.h"
#include "src/plugins/irc/irc-mode.h"
}

#define WEE_CHECK_GET_ARGS(__result, __arguments)                       \
    str = irc_mode_get_arguments_colors (__arguments);                  \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

TEST_GROUP(IrcMode)
{
};

/*
 * Tests functions:
 *   irc_mode_get_arguments
 */

TEST(IrcMode, GetArguments)
{
    char *str, string[1024], expected[1024];

    /* invalid arguments */
    WEE_CHECK_GET_ARGS("", NULL);
    WEE_CHECK_GET_ARGS("", "");
    WEE_CHECK_GET_ARGS("", " ");

    /* simple arguments */
    snprintf (string, sizeof (string), "abc%c02_blue", IRC_COLOR_COLOR_CHAR);
    snprintf (expected, sizeof (expected),
              "abc%s_blue%s",
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"));
    WEE_CHECK_GET_ARGS(expected, string);
    snprintf (string, sizeof (string),
              "abc%c02_blue def%c02_blue",
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR);
    snprintf (expected, sizeof (expected),
              "abc%s_blue%s def%s_blue%s",
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"));
    WEE_CHECK_GET_ARGS(expected, string);
    snprintf (string, sizeof (string),
              "abc%c02_blue def%c02_blue ghi%c02_blue",
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR);
    snprintf (expected, sizeof (expected),
              "abc%s_blue%s def%s_blue%s ghi%s_blue%s",
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"));
    WEE_CHECK_GET_ARGS(expected, string);

    /* some arguments starting with a colon */
    snprintf (string, sizeof (string), ":abc%c02_blue", IRC_COLOR_COLOR_CHAR);
    snprintf (expected, sizeof (expected),
              "abc%s_blue%s",
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"));
    WEE_CHECK_GET_ARGS(expected, string);
    snprintf (string, sizeof (string),
              ":abc%c02_blue def%c02_blue",
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR);
    snprintf (expected, sizeof (expected),
              "abc%s_blue%s def%s_blue%s",
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"));
    WEE_CHECK_GET_ARGS(expected, string);
    snprintf (string, sizeof (string),
              "abc%c02_blue :def%c02_blue",
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR);
    WEE_CHECK_GET_ARGS(expected, string);
    snprintf (string, sizeof (string),
              "abc%c02_blue :def%c02_blue ghi%c02_blue",
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR);
    snprintf (expected, sizeof (expected),
              "abc%s_blue%s def%s_blue%s ghi%s_blue%s",
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"),
              gui_color_get_custom ("|blue"),
              gui_color_get_custom ("reset"));
    WEE_CHECK_GET_ARGS(expected, string);
    snprintf (string, sizeof (string),
              "abc%c02_blue :def%c02_blue :ghi%c02_blue",
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR);
    WEE_CHECK_GET_ARGS(expected, string);
    snprintf (string, sizeof (string),
              ":abc%c02_blue :def%c02_blue :ghi%c02_blue",
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR,
              IRC_COLOR_COLOR_CHAR);
    WEE_CHECK_GET_ARGS(expected, string);
}
