/*
 * test-irc-list.cpp - test IRC list functions
 *
 * Copyright (C) 2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/plugins/irc/irc-list.h"
}

TEST_GROUP(IrcList)
{
};

/*
 * Tests functions:
 *   irc_list_compare_cb
 */

TEST(IrcList, IrcListCompareCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_free_cb
 */

TEST(IrcList, IrcListFreeCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_buffer_set_localvar_filter
 */

TEST(IrcList, IrcListBufferSetLocalvarFilter)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_set_filter
 */

TEST(IrcList, IrcListSetFilter)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_set_sort
 */

TEST(IrcList, IrcListSetSort)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_add_channel_in_hashtable
 */

TEST(IrcList, IrcListAddChannelInHashtable)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_string_match
 */

TEST(IrcList, IrcListStringMatch)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_channel_match_filter
 */

TEST(IrcList, IrcListChannelMatchFilter)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_filter_channels
 */

TEST(IrcList, IrcListFilterChannels)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_parse_messages
 */

TEST(IrcList, IrcListParseMessages)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_buffer_set_title
 */

TEST(IrcList, IrcListBufferSetTitle)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_display_line
 */

TEST(IrcList, IrcListDisplayLine)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_buffer_refresh
 */

TEST(IrcList, IrcListBufferRefresh)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_set_current_line
 */

TEST(IrcList, IrcListSetCurrentLine)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_get_window_info
 */

TEST(IrcList, IrcListGetWindowInfo)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_check_line_outside_window
 */

TEST(IrcList, IrcListCheckLineOutsideWindow)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_window_scrolled_cb
 */

TEST(IrcList, IrcListWindowScrolledCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_move_line_relative
 */

TEST(IrcList, IrcListMoveLineRelative)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_move_line_absolute
 */

TEST(IrcList, IrcListMoveLineAbsolute)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_join_channel
 */

TEST(IrcList, IrcListJoinChannel)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_buffer_input_data_cb
 */

TEST(IrcList, IrcListBufferInputDataCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_create_buffer
 */

TEST(IrcList, IrcListCreateBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_hsignal_redirect_list_cb
 */

TEST(IrcList, IrcListHsignalRedirectListCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_alloc
 */

TEST(IrcList, IrcListAlloc)
{
    struct t_irc_list *list;

    list = irc_list_alloc ();
    CHECK(list);
    POINTERS_EQUAL(NULL, list->buffer);
    POINTERS_EQUAL(NULL, list->channels);
    POINTERS_EQUAL(NULL, list->filter_channels);
    LONGS_EQUAL(0, list->name_max_length);
    POINTERS_EQUAL(NULL, list->filter);
    POINTERS_EQUAL(NULL, list->sort);
    POINTERS_EQUAL(NULL, list->sort_fields);
    LONGS_EQUAL(0, list->sort_fields_count);
    LONGS_EQUAL(0, list->selected_line);
}

/*
 * Tests functions:
 *   irc_list_free_data
 */

TEST(IrcList, IrcListFreeData)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_free
 */

TEST(IrcList, IrcListFree)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_hdata_list_channel_cb
 */

TEST(IrcList, IrcListHdataListChannelCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_list_hdata_list_cb
 */

TEST(IrcList, IrcListHdataListCb)
{
    /* TODO: write tests */
}
