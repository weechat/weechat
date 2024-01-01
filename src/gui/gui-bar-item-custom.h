/*
 * Copyright (C) 2022-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_GUI_BAR_ITEM_CUSTOM_H
#define WEECHAT_GUI_BAR_ITEM_CUSTOM_H

#define GUI_BAR_ITEM_CUSTOM_DEFAULT_CONDITIONS "${...}"
#define GUI_BAR_ITEM_CUSTOM_DEFAULT_CONTENTS "${...}"

struct t_gui_buffer;
struct t_gui_window;
struct t_hashtable;
struct t_weechat_plugin;

enum t_gui_bar_item_custom_option
{
    GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS = 0, /* condition(s) to display   */
    GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT,        /* item content              */
    /* number of bar options */
    GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS,
};

struct t_gui_bar_item_custom
{
    /* custom choices */
    char *name;                              /* item name                   */
    struct t_config_option *options[GUI_BAR_ITEM_CUSTOM_NUM_OPTIONS];

    /* internal vars */
    struct t_gui_bar_item *bar_item;         /* pointer to bar item        */
    struct t_gui_bar_item_custom *prev_item; /* link to previous bar item  */
    struct t_gui_bar_item_custom *next_item; /* link to next bar item      */
};

/* variables */

extern struct t_gui_bar_item_custom *gui_custom_bar_items;
extern struct t_gui_bar_item_custom *last_gui_custom_bar_item;
extern struct t_gui_bar_item_custom *gui_temp_custom_bar_items;
extern struct t_gui_bar_item_custom *last_gui_temp_custom_bar_item;

/* functions */

extern int gui_bar_item_custom_search_option (const char *option_name);
extern struct t_gui_bar_item_custom *gui_bar_item_custom_search (const char *item_name);
extern void gui_bar_item_custom_create_option_temp (struct t_gui_bar_item_custom *temp_item,
                                                    int index_option,
                                                    const char *value);
extern struct t_gui_bar_item_custom *gui_bar_item_custom_alloc (const char *name);
extern struct t_gui_bar_item_custom *gui_bar_item_custom_new (const char *name,
                                                              const char *conditions,
                                                              const char *content);
extern void gui_bar_item_custom_use_temp_items ();
extern int gui_bar_item_custom_rename (struct t_gui_bar_item_custom *item,
                                       const char *new_name);
extern void gui_bar_item_custom_free_data (struct t_gui_bar_item_custom *item);
extern void gui_bar_item_custom_free (struct t_gui_bar_item_custom *item);
extern void gui_bar_item_custom_free_all ();

#endif /* WEECHAT_GUI_BAR_ITEM_CUSTOM_H */
