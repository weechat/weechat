/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_CURSES_CHAT_H
#define WEECHAT_GUI_CURSES_CHAT_H

extern void gui_chat_calculate_line_diff (struct t_gui_window *window,
                                          struct t_gui_line **line,
                                          int *line_pos, int difference);

#endif /* WEECHAT_GUI_CURSES_CHAT_H */
