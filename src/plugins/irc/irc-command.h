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


#ifndef __WEECHAT_IRC_COMMAND_H
#define __WEECHAT_IRC_COMMAND_H 1

#include "irc-server.h"

#define IRC_COMMAND_GET_SERVER(__buffer)                                \
    struct t_weechat_plugin *buffer_plugin = NULL;                      \
    struct t_irc_server *ptr_server = NULL;                             \
    buffer_plugin = weechat_buffer_get (__buffer, "plugin");            \
    if (buffer_plugin == weechat_irc_plugin)                            \
        ptr_server = irc_server_search (                                \
            weechat_buffer_get (__buffer, "category"));

#define IRC_COMMAND_GET_SERVER_CHANNEL(__buffer)                        \
    struct t_weechat_plugin *buffer_plugin = NULL;                      \
    struct t_irc_server *ptr_server = NULL;                             \
    struct t_irc_channel *ptr_channel = NULL;                           \
    buffer_plugin = weechat_buffer_get (__buffer, "plugin");            \
    if (buffer_plugin == weechat_irc_plugin)                            \
    {                                                                   \
        ptr_server = irc_server_search (                                \
            weechat_buffer_get (__buffer, "category"));                 \
        ptr_channel = irc_channel_search (                              \
            ptr_server, weechat_buffer_get (__buffer, "name"));         \
    }

#define IRC_COMMAND_TOO_FEW_ARGUMENTS(__buffer, __command)              \
    weechat_printf (__buffer,                                           \
                    _("%sirc: too few arguments for \"%s\" command"),   \
                    weechat_prefix ("error"), __command);               \
    return WEECHAT_RC_ERROR;


extern void irc_command_quit_server (struct t_irc_server *, char *);
extern void irc_command_init ();

#endif /* irc-command.h */
