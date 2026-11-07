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

extern int gui_window_current_color_attr;
extern void gui_window_set_color_style (WINDOW *window, int style);
extern void gui_window_remove_color_style (WINDOW *window, int style);
extern void gui_window_set_custom_color_fg_bg (WINDOW *window, int fg, int bg,
                                               int reset_attributes);
extern void gui_window_string_apply_color_bg (unsigned char **str,
                                              WINDOW *window);
extern void gui_window_string_apply_color_fg_bg (unsigned char **str,
                                                 WINDOW *window);
extern int gui_window_current_style_fg;
extern int gui_window_current_style_bg;
}

TEST_GROUP(GuiCursesWindow)
{
};

/*
 * Test functions:
 *   gui_window_set_color_style
 *   gui_window_remove_color_style
 */

TEST(GuiCursesWindow, SetRemoveColorStyle)
{
    int old_color_attr;

    old_color_attr = gui_window_current_color_attr;
    gui_window_current_color_attr = 0;

    gui_window_set_color_style (NULL, A_REVERSE | A_BOLD);
    LONGS_EQUAL(A_REVERSE | A_BOLD, gui_window_current_color_attr);

    /* Removing an attribute which is not set keeps the other ones. */
    gui_window_remove_color_style (NULL, A_ITALIC);
    LONGS_EQUAL(A_REVERSE | A_BOLD, gui_window_current_color_attr);

    gui_window_remove_color_style (NULL, A_BOLD);
    LONGS_EQUAL(A_REVERSE, gui_window_current_color_attr);

    gui_window_remove_color_style (NULL, A_REVERSE);
    LONGS_EQUAL(0, gui_window_current_color_attr);

    gui_window_current_color_attr = old_color_attr;
}

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

/*
 * Test functions:
 *   gui_window_string_apply_color_bg (truncated extended color)
 *   gui_window_string_apply_color_fg_bg (truncated extended color)
 */

TEST(GuiCursesWindow, StringApplyColorTruncated)
{
    unsigned char *ptr_string;

    /*
     * Truncated extended background: the extended char is skipped, like in
     * function gui_color_code_size.
     */
    ptr_string = (unsigned char *)"@12x";
    gui_window_string_apply_color_bg (&ptr_string, NULL);
    STRCMP_EQUAL("12x", (const char *)ptr_string);
    LONGS_EQUAL(3, gui_color_code_size ("\x19" "B@12x"));

    ptr_string = (unsigned char *)"01~@12x";
    gui_window_string_apply_color_fg_bg (&ptr_string, NULL);
    STRCMP_EQUAL("12x", (const char *)ptr_string);
    LONGS_EQUAL(6, gui_color_code_size ("\x19" "*01~@12x"));
}
