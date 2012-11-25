/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
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
 * irc-protocol.c: implementation of IRC protocol,
 *                 according to RFC 1459, 2810, 2811 and 2812
 */

/* this define is needed for strptime() (not on OpenBSD) */
#if !defined(__OpenBSD__)
#define _XOPEN_SOURCE 700
#endif

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-protocol.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-ctcp.h"
#include "irc-ignore.h"
#include "irc-message.h"
#include "irc-mode.h"
#include "irc-msgbuffer.h"
#include "irc-nick.h"
#include "irc-sasl.h"
#include "irc-server.h"


/*
 * irc_protocol_is_numeric_command: return 1 if given string is 100% numeric
 */

int
irc_protocol_is_numeric_command (const char *str)
{
    while (str && str[0])
    {
        if (!isdigit ((unsigned char)str[0]))
            return 0;
        str++;
    }
    return 1;
}

/*
 * irc_protocol_log_level_for_command: get log level for IRC command
 */

int
irc_protocol_log_level_for_command (const char *command)
{
    if (!command || !command[0])
        return 0;

    if ((strcmp (command, "privmsg") == 0)
        || (strcmp (command, "notice") == 0))
        return 1;

    if (strcmp (command, "nick") == 0)
        return 2;

    if ((strcmp (command, "join") == 0)
        || (strcmp (command, "part") == 0)
        || (strcmp (command, "quit") == 0))
        return 4;

    return 3;
}

/*
 * irc_protocol_tags: build tags list with IRC command and optional tags and
 *                    nick
 */

const char *
irc_protocol_tags (const char *command, const char *tags, const char *nick)
{
    static char string[1024];
    int log_level;
    char str_log_level[32];

    str_log_level[0] = '\0';

    if (!command && !tags && !nick)
        return NULL;

    if (command && command[0])
    {
        log_level = irc_protocol_log_level_for_command (command);
        if (log_level > 0)
        {
            snprintf (str_log_level, sizeof (str_log_level),
                      ",log%d", log_level);
        }
    }

    snprintf (string, sizeof (string),
              "%s%s%s%s%s%s%s",
              (command && command[0]) ? "irc_" : "",
              (command && command[0]) ? command : "",
              (tags && tags[0]) ? "," : "",
              (tags && tags[0]) ? tags : "",
              (nick && nick[0]) ? ",nick_" : "",
              (nick && nick[0]) ? nick : "",
              str_log_level);

    return string;
}

/*
 * irc_protocol_cb_authenticate: 'authenticate' message received
 */

IRC_PROTOCOL_CALLBACK(authenticate)
{
    int sasl_mechanism;
    const char *sasl_username, *sasl_password;
    char *answer;

    /*
     * AUTHENTICATE message looks like:
     *   AUTHENTICATE +
     *   AUTHENTICATE QQDaUzXAmVffxuzFy77XWBGwABBQAgdinelBrKZaR3wE7nsIETuTVY=
     */

    IRC_PROTOCOL_MIN_ARGS(2);

    if (irc_server_sasl_enabled (server))
    {
        sasl_mechanism = IRC_SERVER_OPTION_INTEGER(server,
                                                   IRC_SERVER_OPTION_SASL_MECHANISM);
        sasl_username = IRC_SERVER_OPTION_STRING(server,
                                                 IRC_SERVER_OPTION_SASL_USERNAME);
        sasl_password = IRC_SERVER_OPTION_STRING(server,
                                                 IRC_SERVER_OPTION_SASL_PASSWORD);
        answer = NULL;
        switch (sasl_mechanism)
        {
            case IRC_SASL_MECHANISM_DH_BLOWFISH:
                answer = irc_sasl_mechanism_dh_blowfish (argv_eol[1],
                                                         sasl_username,
                                                         sasl_password);
                break;
            case IRC_SASL_MECHANISM_EXTERNAL:
                answer = strdup ("+");
                break;
            case IRC_SASL_MECHANISM_PLAIN:
            default:
                answer = irc_sasl_mechanism_plain (sasl_username,
                                                   sasl_password);
                break;
        }
        if (answer)
        {
            irc_server_sendf (server, 0, NULL, "AUTHENTICATE %s", answer);
            free (answer);
        }
        else
        {
            weechat_printf (server->buffer,
                            _("%s%s: error building answer for "
                              "SASL authentication, using mechanism \"%s\""),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            irc_sasl_mechanism_string[IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_SASL_MECHANISM)]);
            irc_server_sendf (server, 0, NULL, "CAP END");
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_cap: 'cap' message received (client capability)
 */

IRC_PROTOCOL_CALLBACK(cap)
{
    char *ptr_caps, **caps_supported, **caps_requested, *cap_option, *cap_req;
    const char *ptr_cap_option;
    int num_caps_supported, num_caps_requested, sasl_requested, sasl_to_do;
    int i, j, timeout, length;

    /*
     * CAP message looks like:
     *   :server CAP * LS :identify-msg multi-prefix sasl
     *   :server CAP * ACK :sasl
     *   :server CAP * NAK :sasl
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    if (strcmp (argv[3], "LS") == 0)
    {
        if (argc > 4)
        {
            ptr_caps = (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4];
            weechat_printf_date_tags (server->buffer, date, NULL,
                                      _("%s%s: client capability, server "
                                        "supports: %s"),
                                      weechat_prefix ("network"),
                                      IRC_PLUGIN_NAME,
                                      ptr_caps);

            /* auto-enable capabilities only when connecting to server */
            if (!server->is_connected)
            {
                sasl_requested = irc_server_sasl_enabled (server);
                sasl_to_do = 0;
                ptr_cap_option = IRC_SERVER_OPTION_STRING(server,
                                                          IRC_SERVER_OPTION_CAPABILITIES);
                length = ((ptr_cap_option && ptr_cap_option[0]) ? strlen (ptr_cap_option) : 0) + 16;
                cap_option = malloc (length);
                cap_req = malloc (length);
                if (cap_option && cap_req)
                {
                    cap_option[0] = '\0';
                    if (ptr_cap_option && ptr_cap_option[0])
                        strcat (cap_option, ptr_cap_option);
                    if (sasl_requested)
                    {
                        if (cap_option[0])
                            strcat (cap_option, ",");
                        strcat (cap_option, "sasl");
                    }
                    cap_req[0] = '\0';
                    caps_requested = weechat_string_split (cap_option, ",", 0, 0,
                                                           &num_caps_requested);
                    caps_supported = weechat_string_split (ptr_caps, " ", 0, 0,
                                                           &num_caps_supported);
                    if (caps_requested && caps_supported)
                    {
                        for (i = 0; i < num_caps_requested; i++)
                        {
                            for (j = 0; j < num_caps_supported; j++)
                            {
                                if (weechat_strcasecmp (caps_requested[i],
                                                        caps_supported[j]) == 0)
                                {
                                    if (strcmp (caps_requested[i], "sasl") == 0)
                                        sasl_to_do = 1;
                                    if (cap_req[0])
                                        strcat (cap_req, " ");
                                    strcat (cap_req, caps_supported[j]);
                                }
                            }
                        }
                    }
                    if (caps_requested)
                        weechat_string_free_split (caps_requested);
                    if (caps_supported)
                        weechat_string_free_split (caps_supported);
                    if (cap_req[0])
                    {
                        weechat_printf (server->buffer,
                                        _("%s%s: client capability, requesting: %s"),
                                        weechat_prefix ("network"),
                                        IRC_PLUGIN_NAME,
                                        cap_req);
                        irc_server_sendf (server, 0, NULL,
                                          "CAP REQ :%s", cap_req);
                    }
                    if (!sasl_to_do)
                        irc_server_sendf (server, 0, NULL, "CAP END");
                    if (sasl_requested && !sasl_to_do)
                    {
                        weechat_printf (server->buffer,
                                        _("%s%s: client capability: sasl not supported"),
                                        weechat_prefix ("network"),
                                        IRC_PLUGIN_NAME);
                    }
                }
                if (cap_option)
                    free (cap_option);
                if (cap_req)
                    free (cap_req);
            }
        }
    }
    else if (strcmp (argv[3], "ACK") == 0)
    {
        if (argc > 4)
        {
            ptr_caps = (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4];
            weechat_printf_date_tags (server->buffer, date, NULL,
                                      _("%s%s: client capability, enabled: %s"),
                                      weechat_prefix ("network"), IRC_PLUGIN_NAME,
                                      ptr_caps);
            sasl_to_do = 0;
            caps_supported = weechat_string_split (ptr_caps, " ", 0, 0,
                                                   &num_caps_supported);
            if (caps_supported)
            {
                for (i = 0; i < num_caps_supported; i++)
                {
                    if (strcmp (caps_supported[i], "sasl") == 0)
                    {
                        sasl_to_do = 1;
                        break;
                    }
                }
                weechat_string_free_split (caps_supported);
            }
            if (sasl_to_do)
            {
                switch (IRC_SERVER_OPTION_INTEGER(server,
                                                  IRC_SERVER_OPTION_SASL_MECHANISM))
                {
                    case IRC_SASL_MECHANISM_DH_BLOWFISH:
                        irc_server_sendf (server, 0, NULL,
                                          "AUTHENTICATE DH-BLOWFISH");
                        break;
                    case IRC_SASL_MECHANISM_EXTERNAL:
                        irc_server_sendf (server, 0, NULL,
                                          "AUTHENTICATE EXTERNAL");
                        break;
                    case IRC_SASL_MECHANISM_PLAIN:
                    default:
                        irc_server_sendf (server, 0, NULL,
                                          "AUTHENTICATE PLAIN");
                        break;
                }
                if (server->hook_timer_sasl)
                    weechat_unhook (server->hook_timer_sasl);
                timeout = IRC_SERVER_OPTION_INTEGER(server,
                                                    IRC_SERVER_OPTION_SASL_TIMEOUT);
                server->hook_timer_sasl = weechat_hook_timer (timeout * 1000,
                                                              0, 1,
                                                              &irc_server_timer_sasl_cb,
                                                              server);
            }
        }
    }
    else if (strcmp (argv[3], "NAK") == 0)
    {
        if (argc > 4)
        {
            ptr_caps = (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4];
            weechat_printf_date_tags (server->buffer, date, NULL,
                                      _("%s%s: client capability, refused: %s"),
                                      weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                      ptr_caps);
            if (!server->is_connected)
                irc_server_sendf (server, 0, NULL, "CAP END");
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_error: error received from server
 */

IRC_PROTOCOL_CALLBACK(error)
{
    char *ptr_args;

    /*
     * ERROR message looks like:
     *   ERROR :Closing Link: irc.server.org (Bad Password)
     */

    IRC_PROTOCOL_MIN_ARGS(2);

    ptr_args = (argv_eol[1][0] == ':') ? argv_eol[1] + 1 : argv_eol[1];

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, NULL,
                                                               NULL),
                              date,
                              irc_protocol_tags (command, NULL, NULL),
                              "%s%s",
                              weechat_prefix ("error"),
                              ptr_args);

    if (strncmp (ptr_args, "Closing Link", 12) == 0)
    {
        irc_server_disconnect (server, !server->is_connected, 1);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_generic_error: generic error (callback used by many error
 *                                messages, but not for message "ERROR")
 */

IRC_PROTOCOL_CALLBACK(generic_error)
{
    int first_arg;
    char *chan_nick, *args;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /*
     * example of error:
     *   :server 404 nick #channel :Cannot send to channel
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    first_arg = (irc_server_strcasecmp (server, argv[2], server->nick) == 0) ? 3 : 2;

    if ((argv[first_arg][0] != ':') && argv[first_arg + 1])
    {
        chan_nick = argv[first_arg];
        args = argv_eol[first_arg + 1];
    }
    else
    {
        chan_nick = NULL;
        args = argv_eol[first_arg];
    }
    if (args[0] == ':')
        args++;

    ptr_channel = NULL;
    if (chan_nick)
        ptr_channel = irc_channel_search (server, chan_nick);

    ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, NULL,
                                                               ptr_buffer),
                              date,
                              irc_protocol_tags (command, NULL, NULL),
                              "%s%s%s%s%s%s",
                              weechat_prefix ("network"),
                              (ptr_channel && chan_nick
                               && (irc_server_strcasecmp (server, chan_nick,
                                                          ptr_channel->name) == 0)) ?
                              IRC_COLOR_CHAT_CHANNEL : "",
                              (chan_nick) ? chan_nick : "",
                              IRC_COLOR_RESET,
                              (chan_nick) ? ": " : "",
                              args);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_invite: 'invite' message received
 */

IRC_PROTOCOL_CALLBACK(invite)
{
    /*
     * INVITE message looks like:
     *   :nick!user@host INVITE mynick :#channel
     */

    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;

    if (!ignored)
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, nick,
                                                                   command, NULL,
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "notify_highlight", NULL),
                                  _("%sYou have been invited to %s%s%s by "
                                    "%s%s%s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  (argv[3][0] == ':') ? argv[3] + 1 : argv[3],
                                  IRC_COLOR_RESET,
                                  irc_nick_color_for_server_message (server, NULL, nick),
                                  nick,
                                  IRC_COLOR_RESET);
    }
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_join: 'join' message received
 */

IRC_PROTOCOL_CALLBACK(join)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_channel_speaking *ptr_nick_speaking;
    char *pos_channel;
    int local_join, display_host;

    /*
     * JOIN message looks like:
     *   :nick!user@host JOIN :#channel
     */

    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;

    local_join = (irc_server_strcasecmp (server, nick, server->nick) == 0);

    pos_channel = (argv[2][0] == ':') ? argv[2] + 1 : argv[2];

    ptr_channel = irc_channel_search (server, pos_channel);
    if (ptr_channel)
    {
        ptr_channel->part = 0;
    }
    else
    {
        /*
         * if someone else joins and channel is not opened, then just
         * ignore it (we should receive our self join first)
         */
        if (!local_join)
            return WEECHAT_RC_OK;

        ptr_channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL,
                                       pos_channel, 1, 1);
        if (!ptr_channel)
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot create new channel \"%s\""),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            pos_channel);
            return WEECHAT_RC_ERROR;
        }
    }

    /*
     * local join? clear nicklist to be sure it is empty (when using znc, after
     * reconnection to network, we receive a JOIN for channel with existing
     * nicks in irc plugin, so we need to clear the nicklist now)
     */
    if (local_join)
        irc_nick_free_all (server, ptr_channel);

    /* reset some variables if joining new channel */
    if (!ptr_channel->nicks)
    {
        irc_channel_set_topic (ptr_channel, NULL);
        if (ptr_channel->modes)
        {
            free (ptr_channel->modes);
            ptr_channel->modes = NULL;
        }
        ptr_channel->limit = 0;
        ptr_channel->names_received = 0;
        ptr_channel->checking_away = 0;
    }

    /* add nick in channel */
    ptr_nick = irc_nick_new (server, ptr_channel, nick, NULL, 0);
    if (ptr_nick)
        ptr_nick->host = strdup (address);

    if (!ignored)
    {
        ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                             && (weechat_config_boolean (irc_config_look_smart_filter_join))) ?
            irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
        display_host = (local_join) ?
            weechat_config_boolean (irc_config_look_display_host_join_local) :
            weechat_config_boolean (irc_config_look_display_host_join);
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, NULL,
                                                                   ptr_channel->buffer),
                                  date,
                                  irc_protocol_tags (command,
                                                     (local_join
                                                      || !weechat_config_boolean (irc_config_look_smart_filter)
                                                      || !weechat_config_boolean (irc_config_look_smart_filter_join)
                                                      || ptr_nick_speaking) ?
                                                     NULL : "irc_smart_filter",
                                                     nick),
                                  _("%s%s%s%s%s%s%s%s%s%s has joined %s%s%s"),
                                  weechat_prefix ("join"),
                                  irc_nick_color_for_server_message (server, ptr_nick, nick),
                                  nick,
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  (display_host) ? " (" : "",
                                  IRC_COLOR_CHAT_HOST,
                                  (display_host) ? address : "",
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  (display_host) ? ")" : "",
                                  IRC_COLOR_MESSAGE_JOIN,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  pos_channel,
                                  IRC_COLOR_MESSAGE_JOIN);

        /* display message in private if private has flag "has_quit_server" */
        if (!local_join)
            irc_channel_display_nick_back_in_pv (server, ptr_nick, nick);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_kick: 'kick' message received
 */

IRC_PROTOCOL_CALLBACK(kick)
{
    char *pos_comment;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick, *ptr_nick_kicked;

    /*
     * KICK message looks like:
     *   :nick1!user@host KICK #channel nick2 :kick reason
     */

    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;

    pos_comment = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;

    ptr_channel = irc_channel_search (server, argv[2]);
    if (!ptr_channel)
        return WEECHAT_RC_OK;

    ptr_nick = irc_nick_search (server, ptr_channel, nick);
    ptr_nick_kicked = irc_nick_search (server, ptr_channel, argv[3]);

    if (pos_comment)
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, NULL,
                                                                   ptr_channel->buffer),
                                  date,
                                  irc_protocol_tags (command, NULL, NULL),
                                  _("%s%s%s%s has kicked %s%s%s %s(%s%s%s)"),
                                  weechat_prefix ("quit"),
                                  irc_nick_color_for_server_message (server, ptr_nick, nick),
                                  nick,
                                  IRC_COLOR_MESSAGE_QUIT,
                                  irc_nick_color_for_server_message (server, ptr_nick_kicked, argv[3]),
                                  argv[3],
                                  IRC_COLOR_MESSAGE_QUIT,
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  pos_comment,
                                  IRC_COLOR_CHAT_DELIMITERS);
    }
    else
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, NULL,
                                                                   ptr_channel->buffer),
                                  date,
                                  irc_protocol_tags (command, NULL, NULL),
                                  _("%s%s%s%s has kicked %s%s%s"),
                                  weechat_prefix ("quit"),
                                  irc_nick_color_for_server_message (server, ptr_nick, nick),
                                  nick,
                                  IRC_COLOR_MESSAGE_QUIT,
                                  irc_nick_color_for_server_message (server, ptr_nick_kicked, argv[3]),
                                  argv[3],
                                  IRC_COLOR_MESSAGE_QUIT);
    }

    if (irc_server_strcasecmp (server, argv[3], server->nick) == 0)
    {
        /*
         * my nick was kicked => free all nicks, channel is not active any
         * more
         */
        irc_nick_free_all (server, ptr_channel);
        if (IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTOREJOIN))
        {
            if (IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTOREJOIN_DELAY) == 0)
            {
                /* immediately rejoin if delay is 0 */
                irc_channel_rejoin (server, ptr_channel);
            }
            else
            {
                /* rejoin channel later, according to delay */
                ptr_channel->hook_autorejoin =
                    weechat_hook_timer (IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTOREJOIN_DELAY) * 1000,
                                        0, 1,
                                        &irc_channel_autorejoin_cb,
                                        ptr_channel);
            }
        }
    }
    else
    {
        /*
         * someone was kicked from channel (but not me) => remove only this
         * nick
         */
        if (ptr_nick_kicked)
            irc_nick_free (server, ptr_channel, ptr_nick_kicked);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_kill: 'kill' message received
 */

IRC_PROTOCOL_CALLBACK(kill)
{
    char *pos_comment;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick, *ptr_nick_killed;

    /*
     * KILL message looks like:
     *   :nick1!user@host KILL mynick :kill reason
     */

    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;

    pos_comment = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        ptr_nick = irc_nick_search (server, ptr_channel, nick);
        ptr_nick_killed = irc_nick_search (server, ptr_channel, argv[2]);

        if (pos_comment)
        {
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, NULL,
                                                                       ptr_channel->buffer),
                                      date,
                                      irc_protocol_tags (command, NULL, NULL),
                                      _("%s%sYou were killed by %s%s%s %s(%s%s%s)"),
                                      weechat_prefix ("quit"),
                                      IRC_COLOR_MESSAGE_QUIT,
                                      irc_nick_color_for_server_message (server, ptr_nick, nick),
                                      nick,
                                      IRC_COLOR_MESSAGE_QUIT,
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_RESET,
                                      pos_comment,
                                      IRC_COLOR_CHAT_DELIMITERS);
        }
        else
        {
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, NULL,
                                                                       ptr_channel->buffer),
                                      date,
                                      irc_protocol_tags (command, NULL, NULL),
                                      _("%s%sYou were killed by %s%s%s"),
                                      weechat_prefix ("quit"),
                                      IRC_COLOR_MESSAGE_QUIT,
                                      irc_nick_color_for_server_message (server, ptr_nick, nick),
                                      nick,
                                      IRC_COLOR_MESSAGE_QUIT);
        }

        if (irc_server_strcasecmp (server, argv[2], server->nick) == 0)
        {
            /*
             * my nick was killed => free all nicks, channel is not active any
             * more
             */
            irc_nick_free_all (server, ptr_channel);
        }
        else
        {
            /*
             * someone was killed on channel (but not me) => remove only this
             * nick
             */
            if (ptr_nick_killed)
                irc_nick_free (server, ptr_channel, ptr_nick_killed);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_mode: 'mode' message received
 */

IRC_PROTOCOL_CALLBACK(mode)
{
    char *pos_modes;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_gui_buffer *ptr_buffer;

    /*
     * MODE message looks like:
     *   :nick!user@host MODE #test +o nick
     */

    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;

    pos_modes = (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3];

    if (irc_channel_is_channel (server, argv[2]))
    {
        ptr_channel = irc_channel_search (server, argv[2]);
        if (ptr_channel)
            irc_mode_channel_set (server, ptr_channel, pos_modes);
        ptr_nick = irc_nick_search (server, ptr_channel, nick);
        ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, NULL,
                                                                   ptr_buffer),
                                  date,
                                  irc_protocol_tags (command, NULL, NULL),
                                  _("%sMode %s%s %s[%s%s%s]%s by %s%s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  (ptr_channel) ? ptr_channel->name : argv[2],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  pos_modes,
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  irc_nick_color_for_server_message (server, ptr_nick, nick),
                                  nick);
    }
    else
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, NULL,
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, NULL, NULL),
                                  _("%sUser mode %s[%s%s%s]%s by %s%s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  pos_modes,
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  irc_nick_color_for_server_message (server, NULL, nick),
                                  nick);
        irc_mode_user_set (server, pos_modes, 0);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_nick: 'nick' message received
 */

IRC_PROTOCOL_CALLBACK(nick)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick, *ptr_nick_found;
    char *new_nick, *old_color, *buffer_name;
    int local_nick;
    struct t_irc_channel_speaking *ptr_nick_speaking;

    /*
     * NICK message looks like:
     *   :oldnick!user@host NICK :newnick
     */

    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;

    new_nick = (argv[2][0] == ':') ? argv[2] + 1 : argv[2];

    local_nick = (irc_server_strcasecmp (server, nick, server->nick) == 0) ? 1 : 0;

    if (local_nick)
        irc_server_set_nick (server, new_nick);

    ptr_nick_found = NULL;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        switch (ptr_channel->type)
        {
            case IRC_CHANNEL_TYPE_PRIVATE:
                /* rename private window if this is with "old nick" */
                if ((irc_server_strcasecmp (server, ptr_channel->name, nick) == 0)
                    && !irc_channel_search (server, new_nick))
                {
                    free (ptr_channel->name);
                    ptr_channel->name = strdup (new_nick);
                    if (ptr_channel->pv_remote_nick_color)
                    {
                        free (ptr_channel->pv_remote_nick_color);
                        ptr_channel->pv_remote_nick_color = NULL;
                    }
                    buffer_name = irc_buffer_build_name (server->name, ptr_channel->name);
                    weechat_buffer_set (ptr_channel->buffer, "name", buffer_name);
                    weechat_buffer_set (ptr_channel->buffer, "short_name",
                                        ptr_channel->name);
                    weechat_buffer_set (ptr_channel->buffer,
                                        "localvar_set_channel", ptr_channel->name);
                }
                break;
            case IRC_CHANNEL_TYPE_CHANNEL:
                /* rename nick in nicklist if found */
                ptr_nick = irc_nick_search (server, ptr_channel, nick);
                if (ptr_nick)
                {
                    ptr_nick_found = ptr_nick;

                    /* temporary disable hotlist */
                    weechat_buffer_set (NULL, "hotlist", "-");

                    /* set host for nick if needed */
                    if (ptr_nick && !ptr_nick->host)
                        ptr_nick->host = strdup (address);

                    /* change nick and display message on all channels */
                    old_color = strdup (ptr_nick->color);
                    irc_nick_change (server, ptr_channel, ptr_nick, new_nick);
                    if (local_nick)
                    {
                        weechat_printf_date_tags (ptr_channel->buffer,
                                                  date,
                                                  irc_protocol_tags (command, NULL, NULL),
                                                  _("%sYou are now known as "
                                                    "%s%s%s"),
                                                  weechat_prefix ("network"),
                                                  IRC_COLOR_CHAT_NICK_SELF,
                                                  new_nick,
                                                  IRC_COLOR_RESET);
                    }
                    else
                    {
                        if (!irc_ignore_check (server, ptr_channel->name,
                                               nick, host))
                        {
                            ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                                                 && (weechat_config_boolean (irc_config_look_smart_filter_nick))) ?
                                irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
                            weechat_printf_date_tags (ptr_channel->buffer,
                                                      date,
                                                      irc_protocol_tags (command,
                                                                         (!weechat_config_boolean (irc_config_look_smart_filter)
                                                                          || !weechat_config_boolean (irc_config_look_smart_filter_nick)
                                                                          || ptr_nick_speaking) ?
                                                                         NULL : "irc_smart_filter",
                                                                         NULL),
                                                      _("%s%s%s%s is now known as "
                                                        "%s%s%s"),
                                                      weechat_prefix ("network"),
                                                      weechat_config_boolean(irc_config_look_color_nicks_in_server_messages) ?
                                                      old_color : IRC_COLOR_CHAT_NICK,
                                                      nick,
                                                      IRC_COLOR_RESET,
                                                      irc_nick_color_for_message (server, ptr_nick, new_nick),
                                                      new_nick,
                                                      IRC_COLOR_RESET);
                        }
                        irc_channel_nick_speaking_rename (ptr_channel,
                                                          nick, new_nick);
                        irc_channel_nick_speaking_time_rename (server, ptr_channel,
                                                               nick, new_nick);
                    }

                    if (old_color)
                        free (old_color);

                    /* enable hotlist */
                    weechat_buffer_set (NULL, "hotlist", "+");
                }
                break;
        }
    }

    if (!local_nick)
        irc_channel_display_nick_back_in_pv (server, ptr_nick_found, new_nick);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_notice: 'notice' message received
 */

IRC_PROTOCOL_CALLBACK(notice)
{
    char *pos_target, *pos_args;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    int notify_private, is_channel, notice_op, notice_voice;
    struct t_gui_buffer *ptr_buffer;

    /*
     * NOTICE message looks like:
     *   NOTICE AUTH :*** Looking up your hostname...
     *   :nick!user@host NOTICE mynick :notice text
     *   :nick!user@host NOTICE #channel :notice text
     */

    IRC_PROTOCOL_MIN_ARGS(3);

    if (ignored)
        return WEECHAT_RC_OK;

    notice_op = 0;
    notice_voice = 0;

    if (argv[0][0] == ':')
    {
        if (argc < 4)
            return WEECHAT_RC_ERROR;
        pos_target = argv[2];
        is_channel = irc_channel_is_channel (server, pos_target + 1);
        if ((pos_target[0] == '@') && is_channel)
        {
            pos_target++;
            notice_op = 1;
        }
        else if ((pos_target[0] == '+') && is_channel)
        {
            pos_target++;
            notice_voice = 1;
        }
        pos_args = (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3];
        if (notice_op && (pos_args[0] == '@') && (pos_args[1] == ' '))
            pos_args += 2;
        else if (notice_voice && (pos_args[0] == '+') && (pos_args[1] == ' '))
            pos_args += 2;
    }
    else
    {
        pos_target = NULL;
        pos_args = (argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2];
    }

    if (nick && (pos_args[0] == '\01')
        && (pos_args[strlen (pos_args) - 1] == '\01'))
    {
        irc_ctcp_display_reply_from_nick (server, date, command, nick, pos_args);
    }
    else
    {
        if (pos_target && irc_channel_is_channel (server, pos_target))
        {
            /* notice for channel */
            ptr_channel = irc_channel_search (server, pos_target);
            ptr_nick = irc_nick_search (server, ptr_channel, nick);
            weechat_printf_date_tags ((ptr_channel) ? ptr_channel->buffer : server->buffer,
                                      date,
                                      irc_protocol_tags (command,
                                                         "notify_message",
                                                         nick),
                                      "%s%s%s%s%s(%s%s%s)%s: %s",
                                      weechat_prefix ("network"),
                                      IRC_COLOR_NOTICE,
                                      /* TRANSLATORS: "Notice" is command name in IRC protocol (translation is frequently the same word) */
                                      _("Notice"),
                                      (notice_op) ? "Op" : ((notice_voice) ? "Voice" : ""),
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      irc_nick_color_for_message (server, ptr_nick, nick),
                                      (nick && nick[0]) ? nick : "?",
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_RESET,
                                      pos_args);
        }
        else
        {
            /* notice for user */
            notify_private = 0;
            if (nick
                && (weechat_strcasecmp (nick, "nickserv") != 0)
                && (weechat_strcasecmp (nick, "chanserv") != 0)
                && (weechat_strcasecmp (nick, "memoserv") != 0))
            {
                notify_private = 1;
            }

            ptr_channel = NULL;
            if (nick && weechat_config_integer (irc_config_look_notice_as_pv) != IRC_CONFIG_LOOK_NOTICE_AS_PV_NEVER)
            {
                ptr_channel = irc_channel_search (server, nick);
                if (!ptr_channel
                    && weechat_config_integer (irc_config_look_notice_as_pv) == IRC_CONFIG_LOOK_NOTICE_AS_PV_ALWAYS)
                {
                    ptr_channel = irc_channel_new (server,
                                                   IRC_CHANNEL_TYPE_PRIVATE,
                                                   nick, 0, 0);
                    if (!ptr_channel)
                    {
                        weechat_printf (server->buffer,
                                        _("%s%s: cannot create new "
                                          "private buffer \"%s\""),
                                        weechat_prefix ("error"),
                                        IRC_PLUGIN_NAME, nick);
                    }
                }
            }

            if (ptr_channel)
            {
                if (!ptr_channel->topic)
                    irc_channel_set_topic (ptr_channel, address);

                weechat_printf_date_tags (ptr_channel->buffer,
                                          date,
                                          irc_protocol_tags (command,
                                                             "notify_private",
                                                             nick),
                                          "%s%s%s%s: %s",
                                          weechat_prefix ("network"),
                                          irc_nick_color_for_message (server, NULL,
                                                                      nick),
                                          nick,
                                          IRC_COLOR_RESET,
                                          pos_args);
                if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
                    && ptr_channel->has_quit_server)
                {
                    ptr_channel->has_quit_server = 0;
                }
            }
            else
            {
                ptr_buffer = irc_msgbuffer_get_target_buffer (server, nick,
                                                              command, NULL,
                                                              NULL);
                /*
                 * if notice is sent from myself (for example another WeeChat
                 * via relay), then display message of outgoing notice
                 */
                if (nick && (irc_server_strcasecmp (server, server->nick, nick) == 0))
                {
                    weechat_printf_date_tags (ptr_buffer,
                                              date,
                                              irc_protocol_tags (command,
                                                                 (notify_private) ? "notify_private" : NULL,
                                                                 server->nick),
                                              "%s%s%s%s -> %s%s%s: %s",
                                              weechat_prefix ("network"),
                                              IRC_COLOR_NOTICE,
                                              /* TRANSLATORS: "Notice" is command name in IRC protocol (translation is frequently the same word) */
                                              _("Notice"),
                                              IRC_COLOR_RESET,
                                              irc_nick_color_for_message (server,
                                                                          NULL,
                                                                          pos_target),
                                              pos_target,
                                              IRC_COLOR_RESET,
                                              pos_args);
                }
                else
                {
                    if (address && address[0])
                    {
                        weechat_printf_date_tags (ptr_buffer,
                                                  date,
                                                  irc_protocol_tags (command,
                                                                     (notify_private) ? "notify_private" : NULL,
                                                                     nick),
                                                  "%s%s%s %s(%s%s%s)%s: %s",
                                                  weechat_prefix ("network"),
                                                  irc_nick_color_for_message (server,
                                                                              NULL,
                                                                              nick),
                                                  nick,
                                                  IRC_COLOR_CHAT_DELIMITERS,
                                                  IRC_COLOR_CHAT_HOST,
                                                  address,
                                                  IRC_COLOR_CHAT_DELIMITERS,
                                                  IRC_COLOR_RESET,
                                                  pos_args);
                    }
                    else
                    {
                        if (nick && nick[0])
                        {
                            weechat_printf_date_tags (ptr_buffer,
                                                      date,
                                                      irc_protocol_tags (command,
                                                                         (notify_private) ? "notify_private" : NULL,
                                                                         nick),
                                                      "%s%s%s%s: %s",
                                                      weechat_prefix ("network"),
                                                      irc_nick_color_for_message (server,
                                                                                  NULL,
                                                                                  nick),
                                                      nick,
                                                      IRC_COLOR_RESET,
                                                      pos_args);
                        }
                        else
                        {
                            weechat_printf_date_tags (ptr_buffer,
                                                      date,
                                                      irc_protocol_tags (command,
                                                                         (notify_private) ? "notify_private" : NULL,
                                                                         NULL),
                                                      "%s%s",
                                                      weechat_prefix ("network"),
                                                      pos_args);
                        }
                    }
                }
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_part: 'part' message received
 */

IRC_PROTOCOL_CALLBACK(part)
{
    char *pos_comment, *join_string;
    int join_length, local_part, display_host;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_channel_speaking *ptr_nick_speaking;

    /*
     * PART message looks like:
     *   :nick!user@host PART #channel :part message
     * On undernet server, it can be:
     *   :nick!user@host PART :#channel
     *   :nick!user@host PART #channel :part message
     */

    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;

    pos_comment = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;

    ptr_channel = irc_channel_search (server,
                                      (argv[2][0] == ':') ? argv[2] + 1 : argv[2]);
    if (ptr_channel)
    {
        ptr_nick = irc_nick_search (server, ptr_channel, nick);
        if (ptr_nick)
        {
            local_part = (irc_server_strcasecmp (server, nick, server->nick) == 0);

            /* display part message */
            if (!ignored)
            {
                ptr_nick_speaking = NULL;
                if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                {
                    ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                                         && (weechat_config_boolean (irc_config_look_smart_filter_quit))) ?
                        irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
                }
                display_host = weechat_config_boolean (irc_config_look_display_host_quit);
                if (pos_comment)
                {
                    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                               command, NULL,
                                                                               ptr_channel->buffer),
                                              date,
                                              irc_protocol_tags (command,
                                                                 (local_part
                                                                  || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                                                                  || !weechat_config_boolean (irc_config_look_smart_filter)
                                                                  || !weechat_config_boolean (irc_config_look_smart_filter_quit)
                                                                  || ptr_nick_speaking) ?
                                                                 NULL : "irc_smart_filter",
                                                                 nick),
                                              _("%s%s%s%s%s%s%s%s%s%s has left %s%s%s "
                                                "%s(%s%s%s)"),
                                              weechat_prefix ("quit"),
                                              irc_nick_color_for_server_message (server, ptr_nick, nick),
                                              nick,
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              (display_host) ? " (" : "",
                                              IRC_COLOR_CHAT_HOST,
                                              (display_host) ? address : "",
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              (display_host) ? ")" : "",
                                              IRC_COLOR_MESSAGE_QUIT,
                                              IRC_COLOR_CHAT_CHANNEL,
                                              ptr_channel->name,
                                              IRC_COLOR_MESSAGE_QUIT,
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              IRC_COLOR_REASON_QUIT,
                                              pos_comment,
                                              IRC_COLOR_CHAT_DELIMITERS);
                }
                else
                {
                    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                               command, NULL,
                                                                               ptr_channel->buffer),
                                              date,
                                              irc_protocol_tags (command,
                                                                 (local_part
                                                                  || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                                                                  || !weechat_config_boolean (irc_config_look_smart_filter)
                                                                  || !weechat_config_boolean (irc_config_look_smart_filter_quit)
                                                                  || ptr_nick_speaking) ?
                                                                 NULL : "irc_smart_filter",
                                                                 nick),
                                              _("%s%s%s%s%s%s%s%s%s%s has left "
                                                "%s%s%s"),
                                              weechat_prefix ("quit"),
                                              irc_nick_color_for_server_message (server, ptr_nick, nick),
                                              nick,
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              (display_host) ? " (" : "",
                                              IRC_COLOR_CHAT_HOST,
                                              (display_host) ? address : "",
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              (display_host) ? ")" : "",
                                              IRC_COLOR_MESSAGE_QUIT,
                                              IRC_COLOR_CHAT_CHANNEL,
                                              ptr_channel->name,
                                              IRC_COLOR_MESSAGE_QUIT);
                }
            }

            /* part request was issued by local client ? */
            if (local_part)
            {
                irc_nick_free_all (server, ptr_channel);

                /* cycling ? => rejoin channel immediately */
                if (ptr_channel->cycle)
                {
                    ptr_channel->cycle = 0;
                    if (ptr_channel->key)
                    {
                        join_length = strlen (ptr_channel->name) + 1 +
                            strlen (ptr_channel->key) + 1;
                        join_string = malloc (join_length);
                        if (join_string)
                        {
                            snprintf (join_string, join_length, "%s %s",
                                      ptr_channel->name,
                                      ptr_channel->key);
                            irc_command_join_server (server, join_string, 1, 1);
                            free (join_string);
                        }
                        else
                            irc_command_join_server (server, ptr_channel->name, 1, 1);
                    }
                    else
                        irc_command_join_server (server, ptr_channel->name, 1, 1);
                }
                else
                {
                    if (weechat_config_boolean (irc_config_look_part_closes_buffer))
                        weechat_buffer_close (ptr_channel->buffer);
                    else
                        ptr_channel->part = 1;
                }
            }
            else
                irc_nick_free (server, ptr_channel, ptr_nick);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_ping: 'ping' command received
 */

IRC_PROTOCOL_CALLBACK(ping)
{
    /*
     * PING message looks like:
     *   PING :server
     */

    IRC_PROTOCOL_MIN_ARGS(2);

    irc_server_sendf (server, 0, NULL, "PONG :%s",
                      (argv[1][0] == ':') ? argv[1] + 1 : argv[1]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_pong: 'pong' command received
 */

IRC_PROTOCOL_CALLBACK(pong)
{
    struct timeval tv;
    int old_lag;

    IRC_PROTOCOL_MIN_ARGS(0);

    if (server->lag_check_time.tv_sec != 0)
    {
        /* calculate lag (time diff with lag check) */
        old_lag = server->lag;
        gettimeofday (&tv, NULL);
        server->lag = (int) weechat_util_timeval_diff (&(server->lag_check_time),
                                                       &tv);
        if (old_lag != server->lag)
            weechat_bar_item_update ("lag");

        /* schedule next lag check */
        server->lag_check_time.tv_sec = 0;
        server->lag_check_time.tv_usec = 0;
        server->lag_next_check = time (NULL) +
            weechat_config_integer (irc_config_network_lag_check);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_privmsg: 'privmsg' command received
 */

IRC_PROTOCOL_CALLBACK(privmsg)
{
    char *pos_args, *pos_target, str_tags[256], *str_color;
    const char *remote_nick;
    int msg_op, msg_voice, is_channel, nick_is_me;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    /*
     * PRIVMSG message looks like:
     *   :nick!user@host PRIVMSG #channel :message for channel here
     *   :nick!user@host PRIVMSG mynick :message for private here
     *   :nick!user@host PRIVMSG #channel :\01ACTION is testing action\01
     *   :nick!user@host PRIVMSG mynick :\01ACTION is testing action\01
     *   :nick!user@host PRIVMSG #channel :\01VERSION\01
     *   :nick!user@host PRIVMSG mynick :\01VERSION\01
     *   :nick!user@host PRIVMSG mynick :\01DCC SEND file.txt 1488915698 50612 128\01
     */

    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;

    if (ignored)
        return WEECHAT_RC_OK;

    pos_args = (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3];

    msg_op = 0;
    msg_voice = 0;
    pos_target = argv[2];
    is_channel = irc_channel_is_channel (server, pos_target);
    if (!is_channel)
    {
        if (irc_channel_is_channel (server, pos_target + 1))
        {
            if (pos_target[0] == '@')
            {
                is_channel = 1;
                pos_target++;
                msg_op = 1;
            }
            else if (pos_target[0] == '+')
            {
                is_channel = 1;
                pos_target++;
                msg_voice = 1;
            }
        }
    }

    /* receiver is a channel ? */
    if (is_channel)
    {
        ptr_channel = irc_channel_search (server, pos_target);
        if (ptr_channel)
        {
            /* CTCP to channel */
            if ((pos_args[0] == '\01')
                && (pos_args[strlen (pos_args) - 1] == '\01'))
            {
                irc_ctcp_recv (server, date, command, ptr_channel,
                               address, nick, NULL, pos_args,
                               argv_eol[0]);
                return WEECHAT_RC_OK;
            }

            /* other message */
            ptr_nick = irc_nick_search (server, ptr_channel, nick);

            if (ptr_nick && !ptr_nick->host)
                ptr_nick->host = strdup (address);

            if (msg_op || msg_voice)
            {
                /* message to channel ops/voiced (to "@#channel" or "+#channel") */
                weechat_printf_date_tags (ptr_channel->buffer,
                                          date,
                                          irc_protocol_tags (command,
                                                             "notify_message",
                                                             nick),
                                          "%s%s%s%s(%s%s%s)%s: %s",
                                          weechat_prefix ("network"),
                                          "Msg",
                                          (msg_op) ? "Op" : ((msg_voice) ? "Voice" : ""),
                                          IRC_COLOR_CHAT_DELIMITERS,
                                          irc_nick_color_for_message (server, ptr_nick, nick),
                                          (nick && nick[0]) ? nick : "?",
                                          IRC_COLOR_CHAT_DELIMITERS,
                                          IRC_COLOR_RESET,
                                          pos_args);
            }
            else
            {
                /* standard message (to "#channel") */
                str_color = irc_color_for_tags (irc_nick_find_color_name ((ptr_nick) ? ptr_nick->name : nick));
                snprintf (str_tags, sizeof (str_tags),
                          "notify_message,prefix_nick_%s",
                          (str_color) ? str_color : "default");
                if (str_color)
                    free (str_color);
                weechat_printf_date_tags (ptr_channel->buffer,
                                          date,
                                          irc_protocol_tags (command, str_tags, nick),
                                          "%s%s",
                                          irc_nick_as_prefix (server, ptr_nick,
                                                              (ptr_nick) ? NULL : nick,
                                                              NULL),
                                          pos_args);
            }

            irc_channel_nick_speaking_add (ptr_channel,
                                           nick,
                                           weechat_string_has_highlight (pos_args,
                                                                         server->nick));
            irc_channel_nick_speaking_time_remove_old (ptr_channel);
            irc_channel_nick_speaking_time_add (server, ptr_channel, nick,
                                                time (NULL));
        }
    }
    else
    {
        nick_is_me = (irc_server_strcasecmp (server, server->nick, nick) == 0);

        remote_nick = (nick_is_me) ? pos_target : nick;

        /* CTCP to user */
        if ((pos_args[0] == '\01')
            && (pos_args[strlen (pos_args) - 1] == '\01'))
        {
            irc_ctcp_recv (server, date, command, NULL,
                           address, nick, remote_nick, pos_args,
                           argv_eol[0]);
            return WEECHAT_RC_OK;
        }

        /* private message received => display it */
        ptr_channel = irc_channel_search (server, remote_nick);

        if (!ptr_channel)
        {
            ptr_channel = irc_channel_new (server,
                                           IRC_CHANNEL_TYPE_PRIVATE,
                                           remote_nick, 0, 0);
            if (!ptr_channel)
            {
                weechat_printf (server->buffer,
                                _("%s%s: cannot create new "
                                  "private buffer \"%s\""),
                                weechat_prefix ("error"),
                                IRC_PLUGIN_NAME, remote_nick);
                return WEECHAT_RC_ERROR;
            }
        }
        irc_channel_set_topic (ptr_channel, address);

        if (nick_is_me)
            str_color = irc_color_for_tags (weechat_config_color (weechat_config_get ("weechat.color.chat_nick_self")));
        else
        {
            if (weechat_config_boolean (irc_config_look_color_pv_nick_like_channel))
                str_color = irc_color_for_tags (irc_nick_find_color_name (nick));
            else
                str_color = irc_color_for_tags (weechat_config_color (weechat_config_get ("weechat.color.chat_nick_other")));
        }
        snprintf (str_tags, sizeof (str_tags),
                  (nick_is_me) ?
                  "notify_none,no_highlight,prefix_nick_%s" :
                  "notify_private,prefix_nick_%s",
                  (str_color) ? str_color : "default");
        if (str_color)
            free (str_color);
        weechat_printf_date_tags (ptr_channel->buffer,
                                  date,
                                  irc_protocol_tags (command, str_tags, nick),
                                  "%s%s",
                                  irc_nick_as_prefix (server, NULL, nick,
                                                      (nick_is_me) ?
                                                      IRC_COLOR_CHAT_NICK_SELF : irc_nick_color_for_pv (ptr_channel, nick)),
                                  pos_args);

        if (ptr_channel->has_quit_server)
            ptr_channel->has_quit_server = 0;

        weechat_hook_signal_send ("irc_pv",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  argv_eol[0]);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_quit: 'quit' command received
 */

IRC_PROTOCOL_CALLBACK(quit)
{
    char *pos_comment;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_channel_speaking *ptr_nick_speaking;
    int local_quit, display_host;

    /*
     * QUIT message looks like:
     *   :nick!user@host QUIT :quit message
     */

    IRC_PROTOCOL_MIN_ARGS(2);
    IRC_PROTOCOL_CHECK_HOST;

    pos_comment = (argc > 2) ?
        ((argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2]) : NULL;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            ptr_nick = NULL;
        else
            ptr_nick = irc_nick_search (server, ptr_channel, nick);

        if (ptr_nick
            || (irc_server_strcasecmp (server, ptr_channel->name, nick) == 0))
        {
            /* display quit message */
            if (!irc_ignore_check (server, ptr_channel->name, nick, host))
            {
                local_quit = (irc_server_strcasecmp (server, nick, server->nick) == 0);
                ptr_nick_speaking = NULL;
                if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                {
                    ptr_nick_speaking = ((weechat_config_boolean (irc_config_look_smart_filter))
                                         && (weechat_config_boolean (irc_config_look_smart_filter_quit))) ?
                        irc_channel_nick_speaking_time_search (server, ptr_channel, nick, 1) : NULL;
                }
                if (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
                {
                    ptr_channel->has_quit_server = 1;
                }
                display_host = weechat_config_boolean (irc_config_look_display_host_quit);
                if (pos_comment && pos_comment[0])
                {
                    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                               command, NULL,
                                                                               ptr_channel->buffer),
                                              date,
                                              irc_protocol_tags (command,
                                                                 (local_quit
                                                                  || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                                                                  || !weechat_config_boolean (irc_config_look_smart_filter)
                                                                  || !weechat_config_boolean (irc_config_look_smart_filter_quit)
                                                                  || ptr_nick_speaking) ?
                                                                 NULL : "irc_smart_filter",
                                                                 nick),
                                              _("%s%s%s%s%s%s%s%s%s%s has quit "
                                                "%s(%s%s%s)"),
                                              weechat_prefix ("quit"),
                                              (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE) ?
                                              irc_nick_color_for_pv (ptr_channel, nick) : irc_nick_color_for_server_message (server, ptr_nick, nick),
                                              nick,
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              (display_host) ? " (" : "",
                                              IRC_COLOR_CHAT_HOST,
                                              (display_host) ? address : "",
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              (display_host) ? ")" : "",
                                              IRC_COLOR_MESSAGE_QUIT,
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              IRC_COLOR_REASON_QUIT,
                                              pos_comment,
                                              IRC_COLOR_CHAT_DELIMITERS);
                }
                else
                {
                    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                               command, NULL,
                                                                               ptr_channel->buffer),
                                              date,
                                              irc_protocol_tags (command,
                                                                 (local_quit
                                                                  || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                                                                  || !weechat_config_boolean (irc_config_look_smart_filter)
                                                                  || !weechat_config_boolean (irc_config_look_smart_filter_quit)
                                                                  || ptr_nick_speaking) ?
                                                                 NULL : "irc_smart_filter",
                                                                 nick),
                                              _("%s%s%s%s%s%s%s%s%s%s has quit"),
                                              weechat_prefix ("quit"),
                                              (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE) ?
                                              irc_nick_color_for_pv (ptr_channel, nick) : irc_nick_color_for_server_message (server, ptr_nick, nick),
                                              nick,
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              (display_host) ? " (" : "",
                                              IRC_COLOR_CHAT_HOST,
                                              (display_host) ? address : "",
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              (display_host) ? ")" : "",
                                              IRC_COLOR_MESSAGE_QUIT);
                }
            }
            if (ptr_nick)
                irc_nick_free (server, ptr_channel, ptr_nick);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_server_mode_reason: command received from server (numeric),
 *                                     format: "mode :reason"
 */

IRC_PROTOCOL_CALLBACK(server_mode_reason)
{
    char *pos_mode, *pos_args;

    IRC_PROTOCOL_MIN_ARGS(3);

    /* skip nickname if at beginning of server message */
    if (irc_server_strcasecmp (server, server->nick, argv[2]) == 0)
    {
        pos_mode = argv[3];
        pos_args = (argc > 4) ? ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;
    }
    else
    {
        pos_mode = argv[2];
        pos_args = (argc > 3) ? ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    }

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, NULL,
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s: %s",
                              weechat_prefix ("network"),
                              pos_mode,
                              (pos_args) ? pos_args : "");

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_numeric: numeric command received from server
 */

IRC_PROTOCOL_CALLBACK(numeric)
{
    char *pos_args;

    IRC_PROTOCOL_MIN_ARGS(3);

    if (irc_server_strcasecmp (server, server->nick, argv[2]) == 0)
    {
        pos_args = (argc > 3) ?
            ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    }
    else
    {
        pos_args = (argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2];
    }

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, NULL,
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s",
                              weechat_prefix ("network"),
                              pos_args);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_topic: 'topic' command received
 */

IRC_PROTOCOL_CALLBACK(topic)
{
    char *pos_topic, *old_topic_color, *topic_color;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_gui_buffer *ptr_buffer;

    /*
     * TOPIC message looks like:
     *   :nick!user@host TOPIC #channel :new topic for channel
     */

    IRC_PROTOCOL_MIN_ARGS(3);

    if (!irc_channel_is_channel (server, argv[2]))
    {
        weechat_printf (server->buffer,
                        _("%s%s: \"%s\" command received without channel"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, "topic");
        return WEECHAT_RC_OK;
    }

    pos_topic = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;

    ptr_channel = irc_channel_search (server, argv[2]);
    ptr_nick = irc_nick_search (server, ptr_channel, nick);
    ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;

    if (pos_topic && pos_topic[0])
    {
        topic_color = irc_color_decode (pos_topic,
                                        weechat_config_boolean (irc_config_network_colors_receive));
        if (weechat_config_boolean (irc_config_look_display_old_topic)
            && ptr_channel && ptr_channel->topic && ptr_channel->topic[0])
        {
            old_topic_color = irc_color_decode (ptr_channel->topic,
                                                weechat_config_boolean (irc_config_network_colors_receive));
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, NULL,
                                                                       ptr_buffer),
                                      date,
                                      irc_protocol_tags (command, NULL, NULL),
                                      _("%s%s%s%s has changed topic for %s%s%s "
                                        "from \"%s%s%s\" to \"%s%s%s\""),
                                      weechat_prefix ("network"),
                                      irc_nick_color_for_server_message (server, ptr_nick, nick),
                                      nick,
                                      IRC_COLOR_RESET,
                                      IRC_COLOR_CHAT_CHANNEL,
                                      argv[2],
                                      IRC_COLOR_RESET,
                                      IRC_COLOR_TOPIC_OLD,
                                      (old_topic_color) ? old_topic_color : ptr_channel->topic,
                                      IRC_COLOR_RESET,
                                      IRC_COLOR_TOPIC_NEW,
                                      (topic_color) ? topic_color : pos_topic,
                                      IRC_COLOR_RESET);
            if (old_topic_color)
                free (old_topic_color);
        }
        else
        {
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, NULL,
                                                                       ptr_buffer),
                                      date,
                                      irc_protocol_tags (command, NULL, NULL),
                                      _("%s%s%s%s has changed topic for %s%s%s "
                                        "to \"%s%s%s\""),
                                      weechat_prefix ("network"),
                                      irc_nick_color_for_server_message (server, ptr_nick, nick),
                                      nick,
                                      IRC_COLOR_RESET,
                                      IRC_COLOR_CHAT_CHANNEL,
                                      argv[2],
                                      IRC_COLOR_RESET,
                                      IRC_COLOR_TOPIC_NEW,
                                      (topic_color) ? topic_color : pos_topic,
                                      IRC_COLOR_RESET);
        }
        if (topic_color)
            free (topic_color);
    }
    else
    {
        if (weechat_config_boolean (irc_config_look_display_old_topic)
            && ptr_channel && ptr_channel->topic && ptr_channel->topic[0])
        {
            old_topic_color = irc_color_decode (ptr_channel->topic,
                                                weechat_config_boolean (irc_config_network_colors_receive));
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, NULL,
                                                                       ptr_buffer),
                                      date,
                                      irc_protocol_tags (command, NULL, NULL),
                                      _("%s%s%s%s has unset topic for %s%s%s "
                                        "(old topic: \"%s%s%s\")"),
                                      weechat_prefix ("network"),
                                      irc_nick_color_for_server_message (server, ptr_nick, nick),
                                      nick,
                                      IRC_COLOR_RESET,
                                      IRC_COLOR_CHAT_CHANNEL,
                                      argv[2],
                                      IRC_COLOR_RESET,
                                      IRC_COLOR_TOPIC_OLD,
                                      (old_topic_color) ? old_topic_color : ptr_channel->topic,
                                      IRC_COLOR_RESET);
            if (old_topic_color)
                free (old_topic_color);
        }
        else
        {
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, NULL,
                                                                       ptr_buffer),
                                      date,
                                      irc_protocol_tags (command, NULL, NULL),
                                      _("%s%s%s%s has unset topic for %s%s%s"),
                                      weechat_prefix ("network"),
                                      irc_nick_color_for_server_message (server, ptr_nick, nick),
                                      nick,
                                      IRC_COLOR_RESET,
                                      IRC_COLOR_CHAT_CHANNEL,
                                      argv[2],
                                      IRC_COLOR_RESET);
        }
    }

    if (ptr_channel)
        irc_channel_set_topic (ptr_channel, pos_topic);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_wallops: 'wallops' command received
 */

IRC_PROTOCOL_CALLBACK(wallops)
{
    /*
     * WALLOPS message looks like:
     *   :nick!user@host WALLOPS :message from admin
     */

    IRC_PROTOCOL_MIN_ARGS(3);

    if (ignored)
        return WEECHAT_RC_OK;

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, nick,
                                                               command, NULL,
                                                               NULL),
                              date,
                              irc_protocol_tags (command, NULL, nick),
                              _("%sWallops from %s%s %s(%s%s%s)%s: %s"),
                              weechat_prefix ("network"),
                              irc_nick_color_for_message (server, NULL, nick),
                              nick,
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_CHAT_HOST,
                              address,
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_001: '001' command (connected to irc server)
 */

IRC_PROTOCOL_CALLBACK(001)
{
    char **commands, **ptr_cmd, *vars_replaced;
    char *away_msg;
    const char *ptr_command;

    /*
     * 001 message looks like:
     *   :server 001 mynick :Welcome to the dancer-ircd Network
     */

    IRC_PROTOCOL_MIN_ARGS(3);

    if (irc_server_strcasecmp (server, server->nick, argv[2]) != 0)
        irc_server_set_nick (server, argv[2]);

    irc_protocol_cb_numeric (server,
                             date, nick, address, host, command,
                             ignored, argc, argv, argv_eol);

    /* connection to IRC server is ok! */
    server->is_connected = 1;
    server->reconnect_delay = 0;
    if (server->hook_timer_connection)
    {
        weechat_unhook (server->hook_timer_connection);
        server->hook_timer_connection = NULL;
    }
    server->lag_next_check = time (NULL) +
        weechat_config_integer (irc_config_network_lag_check);
    irc_server_set_buffer_title (server);

    /* set away message if user was away (before disconnection for example) */
    if (server->away_message && server->away_message[0])
    {
        away_msg = strdup (server->away_message);
        if (away_msg)
        {
            irc_command_away_server (server, away_msg, 0);
            free (away_msg);
        }
    }

    /* send signal "irc_server_connected" with server name */
    weechat_hook_signal_send ("irc_server_connected",
                              WEECHAT_HOOK_SIGNAL_STRING, server->name);

    /* execute command when connected */
    ptr_command = IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_COMMAND);
    if (ptr_command && ptr_command[0])
    {
        /* splitting command on ';' which can be escaped with '\;' */
        commands = weechat_string_split_command (ptr_command, ';');
        if (commands)
        {
            for (ptr_cmd = commands; *ptr_cmd; ptr_cmd++)
            {
                vars_replaced = irc_message_replace_vars (server, NULL,
                                                          *ptr_cmd);
                weechat_command (server->buffer,
                                 (vars_replaced) ? vars_replaced : *ptr_cmd);
                if (vars_replaced)
                    free (vars_replaced);
            }
            weechat_string_free_split_command (commands);
        }

        if (IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_COMMAND_DELAY) > 0)
            server->command_time = time (NULL) + 1;
        else
            irc_server_autojoin_channels (server);
    }
    else
        irc_server_autojoin_channels (server);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_005: '005' command (some infos from server)
 */

IRC_PROTOCOL_CALLBACK(005)
{
    char *pos, *pos2, *pos_start, *error, *isupport2;
    int length_isupport, length, nick_max_length, casemapping;

    /*
     * 005 message looks like:
     *   :server 005 mynick MODES=4 CHANLIMIT=#:20 NICKLEN=16 USERLEN=10
     *     HOSTLEN=63 TOPICLEN=450 KICKLEN=450 CHANNELLEN=30 KEYLEN=23
     *     CHANTYPES=# PREFIX=(ov)@+ CASEMAPPING=ascii CAPAB IRCD=dancer
     *     :are available on this server
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    irc_protocol_cb_numeric (server,
                             date, nick, address, host, command,
                             ignored, argc, argv, argv_eol);

    /* save prefix */
    pos = strstr (argv_eol[3], "PREFIX=");
    if (pos)
    {
        pos += 7;
        pos2 = strchr (pos, ' ');
        if (pos2)
            pos2[0] = '\0';
        irc_server_set_prefix_modes_chars (server, pos);
        if (pos2)
            pos2[0] = ' ';
    }

    /* save max nick length */
    pos = strstr (argv_eol[3], "NICKLEN=");
    if (pos)
    {
        pos += 8;
        pos2 = strchr (pos, ' ');
        if (pos2)
            pos2[0] = '\0';
        error = NULL;
        nick_max_length = (int)strtol (pos, &error, 10);
        if (error && !error[0] && (nick_max_length > 0))
            server->nick_max_length = nick_max_length;
        if (pos2)
            pos2[0] = ' ';
    }

    /* save casemapping */
    pos = strstr (argv_eol[3], "CASEMAPPING=");
    if (pos)
    {
        pos += 12;
        pos2 = strchr (pos, ' ');
        if (pos2)
            pos2[0] = '\0';
        casemapping = irc_server_search_casemapping (pos);
        if (casemapping >= 0)
            server->casemapping = casemapping;
        if (pos2)
            pos2[0] = ' ';
    }

    /* save chantypes */
    pos = strstr (argv_eol[3], "CHANTYPES=");
    if (pos)
    {
        pos += 10;
        pos2 = strchr (pos, ' ');
        if (pos2)
            pos2[0] = '\0';
        if (server->chantypes)
            free (server->chantypes);
        server->chantypes = strdup (pos);
        if (pos2)
            pos2[0] = ' ';
    }

    /* save chanmodes */
    pos = strstr (argv_eol[3], "CHANMODES=");
    if (pos)
    {
        pos += 10;
        pos2 = strchr (pos, ' ');
        if (pos2)
            pos2[0] = '\0';
        if (server->chanmodes)
            free (server->chanmodes);
        server->chanmodes = strdup (pos);
        if (pos2)
            pos2[0] = ' ';
    }

    /* save whole message (concatenate to existing isupport, if any) */
    pos_start = NULL;
    pos = strstr (argv_eol[3], " :");
    length = (pos) ? pos - argv_eol[3] : (int)strlen (argv_eol[3]);
    if (server->isupport)
    {
        length_isupport = strlen (server->isupport);
        isupport2 = realloc (server->isupport,
                             length_isupport + /* existing */
                             1 + length + 1); /* new */
        if (isupport2)
        {
            server->isupport = isupport2;
            pos_start = server->isupport + length_isupport;
        }
    }
    else
    {
        server->isupport = malloc (1 + length + 1);
        if (server->isupport)
            pos_start = server->isupport;
    }

    if (pos_start)
    {
        pos_start[0] = ' ';
        memcpy (pos_start + 1, argv_eol[3], length);
        pos_start[length + 1] = '\0';
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_221: '221' command (user mode string)
 */

IRC_PROTOCOL_CALLBACK(221)
{
    /*
     * 221 message looks like:
     *   :server 221 nick :+s
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[2],
                                                               command, NULL, NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              _("%sUser mode for %s%s%s is %s[%s%s%s]"),
                              weechat_prefix ("network"),
                              irc_nick_color_for_server_message (server, NULL,
                                                                 argv[2]),
                              argv[2],
                              IRC_COLOR_RESET,
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3],
                              IRC_COLOR_CHAT_DELIMITERS);

    if (irc_server_strcasecmp (server, argv[2], server->nick) == 0)
    {
        irc_mode_user_set (server,
                           (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3],
                           1);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_301: '301' command (away message)
 *                      received when we are talking to a user in private
 *                      and that remote user is away (we receive away message)
 */

IRC_PROTOCOL_CALLBACK(301)
{
    char *pos_away_msg;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 301 message looks like:
     *   :server 301 mynick nick :away message for nick
     */

    IRC_PROTOCOL_MIN_ARGS(3);

    if (argc > 4)
    {
        pos_away_msg = (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4];

        /* look for private buffer to display message */
        ptr_channel = irc_channel_search (server, argv[3]);
        if (!weechat_config_boolean (irc_config_look_display_pv_away_once)
            || !ptr_channel
            || !(ptr_channel->away_message)
            || (strcmp (ptr_channel->away_message, pos_away_msg) != 0))
        {
            ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                                       command, "whois",
                                                                       ptr_buffer),
                                      date,
                                      irc_protocol_tags (command, "irc_numeric", NULL),
                                      _("%s%s[%s%s%s]%s is away: %s"),
                                      weechat_prefix ("network"),
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      irc_nick_color_for_server_message (server,
                                                                         NULL,
                                                                         argv[3]),
                                      argv[3],
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_RESET,
                                      pos_away_msg);
            if (ptr_channel)
            {
                if (ptr_channel->away_message)
                    free (ptr_channel->away_message);
                ptr_channel->away_message = strdup (pos_away_msg);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_303: '303' command (ison)
 */

IRC_PROTOCOL_CALLBACK(303)
{
    /*
     * 303 message looks like:
     *   :server 303 mynick :nick1 nick2
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, NULL, NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              _("%sUsers online: %s%s"),
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_NICK,
                              (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_305: '305' command (unaway)
 */

IRC_PROTOCOL_CALLBACK(305)
{
    /*
     * 305 message looks like:
     *   :server 305 mynick :Does this mean you're really back?
     */

    IRC_PROTOCOL_MIN_ARGS(3);

    if (argc > 3)
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "unaway",
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s",
                                  weechat_prefix ("network"),
                                  (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]);
    }

    server->is_away = 0;
    server->away_time = 0;

    weechat_bar_item_update ("away");

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_306: '306' command (now away)
 */

IRC_PROTOCOL_CALLBACK(306)
{
    /*
     * 306 message looks like:
     *   :server 306 mynick :We'll miss you
     */

    IRC_PROTOCOL_MIN_ARGS(3);

    if (argc > 3)
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "away",
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s",
                                  weechat_prefix ("network"),
                                  (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]);
    }

    server->is_away = 1;
    server->away_time = time (NULL);

    weechat_bar_item_update ("away");

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_whois_nick_msg: a whois command with nick and message
 */

IRC_PROTOCOL_CALLBACK(whois_nick_msg)
{
    /*
     * messages look like:
     *   :server 319 flashy FlashCode :some text here
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                               command, "whois",
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s[%s%s%s] %s%s",
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_DELIMITERS,
                              irc_nick_color_for_server_message (server, NULL,
                                                                 argv[3]),
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_whowas_nick_msg: a whowas command with nick and message
 */

IRC_PROTOCOL_CALLBACK(whowas_nick_msg)
{
    /*
     * messages look like:
     *   :server 369 flashy FlashCode :some text here
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                               command, "whowas",
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s[%s%s%s] %s%s",
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_DELIMITERS,
                              irc_nick_color_for_server_message (server, NULL,
                                                                 argv[3]),
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_311: '311' command (whois, user)
 */

IRC_PROTOCOL_CALLBACK(311)
{
    /*
     * 311 message looks like:
     *   :server 311 mynick nick user host * :realname here
     */

    IRC_PROTOCOL_MIN_ARGS(8);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                               command, "whois",
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s[%s%s%s] (%s%s@%s%s)%s: %s",
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_DELIMITERS,
                              irc_nick_color_for_server_message (server, NULL,
                                                                 argv[3]),
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_CHAT_HOST,
                              argv[4],
                              argv[5],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (argv_eol[7][0] == ':') ? argv_eol[7] + 1 : argv_eol[7]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_312: '312' command (whois, server)
 */

IRC_PROTOCOL_CALLBACK(312)
{
    /*
     * 312 message looks like:
     *   :server 312 mynick nick irc.freenode.net :http://freenode.net/
     */

    IRC_PROTOCOL_MIN_ARGS(6);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                               command, "whois",
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s[%s%s%s] %s%s %s(%s%s%s)",
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_DELIMITERS,
                              irc_nick_color_for_server_message (server, NULL,
                                                                 argv[3]),
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              argv[4],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5],
                              IRC_COLOR_CHAT_DELIMITERS);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_314: '314' command (whowas)
 */

IRC_PROTOCOL_CALLBACK(314)
{
    /*
     * 314 message looks like:
     *   :server 314 mynick nick user host * :realname here
     */

    IRC_PROTOCOL_MIN_ARGS(8);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                               command, "whowas",
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              _("%s%s[%s%s%s] (%s%s@%s%s)%s was %s"),
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_DELIMITERS,
                              irc_nick_color_for_server_message (server, NULL,
                                                                 argv[3]),
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_CHAT_HOST,
                              argv[4],
                              argv[5],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (argv_eol[7][0] == ':') ? argv_eol[7] + 1 : argv_eol[7]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_315: '315' command (end of /who)
 */

IRC_PROTOCOL_CALLBACK(315)
{
    struct t_irc_channel *ptr_channel;

    /*
     * 315 message looks like:
     *   :server 315 mynick #channel :End of /WHO list.
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    ptr_channel = irc_channel_search (server, argv[3]);
    if (ptr_channel && (ptr_channel->checking_away > 0))
    {
        ptr_channel->checking_away--;
    }
    else
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "who",
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s[%s%s%s]%s %s",
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_317: '317' command (whois, idle)
 */

IRC_PROTOCOL_CALLBACK(317)
{
    int idle_time, day, hour, min, sec;
    time_t datetime;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 317 message looks like:
     *   :server 317 mynick nick 122877 1205327880 :seconds idle, signon time
     */

    IRC_PROTOCOL_MIN_ARGS(6);

    idle_time = atoi (argv[4]);
    day = idle_time / (60 * 60 * 24);
    hour = (idle_time % (60 * 60 * 24)) / (60 * 60);
    min = ((idle_time % (60 * 60 * 24)) % (60 * 60)) / 60;
    sec = ((idle_time % (60 * 60 * 24)) % (60 * 60)) % 60;

    datetime = (time_t)(atol (argv[5]));

    ptr_buffer = irc_msgbuffer_get_target_buffer (server, argv[3],
                                                  command, "whois", NULL);

    if (day > 0)
    {
        weechat_printf_date_tags (ptr_buffer,
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  _("%s%s[%s%s%s]%s idle: %s%d %s%s, "
                                    "%s%02d %s%s %s%02d %s%s %s%02d "
                                    "%s%s, signon at: %s%s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     argv[3]),
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  day,
                                  IRC_COLOR_RESET,
                                  NG_("day", "days", day),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  hour,
                                  IRC_COLOR_RESET,
                                  NG_("hour", "hours", hour),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  min,
                                  IRC_COLOR_RESET,
                                  NG_("minute", "minutes", min),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  sec,
                                  IRC_COLOR_RESET,
                                  NG_("second", "seconds", sec),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  weechat_util_get_time_string (&datetime));
    }
    else
    {
        weechat_printf_date_tags (ptr_buffer,
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  _("%s%s[%s%s%s]%s idle: %s%02d %s%s "
                                    "%s%02d %s%s %s%02d %s%s, "
                                    "signon at: %s%s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     argv[3]),
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  hour,
                                  IRC_COLOR_RESET,
                                  NG_("hour", "hours", hour),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  min,
                                  IRC_COLOR_RESET,
                                  NG_("minute", "minutes", min),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  sec,
                                  IRC_COLOR_RESET,
                                  NG_("second", "seconds", sec),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  weechat_util_get_time_string (&datetime));
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_321: '321' command (/list start)
 */

IRC_PROTOCOL_CALLBACK(321)
{
    char *pos_args;

    /*
     * 321 message looks like:
     *   :server 321 mynick Channel :Users  Name
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    pos_args = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, "list",
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s%s%s",
                              weechat_prefix ("network"),
                              argv[3],
                              (pos_args) ? " " : "",
                              (pos_args) ? pos_args : "");

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_322: '322' command (channel for /list)
 */

IRC_PROTOCOL_CALLBACK(322)
{
    char *pos_topic;

    /*
     * 322 message looks like:
     *   :server 322 mynick #channel 3 :topic of channel
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    pos_topic = (argc > 5) ?
        ((argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5]) : NULL;

    if (!server->cmd_list_regexp ||
        (regexec (server->cmd_list_regexp, argv[3], 0, NULL, 0) == 0))
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "list",
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s%s%s(%s%s%s)%s%s%s",
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  argv[4],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  (pos_topic && pos_topic[0]) ? ": " : "",
                                  (pos_topic && pos_topic[0]) ? pos_topic : "");
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_323: '323' command (end of /list)
 */

IRC_PROTOCOL_CALLBACK(323)
{
    char *pos_args;

    /*
     * 323 message looks like:
     *   :server 323 mynick :End of /LIST
     */

    IRC_PROTOCOL_MIN_ARGS(3);

    pos_args = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, "list",
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s",
                              weechat_prefix ("network"),
                              (pos_args && pos_args[0]) ? pos_args : "");

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_324: '324' command (channel mode)
 */

IRC_PROTOCOL_CALLBACK(324)
{
    struct t_irc_channel *ptr_channel;

    /*
     * 324 message looks like:
     *   :server 324 mynick #channel +nt
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    ptr_channel = irc_channel_search (server, argv[3]);
    if (ptr_channel)
    {
        irc_channel_set_modes (ptr_channel, ((argc > 4) ? argv_eol[4] : NULL));
        if (argc > 4)
        {
            irc_mode_channel_set (server, ptr_channel,
                                  ptr_channel->modes);
        }
    }
    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, NULL,
                                                               (ptr_channel) ? ptr_channel->buffer : NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              _("%sMode %s%s %s[%s%s%s]"),
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_CHANNEL,
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (argc > 4) ?
                              ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : "",
                              IRC_COLOR_CHAT_DELIMITERS);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_327: '327' command (whois, host)
 */

IRC_PROTOCOL_CALLBACK(327)
{
    char *pos_realname;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 327 message looks like:
     *   :server 327 mynick nick host ip :real hostname/ip
     */

    IRC_PROTOCOL_MIN_ARGS(6);

    pos_realname = (argc > 6) ?
        ((argv_eol[6][0] == ':') ? argv_eol[6] + 1 : argv_eol[6]) : NULL;

    ptr_buffer = irc_msgbuffer_get_target_buffer (server, argv[3],
                                                  command, "whois", NULL);

    if (pos_realname && pos_realname[0])
    {
        weechat_printf_date_tags (ptr_buffer,
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s[%s%s%s] %s%s %s %s(%s%s%s)",
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     argv[3]),
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  argv[4],
                                  argv[5],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  pos_realname,
                                  IRC_COLOR_CHAT_DELIMITERS);
    }
    else
    {
        weechat_printf_date_tags (ptr_buffer,
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s[%s%s%s] %s%s %s",
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     argv[3]),
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  argv[4],
                                  argv[5]);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_328: '328' channel URL
 */

IRC_PROTOCOL_CALLBACK(328)
{
    struct t_irc_channel *ptr_channel;

    /*
     * 328 message looks like:
     *   :server 328 mynick #channel :http://sample.url.com/
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    ptr_channel = irc_channel_search (server, argv[3]);
    if (ptr_channel)
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, NULL,
                                                                   ptr_channel->buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  _("%sURL for %s%s%s: %s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_RESET,
                                  (argv_eol[4][0] == ':') ?
                                  argv_eol[4] + 1 : argv_eol[4]);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_329: '329' command received (channel creation date)
 */

IRC_PROTOCOL_CALLBACK(329)
{
    struct t_irc_channel *ptr_channel;
    time_t datetime;

    /*
     * 329 message looks like:
     *   :server 329 mynick #channel 1205327894
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    ptr_channel = irc_channel_search (server, argv[3]);

    datetime = (time_t)(atol ((argv_eol[4][0] == ':') ?
                              argv_eol[4] + 1 : argv_eol[4]));

    if (ptr_channel)
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, NULL,
                                                                   ptr_channel->buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  /* TRANSLATORS: "%s" after "created on" is a date */
                                  _("%sChannel created on %s"),
                                  weechat_prefix ("network"),
                                  weechat_util_get_time_string (&datetime));
    }
    else
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, NULL,
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  /* TRANSLATORS: "%s" after "created on" is a date */
                                  _("%sChannel %s%s%s created on %s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_RESET,
                                  weechat_util_get_time_string (&datetime));
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_330_343: '330' command (whois, is logged in as),
 *                          '343' command (whois, is opered as)
 */

IRC_PROTOCOL_CALLBACK(330_343)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 330 message looks like:
     *   :server 330 mynick nick1 nick2 :is logged in as
     *   or:
     *   :server 330 mynick #channel http://sample.url.com/
     * 343 message looks like:
     *   :server 343 mynick nick1 nick2 :is opered as
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    if (argc >= 6)
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                                   command, "whois",
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s[%s%s%s] %s%s %s%s",
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     argv[3]),
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  (argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5],
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     argv[4]),
                                  argv[4]);
    }
    else
    {
        ptr_channel = (irc_channel_is_channel (server, argv[3])) ?
            irc_channel_search (server, argv[3]) : NULL;
        ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                                   command, "whois",
                                                                   ptr_buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s[%s%s%s] %s%s",
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     argv[3]),
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_331: '331' command received (no topic for channel)
 */

IRC_PROTOCOL_CALLBACK(331)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 331 message looks like:
     *   :server 331 mynick #channel :There isn't a topic.
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                               command, NULL,
                                                               ptr_buffer),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              _("%sNo topic set for channel %s%s"),
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_CHANNEL,
                              argv[3]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_332: '332' command received (topic of channel)
 */

IRC_PROTOCOL_CALLBACK(332)
{
    char *pos_topic, *topic_no_color, *topic_color;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 332 message looks like:
     *   :server 332 mynick #channel :topic of channel
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    pos_topic = NULL;
    if (argc >= 5)
        pos_topic = (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4];

    ptr_channel = irc_channel_search (server, argv[3]);

    if (ptr_channel && ptr_channel->nicks)
    {
        if (pos_topic)
        {
            topic_no_color = (weechat_config_boolean (irc_config_network_colors_receive)) ?
                NULL : irc_color_decode (pos_topic, 0);
            irc_channel_set_topic (ptr_channel,
                                   (topic_no_color) ? topic_no_color : pos_topic);
            if (topic_no_color)
                free (topic_no_color);
        }
        ptr_buffer = ptr_channel->buffer;
    }
    else
        ptr_buffer = server->buffer;

    topic_color = NULL;
    if (pos_topic)
    {
        topic_color = irc_color_decode (pos_topic,
                                        (weechat_config_boolean (irc_config_network_colors_receive)) ? 1 : 0);
    }
    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, NULL,
                                                               ptr_buffer),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              _("%sTopic for %s%s%s is \"%s%s\""),
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_CHANNEL,
                              argv[3],
                              IRC_COLOR_RESET,
                              (topic_color) ? topic_color : ((pos_topic) ? pos_topic : ""),
                              IRC_COLOR_RESET);
    if (topic_color)
        free (topic_color);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_333: '333' command received (infos about topic (nick / date))
 */

IRC_PROTOCOL_CALLBACK(333)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    time_t datetime;
    const char *topic_nick, *topic_address;
    int arg_date;

    /*
     * 333 message looks like:
     *   :server 333 mynick #channel nick!user@host 1205428096
     *   or:
     *   :server 333 mynick #channel 1205428096
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    topic_nick = (argc > 5) ? irc_message_get_nick_from_host (argv[4]) : NULL;
    topic_address = (argc > 5) ? irc_message_get_address_from_host (argv[4]) : NULL;
    if (topic_nick && topic_address && strcmp (topic_nick, topic_address) == 0)
        topic_address = NULL;

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_nick = (ptr_channel) ?
        irc_nick_search (server, ptr_channel, topic_nick) : NULL;
    arg_date = (argc > 5) ? 5 : 4;
    datetime = (time_t)(atol ((argv_eol[arg_date][0] == ':') ?
                              argv_eol[arg_date] + 1 : argv_eol[arg_date]));

    if (!topic_nick && (datetime == 0))
        return WEECHAT_RC_OK;

    if (ptr_channel && ptr_channel->nicks)
    {
        if (topic_nick)
        {
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, NULL,
                                                                       ptr_channel->buffer),
                                      date,
                                      irc_protocol_tags (command, "irc_numeric", NULL),
                                      /* TRANSLATORS: "%s" after "on" is a date */
                                      _("%sTopic set by %s%s%s%s%s%s%s%s%s on %s"),
                                      weechat_prefix ("network"),
                                      irc_nick_color_for_server_message (server, ptr_nick, topic_nick),
                                      topic_nick,
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      (topic_address && topic_address[0]) ? " (" : "",
                                      IRC_COLOR_CHAT_HOST,
                                      (topic_address) ? topic_address : "",
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      (topic_address && topic_address[0]) ? ")" : "",
                                      IRC_COLOR_RESET,
                                      weechat_util_get_time_string (&datetime));
        }
        else
        {
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, NULL,
                                                                       ptr_channel->buffer),
                                      date,
                                      irc_protocol_tags (command, "irc_numeric", NULL),
                                      /* TRANSLATORS: "%s" after "on" is a date */
                                      _("%sTopic set on %s"),
                                      weechat_prefix ("network"),
                                      weechat_util_get_time_string (&datetime));
        }
    }
    else
    {
        if (topic_nick)
        {
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, NULL,
                                                                       NULL),
                                      date,
                                      irc_protocol_tags (command, "irc_numeric", NULL),
                                      /* TRANSLATORS: "%s" after "on" is a date */
                                      _("%sTopic for %s%s%s set by %s%s%s%s%s%s%s%s%s on %s"),
                                      weechat_prefix ("network"),
                                      IRC_COLOR_CHAT_CHANNEL,
                                      argv[3],
                                      IRC_COLOR_RESET,
                                      irc_nick_color_for_server_message (server, ptr_nick, topic_nick),
                                      topic_nick,
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      (topic_address && topic_address[0]) ? " (" : "",
                                      IRC_COLOR_CHAT_HOST,
                                      (topic_address) ? topic_address : "",
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      (topic_address && topic_address[0]) ? ")" : "",
                                      IRC_COLOR_RESET,
                                      weechat_util_get_time_string (&datetime));
        }
        else
        {
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, NULL,
                                                                       NULL),
                                      date,
                                      irc_protocol_tags (command, "irc_numeric", NULL),
                                      /* TRANSLATORS: "%s" after "on" is a date */
                                      _("%sTopic for %s%s%s set on %s"),
                                      weechat_prefix ("network"),
                                      IRC_COLOR_CHAT_CHANNEL,
                                      argv[3],
                                      IRC_COLOR_RESET,
                                      weechat_util_get_time_string (&datetime));
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_338: '338' command (whois, host)
 */

IRC_PROTOCOL_CALLBACK(338)
{
    /*
     * 338 message looks like:
     *   :server 338 mynick nick host :actually using host
     */

    IRC_PROTOCOL_MIN_ARGS(6);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                               command, "whois",
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s[%s%s%s]%s %s %s%s",
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_DELIMITERS,
                              irc_nick_color_for_server_message (server, NULL,
                                                                 argv[3]),
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5],
                              IRC_COLOR_CHAT_HOST,
                              argv[4]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_341: '341' command received (inviting)
 */

IRC_PROTOCOL_CALLBACK(341)
{
    /*
     * 341 message looks like:
     *   :server 341 mynick nick #channel
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[2],
                                                               command, NULL, NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              _("%s%s%s%s has invited %s%s%s to %s%s%s"),
                              weechat_prefix ("network"),
                              irc_nick_color_for_server_message (server, NULL,
                                                                 argv[2]),
                              argv[2],
                              IRC_COLOR_RESET,
                              irc_nick_color_for_server_message (server, NULL,
                                                                 argv[3]),
                              argv[3],
                              IRC_COLOR_RESET,
                              IRC_COLOR_CHAT_CHANNEL,
                              argv[4],
                              IRC_COLOR_RESET);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_344: '344' command (channel reop)
 */

IRC_PROTOCOL_CALLBACK(344)
{
    /*
     * 344 message looks like:
     *   :server 344 mynick #channel nick!user@host
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, "reop",
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              _("%sChannel reop %s%s%s: %s%s"),
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_CHANNEL,
                              argv[3],
                              IRC_COLOR_RESET,
                              IRC_COLOR_CHAT_HOST,
                              (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_345: '345' command (end of channel reop)
 */

IRC_PROTOCOL_CALLBACK(345)
{
    /*
     * 345 message looks like:
     *   :server 345 mynick #channel :End of Channel Reop List
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, "reop",
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s%s%s: %s",
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_CHANNEL,
                              argv[3],
                              IRC_COLOR_RESET,
                              (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_346: '346' command received (channel invite list)
 */

IRC_PROTOCOL_CALLBACK(346)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    time_t datetime;
    const char *invite_nick, *invite_address;

    /*
     * 346 message looks like:
     *   :server 346 mynick #channel invitemask nick!user@host 1205590879
     *   or:
     *   :server 346 mynick #channel invitemask
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    if (argc >= 6)
    {
        invite_nick = irc_message_get_nick_from_host (argv[5]);
        invite_address = irc_message_get_address_from_host (argv[5]);
        if (argc >= 7)
        {
            datetime = (time_t)(atol (argv[6]));
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, "invitelist",
                                                                       ptr_buffer),
                                      date,
                                      irc_protocol_tags (command, "irc_numeric", NULL),
                                      /* TRANSLATORS: "%s" after "on" is a date */
                                      _("%s%s[%s%s%s] %s%s%s invited by "
                                        "%s%s %s(%s%s%s)%s on %s"),
                                      weechat_prefix ("network"),
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_CHAT_CHANNEL,
                                      argv[3],
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_CHAT_HOST,
                                      argv[4],
                                      IRC_COLOR_RESET,
                                      irc_nick_color_for_server_message (server, NULL,
                                                                         invite_nick),
                                      invite_nick,
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_CHAT_HOST,
                                      invite_address,
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_RESET,
                                      weechat_util_get_time_string (&datetime));
        }
        else
        {
            weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                       command, "invitelist",
                                                                       ptr_buffer),
                                      date,
                                      irc_protocol_tags (command, "irc_numeric", NULL),
                                      _("%s%s[%s%s%s] %s%s%s invited by "
                                        "%s%s %s(%s%s%s)"),
                                      weechat_prefix ("network"),
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_CHAT_CHANNEL,
                                      argv[3],
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_CHAT_HOST,
                                      argv[4],
                                      IRC_COLOR_RESET,
                                      irc_nick_color_for_server_message (server, NULL,
                                                                         invite_nick),
                                      invite_nick,
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_CHAT_HOST,
                                      invite_address,
                                      IRC_COLOR_CHAT_DELIMITERS);
        }
    }
    else
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "invitelist",
                                                                   ptr_buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  _("%s%s[%s%s%s] %s%s%s invited"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  argv[4],
                                  IRC_COLOR_RESET);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_347: '347' command received (end of channel invite list)
 */

IRC_PROTOCOL_CALLBACK(347)
{
    char *pos_args;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 347 message looks like:
     *   :server 347 mynick #channel :End of Channel Invite List
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    pos_args = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, "invitelist",
                                                               ptr_buffer),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s[%s%s%s]%s%s%s",
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_CHAT_CHANNEL,
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (pos_args) ? " " : "",
                              (pos_args) ? pos_args : "");

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_348: '348' command received (channel exception list)
 */

IRC_PROTOCOL_CALLBACK(348)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    time_t datetime;
    const char *exception_nick, *exception_address;

    /*
     * 348 message looks like:
     *   :server 348 mynick #channel nick1!user1@host1 nick2!user2@host2 1205585109
     *   (nick2 is nick who set exception on nick1)
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    if (argc >= 7)
    {
        exception_nick = irc_message_get_nick_from_host (argv[5]);
        exception_address = irc_message_get_address_from_host (argv[5]);
        datetime = (time_t)(atol (argv[6]));
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "exceptionlist",
                                                                   ptr_buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  /* TRANSLATORS: "%s" after "on" is a date */
                                  _("%s%s[%s%s%s]%s exception %s%s%s "
                                    "by %s%s %s(%s%s%s)%s on %s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  IRC_COLOR_CHAT_HOST,
                                  argv[4],
                                  IRC_COLOR_RESET,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     exception_nick),
                                  exception_nick,
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  exception_address,
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  weechat_util_get_time_string (&datetime));
    }
    else
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "exceptionlist",
                                                                   ptr_buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  _("%s%s[%s%s%s]%s exception %s%s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  IRC_COLOR_CHAT_HOST,
                                  argv[4]);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_349: '349' command received (end of channel exception list)
 */

IRC_PROTOCOL_CALLBACK(349)
{
    char *pos_args;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 349 message looks like:
     *   :server 349 mynick #channel :End of Channel Exception List
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    pos_args = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, "exceptionlist",
                                                               ptr_buffer),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s[%s%s%s]%s%s%s",
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_CHAT_CHANNEL,
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (pos_args) ? " " : "",
                              (pos_args) ? pos_args : "");

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_351: '351' command received (server version)
 */

IRC_PROTOCOL_CALLBACK(351)
{
    struct t_gui_buffer *ptr_buffer;

    /*
     * 351 message looks like:
     *   :server 351 mynick dancer-ircd-1.0.36(2006/07/23_13:11:50). server :iMZ dncrTS/v4
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    ptr_buffer = irc_msgbuffer_get_target_buffer (server, NULL, command, NULL,
                                                  NULL);

    if (argc > 5)
    {
        weechat_printf_date_tags (ptr_buffer,
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s %s (%s)",
                                  weechat_prefix ("network"),
                                  argv[3],
                                  argv[4],
                                  (argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5]);
    }
    else
    {
        weechat_printf_date_tags (ptr_buffer,
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s %s",
                                  weechat_prefix ("network"),
                                  argv[3],
                                  argv[4]);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_352: '352' command (who)
 */

IRC_PROTOCOL_CALLBACK(352)
{
    char *pos_attr, *pos_hopcount, *pos_realname;
    int arg_start, length;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    /*
     * 352 message looks like:
     *   :server 352 mynick #channel user host server nick (*) (H/G) :0 flashcode
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    /* silently ignore malformed 352 message (missing infos) */
    if (argc < 8)
        return WEECHAT_RC_OK;

    pos_attr = NULL;
    pos_hopcount = NULL;
    pos_realname = NULL;

    if (argc > 8)
    {
        arg_start = (strcmp (argv[8], "*") == 0) ? 9 : 8;
        if (argv[arg_start][0] == ':')
        {
            pos_attr = NULL;
            pos_hopcount = (argc > arg_start) ? argv[arg_start] + 1 : NULL;
            pos_realname = (argc > arg_start + 1) ? argv_eol[arg_start + 1] : NULL;
        }
        else
        {
            pos_attr = argv[arg_start];
            pos_hopcount = (argc > arg_start + 1) ? argv[arg_start + 1] + 1 : NULL;
            pos_realname = (argc > arg_start + 2) ? argv_eol[arg_start + 2] : NULL;
        }
    }

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_nick = (ptr_channel) ?
        irc_nick_search (server, ptr_channel, argv[7]) : NULL;

    /* update host for nick */
    if (ptr_nick)
    {
        if (ptr_nick->host)
            free (ptr_nick->host);
        length = strlen (argv[4]) + 1 + strlen (argv[5]) + 1;
        ptr_nick->host = malloc (length);
        if (ptr_nick->host)
            snprintf (ptr_nick->host, length, "%s@%s", argv[4], argv[5]);
    }

    /* update away flag for nick */
    if (ptr_channel && ptr_nick && pos_attr)
    {
        irc_nick_set_away (server, ptr_channel, ptr_nick,
                           (pos_attr[0] == 'G') ? 1 : 0);
    }

    /* display output of who (manual who from user) */
    if (!ptr_channel || (ptr_channel->checking_away <= 0))
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "who",
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s[%s%s%s] %s%s %s(%s%s@%s%s)%s "
                                  "%s%s%s%s(%s)",
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     argv[7]),
                                  argv[7],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  argv[4],
                                  argv[5],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  (pos_attr) ? pos_attr : "",
                                  (pos_attr) ? " " : "",
                                  (pos_hopcount) ? pos_hopcount : "",
                                  (pos_hopcount) ? " " : "",
                                  (pos_realname) ? pos_realname : "");
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_353: '353' command received (list of users on a channel)
 */

IRC_PROTOCOL_CALLBACK(353)
{
    char *pos_channel, *pos_nick, *pos_nick_orig, *pos_host, *nickname;
    char *prefixes;
    int args, i, away;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    /*
     * 353 message looks like:
     *   :server 353 mynick = #channel :mynick nick1 @nick2 +nick3
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    if (irc_channel_is_channel (server, argv[3]))
    {
        pos_channel = argv[3];
        args = 4;
    }
    else
    {
        pos_channel = argv[4];
        args = 5;
    }

    IRC_PROTOCOL_MIN_ARGS(args + 1);

    ptr_channel = irc_channel_search (server, pos_channel);

    if (ptr_channel && ptr_channel->nicks)
    {
        for (i = args; i < argc; i++)
        {
            pos_nick = (argv[i][0] == ':') ? argv[i] + 1 : argv[i];
            pos_nick_orig = pos_nick;

            /* skip prefix(es) */
            while (pos_nick[0]
                   && (irc_server_get_prefix_char_index (server, pos_nick[0]) >= 0))
            {
                pos_nick++;
            }

            /* extract nick from host */
            pos_host = strchr (pos_nick, '!');
            if (pos_host)
                nickname = weechat_strndup (pos_nick, pos_host - pos_nick);
            else
                nickname = strdup (pos_nick);

            /* add or update nick on channel */
            if (nickname)
            {
                ptr_nick = irc_nick_search (server, ptr_channel, nickname);
                away = (ptr_nick && ptr_nick->away) ? 1 : 0;
                prefixes = (pos_nick > pos_nick_orig) ?
                    weechat_strndup (pos_nick_orig, pos_nick - pos_nick_orig) : NULL;
                if (!irc_nick_new (server, ptr_channel, nickname, prefixes,
                                   away))
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot create nick \"%s\" "
                                      "for channel \"%s\""),
                                    weechat_prefix ("error"),
                                    IRC_PLUGIN_NAME, nickname, ptr_channel->name);
                }
                free (nickname);
                if (prefixes)
                    free (prefixes);
            }
        }
    }

    if (!ptr_channel)
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "names",
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  _("%sNicks %s%s%s: %s[%s%s%s]"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  pos_channel,
                                  IRC_COLOR_RESET,
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  (argv_eol[args][0] == ':') ?
                                  argv_eol[args] + 1 : argv_eol[args],
                                  IRC_COLOR_CHAT_DELIMITERS);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_366: '366' command received (end of /names list)
 */

IRC_PROTOCOL_CALLBACK(366)
{
    struct t_irc_channel *ptr_channel;
    struct t_infolist *infolist;
    struct t_config_option *ptr_option;
    int num_nicks, num_op, num_halfop, num_voice, num_normal, length, i;
    char *string;
    const char *prefix, *prefix_color, *nickname;

    /*
     * 366 message looks like:
     *   :server 366 mynick #channel :End of /NAMES list.
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    ptr_channel = irc_channel_search (server, argv[3]);
    if (ptr_channel && ptr_channel->nicks)
    {
        /* display users on channel */
        infolist = weechat_infolist_get ("nicklist", ptr_channel->buffer, NULL);
        if (infolist)
        {
            length = 0;
            while (weechat_infolist_next (infolist))
            {
                if (strcmp (weechat_infolist_string (infolist, "type"),
                            "nick") == 0)
                {
                    ptr_option = weechat_config_get (weechat_infolist_string (infolist,
                                                                              "prefix_color"));
                    length +=
                        ((ptr_option) ? strlen (weechat_color (weechat_config_string (ptr_option))) : 0) +
                        strlen (weechat_infolist_string (infolist, "prefix")) +
                        16 + /* nick color */
                        strlen (weechat_infolist_string (infolist, "name")) +
                        16 + /* reset color */
                        1; /* space */
                }
            }
            if (length > 0)
            {
                string = malloc (length);
                if (string)
                {
                    string[0] = '\0';
                    i = 0;
                    while (weechat_infolist_next (infolist))
                    {
                        if (strcmp (weechat_infolist_string (infolist, "type"),
                                    "nick") == 0)
                        {
                            if (i > 0)
                            {
                                strcat (string, IRC_COLOR_RESET);
                                strcat (string, " ");
                            }
                            prefix = weechat_infolist_string (infolist, "prefix");
                            if (prefix[0] && (prefix[0] != ' '))
                            {
                                prefix_color = weechat_infolist_string (infolist,
                                                                        "prefix_color");
                                if (strchr (prefix_color, '.'))
                                {
                                    ptr_option = weechat_config_get (weechat_infolist_string (infolist,
                                                                                              "prefix_color"));
                                    if (ptr_option)
                                        strcat (string, weechat_color (weechat_config_string (ptr_option)));
                                }
                                else
                                {
                                    strcat (string, weechat_color (prefix_color));
                                }
                                strcat (string, prefix);
                            }
                            nickname = weechat_infolist_string (infolist, "name");
                            if (weechat_config_boolean (irc_config_look_color_nicks_in_names))
                            {
                                if (irc_server_strcasecmp (server, nickname, server->nick) == 0)
                                    strcat (string, IRC_COLOR_CHAT_NICK_SELF);
                                else
                                    strcat (string, irc_nick_find_color (nickname));
                            }
                            else
                                strcat (string, IRC_COLOR_RESET);
                            strcat (string, nickname);
                            i++;
                        }
                    }
                    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                               command, "names",
                                                                               ptr_channel->buffer),
                                              date,
                                              irc_protocol_tags (command, "irc_numeric", NULL),
                                              _("%sNicks %s%s%s: %s[%s%s]"),
                                              weechat_prefix ("network"),
                                              IRC_COLOR_CHAT_CHANNEL,
                                              ptr_channel->name,
                                              IRC_COLOR_RESET,
                                              IRC_COLOR_CHAT_DELIMITERS,
                                              string,
                                              IRC_COLOR_CHAT_DELIMITERS);
                    free (string);
                }
            }
            weechat_infolist_free (infolist);
        }

        /* display number of nicks, ops, halfops & voices on the channel */
        irc_nick_count (server, ptr_channel, &num_nicks, &num_op, &num_halfop,
                        &num_voice, &num_normal);
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "names",
                                                                   ptr_channel->buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  _("%sChannel %s%s%s: %s%d%s %s %s(%s%d%s %s, "
                                    "%s%d%s %s, %s%d%s %s, %s%d%s %s%s)"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  ptr_channel->name,
                                  IRC_COLOR_RESET,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  num_nicks,
                                  IRC_COLOR_RESET,
                                  NG_("nick", "nicks", num_nicks),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  num_op,
                                  IRC_COLOR_RESET,
                                  NG_("op", "ops", num_op),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  num_halfop,
                                  IRC_COLOR_RESET,
                                  NG_("halfop", "halfops", num_halfop),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  num_voice,
                                  IRC_COLOR_RESET,
                                  NG_("voice", "voices", num_voice),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  num_normal,
                                  IRC_COLOR_RESET,
                                  NG_("normal", "normals", num_normal),
                                  IRC_COLOR_CHAT_DELIMITERS);

        if (!ptr_channel->names_received)
        {
            irc_command_mode_server (server, ptr_channel, NULL,
                                     IRC_SERVER_SEND_OUTQ_PRIO_LOW);
            irc_channel_check_away (server, ptr_channel);
        }
    }
    else
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "names",
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s%s%s: %s",
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_RESET,
                                  (argv[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
    }

    if (ptr_channel)
        ptr_channel->names_received = 1;

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_367: '367' command received (banlist)
 */

IRC_PROTOCOL_CALLBACK(367)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    time_t datetime;

    /*
     * 367 message looks like:
     *   :server 367 mynick #channel banmask nick!user@host 1205590879
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    if (argc >= 7)
    {
        datetime = (time_t)(atol (argv[6]));
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "banlist",
                                                                   ptr_buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  /* TRANSLATORS: "%s" after "on" is a date */
                                  _("%s%s[%s%s%s] %s%s%s banned by "
                                    "%s%s %s(%s%s%s)%s on %s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  argv[4],
                                  IRC_COLOR_RESET,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     irc_message_get_nick_from_host (argv[5])),
                                  irc_message_get_nick_from_host (argv[5]),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  irc_message_get_address_from_host (argv[5]),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  weechat_util_get_time_string (&datetime));
    }
    else
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "banlist",
                                                                   ptr_buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  _("%s%s[%s%s%s] %s%s%s banned by "
                                    "%s%s %s(%s%s%s)"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  argv[4],
                                  IRC_COLOR_RESET,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     irc_message_get_nick_from_host (argv[5])),
                                  irc_message_get_nick_from_host (argv[5]),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  irc_message_get_address_from_host (argv[5]),
                                  IRC_COLOR_CHAT_DELIMITERS);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_368: '368' command received (end of banlist)
 */

IRC_PROTOCOL_CALLBACK(368)
{
    char *pos_args;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 368 message looks like:
     *   :server 368 mynick #channel :End of Channel Ban List
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    pos_args = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, "banlist",
                                                               ptr_buffer),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s[%s%s%s]%s%s%s",
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_CHAT_CHANNEL,
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (pos_args) ? " " : "",
                              (pos_args) ? pos_args : "");

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_432: '432' command received (erroneous nickname)
 */

IRC_PROTOCOL_CALLBACK(432)
{
    const char *alternate_nick;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 432 message looks like:
     *   :server 432 * mynick :Erroneous Nickname
     */

    irc_protocol_cb_generic_error (server,
                                   date, nick, address, host, command,
                                   ignored, argc, argv, argv_eol);

    if (!server->is_connected)
    {
        ptr_buffer = irc_msgbuffer_get_target_buffer (server, NULL,
                                                      command, NULL, NULL);

        alternate_nick = irc_server_get_alternate_nick (server);
        if (!alternate_nick)
        {
            weechat_printf_date_tags (ptr_buffer, date, NULL,
                                      _("%s%s: all declared nicknames are "
                                        "already in use or invalid, closing "
                                        "connection with server"),
                                      weechat_prefix ("error"),
                                      IRC_PLUGIN_NAME);
            irc_server_disconnect (server, 0, 1);
            return WEECHAT_RC_OK;
        }

        weechat_printf_date_tags (ptr_buffer, date, NULL,
                                  _("%s%s: nickname \"%s\" is invalid, "
                                    "trying nickname \"%s\""),
                                  weechat_prefix ("error"),
                                  IRC_PLUGIN_NAME, server->nick, alternate_nick);

        irc_server_set_nick (server, alternate_nick);

        irc_server_sendf (server, 0, NULL, "NICK %s", server->nick);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_433: '433' command received (nickname already in use)
 */

IRC_PROTOCOL_CALLBACK(433)
{
    const char *alternate_nick;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 433 message looks like:
     *   :server 433 * mynick :Nickname is already in use.
     */

    if (!server->is_connected)
    {
        ptr_buffer = irc_msgbuffer_get_target_buffer (server, NULL,
                                                      command, NULL, NULL);

        alternate_nick = irc_server_get_alternate_nick (server);
        if (!alternate_nick)
        {
            weechat_printf_date_tags (ptr_buffer, date, NULL,
                                      _("%s%s: all declared nicknames are "
                                        "already in use, closing "
                                        "connection with server"),
                                      weechat_prefix ("error"),
                                      IRC_PLUGIN_NAME);
            irc_server_disconnect (server, 0, 1);
            return WEECHAT_RC_OK;
        }

        weechat_printf_date_tags (ptr_buffer, date, NULL,
                                  _("%s%s: nickname \"%s\" is already in use, "
                                    "trying nickname \"%s\""),
                                  weechat_prefix ("network"),
                                  IRC_PLUGIN_NAME, server->nick, alternate_nick);

        irc_server_set_nick (server, alternate_nick);

        irc_server_sendf (server, 0, NULL, "NICK %s", server->nick);
    }
    else
    {
        return irc_protocol_cb_generic_error (server,
                                              date, nick, address, host,
                                              command, ignored, argc, argv,
                                              argv_eol);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_437: '437' command received (nick/channel temporarily
 *                      unavailable)
 */

IRC_PROTOCOL_CALLBACK(437)
{
    const char *alternate_nick;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 437 message looks like:
     *   :server 437 * mynick :Nick/channel is temporarily unavailable
     */

    irc_protocol_cb_generic_error (server,
                                   date, nick, address, host, command,
                                   ignored, argc, argv, argv_eol);

    if (!server->is_connected)
    {
        if ((argc >= 4)
            && (irc_server_strcasecmp (server, server->nick, argv[3]) == 0))
        {
            ptr_buffer = irc_msgbuffer_get_target_buffer (server, NULL,
                                                          command, NULL, NULL);

            alternate_nick = irc_server_get_alternate_nick (server);
            if (!alternate_nick)
            {
                weechat_printf_date_tags (ptr_buffer, date, NULL,
                                          _("%s%s: all declared nicknames are "
                                            "already in use or invalid, closing "
                                            "connection with server"),
                                          weechat_prefix ("error"),
                                          IRC_PLUGIN_NAME);
                irc_server_disconnect (server, 0, 1);
                return WEECHAT_RC_OK;
            }

            weechat_printf_date_tags (ptr_buffer, date, NULL,
                                      _("%s%s: nickname \"%s\" is unavailable, "
                                        "trying nickname \"%s\""),
                                      weechat_prefix ("error"),
                                      IRC_PLUGIN_NAME, server->nick, alternate_nick);

            irc_server_set_nick (server, alternate_nick);

            irc_server_sendf (server, 0, NULL, "NICK %s", server->nick);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_438: '438' command received (not authorized to change
 *                      nickname)
 */

IRC_PROTOCOL_CALLBACK(438)
{
    struct t_gui_buffer *ptr_buffer;

    /*
     * 438 message looks like:
     *   :server 438 mynick newnick :Nick change too fast. Please wait 30 seconds.
     */

    IRC_PROTOCOL_MIN_ARGS(4);

    ptr_buffer = irc_msgbuffer_get_target_buffer (server, NULL,
                                                  command, NULL, NULL);

    if (argc >= 5)
    {
        weechat_printf_date_tags (ptr_buffer,
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s (%s => %s)",
                                  weechat_prefix ("network"),
                                  (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4],
                                  argv[2],
                                  argv[3]);
    }
    else
    {
        weechat_printf_date_tags (ptr_buffer,
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s %s",
                                  weechat_prefix ("network"),
                                  argv[2],
                                  argv[3]);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_728: '728' command received (quietlist)
 */

IRC_PROTOCOL_CALLBACK(728)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    time_t datetime;

    /*
     * 728 message looks like:
     *   :server 728 mynick #channel mode quietmask nick!user@host 1351350090
     */

    IRC_PROTOCOL_MIN_ARGS(6);

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    if (argc >= 8)
    {
        datetime = (time_t)(atol (argv[7]));
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "quietlist",
                                                                   ptr_buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  /* TRANSLATORS: "%s" after "on" is a date */
                                  _("%s%s[%s%s%s] %s%s%s quieted by "
                                    "%s%s %s(%s%s%s)%s on %s"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  argv[5],
                                  IRC_COLOR_RESET,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     irc_message_get_nick_from_host (argv[6])),
                                  irc_message_get_nick_from_host (argv[6]),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  irc_message_get_address_from_host (argv[6]),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_RESET,
                                  weechat_util_get_time_string (&datetime));
    }
    else
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, "quietlist",
                                                                   ptr_buffer),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  _("%s%s[%s%s%s] %s%s%s quieted by "
                                    "%s%s %s(%s%s%s)"),
                                  weechat_prefix ("network"),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_CHANNEL,
                                  argv[3],
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  argv[5],
                                  IRC_COLOR_RESET,
                                  irc_nick_color_for_server_message (server, NULL,
                                                                     irc_message_get_nick_from_host (argv[6])),
                                  irc_message_get_nick_from_host (argv[6]),
                                  IRC_COLOR_CHAT_DELIMITERS,
                                  IRC_COLOR_CHAT_HOST,
                                  irc_message_get_address_from_host (argv[6]),
                                  IRC_COLOR_CHAT_DELIMITERS);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_729: '729' command received (end of quietlist)
 */

IRC_PROTOCOL_CALLBACK(729)
{
    char *pos_args;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;

    /*
     * 729 message looks like:
     *   :server 729 mynick #channel mode :End of Channel Quiet List
     */

    IRC_PROTOCOL_MIN_ARGS(5);

    pos_args = (argc > 5) ?
        ((argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5]) : NULL;

    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                               command, "quietlist",
                                                               ptr_buffer),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s[%s%s%s]%s%s%s",
                              weechat_prefix ("network"),
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_CHAT_CHANNEL,
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_RESET,
                              (pos_args) ? " " : "",
                              (pos_args) ? pos_args : "");

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_900: '900' command (logged in as (SASL))
 */

IRC_PROTOCOL_CALLBACK(900)
{
    /*
     * 900 message looks like:
     *   :server 900 mynick nick!user@host mynick :You are now logged in as mynick
     */

    IRC_PROTOCOL_MIN_ARGS(6);

    weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, argv[3],
                                                               command, NULL,
                                                               NULL),
                              date,
                              irc_protocol_tags (command, "irc_numeric", NULL),
                              "%s%s %s(%s%s%s)",
                              weechat_prefix ("network"),
                              (argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5],
                              IRC_COLOR_CHAT_DELIMITERS,
                              IRC_COLOR_CHAT_HOST,
                              argv[3],
                              IRC_COLOR_CHAT_DELIMITERS);

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_901: '901' command received (you are now logged in)
 */

IRC_PROTOCOL_CALLBACK(901)
{
    /*
     * 901 message looks like:
     *   :server 901 mynick nick user host :You are now logged in. (id nick, username user, hostname host)
     */

    IRC_PROTOCOL_MIN_ARGS(6);

    if (argc >= 7)
    {
        weechat_printf_date_tags (irc_msgbuffer_get_target_buffer (server, NULL,
                                                                   command, NULL,
                                                                   NULL),
                                  date,
                                  irc_protocol_tags (command, "irc_numeric", NULL),
                                  "%s%s",
                                  weechat_prefix ("network"),
                                  (argv_eol[6][0] == ':') ? argv_eol[6] + 1 : argv_eol[6]);
    }
    else
    {
        irc_protocol_cb_numeric (server,
                                 date, nick, address, host, command,
                                 ignored, argc, argv, argv_eol);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cb_sasl_end: '903' to '907' command received
 */

IRC_PROTOCOL_CALLBACK(sasl_end)
{
    /*
     * 903 message looks like:
     *   :server 903 nick :SASL authentication successful
     * 904 message looks like:
     *   :server 904 nick :SASL authentication failed
     */

    irc_protocol_cb_numeric (server,
                             date, nick, address, host, command,
                             ignored, argc, argv, argv_eol);

    if (!server->is_connected)
        irc_server_sendf (server, 0, NULL, "CAP END");

    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_get_message_tags: return hashtable with tags for an IRC message
 *                                example, if tags == "aaa=bbb;ccc;example.com/ddd=eee"
 *                                hashtable will have following keys/values:
 *                                  "aaa" => "bbb"
 *                                  "ccc" => NULL
 *                                  "example.com/ddd" => "eee"
 */

struct t_hashtable *
irc_protocol_get_message_tags (const char *tags)
{
    struct t_hashtable *hashtable;
    char **items, *pos, *key;
    int num_items, i;

    if (!tags || !tags[0])
        return NULL;

    hashtable = weechat_hashtable_new (8,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL,
                                       NULL);
    if (!hashtable)
        return NULL;

    items = weechat_string_split (tags, ";", 0, 0, &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            pos = strchr (items[i], '=');
            if (pos)
            {
                /* format: "tag=value" */
                key = weechat_strndup (items[i], pos - items[i]);
                if (key)
                {
                    weechat_hashtable_set (hashtable, key, pos + 1);
                    free (key);
                }
            }
            else
            {
                /* format: "tag" */
                weechat_hashtable_set (hashtable, items[i], NULL);
            }
        }
        weechat_string_free_split (items);
    }

    return hashtable;
}

/*
 * irc_protocol_get_message_tag_time: get value of time in tags
 *                                    if no tag "time" is in tags, value 0 is
 *                                    returned
 */

time_t
irc_protocol_get_message_tag_time (struct t_hashtable *tags)
{
    const char *tag_time;
    time_t time_value, time_msg, time_gm, time_local;
    struct tm tm_date, tm_date_gm, tm_date_local;

    if (!tags)
        return 0;

    time_value = 0;

    tag_time = weechat_hashtable_get (tags, "time");
    if (!tag_time)
        return time_value;

    /* initialize structure, because strptime does not do it */
    memset (&tm_date, 0, sizeof (struct tm));

    if (strchr (tag_time, '-'))
    {
        /* date is with ISO 8601 format: "2012-11-24T07:41:02.018Z" */
        strptime (tag_time, "%FT%T%z", &tm_date);
        if (tm_date.tm_year > 0)
        {
            time_msg = mktime (&tm_date);
            gmtime_r (&time_msg, &tm_date_gm);
            localtime_r (&time_msg, &tm_date_local);
            time_gm = mktime (&tm_date_gm);
            time_local = mktime (&tm_date_local);
            time_value = mktime (&tm_date_local) + (time_local - time_gm);
        }
    }
    else
    {
        /* date is with timestamp format: "1353403519.478" */
        strptime (tag_time, "%s", &tm_date);
        if (tm_date.tm_year > 0)
            time_value = mktime (&tm_date);
    }

    return time_value;
}

/*
 * irc_protocol_recv_command: execute action when an IRC message is received
 *                            Argument "irc_message" is the full message
 *                            without optional tags.
 */

void
irc_protocol_recv_command (struct t_irc_server *server,
                           const char *irc_message,
                           const char *msg_tags,
                           const char *msg_command,
                           const char *msg_channel)
{
    int i, cmd_found, return_code, argc, decode_color, keep_trailing_spaces;
    int message_ignored;
    char *dup_irc_message, *pos_space;
    struct t_irc_channel *ptr_channel;
    t_irc_recv_func *cmd_recv_func;
    const char *cmd_name;
    time_t date;
    const char *nick1, *address1, *host1;
    char *nick, *address, *address_color, *host, *host_no_color, *host_color;
    char **argv, **argv_eol;
    struct t_hashtable *hash_tags;
    struct t_irc_protocol_msg irc_protocol_messages[] =
        { { "authenticate", /* authenticate */ 1, 0, &irc_protocol_cb_authenticate },
          { "cap", /* client capability */ 1, 0, &irc_protocol_cb_cap },
          { "error", /* error received from IRC server */ 1, 0, &irc_protocol_cb_error },
          { "invite", /* invite a nick on a channel */ 1, 0, &irc_protocol_cb_invite },
          { "join", /* join a channel */ 1, 0, &irc_protocol_cb_join },
          { "kick", /* forcibly remove a user from a channel */ 1, 1, &irc_protocol_cb_kick },
          { "kill", /* close client-server connection */ 1, 1, &irc_protocol_cb_kill },
          { "mode", /* change channel or user mode */ 1, 0, &irc_protocol_cb_mode },
          { "nick", /* change current nickname */ 1, 0, &irc_protocol_cb_nick },
          { "notice", /* send notice message to user */ 1, 1, &irc_protocol_cb_notice },
          { "part", /* leave a channel */ 1, 1, &irc_protocol_cb_part },
          { "ping", /* ping server */ 1, 0, &irc_protocol_cb_ping },
          { "pong", /* answer to a ping message */ 1, 0, &irc_protocol_cb_pong },
          { "privmsg", /* message received */ 1, 1, &irc_protocol_cb_privmsg },
          { "quit", /* close all connections and quit */ 1, 1, &irc_protocol_cb_quit },
          { "topic", /* get/set channel topic */ 0, 1, &irc_protocol_cb_topic },
          { "wallops", /* send a message to all currently connected users who have "
                          "set the 'w' user mode "
                          "for themselves */ 1, 1, &irc_protocol_cb_wallops },
          { "001", /* a server message */ 1, 0, &irc_protocol_cb_001 },
          { "005", /* a server message */ 1, 0, &irc_protocol_cb_005 },
          { "221", /* user mode string */ 1, 0, &irc_protocol_cb_221 },
          { "223", /* whois (charset is) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "264", /* whois (is using encrypted connection) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "275", /* whois (secure connection) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "276", /* whois (has client certificate fingerprint) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "301", /* away message */ 1, 1, &irc_protocol_cb_301 },
          { "303", /* ison */ 1, 0, &irc_protocol_cb_303 },
          { "305", /* unaway */ 1, 0, &irc_protocol_cb_305 },
          { "306", /* now away */ 1, 0, &irc_protocol_cb_306 },
          { "307", /* whois (registered nick) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "310", /* whois (help mode) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "311", /* whois (user) */ 1, 0, &irc_protocol_cb_311 },
          { "312", /* whois (server) */ 1, 0, &irc_protocol_cb_312 },
          { "313", /* whois (operator) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "314", /* whowas */ 1, 0, &irc_protocol_cb_314 },
          { "315", /* end of /who list */ 1, 0, &irc_protocol_cb_315 },
          { "317", /* whois (idle) */ 1, 0, &irc_protocol_cb_317 },
          { "318", /* whois (end) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "319", /* whois (channels) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "320", /* whois (identified user) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "321", /* /list start */ 1, 0, &irc_protocol_cb_321 },
          { "322", /* channel (for /list) */ 1, 0, &irc_protocol_cb_322 },
          { "323", /* end of /list */ 1, 0, &irc_protocol_cb_323 },
          { "324", /* channel mode */ 1, 0, &irc_protocol_cb_324 },
          { "326", /* whois (has oper privs) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "327", /* whois (host) */ 1, 0, &irc_protocol_cb_327 },
          { "328", /* channel url */ 1, 0, &irc_protocol_cb_328 },
          { "329", /* channel creation date */ 1, 0, &irc_protocol_cb_329 },
          { "330", /* is logged in as */ 1, 0, &irc_protocol_cb_330_343 },
          { "331", /* no topic for channel */ 1, 0, &irc_protocol_cb_331 },
          { "332", /* topic of channel */ 0, 1, &irc_protocol_cb_332 },
          { "333", /* infos about topic (nick and date changed) */ 1, 0, &irc_protocol_cb_333 },
          { "335", /* is a bot on */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "338", /* whois (host) */ 1, 0, &irc_protocol_cb_338 },
          { "341", /* inviting */ 1, 0, &irc_protocol_cb_341 },
          { "343", /* is opered as */ 1, 0, &irc_protocol_cb_330_343 },
          { "344", /* channel reop */ 1, 0, &irc_protocol_cb_344 },
          { "345", /* end of channel reop list */ 1, 0, &irc_protocol_cb_345 },
          { "346", /* invite list */ 1, 0, &irc_protocol_cb_346 },
          { "347", /* end of invite list */ 1, 0, &irc_protocol_cb_347 },
          { "348", /* channel exception list */ 1, 0, &irc_protocol_cb_348 },
          { "349", /* end of channel exception list */ 1, 0, &irc_protocol_cb_349 },
          { "351", /* server version */ 1, 0, &irc_protocol_cb_351 },
          { "352", /* who */ 1, 0, &irc_protocol_cb_352 },
          { "353", /* list of nicks on channel */ 1, 0, &irc_protocol_cb_353 },
          { "366", /* end of /names list */ 1, 0, &irc_protocol_cb_366 },
          { "367", /* banlist */ 1, 0, &irc_protocol_cb_367 },
          { "368", /* end of banlist */ 1, 0, &irc_protocol_cb_368 },
          { "369", /* whowas (end) */ 1, 0, &irc_protocol_cb_whowas_nick_msg },
          { "378", /* whois (connecting from) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "379", /* whois (using modes) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "401", /* no such nick/channel */ 1, 0, &irc_protocol_cb_generic_error },
          { "402", /* no such server */ 1, 0, &irc_protocol_cb_generic_error },
          { "403", /* no such channel */ 1, 0, &irc_protocol_cb_generic_error },
          { "404", /* cannot send to channel */ 1, 0, &irc_protocol_cb_generic_error },
          { "405", /* too many channels */ 1, 0, &irc_protocol_cb_generic_error },
          { "406", /* was no such nick */ 1, 0, &irc_protocol_cb_generic_error },
          { "407", /* was no such nick */ 1, 0, &irc_protocol_cb_generic_error },
          { "409", /* no origin */ 1, 0, &irc_protocol_cb_generic_error },
          { "410", /* no services */ 1, 0, &irc_protocol_cb_generic_error },
          { "411", /* no recipient */ 1, 0, &irc_protocol_cb_generic_error },
          { "412", /* no text to send */ 1, 0, &irc_protocol_cb_generic_error },
          { "413", /* no toplevel */ 1, 0, &irc_protocol_cb_generic_error },
          { "414", /* wilcard in toplevel domain */ 1, 0, &irc_protocol_cb_generic_error },
          { "421", /* unknown command */ 1, 0, &irc_protocol_cb_generic_error },
          { "422", /* MOTD is missing */ 1, 0, &irc_protocol_cb_generic_error },
          { "423", /* no administrative info */ 1, 0, &irc_protocol_cb_generic_error },
          { "424", /* file error */ 1, 0, &irc_protocol_cb_generic_error },
          { "431", /* no nickname given */ 1, 0, &irc_protocol_cb_generic_error },
          { "432", /* erroneous nickname */ 1, 0, &irc_protocol_cb_432 },
          { "433", /* nickname already in use */ 1, 0, &irc_protocol_cb_433 },
          { "436", /* nickname collision */ 1, 0, &irc_protocol_cb_generic_error },
          { "437", /* nick/channel unavailable */ 1, 0, &irc_protocol_cb_437 },
          { "438", /* not authorized to change nickname */ 1, 0, &irc_protocol_cb_438 },
          { "441", /* user not in channel */ 1, 0, &irc_protocol_cb_generic_error },
          { "442", /* not on channel */ 1, 0, &irc_protocol_cb_generic_error },
          { "443", /* user already on channel */ 1, 0, &irc_protocol_cb_generic_error },
          { "444", /* user not logged in */ 1, 0, &irc_protocol_cb_generic_error },
          { "445", /* summon has been disabled */ 1, 0, &irc_protocol_cb_generic_error },
          { "446", /* users has been disabled */ 1, 0, &irc_protocol_cb_generic_error },
          { "451", /* you are not registered */ 1, 0, &irc_protocol_cb_generic_error },
          { "461", /* not enough parameters */ 1, 0, &irc_protocol_cb_generic_error },
          { "462", /* you may not register */ 1, 0, &irc_protocol_cb_generic_error },
          { "463", /* your host isn't among the privileged */ 1, 0, &irc_protocol_cb_generic_error },
          { "464", /* password incorrect */ 1, 0, &irc_protocol_cb_generic_error },
          { "465", /* you are banned from this server */ 1, 0, &irc_protocol_cb_generic_error },
          { "467", /* channel key already set */ 1, 0, &irc_protocol_cb_generic_error },
          { "470", /* forwarding to another channel */ 1, 0, &irc_protocol_cb_generic_error },
          { "471", /* channel is already full */ 1, 0, &irc_protocol_cb_generic_error },
          { "472", /* unknown mode char to me */ 1, 0, &irc_protocol_cb_generic_error },
          { "473", /* cannot join channel (invite only) */ 1, 0, &irc_protocol_cb_generic_error },
          { "474", /* cannot join channel (banned from channel) */ 1, 0, &irc_protocol_cb_generic_error },
          { "475", /* cannot join channel (bad channel key) */ 1, 0, &irc_protocol_cb_generic_error },
          { "476", /* bad channel mask */ 1, 0, &irc_protocol_cb_generic_error },
          { "477", /* channel doesn't support modes */ 1, 0, &irc_protocol_cb_generic_error },
          { "481", /* you're not an IRC operator */ 1, 0, &irc_protocol_cb_generic_error },
          { "482", /* you're not channel operator */ 1, 0, &irc_protocol_cb_generic_error },
          { "483", /* you can't kill a server! */ 1, 0, &irc_protocol_cb_generic_error },
          { "484", /* your connection is restricted! */ 1, 0, &irc_protocol_cb_generic_error },
          { "485", /* user is immune from kick/deop */ 1, 0, &irc_protocol_cb_generic_error },
          { "487", /* network split */ 1, 0, &irc_protocol_cb_generic_error },
          { "491", /* no O-lines for your host */ 1, 0, &irc_protocol_cb_generic_error },
          { "501", /* unknown mode flag */ 1, 0, &irc_protocol_cb_generic_error },
          { "502", /* can't change mode for other users */ 1, 0, &irc_protocol_cb_generic_error },
          { "671", /* whois (secure connection) */ 1, 0, &irc_protocol_cb_whois_nick_msg },
          { "728", /* quietlist */ 1, 0, &irc_protocol_cb_728 },
          { "729", /* end of quietlist */ 1, 0, &irc_protocol_cb_729 },
          { "900", /* logged in as (SASL) */ 1, 0, &irc_protocol_cb_900 },
          { "901", /* you are now logged in */ 1, 0, &irc_protocol_cb_901 },
          { "903", /* SASL authentication successful */ 1, 0, &irc_protocol_cb_sasl_end },
          { "904", /* SASL authentication failed */ 1, 0, &irc_protocol_cb_sasl_end },
          { "905", /* SASL message too long */ 1, 0, &irc_protocol_cb_sasl_end },
          { "906", /* SASL authentication aborted */ 1, 0, &irc_protocol_cb_sasl_end },
          { "907", /* You have already completed SASL authentication */ 1, 0, &irc_protocol_cb_sasl_end },
          { "973", /* whois (secure connection) */ 1, 0, &irc_protocol_cb_server_mode_reason },
          { "974", /* whois (secure connection) */ 1, 0, &irc_protocol_cb_server_mode_reason },
          { "975", /* whois (secure connection) */ 1, 0, &irc_protocol_cb_server_mode_reason },
          { NULL, 0, 0, NULL }
        };

    if (!msg_command)
        return;

    dup_irc_message = NULL;
    argv = NULL;
    argv_eol = NULL;
    hash_tags = NULL;
    date = 0;

    /* get tags as hashtable */
    if (msg_tags)
    {
        hash_tags = irc_protocol_get_message_tags (msg_tags);
        if (hash_tags)
            date = irc_protocol_get_message_tag_time (hash_tags);
    }

    /* get nick/host/address from IRC message */
    nick1 = NULL;
    address1 = NULL;
    host1 = NULL;
    if (irc_message && (irc_message[0] == ':'))
    {
        nick1 = irc_message_get_nick_from_host (irc_message);
        address1 = irc_message_get_address_from_host (irc_message);
        host1 = irc_message + 1;
    }
    nick = (nick1) ? strdup (nick1) : NULL;
    address = (address1) ? strdup (address1) : NULL;
    address_color = (address) ? irc_color_decode (address,
                                                  weechat_config_boolean (irc_config_network_colors_receive)) : NULL;
    host = (host1) ? strdup (host1) : NULL;
    if (host)
    {
        pos_space = strchr (host, ' ');
        if (pos_space)
            pos_space[0] = '\0';
    }
    host_no_color = (host) ? irc_color_decode (host, 0) : NULL;
    host_color = (host) ? irc_color_decode (host,
                                            weechat_config_boolean (irc_config_network_colors_receive)) : NULL;

    /* check if message is ignored or not */
    ptr_channel = NULL;
    if (msg_channel)
        ptr_channel = irc_channel_search (server, msg_channel);
    message_ignored = irc_ignore_check (server,
                                        (ptr_channel) ? ptr_channel->name : msg_channel,
                                        nick, host_no_color);

    /* send signal with received command, even if command is ignored */
    irc_server_send_signal (server, "irc_raw_in", msg_command,
                            irc_message, NULL);

    /* send signal with received command, only if message is not ignored */
    if (!message_ignored)
    {
        irc_server_send_signal (server, "irc_in", msg_command,
                                irc_message, NULL);
    }

    /* look for IRC command */
    cmd_found = -1;
    for (i = 0; irc_protocol_messages[i].name; i++)
    {
        if (weechat_strcasecmp (irc_protocol_messages[i].name, msg_command) == 0)
        {
            cmd_found = i;
            break;
        }
    }

    /* command not found */
    if (cmd_found < 0)
    {
        /* for numeric commands, we use default recv function */
        if (irc_protocol_is_numeric_command (msg_command))
        {
            cmd_name = msg_command;
            decode_color = 1;
            keep_trailing_spaces = 0;
            cmd_recv_func = irc_protocol_cb_numeric;
        }
        else
        {
            weechat_printf (server->buffer,
                            _("%s%s: command \"%s\" not found:"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            msg_command);
            weechat_printf (server->buffer,
                            "%s%s",
                            weechat_prefix ("error"), irc_message);
            goto end;
        }
    }
    else
    {
        cmd_name = irc_protocol_messages[cmd_found].name;
        decode_color = irc_protocol_messages[cmd_found].decode_color;
        keep_trailing_spaces = irc_protocol_messages[cmd_found].keep_trailing_spaces;
        cmd_recv_func = irc_protocol_messages[cmd_found].recv_function;
    }

    if (cmd_recv_func != NULL)
    {
        if (irc_message)
        {
            if (decode_color)
                dup_irc_message = irc_color_decode (irc_message,
                                                    weechat_config_boolean (irc_config_network_colors_receive));
            else
                dup_irc_message = strdup (irc_message);
        }
        else
            dup_irc_message = NULL;
        argv = weechat_string_split (dup_irc_message, " ", 0, 0, &argc);
        argv_eol = weechat_string_split (dup_irc_message, " ",
                                         1 + keep_trailing_spaces, 0, NULL);

        return_code = (int) (cmd_recv_func) (server,
                                             date, nick, address_color,
                                             host_color, cmd_name,
                                             message_ignored, argc, argv,
                                             argv_eol);

        if (return_code == WEECHAT_RC_ERROR)
        {
            weechat_printf (server->buffer,
                            _("%s%s: failed to parse command \"%s\" (please "
                              "report to developers):"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            msg_command);
            weechat_printf (server->buffer,
                            "%s%s",
                            weechat_prefix ("error"), irc_message);
        }

        /* send signal with received command (if message is not ignored) */
        if (!message_ignored)
        {
            irc_server_send_signal (server, "irc_in2", msg_command,
                                    irc_message, NULL);
        }
    }

    /* send signal with received command, even if command is ignored */
    irc_server_send_signal (server, "irc_raw_in2", msg_command,
                            irc_message, NULL);

end:
    if (nick)
        free (nick);
    if (address)
        free (address);
    if (address_color)
        free (address_color);
    if (host)
        free (host);
    if (host_no_color)
        free (host_no_color);
    if (host_color)
        free (host_color);
    if (dup_irc_message)
        free (dup_irc_message);
    if (argv)
        weechat_string_free_split (argv);
    if (argv_eol)
        weechat_string_free_split (argv_eol);
    if (hash_tags)
        weechat_hashtable_free (hash_tags);
}
