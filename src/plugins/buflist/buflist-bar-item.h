/*
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WEECHAT_BUFLIST_BAR_ITEM_H
#define WEECHAT_BUFLIST_BAR_ITEM_H 1

#define BUFLIST_BAR_ITEM_NAME "buflist"

extern struct t_arraylist *buflist_list_buffers;

extern void buflist_bar_item_update ();
extern int buflist_bar_item_init ();
extern void buflist_bar_item_end ();

#endif /* WEECHAT_BUFLIST_BAR_ITEM_H */
