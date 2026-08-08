/*
 * SPDX-FileCopyrightText: 2011-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_CURSOR_H
#define WEECHAT_GUI_CURSOR_H

/* cursor variables */

extern int gui_cursor_mode;
extern int gui_cursor_debug;
extern int gui_cursor_x;
extern int gui_cursor_y;

/* cursor functions */

extern void gui_cursor_mode_toggle (void);
extern void gui_cursor_mode_stop (void);
extern void gui_cursor_debug_set (int debug);
extern void gui_cursor_move_xy (int x, int y);
extern void gui_cursor_move_add_xy (int add_x, int add_y);
extern void gui_cursor_move_position (const char *position);
extern void gui_cursor_move_area_add_xy (int add_x, int add_y);
extern int gui_cursor_move_area (const char *area, const char *position);

#endif /* WEECHAT_GUI_CURSOR_H */
