/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test layout functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/core/weechat.h"
#include "src/gui/gui-layout.h"
#include "src/gui/gui-window.h"
}

TEST_GROUP(GuiLayout)
{
};

/*
 * Test functions:
 *   gui_layout_window_store_on_exit
 */

TEST(GuiLayout, WindowStoreOnExit)
{
    struct t_gui_layout *layout, *ptr_layout_zoom;
    struct t_gui_window *window;

    gui_layout_window_store_on_exit (NULL);

    layout = gui_layout_alloc ("test");
    CHECK(layout);
    gui_layout_add (layout);

    /* No zoom: current windows are stored. */
    gui_layout_window_store_on_exit (layout);
    CHECK(layout->layout_windows);
    POINTERS_EQUAL(NULL, layout->layout_windows->child1);
    POINTERS_EQUAL(NULL, layout->layout_windows->child2);
    STRCMP_EQUAL("core", layout->layout_windows->plugin_name);
    STRCMP_EQUAL("weechat", layout->layout_windows->buffer_name);

    /* Zoom: windows stored before zoom are moved to the layout. */
    window = gui_window_split_horizontal (gui_current_window, 50);
    CHECK(window);
    gui_window_zoom (gui_current_window);
    POINTERS_EQUAL(NULL, gui_windows->next_window);
    ptr_layout_zoom = gui_layout_search (GUI_LAYOUT_ZOOM);
    CHECK(ptr_layout_zoom);
    gui_layout_window_store_on_exit (layout);
    POINTERS_EQUAL(NULL, gui_layout_search (GUI_LAYOUT_ZOOM));
    CHECK(layout->layout_windows);
    CHECK(layout->layout_windows->child1);
    CHECK(layout->layout_windows->child2);
    LONGS_EQUAL(50, layout->layout_windows->split_pct);
    LONGS_EQUAL(1, layout->layout_windows->split_horiz);

    /* Zoom while upgrading: current windows are stored, zoom is kept. */
    window = gui_window_split_horizontal (gui_current_window, 50);
    CHECK(window);
    gui_window_zoom (gui_current_window);
    ptr_layout_zoom = gui_layout_search (GUI_LAYOUT_ZOOM);
    CHECK(ptr_layout_zoom);
    weechat_upgrading = 1;
    gui_layout_window_store_on_exit (layout);
    weechat_upgrading = 0;
    POINTERS_EQUAL(ptr_layout_zoom, gui_layout_search (GUI_LAYOUT_ZOOM));
    CHECK(layout->layout_windows);
    POINTERS_EQUAL(NULL, layout->layout_windows->child1);
    POINTERS_EQUAL(NULL, layout->layout_windows->child2);

    /* Zoom layout used as layout: current windows are stored. */
    gui_layout_window_store_on_exit (ptr_layout_zoom);
    POINTERS_EQUAL(ptr_layout_zoom, gui_layout_search (GUI_LAYOUT_ZOOM));
    POINTERS_EQUAL(NULL, ptr_layout_zoom->layout_windows->child1);
    POINTERS_EQUAL(NULL, ptr_layout_zoom->layout_windows->child2);

    gui_layout_remove (ptr_layout_zoom);
    gui_layout_remove (layout);
    gui_window_merge_all (gui_current_window);
}
