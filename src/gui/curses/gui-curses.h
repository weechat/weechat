/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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

#ifdef __CYGWIN__
#include <sys/termios.h>
#endif

struct t_gui_buffer;
struct t_gui_window;
struct t_gui_bar_window;

#define GUI_CURSES_NUM_WEECHAT_COLORS 16

#define GUI_WINDOW_OBJECTS(window)                                      \
    ((struct t_gui_window_curses_objects *)(window->gui_objects))
#define GUI_BAR_WINDOW_OBJECTS(bar_window)                              \
    ((struct t_gui_bar_window_curses_objects *)(bar_window->gui_objects))

struct t_gui_window_curses_objects
{
    WINDOW *win_chat;               /* chat window (example: channel)       */
    WINDOW *win_separator;          /* separation between 2 split (V) win   */
};

struct t_gui_bar_window_curses_objects
{
    WINDOW *win_bar;                /* bar Curses window                    */
    WINDOW *win_separator;          /* separator (optional)                 */
};

extern int gui_term_cols, gui_term_lines;
extern struct t_gui_color gui_weechat_colors[];
extern int gui_color_last_pair;
extern int gui_color_num_bg;

/* color functions */
extern int gui_color_get_pair (int num_color);
extern void gui_color_pre_init ();
extern void gui_color_init ();
extern void gui_color_end ();

/* chat functions */
extern void gui_chat_calculate_line_diff (struct t_gui_window *window,
                                          struct t_gui_line **line,
                                          int *line_pos, int difference);

/* keyboard functions */
extern void gui_keyboard_default_bindings ();
extern int gui_keyboard_read_cb (void *data, int fd);

/* window functions */
extern void gui_window_read_terminal_size ();
extern void gui_window_redraw_buffer (struct t_gui_buffer *buffer);
extern int gui_window_utf_char_valid (const char *utf_char);
extern int gui_window_get_hline_char ();
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
extern void gui_window_set_title (const char *title);

#endif /* gui-curses.h */
