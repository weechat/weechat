/*
 * relay-irc.c - IRC protocol for relay to client: IRC proxy/bouncer
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "../../weechat-plugin.h"
#include "../relay.h"
#include "relay-irc.h"
#include "../relay-buffer.h"
#include "../relay-client.h"
#include "../relay-config.h"
#include "../relay-raw.h"
#include "../relay-server.h"


char *relay_irc_relay_commands[] = { "privmsg", "notice", NULL };
char *relay_irc_ignore_commands[] = { "cap", "pong", "quit", NULL };
char *relay_irc_backlog_commands_tags[RELAY_IRC_NUM_CMD] =
{ "irc_join", "irc_part", "irc_quit", "irc_nick", "irc_privmsg" };
char *relay_irc_server_capabilities[RELAY_IRC_NUM_CAPAB] =
{ "server-time", "echo-message" };


/*
 * Checks if IRC command has to be relayed to client.
 *
 * Returns:
 *   1: command must be relayed
 *   0: command must not be relayed
 */

int
relay_irc_command_relayed (const char *irc_command)
{
    int i;

    if (!irc_command)
        return 0;

    for (i = 0; relay_irc_relay_commands[i]; i++)
    {
        if (weechat_strcasecmp (relay_irc_relay_commands[i], irc_command) == 0)
            return 1;
    }

    /* command must NOT be relayed to client */
    return 0;
}

/*
 * Checks if an IRC command from client must be ignored.
 *
 * Returns:
 *   1: command must be ignored
 *   0: command must not be ignored
 */

int
relay_irc_command_ignored (const char *irc_command)
{
    int i;

    if (!irc_command)
        return 0;

    for (i = 0; relay_irc_ignore_commands[i]; i++)
    {
        if (weechat_strcasecmp (relay_irc_ignore_commands[i], irc_command) == 0)
            return 1;
    }

    /* command must be ignored */
    return 0;
}

/*
 * Searches for a tag of a command (in backlog).
 *
 * Returns index of tag in enum t_relay_irc_command, -1 if tag is not found.
 */

int
relay_irc_search_backlog_commands_tags (const char *tag)
{
    int i;

    if (!tag)
        return -1;

    for (i = 0; i < RELAY_IRC_NUM_CMD; i++)
    {
        if (strcmp (relay_irc_backlog_commands_tags[i], tag) == 0)
            return i;
    }

    /* tag not found */
    return -1;
}

/*
 * Searches for a server capability.
 *
 * Returns index of server capability in enum t_relay_irc_server_capab, -1 if
 * server capability is not found.
 */

int
relay_irc_search_server_capability (const char *capability)
{
    int i;

    if (!capability)
        return -1;

    for (i = 0; i < RELAY_IRC_NUM_CAPAB; i++)
    {
        if (strcmp (relay_irc_server_capabilities[i], capability) == 0)
            return i;
    }

    /* server capability not found */
    return -1;
}

/*
 * Parses an IRC message.
 *
 * Returns hashtable with parsed message, NULL if error.
 */

struct t_hashtable *
relay_irc_message_parse (const char *message)
{
    struct t_hashtable *hash_msg, *hash_parsed;

    if (!message)
        return NULL;

    hash_msg = NULL;
    hash_parsed = NULL;

    hash_msg = weechat_hashtable_new (32,
                                      WEECHAT_HASHTABLE_STRING,
                                      WEECHAT_HASHTABLE_STRING,
                                      NULL, NULL);
    if (!hash_msg)
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory for parsing message"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        goto end;
    }
    weechat_hashtable_set (hash_msg, "message", message);
    hash_parsed = weechat_info_get_hashtable ("irc_message_parse",
                                              hash_msg);
    if (!hash_parsed)
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory for parsing message"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        goto end;
    }

end:
    if (hash_msg)
        weechat_hashtable_free (hash_msg);

    return hash_parsed;
}

/*
 * Sends formatted data to client.
 */

void
relay_irc_sendf (struct t_relay_client *client, const char *format, ...)
{
    int length, number;
    char *pos, hash_key[32], *message, *new_msg1, *new_msg2;
    char modifier_data[128];
    const char *str_message, *ptr_msg1, *ptr_msg2;
    struct t_hashtable *hashtable_in, *hashtable_out;

    if (!client)
        return;

    weechat_va_format (format);
    if (!vbuffer)
        return;

    new_msg1 = NULL;
    hashtable_in = NULL;
    hashtable_out = NULL;

    snprintf (modifier_data, sizeof (modifier_data),
              "0x%lx", (unsigned long)client);
    new_msg1 = weechat_hook_modifier_exec ("relay_client_irc_out1",
                                           modifier_data, vbuffer);

    /* no changes in new message? */
    if (new_msg1 && (strcmp (vbuffer, new_msg1) == 0))
    {
        free (new_msg1);
        new_msg1 = NULL;
    }

    /* message dropped? */
    if (new_msg1 && !new_msg1[0])
        goto end;

    ptr_msg1 = (new_msg1) ? new_msg1 : vbuffer;

    pos = strchr (ptr_msg1, '\r');
    if (pos)
        pos[0] = '\0';
    pos = strchr (ptr_msg1, '\n');
    if (pos)
        pos[0] = '\0';

    hashtable_in = weechat_hashtable_new (32,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING,
                                          NULL, NULL);
    if (!hashtable_in)
        goto end;

    weechat_hashtable_set (hashtable_in, "server", client->protocol_args);
    weechat_hashtable_set (hashtable_in, "message", ptr_msg1);
    hashtable_out = weechat_info_get_hashtable ("irc_message_split",
                                                hashtable_in);
    if (!hashtable_out)
        goto end;

    number = 1;
    while (1)
    {
        snprintf (hash_key, sizeof (hash_key), "msg%d", number);
        str_message = weechat_hashtable_get (hashtable_out, hash_key);
        if (!str_message)
            break;
        new_msg2 = weechat_hook_modifier_exec ("relay_client_irc_out",
                                               modifier_data, str_message);
        /* no changes in new message? */
        if (new_msg2 && (strcmp (str_message, new_msg2) == 0))
        {
            free (new_msg2);
            new_msg2 = NULL;
        }
        /* message not dropped? */
        if (!new_msg2 || new_msg2[0])
        {
            ptr_msg2 = (new_msg2) ? new_msg2 : str_message;
            length = strlen (ptr_msg2) + 16 + 1;
            message = malloc (length);
            if (message)
            {
                snprintf (message, length, "%s\r\n", ptr_msg2);
                relay_client_send (client, RELAY_CLIENT_MSG_STANDARD,
                                   message, strlen (message), NULL);
                free (message);
            }
        }
        if (new_msg2)
            free (new_msg2);
        number++;
    }

end:
    if (new_msg1)
        free (new_msg1);
    if (hashtable_in)
        weechat_hashtable_free (hashtable_in);
    if (hashtable_out)
        weechat_hashtable_free (hashtable_out);
    free (vbuffer);
}

/*
 * Parses CAP command received from IRC server.
 */

void
relay_irc_parse_cap_message (struct t_relay_client *client,
                             struct t_hashtable *parsed_msg)
{
    const char *ptr_param;
    char str_param[64], **caps;
    int i, index, num_caps;

    /* only CAP ACK is parsed */
    ptr_param = weechat_hashtable_get (parsed_msg, "param2");
    if (!ptr_param || (weechat_strcasecmp (ptr_param, "ACK") != 0))
        return;

    index = 3;
    while (1)
    {
        snprintf (str_param, sizeof (str_param), "param%d", index);
        ptr_param = weechat_hashtable_get (parsed_msg, str_param);
        if (!ptr_param)
            break;
        caps = weechat_string_split (ptr_param, " ", NULL, 0, 0, &num_caps);
        if (caps)
        {
            for (i = 0; i < num_caps; i++)
            {
                if (strcmp (caps[i], "-echo-message") == 0)
                    RELAY_IRC_DATA(client, irc_cap_echo_message) = 0;
                else if (strcmp (caps[i], "echo-message") == 0)
                    RELAY_IRC_DATA(client, irc_cap_echo_message) = 1;
            }
            weechat_string_free_split (caps);
        }
        index++;
    }
}

/*
 * Callback for signal "irc_in2".
 *
 * This is called when something is received on IRC server, and message can be
 * relayed (or not) to client.
 */

int
relay_irc_signal_irc_in2_cb (const void *pointer, void *data,
                             const char *signal,
                             const char *type_data, void *signal_data)
{
    struct t_relay_client *client;
    const char *ptr_msg, *irc_nick, *irc_host, *irc_command, *irc_args;
    struct t_hashtable *hash_parsed;

    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    client = (struct t_relay_client *)pointer;
    ptr_msg = (const char *)signal_data;

    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: irc_in2: client: %s%s%s, data: %s",
                        RELAY_PLUGIN_NAME,
                        RELAY_COLOR_CHAT_CLIENT,
                        client->desc,
                        RELAY_COLOR_CHAT,
                        ptr_msg);
    }

    hash_parsed = relay_irc_message_parse (ptr_msg);
    if (hash_parsed)
    {
        irc_nick = weechat_hashtable_get (hash_parsed, "nick");
        irc_host = weechat_hashtable_get (hash_parsed, "host");
        irc_command = weechat_hashtable_get (hash_parsed, "command");
        irc_args = weechat_hashtable_get (hash_parsed, "arguments");

        /* if self nick has changed, update it in client data */
        if (irc_command && (weechat_strcasecmp (irc_command, "nick") == 0)
            && irc_nick && irc_nick[0]
            && irc_args && irc_args[0]
            && (weechat_strcasecmp (irc_nick, RELAY_IRC_DATA(client, nick)) == 0))
        {
            if (RELAY_IRC_DATA(client, nick))
                free (RELAY_IRC_DATA(client, nick));
            RELAY_IRC_DATA(client, nick) = strdup ((irc_args[0] == ':') ?
                                                   irc_args + 1 : irc_args);
        }

        /* if capabilities have changed, parse them to update in client */
        if (irc_command && (weechat_strcasecmp (irc_command, "cap") == 0))
        {
            relay_irc_parse_cap_message (client, hash_parsed);
        }

        /* relay all commands to client, but not ping/pong */
        if (irc_command
            && (weechat_strcasecmp (irc_command, "ping") != 0)
            && (weechat_strcasecmp (irc_command, "pong") != 0))
        {
            relay_irc_sendf (client, ":%s %s %s",
                             (irc_host && irc_host[0]) ? irc_host : RELAY_IRC_DATA(client, address),
                             irc_command,
                             irc_args);
        }

        weechat_hashtable_free (hash_parsed);
    }

    return WEECHAT_RC_OK;
}

/*
 * Gets id of client by looking for tag "relay_client_NNN" in list of tags
 * (comma separated list).
 *
 * Returns number found (NNN), -1 if not found.
 */

int
relay_irc_tag_relay_client_id (const char *tags)
{
    char **argv, *error;
    int result, argc, i;
    long number;

    result = -1;

    if (!tags || !tags[0])
        return result;

    argv = weechat_string_split (tags, ",", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    if (argv)
    {
        for (i = 0; i < argc; i++)
        {
            if (strncmp (argv[i], "relay_client_", 13) == 0)
            {
                error = NULL;
                number = strtol (argv[i] + 13, &error, 10);
                if (error && !error[0])
                {
                    result = number;
                    break;
                }
            }
        }
        weechat_string_free_split (argv);
    }

    return result;
}

/*
 * Callback for signal "irc_outtags".
 *
 * This is called when a message is sent to IRC server (by irc plugin or any
 * other plugin/script).
 */

int
relay_irc_signal_irc_outtags_cb (const void *pointer, void *data,
                                 const char *signal,
                                 const char *type_data,
                                 void *signal_data)
{
    struct t_relay_client *client;
    struct t_hashtable *hash_parsed;
    const char *irc_command, *irc_args, *host, *ptr_message;
    char *pos, *tags, *irc_channel, *message;
    struct t_infolist *infolist_nick;
    char str_infolist_args[256];

    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    client = (struct t_relay_client *)pointer;

    tags = NULL;

    message = strdup ((char *)signal_data);
    if (!message)
        goto end;
    pos = strchr (message, '\r');
    if (pos)
        pos[0] = '\0';

    ptr_message = message;

    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: irc_outtags: client: %s%s%s, message: %s",
                        RELAY_PLUGIN_NAME,
                        RELAY_COLOR_CHAT_CLIENT,
                        client->desc,
                        RELAY_COLOR_CHAT,
                        message);
    }

    pos = strchr (ptr_message, ';');
    if (pos)
    {
        if (pos > ptr_message + 1)
            tags = weechat_strndup (ptr_message, pos - ptr_message);
        ptr_message = pos + 1;
    }

    /*
     * We check if there is a tag "relay_client_NNN" and if NNN (numeric)
     * is equal to current client, then we ignore message, because message
     * was sent from this same client!
     * This is to prevent message from being displayed twice on client.
     */
    if (relay_irc_tag_relay_client_id (tags) == client->id)
        goto end;

    hash_parsed = relay_irc_message_parse (ptr_message);
    if (hash_parsed)
    {
        irc_command = weechat_hashtable_get (hash_parsed, "command");
        irc_args = weechat_hashtable_get (hash_parsed, "arguments");

        pos = strchr (irc_args, ' ');
        irc_channel = (pos) ?
            weechat_strndup (irc_args, pos - irc_args) : strdup (irc_args);

        /* if command has to be relayed, relay it to client */
        if (irc_command && irc_command[0]
            && irc_channel && irc_channel[0]
            && relay_irc_command_relayed (irc_command))
        {
            /*
             * relay command only if capability echo-message is NOS enabled
             * in IRC server (otherwise message would be displayed two times)
             */
            if (!RELAY_IRC_DATA(client, irc_cap_echo_message))
            {
                /* get host for nick (it is self nick) */
                snprintf (str_infolist_args, sizeof (str_infolist_args),
                          "%s,%s,%s",
                          client->protocol_args,
                          irc_channel,
                          RELAY_IRC_DATA(client, nick));

                host = NULL;
                infolist_nick = weechat_infolist_get ("irc_nick", NULL,
                                                      str_infolist_args);
                if (infolist_nick && weechat_infolist_next (infolist_nick))
                    host = weechat_infolist_string (infolist_nick, "host");

                /* send message to client */
                relay_irc_sendf (client,
                                 ":%s%s%s %s",
                                 RELAY_IRC_DATA(client, nick),
                                 (host && host[0]) ? "!" : "",
                                 (host && host[0]) ? host : "",
                                 ptr_message);

                if (infolist_nick)
                    weechat_infolist_free (infolist_nick);
            }
        }
        if (irc_channel)
            free (irc_channel);
        weechat_hashtable_free (hash_parsed);
    }

end:
    if (message)
        free (message);
    if (tags)
        free (tags);

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "irc_disconnected".
 *
 * This is called when connection to an IRC server is lost.
 */

int
relay_irc_signal_irc_disc_cb (const void *pointer, void *data,
                              const char *signal,
                              const char *type_data, void *signal_data)
{
    struct t_relay_client *client;

    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    client = (struct t_relay_client *)pointer;

    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: irc_disconnected: data: %s",
                        RELAY_PLUGIN_NAME, (char *)signal_data);
    }

    if (strcmp ((char *)signal_data, client->protocol_args) == 0)
    {
        relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for hsignals "irc_redirection_*".
 *
 * This is called when an IRC command is redirected.
 */

int
relay_irc_hsignal_irc_redir_cb (const void *pointer, void *data,
                                const char *signal,
                                struct t_hashtable *hashtable)
{
    struct t_relay_client *client;
    int rc, client_id, num_messages, i;
    char pattern[128], **messages;
    const char *output;

    /* make C compiler happy */
    (void) data;

    client = (struct t_relay_client *)pointer;

    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: %s: client: %s%s%s",
                        RELAY_PLUGIN_NAME,
                        signal,
                        RELAY_COLOR_CHAT_CLIENT,
                        client->desc,
                        RELAY_COLOR_CHAT);
    }

    rc = sscanf (signal, "irc_redirection_relay_%d_%s",
                 &client_id, pattern);
    if (rc != 2)
        return WEECHAT_RC_OK;

    /* check that client id found in signal exists */
    if (!relay_client_search_by_id (client_id))
        return WEECHAT_RC_OK;

    /* ignore redirection if it is for another relay client */
    if (client->id != client_id)
        return WEECHAT_RC_OK;

    output = weechat_hashtable_get (hashtable, "output");
    if (!output)
        return WEECHAT_RC_OK;

    messages = weechat_string_split (output, "\n", NULL,
                                     WEECHAT_STRING_SPLIT_STRIP_LEFT
                                     | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                     | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                     0, &num_messages);
    if (messages)
    {
        for (i = 0; i < num_messages; i++)
        {
            relay_irc_sendf (client, "%s", messages[i]);
        }
        weechat_string_free_split (messages);
    }

    return WEECHAT_RC_OK;
}

/*
 * Gets info about a line in a buffer:
 *   - irc command
 *   - date
 *   - nick
 *   - nick1 and nick2 (old and new nick for irc "nick" command)
 *   - host
 *   - message (without colors).
 *
 * Arguments hdata_line_data and line_data must be non NULL, the other arguments
 * can be NULL.
 *
 * Note: tags and message (if given and filled) must be freed after use.
 */

void
relay_irc_get_line_info (struct t_relay_client *client,
                         struct t_gui_buffer *buffer,
                         struct t_hdata *hdata_line_data, void *line_data,
                         int *irc_command, int *irc_action, time_t *date,
                         const char **nick, const char **nick1,
                         const char **nick2, const char **host,
                         char **tags, char **message)
{
    int i, num_tags, command, action, all_tags, length;
    char str_tag[512], *pos, *message_no_color, str_time[256];
    const char *ptr_tag, *ptr_message, *ptr_nick, *ptr_nick1, *ptr_nick2;
    const char *ptr_host, *localvar_nick, *time_format;
    time_t msg_date;
    struct tm *tm, gm_time;

    if (irc_command)
        *irc_command = -1;
    if (irc_action)
        *irc_action = 0;
    if (date)
        *date = 0;
    if (nick)
        *nick = NULL;
    if (nick1)
        *nick1 = NULL;
    if (nick2)
        *nick2 = NULL;
    if (host)
        *host = NULL;
    if (tags)
        *tags = NULL;
    if (message)
        *message = NULL;

    msg_date = weechat_hdata_time (hdata_line_data, line_data, "date");
    num_tags = weechat_hdata_get_var_array_size (hdata_line_data, line_data,
                                                 "tags_array");
    ptr_message = weechat_hdata_pointer (hdata_line_data, line_data, "message");

    /* no tag found, or no message? just exit */
    if ((num_tags <= 0) || !ptr_message)
        return;

    command = -1;
    action = 0;
    ptr_nick = NULL;
    ptr_nick1 = NULL;
    ptr_nick2 = NULL;
    ptr_host = NULL;
    all_tags = weechat_hashtable_has_key (relay_config_hashtable_irc_backlog_tags,
                                          "*");
    for (i = 0; i < num_tags; i++)
    {
        snprintf (str_tag, sizeof (str_tag), "%d|tags_array", i);
        ptr_tag = weechat_hdata_string (hdata_line_data, line_data, str_tag);
        if (ptr_tag)
        {
            if (strcmp (ptr_tag, "irc_action") == 0)
                action = 1;
            else if (strncmp (ptr_tag, "nick_", 5) == 0)
                ptr_nick = ptr_tag + 5;
            else if (strncmp (ptr_tag, "irc_nick1_", 10) == 0)
                ptr_nick1 = ptr_tag + 10;
            else if (strncmp (ptr_tag, "irc_nick2_", 10) == 0)
                ptr_nick2 = ptr_tag + 10;
            else if (strncmp (ptr_tag, "host_", 5) == 0)
                ptr_host = ptr_tag + 5;
            else if ((command < 0)
                     && (all_tags
                         || (weechat_hashtable_has_key (relay_config_hashtable_irc_backlog_tags,
                                                        ptr_tag))))
            {
                command = relay_irc_search_backlog_commands_tags (ptr_tag);
            }
        }
    }

    /* not a supported IRC command? */
    if (command < 0)
        return;

    /* ignore join/part/quit from self nick */
    if ((command == RELAY_IRC_CMD_JOIN) || (command == RELAY_IRC_CMD_PART)
        || (command == RELAY_IRC_CMD_QUIT))
    {
        localvar_nick = weechat_buffer_get_string (buffer, "localvar_nick");
        if (localvar_nick && localvar_nick[0]
            && ptr_nick && (strcmp (ptr_nick, localvar_nick) == 0))
        {
            return;
        }
    }

    /* fills variables with the line data */
    if (irc_command)
        *irc_command = command;
    if (irc_action)
        *irc_action = action;
    if (date)
        *date = msg_date;
    if (nick)
        *nick = ptr_nick;
    if (nick1)
        *nick1 = ptr_nick1;
    if (nick2)
        *nick2 = ptr_nick2;
    if (host)
        *host = ptr_host;
    message_no_color = (ptr_message) ?
        weechat_string_remove_color (ptr_message, NULL) : NULL;

    if ((command == RELAY_IRC_CMD_PRIVMSG) && message && message_no_color)
    {
        pos = message_no_color;
        if (action)
        {
            pos = strchr (message_no_color, ' ');
            if (pos)
            {
                while (pos[0] == ' ')
                {
                    pos++;
                }
            }
            else
                pos = message_no_color;
        }
        /*
         * if server capability "server-time" is NOT enabled, and if the time
         * format is not empty, add time inside message (before message)
         */
        time_format = weechat_config_string (relay_config_irc_backlog_time_format);
        if (!(RELAY_IRC_DATA(client, server_capabilities) & (1 << RELAY_IRC_CAPAB_SERVER_TIME))
            && time_format && time_format[0])
        {
            tm = localtime (&msg_date);
            if (strftime (str_time, sizeof (str_time), time_format, tm) == 0)
                str_time[0] = '\0';
            length = strlen (str_time) + strlen (pos) + 1;
            *message = malloc (length);
            if (*message)
                snprintf (*message, length, "%s%s", str_time, pos);
        }
        else
            *message = strdup (pos);
    }

    /* if server capability "server-time" is enabled, add an irc tag with time */
    if (tags
        && (RELAY_IRC_DATA(client, server_capabilities) & (1 << RELAY_IRC_CAPAB_SERVER_TIME)))
    {
        gmtime_r (&msg_date, &gm_time);
        if (strftime (str_time, sizeof (str_time), "%Y-%m-%dT%H:%M:%S",
                      &gm_time) == 0)
        {
            str_time[0] = '\0';
        }
        snprintf (str_tag, sizeof (str_tag), "@time=%s.000Z ", str_time);
        *tags = strdup (str_tag);
    }

    if (message_no_color)
        free (message_no_color);
}

/*
 * Sends channel backlog to client.
 */

void
relay_irc_send_channel_backlog (struct t_relay_client *client,
                                const char *channel,
                                struct t_gui_buffer *buffer)
{
    struct t_relay_server *ptr_server;
    void *ptr_own_lines, *ptr_line, *ptr_line_data;
    void *ptr_hdata_line, *ptr_hdata_line_data;
    char *tags, *message;
    const char *ptr_nick, *ptr_nick1, *ptr_nick2, *ptr_host, *localvar_nick;
    int irc_command, irc_action, count, max_number, max_minutes;
    time_t date_min, date_min2, date;

    /* get pointer on "own_lines" in buffer */
    ptr_own_lines = weechat_hdata_pointer (weechat_hdata_get ("buffer"),
                                           buffer, "own_lines");
    if (!ptr_own_lines)
        return;

    /* get pointer on "last_line" in lines */
    ptr_line = weechat_hdata_pointer (weechat_hdata_get ("lines"),
                                      ptr_own_lines, "last_line");
    if (!ptr_line)
        return;

    /* get hdata "line" */
    ptr_hdata_line = weechat_hdata_get ("line");
    if (!ptr_hdata_line)
        return;

    /* get hdata "line_data" */
    ptr_hdata_line_data = weechat_hdata_get ("line_data");
    if (!ptr_hdata_line_data)
        return;

    localvar_nick = NULL;
    if (weechat_config_boolean (relay_config_irc_backlog_since_last_message))
        localvar_nick = weechat_buffer_get_string (buffer, "localvar_nick");

    max_number = weechat_config_integer (relay_config_irc_backlog_max_number);
    max_minutes = weechat_config_integer (relay_config_irc_backlog_max_minutes);
    date_min = (max_minutes > 0) ? time (NULL) - (max_minutes * 60) : 0;
    if (weechat_config_boolean (relay_config_irc_backlog_since_last_disconnect))
    {
        ptr_server = relay_server_search (client->protocol_string);
        if (ptr_server && (ptr_server->last_client_disconnect > 0))
        {
            date_min2 = ptr_server->last_client_disconnect;
            if (date_min2 > date_min)
                date_min = date_min2;
        }
    }

    /*
     * loop on lines in buffer, from last to first, and stop when we have
     * reached max number of lines (or max minutes)
     */
    count = 0;
    while (ptr_line)
    {
        ptr_line_data = weechat_hdata_pointer (ptr_hdata_line,
                                               ptr_line, "data");
        if (ptr_line_data)
        {
            relay_irc_get_line_info (client, buffer,
                                     ptr_hdata_line_data, ptr_line_data,
                                     &irc_command,
                                     NULL, /* irc_action */
                                     &date,
                                     &ptr_nick,
                                     NULL, /* nick1 */
                                     NULL, /* nick2 */
                                     NULL, /* host */
                                     NULL, /* tags */
                                     NULL); /* message */
            if (irc_command >= 0)
            {
                /* if we have reached max minutes, exit loop */
                if ((date_min > 0) && (date < date_min))
                    break;
                count++;
            }
            /* if we have reached max number of messages, exit loop */
            if ((max_number > 0) && (count > max_number))
                break;

            if (localvar_nick && localvar_nick[0]
                && ptr_nick && (strcmp (ptr_nick, localvar_nick) == 0))
            {
                /*
                 * stop when we find a line sent by the current nick
                 * (and include this line)
                 */
                ptr_line = weechat_hdata_move (ptr_hdata_line, ptr_line, -1);
                break;
            }
        }
        ptr_line = weechat_hdata_move (ptr_hdata_line, ptr_line, -1);
    }

    if (!ptr_line)
    {
        /* if we have reached beginning of buffer, start from first line */
        ptr_line = weechat_hdata_pointer (weechat_hdata_get ("lines"),
                                          ptr_own_lines, "first_line");
    }
    else
    {
        /* start from line + 1 (the current line must not be sent) */
        ptr_line = weechat_hdata_move (ptr_hdata_line, ptr_line, 1);
    }

    /*
     * loop on lines from line pointer until last line of buffer, and for each
     * irc message, sends it to client
     */
    while (ptr_line)
    {
        ptr_line_data = weechat_hdata_pointer (ptr_hdata_line,
                                               ptr_line, "data");
        if (ptr_line_data)
        {
            relay_irc_get_line_info (client, buffer,
                                     ptr_hdata_line_data, ptr_line_data,
                                     &irc_command,
                                     &irc_action,
                                     &date,
                                     &ptr_nick,
                                     &ptr_nick1,
                                     &ptr_nick2,
                                     &ptr_host,
                                     &tags,
                                     &message);
            switch (irc_command)
            {
                case RELAY_IRC_CMD_JOIN:
                    relay_irc_sendf (client,
                                     "%s:%s%s%s JOIN :%s",
                                     (tags) ? tags : "",
                                     ptr_nick,
                                     (ptr_host) ? "!" : "",
                                     (ptr_host) ? ptr_host : "",
                                     channel);
                    break;
                case RELAY_IRC_CMD_PART:
                    relay_irc_sendf (client,
                                     "%s:%s%s%s PART %s",
                                     (tags) ? tags : "",
                                     ptr_nick,
                                     (ptr_host) ? "!" : "",
                                     (ptr_host) ? ptr_host : "",
                                     channel);
                    break;
                case RELAY_IRC_CMD_QUIT:
                    relay_irc_sendf (client,
                                     "%s:%s%s%s QUIT",
                                     (tags) ? tags : "",
                                     ptr_nick,
                                     (ptr_host) ? "!" : "",
                                     (ptr_host) ? ptr_host : "");
                    break;
                case RELAY_IRC_CMD_NICK:
                    if (ptr_nick1 && ptr_nick2)
                    {
                        relay_irc_sendf (client,
                                         "%s:%s NICK :%s",
                                         (tags) ? tags : "",
                                         ptr_nick1,
                                         ptr_nick2);
                    }
                    break;
                case RELAY_IRC_CMD_PRIVMSG:
                    if (ptr_nick && message)
                    {
                        relay_irc_sendf (client,
                                         "%s:%s%s%s PRIVMSG %s :%s%s%s",
                                         (tags) ? tags : "",
                                         ptr_nick,
                                         (ptr_host) ? "!" : "",
                                         (ptr_host) ? ptr_host : "",
                                         channel,
                                         (irc_action) ? "\01ACTION " : "",
                                         message,
                                         (irc_action) ? "\01": "");
                    }
                    break;
                case RELAY_IRC_NUM_CMD:
                    /* make C compiler happy */
                    break;
            }
            if (tags)
                free (tags);
            if (message)
                free (message);
        }
        ptr_line = weechat_hdata_move (ptr_hdata_line, ptr_line, 1);
    }
}

/*
 * Sends IRC "JOIN" for a channel to client.
 */

void
relay_irc_send_join (struct t_relay_client *client,
                     const char *channel)
{
    char *infolist_name, *nicks, *nicks2;
    const char *nick, *prefix, *topic;
    char *host;
    int length, length_nicks;
    struct t_infolist *infolist_nick, *infolist_channel, *infolist_nicks;
    struct t_gui_buffer *buffer;

    length = strlen (client->protocol_args) + 1 + strlen (channel) + 1
        + strlen (RELAY_IRC_DATA(client, nick)) + 1;
    infolist_name = malloc (length);
    if (infolist_name)
    {
        /* get nick host */
        host = NULL;
        snprintf (infolist_name, length, "%s,%s,%s",
                  client->protocol_args,
                  channel,
                  RELAY_IRC_DATA(client, nick));
        infolist_nick = weechat_infolist_get ("irc_nick", NULL, infolist_name);
        if (infolist_nick)
        {
            if (weechat_infolist_next (infolist_nick))
            {
                host = (char *)weechat_infolist_string (infolist_nick, "host");
                if (host)
                    host = strdup (host);
            }
            weechat_infolist_free (infolist_nick);
        }

        relay_irc_sendf (client,
                         ":%s!%s JOIN %s",
                         RELAY_IRC_DATA(client, nick),
                         (host && host[0]) ? host : "weechat@proxy",
                         channel);
        if (host)
            free (host);
        snprintf (infolist_name, length, "%s,%s",
                  client->protocol_args,
                  channel);

        buffer = NULL;
        infolist_channel = weechat_infolist_get ("irc_channel", NULL,
                                                 infolist_name);
        if (infolist_channel)
        {
            if (weechat_infolist_next (infolist_channel))
            {
                buffer = weechat_infolist_pointer (infolist_channel, "buffer");
                topic = weechat_infolist_string (infolist_channel, "topic");
                if (topic && topic[0])
                {
                    relay_irc_sendf (client,
                                     ":%s 332 %s %s :%s",
                                     RELAY_IRC_DATA(client, address),
                                     RELAY_IRC_DATA(client, nick),
                                     channel, topic);
                }
            }
            weechat_infolist_free (infolist_channel);
        }
        infolist_nicks = weechat_infolist_get ("irc_nick", NULL,
                                               infolist_name);
        if (infolist_nicks)
        {
            length_nicks = 0;
            nicks = NULL;
            while (weechat_infolist_next (infolist_nicks))
            {
                nick = weechat_infolist_string (infolist_nicks, "name");
                prefix = weechat_infolist_string (infolist_nicks, "prefix");
                if (nick && nick[0])
                {
                    length_nicks += strlen (nick) + 1 + 1;
                    if (nicks)
                    {
                        nicks2 = realloc (nicks, length_nicks);
                        if (!nicks2)
                        {
                            if (nicks)
                                free (nicks);
                            return;
                        }
                        nicks = nicks2;
                        strcat (nicks, " ");
                    }
                    else
                    {
                        nicks = malloc (length_nicks);
                        nicks[0] = '\0';
                    }
                    if (prefix && (prefix[0] != ' '))
                        strcat (nicks, prefix);
                    strcat (nicks, nick);
                }
            }
            if (nicks)
            {
                relay_irc_sendf (client,
                                 ":%s 353 %s = %s :%s",
                                 RELAY_IRC_DATA(client, address),
                                 RELAY_IRC_DATA(client, nick),
                                 channel, nicks);
                free (nicks);
            }
            weechat_infolist_free (infolist_nicks);
        }
        relay_irc_sendf (client,
                         ":%s 366 %s %s :End of /NAMES list.",
                         RELAY_IRC_DATA(client, address),
                         RELAY_IRC_DATA(client, nick),
                         channel);
        free (infolist_name);

        /* send backlog to client */
        if (buffer)
            relay_irc_send_channel_backlog (client, channel, buffer);
    }
}

/*
 * Sends IRC "JOIN" for all channels of an IRC server to client.
 */

void
relay_irc_send_join_channels (struct t_relay_client *client)
{
    struct t_infolist *infolist_channels;
    const char *name;
    int type;
    struct t_gui_buffer *buffer;

    infolist_channels = weechat_infolist_get ("irc_channel", NULL,
                                              client->protocol_args);
    if (infolist_channels)
    {
        while (weechat_infolist_next (infolist_channels))
        {
            name = weechat_infolist_string (infolist_channels, "name");
            type = weechat_infolist_integer (infolist_channels, "type");
            buffer = weechat_infolist_pointer (infolist_channels, "buffer");
            if (type == 0)
            {
                /* channel */
                if (weechat_infolist_integer (infolist_channels,
                                              "nicks_count") > 0)
                {
                    relay_irc_send_join (client, name);
                }
            }
            else if (type == 1)
            {
                /* private */
                relay_irc_send_channel_backlog (client, name, buffer);
            }
        }
        weechat_infolist_free (infolist_channels);
    }
}

/*
 * Sends text or command on an IRC buffer.
 */

void
relay_irc_input_send (struct t_relay_client *client, const char *irc_channel,
                      const char *options, const char *format, ...)
{
    char buf_beginning[1024], *buf;
    int length_beginning, length_vbuffer;

    weechat_va_format (format);
    if (!vbuffer)
        return;

    snprintf (buf_beginning, sizeof (buf_beginning),
              "%s;%s;%s;relay_client_%d;",
              client->protocol_args,
              (irc_channel) ? irc_channel : "",
              options,
              client->id);

    length_beginning = strlen (buf_beginning);
    length_vbuffer = strlen (vbuffer);
    buf = malloc (length_beginning + length_vbuffer + 1);
    if (buf)
    {
        memcpy (buf, buf_beginning, length_beginning);
        memcpy (buf + length_beginning, vbuffer, length_vbuffer);
        buf[length_beginning + length_vbuffer] = '\0';
        if (weechat_relay_plugin->debug >= 2)
        {
            weechat_printf (NULL,
                            "%s: irc_input_send: \"%s\"",
                            RELAY_PLUGIN_NAME, buf);
        }

        (void) weechat_hook_signal_send ("irc_input_send",
                                         WEECHAT_HOOK_SIGNAL_STRING, buf);
        free (buf);
    }
    free (vbuffer);
}

/*
 * Hooks signals for a client.
 */

void
relay_irc_hook_signals (struct t_relay_client *client)
{
    char str_signal_name[128];

    /* do nothing if "protocol_args" (irc server name) is not yet initialized */
    if (!client->protocol_args)
        return;

    /*
     * hook signal "xxx,irc_in2_*" to catch IRC data received from
     * this server
     */
    snprintf (str_signal_name, sizeof (str_signal_name),
              "%s,irc_in2_*",
              client->protocol_args);
    RELAY_IRC_DATA(client, hook_signal_irc_in2) =
        weechat_hook_signal (str_signal_name,
                             &relay_irc_signal_irc_in2_cb,
                             client, NULL);

    /*
     * hook signal "xxx,irc_outtags_*" to catch IRC data sent to
     * this server
     */
    snprintf (str_signal_name, sizeof (str_signal_name),
              "%s,irc_outtags_*",
              client->protocol_args);
    RELAY_IRC_DATA(client, hook_signal_irc_outtags) =
        weechat_hook_signal (str_signal_name,
                             &relay_irc_signal_irc_outtags_cb,
                             client, NULL);

    /*
     * hook signal "irc_server_disconnected" to disconnect client if
     * connection to server is lost
     */
    RELAY_IRC_DATA(client, hook_signal_irc_disc) =
        weechat_hook_signal ("irc_server_disconnected",
                             &relay_irc_signal_irc_disc_cb,
                             client, NULL);

    /*
     * hook hsignal "irc_redirection_*" to redirect some messages
     */
    RELAY_IRC_DATA(client, hook_hsignal_irc_redir) =
        weechat_hook_hsignal ("irc_redirection_relay_*",
                              &relay_irc_hsignal_irc_redir_cb,
                              client, NULL);
}

/*
 * Compares two capabilities.
 *
 * Returns:
 *   < 0: capability 1 < capability 2
 *     0: capability 1 == capability 2
 *   > 0: capability 1 > capability 2
 */

int
relay_irc_capability_compare_cb (void *data,
                                 struct t_arraylist *arraylist,
                                 void *pointer1,
                                 void *pointer2)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    return (weechat_strcmp ((const char *)pointer1, (const char *)pointer2));
}

/*
 * Frees a capability in list.
 */

void
relay_irc_capability_free_db (void *data,
                              struct t_arraylist *arraylist,
                              void *pointer)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    free (pointer);
}

/*
 * Checks if a capability is currently enabled in IRC server.
 *
 * Returns:
 *   1: capability is enabled in server
 *   0: capability is NOT enabled in server
 */

int
relay_irc_cap_enabled (struct t_relay_client *client, const char *capability)
{
    char str_info[1024], *info;
    int rc;

    if (!client || !capability || !capability[0])
        return 0;

    snprintf (str_info, sizeof (str_info),
              "%s,%s",
              client->protocol_args,
              capability);

    info = weechat_info_get ("irc_server_cap", str_info);
    rc = (info && (strcmp (info, "1") == 0)) ? 1 : 0;
    if (info)
        free (info);
    return rc;
}

/*
 * Returns an integer with bits set for each supported capability (ie that
 * is either always supported, or enabled in the IRC server).
 */

int
relay_irc_get_supported_caps (struct t_relay_client *client)
{
    int i, caps, check_server;

    caps = 0;
    for (i = 0; i < RELAY_IRC_NUM_CAPAB; i++)
    {
        check_server = RELAY_IRC_CAPAB_FOLLOW_SERVER & (1 << i);
        if (!check_server
            || relay_irc_cap_enabled (client, relay_irc_server_capabilities[i]))
        {
            caps |= (1 << i);
        }
    }

    return caps;
}

/*
 * Returns a sorted list of supported all server capabilities by relay (even
 * those that should not be sent to clients when the IRC server doesn't
 * support them).
 *
 * Note: result must be freed after use.
 */

struct t_arraylist *
relay_irc_get_list_caps ()
{
    struct t_arraylist *list_capab;
    int i;

    list_capab = weechat_arraylist_new (
        8, 1, 0,
        &relay_irc_capability_compare_cb, NULL,
        &relay_irc_capability_free_db, NULL);

    for (i = 0; i < RELAY_IRC_NUM_CAPAB; i++)
    {
        weechat_arraylist_add (list_capab,
                               strdup (relay_irc_server_capabilities[i]));
    }

    return list_capab;
}

/*
 * Processes the "CAP" irc command (received from client)
 */

void
relay_irc_recv_command_capab (struct t_relay_client *client,
                              int num_params, const char **params)
{
    struct t_arraylist *list_caps;
    char **str_caps, **caps;
    const char *ptr_cap;
    int i, j, capability, server_caps, num_caps_received, caps_ok, size;
    int num_caps, supported_caps;

    if (num_params < 1)
        return;

    if (weechat_strcasecmp (params[0], "ls") == 0)
    {
        /* return the list of supported server capabilities */
        list_caps = relay_irc_get_list_caps ();
        if (list_caps)
        {
            supported_caps = relay_irc_get_supported_caps (client);
            str_caps = weechat_string_dyn_alloc (256);
            if (str_caps)
            {
                size = weechat_arraylist_size (list_caps);
                for (i = 0; i < size; i++)
                {
                    ptr_cap = (const char *)weechat_arraylist_get (list_caps, i);
                    capability = relay_irc_search_server_capability (ptr_cap);
                    if (capability >= 0)
                    {
                        if (supported_caps & (1 << capability))
                        {
                            if ((*str_caps)[0])
                                weechat_string_dyn_concat (str_caps, " ", -1);
                            weechat_string_dyn_concat (str_caps, ptr_cap, -1);
                        }
                    }
                }
                relay_irc_sendf (
                    client,
                    ":%s CAP %s LS :%s",
                    RELAY_IRC_DATA(client, address),
                    (RELAY_IRC_DATA(client, nick)) ? RELAY_IRC_DATA(client, nick) : "nick",
                    *str_caps);
                weechat_string_dyn_free (str_caps, 1);
            }
            weechat_arraylist_free (list_caps);
        }
        if (!RELAY_IRC_DATA(client, connected))
            RELAY_IRC_DATA(client, cap_ls_received) = 1;
    }
    else if (weechat_strcasecmp (params[0], "req") == 0)
    {
        /* client is asking for one or more server capabilities */
        list_caps = relay_irc_get_list_caps ();
        if (list_caps)
        {
            supported_caps = relay_irc_get_supported_caps (client);
            num_caps_received = 0;
            caps_ok = 1;
            server_caps = RELAY_IRC_DATA(client, server_capabilities);
            str_caps = weechat_string_dyn_alloc (256);
            if (str_caps)
            {
                for (i = 1; i < num_params; i++)
                {
                    if (!params[i][0])
                        continue;
                    if ((*str_caps)[0])
                        weechat_string_dyn_concat (str_caps, " ", -1);
                    weechat_string_dyn_concat (str_caps, params[i], -1);
                    caps = weechat_string_split (params[i], " ", NULL, 0, 0,
                                                 &num_caps);
                    if (caps)
                    {
                        for (j = 0; j < num_caps; j++)
                        {
                            num_caps_received++;
                            capability = relay_irc_search_server_capability (caps[j]);
                            if ((capability >= 0) && (supported_caps & (1 << capability)))
                                server_caps |= 1 << capability;
                            else
                                caps_ok = 0;
                        }
                        weechat_string_free_split (caps);
                    }
                }
                if (num_caps_received == 0)
                    caps_ok = 0;
                if (caps_ok)
                    RELAY_IRC_DATA(client, server_capabilities) = server_caps;
                relay_irc_sendf (
                    client,
                    ":%s CAP %s %s :%s",
                    RELAY_IRC_DATA(client, address),
                    (RELAY_IRC_DATA(client, nick)) ? RELAY_IRC_DATA(client, nick) : "nick",
                    (caps_ok) ? "ACK" : "NAK",
                    *str_caps);
                weechat_string_dyn_free (str_caps, 1);
            }
            /*
             * if the CAP REQ command is received without arguments, we consider
             * the CAP END is received; this is a workaround for clients like
             * Atomic which are sending "CAP REQ :" (see issue #1040)
             */
            if (num_caps_received == 0)
            {
                if (!RELAY_IRC_DATA(client, connected))
                    RELAY_IRC_DATA(client, cap_end_received) = 1;
            }
            weechat_arraylist_free (list_caps);
        }
    }
    else if (weechat_strcasecmp (params[0], "end") == 0)
    {
        if (!RELAY_IRC_DATA(client, connected))
            RELAY_IRC_DATA(client, cap_end_received) = 1;
    }
}

/*
 * Reads one message from client.
 */

void
relay_irc_recv (struct t_relay_client *client, const char *data)
{
    struct t_hashtable *hash_parsed, *hash_redirect;
    struct t_infolist *infolist_server;
    const char *irc_command, *str_num_params, *isupport, *pos_password;
    const char *ptr_data;
    char str_time[128], str_signal[128], str_server_channel[256], *nick;
    char str_param[128], *str_args, *version, str_command[128], **params;
    char *pos, *password, *irc_is_channel, *info, *error, *str_cmd_lower;
    char modifier_data[128], *new_data;
    long num_params;
    int i, redirect_msg;

    new_data = NULL;
    hash_parsed = NULL;
    params = NULL;

    /* display debug message */
    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: recv from client %s%s%s: \"%s\"",
                        RELAY_PLUGIN_NAME,
                        RELAY_COLOR_CHAT_CLIENT,
                        client->desc,
                        RELAY_COLOR_CHAT,
                        data);
    }

    snprintf (modifier_data, sizeof (modifier_data),
              "0x%lx", (unsigned long)client);
    new_data = weechat_hook_modifier_exec ("relay_client_irc_in",
                                           modifier_data, data);

    /* no changes in new data */
    if (new_data && (strcmp (data, new_data) == 0))
    {
        free (new_data);
        new_data = NULL;
    }

    /* message dropped? */
    if (new_data && !new_data[0])
        goto end;

    ptr_data = (new_data) ? new_data : data;

    /* parse IRC message */
    hash_parsed = relay_irc_message_parse (ptr_data);
    if (!hash_parsed)
        goto end;
    irc_command = weechat_hashtable_get (hash_parsed, "command");
    str_num_params = weechat_hashtable_get (hash_parsed, "num_params");
    error = NULL;
    num_params = strtol (str_num_params, &error, 10);
    if (!error || error[0])
        num_params = 0;
    if (num_params > 0)
    {
        params = malloc ((num_params + 1) * sizeof (params[0]));
        if (!params)
            goto end;
        for (i = 0; i < num_params; i++)
        {
            snprintf (str_param, sizeof (str_param), "param%d", i + 1);
            params[i] = weechat_hashtable_get (hash_parsed, str_param);
        }
        params[num_params] = NULL;
    }

    /*
     * first process the "nick" command (it will be processed again in this
     * function below)
     */
    if (irc_command && (weechat_strcasecmp (irc_command, "nick") == 0))
    {
        if (num_params > 0)
        {
            if (RELAY_IRC_DATA(client, nick))
                free (RELAY_IRC_DATA(client, nick));
            RELAY_IRC_DATA(client, nick) = strdup (params[0]);
        }
    }
    /* server capabilities */
    if (irc_command && (weechat_strcasecmp (irc_command, "cap") == 0))
    {
        relay_irc_recv_command_capab (client, num_params, (const char **)params);
    }
    /* if client is not yet "connected" */
    if (!RELAY_IRC_DATA(client, connected))
    {
        if (irc_command && (weechat_strcasecmp (irc_command, "pass") == 0))
        {
            if (num_params > 0)
            {
                pos_password = params[0];
                if (!client->protocol_args)
                {
                    pos = strchr (pos_password, ':');
                    if (pos)
                    {
                        client->protocol_args = weechat_strndup (
                            pos_password, pos - pos_password);
                        relay_client_set_desc (client);
                        pos_password = pos + 1;
                    }
                }
                if (!RELAY_IRC_DATA(client, password_ok))
                {
                    password = weechat_string_eval_expression (
                        weechat_config_string (relay_config_network_password),
                        NULL, NULL, NULL);
                    if (password)
                    {
                        if (strcmp (password, pos_password) == 0)
                        {
                            RELAY_IRC_DATA(client, password_ok) = 1;
                            weechat_hook_signal_send ("relay_client_auth_ok",
                                                      WEECHAT_HOOK_SIGNAL_POINTER,
                                                      client);
                            relay_client_set_status (client, RELAY_STATUS_CONNECTED);
                        }
                        free (password);
                    }
                }
            }
        }
        if (irc_command && (weechat_strcasecmp (irc_command, "user") == 0))
        {
            /* check if server is known */
            if (!client->protocol_args)
            {
                relay_irc_sendf (client,
                                 ":%s ERROR :WeeChat: server not specified, "
                                 "command \"PASS server:password\" not received",
                                 RELAY_IRC_DATA(client, address));
                relay_irc_sendf (client,
                                 ":%s ERROR :Closing Link",
                                 RELAY_IRC_DATA(client, address));
                relay_client_set_status (client,
                                         RELAY_STATUS_DISCONNECTED);
                goto end;
            }

            /* check if connection to server is OK */
            infolist_server = weechat_infolist_get ("irc_server", NULL,
                                                    client->protocol_args);
            if (infolist_server)
            {
                if (weechat_infolist_next (infolist_server))
                {
                    if (!weechat_infolist_integer (infolist_server,
                                                   "is_connected"))
                    {
                        relay_irc_sendf (client,
                                         ":%s ERROR :WeeChat: no "
                                         "connection to server \"%s\"",
                                         RELAY_IRC_DATA(client, address),
                                         client->protocol_args);
                        relay_irc_sendf (client,
                                         ":%s ERROR :Closing Link",
                                         RELAY_IRC_DATA(client, address));
                        relay_client_set_status (client,
                                                 RELAY_STATUS_DISCONNECTED);
                        weechat_infolist_free (infolist_server);
                        goto end;
                    }
                    if (num_params > 0)
                    {
                        RELAY_IRC_DATA(client, user_received) = 1;
                    }
                }
                weechat_infolist_free (infolist_server);
            }
        }
        if (RELAY_IRC_DATA(client, nick)
            && RELAY_IRC_DATA(client, user_received)
            && (!RELAY_IRC_DATA(client, cap_ls_received)
                || RELAY_IRC_DATA(client, cap_end_received)))
        {
            /* disconnect client if password was not received or wrong */
            if (!RELAY_IRC_DATA(client, password_ok))
            {
                relay_irc_sendf (client,
                                 ":%s ERROR :WeeChat: password error",
                                 RELAY_IRC_DATA(client, address));
                relay_client_set_status (client,
                                         RELAY_STATUS_AUTH_FAILED);
                goto end;
            }

            RELAY_IRC_DATA(client, connected) = 1;
            RELAY_IRC_DATA(client, irc_cap_echo_message) = relay_irc_cap_enabled (
                client, "echo-message");

            /*
             * send nick to client if server nick is different of nick asked
             * by client with command NICK
             */
            nick = weechat_info_get ("irc_nick", client->protocol_args);
            if (nick && (strcmp (nick, RELAY_IRC_DATA(client, nick)) != 0))
            {
                relay_irc_sendf (client,
                                 ":%s!proxy NICK :%s",
                                 RELAY_IRC_DATA(client, nick),
                                 nick);
                free (RELAY_IRC_DATA(client, nick));
                RELAY_IRC_DATA(client, nick) = strdup (nick);
            }
            if (nick)
                free (nick);

            version = weechat_info_get ("version", NULL);
            relay_irc_sendf (client,
                             ":%s 001 %s :Welcome to the Internet "
                             "Relay Chat Network %s!%s@proxy",
                             RELAY_IRC_DATA(client, address),
                             RELAY_IRC_DATA(client, nick),
                             RELAY_IRC_DATA(client, nick),
                             "weechat");
            relay_irc_sendf (client,
                             ":%s 002 %s :Your host is "
                             "weechat-relay-irc, running version %s",
                             RELAY_IRC_DATA(client, address),
                             RELAY_IRC_DATA(client, nick),
                             version);
            snprintf (str_time, sizeof (str_time), "%s",
                      ctime (&client->listen_start_time));
            if (str_time[0])
                str_time[strlen (str_time) - 1] = '\0';
            relay_irc_sendf (client,
                             ":%s 003 %s :This server was created on %s",
                             RELAY_IRC_DATA(client, address),
                             RELAY_IRC_DATA(client, nick),
                             str_time);
            relay_irc_sendf (client,
                             ":%s 004 %s %s %s oirw abiklmnopqstv",
                             RELAY_IRC_DATA(client, address),
                             RELAY_IRC_DATA(client, nick),
                             RELAY_IRC_DATA(client, address),
                             version);
            if (version)
                free (version);
            infolist_server = weechat_infolist_get ("irc_server", NULL,
                                                    client->protocol_args);
            if (infolist_server)
            {
                if (weechat_infolist_next (infolist_server))
                {
                    isupport = weechat_infolist_string (infolist_server,
                                                        "isupport");
                    if (isupport && isupport[0])
                    {
                        while (isupport[0] == ' ')
                        {
                            isupport++;
                        }
                        relay_irc_sendf (client,
                                         ":%s 005 %s %s :are supported "
                                         "by this server",
                                         RELAY_IRC_DATA(client, address),
                                         RELAY_IRC_DATA(client, nick),
                                         isupport);
                    }
                }
                weechat_infolist_free (infolist_server);
            }
            relay_irc_sendf (client,
                             ":%s 251 %s :There are %d users and 0 "
                             "invisible on 1 servers",
                             RELAY_IRC_DATA(client, address),
                             RELAY_IRC_DATA(client, nick),
                             relay_client_count);
            relay_irc_sendf (client,
                             ":%s 255 %s :I have %d clients, 0 "
                             "services and 0 servers",
                             RELAY_IRC_DATA(client, address),
                             RELAY_IRC_DATA(client, nick),
                             relay_client_count);
            relay_irc_sendf (client,
                             ":%s 422 %s :MOTD File is missing",
                             RELAY_IRC_DATA(client, address),
                             RELAY_IRC_DATA(client, nick));

            /* hook signals */
            relay_irc_hook_signals (client);

            /* send JOIN for all channels on server to client */
            relay_irc_send_join_channels (client);
        }
    }
    else
    {
        if (irc_command && weechat_strcasecmp (irc_command, "join") == 0)
        {
            str_args = weechat_string_rebuild_split_string (
                (const char **)params, " ", 0, -1);
            relay_irc_input_send (client, NULL,
                                  "priority_high",
                                  "/join %s",
                                  (str_args) ? str_args : "");
            if (str_args)
                free (str_args);
        }
        else if (irc_command && weechat_strcasecmp (irc_command, "part") == 0)
        {
            str_args = weechat_string_rebuild_split_string (
                (const char **)params, " ", 0, -1);
            relay_irc_input_send (client, NULL,
                                  "priority_high",
                                  "/part %s",
                                  (str_args) ? str_args : "");
            if (str_args)
                free (str_args);
        }
        else if (irc_command && weechat_strcasecmp (irc_command, "ping") == 0)
        {
            str_args = weechat_string_rebuild_split_string (
                (const char **)params, " ", 0, -1);
            relay_irc_sendf (client,
                             ":%s PONG %s :%s",
                             RELAY_IRC_DATA(client, address),
                             RELAY_IRC_DATA(client, address),
                             (str_args) ? str_args : "");
            if (str_args)
                free (str_args);
        }
        else if (irc_command && (weechat_strcasecmp (irc_command, "notice") == 0))
        {
            if (num_params > 1)
            {
                str_args = weechat_string_rebuild_split_string (
                    (const char **)params, " ", 1, -1);
                relay_irc_input_send (client, NULL,
                                      "priority_high",
                                      "/notice %s %s",
                                      params[0],
                                      (str_args) ? str_args : "");
                if (str_args)
                    free (str_args);
            }
        }
        else if (irc_command && (weechat_strcasecmp (irc_command, "privmsg") == 0))
        {
            if (num_params > 1)
            {
                str_args = weechat_string_rebuild_split_string (
                    (const char **)params, " ", 1, -1);
                irc_is_channel = weechat_info_get ("irc_is_channel", params[0]);
                if (irc_is_channel && (strcmp (irc_is_channel, "1") == 0))
                {
                    relay_irc_input_send (client, params[0],
                                          "priority_high,user_message",
                                          "%s",
                                          (str_args) ? str_args : "");
                }
                else
                {
                    relay_irc_input_send (client, NULL,
                                          "priority_high",
                                          "/query %s %s",
                                          params[0],
                                          (str_args) ? str_args : "");
                }
                if (str_args)
                    free (str_args);
                if (irc_is_channel)
                    free (irc_is_channel);
            }
        }
        else if (!relay_irc_command_ignored (irc_command))
        {
            hash_redirect = weechat_hashtable_new (32,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   WEECHAT_HASHTABLE_STRING,
                                                   NULL, NULL);
            if (hash_redirect)
            {
                redirect_msg = 0;
                weechat_hashtable_set (hash_redirect, "server",
                                       client->protocol_args);
                snprintf (str_signal, sizeof (str_signal),
                          "relay_%d", client->id);
                weechat_hashtable_set (hash_redirect, "signal", str_signal);
                if (weechat_strcasecmp (irc_command, "mode") == 0)
                {
                    if (num_params > 0)
                    {
                        if (num_params == 1)
                            redirect_msg = 1;
                        weechat_hashtable_set (hash_redirect, "pattern",
                                               "mode_channel");
                        weechat_hashtable_set (hash_redirect, "string",
                                               params[0]);
                        snprintf (str_server_channel,
                                  sizeof (str_server_channel),
                                  "%s,%s",
                                  client->protocol_args,
                                  params[0]);
                        info = weechat_info_get ("irc_is_channel",
                                                 str_server_channel);
                        if (info && (strcmp (info, "1") == 0))
                        {
                            /* command "MODE #channel ..." */
                            if (num_params == 2)
                            {
                                if ((strcmp (params[1], "b") == 0)
                                    || (strcmp (params[1], "+b") == 0))
                                {
                                    redirect_msg = 1;
                                    weechat_hashtable_set (hash_redirect, "pattern",
                                                           "mode_channel_ban");
                                }
                                else if ((strcmp (params[1], "e") == 0)
                                         || (strcmp (params[1], "+e") == 0))
                                {
                                    redirect_msg = 1;
                                    weechat_hashtable_set (hash_redirect, "pattern",
                                                           "mode_channel_ban_exception");
                                }
                                else if ((strcmp (params[1], "I") == 0)
                                         || (strcmp (params[1], "+I") == 0))
                                {
                                    redirect_msg = 1;
                                    weechat_hashtable_set (hash_redirect, "pattern",
                                                           "mode_channel_invite");
                                }
                            }
                        }
                        else
                        {
                            /* command "MODE nick ..." */
                            if (num_params == 1)
                            {
                                redirect_msg = 1;
                                weechat_hashtable_set (hash_redirect, "pattern",
                                                       "mode_user");
                            }
                        }
                        if (info)
                            free (info);
                    }
                }
                else if (weechat_strcasecmp (irc_command, "ison") == 0)
                {
                    redirect_msg = 1;
                    weechat_hashtable_set (hash_redirect, "pattern", "ison");
                }
                else if (weechat_strcasecmp (irc_command, "list") == 0)
                {
                    redirect_msg = 1;
                    weechat_hashtable_set (hash_redirect, "pattern", "list");
                }
                else if ((weechat_strcasecmp (irc_command, "names") == 0)
                         || (weechat_strcasecmp (irc_command, "topic") == 0)
                         || (weechat_strcasecmp (irc_command, "who") == 0)
                         || (weechat_strcasecmp (irc_command, "whois") == 0)
                         || (weechat_strcasecmp (irc_command, "whowas") == 0))
                {
                    if (num_params > 0)
                    {
                        redirect_msg = 1;
                        snprintf (str_command, sizeof (str_command),
                                  "%s", irc_command);
                        str_cmd_lower = weechat_string_tolower (str_command);
                        if (str_cmd_lower)
                        {
                            weechat_hashtable_set (hash_redirect, "pattern",
                                                   str_cmd_lower);
                            weechat_hashtable_set (hash_redirect, "string",
                                                   params[0]);
                            free (str_cmd_lower);
                        }
                    }
                }
                else if (weechat_strcasecmp (irc_command, "time") == 0)
                {
                    redirect_msg = 1;
                    weechat_hashtable_set (hash_redirect, "pattern", "time");
                }
                else if (weechat_strcasecmp (irc_command, "userhost") == 0)
                {
                    redirect_msg = 1;
                    weechat_hashtable_set (hash_redirect, "pattern", "userhost");
                }
                /*
                 * if redirection has been enabled, send the hsignal for
                 * redirection of IRC message
                 */
                if (redirect_msg)
                {
                    (void) weechat_hook_hsignal_send ("irc_redirect_command",
                                                      hash_redirect);
                }
                weechat_hashtable_free (hash_redirect);
            }
            /* send the IRC message to server */
            relay_irc_input_send (client, NULL,
                                  "priority_high",
                                  "/quote %s",
                                  ptr_data);
        }
    }

end:
    if (new_data)
        free (new_data);
    if (hash_parsed)
        weechat_hashtable_free (hash_parsed);
    if (params)
        free (params);
}

/*
 * Closes connection with client.
 */

void
relay_irc_close_connection (struct t_relay_client *client)
{
    RELAY_IRC_DATA(client, connected) = 0;

    if (RELAY_IRC_DATA(client, hook_signal_irc_in2))
    {
        weechat_unhook (RELAY_IRC_DATA(client, hook_signal_irc_in2));
        RELAY_IRC_DATA(client, hook_signal_irc_in2) = NULL;
    }
    if (RELAY_IRC_DATA(client, hook_signal_irc_outtags))
    {
        weechat_unhook (RELAY_IRC_DATA(client, hook_signal_irc_outtags));
        RELAY_IRC_DATA(client, hook_signal_irc_outtags) = NULL;
    }
    if (RELAY_IRC_DATA(client, hook_signal_irc_disc))
    {
        weechat_unhook (RELAY_IRC_DATA(client, hook_signal_irc_disc));
        RELAY_IRC_DATA(client, hook_signal_irc_disc) = NULL;
    }
    if (RELAY_IRC_DATA(client, hook_hsignal_irc_redir))
    {
        weechat_unhook (RELAY_IRC_DATA(client, hook_hsignal_irc_redir));
        RELAY_IRC_DATA(client, hook_hsignal_irc_redir) = NULL;
    }
}

/*
 * Initializes relay data specific to IRC protocol.
 */

void
relay_irc_alloc (struct t_relay_client *client)
{
    char *password;

    password = weechat_string_eval_expression (weechat_config_string (relay_config_network_password),
                                               NULL, NULL, NULL);

    client->protocol_data = malloc (sizeof (struct t_relay_irc_data));
    if (client->protocol_data)
    {
        RELAY_IRC_DATA(client, address) = strdup ("weechat.relay.irc");
        RELAY_IRC_DATA(client, password_ok) = (password && password[0]) ? 0 : 1;
        RELAY_IRC_DATA(client, nick) = NULL;
        RELAY_IRC_DATA(client, user_received) = 0;
        RELAY_IRC_DATA(client, cap_ls_received) = 0;
        RELAY_IRC_DATA(client, cap_end_received) = 0;
        RELAY_IRC_DATA(client, connected) = 0;
        RELAY_IRC_DATA(client, irc_cap_echo_message) = 0;
        RELAY_IRC_DATA(client, server_capabilities) = 0;
        RELAY_IRC_DATA(client, hook_signal_irc_in2) = NULL;
        RELAY_IRC_DATA(client, hook_signal_irc_outtags) = NULL;
        RELAY_IRC_DATA(client, hook_signal_irc_disc) = NULL;
        RELAY_IRC_DATA(client, hook_hsignal_irc_redir) = NULL;
    }

    if (password)
        free (password);
}

/*
 * Initializes relay data specific to IRC protocol using an infolist.
 *
 * This is called after /upgrade.
 */

void
relay_irc_alloc_with_infolist (struct t_relay_client *client,
                               struct t_infolist *infolist)
{
    client->protocol_data = malloc (sizeof (struct t_relay_irc_data));
    if (client->protocol_data)
    {
        RELAY_IRC_DATA(client, address) = strdup (weechat_infolist_string (infolist, "address"));
        RELAY_IRC_DATA(client, password_ok) = weechat_infolist_integer (infolist, "password_ok");
        if (weechat_infolist_string (infolist, "nick"))
            RELAY_IRC_DATA(client, nick) = strdup (weechat_infolist_string (infolist, "nick"));
        else
            RELAY_IRC_DATA(client, nick) = NULL;
        RELAY_IRC_DATA(client, user_received) = weechat_infolist_integer (infolist, "user_received");
        RELAY_IRC_DATA(client, cap_ls_received) = weechat_infolist_integer (infolist, "cap_ls_received");
        RELAY_IRC_DATA(client, cap_end_received) = weechat_infolist_integer (infolist, "cap_end_received");
        RELAY_IRC_DATA(client, connected) = weechat_infolist_integer (infolist, "connected");
        RELAY_IRC_DATA(client, irc_cap_echo_message) = weechat_infolist_integer (infolist, "irc_cap_echo_message");
        RELAY_IRC_DATA(client, server_capabilities) = weechat_infolist_integer (infolist, "server_capabilities");
        if (RELAY_IRC_DATA(client, connected))
        {
            relay_irc_hook_signals (client);
        }
        else
        {
            RELAY_IRC_DATA(client, hook_signal_irc_in2) = NULL;
            RELAY_IRC_DATA(client, hook_signal_irc_outtags) = NULL;
            RELAY_IRC_DATA(client, hook_signal_irc_disc) = NULL;
            RELAY_IRC_DATA(client, hook_hsignal_irc_redir) = NULL;
        }
    }
}

/*
 * Returns the client initial status: it can be "waiting_auth" or "connected",
 * depending if a password is expected or not.
 */

enum t_relay_status
relay_irc_get_initial_status (struct t_relay_client *client)
{
    return (RELAY_IRC_DATA(client, password_ok)) ?
        RELAY_STATUS_CONNECTED : RELAY_STATUS_WAITING_AUTH;
}

/*
 * Frees relay data specific to IRC protocol.
 */

void
relay_irc_free (struct t_relay_client *client)
{
    if (!client)
        return;

    if (client->protocol_data)
    {
        if (RELAY_IRC_DATA(client, address))
            free (RELAY_IRC_DATA(client, address));
        if (RELAY_IRC_DATA(client, nick))
            free (RELAY_IRC_DATA(client, nick));
        if (RELAY_IRC_DATA(client, hook_signal_irc_in2))
            weechat_unhook (RELAY_IRC_DATA(client, hook_signal_irc_in2));
        if (RELAY_IRC_DATA(client, hook_signal_irc_outtags))
            weechat_unhook (RELAY_IRC_DATA(client, hook_signal_irc_outtags));
        if (RELAY_IRC_DATA(client, hook_signal_irc_disc))
            weechat_unhook (RELAY_IRC_DATA(client, hook_signal_irc_disc));
        if (RELAY_IRC_DATA(client, hook_hsignal_irc_redir))
            weechat_unhook (RELAY_IRC_DATA(client, hook_hsignal_irc_redir));

        free (client->protocol_data);

        client->protocol_data = NULL;
    }
}

/*
 * Adds client IRC data in an infolist.
 *
 * If force_disconnected_state == 1, the infolist contains the client
 * in a disconnected state (but the client is unchanged, still connected if it
 * was).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_irc_add_to_infolist (struct t_infolist_item *item,
                           struct t_relay_client *client,
                           int force_disconnected_state)
{
    if (!item || !client)
        return 0;

    if (!RELAY_CLIENT_HAS_ENDED(client) && force_disconnected_state)
    {
        if (!weechat_infolist_new_var_integer (item, "connected", 0))
            return 0;
    }
    else
    {
        if (!weechat_infolist_new_var_integer (item, "connected", RELAY_IRC_DATA(client, connected)))
            return 0;
    }
    if (!weechat_infolist_new_var_string (item, "address", RELAY_IRC_DATA(client, address)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "password_ok", RELAY_IRC_DATA(client, password_ok)))
        return 0;
    if (!weechat_infolist_new_var_string (item, "nick", RELAY_IRC_DATA(client, nick)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "user_received", RELAY_IRC_DATA(client, user_received)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "cap_ls_received", RELAY_IRC_DATA(client, cap_ls_received)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "cap_end_received", RELAY_IRC_DATA(client, cap_end_received)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "irc_cap_echo_message", RELAY_IRC_DATA(client, irc_cap_echo_message)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "server_capabilities", RELAY_IRC_DATA(client, server_capabilities)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_signal_irc_in2", RELAY_IRC_DATA(client, hook_signal_irc_in2)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_signal_irc_outtags", RELAY_IRC_DATA(client, hook_signal_irc_outtags)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_signal_irc_disc", RELAY_IRC_DATA(client, hook_signal_irc_disc)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_hsignal_irc_redir", RELAY_IRC_DATA(client, hook_hsignal_irc_redir)))
        return 0;

    return 1;
}

/*
 * Prints client IRC data in WeeChat log file (usually for crash dump).
 */

void
relay_irc_print_log (struct t_relay_client *client)
{
    if (client->protocol_data)
    {
        weechat_log_printf ("    address . . . . . . . . : '%s'",  RELAY_IRC_DATA(client, address));
        weechat_log_printf ("    password_ok . . . . . . : %d",    RELAY_IRC_DATA(client, password_ok));
        weechat_log_printf ("    nick. . . . . . . . . . : '%s'",  RELAY_IRC_DATA(client, nick));
        weechat_log_printf ("    user_received . . . . . : %d",    RELAY_IRC_DATA(client, user_received));
        weechat_log_printf ("    cap_ls_received . . . . : %d",    RELAY_IRC_DATA(client, cap_ls_received));
        weechat_log_printf ("    cap_end_received. . . . : %d",    RELAY_IRC_DATA(client, cap_end_received));
        weechat_log_printf ("    connected . . . . . . . : %d",    RELAY_IRC_DATA(client, connected));
        weechat_log_printf ("    irc_cap_echo_message. . : %d",    RELAY_IRC_DATA(client, irc_cap_echo_message));
        weechat_log_printf ("    server_capabilities . . : %d",    RELAY_IRC_DATA(client, server_capabilities));
        weechat_log_printf ("    hook_signal_irc_in2 . . : 0x%lx", RELAY_IRC_DATA(client, hook_signal_irc_in2));
        weechat_log_printf ("    hook_signal_irc_outtags : 0x%lx", RELAY_IRC_DATA(client, hook_signal_irc_outtags));
        weechat_log_printf ("    hook_signal_irc_disc. . : 0x%lx", RELAY_IRC_DATA(client, hook_signal_irc_disc));
        weechat_log_printf ("    hook_hsignal_irc_redir. : 0x%lx", RELAY_IRC_DATA(client, hook_hsignal_irc_redir));
    }
}
