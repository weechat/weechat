/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_CURSES_BAR_WINDOW_H
#define WEECHAT_GUI_CURSES_BAR_WINDOW_H

#define GUI_BAR_WINDOW_OBJECTS(bar_window)                              \
    ((struct t_gui_bar_window_curses_objects *)(bar_window->gui_objects))

struct t_gui_bar_window_curses_objects
{
    WINDOW *win_bar;                /* bar Curses window                    */
    WINDOW *win_separator;          /* separator (optional)                 */
};

#endif /* WEECHAT_GUI_CURSES_BAR_WINDOW_H */
