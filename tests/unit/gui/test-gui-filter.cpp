/*
 * SPDX-FileCopyrightText: 2022-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test filter functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include "src/core/core-hook.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-filter.h"
#include "src/gui/gui-line.h"
#include "src/plugins/weechat-plugin.h"

extern struct t_gui_filter *gui_filter_find_pos (struct t_gui_filter *filter);

}

/* Number of signals "buffer_lines_filtered" received, and last buffer. */
int test_filter_signal_count = 0;
struct t_gui_buffer *test_filter_signal_buffer = NULL;

/* Names of buffers received with the signal, separated by commas. */
char test_filter_signal_buffers[1024];

/* Buffers closed by the signal callback, if set. */
struct t_gui_buffer *test_filter_close_buffer1 = NULL;
struct t_gui_buffer *test_filter_close_buffer2 = NULL;

/*
 * Callback for signal "buffer_lines_filtered": count signals received.
 */

int
test_filter_signal_cb (const void *pointer, void *data, const char *signal,
                       const char *type_data, void *signal_data)
{
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    test_filter_signal_count++;
    test_filter_signal_buffer = (struct t_gui_buffer *)signal_data;
    if (test_filter_signal_buffers[0])
    {
        strncat (test_filter_signal_buffers, ",",
                 sizeof (test_filter_signal_buffers)
                 - strlen (test_filter_signal_buffers) - 1);
    }
    strncat (test_filter_signal_buffers,
             test_filter_signal_buffer->full_name,
             sizeof (test_filter_signal_buffers)
             - strlen (test_filter_signal_buffers) - 1);

    /* Close the other buffer (signal must not be sent for this buffer). */
    if (test_filter_close_buffer1 && test_filter_close_buffer2)
    {
        gui_buffer_close ((signal_data == test_filter_close_buffer1) ?
                          test_filter_close_buffer2 : test_filter_close_buffer1);
        test_filter_close_buffer1 = NULL;
        test_filter_close_buffer2 = NULL;
    }

    return WEECHAT_RC_OK;
}

TEST_GROUP(GuiFilter)
{
};

/*
 * Test functions:
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
 * Test functions:
 *   gui_filter_buffer
 */

TEST(GuiFilter, Buffer)
{
    struct t_gui_buffer *buffer1, *buffer2;
    struct t_gui_filter *filter;
    struct t_gui_line_data *line_data1, *line_data2;
    struct t_hook *hook;

    test_filter_signal_count = 0;
    test_filter_signal_buffer = NULL;
    test_filter_signal_buffers[0] = '\0';

    buffer1 = gui_buffer_new (NULL, "test_filter_1",
                              NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer1);
    buffer2 = gui_buffer_new (NULL, "test_filter_2",
                              NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer2);

    gui_chat_printf_date_tags (buffer1, 0, "tag_test_filter", "line 1");
    gui_chat_printf_date_tags (buffer2, 0, "tag_test_filter", "line 2");
    line_data1 = buffer1->own_lines->last_line->data;
    line_data2 = buffer2->own_lines->last_line->data;

    hook = hook_signal (NULL, "buffer_lines_filtered",
                        &test_filter_signal_cb, NULL, NULL);

    /* No filter: no line changed, no signal. */
    gui_filter_buffer (buffer1, NULL);
    LONGS_EQUAL(0, test_filter_signal_count);
    LONGS_EQUAL(1, line_data1->displayed);

    /* Filter added: line filtered, signal sent. */
    filter = gui_filter_new (1, "test", "core.test_filter_1",
                             "tag_test_filter", "*");
    CHECK(filter);
    gui_filter_buffer (buffer1, NULL);
    LONGS_EQUAL(1, test_filter_signal_count);
    POINTERS_EQUAL(buffer1, test_filter_signal_buffer);
    LONGS_EQUAL(0, line_data1->displayed);
    LONGS_EQUAL(1, buffer1->own_lines->lines_hidden);

    /* No line changed: no signal. */
    gui_filter_buffer (buffer1, NULL);
    LONGS_EQUAL(1, test_filter_signal_count);

    /* Filter removed: line unfiltered, signal sent. */
    gui_filter_free (filter);
    gui_filter_buffer (buffer1, NULL);
    LONGS_EQUAL(2, test_filter_signal_count);
    POINTERS_EQUAL(buffer1, test_filter_signal_buffer);
    LONGS_EQUAL(1, line_data1->displayed);
    LONGS_EQUAL(0, buffer1->own_lines->lines_hidden);

    /* Filter of a single line: no signal. */
    filter = gui_filter_new (1, "test", "core.test_filter_1",
                             "tag_test_filter", "*");
    CHECK(filter);
    gui_filter_buffer (buffer1, line_data1);
    LONGS_EQUAL(2, test_filter_signal_count);
    LONGS_EQUAL(0, line_data1->displayed);
    gui_filter_free (filter);
    gui_filter_buffer (buffer1, line_data1);
    LONGS_EQUAL(2, test_filter_signal_count);
    LONGS_EQUAL(1, line_data1->displayed);

    /*
     * Merged buffers: signal sent only for merged buffers with lines
     * filtered or unfiltered.
     */
    gui_buffer_merge (buffer2, buffer1);
    POINTERS_EQUAL(buffer1->mixed_lines, buffer2->mixed_lines);
    filter = gui_filter_new (1, "test", "core.test_filter_1",
                             "tag_test_filter", "*");
    CHECK(filter);
    gui_filter_buffer (buffer2, NULL);
    LONGS_EQUAL(3, test_filter_signal_count);
    POINTERS_EQUAL(buffer1, test_filter_signal_buffer);
    LONGS_EQUAL(0, line_data1->displayed);
    LONGS_EQUAL(1, line_data2->displayed);
    gui_filter_free (filter);
    filter = gui_filter_new (1, "test", "core.test_filter_*",
                             "tag_test_filter", "*");
    CHECK(filter);
    gui_filter_buffer (buffer1, NULL);
    LONGS_EQUAL(4, test_filter_signal_count);
    POINTERS_EQUAL(buffer2, test_filter_signal_buffer);
    LONGS_EQUAL(0, line_data1->displayed);
    LONGS_EQUAL(0, line_data2->displayed);
    test_filter_signal_buffers[0] = '\0';
    gui_filter_free (filter);
    gui_filter_buffer (buffer1, NULL);
    LONGS_EQUAL(6, test_filter_signal_count);
    LONGS_EQUAL(1, line_data1->displayed);
    LONGS_EQUAL(1, line_data2->displayed);
    /* Signals sent in order of the first line changed in each buffer. */
    STRCMP_EQUAL("core.test_filter_1,core.test_filter_2",
                 test_filter_signal_buffers);

    /* Merged buffer closed by the signal callback: no signal sent for it. */
    test_filter_close_buffer1 = buffer1;
    test_filter_close_buffer2 = buffer2;
    filter = gui_filter_new (1, "test", "core.test_filter_*",
                             "tag_test_filter", "*");
    CHECK(filter);
    test_filter_signal_buffers[0] = '\0';
    gui_filter_buffer (buffer1, NULL);
    LONGS_EQUAL(7, test_filter_signal_count);
    POINTERS_EQUAL(NULL, test_filter_close_buffer1);
    STRCMP_EQUAL("core.test_filter_1", test_filter_signal_buffers);
    gui_filter_free (filter);

    unhook (hook);

    if (gui_buffer_valid (buffer2))
        gui_buffer_close (buffer2);
    if (gui_buffer_valid (buffer1))
        gui_buffer_close (buffer1);
}

/*
 * Test functions:
 *   gui_filter_all_buffers
 */

TEST(GuiFilter, AllBuffers)
{
    /* TODO: write tests */
}

/*
 * Test functions:
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
 * Test functions:
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
 * Test functions:
 *   gui_filter_new_error
 */

TEST(GuiFilter, NewError)
{
    /* TODO: write tests */
}

/*
 * Test functions:
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

    /* Invalid regex */
    POINTERS_EQUAL(NULL, gui_filter_new (0, "abc", "*", "*", "prefix\\t*abc"));
    POINTERS_EQUAL(NULL, gui_filter_new (0, "abc", "*", "*", "*prefix\\tabc"));

    filter_abc = gui_filter_new (0, "abc", "*", "tag_abc", "!regex_abc");
    LONGS_EQUAL(0, filter_abc->enabled);
    STRCMP_EQUAL("abc", filter_abc->name);
    STRCMP_EQUAL("*", filter_abc->buffer_name);
    LONGS_EQUAL(1, filter_abc->num_buffers);
    STRCMP_EQUAL("*", filter_abc->buffers[0]);
    STRCMP_EQUAL(NULL, filter_abc->buffers[1]);
    STRCMP_EQUAL("tag_abc", filter_abc->tags);
    LONGS_EQUAL(1, filter_abc->tags_count);
    STRCMP_EQUAL("tag_abc", filter_abc->tags_array[0][0]);
    STRCMP_EQUAL(NULL, filter_abc->tags_array[0][1]);
    POINTERS_EQUAL(NULL, filter_abc->tags_array[1]);
    STRCMP_EQUAL("!regex_abc", filter_abc->regex);
    POINTERS_EQUAL(NULL, filter_abc->regex_prefix);
    CHECK(filter_abc->regex_message);
    POINTERS_EQUAL(NULL, filter_abc->prev_filter);
    POINTERS_EQUAL(NULL, filter_abc->next_filter);
    POINTERS_EQUAL(filter_abc, gui_filters);
    POINTERS_EQUAL(filter_abc, last_gui_filter);

    /* Filter already existing */
    POINTERS_EQUAL(NULL, gui_filter_new (1, "abc", "*", "tag2_abc", "regex2_abc"));

    filter_xyz = gui_filter_new (1, "xyz", "irc.test.#chan,irc.test.#chan2",
                                 "tag_xyz,tag2_xyz+tag3_xyz", "prefix\\txyz");
    LONGS_EQUAL(1, filter_xyz->enabled);
    STRCMP_EQUAL("xyz", filter_xyz->name);
    STRCMP_EQUAL("irc.test.#chan,irc.test.#chan2", filter_xyz->buffer_name);
    LONGS_EQUAL(2, filter_xyz->num_buffers);
    STRCMP_EQUAL("irc.test.#chan", filter_xyz->buffers[0]);
    STRCMP_EQUAL("irc.test.#chan2", filter_xyz->buffers[1]);
    STRCMP_EQUAL(NULL, filter_xyz->buffers[2]);
    STRCMP_EQUAL("tag_xyz,tag2_xyz+tag3_xyz", filter_xyz->tags);
    LONGS_EQUAL(2, filter_xyz->tags_count);
    STRCMP_EQUAL("tag_xyz", filter_xyz->tags_array[0][0]);
    STRCMP_EQUAL(NULL, filter_xyz->tags_array[0][1]);
    STRCMP_EQUAL("tag2_xyz", filter_xyz->tags_array[1][0]);
    STRCMP_EQUAL("tag3_xyz", filter_xyz->tags_array[1][1]);
    STRCMP_EQUAL(NULL, filter_xyz->tags_array[1][2]);
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
    STRCMP_EQUAL(NULL, filter_test->buffers[1]);
    STRCMP_EQUAL("*", filter_test->tags);
    LONGS_EQUAL(1, filter_test->tags_count);
    STRCMP_EQUAL("*", filter_test->tags_array[0][0]);
    STRCMP_EQUAL(NULL, filter_test->tags_array[0][1]);
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
 * Test functions:
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
 * Test functions:
 *   gui_filter_hdata_filter_cb
 */

TEST(GuiFilter, HdataFilterCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_filter_add_to_infolist
 */

TEST(GuiFilter, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_filter_print_log
 */

TEST(GuiFilter, PrintLog)
{
    /* TODO: write tests */
}
