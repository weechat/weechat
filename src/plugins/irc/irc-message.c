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
 * irc-message.c: functions for IRC messages
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-server.h"
#include "irc-channel.h"


/*
 * irc_message_parse: parse IRC message and return pointer to host, command,
 *                    channel, target nick and arguments (if any)
 */

void
irc_message_parse (const char *message, char **nick, char **host,
                   char **command, char **channel, char **arguments)
{
    const char *pos, *pos2, *pos3, *pos4, *pos5;
    
    if (nick)
        *nick = NULL;
    if (host)
        *host = NULL;
    if (command)
        *command = NULL;
    if (channel)
        *channel = NULL;
    if (arguments)
        *arguments = NULL;
    
    if (!message)
        return;
    
    /*
     * we will use this message as example:
     *   :FlashCode!n=FlashCod@host.com PRIVMSG #channel :hello!
     */
    if (message[0] == ':')
    {
        pos2 = strchr (message, '!');
        pos = strchr (message, ' ');
        if (pos2 && (!pos || pos > pos2))
        {
            if (nick)
                *nick = weechat_strndup (message + 1, pos2 - (message + 1));
        }
        else if (pos)
        {
            if (nick)
                *nick = weechat_strndup (message + 1, pos - (message + 1));
        }
        if (pos)
        {
            if (host)
                *host = weechat_strndup (message + 1, pos - (message + 1));
            pos++;
        }
        else
            pos = message;
    }
    else
        pos = message;

    /* pos is pointer on PRIVMSG #channel :hello! */
    if (pos && pos[0])
    {
        while (pos[0] == ' ')
        {
            pos++;
        }
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            /* pos2 is pointer on #channel :hello! */
            if (command)
                *command = weechat_strndup (pos, pos2 - pos);
            pos2++;
            while (pos2[0] == ' ')
            {
                pos2++;
            }
            if (arguments)
                *arguments = strdup (pos2);
            if (pos2[0] != ':')
            {
                if (irc_channel_is_channel (pos2))
                {
                    pos3 = strchr (pos2, ' ');
                    if (channel)
                    {
                        if (pos3)
                            *channel = weechat_strndup (pos2, pos3 - pos2);
                        else
                            *channel = strdup (pos2);
                    }
                }
                else
                {
                    pos3 = strchr (pos2, ' ');
                    if (nick && !*nick)
                    {
                        if (pos3)
                            *nick = weechat_strndup (pos2, pos3 - pos2);
                        else
                            *nick = strdup (pos2);
                    }
                    if (pos3)
                    {
                        pos4 = pos3;
                        pos3++;
                        while (pos3[0] == ' ')
                        {
                            pos3++;
                        }
                        if (irc_channel_is_channel (pos3))
                        {
                            pos5 = strchr (pos3, ' ');
                            if (channel)
                            {
                                if (pos5)
                                    *channel = weechat_strndup (pos3, pos5 - pos3);
                                else
                                    *channel = strdup (pos3);
                            }
                        }
                        else if (channel && !*channel)
                        {
                            *channel = weechat_strndup (pos2, pos4 - pos2);
                        }
                    }
                }
            }
        }
        else
        {
            if (command)
                *command = strdup (pos);
        }
    }
}

/*
 * irc_message_parse_to_hashtable: parse IRC message and return hashtable with
 *                                 keys: nick, host, command, channel, arguments
 *                                 Note: hashtable has to be free()
 *                                       after use
 */

struct t_hashtable *
irc_message_parse_to_hashtable (const char *message)
{
    char *nick, *host, *command, *channel, *arguments;
    char empty_str[1] = { '\0' };
    struct t_hashtable *hashtable;
    
    irc_message_parse (message, &nick, &host, &command, &channel, &arguments);
    
    hashtable = weechat_hashtable_new (8,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL,
                                       NULL);
    if (!hashtable)
        return NULL;
    
    weechat_hashtable_set (hashtable, "nick", (nick) ? nick : empty_str);
    weechat_hashtable_set (hashtable, "host", (host) ? host : empty_str);
    weechat_hashtable_set (hashtable, "command", (command) ? command : empty_str);
    weechat_hashtable_set (hashtable, "channel", (channel) ? channel : empty_str);
    weechat_hashtable_set (hashtable, "arguments", (arguments) ? arguments : empty_str);
    
    if (nick)
        free (nick);
    if (host)
        free (host);
    if (command)
        free (command);
    if (channel)
        free (channel);
    if (arguments)
        free (arguments);
    
    return hashtable;
}

/*
 * irc_message_get_nick_from_host: get nick from host in an IRC message
 */

const char *
irc_message_get_nick_from_host (const char *host)
{
    static char nick[128];
    char host2[128], *pos_space, *pos;
    const char *ptr_host;
    
    if (!host)
        return NULL;
    
    nick[0] = '\0';
    if (host)
    {
        ptr_host = host;
        pos_space = strchr (host, ' ');
        if (pos_space)
        {
            if (pos_space - host < (int)sizeof (host2))
            {
                strncpy (host2, host, pos_space - host);
                host2[pos_space - host] = '\0';
            }
            else
                snprintf (host2, sizeof (host2), "%s", host);
            ptr_host = host2;
        }
        
        if (ptr_host[0] == ':')
            ptr_host++;
        
        pos = strchr (ptr_host, '!');
        if (pos && (pos - ptr_host < (int)sizeof (nick)))
        {
            strncpy (nick, ptr_host, pos - ptr_host);
            nick[pos - ptr_host] = '\0';
        }
        else
        {
            snprintf (nick, sizeof (nick), "%s", ptr_host);
        }
    }
    
    return nick;
}

/*
 * irc_message_get_address_from_host: get address from host in an IRC message
 */

const char *
irc_message_get_address_from_host (const char *host)
{
    static char address[256];
    char host2[256], *pos_space, *pos;
    const char *ptr_host;
    
    address[0] = '\0';
    if (host)
    {
        ptr_host = host;
        pos_space = strchr (host, ' ');
        if (pos_space)
        {
            if (pos_space - host < (int)sizeof (host2))
            {
                strncpy (host2, host, pos_space - host);
                host2[pos_space - host] = '\0';
            }
            else
                snprintf (host2, sizeof (host2), "%s", host);
            ptr_host = host2;
        }
        
        if (ptr_host[0] == ':')
            ptr_host++;
        pos = strchr (ptr_host, '!');
        if (pos)
            snprintf (address, sizeof (address), "%s", pos + 1);
        else
            snprintf (address, sizeof (address), "%s", ptr_host);
    }
    
    return address;
}

/*
 * irc_message_replace_vars: replace special IRC vars ($nick, $channel,
 *                           $server) in a string
 *                           Note: result has to be free() after use
 */

char *
irc_message_replace_vars (struct t_irc_server *server,
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
