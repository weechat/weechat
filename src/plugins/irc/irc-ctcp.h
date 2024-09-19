/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
struct t_irc_protocol_ctxt;

struct t_irc_ctcp_reply
{
    const char *name;               /* CTCP name                             */
    const char *reply;              /* CTCP reply format                     */
};

extern const struct t_irc_ctcp_reply irc_ctcp_default_reply[];

extern char *irc_ctcp_convert_legacy_format (const char *format);
extern const char *irc_ctcp_get_default_reply (const char *ctcp);
extern const char *irc_ctcp_get_reply (struct t_irc_server *server,
                                       const char *ctcp);
extern void irc_ctcp_parse_type_arguments (const char *message,
                                           char **type, char **arguments);
extern void irc_ctcp_display_reply_from_nick (struct t_irc_protocol_ctxt *ctxt,
                                              const char *arguments);
extern void irc_ctcp_display_reply_to_nick (struct t_irc_protocol_ctxt *ctxt,
                                            const char *target,
                                            const char *arguments);
extern char *irc_ctcp_eval_reply (struct t_irc_server *server,
                                  const char *format);
extern void irc_ctcp_recv (struct t_irc_protocol_ctxt *ctxt,
                           struct t_irc_channel *channel,
                           const char *remote_nick,
                           const char *arguments);
extern void irc_ctcp_send (struct t_irc_server *server,
                           const char *target, const char *type,
                           const char *args);

#endif /* WEECHAT_PLUGIN_IRC_CTCP_H */
