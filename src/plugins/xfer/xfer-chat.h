/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_XFER_CHAT_H
#define WEECHAT_PLUGIN_XFER_CHAT_H

/*
 * maximum length of the partial message accumulated while reading from the
 * remote host: once this limit is reached, the message is discarded as well
 * as the data received; this protects against a remote host sending a huge
 * amount of data without any end-of-line, which would consume all the memory
 */
#define XFER_CHAT_PARTIAL_MESSAGE_MAX_LENGTH (8 * 1024 * 1024)

extern void xfer_chat_sendf (struct t_xfer *xfer, const char *format, ...);
extern int xfer_chat_recv_cb (const void *pointer, void *data, int fd);
extern int xfer_chat_buffer_input_cb (const void *pointer, void *data,
                                      struct t_gui_buffer *buffer,
                                      const char *input_data);
extern int xfer_chat_buffer_close_cb (const void *pointer, void *data,
                                      struct t_gui_buffer *buffer);
extern void xfer_chat_open_buffer (struct t_xfer *xfer);

#endif /* WEECHAT_PLUGIN_XFER_CHAT_H */
