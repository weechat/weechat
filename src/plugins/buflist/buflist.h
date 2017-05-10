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

#ifndef WEECHAT_BUFLIST_H
#define WEECHAT_BUFLIST_H 1

#define weechat_plugin weechat_buflist_plugin
#define BUFLIST_PLUGIN_NAME "buflist"

#define BUFLIST_BAR_NAME "buflist"

struct t_irc_server;
struct t_irc_channel;

extern struct t_weechat_plugin *weechat_buflist_plugin;

extern struct t_hdata *buflist_hdata_buffer;
extern struct t_hdata *buflist_hdata_hotlist;

extern void buflist_buffer_get_irc_pointers(struct t_gui_buffer *buffer,
                                            struct t_irc_server **server,
                                            struct t_irc_channel **channel);
extern struct t_gui_hotlist *buflist_search_hotlist_for_buffer (struct t_gui_buffer *buffer);
extern struct t_arraylist *buflist_sort_buffers ();

#endif /* WEECHAT_BUFLIST_H */
