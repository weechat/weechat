/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_IRC_MESSAGE_H
#define __WEECHAT_IRC_MESSAGE_H 1

struct t_irc_server;
struct t_irc_channel;

extern void irc_message_parse (struct t_irc_server *server, const char *message,
                               char **tags, char **message_without_tags,
                               char **nick, char **host, char **command,
                               char **channel, char **arguments);
extern struct t_hashtable *irc_message_parse_to_hashtable (struct t_irc_server *server,
                                                           const char *message);
extern const char *irc_message_get_nick_from_host (const char *host);
extern const char *irc_message_get_address_from_host (const char *host);
extern char *irc_message_replace_vars (struct t_irc_server *server,
                                       struct t_irc_channel *channel,
                                       const char *string);
extern struct t_hashtable *irc_message_split (struct t_irc_server *server,
                                              const char *message);

#endif /* __WEECHAT_IRC_MESSAGE_H */
