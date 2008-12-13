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


#ifndef __WEECHAT_GUI_BAR_WINDOW_H
#define __WEECHAT_GUI_BAR_WINDOW_H 1

struct t_gui_window;
enum t_gui_bar_position position;

struct t_gui_bar_window
{
    struct t_gui_bar *bar;          /* pointer to bar                       */
    int x, y;                       /* position of window                   */
    int width, height;              /* window size                          */
    int scroll_x, scroll_y;         /* X-Y scroll in bar                    */
    int cursor_x, cursor_y;         /* use to move cursor on screen (for    */
                                    /* input_text item)                     */
    int current_size;               /* current size (width or height)       */
    int items_count;                /* number of items                      */
    char **items_content;           /* content for each item of bar         */
    void *gui_objects;              /* pointer to a GUI specific struct     */
    struct t_gui_bar_window *prev_bar_window; /* link to previous bar win   */
                                              /* (only for non-root bars)   */
    struct t_gui_bar_window *next_bar_window; /* link to next bar win       */
                                              /* (only for non-root bars)   */
};

/* functions */

extern int gui_bar_window_valid (struct t_gui_bar_window *bar_window);
extern void gui_bar_window_calculate_pos_size (struct t_gui_bar_window *bar_window,
                                               struct t_gui_window *window);
extern void gui_bar_window_content_build_item (struct t_gui_bar_window *bar_window,
                                               struct t_gui_window *window,
                                               int item_index);
extern struct t_gui_bar_window *gui_bar_window_search_bar (struct t_gui_window *window,
                                                           struct t_gui_bar *bar);
extern int gui_bar_window_get_current_size (struct t_gui_bar_window *bar_window);
extern void gui_bar_window_set_current_size (struct t_gui_bar *bar, int size);
extern int gui_bar_window_get_size (struct t_gui_bar *bar,
                                    struct t_gui_window *window,
                                    enum t_gui_bar_position position);
extern int gui_bar_window_new (struct t_gui_bar *bar,
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

#endif /* gui-bar-window.h */
