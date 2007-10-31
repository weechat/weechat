/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

#define WINDOW_MIN_WIDTH      10
#define WINDOW_MIN_HEIGHT     5

#define GUI_CURSES(window) ((t_gui_curses_objects *)(window->gui_objects))

typedef struct t_gui_curses_objects t_gui_curses_objects;

struct t_gui_curses_objects
{
    WINDOW *win_title;              /* title window                         */
    WINDOW *win_chat;               /* chat window (example: channel)       */
    WINDOW *win_nick;               /* nick window                          */
    WINDOW *win_status;             /* status window                        */
    WINDOW *win_infobar;            /* info bar window                      */
    WINDOW *win_input;              /* input window                         */
    WINDOW *win_separator;          /* separation between 2 splited (V) win */
};

extern struct t_gui_color gui_weechat_colors[];
extern int gui_refresh_screen_needed;

/* color functions */
extern int gui_color_get_pair (int);
extern void gui_color_init ();

/* chat functions */
extern void gui_chat_calculate_line_diff (struct t_gui_window *,
                                          struct t_gui_line **, int *, int);

/* keyboard functions */
extern void gui_keyboard_default_bindings ();
extern void gui_keyboard_read ();
extern void gui_keyboard_flush ();

/* window functions */
extern void gui_window_wprintw (WINDOW *, char *, ...);
extern void gui_window_curses_clear (WINDOW *, int);
extern void gui_window_set_weechat_color (WINDOW *, int);
extern void gui_window_refresh_screen_sigwinch ();
extern void gui_window_title_set ();
extern void gui_window_title_reset ();

#endif /* gui-curses.h */
