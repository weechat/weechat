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
    struct t_irc_server *server;       /* IRC server                        */
    time_t date;                       /* message date                      */
    int date_usec;                     /* microseconds of date              */
    char *irc_message;                 /* whole raw IRC message             */
    struct t_hashtable *tags;          /* IRC tags                          */
    char *nick;                        /* nick of sender                    */
    int nick_is_me;                    /* nick of sender is myself          */
    char *address;                     /* address of sender                 */
    char *host;                        /* host of sender                    */
    char *command;                     /* IRC command (eg: PRIVMSG)         */
    int ignore_remove;                 /* msg ignored (not displayed)       */
    int ignore_tag;                    /* mse ignored (displayed with tag)  */
    char **params;                     /* IRC command parameters            */
    int num_params;                    /* number of IRC command parameters  */
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
                                      const char *extra_tags);
extern void irc_protocol_recv_command (struct t_irc_server *server,
                                       const char *irc_message,
                                       const char *msg_command,
                                       const char *msg_channel,
                                       int ignore_batch_tag);

#endif /* WEECHAT_PLUGIN_IRC_PROTOCOL_H */
