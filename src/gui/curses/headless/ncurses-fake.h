/*
 * Copyright (C) 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEECHAT_NCURSES_FAKE_H
#define WEECHAT_NCURSES_FAKE_H

#include <stdbool.h>
#include <stddef.h>

#define ERR (-1)
#define OK  (0)

#define TRUE 1
#define FALSE 0

#define COLS 80
#define LINES 25
#define COLORS 256
#define COLOR_PAIRS 256

#define COLOR_PAIR(x) x

#define COLOR_BLACK	0
#define COLOR_RED	1
#define COLOR_GREEN	2
#define COLOR_YELLOW	3
#define COLOR_BLUE	4
#define COLOR_MAGENTA	5
#define COLOR_CYAN	6
#define COLOR_WHITE	7

#define A_NORMAL 0
#define A_BLINK (1 << (11 + 8))
#define A_DIM (1 << (12 + 8))
#define A_BOLD (1 << (13 + 8))
#define A_UNDERLINE (1 << (9 + 8))
#define A_REVERSE (1 << (10 + 8))
#define A_ITALIC (1 << (23 + 8))

#define ACS_HLINE '-'
#define ACS_VLINE '|'

#define getyx(win, x, y)                        \
    x = 0;                                      \
    y = 0;

#define getmaxyx(win, x, y)                     \
    x = 0;                                      \
    y = 0;

struct _window
{
    int _cury, _curx;
    int _maxy, _maxx;
    int _begy, _begx;
};
typedef struct _window WINDOW;

typedef int attr_t;
typedef unsigned chtype;

struct _cchar_t
{
};
typedef struct _cchar_t cchar_t;

extern WINDOW *stdscr;
extern chtype acs_map[];

extern WINDOW *initscr (void);
extern int endwin (void);
extern WINDOW *newwin (int nlines, int ncols, int begin_y, int begin_x);
extern int delwin (WINDOW *win);
extern int move (int y, int x);
extern int wmove (WINDOW *win, int y, int x);
extern int wattr_on (WINDOW *win, attr_t attrs, void *opts);
extern int wattr_off (WINDOW *win, attr_t attrs, void *opts);
extern int wattr_get (WINDOW *win, attr_t *attrs, short *pair, void *opts);
extern int wattr_set (WINDOW *win, attr_t attrs, short pair, void *opts);
extern int wattron (WINDOW *win, int attrs);
extern int wattroff (WINDOW *win, int attrs);
extern int waddstr (WINDOW *win, const char *str);
extern int waddnstr (WINDOW *win, const char *str, int n);
extern int mvaddstr (int y, int x, const char *str);
extern int mvwaddstr (WINDOW *win, int y, int x, const char *str);
extern int wclrtobot (WINDOW *win);
extern int refresh (void);
extern int wrefresh (WINDOW *win);
extern int wnoutrefresh (WINDOW *win);
extern int wclrtoeol (WINDOW *win);
extern int mvwprintw (WINDOW *win, int y, int x, const char *fmt, ...);
extern int init_pair (short pair, short f, short b);
extern bool has_colors (void);
extern int cbreak (void);
extern int start_color (void);
extern int noecho (void);
extern int clear (void);
extern int wclear (WINDOW *win);
extern bool can_change_color (void);
extern int curs_set (int visibility);
extern int nodelay (WINDOW *win, bool bf);
extern int werase (WINDOW *win);
extern int wbkgdset (WINDOW *win, chtype ch);
extern void wbkgrndset (WINDOW *win, const cchar_t *wcval);
extern int setcchar (cchar_t *wcval, const wchar_t *wch, attr_t attrs, short pair, const void *opts);
extern void wchgat (WINDOW *win, int n, attr_t attr, short color,
                    const void *opts);
extern int mvwchgat (WINDOW *win, int y, int x, int n, attr_t attr, short pair,
                     const void *opts);
extern void whline (WINDOW *win, chtype ch, int n);
extern void wvline (WINDOW *win, chtype ch, int n);
extern int mvwhline (WINDOW *win, int y, int x, chtype ch, int n);
extern int mvwvline (WINDOW *win, int y, int x, chtype ch, int n);
extern int raw (void);
extern int wcolor_set (WINDOW *win, short pair, void *opts);
extern void cur_term (void);
extern int use_default_colors (void);
extern int resizeterm (int lines, int columns);
extern int getch (void);
extern int wgetch (WINDOW *win);

#endif /* WEECHAT_NCURSES_FAKE_H */
