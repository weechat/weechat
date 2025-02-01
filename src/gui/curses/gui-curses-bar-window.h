/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
