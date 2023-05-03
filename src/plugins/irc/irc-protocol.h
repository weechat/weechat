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
    irc_protocol_cb_##__command (struct t_irc_server *server,           \
                                 time_t date,                           \
                                 const char *irc_message,               \
                                 struct t_hashtable *tags,              \
                                 const char *nick,                      \
                                 const char *address,                   \
                                 const char *host,                      \
                                 const char *command,                   \
                                 int ignored,                           \
                                 const char **params,                   \
                                 int num_params)

#define IRC_PROTOCOL_RUN_CALLBACK(__name)                               \
    irc_protocol_cb_##__name (server, date, irc_message, tags, nick,    \
                              address, host, command, ignored, params,  \
                              num_params)

#define IRCB(__message, __decode_color, __keep_trailing_spaces,         \
             __func_cb)                                                 \
    { #__message,                                                       \
      __decode_color,                                                   \
      __keep_trailing_spaces,                                           \
      &irc_protocol_cb_##__func_cb }

#define IRC_PROTOCOL_MIN_PARAMS(__min_params)                           \
    (void) date;                                                        \
    (void) irc_message;                                                 \
    (void) tags;                                                        \
    (void) nick;                                                        \
    (void) address;                                                     \
    (void) host;                                                        \
    (void) command;                                                     \
    (void) ignored;                                                     \
    (void) params;                                                      \
    (void) num_params;                                                  \
    if (num_params < __min_params)                                      \
    {                                                                   \
        weechat_printf (server->buffer,                                 \
                        _("%s%s: too few parameters received in "       \
                          "command \"%s\" (received: %d, expected: "    \
                          "at least %d)"),                              \
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,      \
                        command, num_params, __min_params);             \
        return WEECHAT_RC_ERROR;                                        \
    }

#define IRC_PROTOCOL_CHECK_NICK                                         \
    if (!nick || !nick[0])                                              \
    {                                                                   \
        weechat_printf (server->buffer,                                 \
                        _("%s%s: command \"%s\" received without "      \
                          "nick"),                                      \
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,      \
                        command);                                       \
        return WEECHAT_RC_ERROR;                                        \
    }

struct t_irc_server;

typedef int (t_irc_recv_func)(struct t_irc_server *server,
                              time_t date, const char *irc_message,
                              struct t_hashtable *tags,
                              const char *nick, const char *address,
                              const char *host, const char *command,
                              int ignored,
                              const char **params, int num_params);

struct t_irc_protocol_msg
{
    char *name;                     /* IRC message name                      */
    int decode_color;               /* decode color before calling function  */
    int keep_trailing_spaces;       /* keep trailing spaces in message       */
    t_irc_recv_func *recv_function; /* function called when msg is received  */
};

extern const char *irc_protocol_tags (struct t_irc_server *server,
                                      const char *command,
                                      struct t_hashtable *irc_msg_tags,
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
