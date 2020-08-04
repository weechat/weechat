/*
 * test-gui-line.cpp - test line functions
 *
 * Copyright (C) 2018-2020 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include "src/core/wee-string.h"
#include "src/gui/gui-filter.h"
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
 *   gui_line_lines_alloc
 *   gui_line_lines_free
 */

TEST(GuiLine, LinesAlloc)
{
    struct t_gui_lines *lines;

    lines = gui_line_lines_alloc ();
    CHECK(lines);
    POINTERS_EQUAL(NULL, lines->first_line);
    POINTERS_EQUAL(NULL, lines->last_line);
    POINTERS_EQUAL(NULL, lines->last_read_line);
    LONGS_EQUAL(0, lines->lines_count);
    LONGS_EQUAL(0, lines->first_line_not_read);
    LONGS_EQUAL(0, lines->lines_hidden);
    LONGS_EQUAL(0, lines->buffer_max_length);
    LONGS_EQUAL(0, lines->buffer_max_length_refresh);
    LONGS_EQUAL(0, lines->prefix_max_length);
    LONGS_EQUAL(0, lines->prefix_max_length_refresh);

    gui_line_lines_free (lines);

    gui_line_lines_free (NULL);
}

/*
 * Tests functions:
 *   gui_line_tags_alloc
 *   gui_line_tags_free
 */

TEST(GuiLine, TagsAlloc)
{
    struct t_gui_line_data line_data;

    memset (&line_data, 0, sizeof (line_data));

    gui_line_tags_alloc (NULL, NULL);

    line_data.tags_array = (char **)0x1;
    line_data.tags_count = 1;
    gui_line_tags_alloc (&line_data, NULL);
    POINTERS_EQUAL(NULL, line_data.tags_array);
    LONGS_EQUAL(0, line_data.tags_count);

    line_data.tags_array = (char **)0x1;
    line_data.tags_count = 1;
    gui_line_tags_alloc (&line_data, "");
    POINTERS_EQUAL(NULL, line_data.tags_array);
    LONGS_EQUAL(0, line_data.tags_count);

    line_data.tags_array = NULL;
    line_data.tags_count = 0;
    gui_line_tags_alloc (&line_data, "tag1");
    CHECK(line_data.tags_array);
    STRCMP_EQUAL("tag1", line_data.tags_array[0]);
    POINTERS_EQUAL(NULL, line_data.tags_array[1]);
    LONGS_EQUAL(1, line_data.tags_count);
    gui_line_tags_free (&line_data);

    line_data.tags_array = NULL;
    line_data.tags_count = 0;
    gui_line_tags_alloc (&line_data, "tag1,tag2");
    CHECK(line_data.tags_array);
    STRCMP_EQUAL("tag1", line_data.tags_array[0]);
    STRCMP_EQUAL("tag2", line_data.tags_array[1]);
    POINTERS_EQUAL(NULL, line_data.tags_array[2]);
    LONGS_EQUAL(2, line_data.tags_count);
    gui_line_tags_free (&line_data);

    line_data.tags_array = NULL;
    line_data.tags_count = 0;
    gui_line_tags_alloc (&line_data, "tag1,tag2,tag3");
    CHECK(line_data.tags_array);
    STRCMP_EQUAL("tag1", line_data.tags_array[0]);
    STRCMP_EQUAL("tag2", line_data.tags_array[1]);
    STRCMP_EQUAL("tag3", line_data.tags_array[2]);
    POINTERS_EQUAL(NULL, line_data.tags_array[3]);
    LONGS_EQUAL(3, line_data.tags_count);
    gui_line_tags_free (&line_data);

    gui_line_tags_free (NULL);
}

/*
 * Tests functions:
 *   gui_line_prefix_is_same_nick
 */

TEST(GuiLine, PrefixIsSameNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_get_prefix_for_display
 */

TEST(GuiLine, GetPrefixForDisplay)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_get_align
 */

TEST(GuiLine, GetAlign)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_is_displayed
 */

TEST(GuiLine, IsDisplayed)
{
    struct t_gui_line line;
    int old_gui_filters_enabled;

    memset (&line, 0, sizeof (line));
    line.data = (struct t_gui_line_data *)malloc (sizeof (struct t_gui_line_data));

    LONGS_EQUAL(0, gui_line_is_displayed (NULL));

    old_gui_filters_enabled = gui_filters_enabled;

    /* filters NOT enabled: line is always displayed */
    gui_filters_enabled = 0;
    line.data->displayed = 0;
    LONGS_EQUAL(1, gui_line_is_displayed (&line));
    line.data->displayed = 1;
    LONGS_EQUAL(1, gui_line_is_displayed (&line));

    /* filters enabled: line is displayed only if displayed == 1 */
    gui_filters_enabled = 1;
    line.data->displayed = 0;
    LONGS_EQUAL(0, gui_line_is_displayed (&line));
    line.data->displayed = 1;
    LONGS_EQUAL(1, gui_line_is_displayed (&line));

    gui_filters_enabled = old_gui_filters_enabled;

    free (line.data);
}

/*
 * Tests functions:
 *   gui_line_get_first_displayed
 */

TEST(GuiLine, GetFirstDisplayed)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_get_last_displayed
 */

TEST(GuiLine, GetLastDisplayed)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_get_prev_displayed
 */

TEST(GuiLine, GetPrevDisplayed)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_get_next_displayed
 */

TEST(GuiLine, GetNextDisplayed)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_search_text
 */

TEST(GuiLine, SearchText)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_match_regex
 */

TEST(GuiLine, MatchRegex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_has_tag_no_filter
 */

TEST(GuiLine, HasTagNoFilter)
{
    struct t_gui_line_data line_data;

    memset (&line_data, 0, sizeof (line_data));

    LONGS_EQUAL(0, gui_line_has_tag_no_filter (NULL));
    LONGS_EQUAL(0, gui_line_has_tag_no_filter (&line_data));

    gui_line_tags_alloc (&line_data, "tag1,tag2");
    LONGS_EQUAL(0, gui_line_has_tag_no_filter (&line_data));
    gui_line_tags_free (&line_data);

    gui_line_tags_alloc (&line_data, "tag1,no_filter");
    LONGS_EQUAL(1, gui_line_has_tag_no_filter (&line_data));
    gui_line_tags_free (&line_data);

    gui_line_tags_alloc (&line_data, "tag1,no_filter,tag2");
    LONGS_EQUAL(1, gui_line_has_tag_no_filter (&line_data));
    gui_line_tags_free (&line_data);
}

/*
 * Tests functions:
 *   gui_line_match_tags
 */

TEST(GuiLine, MatchTags)
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

/*
 * Tests functions:
 *   gui_line_search_tag_starting_with
 */

TEST(GuiLine, SearchTagStartingWith)
{
    struct t_gui_line line;

    memset (&line, 0, sizeof (line));
    line.data = (struct t_gui_line_data *)malloc (sizeof (struct t_gui_line_data));
    gui_line_tags_alloc (line.data, NULL);

    POINTERS_EQUAL(NULL, gui_line_search_tag_starting_with (NULL, NULL));
    POINTERS_EQUAL(NULL, gui_line_search_tag_starting_with (NULL, "abc"));
    POINTERS_EQUAL(NULL, gui_line_search_tag_starting_with (&line, NULL));

    POINTERS_EQUAL(NULL, gui_line_search_tag_starting_with (&line, "abc"));

    gui_line_tags_alloc (line.data, "tag1,word2,last3");

    POINTERS_EQUAL(NULL, gui_line_search_tag_starting_with (&line, "abc"));
    STRCMP_EQUAL("tag1", gui_line_search_tag_starting_with (&line, "t"));
    STRCMP_EQUAL("tag1", gui_line_search_tag_starting_with (&line, "tag"));
    STRCMP_EQUAL("word2", gui_line_search_tag_starting_with (&line, "w"));
    STRCMP_EQUAL("word2", gui_line_search_tag_starting_with (&line, "word"));
    STRCMP_EQUAL("last3", gui_line_search_tag_starting_with (&line, "l"));
    STRCMP_EQUAL("last3", gui_line_search_tag_starting_with (&line, "last"));

    gui_line_tags_free (line.data);

    free (line.data);
}

/*
 * Tests functions:
 *   gui_line_get_nick_tag
 */

TEST(GuiLine, GetNickTag)
{
    struct t_gui_line line;

    memset (&line, 0, sizeof (line));
    line.data = (struct t_gui_line_data *)malloc (sizeof (struct t_gui_line_data));
    gui_line_tags_alloc (line.data, NULL);

    POINTERS_EQUAL(NULL, gui_line_get_nick_tag (NULL));

    POINTERS_EQUAL(NULL, gui_line_get_nick_tag (&line));

    gui_line_tags_alloc (line.data, "tag1,tag2,tag3");
    POINTERS_EQUAL(NULL, gui_line_get_nick_tag (&line));
    gui_line_tags_free (line.data);

    gui_line_tags_alloc (line.data, "tag1,tag2,nick_jdoe,tag3");
    STRCMP_EQUAL("jdoe", gui_line_get_nick_tag (&line));
    gui_line_tags_free (line.data);

    free (line.data);
}

/*
 * Tests functions:
 *   gui_line_has_highlight
 */

TEST(GuiLine, HasHighlight)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_has_offline_nick
 */

TEST(GuiLine, HasOfflineNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_compute_buffer_max_length
 */

TEST(GuiLine, ComputeBufferMaxLength)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_compute_prefix_max_length
 */

TEST(GuiLine, ComputePrefixMaxLength)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_add_to_list
 */

TEST(GuiLine, AddToList)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_free_data
 */

TEST(GuiLine, FreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_remove_from_list
 */

TEST(GuiLine, RemoveFromList)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_mixed_add
 */

TEST(GuiLine, MixedAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_mixed_free_buffer
 */

TEST(GuiLine, MixedFreeBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_mixed_free_all
 */

TEST(GuiLine, MixedFreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_free
 */

TEST(GuiLine, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_free_all
 */

TEST(GuiLine, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_get_notify_level
 */

TEST(GuiLine, GetNotifyLevel)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_get_highlight
 */

TEST(GuiLine, GetHighlight)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_new
 */

TEST(GuiLine, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_hook_update
 */

TEST(GuiLine, HookUpdate)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_add
 */

TEST(GuiLine, Add)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_add_y
 */

TEST(GuiLine, AddY)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_clear
 */

TEST(GuiLine, Clear)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_mix_buffers
 */

TEST(GuiLine, MixBuffers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_hdata_lines_cb
 */

TEST(GuiLine, HdataLinesCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_hdata_line_cb
 */

TEST(GuiLine, HdataLineCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_hdata_line_data_update_cb
 */

TEST(GuiLine, HdataLineDataUpdateCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_hdata_line_data_cb
 */

TEST(GuiLine, HdataLineDataCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_line_add_to_infolist
 */

TEST(GuiLine, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_lines_print_log
 */

TEST(GuiLine, HdataPrintLog)
{
    /* TODO: write tests */
}
