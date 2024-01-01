/*
 * Copyright (C) 2011-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_GUI_CURSOR_H
#define WEECHAT_GUI_CURSOR_H

/* cursor variables */

extern int gui_cursor_mode;
extern int gui_cursor_debug;
extern int gui_cursor_x;
extern int gui_cursor_y;

/* cursor functions */

extern void gui_cursor_mode_toggle ();
extern void gui_cursor_mode_stop ();
extern void gui_cursor_debug_set (int debug);
extern void gui_cursor_move_xy (int x, int y);
extern void gui_cursor_move_add_xy (int add_x, int add_y);
extern void gui_cursor_move_position (const char *position);
extern void gui_cursor_move_area_add_xy (int add_x, int add_y);
extern void gui_cursor_move_area (const char *area, const char *position);

#endif /* WEECHAT_GUI_CURSOR_H */
