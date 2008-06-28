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

enum t_gui_bar_option
{
    GUI_BAR_OPTION_HIDDEN = 0,
    GUI_BAR_OPTION_PRIORITY,
    GUI_BAR_OPTION_TYPE,
    GUI_BAR_OPTION_CONDITIONS,
    GUI_BAR_OPTION_POSITION,
    GUI_BAR_OPTION_FILLING,
    GUI_BAR_OPTION_SIZE,
    GUI_BAR_OPTION_SIZE_MAX,
    GUI_BAR_OPTION_COLOR_FG,
    GUI_BAR_OPTION_COLOR_DELIM,
    GUI_BAR_OPTION_COLOR_BG,
    GUI_BAR_OPTION_SEPARATOR,
    GUI_BAR_OPTION_ITEMS,
    /* number of bar types */
    GUI_BAR_NUM_OPTIONS,
};

enum t_gui_bar_type
{
    GUI_BAR_TYPE_ROOT = 0,
    GUI_BAR_TYPE_WINDOW,
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

enum t_gui_bar_filling
{
    GUI_BAR_FILLING_HORIZONTAL = 0,
    GUI_BAR_FILLING_VERTICAL,
    /* number of bar positions */
    GUI_BAR_NUM_FILLING,
};

struct t_gui_bar
{
    /* user choices */
    struct t_weechat_plugin *plugin;    /* plugin                           */
    char *name;                         /* bar name                         */
    struct t_config_option *hidden;     /* true if bar is hidden            */
    struct t_config_option *priority;   /* bar priority                     */
    struct t_config_option *type;       /* type (root or window)            */
    struct t_config_option *conditions; /* conditions for display           */
    struct t_config_option *position;   /* bottom, top, left, right         */
    struct t_config_option *filling;    /* filling (H=horizontal,V=vertical)*/
    struct t_config_option *size;       /* size of bar (in chars, 0 = auto) */
    struct t_config_option *size_max;   /* max size of bar (0 = no limit)   */
    struct t_config_option *color_fg;   /* default text color for bar       */
    struct t_config_option *color_delim;/* default delimiter color for bar  */
    struct t_config_option *color_bg;   /* default background color for bar */
    struct t_config_option *separator;  /* true if separator line displayed */
    struct t_config_option *items;      /* bar items                        */
    
    /* internal vars */
    int conditions_count;              /* number of conditions              */
    char **conditions_array;           /* exploded bar conditions           */
    int items_count;                   /* number of bar items               */
    char **items_array;                /* exploded bar items                */
    struct t_gui_bar_window *bar_window; /* pointer to bar window           */
                                       /* (for type root only)              */
    struct t_gui_bar *prev_bar;        /* link to previous bar              */
    struct t_gui_bar *next_bar;        /* link to next bar                  */
};

/* variables */

extern char *gui_bar_type_string[];
extern char *gui_bar_position_string[];
extern char *gui_bar_filling_string[];
extern struct t_gui_bar *gui_bars;
extern struct t_gui_bar *last_gui_bar;
extern struct t_gui_bar *gui_temp_bars;
extern struct t_gui_bar *last_gui_temp_bar;

/* functions */

extern int gui_bar_search_option (const char *option_name);
extern int gui_bar_search_type (const char *type);
extern int gui_bar_search_position (const char *position);
extern int gui_bar_check_conditions_for_window (struct t_gui_bar *bar,
                                                struct t_gui_window *window);
extern int gui_bar_root_get_size (struct t_gui_bar *bar,
                                  enum t_gui_bar_position position);
extern struct t_gui_bar *gui_bar_search (const char *name);
extern int gui_bar_set (struct t_gui_bar *bar, const char *property, const char *value);
extern void gui_bar_create_option_temp (struct t_gui_bar *temp_bar,
                                        int index_option, const char *value);
extern struct t_gui_bar *gui_bar_alloc (const char *name);
extern struct t_gui_bar *gui_bar_new (struct t_weechat_plugin *plugin,
                                      const char *name, const char *hidden,
                                      const char *priority, const char *type,
                                      const char *conditions,
                                      const char *position,
                                      const char *filling, const char *size,
                                      const char *size_max,
                                      const char *color_fg,
                                      const char *color_delim,
                                      const char *color_bg,
                                      const char *separator,
                                      const char *items);
extern void gui_bar_use_temp_bars ();
extern void gui_bar_create_default ();
extern void gui_bar_update (const char *name);
extern void gui_bar_free (struct t_gui_bar *bar);
extern void gui_bar_free_all ();
extern void gui_bar_free_all_plugin (struct t_weechat_plugin *plugin);
extern void gui_bar_print_log ();

/* functions (GUI dependent) */

extern struct t_gui_bar_window *gui_bar_window_search_bar (struct t_gui_window *window,
                                                           struct t_gui_bar *bar);
extern int gui_bar_window_get_current_size (struct t_gui_bar_window *bar_window);
extern void gui_bar_window_set_current_size (struct t_gui_bar *bar, int size);
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
