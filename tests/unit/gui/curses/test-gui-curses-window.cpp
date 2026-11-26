/*
 * test-gui-curses-window.cpp - test window functions (Curses interface)
 *
 * Copyright (C) 2026 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/gui/curses/headless/ncurses-fake.h"
#include "src/gui/gui-color.h"
#include "src/gui/curses/gui-curses-color.h"
#include "src/gui/curses/gui-curses-window.h"

extern int gui_window_current_style_fg;
extern int gui_window_current_style_bg;
extern int gui_color_term_colors;
extern void gui_window_string_apply_color_fg_bg (unsigned char **string,
                                                 WINDOW *window);
}

/*
 * Applies the fg+bg color code "__code" (the part after "^Y*") on stdscr and
 * checks the current foreground and background colors.
 */

#define WEE_CHECK_APPLY_FG_BG(__fg, __bg, __code)                       \
    ptr_string = (unsigned char *)__code;                               \
    gui_window_string_apply_color_fg_bg (&ptr_string, stdscr);          \
    LONGS_EQUAL(__fg, gui_window_current_style_fg);                     \
    LONGS_EQUAL(__bg, gui_window_current_style_bg);

TEST_GROUP(GuiCursesWindow)
{
};

/*
 * Tests functions:
 *   gui_window_string_apply_color_fg_bg
 *   gui_window_set_custom_color_fg_bg
 */

TEST(GuiCursesWindow, StringApplyColorFgBg)
{
    unsigned char *ptr_string;
    int saved_term_colors, saved_fg, saved_bg, i;

    saved_term_colors = gui_color_term_colors;
    saved_fg = gui_window_current_style_fg;
    saved_bg = gui_window_current_style_bg;

    for (i = 0; i < 2; i++)
    {
        /* test with 256 colors, then 8 colors (like TERM=xterm) */
        gui_color_term_colors = (i == 0) ? 256 : 8;

        /* valid WeeChat colors: black on blue */
        WEE_CHECK_APPLY_FG_BG(
            gui_weechat_colors[1].foreground,
            (i == 0) ?
            gui_weechat_colors[9].background : gui_weechat_colors[9].foreground,
            "01~09");

        /* extended color for background */
        WEE_CHECK_APPLY_FG_BG(gui_weechat_colors[1].foreground, 214,
                              "01~@00214");

        /* invalid WeeChat colors for background: current one is kept */
        WEE_CHECK_APPLY_FG_BG(gui_weechat_colors[1].foreground, 214,
                              "01~17");
        WEE_CHECK_APPLY_FG_BG(gui_weechat_colors[1].foreground, 214,
                              "01,18");
        WEE_CHECK_APPLY_FG_BG(gui_weechat_colors[1].foreground, 214,
                              "01,99");
    }

    gui_color_term_colors = saved_term_colors;
    /* remove pairs cached with 8 colors (index depends on number of colors) */
    gui_color_reset_pairs ();
    gui_window_current_style_fg = saved_fg;
    gui_window_current_style_bg = saved_bg;
}
