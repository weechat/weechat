/*
 * irc-message.c - functions for IRC messages
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "irc-message.h"
#include "irc-batch.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-ignore.h"
#include "irc-server.h"
#include "irc-tag.h"


/*
 * Parses command arguments and returns:
 *   - params (array of strings)
 *   - num_params (integer)
 *
 * Leading spaces are skipped, trailing spaces are preserved.
 *
 * Trailing parameters (if starting with ":") are returned as a single
 * parameter.
 *
 * Example:
 *
 *   " #channel nick :trailing parameters "
 *
 *   ==> params[0] == "#channel"
 *       params[1] == "nick"
 *       params[2] == "trailing parameters "
 *       params[3] == NULL
 *       num_params = 3
 */

void
irc_message_parse_params (const char *parameters,
                          char ***params, int *num_params)
{
    const char *ptr_params, *pos_end;
    char **new_params;
    int alloc_params, trailing;

    if (!params && !num_params)
        return;

    if (params)
        *params = NULL;
    if (num_params)
        *num_params = 0;

    if (!parameters)
        return;

    alloc_params = 0;
    if (params)
    {
        *params = malloc ((alloc_params + 1) * sizeof ((*params)[0]));
        if (!*params)
            return;
        (*params)[0] = NULL;
    }

    ptr_params = parameters;
    while (ptr_params[0] == ' ')
    {
        ptr_params++;
    }

    trailing = 0;
    while (1)
    {
        pos_end = NULL;
        if (ptr_params[0] == ':')
        {
            ptr_params++;
            trailing = 1;
        }
        else
        {
            pos_end = strchr (ptr_params, ' ');
        }
        if (!pos_end)
            pos_end = ptr_params + strlen (ptr_params);
        if (params)
        {
            alloc_params++;
            new_params = realloc (*params,
                                  (alloc_params + 1) * sizeof ((*params)[0]));
            if (!new_params)
                return;
            *params = new_params;
            (*params)[alloc_params - 1] = weechat_strndup (ptr_params,
                                                           pos_end - ptr_params);
            (*params)[alloc_params] = NULL;
        }
        if (num_params)
            *num_params += 1;
        if (trailing)
            break;
        ptr_params = pos_end;
        while (ptr_params[0] == ' ')
        {
            ptr_params++;
        }
        if (!ptr_params[0])
            break;
    }
}

/*
 * Parses an IRC message and returns:
 *   - tags (string)
 *   - message without tags (string)
 *   - nick (string)
 *   - user (string)
 *   - host (string)
 *   - command (string)
 *   - channel (string)
 *   - arguments (string)
 *   - text (string)
 *   - params (array of strings)
 *   - num_params (integer)
 *   - pos_command (integer: command index in message)
 *   - pos_arguments (integer: arguments index in message)
 *   - pos_channel (integer: channel index in message)
 *   - pos_text (integer: text index in message)
 *
 * Example:
 *   @time=2015-06-27T16:40:35.000Z :nick!user@host PRIVMSG #weechat :Hello world!
 *
 * Result:
 *               tags: "time=2015-06-27T16:40:35.000Z"
 *   msg_without_tags: ":nick!user@host PRIVMSG #weechat :Hello world!"
 *               nick: "nick"
 *               user: "user"
 *               host: "nick!user@host"
 *            command: "PRIVMSG"
 *            channel: "#weechat"
 *          arguments: "#weechat :Hello world!"
 *               text: "Hello world!"
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
                   char ***params, int *num_params,
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
    if (params)
        *params = NULL;
    if (num_params)
        *num_params = 0;
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
     *   @time=2015-06-27T16:40:35.000Z :nick!user@host PRIVMSG #weechat :Hello world!
     */

    if (ptr_message[0] == '@')
    {
        /*
         * Read tags: they are optional and enabled only if client enabled
         * a server capability.
         * See: https://ircv3.net/specs/extensions/message-tags
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

    /* now we have: ptr_message --> ":nick!user@host PRIVMSG #weechat :Hello world!" */
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

    /* now we have: ptr_message --> "PRIVMSG #weechat :Hello world!" */
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
            /* now we have: pos --> "#weechat :Hello world!" */
            if (arguments)
                *arguments = strdup (pos);
            if (pos_arguments)
                *pos_arguments = pos - message;
            irc_message_parse_params (pos, params, num_params);
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
 *   - user
 *   - host
 *   - command
 *   - channel
 *   - arguments
 *   - text
 *   - num_params
 *   - param1, param2, ..., paramN
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
    char *arguments, *text, **params, str_key[64], str_pos[32];
    char empty_str[1] = { '\0' };
    int i, num_params, pos_command, pos_arguments, pos_channel, pos_text;
    struct t_hashtable *hashtable;

    irc_message_parse (server, message, &tags, &message_without_tags, &nick,
                       &user, &host, &command, &channel, &arguments, &text,
                       &params, &num_params,
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
    snprintf (str_pos, sizeof (str_pos), "%d", num_params);
    weechat_hashtable_set (hashtable, "num_params", str_pos);
    for (i = 0; i < num_params; i++)
    {
        snprintf (str_key, sizeof (str_key), "param%d", i + 1);
        weechat_hashtable_set (hashtable, str_key, params[i]);
    }
    snprintf (str_pos, sizeof (str_pos), "%d", pos_command);
    weechat_hashtable_set (hashtable, "pos_command", str_pos);
    snprintf (str_pos, sizeof (str_pos), "%d", pos_arguments);
    weechat_hashtable_set (hashtable, "pos_arguments", str_pos);
    snprintf (str_pos, sizeof (str_pos), "%d", pos_channel);
    weechat_hashtable_set (hashtable, "pos_channel", str_pos);
    snprintf (str_pos, sizeof (str_pos), "%d", pos_text);
    weechat_hashtable_set (hashtable, "pos_text", str_pos);

    free (tags);
    free (message_without_tags);
    free (nick);
    free (user);
    free (host);
    free (command);
    free (channel);
    free (arguments);
    free (text);
    weechat_string_free_split (params);

    return hashtable;
}

/*
 * Parses capability value.
 *
 * For example for this capability:
 *   draft/multiline=max-bytes=4096,max-lines=24
 *
 * The input value must be: "max-bytes=4096,max-lines=24"
 * The output is a hashtable with following keys/values (as strings):
 *
 *   "max-bytes": "4096"
 *   "max-lines": "24"
 *
 * Note: hashtable must be freed after use.
 */

struct t_hashtable *
irc_message_parse_cap_value (const char *value)
{
    struct t_hashtable *hashtable;
    char **items, *key;
    const char *pos;
    int i, count_items;

    if (!value)
        return NULL;

    hashtable = weechat_hashtable_new (32,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL, NULL);
    if (!hashtable)
        return NULL;

    items = weechat_string_split (value, ",", NULL, 0, 0, &count_items);
    if (items)
    {
        for (i = 0; i < count_items; i++)
        {
            pos = strchr (items[i], '=');
            if (pos)
            {
                key = weechat_strndup (items[i], pos - items[i]);
                if (key)
                {
                    weechat_hashtable_set (hashtable, key, pos + 1);
                    free (key);
                }
            }
            else
            {
                weechat_hashtable_set (hashtable, items[i], NULL);
            }
        }
        weechat_string_free_split (items);
    }

    return hashtable;
}

/*
 * Parses "draft/multiline" cap value and extract:
 *   - max-bytes: maximum allowed total byte length of multiline batched content
 *   - max-lines: maximum allowed number of lines in a multiline batch content
 */

void
irc_message_parse_cap_multiline_value (struct t_irc_server *server,
                                       const char *value)
{
    struct t_hashtable *values;
    const char *ptr_value;
    char *error;
    long number;

    if (!server)
        return;

    server->multiline_max_bytes = IRC_SERVER_MULTILINE_DEFAULT_MAX_BYTES;
    server->multiline_max_lines = IRC_SERVER_MULTILINE_DEFAULT_MAX_LINES;

    if (!value)
        return;

    values = irc_message_parse_cap_value (value);
    if (!values)
        return;

    ptr_value = (const char *)weechat_hashtable_get (values, "max-bytes");
    if (ptr_value)
    {
        error = NULL;
        number = strtol (ptr_value, &error, 10);
        if (error && !error[0])
            server->multiline_max_bytes = number;
    }

    ptr_value = (const char *)weechat_hashtable_get (values, "max-lines");
    if (ptr_value)
    {
        error = NULL;
        number = strtol (ptr_value, &error, 10);
        if (error && !error[0])
            server->multiline_max_lines = number;
    }

    weechat_hashtable_free (values);
}

/*
 * Checks if a message is empty: either empty string or contains only newlines.
 *
 * Returns:
 *   1: message is empty
 *   0: message is NOT empty
 */

int
irc_message_is_empty (const char *message)
{
    const char *ptr_msg;

    if (!message || !message[0])
        return 1;

    ptr_msg = message;
    while (ptr_msg && ptr_msg[0])
    {
        if (ptr_msg[0] != '\n')
            return 0;
        ptr_msg = weechat_utf8_next_char (ptr_msg);
    }

    /* only newlines => consider message is empty */
    return 1;
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
    irc_message_parse (server,
                       message,
                       NULL,  /* tags */
                       NULL,  /* message_without_tags */
                       &nick,
                       NULL,  /* user */
                       &host,
                       NULL,  /* command */
                       &channel,
                       NULL,  /* arguments */
                       NULL,  /* text */
                       NULL,  /* params */
                       NULL,  /* num_params */
                       NULL,  /* pos_command */
                       NULL,  /* pos_arguments */
                       NULL,  /* pos_channel */
                       NULL);  /* pos_text */

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

    free (nick);
    free (host);
    free (host_no_color);
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
 * Hides password in text, if the target is a nick configured in option
 * irc.look.nicks_hide_password.
 *
 * Note: result must be freed after use.
 */

char *
irc_message_hide_password (struct t_irc_server *server, const char *target,
                           const char *text)
{
    int i, hide_password;

    if (!text)
        return NULL;

    /* check if the password must be hidden for this nick */
    hide_password = 0;
    if (irc_config_nicks_hide_password)
    {
        for (i = 0; i < irc_config_num_nicks_hide_password; i++)
        {
            if (weechat_strcasecmp (irc_config_nicks_hide_password[i],
                                    target) == 0)
            {
                hide_password = 1;
                break;
            }
        }
    }

    /* hide password in message displayed using modifier */
    if (hide_password)
    {
        return weechat_hook_modifier_exec ("irc_message_auth", server->name,
                                           text);
    }

    return strdup (text);
}

/*
 * Adds a message + arguments in hashtable.
 */

void
irc_message_split_add (struct t_irc_message_split_context *context,
                       const char *tags, const char *message,
                       const char *arguments)
{
    char key[32], value[32], *buf;
    int length;

    if (!context)
        return;

    if (message)
    {
        snprintf (key, sizeof (key), "msg%d", context->number);
        if (weechat_asprintf (&buf,
                              "%s%s",
                              (tags) ? tags : "",
                              message) >= 0)
        {
            length = strlen (buf);
            weechat_hashtable_set (context->hashtable, key, buf);
            if (weechat_irc_plugin->debug >= 2)
            {
                weechat_printf (NULL,
                                "irc_message_split_add >> %s='%s' (%d bytes)",
                                key, buf, length);
            }
            free (buf);
            context->total_bytes += length + 1;
        }
    }
    if (arguments)
    {
        snprintf (key, sizeof (key), "args%d", context->number);
        weechat_hashtable_set (context->hashtable, key, arguments);
        if (weechat_irc_plugin->debug >= 2)
        {
            weechat_printf (NULL,
                            "irc_message_split_add >> %s='%s'",
                            key, arguments);
        }
    }
    snprintf (value, sizeof (value), "%d", context->number);
    weechat_hashtable_set (context->hashtable, "count", value);
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
 *   message..: :nick!user@host.com PRIVMSG #channel :\001ACTION is eating\001
 *   arguments:
 *     host     : ":nick!user@host.com"
 *     command  : "PRIVMSG"
 *     target   : "#channel"
 *     prefix   : ":\001ACTION "
 *     arguments: "is eating"
 *     suffix   : "\001"
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
irc_message_split_string (struct t_irc_message_split_context *context,
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

    if (!context)
        return 0;

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

    if ((max_length < 2) || !arguments || !arguments[0])
    {
        /*
         * max length is not known (server probably sent values that are not
         * consistent), or no arguments => in this case, we just return message
         * as-is (no split)
         */
        snprintf (message, sizeof (message), "%s%s%s %s%s%s%s%s",
                  (host) ? host : "",
                  (host) ? " " : "",
                  command,
                  (target) ? target : "",
                  (target && target[0]) ? " " : "",
                  (prefix) ? prefix : "",
                  (arguments) ? arguments : "",
                  (suffix) ? suffix : "");
        irc_message_split_add (context, tags, message, (arguments) ? arguments : "");
        (context->number)++;
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
            irc_message_split_add (context, tags, message, dup_arguments);
            (context->number)++;
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
irc_message_split_authenticate (struct t_irc_message_split_context *context,
                                const char *tags, const char *host,
                                const char *command, const char *arguments)
{
    int length;
    char message[8192], *args;
    const char *ptr_args;

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
        irc_message_split_add (context, tags, message, args);
        free (args);
        (context->number)++;
        ptr_args += length;
    }

    if ((length == 0) || (length == 400))
    {
        snprintf (message, sizeof (message), "%s%s%s +",
                  (host) ? host : "",
                  (host) ? " " : "",
                  command);
        irc_message_split_add (context, tags, message, "+");
        (context->number)++;
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
irc_message_split_join (struct t_irc_message_split_context *context,
                        const char *tags, const char *host,
                        const char *arguments,
                        int max_length)
{
    int channels_count, keys_count, length, length_no_channel;
    int length_to_add, index_channel;
    char **channels, **keys, *pos, *str;
    char msg_to_send[16384], keys_to_add[16384];

    max_length -= 2;  /* by default: 512 - 2 = 510 bytes */

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
            irc_message_split_add (context,
                                   tags,
                                   msg_to_send,
                                   msg_to_send + length_no_channel + 1);
            (context->number)++;
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
        irc_message_split_add (context,
                               tags,
                               msg_to_send,
                               msg_to_send + length_no_channel + 1);
    }

    weechat_string_free_split (channels);
    weechat_string_free_split (keys);

    return 1;
}

/*
 * Starts batch for multiline message.
 */

void
irc_message_start_batch (struct t_irc_message_split_context *context,
                         const char *target, const char *batch_ref)
{
    char msg_batch[4096], args_batch[4096];

    snprintf (msg_batch, sizeof (msg_batch),
              "BATCH +%s draft/multiline %s",
              batch_ref,
              target);
    snprintf (args_batch, sizeof (args_batch),
              "+%s draft/multiline %s",
              batch_ref,
              target);
    irc_message_split_add (context, NULL, msg_batch, args_batch);
    (context->number)++;
}

/*
 * Ends batch for multiline message.
 */

void
irc_message_end_batch (struct t_irc_message_split_context *context,
                       const char *batch_ref)
{
    char msg_batch[4096], args_batch[4096];

    snprintf (msg_batch, sizeof (msg_batch),
              "BATCH -%s",
              batch_ref);
    snprintf (args_batch, sizeof (args_batch),
              "-%s",
              batch_ref);
    irc_message_split_add (context, NULL, msg_batch, args_batch);
    (context->number)++;
}

/*
 * Splits a PRIVMSG or NOTICE message, taking care of keeping the '\001' char
 * used in CTCP messages.
 *
 * If multiline == 1, the message is split on newline chars ('\n') and is sent
 * using BATCH command (to group messages together).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_message_split_privmsg_notice (struct t_irc_message_split_context *context,
                                  const char *tags,
                                  const char *host,
                                  const char *command,
                                  const char *target,
                                  const char *arguments,
                                  int max_length_nick_user_host,
                                  int max_length,
                                  int multiline,
                                  int multiline_max_bytes,
                                  int multiline_max_lines)
{
    char prefix[4096], suffix[2], *pos, saved_char, name[256];
    char tags_multiline[4096], **list_lines, batch_ref[16 + 1];
    char  **multiline_args;
    const char *ptr_args;
    int i, length, length_tags, rc, count_lines, batch_lines;
    int index_multiline_args;

    /*
     * message sent looks like:
     *   PRIVMSG #channel :hello world!
     *
     * when IRC server sends message to other people, message looks like:
     *   :nick!user@host.com PRIVMSG #channel :hello world!
     */

    rc = 1;

    /* privmsg/notice with empty message: not allowed */
    if (irc_message_is_empty (arguments))
        return 1;

    if (multiline)
    {
        index_multiline_args = 1;
        multiline_args = weechat_string_dyn_alloc (256);
        if (!multiline_args)
            return 0;

        irc_batch_generate_random_ref (batch_ref, sizeof (batch_ref) - 1);

        /* start batch */
        irc_message_start_batch (context, target, batch_ref);

        /* add messages */
        list_lines = weechat_string_split (arguments, "\n", NULL, 0, 0,
                                           &count_lines);
        if (list_lines)
        {
            batch_lines = 0;
            for (i = 0; i < count_lines; i++)
            {
                if (tags && tags[0])
                {
                    snprintf (tags_multiline, sizeof (tags_multiline),
                              "@batch=%s;%s",
                              batch_ref,
                              tags + 1);
                }
                else
                {
                    snprintf (tags_multiline, sizeof (tags_multiline),
                              "@batch=%s ",
                              batch_ref);
                }
                length_tags = strlen (tags_multiline);
                rc &= irc_message_split_string (
                    context, tags_multiline, host, command, target, ":",
                    list_lines[i], "", ' ', max_length_nick_user_host,
                    max_length);
                if (batch_lines > 0)
                    weechat_string_dyn_concat (multiline_args, "\n", -1);
                weechat_string_dyn_concat (multiline_args,
                                           list_lines[i], -1);
                batch_lines++;
                if ((i < count_lines - 1)
                    && ((batch_lines >= multiline_max_lines)
                        || (context->total_bytes + length_tags
                            + (int)strlen (list_lines[i + 1]) >= multiline_max_bytes)))
                {
                    /* start new batch if we have reached max lines/bytes */
                    irc_message_end_batch (context, batch_ref);
                    snprintf (name, sizeof (name),
                              "multiline_args%d", index_multiline_args);
                    weechat_hashtable_set (context->hashtable, name,
                                           *multiline_args);
                    weechat_string_dyn_copy (multiline_args, NULL);
                    index_multiline_args++;
                    irc_batch_generate_random_ref (batch_ref,
                                                   sizeof (batch_ref) - 1);
                    context->total_bytes = 0;
                    irc_message_start_batch (context, target, batch_ref);
                    batch_lines = 0;
                }
            }
            weechat_string_free_split (list_lines);
        }

        /* end batch */
        irc_message_end_batch (context, batch_ref);

        snprintf (name, sizeof (name),
                  "multiline_args%d", index_multiline_args);
        weechat_hashtable_set (context->hashtable, name, *multiline_args);
        weechat_string_dyn_free (multiline_args, 1);
    }
    else
    {
        list_lines = weechat_string_split (arguments, "\n", NULL, 0, 0,
                                           &count_lines);
        if (list_lines)
        {
            for (i = 0; i < count_lines; i++)
            {
                /* for CTCP, prefix is ":\001xxxx " and suffix "\001" */
                prefix[0] = '\0';
                suffix[0] = '\0';
                ptr_args = list_lines[i];
                length = strlen (list_lines[i]);
                if ((list_lines[i][0] == '\001')
                    && (list_lines[i][length - 1] == '\001'))
                {
                    pos = strchr (list_lines[i], ' ');
                    if (pos)
                    {
                        pos++;
                        saved_char = pos[0];
                        pos[0] = '\0';
                        snprintf (prefix, sizeof (prefix), ":%s", list_lines[i]);
                        pos[0] = saved_char;
                        list_lines[i][length - 1] = '\0';
                        ptr_args = pos;
                    }
                    else
                    {
                        list_lines[i][length - 1] = '\0';
                        snprintf (prefix, sizeof (prefix), ":%s", list_lines[i]);
                        ptr_args = "";
                    }
                    suffix[0] = '\001';
                    suffix[1] = '\0';
                }
                if (!prefix[0])
                    strcpy (prefix, ":");
                rc = irc_message_split_string (context, tags, host, command, target,
                                               prefix, ptr_args, suffix,
                                               ' ', max_length_nick_user_host,
                                               max_length);
            }
            weechat_string_free_split (list_lines);
        }
    }

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
irc_message_split_005 (struct t_irc_message_split_context *context,
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

    return irc_message_split_string (context, tags, host, command, target,
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
    struct t_irc_message_split_context split_context;
    char **argv, **argv_eol, *tags, *host, *command, *arguments, target[4096];
    char *pos, monitor_action[3];
    int split_ok, split_privmsg, argc, index_args, max_length_nick;
    int max_length_user, max_length_host, max_length_nick_user_host;
    int split_msg_max_length, multiline, multiline_max_bytes;
    int multiline_max_lines;

    split_context.hashtable = NULL;
    split_context.number = 1;
    split_context.total_bytes = 0;

    split_ok = 0;
    split_privmsg = 0;
    tags = NULL;
    host = NULL;
    command = NULL;
    arguments = NULL;
    argv = NULL;
    argv_eol = NULL;
    multiline = 0;

    if (server)
    {
        split_msg_max_length = (server->msg_max_length > 0) ?
            server->msg_max_length :
            IRC_SERVER_OPTION_INTEGER(
                server, IRC_SERVER_OPTION_SPLIT_MSG_MAX_LENGTH);
        /* if split disabled, use a high max_length to prevent any split */
        if (split_msg_max_length == 0)
            split_msg_max_length = INT_MAX - 16;
        multiline_max_bytes = server->multiline_max_bytes;
        multiline_max_lines = server->multiline_max_lines;
    }
    else
    {
        split_msg_max_length = 512;
        multiline_max_bytes = 0;
        multiline_max_lines = 0;
    }

    /* debug message */
    if (weechat_irc_plugin->debug >= 2)
    {
        weechat_printf (NULL, "irc_message_split: message='%s', max length=%d",
                        message, split_msg_max_length);
    }

    split_context.hashtable = weechat_hashtable_new (32,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     NULL, NULL);
    if (!split_context.hashtable)
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

    multiline = (
        server
        && ((weechat_strcasecmp (command, "privmsg") == 0)
            || (weechat_strcasecmp (command, "notice") == 0))
        && message
        && strchr (message, '\n')
        && (index_args + 1 <= argc - 1)
        && (weechat_strncmp (argv[index_args + 1], "\001", 1) != 0)
        && (weechat_strncmp (argv[index_args + 1], ":\001", 2) != 0)
        && weechat_hashtable_has_key (server->cap_list, "batch")
        && weechat_hashtable_has_key (server->cap_list, "draft/multiline"));

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
            &split_context, tags, host, command, arguments);
    }
    else if ((weechat_strcasecmp (command, "ison") == 0)
        || (weechat_strcasecmp (command, "wallops") == 0))
    {
        /*
         * ISON :nick1 nick2 nick3
         * WALLOPS :some text here
         */
        split_ok = irc_message_split_string (
            &split_context, tags, host, command, NULL, ":",
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
                &split_context, tags, host, command, NULL, monitor_action,
                argv_eol[index_args] + 2, NULL, ',', max_length_nick_user_host,
                split_msg_max_length);
        }
        else
        {
            split_ok = irc_message_split_string (
                &split_context, tags, host, command, NULL, ":",
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
            split_ok = irc_message_split_join (&split_context, tags, host,
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
        split_privmsg = 1;
        if (index_args + 1 <= argc - 1)
        {
            split_ok = irc_message_split_privmsg_notice (
                &split_context, tags, host, command, argv[index_args],
                (argv_eol[index_args + 1][0] == ':') ?
                argv_eol[index_args + 1] + 1 : argv_eol[index_args + 1],
                max_length_nick_user_host, split_msg_max_length,
                multiline, multiline_max_bytes, multiline_max_lines);
        }
    }
    else if (weechat_strcasecmp (command, "005") == 0)
    {
        /* :server 005 nick MODES=4 CHANLIMIT=#:20 NICKLEN=16 USERLEN=10 ... */
        if (index_args + 1 <= argc - 1)
        {
            split_ok = irc_message_split_005 (
                &split_context, tags, host, command, argv[index_args],
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
                    &split_context, tags, host, command, target, ":",
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
                        &split_context, tags, host, command, target, ":",
                        (argv_eol[index_args + 3][0] == ':') ?
                        argv_eol[index_args + 3] + 1 : argv_eol[index_args + 3],
                        NULL, ' ', -1, split_msg_max_length);
                }
            }
        }
    }

end:
    if (!split_ok
        || (!split_privmsg
            && (weechat_hashtable_get_integer (split_context.hashtable,
                                               "items_count") == 0)))
    {
        split_context.number = (message) ? 1 : 0;
        irc_message_split_add (&split_context,
                               tags,
                               message,
                               arguments);
    }

    free (tags);
    weechat_string_free_split (argv);
    weechat_string_free_split (argv_eol);

    return split_context.hashtable;
}
