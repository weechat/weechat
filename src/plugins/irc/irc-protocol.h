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


#ifndef __WEECHAT_IRC_PROTOCOL_H
#define __WEECHAT_IRC_PROTOCOL_H 1

#define IRC_PROTOCOL_MIN_ARGS(__min_args)                               \
    if (argc < __min_args)                                              \
    {                                                                   \
        weechat_printf (server->buffer,                                 \
                        _("%s%s: too few arguments received from IRC "  \
                          "server for command \"%s\" (received: %d "    \
                          "arguments, expected: at least %d)"),         \
                        weechat_prefix ("error"), "irc", command,       \
                        argc, __min_args);                              \
        return WEECHAT_RC_ERROR;                                        \
    }

#define IRC_PROTOCOL_CHECK_HOST                                         \
    if (argv[0][0] != ':')                                              \
    {                                                                   \
        weechat_printf (server->buffer,                                 \
                        _("%s%s: \"%s\" command received without "      \
                          "host"),                                      \
                        weechat_prefix ("error"), "irc", command);      \
        return WEECHAT_RC_ERROR;                                        \
    }

struct t_irc_server;

typedef int (t_irc_recv_func)(struct t_irc_server *server, const char *comand,
                              int argc, char **argv, char **argv_eol);

struct t_irc_protocol_msg
{
    char *name;                     /* IRC message name                      */
    char *description;              /* message description                   */
    int decode_color;               /* decode color before calling function  */
    t_irc_recv_func *recv_function; /* function called when msg is received  */
};

extern void irc_protocol_recv_command (struct t_irc_server *server,
                                       const char *entire_line,
                                       const char *host, const char *command,
                                       const char *arguments);

#endif /* irc-protocol.h */
