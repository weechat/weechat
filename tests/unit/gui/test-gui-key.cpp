/*
 * test-gui-key.cpp - test key functions
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

extern "C"
{
#include "src/gui/gui-key.h"

extern int gui_key_cmp (const char *key, const char *search, int context);
}

TEST_GROUP(GuiKey)
{
};

/*
 * Tests functions:
 *   gui_key_init
 */

TEST(GuiKey, Init)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_search_context
 */

TEST(GuiKey, SearchContext)
{
    LONGS_EQUAL(-1, gui_key_search_context (NULL));
    LONGS_EQUAL(-1, gui_key_search_context (""));
    LONGS_EQUAL(-1, gui_key_search_context ("invalid"));

    LONGS_EQUAL(0, gui_key_search_context ("default"));
    LONGS_EQUAL(1, gui_key_search_context ("search"));
    LONGS_EQUAL(2, gui_key_search_context ("cursor"));
    LONGS_EQUAL(3, gui_key_search_context ("mouse"));
}

/*
 * Tests functions:
 *   gui_key_get_current_context
 */

TEST(GuiKey, GetCurrentContext)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_grab_init
 */

TEST(GuiKey, GrabInit)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_grab_end_timer_cb
 */

TEST(GuiKey, GrabEndTimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_get_internal_code
 */

TEST(GuiKey, GetInternalCode)
{
    char *str;

    WEE_TEST_STR(NULL, gui_key_get_internal_code (NULL));
    WEE_TEST_STR("", gui_key_get_internal_code (""));
    WEE_TEST_STR("A", gui_key_get_internal_code ("A"));
    WEE_TEST_STR("a", gui_key_get_internal_code ("a"));

    WEE_TEST_STR("@chat:t", gui_key_get_internal_code ("@chat:t"));

    WEE_TEST_STR("\x01[A", gui_key_get_internal_code ("meta-A"));
    WEE_TEST_STR("\x01[a", gui_key_get_internal_code ("meta-a"));

    WEE_TEST_STR("\x01[[A", gui_key_get_internal_code ("meta2-A"));
    WEE_TEST_STR("\x01[[a", gui_key_get_internal_code ("meta2-a"));

    /* ctrl-letter keys are forced to lower case */
    WEE_TEST_STR("\x01" "a", gui_key_get_internal_code ("ctrl-A"));
    WEE_TEST_STR("\x01" "a", gui_key_get_internal_code ("ctrl-a"));

    WEE_TEST_STR(" ", gui_key_get_internal_code ("space"));
}

/*
 * Tests functions:
 *   gui_key_get_expanded_name
 */

TEST(GuiKey, GetExpandedName)
{
    char *str;

    WEE_TEST_STR(NULL, gui_key_get_expanded_name (NULL));
    WEE_TEST_STR("", gui_key_get_expanded_name (""));
    WEE_TEST_STR("A", gui_key_get_expanded_name ("A"));
    WEE_TEST_STR("a", gui_key_get_expanded_name ("a"));

    WEE_TEST_STR("@chat:t", gui_key_get_expanded_name ("@chat:t"));

    WEE_TEST_STR("meta-A", gui_key_get_expanded_name ("\x01[A"));
    WEE_TEST_STR("meta-a", gui_key_get_expanded_name ("\x01[a"));

    WEE_TEST_STR("meta2-A", gui_key_get_expanded_name ("\x01[[A"));
    WEE_TEST_STR("meta2-a", gui_key_get_expanded_name ("\x01[[a"));

    WEE_TEST_STR("ctrl-A", gui_key_get_expanded_name ("\x01" "A"));
    WEE_TEST_STR("ctrl-a", gui_key_get_expanded_name ("\x01" "a"));

    WEE_TEST_STR("space", gui_key_get_expanded_name (" "));
}

/*
 * Tests functions:
 *   gui_key_find_pos
 */

TEST(GuiKey, FindPos)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_insert_sorted
 */

TEST(GuiKey, InsertSorted)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_set_area_type_name
 */

TEST(GuiKey, SetAreaTypeName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_set_areas
 */

TEST(GuiKey, SetAreas)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_set_score
 */

TEST(GuiKey, SetScore)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_is_safe
 */

TEST(GuiKey, IsSafe)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_new
 */

TEST(GuiKey, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_search
 */

TEST(GuiKey, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_cmp
 */

TEST(GuiKey, Cmp)
{
    LONGS_EQUAL(0, gui_key_cmp ("", "", GUI_KEY_CONTEXT_DEFAULT));
    LONGS_EQUAL(0, gui_key_cmp ("a", "", GUI_KEY_CONTEXT_DEFAULT));
    LONGS_EQUAL(-97, gui_key_cmp ("", "a", GUI_KEY_CONTEXT_DEFAULT));
    LONGS_EQUAL(0, gui_key_cmp ("a", "a", GUI_KEY_CONTEXT_DEFAULT));
    LONGS_EQUAL(32, gui_key_cmp ("meta-a", "meta-A", GUI_KEY_CONTEXT_DEFAULT));
    LONGS_EQUAL(-99, gui_key_cmp ("meta-a", "meta-ac", GUI_KEY_CONTEXT_DEFAULT));
    LONGS_EQUAL(0, gui_key_cmp ("meta-ac", "meta-a", GUI_KEY_CONTEXT_DEFAULT));

    LONGS_EQUAL(0, gui_key_cmp ("", "", GUI_KEY_CONTEXT_SEARCH));
    LONGS_EQUAL(0, gui_key_cmp ("a", "", GUI_KEY_CONTEXT_SEARCH));
    LONGS_EQUAL(-97, gui_key_cmp ("", "a", GUI_KEY_CONTEXT_SEARCH));
    LONGS_EQUAL(0, gui_key_cmp ("a", "a", GUI_KEY_CONTEXT_SEARCH));
    LONGS_EQUAL(32, gui_key_cmp ("meta-a", "meta-A", GUI_KEY_CONTEXT_SEARCH));
    LONGS_EQUAL(-99, gui_key_cmp ("meta-a", "meta-ac", GUI_KEY_CONTEXT_SEARCH));
    LONGS_EQUAL(0, gui_key_cmp ("meta-ac", "meta-a", GUI_KEY_CONTEXT_SEARCH));

    LONGS_EQUAL(0, gui_key_cmp ("", "", GUI_KEY_CONTEXT_CURSOR));
    LONGS_EQUAL(0, gui_key_cmp ("a", "", GUI_KEY_CONTEXT_CURSOR));
    LONGS_EQUAL(-97, gui_key_cmp ("", "a", GUI_KEY_CONTEXT_CURSOR));
    LONGS_EQUAL(0, gui_key_cmp ("a", "a", GUI_KEY_CONTEXT_CURSOR));
    LONGS_EQUAL(32, gui_key_cmp ("meta-a", "meta-A", GUI_KEY_CONTEXT_CURSOR));
    LONGS_EQUAL(-99, gui_key_cmp ("meta-a", "meta-ac", GUI_KEY_CONTEXT_CURSOR));
    LONGS_EQUAL(0, gui_key_cmp ("meta-ac", "meta-a", GUI_KEY_CONTEXT_CURSOR));

    LONGS_EQUAL(1, gui_key_cmp ("", "", GUI_KEY_CONTEXT_MOUSE));
    LONGS_EQUAL(1, gui_key_cmp ("a", "", GUI_KEY_CONTEXT_MOUSE));
    LONGS_EQUAL(1, gui_key_cmp ("", "a", GUI_KEY_CONTEXT_MOUSE));
    LONGS_EQUAL(0, gui_key_cmp ("a", "a", GUI_KEY_CONTEXT_MOUSE));
    LONGS_EQUAL(0, gui_key_cmp ("@chat(fset.fset):button2",
                                "@chat(fset.fset):button2",
                                GUI_KEY_CONTEXT_MOUSE));
    LONGS_EQUAL(0, gui_key_cmp ("@chat(fset.fset):button2-gesture-right",
                                "@chat(fset.fset):button2*",
                                GUI_KEY_CONTEXT_MOUSE));
    LONGS_EQUAL(1, gui_key_cmp ("@chat(Fset.fset):button2-gesture-right",
                                "@chat(fset.fset):button2*",
                                GUI_KEY_CONTEXT_MOUSE));
}

/*
 * Tests functions:
 *   gui_key_search_part
 */

TEST(GuiKey, SearchPart)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_bind
 */

TEST(GuiKey, Bind)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_bind_plugin_hashtable_map_cb
 */

TEST(GuiKey, BindPluginHashtableMapCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_bind_plugin
 */

TEST(GuiKey, BindPlugin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_unbind
 */

TEST(GuiKey, Unbind)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_unbind_plugin
 */

TEST(GuiKey, UnbindPlugin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_focus_matching
 */

TEST(GuiKey, FocusMatching)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_focus_command
 */

TEST(GuiKey, FocusCommand)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_focus
 */

TEST(GuiKey, Focus)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_is_complete
 */

TEST(GuiKey, IsComplete)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_pressed
 */

TEST(GuiKey, Pressed)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_free
 */

TEST(GuiKey, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_free_all
 */

TEST(GuiKey, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_buffer_optimize
 */

TEST(GuiKey, BufferOptimize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_buffer_reset
 */

TEST(GuiKey, BufferReset)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_buffer_add
 */

TEST(GuiKey, BufferAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_buffer_search
 */

TEST(GuiKey, BufferSearch)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_buffer_remove
 */

TEST(GuiKey, BufferRemove)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_remove_newline
 */

TEST(GuiKey, PasteRemoveNewline)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_replace_tabs
 */

TEST(GuiKey, PasteReplaceTabs)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_start
 */

TEST(GuiKey, PasteStart)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_get_paste_lines
 */

TEST(GuiKey, GetPasteLines)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_check
 */

TEST(GuiKey, PasteCheck)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_bracketed_timer_cb
 */

TEST(GuiKey, PasteBracketedTimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_bracketed_timer_remove
 */

TEST(GuiKey, PasteBracketedTimerRemove)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_bracketed_timer_add
 */

TEST(GuiKey, PasteBracketedTimerAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_bracketed_start
 */

TEST(GuiKey, PasteBracketedStart)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_bracketed_stop
 */

TEST(GuiKey, PasteBracketedStop)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_accept
 */

TEST(GuiKey, PasteAccept)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_paste_cancel
 */

TEST(GuiKey, PasteCancel)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_end
 */

TEST(GuiKey, End)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_hdata_key_cb
 */

TEST(GuiKey, HdataKeyCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_add_to_infolist
 */

TEST(GuiKey, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_print_log_key
 */

TEST(GuiKey, PrintLogKey)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_key_print_log
 */

TEST(GuiKey, PrintLog)
{
    /* TODO: write tests */
}
