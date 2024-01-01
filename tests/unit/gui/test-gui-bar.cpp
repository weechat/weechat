/*
 * test-gui-bar.cpp - test bar functions
 *
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include "src/gui/gui-bar.h"
}

TEST_GROUP(GuiBar)
{
};

/*
 * Tests functions:
 *   gui_bar_valid
 */

TEST(GuiBar, Valid)
{
    LONGS_EQUAL(0, gui_bar_valid (NULL));
    LONGS_EQUAL(0, gui_bar_valid ((struct t_gui_bar *)0x1));

    LONGS_EQUAL(1, gui_bar_valid (gui_bars));

    LONGS_EQUAL(0, gui_bar_valid (gui_bars + 1));
}

/*
 * Tests functions:
 *   gui_bar_search_default_bar
 */

TEST(GuiBar, SearchDefaultBar)
{
    int i;

    LONGS_EQUAL(-1, gui_bar_search_default_bar (NULL));
    LONGS_EQUAL(-1, gui_bar_search_default_bar (""));
    LONGS_EQUAL(-1, gui_bar_search_default_bar ("zzz"));

    for (i = 0; i < GUI_BAR_NUM_DEFAULT_BARS; i++)
    {
        LONGS_EQUAL(i, gui_bar_search_default_bar (gui_bar_default_name[i]));
    }
}

/*
 * Tests functions:
 *   gui_bar_search_option
 */

TEST(GuiBar, SearchOption)
{
    int i;

    LONGS_EQUAL(-1, gui_bar_search_option (NULL));
    LONGS_EQUAL(-1, gui_bar_search_option (""));
    LONGS_EQUAL(-1, gui_bar_search_option ("zzz"));

    for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
    {
        LONGS_EQUAL(i, gui_bar_search_option (gui_bar_option_string[i]));
    }
}

/*
 * Tests functions:
 *   gui_bar_search_type
 */

TEST(GuiBar, SearchType)
{
    int i;

    LONGS_EQUAL(-1, gui_bar_search_type (NULL));
    LONGS_EQUAL(-1, gui_bar_search_type (""));
    LONGS_EQUAL(-1, gui_bar_search_type ("zzz"));

    for (i = 0; i < GUI_BAR_NUM_TYPES; i++)
    {
        LONGS_EQUAL(i, gui_bar_search_type (gui_bar_type_string[i]));
    }
}

/*
 * Tests functions:
 *   gui_bar_search_position
 */

TEST(GuiBar, SearchPosition)
{
    int i;

    LONGS_EQUAL(-1, gui_bar_search_position (NULL));
    LONGS_EQUAL(-1, gui_bar_search_position (""));
    LONGS_EQUAL(-1, gui_bar_search_position ("zzz"));

    for (i = 0; i < GUI_BAR_NUM_POSITIONS; i++)
    {
        LONGS_EQUAL(i, gui_bar_search_position (gui_bar_position_string[i]));
    }
}

/*
 * Tests functions:
 *   gui_bar_check_size_add
 */

TEST(GuiBar, CheckSizeAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_get_filling
 */

TEST(GuiBar, GetFilling)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_find_pos
 */

TEST(GuiBar, FindPos)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_insert
 */

TEST(GuiBar, Insert)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_check_conditions
 */

TEST(GuiBar, CheckConditions)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_root_get_size
 */

TEST(GuiBar, RootGetSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_search
 */

TEST(GuiBar, Search)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_search_with_option_name
 */

TEST(GuiBar, SearchWithOptionName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_content_build_bar_windows
 */

TEST(GuiBar, ContentBuildBarWindows)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_ask_refresh
 */

TEST(GuiBar, AskRefresh)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_refresh
 */

TEST(GuiBar, Refresh)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_draw
 */

TEST(GuiBar, Draw)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_apply_current_size
 */

TEST(GuiBar, ApplyCurrentSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_free_items_arrays
 */

TEST(GuiBar, FreeItemsArrays)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_set_items_array
 */

TEST(GuiBar, SetItemsArray)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_check_type
 */

TEST(GuiBar, ConfigCheckType)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_change_hidden
 */

TEST(GuiBar, ConfigChangeHidden)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_change_priority
 */

TEST(GuiBar, ConfigChangePriority)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_change_conditions
 */

TEST(GuiBar, ConfigChangeConditions)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_change_position
 */

TEST(GuiBar, ConfigChangePosition)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_change_filling
 */

TEST(GuiBar, ConfigChangeFilling)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_check_size
 */

TEST(GuiBar, ConfigCheckSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_change_size
 */

TEST(GuiBar, ConfigChangeSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_change_size_max
 */

TEST(GuiBar, ConfigChangeSizeMax)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_change_color
 */

TEST(GuiBar, ConfigChangeColor)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_change_separator
 */

TEST(GuiBar, ConfigChangeSeparator)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_config_change_items
 */

TEST(GuiBar, ConfigChangeItems)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_set_name
 */

TEST(GuiBar, SetName)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_set
 */

TEST(GuiBar, BarSet)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_create_option
 */

TEST(GuiBar, BarCreateOption)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_create_option_temp
 */

TEST(GuiBar, CreateOptionTemp)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_alloc
 */

TEST(GuiBar, Alloc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_set_default_value
 */

TEST(GuiBar, SetDefaultValue)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_new_with_options
 */

TEST(GuiBar, NewWithOptions)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_new
 */

TEST(GuiBar, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_new_default
 */

TEST(GuiBar, NewDefault)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_use_temp_bars
 */

TEST(GuiBar, UseTempBars)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_create_default_input
 */

TEST(GuiBar, CreateDefaultInput)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_create_default_title
 */

TEST(GuiBar, CreateDefaultTitle)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_create_default_status
 */

TEST(GuiBar, CreateDefaultStatus)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_create_default_nicklist
 */

TEST(GuiBar, CreateDefaultNicklist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_create_default
 */

TEST(GuiBar, CreateDefault)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_update
 */

TEST(GuiBar, Update)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_scroll
 */

TEST(GuiBar, Scroll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_free
 */

TEST(GuiBar, Free)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_free_all
 */

TEST(GuiBar, FreeAll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_free_bar_windows
 */

TEST(GuiBar, FreeBarWindows)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_hdata_bar_cb
 */

TEST(GuiBar, HdataBarCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_add_to_infolist
 */

TEST(GuiBar, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_print_log
 */

TEST(GuiBar, PrintLog)
{
    /* TODO: write tests */
}
