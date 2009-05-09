/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_GUI_WINDOW_H
#define __WEECHAT_GUI_WINDOW_H 1

#define GUI_WINDOW_MIN_WIDTH          10
#define GUI_WINDOW_MIN_HEIGHT         5

#define GUI_WINDOW_CHAT_MIN_WIDTH     5
#define GUI_WINDOW_CHAT_MIN_HEIGHT    2

struct t_infolist;

extern int gui_init_ok;
extern int gui_ok;
extern int gui_window_refresh_needed;

/* window structures */

struct t_gui_window
{
    /* global position & size */
    int win_x, win_y;                  /* position of window                */
    int win_width, win_height;         /* window geometry                   */
    int win_width_pct;                 /* % of width (compared to win size) */
    int win_height_pct;                /* % of height (compared to win size)*/
    
    /* chat window settings */
    int win_chat_x, win_chat_y;        /* chat window position              */
    int win_chat_width;                /* width of chat window              */
    int win_chat_height;               /* height of chat window             */
    int win_chat_cursor_x;             /* position of cursor in chat window */
    int win_chat_cursor_y;             /* position of cursor in chat window */
    
    /* bar windows */
    struct t_gui_bar_window *bar_windows;     /* bar windows                */
    struct t_gui_bar_window *last_bar_window; /* last bar window            */
    
    /* refresh */
    int refresh_needed;                /* 1 if refresh needed for window    */
    
    /* GUI specific objects */
    void *gui_objects;                 /* pointer to a GUI specific struct  */
    
    /* buffer and layout infos */
    struct t_gui_buffer *buffer;       /* buffer currently displayed        */
    char *layout_plugin_name;          /* plugin and buffer that should be  */
    char *layout_buffer_name;          /* displayed in this window (even if */
                                       /* buffer does not exist yet, it will*/
                                       /* be assigned later)                */
    
    /* scroll */
    int first_line_displayed;          /* = 1 if first line is displayed    */
    struct t_gui_line *start_line;     /* pointer to line if scrolling      */
    int start_line_pos;                /* position in first line displayed  */
    int scroll;                        /* = 1 if "MORE" should be displayed */
    int scroll_lines_after;            /* number of lines after last line   */
                                       /* displayed (with scrolling)        */
    int scroll_reset_allowed;          /* reset scroll allowed (when using  */
                                       /* keys like page_up/down, end, ..)  */
    struct t_gui_window_tree *ptr_tree;/* pointer to leaf in windows tree   */
    
    struct t_gui_window *prev_window;  /* link to previous window           */
    struct t_gui_window *next_window;  /* link to next window               */
};

struct t_gui_window_tree
{
    struct t_gui_window_tree *parent_node; /* pointer to parent node        */
    
    /* node info */
    int split_pct;                     /* % of split size (child1)          */
    int split_horizontal;              /* 1 if horizontal, 0 if vertical    */
    struct t_gui_window_tree *child1;  /* first child, NULL if a leaf       */
    struct t_gui_window_tree *child2;  /* second child, NULL if a leaf      */
    
    /* leaf info */
    struct t_gui_window *window;       /* pointer to window, NULL if a node */
};

/* window variables */

extern struct t_gui_window *gui_windows;
extern struct t_gui_window *last_gui_window;
extern struct t_gui_window *gui_current_window;
extern struct t_gui_window_tree *gui_windows_tree;

/* window functions */
extern void gui_window_ask_refresh (int refresh);
extern int gui_window_tree_init (struct t_gui_window *window);
extern void gui_window_tree_node_to_leaf (struct t_gui_window_tree *node,
                                          struct t_gui_window *window);
extern void gui_window_tree_free (struct t_gui_window_tree **tree);
extern struct t_gui_window *gui_window_new (struct t_gui_window *parent_window,
                                            struct t_gui_buffer *buffer,
                                            int x, int y, int width, int height,
                                            int width_pct, int height_pct);
extern int gui_window_valid (struct t_gui_window *window);
extern int gui_window_get_integer (struct t_gui_window *window,
                                   const char *property);
extern const char *gui_window_get_string (struct t_gui_window *window,
                                          const char *property);
extern void *gui_window_get_pointer (struct t_gui_window *window,
                                     const char *property);
extern void gui_window_set_layout_plugin_name (struct t_gui_window *window,
                                               const char *plugin_name);
extern void gui_window_set_layout_buffer_name (struct t_gui_window *window,
                                               const char *buffer_name);
extern void gui_window_free (struct t_gui_window *window);
extern void gui_window_switch_previous (struct t_gui_window *window);
extern void gui_window_switch_next (struct t_gui_window *window);
extern void gui_window_switch_by_buffer (struct t_gui_window *window,
                                         int buffer_number);
extern void gui_window_scroll (struct t_gui_window *window, char *scroll);
extern void gui_window_scroll_previous_highlight (struct t_gui_window *window);
extern void gui_window_scroll_next_highlight (struct t_gui_window *window);
extern void gui_window_search_start (struct t_gui_window *window);
extern void gui_window_search_restart (struct t_gui_window *window);
extern void gui_window_search_stop (struct t_gui_window *window);
extern int gui_window_search_text (struct t_gui_window *window);
extern void gui_window_zoom (struct t_gui_window *window);
extern int gui_window_add_to_infolist (struct t_infolist *infolist,
                                       struct t_gui_window *window);
extern void gui_window_print_log ();

/* window functions (GUI dependent) */

extern int gui_window_get_width ();
extern int gui_window_get_height ();
extern int gui_window_objects_init (struct t_gui_window *window);
extern void gui_window_objects_free (struct t_gui_window *window,
                                     int free_separator);
extern void gui_window_calculate_pos_size (struct t_gui_window *window);
extern void gui_window_switch_to_buffer (struct t_gui_window *window,
                                         struct t_gui_buffer *buffer,
                                         int set_last_read);
extern void gui_window_switch (struct t_gui_window *window);
extern void gui_window_page_up (struct t_gui_window *window);
extern void gui_window_page_down (struct t_gui_window *window);
extern void gui_window_scroll_up (struct t_gui_window *window);
extern void gui_window_scroll_down (struct t_gui_window *window);
extern void gui_window_scroll_top (struct t_gui_window *window);
extern void gui_window_scroll_bottom (struct t_gui_window *window);
extern void gui_window_refresh_windows ();
extern struct t_gui_window *gui_window_split_horizontal (struct t_gui_window *window,
                                                         int percentage);
extern struct t_gui_window *gui_window_split_vertical (struct t_gui_window *window,
                                                       int percentage);
extern void gui_window_resize (struct t_gui_window *window, int percentage);
extern int gui_window_merge (struct t_gui_window *window);
extern void gui_window_merge_all (struct t_gui_window *window);
extern void gui_window_switch_up (struct t_gui_window *window);
extern void gui_window_switch_down (struct t_gui_window *window);
extern void gui_window_switch_left (struct t_gui_window *window);
extern void gui_window_switch_right (struct t_gui_window *window);
extern void gui_window_refresh_screen (int full_refresh);
extern void gui_window_set_title (const char *title);
extern void gui_window_objects_print_log (struct t_gui_window *window);

#endif /* gui-window.h */
