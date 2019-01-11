/*
 * test-gui-line.cpp - test line functions
 *
 * Copyright (C) 2018-2019 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/core/wee-string.h"
#include "src/gui/gui-line.h"
}

#define WEE_LINE_MATCH_TAGS(__result, __line_tags, __tags)              \
    gui_line_tags_alloc (&line_data, __line_tags);                      \
    tags_array = string_split_tags (__tags, &tags_count);               \
    LONGS_EQUAL(__result, gui_line_match_tags (&line_data, tags_count,  \
                                               tags_array));            \
    gui_line_tags_free (&line_data);                                    \
    string_free_split_tags (tags_array);

TEST_GROUP(GuiLine)
{
};

/*
 * Tests functions:
 *   gui_line_match_tags
 */

TEST(GuiLine, LineMatchTags)
{
    struct t_gui_line_data line_data;
    char ***tags_array;
    int tags_count;

    /* line without tags */
    WEE_LINE_MATCH_TAGS(0, NULL, NULL);
    WEE_LINE_MATCH_TAGS(0, NULL, "irc_join");
    WEE_LINE_MATCH_TAGS(0, NULL, "!*");
    WEE_LINE_MATCH_TAGS(1, NULL, "!irc_join");
    WEE_LINE_MATCH_TAGS(1, NULL, "*");

    /* line with one tag */
    WEE_LINE_MATCH_TAGS(0, "irc_join", NULL);
    WEE_LINE_MATCH_TAGS(0, "irc_join", "irc_quit");
    WEE_LINE_MATCH_TAGS(0, "irc_join", "!*");
    WEE_LINE_MATCH_TAGS(1, "irc_join", "irc_join,irc_quit");
    WEE_LINE_MATCH_TAGS(1, "irc_join", "*");
    WEE_LINE_MATCH_TAGS(1, "irc_join", "irc_quit,*");

    /* line with two tags */
    WEE_LINE_MATCH_TAGS(0, "irc_join,nick_test", NULL);
    WEE_LINE_MATCH_TAGS(0, "irc_join,nick_test", "irc_quit");
    WEE_LINE_MATCH_TAGS(0, "irc_join,nick_test", "irc_part,irc_quit");
    WEE_LINE_MATCH_TAGS(0, "irc_join,nick_test", "irc_join+nick_xxx,irc_quit");
    WEE_LINE_MATCH_TAGS(0, "irc_join,nick_test", "!irc_join,!irc_quit");
    WEE_LINE_MATCH_TAGS(1, "irc_join,nick_test", "*");
    WEE_LINE_MATCH_TAGS(1, "irc_join,nick_test", "irc_quit,*");
    WEE_LINE_MATCH_TAGS(1, "irc_join,nick_test", "!irc_quit");
    WEE_LINE_MATCH_TAGS(1, "irc_join,nick_test", "irc_join+nick_test,irc_quit");
    WEE_LINE_MATCH_TAGS(1, "irc_join,nick_test", "nick_test,irc_quit");
    WEE_LINE_MATCH_TAGS(1, "irc_join,nick_test", "!irc_quit,!irc_302,!irc_notice");
    WEE_LINE_MATCH_TAGS(1, "irc_join,nick_test", "!irc_quit+!irc_302+!irc_notice");
}
