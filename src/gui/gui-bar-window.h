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

#ifndef WEECHAT_GUI_BAR_WINDOW_H
#define WEECHAT_GUI_BAR_WINDOW_H

#include "gui-bar.h"

struct t_infolist;
struct t_gui_buffer;
struct t_gui_window;

struct t_gui_bar_window_coords
{
    int item;                       /* index of item                        */
    int subitem;                    /* index of sub item                    */
    int line;                       /* line                                 */
    int x;                          /* X on screen                          */
    int y;                          /* Y on screen                          */
};

struct t_gui_bar_window
{
    struct t_gui_bar *bar;          /* pointer to bar                       */
    int x, y;                       /* position of window                   */
    int width, height;              /* window size                          */
    int scroll_x, scroll_y;         /* X-Y scroll in bar                    */
    int cursor_x, cursor_y;         /* use to move cursor on screen (for    */
                                    /* input_text item)                     */
    int current_size;               /* current size (width or height)       */
    int items_count;                /* number of bar items                  */
    int *items_subcount;            /* number of sub items                  */
    char ***items_content;          /* content for each (sub)item of bar    */
    int **items_num_lines;          /* number of lines for each (sub)item   */
    int **items_refresh_needed;     /* refresh needed for (sub)item?        */
    int screen_col_size;            /* size of columns on screen            */
                                    /* (for filling with columns)           */
    int screen_lines;               /* number of lines on screen            */
                                    /* (for filling with columns)           */
    int coords_count;               /* number of coords saved               */
    struct t_gui_bar_window_coords **coords; /* coords for filling horiz.   */
                                    /* (size is 5 * coords_count)           */
    void *gui_objects;              /* pointer to a GUI specific struct     */
    struct t_gui_bar_window *prev_bar_window; /* link to previous bar win   */
                                              /* (only for non-root bars)   */
    struct t_gui_bar_window *next_bar_window; /* link to next bar win       */
                                              /* (only for non-root bars)   */
};

/* functions */

extern int gui_bar_window_valid (struct t_gui_bar_window *bar_window);
extern void gui_bar_window_search_by_xy (struct t_gui_window *window,
                                         int x, int y,
                                         struct t_gui_bar_window **bar_window,
                                         char **bar_item, int *bar_item_line,
                                         int *bar_item_col,
                                         struct t_gui_buffer **buffer);
extern void gui_bar_window_calculate_pos_size (struct t_gui_bar_window *bar_window,
                                               struct t_gui_window *window);
extern void gui_bar_window_content_build (struct t_gui_bar_window *bar_window,
                                          struct t_gui_window *window);
extern char *gui_bar_window_content_get_with_filling (struct t_gui_bar_window *bar_window,
                                                      struct t_gui_window *window,
                                                      int *num_spacers);
extern int gui_bar_window_can_use_spacer (struct t_gui_bar_window *bar_window);
extern int *gui_bar_window_compute_spacers_size (int length_on_screen,
                                                 int bar_window_width,
                                                 int num_spacers);
extern struct t_gui_bar_window *gui_bar_window_search_bar (struct t_gui_window *window,
                                                           struct t_gui_bar *bar);
extern int gui_bar_window_get_current_size (struct t_gui_bar_window *bar_window);
extern void gui_bar_window_set_current_size (struct t_gui_bar_window *bar_window,
                                             struct t_gui_window *window,
                                             int size);
extern int gui_bar_window_get_size (struct t_gui_bar *bar,
                                    struct t_gui_window *window,
                                    enum t_gui_bar_position position);
extern void gui_bar_window_coords_add (struct t_gui_bar_window *bar_window,
                                       int index_item, int index_subitem,
                                       int index_line,
                                       int x, int y);
extern void gui_bar_window_coords_free (struct t_gui_bar_window *bar_window);
extern void gui_bar_window_insert (struct t_gui_bar_window *bar_window,
                                   struct t_gui_window *window);
extern void gui_bar_window_new (struct t_gui_bar *bar,
                                struct t_gui_window *window);
extern void gui_bar_window_free (struct t_gui_bar_window *bar_window,
                                 struct t_gui_window *window);
extern int gui_bar_window_remove_unused_bars (struct t_gui_window *window);
extern int gui_bar_window_add_missing_bars (struct t_gui_window *window);
extern void gui_bar_window_scroll (struct t_gui_bar_window *bar_window,
                                   struct t_gui_window *window,
                                   int add_x, int scroll_beginning,
                                   int scroll_end, int add, int percent,
                                   int value);
extern struct t_hdata *gui_bar_window_hdata_bar_window_cb (const void *pointer,
                                                           void *data,
                                                           const char *hdata_name);
extern int gui_bar_window_add_to_infolist (struct t_infolist *infolist,
                                           struct t_gui_bar_window *bar_window);
extern void gui_bar_window_print_log (struct t_gui_bar_window *bar_window);

/* functions (GUI dependent) */

extern int gui_bar_window_objects_init (struct t_gui_bar_window *bar_window);
extern void gui_bar_window_objects_free (struct t_gui_bar_window *bar_window);
extern void gui_bar_window_create_win (struct t_gui_bar_window *bar_window);
extern void gui_bar_window_draw (struct t_gui_bar_window *bar_window,
                                 struct t_gui_window *window);
extern void gui_bar_window_objects_print_log (struct t_gui_bar_window *bar_window);

#endif /* WEECHAT_GUI_BAR_WINDOW_H */
