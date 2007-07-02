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

/* shift ncurses colors for compatibility with colors
   in IRC messages (same as other IRC clients) */

#define WEECHAT_COLOR_BLACK   COLOR_BLACK
#define WEECHAT_COLOR_RED     COLOR_BLUE
#define WEECHAT_COLOR_GREEN   COLOR_GREEN
#define WEECHAT_COLOR_YELLOW  COLOR_CYAN
#define WEECHAT_COLOR_BLUE    COLOR_RED
#define WEECHAT_COLOR_MAGENTA COLOR_MAGENTA
#define WEECHAT_COLOR_CYAN    COLOR_YELLOW
#define WEECHAT_COLOR_WHITE   COLOR_WHITE

#define WINDOW_MIN_WIDTH      10
#define WINDOW_MIN_HEIGHT     5

#define GUI_CURSES(window) ((t_gui_curses_objects *)(window->gui_objects))

typedef struct t_gui_panel_window t_gui_panel_window;

struct t_gui_panel_window
{
    t_gui_panel *panel;             /* pointer to panel                     */
    int x, y;                       /* position of window                   */
    int width, height;              /* window size                          */
    WINDOW *win_panel;              /* panel Curses window                  */
    WINDOW *win_separator;          /* separator (optional)                 */
    t_gui_panel_window *next_panel_window;
                                    /* link to next panel window            */
                                    /* (only used if panel is in windows)   */
};

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
    t_gui_panel_window *panel_windows; /* panel windows                     */
};

extern t_gui_color gui_weechat_colors[];
extern int gui_irc_colors[GUI_NUM_IRC_COLORS][2];
extern int gui_refresh_screen_needed;

/* color functions */
extern int gui_color_get_pair (int);
extern void gui_color_init ();

/* chat functions */
extern void gui_chat_calculate_line_diff (t_gui_window *, t_gui_line **, int *, int);

/* keyboard functions */
extern void gui_keyboard_default_bindings ();
extern void gui_keyboard_read ();

/* window functions */
extern void gui_window_wprintw (WINDOW *, char *, ...);
extern void gui_window_curses_clear (WINDOW *, int);
extern void gui_window_set_weechat_color (WINDOW *, int);
extern void gui_window_refresh_screen_sigwinch ();
extern void gui_window_set_title ();
extern void gui_window_reset_title ();

/* panel functions */
extern int gui_panel_window_get_size (t_gui_panel *, t_gui_window *, int);
extern void gui_panel_redraw_buffer (t_gui_buffer *);

#endif /* gui-curses.h */
