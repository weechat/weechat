/*
 * SPDX-FileCopyrightText: 2026 Thomas Matthijs <github@selckin.be>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test window functions (Curses interface) */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include "src/gui/curses/headless/ncurses-fake.h"
#include "src/gui/gui-color.h"

extern void gui_window_set_custom_color_fg_bg (WINDOW *window, int fg, int bg,
                                               int reset_attributes);
extern int gui_window_current_style_fg;
extern int gui_window_current_style_bg;
}

TEST_GROUP(GuiCursesWindow)
{
};

/*
 * Test functions:
 *   gui_window_set_custom_color_fg_bg (invalid background)
 */

TEST(GuiCursesWindow, CustomColorFgBgInvalid)
{
    WINDOW win;
    int saved_style_fg, saved_style_bg, old_style_fg, old_style_bg;
    int color_blue, color_red;

    saved_style_fg = gui_window_current_style_fg;
    saved_style_bg = gui_window_current_style_bg;
    memset (&win, 0, sizeof (win));
    color_blue = gui_color_search ("blue");
    color_red = gui_color_search ("red");

    /* Valid colors. */
    gui_window_set_custom_color_fg_bg (&win, color_red, color_blue, 0);
    CHECK(gui_window_current_style_bg >= 0);
    old_style_bg = gui_window_current_style_bg;

    /* Basic background out of range (for example "~99"): not changed. */
    gui_window_set_custom_color_fg_bg (&win, color_red, 99, 0);
    LONGS_EQUAL(old_style_bg, gui_window_current_style_bg);
    gui_window_set_custom_color_fg_bg (&win, color_red, 50, 0);
    LONGS_EQUAL(old_style_bg, gui_window_current_style_bg);

    /* Basic text color out of range (for example "*99~"): not changed. */
    old_style_fg = gui_window_current_style_fg;
    gui_window_set_custom_color_fg_bg (&win, 99, color_red, 0);
    LONGS_EQUAL(old_style_fg, gui_window_current_style_fg);
    CHECK(gui_window_current_style_bg != old_style_bg);

    gui_window_current_style_fg = saved_style_fg;
    gui_window_current_style_bg = saved_style_bg;
}
