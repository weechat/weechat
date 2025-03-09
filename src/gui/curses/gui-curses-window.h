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

#ifndef WEECHAT_GUI_CURSES_WINDOW_H
#define WEECHAT_GUI_CURSES_WINDOW_H

#define GUI_WINDOW_OBJECTS(window)                                      \
    ((struct t_gui_window_curses_objects *)(window->gui_objects))

struct t_gui_window_saved_style
{
    int style_fg;
    int style_bg;
    int color_attr;
    int emphasis;
    attr_t attrs;
    short pair;
};

struct t_gui_window_curses_objects
{
    WINDOW *win_chat;               /* chat window (example: channel)       */
    WINDOW *win_separator_horiz;    /* horizontal separator (optional)      */
    WINDOW *win_separator_vertic;   /* vertical separator (optional)        */
};

extern int gui_window_current_color_attr;
extern int gui_window_current_emphasis;

extern void gui_window_read_terminal_size (void);
extern void gui_window_clear (WINDOW *window, int fg, int bg);
extern void gui_window_clrtoeol (WINDOW *window);
extern void gui_window_save_style (WINDOW *window);
extern void gui_window_restore_style (WINDOW *window);
extern void gui_window_reset_style (WINDOW *window, int num_color);
extern void gui_window_reset_color (WINDOW *window, int num_color);
extern void gui_window_set_color_style (WINDOW *window, int style);
extern void gui_window_remove_color_style (WINDOW *window, int style);
extern void gui_window_set_color (WINDOW *window, int fg, int bg);
extern void gui_window_set_weechat_color (WINDOW *window, int num_color);
extern void gui_window_set_custom_color_fg_bg (WINDOW *window, int fg, int bg,
                                               int reset_attributes);
extern void gui_window_set_custom_color_pair (WINDOW *window, int pair);
extern void gui_window_set_custom_color_fg (WINDOW *window, int fg);
extern void gui_window_set_custom_color_bg (WINDOW *window, int bg);
extern void gui_window_toggle_emphasis (void);
extern void gui_window_emphasize (WINDOW *window, int x, int y, int count);
extern void gui_window_string_apply_color_fg (unsigned char **str,
                                              WINDOW *window);
extern void gui_window_string_apply_color_bg (unsigned char **str,
                                              WINDOW *window);
extern void gui_window_string_apply_color_fg_bg (unsigned char **str,
                                                 WINDOW *window);
extern void gui_window_string_apply_color_pair (unsigned char **str,
                                                WINDOW *window);
extern void gui_window_string_apply_color_weechat (unsigned char **str,
                                                   WINDOW *window);
extern void gui_window_string_apply_color_set_attr (unsigned char **str,
                                                    WINDOW *window);
extern void gui_window_string_apply_color_remove_attr (unsigned char **str,
                                                       WINDOW *window);
extern void gui_window_hline (WINDOW *window, int x, int y, int width,
                              const char *string);
extern void gui_window_vline (WINDOW *window, int x, int y, int height,
                              const char *string);
extern void gui_window_set_title (const char *title);

#endif /* WEECHAT_GUI_CURSES_WINDOW_H */
