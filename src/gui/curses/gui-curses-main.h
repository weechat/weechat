/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_CURSES_MAIN_H
#define WEECHAT_GUI_CURSES_MAIN_H

extern int gui_term_cols, gui_term_lines;

extern void gui_main_init (void);
extern void gui_main_loop (void);

#endif /* WEECHAT_GUI_CURSES_MAIN_H */
