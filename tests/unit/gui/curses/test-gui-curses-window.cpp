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
#include "src/core/core-config.h"
#include "src/core/core-config-file.h"
#include "src/gui/gui-color.h"

extern struct t_gui_color *gui_weechat_colors;
extern struct t_gui_color gui_weechat_colors_bold[];
extern int gui_window_current_color_attr;
extern int gui_window_current_color_bold;
extern void gui_window_set_color_style (WINDOW *window, int style);
extern void gui_window_remove_color_style (WINDOW *window, int style);
extern void gui_window_set_custom_color_fg (WINDOW *window, int fg);
extern void gui_window_set_custom_color_fg_bg (WINDOW *window, int fg, int bg,
                                               int reset_attributes);
extern void gui_window_string_apply_color_bg (unsigned char **str,
                                              WINDOW *window);
extern void gui_window_string_apply_color_fg_bg (unsigned char **str,
                                                 WINDOW *window);
extern int gui_window_current_style_fg;
extern int gui_window_current_style_bg;
extern void gui_window_reset_style (WINDOW *window, int weechat_color);
extern void gui_window_reset_color (WINDOW *window, int weechat_color);
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
 *   gui_window_set_color_bold
 *   gui_window_set_custom_color_fg (bold of light colors)
 */

TEST(GuiCursesWindow, ColorBold)
{
    struct t_gui_color *old_colors;
    WINDOW win;
    int old_color_attr, old_color_bold, color_default, color_darkgray;
    int color_lightred;

    old_colors = gui_weechat_colors;
    old_color_attr = gui_window_current_color_attr;
    old_color_bold = gui_window_current_color_bold;

    /* Terminal with less than 16 colors: light colors are displayed bold. */
    gui_weechat_colors = gui_weechat_colors_bold;
    gui_window_current_color_attr = 0;
    gui_window_current_color_bold = 0;
    memset (&win, 0, sizeof (win));

    color_default = gui_color_search ("default");
    color_darkgray = gui_color_search ("darkgray");
    color_lightred = gui_color_search ("lightred");

    /* Light color: bold is set to display it, not the bold attribute. */
    gui_window_set_custom_color_fg (
        &win, color_darkgray | GUI_COLOR_EXTENDED_KEEPATTR_FLAG);
    LONGS_EQUAL(1, gui_window_current_color_bold);
    LONGS_EQUAL(0, gui_window_current_color_attr & A_BOLD);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);

    /* Bold attribute set then removed: bold of light color is kept. */
    gui_window_set_color_style (&win, A_BOLD);
    LONGS_EQUAL(A_BOLD, gui_window_current_color_attr & A_BOLD);
    gui_window_remove_color_style (&win, A_BOLD);
    LONGS_EQUAL(0, gui_window_current_color_attr & A_BOLD);
    LONGS_EQUAL(1, gui_window_current_color_bold);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);

    /* Other color with attributes kept: bold of light color is removed. */
    gui_window_set_custom_color_fg (
        &win, color_default | GUI_COLOR_EXTENDED_KEEPATTR_FLAG);
    LONGS_EQUAL(0, gui_window_current_color_bold);
    LONGS_EQUAL(0, gui_window_current_color_attr & A_BOLD);
    LONGS_EQUAL(0, win._attrs & A_BOLD);

    /* Bold attribute is kept with another color. */
    gui_window_set_color_style (&win, A_BOLD);
    gui_window_set_custom_color_fg (
        &win, color_darkgray | GUI_COLOR_EXTENDED_KEEPATTR_FLAG);
    gui_window_set_custom_color_fg (
        &win, color_default | GUI_COLOR_EXTENDED_KEEPATTR_FLAG);
    LONGS_EQUAL(0, gui_window_current_color_bold);
    LONGS_EQUAL(A_BOLD, gui_window_current_color_attr & A_BOLD);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);

    /* Extended color: no bold to display it. */
    gui_window_set_custom_color_fg (&win, color_lightred);
    LONGS_EQUAL(1, gui_window_current_color_bold);
    LONGS_EQUAL(0, gui_window_current_color_attr & A_BOLD);
    gui_window_set_custom_color_fg (
        &win,
        214 | GUI_COLOR_EXTENDED_FLAG | GUI_COLOR_EXTENDED_KEEPATTR_FLAG);
    LONGS_EQUAL(0, gui_window_current_color_bold);
    LONGS_EQUAL(0, win._attrs & A_BOLD);

    gui_weechat_colors = old_colors;
    gui_window_current_color_attr = old_color_attr;
    gui_window_current_color_bold = old_color_bold;
}

/*
 * Test functions:
 *   gui_window_reset_style
 *   gui_window_reset_color
 *   gui_window_set_custom_color_fg (bold of option colors)
 */

TEST(GuiCursesWindow, OptionColorBold)
{
    WINDOW win;
    int old_color_attr, old_color_bold, color_blue;

    old_color_attr = gui_window_current_color_attr;
    old_color_bold = gui_window_current_color_bold;

    memset (&win, 0, sizeof (win));
    color_blue = gui_color_search ("blue");

    /* Option color with bold attribute: bold is kept with other colors. */
    config_file_option_set (config_color_chat, "*default", 1);
    LONGS_EQUAL(A_BOLD, gui_color[GUI_COLOR_CHAT]->attributes & A_BOLD);
    LONGS_EQUAL(0, gui_color[GUI_COLOR_CHAT]->bold_light);
    gui_window_reset_style (&win, GUI_COLOR_CHAT);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);
    gui_window_set_custom_color_fg (
        &win, color_blue | GUI_COLOR_EXTENDED_KEEPATTR_FLAG);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);
    gui_window_reset_color (&win, GUI_COLOR_CHAT);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);
    config_file_option_reset (config_color_chat, 1);

    /*
     * Light option color on a terminal with less than 16 colors: bold is
     * kept when the bold attribute is removed, and removed with another
     * color.
     */
    config_file_option_set (config_look_color_basic_force_bold, "on", 1);
    config_file_option_set (config_color_chat, "lightred", 1);
    LONGS_EQUAL(0, gui_color[GUI_COLOR_CHAT]->attributes & A_BOLD);
    LONGS_EQUAL(1, gui_color[GUI_COLOR_CHAT]->bold_light);
    gui_window_reset_style (&win, GUI_COLOR_CHAT);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);
    gui_window_set_color_style (&win, A_BOLD);
    gui_window_remove_color_style (&win, A_BOLD);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);
    gui_window_set_custom_color_fg (
        &win, color_blue | GUI_COLOR_EXTENDED_KEEPATTR_FLAG);
    LONGS_EQUAL(0, win._attrs & A_BOLD);

    /*
     * Light option color with bold attribute: bold is kept when the bold
     * attribute is removed (light color), and kept with another color while
     * the bold attribute is set.
     */
    config_file_option_set (config_color_chat, "*lightred", 1);
    LONGS_EQUAL(A_BOLD, gui_color[GUI_COLOR_CHAT]->attributes & A_BOLD);
    LONGS_EQUAL(1, gui_color[GUI_COLOR_CHAT]->bold_light);
    gui_window_reset_style (&win, GUI_COLOR_CHAT);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);
    LONGS_EQUAL(A_BOLD, gui_window_current_color_attr & A_BOLD);
    gui_window_set_custom_color_fg (
        &win, color_blue | GUI_COLOR_EXTENDED_KEEPATTR_FLAG);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);
    gui_window_reset_style (&win, GUI_COLOR_CHAT);
    gui_window_remove_color_style (&win, A_BOLD);
    LONGS_EQUAL(A_BOLD, win._attrs & A_BOLD);
    gui_window_set_custom_color_fg (
        &win, color_blue | GUI_COLOR_EXTENDED_KEEPATTR_FLAG);
    LONGS_EQUAL(0, win._attrs & A_BOLD);
    config_file_option_reset (config_color_chat, 1);
    config_file_option_reset (config_look_color_basic_force_bold, 1);

    gui_window_current_color_attr = old_color_attr;
    gui_window_current_color_bold = old_color_bold;
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
