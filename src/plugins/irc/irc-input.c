/*
 * Copyright (C) 2003-2011 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * irc-input.c: input data management for IRC buffers
 */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-buffer.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-protocol.h"
#include "irc-raw.h"


/*
 * irc_input_user_message_display: display user message
 */

void
irc_input_user_message_display (struct t_gui_buffer *buffer, const char *text)
{
    struct t_irc_nick *ptr_nick;
    char *text_decoded;
    
    text_decoded = irc_color_decode (text,
                                     weechat_config_boolean (irc_config_network_colors_send));
    
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    
    if (ptr_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            ptr_nick = irc_nick_search (ptr_channel, ptr_server->nick);
        else
            ptr_nick = NULL;
        
        weechat_printf_tags (buffer,
                             irc_protocol_tags ("privmsg",
                                                "notify_message,no_highlight",
                                                (ptr_nick) ? ptr_nick->name : ptr_server->nick),
                             "%s%s",
                             irc_nick_as_prefix (ptr_server,
                                                 (ptr_nick) ? ptr_nick : NULL,
                                                 (ptr_nick) ? NULL : ptr_server->nick,
                                                 IRC_COLOR_CHAT_NICK_SELF),
                             (text_decoded) ? text_decoded : text);
    }
    
    if (text_decoded)
        free (text_decoded);
}

/*
 * irc_input_send_user_message: send a PRIVMSG message, and split it if message
 *                              size is > 512 bytes
 *                              Warning: this function makes temporarirly
 *                                       changes in "message"
 */

void
irc_input_send_user_message (struct t_gui_buffer *buffer, int flags,
                             const char *tags, char *message)
{
    int max_length;
    char *pos, *pos_max, *last_space, *pos_next, *next, saved_char;
    
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    
    if (!ptr_server || !ptr_channel || !message || !message[0])
        return;
    
    if (!ptr_server->is_connected)
    {
        weechat_printf (buffer,
                        _("%s%s: you are not connected to server"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return;
    }
    
    next = NULL;
    last_space = NULL;
    saved_char = '\0';
    
    max_length = 512 - 16 - 65 - 10 - strlen (ptr_server->nick) -
        strlen (ptr_channel->name);
    
    if (max_length > 0)
    {
        if ((int)strlen (message) > max_length)
        {
            pos = message;
            pos_max = message + max_length;
            while (pos[0])
            {
                if (pos[0] == ' ')
                    last_space = pos;
                pos_next = weechat_utf8_next_char (pos);
                if (pos_next > pos_max)
                    break;
                pos = pos_next;
            }
            if (last_space && (last_space < pos))
                pos = last_space + 1;
            saved_char = pos[0];
            pos[0] = '\0';
            next = pos;
        }
    }
    
    irc_server_sendf (ptr_server, flags, tags,
                      "PRIVMSG %s :%s", ptr_channel->name, message);
    irc_input_user_message_display (buffer, message);
    
    if (next)
    {
        next[0] = saved_char;
        irc_input_send_user_message (buffer, flags, tags, next);
    }
}

/*
 * irc_input_data: input data in a buffer
 */

int
irc_input_data (struct t_gui_buffer *buffer, const char *input_data, int flags)
{
    const char *ptr_data;
    char *data_with_colors, *msg;
    
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    
    if (buffer == irc_raw_buffer)
    {
        if (weechat_strcasecmp (input_data, "q") == 0)
            weechat_buffer_close (buffer);
    }
    else
    {
        /*
         * if send unknown commands is enabled and that input data is a
         * command, then send this command to IRC server
         */
        if (weechat_config_boolean (irc_config_network_send_unknown_commands)
            && !weechat_string_input_for_buffer (input_data))
        {
            if (ptr_server)
            {
                irc_server_sendf (ptr_server, flags, NULL,
                                  weechat_utf8_next_char (input_data));
            }
            return WEECHAT_RC_OK;
        }
        
        if (ptr_channel)
        {
            ptr_data = weechat_string_input_for_buffer (input_data);
            if (!ptr_data)
                ptr_data = input_data;
            data_with_colors = irc_color_encode (ptr_data,
                                                 weechat_config_boolean (irc_config_network_colors_send));
            
            msg = strdup ((data_with_colors) ? data_with_colors : ptr_data);
            if (msg)
            {
                irc_input_send_user_message (buffer, flags, NULL, msg);
                free (msg);
            }
            
            if (data_with_colors)
                free (data_with_colors);
        }
        else
        {
            weechat_printf (buffer,
                            _("%s%s: this buffer is not a channel!"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_input_data_cb: callback for input data in a buffer
 */

int
irc_input_data_cb (void *data, struct t_gui_buffer *buffer,
                   const char *input_data)
{
    /* make C compiler happy */
    (void) data;
    
    return irc_input_data (buffer, input_data, IRC_SERVER_SEND_OUTQ_PRIO_HIGH);
}

/*
 * irc_input_send_cb: callback for "irc_input_send" signal
 *                    This signal can be used by other plugins/scripts, it
 *                    simulates input or command from user on an IRC buffer
 *                    (it is used for example by Relay plugin)
 *                    Format of signal_data (string) is:
 *                      "server;channel;flags;tags;text"
 *                      - server: server name (required)
 *                      - channel: channel name (optional)
 *                      - flags: flags for irc_server_sendf() (optional)
 *                      - tags: tags for irc_server_sendf() (optional)
 *                      - text: text or command (required)
 */

int
irc_input_send_cb (void *data, const char *signal,
                   const char *type_data, void *signal_data)
{
    const char *ptr_string, *ptr_message;
    char *pos_semicol1, *pos_semicol2, *pos_semicol3, *pos_semicol4, *error;
    char *server, *channel, *flags, *tags;
    long flags_value;
    char *data_with_colors;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    ptr_string = (const char *)signal_data;
    
    server = NULL;
    channel = NULL;
    flags = NULL;
    tags = NULL;
    ptr_message = NULL;
    ptr_server = NULL;
    ptr_channel = NULL;
    
    pos_semicol1 = strchr (ptr_string, ';');
    if (pos_semicol1)
    {
        if (pos_semicol1 > ptr_string + 1)
        {
            server = weechat_strndup (ptr_string, pos_semicol1 - ptr_string);
        }
        pos_semicol2 = strchr (pos_semicol1 + 1, ';');
        if (pos_semicol2)
        {
            if (pos_semicol2 > pos_semicol1 + 1)
            {
                channel = weechat_strndup (pos_semicol1 + 1,
                                           pos_semicol2 - pos_semicol1 - 1);
            }
            pos_semicol3 = strchr (pos_semicol2 + 1, ';');
            if (pos_semicol3)
            {
                if (pos_semicol3 > pos_semicol2 + 1)
                {
                    flags = weechat_strndup (pos_semicol2 + 1,
                                             pos_semicol3 - pos_semicol2 - 1);
                }
                pos_semicol4 = strchr (pos_semicol3 + 1, ';');
                if (pos_semicol4)
                {
                    if (pos_semicol4 > pos_semicol3 + 1)
                    {
                        tags = weechat_strndup (pos_semicol3 + 1,
                                                pos_semicol4 - pos_semicol3 - 1);
                    }
                    ptr_message = pos_semicol4 + 1;
                }
            }
        }
    }
    
    flags_value = IRC_SERVER_SEND_OUTQ_PRIO_HIGH;
    if (flags)
    {
        error = NULL;
        flags_value = strtol (flags, &error, 10);
        if (flags_value < 0)
            flags_value = IRC_SERVER_SEND_OUTQ_PRIO_HIGH;
    }
    
    if (server && ptr_message)
    {
        ptr_server = irc_server_search (server);
        if (ptr_server)
        {
            ptr_buffer = ptr_server->buffer;
            if (channel)
            {
                ptr_channel = irc_channel_search (ptr_server, channel);
                if (ptr_channel)
                    ptr_buffer = ptr_channel->buffer;
            }
            
            /* set tags to use by default */
            irc_server_set_send_default_tags (tags);
            
            /* send text to buffer, or execute command */
            if (weechat_string_input_for_buffer (ptr_message))
            {
                /* text as input */
                irc_input_data (ptr_buffer, ptr_message, flags_value);
            }
            else
            {
                /* command */
                data_with_colors = irc_color_encode (ptr_message,
                                                     weechat_config_boolean (irc_config_network_colors_send));
                weechat_command (ptr_buffer,
                                 (data_with_colors) ? data_with_colors : ptr_message);
                if (data_with_colors)
                    free (data_with_colors);
            }
            
            /* reset tags to use by default */
            irc_server_set_send_default_tags (NULL);
        }
    }
    
    if (server)
        free (server);
    if (channel)
        free (channel);
    if (flags)
        free (flags);
    if (tags)
        free (tags);
    
    return WEECHAT_RC_OK;
}
