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


#ifndef __WEECHAT_GUI_WINDOW_H
#define __WEECHAT_GUI_WINDOW_H 1

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
    
    int new_x, new_y;                  /* used for computing new position   */
    int new_width, new_height;         /* used for computing new size       */
    
    /* chat window settings */
    int win_chat_x, win_chat_y;        /* chat window position              */
    int win_chat_width;                /* width of chat window              */
    int win_chat_height;               /* height of chat window             */
    int win_chat_cursor_x;             /* position of cursor in chat window */
    int win_chat_cursor_y;             /* position of cursor in chat window */
    
    /* nicklist window settings */
    int win_nick_x, win_nick_y;        /* nick window position              */
    int win_nick_width;                /* width of nick window              */
    int win_nick_height;               /* height of nick window             */
    int win_nick_num_max;              /* maximum number of nicks displayed */
    int win_nick_start;                /* # of 1st nick for display (scroll)*/
    
    /* title window settings */
    int win_title_x;                   /* title window position             */
    int win_title_y;                   /* title window position             */
    int win_title_width;               /* width of title window             */
    int win_title_height;              /* height of title window            */
    int win_title_start;               /* first char of title for display   */
    
    /* status bar settings */
    int win_status_x;                  /* status window position            */
    int win_status_y;                  /* status window position            */
    int win_status_width;              /* width of status window            */
    int win_status_height;             /* height of status window           */
    
    /* infobar bar settings */
    int win_infobar_x;                 /* infobar window position           */
    int win_infobar_y;                 /* infobar window position           */
    int win_infobar_width;             /* width of infobar window           */
    int win_infobar_height;            /* height of infobar window          */
    
    /* input window settings */
    int win_input_x;                   /* input window position             */
    int win_input_y;                   /* input window position             */
    int win_input_width;               /* width of input window             */
    int win_input_height;              /* height of input window            */
    int win_input_cursor_x;            /* position of cursor in input win   */
    
    /* refresh */
    int refresh_needed;                /* 1 if refresh needed for window    */
    
    /* GUI specific objects */
    void *gui_objects;                 /* pointer to a GUI specific struct  */
    
    struct t_gui_buffer *buffer;       /* buffer currently displayed        */
    
    int first_line_displayed;          /* = 1 if first line is displayed    */
    struct t_gui_line *start_line;     /* pointer to line if scrolling      */
    int start_line_pos;                /* position in first line displayed  */
    int scroll;                        /* = 1 if "MORE" should be displayed */
    struct t_gui_window_tree *ptr_tree;/* pointer to leaf in windows tree   */
    
    struct t_gui_window *prev_window;  /* link to previous window           */
    struct t_gui_window *next_window;  /* link to next window               */
};

struct t_gui_window_tree
{
    struct t_gui_window_tree *parent_node; /* pointer to parent node        */
    
    /* node info */
    int split_horiz;                   /* 1 if horizontal, 0 if vertical    */
    int split_pct;                     /* % of split size (child1)          */
    struct t_gui_window_tree *child1;  /* first child, NULL if a leaf     */
    struct t_gui_window_tree *child2;  /* second child, NULL if a leaf    */
    
    /* leaf info */
    struct t_gui_window *window;       /* pointer to window, NULL if a node */
};

/* window variables */

extern struct t_gui_window *gui_windows;
extern struct t_gui_window *last_gui_window;
extern struct t_gui_window *gui_current_window;
extern struct t_gui_window_tree *gui_windows_tree;

/* window functions */
extern int gui_window_tree_init (struct t_gui_window *window);
extern void gui_window_tree_node_to_leaf (struct t_gui_window_tree *node,
                                          struct t_gui_window *window);
extern void gui_window_tree_free (struct t_gui_window_tree **tree);
extern struct t_gui_window *gui_window_new (struct t_gui_window *parent,
                                            int x, int y, int width, int height,
                                            int width_pct, int height_pct);
extern int gui_window_valid (struct t_gui_window *window);
extern void gui_window_free (struct t_gui_window *window);
extern struct t_gui_window *gui_window_search_by_buffer (struct t_gui_buffer *buffer);
extern void gui_window_switch_server (struct t_gui_window *window);
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
extern void gui_window_print_log ();

/* window functions (GUI dependent) */

extern int gui_window_get_width ();
extern int gui_window_get_height ();
extern int gui_window_objects_init (struct t_gui_window *window);
extern void gui_window_objects_free (struct t_gui_window *window,
                                     int free_separator,
                                     int free_bar_windows);
extern int gui_window_calculate_pos_size (struct t_gui_window *window,
                                          int force_calculate);
extern void gui_window_redraw_buffer (struct t_gui_buffer *buffer);
extern void gui_window_redraw_all_buffers ();
extern void gui_window_switch_to_buffer (struct t_gui_window *window,
                                         struct t_gui_buffer *buffer);
extern void gui_window_switch (struct t_gui_window *window);
extern void gui_window_page_up (struct t_gui_window *window);
extern void gui_window_page_down (struct t_gui_window *window);
extern void gui_window_scroll_up (struct t_gui_window *window);
extern void gui_window_scroll_down (struct t_gui_window *window);
extern void gui_window_scroll_top (struct t_gui_window *window);
extern void gui_window_scroll_bottom (struct t_gui_window *window);
extern void gui_window_scroll_topic_left (struct t_gui_window *window);
extern void gui_window_scroll_topic_right (struct t_gui_window *window);
extern void gui_window_nicklist_page_up (struct t_gui_window *window);
extern void gui_window_nicklist_page_down (struct t_gui_window *window);
extern void gui_window_nicklist_beginning (struct t_gui_window *window);
extern void gui_window_nicklist_end (struct t_gui_window *window);
extern void gui_window_init_subwindows (struct t_gui_window *window);
extern void gui_window_refresh_windows ();
extern void gui_window_split_horiz (struct t_gui_window *window,
                                    int pourcentage);
extern void gui_window_split_vertic (struct t_gui_window *window,
                                     int pourcentage);
extern void gui_window_resize (struct t_gui_window *window, int pourcentage);
extern int gui_window_merge (struct t_gui_window *window);
extern void gui_window_merge_all (struct t_gui_window *window);
extern void gui_window_switch_up (struct t_gui_window *window);
extern void gui_window_switch_down (struct t_gui_window *window);
extern void gui_window_switch_left (struct t_gui_window *window);
extern void gui_window_switch_right (struct t_gui_window *window);
extern void gui_window_refresh_screen ();
extern void gui_window_title_set ();
extern void gui_window_title_reset ();
extern void gui_window_objects_print_log (struct t_gui_window *window);

#endif /* gui-window.h */
