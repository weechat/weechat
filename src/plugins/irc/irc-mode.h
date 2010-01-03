/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_IRC_MODE_H
#define __WEECHAT_IRC_MODE_H 1

struct t_irc_server;
struct t_irc_channel;

extern int irc_mode_channel_set (struct t_irc_server *server,
                                 struct t_irc_channel *channel,
                                 const char *modes);
extern void irc_mode_user_set (struct t_irc_server *server, const char *modes);
extern int irc_mode_nick_prefix_allowed (struct t_irc_server *server,
                                         char prefix);

#endif /* irc-mode.h */
