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


#ifndef __WEECHAT_IRC_COMPLETION_H
#define __WEECHAT_IRC_COMPLETION_H 1

extern int irc_completion_server_cb (void *, char *, void *, void *);
extern int irc_completion_server_nicks_cb (void *, char *, void *, void *);
extern int irc_completion_servers_cb (void *, char *, void *, void *);
extern int irc_completion_channel_cb (void *, char *, void *, void *);
extern int irc_completion_channel_nicks_cb (void *, char *, void *, void *);
extern int irc_completion_channel_nicks_hosts_cb (void *, char *, void *,
                                                  void *);
extern int irc_completion_channel_topic_cb (void *, char *, void *, void *);
extern int irc_completion_channels_cb (void *, char *, void *, void *);
extern int irc_completion_msg_part_cb (void *, char *, void *, void *);

#endif /* irc-completion.h */
