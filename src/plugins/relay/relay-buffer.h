/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_RELAY_BUFFER_H
#define WEECHAT_PLUGIN_RELAY_BUFFER_H

#define RELAY_BUFFER_NAME "relay.list"

extern struct t_gui_buffer *relay_buffer;
extern int relay_buffer_selected_line;

extern int relay_buffer_is_relay (struct t_gui_buffer *buffer);
extern void relay_buffer_refresh (const char *hotlist);
extern int relay_buffer_input_cb (const void *pointer, void *data,
                                  struct t_gui_buffer *buffer,
                                  const char *input_data);
extern int relay_buffer_close_cb (const void *pointer, void *data,
                                  struct t_gui_buffer *buffer);
extern void relay_buffer_open (void);

#endif /* WEECHAT_PLUGIN_RELAY_BUFFER_H */
