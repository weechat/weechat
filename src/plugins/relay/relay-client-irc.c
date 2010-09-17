/*
 * Copyright (C) 2003-2010 Sebastien Helleu <flashcode@flashtux.org>
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
 * relay-client-irc.c: IRC protocol for relay to client
 *                     (relay acting as an IRC proxy/bouncer)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-client-irc.h"
#include "relay-client.h"
#include "relay-raw.h"


char *relay_client_irc_relay_commands[] = { "privmsg", "notice", NULL };
char *relay_client_irc_ignore_commands[] = { "pong", "quit", NULL };


/*
 * relay_client_irc_command_relayed: return 1 if IRC command has to be
 *                                   relayed to client, or 0 if command
 *                                   must NOT be relayed
 */

int
relay_client_irc_command_relayed (const char *irc_command)
{
    int i;
    
    if (irc_command)
    {
        for (i = 0; relay_client_irc_relay_commands[i]; i++)
        {
            if (weechat_strcasecmp (relay_client_irc_relay_commands[i], irc_command) == 0)
                return 1;
        }
    }
    
    /* command must NOT be relayed to client */
    return 0;
}

/*
 * relay_client_irc_command_ignored: return 1 if IRC command from client
 *                                   has to be ignored
 */

int
relay_client_irc_command_ignored (const char *irc_command)
{
    int i;
    
    if (irc_command)
    {
        for (i = 0; relay_client_irc_ignore_commands[i]; i++)
        {
            if (weechat_strcasecmp (relay_client_irc_ignore_commands[i], irc_command) == 0)
                return 1;
        }
    }
    
    /* command must NOT be relayed to client */
    return 0;
}

/*
 * relay_client_irc_parse_message: parse IRC message
 */

struct t_hashtable *
relay_client_irc_parse_message (const char *message)
{
    struct t_hashtable *hash_msg, *hash_parsed;
    
    hash_msg = NULL;
    hash_parsed = NULL;
    
    hash_msg = weechat_hashtable_new (8,
                                      WEECHAT_HASHTABLE_STRING,
                                      WEECHAT_HASHTABLE_STRING,
                                      NULL,
                                      NULL);
    if (!hash_msg)
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory for parsing message"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        goto end;
    }
    weechat_hashtable_set (hash_msg, "message", (char *)message);
    hash_parsed = weechat_info_get_hashtable ("irc_parse_message",
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
 * relay_client_irc_sendf: send formatted data to client
 */

int
relay_client_irc_sendf (struct t_relay_client *client, const char *format, ...)
{
    va_list args;
    static char buffer[4096];
    int length, num_sent;
    char *pos;
    
    if (!client)
        return 0;
    
    va_start (args, format);
    vsnprintf (buffer, sizeof (buffer) - 3, format, args);
    va_end (args);
    
    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: send: %s",
                        RELAY_PLUGIN_NAME, buffer);
    }
    
    length = strlen (buffer);    
    
    pos = strchr (buffer, '\r');
    if (pos)
        pos[0] = '\0';
    
    relay_raw_print (client, 1, buffer);
    
    if (pos)
        pos[0] = '\r';
    else
    {
        buffer[length] = '\r';
        buffer[length + 1] = '\n';
        buffer[length + 2] = '\0';
        length += 2;
    }
    
    num_sent = send (client->sock, buffer, length, 0);
    
    if (num_sent >= 0)
        client->bytes_sent += num_sent;
    else
    {
        weechat_printf (NULL,
                        _("%s%s: error sending data to client: %s"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                        strerror (errno));
    }
    
    return num_sent;
}

/*
 * relay_client_irc_signal_irc_in2_cb: callback for "irc_in2" signal
 *                                     It is called when something is
 *                                     received on IRC server, and message
 *                                     can be relayed (or not) to client.
 */

int
relay_client_irc_signal_irc_in2_cb (void *data, const char *signal,
                                    const char *type_data, void *signal_data)
{
    struct t_relay_client *client;
    const char *ptr_msg, *irc_host, *irc_command, *irc_args;
    struct t_hashtable *hash_parsed;
    
    /* make C compiler happy */
    (void) signal;
    (void) type_data;
    
    client = (struct t_relay_client *)data;
    ptr_msg = (const char *)signal_data;
    
    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: irc_in2: client: %s, data: %s",
                        RELAY_PLUGIN_NAME,
                        client->protocol_string,
                        ptr_msg);
    }
    
    hash_parsed = relay_client_irc_parse_message (ptr_msg);
    if (hash_parsed)
    {
        irc_host = weechat_hashtable_get (hash_parsed, "host");
        irc_command = weechat_hashtable_get (hash_parsed, "command");
        irc_args = weechat_hashtable_get (hash_parsed, "arguments");

        /* if self nick has changed, update it in client data */
        if (irc_command && (weechat_strcasecmp (irc_command, "nick") == 0))
        {
            if (irc_args && irc_args[0])
            {
                if (RELAY_IRC_DATA(client, nick))
                    free (RELAY_IRC_DATA(client, nick));
                RELAY_IRC_DATA(client, nick) = strdup (irc_args);
            }
        }
        
        /* relay all commands to client, but not ping/pong */
        if (irc_command
            && (weechat_strcasecmp (irc_command, "ping") != 0)
            && (weechat_strcasecmp (irc_command, "pong") != 0))
        {
            relay_client_irc_sendf (client, ":%s %s %s",
                                    (irc_host && irc_host[0]) ? irc_host : RELAY_IRC_DATA(client, address),
                                    irc_command,
                                    irc_args);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * relay_client_irc_tag_relay_client_id: get id of client by looking for tag
 *                                       "relay_client_NNN" in list of tags
 *                                       (comma separated list)
 *                                       Return number found, or -1 if tag
 *                                       is not found.
 */

int
relay_client_irc_tag_relay_client_id (const char *tags)
{
    char **argv, *error;
    int result, argc, i;
    long number;
    
    result = -1;
    
    if (tags && tags[0])
    {
        argv = weechat_string_split (tags, ",", 0, 0, &argc);
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
    }
    
    return result;
}

/*
 * relay_client_irc_signal_irc_outtags_cb: callback for "irc_out" signal
 *                                         It is called when a message is sent
 *                                         to IRC server (by irc plugin or any
 *                                         other plugin/script).
 */

int
relay_client_irc_signal_irc_outtags_cb (void *data, const char *signal,
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
    (void) signal;
    (void) type_data;
    
    client = (struct t_relay_client *)data;
    
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
        weechat_printf (NULL, "%s: irc_out: client: %s, message: %s",
                        RELAY_PLUGIN_NAME,
                        client->protocol_string,
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
    if (relay_client_irc_tag_relay_client_id (tags) == client->id)
        goto end;
    
    hash_parsed = relay_client_irc_parse_message (ptr_message);
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
            && relay_client_irc_command_relayed (irc_command))
        {
            /* get host for nick (it is self nick) */
            snprintf (str_infolist_args, sizeof (str_infolist_args) - 1,
                      "%s,%s,%s",
                      client->protocol_string,
                      irc_channel,
                      RELAY_IRC_DATA(client, nick));
            
            host = NULL;
            infolist_nick = weechat_infolist_get ("irc_nick", NULL, str_infolist_args);
            if (infolist_nick && weechat_infolist_next (infolist_nick))
                host = weechat_infolist_string (infolist_nick, "host");
            
            /* send message to client */
            relay_client_irc_sendf (client,
                                    ":%s%s%s %s",
                                    RELAY_IRC_DATA(client, nick),
                                    (host && host[0]) ? "!" : "",
                                    (host && host[0]) ? host : "",
                                    ptr_message);
            
            if (infolist_nick)
                weechat_infolist_free (infolist_nick);
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
 * relay_client_irc_signal_irc_disc_cb: callback for "irc_disconnected" signal
 *                                      It is called when connection to a
 *                                      server is lost.
 */

int
relay_client_irc_signal_irc_disc_cb (void *data, const char *signal,
                                     const char *type_data, void *signal_data)
{
    struct t_relay_client *client;
    
    /* make C compiler happy */
    (void) signal;
    (void) type_data;
    (void) signal_data;
    
    client = (struct t_relay_client *)data;
    
    if (strcmp ((char *)signal_data, client->protocol_string) == 0)
    {
        relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * relay_client_irc_send_join: send join for a channel to client
 */

void
relay_client_irc_send_join (struct t_relay_client *client,
                            const char *channel)
{
    char *infolist_name, *nicks;
    const char *nick, *prefix;
    char *host;
    int length, length_nicks;
    struct t_infolist *infolist_nick, *infolist_nicks;
    
    length = strlen (client->protocol_string) + 1 + strlen (channel) + 1
        + strlen (RELAY_IRC_DATA(client, nick)) + 1;
    infolist_name = malloc (length);
    if (infolist_name)
    {
        /* get nick host */
        host = NULL;
        snprintf (infolist_name, length, "%s,%s,%s",
                  client->protocol_string,
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
        relay_client_irc_sendf (client,
                                ":%s!%s JOIN %s",
                                RELAY_IRC_DATA(client, nick),
                                (host && host[0]) ? host : "weechat@proxy",
                                channel);
        if (host)
            free (host);
        snprintf (infolist_name, length, "%s,%s",
                  client->protocol_string,
                  channel);
        infolist_nicks = weechat_infolist_get ("irc_nick", NULL, infolist_name);
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
                        nicks = realloc (nicks, length_nicks);
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
                relay_client_irc_sendf (client,
                                        ":%s 353 %s = %s :%s",
                                        RELAY_IRC_DATA(client, address),
                                        RELAY_IRC_DATA(client, nick),
                                        channel, nicks);
                free (nicks);
            }
            weechat_infolist_free (infolist_nicks);
        }
        relay_client_irc_sendf (client,
                                ":%s 366 %s %s :End of /NAMES list.",
                                RELAY_IRC_DATA(client, address),
                                RELAY_IRC_DATA(client, nick),
                                channel);
        free (infolist_name);
    }
}

/*
 * relay_client_irc_send_join_channels: send join for all channels of server to
 *                                      client
 */

void
relay_client_irc_send_join_channels (struct t_relay_client *client)
{
    struct t_infolist *infolist_channels;
    const char *channel;

    infolist_channels = weechat_infolist_get ("irc_channel", NULL,
                                              client->protocol_string);
    if (infolist_channels)
    {
        while (weechat_infolist_next (infolist_channels))
        {
            channel = weechat_infolist_string (infolist_channels, "name");
            relay_client_irc_send_join (client, channel);
        }
        weechat_infolist_free (infolist_channels);
    }
}

/*
 * relay_client_irc_input_send: send text or command on an IRC buffer
 */

void
relay_client_irc_input_send (struct t_relay_client *client,
                             const char *irc_channel,
                             int flags,
                             const char *format, ...)
{
    va_list args;
    static char buffer[4096];
    int length;
    
    snprintf (buffer, sizeof (buffer),
              "%s;%s;%d;relay_client_%d;",
              client->protocol_string,
              (irc_channel) ? irc_channel : "",
              flags,
              client->id);
    
    length = strlen (buffer);
    
    va_start (args, format);
    vsnprintf (buffer + length, sizeof (buffer) - 1 - length, format, args);
    va_end (args);
    
    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL,
                        "%s: irc_input_send: \"%s\"",
                        RELAY_PLUGIN_NAME, buffer);
    }
    
    weechat_hook_signal_send ("irc_input_send",
                              WEECHAT_HOOK_SIGNAL_STRING,
                              buffer);
}

/*
 * relay_client_irc_recv_one_msg: read one message from client
 */

void
relay_client_irc_recv_one_msg (struct t_relay_client *client, char *data)
{
    char *pos, str_time[128], str_signal_name[128], *target;
    const char *irc_command, *irc_channel, *irc_args, *irc_args2;
    const char *nick, *irc_is_channel, *isupport;
    struct t_hashtable *hash_parsed;
    struct t_infolist *infolist_server;
    
    hash_parsed = NULL;
    
    /* remove \r at the end of message */
    pos = strchr (data, '\r');
    if (pos)
        pos[0] = '\0';
    
    /* display debug message */
    if (weechat_relay_plugin->debug >= 2)
    {
        weechat_printf (NULL, "%s: recv from client: \"%s\"",
                        RELAY_PLUGIN_NAME, data);
    }
    
    relay_raw_print (client, 0, data);
    
    /* parse IRC message */
    hash_parsed = relay_client_irc_parse_message (data);
    if (!hash_parsed)
        goto end;
    irc_command = weechat_hashtable_get (hash_parsed, "command");
    irc_channel = weechat_hashtable_get (hash_parsed, "channel");
    irc_args = weechat_hashtable_get (hash_parsed, "arguments");
    
    /* process the message */
    if (irc_command && (weechat_strcasecmp (irc_command, "nick") == 0))
    {
        if (irc_args && irc_args[0])
        {
            if (RELAY_IRC_DATA(client, nick))
                free (RELAY_IRC_DATA(client, nick));
            RELAY_IRC_DATA(client, nick) = strdup (irc_args);
        }
    }
    if (!RELAY_IRC_DATA(client, connected))
    {
        if (irc_command && (weechat_strcasecmp (irc_command, "user") == 0))
        {
            /* check if connection to server is ok */
            infolist_server = weechat_infolist_get ("irc_server", NULL,
                                                    client->protocol_string);
            if (infolist_server)
            {
                if (weechat_infolist_next (infolist_server))
                {
                    if (!weechat_infolist_integer (infolist_server,
                                                   "is_connected"))
                    {
                        relay_client_irc_sendf (client,
                                                ":%s ERROR :WeeChat: no "
                                                "connection to server \"%s\"",
                                                RELAY_IRC_DATA(client, address),
                                                client->protocol_string);
                        relay_client_irc_sendf (client,
                                                ":%s ERROR :Closing Link",
                                                RELAY_IRC_DATA(client, address));
                        relay_client_set_status (client,
                                                 RELAY_STATUS_DISCONNECTED);
                        return;
                    }
                    if (irc_args && irc_args[0])
                    {
                        RELAY_IRC_DATA(client, user_received) = 1;
                    }
                }
                weechat_infolist_free (infolist_server);
            }
        }
        if (RELAY_IRC_DATA(client, nick) && RELAY_IRC_DATA(client, user_received))
        {
            RELAY_IRC_DATA(client, connected) = 1;
            
            /*
             * send nick to client if server nick is different of nick asked
             * by client with command NICK
             */
            nick = weechat_info_get ("irc_nick", client->protocol_string);
            if (nick && (strcmp (nick, RELAY_IRC_DATA(client, nick)) != 0))
            {
                relay_client_irc_sendf (client,
                                        ":%s!proxy NICK :%s",
                                        RELAY_IRC_DATA(client, nick),
                                        nick);
                free (RELAY_IRC_DATA(client, nick));
                RELAY_IRC_DATA(client, nick) = strdup (nick);
            }
            
            relay_client_irc_sendf (client,
                                    ":%s 001 %s :Welcome to the Internet "
                                    "Relay Chat Network %s!%s@proxy",
                                    RELAY_IRC_DATA(client, address),
                                    RELAY_IRC_DATA(client, nick),
                                    RELAY_IRC_DATA(client, nick),
                                    "weechat");
            relay_client_irc_sendf (client,
                                    ":%s 002 %s :Your host is "
                                    "weechat-relay-irc, running version %s",
                                    RELAY_IRC_DATA(client, address),
                                    RELAY_IRC_DATA(client, nick),
                                    weechat_info_get("version", NULL));
            snprintf (str_time, sizeof (str_time), "%s",
                      ctime (&client->listen_start_time));
            if (str_time[0])
                str_time[strlen (str_time) - 1] = '\0';
            relay_client_irc_sendf (client,
                                    ":%s 003 %s :This server was created on %s",
                                    RELAY_IRC_DATA(client, address),
                                    RELAY_IRC_DATA(client, nick),
                                    str_time);
            relay_client_irc_sendf (client,
                                    ":%s 004 %s %s %s oirw abiklmnopqstv",
                                    RELAY_IRC_DATA(client, address),
                                    RELAY_IRC_DATA(client, nick),
                                    RELAY_IRC_DATA(client, address),
                                    weechat_info_get("version", NULL));
            infolist_server = weechat_infolist_get ("irc_server", NULL,
                                                    client->protocol_string);
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
                        /* TODO: split this message into many messages */
                        relay_client_irc_sendf (client,
                                                ":%s 005 %s %s :are supported "
                                                "by this server",
                                                RELAY_IRC_DATA(client, address),
                                                RELAY_IRC_DATA(client, nick),
                                                isupport);
                    }
                }
                weechat_infolist_free (infolist_server);
            }
            relay_client_irc_sendf (client,
                                    ":%s 251 %s :There are %d users and 0 "
                                    "invisible on 1 servers",
                                    RELAY_IRC_DATA(client, address),
                                    RELAY_IRC_DATA(client, nick),
                                    relay_client_count);
            relay_client_irc_sendf (client,
                                    ":%s 255 %s :I have %d clients, 0 "
                                    "services and 0 servers",
                                    RELAY_IRC_DATA(client, address),
                                    RELAY_IRC_DATA(client, nick),
                                    relay_client_count);
            relay_client_irc_sendf (client,
                                    ":%s 422 %s :MOTD File is missing",
                                    RELAY_IRC_DATA(client, address),
                                    RELAY_IRC_DATA(client, nick));
            
            /*
             * hook signal "xxx,irc_in2_*" to catch IRC data received from
             * this server
             */
            snprintf (str_signal_name, sizeof (str_signal_name),
                      "%s,irc_in2_*",
                      client->protocol_string);
            RELAY_IRC_DATA(client, hook_signal_irc_in2) =
                weechat_hook_signal (str_signal_name,
                                     &relay_client_irc_signal_irc_in2_cb,
                                     client);

            /*
             * hook signal "xxx,irc_outtags_*" to catch IRC data sent to
             * this server
             */
            snprintf (str_signal_name, sizeof (str_signal_name),
                      "%s,irc_outtags_*",
                      client->protocol_string);
            RELAY_IRC_DATA(client, hook_signal_irc_outtags) =
                weechat_hook_signal (str_signal_name,
                                     &relay_client_irc_signal_irc_outtags_cb,
                                     client);
            
            /*
             * hook signal "irc_server_disconnected" to disconnect client if
             * connection to server is lost
             */
            RELAY_IRC_DATA(client, hook_signal_irc_disc) =
                weechat_hook_signal ("irc_server_disconnected",
                                     &relay_client_irc_signal_irc_disc_cb,
                                     client);
            
            /* send JOIN for all channels on server to client */
            relay_client_irc_send_join_channels (client);
        }
    }
    else
    {
        if (irc_command && weechat_strcasecmp (irc_command, "ping") == 0)
        {
            relay_client_irc_sendf (client,
                                    ":%s PONG %s :%s",
                                    RELAY_IRC_DATA(client, address),
                                    RELAY_IRC_DATA(client, address),
                                    irc_args);
        }
        else if (irc_command && irc_channel && irc_channel[0]
                 && irc_args && irc_args[0]
                 && (weechat_strcasecmp (irc_command, "notice") == 0))
        {
            irc_args2 = strchr (irc_args, ' ');
            if (irc_args2)
            {
                target = weechat_strndup (irc_args, irc_args2 - irc_args);
                if (target)
                {
                    while (irc_args2[0] == ' ')
                    {
                        irc_args2++;
                    }
                    if (irc_args2[0] == ':')
                        irc_args2++;
                    relay_client_irc_input_send (client, NULL, 1,
                                                 "/notice %s %s",
                                                 target,
                                                 irc_args2);
                    free (target);
                }
            }
        }
        else if (irc_command && irc_channel && irc_channel[0]
                 && irc_args && irc_args[0]
                 && (weechat_strcasecmp (irc_command, "privmsg") == 0))
        {
            irc_args2 = strchr (irc_args, ' ');
            if (!irc_args2)
                irc_args2 = irc_args;
            while (irc_args2[0] == ' ')
            {
                irc_args2++;
            }
            if (irc_args2[0] == ':')
                irc_args2++;
            irc_is_channel = weechat_info_get ("irc_is_channel", irc_channel);
            if (irc_is_channel && (strcmp (irc_is_channel, "1") == 0))
            {
                relay_client_irc_input_send (client, irc_channel, 1,
                                             "%s", irc_args2);
            }
            else
            {
                relay_client_irc_input_send (client, NULL, 1,
                                             "/query %s %s",
                                             irc_channel, irc_args2);
            }
        }
        else if (!relay_client_irc_command_ignored (irc_command))
        {
            relay_client_irc_input_send (client, NULL, 1,
                                         "/quote %s",
                                         data);
        }
    }
    
end:
    if (hash_parsed)
        weechat_hashtable_free (hash_parsed);
}

/*
 * relay_client_irc_recv: read data from client
 */

void
relay_client_irc_recv (struct t_relay_client *client, const char *data)
{
    char **items;
    int items_count, i;
    
    items = weechat_string_split (data, "\n", 0, 0, &items_count);
    for (i = 0; i < items_count; i++)
    {
        relay_client_irc_recv_one_msg (client, items[i]);
    }
    if (items)
        weechat_string_free_split (items);
}

/*
 * relay_client_irc_close_connection: called when connection with client is
 *                                    closed
 */

void
relay_client_irc_close_connection (struct t_relay_client *client)
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
}

/*
 * relay_client_irc_alloc: init relay data specific to IRC protocol
 */

void
relay_client_irc_alloc (struct t_relay_client *client)
{
    struct t_relay_client_irc_data *irc_data;
    
    client->protocol_data = malloc (sizeof (*irc_data));
    if (client->protocol_data)
    {
        RELAY_IRC_DATA(client, address) = strdup ("weechat.relay.irc");
        RELAY_IRC_DATA(client, nick) = NULL;
        RELAY_IRC_DATA(client, user_received) = 0;
        RELAY_IRC_DATA(client, connected) = 0;
        RELAY_IRC_DATA(client, hook_signal_irc_in2) = NULL;
        RELAY_IRC_DATA(client, hook_signal_irc_outtags) = NULL;
        RELAY_IRC_DATA(client, hook_signal_irc_disc) = NULL;
    }
}

/*
 * relay_client_irc_free: free relay data specific to IRC protocol
 */

void
relay_client_irc_free (struct t_relay_client *client)
{
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
        
        free (client->protocol_data);
        
        client->protocol_data = NULL;
    }
}

/*
 * relay_client_irc_print_log: print IRC client infos in log (usually for
 *                             crash dump)
 */

void
relay_client_irc_print_log (struct t_relay_client *client)
{
    if (client->protocol_data)
    {
        weechat_log_printf ("    address. . . . . . . . : '%s'",  RELAY_IRC_DATA(client, address));
        weechat_log_printf ("    nick . . . . . . . . . : '%s'",  RELAY_IRC_DATA(client, nick));
        weechat_log_printf ("    user_received. . . . . : %d",    RELAY_IRC_DATA(client, user_received));
        weechat_log_printf ("    connected. . . . . . . : %d",    RELAY_IRC_DATA(client, connected));
        weechat_log_printf ("    hook_signal_irc_in2. . : 0x%lx", RELAY_IRC_DATA(client, hook_signal_irc_in2));
        weechat_log_printf ("    hook_signal_irc_outtags: 0x%lx", RELAY_IRC_DATA(client, hook_signal_irc_outtags));
        weechat_log_printf ("    hook_signal_irc_disc . : 0x%lx", RELAY_IRC_DATA(client, hook_signal_irc_disc));
    }
}
