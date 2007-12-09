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


#ifndef __WEECHAT_IRC_COMMAND_H
#define __WEECHAT_IRC_COMMAND_H 1

#define IRC_COMMAND_GET_SERVER(buffer)                          \
    struct t_irc_server *ptr_server = irc_server_search (       \
        weechat_buffer_get (buffer, "category"))

#define IRC_COMMAND_GET_SERVER_CHANNEL(buffer)                     \
    struct t_irc_server *ptr_server = irc_server_search (               \
        weechat_buffer_get (buffer, "category"));                       \
    struct t_irc_channel *ptr_channel = irc_channel_search (            \
        ptr_server,                                                     \
        weechat_buffer_get (buffer, "name"))


extern void irc_command_init ();

#endif /* irc-command.h */
