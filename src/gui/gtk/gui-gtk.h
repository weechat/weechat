/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_GUI_GTK_H
#define __WEECHAT_GUI_GTK_H 1

#include <gtk/gtk.h>

struct t_gui_window;
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

/* shift ncurses colors for compatibility with colors
   in IRC messages (same as other IRC clients) */

#define WEECHAT_COLOR_BLACK   COLOR_BLACK
#define WEECHAT_COLOR_RED     COLOR_BLUE
#define WEECHAT_COLOR_GREEN   COLOR_GREEN
#define WEECHAT_COLOR_YELLOW  COLOR_CYAN
#define WEECHAT_COLOR_BLUE    COLOR_RED
#define WEECHAT_COLOR_MAGENTA COLOR_MAGENTA
#define WEECHAT_COLOR_CYAN    COLOR_YELLOW
#define WEECHAT_COLOR_WHITE   COLOR_WHITE

#define GUI_GTK(window) ((struct t_gui_gtk_objects *)(window->gui_objects))

struct t_gui_bar_window
{
    struct t_gui_bar *bar;          /* pointer to bar                       */
    int x, y;                       /* position of window                   */
    int width, height;              /* window size                          */
    struct t_gui_bar_window *next_bar_window;
                                    /* link to next bar window              */
                                    /* (only used if bar is in windows)     */
};

struct t_gui_gtk_objects
{
    GtkWidget *textview_chat;       /* textview widget for chat             */
    GtkTextBuffer *textbuffer_chat; /* textbuffer widget for chat           */
    GtkTextTag *texttag_chat;       /* texttag widget for chat              */
    GtkWidget *textview_nicklist;   /* textview widget for nicklist         */
    GtkTextBuffer *textbuffer_nicklist; /* textbuffer widget for nicklist   */
    struct t_gui_bar_window *bar_windows; /* bar windows                    */
};

//extern t_gui_color gui_weechat_colors[];
//extern int gui_irc_colors[GUI_NUM_IRC_COLORS][2];

extern GtkWidget *gui_gtk_main_window;
extern GtkWidget *gui_gtk_vbox1;
extern GtkWidget *gui_gtk_entry_topic;
extern GtkWidget *gui_gtk_notebook1;
extern GtkWidget *gui_gtk_vbox2;
extern GtkWidget *gui_gtk_hbox1;
extern GtkWidget *gui_gtk_hpaned1;
extern GtkWidget *gui_gtk_scrolledwindow_chat;
extern GtkWidget *gui_gtk_scrolledwindow_nick;
extern GtkWidget *gui_gtk_entry_input;
extern GtkWidget *gui_gtk_label1;

/* color functions */
extern int gui_color_get_pair (int num_color);
extern void gui_color_init ();

/* chat functions */
extern void gui_chat_calculate_line_diff (struct t_gui_window *window,
                                          struct t_gui_line **line,
                                          int *line_pos, int difference);

/* keyboard functions */
extern void gui_keyboard_default_bindings ();
extern void gui_keyboard_read ();
extern void gui_keyboard_flush ();

/* window functions */
extern void gui_window_title_set ();
extern void gui_window_title_reset ();

#endif /* gui-gtk.h */
