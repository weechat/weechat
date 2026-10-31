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
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-layout.h"
#include "src/gui/gui-window.h"

extern void gui_buffer_visited_remove_all ();
}

TEST_GROUP(GuiLayout)
{
};

/*
 * Return the names of visited buffers, separated by commas.
 */

const char *
test_gui_layout_visited_buffers (void)
{
    static char names[1024];
    struct t_gui_buffer_visited *ptr_visited;

    names[0] = '\0';
    for (ptr_visited = gui_buffers_visited; ptr_visited;
         ptr_visited = ptr_visited->next_buffer)
    {
        if (names[0])
            strcat (names, ",");
        strcat (names, ptr_visited->buffer->name);
    }
    return names;
}

/*
 * Test functions:
 *   gui_layout_window_apply
 */

TEST(GuiLayout, WindowApply)
{
    struct t_gui_layout *layout;
    struct t_gui_buffer *buffer1, *buffer2, *buffer3;

    gui_layout_window_apply (NULL, -1);

    buffer1 = gui_buffer_new (NULL, "test1", NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer1);
    buffer2 = gui_buffer_new (NULL, "test2", NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer2);
    buffer3 = gui_buffer_new (NULL, "test3", NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer3);

    gui_buffer_visited_remove_all ();

    /* Layout: test1 in top window (current), weechat in bottom window. */
    CHECK(gui_window_split_horizontal (gui_current_window, 50));
    gui_window_switch_to_buffer (gui_current_window, buffer1, 1);
    layout = gui_layout_alloc ("test");
    CHECK(layout);
    gui_layout_add (layout);
    gui_layout_window_store (layout);

    /* Single window, switch to test2 then test3. */
    gui_window_merge_all (gui_current_window);
    gui_window_switch_to_buffer (gui_current_window, buffer2, 1);
    gui_window_switch_to_buffer (gui_current_window, buffer3, 1);
    STRCMP_EQUAL("weechat,test1,test2,test3",
                 test_gui_layout_visited_buffers ());

    /*
     * Apply layout: the switches done to build the windows are not added to
     * visited buffers, only the switch from test3 to test1 in current window.
     */
    gui_layout_window_apply (layout, layout->internal_id_current_window);
    CHECK(gui_windows->next_window);
    POINTERS_EQUAL(buffer1, gui_current_window->buffer);
    STRCMP_EQUAL("weechat,test2,test3,test1",
                 test_gui_layout_visited_buffers ());

    /* Visited buffers frozen: list is not updated at all. */
    gui_window_merge_all (gui_current_window);
    gui_window_switch_to_buffer (gui_current_window, buffer2, 1);
    STRCMP_EQUAL("weechat,test3,test1,test2",
                 test_gui_layout_visited_buffers ());
    gui_buffers_visited_frozen = 1;
    gui_layout_window_apply (layout, layout->internal_id_current_window);
    gui_buffers_visited_frozen = 0;
    POINTERS_EQUAL(buffer1, gui_current_window->buffer);
    STRCMP_EQUAL("weechat,test3,test1,test2",
                 test_gui_layout_visited_buffers ());

    gui_window_merge_all (gui_current_window);
    gui_window_switch_to_buffer (gui_current_window, gui_buffers, 1);
    gui_layout_remove (layout);
    gui_buffer_close (buffer1);
    gui_buffer_close (buffer2);
    gui_buffer_close (buffer3);
    gui_buffer_visited_remove_all ();
}

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
