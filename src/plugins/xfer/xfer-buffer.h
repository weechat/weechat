/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_XFER_BUFFER_H
#define WEECHAT_PLUGIN_XFER_BUFFER_H

#define XFER_BUFFER_NAME "xfer.list"

extern struct t_gui_buffer *xfer_buffer;
extern int xfer_buffer_selected_line;

extern void xfer_buffer_refresh (const char *hotlist);
extern int xfer_buffer_input_cb (const void *pointer, void *data,
                                 struct t_gui_buffer *buffer,
                                 const char *input_data);
extern int xfer_buffer_close_cb (const void *pointer, void *data,
                                 struct t_gui_buffer *buffer);
extern void xfer_buffer_open (void);

#endif /* WEECHAT_PLUGIN_XFER_BUFFER_H */
