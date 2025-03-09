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

#ifndef WEECHAT_GUI_CURSES_COLOR_H
#define WEECHAT_GUI_CURSES_COLOR_H

#define GUI_CURSES_NUM_WEECHAT_COLORS 17

#ifndef A_ITALIC /* A_ITALIC is defined in ncurses >= 5.9 patch 20130831 */
#define A_ITALIC 0
#endif /* A_ITALIC */

#define A_ALL_ATTR A_BLINK | A_DIM | A_BOLD | A_UNDERLINE | A_REVERSE | A_ITALIC

extern struct t_gui_color *gui_weechat_colors;
extern int gui_color_term_colors;
extern int gui_color_num_pairs;
extern int gui_color_pairs_auto_reset;
extern int gui_color_pairs_auto_reset_pending;
extern time_t gui_color_pairs_auto_reset_last;
extern int gui_color_buffer_refresh_needed;

extern int gui_color_get_gui_attrs (int color);
extern int gui_color_get_pair (int fg, int bg);
extern int gui_color_weechat_get_pair (int weechat_color);
extern void gui_color_alloc (void);

#endif /* WEECHAT_GUI_CURSES_COLOR_H */
