/*
 * irc-input.c - input data management for IRC buffers
 *
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-buffer.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-message.h"
#include "irc-nick.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-list.h"
#include "irc-msgbuffer.h"
#include "irc-protocol.h"
#include "irc-raw.h"


/*
 * Displays user message.
 *
 * If ctcp_type == "action", then message is displayed as an action
 * (like command /me), for example:
 *
 *    * | nick is testing
 *
 * If target is a channel or a nick, the message is displayed like this:
 *
 *   nick | test
 *
 * If target is a channel with STATUSMSG (for example "@#test"), the message
 * is displayed with the target, like this (message, action, notice):
 *
 *   Msg(nick) -> @#test: test message for ops
 *   Action -> @#test: nick is testing
 *   Notice(nick) -> @#test: test notice for ops
 *
 * If decode_colors is 1, colors are stripped if the option
 * irc.network.colors_send is off.
 */

void
irc_input_user_message_display (struct t_irc_server *server,
                                time_t date,
                                int date_usec,
                                struct t_hashtable *tags,
                                const char *target,
                                const char *address,
                                const char *command,
                                const char *ctcp_type,
                                const char *text,
                                int decode_colors)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    struct t_irc_nick *ptr_nick;
    struct t_irc_protocol_ctxt ctxt;
    const char *ptr_target;
    char *text2, *text_decoded, str_tags[256], *str_color;
    const char *ptr_text;
    int is_notice, is_action, is_channel, display_target;

    if (!server || !target)
        return;

    memset (&ctxt, 0, sizeof (ctxt));
    ctxt.server = server;
    ctxt.date = date;
    ctxt.date_usec = date_usec;
    ctxt.tags = tags;
    ctxt.address = (char *)address;
    ctxt.command = (char *)command;

    is_notice = (weechat_strcasecmp (command, "notice") == 0);
    is_action = (ctcp_type && (weechat_strcasecmp (ctcp_type, "action") == 0));

    is_channel = 0;
    ptr_channel = NULL;
    display_target = is_notice || (ctcp_type && !is_action);
    ptr_target = target;
    if (irc_server_prefix_char_statusmsg (server, target[0])
        && irc_channel_is_channel (server, target + 1))
    {
        ptr_channel = irc_channel_search (server, target + 1);
        is_channel = 1;
        display_target = 1;
        ptr_target = target + 1;
    }
    else
    {
        ptr_channel = irc_channel_search (server, target);
        if (irc_channel_is_channel (server, target))
            is_channel = 1;
    }

    if (ptr_channel)
    {
        ptr_buffer = ptr_channel->buffer;
    }
    else
    {
        ptr_buffer = irc_msgbuffer_get_target_buffer (
            server, ptr_target, command, NULL, NULL);
        if (!ptr_buffer)
            ptr_buffer = server->buffer;
    }

    if (ptr_buffer == server->buffer)
        display_target = 1;

    text2 = irc_message_hide_password (server, target, text);
    text_decoded = (decode_colors) ?
        irc_color_decode (
            (text2) ? text2 : text,
            weechat_config_boolean (irc_config_network_colors_send)) : NULL;

    ptr_nick = NULL;
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
        ptr_nick = irc_nick_search (server, ptr_channel, server->nick);

    ctxt.nick = (ptr_nick) ? ptr_nick->name : server->nick;
    ctxt.nick_is_me = (irc_server_strcasecmp (server, ctxt.nick, server->nick) == 0);

    if (is_action)
    {
        snprintf (str_tags, sizeof (str_tags),
                  "irc_action,self_msg,notify_none,no_highlight");
    }
    else
    {
        if (display_target)
        {
            snprintf (str_tags, sizeof (str_tags),
                      "%sself_msg,notify_none,no_highlight",
                      (ctcp_type) ? "irc_ctcp," : "");
        }
        else
        {
            str_color = irc_color_for_tags (
                weechat_config_color (
                    weechat_config_get ("weechat.color.chat_nick_self")));
            snprintf (str_tags, sizeof (str_tags),
                      "%sself_msg,notify_none,no_highlight,prefix_nick_%s",
                      (ctcp_type) ? "irc_ctcp," : "",
                      (str_color) ? str_color : "default");
            free (str_color);
        }
    }

    ptr_text = (text_decoded) ? text_decoded : ((text2) ? text2 : text);

    if (is_action)
    {
        if (display_target)
        {
            weechat_printf_datetime_tags (
                ptr_buffer,
                date,
                date_usec,
                irc_protocol_tags (&ctxt, str_tags),
                "%s%s -> %s%s%s: %s%s%s%s%s%s",
                weechat_prefix ("network"),
                /* TRANSLATORS: "Action" is an IRC CTCP "ACTION" sent with /me or /action */
                _("Action"),
                (is_channel) ?
                IRC_COLOR_CHAT_CHANNEL : irc_nick_color_for_msg (server, 0, NULL, target),
                target,
                IRC_COLOR_RESET,
                irc_nick_mode_for_display (server, ptr_nick, 0),
                IRC_COLOR_CHAT_NICK_SELF,
                server->nick,
                (ptr_text && ptr_text[0]) ? IRC_COLOR_RESET : "",
                (ptr_text && ptr_text[0]) ? " " : "",
                (ptr_text && ptr_text[0]) ? ptr_text : "");
        }
        else
        {
            weechat_printf_datetime_tags (
                ptr_buffer,
                date,
                date_usec,
                irc_protocol_tags (&ctxt, str_tags),
                "%s%s%s%s%s%s%s",
                weechat_prefix ("action"),
                irc_nick_mode_for_display (server, ptr_nick, 0),
                IRC_COLOR_CHAT_NICK_SELF,
                server->nick,
                IRC_COLOR_RESET,
                (ptr_text && ptr_text[0]) ? " " : "",
                (ptr_text && ptr_text[0]) ? ptr_text : "");
        }
    }
    else if (ctcp_type)
    {
        weechat_printf_datetime_tags (
            ptr_buffer,
            date,
            date_usec,
            irc_protocol_tags (&ctxt, str_tags),
            _("%sCTCP query to %s%s%s: %s%s%s%s%s"),
            weechat_prefix ("network"),
            (is_channel) ?
            IRC_COLOR_CHAT_CHANNEL : irc_nick_color_for_msg (server, 0, NULL, target),
            target,
            IRC_COLOR_RESET,
            IRC_COLOR_CHAT_CHANNEL,
            ctcp_type,
            IRC_COLOR_RESET,
            (ptr_text && ptr_text[0]) ? " " : "",
            (ptr_text && ptr_text[0]) ? ptr_text : "");
    }
    else if (display_target)
    {
        weechat_printf_datetime_tags (
            ptr_buffer,
            date,
            date_usec,
            irc_protocol_tags (&ctxt, str_tags),
            "%s%s%s%s%s(%s%s%s%s)%s -> %s%s%s: %s",
            weechat_prefix ("network"),
            (is_notice) ? IRC_COLOR_NOTICE : "",
            (is_notice) ?
            /* TRANSLATORS: "Notice" is command name in IRC protocol (translation is frequently the same word) */
            _("Notice") :
            "Msg",
            (is_notice) ? IRC_COLOR_RESET : "",
            IRC_COLOR_CHAT_DELIMITERS,
            irc_nick_mode_for_display (server, ptr_nick, 0),
            IRC_COLOR_CHAT_NICK_SELF,
            server->nick,
            IRC_COLOR_CHAT_DELIMITERS,
            IRC_COLOR_RESET,
            (is_channel) ?
            IRC_COLOR_CHAT_CHANNEL : irc_nick_color_for_msg (server, 0, NULL, target),
            target,
            IRC_COLOR_RESET,
            (ptr_text) ? ptr_text : "");
    }
    else
    {
        weechat_printf_datetime_tags (
            ptr_buffer,
            date,
            date_usec,
            irc_protocol_tags (&ctxt, str_tags),
            "%s%s",
            irc_nick_as_prefix (
                server,
                (ptr_nick) ? ptr_nick : NULL,
                (ptr_nick) ? NULL : server->nick,
                IRC_COLOR_CHAT_NICK_SELF),
            (ptr_text) ? ptr_text : "");
    }

    free (text2);
    free (text_decoded);
}

/*
 * Sends a PRIVMSG message, and split it if message size is > 512 bytes
 * (by default).
 *
 * Warning: this function makes temporary changes in "message".
 */

void
irc_input_send_user_message (struct t_gui_buffer *buffer, int flags,
                             const char *tags, char *message)
{
    int i, action, list_size;
    struct t_arraylist *list_messages;

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
    list_messages = irc_server_sendf (ptr_server,
                                      flags | IRC_SERVER_SEND_RETURN_LIST
                                      | IRC_SERVER_SEND_MULTILINE,
                                      tags,
                                      "PRIVMSG %s :%s",
                                      ptr_channel->name, message);
    if (list_messages)
    {
        /* display only if capability "echo-message" is NOT enabled */
        if (!weechat_hashtable_has_key (ptr_server->cap_list, "echo-message"))
        {
            action = ((strncmp (message, "\01ACTION ", 8) == 0)
                      || (strncmp (message, "\01ACTION\01", 8) == 0));
            list_size = weechat_arraylist_size (list_messages);
            for (i = 0; i < list_size; i++)
            {
                irc_input_user_message_display (
                    ptr_server,
                    0,  /* date */
                    0,  /* date_usec */
                    NULL,  /* tags */
                    ptr_channel->name,
                    NULL,  /* address */
                    "privmsg",
                    (action) ? "action" : NULL,
                    (const char *)weechat_arraylist_get (list_messages, i),
                    1);  /* decode_colors */
            }
        }
        weechat_arraylist_free (list_messages);
    }
}

/*
 * Input data in a buffer.
 */

int
irc_input_data (struct t_gui_buffer *buffer, const char *input_data, int flags,
                int force_user_message)
{
    const char *ptr_data;
    char *data_with_colors, *msg;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    if (buffer == irc_raw_buffer)
    {
        if (weechat_strcmp (input_data, "q") == 0)
            weechat_buffer_close (buffer);
        else
            irc_raw_filter_options (input_data);
    }
    else if (weechat_strcmp (
                 weechat_buffer_get_string (buffer,
                                            "localvar_type"), "list") == 0)
    {
        irc_list_buffer_input_data (buffer, input_data);
    }
    else
    {
        /*
         * if send unknown commands is enabled and that input data is a
         * command, then send this command to IRC server
         */
        if (!force_user_message
            && weechat_config_boolean (irc_config_network_send_unknown_commands)
            && !weechat_string_input_for_buffer (input_data))
        {
            if (ptr_server)
            {
                irc_server_sendf (ptr_server,
                                  flags | IRC_SERVER_SEND_MULTILINE,
                                  NULL,
                                  "%s", weechat_utf8_next_char (input_data));
            }
            return WEECHAT_RC_OK;
        }

        if (ptr_channel)
        {
            ptr_data = input_data;
            if (!force_user_message)
            {
                ptr_data = weechat_string_input_for_buffer (input_data);
                if (!ptr_data)
                    ptr_data = input_data;
            }
            data_with_colors = irc_color_encode (
                ptr_data,
                weechat_config_boolean (irc_config_network_colors_send));

            msg = strdup ((data_with_colors) ? data_with_colors : ptr_data);
            if (msg)
            {
                irc_input_send_user_message (buffer, flags, NULL, msg);
                free (msg);
            }

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
 * Callback for input data in a buffer.
 */

int
irc_input_data_cb (const void *pointer, void *data,
                   struct t_gui_buffer *buffer,
                   const char *input_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return irc_input_data (buffer, input_data,
                           IRC_SERVER_SEND_OUTQ_PRIO_HIGH, 0);
}

/*
 * Callback for signal "irc_input_send" signal.
 *
 * This signal can be used by other plugins/scripts, it simulates input or
 * command from user on an IRC buffer (it is used for example by Relay plugin).
 *
 * Format of signal_data (string) is "server;channel;options;tags;text"
 *   server: server name (required)
 *   channel: channel name (optional)
 *   options: comma-separated list of options (optional):
 *     "priority_high": send with high priority (default)
 *     "priority_low": send with low priority
 *     "user_message": force user message (don't execute a command)
 *   tags: tags for irc_server_sendf() (optional)
 *   text: text or command (required).
 */

int
irc_input_send_cb (const void *pointer, void *data,
                   const char *signal,
                   const char *type_data, void *signal_data)
{
    const char *ptr_string, *ptr_message;
    char *pos_semicol1, *pos_semicol2, *pos_semicol3, *pos_semicol4;
    char *server, *channel, *options, *tags, *data_with_colors, **list_options;
    int i, num_options, flags, force_user_message;
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    ptr_string = (const char *)signal_data;

    server = NULL;
    channel = NULL;
    options = NULL;
    flags = IRC_SERVER_SEND_OUTQ_PRIO_HIGH;
    force_user_message = 0;
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
                    options = weechat_strndup (pos_semicol2 + 1,
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

    if (options && options[0])
    {
        list_options = weechat_string_split (
            options,
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &num_options);
        if (list_options)
        {
            for (i = 0; i < num_options; i++)
            {
                if (strcmp (list_options[i], "priority_high") == 0)
                    flags = IRC_SERVER_SEND_OUTQ_PRIO_HIGH;
                else if (strcmp (list_options[i], "priority_low") == 0)
                    flags = IRC_SERVER_SEND_OUTQ_PRIO_LOW;
                else if (strcmp (list_options[i], "user_message") == 0)
                    force_user_message = 1;
            }
            weechat_string_free_split (list_options);
        }
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
            if (force_user_message
                || weechat_string_input_for_buffer (ptr_message))
            {
                /* text as input */
                irc_input_data (ptr_buffer, ptr_message, flags, 1);
            }
            else
            {
                /* command */
                data_with_colors = irc_color_encode (
                    ptr_message,
                    weechat_config_boolean (irc_config_network_colors_send));
                weechat_command (
                    ptr_buffer,
                    (data_with_colors) ? data_with_colors : ptr_message);
                free (data_with_colors);
            }

            /* reset tags to use by default */
            irc_server_set_send_default_tags (NULL);
        }
    }

    free (server);
    free (channel);
    free (options);
    free (tags);

    return WEECHAT_RC_OK;
}
