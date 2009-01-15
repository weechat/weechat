/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_JABBER_BUFFER_H
#define __WEECHAT_JABBER_BUFFER_H 1

#define JABBER_BUFFER_ALL_SERVERS_NAME "servers"

struct t_gui_buffer;
struct t_jabber_server;
struct t_jabber_muc;

extern struct t_gui_buffer *jabber_buffer_servers;

extern void jabber_buffer_get_server_muc (struct t_gui_buffer *buffer,
                                          struct t_jabber_server **server,
                                          struct t_jabber_muc **muc);
extern char *jabber_buffer_build_name (const char *server, const char *muc);
extern char *jabber_buffer_get_server_prefix (struct t_jabber_server *server,
                                              char *prefix_code);
extern void jabber_buffer_merge_servers ();
extern void jabber_buffer_split_server ();
extern int jabber_buffer_close_cb (void *data, struct t_gui_buffer *buffer);

#endif /* jabber-buffer.h */
