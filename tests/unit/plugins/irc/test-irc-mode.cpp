/*
 * SPDX-FileCopyrightText: 2019-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test IRC mode functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/core/core-string.h"
#include "src/gui/gui-color.h"
#include "src/plugins/irc/irc-color.h"
#include "src/plugins/irc/irc-mode.h"
#include "src/plugins/irc/irc-server.h"
}

#define WEE_CHECK_GET_ARGS(__result, __arguments)                       \
    str = irc_mode_get_arguments_colors (__arguments);                  \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);

#define WEE_CHECK_SPLIT_MODES(__result, __modes, __max_modes)          \
    messages = irc_mode_split_channel_modes (server, __modes,           \
                                             __max_modes);              \
    CHECK(messages);                                                    \
    str = string_rebuild_split_string ((const char **)messages, "|",    \
                                       0, -1);                          \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);                                                         \
    string_free_split (messages);

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

/*
 * Test functions:
 *   irc_mode_split_channel_modes
 */

TEST(IrcMode, SplitChannelModes)
{
    struct t_irc_server *server;
    char **messages, *str;

    server = irc_server_alloc ("server_split_modes");
    CHECK(server);

    irc_server_set_prefix_modes_chars (server, "(ohv)@%+");
    free (server->chanmodes);
    server->chanmodes = strdup ("eIbq,k,flj,CFLMPQScgimnprstuz");

    /* Invalid arguments */
    POINTERS_EQUAL(NULL, irc_mode_split_channel_modes (NULL, NULL, 4));
    POINTERS_EQUAL(NULL, irc_mode_split_channel_modes (server, NULL, 4));
    POINTERS_EQUAL(NULL, irc_mode_split_channel_modes (NULL, "+o nick", 4));

    /* No split needed: modes returned unchanged */
    WEE_CHECK_SPLIT_MODES("", "", 4);
    WEE_CHECK_SPLIT_MODES("+nt", "+nt", 4);
    WEE_CHECK_SPLIT_MODES("+b", "+b", 1);
    WEE_CHECK_SPLIT_MODES("+o nick1", "+o nick1", 1);
    WEE_CHECK_SPLIT_MODES("+oooo n1 n2 n3 n4", "+oooo n1 n2 n3 n4", 4);
    WEE_CHECK_SPLIT_MODES("+oo  n1   n2  ", "+oo  n1   n2  ", 4);
    WEE_CHECK_SPLIT_MODES("+ntimso n1", "+ntimso n1", 1);
    WEE_CHECK_SPLIT_MODES("-lkk key1 key2", "-lkk key1 key2", 2);

    /* Argument with a colon (trailing parameter): no split */
    WEE_CHECK_SPLIT_MODES("+oo n1 :n2", "+oo n1 :n2", 1);
    WEE_CHECK_SPLIT_MODES(":+oo n1 n2", ":+oo n1 n2", 1);

    /* Split of nicks (prefix modes) */
    WEE_CHECK_SPLIT_MODES("+oooo n1 n2 n3 n4|+o n5",
                          "+ooooo n1 n2 n3 n4 n5", 4);
    WEE_CHECK_SPLIT_MODES("+oooo n1 n2 n3 n4|+oooo n5 n6 n7 n8|+o n9",
                          "+ooooooooo n1 n2 n3 n4 n5 n6 n7 n8 n9", 4);
    WEE_CHECK_SPLIT_MODES("+o n1|+o n2|+o n3",
                          "+ooo n1 n2 n3", 1);
    WEE_CHECK_SPLIT_MODES("+oo n1 n2|+o n3",
                          "  +ooo   n1  n2   n3  ", 2);

    /* Modes without "+" or "-" (default is "+") */
    WEE_CHECK_SPLIT_MODES("+oo n1 n2|+o n3", "ooo n1 n2 n3", 2);

    /* Mix of set/unset modes */
    WEE_CHECK_SPLIT_MODES("+oo-v n1 n2 n3|-v+h n4 n5",
                          "+oo-vv+h n1 n2 n3 n4 n5", 3);
    WEE_CHECK_SPLIT_MODES("-oo n1 n2|-o+v n3 n4",
                          "-ooo+v n1 n2 n3 n4", 2);
    WEE_CHECK_SPLIT_MODES("+o n1|+o n2", "+o+o n1 n2", 1);

    /* Modes without argument are not counted */
    WEE_CHECK_SPLIT_MODES("+ntoo n1 n2|+oim n3",
                          "+ntoooim n1 n2 n3", 2);
    WEE_CHECK_SPLIT_MODES("+o n1|+o-nt n2",
                          "+oo-nt n1 n2", 1);

    /* Type A (list), with and without argument */
    WEE_CHECK_SPLIT_MODES("+bb mask1 mask2|+be mask3 mask4",
                          "+bbbe mask1 mask2 mask3 mask4", 2);
    WEE_CHECK_SPLIT_MODES("+o n1|+bb mask1",
                          "+obb n1 mask1", 1);

    /* Type B (always an argument) */
    WEE_CHECK_SPLIT_MODES("+ko key n1|-k+o key n2",
                          "+ko-k+o key n1 key n2", 2);

    /* Type C (argument only when set) */
    WEE_CHECK_SPLIT_MODES("+lo 10 n1|+o-l n2",
                          "+loo-l 10 n1 n2", 2);

    /* Missing arguments */
    WEE_CHECK_SPLIT_MODES("+o n1|+ooo n2",
                          "+oooo n1 n2", 1);

    /* Extra arguments kept in the last message */
    WEE_CHECK_SPLIT_MODES("+oo n1 n2|+o n3 extra1 extra2",
                          "+ooo n1 n2 n3 extra1 extra2", 2);
    WEE_CHECK_SPLIT_MODES("+o n1|+o-l n2 n3",
                          "+oo-l n1 n2 n3", 1);

    /* Invalid max_modes (< 1) is considered as 1 */
    WEE_CHECK_SPLIT_MODES("+o n1|+o n2", "+oo n1 n2", 0);
    WEE_CHECK_SPLIT_MODES("+o n1|+o n2", "+oo n1 n2", -1);

    irc_server_free (server);
}
