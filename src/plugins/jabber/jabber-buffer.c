/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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

/* jabber-buffer.c: buffer functions for Jabber plugin */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-buffer.h"
#include "jabber-command.h"
#include "jabber-config.h"
#include "jabber-muc.h"
#include "jabber-server.h"


/* buffer for all servers (if using one buffer for all servers) */
struct t_gui_buffer *jabber_buffer_servers = NULL;


/*
 * jabber_buffer_get_server_muc: get Jabber server and MUC pointers
 *                               with a buffer pointer
 *                               (buffer may be a server or a MUC)
 */

void
jabber_buffer_get_server_muc (struct t_gui_buffer *buffer,
                              struct t_jabber_server **server,
                              struct t_jabber_muc **muc)
{
    struct t_jabber_server *ptr_server;
    struct t_jabber_muc *ptr_muc;
    
    if (server)
        *server = NULL;
    if (muc)
        *muc = NULL;
    
    if (!buffer)
        return;
    
    /* look for a server or MUC using this buffer */
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer == buffer)
        {
            if (server)
            {
                if (weechat_config_boolean (jabber_config_look_one_server_buffer))
                    *server = jabber_current_server;
                else
                    *server = ptr_server;
            }
            return;
        }
        
        for (ptr_muc = ptr_server->mucs; ptr_muc;
             ptr_muc = ptr_muc->next_muc)
        {
            if (ptr_muc->buffer == buffer)
            {
                if (server)
                    *server = ptr_server;
                if (muc)
                    *muc = ptr_muc;
                return;
            }
        }
    }
    
    /* no server or MUC found */
}

/*
 * jabber_buffer_build_name: build buffer name with a server and a MUC
 */

char *
jabber_buffer_build_name (const char *server, const char *muc)
{
    static char buffer[128];
    
    buffer[0] = '\0';
    
    if (!server && !muc)
        return buffer;
    
    if (server && muc)
        snprintf (buffer, sizeof (buffer), "%s.%s", server, muc);
    else
        snprintf (buffer, sizeof (buffer), "%s",
                  (server) ? server : muc);
    
    return buffer;
}

/*
 * jabber_buffer_get_server_prefix: return prefix, with server name if server
 *                                  buffers are displayed in only one buffer
 */

char *
jabber_buffer_get_server_prefix (struct t_jabber_server *server,
                                 char *prefix_code)
{
    static char buf[256];
    const char *prefix;
    
    prefix = (prefix_code && prefix_code[0]) ?
        weechat_prefix (prefix_code) : NULL;
    
    if (weechat_config_boolean (jabber_config_look_one_server_buffer) && server)
    {
        snprintf (buf, sizeof (buf), "%s%s[%s%s%s]%s ",
                  (prefix) ? prefix : "",
                  JABBER_COLOR_CHAT_DELIMITERS,
                  JABBER_COLOR_CHAT_SERVER,
                  server->name,
                  JABBER_COLOR_CHAT_DELIMITERS,
                  JABBER_COLOR_CHAT);
    }
    else
    {
        snprintf (buf, sizeof (buf), "%s",
                  (prefix) ? prefix : "");
    }
    return buf;
}

/*
 * jabber_buffer_merge_servers: merge server buffers in one buffer
 */

void
jabber_buffer_merge_servers ()
{
    struct t_jabber_server *ptr_server;
    struct t_gui_buffer *ptr_buffer;
    int number, number_selected;
    char charset_modifier[256];
    
    jabber_buffer_servers = NULL;
    jabber_current_server = NULL;
    
    /* choose server buffer with lower number (should be first created) */
    number_selected = -1;
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer)
        {
            number = weechat_buffer_get_integer (ptr_server->buffer, "number");
            if ((number_selected == -1) || (number < number_selected))
            {
                jabber_buffer_servers = ptr_server->buffer;
                jabber_current_server = ptr_server;
                number_selected = number;
            }
        }
    }
    
    if (jabber_buffer_servers)
    {
        weechat_buffer_set (jabber_buffer_servers,
                            "name", JABBER_BUFFER_ALL_SERVERS_NAME);
        weechat_buffer_set (jabber_buffer_servers,
                            "short_name", JABBER_BUFFER_ALL_SERVERS_NAME);
        weechat_buffer_set (jabber_buffer_servers, "key_bind_meta-s",
                            "/command jabber /jabber switch");
        weechat_buffer_set (jabber_buffer_servers,
                            "localvar_set_server", JABBER_BUFFER_ALL_SERVERS_NAME);
        weechat_buffer_set (jabber_buffer_servers,
                            "localvar_set_muc", JABBER_BUFFER_ALL_SERVERS_NAME);
        snprintf (charset_modifier, sizeof (charset_modifier),
                  "jabber.%s", jabber_current_server->name);
        weechat_buffer_set (jabber_buffer_servers,
                            "localvar_set_charset_modifier",
                            charset_modifier);
        weechat_hook_signal_send ("logger_stop",
                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                  jabber_buffer_servers);
        weechat_hook_signal_send ("logger_start",
                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                  jabber_buffer_servers);
        
        for (ptr_server = jabber_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->buffer
                && (ptr_server->buffer != jabber_buffer_servers))
            {
                ptr_buffer = ptr_server->buffer;
                ptr_server->buffer = jabber_buffer_servers;
                weechat_buffer_close (ptr_buffer);
            }
        }
        
        jabber_server_set_buffer_title (jabber_current_server);
        jabber_server_buffer_set_highlight_words (jabber_buffer_servers);
    }
}

/*
 * jabber_buffer_split_server: split the server buffer into many buffers (one by server)
 */

void
jabber_buffer_split_server ()
{
    struct t_jabber_server *ptr_server;
    char buffer_name[256], charset_modifier[256];
    
    if (jabber_buffer_servers)
    {
        weechat_buffer_set (jabber_buffer_servers, "key_unbind_meta-s", "");
    }
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer && (ptr_server != jabber_current_server))
        {
            jabber_server_create_buffer (ptr_server, 0);
        }
    }
    
    if (jabber_current_server)
    {
        snprintf (buffer_name, sizeof (buffer_name),
                  "server.%s", jabber_current_server->name);
        weechat_buffer_set (jabber_current_server->buffer, "name", buffer_name);
        weechat_buffer_set (jabber_current_server->buffer,
                            "short_name", jabber_current_server->name);
        weechat_buffer_set (jabber_current_server->buffer,
                            "localvar_set_server", jabber_current_server->name);
        weechat_buffer_set (jabber_current_server->buffer,
                            "localvar_set_muc", jabber_current_server->name);
        snprintf (charset_modifier, sizeof (charset_modifier),
                  "jabber.%s", jabber_current_server->name);
        weechat_buffer_set (jabber_current_server->buffer,
                            "localvar_set_charset_modifier",
                            charset_modifier);
        weechat_hook_signal_send ("logger_stop",
                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                  jabber_current_server->buffer);
        weechat_hook_signal_send ("logger_start",
                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                  jabber_current_server->buffer);
    }
    
    jabber_buffer_servers = NULL;
    jabber_current_server = NULL;
}

/*
 * jabber_buffer_close_cb: callback called when a buffer is closed
 */

int
jabber_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    JABBER_GET_SERVER_MUC(buffer);
    
    /* make C compiler happy */
    (void) data;
    
    if (ptr_muc)
    {
        /* send PART for channel if its buffer is closed */
        //if ((ptr_channel->type == JABBER_CHANNEL_TYPE_CHANNEL)
        //    && (ptr_channel->nicks))
        //{
        //    jabber_command_part_channel (ptr_server, ptr_channel->name, NULL);
        //}
        jabber_muc_free (ptr_server, ptr_muc);
    }
    else
    {
        if (ptr_server)
        {
            /* send PART on all channels for server, then disconnect from server */
            //ptr_channel = ptr_server->channels;
            //while (ptr_channel)
            //{
            //    next_channel = ptr_channel->next_channel;
            //    weechat_buffer_close (ptr_channel->buffer);
            //    ptr_channel = next_channel;
            //}
            jabber_server_disconnect (ptr_server, 0);
            ptr_server->buffer = NULL;
        }
    }
    
    if (jabber_buffer_servers == buffer)
        jabber_buffer_servers = NULL;
    if (ptr_server && (jabber_current_server == ptr_server))
        jabber_current_server = NULL;
    
    return WEECHAT_RC_OK;
}
