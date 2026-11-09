/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_BAR_ITEM_H
#define WEECHAT_PLUGIN_OTR_BAR_ITEM_H

extern char *otr_bar_item_cb (const void *pointer, void *data,
                              struct t_gui_bar_item *item,
                              struct t_gui_window *window,
                              struct t_gui_buffer *buffer,
                              struct t_hashtable *extra_info);

#endif /* WEECHAT_PLUGIN_OTR_BAR_ITEM_H */
