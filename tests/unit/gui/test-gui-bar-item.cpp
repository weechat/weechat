/*
 * test-gui-bar-item.cpp - test bar item functions
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
#include "src/gui/gui-bar.h"
#include "src/gui/gui-bar-item.h"

extern char *gui_bar_item_buffer_name_cb (const void *pointer, void *data,
                                          struct t_gui_bar_item *item,
                                          struct t_gui_window *window,
                                          struct t_gui_buffer *buffer,
                                          struct t_hashtable *extra_info);
}

TEST_GROUP(GuiBarItem)
{
};

/*
 * Tests functions:
 *   gui_bar_item_valid
 */

TEST(GuiBarItem, Valid)
{
    LONGS_EQUAL(0, gui_bar_item_valid (NULL));
    LONGS_EQUAL(0, gui_bar_item_valid ((struct t_gui_bar_item *)0x1));

    LONGS_EQUAL(1, gui_bar_item_valid (gui_bar_items));

    LONGS_EQUAL(0, gui_bar_item_valid (gui_bar_items + 1));
}

/*
 * Tests functions:
 *   gui_bar_item_search_default
 */

TEST(GuiBarItem, SearchDefault)
{
    LONGS_EQUAL(-1, gui_bar_item_search_default (NULL));
    LONGS_EQUAL(-1, gui_bar_item_search_default (""));
    LONGS_EQUAL(-1, gui_bar_item_search_default ("zzz"));

    CHECK(gui_bar_item_search_default ("scroll") >= 0);
    CHECK(gui_bar_item_search_default ("time") >= 0);
}

/*
 * Tests functions:
 *   gui_bar_item_search
 */

TEST(GuiBarItem, Search)
{
    struct t_gui_bar_item *ptr_item;

    POINTERS_EQUAL(NULL, gui_bar_item_search (NULL));
    POINTERS_EQUAL(NULL, gui_bar_item_search (""));
    POINTERS_EQUAL(NULL, gui_bar_item_search ("zzz"));

    ptr_item = gui_bar_item_search ("buffer_name");
    CHECK(ptr_item);
    POINTERS_EQUAL(NULL, ptr_item->plugin);
    STRCMP_EQUAL("buffer_name", ptr_item->name);
    POINTERS_EQUAL(&gui_bar_item_buffer_name_cb, ptr_item->build_callback);
    POINTERS_EQUAL(NULL, ptr_item->build_callback_pointer);
    POINTERS_EQUAL(NULL, ptr_item->build_callback_data);
}

/*
 * Tests functions:
 *   gui_bar_item_search_with_plugin
 */

TEST(GuiBarItem, SearchWithPlugin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_used_in_bar
 */

TEST(GuiBarItem, UsedInBar)
{
    struct t_gui_bar *ptr_bar;

    ptr_bar = gui_bar_search ("status");
    CHECK(ptr_bar);

    LONGS_EQUAL(0, gui_bar_item_used_in_bar (NULL, NULL, 0));
    LONGS_EQUAL(0, gui_bar_item_used_in_bar (NULL, "", 0));
    LONGS_EQUAL(0, gui_bar_item_used_in_bar (ptr_bar, "zzz", 0));
    LONGS_EQUAL(0, gui_bar_item_used_in_bar (ptr_bar, "buffer_", 0));

    LONGS_EQUAL(1, gui_bar_item_used_in_bar (ptr_bar, "buffer_name", 0));
    LONGS_EQUAL(1, gui_bar_item_used_in_bar (ptr_bar, "buffer_", 1));
}

/*
 * Tests functions:
 *   gui_bar_item_used_in_at_least_one_bar
 */

TEST(GuiBarItem, UsedInAtLeastOneBar)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_get_vars
 */

TEST(GuiBarItem, GetVars)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_get_value
 */

TEST(GuiBarItem, GetValue)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_count_lines
 */

TEST(GuiBarItem, CountLines)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_new
 */

TEST(GuiBarItem, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_update
 */

TEST(GuiBarItem, Update)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_free
 */

TEST(GuiBarItem, Free)
{
    /* test free of NULL bar item */
    gui_bar_item_free (NULL);
}

/*
 * Tests functions:
 *   gui_bar_item_free_all
 */

TEST(GuiBarItem, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_free_all_plugin
 */

TEST(GuiBarItem, FreeAllPlugin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_input_paste_cb
 */

TEST(GuiBarItem, InputPasteCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_input_prompt_cb
 */

TEST(GuiBarItem, InputPromptCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_input_search_cb
 */

TEST(GuiBarItem, InputSearchCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_input_text_cb
 */

TEST(GuiBarItem, InputTextCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_time_cb
 */

TEST(GuiBarItem, TimeCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_count_cb
 */

TEST(GuiBarItem, BufferCountCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_last_number_cb
 */

TEST(GuiBarItem, BufferLastNumberCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_plugin_cb
 */

TEST(GuiBarItem, BufferPluginCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_number_cb
 */

TEST(GuiBarItem, BufferNumberCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_name_cb
 */

TEST(GuiBarItem, BufferNameCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_short_name_cb
 */

TEST(GuiBarItem, BufferShortNameCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_modes_cb
 */

TEST(GuiBarItem, BufferModesCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_filter_cb
 */

TEST(GuiBarItem, BufferFilterCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_nicklist_count_cb
 */

TEST(GuiBarItem, BufferNicklistCountCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_nicklist_count_groups_cb
 */

TEST(GuiBarItem, BufferNicklistCountGroupsCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_nicklist_count_all_cb
 */

TEST(GuiBarItem, BufferNicklistCountAllCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_zoom_cb
 */

TEST(GuiBarItem, BufferZoomCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_scroll_cb
 */

TEST(GuiBarItem, ScrollCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_hotlist_cb
 */

TEST(GuiBarItem, HotlistCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_completion_cb
 */

TEST(GuiBarItem, CompletionCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_title_cb
 */

TEST(GuiBarItem, BufferTitleCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_buffer_nicklist_cb
 */

TEST(GuiBarItem, BufferNicklistCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_window_number_cb
 */

TEST(GuiBarItem, WindowNumberCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_mouse_status_cb
 */

TEST(GuiBarItem, MouseStatusCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_away_cb
 */

TEST(GuiBarItem, AwayCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_spacer_cb
 */

TEST(GuiBarItem, SpacerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_focus_buffer_nicklist_cb
 */

TEST(GuiBarItem, FocusBufferNicklistCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_timer_cb
 */

TEST(GuiBarItem, TimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_signal_cb
 */

TEST(GuiBarItem, SignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_hook_signal
 */

TEST(GuiBarItem, HookSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_init
 */

TEST(GuiBarItem, Init)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_end
 */

TEST(GuiBarItem, End)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_hdata_bar_item_cb
 */

TEST(GuiBarItem, HdataBarItemCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_add_to_infolist
 */

TEST(GuiBarItem, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_item_print_log
 */

TEST(GuiBarItem, PrintLog)
{
    /* TODO: write tests */
}
