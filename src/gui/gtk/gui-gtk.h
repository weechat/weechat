/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __WEECHAT_GUI_GTK_H
#define __WEECHAT_GUI_GTK_H 1

#include <gtk/gtk.h>

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

#define GUI_GTK(window) ((t_gui_gtk_objects *)(window->gui_objects))

typedef struct t_gui_panel_window t_gui_panel_window;

struct t_gui_panel_window
{
    t_gui_panel *panel;             /* pointer to panel                     */
    int x, y;                       /* position of window                   */
    int width, height;              /* window size                          */
};

typedef struct t_gui_gtk_objects t_gui_gtk_objects;

struct t_gui_gtk_objects
{
    GtkWidget *textview_chat;       /* textview widget for chat             */
    GtkTextBuffer *textbuffer_chat; /* textbuffer widget for chat           */
    GtkTextTag *texttag_chat;       /* texttag widget for chat              */
    GtkWidget *textview_nicklist;   /* textview widget for nicklist         */
    GtkTextBuffer *textbuffer_nicklist; /* textbuffer widget for nicklist   */
    t_gui_panel_window *panel_windows;  /* panel windows                    */
};

extern t_gui_color gui_weechat_colors[];
extern int gui_irc_colors[GUI_NUM_IRC_COLORS][2];

extern GtkWidget *gtk_main_window;
extern GtkWidget *vbox1;
extern GtkWidget *entry_topic;
extern GtkWidget *notebook1;
extern GtkWidget *vbox2;
extern GtkWidget *hbox1;
extern GtkWidget *hpaned1;
extern GtkWidget *scrolledwindow_chat;
extern GtkWidget *scrolledwindow_nick;
extern GtkWidget *entry_input;
extern GtkWidget *label1;

/* color functions */
extern int gui_color_get_pair (int);
extern void gui_color_init ();

/* chat functions */
extern void gui_chat_calculate_line_diff (t_gui_window *, t_gui_line **, int *, int);

/* keyboard functions */
extern void gui_keyboard_default_bindings ();
extern void gui_keyboard_read ();

/* window functions */
extern void gui_window_set_title ();
extern void gui_window_reset_title ();

#endif /* gui-gtk.h */
