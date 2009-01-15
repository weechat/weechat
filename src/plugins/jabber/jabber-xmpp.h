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


#ifndef __WEECHAT_JABBER_XMPP_H
#define __WEECHAT_JABBER_XMPP_H 1

#include <iksemel.h>

struct t_jabber_server;
struct t_jabber_muc;

extern const char *jabber_xmpp_tags (const char *command, const char *tags);
extern void jabber_xmpp_send_chat_message (struct t_jabber_server *server,
                                           struct t_jabber_muc *muc,
                                           const char *message);
extern int jabber_xmpp_iks_stream_hook (void *user_data, int type, iks *node);
extern void jabber_xmpp_iks_log (void *user_data, const char *data,
                                 size_t size, int is_incoming);
extern int jabber_xmpp_iks_result (void *user_data, ikspak *pak);
extern int jabber_xmpp_iks_error (void *user_data, ikspak *pak);
extern int jabber_xmpp_iks_roster (void *user_data, ikspak *pak);

#endif /* jabber-xmpp.h */
