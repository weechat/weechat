/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_GUI_BAR_ITEM_H
#define WEECHAT_GUI_BAR_ITEM_H

enum t_gui_bar_item_weechat
{
    GUI_BAR_ITEM_INPUT_PASTE = 0,
    GUI_BAR_ITEM_INPUT_PROMPT,
    GUI_BAR_ITEM_INPUT_SEARCH,
    GUI_BAR_ITEM_INPUT_TEXT,
    GUI_BAR_ITEM_TIME,
    GUI_BAR_ITEM_BUFFER_COUNT,
    GUI_BAR_ITEM_BUFFER_LAST_NUMBER,
    GUI_BAR_ITEM_BUFFER_PLUGIN,
    GUI_BAR_ITEM_BUFFER_NUMBER,
    GUI_BAR_ITEM_BUFFER_NAME,
    GUI_BAR_ITEM_BUFFER_SHORT_NAME,
    GUI_BAR_ITEM_BUFFER_MODES,
    GUI_BAR_ITEM_BUFFER_FILTER,
    GUI_BAR_ITEM_BUFFER_ZOOM,
    GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT,
    GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT_GROUPS,
    GUI_BAR_ITEM_BUFFER_NICKLIST_COUNT_ALL,
    GUI_BAR_ITEM_SCROLL,
    GUI_BAR_ITEM_HOTLIST,
    GUI_BAR_ITEM_COMPLETION,
    GUI_BAR_ITEM_BUFFER_TITLE,
    GUI_BAR_ITEM_BUFFER_NICKLIST,
    GUI_BAR_ITEM_WINDOW_NUMBER,
    GUI_BAR_ITEM_MOUSE_STATUS,
    GUI_BAR_ITEM_LAG,
    GUI_BAR_ITEM_AWAY,
    GUI_BAR_ITEM_SPACER,
    /* number of bar items */
    GUI_BAR_NUM_ITEMS,
};

struct t_gui_bar;
struct t_gui_buffer;
struct t_gui_window;
struct t_infolist;

struct t_gui_bar_item
{
    struct t_weechat_plugin *plugin; /* plugin                              */
    char *name;                      /* bar item name                       */
    char *(*build_callback)(const void *pointer,
                            void *data,
                            struct t_gui_bar_item *item,
                            struct t_gui_window *window,
                            struct t_gui_buffer *buffer,
                            struct t_hashtable *extra_info);
                                     /* callback called for building item   */
    const void *build_callback_pointer; /* pointer for callback             */
    void *build_callback_data;          /* data for callback                */
    struct t_gui_bar_item *prev_item; /* link to previous bar item          */
    struct t_gui_bar_item *next_item; /* link to next bar item              */
};

struct t_gui_bar_item_hook
{
    struct t_hook *hook;                   /* pointer to hook               */
    struct t_gui_bar_item_hook *next_hook; /* next hook                     */
};

/* variables */

extern struct t_gui_bar_item *gui_bar_items;
extern struct t_gui_bar_item *last_gui_bar_item;
extern char *gui_bar_item_names[];

/* functions */

extern int gui_bar_item_valid (struct t_gui_bar_item *bar_item);
extern int gui_bar_item_search_default (const char *item_name);
extern struct t_gui_bar_item *gui_bar_item_search (const char *name);
extern int gui_bar_item_used_in_bar (struct t_gui_bar *bar,
                                     const char *item_name,
                                     int partial_name);
extern int gui_bar_item_used_in_at_least_one_bar (const char *item_name,
                                                  int partial_name,
                                                  int ignore_hidden_bars);
extern void gui_bar_item_get_vars (const char *item_name,
                                   char **buffer, char **prefix, char **name,
                                   char **suffix);
extern char *gui_bar_item_get_value (struct t_gui_bar *bar,
                                     struct t_gui_window *window,
                                     int item, int subitem);
extern int gui_bar_item_count_lines (char *string);
extern struct t_gui_bar_item *gui_bar_item_new (struct t_weechat_plugin *plugin,
                                                const char *name,
                                                char *(*build_callback)(const void *pointer,
                                                                        void *data,
                                                                        struct t_gui_bar_item *item,
                                                                        struct t_gui_window *window,
                                                                        struct t_gui_buffer *buffer,
                                                                        struct t_hashtable *extra_info),
                                                const void *build_callback_pointer,
                                                void *build_callback_data);
extern void gui_bar_item_update (const char *name);
extern void gui_bar_item_free (struct t_gui_bar_item *item);
extern void gui_bar_item_free_all ();
extern void gui_bar_item_free_all_plugin (struct t_weechat_plugin *plugin);
extern void gui_bar_item_init ();
extern void gui_bar_item_end ();
extern struct t_hdata *gui_bar_item_hdata_bar_item_cb (const void *pointer,
                                                       void *data,
                                                       const char *hdata_name);
extern int gui_bar_item_add_to_infolist (struct t_infolist *infolist,
                                         struct t_gui_bar_item *bar_item);
extern void gui_bar_item_print_log ();

#endif /* WEECHAT_GUI_BAR_ITEM_H */
