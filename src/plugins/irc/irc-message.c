/*
 * irc-message.c - functions for IRC messages
 *
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-ignore.h"
#include "irc-server.h"
#include "irc-tag.h"


/*
 * Parses an IRC message and returns:
 *   - tags (string)
 *   - message without tags (string)
 *   - nick (string)
 *   - host (string)
 *   - command (string)
 *   - channel (string)
 *   - arguments (string)
 *   - text (string)
 *   - pos_command (integer: command index in message)
 *   - pos_arguments (integer: arguments index in message)
 *   - pos_channel (integer: channel index in message)
 *   - pos_text (integer: text index in message)
 *
 * Example:
 *   @time=2015-06-27T16:40:35.000Z :nick!user@host PRIVMSG #weechat :hello!
 *
 * Result:
 *               tags: "time=2015-06-27T16:40:35.000Z"
 *   msg_without_tags: ":nick!user@host PRIVMSG #weechat :hello!"
 *               nick: "nick"
 *               user: "user"
 *               host: "nick!user@host"
 *            command: "PRIVMSG"
 *            channel: "#weechat"
 *          arguments: "#weechat :hello!"
 *               text: "hello!"
 *        pos_command: 47
 *      pos_arguments: 55
 *        pos_channel: 55
 *           pos_text: 65
 */

void
irc_message_parse (struct t_irc_server *server, const char *message,
                   char **tags, char **message_without_tags, char **nick,
                   char **user, char **host, char **command, char **channel,
                   char **arguments, char **text,
                   int *pos_command, int *pos_arguments, int *pos_channel,
                   int *pos_text)
{
    const char *ptr_message, *pos, *pos2, *pos3, *pos4;

    if (tags)
        *tags = NULL;
    if (message_without_tags)
        *message_without_tags = NULL;
    if (nick)
        *nick = NULL;
    if (user)
        *user = NULL;
    if (host)
        *host = NULL;
    if (command)
        *command = NULL;
    if (channel)
        *channel = NULL;
    if (arguments)
        *arguments = NULL;
    if (text)
        *text = NULL;
    if (pos_command)
        *pos_command = -1;
    if (pos_arguments)
        *pos_arguments = -1;
    if (pos_channel)
        *pos_channel = -1;
    if (pos_text)
        *pos_text = -1;

    if (!message)
        return;

    ptr_message = message;

    /*
     * we will use this message as example:
     *
     *   @time=2015-06-27T16:40:35.000Z :nick!user@host PRIVMSG #weechat :hello!
     */

    if (ptr_message[0] == '@')
    {
        /*
         * Read tags: they are optional and enabled only if client enabled
         * a server capability.
         * See: https://ircv3.net/specs/core/message-tags-3.2.html
         */
        pos = strchr (ptr_message, ' ');
        if (pos)
        {
            if (tags)
            {
                *tags = weechat_strndup (ptr_message + 1,
                                         pos - (ptr_message + 1));
            }
            ptr_message = pos + 1;
            while (ptr_message[0] == ' ')
            {
                ptr_message++;
            }
        }
    }

    if (message_without_tags)
        *message_without_tags = strdup (ptr_message);

    /* now we have: ptr_message --> ":nick!user@host PRIVMSG #weechat :hello!" */
    if (ptr_message[0] == ':')
    {
        /* read host/nick */
        pos3 = strchr (ptr_message, '@');
        pos2 = strchr (ptr_message, '!');
        pos = strchr (ptr_message, ' ');
        /* if the prefix doesn't contain a '!', split the nick at '@' */
        if (!pos2 || (pos && pos2 > pos))
            pos2 = pos3;
        if (pos2 && pos3 && (pos3 > pos2))
        {
            if (user)
                *user = weechat_strndup (pos2 + 1, pos3 - pos2 - 1);
        }
        if (pos2 && (!pos || pos > pos2))
        {
            if (nick)
                *nick = weechat_strndup (ptr_message + 1, pos2 - (ptr_message + 1));
        }
        else if (pos)
        {
            if (nick)
                *nick = weechat_strndup (ptr_message + 1, pos - (ptr_message + 1));
        }
        if (pos)
        {
            if (host)
                *host = weechat_strndup (ptr_message + 1, pos - (ptr_message + 1));
            ptr_message = pos + 1;
            while (ptr_message[0] == ' ')
            {
                ptr_message++;
            }
        }
        else
        {
            if (host)
                *host = strdup (ptr_message + 1);
            ptr_message += strlen (ptr_message);
        }
    }

    /* now we have: ptr_message --> "PRIVMSG #weechat :hello!" */
    if (ptr_message[0])
    {
        pos = strchr (ptr_message, ' ');
        if (pos)
        {
            if (command)
                *command = weechat_strndup (ptr_message, pos - ptr_message);
            if (pos_command)
                *pos_command = ptr_message - message;
            pos++;
            while (pos[0] == ' ')
            {
                pos++;
            }
            /* now we have: pos --> "#weechat :hello!" */
            if (arguments)
                *arguments = strdup (pos);
            if (pos_arguments)
                *pos_arguments = pos - message;
            if ((pos[0] == ':')
                && ((strncmp (ptr_message, "JOIN ", 5) == 0)
                    || (strncmp (ptr_message, "PART ", 5) == 0)))
            {
                pos++;
            }
            if (pos[0] == ':')
            {
                if (text)
                    *text = strdup (pos + 1);
                if (pos_text)
                    *pos_text = pos - message + 1;
            }
            else
            {
                if (irc_channel_is_channel (server, pos))
                {
                    pos2 = strchr (pos, ' ');
                    if (channel)
                    {
                        if (pos2)
                            *channel = weechat_strndup (pos, pos2 - pos);
                        else
                            *channel = strdup (pos);
                    }
                    if (pos_channel)
                        *pos_channel = pos - message;
                    if (pos2)
                    {
                        while (pos2[0] == ' ')
                        {
                            pos2++;
                        }
                        if (pos2[0] == ':')
                            pos2++;
                        if (text)
                            *text = strdup (pos2);
                        if (pos_text)
                            *pos_text = pos2 - message;
                    }
                }
                else
                {
                    pos2 = strchr (pos, ' ');
                    if (nick && !*nick)
                    {
                        if (pos2)
                            *nick = weechat_strndup (pos, pos2 - pos);
                        else
                            *nick = strdup (pos);
                    }
                    if (pos2)
                    {
                        pos3 = pos2;
                        pos2++;
                        while (pos2[0] == ' ')
                        {
                            pos2++;
                        }
                        if (irc_channel_is_channel (server, pos2))
                        {
                            pos4 = strchr (pos2, ' ');
                            if (channel)
                            {
                                if (pos4)
                                    *channel = weechat_strndup (pos2, pos4 - pos2);
                                else
                                    *channel = strdup (pos2);
                            }
                            if (pos_channel)
                                *pos_channel = pos2 - message;
                            if (pos4)
                            {
                                while (pos4[0] == ' ')
                                {
                                    pos4++;
                                }
                                if (pos4[0] == ':')
                                    pos4++;
                                if (text)
                                    *text = strdup (pos4);
                                if (pos_text)
                                    *pos_text = pos4 - message;
                            }
                        }
                        else
                        {
                            if (channel)
                                *channel = weechat_strndup (pos, pos3 - pos);
                            if (pos_channel)
                                *pos_channel = pos - message;
                            pos4 = strchr (pos3, ' ');
                            if (pos4)
                            {
                                while (pos4[0] == ' ')
                                {
                                    pos4++;
                                }
                                if (pos4[0] == ':')
                                    pos4++;
                                if (text)
                                    *text = strdup (pos4);
                                if (pos_text)
                                    *pos_text = pos4 - message;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (command)
                *command = strdup (ptr_message);
            if (pos_command)
                *pos_command = ptr_message - message;
        }
    }
}

/*
 * Parses an IRC message and returns hashtable with keys:
 *   - tags
 *   - tag_xxx (one key per tag, with unescaped value)
 *   - message_without_tags
 *   - nick
 *   - host
 *   - command
 *   - channel
 *   - arguments
 *   - text
 *   - pos_command
 *   - pos_arguments
 *   - pos_channel
 *   - pos_text
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
irc_message_parse_to_hashtable (struct t_irc_server *server,
                                const char *message)
{
    char *tags, *message_without_tags, *nick, *user, *host, *command, *channel;
    char *arguments, *text, str_pos[32];
    char empty_str[1] = { '\0' };
    int pos_command, pos_arguments, pos_channel, pos_text;
    struct t_hashtable *hashtable;

    irc_message_parse (server, message, &tags, &message_without_tags, &nick,
                       &user, &host, &command, &channel, &arguments, &text,
                       &pos_command, &pos_arguments, &pos_channel, &pos_text);

    hashtable = weechat_hashtable_new (32,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    weechat_hashtable_set (hashtable, "tags",
                           (tags) ? tags : empty_str);
    irc_tag_parse (tags, hashtable, "tag_");
    weechat_hashtable_set (hashtable, "message_without_tags",
                           (message_without_tags) ? message_without_tags : empty_str);
    weechat_hashtable_set (hashtable, "nick",
                           (nick) ? nick : empty_str);
    weechat_hashtable_set (hashtable, "user",
                           (user) ? user : empty_str);
    weechat_hashtable_set (hashtable, "host",
                           (host) ? host : empty_str);
    weechat_hashtable_set (hashtable, "command",
                           (command) ? command : empty_str);
    weechat_hashtable_set (hashtable, "channel",
                           (channel) ? channel : empty_str);
    weechat_hashtable_set (hashtable, "arguments",
                           (arguments) ? arguments : empty_str);
    weechat_hashtable_set (hashtable, "text",
                           (text) ? text : empty_str);
    snprintf (str_pos, sizeof (str_pos), "%d", pos_command);
    weechat_hashtable_set (hashtable, "pos_command", str_pos);
    snprintf (str_pos, sizeof (str_pos), "%d", pos_arguments);
    weechat_hashtable_set (hashtable, "pos_arguments", str_pos);
    snprintf (str_pos, sizeof (str_pos), "%d", pos_channel);
    weechat_hashtable_set (hashtable, "pos_channel", str_pos);
    snprintf (str_pos, sizeof (str_pos), "%d", pos_text);
    weechat_hashtable_set (hashtable, "pos_text", str_pos);

    if (tags)
        free (tags);
    if (message_without_tags)
        free (message_without_tags);
    if (nick)
        free (nick);
    if (user)
        free (user);
    if (host)
        free (host);
    if (command)
        free (command);
    if (channel)
        free (channel);
    if (arguments)
        free (arguments);
    if (text)
        free (text);

    return hashtable;
}

/*
 * Encodes/decodes an IRC message using a charset.
 *
 * Note: result must be freed after use.
 */

char *
irc_message_convert_charset (const char *message, int pos_start,
                             const char *modifier, const char *modifier_data)
{
    char *text, *msg_result;
    int length;

    text = weechat_hook_modifier_exec (modifier, modifier_data,
                                       message + pos_start);
    if (!text)
        return NULL;

    length = pos_start + strlen (text) + 1;
    msg_result = malloc (length);
    if (msg_result)
    {
        msg_result[0] = '\0';
        if (pos_start > 0)
        {
            memcpy (msg_result, message, pos_start);
            msg_result[pos_start] = '\0';
        }
        strcat (msg_result, text);
    }

    free (text);

    return msg_result;
}

/*
 * Gets nick from host in an IRC message.
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
 * Gets address from host in an IRC message.
 */

const char *
irc_message_get_address_from_host (const char *host)
{
    static char address[256];
    char host2[256], *pos_space, *pos;
    const char *ptr_host;

    if (!host)
        return NULL;

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
 * Checks if a raw message is ignored (nick ignored on this server/channel).
 *
 * Returns:
 *   0: message not ignored (displayed)
 *   1: message ignored (not displayed)
 */

int
irc_message_ignored (struct t_irc_server *server, const char *message)
{
    char *nick, *host, *host_no_color, *channel;
    struct t_irc_channel *ptr_channel;
    int ignored;

    if (!server || !message)
        return 0;

    /* parse raw message */
    irc_message_parse (server, message,
                       NULL, NULL, &nick, NULL, &host,
                       NULL, &channel, NULL,
                       NULL, NULL, NULL,
                       NULL, NULL);

    /* remove colors from host */
    host_no_color = (host) ? irc_color_decode (host, 0) : NULL;

    /* search channel */
    ptr_channel = (channel) ? irc_channel_search (server, channel) : NULL;

    /* check if message is ignored or not */
    ignored = irc_ignore_check (
        server,
        (ptr_channel) ? ptr_channel->name : channel,
        nick,
        host_no_color);

    if (nick)
        free (nick);
    if (host)
        free (host);
    if (host_no_color)
        free (host_no_color);
    if (channel)
        free (channel);

    return ignored;
}

/*
 * Replaces special IRC vars ($nick, $channel, $server) in a string.
 *
 * Note: result must be freed after use.
 */

char *
irc_message_replace_vars (struct t_irc_server *server,
                          const char *channel_name, const char *string)
{
    const char *var_nick, *var_channel, *var_server;
    char empty_string[1] = { '\0' };
    char *res, *temp;

    var_nick = (server && server->nick) ? server->nick : empty_string;
    var_channel = (channel_name) ? channel_name : empty_string;
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
 * Adds a message + arguments in hashtable.
 */

void
irc_message_split_add (struct t_hashtable *hashtable, int number,
                       const char *tags, const char *message,
                       const char *arguments)
{
    char key[32], value[32], *buf;
    int length;

    if (message)
    {
        length = ((tags) ? strlen (tags) : 0) + strlen (message) + 1;
        buf = malloc (length);
        if (buf)
        {
            snprintf (key, sizeof (key), "msg%d", number);
            snprintf (buf, length, "%s%s",
                      (tags) ? tags : "",
                      message);
            weechat_hashtable_set (hashtable, key, buf);
            if (weechat_irc_plugin->debug >= 2)
            {
                weechat_printf (NULL,
                                "irc_message_split_add >> %s='%s' (%d bytes)",
                                key, buf, length - 1);
            }
            free (buf);
        }
    }
    if (arguments)
    {
        snprintf (key, sizeof (key), "args%d", number);
        weechat_hashtable_set (hashtable, key, arguments);
        if (weechat_irc_plugin->debug >= 2)
        {
            weechat_printf (NULL,
                            "irc_message_split_add >> %s='%s'",
                            key, arguments);
        }
    }
    snprintf (value, sizeof (value), "%d", number);
    weechat_hashtable_set (hashtable, "count", value);
}

/*
 * Splits "arguments" using delimiter and max length.
 *
 * Examples of arguments for this function:
 *
 *   message..: :nick!user@host.com PRIVMSG #channel :Hello world!
 *   arguments:
 *     host     : ":nick!user@host.com"
 *     command  : "PRIVMSG"
 *     target   : "#channel"
 *     prefix   : ":"
 *     arguments: "Hello world!"
 *     suffix   : ""
 *
 *   message..: :nick!user@host.com PRIVMSG #channel :\01ACTION is eating\01
 *   arguments:
 *     host     : ":nick!user@host.com"
 *     command  : "PRIVMSG"
 *     target   : "#channel"
 *     prefix   : ":\01ACTION "
 *     arguments: "is eating"
 *     suffix   : "\01"
 *
 * Messages added to hashtable are:
 *   host + command + target + prefix + XXX + suffix
 * (where XXX is part of "arguments")
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_message_split_string (struct t_hashtable *hashtable,
                          const char *tags,
                          const char *host,
                          const char *command,
                          const char *target,
                          const char *prefix,
                          const char *arguments,
                          const char *suffix,
                          const char delimiter,
                          int max_length_nick_user_host,
                          int max_length)
{
    const char *pos, *pos_max, *pos_next, *pos_last_delim;
    char message[8192], *dup_arguments;
    int number;

    max_length -= 2;  /* by default: 512 - 2 = 510 bytes */
    if (max_length_nick_user_host >= 0)
        max_length -= max_length_nick_user_host;
    else
        max_length -= (host) ? strlen (host) + 1 : 0;
    max_length -= strlen (command) + 1;
    if (target)
        max_length -= strlen (target);
    if (prefix)
        max_length -= strlen (prefix);
    if (suffix)
        max_length -= strlen (suffix);

    if (max_length < 2)
        return 0;

    /* debug message */
    if (weechat_irc_plugin->debug >= 2)
    {
        weechat_printf (NULL,
                        "irc_message_split_string: tags='%s', host='%s', "
                        "command='%s', target='%s', prefix='%s', "
                        "arguments='%s', suffix='%s', max_length=%d",
                        tags, host, command, target, prefix, arguments, suffix,
                        max_length);
    }

    number = 1;

    if (!arguments || !arguments[0])
    {
        snprintf (message, sizeof (message), "%s%s%s %s%s%s%s",
                  (host) ? host : "",
                  (host) ? " " : "",
                  command,
                  (target) ? target : "",
                  (target && target[0]) ? " " : "",
                  (prefix) ? prefix : "",
                  (suffix) ? suffix : "");
        irc_message_split_add (hashtable, 1, tags, message, "");
        return 1;
    }

    while (arguments && arguments[0])
    {
        pos = arguments;
        pos_max = pos + max_length;
        pos_last_delim = NULL;
        while (pos[0])
        {
            if (pos[0] == delimiter)
                pos_last_delim = pos;
            pos_next = weechat_utf8_next_char (pos);
            if (pos_next > pos_max)
                break;
            pos = pos_next;
        }
        if (pos[0] && pos_last_delim)
            pos = pos_last_delim;
        dup_arguments = weechat_strndup (arguments, pos - arguments);
        if (dup_arguments)
        {
            snprintf (message, sizeof (message), "%s%s%s %s%s%s%s%s",
                      (host) ? host : "",
                      (host) ? " " : "",
                      command,
                      (target) ? target : "",
                      (target && target[0]) ? " " : "",
                      (prefix) ? prefix : "",
                      dup_arguments,
                      (suffix) ? suffix : "");
            irc_message_split_add (hashtable, number, tags, message,
                                   dup_arguments);
            number++;
            free (dup_arguments);
        }
        arguments = (pos == pos_last_delim) ? pos + 1 : pos;
    }

    return 1;
}

/*
 * Splits a AUTHENTICATE message: in 400-byte chunks, and adds an extra
 * message "AUTHENTICATE +" if the last message has exactly 400 bytes.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_message_split_authenticate (struct t_hashtable *hashtable,
                                const char *tags, const char *host,
                                const char *command, const char *arguments)
{
    int number, length;
    char message[8192], *args;
    const char *ptr_args;

    number = 1;

    length = 0;
    ptr_args = arguments;
    while (ptr_args && ptr_args[0])
    {
        length = strlen (ptr_args);
        if (length == 0)
            break;
        if (length > 400)
            length = 400;
        args = weechat_strndup (ptr_args, length);
        if (!args)
            return 0;
        snprintf (message, sizeof (message), "%s%s%s %s",
                  (host) ? host : "",
                  (host) ? " " : "",
                  command,
                  args);
        irc_message_split_add (hashtable, number, tags, message, args);
        free (args);
        number++;
        ptr_args += length;
    }

    if ((length == 0) || (length == 400))
    {
        snprintf (message, sizeof (message), "%s%s%s +",
                  (host) ? host : "",
                  (host) ? " " : "",
                  command);
        irc_message_split_add (hashtable, number, tags, message, "+");
        number++;
    }

    return 1;
}

/*
 * Splits a JOIN message, taking care of keeping channel keys with channel
 * names.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_message_split_join (struct t_hashtable *hashtable,
                        const char *tags, const char *host,
                        const char *arguments,
                        int max_length)
{
    int number, channels_count, keys_count, length, length_no_channel;
    int length_to_add, index_channel;
    char **channels, **keys, *pos, *str;
    char msg_to_send[16384], keys_to_add[16384];

    max_length -= 2;  /* by default: 512 - 2 = 510 bytes */

    number = 1;

    channels = NULL;
    channels_count = 0;
    keys = NULL;
    keys_count = 0;

    pos = strchr (arguments, ' ');
    if (pos)
    {
        str = weechat_strndup (arguments, pos - arguments);
        if (!str)
            return 0;
        channels = weechat_string_split (str, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &channels_count);
        free (str);
        while (pos[0] == ' ')
        {
            pos++;
        }
        if (pos[0])
            keys = weechat_string_split (pos, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &keys_count);
    }
    else
    {
        channels = weechat_string_split (arguments, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &channels_count);
    }

    snprintf (msg_to_send, sizeof (msg_to_send), "%s%sJOIN",
              (host) ? host : "",
              (host) ? " " : "");
    length = strlen (msg_to_send);
    length_no_channel = length;
    keys_to_add[0] = '\0';
    index_channel = 0;
    while (index_channel < channels_count)
    {
        length_to_add = 1 + strlen (channels[index_channel]);
        if (index_channel < keys_count)
            length_to_add += 1 + strlen (keys[index_channel]);
        if ((length + length_to_add < max_length)
            || (length == length_no_channel))
        {
            if (length + length_to_add < (int)sizeof (msg_to_send))
            {
                strcat (msg_to_send, (length == length_no_channel) ? " " : ",");
                strcat (msg_to_send, channels[index_channel]);
            }
            if (index_channel < keys_count)
            {
                if (strlen (keys_to_add) + 1 +
                    strlen (keys[index_channel]) < (int)sizeof (keys_to_add))
                {
                    strcat (keys_to_add, (keys_to_add[0]) ? "," : " ");
                    strcat (keys_to_add, keys[index_channel]);
                }
            }
            length += length_to_add;
            index_channel++;
        }
        else
        {
            strcat (msg_to_send, keys_to_add);
            irc_message_split_add (hashtable, number,
                                   tags,
                                   msg_to_send,
                                   msg_to_send + length_no_channel + 1);
            number++;
            snprintf (msg_to_send, sizeof (msg_to_send), "%s%sJOIN",
                      (host) ? host : "",
                      (host) ? " " : "");
            length = strlen (msg_to_send);
            keys_to_add[0] = '\0';
        }
    }

    if (length > length_no_channel)
    {
        strcat (msg_to_send, keys_to_add);
        irc_message_split_add (hashtable, number,
                               tags,
                               msg_to_send,
                               msg_to_send + length_no_channel + 1);
    }

    if (channels)
        weechat_string_free_split (channels);
    if (keys)
        weechat_string_free_split (keys);

    return 1;
}

/*
 * Splits a PRIVMSG or NOTICE message, taking care of keeping the '\01' char
 * used in CTCP messages.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_message_split_privmsg_notice (struct t_hashtable *hashtable,
                                  const char *tags, const char *host,
                                  const char *command, const char *target,
                                  const char *arguments,
                                  int max_length_nick_user_host,
                                  int max_length)
{
    char *arguments2, prefix[4096], suffix[2], *pos, saved_char;
    const char *ptr_args;
    int length, rc;

    /*
     * message sent looks like:
     *   PRIVMSG #channel :hello world!
     *
     * when IRC server sends message to other people, message looks like:
     *   :nick!user@host.com PRIVMSG #channel :hello world!
     */

    arguments2 = strdup (arguments);
    if (!arguments2)
        return 0;

    ptr_args = arguments2;

    /* for CTCP, prefix will be ":\01xxxx " and suffix "\01" */
    prefix[0] = '\0';
    suffix[0] = '\0';
    length = strlen (arguments2);
    if ((arguments2[0] == '\01')
        && (arguments2[length - 1] == '\01'))
    {
        pos = strchr (arguments2, ' ');
        if (pos)
        {
            pos++;
            saved_char = pos[0];
            pos[0] = '\0';
            snprintf (prefix, sizeof (prefix), ":%s", arguments2);
            pos[0] = saved_char;
            arguments2[length - 1] = '\0';
            ptr_args = pos;
            suffix[0] = '\01';
            suffix[1] = '\0';
        }
    }
    if (!prefix[0])
        strcpy (prefix, ":");

    rc = irc_message_split_string (hashtable, tags, host, command, target,
                                   prefix, ptr_args, suffix,
                                   ' ', max_length_nick_user_host, max_length);

    free (arguments2);

    return rc;
}

/*
 * Splits a 005 message (isupport).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_message_split_005 (struct t_hashtable *hashtable,
                       const char *tags, const char *host, const char *command,
                       const char *target, const char *arguments,
                       int max_length)
{
    char *pos, suffix[4096];

    /*
     * 005 message looks like:
     *   :server 005 mynick MODES=4 CHANLIMIT=#:20 NICKLEN=16 USERLEN=10
     *     HOSTLEN=63 TOPICLEN=450 KICKLEN=450 CHANNELLEN=30 KEYLEN=23
     *     CHANTYPES=# PREFIX=(ov)@+ CASEMAPPING=ascii CAPAB IRCD=dancer
     *     :are available on this server
     */

    /* search suffix */
    suffix[0] = '\0';
    pos = strstr (arguments, " :");
    if (pos)
    {
        snprintf (suffix, sizeof (suffix), "%s", pos);
        pos[0] = '\0';
    }

    return irc_message_split_string (hashtable, tags, host, command, target,
                                     NULL, arguments, suffix, ' ', -1,
                                     max_length);
}

/*
 * Splits an IRC message about to be sent to IRC server.
 *
 * The maximum length of an IRC message is 510 bytes for user data + final
 * "\r\n", so full size is 512 bytes (the user data does not include the
 * optional tags before the host).
 *
 * The 512 max length is the default (recommended) and can be changed with the
 * server option called "split_msg_max_length" (0 to disable completely the
 * split).
 *
 * The split takes care about type of message to do a split at best place in
 * message.
 *
 * The hashtable returned contains keys "msg1", "msg2", ..., "msgN" with split
 * of message (these messages do not include the final "\r\n").
 *
 * Hashtable contains "args1", "args2", ..., "argsN" with split of arguments
 * only (no host/command here).
 *
 * Each message ("msgN") in hashtable has command and arguments, and then is
 * ready to be sent to IRC server.
 *
 * Returns hashtable with split message.
 *
 * Note: result must be freed after use.
 */

struct t_hashtable *
irc_message_split (struct t_irc_server *server, const char *message)
{
    struct t_hashtable *hashtable;
    char **argv, **argv_eol, *tags, *host, *command, *arguments, target[4096];
    char *pos, monitor_action[3];
    int split_ok, argc, index_args, max_length_nick, max_length_user;
    int max_length_host, max_length_nick_user_host, split_msg_max_length;

    split_ok = 0;
    tags = NULL;
    host = NULL;
    command = NULL;
    arguments = NULL;
    argv = NULL;
    argv_eol = NULL;

    if (server)
    {
        split_msg_max_length = IRC_SERVER_OPTION_INTEGER(
            server, IRC_SERVER_OPTION_SPLIT_MSG_MAX_LENGTH);

        /*
         * split disabled? use a very high max_length so the message should
         * never be split
         */
        if (split_msg_max_length == 0)
            split_msg_max_length = INT_MAX - 16;
    }
    else
    {
        split_msg_max_length = 512;  /* max length by default */
    }

    /* debug message */
    if (weechat_irc_plugin->debug >= 2)
    {
        weechat_printf (NULL, "irc_message_split: message='%s', max length=%d",
                        message, split_msg_max_length);
    }

    hashtable = weechat_hashtable_new (32,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    if (!message || !message[0])
        goto end;

    if (message[0] == '@')
    {
        pos = strchr (message, ' ');
        if (pos)
        {
            tags = weechat_strndup (message, pos - message + 1);
            message = pos + 1;
        }
    }

    argv = weechat_string_split (message, " ", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    argv_eol = weechat_string_split (message, " ", NULL,
                                     WEECHAT_STRING_SPLIT_STRIP_LEFT
                                     | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
                                     | WEECHAT_STRING_SPLIT_KEEP_EOL,
                                     0, NULL);

    if (argc < 2)
        goto end;

    if (argv[0][0] == ':')
    {
        if (argc < 3)
            goto end;
        host = argv[0];
        command = argv[1];
        arguments = argv_eol[2];
        index_args = 2;
    }
    else
    {
        command = argv[0];
        arguments = argv_eol[1];
        index_args = 1;
    }

    max_length_nick = (server && (server->nick_max_length > 0)) ?
        server->nick_max_length : 16;
    max_length_user = (server && (server->user_max_length > 0)) ?
        server->user_max_length : 10;
    max_length_host = (server && (server->host_max_length > 0)) ?
        server->host_max_length : 63;

    max_length_nick_user_host = 1 +  /* ":"  */
        max_length_nick +            /* nick */
        1 +                          /* "!"  */
        max_length_user +            /* user */
        1 +                          /* @    */
        max_length_host +            /* host */
        1;                           /* " "  */

    if (weechat_strcasecmp (command, "authenticate") == 0)
    {
        /* AUTHENTICATE UzXAmVffxuzFy77XWBGwABBQAgdinelBrKZaR3wE7nsIETuTVY= */
        split_ok = irc_message_split_authenticate (
            hashtable, tags, host, command, arguments);
    }
    else if ((weechat_strcasecmp (command, "ison") == 0)
        || (weechat_strcasecmp (command, "wallops") == 0))
    {
        /*
         * ISON :nick1 nick2 nick3
         * WALLOPS :some text here
         */
        split_ok = irc_message_split_string (
            hashtable, tags, host, command, NULL, ":",
            (argv_eol[index_args][0] == ':') ?
            argv_eol[index_args] + 1 : argv_eol[index_args],
            NULL, ' ', max_length_nick_user_host, split_msg_max_length);
    }
    else if (weechat_strcasecmp (command, "monitor") == 0)
    {
        /*
         * MONITOR + nick1,nick2,nick3
         * MONITOR - nick1,nick2,nick3
         */
        if (((argv_eol[index_args][0] == '+') || (argv_eol[index_args][0] == '-'))
            && (argv_eol[index_args][1] == ' '))
        {
            snprintf (monitor_action, sizeof (monitor_action),
                      "%c ", argv_eol[index_args][0]);
            split_ok = irc_message_split_string (
                hashtable, tags, host, command, NULL, monitor_action,
                argv_eol[index_args] + 2, NULL, ',', max_length_nick_user_host,
                split_msg_max_length);
        }
        else
        {
            split_ok = irc_message_split_string (
                hashtable, tags, host, command, NULL, ":",
                (argv_eol[index_args][0] == ':') ?
                argv_eol[index_args] + 1 : argv_eol[index_args],
                NULL, ',', max_length_nick_user_host, split_msg_max_length);
        }
    }
    else if (weechat_strcasecmp (command, "join") == 0)
    {
        /* JOIN #channel1,#channel2,#channel3 key1,key2 */
        if ((int)strlen (message) > split_msg_max_length - 2)
        {
            /* split join if it's too long */
            split_ok = irc_message_split_join (hashtable, tags, host,
                                               arguments, split_msg_max_length);
        }
    }
    else if ((weechat_strcasecmp (command, "privmsg") == 0)
             || (weechat_strcasecmp (command, "notice") == 0))
    {
        /*
         * PRIVMSG target :some text here
         * NOTICE target :some text here
         */
        if (index_args + 1 <= argc - 1)
        {
            split_ok = irc_message_split_privmsg_notice (
                hashtable, tags, host, command, argv[index_args],
                (argv_eol[index_args + 1][0] == ':') ?
                argv_eol[index_args + 1] + 1 : argv_eol[index_args + 1],
                max_length_nick_user_host, split_msg_max_length);
        }
    }
    else if (weechat_strcasecmp (command, "005") == 0)
    {
        /* :server 005 nick MODES=4 CHANLIMIT=#:20 NICKLEN=16 USERLEN=10 ... */
        if (index_args + 1 <= argc - 1)
        {
            split_ok = irc_message_split_005 (
                hashtable, tags, host, command, argv[index_args],
                (argv_eol[index_args + 1][0] == ':') ?
                argv_eol[index_args + 1] + 1 : argv_eol[index_args + 1],
                split_msg_max_length);
        }
    }
    else if (weechat_strcasecmp (command, "353") == 0)
    {
        /*
         * list of users on channel:
         *   :server 353 mynick = #channel :mynick nick1 @nick2 +nick3
         */
        if (index_args + 2 <= argc - 1)
        {
            if (irc_channel_is_channel (server, argv[index_args + 1]))
            {
                snprintf (target, sizeof (target), "%s %s",
                          argv[index_args], argv[index_args + 1]);
                split_ok = irc_message_split_string (
                    hashtable, tags, host, command, target, ":",
                    (argv_eol[index_args + 2][0] == ':') ?
                    argv_eol[index_args + 2] + 1 : argv_eol[index_args + 2],
                    NULL, ' ', -1, split_msg_max_length);
            }
            else
            {
                if (index_args + 3 <= argc - 1)
                {
                    snprintf (target, sizeof (target), "%s %s %s",
                              argv[index_args], argv[index_args + 1],
                              argv[index_args + 2]);
                    split_ok = irc_message_split_string (
                        hashtable, tags, host, command, target, ":",
                        (argv_eol[index_args + 3][0] == ':') ?
                        argv_eol[index_args + 3] + 1 : argv_eol[index_args + 3],
                        NULL, ' ', -1, split_msg_max_length);
                }
            }
        }
    }

end:
    if (!split_ok
        || (weechat_hashtable_get_integer (hashtable, "items_count") == 0))
    {
        irc_message_split_add (hashtable,
                               (message) ? 1 : 0,
                               tags,
                               message,
                               arguments);
    }

    if (tags)
        free (tags);
    if (argv)
        weechat_string_free_split (argv);
    if (argv_eol)
        weechat_string_free_split (argv_eol);

    return hashtable;
}
