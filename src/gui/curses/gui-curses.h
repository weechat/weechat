/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_GUI_CURSES_H
#define __WEECHAT_GUI_CURSES_H 1

#ifdef HAVE_NCURSESW_CURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

struct t_gui_buffer;

#define GUI_WINDOW_MIN_WIDTH          10
#define GUI_WINDOW_MIN_HEIGHT         5

#define GUI_WINDOW_CHAT_MIN_WIDTH     5
#define GUI_WINDOW_CHAT_MIN_HEIGHT    2

#define GUI_CURSES_NUM_WEECHAT_COLORS 15

#define GUI_CURSES(window) ((struct t_gui_curses_objects *)(window->gui_objects))

struct t_gui_bar_window
{
    struct t_gui_bar *bar;          /* pointer to bar                       */
    int x, y;                       /* position of window                   */
    int width, height;              /* window size                          */
    int scroll_x, scroll_y;         /* X-Y scroll in bar                    */
    int cursor_x, cursor_y;         /* use to move cursor on screen (for    */
                                    /* input_text item)                     */
    WINDOW *win_bar;                /* bar Curses window                    */
    int current_size;               /* current size (width or height)       */
    WINDOW *win_separator;          /* separator (optional)                 */
    struct t_gui_bar_window *prev_bar_window; /* link to previous bar win   */
                                              /* (only for non-root bars)   */
    struct t_gui_bar_window *next_bar_window; /* link to next bar win       */
                                              /* (only for non-root bars)   */
};

struct t_gui_curses_objects
{
    WINDOW *win_chat;               /* chat window (example: channel)       */
    WINDOW *win_separator;          /* separation between 2 splited (V) win */
    struct t_gui_bar_window *bar_windows;     /* bar windows                */
    struct t_gui_bar_window *last_bar_window; /* last bar window            */
};

extern struct t_gui_color gui_weechat_colors[];

/* color functions */
extern int gui_color_get_pair (int num_color);
extern void gui_color_pre_init ();
extern void gui_color_init ();
extern void gui_color_end ();

/* bar functions */
extern void gui_bar_window_calculate_pos_size (struct t_gui_bar_window *bar_window,
                                               struct t_gui_window *window);
extern void gui_bar_window_create_win (struct t_gui_bar_window *bar_window);
extern int gui_bar_window_remove_unused_bars (struct t_gui_window *window);
extern int gui_bar_window_add_missing_bars (struct t_gui_window *window);

/* chat functions */
extern void gui_chat_calculate_line_diff (struct t_gui_window *window,
                                          struct t_gui_line **line,
                                          int *line_pos, int difference);

/* keyboard functions */
extern void gui_keyboard_default_bindings ();
extern int gui_keyboard_read_cb (void *data);

/* window functions */
extern void gui_window_redraw_buffer (struct t_gui_buffer *buffer);
extern int gui_window_utf_char_valid (const char *utf_char);
extern void gui_window_wprintw (WINDOW *window, const char *data, ...);
extern void gui_window_clear_weechat (WINDOW *window, int num_color);
extern void gui_window_clear (WINDOW *window, int bg);
extern void gui_window_reset_style (WINDOW *window, int num_color);
extern void gui_window_set_color_style (WINDOW *window, int style);
extern void gui_window_remove_color_style (WINDOW *window, int style);
extern void gui_window_set_color (WINDOW *window, int fg, int bg);
extern void gui_window_set_weechat_color (WINDOW *window, int num_color);
extern void gui_window_set_custom_color_fg_bg (WINDOW *window, int fg, int bg);
extern void gui_window_set_custom_color_fg (WINDOW *window, int fg);
extern void gui_window_set_custom_color_bg (WINDOW *window, int bg);
extern void gui_window_clrtoeol_with_current_bg (WINDOW *window);
extern void gui_window_title_set ();
extern void gui_window_title_reset ();

#endif /* gui-curses.h */
