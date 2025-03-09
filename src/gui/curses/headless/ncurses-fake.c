/*
 * ncurses-fake.c - fake ncurses lib (for headless mode and tests)
 *
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

#include "ncurses-fake.h"


/* simulate 80x25 terminal */
WINDOW _stdscr = { 0, 0, 24, 79, 0, 0 };
WINDOW *stdscr = &_stdscr;
chtype acs_map[256];


WINDOW *
initscr (void)
{
    return stdscr;
}

int
endwin (void)
{
    return OK;
}

WINDOW *
newwin (int nlines, int ncols, int begin_y, int begin_x)
{
    (void) nlines;
    (void) ncols;
    (void) begin_y;
    (void) begin_x;

    return stdscr;
}

int
delwin (WINDOW *win)
{
    (void) win;

    return OK;
}

int
move (int y, int x)
{
    (void) y;
    (void) x;

    return OK;
}

int
wmove (WINDOW *win, int y, int x)
{
    (void) win;
    (void) y;
    (void) x;

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
wattr_set (WINDOW *win, attr_t attrs, short pair, void *opts)
{
    (void) win;
    (void) attrs;
    (void) pair;
    (void) opts;

    return OK;
}

int
wattron (WINDOW *win, int attrs)
{
    (void) win;
    (void) attrs;

    return OK;
}

int
wattroff (WINDOW *win, int attrs)
{
    (void) win;
    (void) attrs;

    return OK;
}

int
waddstr (WINDOW *win, const char *str)
{
    (void) win;
    (void) str;

    return OK;
}

int
waddnstr (WINDOW *win, const char *str, int n)
{
    (void) win;
    (void) str;
    (void) n;

    return OK;
}

int
mvaddstr (int y, int x, const char *str)
{
    (void) y;
    (void) x;
    (void) str;

    return OK;
}

int
mvwaddstr (WINDOW *win, int y, int x, const char *str)
{
    (void) win;
    (void) y;
    (void) x;
    (void) str;

    return OK;
}

int
wclrtobot (WINDOW *win)
{
    (void) win;

    return OK;
}

int
refresh (void)
{
    return OK;
}

int
wrefresh (WINDOW *win)
{
    (void) win;

    return OK;
}

int
wnoutrefresh (WINDOW *win)
{
    (void) win;

    return OK;
}

int
wclrtoeol (WINDOW *win)
{
    (void) win;

    return OK;
}

int
mvwprintw (WINDOW *win, int y, int x, const char *fmt, ...)
{
    (void) win;
    (void) y;
    (void) x;
    (void) fmt;

    return OK;
}

int
init_pair (short pair, short f, short b)
{
    (void) pair;
    (void) f;
    (void) b;

    return OK;
}

bool
has_colors (void)
{
    return TRUE;
}

int
cbreak (void)
{
    return OK;
}

int
start_color (void)
{
    return OK;
}

int
noecho (void)
{
    return OK;
}

int
clear (void)
{
    return OK;
}

int
wclear (WINDOW *win)
{
    (void) win;

    return OK;
}

bool
can_change_color (void)
{
    /* not supported in WeeChat anyway */
    return FALSE;
}

int
curs_set (int visibility)
{
    (void) visibility;

    return 1;  /* 0 == invisible, 1 == normal, 2 == very visible */
}

int
nodelay (WINDOW *win, bool bf)
{
    (void) win;
    (void) bf;

    return OK;
}

int
werase (WINDOW *win)
{
    (void) win;

    return OK;
}

int
wbkgdset (WINDOW *win, chtype ch)
{
    (void) win;
    (void) ch;

    return OK;
}

void
wbkgrndset (WINDOW *win, const cchar_t *wcval)
{
    (void) win;
    (void) wcval;
}

int
setcchar (cchar_t *wcval, const wchar_t *wch, attr_t attrs, short pair, const void *opts)
{
    (void) wcval;
    (void) wch;
    (void) attrs;
    (void) pair;
    (void) opts;

    return OK;
}

void
wchgat (WINDOW *win, int n, attr_t attr, short color, const void *opts)
{
    (void) win;
    (void) n;
    (void) attr;
    (void) color;
    (void) opts;
}

int
mvwchgat (WINDOW *win, int y, int x, int n, attr_t attr, short pair,
          const void *opts)
{
    (void) win;
    (void) y;
    (void) x;
    (void) n;
    (void) attr;
    (void) pair;
    (void) opts;

    return OK;
}


void
whline (WINDOW *win, chtype ch, int n)
{
    (void) win;
    (void) ch;
    (void) n;
}

void
wvline (WINDOW *win, chtype ch, int n)
{
    (void) win;
    (void) ch;
    (void) n;
}

int
mvwhline (WINDOW *win, int y, int x, chtype ch, int n)
{
    (void) win;
    (void) y;
    (void) x;
    (void) ch;
    (void) n;

    return OK;
}

int
mvwvline (WINDOW *win, int y, int x, chtype ch, int n)
{
    (void) win;
    (void) y;
    (void) x;
    (void) ch;
    (void) n;

    return OK;
}

int
raw (void)
{
    return OK;
}

int
wcolor_set (WINDOW *win, short pair, void *opts)
{
    (void) win;
    (void) pair;
    (void) opts;

    return OK;
}

void
cur_term (void)
{
}

int
use_default_colors (void)
{
    return OK;
}

int
resizeterm (int lines, int columns)
{
    (void) lines;
    (void) columns;

    return OK;
}

int
getch (void)
{
    return ERR;
}

int
wgetch (WINDOW *win)
{
    (void) win;

    return OK;
}
