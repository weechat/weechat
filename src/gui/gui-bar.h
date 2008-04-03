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


#ifndef __WEECHAT_GUI_BAR_H
#define __WEECHAT_GUI_BAR_H 1

struct t_weechat_plugin;
struct t_gui_window;

enum t_gui_bar_type
{
    GUI_BAR_TYPE_ROOT = 0,
    GUI_BAR_TYPE_WINDOW,
    GUI_BAR_TYPE_WINDOW_ACTIVE,
    GUI_BAR_TYPE_WINDOW_INACTIVE,
    /* number of bar types */
    GUI_BAR_NUM_TYPES,
};

enum t_gui_bar_position
{
    GUI_BAR_POSITION_BOTTOM = 0,
    GUI_BAR_POSITION_TOP,
    GUI_BAR_POSITION_LEFT,
    GUI_BAR_POSITION_RIGHT,
    /* number of bar positions */
    GUI_BAR_NUM_POSITIONS,
};

struct t_gui_bar
{
    /* user choices */
    struct t_weechat_plugin *plugin;   /* plugin                            */
    int number;                        /* bar number                        */
    char *name;                        /* bar name                          */
    int type;                          /* type (root or window)             */
    enum t_gui_bar_position position;  /* bottom, top, left, right          */
    int size;                          /* size of bar (in chars, 0 = auto)  */
    int separator;                     /* 1 if separator (line) displayed   */
    char *items;                       /* bar items                         */
    
    /* internal vars */
    int current_size;                  /* current bar size (strictly > 0)   */
    int items_count;                   /* number of bar items               */
    char **items_array;                /* exploded bar items                */
    struct t_gui_bar_window *bar_window; /* pointer to bar window           */
                                       /* (for type root only)              */
    struct t_gui_bar *prev_bar;        /* link to previous bar              */
    struct t_gui_bar *next_bar;        /* link to next bar                  */
};

/* variables */

extern char *gui_bar_type_str[];
extern char *gui_bar_position_str[];
extern struct t_gui_bar *gui_bars;
extern struct t_gui_bar *last_gui_bar;

/* functions */

extern int gui_bar_get_type (char *type);
extern int gui_bar_get_position (char *position);
extern int gui_bar_root_get_size (struct t_gui_bar *bar,
                                  enum t_gui_bar_position position);
extern struct t_gui_bar *gui_bar_search (char *name);
extern struct t_gui_bar *gui_bar_new (struct t_weechat_plugin *plugin,
                                      char *name, char *type, char *position,
                                      int size, int separator, char *items);
extern void gui_bar_set (struct t_gui_bar *bar, char *property, char *value);
extern void gui_bar_update (char *name);
extern void gui_bar_free (struct t_gui_bar *bar);
extern void gui_bar_free_all ();
extern void gui_bar_free_all_plugin (struct t_weechat_plugin *plugin);
extern void gui_bar_print_log ();

/* functions (GUI dependent) */

extern int gui_bar_window_get_size (struct t_gui_bar *bar,
                                    struct t_gui_window *window,
                                    enum t_gui_bar_position position);
extern int gui_bar_check_size_add (struct t_gui_bar *bar, int add_size);
extern int gui_bar_window_new (struct t_gui_bar *bar,
                               struct t_gui_window *window);
extern void gui_bar_window_free (struct t_gui_bar_window *bar_window,
                                 struct t_gui_window *window);
extern void gui_bar_free_bar_windows (struct t_gui_bar *bar);
extern void gui_bar_draw (struct t_gui_bar *bar);
extern void gui_bar_window_print_log (struct t_gui_bar_window *bar_window);

#endif /* gui-bar.h */
