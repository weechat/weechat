/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_CURSES_H
#define WEECHAT_GUI_CURSES_H

#ifdef WEECHAT_HEADLESS
#include "ncurses-fake.h"
#else
#define NCURSES_WIDECHAR 1
#ifdef HAVE_NCURSESW_CURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif /* HAVE_NCURSESW_CURSES_H */
#endif /* WEECHAT_HEADLESS */

#endif /* WEECHAT_GUI_CURSES_H */
