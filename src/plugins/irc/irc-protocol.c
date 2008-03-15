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

/* irc-protocol.c: description of IRC commands, according to RFC 1459, 2810,
                   2811 2812 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#if defined(__OpenBSD__)
#include <utf8/wchar.h>
#else
#include <wchar.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <sys/time.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-protocol.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"
#include "irc-mode.h"


/*
 * irc_protocol_get_nick_from_host: get nick from host in an IRC message
 */

char *
irc_protocol_get_nick_from_host (char *host)
{
    static char nick[128];
    char *pos;

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
irc_protocol_get_address_from_host (char *host)
{
    static char address[256];
    char *pos;
    
    address[0] = '\0';
    if (host && (host[0] == ':'))
    {
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
 * irc_protocol_replace_vars: replace special IRC vars ($nick, $channel, $server) in a string
 *                            Note: result has to be free() after use
 */ 

char *
irc_protocol_replace_vars (struct t_irc_server *server,
                           struct t_irc_channel *channel, char *string)
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
 * irc_protocol_get_wide_char: get wide char from string (first char)
 */

wint_t
irc_protocol_get_wide_char (char *string)
{
    int char_size;
    wint_t result;
    
    if (!string || !string[0])
        return WEOF;
    
    char_size = weechat_utf8_char_size (string);
    switch (char_size)
    {
        case 1:
            result = (wint_t)string[0];
            break;
        case 2:
            result = ((wint_t)((unsigned char)string[0])) << 8
                |  ((wint_t)((unsigned char)string[1]));
            break;
        case 3:
            result = ((wint_t)((unsigned char)string[0])) << 16
                |  ((wint_t)((unsigned char)string[1])) << 8
                |  ((wint_t)((unsigned char)string[2]));
            break;
        case 4:
            result = ((wint_t)((unsigned char)string[0])) << 24
                |  ((wint_t)((unsigned char)string[1])) << 16
                |  ((wint_t)((unsigned char)string[2])) << 8
                |  ((wint_t)((unsigned char)string[3]));
            break;
        default:
            result = WEOF;
    }
    return result;
}

/*
 * irc_protocol_is_word_char: return 1 if given character is a "word character"
 */

int
irc_protocol_is_word_char (char *str)
{
    wint_t c = irc_protocol_get_wide_char (str);

    if (c == WEOF)
        return 0;
    
    if (iswalnum (c))
        return 1;
    
    switch (c)
    {
        case '-':
        case '_':
        case '|':
            return 1;
    }
    
    /* not a 'word char' */
    return 0;
}

/*
 * irc_protocol_is_highlight: return 1 if given message contains highlight (with given nick
 *                            or at least one of string in "irc_higlight" setting)
 */

int
irc_protocol_is_highlight (char *message, char *nick)
{
    char *msg, *highlight, *match, *match_pre, *match_post, *msg_pos, *pos, *pos_end;
    int end, length, startswith, endswith, wildcard_start, wildcard_end;
    
    /* empty message ? */
    if (!message || !message[0])
        return 0;
    
    /* highlight by nickname */
    match = strstr (message, nick);
    if (match)
    {
        match_pre = weechat_utf8_prev_char (message, match);
        if (!match_pre)
            match_pre = match - 1;
        match_post = match + strlen(nick);
        startswith = ((match == message) || (!irc_protocol_is_word_char (match_pre)));
        endswith = ((!match_post[0]) || (!irc_protocol_is_word_char (match_post)));
        if (startswith && endswith)
            return 1;
    }
    
    /* no highlight by nickname and "irc_highlight" is empty */
    if (!weechat_config_string (irc_config_irc_highlight)
        || !weechat_config_string (irc_config_irc_highlight)[0])
        return 0;
    
    /* convert both strings to lower case */
    if ((msg = strdup (message)) == NULL)
        return 0;
    highlight = strdup (weechat_config_string (irc_config_irc_highlight));
    if (!highlight)
    {
        free (msg);
        return 0;
    }
    pos = msg;
    while (pos[0])
    {
        pos[0] = tolower (pos[0]);
        pos++;
    }
    pos = highlight;
    while (pos[0])
    {
        pos[0] = tolower (pos[0]);
        pos++;
    }
    
    /* look in "irc_highlight" for highlight */
    pos = highlight;
    end = 0;
    while (!end)
    {
        pos_end = strchr (pos, ',');
        if (!pos_end)
        {
            pos_end = strchr (pos, '\0');
            end = 1;
        }
        /* error parsing string! */
        if (!pos_end)
        {
            free (msg);
            free (highlight);
            return 0;
        }
        
        length = pos_end - pos;
        pos_end[0] = '\0';
        if (length > 0)
        {
            if ((wildcard_start = (pos[0] == '*')))
            {
                pos++;
                length--;
            }
            if ((wildcard_end = (*(pos_end - 1) == '*')))
            {
                *(pos_end - 1) = '\0';
                length--;
            }
        }
            
        if (length > 0)
        {
            msg_pos = msg;
            /* highlight found! */
            while ((match = strstr (msg_pos, pos)) != NULL)
            {
                match_pre = match - 1;
                match_pre = weechat_utf8_prev_char (msg, match);
                if (!match_pre)
                    match_pre = match - 1;
                match_post = match + length;
                startswith = ((match == msg) || (!irc_protocol_is_word_char (match_pre)));
                endswith = ((!match_post[0]) || (!irc_protocol_is_word_char (match_post)));
                if ((wildcard_start && wildcard_end) ||
                    (!wildcard_start && !wildcard_end && 
                     startswith && endswith) ||
                    (wildcard_start && endswith) ||
                    (wildcard_end && startswith))
                {
                    free (msg);
                    free (highlight);
                    return 1;
                }
                msg_pos = match_post;
            }
        }
        
        if (!end)
            pos = pos_end + 1;
    }
    
    /* no highlight found with "irc_highlight" list */
    free (msg);
    free (highlight);
    return 0;
}

/*
 * irc_protocol_cmd_error: error received from server
 */

int
irc_protocol_cmd_error (struct t_irc_server *server, char *command,
                        int argc, char **argv, char **argv_eol,
                        int ignore, int highlight)
{
    int first_arg;
    char *chan_nick, *args;
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    /* make C compiler happy */
    (void) argc;
    (void) ignore;
    (void) highlight;
    
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
    
    weechat_printf (server->buffer,
                    "%s%s%s%s",
                    weechat_prefix ("network"),
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
irc_protocol_cmd_invite (struct t_irc_server *server, char *command,
                         int argc, char **argv, char **argv_eol,
                         int ignore, int highlight)
{
    /* INVITE message looks like:
       :nick!user@host INVITE mynick :#channel
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) argv_eol;
    (void) highlight;
    
    if (!ignore)
    {
        weechat_printf (server->buffer,
                        _("%sYou have been invited to %s%s%s by "
                          "%s%s"),
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_CHANNEL,
                        (argv[3][0] == ':') ? argv[3] + 1 : argv[3],
                        IRC_COLOR_CHAT,
                        IRC_COLOR_CHAT_NICK,
                        irc_protocol_get_nick_from_host (argv[0]));
        weechat_buffer_set (server->buffer, "hotlist",
                            WEECHAT_HOTLIST_HIGHLIGHT);
    }
    
    return WEECHAT_RC_OK;
}


/*
 * irc_protocol_cmd_join: 'join' message received
 */

int
irc_protocol_cmd_join (struct t_irc_server *server, char *command,
                       int argc, char **argv, char **argv_eol,
                       int ignore, int highlight)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    char *pos_channel;
    
    /* JOIN message looks like:
       :nick!user@host JOIN :#channel
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) argv_eol;
    (void) highlight;
    
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
                            weechat_prefix ("error"), "irc", pos_channel);
            return WEECHAT_RC_ERROR;
        }
    }
    
    if (!ignore)
    {
        weechat_printf (ptr_channel->buffer,
                        _("%s%s%s %s(%s%s%s)%s has joined %s%s"),
                        weechat_prefix ("join"),
                        IRC_COLOR_CHAT_NICK,
                        irc_protocol_get_nick_from_host (argv[0]),
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT_HOST,
                        irc_protocol_get_address_from_host (argv[0]),
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT,
                        IRC_COLOR_CHAT_CHANNEL,
                        pos_channel);
    }
    
    /* remove topic and display channel creation date if joining new channel */
    if (!ptr_channel->nicks)
    {
        if (ptr_channel->topic)
        {
            free (ptr_channel->topic);
            ptr_channel->topic = NULL;
            weechat_buffer_set (ptr_channel->buffer, "title", NULL);
        }
        ptr_channel->display_creation_date = 1;
    }
    
    /* add nick in channel */
    ptr_nick = irc_nick_new (server, ptr_channel,
                             irc_protocol_get_nick_from_host (argv[0]),
                             0, 0, 0, 0, 0, 0, 0);
    if (ptr_nick)
        ptr_nick->host = strdup (irc_protocol_get_address_from_host (argv[0]));
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_kick: 'kick' message received
 */

int
irc_protocol_cmd_kick (struct t_irc_server *server, char *command,
                       int argc, char **argv, char **argv_eol,
                       int ignore, int highlight)
{
    char *pos_comment;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    /* KICK message looks like:
       :nick1!user@host KICK #channel nick2 :kick reason
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) highlight;
    
    pos_comment = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;
    
    ptr_channel = irc_channel_search (server, argv[2]);
    if (!ptr_channel)
    {
        weechat_printf (server->buffer,
                        _("%s%s: channel \"%s\" not found for "
                          "\"%s\" command"),
                        weechat_prefix ("error"), "irc", argv[2],
                        "kick");
        return WEECHAT_RC_ERROR;
    }
    
    if (!ignore)
    {
        if (pos_comment)
        {
            weechat_printf (ptr_channel->buffer,
                            _("%s%s%s%s has kicked %s%s%s from %s%s %s(%s%s%s)"),
                            weechat_prefix ("quit"),
                            IRC_COLOR_CHAT_NICK,
                            irc_protocol_get_nick_from_host (argv[0]),
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
            weechat_printf (ptr_channel->buffer,
                            _("%s%s%s%s has kicked %s%s%s from %s%s"),
                            weechat_prefix ("quit"),
                            IRC_COLOR_CHAT_NICK,
                            irc_protocol_get_nick_from_host (argv[0]),
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
irc_protocol_cmd_kill (struct t_irc_server *server, char *command,
                       int argc, char **argv, char **argv_eol,
                       int ignore, int highlight)
{
    char *pos_comment;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* KILL message looks like:
       :nick1!user@host KILL mynick :kill reason
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) highlight;
    
    pos_comment = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (!ignore)
        {
            if (pos_comment)
            {
                weechat_printf (ptr_channel->buffer,
                                _("%sYou were killed by %s%s %s(%s%s%s)"),
                                weechat_prefix ("quit"),
                                IRC_COLOR_CHAT_NICK,
                                irc_protocol_get_nick_from_host (argv[0]),
                                IRC_COLOR_CHAT_DELIMITERS,
                                IRC_COLOR_CHAT,
                                pos_comment,
                                IRC_COLOR_CHAT_DELIMITERS);
            }
            else
            {
                weechat_printf (ptr_channel->buffer,
                                _("%sYou were killed by %s%s"),
                                weechat_prefix ("quit"),
                                IRC_COLOR_CHAT_NICK,
                                irc_protocol_get_nick_from_host (argv[0]));
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
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_mode: 'mode' message received
 */

int
irc_protocol_cmd_mode (struct t_irc_server *server, char *command,
                       int argc, char **argv, char **argv_eol,
                       int ignore, int highlight)
{
    char *pos_modes;
    struct t_irc_channel *ptr_channel;
    
    /* MODE message looks like:
       :nick!user@host MODE #test +o nick
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) highlight;
    
    pos_modes = (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3];
    
    if (irc_channel_is_channel (argv[2]))
    {
        ptr_channel = irc_channel_search (server, argv[2]);
        if (ptr_channel)
        {
            irc_mode_channel_set (server, ptr_channel, pos_modes);
            irc_server_sendf (server, "MODE %s", ptr_channel->name);
        }
        if (!ignore)
        {
            weechat_printf ((ptr_channel) ?
                            ptr_channel->buffer : server->buffer,
                            _("%sMode %s%s %s[%s%s%s]%s by %s%s"),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_CHANNEL,
                            ptr_channel->name,
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT,
                            pos_modes,
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_NICK,
                            irc_protocol_get_nick_from_host (argv[0]));
        }
    }
    else
    {
        if (!ignore)
        {
            weechat_printf (server->buffer,
                            _("%sUser mode %s[%s%s%s]%s by %s%s"),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT,
                            pos_modes,
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_NICK,
                            irc_protocol_get_nick_from_host (argv[0]));
        }
        irc_mode_user_set (server, pos_modes);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_nick: 'nick' message received
 */

int
irc_protocol_cmd_nick (struct t_irc_server *server, char *command,
                       int argc, char **argv, char **argv_eol,
                       int ignore, int highlight)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    char *old_nick, *new_nick;
    int nick_is_me;
    
    /* NICK message looks like:
       :oldnick!user@host NICK :newnick
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) argv_eol;
    (void) highlight;
    
    old_nick = irc_protocol_get_nick_from_host (argv[0]);
    new_nick = (argv[2][0] == ':') ? argv[2] + 1 : argv[2];
    
    nick_is_me = (strcmp (old_nick, server->nick) == 0) ? 1 : 0;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        switch (ptr_channel->type)
        {
            case IRC_CHANNEL_TYPE_PRIVATE:
            case IRC_CHANNEL_TYPE_DCC_CHAT:
                /* rename private window if this is with "old nick" */
                if (weechat_strcasecmp (ptr_channel->name, old_nick) == 0)
                {
                    free (ptr_channel->name);
                    ptr_channel->name = strdup (new_nick);
                }
                weechat_buffer_set (ptr_channel->buffer, "name", new_nick);
                break;
            case IRC_CHANNEL_TYPE_CHANNEL:
                /* rename nick in nicklist if found */
                ptr_nick = irc_nick_search (ptr_channel, old_nick);
                if (ptr_nick)
                {
                    /* temporary disable hotlist */
                    weechat_buffer_set (NULL, "hotlist", "-");
                    
                    /* change nick and display message on all channels */
                    irc_nick_change (server, ptr_channel, ptr_nick, new_nick);
                    if (!ignore)
                    {
                        if (nick_is_me)
                        {
                            weechat_printf (ptr_channel->buffer,
                                            _("%sYou are now known as %s%s"),
                                            weechat_prefix ("network"),
                                            IRC_COLOR_CHAT_NICK,
                                            new_nick);
                        }
                        else
                        {
                            weechat_printf (ptr_channel->buffer,
                                            _("%s%s%s%s is now known as %s%s"),
                                            weechat_prefix ("network"),
                                            IRC_COLOR_CHAT_NICK,
                                            old_nick,
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
irc_protocol_cmd_notice (struct t_irc_server *server, char *command,
                         int argc, char **argv, char **argv_eol,
                         int ignore, int highlight)
{
    char *nick, *host, *pos_args, *pos_end, *pos_usec;
    struct timeval tv;
    long sec1, usec1, sec2, usec2, difftime;
    struct t_irc_channel *ptr_channel;
    int highlight_displayed, look_infobar_delay_highlight;
    
    /* NOTICE message looks like:
       NOTICE AUTH :*** Looking up your hostname...
       :nick!user@host NOTICE mynick :notice text
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    if (argv[0][0] == ':')
    {
        nick = irc_protocol_get_nick_from_host (argv[0]);
        host = irc_protocol_get_address_from_host (argv[0]);
        pos_args = (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3];
    }
    else
    {
        nick = NULL;
        host = NULL;
        pos_args = (argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2];
    }
    
    look_infobar_delay_highlight = weechat_config_integer (
        weechat_config_get_weechat ("look_infobar_delay_highlight"));
    
    if (!ignore)
    {
        if (nick && strncmp (pos_args, "\01VERSION", 8) == 0)
        {
            pos_args += 9;
            pos_end = strchr (pos_args, '\01');
            if (pos_end)
                pos_end[0] = '\0';
            weechat_printf (server->buffer,
                            _("%sCTCP %sVERSION%s reply from %s%s%s: %s"),
                            weechat_prefix ("network"),
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
                        
                        weechat_printf (server->buffer,
                                        _("%sCTCP %sPING%s reply from "
                                          "%s%s%s: %ld.%ld %s"),
                                        weechat_prefix ("network"),
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
                if (nick && weechat_config_boolean (irc_config_irc_notice_as_pv))
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
                                            weechat_prefix ("error"), "irc",
                                            nick);
                            return WEECHAT_RC_ERROR;
                        }
                    }
                    if (!ptr_channel->topic)
                    {
                        ptr_channel->topic = strdup ((host) ? host : "");
                        weechat_buffer_set (ptr_channel->buffer,
                                            "title", ptr_channel->topic);
                    }
                    
                    if (highlight
                        || irc_protocol_is_highlight (pos_args, server->nick))
                    {
                        weechat_printf (ptr_channel->buffer,
                                        "%s%s",
                                        irc_nick_as_prefix (NULL, nick,
                                                            IRC_COLOR_CHAT_HIGHLIGHT),
                                        pos_args);
                        if ((look_infobar_delay_highlight > 0)
                            && (ptr_channel->buffer != weechat_current_buffer))
                        {
                            weechat_infobar_printf (look_infobar_delay_highlight,
                                                    IRC_COLOR_INFOBAR_HIGHLIGHT,
                                                    _("Private %s> %s"),
                                                    nick, pos_args);
                        }
                        highlight_displayed = 1;
                    }
                    else
                    {
                        weechat_printf (ptr_channel->buffer,
                                        "%s%s",
                                        irc_nick_as_prefix (NULL, nick,
                                                            IRC_COLOR_CHAT_NICK_OTHER),
                                        pos_args);
                        highlight_displayed = 0;
                    }
                    
                    /* send "irc_highlight" signal */
                    if (highlight_displayed)
                    {
                        weechat_hook_signal_send ("irc_highlight",
                                                  WEECHAT_HOOK_SIGNAL_STRING,
                                                  argv_eol[0]);
                    }
                }
                else
                {
                    if (host && host[0])
                    {
                        weechat_printf (server->buffer,
                                        "%s%s%s %s(%s%s%s)%s: %s",
                                        weechat_prefix ("network"),
                                        IRC_COLOR_CHAT_NICK,
                                        nick,
                                        IRC_COLOR_CHAT_DELIMITERS,
                                        IRC_COLOR_CHAT_HOST,
                                        host,
                                        IRC_COLOR_CHAT_DELIMITERS,
                                        IRC_COLOR_CHAT,
                                        pos_args);
                    }
                    else
                    {
                        if (nick && nick[0])
                        {
                            weechat_printf (server->buffer,
                                            "%s%s%s%s: %s",
                                            weechat_prefix ("network"),
                                            IRC_COLOR_CHAT_NICK,
                                            nick,
                                            IRC_COLOR_CHAT,
                                            pos_args);
                        }
                        else
                        {
                            weechat_printf (server->buffer,
                                            "%s%s",
                                            weechat_prefix ("network"),
                                            pos_args);
                        }
                    }
                }
                
                if (nick
                    && (weechat_strcasecmp (nick, "nickserv") != 0)
                    && (weechat_strcasecmp (nick, "chanserv") != 0)
                    && (weechat_strcasecmp (nick, "memoserv") != 0))
                {
                    weechat_buffer_set (server->buffer, "hotlist",
                                        WEECHAT_HOTLIST_PRIVATE);
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
irc_protocol_cmd_part (struct t_irc_server *server, char *command,
                       int argc, char **argv, char **argv_eol,
                       int ignore, int highlight)
{
    char *nick, *host, *pos_comment, *join_string;
    int join_length;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* PART message looks like:
       :nick!user@host PART #channel :part message
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) highlight;
    
    nick = irc_protocol_get_nick_from_host (argv[0]);
    host = irc_protocol_get_address_from_host (argv[0]);
    
    pos_comment = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    
    ptr_channel = irc_channel_search (server, argv[2]);
    if (ptr_channel)
    {
        ptr_nick = irc_nick_search (ptr_channel, nick);
        if (ptr_nick)
        {
            /* display part message */
            if (!ignore)
            {
                if (pos_comment)
                {
                    weechat_printf (ptr_channel->buffer,
                                    _("%s%s%s %s(%s%s%s)%s has left %s%s %s(%s%s%s)"),
                                    weechat_prefix ("quit"),
                                    IRC_COLOR_CHAT_NICK,
                                    nick,
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    IRC_COLOR_CHAT_HOST,
                                    host,
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
                    weechat_printf (ptr_channel->buffer,
                                    _("%s%s%s %s(%s%s%s)%s has left %s%s"),
                                    weechat_prefix ("quit"),
                                    IRC_COLOR_CHAT_NICK,
                                    nick,
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    IRC_COLOR_CHAT_HOST,
                                    host,
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
                        join_string = (char *)malloc (join_length * sizeof (char));
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
                if (ptr_channel->close)
                {
                    weechat_buffer_close (ptr_channel->buffer, 1);
                    ptr_channel->buffer = NULL;
                    irc_channel_free (server, ptr_channel);
                    ptr_channel = NULL;
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
irc_protocol_cmd_ping (struct t_irc_server *server, char *command,
                       int argc, char **argv, char **argv_eol,
                       int ignore, int highlight)
{
    /* PING message looks like:
       PING :server
    */
    
    IRC_PROTOCOL_MIN_ARGS(2);
    
    /* make C compiler happy */
    (void) argv_eol;
    (void) ignore;
    (void) highlight;
    
    irc_server_sendf (server, "PONG :%s",
                      (argv[1][0] == ':') ? argv[1] + 1 : argv[1]);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_pong: 'pong' command received
 */

int
irc_protocol_cmd_pong (struct t_irc_server *server, char *command,
                       int argc, char **argv, char **argv_eol,
                       int ignore, int highlight)
{
    struct timeval tv;
    int old_lag;
    
    /* make C compiler happy */
    (void) command;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    (void) ignore;
    (void) highlight;
    
    if (server->lag_check_time.tv_sec != 0)
    {
        /* calculate lag (time diff with lag check) */
        old_lag = server->lag;
        gettimeofday (&tv, NULL);
        server->lag = (int) weechat_timeval_diff (&(server->lag_check_time), &tv);
        //if (old_lag != server->lag)
        //    gui_status_draw (gui_current_window->buffer, 1);
        
        /* schedule next lag check */
        server->lag_check_time.tv_sec = 0;
        server->lag_check_time.tv_usec = 0;
        server->lag_next_check = time (NULL) +
            weechat_config_integer (irc_config_irc_lag_check);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_reply_version: send version in reply to "CTCP VERSION" request
 */

void
irc_protocol_reply_version (struct t_irc_server *server,
                            struct t_irc_channel *channel, char *nick,
                            char *message, char *str_version, int ignore)
{
    char *pos, *version, *date;
    struct t_gui_buffer *ptr_buffer;
    
    ptr_buffer = (channel) ? channel->buffer : server->buffer;
    
    if (!ignore)
    {
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
        
        version = weechat_info_get ("version");
        date = weechat_info_get ("date");
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
                                weechat_prefix ("network"),
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
                                weechat_prefix ("network"),
                                IRC_COLOR_CHAT_CHANNEL,
                                IRC_COLOR_CHAT,
                                IRC_COLOR_CHAT_NICK,
                                nick);
            }
        }
        weechat_hook_signal_send ("irc_ctcp",
                                  WEECHAT_HOOK_SIGNAL_STRING,
                                  message);
    }
}

/*
 * irc_protocol_cmd_privmsg: 'privmsg' command received
 */

int
irc_protocol_cmd_privmsg (struct t_irc_server *server, char *command,
                          int argc, char **argv, char **argv_eol,
                          int ignore, int highlight)
{
    char *nick, *host, *pos_args, *pos_end_01, *pos, *pos_message;
    char *dcc_args, *pos_file, *pos_addr, *pos_port, *pos_size, *pos_start_resume;  /* for DCC */
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    int highlight_displayed, look_infobar_delay_highlight;

    /* PRIVMSG message looks like:
       :nick!user@host PRIVMSG #channel :message for channel here
       :nick!user@host PRIVMSG mynick :message for private here
       :nick!user@host PRIVMSG #channel :ACTION is testing action
       :nick!user@host PRIVMSG mynick :ACTION is testing action
       :nick!user@host PRIVMSG mynick :\01DCC SEND file.txt 1488915698 50612 128\01
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    IRC_PROTOCOL_CHECK_HOST;
    
    look_infobar_delay_highlight = weechat_config_integer (
        weechat_config_get_weechat ("look_infobar_delay_highlight"));
    
    nick = irc_protocol_get_nick_from_host (argv[0]);
    host = irc_protocol_get_address_from_host (argv[0]);
    
    pos_args = (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3];
    
    /* receiver is a channel ? */
    if (irc_channel_is_channel (argv[2]))
    {
        ptr_channel = irc_channel_search (server, argv[2]);
        if (ptr_channel)
        {
            if (strncmp (pos_args, "\01ACTION ", 8) == 0)
            {
                if (!ignore)
                {
                    pos_args += 8;
                    pos_end_01 = strchr (pos_args, '\01');
                    if (pos_end_01)
                        pos_end_01[0] = '\0';
                    
                    if (highlight
                        || irc_protocol_is_highlight (pos_args, server->nick))
                    {
                        weechat_printf (ptr_channel->buffer,
                                        "%s%s%s %s%s",
                                        weechat_prefix ("action"),
                                        IRC_COLOR_CHAT_HIGHLIGHT,
                                        nick,
                                        IRC_COLOR_CHAT,
                                        pos_args);
                        if ((look_infobar_delay_highlight > 0)
                            && (ptr_channel->buffer != weechat_current_buffer))
                            weechat_infobar_printf (look_infobar_delay_highlight,
                                                    "color_infobar_highlight",
                                                    _("Channel %s: * %s %s"),
                                                    ptr_channel->name,
                                                    nick,
                                                    pos_args);
                        weechat_hook_signal_send ("irc_highlight",
                                                  WEECHAT_HOOK_SIGNAL_STRING,
                                                  argv_eol[0]);
                    }
                    else
                    {
                        weechat_printf (ptr_channel->buffer,
                                        "%s%s%s %s%s",
                                        weechat_prefix ("action"),
                                        IRC_COLOR_CHAT_NICK,
                                        nick,
                                        IRC_COLOR_CHAT,
                                        pos_args);
                    }
                    irc_channel_add_nick_speaking (ptr_channel, nick);
                    
                    if (pos_end_01)
                        pos_end_01[0] = '\01';
                }
                return WEECHAT_RC_OK;
            }
            if (strncmp (pos_args, "\01SOUND ", 7) == 0)
            {
                if (!ignore)
                {
                    pos_args += 7;
                    pos_end_01 = strchr (pos_args, '\01');
                    if (pos_end_01)
                        pos_end_01[0] = '\0';
                    
                    weechat_printf (ptr_channel->buffer,
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
                weechat_printf (ptr_channel->buffer,
                                _("%sCTCP %sPING%s received from %s%s"),
                                weechat_prefix ("network"),
                                IRC_COLOR_CHAT_CHANNEL,
                                IRC_COLOR_CHAT,
                                IRC_COLOR_CHAT_NICK,
                                nick);
                if (pos_end_01)
                    pos_end_01[0] = '\01';
                return WEECHAT_RC_OK;
            }
            if (strncmp (pos_args, "\01VERSION", 8) == 0)
            {
                if (!ignore)
                {
                    irc_protocol_reply_version (server, ptr_channel, nick,
                                                argv_eol[0], pos_args, ignore);
                }
                return WEECHAT_RC_OK;
            }
            
            /* unknown CTCP ? */
            pos_end_01 = strchr (pos_args + 1, '\01');
            if ((pos_args[0] == '\01')
                && pos_end_01 && (pos_end_01[1] == '\0'))
            {
                if (!ignore)
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
                        weechat_printf (ptr_channel->buffer,
                                        _("%sUnknown CTCP %s%s%s received "
                                          "from %s%s%s: %s"),
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
                        weechat_printf (ptr_channel->buffer,
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
            if (!ignore)
            {
                ptr_nick = irc_nick_search (ptr_channel, nick);
                if (highlight
                    || irc_protocol_is_highlight (pos_args, server->nick))
                {
                    weechat_printf (ptr_channel->buffer,
                                    "%s%s",
                                    irc_nick_as_prefix (ptr_nick,
                                                        (ptr_nick) ? NULL : nick,
                                                        IRC_COLOR_CHAT_HIGHLIGHT),
                                    pos_args);
                    if ((look_infobar_delay_highlight > 0)
                        && (ptr_channel->buffer != weechat_current_buffer))
                        weechat_infobar_printf (look_infobar_delay_highlight,
                                                "color_infobar_highlight",
                                                _("Channel %s: %s> %s"),
                                                ptr_channel->name,
                                                nick,
                                                pos_args);
                    weechat_buffer_set (ptr_channel->buffer, "hotlist",
                                        WEECHAT_HOTLIST_HIGHLIGHT);
                    weechat_hook_signal_send ("irc_highlight",
                                              WEECHAT_HOOK_SIGNAL_STRING,
                                              argv_eol[0]);
                }
                else
                {
                    weechat_printf (ptr_channel->buffer,
                                    "%s%s",
                                    irc_nick_as_prefix (ptr_nick,
                                                        (ptr_nick) ? NULL : nick,
                                                        NULL),
                                    pos_args);
                    weechat_buffer_set (ptr_channel->buffer, "hotlist",
                                        WEECHAT_HOTLIST_MESSAGE);
                }
                irc_channel_add_nick_speaking (ptr_channel, nick);
            }
        }
        else
        {
            weechat_printf (server->buffer,
                            _("%s%s: channel \"%s\" not found for \"%s\" "
                              "command"),
                            weechat_prefix ("error"), "irc",
                            argv[2], "privmsg");
            return WEECHAT_RC_ERROR;
        }
    }
    else
    {
        /* version asked by another user => answer with WeeChat version */
        if (strncmp (pos_args, "\01VERSION", 8) == 0)
        {
            if (!ignore)
            {
                irc_protocol_reply_version (server, NULL, nick, argv_eol[0],
                                            pos_args, ignore);
            }
            return WEECHAT_RC_OK;
        }
            
        /* ping request from another user => answer */
        if (strncmp (pos_args, "\01PING", 5) == 0)
        {
            if (!ignore)
            {
                pos_args += 5;
                while (pos_args[0] == ' ')
                {
                    pos_args++;
                }
                pos_end_01 = strchr (pos, '\01');
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
                weechat_printf (server->buffer,
                                _("%sCTCP %sPING%s received from %s%s"),
                                weechat_prefix ("network"),
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
            if (!ignore)
            {
                /* check if DCC SEND is ok, i.e. with 0x01 at end */
                pos_end_01 = strchr (pos_args + 1, '\01');
                if (!pos_end_01)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                pos_end_01[0] = '\0';
                dcc_args = strdup (pos_args + 9);
                pos_end_01[0] = '\01';
                
                if (!dcc_args)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: not enough memory for \"%s\" "
                                      "command"),
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                
                //irc_dcc_add (server, IRC_DCC_FILE_RECV,
                //             strtoul (pos_addr, NULL, 10),
                //             atoi (pos_port), nick, -1, pos_file, NULL,
                //             strtoul (pos_size, NULL, 10));
                
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
            if (!ignore)
            {
                /* check if DCC RESUME is ok, i.e. with 0x01 at end */
                pos_end_01 = strchr (pos_args + 1, '\01');
                if (!pos_end_01)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                pos_end_01[0] = '\0';
                dcc_args = strdup (pos_args + 11);
                pos_end_01[0] = '\01';
                
                if (!dcc_args)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: not enough memory for \"%s\" "
                                      "command"),
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                    
                //irc_dcc_accept_resume (server, pos_file, atoi (pos_port),
                //                       strtoul (pos_start_resume, NULL, 10));
                
                weechat_hook_signal_send ("irc_dcc",
                                          WEECHAT_HOOK_SIGNAL_STRING,
                                          argv_eol[0]);
                
                free (dcc_args);
            }
            return WEECHAT_RC_OK;
        }
            
        /* incoming DCC ACCEPT (resume accepted by sender) */
        if (strncmp (pos, "\01DCC ACCEPT", 11) == 0)
        {
            if (!ignore)
            {
                /* check if DCC ACCEPT is ok, i.e. with 0x01 at end */
                pos_end_01 = strchr (pos_args + 1, '\01');
                if (!pos_end_01)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                pos_end_01[0] = '\0';
                dcc_args = strdup (pos_args + 11);
                pos_end_01[0] = '\01';
                
                if (!dcc_args)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: not enough memory for \"%s\" "
                                      "command"),
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                
                //irc_dcc_start_resume (server, pos_file, atoi (pos_port),
                //                      strtoul (pos_start_resume, NULL, 10));
                
                weechat_hook_signal_send ("irc_dcc",
                                          WEECHAT_HOOK_SIGNAL_STRING,
                                          argv_eol[0]);
                
                free (dcc_args);
            }
            return WEECHAT_RC_OK;
        }
            
        /* incoming DCC CHAT */
        if (strncmp (pos, "\01DCC CHAT", 9) == 0)
        {
            if (!ignore)
            {
                /* check if DCC CHAT is ok, i.e. with 0x01 at end */
                pos_end_01 = strchr (pos_args + 1, '\01');
                if (!pos_end_01)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: cannot parse \"%s\" command"),
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
                    return WEECHAT_RC_ERROR;
                }
                
                pos_end_01[0] = '\0';
                dcc_args = strdup (pos_args + 9);
                pos_end_01[0] = '\01';
                
                if (!dcc_args)
                {
                    weechat_printf (server->buffer,
                                    _("%s%s: not enough memory for \"%s\" "
                                      "command"),
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                                    weechat_prefix ("error"), "irc",
                                    "privmsg");
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
                                    weechat_prefix ("error"), "irc",
                                    IRC_COLOR_CHAT_NICK,
                                    nick,
                                    IRC_COLOR_CHAT,
                                    pos_file);
                    free (dcc_args);
                    return WEECHAT_RC_ERROR;
                }
                
                //irc_dcc_add (server, IRC_DCC_CHAT_RECV,
                //             strtoul (pos_addr, NULL, 10),
                //             atoi (pos_port), nick, -1, NULL, NULL, 0);
                
                weechat_hook_signal_send ("irc_dcc",
                                          WEECHAT_HOOK_SIGNAL_STRING,
                                          argv_eol[0]);
                
                free (dcc_args);
            }
            return WEECHAT_RC_OK;
        }
        
        /* private message received => display it */
        ptr_channel = irc_channel_search (server, nick);
        
        if (strncmp (pos, "\01ACTION ", 8) == 0)
        {
            if (!ignore)
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
                                        weechat_prefix ("error"), "irc",
                                        nick);
                        return WEECHAT_RC_ERROR;
                    }
                }
                if (!ptr_channel->topic)
                {
                    ptr_channel->topic = strdup (host);
                    weechat_buffer_set (ptr_channel->buffer,
                                        "title", ptr_channel->topic);
                }
                    
                pos_args += 8;
                pos_end_01 = strchr (pos, '\01');
                if (pos_end_01)
                    pos_end_01[0] = '\0';
                if (highlight
                    || irc_protocol_is_highlight (pos_args, server->nick))
                {
                    weechat_printf (ptr_channel->buffer,
                                    "%s%s%s %s%s",
                                    weechat_prefix ("action"),
                                    IRC_COLOR_CHAT_HIGHLIGHT,
                                    nick,
                                    IRC_COLOR_CHAT,
                                    pos_args);
                    if ((look_infobar_delay_highlight > 0)
                        && (ptr_channel->buffer != weechat_current_buffer))
                    {
                        weechat_infobar_printf (look_infobar_delay_highlight,
                                                "look_infobar_highlight",
                                                _("Channel %s: * %s %s"),
                                                ptr_channel->name,
                                                nick, pos);
                    }
                    weechat_hook_signal_send ("irc_highlight",
                                              WEECHAT_HOOK_SIGNAL_STRING,
                                              argv_eol[0]);
                    weechat_buffer_set (ptr_channel->buffer, "hotlist",
                                        WEECHAT_HOTLIST_HIGHLIGHT);
                }
                else
                {
                    weechat_printf (ptr_channel->buffer,
                                    "%s%s%s %s%s",
                                    weechat_prefix ("action"),
                                    IRC_COLOR_CHAT_NICK,
                                    nick,
                                    IRC_COLOR_CHAT,
                                    pos_args);
                    weechat_hook_signal_send ("irc_pv",
                                              WEECHAT_HOOK_SIGNAL_STRING,
                                              argv_eol[0]);
                    weechat_buffer_set (ptr_channel->buffer, "hotlist",
                                        WEECHAT_HOTLIST_MESSAGE);
                }
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
                if (!ignore)
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
                        weechat_printf (server->buffer,
                                        _("%sUnknown CTCP %s%s%s received "
                                          "from %s%s%s: %s"),
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
                        weechat_printf (server->buffer,
                                        _("%sUnknown CTCP %s%s%s received "
                                          "from %s%s"),
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
            else
            {
                /* private message */
                if (!ignore)
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
                                            weechat_prefix ("error"),
                                            "irc", nick);
                            return WEECHAT_RC_ERROR;
                        }
                    }
                    if (ptr_channel->topic)
                        free (ptr_channel->topic);
                    ptr_channel->topic = strdup (host);
                    weechat_buffer_set (ptr_channel->buffer, "title",
                                        ptr_channel->topic);
                    
                    if (highlight
                        || irc_protocol_is_highlight (pos, server->nick))
                    {
                        weechat_printf (ptr_channel->buffer,
                                        "%s%s",
                                        irc_nick_as_prefix (NULL,
                                                            nick,
                                                            IRC_COLOR_CHAT_HIGHLIGHT),
                                        pos_args);
                        if ((look_infobar_delay_highlight > 0)
                            && (ptr_channel->buffer != weechat_current_buffer))
                            weechat_infobar_printf (look_infobar_delay_highlight,
                                                    "color_infobar_highlight",
                                                    _("Private %s> %s"),
                                                    nick, pos_args);
                        highlight_displayed = 1;
                    }
                    else
                    {
                        weechat_printf (ptr_channel->buffer,
                                        "%s%s",
                                        irc_nick_as_prefix (NULL,
                                                            nick,
                                                            IRC_COLOR_CHAT_NICK_OTHER),
                                        pos_args);
                        highlight_displayed = 0;
                    }
                    
                    weechat_hook_signal_send ("irc_pv",
                                              WEECHAT_HOOK_SIGNAL_STRING,
                                              argv_eol[0]);
                    
                    if (highlight_displayed)
                    {
                        weechat_hook_signal_send ("irc_highlight",
                                                  WEECHAT_HOOK_SIGNAL_STRING,
                                                  argv_eol[0]);
                    }
                    
                    weechat_buffer_set (ptr_channel->buffer,
                                            "hotlist", WEECHAT_HOTLIST_PRIVATE);
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
irc_protocol_cmd_quit (struct t_irc_server *server, char *command,
                       int argc, char **argv, char **argv_eol,
                       int ignore, int highlight)
{
    char *nick, *host, *pos_comment;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* QUIT message looks like:
       :nick!user@host QUIT :quit message
    */
    
    IRC_PROTOCOL_MIN_ARGS(2);
    IRC_PROTOCOL_CHECK_HOST;
    
    /* make C compiler happy */
    (void) highlight;
    
    nick = irc_protocol_get_nick_from_host (argv[0]);
    host = irc_protocol_get_address_from_host (argv[0]);
    
    pos_comment = (argc > 2) ?
        ((argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2]) : NULL;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            || (ptr_channel->type == IRC_CHANNEL_TYPE_DCC_CHAT))
            ptr_nick = NULL;
        else
            ptr_nick = irc_nick_search (ptr_channel, nick);
        
        if (ptr_nick || (strcmp (ptr_channel->name, nick) == 0))
        {
            if (ptr_nick)
                irc_nick_free (ptr_channel, ptr_nick);
            if (!ignore)
            {
                if (pos_comment && pos_comment[0])
                {
                    weechat_printf (ptr_channel->buffer,
                                    _("%s%s%s %s(%s%s%s)%s has quit "
                                      "%s(%s%s%s)"),
                                    weechat_prefix ("quit"),
                                    IRC_COLOR_CHAT_NICK,
                                    nick,
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    IRC_COLOR_CHAT_HOST,
                                    host,
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    IRC_COLOR_CHAT,
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    IRC_COLOR_CHAT,
                                    pos_comment,
                                    IRC_COLOR_CHAT_DELIMITERS);
                }
                else
                {
                    weechat_printf (ptr_channel->buffer,
                                    _("%s%s%s %s(%s%s%s)%s has quit"),
                                    weechat_prefix ("quit"),
                                    IRC_COLOR_CHAT_NICK,
                                    nick,
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    IRC_COLOR_CHAT_HOST,
                                    host,
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
irc_protocol_cmd_server_mode_reason (struct t_irc_server *server, char *command,
                                     int argc, char **argv, char **argv_eol,
                                     int ignore, int highlight)
{
    char *pos_args, *ptr_msg, *ptr_msg2;
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) argv;
    (void) highlight;
    
    if (!ignore)
    {
        /* skip nickname if at beginning of server message */
        pos_args = argv_eol[2];
        if (strncmp (server->nick, pos_args, strlen (server->nick)) == 0)
        {
            pos_args += strlen (server->nick) + 1;
            while (pos_args[0] == ' ')
            {
                pos_args++;
            }
        }
        
        ptr_msg = strchr (pos_args, ' ');
        ptr_msg2 = NULL;
        if (ptr_msg)
        {
            ptr_msg[0] = '\0';
            ptr_msg2 = ptr_msg + 1;
            while (ptr_msg2[0] == ' ')
            {
                ptr_msg2++;
            }
            if (ptr_msg2[0] == ':')
            {
                ptr_msg2++;
            }
        }
        
        weechat_printf (server->buffer,
                        "%s%s: %s",
                        weechat_prefix ("network"),
                        pos_args,
                        (ptr_msg2) ? ptr_msg2 : "");

        if (ptr_msg)
            ptr_msg[0] = ' ';
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_numeric: numeric command received from server
 */

int
irc_protocol_cmd_numeric (struct t_irc_server *server, char *command,
                          int argc, char **argv, char **argv_eol,
                          int ignore, int highlight)
{
    char *pos_args;
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) argv;
    (void) highlight;
    
    if (!ignore)
    {
        if (weechat_strcasecmp (server->nick, argv[2]) == 0)
        {
            pos_args = (argc > 3) ?
                ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
        }
        else
        {
            pos_args = (argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2];
        }
        
        weechat_printf (server->buffer,
                        "%s%s",
                        weechat_prefix ("network"),
                        pos_args);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_topic: 'topic' command received
 */

int
irc_protocol_cmd_topic (struct t_irc_server *server, char *command,
                        int argc, char **argv, char **argv_eol,
                        int ignore, int highlight)
{
    char *pos_topic;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *buffer;
    
    /* TOPIC message looks like:
       :nick!user@host TOPIC #channel :new topic for channel
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) highlight;
    
    if (!irc_channel_is_channel (argv[2]))
    {
        weechat_printf (server->buffer,
                        _("%s%s: \"%s\" command received without channel"),
                        weechat_prefix ("error"), "irc", "topic");
        return WEECHAT_RC_ERROR;
    }
    
    pos_topic = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    
    ptr_channel = irc_channel_search (server, argv[2]);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!ignore)
    {
        if (pos_topic && pos_topic[0])
        {
            weechat_printf (buffer,
                            _("%s%s%s%s has changed topic for %s%s%s to: "
                              "\"%s%s\""),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_NICK,
                            irc_protocol_get_nick_from_host (argv[0]),
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_CHANNEL,
                            argv[2],
                            IRC_COLOR_CHAT,
                            pos_topic,
                            IRC_COLOR_CHAT);
        }
        else
        {
            weechat_printf (buffer,
                            _("%s%s%s%s has unset topic for %s%s"),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_NICK,
                            irc_protocol_get_nick_from_host (argv[0]),
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_CHANNEL,
                            argv[2]);
        }
    }
    
    if (ptr_channel)
    {
        if (ptr_channel->topic)
            free (ptr_channel->topic);
        if (pos_topic)
            ptr_channel->topic = strdup (pos_topic);
        else
            ptr_channel->topic = strdup ("");
        weechat_buffer_set (ptr_channel->buffer, "title", ptr_channel->topic);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_wallops: 'wallops' command received
 */

int
irc_protocol_cmd_wallops (struct t_irc_server *server, char *command,
                          int argc, char **argv, char **argv_eol,
                          int ignore, int highlight)
{
    /* WALLOPS message looks like:
       :nick!user@host WALLOPS :message from admin
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) highlight;
    
    if (!ignore)
    {
        weechat_printf (server->buffer,
                        _("%sWallops from %s%s %s(%s%s%s)%s: %s"),
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_NICK,
                        irc_protocol_get_nick_from_host (argv[0]),
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT_HOST,
                        irc_protocol_get_address_from_host (argv[0]),
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
irc_protocol_cmd_001 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    char **commands, **ptr_cmd, *vars_replaced;
    char *away_msg;

    /* 001 message looks like:
       :server 001 mynick :Welcome to the dancer-ircd Network
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    if (strcmp (server->nick, argv[2]) != 0)
        irc_server_set_nick (server, argv[2]);
    
    irc_protocol_cmd_numeric (server, command, argc, argv, argv_eol,
                              ignore, highlight);
    
    /* connection to IRC server is ok! */
    server->is_connected = 1;
    server->lag_next_check = time (NULL) +
        weechat_config_integer (irc_config_irc_lag_check);
    
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
irc_protocol_cmd_005 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    char *pos, *pos2;

    /* 005 message looks like:
       :server 005 mynick MODES=4 CHANLIMIT=#:20 NICKLEN=16 USERLEN=10
           HOSTLEN=63 TOPICLEN=450 KICKLEN=450 CHANNELLEN=30 KEYLEN=23
           CHANTYPES=# PREFIX=(ov)@+ CASEMAPPING=ascii CAPAB IRCD=dancer
           :are available on this server
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    irc_protocol_cmd_numeric (server, command, argc, argv, argv_eol,
                              ignore, highlight);
    
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
irc_protocol_cmd_221 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    /* 221 message looks like:
       :server 221 nick :+s
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    /* make C compiler happy */
    (void) highlight;
    
    if (!ignore)
    {
        weechat_printf (server->buffer,
                        _("%sUser mode for %s%s%s is %s[%s%s%s]"),
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_NICK,
                        argv[2],
                        IRC_COLOR_CHAT,
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT,
                        (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3],
                        IRC_COLOR_CHAT_DELIMITERS);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_301: '301' command (away message)
 *                       received when we are talking to a user in private
 *                       and that remote user is away (we receive away message)
 */

int
irc_protocol_cmd_301 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    char *pos_away_msg;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    
    /* 301 message looks like:
       :server 301 mynick nick :away message for nick
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) highlight;
    
    if ((argc > 4) && !ignore)
    {
        pos_away_msg = (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4];
        
        /* look for private buffer to display message */
        ptr_channel = irc_channel_search (server, argv[3]);
        if (!weechat_config_boolean (irc_config_irc_show_away_once)
            || !ptr_channel
            || !(ptr_channel->away_message)
            || (strcmp (ptr_channel->away_message, pos_away_msg) != 0))
        {
            ptr_buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
            weechat_printf (ptr_buffer,
                            _("%s%s%s%s is away: %s"),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_NICK,
                            argv[3],
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
irc_protocol_cmd_303 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    /* 301 message looks like:
       :server 303 mynick :nick1 nick2
    */

    IRC_PROTOCOL_MIN_ARGS(4);
    
    /* make C compiler happy */
    (void) argv;
    (void) highlight;
    
    if (!ignore)
    {
        weechat_printf (server->buffer,
                        _("%sUsers online: %s%s"),
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_NICK,
                        (argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_305: '305' command (unaway)
 */

int
irc_protocol_cmd_305 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    /* 305 message looks like:
       :server 305 mynick :Does this mean you're really back?
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) argv;
    (void) highlight;
    
    if (!ignore && (argc > 3))
    {
        weechat_printf (server->buffer,
                        "%s%s",
                        weechat_prefix ("network"),
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
irc_protocol_cmd_306 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    /* 306 message looks like:
       :server 306 mynick :We'll miss you
    */
    
    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) argv;
    (void) highlight;
    
    if (!ignore && (argc > 3))
    {
        weechat_printf (server->buffer,
                        "%s%s",
                        weechat_prefix ("network"),
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
irc_protocol_cmd_whois_nick_msg (struct t_irc_server *server, char *command,
                                 int argc, char **argv, char **argv_eol,
                                 int ignore, int highlight)
{
    /* messages look like:
       :server 319 flashy FlashCode :some text here
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    /* make C compiler happy */
    (void) highlight;
    
    if (!ignore)
    {
        weechat_printf (server->buffer,
                        "%s%s[%s%s%s] %s%s",
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT_NICK,
                        argv[3],
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT,
                        (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_311: '311' command (whois, user)
 */

int
irc_protocol_cmd_311 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    /* 311 message looks like:
       :server 311 mynick nick user host * :realname here
    */
    
    IRC_PROTOCOL_MIN_ARGS(8);
    
    /* make C compiler happy */
    (void) highlight;
    
    if (!ignore)
    {
        weechat_printf (server->buffer,
                        "%s%s[%s%s%s] (%s%s@%s%s)%s: %s",
                        weechat_prefix ("network"),
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
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_312: '312' command (whois, server)
 */

int
irc_protocol_cmd_312 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    /* 312 message looks like:
       :server 312 mynick nick irc.freenode.net :http://freenode.net/
    */
    
    IRC_PROTOCOL_MIN_ARGS(6);
    
    /* make C compiler happy */
    (void) highlight;
    
    if (!ignore)
    {
        weechat_printf (server->buffer,
                        "%s%s[%s%s%s] %s%s %s(%s%s%s)",
                        weechat_prefix ("network"),
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
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_314: '314' command (whowas)
 */

int
irc_protocol_cmd_314 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    /* 314 message looks like:
       :server 314 mynick nick user host * :realname here
    */
    
    IRC_PROTOCOL_MIN_ARGS(8);
    
    /* make C compiler happy */
    (void) highlight;
    
    if (!ignore)
    {
        weechat_printf (server->buffer,
                        _("%s%s%s %s(%s%s@%s%s)%s was %s"),
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_NICK,
                        argv[3],
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT_HOST,
                        argv[4],
                        argv[5],
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT,
                        (argv_eol[7][0] == ':') ? argv_eol[7] + 1 : argv_eol[7]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_315: '315' command (end of /who)
 */

int
irc_protocol_cmd_315 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    /* 315 message looks like:
       :server 315 mynick #channel :End of /WHO list.
    */
    
    struct t_irc_channel *ptr_channel;

    IRC_PROTOCOL_MIN_ARGS(5);
    
    /* make C compiler happy */
    (void) highlight;
    
    ptr_channel = irc_channel_search (server, argv[3]);
    if (ptr_channel && (ptr_channel->checking_away > 0))
    {
        ptr_channel->checking_away--;
    }
    else
    {
        if (!ignore)
        {
            weechat_printf (server->buffer,
                            "%s%s[%s%s%s]%s %s",
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT_CHANNEL,
                            argv[3],
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT,
                            (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_317: '317' command (whois, idle)
 */

int
irc_protocol_cmd_317 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    int idle_time, day, hour, min, sec;
    time_t datetime;
    
    /* 317 message looks like:
       :server 317 mynick nick 122877 1205327880 :seconds idle, signon time
    */
    
    IRC_PROTOCOL_MIN_ARGS(6);
    
    /* make C compiler happy */
    (void) argv_eol;
    (void) highlight;
    
    if (!ignore)
    {
        idle_time = atoi (argv[4]);
        day = idle_time / (60 * 60 * 24);
        hour = (idle_time % (60 * 60 * 24)) / (60 * 60);
        min = ((idle_time % (60 * 60 * 24)) % (60 * 60)) / 60;
        sec = ((idle_time % (60 * 60 * 24)) % (60 * 60)) % 60;
        
        datetime = (time_t)(atol (argv[5]));
        
        if (day > 0)
        {
            weechat_printf (server->buffer,
                            _("%s%s[%s%s%s]%s idle: %s%d %s%s, "
                              "%s%02d %s%s %s%02d %s%s %s%02d "
                              "%s%s, signon at: %s%s"),
                            weechat_prefix ("network"),
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
            weechat_printf (server->buffer,
                            _("%s%s[%s%s%s]%s idle: %s%02d %s%s "
                              "%s%02d %s%s %s%02d %s%s, "
                              "signon at: %s%s"),
                            weechat_prefix ("network"),
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
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_321: '321' command (/list start)
 */

int
irc_protocol_cmd_321 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    char *pos_args;
    
    /* 321 message looks like:
       :server 321 mynick Channel :Users  Name
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);

    pos_args = (argc > 4) ?
        ((argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]) : NULL;
    
    /* make C compiler happy */
    (void) highlight;
    
    if (!ignore)
    {
        weechat_printf (server->buffer,
                        "%s%s%s%s",
                        weechat_prefix ("network"),
                        argv[3],
                        (pos_args) ? " " : "",
                        (pos_args) ? pos_args : "");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_322: '322' command (channel for /list)
 */

int
irc_protocol_cmd_322 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    char *pos_topic;
    
    /* 322 message looks like:
       :server 322 mynick #channel 3 :topic of channel
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    /* make C compiler happy */
    (void) highlight;
    
    pos_topic = (argc > 5) ?
        ((argv_eol[5][0] == ':') ? argv_eol[5] + 1 : argv_eol[5]) : NULL;
    
    if (!ignore)
    {
        if (!server->cmd_list_regexp ||
            (regexec (server->cmd_list_regexp, argv[3], 0, NULL, 0) == 0))
        {
            weechat_printf (server->buffer,
                            "%s%s%s%s(%s%s%s)%s%s%s",
                            weechat_prefix ("network"),
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
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_323: '323' command (end of /list)
 */

int
irc_protocol_cmd_323 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    char *pos_args;
    
    /* 323 message looks like:
       :server 323 mynick :End of /LIST
    */

    IRC_PROTOCOL_MIN_ARGS(3);
    
    /* make C compiler happy */
    (void) argv;
    (void) highlight;
    
    pos_args = (argc > 3) ?
        ((argv_eol[3][0] == ':') ? argv_eol[3] + 1 : argv_eol[3]) : NULL;
    
    if (!ignore)
    {
        weechat_printf (server->buffer,
                        "%s%s",
                        weechat_prefix ("network"),
                        (pos_args && pos_args[0]) ? pos_args : "");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_324: '324' command (channel mode)
 */

int
irc_protocol_cmd_324 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_modes, *pos;
    struct t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) ignore;
    (void) highlight;
    
    pos_channel = strchr (arguments, ' ');
    if (pos_channel)
    {
        pos_channel[0] = '\0';
        pos_channel++;
        while (pos_channel[0] == ' ')
            pos_channel++;
        
        pos_modes = strchr (pos_channel, ' ');
        if (pos_modes)
        {
            pos_modes[0] = '\0';
            pos_modes++;
            while (pos_modes[0] == ' ')
                pos_modes++;

            /* remove spaces after modes */
            pos = pos_modes + strlen (pos_modes) - 1;
            while ((pos >= pos_modes) && (pos[0] == ' '))
            {
                pos[0] = '\0';
                pos--;
            }

            /* search channel */
            ptr_channel = irc_channel_search (server, pos_channel);
            if (ptr_channel)
            {
                if (pos_modes[0])
                {
                    if (ptr_channel->modes)
                        ptr_channel->modes = (char *)realloc (ptr_channel->modes,
                                                              (strlen (pos_modes) + 1) * sizeof (char));
                    else
                        ptr_channel->modes = (char *)malloc ((strlen (pos_modes) + 1) * sizeof (char));
                    strcpy (ptr_channel->modes, pos_modes);
                    irc_mode_channel_set (server, ptr_channel, pos_modes);
                }
                else
                {
                    if (ptr_channel->modes)
                    {
                        free (ptr_channel->modes);
                        ptr_channel->modes = NULL;
                    }
                }
                //gui_status_draw (ptr_channel->buffer, 1);
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_327: '327' command (whois, host)
 */

int
irc_protocol_cmd_327 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_host1, *pos_host2, *pos_other;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        pos_nick = strchr (arguments, ' ');
        if (pos_nick)
        {
            while (pos_nick[0] == ' ')
                pos_nick++;
            pos_host1 = strchr (pos_nick, ' ');
            if (pos_host1)
            {
                pos_host1[0] = '\0';
                pos_host1++;
                while (pos_host1[0] == ' ')
                    pos_host1++;
                pos_host2 = strchr (pos_host1, ' ');
                if (pos_host2)
                {
                    pos_host2[0] = '\0';
                    pos_host2++;
                    while (pos_host2[0] == ' ')
                        pos_host2++;

                    pos_other = strchr (pos_host2, ' ');
                    if (pos_other)
                    {
                        pos_other[0] = '\0';
                        pos_other++;
                        while (pos_other[0] == ' ')
                            pos_other++;
                    }
                    
                    weechat_printf (server->buffer,
                                    "%s%s[%s%s%s] %s%s %s %s%s%s%s%s%s",
                                    weechat_prefix ("network"),
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    IRC_COLOR_CHAT_NICK,
                                    pos_nick,
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    IRC_COLOR_CHAT_HOST,
                                    pos_host1,
                                    pos_host2,
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    (pos_other) ? "(" : "",
                                    IRC_COLOR_CHAT,
                                    (pos_other) ? pos_other : "",
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    (pos_other) ? ")" : "");
                }
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_329: '329' command received (channel creation date)
 */

int
irc_protocol_cmd_329 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    struct t_irc_channel *ptr_channel;
    time_t datetime;
    
    /* 329 message looks like:
       :server 329 mynick #channel 1205327894
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    /* make C compiler happy */
    (void) highlight;
    
    ptr_channel = irc_channel_search (server, argv[3]);
    
    if (!ignore)
    {
        datetime = (time_t)(atol ((argv_eol[4][0] == ':') ?
                                  argv_eol[4] + 1 : argv_eol[4]));
        
        if (ptr_channel)
        {
            if (ptr_channel->display_creation_date)
            {
                weechat_printf (ptr_channel->buffer,
                                _("%sChannel created on %s"),
                                weechat_prefix ("network"),
                                ctime (&datetime));
                ptr_channel->display_creation_date = 0;
            }
        }
        else
        {
            weechat_printf (server->buffer,
                            _("%sChannel %s%s%s created on %s"),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_CHANNEL,
                            argv[3],
                            IRC_COLOR_CHAT,
                            ctime (&datetime));
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_331: '331' command received (no topic for channel)
 */

int
irc_protocol_cmd_331 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    struct t_irc_channel *ptr_channel;
    
    /* 331 message looks like:
       :server 331 mynick #channel :There isn't a topic.
    */
    
    IRC_PROTOCOL_MIN_ARGS(4);
    
    /* make C compiler happy */
    (void) argv_eol;
    (void) highlight;
    
    if (!ignore)
    {
        ptr_channel = irc_channel_search (server, argv[3]);
        weechat_printf ((ptr_channel) ?
                        ptr_channel->buffer : server->buffer,
                        _("%sNo topic set for channel %s%s"),
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_CHANNEL,
                        argv[3]);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_332: '332' command received (topic of channel)
 */

int
irc_protocol_cmd_332 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    char *pos_topic;
    struct t_irc_channel *ptr_channel;
    
    /* 332 message looks like:
       :server 332 mynick #channel :topic of channel
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    /* make C compiler happy */
    (void) highlight;
    
    pos_topic = (argv_eol[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4];
    
    ptr_channel = irc_channel_search (server, argv[3]);
    
    if (ptr_channel && ptr_channel->nicks)
    {
        if (ptr_channel->topic)
            free (ptr_channel->topic);
        ptr_channel->topic = strdup (pos_topic);
        weechat_buffer_set (ptr_channel->buffer, "title", ptr_channel->topic);
    }
    
    if (!ignore)
    {
        weechat_printf ((ptr_channel && ptr_channel->nicks) ?
                        ptr_channel->buffer : server->buffer,
                        _("%sTopic for %s%s%s is: \"%s%s\""),
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_CHANNEL,
                        argv[3],
                        IRC_COLOR_CHAT,
                        pos_topic,
                        IRC_COLOR_CHAT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_333: '333' command received (infos about topic (nick / date))
 */

int
irc_protocol_cmd_333 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    struct t_irc_channel *ptr_channel;
    time_t datetime;
    
    /* 333 message looks like:
       :server 333 mynick #channel nick 1205428096
    */
    
    IRC_PROTOCOL_MIN_ARGS(6);
    
    /* make C compiler happy */
    (void) highlight;
    
    if (!ignore)
    {
        ptr_channel = irc_channel_search (server, argv[3]);
        datetime = (time_t)(atol ((argv_eol[5][0] == ':') ?
                                  argv_eol[5] + 1 : argv_eol[5]));
        if (ptr_channel && ptr_channel->nicks)
        {
            weechat_printf (ptr_channel->buffer,
                            _("%sTopic set by %s%s%s on %s"),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_NICK,
                            argv[4],
                            IRC_COLOR_CHAT,
                            ctime (&datetime));
        }
        else
        {
            weechat_printf (server->buffer,
                            _("%sTopic for %s%s%s set by %s%s%s on %s"),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_CHANNEL,
                            argv[3],
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_NICK,
                            argv[4],
                            IRC_COLOR_CHAT,
                            ctime (&datetime));
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_338: '338' command (whois, host)
 */

int
irc_protocol_cmd_338 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_host, *pos_message;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        pos_nick = strchr (arguments, ' ');
        if (pos_nick)
        {
            while (pos_nick[0] == ' ')
                pos_nick++;
            pos_host = strchr (pos_nick, ' ');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                while (pos_host[0] == ' ')
                    pos_host++;
                pos_message = strchr (pos_host, ' ');
                if (pos_message)
                {
                    pos_message[0] = '\0';
                    pos_message++;
                    while (pos_message[0] == ' ')
                        pos_message++;
                    if (pos_message[0] == ':')
                        pos_message++;
                    
                    weechat_printf (server->buffer,
                                    "%s%s[%s%s%s] %s%s %s%s %s%s",
                                    weechat_prefix ("network"),
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    IRC_COLOR_CHAT_NICK,
                                    pos_nick,
                                    IRC_COLOR_CHAT_DELIMITERS,
                                    IRC_COLOR_CHAT_NICK,
                                    pos_nick,
                                    IRC_COLOR_CHAT,
                                    pos_message,
                                    IRC_COLOR_CHAT_HOST,
                                    pos_host);
                }
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_341: '341' command received (inviting)
 */

int
irc_protocol_cmd_341 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_channel;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos_nick = strchr (arguments, ' ');
    if (pos_nick)
    {
        pos_nick[0] = '\0';
        pos_nick++;
        while (pos_nick[0] == ' ')
            pos_nick++;
        
        pos_channel = strchr (pos_nick, ' ');
        if (pos_channel)
        {
            if (!ignore)
            {
                pos_channel[0] = '\0';
                pos_channel++;
                while (pos_channel[0] == ' ')
                    pos_channel++;
                if (pos_channel[0] == ':')
                    pos_channel++;
                
                weechat_printf (server->buffer,
                                _("%s%s%s%s has invited %s%s%s on %s%s"),
                                weechat_prefix ("network"),
                                IRC_COLOR_CHAT_NICK,
                                arguments,
                                IRC_COLOR_CHAT,
                                IRC_COLOR_CHAT_NICK,
                                pos_nick,
                                IRC_COLOR_CHAT,
                                IRC_COLOR_CHAT_CHANNEL,
                                pos_channel);
            }
        }
        else
        {
            weechat_printf (server->buffer,
                            _("%s%s: cannot identify channel for \"%s\" "
                              "command"),
                            weechat_prefix ("error"), "irc", "341");
            return WEECHAT_RC_ERROR;
        }
    }
    else
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot identify nickname for \"%s\" "
                          "command"),
                        weechat_prefix ("error"), "irc", "341");
        return WEECHAT_RC_ERROR;
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_344: '344' command (channel reop)
 */

int
irc_protocol_cmd_344 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_host;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;

    if (!ignore)
    {
        pos_channel = strchr (arguments, ' ');
        if (pos_channel)
        {
            while (pos_channel[0] == ' ')
                pos_channel++;
            pos_host = strchr (pos_channel, ' ');
            if (pos_host)
            {
                pos_host[0] = '\0';
                pos_host++;
                while (pos_host[0] == ' ')
                    pos_host++;
                
                weechat_printf (server->buffer,
                                _("%sChannel reop %s%s%s: %s%s"),
                                weechat_prefix ("network"),
                                IRC_COLOR_CHAT_CHANNEL,
                                pos_channel,
                                IRC_COLOR_CHAT,
                                IRC_COLOR_CHAT_HOST,
                                pos_host);
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_345: '345' command (end of channel reop)
 */

int
irc_protocol_cmd_345 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    /* skip nickname if at beginning of server message */
    if (strncmp (server->nick, arguments, strlen (server->nick)) == 0)
    {
        arguments += strlen (server->nick) + 1;
        while (arguments[0] == ' ')
            arguments++;
    }
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
        if (!ignore)
        {
            weechat_printf (server->buffer,
                            "%s%s%s %s%s",
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_CHANNEL,
                            arguments,
                            IRC_COLOR_CHAT,
                            pos);
        }
    }
    else
    {
        if (!ignore)
        {
            weechat_printf (server->buffer,
                            "%s%s",
                            weechat_prefix ("network"),
                            arguments);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_348: '348' command received (channel exception list)
 */

int
irc_protocol_cmd_348 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_exception, *pos_user, *pos_date, *pos;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *buffer;
    time_t datetime;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    /* look for channel */
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot parse \"%s\" command"),
                        weechat_prefix ("error"), "irc", "348");
        return WEECHAT_RC_ERROR;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    /* look for exception mask */
    pos_exception = strchr (pos_channel, ' ');
    if (!pos_exception)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot parse \"%s\" command"),
                        weechat_prefix ("error"), "irc", "348");
        return WEECHAT_RC_ERROR;
    }
    pos_exception[0] = '\0';
    pos_exception++;
    while (pos_exception[0] == ' ')
        pos_exception++;
    
    /* look for user who set exception */
    pos_user = strchr (pos_exception, ' ');
    if (pos_user)
    {
        pos_user[0] = '\0';
        pos_user++;
        while (pos_user[0] == ' ')
            pos_user++;
        
        /* look for date/time */
        pos_date = strchr (pos_user, ' ');
        if (pos_date)
        {
            pos_date[0] = '\0';
            pos_date++;
            while (pos_date[0] == ' ')
                pos_date++;
        }
    }
    else
        pos_date = NULL;
    
    ptr_channel = irc_channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!ignore)
    {
        weechat_printf (buffer,
                        _("%s%s[%s%s%s]%s exception %s%s%s"),
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT_CHANNEL,
                        pos_channel,
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT,
                        IRC_COLOR_CHAT_HOST,
                        pos_exception,
                        IRC_COLOR_CHAT);
        if (pos_user)
        {
            pos = strchr (pos_user, '!');
            if (pos)
            {
                pos[0] = '\0';
                weechat_printf (buffer,
                                _(" by %s%s %s(%s%s%s)"),
                                IRC_COLOR_CHAT_NICK,
                                pos_user,
                                IRC_COLOR_CHAT_DELIMITERS,
                                IRC_COLOR_CHAT_HOST,
                                pos + 1,
                                IRC_COLOR_CHAT_DELIMITERS);
            }
            else
                weechat_printf (buffer,
                                _(" by %s%s"),
                                IRC_COLOR_CHAT_NICK,
                                pos_user);
        }
        if (pos_date)
        {
            datetime = (time_t)(atol (pos_date));
            weechat_printf (buffer,
                            "%s, %s",
                            IRC_COLOR_CHAT,
                            ctime (&datetime));
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_349: '349' command received (end of channel exception list)
 */

int
irc_protocol_cmd_349 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_msg;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *buffer;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot parse \"%s\" command"),
                        weechat_prefix ("error"), "irc", "349");
        return WEECHAT_RC_ERROR;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    pos_msg = strchr (pos_channel, ' ');
    if (!pos_msg)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot parse \"%s\" command"),
                        weechat_prefix ("error"), "irc", "349");
        return WEECHAT_RC_ERROR;
    }
    pos_msg[0] = '\0';
    pos_msg++;
    while (pos_msg[0] == ' ')
        pos_msg++;
    if (pos_msg[0] == ':')
        pos_msg++;
    
    ptr_channel = irc_channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!ignore)
    {
        weechat_printf (buffer,
                        "%s%s[%s%s%s] %s%s",
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT_CHANNEL,
                        pos_channel,
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT,
                        pos_msg);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_351: '351' command received (server version)
 */

int
irc_protocol_cmd_351 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos, *pos2;
    
    /* make C compiler happy */
    (void) server;
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos = strchr (arguments, ' ');
    if (pos)
    {
        pos[0] = '\0';
        pos++;
    }
    else
        pos = arguments;
    
    pos2 = strstr (pos, " :");
    if (pos2)
    {
        pos2[0] = '\0';
        pos2 += 2;
    }
    
    if (!ignore)
    {
        if (pos2)
            weechat_printf (server->buffer,
                            "%s%s %s",
                            weechat_prefix ("network"),
                            pos,
                            pos2);
        else
            weechat_printf (server->buffer,
                            "%s%s",
                            weechat_prefix ("network"),
                            pos);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_352: '352' command (who)
 */

int
irc_protocol_cmd_352 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    char *pos_attr, *pos_hopcount, *pos_realname;
    int arg_start, length;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    
    /* 352 message looks like:
       :server 352 mynick #channel user host server nick (*) (H/G) :0 flashcode
    */
    
    IRC_PROTOCOL_MIN_ARGS(9);
    
    /* make C compiler happy */
    (void) highlight;

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
                ptr_nick->host = (char *)malloc (length * sizeof (char));
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
        if (!ignore)
        {
            weechat_printf (server->buffer,
                            "%s%s[%s%s%s] %s%s%s(%s%s@%s%s)%s "
                            "%s%s%s%s(%s)",
                            weechat_prefix ("network"),
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
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_353: '353' command received (list of users on a channel)
 */

int
irc_protocol_cmd_353 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
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
    
    /* make C compiler happy */
    (void) highlight;
    
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
                                weechat_prefix ("error"), "irc",
                                pos_nick, ptr_channel->name);
            }
        }
    }
    
    if (!ignore)
    {
        weechat_printf ((ptr_channel && ptr_channel->nicks) ?
                        ptr_channel->buffer : server->buffer,
                        _("%sNicks %s%s%s: %s[%s%s%s]"),
                        weechat_prefix ("network"),
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
irc_protocol_cmd_366 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    struct t_irc_channel *ptr_channel;
    int num_nicks, num_op, num_halfop, num_voice, num_normal;
    
    /* 366 message looks like:
       :server 366 mynick #channel :End of /NAMES list.
    */
    
    IRC_PROTOCOL_MIN_ARGS(5);
    
    /* make C compiler happy */
    (void) highlight;
    
    ptr_channel = irc_channel_search (server, argv[3]);
    if (ptr_channel && ptr_channel->nicks)
    {
        if (!ignore)
        {
            /* display number of nicks, ops, halfops & voices on the channel */
            irc_nick_count (ptr_channel, &num_nicks, &num_op, &num_halfop,
                            &num_voice, &num_normal);
            weechat_printf (ptr_channel->buffer,
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
        }
        irc_command_mode_server (server, ptr_channel->name);
        irc_channel_check_away (server, ptr_channel, 1);
    }
    else
    {
        if (!ignore)
        {
            weechat_printf (server->buffer,
                            "%s%s%s%s: %s",
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_CHANNEL,
                            argv[3],
                            IRC_COLOR_CHAT,
                            (argv[4][0] == ':') ? argv_eol[4] + 1 : argv_eol[4]);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_367: '367' command received (banlist)
 */

int
irc_protocol_cmd_367 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_ban, *pos_user, *pos_date, *pos;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *buffer;
    time_t datetime;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    /* look for channel */
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot parse \"%s\" command"),
                        weechat_prefix ("error"), "irc", "367");
        return WEECHAT_RC_ERROR;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    /* look for ban mask */
    pos_ban = strchr (pos_channel, ' ');
    if (!pos_ban)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot parse \"%s\" command"),
                        weechat_prefix ("error"), "irc", "367");
        return WEECHAT_RC_ERROR;
    }
    pos_ban[0] = '\0';
    pos_ban++;
    while (pos_ban[0] == ' ')
        pos_ban++;
    
    /* look for user who set ban */
    pos_date = NULL;
    pos_user = strchr (pos_ban, ' ');
    if (pos_user)
    {
        pos_user[0] = '\0';
        pos_user++;
        while (pos_user[0] == ' ')
            pos_user++;
        
        /* look for date/time */
        pos_date = strchr (pos_user, ' ');
        if (pos_date)
        {
            pos_date[0] = '\0';
            pos_date++;
            while (pos_date[0] == ' ')
                pos_date++;   
        }
    }
    
    ptr_channel = irc_channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!ignore)
    {
        if (pos_user)
        {
            weechat_printf (buffer,
                            _("%s%s[%s%s%s] %s%s%s banned by "),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT_CHANNEL,
                            pos_channel,
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT_HOST,
                            pos_ban,
                            IRC_COLOR_CHAT);
            pos = strchr (pos_user, '!');
            if (pos)
            {
                pos[0] = '\0';
                weechat_printf (buffer,
                                "%s%s %s(%s%s%s)",
                                IRC_COLOR_CHAT_NICK,
                                pos_user,
                                IRC_COLOR_CHAT_DELIMITERS,
                                IRC_COLOR_CHAT_HOST,
                                pos + 1,
                                IRC_COLOR_CHAT_DELIMITERS);
            }
            else
            {
                weechat_printf (buffer,
                                "%s%s",
                                IRC_COLOR_CHAT_NICK,
                                pos_user);
            }
            if (pos_date)
            {
                datetime = (time_t)(atol (pos_date));
                weechat_printf (buffer,
                                "%s, %s",
                                IRC_COLOR_CHAT,
                                ctime (&datetime));
            }
        }
        else
            weechat_printf (buffer,
                            _("%s%s[%s%s%s] %s%s%s banned"),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT_CHANNEL,
                            pos_channel,
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT_HOST,
                            pos_ban,
                            IRC_COLOR_CHAT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_368: '368' command received (end of banlist)
 */

int
irc_protocol_cmd_368 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_channel, *pos_msg;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *buffer;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    pos_channel = strchr (arguments, ' ');
    if (!pos_channel)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot parse \"%s\" command"),
                        weechat_prefix ("error"), "irc", "368");
        return WEECHAT_RC_ERROR;
    }
    pos_channel[0] = '\0';
    pos_channel++;
    while (pos_channel[0] == ' ')
        pos_channel++;
    
    pos_msg = strchr (pos_channel, ' ');
    if (!pos_msg)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot parse \"%s\" command"),
                        weechat_prefix ("error"), "irc", "368");
        return WEECHAT_RC_ERROR;
    }
    pos_msg[0] = '\0';
    pos_msg++;
    while (pos_msg[0] == ' ')
        pos_msg++;
    if (pos_msg[0] == ':')
        pos_msg++;
    
    ptr_channel = irc_channel_search (server, pos_channel);
    buffer = (ptr_channel) ? ptr_channel->buffer : server->buffer;
    
    if (!ignore)
    {
        weechat_printf (buffer,
                        "%s%s[%s%s%s] %s%s",
                        weechat_prefix ("network"),
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT_CHANNEL,
                        pos_channel,
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT,
                        pos_msg);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_432: '432' command received (erroneous nickname)
 */

int
irc_protocol_cmd_432 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    /* Note: this IRC command can not be ignored */
    
    int i, nick_found, nick_to_use;
    
    irc_protocol_cmd_error (server, command, argc, argv, argv_eol, ignore,
                            highlight);
    
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
                            weechat_prefix ("error"), "irc");
            irc_server_disconnect (server, 1);
            return WEECHAT_RC_OK;
        }
        
        weechat_printf (server->buffer,
                        _("%s%s: nickname \"%s\" is invalid, "
                          "trying nickname #%d (\"%s\")"),
                        weechat_prefix ("error"), "irc", server->nick,
                        nick_to_use + 1, server->nicks_array[nick_to_use]);
        
        irc_server_set_nick (server, server->nicks_array[nick_to_use]);
        
        irc_server_sendf (server, "NICK %s", server->nick);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_433: '433' command received (nickname already in use)
 */

int
irc_protocol_cmd_433 (struct t_irc_server *server, char *command,
                      int argc, char **argv, char **argv_eol,
                      int ignore, int highlight)
{
    /* Note: this IRC command can not be ignored */
    
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
                            weechat_prefix ("error"), "irc");
            irc_server_disconnect (server, 1);
            return WEECHAT_RC_OK;
        }
        
        weechat_printf (server->buffer,
                        _("%s%s: nickname \"%s\" is already in use, "
                          "trying nickname #%d (\"%s\")"),
                        weechat_prefix ("info"), "irc", server->nick,
                        nick_to_use + 1, server->nicks_array[nick_to_use]);
        
        irc_server_set_nick (server, server->nicks_array[nick_to_use]);
        
        irc_server_sendf (server, "NICK %s", server->nick);
    }
    else
    {
        return irc_protocol_cmd_error (server, command, argc, argv, argv_eol,
                                       ignore, highlight);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_438: '438' command received (not authorized to change nickname)
 */

int
irc_protocol_cmd_438 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos, *pos2;
    
    /* make C compiler happy */
    (void) server;
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        pos = strchr (arguments, ' ');
        if (pos)
        {
            pos[0] = '\0';
            pos++;
            
            pos2 = strstr (pos, " :");
            if (pos2)
            {
                pos2[0] = '\0';
                pos2 += 2;
                weechat_printf (server->buffer,
                                "%s%s (%s => %s)",
                                weechat_prefix ("network"),
                                pos2,
                                arguments,
                                pos);
            }
            else
                weechat_printf (server->buffer,
                                "%s%s (%s)",
                                weechat_prefix ("network"),
                                pos,
                                arguments);
        }
        else
        {
            weechat_printf (server->buffer,
                            "%s%s",
                            weechat_prefix ("network"),
                            arguments);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_cmd_671: '671' command (whois, secure connection)
 */

int
irc_protocol_cmd_671 (struct t_irc_server *server, char *irc_message, char *host,
                      char *nick, char *arguments, int ignore, int highlight)
{
    char *pos_nick, *pos_message;
    
    /* make C compiler happy */
    (void) irc_message;
    (void) host;
    (void) nick;
    (void) highlight;
    
    if (!ignore)
    {
        pos_nick = strchr (arguments, ' ');
        if (pos_nick)
        {
            while (pos_nick[0] == ' ')
                pos_nick++;
            pos_message = strchr (pos_nick, ' ');
            if (pos_message)
            {
                pos_message[0] = '\0';
                pos_message++;
                while (pos_message[0] == ' ')
                    pos_message++;
                if (pos_message[0] == ':')
                    pos_message++;
                
                weechat_printf (server->buffer,
                                "%s%s[%s%s%s] %s%s",
                                weechat_prefix ("network"),
                                IRC_COLOR_CHAT_DELIMITERS,
                                IRC_COLOR_CHAT_NICK,
                                pos_nick,
                                IRC_COLOR_CHAT_DELIMITERS,
                                IRC_COLOR_CHAT,
                                pos_message);
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_protocol_is_numeric_command: return 1 if given string is 100% numeric
 */

int
irc_protocol_is_numeric_command (char *str)
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
irc_protocol_recv_command (struct t_irc_server *server, char *entire_line,
                           char *host, char *command, char *arguments)
{
    int i, cmd_found, return_code, ignore, highlight, argc;
    char *pos, *nick;
    char *dup_entire_line, *dup_host, *dup_arguments, *irc_message;
    t_irc_recv_func *cmd_recv_func;
    t_irc_recv_func2 *cmd_recv_func2;
    char *cmd_name, **argv, **argv_eol;
    struct t_irc_protocol_msg irc_protocol_messages[] =
        { { "error", N_("error received from IRC server"), NULL, &irc_protocol_cmd_error },
          { "invite", N_("invite a nick on a channel"), NULL, &irc_protocol_cmd_invite },
          { "join", N_("join a channel"), NULL, &irc_protocol_cmd_join },
          { "kick", N_("forcibly remove a user from a channel"), NULL, &irc_protocol_cmd_kick },
          { "kill", N_("close client-server connection"), NULL, &irc_protocol_cmd_kill },
          { "mode", N_("change channel or user mode"), NULL, &irc_protocol_cmd_mode },
          { "nick", N_("change current nickname"), NULL, &irc_protocol_cmd_nick },
          { "notice", N_("send notice message to user"), NULL, &irc_protocol_cmd_notice },
          { "part", N_("leave a channel"), NULL, &irc_protocol_cmd_part },
          { "ping", N_("ping server"), NULL, &irc_protocol_cmd_ping },
          { "pong", N_("answer to a ping message"), NULL, &irc_protocol_cmd_pong },
          { "privmsg", N_("message received"), NULL, &irc_protocol_cmd_privmsg },
          { "quit", N_("close all connections and quit"), NULL, &irc_protocol_cmd_quit },
          { "topic", N_("get/set channel topic"), NULL, &irc_protocol_cmd_topic },
          { "wallops", N_("send a message to all currently connected users who have "
                          "set the 'w' user mode "
                          "for themselves"), NULL, &irc_protocol_cmd_wallops },
          { "001", N_("a server message"), NULL, &irc_protocol_cmd_001 },
          { "005", N_("a server message"), NULL, &irc_protocol_cmd_005 },
          { "221", N_("user mode string"), NULL, &irc_protocol_cmd_221 },
          { "301", N_("away message"), NULL, &irc_protocol_cmd_301 },
          { "303", N_("ison"), NULL, &irc_protocol_cmd_303 },
          { "305", N_("unaway"), NULL, &irc_protocol_cmd_305 },
          { "306", N_("now away"), NULL, &irc_protocol_cmd_306 },
          { "307", N_("whois (registered nick)"), NULL, &irc_protocol_cmd_whois_nick_msg },
          { "310", N_("whois (help mode)"), NULL, &irc_protocol_cmd_whois_nick_msg },
          { "311", N_("whois (user)"), NULL, &irc_protocol_cmd_311 },
          { "312", N_("whois (server)"), NULL, &irc_protocol_cmd_312 },
          { "313", N_("whois (operator)"), NULL, &irc_protocol_cmd_whois_nick_msg },
          { "314", N_("whowas"), NULL, &irc_protocol_cmd_314 },
          { "315", N_("end of /who list"), NULL, &irc_protocol_cmd_315 },
          { "317", N_("whois (idle)"), NULL, &irc_protocol_cmd_317 },
          { "318", N_("whois (end)"), NULL, &irc_protocol_cmd_whois_nick_msg },
          { "319", N_("whois (channels)"), NULL, &irc_protocol_cmd_whois_nick_msg },
          { "320", N_("whois (identified user)"), NULL, &irc_protocol_cmd_whois_nick_msg },
          { "321", N_("/list start"), NULL, &irc_protocol_cmd_321 },  
          { "322", N_("channel (for /list)"), NULL, &irc_protocol_cmd_322 },
          { "323", N_("end of /list"), NULL, &irc_protocol_cmd_323 },
          { "324", N_("channel mode"), &irc_protocol_cmd_324, NULL },
          { "326", N_("whois (has oper privs)"), NULL, &irc_protocol_cmd_whois_nick_msg },
          { "327", N_("whois (host)"), &irc_protocol_cmd_327, NULL },
          { "329", N_("channel creation date"), NULL, &irc_protocol_cmd_329 },
          { "331", N_("no topic for channel"), NULL, &irc_protocol_cmd_331 },
          { "332", N_("topic of channel"), NULL, &irc_protocol_cmd_332 },
          { "333", N_("infos about topic (nick and date changed)"), NULL, &irc_protocol_cmd_333 },
          { "338", N_("whois (host)"), &irc_protocol_cmd_338, NULL },
          { "341", N_("inviting"), &irc_protocol_cmd_341, NULL },
          { "344", N_("channel reop"), &irc_protocol_cmd_344, NULL },
          { "345", N_("end of channel reop list"), &irc_protocol_cmd_345, NULL },
          { "348", N_("channel exception list"), &irc_protocol_cmd_348, NULL },
          { "349", N_("end of channel exception list"), &irc_protocol_cmd_349, NULL },
          { "351", N_("server version"), &irc_protocol_cmd_351, NULL },
          { "352", N_("who"), NULL, &irc_protocol_cmd_352 },
          { "353", N_("list of nicks on channel"), NULL, &irc_protocol_cmd_353 },
          { "366", N_("end of /names list"), NULL, &irc_protocol_cmd_366 },
          { "367", N_("banlist"), &irc_protocol_cmd_367, NULL },
          { "368", N_("end of banlist"), &irc_protocol_cmd_368, NULL },
          { "378", N_("whois (connecting from)"), NULL, &irc_protocol_cmd_whois_nick_msg },
          { "379", N_("whois (using modes)"), NULL, &irc_protocol_cmd_whois_nick_msg },
          { "401", N_("no such nick/channel"), NULL, &irc_protocol_cmd_error },
          { "402", N_("no such server"), NULL, &irc_protocol_cmd_error },
          { "403", N_("no such channel"), NULL, &irc_protocol_cmd_error },
          { "404", N_("cannot send to channel"), NULL, &irc_protocol_cmd_error },
          { "405", N_("too many channels"), NULL, &irc_protocol_cmd_error },
          { "406", N_("was no such nick"), NULL, &irc_protocol_cmd_error },
          { "407", N_("was no such nick"), NULL, &irc_protocol_cmd_error },
          { "409", N_("no origin"), NULL, &irc_protocol_cmd_error },
          { "410", N_("no services"), NULL, &irc_protocol_cmd_error },
          { "411", N_("no recipient"), NULL, &irc_protocol_cmd_error },
          { "412", N_("no text to send"), NULL, &irc_protocol_cmd_error },
          { "413", N_("no toplevel"), NULL, &irc_protocol_cmd_error },
          { "414", N_("wilcard in toplevel domain"), NULL, &irc_protocol_cmd_error },
          { "421", N_("unknown command"), NULL, &irc_protocol_cmd_error },
          { "422", N_("MOTD is missing"), NULL, &irc_protocol_cmd_error },
          { "423", N_("no administrative info"), NULL, &irc_protocol_cmd_error },
          { "424", N_("file error"), NULL, &irc_protocol_cmd_error },
          { "431", N_("no nickname given"), NULL, &irc_protocol_cmd_error },
          { "432", N_("erroneous nickname"), NULL, &irc_protocol_cmd_432 },
          { "433", N_("nickname already in use"), NULL, &irc_protocol_cmd_433 },
          { "436", N_("nickname collision"), NULL, &irc_protocol_cmd_error },
          { "437", N_("resource unavailable"), NULL, &irc_protocol_cmd_error },
          { "438", N_("not authorized to change nickname"), &irc_protocol_cmd_438, NULL },
          { "441", N_("user not in channel"), NULL, &irc_protocol_cmd_error },
          { "442", N_("not on channel"), NULL, &irc_protocol_cmd_error },
          { "443", N_("user already on channel"), NULL, &irc_protocol_cmd_error },
          { "444", N_("user not logged in"), NULL, &irc_protocol_cmd_error },
          { "445", N_("summon has been disabled"), NULL, &irc_protocol_cmd_error },
          { "446", N_("users has been disabled"), NULL, &irc_protocol_cmd_error },
          { "451", N_("you are not registered"), NULL, &irc_protocol_cmd_error },
          { "461", N_("not enough parameters"), NULL, &irc_protocol_cmd_error },
          { "462", N_("you may not register"), NULL, &irc_protocol_cmd_error },
          { "463", N_("your host isn't among the privileged"), NULL, &irc_protocol_cmd_error },
          { "464", N_("password incorrect"), NULL, &irc_protocol_cmd_error },
          { "465", N_("you are banned from this server"), NULL, &irc_protocol_cmd_error },
          { "467", N_("channel key already set"), NULL, &irc_protocol_cmd_error },
          { "470", N_("forwarding to another channel"), NULL, &irc_protocol_cmd_error },
          { "471", N_("channel is already full"), NULL, &irc_protocol_cmd_error },
          { "472", N_("unknown mode char to me"), NULL, &irc_protocol_cmd_error },
          { "473", N_("cannot join channel (invite only)"), NULL, &irc_protocol_cmd_error },
          { "474", N_("cannot join channel (banned from channel)"), NULL, &irc_protocol_cmd_error },
          { "475", N_("cannot join channel (bad channel key)"), NULL, &irc_protocol_cmd_error },
          { "476", N_("bad channel mask"), NULL, &irc_protocol_cmd_error },
          { "477", N_("channel doesn't support modes"), NULL, &irc_protocol_cmd_error },
          { "481", N_("you're not an IRC operator"), NULL, &irc_protocol_cmd_error },
          { "482", N_("you're not channel operator"), NULL, &irc_protocol_cmd_error },
          { "483", N_("you can't kill a server!"), NULL, &irc_protocol_cmd_error },
          { "484", N_("your connection is restricted!"), NULL, &irc_protocol_cmd_error },
          { "485", N_("user is immune from kick/deop"), NULL, &irc_protocol_cmd_error },
          { "487", N_("network split"), NULL, &irc_protocol_cmd_error },
          { "491", N_("no O-lines for your host"), NULL, &irc_protocol_cmd_error },
          { "501", N_("unknown mode flag"), NULL, &irc_protocol_cmd_error },
          { "502", N_("can't change mode for other users"), NULL, &irc_protocol_cmd_error },
          { "671", N_("whois (secure connection)"), &irc_protocol_cmd_671, NULL },
          { "973", N_("whois (secure connection)"), NULL, &irc_protocol_cmd_server_mode_reason },
          { "974", N_("whois (secure connection)"), NULL, &irc_protocol_cmd_server_mode_reason },
          { "975", N_("whois (secure connection)"), NULL, &irc_protocol_cmd_server_mode_reason },
          { NULL, NULL, NULL, NULL }
        };
    
    if (!command)
        return;
    
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
            cmd_recv_func = NULL;
            cmd_recv_func2 = irc_protocol_cmd_numeric;
        }
        else
        {
            weechat_printf (server->buffer,
                            _("%s%s: command \"%s\" not found:"),
                            weechat_prefix ("error"), "irc", command);
            weechat_printf (server->buffer,
                            "%s%s",
                            weechat_prefix ("error"), entire_line);
            return;
        }
    }
    else
    {
        cmd_name = irc_protocol_messages[cmd_found].name;
        cmd_recv_func = irc_protocol_messages[cmd_found].recv_function;
        cmd_recv_func2 = irc_protocol_messages[cmd_found].recv_function2;
    }
    
    if ((cmd_recv_func != NULL) || (cmd_recv_func2 != NULL))
    {
        argv = weechat_string_explode (entire_line, " ", 0, 0, &argc);
        argv_eol = weechat_string_explode (entire_line, " ", 1, 0, NULL);
        dup_entire_line = (entire_line) ? strdup (entire_line) : NULL;
        dup_host = (host) ? strdup (host) : NULL;
        dup_arguments = (arguments) ? strdup (arguments) : NULL;
        
        ignore = 0;
        highlight = 0;
        
        //return_code = plugin_msg_handler_exec (server->name,
        //                                       cmd_name,
        //                                       dup_entire_line);
        /* plugin handler choosed to discard message for WeeChat,
           so we ignore this message in standard handler */
        //if (return_code & PLUGIN_RC_OK_IGNORE_WEECHAT)
        //    ignore = 1;
        /* plugin asked for highlight ? */
        //if (return_code & PLUGIN_RC_OK_WITH_HIGHLIGHT)
        //    highlight = 1;
        
        pos = (dup_host) ? strchr (dup_host, '!') : NULL;
        if (pos)
            pos[0] = '\0';
        nick = (dup_host) ? strdup (dup_host) : NULL;
        if (pos)
            pos[0] = '!';
        irc_message = strdup (dup_entire_line);

        if (cmd_recv_func2 != NULL)
        {
            return_code = (int) (cmd_recv_func2) (server, cmd_name,
                                                  argc, argv, argv_eol,
                                                  ignore, highlight);
        }
        else
        {
            return_code = (int) (cmd_recv_func) (server, irc_message,
                                                 dup_host, nick,
                                                 dup_arguments,
                                                 ignore, highlight);
        }

        if (return_code == WEECHAT_RC_ERROR)
        {
            weechat_printf (server->buffer,
                            _("%s%s: failed to parse command \"%s\" (please "
                              "report to developers):"),
                            weechat_prefix ("error"), "irc", command);
            weechat_printf (server->buffer,
                            "%s%s",
                            weechat_prefix ("error"), entire_line);
        }
        
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
