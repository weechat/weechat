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

/* irc-protocol.c: implementation of IRC protocol, according to
                   RFC 1459, 2810, 2811 2812 */


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
#include "irc-color.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"
#include "irc-mode.h"
#include "irc-ignore.h"


/*
 * irc_protocol_get_nick_from_host: get nick from host in an IRC message
 */

char *
irc_protocol_get_nick_from_host (const char *host)
{
    static char nick[128];
    char *pos;
    
    if (!host)
        return NULL;
    
    nick[0] = '\0';
    if (host)
    {
        if (host[0] == ':')
            host++;
        pos = strchr (host, '!');
        if (pos && (pos - host < (int)sizeof (nick)))
        {
            strncpy (nick, host, pos - host);
            nick[pos - host] = '\0';
        }
        else
            snprintf (nick, sizeof (nick), "%s", host);
    }
    return nick;
}

/*
 * irc_protocol_get_address_from_host: get address from host in an IRC message
 */

char *
irc_protocol_get_address_from_host (const char *host)
{
    static char address[256];
    char *pos;
    
    address[0] = '\0';
    if (host)
    {
        if (host[0] == ':')
            host++;
        pos = strchr (host, '!');
        if (pos)
            snprintf (address, sizeof (address), "%s", pos + 1);
        else
            snprintf (address, sizeof (address), "%s", host);
    }
    return address;
}

/*
 * irc_protocol_tags: build tags list with IRC command and/or tags
 */

char *
irc_protocol_tags (const char *command, const char *tags)
{
    static char string[256];
    
    if (command && tags)
    {
        snprintf (string, sizeof (string), "irc_cmd_%s,%s", command, tags);
        return string;
    }
    
    if (command)
    {
        snprintf (string, sizeof (string), "irc_cmd_%s", command);
        return string;
    }
    
    if (tags)
    {
        snprintf (string, sizeof (string), "%s", tags);
        return string;
    }
    
    return NULL;
}

/*
 * irc_protocol_replace_vars: replace special IRC vars ($nick, $channel,
 *                            $server) in a string
 *                            Note: result has to be free() after use
 */ 

char *
irc_protocol_replace_vars (struct t_irc_server *server,
                           struct t_irc_channel *channel, const char *string)
{
    char *var_nick, *var_channel, *var_server;
    char empty_string[1] = { '\0' };
    char *res, *temp;
    
    var_nick = (server && server->nick) ? server->nick : empty_string;
    var_channel = (channel) ? channel->name : empty_string;
    var_server = (server) ? server->name : empty_string;
    
    /* replace nick */
    temp = weechat_string_replace (string, "$nick", var_nick);
    if (!temp)
        return NULL;
    res = temp;
    
    /* replace channel */
    temp = weechat_string_replace (res, "$channel", var_channel);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* replace server */
    temp = weechat_string_replace (res, "$server", var_server);
    free (res);
    if (!temp)
        return NULL;
    res = temp;
    
    /* return result */
    return res;
}

/*
 * irc_protocol_cmd_error: error received from server
 */

int
irc_protocol_cmd_error (struct t_irc_server *server, const char *command,
                        int argc, char **argv, char **argv_eol)
{
    int first_arg;
    char *chan_nick, *args;
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    /* make C compiler happy */
    (void) argc;
    
    first_arg = (strcmp (argv[2], server->nick) == 0) ? 3 : 2;
    
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
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags (command, "irc_error"),
                         "%s%s%s%s",
                         irc_buffer_get_server_prefix (server, "network"),
                         (chan_nick) ? chan_nick : "",
                         (chan_nick) ? ": " : "",
                         args);
    
    if (strncmp (args, "Closing Link", 12) == 0)
        irc_server_disconnect (server, 1);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_invite: 'invite' message received
 */

int
irc_protocol_cmd_invite (struct t_irc_server *server, const char *command,
                         int argc, char **argv, char **argv_eol)
{
    /* INVITE message looks like:
       :nick!user@host INVITE mynick :#channel
    */
    
    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) argv_eol;
    
    if (!irc_ignore_check (server, NULL, nick, host))
    {
        weechat_printf_tags (server->buffer,
                             "irc_invite,notify_highlight",
                             _("%sYou have been invited to %s%s%s by "
                               "%s%s"),
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_CHANNEL,
                             (argv[3][0] == ':') ? argv[3] + 1 : argv[3],
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_NICK,
                             nick);
    }
    return WEECHAT_RC_OK;
}


/*
 * irc_protocol_cmd_join: 'join' message received
 */

int
irc_protocol_cmd_join (struct t_irc_server *server, const char *command,
                       int argc, char **argv, char **argv_eol)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    char *pos_channel;
    
    /* JOIN message looks like:
       :nick!user@host JOIN :#channel
    */

    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) argv_eol;
    
    pos_channel = (argv[2][0] == ':') ? argv[2] + 1 : argv[2];
    
    ptr_channel = irc_channel_search (server, pos_channel);
    if (!ptr_channel)
    {
        ptr_channel = irc_channel_new (server, IRC_CHANNEL_TYPE_CHANNEL,
                                       pos_channel, 1);
        if (!ptr_channel)
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot create new channel \"%s\""),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME, pos_channel);
            return WEECHAT_RC_ERROR;
        }
    }
    
    if (!irc_ignore_check (server, ptr_channel, nick, host))
    {
        weechat_printf_tags (ptr_channel->buffer,
                             "irc_join",
                             _("%s%s%s %s(%s%s%s)%s has joined %s%s"),
                             weechat_prefix ("join"),
                             IRC_COLOR_CHAT_NICK,
                             nick,
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_HOST,
                             address,
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_CHANNEL,
                             pos_channel);
    }
    
    /* remove topic and display channel creation date if joining new channel */
    if (!ptr_channel->nicks)
    {
        if (ptr_channel->topic)
            irc_channel_set_topic (ptr_channel, NULL);
        
        ptr_channel->display_creation_date = 1;
    }
    
    /* add nick in channel */
    ptr_nick = irc_nick_new (server, ptr_channel, nick, 0, 0, 0, 0, 0, 0, 0);
    if (ptr_nick)
        ptr_nick->host = strdup (address);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_kick: 'kick' message received
 */

int
irc_protocol_cmd_kick (struct t_irc_server *server, const char *command,
                       int argc, char **argv, char **argv_eol)
{
    char *pos_comment;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* KICK message looks like:
       :nick1!user@host KICK #channel nick2 :kick reason
    */
    
    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;
    
    pos_comment = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;
    
    ptr_channel = irc_channel_search (server, argv[2]);
    if (!ptr_channel)
    {
        weechat_printf (server->buffer,
                        _("%s%s: channel \"%s\" not found for "
                          "\"%s\" command"),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME, argv[2], "kick");
        return WEECHAT_RC_ERROR;
    }
    
    if (!irc_ignore_check (server, ptr_channel, nick, host))
    {
        if (pos_comment)
        {
            weechat_printf_tags (ptr_channel->buffer,
                                 "irc_kick",
                                 _("%s%s%s%s has kicked %s%s%s from %s%s "
                                   "%s(%s%s%s)"),
                                 weechat_prefix ("quit"),
                                 IRC_COLOR_CHAT_NICK,
                                 nick,
                                 IRC_COLOR_CHAT,
                                 IRC_COLOR_CHAT_NICK,
                                 argv[3],
                                 IRC_COLOR_CHAT,
                                 IRC_COLOR_CHAT_CHANNEL,
                                 argv[2],
                                 IRC_COLOR_CHAT_DELIMITERS,
                                 IRC_COLOR_CHAT,
                                 pos_comment,
                                 IRC_COLOR_CHAT_DELIMITERS);
        }
        else
        {
            weechat_printf_tags (ptr_channel->buffer,
                                 "irc_kick",
                                 _("%s%s%s%s has kicked %s%s%s from %s%s"),
                                 weechat_prefix ("quit"),
                                 IRC_COLOR_CHAT_NICK,
                                 nick,
                                 IRC_COLOR_CHAT,
                                 IRC_COLOR_CHAT_NICK,
                                 argv[3],
                                 IRC_COLOR_CHAT,
                                 IRC_COLOR_CHAT_CHANNEL,
                                 argv[2]);
        }
    }
    
    if (strcmp (argv[3], server->nick) == 0)
    {
        /* my nick was kicked => free all nicks, channel is not active any
           more */
        irc_nick_free_all (ptr_channel);
        if (server->autorejoin)
            irc_command_join_server (server, ptr_channel->name);
    }
    else
    {
        /* someone was kicked from channel (but not me) => remove only this
           nick */
        ptr_nick = irc_nick_search (ptr_channel, argv[3]);
        if (ptr_nick)
            irc_nick_free (ptr_channel, ptr_nick);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_kill: 'kill' message received
 */

int
irc_protocol_cmd_kill (struct t_irc_server *server, const char *command,
                       int argc, char **argv, char **argv_eol)
{
    char *pos_comment;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* KILL message looks like:
       :nick1!user@host KILL mynick :kill reason
    */
    
    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;
    
    pos_comment = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (!irc_ignore_check (server, ptr_channel, nick, host))
        {
            if (pos_comment)
            {
                weechat_printf_tags (ptr_channel->buffer,
                                     "irc_kill",
                                     _("%sYou were killed by %s%s %s(%s%s%s)"),
                                     weechat_prefix ("quit"),
                                     IRC_COLOR_CHAT_NICK,
                                     nick,
                                     IRC_COLOR_CHAT_DELIMITERS,
                                     IRC_COLOR_CHAT,
                                     pos_comment,
                                     IRC_COLOR_CHAT_DELIMITERS);
            }
            else
            {
                weechat_printf_tags (ptr_channel->buffer,
                                     "irc_kill",
                                     _("%sYou were killed by %s%s"),
                                     weechat_prefix ("quit"),
                                     IRC_COLOR_CHAT_NICK,
                                     nick);
            }
        }
        
        if (strcmp (argv[2], server->nick) == 0)
        {
            /* my nick was killed => free all nicks, channel is not active any
               more */
            irc_nick_free_all (ptr_channel);
        }
        else
        {
            /* someone was killed on channel (but not me) => remove only this
               nick */
            ptr_nick = irc_nick_search (ptr_channel, argv[2]);
            if (ptr_nick)
                irc_nick_free (ptr_channel, ptr_nick);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_mode: 'mode' message received
 */

int
irc_protocol_cmd_mode (struct t_irc_server *server, const char *command,
                       int argc, char **argv, char **argv_eol)
{
    char *pos_modes;
    struct t_irc_channel *ptr_channel;
    
    /* MODE message looks like:
       :nick!user@host MODE #test +o nick
    */
    
    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;
    
    pos_modes = (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3];
    
    if (irc_channel_is_channel (argv[2]))
    {
        ptr_channel = irc_channel_search (server, argv[2]);
        if (ptr_channel)
        {
            irc_mode_channel_set (server, ptr_channel, pos_modes);
            irc_server_sendf (server, "MODE %s", ptr_channel->name);
        }
        if (!irc_ignore_check (server, ptr_channel, nick, host))
        {
            weechat_printf_tags ((ptr_channel) ?
                                 ptr_channel->buffer : server->buffer,
                                 "irc_mode",
                                 _("%sMode %s%s %s[%s%s%s]%s by %s%s"),
                                 (ptr_channel) ? weechat_prefix ("network") : irc_buffer_get_server_prefix (server, "network"),
                                 IRC_COLOR_CHAT_CHANNEL,
                                 (ptr_channel) ? ptr_channel->name : argv[2],
                                 IRC_COLOR_CHAT_DELIMITERS,
                                 IRC_COLOR_CHAT,
                                 pos_modes,
                                 IRC_COLOR_CHAT_DELIMITERS,
                                 IRC_COLOR_CHAT,
                                 IRC_COLOR_CHAT_NICK,
                                 nick);
        }
    }
    else
    {
        if (!irc_ignore_check (server, NULL, nick, host))
        {
            weechat_printf_tags (server->buffer,
                                 "irc_mode",
                                 _("%sUser mode %s[%s%s%s]%s by %s%s"),
                                 irc_buffer_get_server_prefix (server, "network"),
                                 IRC_COLOR_CHAT_DELIMITERS,
                                 IRC_COLOR_CHAT,
                                 pos_modes,
                                 IRC_COLOR_CHAT_DELIMITERS,
                                 IRC_COLOR_CHAT,
                                 IRC_COLOR_CHAT_NICK,
                                 nick);
        }
        irc_mode_user_set (server, pos_modes);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_nick: 'nick' message received
 */

int
irc_protocol_cmd_nick (struct t_irc_server *server, const char *command,
                       int argc, char **argv, char **argv_eol)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    char *new_nick;
    int nick_is_me;
    
    /* NICK message looks like:
       :oldnick!user@host NICK :newnick
    */
    
    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) argv_eol;
    
    new_nick = (argv[2][0] == ':') ? argv[2] + 1 : argv[2];
    
    nick_is_me = (strcmp (nick, server->nick) == 0) ? 1 : 0;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        switch (ptr_channel->type)
        {
            case IRC_CHANNEL_TYPE_PRIVATE:
                /* rename private window if this is with "old nick" */
                if (weechat_strcasecmp (ptr_channel->name, nick) == 0)
                {
                    free (ptr_channel->name);
                    ptr_channel->name = strdup (new_nick);
                    weechat_buffer_set (ptr_channel->buffer, "name", new_nick);
                }
                break;
            case IRC_CHANNEL_TYPE_CHANNEL:
                /* rename nick in nicklist if found */
                ptr_nick = irc_nick_search (ptr_channel, nick);
                if (ptr_nick)
                {
                    /* temporary disable hotlist */
                    weechat_buffer_set (NULL, "hotlist", "-");
                    
                    /* change nick and display message on all channels */
                    irc_nick_change (server, ptr_channel, ptr_nick, new_nick);
                    if (nick_is_me)
                    {
                        weechat_printf_tags (ptr_channel->buffer,
                                             "irc_nick",
                                             _("%sYou are now known as "
                                               "%s%s"),
                                             weechat_prefix ("network"),
                                             IRC_COLOR_CHAT_NICK,
                                             new_nick);
                    }
                    else
                    {
                        if (!irc_ignore_check (server, ptr_channel, nick, host))
                        {
                            weechat_printf_tags (ptr_channel->buffer,
                                                 "irc_nick",
                                                 _("%s%s%s%s is now known as "
                                                   "%s%s"),
                                                 weechat_prefix ("network"),
                                                 IRC_COLOR_CHAT_NICK,
                                                 nick,
                                                 IRC_COLOR_CHAT,
                                                 IRC_COLOR_CHAT_NICK,
                                                 new_nick);
                        }
                    }
                    
                    /* enable hotlist */
                    weechat_buffer_set (NULL, "hotlist", "+");
                }
                break;
        }
    }
    
    if (nick_is_me)
        irc_server_set_nick (server, new_nick);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_notice: 'notice' message received
 */

int
irc_protocol_cmd_notice (struct t_irc_server *server, const char *command,
                         int argc, char **argv, char **argv_eol)
{
    char *pos_args, *pos_end, *pos_usec, tags[128];
    struct timeval tv;
    long sec1, usec1, sec2, usec2, difftime;
    struct t_irc_channel *ptr_channel;
    
    /* NOTICE message looks like:
       NOTICE AUTH :*** Looking up your hostname...
       :nick!user@host NOTICE mynick :notice text
    */
    
    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(3);

    if (argv[0][0] == ':')
        pos_args = (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3];
    else
        pos_args = (argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2];
    
    if (nick && irc_ignore_check (server, NULL, nick, host))
        return WEECHAT_RC_OK;
    
    if (nick && strncmp (pos_args, "\01VERSION", 8) == 0)
    {
        pos_args += 9;
        pos_end = strchr (pos_args, '\01');
        if (pos_end)
            pos_end[0] = '\0';
        weechat_printf_tags (server->buffer,
                             "irc_notice",
                             _("%sCTCP %sVERSION%s reply from %s%s%s: %s"),
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_CHANNEL,
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_NICK,
                             nick,
                             IRC_COLOR_CHAT,
                             pos_args);
        if (pos_end)
            pos_end[0] = '\01';
    }
    else
    {
        if (nick && strncmp (pos_args, "\01PING", 5) == 0)
        {
            pos_args += 5;
            while (pos_args[0] == ' ')
            {
                pos_args++;
            }
            pos_usec = strchr (pos_args, ' ');
            if (pos_usec)
            {
                pos_usec[0] = '\0';
                pos_end = strchr (pos_usec + 1, '\01');
                if (pos_end)
                {
                    pos_end[0] = '\0';
                        
                    gettimeofday (&tv, NULL);
                    sec1 = atol (pos_args);
                    usec1 = atol (pos_usec + 1);
                    sec2 = tv.tv_sec;
                    usec2 = tv.tv_usec;
                        
                    difftime = ((sec2 * 1000000) + usec2) -
                        ((sec1 * 1000000) + usec1);
                        
                    weechat_printf_tags (server->buffer,
                                         "irc_notice,irc_ctcp",
                                         _("%sCTCP %sPING%s reply from "
                                           "%s%s%s: %ld.%ld %s"),
                                         irc_buffer_get_server_prefix (server,
                                                                       "network"),
                                         IRC_COLOR_CHAT_CHANNEL,
                                         IRC_COLOR_CHAT,
                                         IRC_COLOR_CHAT_NICK,
                                         nick,
                                         IRC_COLOR_CHAT,
                                         difftime / 1000000,
                                         (difftime % 1000000) / 1000,
                                         (NG_("second", "seconds",
                                              (difftime / 1000000))));
                        
                    pos_end[0] = '\01';
                }
                pos_usec[0] = ' ';
            }
        }
        else
        {
            if (nick
                && (weechat_strcasecmp (nick, "nickserv") != 0)
                && (weechat_strcasecmp (nick, "chanserv") != 0)
                && (weechat_strcasecmp (nick, "memoserv") != 0))
            {
                snprintf (tags, sizeof (tags),
                          "%s", "irc_notice,notify_private");
            }
            else
            {
                snprintf (tags, sizeof (tags),
                          "%s", "irc_notice");
            }
            if (nick && weechat_config_boolean (irc_config_look_notice_as_pv))
            {
                ptr_channel = irc_channel_search (server, nick);
                if (!ptr_channel)
                {
                    ptr_channel = irc_channel_new (server,
                                                   IRC_CHANNEL_TYPE_PRIVATE,
                                                   nick, 0);
                    if (!ptr_channel)
                    {
                        weechat_printf (server->buffer,
                                        _("%s%s: cannot create new "
                                          "private buffer \"%s\""),
                                        irc_buffer_get_server_prefix (server,
                                                                      "error"),
                                        IRC_PLUGIN_NAME, nick);
                        return WEECHAT_RC_ERROR;
                    }
                }
                if (!ptr_channel->topic)
                    irc_channel_set_topic (ptr_channel, address);
                
                weechat_printf_tags (ptr_channel->buffer,
                                     tags,
                                     "%s%s",
                                     irc_nick_as_prefix (NULL, nick,
                                                         IRC_COLOR_CHAT_NICK_OTHER),
                                     pos_args);
            }
            else
            {
                if (address && address[0])
                {
                    weechat_printf_tags (server->buffer,
                                         tags,
                                         "%s%s%s %s(%s%s%s)%s: %s",
                                         irc_buffer_get_server_prefix (server,
                                                                       "network"),
                                         IRC_COLOR_CHAT_NICK,
                                         nick,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT_HOST,
                                         address,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT,
                                         pos_args);
                }
                else
                {
                    if (nick && nick[0])
                    {
                        weechat_printf_tags (server->buffer,
                                             tags,
                                             "%s%s%s%s: %s",
                                             irc_buffer_get_server_prefix (server,
                                                                           "network"),
                                             IRC_COLOR_CHAT_NICK,
                                             nick,
                                             IRC_COLOR_CHAT,
                                             pos_args);
                    }
                    else
                    {
                        weechat_printf_tags (server->buffer,
                                             tags,
                                             "%s%s",
                                             irc_buffer_get_server_prefix (server,
                                                                           "network"),
                                             pos_args);
                    }
                }
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_part: 'part' message received
 */

int
irc_protocol_cmd_part (struct t_irc_server *server, const char *command,
                       int argc, char **argv, char **argv_eol)
{
    char *pos_comment, *join_string;
    int join_length;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* PART message looks like:
       :nick!user@host PART #channel :part message
    */
    
    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;
    
    pos_comment = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    
    ptr_channel = irc_channel_search (server, argv[2]);
    if (ptr_channel)
    {
        ptr_nick = irc_nick_search (ptr_channel, nick);
        if (ptr_nick)
        {
            /* display part message */
            if (!irc_ignore_check (server, ptr_channel, nick, host))
            {
                if (pos_comment)
                {
                    weechat_printf_tags (ptr_channel->buffer,
                                         "irc_part",
                                         _("%s%s%s %s(%s%s%s)%s has left %s%s "
                                           "%s(%s%s%s)"),
                                         weechat_prefix ("quit"),
                                         IRC_COLOR_CHAT_NICK,
                                         nick,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT_HOST,
                                         address,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT,
                                         IRC_COLOR_CHAT_CHANNEL,
                                         ptr_channel->name,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT,
                                         pos_comment,
                                         IRC_COLOR_CHAT_DELIMITERS);
                }
                else
                {
                    weechat_printf_tags (ptr_channel->buffer,
                                         "irc_part",
                                         _("%s%s%s %s(%s%s%s)%s has left "
                                           "%s%s"),
                                         weechat_prefix ("quit"),
                                         IRC_COLOR_CHAT_NICK,
                                         nick,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT_HOST,
                                         address,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT,
                                         IRC_COLOR_CHAT_CHANNEL,
                                         ptr_channel->name);
                }
            }
            
            /* part request was issued by local client ? */
            if (strcmp (ptr_nick->name, server->nick) == 0)
            {
                irc_nick_free_all (ptr_channel);
                
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
                            irc_command_join_server (server, join_string);
                            free (join_string);
                        }
                        else
                            irc_command_join_server (server, ptr_channel->name);
                    }
                    else
                        irc_command_join_server (server, ptr_channel->name);
                }
            }
            else
                irc_nick_free (ptr_channel, ptr_nick);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_ping: 'ping' command received
 */

int
irc_protocol_cmd_ping (struct t_irc_server *server, const char *command,
                       int argc, char **argv, char **argv_eol)
{
    /* PING message looks like:
       PING :server
    */
    
    IRC_PROTOCOL_MIN_ARGS(2);
    
    /* make C compiler happy */
    (void) argv_eol;
    
    irc_server_sendf (server, "PONG :%s",
                      (argv[1][0] == ':') ? argv[1] + 1 : argv[1]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_pong: 'pong' command received
 */

int
irc_protocol_cmd_pong (struct t_irc_server *server, const char *command,
                       int argc, char **argv, char **argv_eol)
{
    struct timeval tv;
    int old_lag;
    
    /* make C compiler happy */
    (void) command;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    
    if (server->lag_check_time.tv_sec != 0)
    {
        /* calculate lag (time diff with lag check) */
        old_lag = server->lag;
        gettimeofday (&tv, NULL);
        server->lag = (int) weechat_timeval_diff (&(server->lag_check_time),
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
 * irc_protocol_reply_version: send version in reply to "CTCP VERSION" request
 */

void
irc_protocol_reply_version (struct t_irc_server *server,
                            struct t_irc_channel *channel, const char *nick,
                            const char *message, const char *str_version)
{
    char *pos, *version, *date;
    struct t_gui_buffer *ptr_buffer;
    
    ptr_buffer = (channel) ? channel->buffer : server->buffer;
    
    pos = strchr (str_version, ' ');
    if (pos)
    {
        while (pos[0] == ' ')
            pos++;
        if (pos[0] == '\01')
            pos = NULL;
        else if (!pos[0])
            pos = NULL;
    }
    
    version = weechat_info_get ("version", "");
    date = weechat_info_get ("date", "");
    if (version && date)
    {
        irc_server_sendf (server,
                          "NOTICE %s :%sVERSION WeeChat %s (%s)%s",
                          nick, "\01", version, date, "\01");
        
        if (pos)
        {
            weechat_printf (ptr_buffer,
                            _("%sCTCP %sVERSION%s received from %s%s%s: "
                              "%s"),
                            (ptr_buffer == server->buffer) ?
                            irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                            IRC_COLOR_CHAT_CHANNEL,
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_NICK,
                            nick,
                            IRC_COLOR_CHAT,
                            pos);
        }
        else
        {
            weechat_printf (ptr_buffer,
                            _("%sCTCP %sVERSION%s received from %s%s"),
                            (ptr_buffer == server->buffer) ?
                            irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                            IRC_COLOR_CHAT_CHANNEL,
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_NICK,
                            nick);
        }
    }
    weechat_hook_signal_send ("irc_ctcp",
                              WEECHAT_HOOK_SIGNAL_STRING,
                              (void *)message);
}

/*
 * irc_protocol_cmd_privmsg: 'privmsg' command received
 */

int
irc_protocol_cmd_privmsg (struct t_irc_server *server, const char *command,
                          int argc, char **argv, char **argv_eol)
{
    char *pos_args, *pos_end_01, *pos, *pos_message;
    char *dcc_args, *pos_file, *pos_addr, *pos_port, *pos_size, *pos_start_resume;  /* for DCC */
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    char plugin_id[128];
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* PRIVMSG message looks like:
       :nick!user@host PRIVMSG #channel :message for channel here
       :nick!user@host PRIVMSG mynick :message for private here
       :nick!user@host PRIVMSG #channel :ACTION is testing action
       :nick!user@host PRIVMSG mynick :ACTION is testing action
       :nick!user@host PRIVMSG mynick :\01DCC SEND file.txt 1488915698 50612 128\01
    */
    
    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;
    
    pos_args = (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3];
    
    /* receiver is a channel ? */
    if (irc_channel_is_channel (argv[2]))
    {
        ptr_channel = irc_channel_search (server, argv[2]);
        if (ptr_channel)
        {
            if (strncmp (pos_args, "\01ACTION ", 8) == 0)
            {
                if (!irc_ignore_check (server, ptr_channel, nick, host))
                {
                    pos_args += 8;
                    pos_end_01 = strchr (pos_args, '\01');
                    if (pos_end_01)
                        pos_end_01[0] = '\0';
                    
                    weechat_printf_tags (ptr_channel->buffer,
                                         "irc_privmsg,irc_action,notify_message",
                                         "%s%s%s %s%s",
                                         weechat_prefix ("action"),
                                         IRC_COLOR_CHAT_NICK,
                                         nick,
                                         IRC_COLOR_CHAT,
                                         pos_args);
                    
                    irc_channel_add_nick_speaking (ptr_channel, nick);
                    
                    if (pos_end_01)
                        pos_end_01[0] = '\01';
                }
                
                return WEECHAT_RC_OK;
            }
            if (strncmp (pos_args, "\01SOUND ", 7) == 0)
            {
                if (!irc_ignore_check (server, ptr_channel, nick, host))
                {
                    pos_args += 7;
                    pos_end_01 = strchr (pos_args, '\01');
                    if (pos_end_01)
                        pos_end_01[0] = '\0';
                    
                    weechat_printf_tags (ptr_channel->buffer,
                                         "irc_privmsg,irc_ctcp",
                                         _("%sReceived a CTCP %sSOUND%s \"%s\" "
                                           "from %s%s"),
                                         weechat_prefix ("network"),
                                         IRC_COLOR_CHAT_CHANNEL,
                                         IRC_COLOR_CHAT,
                                         pos_args,
                                         IRC_COLOR_CHAT_NICK,
                                         nick);
                    
                    if (pos_end_01)
                        pos_end_01[0] = '\01';
                }
                
                return WEECHAT_RC_OK;
            }
            if (strncmp (pos_args, "\01PING", 5) == 0)
            {
                if (!irc_ignore_check (server, ptr_channel, nick, host))
                {
                    pos_args += 5;
                    while (pos_args[0] == ' ')
                        pos_args++;
                    pos_end_01 = strchr (pos_args, '\01');
                    if (pos_end_01)
                        pos_end_01[0] = '\0';
                    else
                        pos_args = NULL;
                    if (pos_args && !pos_args[0])
                        pos_args = NULL;
                    if (pos_args)
                        irc_server_sendf (server, "NOTICE %s :\01PING %s\01",
                                          nick, pos_args);
                    else
                        irc_server_sendf (server, "NOTICE %s :\01PING\01",
                                          nick);
                    weechat_printf_tags (ptr_channel->buffer,
                                         "irc_privmsg,irc_ctcp",
                                         _("%sCTCP %sPING%s received from %s%s"),
                                         weechat_prefix ("network"),
                                         IRC_COLOR_CHAT_CHANNEL,
                                         IRC_COLOR_CHAT,
                                         IRC_COLOR_CHAT_NICK,
                                         nick);
                    if (pos_end_01)
                        pos_end_01[0] = '\01';
                }
                
                return WEECHAT_RC_OK;
            }
            if (strncmp (pos_args, "\01VERSION", 8) == 0)
            {
                if (!irc_ignore_check (server, ptr_channel, nick, host))
                {
                    irc_protocol_reply_version (server, ptr_channel, nick,
                                                argv_eol[0], pos_args);
                }
                
                return WEECHAT_RC_OK;
            }
            
            /* unknown CTCP ? */
            pos_end_01 = strchr (pos_args + 1, '\01');
            if ((pos_args[0] == '\01')
                && pos_end_01 && (pos_end_01[1] == '\0'))
            {
                if (!irc_ignore_check (server, ptr_channel, nick, host))
                {
                    pos_args++;
                    pos_end_01[0] = '\0';
                    pos = strchr (pos_args, ' ');
                    if (pos)
                    {
                        pos[0] = '\0';
                        pos_message = pos + 1;
                        while (pos_message[0] == ' ')
                        {
                            pos_message++;
                        }
                        if (!pos_message[0])
                        pos_message = NULL;
                    }
                    else
                        pos_message = NULL;
                    
                    if (pos_message)
                    {
                        weechat_printf_tags (ptr_channel->buffer,
                                             "irc_privmsg,irc_ctcp",
                                             _("%sUnknown CTCP %s%s%s "
                                               "received from %s%s%s: %s"),
                                             weechat_prefix ("network"),
                                             IRC_COLOR_CHAT_CHANNEL,
                                             pos_args,
                                             IRC_COLOR_CHAT,
                                             IRC_COLOR_CHAT_NICK,
                                             nick,
                                             IRC_COLOR_CHAT,
                                             pos_message);
                    }
                    else
                    {
                        weechat_printf_tags (ptr_channel->buffer,
                                             "irc_privmsg,irc_ctcp",
                                             _("%sUnknown CTCP %s%s%s "
                                               "received from %s%s"),
                                             weechat_prefix ("network"),
                                             IRC_COLOR_CHAT_CHANNEL,
                                             pos_args,
                                             IRC_COLOR_CHAT,
                                             IRC_COLOR_CHAT_NICK,
                                             nick);
                    }
                    if (pos_end_01)
                        pos_end_01[0] = '\01';
                    if (pos)
                        pos[0] = ' ';
                    
                    weechat_hook_signal_send ("irc_ctcp",
                                              WEECHAT_HOOK_SIGNAL_STRING,
                                              argv_eol[0]);
                }
                
                return WEECHAT_RC_OK;
            }
            
            /* other message */
            ptr_nick = irc_nick_search (ptr_channel, nick);
            
            if (!irc_ignore_check (server, ptr_channel, nick, host))
            {
                weechat_printf_tags (ptr_channel->buffer,
                                     "irc_privmsg,notify_message",
                                     "%s%s",
                                     irc_nick_as_prefix (ptr_nick,
                                                         (ptr_nick) ? NULL : nick,
                                                         NULL),
                                     pos_args);
                
                irc_channel_add_nick_speaking (ptr_channel, nick);
            }
        }
        else
        {
            weechat_printf (server->buffer,
                            _("%s%s: channel \"%s\" not found for \"%s\" "
                              "command"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME, argv[2], "privmsg");
            return WEECHAT_RC_ERROR;
        }
    }
    else
    {
        /* version asked by another user => answer with WeeChat version */
        if (strncmp (pos_args, "\01VERSION", 8) == 0)
        {
            if (!irc_ignore_check (server, NULL, nick, host))
            {
                irc_protocol_reply_version (server, NULL, nick, argv_eol[0],
                                            pos_args);
            }
            
            return WEECHAT_RC_OK;
        }
            
        /* ping request from another user => answer */
        if (strncmp (pos_args, "\01PING", 5) == 0)
        {
            if (!irc_ignore_check (server, NULL, nick, host))
            {
                pos_args += 5;
                while (pos_args[0] == ' ')
                {
                    pos_args++;
                }
                pos_end_01 = strchr (pos_args, '\01');
                if (pos_end_01)
                    pos_end_01[0] = '\0';
                else
                    pos_args = NULL;
                if (pos_args && !pos_args[0])
                    pos_args = NULL;
                if (pos_args)
                    irc_server_sendf (server, "NOTICE %s :\01PING %s\01",
                                      nick, pos_args);
                else
                    irc_server_sendf (server, "NOTICE %s :\01PING\01",
                                      nick);
                weechat_printf_tags (server->buffer,
                                     "irc_privmsg,irc_ctcp",
                                     _("%sCTCP %sPING%s received from %s%s"),
                                     irc_buffer_get_server_prefix (server,
                                                                   "network"),
                                     IRC_COLOR_CHAT_CHANNEL,
                                     IRC_COLOR_CHAT,
                                     IRC_COLOR_CHAT_NICK,
                                     nick);
                weechat_hook_signal_send ("irc_ctcp",
                                          WEECHAT_HOOK_SIGNAL_STRING,
                                          argv_eol[0]);
                if (pos_end_01)
                    pos_end_01[0] = '\01';
            }
            
            return WEECHAT_RC_OK;
        }
        
        /* incoming DCC file */
        if (strncmp (pos_args, "\01DCC SEND", 9) == 0)
        {
            if (!irc_ignore_check (server, NULL, nick, host))
            {
                /* check if DCC SEND is ok, i.e. with 0x01 at end */
                pos_end_01 = strchr (pos_args + 1, '\01');
                if (!pos_end_01)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                dcc_args = weechat_strndup (pos_args + 9,
                                            pos_end_01 - pos_args - 9);
                
                if (!dcc_args)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: not enough memory for \"%s\" "
                                      "command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                /* DCC filename */
                pos_file = dcc_args;
                while (pos_file[0] == ' ')
                {
                    pos_file++;
                }
                
                /* look for file size */
                pos_size = strrchr (pos_file, ' ');
                if (!pos_size)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                pos = pos_size;
                pos_size++;
                while (pos[0] == ' ')
                {
                    pos--;
                }
                pos[1] = '\0';
                
                /* look for DCC port */
                pos_port = strrchr (pos_file, ' ');
                if (!pos_port)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                pos = pos_port;
                pos_port++;
                while (pos[0] == ' ')
                {
                    pos--;
                }
                pos[1] = '\0';
                
                /* look for DCC IP address */
                pos_addr = strrchr (pos_file, ' ');
                if (!pos_addr)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                pos = pos_addr;
                pos_addr++;
                while (pos[0] == ' ')
                {
                    pos--;
                }
                pos[1] = '\0';
                
                /* add DCC file via xfer plugin */
                infolist = weechat_infolist_new ();
                if (infolist)
                {
                    item = weechat_infolist_new_item (infolist);
                    if (item)
                    {
                        weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                        snprintf (plugin_id, sizeof (plugin_id),
                                  "%x", (unsigned int)server);
                        weechat_infolist_new_var_string (item, "plugin_id", plugin_id);
                        weechat_infolist_new_var_string (item, "type", "file_recv");
                        weechat_infolist_new_var_string (item, "protocol", "dcc");
                        weechat_infolist_new_var_string (item, "remote_nick", nick);
                        weechat_infolist_new_var_string (item, "local_nick", server->nick);
                        weechat_infolist_new_var_string (item, "filename", pos_file);
                        weechat_infolist_new_var_string (item, "size", pos_size);
                        weechat_infolist_new_var_string (item, "address", pos_addr);
                        weechat_infolist_new_var_integer (item, "port", atoi (pos_port));
                        weechat_hook_signal_send ("xfer_add",
                                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                                  infolist);
                    }
                    weechat_infolist_free (infolist);
                }
                
                weechat_hook_signal_send ("irc_dcc",
                                          WEECHAT_HOOK_SIGNAL_STRING,
                                          argv_eol[0]);
                
                free (dcc_args);
            }
            
            return WEECHAT_RC_OK;
        }
        
        /* incoming DCC RESUME (asked by receiver) */
        if (strncmp (pos_args, "\01DCC RESUME", 11) == 0)
        {
            if (!irc_ignore_check (server, NULL, nick, host))
            {
                /* check if DCC RESUME is ok, i.e. with 0x01 at end */
                pos_end_01 = strchr (pos_args + 1, '\01');
                if (!pos_end_01)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                dcc_args = weechat_strndup (pos_args + 11,
                                            pos_end_01 - pos_args - 11);
                
                if (!dcc_args)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: not enough memory for \"%s\" "
                                      "command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                /* DCC filename */
                pos_file = dcc_args;
                while (pos_file[0] == ' ')
                {
                    pos_file++;
                }
                
                /* look for resume start position */
                pos_start_resume = strrchr (pos_file, ' ');
                if (!pos_start_resume)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                pos = pos_start_resume;
                pos_start_resume++;
                while (pos[0] == ' ')
                {
                    pos--;
                }
                pos[1] = '\0';
                
                /* look for DCC port */
                pos_port = strrchr (pos_file, ' ');
                if (!pos_port)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                pos = pos_port;
                pos_port++;
                while (pos[0] == ' ')
                {
                    pos--;
                }
                pos[1] = '\0';
                
                /* accept resume via xfer plugin */
                infolist = weechat_infolist_new ();
                if (infolist)
                {
                    item = weechat_infolist_new_item (infolist);
                    if (item)
                    {
                        weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                        snprintf (plugin_id, sizeof (plugin_id),
                                  "%x", (unsigned int)server);
                        weechat_infolist_new_var_string (item, "plugin_id", plugin_id);
                        weechat_infolist_new_var_string (item, "type", "file_recv");
                        weechat_infolist_new_var_string (item, "filename", pos_file);
                        weechat_infolist_new_var_integer (item, "port", atoi (pos_port));
                        weechat_infolist_new_var_string (item, "start_resume", pos_start_resume);
                        weechat_hook_signal_send ("xfer_accept_resume",
                                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                                  infolist);
                    }
                    weechat_infolist_free (infolist);
                }
                
                weechat_hook_signal_send ("irc_dcc",
                                          WEECHAT_HOOK_SIGNAL_STRING,
                                          argv_eol[0]);
                
                free (dcc_args);
            } 
            
            return WEECHAT_RC_OK;
        }
            
        /* incoming DCC ACCEPT (resume accepted by sender) */
        if (strncmp (pos_args, "\01DCC ACCEPT", 11) == 0)
        {
            if (!irc_ignore_check (server, NULL, nick, host))
            {
                /* check if DCC ACCEPT is ok, i.e. with 0x01 at end */
                pos_end_01 = strchr (pos_args + 1, '\01');
                if (!pos_end_01)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                dcc_args = weechat_strndup (pos_args + 11,
                                            pos_end_01 - pos_args - 11);
                
                if (!dcc_args)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: not enough memory for \"%s\" "
                                      "command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                /* DCC filename */
                pos_file = dcc_args;
                while (pos_file[0] == ' ')
                {
                    pos_file++;
                }
                
                /* look for resume start position */
                pos_start_resume = strrchr (pos_file, ' ');
                if (!pos_start_resume)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                pos = pos_start_resume;
                pos_start_resume++;
                while (pos[0] == ' ')
                {
                    pos--;
                }
                pos[1] = '\0';
                
                /* look for DCC port */
                pos_port = strrchr (pos_file, ' ');
                if (!pos_port)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                pos = pos_port;
                pos_port++;
                while (pos[0] == ' ')
                {
                    pos--;
                }
                pos[1] = '\0';
                
                /* resume file via xfer plugin */
                infolist = weechat_infolist_new ();
                if (infolist)
                {
                    item = weechat_infolist_new_item (infolist);
                    if (item)
                    {
                        weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                        snprintf (plugin_id, sizeof (plugin_id),
                                  "%x", (unsigned int)server);
                        weechat_infolist_new_var_string (item, "plugin_id", plugin_id);
                        weechat_infolist_new_var_string (item, "type", "file_recv");
                        weechat_infolist_new_var_string (item, "filename", pos_file);
                        weechat_infolist_new_var_integer (item, "port", atoi (pos_port));
                        weechat_infolist_new_var_string (item, "start_resume", pos_start_resume);
                        weechat_hook_signal_send ("xfer_start_resume",
                                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                                  infolist);
                    }
                    weechat_infolist_free (infolist);
                }
                
                weechat_hook_signal_send ("irc_dcc",
                                          WEECHAT_HOOK_SIGNAL_STRING,
                                          argv_eol[0]);
                
                free (dcc_args);
            }
            
            return WEECHAT_RC_OK;
        }
            
        /* incoming DCC CHAT */
        if (strncmp (pos_args, "\01DCC CHAT", 9) == 0)
        {
            if (!irc_ignore_check (server, NULL, nick, host))
            {
                /* check if DCC CHAT is ok, i.e. with 0x01 at end */
                pos_end_01 = strchr (pos_args + 1, '\01');
                if (!pos_end_01)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                dcc_args = weechat_strndup (pos_args + 9,
                                            pos_end_01 - pos_args - 9);
                
                if (!dcc_args)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: not enough memory for \"%s\" "
                                      "command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                /* CHAT type */
                pos_file = dcc_args;
                while (pos_file[0] == ' ')
                {
                    pos_file++;
                }
                
                /* DCC IP address */
                pos_addr = strchr (pos_file, ' ');
                if (!pos_addr)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                pos_addr[0] = '\0';
                pos_addr++;
                while (pos_addr[0] == ' ')
                {
                    pos_addr++;
                }
                
                /* look for DCC port */
                pos_port = strchr (pos_addr, ' ');
                if (!pos_port)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME, "privmsg");
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                pos_port[0] = '\0';
                pos_port++;
                while (pos_port[0] == ' ')
                {
                    pos_port++;
                }
                
                if (weechat_strcasecmp (pos_file, "chat") != 0)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: unknown DCC CHAT type "
                                      "received from %s%s%s: \"%s\""),
                                    irc_buffer_get_server_prefix (server, "error"),
                                    IRC_PLUGIN_NAME,
                                    IRC_COLOR_CHAT_NICK,
                                    nick,
                                    IRC_COLOR_CHAT,
                                    pos_file);
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                
                /* add DCC chat via xfer plugin */
                infolist = weechat_infolist_new ();
                if (infolist)
                {
                    item = weechat_infolist_new_item (infolist);
                    if (item)
                    {
                        weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                        snprintf (plugin_id, sizeof (plugin_id),
                                  "%x", (unsigned int)server);
                        weechat_infolist_new_var_string (item, "plugin_id", plugin_id);
                        weechat_infolist_new_var_string (item, "type", "chat_recv");
                        weechat_infolist_new_var_string (item, "remote_nick", nick);
                        weechat_infolist_new_var_string (item, "local_nick", server->nick);
                        weechat_infolist_new_var_string (item, "address", pos_addr);
                        weechat_infolist_new_var_integer (item, "port", atoi (pos_port));
                        weechat_hook_signal_send ("xfer_add",
                                                  WEECHAT_HOOK_SIGNAL_POINTER,
                                                  infolist);
                    }
                    weechat_infolist_free (infolist);
                }
                
                weechat_hook_signal_send ("irc_dcc",
                                          WEECHAT_HOOK_SIGNAL_STRING,
                                          argv_eol[0]);
                
                free (dcc_args);
            }
            
            return WEECHAT_RC_OK;
        }
        
        /* private message received => display it */
        ptr_channel = irc_channel_search (server, nick);
        
        if (strncmp (pos_args, "\01ACTION ", 8) == 0)
        {
            if (!irc_ignore_check (server, ptr_channel, nick, host))
            {
                if (!ptr_channel)
                {
                    ptr_channel = irc_channel_new (server,
                                                   IRC_CHANNEL_TYPE_PRIVATE,
                                                   nick, 0);
                    if (!ptr_channel)
                    {
                        weechat_printf (server->buffer,
                                        _("%s%s: cannot create new "
                                          "private buffer \"%s\""),
                                        irc_buffer_get_server_prefix (server,
                                                                      "error"),
                                        IRC_PLUGIN_NAME, nick);
                        return WEECHAT_RC_ERROR;
                    }
                }
                if (!ptr_channel->topic)
                    irc_channel_set_topic (ptr_channel, address);
                
                pos_args += 8;
                pos_end_01 = strchr (pos_args, '\01');
                if (pos_end_01)
                    pos_end_01[0] = '\0';
                
                weechat_printf_tags (ptr_channel->buffer,
                                     "irc_privmsg,irc_action,notify_private",
                                     "%s%s%s %s%s",
                                     weechat_prefix ("action"),
                                     IRC_COLOR_CHAT_NICK,
                                     nick,
                                     IRC_COLOR_CHAT,
                                     pos_args);
                weechat_hook_signal_send ("irc_pv",
                                          WEECHAT_HOOK_SIGNAL_STRING,
                                          argv_eol[0]);
                
                if (pos_end_01)
                    pos_end_01[0] = '\01';
            }
        }
        else
        {
            /* unknown CTCP ? */
            pos_end_01 = strchr (pos_args + 1, '\01');
            if ((pos_args[0] == '\01')
                && pos_end_01 && (pos_end_01[1] == '\0'))
            {
                if (!irc_ignore_check (server, ptr_channel, nick, host))
                {
                    pos_args++;
                    pos_end_01[0] = '\0';
                    pos = strchr (pos_args, ' ');
                    if (pos)
                    {
                        pos[0] = '\0';
                        pos_message = pos + 1;
                        while (pos_message[0] == ' ')
                        {
                            pos_message++;
                        }
                        if (!pos_message[0])
                            pos_message = NULL;
                    }
                    else
                        pos_message = NULL;
                    
                    if (pos_message)
                    {
                        weechat_printf_tags (server->buffer,
                                             "irc_privmsg,irc_ctcp",
                                             _("%sUnknown CTCP %s%s%s "
                                               "received from %s%s%s: %s"),
                                             irc_buffer_get_server_prefix (server,
                                                                           "network"),
                                             IRC_COLOR_CHAT_CHANNEL,
                                             pos_args,
                                             IRC_COLOR_CHAT,
                                             IRC_COLOR_CHAT_NICK,
                                             nick,
                                             IRC_COLOR_CHAT,
                                             pos_message);
                    }
                    else
                    {
                        weechat_printf_tags (server->buffer,
                                             "irc_privmsg,irc_ctcp",
                                             _("%sUnknown CTCP %s%s%s "
                                               "received from %s%s"),
                                             irc_buffer_get_server_prefix (server,
                                                                           "network"),
                                             IRC_COLOR_CHAT_CHANNEL,
                                             pos_args,
                                             IRC_COLOR_CHAT,
                                             IRC_COLOR_CHAT_NICK,
                                             nick);
                    }
                    if (pos_end_01)
                        pos_end_01[0] = '\01';
                    if (pos)
                        pos[0] = ' ';
                    
                    weechat_hook_signal_send ("irc_ctcp",
                                              WEECHAT_HOOK_SIGNAL_STRING,
                                              argv_eol[0]);
                }
            }
            else
            {
                /* private message */
                if (!irc_ignore_check (server, ptr_channel, nick, host))
                {
                    if (!ptr_channel)
                    {
                        ptr_channel = irc_channel_new (server,
                                                       IRC_CHANNEL_TYPE_PRIVATE,
                                                       nick, 0);
                        if (!ptr_channel)
                        {
                            weechat_printf (server->buffer,
                                            _("%s%s: cannot create new "
                                              "private buffer \"%s\""),
                                            irc_buffer_get_server_prefix (server,
                                                                          "error"),
                                            IRC_PLUGIN_NAME, nick);
                            return WEECHAT_RC_ERROR;
                        }
                    }
                    irc_channel_set_topic (ptr_channel, address);
                    
                    weechat_printf_tags (ptr_channel->buffer,
                                         "irc_privmsg,notify_private",
                                         "%s%s",
                                         irc_nick_as_prefix (NULL,
                                                             nick,
                                                             IRC_COLOR_CHAT_NICK_OTHER),
                                         pos_args);
                    
                    weechat_hook_signal_send ("irc_pv",
                                              WEECHAT_HOOK_SIGNAL_STRING,
                                              argv_eol[0]);
                }
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_quit: 'quit' command received
 */

int
irc_protocol_cmd_quit (struct t_irc_server *server, const char *command,
                       int argc, char **argv, char **argv_eol)
{
    char *pos_comment;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* QUIT message looks like:
       :nick!user@host QUIT :quit message
    */
    
    IRC_PROTOCOL_GET_HOST;
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
            ptr_nick = irc_nick_search (ptr_channel, nick);
        
        if (ptr_nick || (strcmp (ptr_channel->name, nick) == 0))
        {
            if (ptr_nick)
                irc_nick_free (ptr_channel, ptr_nick);
            
            /* display quit message */
            if (!irc_ignore_check (server, ptr_channel, nick, host))
            {
                if (pos_comment && pos_comment[0])
                {
                    weechat_printf_tags (ptr_channel->buffer,
                                         "irc_quit",
                                         _("%s%s%s %s(%s%s%s)%s has quit "
                                           "%s(%s%s%s)"),
                                         weechat_prefix ("quit"),
                                         IRC_COLOR_CHAT_NICK,
                                         nick,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT_HOST,
                                         address,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT,
                                         pos_comment,
                                         IRC_COLOR_CHAT_DELIMITERS);
                }
                else
                {
                    weechat_printf_tags (ptr_channel->buffer,
                                         "irc_quit",
                                         _("%s%s%s %s(%s%s%s)%s has quit"),
                                         weechat_prefix ("quit"),
                                         IRC_COLOR_CHAT_NICK,
                                         nick,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT_HOST,
                                         address,
                                         IRC_COLOR_CHAT_DELIMITERS,
                                         IRC_COLOR_CHAT);
                }
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_server_mode_reason: command received from server (numeric),
 *                                      format: "mode :reason"
 */

int
irc_protocol_cmd_server_mode_reason (struct t_irc_server *server, const char *command,
                                     int argc, char **argv, char **argv_eol)
{
    char *pos_mode, *pos_args;
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* skip nickname if at beginning of server message */
    if (strcmp (server->nick, argv[2]) == 0)
    {
        pos_mode = argv[3];
        pos_args = (argc > 4) ? ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;
    }
    else
    {
        pos_mode = argv[2];
        pos_args = (argc > 3) ? ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    }
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags (command, "irc_numeric"),
                         "%s%s: %s",
                         irc_buffer_get_server_prefix (server, "network"),
                         pos_mode,
                         (pos_args) ? pos_args : "");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_numeric: numeric command received from server
 */

int
irc_protocol_cmd_numeric (struct t_irc_server *server, const char *command,
                          int argc, char **argv, char **argv_eol)
{
    char *pos_args;
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) argv;
    
    if (weechat_strcasecmp (server->nick, argv[2]) == 0)
    {
        pos_args = (argc > 3) ?
            ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    }
    else
    {
        pos_args = (argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2];
    }
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags (command, "irc_numeric"),
                         "%s%s",
                         irc_buffer_get_server_prefix (server, "network"),
                         pos_args);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_topic: 'topic' command received
 */

int
irc_protocol_cmd_topic (struct t_irc_server *server, const char *command,
                        int argc, char **argv, char **argv_eol)
{
    char *pos_topic, *topic_color;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    
    /* TOPIC message looks like:
       :nick!user@host TOPIC #channel :new topic for channel
    */
    
    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(3);
    
    if (!irc_channel_is_channel (argv[2]))
    {
        weechat_printf (server->buffer,
                        _("%s%s: \"%s\" command received without channel"),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME, "topic");
        return WEECHAT_RC_ERROR;
    }
    
    pos_topic = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    
    ptr_channel = irc_channel_search (server, argv[2]);
    ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;

    if (!irc_ignore_check (server, ptr_channel, nick, host))
    {
        if (pos_topic && pos_topic[0])
        {
            topic_color = irc_color_decode (pos_topic,
                                            weechat_config_boolean (irc_config_network_colors_receive));
            weechat_printf_tags (ptr_buffer,
                                 "irc_topic",
                                 _("%s%s%s%s has changed topic for %s%s%s to: "
                                   "\"%s%s\""),
                                 (ptr_buffer == server->buffer) ?
                                 irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                                 IRC_COLOR_CHAT_NICK,
                                 nick,
                                 IRC_COLOR_CHAT,
                                 IRC_COLOR_CHAT_CHANNEL,
                                 argv[2],
                                 IRC_COLOR_CHAT,
                                 (topic_color) ? topic_color : pos_topic,
                                 IRC_COLOR_CHAT);
            if (topic_color)
                free (topic_color);
        }
        else
        {
            weechat_printf_tags (ptr_buffer,
                                 "irc_topic",
                                 _("%s%s%s%s has unset topic for %s%s"),
                                 (ptr_buffer == server->buffer) ?
                                 irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                                 IRC_COLOR_CHAT_NICK,
                                 nick,
                                 IRC_COLOR_CHAT,
                                 IRC_COLOR_CHAT_CHANNEL,
                                 argv[2]);
        }
    }
    
    if (ptr_channel)
        irc_channel_set_topic (ptr_channel, pos_topic);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_wallops: 'wallops' command received
 */

int
irc_protocol_cmd_wallops (struct t_irc_server *server, const char *command,
                          int argc, char **argv, char **argv_eol)
{
    /* WALLOPS message looks like:
       :nick!user@host WALLOPS :message from admin
    */
    
    IRC_PROTOCOL_GET_HOST;
    IRC_PROTOCOL_MIN_ARGS(3);
    
    if (!irc_ignore_check (server, NULL, nick, host))
    {
        weechat_printf_tags (server->buffer,
                             "irc_wallops",
                             _("%sWallops from %s%s %s(%s%s%s)%s: %s"),
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_NICK,
                             nick,
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_HOST,
                             address,
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             (argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_001: '001' command (connected to irc server)
 */

int
irc_protocol_cmd_001 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char **commands, **ptr_cmd, *vars_replaced;
    char *away_msg;

    /* 001 message looks like:
       :server 001 mynick :Welcome to the dancer-ircd Network
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    if (strcmp (server->nick, argv[2]) != 0)
        irc_server_set_nick (server, argv[2]);
    
    irc_protocol_cmd_numeric (server, command, argc, argv, argv_eol);
    
    /* connection to IRC server is ok! */
    server->is_connected = 1;
    server->lag_next_check = time (NULL) +
        weechat_config_integer (irc_config_network_lag_check);
    
    /* set away message if user was away (before disconnection for example) */
    if (server->away_message && server->away_message[0])
    {
        away_msg = strdup (server->away_message);
        if (away_msg)
        {
            irc_command_away_server (server, away_msg);
            free (away_msg);
        }
    }
    
    /* execute command when connected */
    if (server->command && server->command[0])
    {
	/* splitting command on ';' which can be escaped with '\;' */ 
	commands = weechat_string_split_command (server->command, ';');
	if (commands)
	{
	    for (ptr_cmd = commands; *ptr_cmd; ptr_cmd++)
            {
                vars_replaced = irc_protocol_replace_vars (server, NULL,
                                                           *ptr_cmd);
                weechat_command (server->buffer,
                                 (vars_replaced) ? vars_replaced : *ptr_cmd);
                if (vars_replaced)
                    free (vars_replaced);
            }
	    weechat_string_free_splitted_command (commands);
	}
	
	if (server->command_delay > 0)
            server->command_time = time (NULL) + 1;
        else
            irc_server_autojoin_channels (server);
    }
    else
        irc_server_autojoin_channels (server);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_005: '005' command (some infos from server)
 */

int
irc_protocol_cmd_005 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos, *pos2;

    /* 005 message looks like:
       :server 005 mynick MODES=4 CHANLIMIT=#:20 NICKLEN=16 USERLEN=10
           HOSTLEN=63 TOPICLEN=450 KICKLEN=450 CHANNELLEN=30 KEYLEN=23
           CHANTYPES=# PREFIX=(ov)@+ CASEMAPPING=ascii CAPAB IRCD=dancer
           :are available on this server
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    irc_protocol_cmd_numeric (server, command, argc, argv, argv_eol);
    
    pos = strstr (argv_eol[3], "PREFIX=");
    if (pos)
    {
        pos += 7;
        if (pos[0] == '(')
        {
            pos2 = strchr (pos, ')');
            if (!pos2)
                return WEECHAT_RC_OK;
            pos = pos2 + 1;
        }
        pos2 = strchr (pos, ' ');
        if (pos2)
            pos2[0] = '\0';
        if (server->prefix)
            free (server->prefix);
        server->prefix = strdup (pos);
        if (pos2)
            pos2[0] = ' ';
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_221: '221' command (user mode string)
 */

int
irc_protocol_cmd_221 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 221 message looks like:
       :server 221 nick :+s
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags (command, "irc_numeric"),
                         _("%sUser mode for %s%s%s is %s[%s%s%s]"),
                         irc_buffer_get_server_prefix (server, "network"),
                         IRC_COLOR_CHAT_NICK,
                         argv[2],
                         IRC_COLOR_CHAT,
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT,
                         (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3],
                         IRC_COLOR_CHAT_DELIMITERS);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_301: '301' command (away message)
 *                       received when we are talking to a user in private
 *                       and that remote user is away (we receive away message)
 */

int
irc_protocol_cmd_301 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos_away_msg;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    
    /* 301 message looks like:
       :server 301 mynick nick :away message for nick
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    if (argc > 4)
    {
        pos_away_msg = (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4];
        
        /* look for private buffer to display message */
        ptr_channel = irc_channel_search (server, argv[3]);
        if (!weechat_config_boolean (irc_config_look_show_away_once)
            || !ptr_channel
            || !(ptr_channel->away_message)
            || (strcmp (ptr_channel->away_message, pos_away_msg) != 0))
        {
            ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
            weechat_printf_tags (ptr_buffer,
                                 irc_protocol_tags (command, "irc_numeric"),
                                 _("%s%s[%s%s%s]%s is away: %s"),
                                 (ptr_buffer == server->buffer) ?
                                 irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                                 IRC_COLOR_CHAT_DELIMITERS,
                                 IRC_COLOR_CHAT_NICK,
                                 argv[3],
                                 IRC_COLOR_CHAT_DELIMITERS,
                                 IRC_COLOR_CHAT,
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
 * irc_protocol_cmd_303: '303' command (ison)
 */

int
irc_protocol_cmd_303 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 301 message looks like:
       :server 303 mynick :nick1 nick2
    */

    IRC_PROTOCOL_MIN_ARGS(4);
    
    /* make C compiler happy */
    (void) argv;
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags (command, "irc_numeric"),
                         _("%sUsers online: %s%s"),
                         irc_buffer_get_server_prefix (server, "network"),
                         IRC_COLOR_CHAT_NICK,
                         (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_305: '305' command (unaway)
 */

int
irc_protocol_cmd_305 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 305 message looks like:
       :server 305 mynick :Does this mean you're really back?
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) argv;
    
    if (argc > 3)
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             "%s%s",
                             irc_buffer_get_server_prefix (server, "network"),
                             (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]);
    }
    
    server->is_away = 0;
    server->away_time = 0;
    
    /*
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if ((ptr_window->buffer->protocol == irc_protocol)
            && (IRC_BUFFER_SERVER(ptr_window->buffer) == server))
            gui_status_draw (ptr_window->buffer, 1);
    }
    */
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_306: '306' command (now away)
 */

int
irc_protocol_cmd_306 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 306 message looks like:
       :server 306 mynick :We'll miss you
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) argv;
    
    if (argc > 3)
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             "%s%s",
                             irc_buffer_get_server_prefix (server, "network"),
                             (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]);
    }
    
    server->is_away = 1;
    server->away_time = time (NULL);
    
    /*
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window->buffer->protocol == irc_protocol)
        {
            if (IRC_BUFFER_SERVER(ptr_window->buffer) == server)
            {
                gui_status_draw (ptr_window->buffer, 1);
                ptr_window->buffer->last_read_line =
                    ptr_window->buffer->last_line;
            }
        }
    }
    */
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_whois_nick_msg: a whois command with nick and message
 */

int
irc_protocol_cmd_whois_nick_msg (struct t_irc_server *server, const char *command,
                                 int argc, char **argv, char **argv_eol)
{
    /* messages look like:
       :server 319 flashy FlashCode :some text here
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags (command, "irc_numeric"),
                         "%s%s[%s%s%s] %s%s",
                         irc_buffer_get_server_prefix (server, "network"),
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT_NICK,
                         argv[3],
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT,
                         (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_311: '311' command (whois, user)
 */

int
irc_protocol_cmd_311 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 311 message looks like:
       :server 311 mynick nick user host * :realname here
    */
    
    IRC_PROTOCOL_MIN_ARGS(8);
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags (command, "irc_numeric"),
                         "%s%s[%s%s%s] (%s%s@%s%s)%s: %s",
                         irc_buffer_get_server_prefix (server, "network"),
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT_NICK,
                         argv[3],
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT_HOST,
                         argv[4],
                         argv[5],
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT,
                         (argv_eol[7][0] == ':') ? argv_eol[7] + 1 : argv_eol[7]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_312: '312' command (whois, server)
 */

int
irc_protocol_cmd_312 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 312 message looks like:
       :server 312 mynick nick irc.freenode.net :http://freenode.net/
    */
    
    IRC_PROTOCOL_MIN_ARGS(6);
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags(command, "irc_numeric"),
                         "%s%s[%s%s%s] %s%s %s(%s%s%s)",
                         irc_buffer_get_server_prefix (server, "network"),
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT_NICK,
                         argv[3],
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT,
                         argv[4],
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT,
                         (argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5],
                         IRC_COLOR_CHAT_DELIMITERS);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_314: '314' command (whowas)
 */

int
irc_protocol_cmd_314 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 314 message looks like:
       :server 314 mynick nick user host * :realname here
    */
    
    IRC_PROTOCOL_MIN_ARGS(8);
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags(command, "irc_numeric"),
                         _("%s%s%s %s(%s%s@%s%s)%s was %s"),
                         irc_buffer_get_server_prefix (server, "network"),
                         IRC_COLOR_CHAT_NICK,
                         argv[3],
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT_HOST,
                         argv[4],
                         argv[5],
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT,
                         (argv_eol[7][0] == ':') ? argv_eol[7] + 1 : argv_eol[7]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_315: '315' command (end of /who)
 */

int
irc_protocol_cmd_315 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 315 message looks like:
       :server 315 mynick #channel :End of /WHO list.
    */
    
    struct t_irc_channel *ptr_channel;

    IRC_PROTOCOL_MIN_ARGS(5);
    
    ptr_channel = irc_channel_search (server, argv[3]);
    if (ptr_channel && (ptr_channel->checking_away > 0))
    {
        ptr_channel->checking_away--;
    }
    else
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             "%s%s[%s%s%s]%s %s",
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_CHANNEL,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_317: '317' command (whois, idle)
 */

int
irc_protocol_cmd_317 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    int idle_time, day, hour, min, sec;
    time_t datetime;
    
    /* 317 message looks like:
       :server 317 mynick nick 122877 1205327880 :seconds idle, signon time
    */
    
    IRC_PROTOCOL_MIN_ARGS(6);
    
    /* make C compiler happy */
    (void) argv_eol;
    
    idle_time = atoi (argv[4]);
    day = idle_time / (60 * 60 * 24);
    hour = (idle_time % (60 * 60 * 24)) / (60 * 60);
    min = ((idle_time % (60 * 60 * 24)) % (60 * 60)) / 60;
    sec = ((idle_time % (60 * 60 * 24)) % (60 * 60)) % 60;
    
    datetime = (time_t)(atol (argv[5]));
    
    if (day > 0)
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             _("%s%s[%s%s%s]%s idle: %s%d %s%s, "
                               "%s%02d %s%s %s%02d %s%s %s%02d "
                               "%s%s, signon at: %s%s"),
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_NICK,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_CHANNEL,
                             day,
                             IRC_COLOR_CHAT,
                             NG_("day", "days", day),
                             IRC_COLOR_CHAT_CHANNEL,
                             hour,
                             IRC_COLOR_CHAT,
                             NG_("hour", "hours", hour),
                             IRC_COLOR_CHAT_CHANNEL,
                             min,
                             IRC_COLOR_CHAT,
                             NG_("minute", "minutes", min),
                             IRC_COLOR_CHAT_CHANNEL,
                             sec,
                             IRC_COLOR_CHAT,
                             NG_("second", "seconds", sec),
                             IRC_COLOR_CHAT_CHANNEL,
                             ctime (&datetime));
    }
    else
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             _("%s%s[%s%s%s]%s idle: %s%02d %s%s "
                               "%s%02d %s%s %s%02d %s%s, "
                               "signon at: %s%s"),
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_NICK,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_CHANNEL,
                             hour,
                             IRC_COLOR_CHAT,
                             NG_("hour", "hours", hour),
                             IRC_COLOR_CHAT_CHANNEL,
                             min,
                             IRC_COLOR_CHAT,
                             NG_("minute", "minutes", min),
                             IRC_COLOR_CHAT_CHANNEL,
                             sec,
                             IRC_COLOR_CHAT,
                             NG_("second", "seconds", sec),
                             IRC_COLOR_CHAT_CHANNEL,
                             ctime (&datetime));
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_321: '321' command (/list start)
 */

int
irc_protocol_cmd_321 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos_args;
    
    /* 321 message looks like:
       :server 321 mynick Channel :Users  Name
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);

    pos_args = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags(command, "irc_numeric"),
                         "%s%s%s%s",
                         irc_buffer_get_server_prefix (server, "network"),
                         argv[3],
                         (pos_args) ? " " : "",
                         (pos_args) ? pos_args : "");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_322: '322' command (channel for /list)
 */

int
irc_protocol_cmd_322 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos_topic;
    
    /* 322 message looks like:
       :server 322 mynick #channel 3 :topic of channel
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    pos_topic = (argc > 5) ?
        ((argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5]) : NULL;
    
    if (!server->cmd_list_regexp ||
        (regexec (server->cmd_list_regexp, argv[3], 0, NULL, 0) == 0))
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             "%s%s%s%s(%s%s%s)%s%s%s",
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_CHANNEL,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             argv[4],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             (pos_topic && pos_topic[0]) ? ": " : "",
                             (pos_topic && pos_topic[0]) ? pos_topic : "");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_323: '323' command (end of /list)
 */

int
irc_protocol_cmd_323 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos_args;
    
    /* 323 message looks like:
       :server 323 mynick :End of /LIST
    */

    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) argv;
    
    pos_args = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags(command, "irc_numeric"),
                         "%s%s",
                         irc_buffer_get_server_prefix (server, "network"),
                         (pos_args && pos_args[0]) ? pos_args : "");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_324: '324' command (channel mode)
 */

int
irc_protocol_cmd_324 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    struct t_irc_channel *ptr_channel;
    
    /* 324 message looks like:
       :server 324 mynick #channel +nt
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    /* make C compiler happy */
    (void) argv_eol;
    
    ptr_channel = irc_channel_search (server, argv[3]);
    if (ptr_channel)
    {
        if (argc > 4)
        {
            if (ptr_channel->modes)
                free (ptr_channel->modes);
            ptr_channel->modes = strdup (argv[4]);
            irc_mode_channel_set (server, ptr_channel,
                                  ptr_channel->modes);
        }
        else
        {
            if (ptr_channel->modes)
            {
                free (ptr_channel->modes);
                ptr_channel->modes = NULL;
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_327: '327' command (whois, host)
 */

int
irc_protocol_cmd_327 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos_realname;
    
    /* 327 message looks like:
       :server 327 mynick nick host ip :real hostname/ip
    */

    IRC_PROTOCOL_MIN_ARGS(6);
    
    pos_realname = (argc > 6) ?
        ((argv_eol[6][0] == ':') ? argv_eol[6] + 1 : argv_eol[6]) : NULL;
    
    if (pos_realname && pos_realname[0])
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             "%s%s[%s%s%s] %s%s %s %s(%s%s%s)",
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_NICK,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_HOST,
                             argv[4],
                             argv[5],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             pos_realname,
                             IRC_COLOR_CHAT_DELIMITERS);
    }
    else
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             "%s%s[%s%s%s] %s%s %s",
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_NICK,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_HOST,
                             argv[4],
                             argv[5]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_329: '329' command received (channel creation date)
 */

int
irc_protocol_cmd_329 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    struct t_irc_channel *ptr_channel;
    time_t datetime;
    
    /* 329 message looks like:
       :server 329 mynick #channel 1205327894
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    ptr_channel = irc_channel_search (server, argv[3]);
    
    datetime = (time_t)(atol ((argv_eol[4][0] == ':') ?
                              argv_eol[4] + 1 : argv_eol[4]));
    
    if (ptr_channel)
    {
        if (ptr_channel->display_creation_date)
        {
            weechat_printf_tags (ptr_channel->buffer,
                                 irc_protocol_tags(command, "irc_numeric"),
                                 _("%sChannel created on %s"),
                                 weechat_prefix ("network"),
                                 ctime (&datetime));
            ptr_channel->display_creation_date = 0;
        }
    }
    else
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             _("%sChannel %s%s%s created on %s"),
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_CHANNEL,
                             argv[3],
                             IRC_COLOR_CHAT,
                             ctime (&datetime));
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_331: '331' command received (no topic for channel)
 */

int
irc_protocol_cmd_331 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    
    /* 331 message looks like:
       :server 331 mynick #channel :There isn't a topic.
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    /* make C compiler happy */
    (void) argv_eol;
    
    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    weechat_printf_tags (ptr_buffer,
                         irc_protocol_tags(command, "irc_numeric"),
                         _("%sNo topic set for channel %s%s"),
                         (ptr_buffer == server->buffer) ?
                         irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                         IRC_COLOR_CHAT_CHANNEL,
                         argv[3]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_332: '332' command received (topic of channel)
 */

int
irc_protocol_cmd_332 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos_topic;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    
    /* 332 message looks like:
       :server 332 mynick #channel :topic of channel
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    pos_topic = (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4];
    
    ptr_channel = irc_channel_search (server, argv[3]);
    
    if (ptr_channel && ptr_channel->nicks)
    {
        irc_channel_set_topic (ptr_channel, pos_topic);
        ptr_buffer = ptr_channel->buffer;
    }
    else
        ptr_buffer = server->buffer;
    
    weechat_printf_tags (ptr_buffer,
                         irc_protocol_tags(command, "irc_numeric"),
                         _("%sTopic for %s%s%s is: \"%s%s\""),
                         (ptr_buffer == server->buffer) ?
                         irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                         IRC_COLOR_CHAT_CHANNEL,
                         argv[3],
                         IRC_COLOR_CHAT,
                         pos_topic,
                         IRC_COLOR_CHAT);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_333: '333' command received (infos about topic (nick / date))
 */

int
irc_protocol_cmd_333 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    struct t_irc_channel *ptr_channel;
    time_t datetime;
    
    /* 333 message looks like:
       :server 333 mynick #channel nick 1205428096
    */
    
    IRC_PROTOCOL_MIN_ARGS(6);
    
    ptr_channel = irc_channel_search (server, argv[3]);
    datetime = (time_t)(atol ((argv_eol[5][0] == ':') ?
                              argv_eol[5] + 1 : argv_eol[5]));
    if (ptr_channel && ptr_channel->nicks)
    {
        weechat_printf_tags (ptr_channel->buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             _("%sTopic set by %s%s%s on %s"),
                             weechat_prefix ("network"),
                             IRC_COLOR_CHAT_NICK,
                             argv[4],
                             IRC_COLOR_CHAT,
                             ctime (&datetime));
    }
    else
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             _("%sTopic for %s%s%s set by %s%s%s on %s"),
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_CHANNEL,
                             argv[3],
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_NICK,
                             argv[4],
                             IRC_COLOR_CHAT,
                             ctime (&datetime));
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_338: '338' command (whois, host)
 */

int
irc_protocol_cmd_338 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 338 message looks like:
       :server 338 mynick nick host :actually using host
    */
    
    IRC_PROTOCOL_MIN_ARGS(6);
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags(command, "irc_numeric"),
                         "%s%s[%s%s%s]%s %s %s%s",
                         irc_buffer_get_server_prefix (server, "network"),
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT_NICK,
                         argv[3],
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT,
                         (argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5],
                         IRC_COLOR_CHAT_HOST,
                         argv[4]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_341: '341' command received (inviting)
 */

int
irc_protocol_cmd_341 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 341 message looks like:
       :server 341 mynick nick #channel
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    /* make C compiler happy */
    (void) argv_eol;
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags(command, "irc_numeric"),
                         _("%s%s%s%s has invited %s%s%s on %s%s"),
                         irc_buffer_get_server_prefix (server, "network"),
                         IRC_COLOR_CHAT_NICK,
                         argv[2],
                         IRC_COLOR_CHAT,
                         IRC_COLOR_CHAT_NICK,
                         argv[3],
                         IRC_COLOR_CHAT,
                         IRC_COLOR_CHAT_CHANNEL,
                         argv[4]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_344: '344' command (channel reop)
 */

int
irc_protocol_cmd_344 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 344 message looks like:
       :server 344 mynick #channel nick!user@host
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags(command, "irc_numeric"),
                         _("%sChannel reop %s%s%s: %s%s"),
                         irc_buffer_get_server_prefix (server, "network"),
                         IRC_COLOR_CHAT_CHANNEL,
                         argv[3],
                         IRC_COLOR_CHAT,
                         IRC_COLOR_CHAT_HOST,
                         (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_345: '345' command (end of channel reop)
 */

int
irc_protocol_cmd_345 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 345 message looks like:
       :server 345 mynick #channel :End of Channel Reop List
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    weechat_printf_tags (server->buffer,
                         irc_protocol_tags(command, "irc_numeric"),
                         "%s%s%s%s: %s",
                         irc_buffer_get_server_prefix (server, "network"),
                         IRC_COLOR_CHAT_CHANNEL,
                         argv[3],
                         IRC_COLOR_CHAT,
                         (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_348: '348' command received (channel exception list)
 */

int
irc_protocol_cmd_348 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    time_t datetime;
    
    /* 348 message looks like:
       :server 348 mynick #channel nick1!user1@host1 nick2!user2@host2 1205585109
       (nick2 is nick who set exception on nick1)
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    /* make C compiler happy */
    (void) argv_eol;
    
    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    if (argc >= 7)
    {
        datetime = (time_t)(atol (argv[6]));
        weechat_printf_tags (ptr_buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             _("%s%s[%s%s%s]%s exception %s%s%s "
                               "by %s%s %s(%s%s%s)%s on %s"),
                             (ptr_buffer == server->buffer) ?
                             irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_CHANNEL,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_HOST,
                             argv[4],
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_NICK,
                             irc_protocol_get_nick_from_host (argv[5]),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_HOST,
                             irc_protocol_get_address_from_host (argv[5]),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             ctime (&datetime));
    }
    else
    {
        weechat_printf_tags (ptr_buffer,
                             irc_protocol_tags(command, "irc_numeric"),
                             _("%s%s[%s%s%s]%s exception %s%s"),
                             (ptr_buffer == server->buffer) ?
                             irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_CHANNEL,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_HOST,
                             argv[4]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_349: '349' command received (end of channel exception list)
 */

int
irc_protocol_cmd_349 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos_args;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    
    /* 349 message looks like:
       :server 349 mynick #channel :End of Channel Exception List
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    pos_args = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;
    
    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    weechat_printf_tags (ptr_buffer,
                         irc_protocol_tags (command, "irc_numeric"),
                         "%s%s[%s%s%s]%s%s%s",
                         (ptr_buffer == server->buffer) ?
                         irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT_CHANNEL,
                         argv[3],
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT,
                         (pos_args) ? " " : "",
                         (pos_args) ? pos_args : "");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_351: '351' command received (server version)
 */

int
irc_protocol_cmd_351 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 351 message looks like:
       :server 351 mynick dancer-ircd-1.0.36(2006/07/23_13:11:50). server :iMZ dncrTS/v4
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    if (argc > 5)
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             "%s%s %s (%s)",
                             irc_buffer_get_server_prefix (server, "network"),
                             argv[3],
                             argv[4],
                             (argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5]);
    }
    else
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             "%s%s %s",
                             irc_buffer_get_server_prefix (server, "network"),
                             argv[3],
                             argv[4]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_352: '352' command (who)
 */

int
irc_protocol_cmd_352 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos_attr, *pos_hopcount, *pos_realname;
    int arg_start, length;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* 352 message looks like:
       :server 352 mynick #channel user host server nick (*) (H/G) :0 flashcode
    */
    
    IRC_PROTOCOL_MIN_ARGS(9);
    
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
    
    ptr_channel = irc_channel_search (server, argv[3]);
    if (ptr_channel)
    {
        if (ptr_channel->checking_away > 0)
        {
            ptr_nick = irc_nick_search (ptr_channel, argv[7]);
            if (ptr_nick)
            {
                if (ptr_nick->host)
                    free (ptr_nick->host);
                length = strlen (argv[4]) + 1 + strlen (argv[5]) + 1;
                ptr_nick->host = malloc (length);
                if (ptr_nick->host)
                    snprintf (ptr_nick->host, length, "%s@%s", argv[4], argv[5]);
                if (pos_attr)
                    irc_nick_set_away (ptr_channel, ptr_nick,
                                       (pos_attr[0] == 'G') ? 1 : 0);
            }
        }
    }
    else
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             "%s%s[%s%s%s] %s%s%s(%s%s@%s%s)%s "
                             "%s%s%s%s(%s)",
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_CHANNEL,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_NICK,
                             argv[7],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_HOST,
                             argv[4],
                             argv[5],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             (pos_attr) ? pos_attr : "",
                             (pos_attr) ? " " : "",
                             (pos_hopcount) ? pos_hopcount : "",
                             (pos_hopcount) ? " " : "",
                             (pos_realname) ? pos_realname : "");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_353: '353' command received (list of users on a channel)
 */

int
irc_protocol_cmd_353 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos_channel, *pos_nick, *color;
    int args, i, prefix_found;
    int is_chanowner, is_chanadmin, is_chanadmin2, is_op, is_halfop;
    int has_voice, is_chanuser;
    struct t_irc_channel *ptr_channel;
    
    /* 353 message looks like:
       :server 353 mynick = #channel :mynick nick1 @nick2 +nick3
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    if (irc_channel_is_channel (argv[3]))
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
    
    for (i = args; i < argc; i++)
    {
        pos_nick = (argv[i][0] == ':') ? argv[i] + 1 : argv[i];
        
        is_chanowner = 0;
        is_chanadmin = 0;
        is_chanadmin2 = 0;
        is_op = 0;
        is_halfop = 0;
        has_voice = 0;
        is_chanuser = 0;
        prefix_found = 1;
        
        while (prefix_found)
        {
            prefix_found = 0;
            
            if (irc_mode_nick_prefix_allowed (server, pos_nick[0]))
            {
                prefix_found = 1;
                switch (pos_nick[0])
                {
                    case '@': /* op */
                        is_op = 1;
                        color = IRC_COLOR_NICKLIST_PREFIX1;
                        break;
                    case '~': /* channel owner */
                        is_chanowner = 1;
                        color = IRC_COLOR_NICKLIST_PREFIX1;
                        break;
                    case '&': /* channel admin */
                        is_chanadmin = 1;
                        color = IRC_COLOR_NICKLIST_PREFIX1;
                        break;
                    case '!': /* channel admin (2) */
                        is_chanadmin2 = 1;
                        color = IRC_COLOR_NICKLIST_PREFIX1;
                        break;
                    case '%': /* half-op */
                        is_halfop = 1;
                        color = IRC_COLOR_NICKLIST_PREFIX2;
                        break;
                    case '+': /* voice */
                        has_voice = 1;
                        color = IRC_COLOR_NICKLIST_PREFIX3;
                        break;
                    case '-': /* channel user */
                        is_chanuser = 1;
                        color = IRC_COLOR_NICKLIST_PREFIX4;
                        break;
                    default:
                        color = IRC_COLOR_CHAT;
                        break;
                }
            }
            if (prefix_found)
                pos_nick++;
        }
        if (ptr_channel && ptr_channel->nicks)
        {
            if (!irc_nick_new (server, ptr_channel, pos_nick,
                               is_chanowner, is_chanadmin, is_chanadmin2,
                               is_op, is_halfop, has_voice, is_chanuser))
            {
                weechat_printf (server->buffer,
                                _("%s%s: cannot create nick \"%s\" "
                                  "for channel \"%s\""),
                                irc_buffer_get_server_prefix (server, "error"),
                                IRC_PLUGIN_NAME, pos_nick, ptr_channel->name);
            }
        }
    }

    if (!ptr_channel)
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             _("%sNicks %s%s%s: %s[%s%s%s]"),
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_CHANNEL,
                             pos_channel,
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             (argv_eol[args][0] == ':') ?
                             argv_eol[args] + 1 : argv_eol[args],
                             IRC_COLOR_CHAT_DELIMITERS);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_366: '366' command received (end of /names list)
 */

int
irc_protocol_cmd_366 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    struct t_irc_channel *ptr_channel;
    struct t_infolist *infolist;
    struct t_config_option *ptr_option;
    int num_nicks, num_op, num_halfop, num_voice, num_normal, length, i;
    char *string, *prefix;
    
    /* 366 message looks like:
       :server 366 mynick #channel :End of /NAMES list.
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
                    weechat_config_search_with_string (weechat_infolist_string (infolist,
                                                                                "prefix_color"),
                                                       NULL, NULL, &ptr_option,
                                                       NULL);
                    length +=
                        ((ptr_option) ? strlen (weechat_color (weechat_config_string (ptr_option))) : 0) +
                        strlen (weechat_infolist_string (infolist, "prefix")) +
                        strlen (IRC_COLOR_CHAT) +
                        strlen (weechat_infolist_string (infolist, "name")) + 1;
                }
            }
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
                            strcat (string, " ");
                        prefix = weechat_infolist_string (infolist, "prefix");
                        if (prefix[0] && (prefix[0] != ' '))
                        {
                            weechat_config_search_with_string (weechat_infolist_string (infolist,
                                                                                        "prefix_color"),
                                                               NULL, NULL, &ptr_option,
                                                               NULL);
                            if (ptr_option)
                                strcat (string, weechat_color (weechat_config_string (ptr_option)));
                            strcat (string, prefix);
                        }
                        strcat (string, IRC_COLOR_CHAT);
                        strcat (string, weechat_infolist_string (infolist, "name"));
                        i++;
                    }
                }
                weechat_printf_tags (ptr_channel->buffer,
                                     irc_protocol_tags (command, "irc_numeric"),
                                     _("%sNicks %s%s%s: %s[%s%s]"),
                                     weechat_prefix ("network"),
                                     IRC_COLOR_CHAT_CHANNEL,
                                     ptr_channel->name,
                                     IRC_COLOR_CHAT,
                                     IRC_COLOR_CHAT_DELIMITERS,
                                     string,
                                     IRC_COLOR_CHAT_DELIMITERS);
                free (string);
            }
            weechat_infolist_free (infolist);
        }
        
        /* display number of nicks, ops, halfops & voices on the channel */
        irc_nick_count (ptr_channel, &num_nicks, &num_op, &num_halfop,
                        &num_voice, &num_normal);
        weechat_printf_tags (ptr_channel->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             _("%sChannel %s%s%s: %s%d%s %s %s(%s%d%s %s, "
                               "%s%d%s %s, %s%d%s %s, %s%d%s %s%s)"),
                             weechat_prefix ("network"),
                             IRC_COLOR_CHAT_CHANNEL,
                             ptr_channel->name,
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_CHANNEL,
                             num_nicks,
                             IRC_COLOR_CHAT,
                             (num_nicks > 1) ? _("nicks") : _("nick"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_CHANNEL,
                             num_op,
                             IRC_COLOR_CHAT,
                             (num_op > 1) ? _("ops") : _("op"),
                             IRC_COLOR_CHAT_CHANNEL,
                             num_halfop,
                             IRC_COLOR_CHAT,
                             (num_halfop > 1) ? _("halfops") : _("halfop"),
                             IRC_COLOR_CHAT_CHANNEL,
                             num_voice,
                             IRC_COLOR_CHAT,
                             (num_voice > 1) ? _("voices") : _("voice"),
                             IRC_COLOR_CHAT_CHANNEL,
                             num_normal,
                             IRC_COLOR_CHAT,
                             _("normal"),
                             IRC_COLOR_CHAT_DELIMITERS);
        
        irc_command_mode_server (server, ptr_channel->name);
        irc_channel_check_away (server, ptr_channel, 1);
    }
    else
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             "%s%s%s%s: %s",
                             irc_buffer_get_server_prefix (server, "network"),
                             IRC_COLOR_CHAT_CHANNEL,
                             argv[3],
                             IRC_COLOR_CHAT,
                             (argv[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_367: '367' command received (banlist)
 */

int
irc_protocol_cmd_367 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    time_t datetime;
    
    /* 367 message looks like:
       :server 367 mynick #channel banmask nick!user@host 1205590879
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    /* make C compiler happy */
    (void) argv_eol;
    
    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    if (argc >= 7)
    {
        datetime = (time_t)(atol (argv[6]));
        weechat_printf_tags (ptr_buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             _("%s%s[%s%s%s] %s%s%s banned by "
                               "%s%s %s(%s%s%s)%s on %s"),
                             (ptr_buffer == server->buffer) ?
                             irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_CHANNEL,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_HOST,
                             argv[4],
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_NICK,
                             irc_protocol_get_nick_from_host (argv[5]),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_HOST,
                             irc_protocol_get_address_from_host (argv[5]),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT,
                             ctime (&datetime));
    }
    else
    {
        weechat_printf_tags (ptr_buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             _("%s%s[%s%s%s] %s%s%s banned by "
                               "%s%s %s(%s%s%s)"),
                             (ptr_buffer == server->buffer) ?
                             irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_CHANNEL,
                             argv[3],
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_HOST,
                             argv[4],
                             IRC_COLOR_CHAT,
                             IRC_COLOR_CHAT_NICK,
                             irc_protocol_get_nick_from_host (argv[5]),
                             IRC_COLOR_CHAT_DELIMITERS,
                             IRC_COLOR_CHAT_HOST,
                             irc_protocol_get_address_from_host (argv[5]),
                             IRC_COLOR_CHAT_DELIMITERS);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_368: '368' command received (end of banlist)
 */

int
irc_protocol_cmd_368 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    char *pos_args;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    
    /* 368 message looks like:
       :server 368 mynick #channel :End of Channel Ban List
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    pos_args = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;
    
    ptr_channel = irc_channel_search (server, argv[3]);
    ptr_buffer = (ptr_channel && ptr_channel->nicks) ?
        ptr_channel->buffer : server->buffer;
    weechat_printf_tags (ptr_buffer,
                         irc_protocol_tags (command, "irc_numeric"),
                         "%s%s[%s%s%s]%s%s%s",
                         (ptr_buffer == server->buffer) ?
                         irc_buffer_get_server_prefix (server, "network") : weechat_prefix ("network"),
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT_CHANNEL,
                         argv[3],
                         IRC_COLOR_CHAT_DELIMITERS,
                         IRC_COLOR_CHAT,
                         (pos_args) ? " " : "",
                         (pos_args) ? pos_args : "");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_432: '432' command received (erroneous nickname)
 */

int
irc_protocol_cmd_432 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    int i, nick_found, nick_to_use;
    
    irc_protocol_cmd_error (server, command, argc, argv, argv_eol);
    
    if (!server->is_connected)
    {
        nick_found = -1;
        nick_to_use = -1;
        for (i = 0; i < server->nicks_count; i++)
        {
            if (strcmp (server->nick, server->nicks_array[i]) == 0)
            {
                nick_found = i;
                break;
            }
        }
        if (nick_found < 0)
            nick_to_use = 0;
        else
        {
            if (nick_found < server->nicks_count - 1)
                nick_to_use = nick_found + 1;
        }
        if (nick_to_use < 0)
        {
            weechat_printf (server->buffer,
                            _("%s%s: all declared nicknames are "
                              "already in use or invalid, closing "
                              "connection with server"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
            irc_server_disconnect (server, 1);
            return WEECHAT_RC_OK;
        }
        
        weechat_printf (server->buffer,
                        _("%s%s: nickname \"%s\" is invalid, "
                          "trying nickname #%d (\"%s\")"),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME, server->nick, nick_to_use + 1,
                        server->nicks_array[nick_to_use]);
        
        irc_server_set_nick (server, server->nicks_array[nick_to_use]);
        
        irc_server_sendf (server, "NICK %s", server->nick);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_433: '433' command received (nickname already in use)
 */

int
irc_protocol_cmd_433 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    int i, nick_found, nick_to_use;
    
    if (!server->is_connected)
    {
        nick_found = -1;
        nick_to_use = -1;
        for (i = 0; i < server->nicks_count; i++)
        {
            if (strcmp (server->nick, server->nicks_array[i]) == 0)
            {
                nick_found = i;
                break;
            }
        }
        if (nick_found < 0)
            nick_to_use = 0;
        else
        {
            if (nick_found < server->nicks_count - 1)
                nick_to_use = nick_found + 1;
        }
        if (nick_to_use < 0)
        {
            weechat_printf (server->buffer,
                            _("%s%s: all declared nicknames are "
                              "already in use, closing "
                              "connection with server"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
            irc_server_disconnect (server, 1);
            return WEECHAT_RC_OK;
        }
        
        weechat_printf (server->buffer,
                        _("%s%s: nickname \"%s\" is already in use, "
                          "trying nickname #%d (\"%s\")"),
                        irc_buffer_get_server_prefix (server, NULL),
                        IRC_PLUGIN_NAME, server->nick,
                        nick_to_use + 1, server->nicks_array[nick_to_use]);
        
        irc_server_set_nick (server, server->nicks_array[nick_to_use]);
        
        irc_server_sendf (server, "NICK %s", server->nick);
    }
    else
    {
        return irc_protocol_cmd_error (server, command, argc, argv, argv_eol);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_438: '438' command received (not authorized to change nickname)
 */

int
irc_protocol_cmd_438 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 438 message looks like:
       :server 438 mynick newnick :Nick change too fast. Please wait 30 seconds.
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    if (argc >= 5)
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             "%s%s (%s => %s)",
                             irc_buffer_get_server_prefix (server, "network"),
                             (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4],
                             argv[2],
                             argv[3]);
    }
    else
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             "%s%s %s",
                             irc_buffer_get_server_prefix (server, "network"),
                             argv[2],
                             argv[3]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_901: '901' command received (you are now logged in)
 */

int
irc_protocol_cmd_901 (struct t_irc_server *server, const char *command,
                      int argc, char **argv, char **argv_eol)
{
    /* 901 message looks like:
       :server 901 mynick nick user host :You are now logged in. (id nick, username user, hostname host)
    */
    
    IRC_PROTOCOL_MIN_ARGS(6);
    
    if (argc >= 7)
    {
        weechat_printf_tags (server->buffer,
                             irc_protocol_tags (command, "irc_numeric"),
                             "%s%s",
                             irc_buffer_get_server_prefix (server, "network"),
                             (argv_eol[6][0] == ':') ? argv_eol[6] + 1 : argv_eol[6]);
    }
    else
        irc_protocol_cmd_numeric (server, command, argc, argv, argv_eol);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_is_numeric_command: return 1 if given string is 100% numeric
 */

int
irc_protocol_is_numeric_command (const char *str)
{
    while (str && str[0])
    {
        if (!isdigit (str[0]))
            return 0;
        str++;
    }
    return 1;
}

/*
 * irc_protocol_recv_command: executes action when receiving IRC command
 *                            return:  0 = all ok, command executed
 *                                    -1 = command failed
 *                                    -2 = no command to execute
 *                                    -3 = command not found
 */

void
irc_protocol_recv_command (struct t_irc_server *server, const char *entire_line,
                           const char *host, const char *command,
                           const char *arguments)
{
    int i, cmd_found, return_code, argc, decode_color;
    char *pos, *nick;
    char *dup_entire_line, *dup_host, *dup_arguments, *irc_message;
    t_irc_recv_func *cmd_recv_func;
    const char *cmd_name;
    char **argv, **argv_eol;
    struct t_irc_protocol_msg irc_protocol_messages[] =
        { { "error", /* error received from IRC server */ 1, &irc_protocol_cmd_error },
          { "invite", /* invite a nick on a channel */ 1, &irc_protocol_cmd_invite },
          { "join", /* join a channel */ 1, &irc_protocol_cmd_join },
          { "kick", /* forcibly remove a user from a channel */ 1, &irc_protocol_cmd_kick },
          { "kill", /* close client-server connection */ 1, &irc_protocol_cmd_kill },
          { "mode", /* change channel or user mode */ 1, &irc_protocol_cmd_mode },
          { "nick", /* change current nickname */ 1, &irc_protocol_cmd_nick },
          { "notice", /* send notice message to user */ 1, &irc_protocol_cmd_notice },
          { "part", /* leave a channel */ 1, &irc_protocol_cmd_part },
          { "ping", /* ping server */ 1, &irc_protocol_cmd_ping },
          { "pong", /* answer to a ping message */ 1, &irc_protocol_cmd_pong },
          { "privmsg", /* message received */ 1, &irc_protocol_cmd_privmsg },
          { "quit", /* close all connections and quit */ 1, &irc_protocol_cmd_quit },
          { "topic", /* get/set channel topic */ 0, &irc_protocol_cmd_topic },
          { "wallops", /* send a message to all currently connected users who have "
                          "set the 'w' user mode "
                          "for themselves */ 1, &irc_protocol_cmd_wallops },
          { "001", /* a server message */ 1, &irc_protocol_cmd_001 },
          { "005", /* a server message */ 1, &irc_protocol_cmd_005 },
          { "221", /* user mode string */ 1, &irc_protocol_cmd_221 },
          { "301", /* away message */ 1, &irc_protocol_cmd_301 },
          { "303", /* ison */ 1, &irc_protocol_cmd_303 },
          { "305", /* unaway */ 1, &irc_protocol_cmd_305 },
          { "306", /* now away */ 1, &irc_protocol_cmd_306 },
          { "307", /* whois (registered nick) */ 1, &irc_protocol_cmd_whois_nick_msg },
          { "310", /* whois (help mode) */ 1, &irc_protocol_cmd_whois_nick_msg },
          { "311", /* whois (user) */ 1, &irc_protocol_cmd_311 },
          { "312", /* whois (server) */ 1, &irc_protocol_cmd_312 },
          { "313", /* whois (operator) */ 1, &irc_protocol_cmd_whois_nick_msg },
          { "314", /* whowas */ 1, &irc_protocol_cmd_314 },
          { "315", /* end of /who list */ 1, &irc_protocol_cmd_315 },
          { "317", /* whois (idle) */ 1, &irc_protocol_cmd_317 },
          { "318", /* whois (end) */ 1, &irc_protocol_cmd_whois_nick_msg },
          { "319", /* whois (channels) */ 1, &irc_protocol_cmd_whois_nick_msg },
          { "320", /* whois (identified user) */ 1, &irc_protocol_cmd_whois_nick_msg },
          { "321", /* /list start */ 1, &irc_protocol_cmd_321 },  
          { "322", /* channel (for /list) */ 1, &irc_protocol_cmd_322 },
          { "323", /* end of /list */ 1, &irc_protocol_cmd_323 },
          { "324", /* channel mode */ 1, &irc_protocol_cmd_324 },
          { "326", /* whois (has oper privs) */ 1, &irc_protocol_cmd_whois_nick_msg },
          { "327", /* whois (host) */ 1, &irc_protocol_cmd_327 },
          { "329", /* channel creation date */ 1, &irc_protocol_cmd_329 },
          { "331", /* no topic for channel */ 1, &irc_protocol_cmd_331 },
          { "332", /* topic of channel */ 1, &irc_protocol_cmd_332 },
          { "333", /* infos about topic (nick and date changed) */ 1, &irc_protocol_cmd_333 },
          { "338", /* whois (host) */ 1, &irc_protocol_cmd_338 },
          { "341", /* inviting */ 1, &irc_protocol_cmd_341 },
          { "344", /* channel reop */ 1, &irc_protocol_cmd_344 },
          { "345", /* end of channel reop list */ 1, &irc_protocol_cmd_345 },
          { "348", /* channel exception list */ 1, &irc_protocol_cmd_348 },
          { "349", /* end of channel exception list */ 1, &irc_protocol_cmd_349 },
          { "351", /* server version */ 1, &irc_protocol_cmd_351 },
          { "352", /* who */ 1, &irc_protocol_cmd_352 },
          { "353", /* list of nicks on channel */ 1, &irc_protocol_cmd_353 },
          { "366", /* end of /names list */ 1, &irc_protocol_cmd_366 },
          { "367", /* banlist */ 1, &irc_protocol_cmd_367 },
          { "368", /* end of banlist */ 1, &irc_protocol_cmd_368 },
          { "378", /* whois (connecting from) */ 1, &irc_protocol_cmd_whois_nick_msg },
          { "379", /* whois (using modes) */ 1, &irc_protocol_cmd_whois_nick_msg },
          { "401", /* no such nick/channel */ 1, &irc_protocol_cmd_error },
          { "402", /* no such server */ 1, &irc_protocol_cmd_error },
          { "403", /* no such channel */ 1, &irc_protocol_cmd_error },
          { "404", /* cannot send to channel */ 1, &irc_protocol_cmd_error },
          { "405", /* too many channels */ 1, &irc_protocol_cmd_error },
          { "406", /* was no such nick */ 1, &irc_protocol_cmd_error },
          { "407", /* was no such nick */ 1, &irc_protocol_cmd_error },
          { "409", /* no origin */ 1, &irc_protocol_cmd_error },
          { "410", /* no services */ 1, &irc_protocol_cmd_error },
          { "411", /* no recipient */ 1, &irc_protocol_cmd_error },
          { "412", /* no text to send */ 1, &irc_protocol_cmd_error },
          { "413", /* no toplevel */ 1, &irc_protocol_cmd_error },
          { "414", /* wilcard in toplevel domain */ 1, &irc_protocol_cmd_error },
          { "421", /* unknown command */ 1, &irc_protocol_cmd_error },
          { "422", /* MOTD is missing */ 1, &irc_protocol_cmd_error },
          { "423", /* no administrative info */ 1, &irc_protocol_cmd_error },
          { "424", /* file error */ 1, &irc_protocol_cmd_error },
          { "431", /* no nickname given */ 1, &irc_protocol_cmd_error },
          { "432", /* erroneous nickname */ 1, &irc_protocol_cmd_432 },
          { "433", /* nickname already in use */ 1, &irc_protocol_cmd_433 },
          { "436", /* nickname collision */ 1, &irc_protocol_cmd_error },
          { "437", /* resource unavailable */ 1, &irc_protocol_cmd_error },
          { "438", /* not authorized to change nickname */ 1, &irc_protocol_cmd_438 },
          { "441", /* user not in channel */ 1, &irc_protocol_cmd_error },
          { "442", /* not on channel */ 1, &irc_protocol_cmd_error },
          { "443", /* user already on channel */ 1, &irc_protocol_cmd_error },
          { "444", /* user not logged in */ 1, &irc_protocol_cmd_error },
          { "445", /* summon has been disabled */ 1, &irc_protocol_cmd_error },
          { "446", /* users has been disabled */ 1, &irc_protocol_cmd_error },
          { "451", /* you are not registered */ 1, &irc_protocol_cmd_error },
          { "461", /* not enough parameters */ 1, &irc_protocol_cmd_error },
          { "462", /* you may not register */ 1, &irc_protocol_cmd_error },
          { "463", /* your host isn't among the privileged */ 1, &irc_protocol_cmd_error },
          { "464", /* password incorrect */ 1, &irc_protocol_cmd_error },
          { "465", /* you are banned from this server */ 1, &irc_protocol_cmd_error },
          { "467", /* channel key already set */ 1, &irc_protocol_cmd_error },
          { "470", /* forwarding to another channel */ 1, &irc_protocol_cmd_error },
          { "471", /* channel is already full */ 1, &irc_protocol_cmd_error },
          { "472", /* unknown mode char to me */ 1, &irc_protocol_cmd_error },
          { "473", /* cannot join channel (invite only) */ 1, &irc_protocol_cmd_error },
          { "474", /* cannot join channel (banned from channel) */ 1, &irc_protocol_cmd_error },
          { "475", /* cannot join channel (bad channel key) */ 1, &irc_protocol_cmd_error },
          { "476", /* bad channel mask */ 1, &irc_protocol_cmd_error },
          { "477", /* channel doesn't support modes */ 1, &irc_protocol_cmd_error },
          { "481", /* you're not an IRC operator */ 1, &irc_protocol_cmd_error },
          { "482", /* you're not channel operator */ 1, &irc_protocol_cmd_error },
          { "483", /* you can't kill a server! */ 1, &irc_protocol_cmd_error },
          { "484", /* your connection is restricted! */ 1, &irc_protocol_cmd_error },
          { "485", /* user is immune from kick/deop */ 1, &irc_protocol_cmd_error },
          { "487", /* network split */ 1, &irc_protocol_cmd_error },
          { "491", /* no O-lines for your host */ 1, &irc_protocol_cmd_error },
          { "501", /* unknown mode flag */ 1, &irc_protocol_cmd_error },
          { "502", /* can't change mode for other users */ 1, &irc_protocol_cmd_error },
          { "671", /* whois (secure connection) */ 1, &irc_protocol_cmd_whois_nick_msg },
          { "901", /* you are now logged in */ 1, &irc_protocol_cmd_901 },
          { "973", /* whois (secure connection) */ 1, &irc_protocol_cmd_server_mode_reason },
          { "974", /* whois (secure connection) */ 1, &irc_protocol_cmd_server_mode_reason },
          { "975", /* whois (secure connection) */ 1, &irc_protocol_cmd_server_mode_reason },
          { NULL, 0, NULL }
        };
    
    if (!command)
        return;
    
    /* send signal with received command */
    irc_server_send_signal (server, "irc_in", command, entire_line);
    
    /* look for IRC command */
    cmd_found = -1;
    for (i = 0; irc_protocol_messages[i].name; i++)
    {
        if (weechat_strcasecmp (irc_protocol_messages[i].name, command) == 0)
        {
            cmd_found = i;
            break;
        }
    }
    
    /* command not found */
    if (cmd_found < 0)
    {
        /* for numeric commands, we use default recv function */
        if (irc_protocol_is_numeric_command (command))
        {
            cmd_name = command;
            decode_color = 1;
            cmd_recv_func = irc_protocol_cmd_numeric;
        }
        else
        {
            weechat_printf (server->buffer,
                            _("%s%s: command \"%s\" not found:"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME, command);
            weechat_printf (server->buffer,
                            "%s%s",
                            irc_buffer_get_server_prefix (server, "error"),
                            entire_line);
            return;
        }
    }
    else
    {
        cmd_name = irc_protocol_messages[cmd_found].name;
        decode_color = irc_protocol_messages[cmd_found].decode_color;
        cmd_recv_func = irc_protocol_messages[cmd_found].recv_function;
    }
    
    if (cmd_recv_func != NULL)
    {
        if (entire_line)
        {
            if (decode_color)
                dup_entire_line = irc_color_decode (entire_line,
                                                    weechat_config_boolean (irc_config_network_colors_receive));
            else
                dup_entire_line = strdup (entire_line);
        }
        else
            dup_entire_line = NULL;
        argv = weechat_string_explode (dup_entire_line, " ", 0, 0, &argc);
        argv_eol = weechat_string_explode (dup_entire_line, " ", 1, 0, NULL);
        dup_host = (host) ? strdup (host) : NULL;
        dup_arguments = (arguments) ? strdup (arguments) : NULL;
        
        pos = (dup_host) ? strchr (dup_host, '!') : NULL;
        if (pos)
            pos[0] = '\0';
        nick = (dup_host) ? strdup (dup_host) : NULL;
        if (pos)
            pos[0] = '!';
        irc_message = strdup (dup_entire_line);
        
        return_code = (int) (cmd_recv_func) (server, cmd_name,
                                             argc, argv, argv_eol);
        
        if (return_code == WEECHAT_RC_ERROR)
        {
            weechat_printf (server->buffer,
                            _("%s%s: failed to parse command \"%s\" (please "
                              "report to developers):"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME, command);
            weechat_printf (server->buffer,
                            "%s%s",
                            irc_buffer_get_server_prefix (server, "error"),
                            entire_line);
        }
        
        /* send signal with received command */
        irc_server_send_signal (server, "irc_in2", command, entire_line);
        
        if (irc_message)
            free (irc_message);
        if (nick)
            free (nick);
        if (dup_entire_line)
            free (dup_entire_line);
        if (dup_host)
            free (dup_host);
        if (dup_arguments)
            free (dup_arguments);
        if (argv)
            weechat_string_free_exploded (argv);
        if (argv_eol)
            weechat_string_free_exploded (argv_eol);
    }
}
