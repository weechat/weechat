/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_IRC_PROTOCOL_H
#define __WEECHAT_IRC_PROTOCOL_H 1

#define IRC_PROTOCOL_CALLBACK(__command)                                \
    int                                                                 \
    irc_protocol_cb_##__command (struct t_irc_server *server,           \
                                 time_t date,                           \
                                 const char *nick,                      \
                                 const char *address,                   \
                                 const char *host,                      \
                                 const char *command,                   \
                                 int ignored,                           \
                                 int argc,                              \
                                 char **argv,                           \
                                 char **argv_eol)

#define IRC_PROTOCOL_MIN_ARGS(__min_args)                               \
    (void) date;                                                        \
    (void) nick;                                                        \
    (void) address;                                                     \
    (void) host;                                                        \
    (void) command;                                                     \
    (void) ignored;                                                     \
    (void) argv;                                                        \
    (void) argv_eol;                                                    \
    if (argc < __min_args)                                              \
    {                                                                   \
        weechat_printf (server->buffer,                                 \
                        _("%s%s: too few arguments received from IRC "  \
                          "server for command \"%s\" (received: %d "    \
                          "arguments, expected: at least %d)"),         \
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,      \
                        command, argc, __min_args);                     \
        return WEECHAT_RC_ERROR;                                        \
    }

#define IRC_PROTOCOL_CHECK_HOST                                         \
    if (argv[0][0] != ':')                                              \
    {                                                                   \
        weechat_printf (server->buffer,                                 \
                        _("%s%s: \"%s\" command received without "      \
                          "host"),                                      \
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,      \
                        command);                                       \
        return WEECHAT_RC_ERROR;                                        \
    }

struct t_irc_server;

typedef int (t_irc_recv_func)(struct t_irc_server *server,
                              time_t date, const char *nick,
                              const char *address, const char *host,
                              const char *command,
                              int ignored,
                              int argc, char **argv, char **argv_eol);

struct t_irc_protocol_msg
{
    char *name;                     /* IRC message name                      */
    int decode_color;               /* decode color before calling function  */
    int keep_trailing_spaces;       /* keep trailing spaces in message       */
    t_irc_recv_func *recv_function; /* function called when msg is received  */
};

extern const char *irc_protocol_tags (const char *command, const char *tags,
                                      const char *nick);
extern void irc_protocol_recv_command (struct t_irc_server *server,
                                       const char *irc_message,
                                       const char *msg_tags,
                                       const char *msg_command,
                                       const char *msg_channel);

#endif /* __WEECHAT_IRC_PROTOCOL_H */
