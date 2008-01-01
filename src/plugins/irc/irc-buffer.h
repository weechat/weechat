/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_IRC_BUFFER_H
#define __WEECHAT_IRC_BUFFER_H 1

#define IRC_BUFFER_SERVER(buffer) (((t_irc_buffer_data *)(buffer->protocol_data))->server)
#define IRC_BUFFER_CHANNEL(buffer) (((t_irc_buffer_data *)(buffer->protocol_data))->channel)
#define IRC_BUFFER_ALL_SERVERS(buffer) (((t_irc_buffer_data *)(buffer->protocol_data))->all_servers)

#define IRC_BUFFER_GET_SERVER(buffer) \
    t_irc_server *ptr_server = IRC_BUFFER_SERVER(buffer)
#define IRC_BUFFER_GET_CHANNEL(buffer) \
    t_irc_channel *ptr_channel = IRC_BUFFER_CHANNEL(buffer)
#define IRC_BUFFER_GET_SERVER_CHANNEL(buffer) \
    t_irc_server *ptr_server = IRC_BUFFER_SERVER(buffer); \
    t_irc_channel *ptr_channel = IRC_BUFFER_CHANNEL(buffer)

struct t_irc_buffer_data
{
    struct t_irc_server *server;
    struct t_irc_channel *channel;
    int all_servers;
};

extern struct t_irc_buffer_data *irc_buffer_data_create (struct t_irc_server *server);
extern void irc_buffer_data_free (struct t_gui_buffer *buffer);
extern void irc_buffer_merge_servers (struct t_gui_window *window);
extern void irc_buffer_split_server (struct t_gui_window *window);

#endif /* irc-buffer.h */
