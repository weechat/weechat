/*
 * test-gui-filter.cpp - test filter functions
 *
 * Copyright (C) 2022-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-filter.h"
#include "src/gui/gui-line.h"

extern struct t_gui_filter *gui_filter_find_pos (struct t_gui_filter *filter);

}

TEST_GROUP(GuiFilter)
{
};

/*
 * Tests functions:
 *   gui_filter_check_line
 */

TEST(GuiFilter, CheckLine)
{
    struct t_gui_filter *filter1, *filter2;
    struct t_gui_line_data *line_data, *line_data_no_filter;

    gui_chat_printf_date_tags (NULL, 0, "tag1,tag2,tag3", "this is a test");
    gui_chat_printf_date_tags (NULL, 0, "no_filter", "this is a test");

    line_data = gui_buffers->lines->last_line->prev_line->data;
    line_data_no_filter = gui_buffers->lines->last_line->data;

    LONGS_EQUAL(1, gui_filter_check_line (line_data));

    filter1 = gui_filter_new (1, "test1", "irc.test.#chan", "tag_xxx", "xxx");
    filter2 = gui_filter_new (1, "test2", "*", "*", "this is");

    LONGS_EQUAL(0, gui_filter_check_line (line_data));
    LONGS_EQUAL(1, gui_filter_check_line (line_data_no_filter));

    filter2->enabled = 0;
    LONGS_EQUAL(1, gui_filter_check_line (line_data));
    filter2->enabled = 1;
    LONGS_EQUAL(0, gui_filter_check_line (line_data));

    gui_filter_free (filter1);
    gui_filter_free (filter2);

    LONGS_EQUAL(1, gui_filter_check_line (line_data));
    LONGS_EQUAL(1, gui_filter_check_line (line_data_no_filter));

    filter1 = gui_filter_new (1, "test1", "*", "*", "!xxx");
    LONGS_EQUAL(0, gui_filter_check_line (line_data));
    LONGS_EQUAL(1, gui_filter_check_line (line_data_no_filter));

    gui_filter_free (filter1);
}

/*
 * Tests functions:
 *   gui_filter_buffer
 */

TEST(GuiFilter, Buffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_filter_all_buffers
 */

TEST(GuiFilter, AllBuffers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_filter_global_enable
 *   gui_filter_global_disable
 */

TEST(GuiFilter, GlobalEnableDisable)
{
    LONGS_EQUAL(1, gui_filters_enabled);
    gui_filter_global_disable ();
    LONGS_EQUAL(0, gui_filters_enabled);
    gui_filter_global_enable ();
    LONGS_EQUAL(1, gui_filters_enabled);
}

/*
 * Tests functions:
 *   gui_filter_search_by_name
 */

TEST(GuiFilter, SearchByName)
{
    struct t_gui_filter *filter_abc, *filter_xyz;

    POINTERS_EQUAL(NULL, gui_filter_search_by_name (NULL));
    POINTERS_EQUAL(NULL, gui_filter_search_by_name (""));
    POINTERS_EQUAL(NULL, gui_filter_search_by_name ("abc"));
    POINTERS_EQUAL(NULL, gui_filter_search_by_name ("xyz"));
    POINTERS_EQUAL(NULL, gui_filter_search_by_name ("zzz"));

    filter_xyz = gui_filter_new (1, "xyz", "*", "tag_xyz", "regex_xyz");
    POINTERS_EQUAL(NULL, gui_filter_search_by_name ("abc"));
    POINTERS_EQUAL(filter_xyz, gui_filter_search_by_name ("xyz"));
    POINTERS_EQUAL(NULL, gui_filter_search_by_name ("zzz"));

    filter_abc = gui_filter_new (1, "abc", "*", "tag_abc", "regex_abc");
    POINTERS_EQUAL(filter_abc, gui_filter_search_by_name ("abc"));
    POINTERS_EQUAL(filter_xyz, gui_filter_search_by_name ("xyz"));
    POINTERS_EQUAL(NULL, gui_filter_search_by_name ("zzz"));

    gui_filter_free (filter_abc);
    gui_filter_free (filter_xyz);
}

/*
 * Tests functions:
 *   gui_filter_new_error
 */

TEST(GuiFilter, NewError)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_filter_find_pos
 *   gui_filter_add_to_list
 *   gui_filter_remove_from_list
 *   gui_filter_new
 *   gui_filter_free
 *   gui_filter_free_all
 */

TEST(GuiFilter, New)
{
    struct t_gui_filter *filter_abc, *filter_test, *filter_xyz;

    POINTERS_EQUAL(NULL, gui_filter_new (1, NULL, NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, gui_filter_new (1, "test", NULL, NULL, NULL));
    POINTERS_EQUAL(NULL, gui_filter_new (1, "test", "*", NULL, NULL));
    POINTERS_EQUAL(NULL, gui_filter_new (1, "test", "*", "tag_abc", NULL));

    /* invalid regex */
    POINTERS_EQUAL(NULL, gui_filter_new (0, "abc", "*", "*", "prefix\\t*abc"));
    POINTERS_EQUAL(NULL, gui_filter_new (0, "abc", "*", "*", "*prefix\\tabc"));

    filter_abc = gui_filter_new (0, "abc", "*", "tag_abc", "!regex_abc");
    LONGS_EQUAL(0, filter_abc->enabled);
    STRCMP_EQUAL("abc", filter_abc->name);
    STRCMP_EQUAL("*", filter_abc->buffer_name);
    LONGS_EQUAL(1, filter_abc->num_buffers);
    STRCMP_EQUAL("*", filter_abc->buffers[0]);
    POINTERS_EQUAL(NULL, filter_abc->buffers[1]);
    STRCMP_EQUAL("tag_abc", filter_abc->tags);
    LONGS_EQUAL(1, filter_abc->tags_count);
    STRCMP_EQUAL("tag_abc", filter_abc->tags_array[0][0]);
    POINTERS_EQUAL(NULL, filter_abc->tags_array[0][1]);
    POINTERS_EQUAL(NULL, filter_abc->tags_array[1]);
    STRCMP_EQUAL("!regex_abc", filter_abc->regex);
    POINTERS_EQUAL(NULL, filter_abc->regex_prefix);
    CHECK(filter_abc->regex_message);
    POINTERS_EQUAL(NULL, filter_abc->prev_filter);
    POINTERS_EQUAL(NULL, filter_abc->next_filter);
    POINTERS_EQUAL(filter_abc, gui_filters);
    POINTERS_EQUAL(filter_abc, last_gui_filter);

    /* filter already existing */
    POINTERS_EQUAL(NULL, gui_filter_new (1, "abc", "*", "tag2_abc", "regex2_abc"));

    filter_xyz = gui_filter_new (1, "xyz", "irc.test.#chan,irc.test.#chan2",
                                 "tag_xyz,tag2_xyz+tag3_xyz", "prefix\\txyz");
    LONGS_EQUAL(1, filter_xyz->enabled);
    STRCMP_EQUAL("xyz", filter_xyz->name);
    STRCMP_EQUAL("irc.test.#chan,irc.test.#chan2", filter_xyz->buffer_name);
    LONGS_EQUAL(2, filter_xyz->num_buffers);
    STRCMP_EQUAL("irc.test.#chan", filter_xyz->buffers[0]);
    STRCMP_EQUAL("irc.test.#chan2", filter_xyz->buffers[1]);
    POINTERS_EQUAL(NULL, filter_xyz->buffers[2]);
    STRCMP_EQUAL("tag_xyz,tag2_xyz+tag3_xyz", filter_xyz->tags);
    LONGS_EQUAL(2, filter_xyz->tags_count);
    STRCMP_EQUAL("tag_xyz", filter_xyz->tags_array[0][0]);
    POINTERS_EQUAL(NULL, filter_xyz->tags_array[0][1]);
    STRCMP_EQUAL("tag2_xyz", filter_xyz->tags_array[1][0]);
    STRCMP_EQUAL("tag3_xyz", filter_xyz->tags_array[1][1]);
    POINTERS_EQUAL(NULL, filter_xyz->tags_array[1][2]);
    POINTERS_EQUAL(NULL, filter_xyz->tags_array[2]);
    STRCMP_EQUAL("prefix\\txyz", filter_xyz->regex);
    CHECK(filter_xyz->regex_prefix);
    CHECK(filter_xyz->regex_message);
    POINTERS_EQUAL(filter_abc, filter_xyz->prev_filter);
    POINTERS_EQUAL(NULL, filter_xyz->next_filter);
    POINTERS_EQUAL(filter_abc, gui_filters);
    POINTERS_EQUAL(filter_xyz, last_gui_filter);

    filter_test = gui_filter_new (1, "test", "*", "*", "regex_test");
    LONGS_EQUAL(1, filter_test->enabled);
    STRCMP_EQUAL("test", filter_test->name);
    STRCMP_EQUAL("*", filter_test->buffer_name);
    LONGS_EQUAL(1, filter_test->num_buffers);
    STRCMP_EQUAL("*", filter_test->buffers[0]);
    POINTERS_EQUAL(NULL, filter_test->buffers[1]);
    STRCMP_EQUAL("*", filter_test->tags);
    LONGS_EQUAL(1, filter_test->tags_count);
    STRCMP_EQUAL("*", filter_test->tags_array[0][0]);
    POINTERS_EQUAL(NULL, filter_test->tags_array[0][1]);
    POINTERS_EQUAL(NULL, filter_test->tags_array[1]);
    STRCMP_EQUAL("regex_test", filter_test->regex);
    POINTERS_EQUAL(NULL, filter_test->regex_prefix);
    CHECK(filter_test->regex_message);
    POINTERS_EQUAL(filter_abc, filter_test->prev_filter);
    POINTERS_EQUAL(filter_xyz, filter_test->next_filter);
    POINTERS_EQUAL(filter_abc, gui_filters);
    POINTERS_EQUAL(filter_xyz, last_gui_filter);

    gui_filter_free (filter_test);

    gui_filter_free (NULL);

    gui_filter_free_all ();
}

/*
 * Tests functions:
 *   gui_filter_rename
 */

TEST(GuiFilter, Rename)
{
    struct t_gui_filter *filter1, *filter2;

    LONGS_EQUAL(0, gui_filter_rename (NULL, NULL));

    filter1 = gui_filter_new (1, "abc", "*", "tag_abc", "regex_abc");
    POINTERS_EQUAL(filter1, gui_filters);
    POINTERS_EQUAL(filter1, last_gui_filter);

    filter2 = gui_filter_new (1, "xyz", "*", "tag_xyz", "regex_xyz");
    POINTERS_EQUAL(filter1, gui_filters);
    POINTERS_EQUAL(filter2, last_gui_filter);

    LONGS_EQUAL(0, gui_filter_rename (filter1, NULL));
    LONGS_EQUAL(0, gui_filter_rename (filter1, "abc"));

    LONGS_EQUAL(1, gui_filter_rename (filter1, "a"));
    STRCMP_EQUAL("a", filter1->name);
    POINTERS_EQUAL(filter1, gui_filters);
    POINTERS_EQUAL(filter2, last_gui_filter);

    LONGS_EQUAL(1, gui_filter_rename (filter1, "z"));
    STRCMP_EQUAL("z", filter1->name);
    POINTERS_EQUAL(filter2, gui_filters);
    POINTERS_EQUAL(filter1, last_gui_filter);

    LONGS_EQUAL(1, gui_filter_rename (filter2, "zzz"));
    STRCMP_EQUAL("zzz", filter2->name);
    POINTERS_EQUAL(filter1, gui_filters);
    POINTERS_EQUAL(filter2, last_gui_filter);

    gui_filter_free (filter1);
    gui_filter_free (filter2);
}

/*
 * Tests functions:
 *   gui_filter_hdata_filter_cb
 */

TEST(GuiFilter, HdataFilterCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_filter_add_to_infolist
 */

TEST(GuiFilter, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_filter_print_log
 */

TEST(GuiFilter, PrintLog)
{
    /* TODO: write tests */
}
