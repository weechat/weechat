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

#ifndef WEECHAT_PLUGIN_IRC_PROTOCOL_H
#define WEECHAT_PLUGIN_IRC_PROTOCOL_H

#include <time.h>

#define IRC_PROTOCOL_CALLBACK(__command)                                \
    int                                                                 \
    irc_protocol_cb_##__command (                                       \
        struct t_irc_protocol_ctxt *ctxt)

#define IRCB(__message, __decode_color, __keep_trailing_spaces,         \
             __func_cb)                                                 \
    { #__message,                                                       \
      __decode_color,                                                   \
      __keep_trailing_spaces,                                           \
      &irc_protocol_cb_##__func_cb }

#define IRC_PROTOCOL_MIN_PARAMS(__min_params)                           \
    if (ctxt->num_params < __min_params)                                \
    {                                                                   \
        weechat_printf (ctxt->server->buffer,                           \
                        _("%s%s: too few parameters received in "       \
                          "command \"%s\" (received: %d, expected: "    \
                          "at least %d)"),                              \
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,      \
                        ctxt->command, ctxt->num_params, __min_params); \
        return WEECHAT_RC_ERROR;                                        \
    }

#define IRC_PROTOCOL_CHECK_NICK                                         \
    if (!ctxt->nick || !ctxt->nick[0])                                  \
    {                                                                   \
        weechat_printf (ctxt->server->buffer,                           \
                        _("%s%s: command \"%s\" received without "      \
                          "nick"),                                      \
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,      \
                        ctxt->command);                                 \
        return WEECHAT_RC_ERROR;                                        \
    }

struct t_irc_server;

struct t_irc_protocol_ctxt
{
    struct t_irc_server *server;
    time_t date;
    char *irc_message;
    struct t_hashtable *tags;
    char *nick;
    int nick_is_me;
    char *address;
    char *host;
    char *command;
    int ignored;
    char **params;
    int num_params;
};

typedef int (t_irc_recv_func)(struct t_irc_protocol_ctxt *ctxt);

struct t_irc_protocol_msg
{
    char *name;                     /* IRC message name                      */
    int decode_color;               /* decode color before calling function  */
    int keep_trailing_spaces;       /* keep trailing spaces in message       */
    t_irc_recv_func *recv_function; /* function called when msg is received  */
};

extern const char *irc_protocol_tags (struct t_irc_protocol_ctxt *ctxt,
                                      const char *extra_tags,
                                      const char *nick,
                                      const char *address);
extern time_t irc_protocol_parse_time (const char *time);
extern void irc_protocol_recv_command (struct t_irc_server *server,
                                       const char *irc_message,
                                       const char *msg_command,
                                       const char *msg_channel,
                                       int ignore_batch_tag);

#endif /* WEECHAT_PLUGIN_IRC_PROTOCOL_H */
