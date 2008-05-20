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

/* irc-input.c: IRC input data (read from user) */


#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"
#include "irc-color.h"
#include "irc-config.h"


/*
 * irc_input_user_message_display: display user message
 */

void
irc_input_user_message_display (struct t_gui_buffer *buffer, char *text)
{
    struct t_irc_nick *ptr_nick;
    char *text_decoded;
    
    text_decoded = irc_color_decode (text,
                                     weechat_config_boolean (irc_config_network_colors_send));
    
    IRC_GET_SERVER_CHANNEL(buffer);
    
    if (ptr_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
        {
            weechat_printf (buffer,
                            "%s%s",
                            irc_nick_as_prefix (NULL, ptr_server->nick,
                                                IRC_COLOR_CHAT_NICK_SELF),
                            (text_decoded) ? text_decoded : text);
        }
        else
        {
            ptr_nick = irc_nick_search (ptr_channel, ptr_server->nick);
            if (ptr_nick)
            {
                weechat_printf (buffer,
                                "%s%s",
                                irc_nick_as_prefix (ptr_nick, NULL,
                                                    IRC_COLOR_CHAT_NICK_SELF),
                                (text_decoded) ? text_decoded : text);
            }
            else
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: cannot find nick for sending "
                                  "message"),
                                weechat_prefix ("error"), "irc");
            }
        }
    }
    
    if (text_decoded)
        free (text_decoded);
}

/*
 * irc_input_send_user_message: send a PRIVMSG message, and split it
 *                              if > 512 bytes
 */

void
irc_input_send_user_message (struct t_gui_buffer *buffer, char *text)
{
    int max_length;
    char *pos, *pos_next, *pos_max, *next, saved_char, *last_space;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    
    if (!ptr_server || !ptr_channel || !text || !text[0])
        return;
    
    if (!ptr_server->is_connected)
    {
        weechat_printf (buffer,
                        _("%s%s: you are not connected to server"),
                        weechat_prefix ("error"), "irc");
        return;
    }
    
    next = NULL;
    last_space = NULL;
    saved_char = '\0';
    
    max_length = 512 - 16 - 65 - 10 - strlen (ptr_server->nick) -
        strlen (ptr_channel->name);
    
    if (max_length > 0)
    {
        if ((int)strlen (text) > max_length)
        {
            pos = text;
            pos_max = text + max_length;
            while (pos && pos[0])
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
    
    irc_server_sendf_queued (ptr_server, "PRIVMSG %s :%s",
                             ptr_channel->name, text);
    irc_input_user_message_display (buffer, text);
    
    if (next)
    {
        next[0] = saved_char;
        irc_input_send_user_message (buffer, next);
    }
}

/*
 * irc_input_data_cb: callback for input data in a buffer
 */

int
irc_input_data_cb (void *data, struct t_gui_buffer *buffer, char *input_data)
{
    char *data_with_colors;
    
    /* make C compiler happy */
    (void) data;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    
    if (ptr_channel)
    {
        data_with_colors = irc_color_encode (input_data,
                                             weechat_config_boolean (irc_config_network_colors_send));
        
        irc_input_send_user_message (buffer,
                                     (data_with_colors) ? data_with_colors : input_data);
        
        if (data_with_colors)
            free (data_with_colors);
    }
    else
    {
        weechat_printf (buffer,
                        _("%s: this buffer is not a channel!"),
                        "irc");
    }
    
    return WEECHAT_RC_OK;
}
