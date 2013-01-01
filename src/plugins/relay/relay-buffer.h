/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_RELAY_BUFFER_H
#define __WEECHAT_RELAY_BUFFER_H 1

#define RELAY_BUFFER_NAME "relay.list"

extern struct t_gui_buffer *relay_buffer;
extern int relay_buffer_selected_line;

extern void relay_buffer_refresh (const char *hotlist);
extern int relay_buffer_input_cb (void *data, struct t_gui_buffer *buffer,
                                 const char *input_data);
extern int relay_buffer_close_cb (void *data, struct t_gui_buffer *buffer);
extern void relay_buffer_open ();

#endif /* __WEECHAT_RELAY_BUFFER_H */
