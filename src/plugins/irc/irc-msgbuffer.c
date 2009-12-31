/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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

/* irc-msgbuffer.c: target buffer for IRC messages
                    (weechat, current, private) */


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


char *irc_msgbuffer_target_string[] =
{ "weechat", "server", "current", "private" };


/*
 * irc_msgbuffer_get_string: get string value for target
 */

const char *
irc_msgbuffer_get_string (int target)
{
    if ((target < 0) || (target >= IRC_MSGBUFFER_NUM_TARGETS))
        return NULL;
    
    return irc_msgbuffer_target_string[target];
}

/*
 * irc_msgbuffer_get_option: get pointer to option with IRC message
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
        
        /* search for msgbuffer in config file, for server */
        ptr_option = weechat_config_search_option (irc_config_file,
                                                   irc_config_section_msgbuffer,
                                                   option_name);
        if (ptr_option)
            return ptr_option;
    }
    
    /* search for msgbuffer in config file */
    ptr_option = weechat_config_search_option (irc_config_file,
                                               irc_config_section_msgbuffer,
                                               message);
    if (ptr_option)
        return ptr_option;
    
    /* no msgbuffer found in config */
    return NULL;
}


/*
 * irc_msgbuffer_get_target_buffer: get target for IRC message
 *                                  message is IRC message
 *                                    (for example: "invite", "312")
 *                                  alias is optional alias for message
 *                                    (for example "whois")
 */

struct t_gui_buffer *
irc_msgbuffer_get_target_buffer (struct t_irc_server *server, const char *nick,
                                 const char *message, const char *alias)
{
    struct t_config_option *ptr_option;
    int target;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_channel *ptr_channel;
    struct t_weechat_plugin *buffer_plugin;
    
    ptr_option = irc_msgbuffer_get_option (server, message);
    if (!ptr_option && alias && alias[0])
        ptr_option = irc_msgbuffer_get_option (server, alias);
    
    target = (ptr_option) ?
        weechat_config_integer (ptr_option) : -1;
    
    switch (target)
    {
        case IRC_MSGBUFFER_TARGET_WEECHAT:
            return NULL;
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
