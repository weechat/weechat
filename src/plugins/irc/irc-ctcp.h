/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_IRC_CTCP_H
#define WEECHAT_PLUGIN_IRC_CTCP_H

#include <time.h>

struct t_irc_server;
struct t_irc_channel;

struct t_irc_ctcp_reply
{
    char *name;                     /* CTCP name                             */
    char *reply;                    /* CTCP reply format                     */
};

extern const char *irc_ctcp_get_default_reply (const char *ctcp);
extern const char *irc_ctcp_get_reply (struct t_irc_server *server,
                                       const char *ctcp);
extern void irc_ctcp_display_reply_from_nick (struct t_irc_server *server,
                                              time_t date,
                                              struct t_hashtable *tags,
                                              const char *command,
                                              const char *nick,
                                              const char *address,
                                              const char *arguments);
extern char *irc_ctcp_replace_variables (struct t_irc_server *server,
                                         const char *format);
extern void irc_ctcp_recv (struct t_irc_server *server, time_t date,
                           struct t_hashtable *tags, const char *command,
                           struct t_irc_channel *channel, const char *target,
                           const char *address, const char *nick,
                           const char *remote_nick, const char *arguments,
                           const char *message);
extern void irc_ctcp_send (struct t_irc_server *server,
                           const char *target, const char *type,
                           const char *args);

#endif /* WEECHAT_PLUGIN_IRC_CTCP_H */
