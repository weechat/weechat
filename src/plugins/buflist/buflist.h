/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_BUFLIST_H
#define WEECHAT_PLUGIN_BUFLIST_H

#define weechat_plugin weechat_buflist_plugin
#define BUFLIST_PLUGIN_NAME "buflist"
#define BUFLIST_PLUGIN_PRIORITY 10000

#define BUFLIST_BAR_NAME "buflist"

struct t_gui_bar_item;

extern struct t_weechat_plugin *weechat_buflist_plugin;

extern struct t_hdata *buflist_hdata_window;
extern struct t_hdata *buflist_hdata_buffer;
extern struct t_hdata *buflist_hdata_hotlist;
extern struct t_hdata *buflist_hdata_bar;
extern struct t_hdata *buflist_hdata_bar_item;
extern struct t_hdata *buflist_hdata_bar_window;

extern void buflist_add_bar (void);
extern void buflist_buffer_get_irc_pointers (struct t_gui_buffer *buffer,
                                             void **irc_server,
                                             void **irc_channel);
extern struct t_arraylist *buflist_sort_buffers (struct t_gui_bar_item *item);

#endif /* WEECHAT_PLUGIN_BUFLIST_H */
