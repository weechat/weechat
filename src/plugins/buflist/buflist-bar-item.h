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

#ifndef WEECHAT_PLUGIN_BUFLIST_BAR_ITEM_H
#define WEECHAT_PLUGIN_BUFLIST_BAR_ITEM_H

#define BUFLIST_BAR_ITEM_NAME "buflist"

#define BUFLIST_BAR_NUM_ITEMS 5

struct t_gui_bar_item;

extern struct t_gui_bar_item *buflist_bar_item_buflist[BUFLIST_BAR_NUM_ITEMS];
extern struct t_arraylist *buflist_list_buffers[BUFLIST_BAR_NUM_ITEMS];

extern const char *buflist_bar_item_get_name (int index);
extern int buflist_bar_item_get_index (const char *item_name);
extern int buflist_bar_item_get_index_with_pointer (struct t_gui_bar_item *item);
extern void buflist_bar_item_update (int index, int force);
extern int buflist_bar_item_init (void);
extern void buflist_bar_item_end (void);

#endif /* WEECHAT_PLUGIN_BUFLIST_BAR_ITEM_H */
