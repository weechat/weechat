/*
 * ncurses-fake.c - fake ncurses lib used for tests
 *
 * Copyright (C) 2014-2018 Sébastien Helleu <flashcode@flashtux.org>
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

#define ERR (-1)
#define OK  (0)

struct _window
{
    int _cury, _curx;
    int _maxy, _maxx;
    int _begy, _begx;
};
typedef struct _window WINDOW;

typedef unsigned char bool;
typedef int attr_t;
typedef unsigned chtype;

/* simulate 80x25 terminal */
WINDOW stdscr = { 0, 0, 24, 79, 0, 0 };
chtype acs_map[256];


WINDOW
*initscr ()
{
    return &stdscr;
}

int
endwin ()
{
    return OK;
}

WINDOW
*newwin ()
{
    return &stdscr;
}

int
delwin ()
{
    return OK;
}

int
wmove (WINDOW *win, int y, int x)
{
    (void)win;
    (void)y;
    (void)x;
    return OK;
}

int
wattr_on (WINDOW *win, attr_t attrs, void *opts)
{
    (void) win;
    (void) attrs;
    (void) opts;
    return OK;
}

int
wattr_off (WINDOW *win, attr_t attrs, void *opts)
{
    (void) win;
    (void) attrs;
    (void) opts;
    return OK;
}

int
wattr_get (WINDOW *win, attr_t *attrs, short *pair, void *opts)
{
    (void) win;
    (void) attrs;
    (void) pair;
    (void) opts;
    return OK;
}

int
wattr_set (WINDOW *win, attr_t *attrs, short *pair, void *opts)
{
    (void) win;
    (void) attrs;
    (void) pair;
    (void) opts;
    return OK;
}

int
waddnstr(WINDOW *win, const char *str, int n)
{
    (void) win;
    (void) str;
    (void) n;
    return OK;
}

int
wclrtobot(WINDOW *win)
{
    (void) win;
    return OK;
}

int
wrefresh(WINDOW *win)
{
    (void) win;
    return OK;
}

int
wnoutrefresh(WINDOW *win)
{
    (void) win;
    return OK;
}

int
wclrtoeol(WINDOW *win)
{
    (void) win;
    return OK;
}

int
mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
    (void) win;
    (void) y;
    (void) x;
    (void) fmt;
    return OK;
}

int
init_pair(short pair, short f, short b)
{
    (void) pair;
    (void) f;
    (void) b;
    return OK;
}

int
has_colors()
{
    return 1;
}

int
cbreak()
{
    return OK;
}

int
start_color()
{
    return OK;
}

int
noecho()
{
    return OK;
}

int
wclear(WINDOW *win)
{
    (void) win;
    return OK;
}

int
wgetch(WINDOW *win)
{
    (void) win;
    return OK;
}

int
can_change_color()
{
    /* not supported in WeeChat anyway */
    return 0;
}

int
curs_set(int visibility)
{
    (void) visibility;
    return 1;  /* 0 == invisible, 1 == normal, 2 == very visible */
}

int
nodelay(WINDOW *win, bool bf)
{
    (void) win;
    (void) bf;
    return OK;
}

int
werase(WINDOW *win)
{
    (void) win;
    return OK;
}

int
wbkgdset(WINDOW *win, chtype ch)
{
    (void) win;
    (void) ch;
    return OK;
}

void
wchgat(WINDOW *win, int n, attr_t attr, short color, const void *opts)
{
    (void) win;
    (void) n;
    (void) attr;
    (void) color;
    (void) opts;
}

void
whline()
{
}

void
wvline()
{
}

void
raw()
{
}

void
wcolor_set()
{
}

void
cur_term()
{
}

void
use_default_colors()
{
}

void
resizeterm()
{
}

int
COLS()
{
    /* simulate 80x25 terminal */
    return 80;
}

int
LINES()
{
    /* simulate 80x25 terminal */
    return 25;
}

int
COLORS()
{
    /* simulate 256-color terminal */
    return 256;
}

int
COLOR_PAIRS()
{
    /* simulate 256-color terminal */
    return 256;
}
