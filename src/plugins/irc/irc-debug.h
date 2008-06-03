/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_IRC_DEBUG_H
#define __WEECHAT_IRC_DEBUG_H 1

#define IRC_DEBUG_PREFIX_RECV     "-->"
#define IRC_DEBUG_PREFIX_RECV_MOD "==>"
#define IRC_DEBUG_PREFIX_SEND     "<--"
#define IRC_DEBUG_PREFIX_SEND_MOD "<=="

struct t_irc_server;

extern void irc_debug_printf (struct t_irc_server *server, int send,
                              int modified, const char *message);
extern void irc_debug_init ();

#endif /* irc-debug.h */
