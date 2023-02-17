/*
 * irc-msgbuffer.c - target buffer for IRC messages
 *
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/utsname.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-msgbuffer.h"
#include "irc-channel.h"
#include "irc-config.h"
#include "irc-server.h"


/*
 * Gets pointer to option with IRC message.
 *
 * Returns pointer to option found, NULL if not found.
 */

struct t_config_option *
irc_msgbuffer_get_option (struct t_irc_server *server, const char *message)
{
    struct t_config_option *ptr_option;
    char option_name[512];

    if (server)
    {
        snprintf (option_name, sizeof (option_name),
                  "%s.%s", server->name, message);

        /* search for msgbuffer in configuration file, for server */
        ptr_option = weechat_config_search_option (irc_config_file,
                                                   irc_config_section_msgbuffer,
                                                   option_name);
        if (ptr_option)
            return ptr_option;
    }

    /* search for msgbuffer in configuration file */
    ptr_option = weechat_config_search_option (irc_config_file,
                                               irc_config_section_msgbuffer,
                                               message);
    if (ptr_option)
        return ptr_option;

    /* no msgbuffer found in configuration */
    return NULL;
}

/*
 * Gets target for IRC message.
 *
 * Arguments:
 *   message: IRC message (for example: "invite", "312")
 *   alias: optional alias for message (for example "whois")
 *   default_buffer: used if no target is defined (optional, by default server
 *                   buffer is used).
 *
 * Returns pointer to buffer found, NULL if not found.
 */

struct t_gui_buffer *
irc_msgbuffer_get_target_buffer (struct t_irc_server *server, const char *nick,
                                 const char *message, const char *alias,
                                 struct t_gui_buffer *default_buffer)
{
    struct t_config_option *ptr_option;
    int target;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_channel *ptr_channel;
    struct t_weechat_plugin *buffer_plugin;

    ptr_option = NULL;

    if (message && message[0])
        ptr_option = irc_msgbuffer_get_option (server, message);
    if (!ptr_option && alias && alias[0])
        ptr_option = irc_msgbuffer_get_option (server, alias);

    if (!ptr_option)
    {
        if (default_buffer)
            return default_buffer;
        return (server) ? server->buffer : NULL;
    }

    target = weechat_config_integer (ptr_option);
    switch (target)
    {
        case IRC_MSGBUFFER_TARGET_WEECHAT:
            return NULL;
            break;
        case IRC_MSGBUFFER_TARGET_SERVER:
            return (server) ? server->buffer : NULL;
            break;
        case IRC_MSGBUFFER_TARGET_CURRENT:
            break;
        case IRC_MSGBUFFER_TARGET_PRIVATE:
            ptr_channel = irc_channel_search (server, nick);
            if (ptr_channel)
                return ptr_channel->buffer;
            if (weechat_config_integer (irc_config_look_msgbuffer_fallback) ==
                IRC_CONFIG_LOOK_MSGBUFFER_FALLBACK_SERVER)
            {
                return (server) ? server->buffer : NULL;
            }
            break;
        default:
            return (server) ? server->buffer : NULL;
            break;
    }

    ptr_buffer = weechat_current_buffer ();
    buffer_plugin = weechat_buffer_get_pointer (ptr_buffer, "plugin");
    if (buffer_plugin == weechat_irc_plugin)
        return ptr_buffer;

    return (server) ? server->buffer : NULL;
}
