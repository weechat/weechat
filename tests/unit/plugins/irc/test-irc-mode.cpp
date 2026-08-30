/*
 * SPDX-FileCopyrightText: 2019-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
 * Test functions:
 *   irc_mode_get_arguments
 */

TEST(IrcMode, GetArguments)
{
    char *str, string[1024], expected[1024];

    /* Invalid arguments */
    WEE_CHECK_GET_ARGS("", NULL);
    WEE_CHECK_GET_ARGS("", "");
    WEE_CHECK_GET_ARGS("", " ");

    /* Simple arguments */
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

    /* Some arguments starting with a colon */
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
