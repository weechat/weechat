/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* window structures */

typedef struct t_gui_window_tree t_gui_window_tree;
typedef struct t_gui_window t_gui_window;

struct t_gui_window
{
    /* global position & size */
    int win_x, win_y;               /* position of window                   */
    int win_width, win_height;      /* window geometry                      */
    int win_width_pct;              /* % of width (compared to term size)   */
    int win_height_pct;             /* % of height (compared to term size)  */
    
    int new_x, new_y;               /* used for computing new position      */
    int new_width, new_height;      /* used for computing new size          */
    
    /* chat window settings */
    int win_chat_x, win_chat_y;     /* chat window position                 */
    int win_chat_width;             /* width of chat window                 */
    int win_chat_height;            /* height of chat window                */
    int win_chat_cursor_x;          /* position of cursor in chat window    */
    int win_chat_cursor_y;          /* position of cursor in chat window    */
    
    /* nicklist window settings */
    int win_nick_x, win_nick_y;     /* nick window position                 */
    int win_nick_width;             /* width of nick window                 */
    int win_nick_height;            /* height of nick window                */
    int win_nick_num_max;           /* maximum number of nicks displayed    */
    int win_nick_start;             /* # of 1st nick for display (scroll)   */
    
    /* title window settings */
    int win_title_x;                /* title window position                */
    int win_title_y;                /* title window position                */
    int win_title_width;            /* width of title window                */
    int win_title_height;           /* height of title window               */
    int win_title_start;            /* first char of title for display      */
    
    /* status bar settings */
    int win_status_x;               /* status window position               */
    int win_status_y;               /* status window position               */
    int win_status_width;           /* width of status window               */
    int win_status_height;          /* height of status window              */
    
    /* infobar bar settings */
    int win_infobar_x;              /* infobar window position              */
    int win_infobar_y;              /* infobar window position              */
    int win_infobar_width;          /* width of infobar window              */
    int win_infobar_height;         /* height of infobar window             */
    
    /* input window settings */
    int win_input_x;                /* input window position                */
    int win_input_y;                /* input window position                */
    int win_input_width;            /* width of input window                */
    int win_input_height;           /* height of input window               */
    int win_input_cursor_x;         /* position of cursor in input window   */
    
    /* GUI specific objects */
    void *gui_objects;              /* pointer to a GUI specific struct     */
    
    int current_style_fg;;          /* current color used for foreground    */
    int current_style_bg;;          /* current color used for background    */
    int current_style_attr;         /* current attributes (bold, ..)        */
    int current_color_attr;         /* attr sum of last color(s) displayed  */
    
    /* DCC */
    void *dcc_first;                /* first dcc displayed                  */
    void *dcc_selected;             /* selected dcc                         */
    void *dcc_last_displayed;       /* last dcc displayed (for scroll)      */
    
    t_gui_buffer *buffer;           /* buffer currently displayed in window */
    
    int first_line_displayed;       /* = 1 if first line is displayed       */
    t_gui_line *start_line;         /* pointer to line if scrolling         */
    int start_line_pos;             /* position in first line displayed     */
    int scroll;                     /* = 1 if "MORE" should be displayed    */
    t_gui_window_tree *ptr_tree;    /* pointer to leaf in windows tree      */
    
    t_gui_window *prev_window;      /* link to previous window              */
    t_gui_window *next_window;      /* link to next window                  */
};

struct t_gui_window_tree
{
    t_gui_window_tree *parent_node; /* pointer to parent node               */
    
    /* node info */
    int split_horiz;                /* 1 if horizontal, 0 if vertical       */
    int split_pct;                  /* % of split size (represents child1)  */
    t_gui_window_tree *child1;      /* first child, NULL if a leaf          */
    t_gui_window_tree *child2;      /* second child, NULL if a leaf         */
    
    /* leaf info */
    t_gui_window *window;           /* pointer to window, NULL if a node    */
};

/* window variables */

extern t_gui_window *gui_windows;
extern t_gui_window *last_gui_window;
extern t_gui_window *gui_current_window;
extern t_gui_window_tree *gui_windows_tree;

#endif /* gui-window.h */
