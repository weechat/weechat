/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_GUI_GTK_H
#define __WEECHAT_GUI_GTK_H 1

#include <gtk/gtk.h>

struct t_gui_window;
struct t_gui_buffer;
struct t_gui_line;

/* TODO: remove these temporary defines */

#define A_BOLD      1
#define A_UNDERLINE 2
#define A_REVERSE   4

#define COLOR_BLACK   0
#define COLOR_BLUE    1
#define COLOR_GREEN   2
#define COLOR_CYAN    3
#define COLOR_RED     4
#define COLOR_MAGENTA 5
#define COLOR_YELLOW  6
#define COLOR_WHITE   7

/*
 * shift ncurses colors for compatibility with colors
 * in IRC messages (same as other IRC clients)
 */

#define WEECHAT_COLOR_BLACK   COLOR_BLACK
#define WEECHAT_COLOR_RED     COLOR_BLUE
#define WEECHAT_COLOR_GREEN   COLOR_GREEN
#define WEECHAT_COLOR_YELLOW  COLOR_CYAN
#define WEECHAT_COLOR_BLUE    COLOR_RED
#define WEECHAT_COLOR_MAGENTA COLOR_MAGENTA
#define WEECHAT_COLOR_CYAN    COLOR_YELLOW
#define WEECHAT_COLOR_WHITE   COLOR_WHITE

#define GUI_WINDOW_OBJECTS(window)                                      \
    ((struct t_gui_window_gtk_objects *)(window->gui_objects))
#define GUI_BAR_WINDOW_OBJECTS(bar_window)                              \
    ((struct t_gui_bar_window_gtk_objects *)(bar_window->gui_objects))

struct t_gui_window_gtk_objects
{
    GtkWidget *textview_chat;       /* textview widget for chat             */
    GtkTextBuffer *textbuffer_chat; /* textbuffer widget for chat           */
    GtkTextTag *texttag_chat;       /* texttag widget for chat              */
    struct t_gui_bar_window *bar_windows;     /* bar windows                */
    struct t_gui_bar_window *last_bar_window; /* last bar window            */
    int current_style_fg;           /* current foreground color             */
    int current_style_bg;           /* current background color             */
    int current_style_attr;         /* current attributes (bold, ..)        */
    int current_color_attr;         /* attr sum of last color(s) used       */
};

struct t_gui_bar_window_gtk_objects
{
};

extern GtkWidget *gui_gtk_main_window;
extern GtkWidget *gui_gtk_vbox1;
extern GtkWidget *gui_gtk_notebook1;
extern GtkWidget *gui_gtk_vbox2;
extern GtkWidget *gui_gtk_hbox1;
extern GtkWidget *gui_gtk_hpaned1;
extern GtkWidget *gui_gtk_scrolledwindow_chat;
extern GtkWidget *gui_gtk_entry_input;
extern GtkWidget *gui_gtk_label1;

/* color functions */
extern int gui_color_get_pair (int fg, int bg);
extern void gui_color_pre_init ();
extern void gui_color_init ();
extern void gui_color_end ();

/* chat functions */
extern void gui_chat_calculate_line_diff (struct t_gui_window *window,
                                          struct t_gui_line **line,
                                          int *line_pos, int difference);

/* key functions */
extern void gui_key_default_bindings (int context);
extern void gui_key_read ();

/* window functions */
extern void gui_window_redraw_buffer (struct t_gui_buffer *buffer);
extern void gui_window_set_title (const char *title);

#endif /* __WEECHAT_GUI_GTK_H */
