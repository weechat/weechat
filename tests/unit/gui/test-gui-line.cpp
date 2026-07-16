/*
 * SPDX-FileCopyrightText: 2018-2026 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Test line functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "src/core/core-config.h"
#include "src/core/core-config-file.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-chat.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-filter.h"
#include "src/gui/gui-hotlist.h"
#include "src/gui/gui-line.h"
}

#define WEE_BUILD_STR_PREFIX_MSG(__result, __prefix, __message)         \
    line = gui_line_new (gui_buffers, -1, 0, 0, 0, 0, "tag1,tag2",      \
                         __prefix, __message, -1);                      \
    str = gui_line_build_string_prefix_message (line->data->prefix,     \
                                                line->data->message);   \
    STRCMP_EQUAL(__result, str);                                        \
    free (str);                                                         \
    gui_line_free_data (line);                                          \
    free (line);

#define WEE_BUILD_STR_MSG_TAGS(__tags, __message, __colors)             \
    line = gui_line_new (gui_buffers, -1, 0, 0, 0, 0, __tags,           \
                         NULL, __message, -1);                          \
    str = gui_line_build_string_message_tags (line->data->message,      \
                                              line->data->tags_count,   \
                                              line->data->tags_array,   \
                                              __colors);                \
    STRCMP_EQUAL(str_result, str);                                      \
    free (str);                                                         \
    gui_line_free_data (line);                                          \
    free (line);

#define WEE_LINE_MATCH_TAGS(__result, __line_tags, __tags)              \
    gui_line_tags_alloc (&line_data, __line_tags);                      \
    tags_array = string_split_tags (__tags, &tags_count);               \
    LONGS_EQUAL(__result, gui_line_match_tags (&line_data, tags_count,  \
                                               tags_array));            \
    gui_line_tags_free (&line_data);                                    \
    string_free_split_tags (tags_array);

#define WEE_LINE_ADD_Y(__y, __msg)                                      \
    gui_line_add_y (gui_line_new (buffer, (__y), 0, 0, 0, 0,            \
                                  NULL, NULL, (__msg), -1))

TEST_GROUP(GuiLine)
{
};

/*
 * Test functions:
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
 * Test functions:
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
    STRCMP_EQUAL(NULL, line_data.tags_array[1]);
    LONGS_EQUAL(1, line_data.tags_count);
    gui_line_tags_free (&line_data);

    line_data.tags_array = NULL;
    line_data.tags_count = 0;
    gui_line_tags_alloc (&line_data, "tag1,tag2");
    CHECK(line_data.tags_array);
    STRCMP_EQUAL("tag1", line_data.tags_array[0]);
    STRCMP_EQUAL("tag2", line_data.tags_array[1]);
    STRCMP_EQUAL(NULL, line_data.tags_array[2]);
    LONGS_EQUAL(2, line_data.tags_count);
    gui_line_tags_free (&line_data);

    line_data.tags_array = NULL;
    line_data.tags_count = 0;
    gui_line_tags_alloc (&line_data, "tag1,tag2,tag3");
    CHECK(line_data.tags_array);
    STRCMP_EQUAL("tag1", line_data.tags_array[0]);
    STRCMP_EQUAL("tag2", line_data.tags_array[1]);
    STRCMP_EQUAL("tag3", line_data.tags_array[2]);
    STRCMP_EQUAL(NULL, line_data.tags_array[3]);
    LONGS_EQUAL(3, line_data.tags_count);
    gui_line_tags_free (&line_data);

    gui_line_tags_free (NULL);
}

/*
 * Test functions:
 *   gui_line_prefix_is_same_nick
 */

TEST(GuiLine, PrefixIsSameNick)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_get_prefix_for_display
 */

TEST(GuiLine, GetPrefixForDisplay)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_get_align
 */

TEST(GuiLine, GetAlign)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_build_string_prefix_message
 */

TEST(GuiLine, BuildStringPrefixMessage)
{
    struct t_gui_line *line;
    char *str, str_prefix[256], str_message[256];

    WEE_BUILD_STR_PREFIX_MSG("\tmessage", NULL, "message");
    WEE_BUILD_STR_PREFIX_MSG("\tmessage", "", "message");
    WEE_BUILD_STR_PREFIX_MSG("prefix\tmessage", "prefix", "message");

    snprintf (str_prefix, sizeof (str_prefix),
              "%sblue prefix",
              gui_color_get_custom ("blue"));
    snprintf (str_message, sizeof (str_message),
              "%sred message",
              gui_color_get_custom ("red"));
    WEE_BUILD_STR_PREFIX_MSG("blue prefix\tred message", str_prefix, str_message);
}

/*
 * Test functions:
 *   gui_line_build_string_message_nick_offline
 */

TEST(GuiLine, BuildStringMessageNickOffline)
{
    char *str, str_msg_expected[256], str_msg[256];

    STRCMP_EQUAL(NULL, gui_line_build_string_message_nick_offline (NULL));
    WEE_TEST_STR("", gui_line_build_string_message_nick_offline (""));

    snprintf (str_msg_expected, sizeof (str_msg_expected),
              "%stest",
              GUI_COLOR(GUI_COLOR_CHAT_NICK_OFFLINE));
    WEE_TEST_STR(str_msg_expected, gui_line_build_string_message_nick_offline ("test"));

    snprintf (str_msg, sizeof (str_msg),
              "%stest",
              gui_color_get_custom ("blue"));
    WEE_TEST_STR(str_msg_expected, gui_line_build_string_message_nick_offline (str_msg));
}

/*
 * Test functions:
 *   gui_line_build_string_message_tags
 */

TEST(GuiLine, BuildStringMessageTags)
{
    struct t_gui_line *line;
    char *str, str_message[256], str_result[256];

    line = gui_line_new (gui_buffers, -1, 0, 0, 0, 0, "tag1,tag2", NULL, "test", -1);
    STRCMP_EQUAL(NULL,
                 gui_line_build_string_message_tags (line->data->message,
                                                     -1,
                                                     line->data->tags_array,
                                                     1));
    STRCMP_EQUAL(NULL,
                 gui_line_build_string_message_tags (line->data->message,
                                                     1,
                                                     NULL,
                                                     1));
    gui_line_free_data (line);
    free (line);

    snprintf (str_result, sizeof (str_result),
              "message%s [%s%s]",
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
              GUI_COLOR(GUI_COLOR_CHAT_TAGS),
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    WEE_BUILD_STR_MSG_TAGS(NULL, "message", 1);

    snprintf (str_result, sizeof (str_result),
              "message%s [%s%s]",
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
              GUI_COLOR(GUI_COLOR_CHAT_TAGS),
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    WEE_BUILD_STR_MSG_TAGS("", "message", 1);

    snprintf (str_result, sizeof (str_result),
              "message%s [%stag1%s]",
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
              GUI_COLOR(GUI_COLOR_CHAT_TAGS),
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    WEE_BUILD_STR_MSG_TAGS("tag1", "message", 1);

    snprintf (str_result, sizeof (str_result),
              "message%s [%stag1,tag2,tag3%s]",
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
              GUI_COLOR(GUI_COLOR_CHAT_TAGS),
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    WEE_BUILD_STR_MSG_TAGS("tag1,tag2,tag3", "message", 1);

    snprintf (str_message, sizeof (str_message),
              "message %sin red",
              gui_color_get_custom ("red"));
    snprintf (str_result, sizeof (str_result),
              "message %sin red%s [%stag1,tag2,tag3%s]",
              gui_color_get_custom ("red"),
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
              GUI_COLOR(GUI_COLOR_CHAT_TAGS),
              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
    WEE_BUILD_STR_MSG_TAGS("tag1,tag2,tag3", str_message, 1);

    snprintf (str_message, sizeof (str_message),
              "message %sin red",
              gui_color_get_custom ("red"));
    snprintf (str_result, sizeof (str_result),
              "message in red [tag1,tag2,tag3]");
    WEE_BUILD_STR_MSG_TAGS("tag1,tag2,tag3", str_message, 0);
}

/*
 * Test functions:
 *   gui_line_is_displayed
 */

TEST(GuiLine, IsDisplayed)
{
    struct t_gui_line line;
    int old_gui_filters_enabled;

    memset (&line, 0, sizeof (line));
    line.data = (struct t_gui_line_data *)calloc (1, sizeof (struct t_gui_line_data));

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
 * Test functions:
 *   gui_line_get_first_displayed
 */

TEST(GuiLine, GetFirstDisplayed)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_get_last_displayed
 */

TEST(GuiLine, GetLastDisplayed)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_get_prev_displayed
 */

TEST(GuiLine, GetPrevDisplayed)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_get_next_displayed
 */

TEST(GuiLine, GetNextDisplayed)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_search_by_id
 */

TEST(GuiLine, SearchById)
{
    POINTERS_EQUAL(NULL, gui_line_search_by_id (NULL, -1));
    POINTERS_EQUAL(NULL, gui_line_search_by_id (gui_buffers, -1));

    POINTERS_EQUAL(
        gui_buffers->own_lines->last_line,
        gui_line_search_by_id (gui_buffers,
                               gui_buffers->own_lines->last_line->data->id));
}

/*
 * Test functions:
 *   gui_line_search_text
 */

TEST(GuiLine, SearchText)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_match_regex
 */

TEST(GuiLine, MatchRegex)
{
    /* TODO: write tests */
}

/*
 * Test functions:
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
 * Test functions:
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
 * Test functions:
 *   gui_line_search_tag_starting_with
 */

TEST(GuiLine, SearchTagStartingWith)
{
    struct t_gui_line line;

    memset (&line, 0, sizeof (line));
    line.data = (struct t_gui_line_data *)calloc (1, sizeof (struct t_gui_line_data));
    gui_line_tags_alloc (line.data, NULL);

    STRCMP_EQUAL(NULL, gui_line_search_tag_starting_with (NULL, NULL));
    STRCMP_EQUAL(NULL, gui_line_search_tag_starting_with (NULL, "abc"));
    STRCMP_EQUAL(NULL, gui_line_search_tag_starting_with (&line, NULL));

    STRCMP_EQUAL(NULL, gui_line_search_tag_starting_with (&line, "abc"));

    gui_line_tags_alloc (line.data, "tag1,word2,last3");

    STRCMP_EQUAL(NULL, gui_line_search_tag_starting_with (&line, "abc"));
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
 * Test functions:
 *   gui_line_get_nick_tag
 */

TEST(GuiLine, GetNickTag)
{
    struct t_gui_line line;

    memset (&line, 0, sizeof (line));
    line.data = (struct t_gui_line_data *)calloc (1, sizeof (struct t_gui_line_data));
    gui_line_tags_alloc (line.data, NULL);

    STRCMP_EQUAL(NULL, gui_line_get_nick_tag (NULL));

    STRCMP_EQUAL(NULL, gui_line_get_nick_tag (&line));

    gui_line_tags_alloc (line.data, "tag1,tag2,tag3");
    STRCMP_EQUAL(NULL, gui_line_get_nick_tag (&line));
    gui_line_tags_free (line.data);

    gui_line_tags_alloc (line.data, "tag1,tag2,nick_jdoe,tag3");
    STRCMP_EQUAL("jdoe", gui_line_get_nick_tag (&line));
    gui_line_tags_free (line.data);

    free (line.data);
}

/*
 * Test functions:
 *   gui_line_has_highlight
 */

TEST(GuiLine, HasHighlight)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_has_offline_nick
 */

TEST(GuiLine, HasOfflineNick)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_is_action
 */

TEST(GuiLine, IsAction)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_compute_buffer_max_length
 */

TEST(GuiLine, ComputeBufferMaxLength)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_compute_prefix_max_length
 */

TEST(GuiLine, ComputePrefixMaxLength)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_add_to_list
 */

TEST(GuiLine, AddToList)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_free_data
 */

TEST(GuiLine, FreeData)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_remove_from_list
 */

TEST(GuiLine, RemoveFromList)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_mixed_add
 */

TEST(GuiLine, MixedAdd)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_mixed_free_buffer
 */

TEST(GuiLine, MixedFreeBuffer)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_mixed_free_all
 */

TEST(GuiLine, MixedFreeAll)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_free
 */

TEST(GuiLine, Free)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_free_all
 */

TEST(GuiLine, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_get_max_notify_level
 */

TEST(GuiLine, GetMaxNotifyLevel)
{
    struct t_gui_line line;

    memset (&line, 0, sizeof (line));
    line.data = (struct t_gui_line_data *)calloc (1, sizeof (struct t_gui_line_data));
    line.data->buffer = gui_buffers;
    gui_line_tags_alloc (line.data, NULL);

    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, gui_line_get_max_notify_level (&line));

    gui_line_tags_alloc (line.data, "nick_alice");
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, gui_line_get_max_notify_level (&line));
    gui_line_tags_free (line.data);

    gui_buffer_set_hotlist_max_level_nicks (gui_buffers, "alice:3");
    gui_line_tags_alloc (line.data, "nick_alice");
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, gui_line_get_max_notify_level (&line));
    gui_line_tags_free (line.data);

    gui_buffer_set_hotlist_max_level_nicks (gui_buffers, "alice:2");
    gui_line_tags_alloc (line.data, "nick_alice");
    LONGS_EQUAL(GUI_HOTLIST_PRIVATE, gui_line_get_max_notify_level (&line));
    gui_line_tags_free (line.data);

    gui_buffer_set_hotlist_max_level_nicks (gui_buffers, "alice:1");
    gui_line_tags_alloc (line.data, "nick_alice");
    LONGS_EQUAL(GUI_HOTLIST_MESSAGE, gui_line_get_max_notify_level (&line));
    gui_line_tags_free (line.data);

    gui_buffer_set_hotlist_max_level_nicks (gui_buffers, "alice:0");
    gui_line_tags_alloc (line.data, "nick_alice");
    LONGS_EQUAL(GUI_HOTLIST_LOW, gui_line_get_max_notify_level (&line));
    gui_line_tags_free (line.data);

    gui_buffer_set_hotlist_max_level_nicks (gui_buffers, "alice:-1");
    gui_line_tags_alloc (line.data, "nick_alice");
    LONGS_EQUAL(-1, gui_line_get_max_notify_level (&line));
    gui_line_tags_free (line.data);

    gui_buffer_set_hotlist_max_level_nicks (gui_buffers, NULL);

    free (line.data);
}

/*
 * Test functions:
 *   gui_line_set_notify_level
 */

TEST(GuiLine, SetNotifyLevel)
{
    struct t_gui_line line;

    memset (&line, 0, sizeof (line));
    line.data = (struct t_gui_line_data *)calloc (1, sizeof (struct t_gui_line_data));
    line.data->buffer = gui_buffers;
    gui_line_tags_alloc (line.data, NULL);

    line.data->notify_level = 99;
    gui_line_set_notify_level (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(GUI_HOTLIST_LOW, line.data->notify_level);

    /* notify: none */
    line.data->notify_level = 99;
    gui_line_tags_alloc (line.data, "notify_none");
    gui_line_set_notify_level (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(-1, line.data->notify_level);
    gui_line_tags_free (line.data);

    /* notify: message */
    line.data->notify_level = 99;
    gui_line_tags_alloc (line.data, "notify_message");
    gui_line_set_notify_level (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(GUI_HOTLIST_MESSAGE, line.data->notify_level);
    gui_line_tags_free (line.data);

    /* notify: private */
    line.data->notify_level = 99;
    gui_line_tags_alloc (line.data, "notify_private");
    gui_line_set_notify_level (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(GUI_HOTLIST_PRIVATE, line.data->notify_level);
    gui_line_tags_free (line.data);

    /* notify: highlight */
    line.data->notify_level = 99;
    gui_line_tags_alloc (line.data, "notify_highlight");
    gui_line_set_notify_level (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, line.data->notify_level);
    gui_line_tags_free (line.data);

    /* notify: highlight, max: private */
    line.data->notify_level = 99;
    gui_line_tags_alloc (line.data, "notify_highlight");
    gui_line_set_notify_level (&line, GUI_HOTLIST_PRIVATE);
    LONGS_EQUAL(GUI_HOTLIST_PRIVATE, line.data->notify_level);
    gui_line_tags_free (line.data);

    /* notify: highlight, max: message */
    line.data->notify_level = 99;
    gui_line_tags_alloc (line.data, "notify_highlight");
    gui_line_set_notify_level (&line, GUI_HOTLIST_MESSAGE);
    LONGS_EQUAL(GUI_HOTLIST_MESSAGE, line.data->notify_level);
    gui_line_tags_free (line.data);

    /* notify: highlight, max: low */
    line.data->notify_level = 99;
    gui_line_tags_alloc (line.data, "notify_highlight");
    gui_line_set_notify_level (&line, GUI_HOTLIST_LOW);
    LONGS_EQUAL(GUI_HOTLIST_LOW, line.data->notify_level);
    gui_line_tags_free (line.data);

    /* notify: highlight, max: -1 */
    line.data->notify_level = 99;
    gui_line_tags_alloc (line.data, "notify_highlight");
    gui_line_set_notify_level (&line, -1);
    LONGS_EQUAL(-1, line.data->notify_level);
    gui_line_tags_free (line.data);

    free (line.data);
}

/*
 * Test functions:
 *   gui_line_set_highlight
 */

TEST(GuiLine, SetHighlight)
{
    struct t_gui_line line;

    memset (&line, 0, sizeof (line));
    line.data = (struct t_gui_line_data *)calloc (1, sizeof (struct t_gui_line_data));
    line.data->buffer = gui_buffers;
    //line.data->message = strdup ("test");
    gui_line_tags_alloc (line.data, NULL);

    /* notify: none */
    line.data->notify_level = -1;
    line.data->highlight = -1;
    gui_line_set_highlight (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(0, line.data->highlight);

    /* notify: low */
    line.data->notify_level = 0;
    line.data->highlight = -1;
    gui_line_set_highlight (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(0, line.data->highlight);

    /* notify: message */
    line.data->notify_level = 1;
    line.data->highlight = -1;
    gui_line_set_highlight (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(0, line.data->highlight);

    /* notify: private */
    line.data->notify_level = 2;
    line.data->highlight = -1;
    gui_line_set_highlight (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(0, line.data->highlight);

    /* notify: highlight */
    line.data->notify_level = 3;
    line.data->highlight = -1;
    gui_line_set_highlight (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(1, line.data->highlight);

    /* notify: message, max: private */
    line.data->notify_level = 2;
    line.data->highlight = -1;
    gui_line_set_highlight (&line, GUI_HOTLIST_PRIVATE);
    LONGS_EQUAL(0, line.data->highlight);

    /* notify: message, max: message */
    line.data->notify_level = 1;
    line.data->highlight = -1;
    gui_line_set_highlight (&line, GUI_HOTLIST_MESSAGE);
    LONGS_EQUAL(0, line.data->highlight);

    /* notify: low, max: low */
    line.data->notify_level = 0;
    line.data->highlight = -1;
    gui_line_set_highlight (&line, GUI_HOTLIST_LOW);
    LONGS_EQUAL(0, line.data->highlight);

    /* notify: none, max: -1 */
    line.data->notify_level = -1;
    line.data->highlight = -1;
    gui_line_set_highlight (&line, -1);
    LONGS_EQUAL(0, line.data->highlight);

    config_file_option_set (config_look_highlight, "test", 1);

    /* notify: message, line with highlight */
    line.data->message = strdup ("this is a test");
    line.data->notify_level = 1;
    line.data->highlight = -1;
    gui_line_set_highlight (&line, GUI_HOTLIST_HIGHLIGHT);
    LONGS_EQUAL(1, line.data->highlight);
    free (line.data->message);
    line.data->message = NULL;

    config_file_option_reset (config_look_highlight, 1);

    free (line.data);
}

/*
 * Test functions:
 *   gui_line_new
 */

TEST(GuiLine, New)
{
    struct t_gui_buffer *buffer;
    struct t_gui_line *line1, *line2, *line3, *line4;
    struct timeval date_printed, date;
    char *str_time;

    gettimeofday (&date_printed, NULL);
    date.tv_sec = date_printed.tv_sec - 1;
    date.tv_usec = date_printed.tv_usec;
    str_time = gui_chat_get_time_string (date.tv_sec, date.tv_usec, 0);

    POINTERS_EQUAL(NULL,
                   gui_line_new (
                       NULL, 0,
                       date.tv_sec, date.tv_usec,
                       date_printed.tv_sec, date_printed.tv_usec,
                       NULL, NULL, NULL, -1));

    /* create a new test buffer (formatted content) */
    buffer = gui_buffer_new_user ("test", GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    line1 = gui_line_new (buffer,
                          0,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          NULL, NULL, NULL, -1);
    CHECK(line1);
    CHECK(line1->data);
    POINTERS_EQUAL(NULL, line1->prev_line);
    POINTERS_EQUAL(NULL, line1->next_line);
    POINTERS_EQUAL(buffer, line1->data->buffer);
    LONGS_EQUAL(0, line1->data->id);
    LONGS_EQUAL(-1, line1->data->y);
    LONGS_EQUAL(date.tv_sec, line1->data->date);
    LONGS_EQUAL(date.tv_usec, line1->data->date_usec);
    LONGS_EQUAL(date_printed.tv_sec, line1->data->date_printed);
    LONGS_EQUAL(date_printed.tv_usec, line1->data->date_usec_printed);
    STRCMP_EQUAL(str_time, line1->data->str_time);
    LONGS_EQUAL(0, line1->data->tags_count);
    POINTERS_EQUAL(NULL, line1->data->tags_array);
    LONGS_EQUAL(1, line1->data->displayed);
    LONGS_EQUAL(GUI_HOTLIST_LOW, line1->data->notify_level);
    LONGS_EQUAL(0, line1->data->highlight);
    LONGS_EQUAL(0, line1->data->refresh_needed);
    STRCMP_EQUAL("", line1->data->prefix);
    LONGS_EQUAL(0, line1->data->prefix_length);
    STRCMP_EQUAL("", line1->data->message);
    gui_line_add (line1, 1);
    POINTERS_EQUAL(NULL, line1->prev_line);
    POINTERS_EQUAL(NULL, line1->next_line);

    line2 = gui_line_new (buffer,
                          0,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "tag1,tag2,tag3",
                          "prefix", "message", -1);
    CHECK(line2);
    CHECK(line2->data);
    POINTERS_EQUAL(NULL, line2->prev_line);
    POINTERS_EQUAL(NULL, line2->next_line);
    POINTERS_EQUAL(buffer, line2->data->buffer);
    LONGS_EQUAL(1, line2->data->id);
    LONGS_EQUAL(-1, line2->data->y);
    LONGS_EQUAL(date.tv_sec, line2->data->date);
    LONGS_EQUAL(date.tv_usec, line2->data->date_usec);
    LONGS_EQUAL(date_printed.tv_sec, line2->data->date_printed);
    LONGS_EQUAL(date_printed.tv_usec, line2->data->date_usec_printed);
    STRCMP_EQUAL(str_time, line2->data->str_time);
    LONGS_EQUAL(3, line2->data->tags_count);
    CHECK(line2->data->tags_array);
    STRCMP_EQUAL("tag1", line2->data->tags_array[0]);
    STRCMP_EQUAL("tag2", line2->data->tags_array[1]);
    STRCMP_EQUAL("tag3", line2->data->tags_array[2]);
    LONGS_EQUAL(1, line2->data->displayed);
    LONGS_EQUAL(GUI_HOTLIST_LOW, line2->data->notify_level);
    LONGS_EQUAL(0, line2->data->highlight);
    LONGS_EQUAL(0, line2->data->refresh_needed);
    STRCMP_EQUAL("prefix", line2->data->prefix);
    LONGS_EQUAL(6, line2->data->prefix_length);
    STRCMP_EQUAL("message", line2->data->message);
    gui_line_add (line2, 1);
    POINTERS_EQUAL(line1, line2->prev_line);
    POINTERS_EQUAL(NULL, line2->next_line);

    /* simulate next_line_id == INT_MAX and display 2 lines */
    buffer->next_line_id = INT_MAX;
    line3 = gui_line_new (buffer,
                          0,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          NULL, NULL, "test", -1);
    CHECK(line3);
    LONGS_EQUAL(INT_MAX, line3->data->id);
    line4 = gui_line_new (buffer,
                          0,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          NULL, NULL, "test", -1);
    CHECK(line4);
    LONGS_EQUAL(0, line4->data->id);

    gui_buffer_close (buffer);

    /* create a new test buffer (free content) */
    buffer = gui_buffer_new_user ("test", GUI_BUFFER_TYPE_FREE);
    CHECK(buffer);

    line1 = gui_line_new (buffer,
                          0,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          NULL, NULL, NULL, -1);
    CHECK(line1);
    CHECK(line1->data);
    POINTERS_EQUAL(NULL, line1->prev_line);
    POINTERS_EQUAL(NULL, line1->next_line);
    POINTERS_EQUAL(buffer, line1->data->buffer);
    LONGS_EQUAL(0, line1->data->id);
    LONGS_EQUAL(0, line1->data->y);
    LONGS_EQUAL(date.tv_sec, line1->data->date);
    LONGS_EQUAL(date.tv_usec, line1->data->date_usec);
    LONGS_EQUAL(date_printed.tv_sec, line1->data->date_printed);
    LONGS_EQUAL(date_printed.tv_usec, line1->data->date_usec_printed);
    STRCMP_EQUAL(NULL, line1->data->str_time);
    LONGS_EQUAL(0, line1->data->tags_count);
    POINTERS_EQUAL(NULL, line1->data->tags_array);
    LONGS_EQUAL(1, line1->data->displayed);
    LONGS_EQUAL(GUI_HOTLIST_LOW, line1->data->notify_level);
    LONGS_EQUAL(0, line1->data->highlight);
    LONGS_EQUAL(1, line1->data->refresh_needed);
    STRCMP_EQUAL(NULL, line1->data->prefix);
    LONGS_EQUAL(0, line1->data->prefix_length);
    STRCMP_EQUAL("", line1->data->message);
    gui_line_add (line1, 1);
    POINTERS_EQUAL(NULL, line1->prev_line);
    POINTERS_EQUAL(NULL, line1->next_line);

    line2 = gui_line_new (buffer,
                          3,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "tag1,tag2,tag3",
                          NULL, "message", -1);
    CHECK(line2);
    CHECK(line2->data);
    POINTERS_EQUAL(NULL, line2->prev_line);
    POINTERS_EQUAL(NULL, line2->next_line);
    POINTERS_EQUAL(buffer, line2->data->buffer);
    LONGS_EQUAL(3, line2->data->id);
    LONGS_EQUAL(3, line2->data->y);
    LONGS_EQUAL(date.tv_sec, line2->data->date);
    LONGS_EQUAL(date.tv_usec, line2->data->date_usec);
    LONGS_EQUAL(date_printed.tv_sec, line2->data->date_printed);
    LONGS_EQUAL(date_printed.tv_usec, line2->data->date_usec_printed);
    STRCMP_EQUAL(NULL, line2->data->str_time);
    LONGS_EQUAL(3, line2->data->tags_count);
    CHECK(line2->data->tags_array);
    STRCMP_EQUAL("tag1", line2->data->tags_array[0]);
    STRCMP_EQUAL("tag2", line2->data->tags_array[1]);
    STRCMP_EQUAL("tag3", line2->data->tags_array[2]);
    LONGS_EQUAL(1, line2->data->displayed);
    LONGS_EQUAL(GUI_HOTLIST_LOW, line2->data->notify_level);
    LONGS_EQUAL(0, line2->data->highlight);
    LONGS_EQUAL(1, line2->data->refresh_needed);
    STRCMP_EQUAL(NULL, line2->data->prefix);
    LONGS_EQUAL(0, line2->data->prefix_length);
    STRCMP_EQUAL("message", line2->data->message);
    gui_line_add (line2, 1);
    CHECK(line2->prev_line);
    POINTERS_EQUAL(NULL, line2->next_line);

    gui_buffer_close (buffer);

    free (str_time);
}

/*
 * Test functions:
 *   gui_line_hook_update
 */

TEST(GuiLine, HookUpdate)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_add
 */

TEST(GuiLine, Add)
{
    struct t_gui_buffer *buffer;
    struct t_gui_line *line1, *line2, *line3;
    struct timeval date_printed, date;

    gettimeofday (&date_printed, NULL);
    date.tv_sec = date_printed.tv_sec;
    date.tv_usec = date_printed.tv_usec;

    /* make the hotlist "add conditions" always true for this test */
    config_file_option_set (config_look_hotlist_add_conditions, "1", 1);

    buffer = gui_buffer_new_user ("test_add", GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);
    POINTERS_EQUAL(NULL, gui_hotlist);

    /* the buffer has no line yet */
    POINTERS_EQUAL(NULL, buffer->own_lines->first_line);
    POINTERS_EQUAL(NULL, buffer->own_lines->last_line);
    LONGS_EQUAL(0, buffer->own_lines->lines_count);

    /*
     * add a first line with notify level "none" (not added to the hotlist):
     * it becomes both the first and the last line of the buffer
     */
    line1 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "notify_none", "prefix", "message1", 0);
    CHECK(line1);
    LONGS_EQUAL(-1, line1->data->notify_level);
    gui_line_add (line1, 1);
    POINTERS_EQUAL(line1, buffer->own_lines->first_line);
    POINTERS_EQUAL(line1, buffer->own_lines->last_line);
    LONGS_EQUAL(1, buffer->own_lines->lines_count);
    POINTERS_EQUAL(NULL, line1->prev_line);
    POINTERS_EQUAL(NULL, line1->next_line);
    POINTERS_EQUAL(NULL, gui_hotlist);

    /* add a second line: it is appended after the first one */
    line2 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "notify_none", "prefix", "message2", 0);
    CHECK(line2);
    gui_line_add (line2, 1);
    POINTERS_EQUAL(line1, buffer->own_lines->first_line);
    POINTERS_EQUAL(line2, buffer->own_lines->last_line);
    LONGS_EQUAL(2, buffer->own_lines->lines_count);
    POINTERS_EQUAL(line2, line1->next_line);
    POINTERS_EQUAL(line1, line2->prev_line);
    POINTERS_EQUAL(NULL, line2->next_line);
    POINTERS_EQUAL(NULL, gui_hotlist);

    /*
     * a line with notify level "message" is added to the hotlist with
     * priority "message" (add_to_hotlist == 1)
     */
    gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);
    line3 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "notify_message", "prefix", "message3", 0);
    CHECK(line3);
    LONGS_EQUAL(0, line3->data->highlight);
    LONGS_EQUAL(GUI_HOTLIST_MESSAGE, line3->data->notify_level);
    gui_line_add (line3, 1);
    CHECK(gui_hotlist);
    LONGS_EQUAL(GUI_HOTLIST_MESSAGE, gui_hotlist->priority);
    POINTERS_EQUAL(buffer, gui_hotlist->buffer);

    /*
     * the same "message" line added with add_to_hotlist == 0 must NOT be
     * added to the hotlist (non-highlighted counterpart of the highlight
     * check below)
     */
    gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);
    POINTERS_EQUAL(NULL, gui_hotlist);
    line3 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "notify_message", "prefix", "message4", 0);
    CHECK(line3);
    gui_line_add (line3, 0);
    POINTERS_EQUAL(NULL, gui_hotlist);

    /*
     * a line with notify level "private" is added to the hotlist with
     * priority "private"
     */
    gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);
    line3 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "notify_private", "prefix", "message5", 0);
    CHECK(line3);
    LONGS_EQUAL(GUI_HOTLIST_PRIVATE, line3->data->notify_level);
    gui_line_add (line3, 1);
    CHECK(gui_hotlist);
    LONGS_EQUAL(GUI_HOTLIST_PRIVATE, gui_hotlist->priority);
    POINTERS_EQUAL(buffer, gui_hotlist->buffer);

    /*
     * a highlighted line added normally (add_to_hotlist == 1) is added to the
     * hotlist with priority "highlight"
     */
    gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);
    line3 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "tag1,tag2", "prefix", "message6",
                          1);  /* known_highlight */
    CHECK(line3);
    LONGS_EQUAL(1, line3->data->highlight);
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, line3->data->notify_level);
    gui_line_add (line3, 1);
    CHECK(gui_hotlist);
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, gui_hotlist->priority);
    POINTERS_EQUAL(buffer, gui_hotlist->buffer);

    /*
     * a highlighted line replayed from an upgrade file (add_to_hotlist == 0)
     * must NOT be added to the hotlist (it is restored separately, see
     * upgrade_weechat_read_buffer_line())
     */
    gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);
    POINTERS_EQUAL(NULL, gui_hotlist);
    line3 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "tag1,tag2", "prefix", "message7",
                          1);  /* known_highlight */
    CHECK(line3);
    LONGS_EQUAL(1, line3->data->highlight);
    LONGS_EQUAL(GUI_HOTLIST_HIGHLIGHT, line3->data->notify_level);
    gui_line_add (line3, 0);  /* do not add to hotlist */
    POINTERS_EQUAL(NULL, gui_hotlist);

    /*
     * a line that is not displayed must NOT be added to the hotlist, even
     * when it is a highlight
     */
    gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);
    line3 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "notify_highlight", "prefix", "message8", 1);
    CHECK(line3);
    LONGS_EQUAL(1, line3->data->highlight);
    line3->data->displayed = 0;
    gui_line_add (line3, 1);
    POINTERS_EQUAL(NULL, gui_hotlist);

    gui_hotlist_clear (GUI_HOTLIST_MASK_MAX);
    config_file_option_reset (config_look_hotlist_add_conditions, 1);
    gui_buffer_close (buffer);

    /*
     * check that the oldest lines are removed when the max number of lines in
     * buffer is reached (option weechat.history.max_buffer_lines_number)
     */
    config_file_option_set (config_history_max_buffer_lines_number, "2", 1);
    buffer = gui_buffer_new_user ("test_add_max_lines",
                                  GUI_BUFFER_TYPE_FORMATTED);
    CHECK(buffer);

    line1 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "notify_none", NULL, "line1", 0);
    gui_line_add (line1, 1);
    LONGS_EQUAL(1, buffer->own_lines->lines_count);

    line2 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "notify_none", NULL, "line2", 0);
    gui_line_add (line2, 1);
    LONGS_EQUAL(2, buffer->own_lines->lines_count);

    /* adding a third line removes the oldest one (line1) */
    line3 = gui_line_new (buffer, -1,
                          date.tv_sec, date.tv_usec,
                          date_printed.tv_sec, date_printed.tv_usec,
                          "notify_none", NULL, "line3", 0);
    gui_line_add (line3, 1);
    LONGS_EQUAL(2, buffer->own_lines->lines_count);
    POINTERS_EQUAL(line2, buffer->own_lines->first_line);
    POINTERS_EQUAL(line3, buffer->own_lines->last_line);

    config_file_option_reset (config_history_max_buffer_lines_number, 1);
    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   gui_line_add_y
 */

TEST(GuiLine, AddY)
{
    struct t_gui_buffer *buffer;
    struct t_gui_line *ptr_line;

    buffer = gui_buffer_new_user ("test_add_y", GUI_BUFFER_TYPE_FREE);
    CHECK(buffer);

    /* a free-content buffer starts with no line */
    POINTERS_EQUAL(NULL, buffer->own_lines->first_line);
    POINTERS_EQUAL(NULL, buffer->own_lines->last_line);
    LONGS_EQUAL(0, buffer->own_lines->lines_count);

    /*
     * first line at y == 0: the buffer is empty (last_line == NULL), so the
     * fast path is not taken and the (empty) scan appends the line
     */
    WEE_LINE_ADD_Y(0, "line0");
    LONGS_EQUAL(1, buffer->own_lines->lines_count);
    ptr_line = buffer->own_lines->first_line;
    CHECK(ptr_line);
    POINTERS_EQUAL(ptr_line, buffer->own_lines->last_line);
    LONGS_EQUAL(0, ptr_line->data->y);
    STRCMP_EQUAL("line0", ptr_line->data->message);

    /*
     * append y == 2 leaving a gap: last_line->y (0) < 2, so the fast path
     * appends directly (gui_line_add_y does not fill gaps by itself)
     */
    WEE_LINE_ADD_Y(2, "line2");
    LONGS_EQUAL(2, buffer->own_lines->lines_count);
    LONGS_EQUAL(2, buffer->own_lines->last_line->data->y);
    STRCMP_EQUAL("line2", buffer->own_lines->last_line->data->message);

    /*
     * insert y == 1 in the middle: last_line->y (2) is not < 1, so the full
     * scan runs and inserts the line before y == 2
     */
    WEE_LINE_ADD_Y(1, "line1");
    LONGS_EQUAL(3, buffer->own_lines->lines_count);
    ptr_line = buffer->own_lines->first_line;
    LONGS_EQUAL(0, ptr_line->data->y);
    STRCMP_EQUAL("line0", ptr_line->data->message);
    ptr_line = ptr_line->next_line;
    LONGS_EQUAL(1, ptr_line->data->y);
    STRCMP_EQUAL("line1", ptr_line->data->message);
    ptr_line = ptr_line->next_line;
    LONGS_EQUAL(2, ptr_line->data->y);
    STRCMP_EQUAL("line2", ptr_line->data->message);
    POINTERS_EQUAL(NULL, ptr_line->next_line);
    POINTERS_EQUAL(ptr_line, buffer->own_lines->last_line);

    /*
     * replace a middle line (y == 1): the scan finds the existing line and
     * replaces its data, the line count is unchanged
     */
    WEE_LINE_ADD_Y(1, "line1-new");
    LONGS_EQUAL(3, buffer->own_lines->lines_count);
    ptr_line = buffer->own_lines->first_line->next_line;
    LONGS_EQUAL(1, ptr_line->data->y);
    STRCMP_EQUAL("line1-new", ptr_line->data->message);

    /*
     * replace the LAST line (y == 2): the fast path must NOT trigger here
     * (last_line->y (2) is not < 2); if it used "<=" the line would be
     * wrongly appended as a duplicate instead of replaced
     */
    WEE_LINE_ADD_Y(2, "line2-new");
    LONGS_EQUAL(3, buffer->own_lines->lines_count);
    LONGS_EQUAL(2, buffer->own_lines->last_line->data->y);
    STRCMP_EQUAL("line2-new", buffer->own_lines->last_line->data->message);

    /* append y == 10 far past the end: fast path (last_line->y (2) < 10) */
    WEE_LINE_ADD_Y(10, "line10");
    LONGS_EQUAL(4, buffer->own_lines->lines_count);
    LONGS_EQUAL(10, buffer->own_lines->last_line->data->y);
    STRCMP_EQUAL("line10", buffer->own_lines->last_line->data->message);

    /* final list must stay ordered by y: 0, 1, 2, 10 */
    ptr_line = buffer->own_lines->first_line;
    LONGS_EQUAL(0, ptr_line->data->y);
    ptr_line = ptr_line->next_line;
    LONGS_EQUAL(1, ptr_line->data->y);
    ptr_line = ptr_line->next_line;
    LONGS_EQUAL(2, ptr_line->data->y);
    ptr_line = ptr_line->next_line;
    LONGS_EQUAL(10, ptr_line->data->y);
    POINTERS_EQUAL(NULL, ptr_line->next_line);
    POINTERS_EQUAL(ptr_line, buffer->own_lines->last_line);

    gui_buffer_close (buffer);
}

/*
 * Test functions:
 *   gui_line_clear
 */

TEST(GuiLine, Clear)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_mix_buffers
 */

TEST(GuiLine, MixBuffers)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_hdata_lines_cb
 */

TEST(GuiLine, HdataLinesCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_hdata_line_cb
 */

TEST(GuiLine, HdataLineCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_hdata_line_data_update_cb
 */

TEST(GuiLine, HdataLineDataUpdateCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_hdata_line_data_cb
 */

TEST(GuiLine, HdataLineDataCb)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_line_add_to_infolist
 */

TEST(GuiLine, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Test functions:
 *   gui_lines_print_log
 */

TEST(GuiLine, HdataPrintLog)
{
    /* TODO: write tests */
}
