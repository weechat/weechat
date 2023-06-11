/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_GUI_BAR_H
#define WEECHAT_GUI_BAR_H

struct t_infolist;
struct t_weechat_plugin;
struct t_gui_window;

enum t_gui_bar_default
{
    GUI_BAR_DEFAULT_INPUT,
    GUI_BAR_DEFAULT_TITLE,
    GUI_BAR_DEFAULT_STATUS,
    GUI_BAR_DEFAULT_NICKLIST,
    /* number of default bars */
    GUI_BAR_NUM_DEFAULT_BARS,
};

enum t_gui_bar_option
{
    GUI_BAR_OPTION_HIDDEN = 0,          /* true if bar is hidden            */
    GUI_BAR_OPTION_PRIORITY,            /* bar priority                     */
    GUI_BAR_OPTION_TYPE,                /* type (root or window)            */
    GUI_BAR_OPTION_CONDITIONS,          /* condition(s) for display         */
    GUI_BAR_OPTION_POSITION,            /* bottom, top, left, right         */
    GUI_BAR_OPTION_FILLING_TOP_BOTTOM,  /* filling when pos. is top/bottom  */
    GUI_BAR_OPTION_FILLING_LEFT_RIGHT,  /* filling when pos. is left/right  */
    GUI_BAR_OPTION_SIZE,                /* size of bar (in chars, 0 = auto) */
    GUI_BAR_OPTION_SIZE_MAX,            /* max size of bar (0 = no limit)   */
    GUI_BAR_OPTION_COLOR_FG,            /* default text color for bar       */
    GUI_BAR_OPTION_COLOR_DELIM,         /* default delimiter color for bar  */
    GUI_BAR_OPTION_COLOR_BG,            /* default background color for bar */
    GUI_BAR_OPTION_COLOR_BG_INACTIVE,   /* bg color for inactive window     */
                                        /* (window bars only)               */
    GUI_BAR_OPTION_SEPARATOR,           /* true if separator line displayed */
    GUI_BAR_OPTION_ITEMS,               /* bar items                        */
    /* number of bar options */
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
    GUI_BAR_FILLING_COLUMNS_HORIZONTAL,
    GUI_BAR_FILLING_COLUMNS_VERTICAL,
    /* number of bar positions */
    GUI_BAR_NUM_FILLING,
};

struct t_gui_bar
{
    /* user choices */
    char *name;                         /* bar name                         */
    struct t_config_option *options[GUI_BAR_NUM_OPTIONS];

    /* internal vars */
    int items_count;                    /* number of bar items              */
    int *items_subcount;                /* number of sub items              */
    char ***items_array;                /* bar items (after split)          */
    char ***items_buffer;               /* buffer name for each (sub)item   */
    char ***items_prefix;               /* prefix for each (sub)item        */
    char ***items_name;                 /* name for each (sub)item          */
    char ***items_suffix;               /* suffix for each (sub)item        */
    struct t_gui_bar_window *bar_window; /* pointer to bar window           */
                                        /* (for type root only)             */
    int bar_refresh_needed;             /* refresh for bar is needed?       */
    struct t_gui_bar *prev_bar;         /* link to previous bar             */
    struct t_gui_bar *next_bar;         /* link to next bar                 */
};

/* variables */

extern char *gui_bar_option_string[];
extern char *gui_bar_type_string[];
extern char *gui_bar_position_string[];
extern char *gui_bar_filling_string[];
extern char *gui_bar_default_name[];
extern char *gui_bar_default_values[][GUI_BAR_NUM_OPTIONS];
extern struct t_gui_bar *gui_bars;
extern struct t_gui_bar *last_gui_bar;
extern struct t_gui_bar *gui_temp_bars;
extern struct t_gui_bar *last_gui_temp_bar;

/* functions */

extern int gui_bar_valid (struct t_gui_bar *bar);
extern int gui_bar_search_default_bar (const char *bar_name);
extern int gui_bar_search_option (const char *option_name);
extern int gui_bar_search_type (const char *type);
extern int gui_bar_search_position (const char *position);
extern enum t_gui_bar_filling gui_bar_get_filling (struct t_gui_bar *bar);
extern int gui_bar_check_conditions (struct t_gui_bar *bar,
                                     struct t_gui_window *window);
extern int gui_bar_root_get_size (struct t_gui_bar *bar,
                                  enum t_gui_bar_position position);
extern struct t_gui_bar *gui_bar_search (const char *name);
extern void gui_bar_ask_refresh (struct t_gui_bar *bar);
extern void gui_bar_draw (struct t_gui_bar *bar);
extern int gui_bar_set (struct t_gui_bar *bar, const char *property, const char *value);
extern void gui_bar_create_option_temp (struct t_gui_bar *temp_bar,
                                        int index_option, const char *value);
extern struct t_gui_bar *gui_bar_alloc (const char *name);
extern struct t_gui_bar *gui_bar_new (const char *name,
                                      const char *hidden,
                                      const char *priority,
                                      const char *type,
                                      const char *conditions,
                                      const char *position,
                                      const char *filling_top_bottom,
                                      const char *filling_left_right,
                                      const char *size,
                                      const char *size_max,
                                      const char *color_fg,
                                      const char *color_delim,
                                      const char *color_bg,
                                      const char *color_bg_inactive,
                                      const char *separator,
                                      const char *items);
extern void gui_bar_use_temp_bars ();
extern void gui_bar_create_default_input ();
extern void gui_bar_create_default_title ();
extern void gui_bar_create_default_status ();
extern void gui_bar_create_default_nicklist ();
extern void gui_bar_create_default ();
extern void gui_bar_update (const char *name);
extern int gui_bar_scroll (struct t_gui_bar *bar, struct t_gui_window *window,
                           const char *scroll);
extern void gui_bar_free (struct t_gui_bar *bar);
extern void gui_bar_free_all ();
extern struct t_hdata *gui_bar_hdata_bar_cb (const void *pointer,
                                             void *data,
                                             const char *hdata_name);
extern int gui_bar_add_to_infolist (struct t_infolist *infolist,
                                    struct t_gui_bar *bar);
extern void gui_bar_print_log ();

#endif /* WEECHAT_GUI_BAR_H */
