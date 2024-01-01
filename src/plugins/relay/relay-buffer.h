/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
extern void relay_buffer_open ();

#endif /* WEECHAT_PLUGIN_RELAY_BUFFER_H */
