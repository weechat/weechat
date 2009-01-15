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

/* irc-buffer.c: buffer functions for IRC plugin */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-server.h"


/* buffer for all servers (if using one buffer for all servers) */
struct t_gui_buffer *irc_buffer_servers = NULL;


/*
 * irc_buffer_get_server_channel: get IRC server and channel pointers with a
 *                                buffer pointer
 *                                (buffer may be a server or a channel)
 */

void
irc_buffer_get_server_channel (struct t_gui_buffer *buffer,
                               struct t_irc_server **server,
                               struct t_irc_channel **channel)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    
    if (server)
        *server = NULL;
    if (channel)
        *channel = NULL;
    
    if (!buffer)
        return;
    
    /* look for a server or channel using this buffer */
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer == buffer)
        {
            if (server)
            {
                if (weechat_config_boolean (irc_config_look_one_server_buffer))
                    *server = irc_current_server;
                else
                    *server = ptr_server;
            }
            return;
        }
        
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->buffer == buffer)
            {
                if (server)
                    *server = ptr_server;
                if (channel)
                    *channel = ptr_channel;
                return;
            }
        }
    }
    
    /* no server or channel found */
}

/*
 * irc_buffer_build_name: build buffer name with a server and a channel
 */

char *
irc_buffer_build_name (const char *server, const char *channel)
{
    static char buffer[128];
    
    buffer[0] = '\0';
    
    if (!server && !channel)
        return buffer;
    
    if (server && channel)
        snprintf (buffer, sizeof (buffer), "%s.%s", server, channel);
    else
        snprintf (buffer, sizeof (buffer), "%s",
                  (server) ? server : channel);
    
    return buffer;
}

/*
 * irc_buffer_get_server_prefix: return prefix, with server name if server
 *                               buffers are displayed in only one buffer
 */

char *
irc_buffer_get_server_prefix (struct t_irc_server *server, char *prefix_code)
{
    static char buf[256];
    const char *prefix;
    
    prefix = (prefix_code && prefix_code[0]) ?
        weechat_prefix (prefix_code) : NULL;
    
    if (weechat_config_boolean (irc_config_look_one_server_buffer) && server)
    {
        snprintf (buf, sizeof (buf), "%s%s[%s%s%s]%s ",
                  (prefix) ? prefix : "",
                  IRC_COLOR_CHAT_DELIMITERS,
                  IRC_COLOR_CHAT_SERVER,
                  server->name,
                  IRC_COLOR_CHAT_DELIMITERS,
                  IRC_COLOR_CHAT);
    }
    else
    {
        snprintf (buf, sizeof (buf), "%s",
                  (prefix) ? prefix : "");
    }
    return buf;
}

/*
 * irc_buffer_merge_servers: merge server buffers in one buffer
 */

void
irc_buffer_merge_servers ()
{
    struct t_irc_server *ptr_server;
    struct t_gui_buffer *ptr_buffer;
    int number, number_selected;
    char charset_modifier[256];
    
    irc_buffer_servers = NULL;
    irc_current_server = NULL;
    
    /* choose server buffer with lower number (should be first created) */
    number_selected = -1;
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer)
        {
            number = weechat_buffer_get_integer (ptr_server->buffer, "number");
            if ((number_selected == -1) || (number < number_selected))
            {
                irc_buffer_servers = ptr_server->buffer;
                irc_current_server = ptr_server;
                number_selected = number;
            }
        }
    }
    
    if (irc_buffer_servers)
    {
        weechat_buffer_set (irc_buffer_servers,
                            "name", IRC_BUFFER_ALL_SERVERS_NAME);
        weechat_buffer_set (irc_buffer_servers,
                            "short_name", IRC_BUFFER_ALL_SERVERS_NAME);
        weechat_buffer_set (irc_buffer_servers, "key_bind_meta-s",
                            "/command irc /server switch");
        weechat_buffer_set (irc_buffer_servers,
                            "localvar_set_server", IRC_BUFFER_ALL_SERVERS_NAME);
        weechat_buffer_set (irc_buffer_servers,
                            "localvar_set_channel", IRC_BUFFER_ALL_SERVERS_NAME);
        snprintf (charset_modifier, sizeof (charset_modifier),
                  "irc.%s", irc_current_server->name);
        weechat_buffer_set (irc_buffer_servers,
                            "localvar_set_charset_modifier",
                            charset_modifier);
        weechat_hook_signal_send ("logger_stop",
                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                  irc_buffer_servers);
        weechat_hook_signal_send ("logger_start",
                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                  irc_buffer_servers);
        
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->buffer
                && (ptr_server->buffer != irc_buffer_servers))
            {
                ptr_buffer = ptr_server->buffer;
                ptr_server->buffer = irc_buffer_servers;
                weechat_buffer_close (ptr_buffer);
            }
        }
        
        irc_server_set_buffer_title (irc_current_server);
        irc_server_buffer_set_highlight_words (irc_buffer_servers);
    }
}

/*
 * irc_buffer_split_server: split the server buffer into many buffers (one by server)
 */

void
irc_buffer_split_server ()
{
    struct t_irc_server *ptr_server;
    char buffer_name[256], charset_modifier[256];
    
    if (irc_buffer_servers)
    {
        weechat_buffer_set (irc_buffer_servers, "key_unbind_meta-s", "");
    }
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer && (ptr_server != irc_current_server))
        {
            irc_server_create_buffer (ptr_server, 0);
        }
    }
    
    if (irc_current_server)
    {
        snprintf (buffer_name, sizeof (buffer_name),
                  "server.%s", irc_current_server->name);
        weechat_buffer_set (irc_current_server->buffer, "name", buffer_name);
        weechat_buffer_set (irc_current_server->buffer,
                            "short_name", irc_current_server->name);
        weechat_buffer_set (irc_current_server->buffer,
                            "localvar_set_server", irc_current_server->name);
        weechat_buffer_set (irc_current_server->buffer,
                            "localvar_set_channel", irc_current_server->name);
        snprintf (charset_modifier, sizeof (charset_modifier),
                  "irc.%s", irc_current_server->name);
        weechat_buffer_set (irc_current_server->buffer,
                            "localvar_set_charset_modifier",
                            charset_modifier);
        weechat_hook_signal_send ("logger_stop",
                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                  irc_current_server->buffer);
        weechat_hook_signal_send ("logger_start",
                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                  irc_current_server->buffer);
    }
    
    irc_buffer_servers = NULL;
    irc_current_server = NULL;
}

/*
 * irc_buffer_close_cb: callback called when a buffer is closed
 */

int
irc_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_irc_channel *next_channel;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    
    /* make C compiler happy */
    (void) data;
    
    if (ptr_channel)
    {
        /* send PART for channel if its buffer is closed */
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            && (ptr_channel->nicks))
        {
            irc_command_part_channel (ptr_server, ptr_channel->name, NULL);
        }
        irc_channel_free (ptr_server, ptr_channel);
    }
    else
    {
        if (ptr_server)
        {
            /* send PART on all channels for server, then disconnect from server */
            ptr_channel = ptr_server->channels;
            while (ptr_channel)
            {
                next_channel = ptr_channel->next_channel;
                weechat_buffer_close (ptr_channel->buffer);
                ptr_channel = next_channel;
            }
            irc_server_disconnect (ptr_server, 0);
            ptr_server->buffer = NULL;
        }
    }

    if (irc_buffer_servers == buffer)
        irc_buffer_servers = NULL;
    if (ptr_server && (irc_current_server == ptr_server))
        irc_current_server = NULL;
    
    return WEECHAT_RC_OK;
}
