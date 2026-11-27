/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test window functions (Curses interface) */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "src/gui/curses/headless/ncurses-fake.h"
#include "src/gui/gui-color.h"
#include "src/gui/curses/gui-curses-color.h"
#include "src/gui/curses/gui-curses-window.h"

extern int gui_window_current_style_fg;
extern int gui_window_current_style_bg;
extern int gui_window_user_style_depth;
extern int gui_color_term_colors;
extern void gui_window_string_apply_color_fg_bg (unsigned char **string,
                                                 WINDOW *window);
}

/*
 * Apply the fg+bg color code "__code" (the part after "^Y*") on stdscr and
 * check the current foreground and background colors.
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
 * Test functions:
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
        /* Test with 256 colors, then 8 colors (like TERM=xterm). */
        gui_color_term_colors = (i == 0) ? 256 : 8;

        /* Valid WeeChat colors: black on blue. */
        WEE_CHECK_APPLY_FG_BG(
            gui_weechat_colors[1].foreground,
            (i == 0) ?
            gui_weechat_colors[9].background : gui_weechat_colors[9].foreground,
            "01~09");

        /* Extended color for background. */
        WEE_CHECK_APPLY_FG_BG(gui_weechat_colors[1].foreground, 214,
                              "01~@00214");

        /* Invalid WeeChat colors for background: current one is kept. */
        WEE_CHECK_APPLY_FG_BG(gui_weechat_colors[1].foreground, 214,
                              "01~17");
        WEE_CHECK_APPLY_FG_BG(gui_weechat_colors[1].foreground, 214,
                              "01,18");
        WEE_CHECK_APPLY_FG_BG(gui_weechat_colors[1].foreground, 214,
                              "01,99");
    }

    gui_color_term_colors = saved_term_colors;
    /* Remove pairs cached with 8 colors (index depends on number of colors). */
    gui_color_reset_pairs ();
    gui_window_current_style_fg = saved_fg;
    gui_window_current_style_bg = saved_bg;
}

/*
 * Test functions:
 *   gui_window_user_style_push
 *   gui_window_user_style_pop
 *   gui_window_user_style_reset
 */

TEST(GuiCursesWindow, UserStyle)
{
    int saved_fg, saved_bg, saved_emphasis, i;

    saved_fg = gui_window_current_style_fg;
    saved_bg = gui_window_current_style_bg;
    saved_emphasis = gui_window_current_emphasis;

    gui_window_user_style_reset ();
    LONGS_EQUAL(0, gui_window_user_style_depth);

    /* Pop with empty stack: no effect. */
    gui_window_current_style_fg = 1;
    gui_window_current_style_bg = 2;
    gui_window_user_style_pop (stdscr);
    LONGS_EQUAL(0, gui_window_user_style_depth);
    LONGS_EQUAL(1, gui_window_current_style_fg);
    LONGS_EQUAL(2, gui_window_current_style_bg);

    /* Nested push/pop. */
    gui_window_current_emphasis = 0;
    gui_window_user_style_push (stdscr);
    LONGS_EQUAL(1, gui_window_user_style_depth);
    gui_window_current_style_fg = 3;
    gui_window_current_style_bg = 4;
    gui_window_current_emphasis = 1;
    gui_window_user_style_push (stdscr);
    LONGS_EQUAL(2, gui_window_user_style_depth);
    gui_window_current_style_fg = 5;
    gui_window_current_style_bg = 6;
    gui_window_current_emphasis = 0;
    gui_window_user_style_pop (stdscr);
    LONGS_EQUAL(1, gui_window_user_style_depth);
    LONGS_EQUAL(3, gui_window_current_style_fg);
    LONGS_EQUAL(4, gui_window_current_style_bg);
    /* Emphasis is not restored (it is toggled by search in buffer). */
    LONGS_EQUAL(0, gui_window_current_emphasis);
    gui_window_current_emphasis = 1;
    gui_window_user_style_pop (stdscr);
    LONGS_EQUAL(0, gui_window_user_style_depth);
    LONGS_EQUAL(1, gui_window_current_style_fg);
    LONGS_EQUAL(2, gui_window_current_style_bg);
    LONGS_EQUAL(1, gui_window_current_emphasis);

    /* Stack overflow: extra pops do nothing until the stack is reached. */
    for (i = 0; i < 36; i++)
    {
        gui_window_current_style_fg = 100 + i;
        gui_window_user_style_push (stdscr);
    }
    LONGS_EQUAL(36, gui_window_user_style_depth);
    gui_window_current_style_fg = 999;
    for (i = 0; i < 4; i++)
    {
        gui_window_user_style_pop (stdscr);
        LONGS_EQUAL(999, gui_window_current_style_fg);
    }
    LONGS_EQUAL(32, gui_window_user_style_depth);
    gui_window_user_style_pop (stdscr);
    LONGS_EQUAL(131, gui_window_current_style_fg);

    /* Reset. */
    gui_window_user_style_reset ();
    LONGS_EQUAL(0, gui_window_user_style_depth);

    gui_window_current_style_fg = saved_fg;
    gui_window_current_style_bg = saved_bg;
    gui_window_current_emphasis = saved_emphasis;
}
