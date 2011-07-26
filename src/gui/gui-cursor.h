/*
 * Copyright (C) 2011 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_GUI_CURSOR_H
#define __WEECHAT_GUI_CURSOR_H 1

/* cursor structures */

struct t_gui_cursor_info
{
    int x, y;                          /* (x,y) on screen                   */
    struct t_gui_window *window;       /* window found                      */
    int chat;                          /* 1 for chat area, otherwise 0      */
    struct t_gui_bar_window *bar_window; /* bar window found                */
    char *bar_item;                    /* bar item found                    */
    int item_line;                     /* line in bar item                  */
    int item_col;                      /* column in bar item                */
};

/* cursor variables */

extern int gui_cursor_mode;
extern int gui_cursor_debug;
extern int gui_cursor_x;
extern int gui_cursor_y;

/* cursor functions */

extern void gui_cursor_mode_toggle ();
extern void gui_cursor_debug_toggle ();
extern void gui_cursor_get_info (int x, int y,
                                 struct t_gui_cursor_info *cursor_info);
extern void gui_cursor_move_xy (int x, int y);
extern void gui_cursor_move_add_xy (int add_x, int add_y);
extern void gui_cursor_move_area_add_xy (int add_x, int add_y);
extern void gui_cursor_move_area (const char *area);

#endif /* __WEECHAT_GUI_CURSOR_H */
