/*
 * test-irc-mode.cpp - test IRC mode functions
 *
 * Copyright (C) 2019-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/plugins/irc/irc-mode.h"
}

#define WEE_CHECK_GET_ARGS(__result, __arguments)                       \
    str = irc_mode_get_arguments (__arguments);                         \
    if (__result == NULL)                                               \
    {                                                                   \
        POINTERS_EQUAL(NULL, str);                                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        STRCMP_EQUAL(__result, str);                                    \
    }                                                                   \
    if (str)                                                            \
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
    char *str;

    /* invalid arguments */
    WEE_CHECK_GET_ARGS("", irc_mode_get_arguments (NULL));
    WEE_CHECK_GET_ARGS("", irc_mode_get_arguments (""));
    WEE_CHECK_GET_ARGS("", irc_mode_get_arguments (" "));

    /* simple arguments */
    WEE_CHECK_GET_ARGS("abc", irc_mode_get_arguments ("abc"));
    WEE_CHECK_GET_ARGS("abc def", irc_mode_get_arguments ("abc def"));
    WEE_CHECK_GET_ARGS("abc def ghi", irc_mode_get_arguments ("abc def ghi"));

    /* some arguments starting with a colon */
    WEE_CHECK_GET_ARGS("abc", irc_mode_get_arguments (":abc"));
    WEE_CHECK_GET_ARGS("abc def", irc_mode_get_arguments (":abc def"));
    WEE_CHECK_GET_ARGS("abc def", irc_mode_get_arguments ("abc :def"));
    WEE_CHECK_GET_ARGS("abc def ghi", irc_mode_get_arguments ("abc :def ghi"));
    WEE_CHECK_GET_ARGS("abc def ghi", irc_mode_get_arguments ("abc :def :ghi"));
    WEE_CHECK_GET_ARGS("abc def ghi", irc_mode_get_arguments (":abc :def :ghi"));
}
