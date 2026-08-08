/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
