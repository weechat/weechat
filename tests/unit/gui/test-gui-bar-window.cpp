/*
 * test-gui-bar-window.cpp - test bar window functions
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
#include "src/gui/gui-bar-window.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-window.h"

extern int gui_bar_window_item_is_spacer (const char *item);
}

TEST_GROUP(GuiBarWindow)
{
};

/*
 * Tests functions:
 *   gui_bar_window_valid
 */

TEST(GuiBarWindow, Valid)
{
    LONGS_EQUAL(0, gui_bar_window_valid (NULL));
    LONGS_EQUAL(0, gui_bar_window_valid ((struct t_gui_bar_window *)0x1));

    LONGS_EQUAL(1, gui_bar_window_valid (gui_windows->bar_windows));

    LONGS_EQUAL(0, gui_bar_window_valid (gui_windows->bar_windows + 1));
}

/*
 * Tests functions:
 *   gui_bar_window_search_bar
 */

TEST(GuiBarWindow, SearchBar)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_search_by_xy
 */

TEST(GuiBarWindow, SearchByXy)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_get_size
 */

TEST(GuiBarWindow, GetSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_calculate_pos_size
 */

TEST(GuiBarWindow, CalculatePosSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_find_pos
 */

TEST(GuiBarWindow, FindPos)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_content_alloc
 */

TEST(GuiBarWindow, ContentAlloc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_content_free
 */

TEST(GuiBarWindow, ContentFree)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_content_build_item
 */

TEST(GuiBarWindow, ContentBuildItem)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_content_build
 */

TEST(GuiBarWindow, ContentBuild)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_content_get
 */

TEST(GuiBarWindow, ContentGet)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_item_is_spacer
 */

TEST(GuiBarWindow, ItemIsSpacer)
{
    char str_spacer[16];

    LONGS_EQUAL(0, gui_bar_window_item_is_spacer (NULL));
    LONGS_EQUAL(0, gui_bar_window_item_is_spacer (""));

    snprintf (str_spacer, sizeof (str_spacer),
              "%c",
              GUI_COLOR_COLOR_CHAR);
    LONGS_EQUAL(0, gui_bar_window_item_is_spacer (str_spacer));

    snprintf (str_spacer, sizeof (str_spacer),
              "%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR);
    LONGS_EQUAL(0, gui_bar_window_item_is_spacer (str_spacer));

    snprintf (str_spacer, sizeof (str_spacer),
              "%c%c%c",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_SPACER);
    LONGS_EQUAL(1, gui_bar_window_item_is_spacer (str_spacer));

    snprintf (str_spacer, sizeof (str_spacer),
              "%c%c%c ",
              GUI_COLOR_COLOR_CHAR,
              GUI_COLOR_BAR_CHAR,
              GUI_COLOR_BAR_SPACER);
    LONGS_EQUAL(0, gui_bar_window_item_is_spacer (str_spacer));
}

/*
 * Tests functions:
 *   gui_bar_window_content_get_with_filling
 */

TEST(GuiBarWindow, ContentGetWithFilling)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_can_use_spacer
 */

TEST(GuiBarWindow, CanUseSpacer)
{
    struct t_gui_bar *bar;
    struct t_gui_bar_window *bar_window;

    bar = gui_bar_search ("title");
    CHECK(bar);
    bar_window = gui_bar_window_search_bar (gui_windows, bar);
    CHECK(bar_window);
    LONGS_EQUAL(1, gui_bar_window_can_use_spacer (bar_window));

    bar = gui_bar_search ("status");
    CHECK(bar);
    bar_window = gui_bar_window_search_bar (gui_windows, bar);
    CHECK(bar_window);
    LONGS_EQUAL(1, gui_bar_window_can_use_spacer (bar_window));

    bar = gui_bar_search ("input");
    CHECK(bar);
    bar_window = gui_bar_window_search_bar (gui_windows, bar);
    CHECK(bar_window);
    LONGS_EQUAL(0, gui_bar_window_can_use_spacer (bar_window));
}

/*
 * Tests functions:
 *   gui_bar_window_compute_spacers_size
 */

TEST(GuiBarWindow, ComputeSpacersSize)
{
    int *spacers;

    POINTERS_EQUAL(NULL, gui_bar_window_compute_spacers_size (-1, 0, 0));
    POINTERS_EQUAL(NULL, gui_bar_window_compute_spacers_size (10, 0, 0));
    POINTERS_EQUAL(NULL, gui_bar_window_compute_spacers_size (10, 20, 0));
    POINTERS_EQUAL(NULL, gui_bar_window_compute_spacers_size (10, 20, 0));

    /* length on screen == bar window width */
    POINTERS_EQUAL(NULL, gui_bar_window_compute_spacers_size (20, 20, 3));

    /* length on screen > bar window width */
    POINTERS_EQUAL(NULL, gui_bar_window_compute_spacers_size (25, 20, 3));

    /* single spacer */
    spacers = gui_bar_window_compute_spacers_size (10, 20, 1);
    CHECK(spacers);
    LONGS_EQUAL(10, spacers[0]);
    free (spacers);

    /* 2 spacers */
    spacers = gui_bar_window_compute_spacers_size (10, 20, 2);
    CHECK(spacers);
    LONGS_EQUAL(5, spacers[0]);
    LONGS_EQUAL(5, spacers[1]);
    free (spacers);

    /* 3 spacers */
    spacers = gui_bar_window_compute_spacers_size (10, 20, 3);
    CHECK(spacers);
    LONGS_EQUAL(4, spacers[0]);
    LONGS_EQUAL(3, spacers[1]);
    LONGS_EQUAL(3, spacers[2]);
    free (spacers);

    /* 4 spacers */
    spacers = gui_bar_window_compute_spacers_size (10, 20, 4);
    CHECK(spacers);
    LONGS_EQUAL(3, spacers[0]);
    LONGS_EQUAL(3, spacers[1]);
    LONGS_EQUAL(2, spacers[2]);
    LONGS_EQUAL(2, spacers[3]);
    free (spacers);

    /* 12 spacers */
    spacers = gui_bar_window_compute_spacers_size (10, 20, 12);
    CHECK(spacers);
    LONGS_EQUAL(1, spacers[0]);
    LONGS_EQUAL(1, spacers[1]);
    LONGS_EQUAL(1, spacers[2]);
    LONGS_EQUAL(1, spacers[3]);
    LONGS_EQUAL(1, spacers[4]);
    LONGS_EQUAL(1, spacers[5]);
    LONGS_EQUAL(1, spacers[6]);
    LONGS_EQUAL(1, spacers[7]);
    LONGS_EQUAL(1, spacers[8]);
    LONGS_EQUAL(1, spacers[9]);
    LONGS_EQUAL(0, spacers[10]);
    LONGS_EQUAL(0, spacers[11]);
    free (spacers);
}

/*
 * Tests functions:
 *   gui_bar_window_coords_add
 */

TEST(GuiBarWindow, CoordsAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_coords_free
 */

TEST(GuiBarWindow, CoordsFree)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_insert
 */

TEST(GuiBarWindow, Insert)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_new
 */

TEST(GuiBarWindow, New)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_get_current_size
 */

TEST(GuiBarWindow, GetCurrentSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_get_max_size_in_window
 */

TEST(GuiBarWindow, GetMaxSizeInWindow)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_get_max_size
 */

TEST(GuiBarWindow, GetMaxSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_set_current_size
 */

TEST(GuiBarWindow, SetCurrentSize)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_free
 */

TEST(GuiBarWindow, Free)
{
    /* test free of NULL bar window */
    gui_bar_window_free (NULL, gui_current_window);
    gui_bar_window_free (NULL, NULL);
}

/*
 * Tests functions:
 *   gui_bar_window_remove_unused_bars
 */

TEST(GuiBarWindow, RemoveUnusedBars)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_add_missing_bars
 */

TEST(GuiBarWindow, AddMissingBars)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_scroll
 */

TEST(GuiBarWindow, Scroll)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_update_cb
 */

TEST(GuiBarWindow, UpdateCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_hdata_bar_window_cb
 */

TEST(GuiBarWindow, HdataBarWindowCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_add_to_infolist
 */

TEST(GuiBarWindow, AddToInfolist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   gui_bar_window_print_log
 */

TEST(GuiBarWindow, PrintLog)
{
    /* TODO: write tests */
}
